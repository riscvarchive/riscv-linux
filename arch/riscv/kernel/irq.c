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
	char			name[20];
};
DEFINE_PER_CPU(struct riscv_irq_data, riscv_irq_data);

static void riscv_software_interrupt(void)
{
	irqreturn_t ret;

#ifdef CONFIG_SMP
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
	csr_clear(sie, d->hwirq);
}

static void riscv_irq_unmask(struct irq_data *d)
{
	csr_set(sie, d->hwirq);
}

static int riscv_intc_init(struct device_node *node, struct device_node *parent)
{
	u32 cpu;
	const char *isa;

	if (parent) return 0; // should have no interrupt parent
	if (of_property_read_u32(node->parent, "reg", &cpu)) return 0;
	if (of_property_read_string(node->parent, "riscv,isa", &isa)) return 0;

	if (cpu < NR_CPUS) {
		struct riscv_irq_data *data = &per_cpu(riscv_irq_data, cpu);
		snprintf(data->name, sizeof(data->name), "riscv,cpu_intc,%d", cpu);
		data->chip.name = data->name;
		data->chip.irq_mask = riscv_irq_mask;
		data->chip.irq_unmask = riscv_irq_unmask;
		data->domain = irq_domain_add_linear(node, 8*sizeof(uintptr_t), &riscv_irqdomain_ops, data);
		WARN_ON(!data->domain);
		printk("%s: %d local interrupts mapped\n", data->name, 8*(int)sizeof(uintptr_t));

	}
	return 0;
}

IRQCHIP_DECLARE(riscv, "riscv,cpu-intc", riscv_intc_init);

void __init init_IRQ(void)
{
	irqchip_init();
}
