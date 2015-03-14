#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>

asmlinkage void __irq_entry do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs;

	old_regs = set_irq_regs(regs);
	irq_enter();
	generic_handle_irq(irq);
	irq_exit();
	set_irq_regs(old_regs);
}

static void riscv_irq_mask(struct irq_data *d)
{
	if (d->irq == IRQ_TIMER)
		csr_clear(sstatus, SR_TIE);
	else
		BUG_ON(!arch_irqs_disabled());
}

static void riscv_irq_unmask(struct irq_data *d)
{
	if (d->irq == IRQ_TIMER)
		csr_set(sstatus, SR_TIE);
	else
		BUG_ON(!arch_irqs_disabled());
}

struct irq_chip riscv_irq_chip = {
	.name = "riscv",
	.irq_mask = riscv_irq_mask,
	.irq_mask_ack = riscv_irq_mask,
	.irq_unmask = riscv_irq_unmask,
};

void __init init_IRQ(void)
{
	unsigned int irq;
	for (irq = 0; irq < NR_IRQS; irq++)
	{
		irq_set_chip_and_handler(irq, &riscv_irq_chip, handle_level_irq);
	}
}
