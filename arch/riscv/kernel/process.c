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

void __noreturn cpu_idle(void)
{
	for (;;) {
		tick_nohz_idle_enter();
		rcu_idle_enter();
		while (!need_resched())
			cpu_relax();
		rcu_idle_exit();
		tick_nohz_idle_exit();
		schedule_preempt_disabled();
	}
}

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

void exit_thread(void)
{
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
		childregs->v[0] = 0; /* Return value of fork() */
		p->thread.ra = (unsigned long)ret_from_fork;
	}
	p->thread.sp = (unsigned long)childregs; /* kernel sp */
/*
	if (clone_flags & CLONE_SETTLS) {
	}
*/
	return 0;
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return task_pt_regs(tsk)->epc;
}

