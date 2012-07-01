#ifndef _ASM_RISCV_PGTABLE_H
#define _ASM_RISCV_PGTABLE_H

#ifndef __ASSEMBLY__
#ifdef CONFIG_MMU

#include <asm-generic/pgtable-nopud.h>
#include <asm/pgtable-bits.h>

#include <linux/mm_types.h>
#include <linux/bug.h>

/* 
 * define PAGE_SHIFT 13 (64bit)
 * define PAGE_SIZE (1 << PAGE_SHIFT) 
 */
#define PMD_SHIFT   23
#define PGDIR_SHIFT 33

#define PMD_SIZE    (1UL << PMD_SHIFT)
#define PMD_MASK    (~(PMD_SIZE-1))

#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define VMALLOC_START (PAGE_OFFSET - 0x4000000)
#define VMALLOC_END (PAGE_OFFSET)

#define pgd_index(address)	(((address) >> PGDIR_SHIFT) & (PTRS_PER_PGD-1))

static inline int pud_present(pud_t pud)
{ 
	return pud_val(pud) & _PAGE_PRESENT;
}

static inline int pmd_present(pmd_t pmd)
{
	return pmd_val(pmd) & _PAGE_PRESENT;
}

static inline int pte_present(pte_t pte)
{ 
	return pte_val(pte) & _PAGE_PRESENT;
}

static inline int pud_none(pud_t pud)
{
	return !(pud_val(pud) & _PAGE_PRESENT);
}

static inline int pmd_none(pmd_t pmd)
{
	return !(pmd_val(pmd) & _PAGE_PRESENT);
}

static inline int pte_none(pte_t pte)
{
	return !(pte_val(pte) & _PAGE_PRESENT);
}

/*
 * Certain architectures need to do special things when PTEs
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
static inline void set_pte_at(struct mm_struct *mm, unsigned long addr, 
	pte_t *ptep, pte_t entry) 
{
	*(ptep) = entry;
}

static inline void pte_clear(struct mm_struct *mm, unsigned long addr, 
	pte_t *ptep)
{
	set_pte_at(mm, addr, ptep, __pte(0));
}

static inline int pud_bad(pud_t pud)
{
	return (pud_val(pud) & ~PAGE_MASK); 
}

static inline int pmd_bad(pmd_t pmd)
{
	return (pmd_val(pmd) & ~PAGE_MASK);
}

#define pmd_page(pmd) (pfn_to_page(pmd_phys(pmd) >> PAGE_SHIFT))
#define pte_page(x) pfn_to_page(pte_pfn(x))

/* pte_present */

/* pte_read */

static inline int pte_write(pte_t pte)
{
	return pte_val(pte) & _SUPERVISOR_WRITABLE;
}

static inline int pte_dirty(pte_t pte)
{
	return pte_val(pte) & _PAGE_DIRTY;
}

/* What is this? */
static inline int pte_file(pte_t pte)
{
	return 0;
}

static inline int pte_young(pte_t pte)
{
	return pte_val(pte) & _PAGE_REFERENCED;
}

/* pte_rdprotect */

static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_USER_WRITABLE));
}

/* pte_mkread */

static inline pte_t pte_mkwrite(pte_t pte)
{
	return __pte(pte_val(pte) | _USER_WRITABLE);
}

/* pte_mkexec */

static inline pte_t pte_mkdirty(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_DIRTY);
}

static inline pte_t pte_mkclean(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_DIRTY));
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_REFERENCED)); 
}

static inline pte_t pte_mkold(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_REFERENCED);
}

static inline int pte_special(pte_t pte)
{
	return pte_val(pte) & _PAGE_SPECIAL;
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SPECIAL);
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return pte;
}

#define mk_pte(page, prot)	pfn_pte(page_to_pfn(page), prot)
#define mk_pte_phys(physpage, pgprot) \
({                                                                      \
  pte_t __pte;                                                    \
                  \
  pte_val(__pte) = (physpage) + pgprot_val(pgprot);               \
  __pte;                                                          \
})

