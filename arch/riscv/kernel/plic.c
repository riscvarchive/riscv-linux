/*
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#define MAX_DEVICES	1024 // 0 is reserved
#define MAX_CONTEXTS	15872

#define PRIORITY_BASE	0
#define ENABLE_BASE	0x2000
#define ENABLE_SIZE	0x80
#define HART_BASE	0x200000
#define HART_SIZE	0x1000

#define PLIC_HART_CONTEXT(data, i)	(struct plic_hart_context *)((char*)data->reg + HART_BASE + HART_SIZE*i)
#define PLIC_ENABLE_CONTEXT(data, i)	(struct plic_enable_context *)((char*)data->reg + ENABLE_BASE + ENABLE_SIZE*i)
#define PLIC_PRIORITY(data)		(struct plic_priority *)((char *)data->reg + PRIORITY_BASE)

struct plic_hart_context {
	volatile u32 threshold;
	volatile u32 claim;
};

struct plic_enable_context {
	atomic_t mask[32]; // 32-bit * 32-entry
};

struct plic_priority {
	volatile u32 prio[MAX_DEVICES];
};

struct plic_data {
	struct irq_chip		chip;
	struct irq_domain	*domain;
	u32			ndev;
	void __iomem		*reg;
	int			handlers;
	struct plic_handler	*handler;
	char			name[30];
};

struct plic_handler {
	struct plic_hart_context	*context;
	struct plic_data		*data;
};

static void plic_disable(struct plic_data *data, int i, int hwirq)
{
	struct plic_enable_context *enable = PLIC_ENABLE_CONTEXT(data, i);
	atomic_and(~(1 << (hwirq % 32)), &enable->mask[hwirq / 32]);
}

static void plic_enable(struct plic_data *data, int i, int hwirq)
{
	struct plic_enable_context *enable = PLIC_ENABLE_CONTEXT(data, i);
	atomic_or((1 << (hwirq % 32)), &enable->mask[hwirq / 32]);
}

// There is no need to mask/unmask PLIC interrupts
// They are "masked" by reading claim and "unmasked" when writing it back.
static void plic_irq_mask(struct irq_data *d) { }
static void plic_irq_unmask(struct irq_data *d) { }

static void plic_irq_enable(struct irq_data *d)
{
	struct plic_data *data = irq_data_get_irq_chip_data(d);
	struct plic_priority *priority = PLIC_PRIORITY(data);
	int i;
	iowrite32(1, &priority->prio[d->hwirq]);
	for (i = 0; i < data->handlers; ++i)
		if (data->handler[i].context)
			plic_enable(data, i, d->hwirq);
}

static void plic_irq_disable(struct irq_data *d)
{
	struct plic_data *data = irq_data_get_irq_chip_data(d);
	struct plic_priority *priority = PLIC_PRIORITY(data);
	int i;
	iowrite32(0, &priority->prio[d->hwirq]);
	for (i = 0; i < data->handlers; ++i)
		if (data->handler[i].context)
			plic_disable(data, i, d->hwirq);
}

static int plic_irqdomain_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq)
{
	struct plic_data *data = d->host_data;

        irq_set_chip_and_handler(irq, &data->chip, handle_simple_irq);
        irq_set_chip_data(irq, data);
        irq_set_noprobe(irq);

        return 0;
}

static const struct irq_domain_ops plic_irqdomain_ops = {
	.map	= plic_irqdomain_map,
	.xlate	= irq_domain_xlate_onecell,
};

static void plic_chained_handle_irq(struct irq_desc *desc)
{
        struct plic_handler *handler = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct irq_domain *domain = handler->data->domain;
	u32 what;

	chained_irq_enter(chip, desc);

	while ((what = ioread32(&handler->context->claim))) {
		int irq = irq_find_mapping(domain, what);
		if (irq > 0) {
			generic_handle_irq(irq);
		} else {
			handle_bad_irq(desc);
		}
		iowrite32(what, &handler->context->claim);
	}

	chained_irq_exit(chip, desc);
}

// TODO: add a /sys interface to set priority + per-hart enables for steering

static int plic_init(struct device_node *node, struct device_node *parent)
{
	struct plic_data *data;
	struct resource resource;
	int i, ok = 0;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (WARN_ON(!data)) return -ENOMEM;

	data->reg = of_iomap(node, 0);
	if (WARN_ON(!data->reg)) return -EIO;

	of_property_read_u32(node, "riscv,ndev", &data->ndev);
	if (WARN_ON(!data->ndev)) return -EINVAL;

	data->handlers = of_irq_count(node);
	if (WARN_ON(!data->handlers)) return -EINVAL;

	data->handler = kzalloc(sizeof(*data->handler)*data->handlers, GFP_KERNEL);
	if (WARN_ON(!data->handler)) return -ENOMEM;

	data->domain = irq_domain_add_linear(node, data->ndev+1, &plic_irqdomain_ops, data);
	if (WARN_ON(!data->domain)) return -ENOMEM;

	of_address_to_resource(node, 0, &resource);
	snprintf(data->name, sizeof(data->name), "riscv,plic0,%llx", resource.start);
	data->chip.name = data->name;
	data->chip.irq_mask = plic_irq_mask;
	data->chip.irq_unmask = plic_irq_unmask;
	data->chip.irq_enable = plic_irq_enable;
	data->chip.irq_disable = plic_irq_disable;

	for (i = 0; i < data->handlers; ++i) {
		struct plic_handler *handler = &data->handler[i];
		struct of_phandle_args parent;
		int parent_irq, hwirq;

		if (of_irq_parse_one(node, i, &parent)) continue;
		if (parent.args[0] == -1) continue; // skip context holes

		// skip any contexts that lead to inactive harts
		if (of_device_is_compatible(parent.np, "riscv,cpu-intc") &&
		    parent.np->parent &&
		    riscv_of_processor_hart(parent.np->parent) < 0) continue;

		parent_irq = irq_create_of_mapping(&parent);
		if (!parent_irq) continue;

		handler->context = PLIC_HART_CONTEXT(data, i);
		handler->data = data;
		iowrite32(0, &handler->context->threshold); // hwirq prio must be > this to trigger an interrupt
		for (hwirq = 1; hwirq <= data->ndev; ++hwirq) plic_disable(data, i, hwirq);
		irq_set_chained_handler_and_data(parent_irq, plic_chained_handle_irq, handler);
		++ok;
	}

	printk("%s: mapped %d interrupts to %d/%d handlers\n", data->name, data->ndev, ok, data->handlers);
	WARN_ON(!ok);
	return 0;
}

IRQCHIP_DECLARE(plic0, "riscv,plic0", plic_init);
