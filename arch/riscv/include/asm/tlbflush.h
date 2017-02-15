#ifndef _ASM_RISCV_TLBFLUSH_H
#define _ASM_RISCV_TLBFLUSH_H

#ifdef CONFIG_MMU

#include <linux/mm.h>
#include <linux/bug.h>
#include <asm/csr.h>

/* Flush entire local TLB */
static inline void local_flush_tlb_all(void)
{
	__asm__ __volatile__ ("sfence.vma");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
	__asm__ __volatile__ ("sfence.vma %0" : : "r" (addr));
}

#ifndef CONFIG_SMP

#define flush_tlb_all() local_flush_tlb_all()
#define flush_tlb_page(vma, addr) local_flush_tlb_page(addr)
#define flush_tlb_range(vma, start, end) local_flush_tlb_all()

#else /* CONFIG_SMP */

#include <asm/sbi.h>

#define flush_tlb_all() sbi_remote_sfence_vm(0, 0)
#define flush_tlb_page(vma, addr) flush_tlb_range(vma, (addr), (addr) + 1)
#define flush_tlb_range(vma, start, end) \
	sbi_remote_sfence_vm_range(0, 0, (start), (end) - (start))

#endif /* CONFIG_SMP */

/* Flush the TLB entries of the specified mm context */
static inline void flush_tlb_mm(struct mm_struct *mm)
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
