/*
 * {read,write}{b,w,l,q} based on arch/arm64/include/asm/io.h
 *   which was based on arch/arm/include/io.h
 *
 * Copyright (C) 1996-2000 Russell King
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2014 Regents of the University of California
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

#ifndef _ASM_RISCV_IO_H
#define _ASM_RISCV_IO_H

#ifdef CONFIG_MMU

extern void __iomem *ioremap(phys_addr_t offset, unsigned long size);

/*
 * The RISC-V ISA doesn't yet specify how to query of modify PMAs, so we can't
 * change the properties of memory regions.  This should be fixed by the
 * upcoming platform spec.
 */
#define ioremap_nocache(addr, size) ioremap((addr), (size))
#define ioremap_wc(addr, size) ioremap((addr), (size))
#define ioremap_wt(addr, size) ioremap((addr), (size))

extern void iounmap(void __iomem *addr);

#endif /* CONFIG_MMU */

/* Generic IO read/write.  These perform native-endian accesses. */
#define __raw_writeb __raw_writeb
static inline void __raw_writeb(u8 val, volatile void __iomem *addr)
{
	asm volatile("sb %0, 0(%1)" : : "r" (val), "r" (addr));
}

#define __raw_writew __raw_writew
static inline void __raw_writew(u16 val, volatile void __iomem *addr)
{
	asm volatile("sh %0, 0(%1)" : : "r" (val), "r" (addr));
}

#define __raw_writel __raw_writel
static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
	asm volatile("sw %0, 0(%1)" : : "r" (val), "r" (addr));
}

#ifdef __riscv64
#define __raw_writeq __raw_writeq
static inline void __raw_writeq(u64 val, volatile void __iomem *addr)
{
	asm volatile("sd %0, 0(%1)" : : "r" (val), "r" (addr));
}
#endif

#define __raw_readb __raw_readb
static inline u8 __raw_readb(const volatile void __iomem *addr)
{
	u8 val;

	asm volatile("lb %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

#define __raw_readw __raw_readw
static inline u16 __raw_readw(const volatile void __iomem *addr)
{
	u16 val;

	asm volatile("lh %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

#define __raw_readl __raw_readl
static inline u32 __raw_readl(const volatile void __iomem *addr)
{
	u32 val;

	asm volatile("lw %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

#ifdef __riscv64
#define __raw_readq __raw_readq
static inline u64 __raw_readq(const volatile void __iomem *addr)
{
	u64 val;

	asm volatile("ld %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}
#endif

/*
 * FIXME: I'm flip-flopping on whether or not we should keep this or enforce
 * the ordering with I/O on spinlocks.  The worry is that drivers won't get
 * this correct, but I also don't want to introduce a fence into the lock code
 * that otherwise only uses AMOs and LR/SC.   For now I'm leaving this here:
 * "w,o" is sufficient to ensure that all writes to the device has completed
 * before the write to the spinlock is allowed to commit.  I surmised this from
 * reading "ACQUIRES VS I/O ACCESSES" in memory-barriers.txt.
 */
#define mmiowb()	__asm__ __volatile__ ("fence o,w" : : : "memory");

/*
 * Unordered I/O memory access primitives.  These are even more relaxed than
 * the relaxed versions, as they don't even order accesses between successive
 * operations to the I/O regions.
 */
#define readb_cpu(c)		({ u8  __r = __raw_readb(c); __r; })
#define readw_cpu(c)		({ u16 __r = le16_to_cpu((__force __le16)__raw_readw(c)); __r; })
#define readl_cpu(c)		({ u32 __r = le32_to_cpu((__force __le32)__raw_readl(c)); __r; })
#define readq_cpu(c)		({ u64 __r = le64_to_cpu((__force __le64)__raw_readq(c)); __r; })

#define writeb_cpu(v,c)		((void)__raw_writeb((v),(c)))
#define writew_cpu(v,c)		((void)__raw_writew((__force u16)cpu_to_le16(v),(c)))
#define writel_cpu(v,c)		((void)__raw_writel((__force u32)cpu_to_le32(v),(c)))
#define writeq_cpu(v,c)		((void)__raw_writeq((__force u64)cpu_to_le64(v),(c)))

/*
 * Relaxed I/O memory access primitives. These follow the Device memory
 * ordering rules but do not guarantee any ordering relative to Normal memory
 * accesses.  The memory barriers here are necessary as RISC-V doesn't define
 * any ordering constraints on accesses to the device I/O space.  These are
 * defined to order the indicated access (either a read or write) with all
 * other I/O memory accesses.
 */
/*
 * FIXME: The platform spec will define the default Linux-capable platform to
 * have some extra IO ordering constraints that will make these fences
 * unnecessary.
 */
#define __iorrmb()	__asm__ __volatile__ ("fence i,io" : : : "memory");
#define __iorwmb()	__asm__ __volatile__ ("fence io,o" : : : "memory");

#define readb_relaxed(c)	({ u8  __v = readb_cpu(c); __iorrmb(); __v; })
#define readw_relaxed(c)	({ u16 __v = readw_cpu(c); __iorrmb(); __v; })
#define readl_relaxed(c)	({ u32 __v = readl_cpu(c); __iorrmb(); __v; })
#define readq_relaxed(c)	({ u64 __v = readq_cpu(c); __iorrmb(); __v; })

#define writeb_relaxed(v,c)	({ __iorwmb(); writeb_cpu((v),(c)); })
#define writew_relaxed(v,c)	({ __iorwmb(); writew_cpu((v),(c)); })
#define writel_relaxed(v,c)	({ __iorwmb(); writel_cpu((v),(c)); })
#define writeq_relaxed(v,c)	({ __iorwmb(); writeq_cpu((v),(c)); })

/*
 * I/O memory access primitives. Reads are ordered relative to any
 * following Normal memory access. Writes are ordered relative to any prior
 * Normal memory access.  The memory barriers here are necessary as RISC-V
 * doesn't define any ordering between the memory space and the I/O space.
 * They may be stronger than necessary ("i,r" and "w,o" might be sufficient),
 * but I feel kind of queasy making these weaker in any manner than the relaxed
 * versions above.
 */
#define __iormb()	__asm__ __volatile__ ("fence i,ior" : : : "memory");
#define __iowmb()	__asm__ __volatile__ ("fence iow,o" : : : "memory");

#define readb(c)		({ u8  __v = readb_cpu(c); __iormb(); __v; })
#define readw(c)		({ u16 __v = readw_cpu(c); __iormb(); __v; })
#define readl(c)		({ u32 __v = readl_cpu(c); __iormb(); __v; })
#define readq(c)		({ u64 __v = readq_cpu(c); __iormb(); __v; })

#define writeb(v,c)		({ __iowmb(); writeb_cpu((v),(c)); })
#define writew(v,c)		({ __iowmb(); writew_cpu((v),(c)); })
#define writel(v,c)		({ __iowmb(); writel_cpu((v),(c)); })
#define writeq(v,c)		({ __iowmb(); writeq_cpu((v),(c)); })

#include <asm-generic/io.h>

#endif /* _ASM_RISCV_IO_H */
