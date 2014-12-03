#ifndef _ASM_RISCV_PGTABLE_H
#define _ASM_RISCV_PGTABLE_H

#include <asm/pgtable-bits.h>

#ifndef __ASSEMBLY__

#ifdef CONFIG_MMU

/* Page Upper Directory not used in RISC-V */
#include <asm-generic/pgtable-nopud.h>
#include <asm/page.h>
#include <linux/mm_types.h>

#ifdef CONFIG_64BIT
#include <asm/pgtable-64.h>
#else
#include <asm/pgtable-32.h>
#endif /* CONFIG_64BIT */

/* Number of entries in the page global directory */
#define PTRS_PER_PGD    (PAGE_SIZE / sizeof(pgd_t))
/* Number of entries in the page table */
#define PTRS_PER_PTE    (PAGE_SIZE / sizeof(pte_t))

/* Number of PGD entries that a user-mode program can use */
#define USER_PTRS_PER_PGD   (TASK_SIZE / PGDIR_SIZE)
#define FIRST_USER_ADDRESS  0

/* Page protection bits */
#define _PAGE_BASE      (_PAGE_ACCESSED | _PAGE_V)
#define _PAGE_RD        (_PAGE_SR | _PAGE_UR)
#define _PAGE_WR        (_PAGE_SW | _PAGE_UW)
#define _PAGE_EX        (_PAGE_SX | _PAGE_UX)

#define PAGE_KERNEL     __pgprot(_PAGE_BASE | _PAGE_SR | _PAGE_SW | _PAGE_SX | _PAGE_G)
#define PAGE_NONE       __pgprot(0)

#define PAGE_R          __pgprot(_PAGE_BASE | _PAGE_RD)
#define PAGE_W          __pgprot(_PAGE_BASE | _PAGE_WR)
#define PAGE_RW         __pgprot(_PAGE_BASE | _PAGE_RD | _PAGE_WR)
#define PAGE_RX         __pgprot(_PAGE_BASE | _PAGE_RD | _PAGE_EX)
#define PAGE_RWX        __pgprot(_PAGE_BASE | _PAGE_RD | _PAGE_WR | _PAGE_EX)

static inline void pgtable_cache_init(void)
{
	/* No page table caches to initialize */
}

static inline int pmd_present(pmd_t pmd)
{
	return (pmd_val(pmd) & _PAGE_PRESENT);
}

static inline int pmd_none(pmd_t pmd)
{
	return (pmd_val(pmd) == 0);
}

static inline int pmd_bad(pmd_t pmd)
{
	return !pmd_present(pmd);
}

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	*pmdp = pmd;
}

static inline void pmd_clear(pmd_t *pmdp)
{
	set_pmd(pmdp, __pmd(0));
}

#define pte_unmap(pte) ((void)(pte))

#define pgd_index(addr) (((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

/* Locate an entry in the page global directory */
static inline pgd_t *pgd_offset(const struct mm_struct *mm, unsigned long addr)
{
	return mm->pgd + pgd_index(addr);
}
/* Locate an entry in the kernel page global directory */
#define pgd_offset_k(addr)      pgd_offset(&init_mm, (addr))

extern struct page *mem_map;

static inline struct page *pmd_page(pmd_t pmd)
{
	return pfn_to_page(pmd_val(pmd) >> PTE_PFN_SHIFT);
}

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_val(pmd) & PTE_PFN_MASK);
}

/* Yields the page frame number (PFN) of a page table entry */
static inline unsigned long pte_pfn(pte_t pte)
{
	return (pte_val(pte) >> PTE_PFN_SHIFT);
}

#define pte_page(x)     pfn_to_page(pte_pfn(x))

/* Constructs a page table entry */
static inline pte_t pfn_pte(unsigned long pfn, pgprot_t prot)
{
	return __pte((pfn << PTE_PFN_SHIFT) | pgprot_val(prot));
}

static inline pte_t mk_pte(struct page *page, pgprot_t prot)
{
	return pfn_pte(page_to_pfn(page), prot);
}

#define pte_index(addr) (((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1)) 
#define pte_offset_map(dir, addr) \
	((pte_t *)(page_address(pmd_page(*(dir)))) + pte_index(addr))

static inline pte_t *pte_offset_kernel(pmd_t *pmd, unsigned long addr)
{
	return (pte_t *)pmd_page_vaddr(*pmd) + pte_index(addr);
}

/*
 * Encode and decode a swap entry
 *
 * Format of swap PTE:
 */
#define __swp_type(x)               ((x).val & 0x1f)
#define __swp_offset(x)             ((x).val >> PTE_PFN_SHIFT)
#define __swp_entry(type, offset)   \
	((swp_entry_t) { ((type) & 0x1f) | ((offset) << PTE_PFN_SHIFT) })
#define __pte_to_swp_entry(pte)     ((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)       ((pte_t) { (x).val })

