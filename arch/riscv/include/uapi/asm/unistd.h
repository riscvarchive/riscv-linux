/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <asm-generic/unistd.h>

#define __NR_sysriscv  __NR_arch_specific_syscall
#ifndef __riscv_atomic
__SYSCALL(__NR_sysriscv, sys_sysriscv)
#endif

#define RISCV_ATOMIC_CMPXCHG    1
#define RISCV_ATOMIC_CMPXCHG64  2
