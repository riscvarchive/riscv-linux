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

#ifndef _ASM_RISCV_IO_H
#define _ASM_RISCV_IO_H

#include <asm-generic/io.h>

#ifdef __KERNEL__

#ifdef CONFIG_MMU

extern void __iomem *ioremap(phys_addr_t offset, unsigned long size);

#define ioremap_nocache(addr, size) ioremap((addr), (size))
#define ioremap_wc(addr, size) ioremap((addr), (size))
#define ioremap_wt(addr, size) ioremap((addr), (size))

extern void iounmap(void __iomem *addr);

#endif /* CONFIG_MMU */

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_IO_H */
