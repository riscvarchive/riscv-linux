#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tick.h>

#include <asm/unistd.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/pcr.h>
#include <asm/string.h>

extern asmlinkage void ret_from_fork(void);

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

void start_thread(struct pt_regs * regs, unsigned long pc, 
	unsigned long sp)
{
	unsigned long status;

	/* Remove kernel privileges */
	status = regs->status & ~(SR_PS);

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
	unsigned long unused,
	struct task_struct *p, struct pt_regs *regs)
{
	struct pt_regs *childregs;
	unsigned long childksp;

	childksp = (unsigned long)task_stack_page(p) + THREAD_SIZE;
	childregs = (struct pt_regs *)childksp - 1;

	memcpy(childregs, regs, sizeof(struct pt_regs));

	childregs->sp = (user_mode(regs) ? usp : (unsigned long)childregs);
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

	return do_fork(flags | CLONE_VM | CLONE_UNTRACED,
		0, &kregs, 0, NULL, NULL);
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return tsk->thread.pc;
}

int kernel_execve(const char *filename,
	char *const argv[], char *const envp[])
{
	register unsigned long __a0 __asm__ ("a0");
	register unsigned long __a1 __asm__ ("a1");
	register unsigned long __a2 __asm__ ("a2");
	register int __res __asm__ ("v0");

	__res = __NR_execve;
	__a0 = (unsigned long)(filename);
	__a1 = (unsigned long)(argv);
	__a2 = (unsigned long)(envp);

	__asm__ __volatile__ (
		"syscall;"
		: "+r" (__res)
		: "r" (__a0), "r" (__a1), "r" (__a2)
		: "memory");
	return __res;
}
