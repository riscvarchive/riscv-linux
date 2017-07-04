/*
 * Copyright (C) 2012 Regents of the University of California
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

#include <asm-generic/unistd.h>

/*
 * These system calls add support for AMOs on RISC-V systems without support
 * for the A extension.
 */
#define __NR_sysriscv_cmpxchg32		(__NR_arch_specific_syscall + 0)
__SYSCALL(__NR_sysriscv_cmpxchg32, sys_sysriscv_cmpxchg32)
#define __NR_sysriscv_cmpxchg64		(__NR_arch_specific_syscall + 1)
__SYSCALL(__NR_sysriscv_cmpxchg64, sys_sysriscv_cmpxchg64)
