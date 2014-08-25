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

	printk("epc: %016lx ra : %016lx s0 : %016lx\n",
		regs->epc, regs->ra, regs->s[0]);
	printk("s1 : %016lx s2 : %016lx s3 : %016lx\n",
		regs->s[1], regs->s[2], regs->s[3]);
	printk("s4 : %016lx s5 : %016lx s6 : %016lx\n",
		regs->s[4], regs->s[5], regs->s[6]);
	printk("s7 : %016lx s8 : %016lx s9 : %016lx\n",
		regs->s[7], regs->s[8], regs->s[9]);
	printk("s10: %016lx s11: %016lx sp : %016lx\n",
		regs->s[10], regs->s[11], regs->sp);
	printk("tp : %016lx v0 : %016lx v1 : %016lx\n",
		regs->tp, regs->v[0], regs->v[1]);
	printk("a0 : %016lx a1 : %016lx a2 : %016lx\n",
		regs->a[0], regs->a[1], regs->a[2]);
	printk("a3 : %016lx a4 : %016lx a5 : %016lx\n",
		regs->a[3], regs->a[4], regs->a[5]);
	printk("a6 : %016lx a7 : %016lx t0 : %016lx\n",
		regs->a[6], regs->a[7], regs->t[0]);
	printk("t1 : %016lx t2 : %016lx t3 : %016lx\n",
		regs->t[1], regs->t[2], regs->t[3]);
	printk("t4 : %016lx gp : %016lx\n",
		regs->t[4], regs->gp);

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
		childregs->status = (/*SR_IM |*/ SR_VM | SR_S64 | SR_U64 | SR_EF | SR_PEI | SR_PS | SR_S);

		p->thread.ra = (unsigned long)ret_from_kernel_thread;
		p->thread.s[0] = usp; /* fn */
		p->thread.s[1] = arg;
	} else {
		*childregs = *(current_pt_regs());
		if (usp) /* User fork */
			childregs->sp = usp;
		if (clone_flags & CLONE_SETTLS)
			childregs->tp = childregs->a[5];
		childregs->v[0] = 0; /* Return value of fork() */
		p->thread.ra = (unsigned long)ret_from_fork;
	}
	p->thread.sp = (unsigned long)childregs; /* kernel sp */
	return 0;
}
