#ifndef _ASM_RISCV_PGTABLE_H
#define _ASM_RISCV_PGTABLE_H

#ifndef __ASSEMBLY__
#ifdef CONFIG_MMU

#include <linux/mm_types.h>
#include <linux/bug.h>

/* Justification for 0x4000000 = 64 MB */
#define VMALLOC_START (PAGE_OFFSET - 0x4000000)
#define VMALLOC_END (PAGE_OFFSET)

static inline int pud_none(pud_t pud)	   { return 0; }
static inline int pgd_none(pgd_t pgd)    { return 0; }
static inline pte_t pte_mkwrite(pte_t pte) { return pte; }
static inline pud_t* pud_offset(pgd_t *pgd, unsigned long address) { return (pud_t*)0; }
static inline pmd_t* pmd_offset(pud_t *pud, unsigned long address) { return (pmd_t*)0; }

#include <asm/pgalloc.h>
static inline void pgtable_cache_init(void) {}

static inline int pgd_present(pgd_t pgd) { return 1; }
static inline int pgd_bad(pgd_t pgd)     { return 0; }

#define pmd_alloc_one(mm, address) NULL
#define pud_alloc_one(mm, address) NULL
#define pgd_alloc_one(mm, address) NULL
static inline int pud_present(pud_t pud) { return 1; }
static inline int pud_bad(pud_t pud)	   { return 0; }

static inline int pmd_present(pmd_t pmd) { return 1; }
static inline int pmd_none(pmd_t pmd)	   { return 0; }
static inline int pmd_bad(pmd_t pmd)	   { return 0; }

static inline int pte_young(pte_t pte) { return 0; }
static inline int pte_none(pte_t pte)  { return 0; }
static inline int pte_present(pte_t pte)  { return 0; }
static inline int pte_file(pte_t pte)  { return 0; }
static inline int pte_dirty(pte_t pte)  { return 0; }
static inline int pte_special(pte_t pte) {return 0;}
static inline pte_t pte_mkspecial(pte_t pte) {return pte;}
static inline pte_t pte_mkclean(pte_t pte) {return pte;}
static inline pte_t pte_mkyoung(pte_t pte) {return pte;}
static inline pte_t pte_mkdirty(pte_t pte) {return pte;}
static inline int pte_write(pte_t pte) { return 0; }
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot) { return pte; }


extern unsigned long empty_zero_page;
extern unsigned long zero_page_mask;

extern long arch_ptrace(struct task_struct *child, long request,
     unsigned long addr, unsigned long data);
extern void ptrace_disable(struct task_struct *task);

#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

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


#define PAGE_KERNEL __pgprot(1)
#define PAGE_NONE __pgprot(1)
#define PAGE_COPY __pgprot(1)
#define PAGE_SHARED __pgprot(1)
#define PAGE_READONLY __pgprot(1)
#define PAGE_KERNEL_UNCACHED __pgprot(1)

#define pgd_ERROR(e) \
	printk(KERN_ERR "%s:%d: bad pgd %08lx.\n", \
		__FILE__, __LINE__, pgd_val(e))
#define pgd_clear(pgdp)		do { } while (0)

#define pud_ERROR(e) \
	printk(KERN_ERR "%s:%d: bad pud %08lx.\n", \
		__FILE__, __LINE__, pud_val(e))
#define pud_clear(pudp)		do { } while (0)

#define pmd_ERROR(e) \
	printk(KERN_ERR "%s:%d: bad pmd %08lx.\n", \
		__FILE__, __LINE__, pmd_val(e))
#define pmd_clear(pmdp)		do { } while (0)

#define pmd_phys(pmd)		__pa((void *)pmd_val(pmd))
#define pmd_page(pmd)		(pfn_to_page(pmd_phys(pmd) >> PAGE_SHIFT))
#define mk_pte(page, prot)	pfn_pte(page_to_pfn(page), prot)
#define mk_pte_phys(physpage, pgprot) \
({                                                                      \
  pte_t __pte;                                                    \
                  \
  pte_val(__pte) = (physpage) + pgprot_val(pgprot);               \
  __pte;                                                          \
})


