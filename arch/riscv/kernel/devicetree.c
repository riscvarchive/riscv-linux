/*
 * Enables use of Device Tree for RISC-V peripheral devices.
 *
 * Author: Jamey Hicks <jamey.hicks@gmail.com>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/irqchip.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>

int __init devicetree_init(void)
{
	resource_size_t initial_dtb = CONFIG_DTB_START;
	unsigned long map_len = 4096;
	unsigned long size = 128;
	void *dt;
	int err = 0;

	printk("%s:%d devicetree physaddr %lx\n", __FUNCTION__, __LINE__, initial_dtb);
	initial_boot_params = dt = ioremap(initial_dtb, map_len);
	size = of_get_flat_dt_size();
	printk("dtb size %ld initial_boot_params=%p\n", size, initial_boot_params);
	if (map_len < size) {
		iounmap(dt);
		initial_boot_params = dt = ioremap(initial_dtb, size);
		map_len = size;
	}
	unflatten_and_copy_device_tree();
	printk("unflattened, maybe\n");
	iounmap(dt);

	return err;
}

int __init devicetree_populate(void)
{
	printk("calling of_irq_init\n");
	irqchip_init();
	printk("calling of_platform_populate\n");
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
	return 0;
}

void __exit devicetree_exit(void)
{

}

subsys_initcall(devicetree_init);
late_initcall(devicetree_populate);
module_exit(devicetree_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jamey Hicks <jamey.hicks@gmail.com>");
MODULE_DESCRIPTION("Simple device tree for devices");
