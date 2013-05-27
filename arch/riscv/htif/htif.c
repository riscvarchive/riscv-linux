#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <asm/page.h>
#include <asm/barrier.h>
#include <asm/htif.h>


struct device htif_bus = {
	.init_name = "htif",
};

static int htif_bus_match(struct device *dev, struct device_driver *drv)
{
	return !strcmp(to_htif_dev(dev)->type, to_htif_driver(drv)->type);
}

struct bus_type htif_bus_type = {
	.name = "htif",
	.match = htif_bus_match,
};

int htif_register_driver(struct htif_driver *drv)
{
	drv->driver.bus = &htif_bus_type;
	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(htif_register_driver);

void htif_unregister_driver(struct htif_driver *drv)
{
	drv->driver.bus = &htif_bus_type;
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(htif_unregister_driver);


static void htif_dev_release(struct device *dev)
{
	kfree(to_htif_dev(dev));
}

static int htif_dev_register(struct htif_dev *dev)
{
	dev->dev.parent = &htif_bus;
	dev->dev.bus = &htif_bus_type;
	dev->dev.release = &htif_dev_release;
	dev_set_name(&dev->dev, "htif%u", dev->minor);
	return device_register(&dev->dev);
}

static inline void htif_dev_add(unsigned int minor, const char *id, size_t len)
{
	struct htif_dev *dev;
	char *s;
	size_t n;

	/* Parse identity string */
	s = strnchr(id, len, ' ');
	if (s == id)
		return;
	n = (s != NULL) ? (s - id) : len;
	s = kmalloc(len + 1, GFP_KERNEL);
	if (s == NULL)
		return;
	memcpy(s, id, len);
	s[len] = '\0';
	if (n < len)
		s[n++] = '\0';

	dev = kzalloc(sizeof(struct htif_dev), GFP_KERNEL);
	if (dev == NULL)
		goto err_dev_alloc;
	dev->minor = minor;
	dev->type = s;
	dev->spec = s + n;
	if (htif_dev_register(dev))
		goto err_dev_reg;
	return;

err_dev_reg:
	put_device(&dev->dev);
err_dev_alloc:
	kfree(s);
	return;
}

static int htif_bus_enumerate(void)
{
	char buf[64] __attribute__((aligned(64)));
	unsigned int minor;

	for (minor = 0; minor < HTIF_NR_DEV; minor++) {
		mb();
		htif_tohost(minor, HTIF_CMD_IDENTITY, (__pa(buf) << 8) | 0xFF);
		htif_fromhost();
		mb();

		if (buf[0] != '\0')
			htif_dev_add(minor, buf, strnlen(buf, sizeof(buf)));
	}
	return 0;
}

static int __init htif_bus_init(void)
{
	int ret;
	ret = bus_register(&htif_bus_type);
	if (ret)
		return ret;
	ret = device_register(&htif_bus);
	if (ret) {
		bus_unregister(&htif_bus_type);
		return ret;
	}
	htif_bus_enumerate();
	return 0;
}
early_initcall(htif_bus_init);

