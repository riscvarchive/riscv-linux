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

unsigned long get_wchan(struct task_struct *task)
{
	if (!task || task == current || task->state == TASK_RUNNING)
		return 0;
	if (!task_stack_page(task))
		return 0;
	return task->thread.ra;
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
