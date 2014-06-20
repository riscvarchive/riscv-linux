#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/spinlock.h>
#include <linux/module.h>

#include <asm/htif.h>

#define DRIVER_NAME "htifbd"

#define SECTOR_SIZE_SHIFT	(9)
#define SECTOR_SIZE		(1UL << SECTOR_SIZE_SHIFT)

struct htifbd_dev {
	size_t size;		/* size in bytes */
	spinlock_t lock;
	struct gendisk *gd;
	struct htif_dev *dev;
};

static int htifbd_open(struct block_device *bd, fmode_t mode)
{
	return 0;
}

static void htifbd_release(struct gendisk *gd, fmode_t mode)
{
}

static int htifbd_ioctl(struct block_device *bd, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	struct hd_geometry geo;
	struct htifbd_dev *dev;

	dev = bd->bd_disk->private_data;
	switch (cmd) {
	case HDIO_GETGEO:
		geo.cylinders = (dev->size >> 6);
		geo.heads = 4;
		geo.sectors = 16;
		if (copy_to_user((void __user *)arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY;
}

static void htifbd_transfer(struct htifbd_dev *dev, unsigned long sector,
	unsigned long nsect, char *buf, int direction)
{
	/* HTIF disk address packet */
	volatile struct htifbd_dap {
		unsigned long address;
		unsigned long offset;	/* offset in bytes */
		unsigned long length;	/* length in bytes */
		unsigned long tag;
	} req;
	unsigned long offset, length;
	unsigned long htif_cmd;

	offset = (sector << SECTOR_SIZE_SHIFT);
	length = (nsect << SECTOR_SIZE_SHIFT);

	if ((offset + length) > dev->size) {
		pr_notice(DRIVER_NAME "out-of-bounds access to %s with"
			"offset=%lx length=%lx\n",
			dev->gd->disk_name, offset, length);
		return;
	}

	req.address = (unsigned long)__pa(buf);
	req.offset = offset;
	req.length = length;
	req.tag = 0;

	if (direction == READ) {
		htif_cmd = HTIF_CMD_READ;
	} else if (direction == WRITE) {
		htif_cmd = HTIF_CMD_WRITE;
	} else {
		return;
	}

	mb();
	htif_tohost(dev->dev->minor, htif_cmd, __pa(&req));
	htif_fromhost();
	mb();
}

static void htifbd_request(struct request_queue *q)
{
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		struct htifbd_dev *dev;

		dev = req->rq_disk->private_data;
		if (req->cmd_type != REQ_TYPE_FS) {
			pr_notice(DRIVER_NAME ": ignoring non-fs request for %s\n",
				req->rq_disk->disk_name);
			__blk_end_request_all(req, -EIO);
			continue;
		}

		htifbd_transfer(dev, blk_rq_pos(req), blk_rq_cur_sectors(req),
			req->buffer, rq_data_dir(req));
		if (!__blk_end_request_cur(req, 0)) {
			req = blk_fetch_request(q);
		}
	}
}

static const struct block_device_operations htifbd_ops = {
	.owner = THIS_MODULE,
	.open = htifbd_open,
	.release = htifbd_release,
	.ioctl = htifbd_ioctl,
};

static int htifbd_major;

static int htifbd_probe(struct device *dev)
{
	static unsigned int htifbd_nr = 0;
	static const char size_str[] = "size=";

	struct htif_dev *htif_dev;
	struct htifbd_dev *htifbd_dev;
	struct gendisk *gd;
	unsigned long size;

	htif_dev = to_htif_dev(dev);
	pr_info(DRIVER_NAME ": detected disk with ID %u\n", htif_dev->minor);

	if (unlikely(strncmp(htif_dev->spec, size_str, sizeof(size_str) - 1)
		|| kstrtoul(htif_dev->spec + sizeof(size_str) - 1, 10, &size))) {
		pr_err(DRIVER_NAME ": unable to determine size of disk %u\n",
			htif_dev->minor);
		goto err_out;
	}
	if (unlikely(size & (SECTOR_SIZE - 1))) {
		pr_warn(DRIVER_NAME ": size of disk %u not a multiple of sector size\n",
			htif_dev->minor);
	}

	htifbd_dev = kzalloc(sizeof(struct htifbd_dev), GFP_KERNEL);
	if (unlikely(htifbd_dev == NULL))
		goto err_out;
	htifbd_dev->size = size;
	htifbd_dev->dev = htif_dev;

	gd = alloc_disk(1);
	if (unlikely(gd == NULL))
		goto err_gd_alloc;

	spin_lock_init(&htifbd_dev->lock);
	gd->queue = blk_init_queue(htifbd_request, &htifbd_dev->lock);
	if (unlikely(gd->queue == NULL))
		goto err_queue_init;

	gd->major = htifbd_major;
	gd->minors = 1;
	gd->first_minor = 0;
	gd->fops = &htifbd_ops;
	gd->private_data = htifbd_dev;
	set_capacity(gd, size >> SECTOR_SIZE_SHIFT);
	snprintf(gd->disk_name, DISK_NAME_LEN - 1, "htifbd%u", htifbd_nr++);
	pr_info(DRIVER_NAME ": adding %s\n", gd->disk_name);

	htifbd_dev->gd = gd;
	add_disk(gd);
	return 0;

err_queue_init:
	put_disk(gd);
err_gd_alloc:
	kfree(htifbd_dev);
err_out:
	return -ENODEV;
}


static struct htif_driver htifbd_driver = {
	.type = "disk",
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.probe = htifbd_probe,
	},
};

static int __init htifbd_init(void)
{
	int ret;
	ret = register_blkdev(0, DRIVER_NAME);
	if (unlikely(ret < 0))
		return ret;
	htifbd_major = ret;
	ret = htif_register_driver(&htifbd_driver);
	if (unlikely(ret)) {
		unregister_blkdev(0, DRIVER_NAME);
		return ret;
	}
	return 0;
}

/* Normally, this would be module_init, but that initcall would happen
 * earlier than the registration of the I/O elevators.  You would have
 * to place this in drivers/block, but I'd rather not...
 */
late_initcall(htifbd_init);
