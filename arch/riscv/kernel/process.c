#include <linux/kernel.h>
#include <asm/processor.h>

void __noreturn cpu_idle(void)
{
	for (;;) {
	}
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
	return 0;
}

long kernel_thread(int (*fn)(void *), void *arg,unsigned long flags)
{
	return 0;
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return 0;
}

int kernel_execve(const char *filename,
	char *const argv[], char *const envp[])
{
	return 0;
}
