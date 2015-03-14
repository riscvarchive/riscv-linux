#ifndef _ASM_RISCV_PGTABLE_H
#define _ASM_RISCV_PGTABLE_H

#include <linux/mmzone.h>

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
#define _PAGE_R         (_PAGE_SR | _PAGE_UR)
#define _PAGE_W         (_PAGE_SW | _PAGE_UW)
#define _PAGE_X         (_PAGE_SX | _PAGE_UX)

#define PAGE_NONE		__pgprot(0)
#define PAGE_READ		__pgprot(_PAGE_BASE | _PAGE_R)
#define PAGE_WRITE		__pgprot(_PAGE_BASE | _PAGE_W)
#define PAGE_EXEC		__pgprot(_PAGE_BASE | _PAGE_X)

#define PAGE_READ_EXEC  	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_X)
#define PAGE_WRITE_EXEC		__pgprot(_PAGE_BASE | _PAGE_W | _PAGE_X)
#define PAGE_COPY		__pgprot(_PAGE_BASE | _PAGE_SR)
#define PAGE_COPY_EXEC		__pgprot(_PAGE_BASE | _PAGE_SR | _PAGE_X)
#define PAGE_SHARED		__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_W)
#define PAGE_SHARED_EXEC	__pgprot(_PAGE_BASE | _PAGE_R | _PAGE_W | _PAGE_X)

#define PAGE_KERNEL		__pgprot(_PAGE_BASE | _PAGE_SR | _PAGE_SW | _PAGE_G)

#define swapper_pg_dir NULL

/* MAP_PRIVATE permissions: xwr (copy-on-write) */
#define __P000	PAGE_NONE
#define __P001	PAGE_READ
#define __P010	PAGE_COPY
#define __P011	PAGE_READ
#define __P100	PAGE_EXEC
#define __P101	PAGE_READ_EXEC
#define __P110	PAGE_COPY_EXEC
#define __P111	PAGE_READ_EXEC

/* MAP_SHARED permissions: xwr */
#define __S000	PAGE_NONE
#define __S001	PAGE_READ
#define __S010	PAGE_WRITE
#define __S011	PAGE_SHARED
#define __S100	PAGE_EXEC
#define __S101	PAGE_READ_EXEC
#define __S110	PAGE_WRITE_EXEC
#define __S111	PAGE_SHARED_EXEC

/*
 * ZERO_PAGE is a global shared page that is always zero,
 * used for zero-mapped memory areas, etc.
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr) (virt_to_page(empty_zero_page))

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


#define pgd_index(addr) (((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

/* Locate an entry in the page global directory */
static inline pgd_t *pgd_offset(const struct mm_struct *mm, unsigned long addr)
{
	return mm->pgd + pgd_index(addr);
}
/* Locate an entry in the kernel page global directory */
#define pgd_offset_k(addr)      pgd_offset(&init_mm, (addr))

static inline struct page *pmd_page(pmd_t pmd)
{
	return pfn_to_page(pmd_val(pmd) >> PAGE_SHIFT);
}

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_val(pmd) & PAGE_MASK);
}

/* Yields the page frame number (PFN) of a page table entry */
static inline unsigned long pte_pfn(pte_t pte)
{
	return (pte_val(pte) >> PAGE_SHIFT);
}

#define pte_page(x)     pfn_to_page(pte_pfn(x))

/* Constructs a page table entry */
static inline pte_t pfn_pte(unsigned long pfn, pgprot_t prot)
{
	return __pte((pfn << PAGE_SHIFT) | pgprot_val(prot));
}

static inline pte_t mk_pte(struct page *page, pgprot_t prot)
{
	return pfn_pte(page_to_pfn(page), prot);
}

#define pte_index(addr) (((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1)) 

static inline pte_t *pte_offset_kernel(pmd_t *pmd, unsigned long addr)
{
	return (pte_t *)pmd_page_vaddr(*pmd) + pte_index(addr);
}

#define pte_offset_map(dir, addr)	pte_offset_kernel((dir), (addr))
#define pte_unmap(pte)			((void)(pte))

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
	return pte_val(pte) & _PAGE_W;
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

