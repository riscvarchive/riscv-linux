#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/smp.h>

struct plic_context {
	volatile int priority_threshold;
	volatile int claim;
};

static DEFINE_PER_CPU(struct plic_context *, plic_context);
static DEFINE_PER_CPU(unsigned int, irq_in_progress);

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

static void plic_interrupt(void)
{
	unsigned int cpu = smp_processor_id();
	unsigned int irq = per_cpu(plic_context, cpu)->claim;

	BUG_ON(per_cpu(irq_in_progress, cpu) != 0);

	if (irq) {
		per_cpu(irq_in_progress, cpu) = irq;
		generic_handle_irq(irq);
	}
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
		case INTERRUPT_CAUSE_EXTERNAL:
			plic_interrupt();
			break;
		default:
			BUG();
	}

	irq_exit();
	set_irq_regs(old_regs);
}

static void plic_irq_mask(struct irq_data *d)
{
	unsigned int cpu = smp_processor_id();

	BUG_ON(d->irq != per_cpu(irq_in_progress, cpu));
}

static void plic_irq_unmask(struct irq_data *d)
{
	unsigned int cpu = smp_processor_id();

	BUG_ON(d->irq != per_cpu(irq_in_progress, cpu));

	per_cpu(plic_context, cpu)->claim = per_cpu(irq_in_progress, cpu);
	per_cpu(irq_in_progress, cpu) = 0;
}

struct irq_chip plic_irq_chip = {
	.name = "riscv",
	.irq_mask = plic_irq_mask,
	.irq_mask_ack = plic_irq_mask,
	.irq_unmask = plic_irq_unmask,
};

void __init init_IRQ(void)
{
	/* Enable software interrupts (and disable the others) */
	csr_write(sie, SIE_SSIE);
}