#define PGD_SHIFT 9
#define PMD_SHIFT	20
#define PUD_SHIFT	31
#define PGDIR_SHIFT	42

#define PGD_SIZE  (1UL << PGD_SHIFT)
#define PGD_MASK  (~(PGD_SIZE-1))
#define PMD_SIZE        (1UL << PMD_SHIFT)
#define PMD_MASK        (~(PMD_SIZE-1))
#define PUD_SIZE	(1UL << PUD_SHIFT)
#define PUD_MASK	(~(PUD_SIZE-1))
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define pfn_pte(pfn,pgprot) mk_pte_phys(__pa((pfn) << PAGE_SHIFT),(pgprot))
#define pte_pfn(x) (pte_val(x) >> PAGE_SHIFT)
#define pte_page(x) pfn_to_page(pte_pfn(x))

#define __pgd_offset(address)	pgd_index(address)
#define __pud_offset(address)	(((address) >> PUD_SHIFT) & (PTRS_PER_PUD-1))
#define __pmd_offset(address)	(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

#define pgd_offset_k(address)	pgd_offset(&init_mm, address)
#define pgd_index(address)	(((address) >> PGDIR_SHIFT) & (PTRS_PER_PGD-1))
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

static inline void pud_free(struct mm_struct *mm, pud_t *pud) {}
static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd) {}

#define PTRS_PER_PGD	1024
#define PTRS_PER_PTE	1024

static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
}

/*
 * Certain architectures need to do special things when PTEs
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t entry)
{
}

static inline pte_t pte_mkold(pte_t pte)
{
	return pte;
}

static inline void pte_clear(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
}

static inline pte_t pte_wrprotect(pte_t pte)
{
	return pte;
}



#ifndef __HAVE_ARCH_PTEP_SET_ACCESS_FLAGS
extern int ptep_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pte_t *ptep,
				 pte_t entry, int dirty);
#endif

#ifndef __HAVE_ARCH_PMDP_SET_ACCESS_FLAGS
extern int pmdp_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pmd_t *pmdp,
				 pmd_t entry, int dirty);
#endif


#ifndef __HAVE_ARCH_PTEP_CLEAR_YOUNG_FLUSH
int ptep_clear_flush_young(struct vm_area_struct *vma,
			   unsigned long address, pte_t *ptep);
#endif

#ifndef __HAVE_ARCH_PMDP_CLEAR_YOUNG_FLUSH
int pmdp_clear_flush_young(struct vm_area_struct *vma,
			   unsigned long address, pmd_t *pmdp);
#endif

#ifndef __HAVE_ARCH_PMDP_GET_AND_CLEAR
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
static inline pmd_t pmdp_get_and_clear(struct mm_struct *mm,
				       unsigned long address,
				       pmd_t *pmdp)
{
	pmd_t pmd = *pmdp;
	pmd_clear(mm, address, pmdp);
	return pmd;
}
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */
#endif

#ifndef __HAVE_ARCH_PTEP_CLEAR_FLUSH
extern pte_t ptep_clear_flush(struct vm_area_struct *vma,
			      unsigned long address,
			      pte_t *ptep);
#endif

#ifndef __HAVE_ARCH_PMDP_CLEAR_FLUSH
extern pmd_t pmdp_clear_flush(struct vm_area_struct *vma,
			      unsigned long address,
			      pmd_t *pmdp);
#endif

#ifndef __HAVE_ARCH_PMDP_SPLITTING_FLUSH
extern pmd_t pmdp_splitting_flush(struct vm_area_struct *vma,
				  unsigned long address,
				  pmd_t *pmdp);
#endif

