/*
 * Copyright (C) 2014 Darius Rad <darius@bluespec.com>
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

#ifndef _ASM_RISCV_SYSCALLS_H
#define _ASM_RISCV_SYSCALLS_H

#include <linux/linkage.h>

#include <asm-generic/syscalls.h>

/* kernel/sys_riscv.c */
asmlinkage long sys_sysriscv_cmpxchg32(u32 __user * ptr, u32 new, u32 old);
asmlinkage long sys_sysriscv_cmpxchg64(u64 __user * ptr, u64 new, u64 old);

#endif /* _ASM_RISCV_SYSCALLS_H */
