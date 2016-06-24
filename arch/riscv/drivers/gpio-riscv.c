#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <asm/config-string.h>

struct riscv_gpio_chip {
	struct gpio_chip chip;
	uint32_t __iomem *reg;
};

static int riscv_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	return 0; /* output */
}

static int riscv_gpio_get_value(struct gpio_chip *gc, unsigned int offset)
{
	struct riscv_gpio_chip *rv = container_of(gc, struct riscv_gpio_chip, chip);
	return !!(*rv->reg & BIT(offset));
}

static void riscv_gpio_set_value(struct gpio_chip *gc,
	unsigned int offset, int value)
{
	struct riscv_gpio_chip *rv = container_of(gc, struct riscv_gpio_chip, chip);
	u32 mask = BIT(offset);
	u32 val = value ? mask : 0;
	*rv->reg = (*rv->reg & ~mask) | val;
}

static int riscv_gpio_probe(struct platform_device *pdev)
{
	struct riscv_gpio_chip *rv;
	struct resource *res;
	void *base;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	rv = kzalloc(sizeof(*rv), GFP_KERNEL);
	if (!rv)
		return -ENOMEM;

	rv->chip.label = "riscv-gpio";
	rv->chip.base = -1;
	rv->chip.ngpio = config_string_u64(pdev, "ngpio");
	rv->chip.get_direction = riscv_gpio_get_direction;
	rv->chip.get = riscv_gpio_get_value;
	rv->chip.set = riscv_gpio_set_value;
	rv->reg = base;

	platform_set_drvdata(pdev, rv);
	ret = gpiochip_add(&rv->chip);

	if (ret == 0) {
		dev_info(&pdev->dev, "loaded %d GPIOs\n", rv->chip.ngpio);
	} else {
		dev_warn(&pdev->dev, "failed to gpiochip_add (%d)\n", ret);
	}

	return ret;
}

static int riscv_gpio_remove(struct platform_device *pdev)
{
	struct riscv_gpio_chip *rv;

	rv = platform_get_drvdata(pdev);
	gpiochip_remove(&rv->chip);
	kfree(rv);

	return 0;
}

static struct platform_driver riscv_gpio_driver = {
	.driver = {
		.name = "gpio",
	},
	.probe = riscv_gpio_probe,
	.remove = riscv_gpio_remove,
};

module_platform_driver(riscv_gpio_driver);
MODULE_AUTHOR("Wesley W. Terpstra <wesley@sifive.com>");
MODULE_DESCRIPTION("RISC-V GPIO driver");
MODULE_LICENSE("BSD");
