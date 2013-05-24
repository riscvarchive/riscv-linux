#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/module.h>

struct htifbd_dev {
	int size;			/* size, in 512b sectors */
	int dev_id;			/* HTIF device number */
	struct gendisk *gd;		/* gendisk */
	spinlock_t lock;
};

struct htifbd_htif_request {
	unsigned long address;
	unsigned long offset;		/* offset, in bytes */
	unsigned long size;		/* size, in bytes */
	unsigned long tag;
};

static struct request_queue *htifbd_queue;
static spinlock_t htifbd_lock;

static int htifbd_open(struct block_device *bd, fmode_t mode)
{
	return 0;
}

static int htifbd_release(struct gendisk *gd, fmode_t mode)
{
	return 0;
}

static int htifbd_ioctl(struct block_device *bd, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	unsigned long size;
	struct hd_geometry geo;
	struct htifbd_dev *dev = bd->bd_disk->private_data;
	
	switch (cmd) {
	case HDIO_GETGEO:
		size = dev->size * 512;
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		if (copy_to_user((void __user *)arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY;
}

static const struct block_device_operations htifbd_ops = {
	.owner = THIS_MODULE,
	.open = htifbd_open,
	.release = htifbd_release,
	.ioctl = htifbd_ioctl,
};



static unsigned long htifbd_transfer(struct htifbd_dev *dev, unsigned long sector,
	unsigned long nsect, char *buf, int direction)
{
	unsigned long offset = sector * 512;
	unsigned long size = nsect * 512;
	volatile struct htifbd_htif_request r;
	unsigned long request_pa;
	unsigned long packet;
	unsigned long device_id;
	unsigned long fromhost;

	if ((offset + size) > dev->size * 512) {
		printk(KERN_NOTICE "beyond the end, offset=%lx size=%ld\n", offset, size);
		return 0;
	}

	r.address = (unsigned long)__pa(buf);
	r.offset = offset;
	r.size = size;
	r.tag = 0;

	request_pa = (unsigned long)__pa(&r);

	device_id = dev->dev_id;

	if (direction == READ) {
		packet = (device_id << HTIF_DEV_SHIFT | HTIF_COMMAND_READ | request_pa);

		while (mtpcr(PCR_TOHOST, packet));
		while ((fromhost = mtpcr(PCR_FROMHOST, 0)) == 0);

		return size;
	} else if (direction == WRITE) {
		packet = (device_id << HTIF_DEV_SHIFT | HTIF_COMMAND_WRITE | request_pa);

		while (mtpcr(PCR_TOHOST, packet));
		while ((fromhost = mtpcr(PCR_FROMHOST, 0)) == 0);

		return size;
	}

	return 0;
}

static void htifbd_request(struct request_queue *q)
{
	struct request *req;
	unsigned long count;
	struct htifbd_dev *dev;

	while ((req = blk_fetch_request(q)) != NULL) {
		dev = req->rq_disk->private_data;
		if (!(req->cmd_type == REQ_TYPE_FS )) {
			printk("htifbd: punting non-fs request\n");
			blk_end_request(req, -EIO, 0);
			continue;
		}

		count = htifbd_transfer(dev, blk_rq_pos(req), blk_rq_sectors(req),
			req->buffer, rq_data_dir(req));

		blk_end_request(req, 0, count);
	}
}

#define HTIFBD_MAX_MINORS 256

const char *disk_query_string = "disk size=";

char query_buffer[64] __attribute__((aligned(64)));

static int __init htifbd_query(unsigned long dev_id)
{
	unsigned long packet;
	unsigned long buf_pa;
	unsigned long fromhost;
	unsigned long size;
	int err;

	buf_pa = (unsigned long)(__pa(query_buffer));

	packet = (dev_id << HTIF_DEV_SHIFT | HTIF_COMMAND_IDENTITY | 
		(unsigned long)(buf_pa) << 8 | 0xFF);

	while (mtpcr(PCR_TOHOST, packet));
	while ((fromhost = mtpcr(PCR_FROMHOST, 0)) == 0);

	err = strncmp(query_buffer, disk_query_string, strlen(disk_query_string));
	if (err)
		return 0;
	
	err = kstrtoul(query_buffer + strlen(disk_query_string), 10, &size);
	if (err) {
		printk("htifbd: unable to determine the size of disk %ld\n", dev_id);
		return 0;
	}

	printk("htifbd: disk found at device id %ld, size: %ld bytes\n", dev_id, size);

	return size;
}

static int __init htifbd_init(void)
{
	int major;
	unsigned long i;
	struct htifbd_dev *htifbd_device;
	unsigned long size;
	char name[10];

	major = register_blkdev(0, "htifbd");
	if (!major)
		goto out;

	printk("htifbd: enumerating block devices\n");

	/* Initialize each one */
	for (i = 0; i < HTIFBD_MAX_MINORS; i++) {
		size = htifbd_query(i);

		if (!size)
			continue;

		size = size >> 9; /* Obtain size in 512-byte sectors */

		htifbd_device = kmalloc(1 * sizeof(struct htifbd_dev), GFP_KERNEL);
		memset(htifbd_device, 0, sizeof(struct htifbd_dev));
		htifbd_device->size = size;
		htifbd_device->dev_id = i;
		
		spin_lock_init(&htifbd_lock);
		htifbd_queue = blk_init_queue(htifbd_request, &htifbd_lock);

		spin_lock_init(&htifbd_device->lock);
		htifbd_device->gd = alloc_disk(1);

		if (!htifbd_device->gd) {
			printk("htifbd: alloc_disk failure\n");
			goto out;
		}

		htifbd_device->gd->major = major;
		htifbd_device->gd->minors = 1;
		htifbd_device->gd->first_minor = 0;
		htifbd_device->gd->queue = htifbd_queue;
		htifbd_device->gd->private_data = htifbd_device;
		set_capacity(htifbd_device->gd, size);
		
		sprintf(name, "htifbd%ld", i);
		printk("htifbd: creating %s\n", name);
		strcpy(htifbd_device->gd->disk_name, name);	

		htifbd_device->gd->fops = &htifbd_ops;

		add_disk(htifbd_device->gd);
	}

	return 0;

out:
	unregister_blkdev(0, "htifbd");
	return 0;
}
/* normally, this would be module_init, but this initcall would happen
 * earlier than the registration of the i/o elevators. you would have
 * to place this in drivers/block, but i'd rather not...
 */
late_initcall(htifbd_init);
