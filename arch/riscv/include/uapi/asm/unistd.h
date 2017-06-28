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

/* FIXME: This exists for now in order to maintain compatibility with our
 * pre-upstream glibc, and will be removed for our real Linux submission.
 */
#define __ARCH_WANT_RENAMEAT

#include <asm-generic/unistd.h>

/*
 * These system calls add support for AMOs on RISC-V systems without support
 * for the A extension.
 */
#define __NR_sysriscv_cmpxchg32		(__NR_arch_specific_syscall + 0)
#define __NR_sysriscv_cmpxchg64		(__NR_arch_specific_syscall + 1)
