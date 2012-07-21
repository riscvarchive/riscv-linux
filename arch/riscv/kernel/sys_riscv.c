#include <linux/syscalls.h>
#include <linux/linkage.h>

asmlinkage long sys_clone(unsigned long clone_flags,
	unsigned long newsp, void __user *parent_tid,
	void __user *child_tid, struct pt_regs *regs)
{
	return 0;
}

asmlinkage long sys_execve(const char __user *name,
	const char __user * const __user *argv,
	const char __user * const __user *envp, struct pt_regs *regs)
{
	return 0;
}

asmlinkage long sys_mmap(unsigned long addr, unsigned long len,
	unsigned long prot, unsigned long flags,
	unsigned long fd, off_t offset)
{
	return 0;
}
