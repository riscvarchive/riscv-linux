#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/ptrace.h>

#include <asm/unistd.h>
#include <asm/processor.h>
#include <asm/csr.h>
#include <asm/string.h>

extern asmlinkage void ret_from_fork(void);
extern asmlinkage void ret_from_kernel_thread(void);

#if 0
/*
 * Default to the generic definition until the RISC-V specification
 * incorporates architectural support an explicit sleep mode.
 */
void arch_cpu_idle(void)
{
	local_irq_enable();
}
#endif

void show_regs(struct pt_regs *regs)
{
	show_regs_print_info(KERN_DEFAULT);

	printk("epc: %016lx ra : %016lx sp : %016lx\n",
		regs->epc, regs->ra, regs->sp);
	printk("gp : %016lx tp : %016lx t0 : %016lx\n",
		regs->gp, regs->tp, regs->t0);
	printk("t1 : %016lx t2 : %016lx s0 : %016lx\n",
		regs->t1, regs->t2, regs->s0);
	printk("s1 : %016lx a0 : %016lx a1 : %016lx\n",
		regs->s1, regs->a0, regs->a1);
	printk("a2 : %016lx a3 : %016lx a4 : %016lx\n",
		regs->a2, regs->a3, regs->a4);
	printk("a5 : %016lx a6 : %016lx a7 : %016lx\n",
		regs->a5, regs->a6, regs->a7);
	printk("s2 : %016lx s3 : %016lx s4 : %016lx\n",
		regs->s2, regs->s3, regs->s4);
	printk("s5 : %016lx s6 : %016lx s7 : %016lx\n",
		regs->s5, regs->s6, regs->s7);
	printk("s8 : %016lx s9 : %016lx s10: %016lx\n",
		regs->s8, regs->s9, regs->s10);
	printk("s11: %016lx t3 : %016lx t4 : %016lx\n",
		regs->s11, regs->t3, regs->t4);
	printk("t5 : %016lx t6 : %016lx\n",
		regs->t5, regs->t6);

	printk("status: %016lx badvaddr: %016lx cause: %016lx\n",
		regs->status, regs->badvaddr, regs->cause);
}

void start_thread(struct pt_regs *regs, unsigned long pc, 
	unsigned long sp)
{
	/* Remove supervisor privileges */
	regs->status &= ~(SR_PS);
	regs->epc = pc;
	regs->sp = sp;
}

void flush_thread(void)
{
}

int copy_thread(unsigned long clone_flags, unsigned long usp,
	unsigned long arg, struct task_struct *p)
{
	struct pt_regs *childregs = task_pt_regs(p);

	/* p->thread holds context to be restored by __switch_to() */
	if (unlikely(p->flags & PF_KTHREAD)) {
		/* Kernel thread */
		const register unsigned long gp __asm__ ("gp");
		memset(childregs, 0, sizeof(struct pt_regs));
		childregs->gp = gp;
		childregs->status = (/*SR_IM |*/ SR_ER | SR_VM | SR_S64 | SR_U64 | SR_EF | SR_PEI | SR_PS | SR_S);

		p->thread.ra = (unsigned long)ret_from_kernel_thread;
		p->thread.s[0] = usp; /* fn */
		p->thread.s[1] = arg;
	} else {
		*childregs = *(current_pt_regs());
		if (usp) /* User fork */
			childregs->sp = usp;
		if (clone_flags & CLONE_SETTLS)
			childregs->tp = childregs->a5;
		childregs->a0 = 0; /* Return value of fork() */
		p->thread.ra = (unsigned long)ret_from_fork;
	}
	p->thread.sp = (unsigned long)childregs; /* kernel sp */
	return 0;
}
