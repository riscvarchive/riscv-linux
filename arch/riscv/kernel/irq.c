#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/sbi-con.h>
#include <asm/smp.h>

static void riscv_software_interrupt(void)
{
	irqreturn_t ret;

#ifdef CONFIG_SMP
	ret = handle_ipi();
	if (ret != IRQ_NONE)
		return;
#endif

	ret = sbi_console_isr();
	if (ret != IRQ_NONE)
		return;

	BUG();
}

extern void plic_interrupt(void);

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
#ifdef CONFIG_PLIC
			plic_interrupt();
#endif
			break;
		default:
			BUG();
	}

	irq_exit();
	set_irq_regs(old_regs);
}

void __init init_IRQ(void)
{
	/* Enable software interrupts (and disable the others) */
	csr_write(sie, SIE_SSIE);
}
