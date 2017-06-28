/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2014 Darius Rad <darius@bluespec.com>
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <linux/syscalls.h>
#include <asm/cmpxchg.h>
#include <asm/unistd.h>

#ifdef CONFIG_64BIT
SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	if (unlikely(offset & (~PAGE_MASK)))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd, offset >> PAGE_SHIFT);
}
#else
SYSCALL_DEFINE6(mmap2, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags,
	unsigned long, fd, off_t, offset)
{
	/*
	 * Note that the shift for mmap2 is constant (12),
	 * regardless of PAGE_SIZE
	 */
	if (unlikely(offset & (~PAGE_MASK >> 12)))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd,
		offset >> (PAGE_SHIFT - 12));
}
#endif /* !CONFIG_64BIT */

SYSCALL_DEFINE3(sysriscv_cmpxchg32, u32 __user *, ptr, u32, new, u32, old)
{
	u32 prev;
	unsigned int err;

	if (!access_ok(VERIFY_WRITE, ptr, sizeof(*ptr)))
		return -EFAULT;

#ifdef CONFIG_ISA_A
	err = 0;
	prev = cmpxchg32(ptr, old, new);
#else
	preempt_disable();
	err = __get_user(prev, ptr);
	if (likely(!err && prev == old))
		err = __put_user(new, ptr);
	preempt_enable();
#endif

	return unlikely(err) ? err : prev;
}

SYSCALL_DEFINE3(sysriscv_cmpxchg64, u64 __user *, ptr, u64, new, u64, old)
{
#ifdef CONFIG_64BIT
	u64 prev;
	unsigned int err;

	if (!access_ok(VERIFY_WRITE, ptr, sizeof(*ptr)))
		return -EFAULT;

#ifdef CONFIG_ISA_A
	err = 0;
	prev = cmpxchg64(ptr, old, new);
#else
	preempt_disable();
	err = __get_user(prev, ptr);
	if (likely(!err && prev == old))
		err = __put_user(new, ptr);
	preempt_enable();
#endif
	return unlikely(err) ? err : prev;
#else
	return -ENOTSUPP;
#endif
}