static inline int pte_special(pte_t pte)
{
	return pte_val(pte) & _PAGE_SPECIAL;
}

/* static inline pte_t pte_rdprotect(pte_t pte) */

static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_W));
}

/* static inline pte_t pte_mkread(pte_t pte) */

static inline pte_t pte_mkwrite(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_W);
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

static inline pte_t pte_mkspecial(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SPECIAL);
}

/* Modify page protection bits */
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

#define pgd_ERROR(e) \
	pr_err("%s:%d: bad pgd " PTE_FMT ".\n", __FILE__, __LINE__, pgd_val(e))


/* Commit new configuration to MMU hardware */
static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
}

/*
 * Encode and decode a swap entry
 *
 * Format of swap PTE:
 *	bit    0   :	_PAGE_PRESENT (zero)
 *	bit    1   :	_PAGE_FILE (zero)
 *	bits 2-8   :	swap type
 *	bits 9-XLEN:	swap offset
 */
#define __SWP_TYPE_SHIFT	2
#define __SWP_TYPE_BITS		7
#define __SWP_TYPE_MASK		((1UL << __SWP_TYPE_BITS) - 1)
#define __SWP_OFFSET_SHIFT	(__SWP_TYPE_BITS + __SWP_TYPE_SHIFT)

#define MAX_SWAPFILES_CHECK()	\
	BUILD_BUG_ON(MAX_SWAPFILES_SHIFT > __SWP_TYPE_BITS)

#define __swp_type(x)		(((x).val >> __SWP_TYPE_SHIFT) & __SWP_TYPE_MASK)
#define __swp_offset(x)		((x).val >> __SWP_OFFSET_SHIFT)
#define __swp_entry(type, offset) ((swp_entry_t) \
	{ ((type) << __SWP_TYPE_SHIFT) | ((offset) << __SWP_OFFSET_SHIFT) })

#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	((pte_t) { (x).val })

/*
 * Encode and decode a non-linear file mapping entry
 *
 * Format of file PTE:
 *	bit                0:	_PAGE_PRESENT (zero)
 *	bit                1:	_PAGE_FILE (zero)
 *	bits PAGE_SHIFT-XLEN:	file offset / PAGE_SIZE
 */
#ifdef CONFIG_64BIT
#define PTE_FILE_MAX_BITS	(64 - PAGE_SHIFT)
#else
#define PTE_FILE_MAX_BITS	(32 - PAGE_SHIFT)
#endif

static inline pte_t pgoff_to_pte(unsigned long off)
{
	return __pte((off << PAGE_SHIFT) | _PAGE_FILE);
}

static inline unsigned long pte_to_pgoff(pte_t pte)
{
	return (pte_val(pte) >> PAGE_SHIFT);
}

#ifdef CONFIG_FLATMEM
#define kern_addr_valid(addr)   (1) /* FIXME */
#endif

extern void paging_init(void);

#ifndef __PAGETABLE_PMD_FOLDED
extern pmd_t ident_pm_dir[PTRS_PER_PMD];
extern pmd_t kern_pm_dir[PTRS_PER_PMD];
#endif /* __PAGETABLE_PMD_FOLDED */

static inline void pgtable_cache_init(void)
{
	/* No page table caches to initialize */
}

#endif /* CONFIG_MMU */

#define VMALLOC_SIZE     _AC(0x8000000,UL)
#define VMALLOC_END      (PAGE_OFFSET - 1)
#define VMALLOC_START    (PAGE_OFFSET - VMALLOC_SIZE)

/* Task size is 0x40000000000 for RV64 or 0xb800000 for RV32.
   Note that PGDIR_SIZE must evenly divide TASK_SIZE. */
#ifdef CONFIG_64BIT
#define TASK_SIZE (PGDIR_SIZE * PTRS_PER_PGD / 2)
#else
#define TASK_SIZE VMALLOC_START
#endif

#include <asm-generic/pgtable.h>

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_RISCV_PGTABLE_H */
