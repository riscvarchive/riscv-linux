#ifndef _ASM_RISCV_TLB_H
#define _ASM_RISCV_TLB_H

#include <asm-generic/tlb.h>

static inline void tlb_flush(struct mmu_gather *tlb)
{
	flush_tlb_mm(tlb->mm);
}

#endif /* _ASM_RISCV_TLB_H */