/*
 * Encode and decode a non-linear file mapping entry
 *
 * Format of file PTE:
 */
#ifdef CONFIG_64BIT
#define PTE_FILE_MAX_BITS       (64 - PTE_PFN_SHIFT)
#else
#define PTE_FILE_MAX_BITS       (32 - PTE_PFN_SHIFT)
#endif /* CONFIG_64BIT */

static inline pte_t pgoff_to_pte(unsigned long off)
{
	return __pte((off << PTE_PFN_SHIFT) | _PAGE_FILE);
}

static inline unsigned long pte_to_pgoff(pte_t pte)
{
	return (pte_val(pte) >> PTE_PFN_SHIFT);
}

/*
 * Certain architectures need to do special things when PTEs within
 * a page table are directly modified.  Thus, the following hook is
 * made available.
 */
static inline void set_pte(pte_t *ptep, pte_t pteval)
{
	*ptep = pteval;
}

static inline void set_pte_at(struct mm_struct *mm,
	unsigned long addr, pte_t *ptep, pte_t pteval)
{
	set_pte(ptep, pteval);
}

static inline void pte_clear(struct mm_struct *mm,
	unsigned long addr, pte_t *ptep)
{
	set_pte_at(mm, addr, ptep, __pte(0));
}

static inline int pte_present(pte_t pte)
{
	return (pte_val(pte) & _PAGE_PRESENT);
}

static inline int pte_none(pte_t pte)
{
	return (pte_val(pte) == 0);
}

/* static inline int pte_read(pte_t pte) */

static inline int pte_write(pte_t pte)
{
	return pte_val(pte) & _PAGE_WR;
}

/* static inline int pte_exec(pte_t pte) */

static inline int pte_dirty(pte_t pte)
{
	return pte_val(pte) & _PAGE_DIRTY;
}

static inline int pte_file(pte_t pte)
{
	return pte_val(pte) & _PAGE_FILE;
}

static inline int pte_young(pte_t pte)
{
	return pte_val(pte) & _PAGE_ACCESSED;
}

/* static inline pte_t pte_rdprotect(pte_t pte) */

static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_WR));
}

/* static inline pte_t pte_mkread(pte_t pte) */

static inline pte_t pte_mkwrite(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_WR);
}

/* static inline pte_t pte_mkexec(pte_t pte) */

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
	return __pte(pte_val(pte) & ~(_PAGE_ACCESSED));
}

static inline pte_t pte_mkold(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_ACCESSED);
}

static inline int pte_special(pte_t pte)
{
	return 0;
}

/* 
 * pte_mkspecial instead does a write protect. We only expect 
 * do_anonymous_page to use this function to mark the empty_zero_page as
 * non-writable.
 */
static inline pte_t pte_mkspecial(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_WR));
}

/* Modify page protection bits */
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

#define pgd_ERROR(e) \
	printk("%s:%d: bad pgd %016lx.\n", __FILE__, __LINE__, pgd_val(e))

/* MAP_PRIVATE permissions: force writes to copy the page */
#define __P000	PAGE_NONE
#define __P001	PAGE_R
#define __P010	PAGE_NONE
#define __P011	PAGE_R
#define __P100	PAGE_RX
#define __P101	PAGE_RX
#define __P110	PAGE_RX
#define __P111	PAGE_RX

/* MAP_SHARED permissions */
#define __S000	PAGE_NONE
#define __S001	PAGE_R
#define __S010	PAGE_W
#define __S011	PAGE_RW
#define __S100	PAGE_RX
#define __S101	PAGE_RX
#define __S110	PAGE_RWX
#define __S111	PAGE_RWX

extern unsigned long empty_zero_page[PTRS_PER_PTE];
#define ZERO_PAGE(vaddr) (virt_to_page(empty_zero_page))


/* Commit new configuration to MMU hardware */
static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
}

#define io_remap_pfn_range(vma, vaddr, pfn, size, prot) \
	remap_pfn_range(vma, vaddr, pfn, size, prot)

#endif /* CONFIG_MMU */

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern void paging_init(void);

#ifndef __PAGETABLE_PMD_FOLDED
extern pmd_t ident_pm_dir[PTRS_PER_PMD];
extern pmd_t kern_pm_dir[PTRS_PER_PMD];
#endif /* __PAGETABLE_PMD_FOLDED */

#include <asm-generic/pgtable.h>

#endif /* !__ASSEMBLY__ */

#ifdef CONFIG_64BIT
#define VMALLOC_START    _AC(0xfffffffff8000000,UL)
#define VMALLOC_END      _AC(0xffffffffffffffff,UL)
#else
#define VMALLOC_START    _AC(0xf8000000,UL)
#define VMALLOC_END      _AC(0xffffffff,UL)
#endif

#endif /* _ASM_RISCV_PGTABLE_H */
