#include <linux/syscalls.h>
#include <asm/unistd.h>

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	if (unlikely(offset & (~PAGE_MASK)))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd, offset >> PAGE_SHIFT);
}

#if !defined(CONFIG_64BIT)
SYSCALL_DEFINE6(mmap2, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	if (unlikely(offset & (~PAGE_MASK)))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd, offset >> (PAGE_SHIFT - 12));
}
#endif

#ifdef CONFIG_RV_SYSRISCV_ATOMIC
SYSCALL_DEFINE4(sysriscv, unsigned long, cmd, unsigned long, arg1,
	unsigned long, arg2, unsigned long, arg3)
{
	unsigned long flags;
	unsigned long prev;
	unsigned int *ptr;
	unsigned int err;

	switch (cmd) {
	case RISCV_ATOMIC_CMPXCHG:
		ptr = (unsigned int *)arg1;
		if (!access_ok(VERIFY_WRITE, ptr, sizeof(unsigned int)))
			return -EFAULT;

		preempt_disable();
		raw_local_irq_save(flags);
		err = __get_user(prev, ptr);
		if (likely(!err && prev == arg2))
			err = __put_user(arg3, ptr);
		raw_local_irq_restore(flags);
		preempt_enable();

		return unlikely(err) ? err : prev;

	case RISCV_ATOMIC_CMPXCHG64:
		ptr = (unsigned int *)arg1;
		if (!access_ok(VERIFY_WRITE, ptr, sizeof(unsigned long)))
			return -EFAULT;

		preempt_disable();
		raw_local_irq_save(flags);
		err = __get_user(prev, ptr);
		if (likely(!err && prev == arg2))
			err = __put_user(arg3, ptr);
		raw_local_irq_restore(flags);
		preempt_enable();

		return unlikely(err) ? err : prev;
	}

	return -EINVAL;
}
#endif /* CONFIG_RV_SYSRISCV_ATOMIC */
