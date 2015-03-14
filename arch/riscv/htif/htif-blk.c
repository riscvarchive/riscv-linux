#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/module.h>

#include <asm/htif.h>

#define DRIVER_NAME "htifblk"

#define SECTOR_SIZE_SHIFT	(9)
#define SECTOR_SIZE		(1UL << SECTOR_SIZE_SHIFT)

struct htifblk_device {
	struct htif_device *dev;
	struct gendisk *disk;
	spinlock_t lock;
	u64 size;		/* size in bytes */
	struct request *req;
	sbi_device_message msg_buf;
	unsigned int tag;
};

struct htifblk_request {
	u64 addr;
	u64 offset;	/* offset in bytes */
	u64 size;	/* length in bytes */
	u64 tag;
} __packed;

static int major;

static int htifblk_segment(struct htifblk_device *dev,
	struct request *req)
{
	static struct htifblk_request pkt __aligned(HTIF_ALIGN);
	u64 offset, size, end;
	unsigned long cmd;

	offset = (blk_rq_pos(req) << SECTOR_SIZE_SHIFT);
	size = (blk_rq_cur_sectors(req) << SECTOR_SIZE_SHIFT);

	end = offset + size;
	if (unlikely(end < offset || end > dev->size)) {
		dev_err(&dev->dev->dev, "out-of-bounds access:"
			" offset=%llu size=%llu\n", offset, size);
		return -EINVAL;
	}

	rmb();
	pkt.addr = __pa(req->buffer);
	pkt.offset = offset;
	pkt.size = size;
	pkt.tag = dev->tag;

	switch (rq_data_dir(req)) {
	case READ:
		cmd = HTIF_CMD_READ;
		break;
	case WRITE:
		cmd = HTIF_CMD_WRITE;
		break;
	default:
		return -EINVAL;
	}

	dev->req = req;
	dev->msg_buf.dev = dev->dev->index;
	dev->msg_buf.cmd = cmd;
	dev->msg_buf.data = __pa(&pkt);
	htif_tohost(&dev->msg_buf);
	return 0;
}

static void htifblk_request(struct request_queue *q)
{
	struct htifblk_device *dev;
	struct request *req;
	unsigned long flags;
	int ret;

	dev = q->queuedata;
	spin_lock_irqsave(q->queue_lock, flags);
	if (dev->req != NULL)
		goto out;

	while ((req = blk_fetch_request(q)) != NULL) {
		if (req->cmd_type == REQ_TYPE_FS) {
			ret = htifblk_segment(dev, req);
			if (unlikely(ret)) {
				WARN_ON(__blk_end_request_cur(req, ret));
				continue;
			}
			blk_stop_queue(q);
			break;
		} else {
			blk_dump_rq_flags(req, DRIVER_NAME
				": ignored non-fs request");
			__blk_end_request_all(req, -EIO);
			continue;
		}
	}
out:
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static irqreturn_t htifblk_isr(struct htif_device *dev, sbi_device_message *msg)
{
	struct htifblk_device *htifblk_dev;
	irqreturn_t ret;
	int err;

	htifblk_dev = dev_get_drvdata(&dev->dev);
	ret = IRQ_NONE;

	spin_lock(&htifblk_dev->lock);
	if (unlikely(htifblk_dev->req == NULL)) {
		dev_err(&dev->dev, "null request\n");
		goto out;
	}

	err = 0;
	if (unlikely(msg->data != htifblk_dev->tag)) {
		dev_err(&dev->dev, "tag mismatch: expected=%u actual=%lu\n",
			htifblk_dev->tag, msg->data);
		err = -EIO;
	}

	wmb();
	WARN_ON(__blk_end_request_cur(htifblk_dev->req, err));
	htifblk_dev->req = NULL;
	blk_start_queue(htifblk_dev->disk->queue);
	ret = IRQ_HANDLED;
out:
	spin_unlock(&htifblk_dev->lock);
	return ret;
}

static const struct block_device_operations htifblk_fops = {
	.owner		= THIS_MODULE,
};

static int htifblk_probe(struct device *dev)
{
	static unsigned int index = 0;
	static const char prefix[] = " size=";

	struct htif_device *htif_dev;
	struct htifblk_device *htifblk_dev;
	struct gendisk *disk;
	struct request_queue *queue;
	const char *str;
	u64 size;
	int ret;

	dev_info(dev, "detected disk\n");
	htif_dev = to_htif_dev(dev);

	str = strstr(htif_dev->id, prefix);
	if (unlikely(str == NULL
	    || kstrtou64(str + sizeof(prefix) - 1, 10, &size))) {
		dev_err(dev, "error determining size of disk\n");
		return -ENODEV;
	}
	if (unlikely(size & (SECTOR_SIZE - 1))) {
		dev_warn(dev, "disk size not a multiple of sector size:"
			" %llu\n", size);
	}

	ret = -ENOMEM;
	htifblk_dev = devm_kzalloc(dev, sizeof(struct htifblk_device), GFP_KERNEL);
	if (unlikely(htifblk_dev == NULL))
		goto out;

	htifblk_dev->size = size;
	htifblk_dev->dev = htif_dev;
	htifblk_dev->tag = index;
	spin_lock_init(&htifblk_dev->lock);

	disk = alloc_disk(1);
	if (unlikely(disk == NULL))
		goto out;

	queue = blk_init_queue(htifblk_request, &htifblk_dev->lock);
	if (unlikely(queue == NULL))
		goto out_put_disk;

	queue->queuedata = htifblk_dev;
	blk_queue_max_segments(queue, 1);
	blk_queue_dma_alignment(queue, HTIF_ALIGN - 1);

	disk->queue = queue;
	disk->major = major;
	disk->minors = 1;
	disk->first_minor = 0;
	disk->fops = &htifblk_fops;
	set_capacity(disk, size >> SECTOR_SIZE_SHIFT);
	snprintf(disk->disk_name, DISK_NAME_LEN - 1, "htifblk%u", index++);

	htifblk_dev->disk = disk;
	add_disk(disk);
	dev_info(dev, "added %s\n", disk->disk_name);

	ret = htif_request_irq(htif_dev, htifblk_isr);
	if (unlikely(ret))
		goto out_del_disk;

	dev_set_drvdata(dev, htifblk_dev);
	return 0;

out_del_disk:
	del_gendisk(disk);
	blk_cleanup_queue(disk->queue);
out_put_disk:
	put_disk(disk);
out:
	return ret;
}


static struct htif_driver htifblk_driver = {
	.type = "disk",
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.probe = htifblk_probe,
	},
};

static int __init htifblk_init(void)
{
	int ret;

	ret = register_blkdev(0, DRIVER_NAME);
	if (unlikely(ret < 0))
		return ret;
	major = ret;

	ret = htif_register_driver(&htifblk_driver);
	if (unlikely(ret))
		unregister_blkdev(0, DRIVER_NAME);

	return ret;
}

static void __exit htifblk_exit(void)
{
	unregister_blkdev(major, DRIVER_NAME);
	htif_unregister_driver(&htifblk_driver);
}

module_init(htifblk_init);
module_exit(htifblk_exit);

MODULE_DESCRIPTION("HTIF block driver");
MODULE_LICENSE("GPL");
