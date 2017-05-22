/*
 * Copyright (C) 2013 Regents of the University of California
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

#ifndef _ASM_RISCV_STRING_H
#define _ASM_RISCV_STRING_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/linkage.h>

#define __HAVE_ARCH_MEMSET
extern asmlinkage void *memset(void *, int, size_t);

#define __HAVE_ARCH_MEMCPY
extern asmlinkage void *memcpy(void *, const void *, size_t);

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_STRING_H */
