#ifndef _ASM_RISCV_TLBFLUSH_H
#define _ASM_RISCV_TLBFLUSH_H

#ifdef CONFIG_MMU

#include <linux/mm.h>
#include <linux/bug.h>
#include <asm/csr.h>

/* Flush all TLB entries */
static inline void flush_tlb_all(void)
{
	__asm__ __volatile__ ("sfence.vm");
}

/* Flush the TLB entries of the specified mm context */
static inline void flush_tlb_mm(struct mm_struct *mm)
{
	flush_tlb_all();
}

/* Flush one page */
static inline void flush_tlb_page(struct vm_area_struct *vma,
	unsigned long addr)
{
	flush_tlb_all();
}

/* Flush a range of pages */
static inline void flush_tlb_range(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	flush_tlb_all();
}

/* Flush a range of kernel pages */
static inline void flush_tlb_kernel_range(unsigned long start,
	unsigned long end)
{
	flush_tlb_all();
}

#else /* !CONFIG_MMU */

static inline void flush_tlb_all(void)
{
	BUG();
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	BUG();
}

static inline void flush_tlb_page(struct vm_area_struct *vma,
	unsigned long addr)
{
	BUG();
}

static inline void flush_tlb_range(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	BUG();
}

static inline void flush_tlb_kernel_range(unsigned long start,
	unsigned long end)
{
	BUG();
}

#endif /* CONFIG_MMU */

#endif /* _ASM_RISCV_TLBFLUSH_H */
