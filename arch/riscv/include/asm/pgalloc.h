#ifndef _ASM_RISCV_PGALLOC_H
#define _ASM_RISCV_PGALLOC_H

/* borrowed from score */

#include <linux/mm.h>
#include <asm/tlb.h>

static inline void pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmd,
	pte_t *pte)
{
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
	pgtable_t pte)
{
}

#ifndef __PAGETABLE_PMD_FOLDED
static inline void pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
}
#endif /* __PAGETABLE_PMD_FOLDED */

#define pmd_pgtable(pmd)	pmd_page(pmd)

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	return NULL;
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
}

#ifndef __PAGETABLE_PMD_FOLDED

static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	return NULL;
}

static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
}

#endif /* __PAGETABLE_PMD_FOLDED */

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
	unsigned long address)
{
	return NULL;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm,
	unsigned long address)
{
	return NULL;
}

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
}

#define __pte_free_tlb(tlb, pte, buf)   \
do {                                    \
	pgtable_page_dtor(pte);             \
	tlb_remove_page((tlb), pte);        \
} while (0)

#ifndef __PAGETABLE_PMD_FOLDED
#define __pmd_free_tlb(tlb, pmd, addr)  pmd_free((tlb)->mm, pmd)
#endif /* __PAGETABLE_PMD_FOLDED */

static inline void check_pgt_cache(void)
{
}

#endif /* _ASM_RISCV_PGALLOC_H */
