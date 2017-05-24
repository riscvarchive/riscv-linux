#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/fs.h>
#include <linux/hdreg.h>

#include <asm/io.h>
#include <asm/sbi.h>
#include <asm/page.h>

MODULE_LICENSE("Dual BSD/GPL");

#define SBI_DISK_NAME "sbi-disk"
#define SBI_DISK_MINORS 16
#define SBI_DISK_SECTORS 0x200000
#define SECTOR_SIZE 512
#define SECTOR_SHIFT 9

static int sbi_disk_major = 0;

struct sbi_disk_dev {
	struct gendisk *gd;
	struct request_queue *queue;
	spinlock_t lock;
};

static struct sbi_disk_dev sbi_disk_dev;

static struct block_device_operations sbi_disk_fops = {
	.owner = THIS_MODULE
};

#ifdef CONFIG_SBI_DISK_MULTISEGMENT
static inline void sbi_disk_transfer_segment(
		struct bio_vec *bvec, unsigned long offset, int write)
{
	unsigned long addr = page_to_phys(bvec->bv_page) + bvec->bv_offset;
	if (write) {
		printk(KERN_INFO "sbi_disk_write(%lx, %lx, %d)\n",
				addr, offset, bvec->bv_len);
		sbi_disk_write(addr, offset, bvec->bv_len);
	} else {
		printk(KERN_INFO "sbi_disk_read(%lx, %lx, %d)\n",
				addr, offset, bvec->bv_len);
		sbi_disk_read(addr, offset, bvec->bv_len);
	}
}

static void sbi_disk_transfer(struct request *req, int write)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	unsigned long offset = blk_rq_pos(req) << SECTOR_SHIFT;

	rq_for_each_segment(bvec, req, iter) {
		sbi_disk_transfer_segment(&bvec, offset, write);
		offset += bvec.bv_len;
	}
}
#else
static void sbi_disk_transfer(struct request *req, int write)
{
	unsigned long addr = bio_to_phys(req->bio);
	unsigned long offset = blk_rq_pos(req) << SECTOR_SHIFT;
	unsigned long bytes = blk_rq_bytes(req);

	if (write) {
		printk(KERN_INFO "sbi_disk_write(%lx, %lx, %ld)\n", addr, offset, bytes);
		sbi_disk_write(addr, offset, bytes);
	} else {
		printk(KERN_INFO "sbi_disk_read(%lx, %lx, %ld)\n", addr, offset, bytes);
		sbi_disk_read(addr, offset, bytes);
	}
}
#endif

static unsigned long sbi_disk_request(struct request *req)
{
	switch (req_op(req)) {
	case REQ_OP_DISCARD:
	case REQ_OP_FLUSH:
		break;
	case REQ_OP_READ:
		sbi_disk_transfer(req, 0);
		break;
	case REQ_OP_WRITE:
		sbi_disk_transfer(req, 1);
		break;
	default:
		printk(KERN_ERR "unhandleable sbi_disk request\n");
		return -EIO;
	}

	return 0;
}

static void sbi_disk_rq_handler(struct request_queue *rq)
{
	struct request *req;

	while ((req = blk_fetch_request(rq)) != NULL)
		__blk_end_request_cur(req, sbi_disk_request(req));
}

static int sbi_disk_setup(struct sbi_disk_dev *dev)
{
	unsigned long size = sbi_disk_size();

	if (size == 0)
		return 0;

	spin_lock_init(&dev->lock);

	dev->queue = blk_init_queue(sbi_disk_rq_handler, &dev->lock);
	if (!dev->queue)
		goto exit_queue;
	blk_queue_logical_block_size(dev->queue, SECTOR_SIZE);
#ifndef CONFIG_SBI_DISK_MULTISEGMENT
	blk_queue_max_segments(dev->queue, 1);
#endif

	dev->gd = alloc_disk(SBI_DISK_MINORS);
	if (!dev->gd)
		goto exit_gendisk;
	dev->gd->major = sbi_disk_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &sbi_disk_fops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_name, 32, SBI_DISK_NAME);
	set_capacity(dev->gd, size >> SECTOR_SHIFT);
	add_disk(dev->gd);
	printk(KERN_INFO "disk [%s] of %lu bytes loaded\n",
			dev->gd->disk_name, size);

	return 0;

exit_gendisk:
	blk_cleanup_queue(dev->queue);
exit_queue:
	return -ENOMEM;
}

static void sbi_disk_teardown(struct sbi_disk_dev *dev)
{
	del_gendisk(dev->gd);
	put_disk(dev->gd);
	blk_cleanup_queue(dev->queue);
}

static int __init sbi_disk_init(void)
{
	int retval;

	retval = register_blkdev(0, SBI_DISK_NAME);
	if (retval <= 0)
		return retval;
	sbi_disk_major = retval;

	retval = sbi_disk_setup(&sbi_disk_dev);

	return retval;
}

static void __exit sbi_disk_exit(void)
{
	sbi_disk_teardown(&sbi_disk_dev);
	unregister_blkdev(sbi_disk_major, SBI_DISK_NAME);
}

module_init(sbi_disk_init);
module_exit(sbi_disk_exit);
