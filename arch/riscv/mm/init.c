#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/memblock.h>
#include <linux/swap.h>

#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/fixmap.h>
#include <asm/io.h>

#ifdef CONFIG_NUMA
static void __init zone_sizes_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];
	int nid;

	memset(zones_size, 0, sizeof(zones_size));

	for_each_online_node(nid) {
		pg_data_t *pgdat;
		unsigned long start_pfn, end_pfn;

		pgdat = NODE_DATA(nid);
		start_pfn = pgdat->bdata->node_min_pfn;
		end_pfn = pgdat->bdata->node_low_pfn;
		memblock_add_node(start_pfn,
			PFN_PHYS(end_pfn - start_pfn), nid);
	}

	zones_size[ZONE_NORMAL] = pfn_base + max_mapnr;
	free_area_init_nodes(zones_size);
}
#else
static void __init zone_sizes_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];

	memset(zones_size, 0, sizeof(zones_size));
	memblock_add_node(PFN_PHYS(pfn_base), PFN_PHYS(max_mapnr), 0);
	zones_size[ZONE_NORMAL] = pfn_base + max_mapnr;
	free_area_init_nodes(zones_size);
}
#endif /* CONFIG_NUMA */

void setup_zero_page(void)
{
	memset((void *)empty_zero_page, 0, PAGE_SIZE);
}

void __init paging_init(void)
{
	setup_zero_page();
	local_flush_tlb_all();
	zone_sizes_init();
}

void __init mem_init(void)
{
#ifdef CONFIG_FLATMEM
	BUG_ON(!mem_map);
#endif /* CONFIG_FLATMEM */

	high_memory = (void *)(__va(PFN_PHYS(max_low_pfn)));
	free_all_bootmem();

	mem_init_print_info(NULL);
}

void free_initmem(void)
{
	free_initmem_default(0);
}

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
//	free_reserved_area(start, end, 0, "initrd");
}
#endif /* CONFIG_BLK_DEV_INITRD */


static pte_t bm_pte[PTRS_PER_PTE] __page_aligned_bss;
#ifndef __PAGETABLE_PMD_FOLDED
static pmd_t bm_pmd[PTRS_PER_PMD] __page_aligned_bss __maybe_unused;
#endif

static inline pmd_t *pmd_offset_fixmap(unsigned long addr)
{
	pud_t *pud = pud_offset(pgd_offset_k(addr), addr);

	BUG_ON(pud_none(*pud) || pud_bad(*pud));

	return pmd_offset(pud, addr);
}

static inline pte_t *pte_offset_fixmap(unsigned long addr)
{
	return &bm_pte[pte_index(addr)];
}

void __init early_fixmap_init(void)
{
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr = FIXADDR_START;

	pud = pud_offset(pgd_offset_k(addr), addr);
#ifndef __PAGETABLE_PMD_FOLDED
	BUG_ON(pud_present(*pud));
	pud_populate(&init_mm, pud, bm_pmd);
#endif
	pmd = pmd_offset(pud, addr);
	BUG_ON(pmd_present(*pmd));
	pmd_populate_kernel(&init_mm, pmd, bm_pte);

	/*
	 * The boot-ioremap range spans multiple pmds, for which
	 * we are not prepared:
	 */
	BUILD_BUG_ON((__fix_to_virt(FIX_BTMAP_BEGIN) >> PMD_SHIFT)
		     != (__fix_to_virt(FIX_BTMAP_END) >> PMD_SHIFT));

	if ((pmd != pmd_offset_fixmap(fix_to_virt(FIX_BTMAP_BEGIN)))
	     || pmd != pmd_offset_fixmap(fix_to_virt(FIX_BTMAP_END))) {
		WARN_ON(1);
		pr_warn("pmd %p != %p, %p\n",
			pmd, pmd_offset_fixmap(fix_to_virt(FIX_BTMAP_BEGIN)),
			pmd_offset_fixmap(fix_to_virt(FIX_BTMAP_END)));
		pr_warn("fix_to_virt(FIX_BTMAP_BEGIN): %08lx\n",
			fix_to_virt(FIX_BTMAP_BEGIN));
		pr_warn("fix_to_virt(FIX_BTMAP_END):   %08lx\n",
			fix_to_virt(FIX_BTMAP_END));

		pr_warn("FIX_BTMAP_END:       %d\n", FIX_BTMAP_END);
		pr_warn("FIX_BTMAP_BEGIN:     %d\n", FIX_BTMAP_BEGIN);
	}
}

void __set_fixmap(enum fixed_addresses idx,
			       phys_addr_t phys, pgprot_t flags)
{
	unsigned long addr = __fix_to_virt(idx);
	pte_t *pte;

	BUG_ON(idx <= FIX_HOLE || idx >= __end_of_fixed_addresses);

	pte = pte_offset_fixmap(addr);

	if (pgprot_val(flags)) {
		set_pte(pte, pfn_pte(phys >> PAGE_SHIFT, flags));
	} else {
		pte_clear(&init_mm, addr, pte);
		flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
	}
}
