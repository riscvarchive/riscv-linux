/*
 * SiFive GPIO driver
 *
 * Copyright (C) 2018 SiFive, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/of_irq.h>
#include <linux/gpio/driver.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define GPIO_INPUT_VAL	0x00
#define GPIO_INPUT_EN	0x04
#define GPIO_OUTPUT_EN	0x08
#define GPIO_OUTPUT_VAL	0x0C
#define GPIO_RISE_IE	0x18
#define GPIO_RISE_IP	0x1C
#define GPIO_FALL_IE	0x20
#define GPIO_FALL_IP	0x24
#define GPIO_HIGH_IE	0x28
#define GPIO_HIGH_IP	0x2C
#define GPIO_LOW_IE	0x30
#define GPIO_LOW_IP	0x34
#define GPIO_OUTPUT_XOR	0x40

#define MAX_GPIO	32

struct sifive_gpio {
	raw_spinlock_t		lock;
	void __iomem		*base;
	struct gpio_chip	gc;
	unsigned long		enabled;
	unsigned		trigger[MAX_GPIO];
	unsigned int		irq_parent[MAX_GPIO];
	struct sifive_gpio	*self_ptr[MAX_GPIO];
};

static void sifive_assign_bit(void __iomem *ptr, int offset, int value)
{
	// It's frustrating that we are not allowed to use the device atomics
	// which are GUARANTEED to be supported by this device on RISC-V
	u32 bit = BIT(offset), old = ioread32(ptr);
	if (value)
		iowrite32(old | bit, ptr);
	else
		iowrite32(old & ~bit, ptr);
}

static int sifive_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	if (offset >= gc->ngpio)
		return -EINVAL;

	raw_spin_lock_irqsave(&chip->lock, flags);
	sifive_assign_bit(chip->base + GPIO_OUTPUT_EN, offset, 0);
	sifive_assign_bit(chip->base + GPIO_INPUT_EN,  offset, 1);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int sifive_direction_output(struct gpio_chip *gc, unsigned offset, int value)
{
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	if (offset >= gc->ngpio)
		return -EINVAL;

	raw_spin_lock_irqsave(&chip->lock, flags);
	sifive_assign_bit(chip->base + GPIO_INPUT_EN,   offset, 0);
	sifive_assign_bit(chip->base + GPIO_OUTPUT_VAL, offset, value);
	sifive_assign_bit(chip->base + GPIO_OUTPUT_EN,  offset, 1);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int sifive_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct sifive_gpio *chip = gpiochip_get_data(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;

	return !(ioread32(chip->base + GPIO_OUTPUT_EN) & BIT(offset));
}

static int sifive_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct sifive_gpio *chip = gpiochip_get_data(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;

	return !!(ioread32(chip->base + GPIO_INPUT_VAL) & BIT(offset));
}

static void sifive_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	if (offset >= gc->ngpio)
		return;

	raw_spin_lock_irqsave(&chip->lock, flags);
	sifive_assign_bit(chip->base + GPIO_OUTPUT_VAL, offset, value);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}

static void sifive_set_ie(struct sifive_gpio *chip, int offset)
{
	unsigned long flags;
	unsigned trigger;

	raw_spin_lock_irqsave(&chip->lock, flags);
	trigger = (chip->enabled & BIT(offset)) ? chip->trigger[offset] : 0;
	sifive_assign_bit(chip->base + GPIO_RISE_IE, offset, trigger & IRQ_TYPE_EDGE_RISING);
	sifive_assign_bit(chip->base + GPIO_FALL_IE, offset, trigger & IRQ_TYPE_EDGE_FALLING);
	sifive_assign_bit(chip->base + GPIO_HIGH_IE, offset, trigger & IRQ_TYPE_LEVEL_HIGH);
	sifive_assign_bit(chip->base + GPIO_LOW_IE,  offset, trigger & IRQ_TYPE_LEVEL_LOW);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}

static int sifive_irq_set_type(struct irq_data *d, unsigned trigger)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	int offset = irqd_to_hwirq(d);

	if (offset < 0 || offset >= gc->ngpio)
		return -EINVAL;

	chip->trigger[offset] = trigger;
	sifive_set_ie(chip, offset);
	return 0;
}

/* chained_irq_{enter,exit} already mask the parent */
static void sifive_irq_mask(struct irq_data *d) { }
static void sifive_irq_unmask(struct irq_data *d) { }

static void sifive_irq_enable(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	int offset = irqd_to_hwirq(d) % MAX_GPIO; // must not fail
	u32 bit = BIT(offset);

	/* Switch to input */
	sifive_direction_input(gc, offset);

	/* Clear any sticky pending interrupts */
	iowrite32(bit, chip->base + GPIO_RISE_IP);
	iowrite32(bit, chip->base + GPIO_FALL_IP);
	iowrite32(bit, chip->base + GPIO_HIGH_IP);
	iowrite32(bit, chip->base + GPIO_LOW_IP);

	/* Enable interrupts */
	assign_bit(offset, &chip->enabled, 1);
	sifive_set_ie(chip, offset);
}

