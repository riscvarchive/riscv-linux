#include <linux/device.h>
#include <linux/export.h>
#include <linux/string.h>

#include <asm/htif.h>

int htif_register_driver(struct htif_driver *drv)
{
	drv->driver.bus = &htif_bus_type;
	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(htif_register_driver);

void htif_unregister_driver(struct htif_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(htif_unregister_driver);

static int htif_bus_match(struct device *dev, struct device_driver *drv)
{
	const char *id;
	const char *s;
	size_t n;

	id = to_htif_dev(dev)->id;
	s = strnchr(id, HTIF_MAX_ID, ' ');
	n = (s != NULL) ? (s - id) : HTIF_MAX_ID;
	return !strncmp(id, to_htif_driver(drv)->type, n);
}

struct bus_type htif_bus_type = {
	.name = "htif",
	.match = htif_bus_match,
};
EXPORT_SYMBOL(htif_bus_type);

static int __init htif_driver_init(void)
{
	return bus_register(&htif_bus_type);
}
postcore_initcall(htif_driver_init);
