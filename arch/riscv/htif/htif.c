#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/export.h>

#include <asm/htif.h>

struct htif_desc {
	struct htif_device *dev;
	htif_irq_handler_t handler;
};

struct htif_bus {
	struct device dev;
	struct htif_desc *desc;
	unsigned int count;
};

static struct htif_bus bus = {
	.desc = NULL,
	.count = 0,
};
static DEFINE_SPINLOCK(bus_lock);

int htif_request_irq(struct htif_device *dev, htif_irq_handler_t handler)
{
	int ret = -EINVAL;

	if (!WARN_ON(dev->index >= bus.count)) {
		struct htif_desc *desc;
		unsigned long flags;

		desc = bus.desc + dev->index;
		spin_lock_irqsave(&bus_lock, flags);
		if (!WARN_ON(desc->dev != dev)) {
			desc->handler = handler;
			ret = 0;
		}
		spin_unlock_irqrestore(&bus_lock, flags);
	}

	return ret;
}
EXPORT_SYMBOL(htif_request_irq);

void htif_free_irq(struct htif_device *dev)
{
	htif_request_irq(dev, NULL);
}
EXPORT_SYMBOL(htif_free_irq);

static irqreturn_t htif_isr(int irq, void *dev_id)
{
	unsigned long data;
	irqreturn_t ret;

	ret = IRQ_NONE;
	while ((data = __htif_fromhost())) {
		struct htif_desc *desc;
		struct htif_device *dev;
		htif_irq_handler_t handler;
		unsigned int index;

		index = (data >> HTIF_DEV_SHIFT);
		desc = bus.desc + index;
		handler = NULL;

		spin_lock(&bus_lock);
		if (!WARN_ON(index >= bus.count)) {
			dev = desc->dev;
			handler = desc->handler;
		}
		spin_unlock(&bus_lock);

		if (likely(handler != NULL))
			ret |= handler(dev, data);
	}
	return ret;
}

static void htif_dev_release(struct device *dev)
{
	struct htif_device *htif_dev = to_htif_dev(dev);

	if (!WARN_ON(htif_dev->index >= bus.count)) {
		struct htif_desc *desc;
		unsigned long flags;

		desc = bus.desc + htif_dev->index;
		spin_lock_irqsave(&bus_lock, flags);
		WARN_ON(htif_dev != desc->dev);
		desc->dev = NULL;
		desc->handler = NULL;
		spin_unlock_irqrestore(&bus_lock, flags);
	}

	kfree(htif_dev);
}

static int __init htif_init(void)
{
	unsigned int i;
	int ret;

	dev_set_name(&bus.dev, htif_bus_type.name);
	ret = device_register(&bus.dev);
	if (unlikely(ret)) {
		dev_err(&bus.dev, "error registering bus: %d\n", ret);
		return ret;
	}

	/* Enumerate devices */
	for (i = 0; i < HTIF_MAX_DEV; i++) {
		char id[HTIF_MAX_ID] __aligned(HTIF_ALIGN);
		struct htif_device *dev;
		size_t len;

		rmb();
		htif_tohost(i, HTIF_CMD_IDENTIFY, (__pa(id) << 8) | 0xFF);
		htif_fromhost();
		wmb();

		len = strnlen(id, sizeof(id));
		if (len <= 0)
			break;

		dev = kzalloc(sizeof(struct htif_device) + len + 1, GFP_KERNEL);
		if (unlikely(dev == NULL))
			return -ENOMEM;

		if (i >= bus.count) {
			struct htif_desc *desc;
			unsigned int count;

			count = bus.count + 8;
			desc = krealloc(bus.desc,
				sizeof(struct htif_desc) * count, GFP_KERNEL);
			if (unlikely(desc == NULL)) {
				kfree(dev);
				return -ENOMEM;
			}
			memset(desc + bus.count, 0, sizeof(struct htif_desc) * 8);
			bus.desc = desc;
			bus.count = count;
		}
		bus.desc[i].dev = dev;

		memcpy(dev->id, id, len);
		dev->id[len] = '\0';
		dev->index = i;
		dev->dev.parent = &bus.dev;
		dev->dev.bus = &htif_bus_type;
		dev->dev.release = &htif_dev_release;
		dev_set_name(&dev->dev, "htif%u", i);

		ret = device_register(&dev->dev);
		if (unlikely(ret)) {
			dev_err(&bus.dev, "error registering device %s: %d\n",
				dev_name(&dev->dev), ret);
			put_device(&dev->dev);
		}
	}

	return request_irq(IRQ_HOST, htif_isr, 0, dev_name(&bus.dev), NULL);
}
subsys_initcall(htif_init);
