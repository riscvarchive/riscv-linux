/*
 * Copyright (C) 2012 Regents of the University of California
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

#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/of.h>
#include <linux/seq_file.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/smp.h>

struct riscv_irq_data {
	struct irq_chip		chip;
	struct irq_domain	*domain;
	int			hart;
	char			name[20];
};
DEFINE_PER_CPU(struct riscv_irq_data, riscv_irq_data);
DEFINE_PER_CPU(atomic_long_t, riscv_early_sie);

static void riscv_software_interrupt(void)
{
#ifdef CONFIG_SMP
	irqreturn_t ret;

	ret = handle_ipi();
	if (ret != IRQ_NONE)
		return;
#endif

	BUG();
}

asmlinkage void __irq_entry do_IRQ(unsigned int cause, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();

	/* There are three classes of interrupt: timer, software, and
	   external devices.  We dispatch between them here.  External
	   device interrupts use the generic IRQ mechanisms. */
	switch (cause) {
		case INTERRUPT_CAUSE_TIMER:
			riscv_timer_interrupt();
			break;
		case INTERRUPT_CAUSE_SOFTWARE:
			riscv_software_interrupt();
			break;
		default: {
			struct irq_domain *domain = per_cpu(riscv_irq_data, smp_processor_id()).domain;
			generic_handle_irq(irq_find_mapping(domain, cause));
			break;
		}
	}

	irq_exit();
	set_irq_regs(old_regs);
}

static int riscv_irqdomain_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq)
{
	struct riscv_irq_data *data = d->host_data;

        irq_set_chip_and_handler(irq, &data->chip, handle_simple_irq);
        irq_set_chip_data(irq, data);
        irq_set_noprobe(irq);

        return 0;
}

static const struct irq_domain_ops riscv_irqdomain_ops = {
	.map	= riscv_irqdomain_map,
	.xlate	= irq_domain_xlate_onecell,
};

static void riscv_irq_mask(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);
	BUG_ON(smp_processor_id() != data->hart);
	csr_clear(sie, 1 << (long)d->hwirq);
}

static void riscv_irq_unmask(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);
	BUG_ON(smp_processor_id() != data->hart);
	csr_set(sie, 1 << (long)d->hwirq);
}

static void riscv_irq_enable_helper(void *d)
{
	riscv_irq_unmask(d);
}

static void riscv_irq_enable(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);
	atomic_long_or((1 << (long)d->hwirq), &per_cpu(riscv_early_sie, data->hart));
	if (data->hart == smp_processor_id()) {
		riscv_irq_unmask(d);
	} else if (cpu_online(data->hart)) {
		smp_call_function_single(data->hart, riscv_irq_enable_helper, d, true);
	}
}

static void riscv_irq_disable_helper(void *d)
{
	riscv_irq_mask(d);
}

static void riscv_irq_disable(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);
	atomic_long_and(~(1 << (long)d->hwirq), &per_cpu(riscv_early_sie, data->hart));
	if (data->hart == smp_processor_id()) {
		riscv_irq_mask(d);
	} else if (cpu_online(data->hart)) {
		smp_call_function_single(data->hart, riscv_irq_disable_helper, d, true);
	}
}

static void riscv_irq_mask_noop(struct irq_data *d) { }

static void riscv_irq_unmask_noop(struct irq_data *d) { }

static void riscv_irq_enable_noop(struct irq_data *d)
{
	struct device_node *data = irq_data_get_irq_chip_data(d);
	u32 hart;

	if (!of_property_read_u32(data, "reg", &hart)) {
		printk("WARNING: enabled interrupt %d for missing hart %d (this interrupt has no handler)\n", (int)d->hwirq, hart);
	}
}

static struct irq_chip riscv_noop_chip = {
	.name = "riscv,cpu-intc,noop",
	.irq_mask = riscv_irq_mask_noop,
	.irq_unmask = riscv_irq_unmask_noop,
	.irq_enable = riscv_irq_enable_noop,
};

static int riscv_irqdomain_map_noop(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq)
{
	struct device_node *data = d->host_data;
	irq_set_chip_and_handler(irq, &riscv_noop_chip, handle_simple_irq);
	irq_set_chip_data(irq, data);
	return 0;
}

static const struct irq_domain_ops riscv_irqdomain_ops_noop = {
	.map    = riscv_irqdomain_map_noop,
	.xlate  = irq_domain_xlate_onecell,
};

static int riscv_intc_init(struct device_node *node, struct device_node *parent)
{
	int hart;

	if (parent) return 0; // should have no interrupt parent

	if ((hart = riscv_of_processor_hart(node->parent)) >= 0) {
		struct riscv_irq_data *data = &per_cpu(riscv_irq_data, hart);
		snprintf(data->name, sizeof(data->name), "riscv,cpu_intc,%d", hart);
		data->hart = hart;
		data->chip.name = data->name;
		data->chip.irq_mask = riscv_irq_mask;
		data->chip.irq_unmask = riscv_irq_unmask;
		data->chip.irq_enable = riscv_irq_enable;
		data->chip.irq_disable = riscv_irq_disable;
		data->domain = irq_domain_add_linear(node, 8*sizeof(uintptr_t), &riscv_irqdomain_ops, data);
		WARN_ON(!data->domain);
		printk("%s: %d local interrupts mapped\n", data->name, 8*(int)sizeof(uintptr_t));
	} else {
		/* If a hart is disabled, create a no-op irq domain.
		 * Devices may still have interrupts connected to those harts.
		 * This is not wrong... unless they actually load a driver that needs it!
		 */
		irq_domain_add_linear(node, 8*sizeof(uintptr_t), &riscv_irqdomain_ops_noop, node->parent);
	}
	return 0;
}

IRQCHIP_DECLARE(riscv, "riscv,cpu-intc", riscv_intc_init);

void __init init_IRQ(void)
{
	irqchip_init();
}