#define pmd_alloc_one(mm, address) NULL
#define pgd_alloc_one(mm, address) NULL

static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd) {}
/* pgd_free */

/* the done line ==================================================== */

/* Justification for 0x4000000 = 64 MB */

static inline void pgtable_cache_init(void) {}

static inline pmd_t* pmd_offset(pud_t *pud, unsigned long address) { return (pmd_t*)0; }

extern unsigned long empty_zero_page;
extern unsigned long zero_page_mask;

extern long arch_ptrace(struct task_struct *child, long request,
     unsigned long addr, unsigned long data);
extern void ptrace_disable(struct task_struct *task);

/* I don't understand this! */
#define PAGE_KERNEL   __pgprot(_SUPERVISOR_READABLE | _SUPERVISOR_WRITABLE | _SUPERVISOR_EXECUTABLE)
#define PAGE_NONE     __pgprot(0)
#define PAGE_COPY     __pgprot(1)
#define PAGE_SHARED   __pgprot(1)
#define PAGE_READONLY __pgprot(_SUPERVISOR_READABLE | _USER_READABLE)
#define PAGE_KERNEL_UNCACHED __pgprot(1)

/* private */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

/* shared */
#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED

#define FIRST_USER_ADDRESS	0

#define ZERO_PAGE(vaddr) \
	(virt_to_page((void *)(empty_zero_page + \
	 (((unsigned long)(vaddr)) & zero_page_mask))))

#define PTE_FILE_MAX_BITS	30
#define pte_to_pgoff(_pte)		\
	(((_pte).pte & 0x1ff) | (((_pte).pte >> 11) << 9))
#define pgoff_to_pte(off)		\
	((pte_t) {((off) & 0x1ff) | (((off) >> 9) << 11) | (1 << 10)})
#define __pte_to_swp_entry(pte)		\
	((swp_entry_t) { pte_val(pte)})
#define pte_unmap(pte) ((void)(pte))
#define __swp_entry_to_pte(x)	((pte_t) {(x).val})
#define __swp_type(x)		((x).val & 0x1f)
#define __swp_offset(x) 	((x).val >> 11)
#define __swp_entry(type, offset) ((swp_entry_t){(type) | ((offset) << 11)})



#define pgd_ERROR(e) \
	printk(KERN_ERR "%s:%d: bad pgd %08lx.\n", \
		__FILE__, __LINE__, pgd_val(e))

#define pud_clear(pudp)		do { } while (0)

#define pmd_ERROR(e) \
	printk(KERN_ERR "%s:%d: bad pmd %08lx.\n", \
		__FILE__, __LINE__, pmd_val(e))
#define pmd_clear(pmdp)		do { } while (0)

#define pmd_phys(pmd)		__pa((void *)pmd_val(pmd))


#define pfn_pte(pfn,pgprot) mk_pte_phys(__pa((pfn) << PAGE_SHIFT),(pgprot))
#define pte_pfn(x) (pte_val(x) >> PAGE_SHIFT)

#define __pgd_offset(address)	pgd_index(address)
#define __pmd_offset(address)	(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

#define pgd_offset_k(address)	pgd_offset(&init_mm, address)
#define pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))

#define __pte_offset(address)		\
	(((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset(dir, address)	\
	((pte_t *) pmd_page_vaddr(*(dir)) + __pte_offset(address))
#define pte_offset_kernel(dir, address)	\
	((pte_t *) pmd_page_vaddr(*(dir)) + __pte_offset(address))

#define pte_offset_map(dir, address)	\
	((pte_t *)page_address(pmd_page(*(dir))) + __pte_offset(address))
#define pte_unmap(pte) ((void)(pte))

#define pmd_page_vaddr(pmd)	pmd_val(pmd)


#define PTRS_PER_PGD	1024
#define PTRS_PER_PTE	1024

static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
}

#endif /* CONFIG_MMU */

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

#endif /* !__ASSEMBLY__ */

#include <asm-generic/pgtable.h>

#endif /* _ASM_RISCV_PGTABLE_H */
