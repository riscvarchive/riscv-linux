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

#define HART_BASE	0x200000
#define HART_SIZE	0x1000

struct plic_context {
	volatile u32 priority_threshold;
	volatile u32 claim;
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
	struct plic_context	*context;
	struct plic_data	*data;
};

// TODO: add a /sys interface to set priority + steering

static void plic_irq_mask(struct irq_data *d)
{
	// TODO: disable interrupt ... globally
}

static void plic_irq_unmask(struct irq_data *d)
{
	// struct plic_data *data = irq_data_get_irq_chip_data(d);
	// TODO: enable interrupt ... globally
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
	}

	chained_irq_exit(chip, desc);
}

static int plic_init(struct device_node *node, struct device_node *parent)
{
	struct plic_data *data;
	struct resource resource;
	int i;

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

	for (i = 0; i < data->handlers; ++i) {
		struct plic_handler *handler = &data->handler[i];
		int irq = irq_of_parse_and_map(node, i);
		if (WARN_ON(!irq)) continue;
		handler->context = data->reg + HART_BASE + i*HART_SIZE;
		handler->data = data;
		irq_set_chained_handler_and_data(irq, plic_chained_handle_irq, handler);
	}

	printk("%s: mapped %d interrupts to %d handlers\n", data->name, data->ndev, data->handlers);
	return 0;
}

IRQCHIP_DECLARE(plic0, "riscv,plic0", plic_init);
