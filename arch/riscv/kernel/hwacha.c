#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>

#include <asm/current.h>
#include <asm/ptrace.h>
#include <asm/processor.h>
#include <asm/irq.h>
#include <asm/hwacha.h>

extern void do_page_fault(struct pt_regs *);

static irqreturn_t hwacha_interrupt(int irq, void *dev_id)
{
	long sp = current->thread.sp;
	struct pt_regs *regs = (struct pt_regs *)sp - 1;
	long cause = hwacha_vxcptcause();
	long aux = hwacha_vxcptaux();
	siginfo_t info;

	switch (cause) {
	case HWACHA_CAUSE_VF_FAULT_FETCH:
		regs->cause = EXC_INST_ACCESS;
		regs->epc = aux;
		do_page_fault(regs);
		break;
	case HWACHA_CAUSE_FAULT_LOAD:
		regs->cause = EXC_LOAD_ACCESS;
		regs->badvaddr = aux;
		do_page_fault(regs);
		break;
	case HWACHA_CAUSE_FAULT_STORE:
		regs->cause = EXC_STORE_ACCESS;
		regs->badvaddr = aux;
		do_page_fault(regs);
		break;
	default:
		info.si_signo = SIGILL;
		info.si_errno = 0;
		info.si_code = ILL_PRVOPC;
		info.si_addr = (void *)(regs->epc);
		force_sig_info(SIGILL, &info, current);
		break;
	}
	return IRQ_HANDLED;
}

static struct irqaction hwacha_irq = {
	.handler = hwacha_interrupt,
	.flags = IRQF_DISABLED,
	.name = "hwacha",
	.dev_id = &hwacha_interrupt,
};

void __init hwacha_init(void)
{
	setup_irq(IRQ_COP, &hwacha_irq);
}
