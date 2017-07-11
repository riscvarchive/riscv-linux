/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
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

static void riscv_software_interrupt(void)
{
#ifdef CONFIG_SMP
	irqreturn_t ret;

	ret = handle_ipi();

	WARN_ON(ret == IRQ_NONE);
#else
	/*
	 * We currently only use software interrupts to pass inter-processor
	 * interrupts, so if a non-SMP system gets a software interrupt then we
	 * don't know what to do.
	 */
	pr_warning("Software Interrupt without CONFIG_SMP\n");
#endif
}

asmlinkage void __irq_entry do_IRQ(unsigned int cause, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	struct irq_domain *domain;

	irq_enter();

	/*
	 * There are three classes of interrupt: timer, software, and
	 * external devices.  We dispatch between them here.  External
	 * device interrupts use the generic IRQ mechanisms.
	 */
	switch (cause) {
	case INTERRUPT_CAUSE_TIMER:
		riscv_timer_interrupt();
		break;
	case INTERRUPT_CAUSE_SOFTWARE:
		riscv_software_interrupt();
		break;
	default:
		domain = per_cpu(riscv_irq_data, smp_processor_id()).domain;
		generic_handle_irq(irq_find_mapping(domain, cause));
		break;
	}

	irq_exit();
	set_irq_regs(old_regs);
}

static int riscv_irqdomain_map(struct irq_domain *d, unsigned int irq,
			       irq_hw_number_t hwirq)
{
	struct riscv_irq_data *data = d->host_data;

	irq_set_chip_and_handler(irq, &data->chip, handle_simple_irq);
	irq_set_chip_data(irq, data);
	irq_set_noprobe(irq);
	irq_set_affinity(irq, cpumask_of(data->hart));

	return 0;
}

static const struct irq_domain_ops riscv_irqdomain_ops = {
	.map	= riscv_irqdomain_map,
	.xlate	= irq_domain_xlate_onecell,
};

/*
 * On RISC-V systems local interrupts are masked or unmasked by writing the SIE
 * (Supervisor Interrupt Enable) CSR.  As CSRs can only be written on the local
 * hart, these functions can only be called on the hart that corresponds to the
 * IRQ chip.  They are only called internally to this module, so they BUG_ON if
 * this condition is violated rather than attempting to handle the error by
 * forwarding to the target hart, as that's already expected to have been done.
 */
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

/* Callbacks for twiddling SIE on another hart. */
static void riscv_irq_enable_helper(void *d)
{
	riscv_irq_unmask(d);
}

static void riscv_irq_disable_helper(void *d)
{
	riscv_irq_mask(d);
}

static void riscv_irq_enable(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);

	/*
	 * It's only possible to write SIE on the current hart.  This jumps
	 * over to the target hart if it's not the current one.  It's invalid
	 * to write SIE on a hart that's not currently running.
	 */
	if (data->hart == smp_processor_id())
		riscv_irq_unmask(d);
	else if (cpu_online(data->hart))
		smp_call_function_single(data->hart,
					 riscv_irq_enable_helper,
					 d,
					 true);
	else
		WARN_ON_ONCE(1);
}

static void riscv_irq_disable(struct irq_data *d)
{
	struct riscv_irq_data *data = irq_data_get_irq_chip_data(d);

	/*
	 * It's only possible to write SIE on the current hart.  This jumps
	 * over to the target hart if it's not the current one.  It's invalid
	 * to write SIE on a hart that's not currently running.
	 */
	if (data->hart == smp_processor_id())
		riscv_irq_mask(d);
	else if (cpu_online(data->hart))
		smp_call_function_single(data->hart,
					 riscv_irq_disable_helper,
					 d,
					 true);
	else
		WARN_ON_ONCE(1);
}

static int riscv_intc_init(struct device_node *node, struct device_node *parent)
{
	int hart;
	struct riscv_irq_data *data;

	if (parent)
		return 0;

	hart = riscv_of_processor_hart(node->parent);
	if (hart < 0)
		return -EIO;

	data = &per_cpu(riscv_irq_data, hart);
	snprintf(data->name, sizeof(data->name), "riscv,cpu_intc,%d", hart);
	data->hart = hart;
	data->chip.name = data->name;
	data->chip.irq_mask = riscv_irq_mask;
	data->chip.irq_unmask = riscv_irq_unmask;
	data->chip.irq_enable = riscv_irq_enable;
	data->chip.irq_disable = riscv_irq_disable;
	data->domain = irq_domain_add_linear(
		node,
		8*sizeof(uintptr_t),
		&riscv_irqdomain_ops,
		data);
	if (!data->domain)
		goto error_add_linear;
	pr_info("%s: %d local interrupts mapped\n",
	        data->name, 8*(int)sizeof(uintptr_t));
	return 0;

error_add_linear:
	pr_warning("%s: unable to add IRQ domain\n",
		   data->name);
	return -(ENXIO);

}

IRQCHIP_DECLARE(riscv, "riscv,cpu-intc", riscv_intc_init);
