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

/*
 * ioremap_nocache     -   map bus memory into CPU space
 * @offset:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap_nocache performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 *
 * This version of ioremap ensures that the memory is marked uncachable
 * on the CPU as well as honouring existing caching rules from things like
 * the PCI bus. Note that there are other caches and buffers on many
 * busses. In particular driver authors should read up on PCI writes.
 *
 * It's useful if some control registers are in such an area and
 * write combining or read caching is not desirable.
 *
 * Must be freed with iounmap.
 */
static inline void __iomem *ioremap_nocache(
	phys_addr_t offset, unsigned long size)
{
	return ioremap(offset, size);
}

/**
 * ioremap_wc	-	map memory into CPU space write combined
 * @offset:	bus address of the memory
 * @size:	size of the resource to map
 *
 * This version of ioremap ensures that the memory is marked write combining.
 * Write combining allows faster writes to some hardware devices.
 *
 * Must be freed with iounmap.
 */
static inline void __iomem *ioremap_wc(
	phys_addr_t offset, unsigned long size)
{
	return ioremap(offset, size);
}

#define ioremap_wt ioremap_nocache

extern void iounmap(void __iomem *addr);

#endif /* CONFIG_MMU */

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_IO_H */
