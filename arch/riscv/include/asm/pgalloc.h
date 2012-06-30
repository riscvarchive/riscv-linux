#ifndef _ASM_RISCV_PGALLOC_H
#define _ASM_RISCV_PGALLOC_H

/* borrowed from score */

#include <linux/mm.h>

static inline void pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmd,
	pte_t *pte)
{
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
	pgtable_t pte)
{
}

static inline void pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
}

#define pmd_pgtable(pmd)	pmd_page(pmd)

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	return (pgd_t*)0;
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
}

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
	unsigned long address)
{
	return (pte_t*)0;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm,
	unsigned long address)
{
	return (struct page*)0;
}

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
}

#define __pte_free_tlb(tlb, pte, buf)			\
do {							\
	pgtable_page_dtor(pte);				\
	tlb_remove_page((tlb), pte);			\
} while (0)

#define __pmd_free_tlb(tlb, x, addr)    pmd_free((tlb)->mm, x)

#define check_pgt_cache()          do { } while (0)

#endif /* _ASM_RISCV_PGALLOC_H */
