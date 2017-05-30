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

#ifdef __KERNEL__

#ifdef CONFIG_MMU

extern void __iomem *ioremap(phys_addr_t offset, unsigned long size);

#define ioremap_nocache(addr, size) ioremap((addr), (size))
#define ioremap_wc(addr, size) ioremap((addr), (size))
#define ioremap_wt(addr, size) ioremap((addr), (size))

extern void iounmap(void __iomem *addr);

#endif /* CONFIG_MMU */

/*
 * Generic IO read/write.  These perform native-endian accesses.
 */
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

/* IO barriers.  These only fence on the IO bits because they're only required
 * to order device access.  We're defining mmiowb because our AMO instructions
 * (which are used to implement locks) don't specify ordering.  From Chapter 7
 * of v2.2 of the user ISA:
 * "The bits order accesses to one of the two address domains, memory or I/O,
 * depending on which address domain the atomic instruction is accessing. No
 * ordering constraint is implied to accesses to the other domain, and a FENCE
 * instruction should be used to order across both domains."
 */

#define __iormb()               __asm__ __volatile__ ("fence i,io" : : : "memory");
#define __iowmb()               __asm__ __volatile__ ("fence io,o" : : : "memory");

#define mmiowb()                __asm__ __volatile__ ("fence io,io" : : : "memory");

/*
 * Relaxed I/O memory access primitives. These follow the Device memory
 * ordering rules but do not guarantee any ordering relative to Normal memory
 * accesses.
 */
#define readb_relaxed(c)        ({ u8  __r = __raw_readb(c); __r; })
#define readw_relaxed(c)        ({ u16 __r = le16_to_cpu((__force __le16)__raw_readw(c)); __r; })
#define readl_relaxed(c)        ({ u32 __r = le32_to_cpu((__force __le32)__raw_readl(c)); __r; })
#define readq_relaxed(c)        ({ u64 __r = le64_to_cpu((__force __le64)__raw_readq(c)); __r; })

#define writeb_relaxed(v,c)     ((void)__raw_writeb((v),(c)))
#define writew_relaxed(v,c)     ((void)__raw_writew((__force u16)cpu_to_le16(v),(c)))
#define writel_relaxed(v,c)     ((void)__raw_writel((__force u32)cpu_to_le32(v),(c)))
#define writeq_relaxed(v,c)     ((void)__raw_writeq((__force u64)cpu_to_le64(v),(c)))

/*
 * I/O memory access primitives. Reads are ordered relative to any
 * following Normal memory access. Writes are ordered relative to any prior
 * Normal memory access.
 */
#define readb(c)                ({ u8  __v = readb_relaxed(c); __iormb(); __v; })
#define readw(c)                ({ u16 __v = readw_relaxed(c); __iormb(); __v; })
#define readl(c)                ({ u32 __v = readl_relaxed(c); __iormb(); __v; })
#define readq(c)                ({ u64 __v = readq_relaxed(c); __iormb(); __v; })

#define writeb(v,c)             ({ __iowmb(); writeb_relaxed((v),(c)); })
#define writew(v,c)             ({ __iowmb(); writew_relaxed((v),(c)); })
#define writel(v,c)             ({ __iowmb(); writel_relaxed((v),(c)); })
#define writeq(v,c)             ({ __iowmb(); writeq_relaxed((v),(c)); })

#include <asm-generic/io.h>

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_IO_H */