static void sifive_irq_disable(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct sifive_gpio *chip = gpiochip_get_data(gc);
	int offset = irqd_to_hwirq(d) % MAX_GPIO; // must not fail

	assign_bit(offset, &chip->enabled, 0);
	sifive_set_ie(chip, offset);
}

static struct irq_chip sifive_irqchip = {
	.name		= "sifive-gpio",
	.irq_set_type	= sifive_irq_set_type,
	.irq_mask	= sifive_irq_mask,
	.irq_unmask	= sifive_irq_unmask,
	.irq_enable	= sifive_irq_enable,
	.irq_disable	= sifive_irq_disable,
};

static void sifive_irq_handler(struct irq_desc *desc)
{
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	struct sifive_gpio **self_ptr = irq_desc_get_handler_data(desc);
	struct sifive_gpio *chip = *self_ptr;
	int offset = self_ptr - &chip->self_ptr[0];
	u32 bit = BIT(offset);

	chained_irq_enter(irqchip, desc);

	/* Re-arm the edge triggers so don't miss the next one */
	iowrite32(bit, chip->base + GPIO_RISE_IP);
	iowrite32(bit, chip->base + GPIO_FALL_IP);

	generic_handle_irq(irq_find_mapping(chip->gc.irq.domain, offset));

	/* Re-arm the level triggers after handling to prevent spurious refire */
	iowrite32(bit, chip->base + GPIO_HIGH_IP);
	iowrite32(bit, chip->base + GPIO_LOW_IP);

	chained_irq_exit(irqchip, desc);
}

static int sifive_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct sifive_gpio *chip;
	struct resource *res;
	int gpio, irq, ret, ngpio;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(dev, "out of memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(chip->base)) {
		dev_err(dev, "failed to allocate device memory\n");
		return PTR_ERR(chip->base);
	}

	ngpio = of_irq_count(node);
	if (ngpio >= MAX_GPIO) {
		dev_err(dev, "too many interrupts\n");
		return -EINVAL;
	}

	raw_spin_lock_init(&chip->lock);
	chip->gc.direction_input = sifive_direction_input;
	chip->gc.direction_output = sifive_direction_output;
	chip->gc.get_direction = sifive_get_direction;
	chip->gc.get = sifive_get_value;
	chip->gc.set = sifive_set_value;
	chip->gc.base = -1;
	chip->gc.ngpio = ngpio;
	chip->gc.label = dev_name(dev);
	chip->gc.parent = dev;
	chip->gc.owner = THIS_MODULE;

	ret = gpiochip_add_data(&chip->gc, chip);
	if (ret)
		return ret;

	/* Disable all GPIO interrupts before enabling parent interrupts */
	iowrite32(0, chip->base + GPIO_RISE_IE);
	iowrite32(0, chip->base + GPIO_FALL_IE);
	iowrite32(0, chip->base + GPIO_HIGH_IE);
	iowrite32(0, chip->base + GPIO_LOW_IE);
	chip->enabled = 0;

	ret = gpiochip_irqchip_add(&chip->gc, &sifive_irqchip, 0, handle_simple_irq, IRQ_TYPE_NONE);
	if (ret) {
		dev_err(dev, "could not add irqchip\n");
		gpiochip_remove(&chip->gc);
		return ret;
	}

	chip->gc.irq.num_parents = ngpio;
	chip->gc.irq.parents = &chip->irq_parent[0];
	chip->gc.irq.map = &chip->irq_parent[0];

	for (gpio = 0; gpio < ngpio; ++gpio) {
		irq = platform_get_irq(pdev, gpio);
		if (irq < 0) {
			dev_err(dev, "invalid IRQ\n");
			gpiochip_remove(&chip->gc);
			return -ENODEV;
		}

		chip->irq_parent[gpio] = irq;
		chip->self_ptr[gpio] = chip;
		chip->trigger[gpio] = IRQ_TYPE_LEVEL_HIGH;
	}

	for (gpio = 0; gpio < ngpio; ++gpio) {
		irq = chip->irq_parent[gpio];
		irq_set_chained_handler_and_data(irq, sifive_irq_handler, &chip->self_ptr[gpio]);
		irq_set_parent(irq_find_mapping(chip->gc.irq.domain, gpio), irq);
	}

	platform_set_drvdata(pdev, chip);
	dev_info(dev, "SiFive GPIO chip registered %d GPIOs\n", ngpio);

	return 0;
}

static const struct of_device_id sifive_gpio_match[] = {
	{
		.compatible = "sifive,gpio0",
	},
	{ },
};

static struct platform_driver sifive_gpio_driver = {
	.probe		= sifive_gpio_probe,
	.driver = {
		.name	= "sifive_gpio",
		.of_match_table = of_match_ptr(sifive_gpio_match),
	},
};
builtin_platform_driver(sifive_gpio_driver)
