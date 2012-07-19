#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tick.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/pcr.h>

extern asmlinkage void ret_from_fork(void);

void __noreturn cpu_idle(void)
{
	while (1) {
		tick_nohz_idle_enter();
		rcu_idle_enter();
		while (!need_resched())
			cpu_relax();
		rcu_idle_exit();
		tick_nohz_idle_exit();
		schedule_preempt_disabled();
	}
}

struct task_struct* __switch_to(struct task_struct *prev_p,
	struct task_struct *next_p)
{
	return prev_p; 
}

void exit_thread(void)
{
}

void flush_thread(void)
{
}

int copy_thread(unsigned long clone_flags, unsigned long usp,
	unsigned long unused,
	struct task_struct *p, struct pt_regs *regs)
{
	struct pt_regs *childregs;
	unsigned long childksp;

	childksp = (unsigned long)task_stack_page(p) + THREAD_SIZE;

	childregs = (struct pt_regs *)childksp - 1;

	*childregs = *regs;

	if (user_mode(regs)) {
		childregs->sp = usp;
	} else {
		childregs->sp = (unsigned long)childregs;
	}

	childregs->v[0] = 0; /* Set return value for child: v0 */
	childregs->tp = (unsigned long)task_stack_page(p);

	p->thread.status = (SR_IM | SR_VM | SR_S64 | SR_U64 | SR_PS | SR_S);
	p->thread.sp = (unsigned long)childregs; /* ksp */
	p->thread.pc = (unsigned long)ret_from_fork; /* pc */
	p->thread.tp = (unsigned long)task_stack_page(p);

	return 0;
}

static void __noreturn kernel_thread_helper(void *arg, int(*fn)(void *))
{
	do_exit(fn(arg));
}

long kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct pt_regs kregs;

	memset(&kregs, 0, sizeof(kregs));

	kregs.a[0] = (unsigned long)arg;
	kregs.a[1] = (unsigned long)fn;
	kregs.ra = (unsigned long)kernel_thread_helper;
	kregs.status = (SR_IM | SR_VM | SR_S64 | SR_U64 | SR_S | SR_PS);

	return do_fork(flags | CLONE_VM | CLONE_UNTRACED, 0, &kregs, 0, NULL, NULL);
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return tsk->thread.pc;
}

int kernel_execve(const char *filename,
	char *const argv[], char *const envp[])
{
	int ret;

	__asm__ __volatile__ (
		"move a0, %0;"
		"move a1, %1;"
		"move a2, %2;"
		"syscall;"
		"move %3, v0"
		: "=r" (ret)
		: "r" (filename), "r" (argv), "r" (envp)
		: "memory");
	return ret;
}
