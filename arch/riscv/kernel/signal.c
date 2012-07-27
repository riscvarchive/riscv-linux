#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm/syscalls.h>

asmlinkage long __sys_sigaltstack(const stack_t __user *uss,
	stack_t __user *uoss, struct pt_regs *regs)
{
	return 0;
}

asmlinkage long __sys_rt_sigreturn(struct pt_regs *regs)
{
	return 0;
}
