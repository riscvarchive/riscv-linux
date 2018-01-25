#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/memblock.h>
#include <linux/swap.h>

#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/pgtable.h>
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
	init_mm.pgd = (pgd_t *)pfn_to_virt(csr_read(sptbr));

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