#ifndef __HAVE_ARCH_PAGE_TEST_AND_CLEAR_DIRTY
#define page_test_and_clear_dirty(pfn, mapped)	(0)
#endif

#ifndef __HAVE_ARCH_PAGE_TEST_AND_CLEAR_DIRTY
#define pte_maybe_dirty(pte)		pte_dirty(pte)
#else
#define pte_maybe_dirty(pte)		(1)
#endif

#ifndef __HAVE_ARCH_PAGE_TEST_AND_CLEAR_YOUNG
#define page_test_and_clear_young(pfn) (0)
#endif

#ifndef __HAVE_ARCH_PGD_OFFSET_GATE
#define pgd_offset_gate(mm, addr)	pgd_offset(mm, addr)
#endif

#ifndef __HAVE_ARCH_MOVE_PTE
#define move_pte(pte, prot, old_addr, new_addr)	(pte)
#endif

#ifndef flush_tlb_fix_spurious_fault
#define flush_tlb_fix_spurious_fault(vma, address) flush_tlb_page(vma, address)
#endif

#ifndef pgprot_noncached
#define pgprot_noncached(prot)	(prot)
#endif

#ifndef pgprot_writecombine
#define pgprot_writecombine pgprot_noncached
#endif

/*
 * When walking page tables, get the address of the next boundary,
 * or the end address of the range if that comes earlier.  Although no
 * vma end wraps to 0, rounded up __boundary may wrap to 0 throughout.
 */

#define pgd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#ifndef pud_addr_end
#define pud_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PUD_SIZE) & PUD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})
#endif

#ifndef pmd_addr_end
#define pmd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})
#endif

/*
 * When walking page tables, we usually want to skip any p?d_none entries;
 * and any p?d_bad entries - reporting the error before resetting to none.
 * Do the tests inline, but report and clear the bad entry in mm/memory.c.
 */
void pgd_clear_bad(pgd_t *);
void pud_clear_bad(pud_t *);
void pmd_clear_bad(pmd_t *);


/*
 * A facility to provide lazy MMU batching.  This allows PTE updates and
 * page invalidations to be delayed until a call to leave lazy MMU mode
 * is issued.  Some architectures may benefit from doing this, and it is
 * beneficial for both shadow and direct mode hypervisors, which may batch
 * the PTE updates which happen during this window.  Note that using this
 * interface requires that read hazards be removed from the code.  A read
 * hazard could result in the direct mode hypervisor case, since the actual
 * write to the page tables may not yet have taken place, so reads though
 * a raw PTE pointer after it has been modified are not guaranteed to be
 * up to date.  This mode can only be entered and left under the protection of
 * the page table locks for all page tables which may be modified.  In the UP
 * case, this is required so that preemption is disabled, and in the SMP case,
 * it must synchronize the delayed page table writes properly on other CPUs.
 */
#ifndef __HAVE_ARCH_ENTER_LAZY_MMU_MODE
#define arch_enter_lazy_mmu_mode()	do {} while (0)
#define arch_leave_lazy_mmu_mode()	do {} while (0)
#define arch_flush_lazy_mmu_mode()	do {} while (0)
#endif

/*
 * A facility to provide batching of the reload of page tables and
 * other process state with the actual context switch code for
 * paravirtualized guests.  By convention, only one of the batched
 * update (lazy) modes (CPU, MMU) should be active at any given time,
 * entry should never be nested, and entry and exits should always be
 * paired.  This is for sanity of maintaining and reasoning about the
 * kernel code.  In this case, the exit (end of the context switch) is
 * in architecture-specific code, and so doesn't need a generic
 * definition.
 */
#ifndef __HAVE_ARCH_START_CONTEXT_SWITCH
#define arch_start_context_switch(prev)	do {} while (0)
#endif

#endif /* CONFIG_MMU */

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

#endif /* !__ASSEMBLY__ */

#include <asm-generic/pgtable.h>

#endif /* _ASM_RISCV_PGTABLE_H */
