#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/smp.h>

asmlinkage void __irq_entry do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs;

	if (irq == IRQ_SOFTWARE && handle_ipi())
		return;

	old_regs = set_irq_regs(regs);
	irq_enter();
	generic_handle_irq(irq);
	irq_exit();
	set_irq_regs(old_regs);
}

static void riscv_irq_mask(struct irq_data *d)
{
	unsigned long ret = sbi_mask_interrupt(d->irq);
	BUG_ON(ret != 0);
}

static void riscv_irq_unmask(struct irq_data *d)
{
	unsigned long ret = sbi_unmask_interrupt(d->irq);
	BUG_ON(ret != 0);
}

struct irq_chip riscv_irq_chip = {
	.name = "riscv",
	.irq_mask = riscv_irq_mask,
	.irq_mask_ack = riscv_irq_mask,
	.irq_unmask = riscv_irq_unmask,
};

void __init init_IRQ(void)
{
	int ret;

	ret = irq_alloc_desc_at(IRQ_TIMER, numa_node_id());
	BUG_ON(ret < 0);
	irq_set_chip_and_handler(IRQ_TIMER, &riscv_irq_chip, handle_level_irq);

	ret = irq_alloc_desc_at(IRQ_SOFTWARE, numa_node_id());
	BUG_ON(ret < 0);
	irq_set_chip_and_handler(IRQ_SOFTWARE, &riscv_irq_chip,
				 handle_level_irq);
}
