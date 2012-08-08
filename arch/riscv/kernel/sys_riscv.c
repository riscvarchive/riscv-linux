#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/linkage.h>

asmlinkage long __sys_clone(unsigned long clone_flags,
	unsigned long newsp, void __user *parent_tid,
	void __user *child_tid, struct pt_regs *regs)
{
	return 0;
}

asmlinkage long __sys_execve(const char __user *name,
	const char __user * const __user *argv,
	const char __user * const __user *envp, struct pt_regs *regs)
{
	long error;
	char *filename;

	filename = getname(name);
	if (IS_ERR(filename))
		return PTR_ERR(filename);

	error = do_execve(filename, argv, envp, regs);
	putname(filename);
	return error;
}

asmlinkage long sys_mmap(unsigned long addr, unsigned long len,
	unsigned long prot, unsigned long flags,
	unsigned long fd, off_t offset)
{
	return 0;
}

asmlinkage long sys_fork(struct pt_regs *regs)
{
	return do_fork(SIGCHLD, regs->sp, regs, 0, NULL, NULL);
}

asmlinkage long sys_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD,
		regs->sp, regs, 0, NULL, NULL);
}
