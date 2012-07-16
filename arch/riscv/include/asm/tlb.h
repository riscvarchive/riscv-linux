#ifndef _ASM_RISCV_TLB_H
#define _ASM_RISCV_TLB_H

#include <asm-generic/tlb.h>

static inline void tlb_flush(struct mmu_gather *tlb)
{
	flush_tlb_mm(tlb->mm);
}

static inline void tlb_start_vma(struct mmu_gather *tlb,
	struct vm_area_struct *vma)
{
}

static inline void tlb_end_vma(struct mmu_gather *tlb,
	struct vm_area_struct *vma)
{
}

/* Remove TLB entry for PTE mapped at a given virtual address */
static inline void __tlb_remove_tlb_entry(struct mmu_gather *tlb,
	pte_t *ptep, unsigned long addr)
{
}

#endif /* _ASM_RISCV_TLB_H */
