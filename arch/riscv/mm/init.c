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

static void __init pagetable_init(void)
{
	/* Remove identity mapping to catch NULL pointer dereferences */
	swapper_pg_dir[0] = __pgd(0);
}

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

	zones_size[ZONE_NORMAL] = max_low_pfn;
	free_area_init_nodes(zones_size);
}
#else
static void __init zone_sizes_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];

	memset(zones_size, 0, sizeof(zones_size));
	memblock_add_node(0, PFN_PHYS(max_low_pfn), 0);
	zones_size[ZONE_NORMAL] = max_low_pfn;
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
	pagetable_init();
	flush_tlb_all();
	zone_sizes_init();
}

void __init mem_init(void)
{
#ifdef CONFIG_FLATMEM
	BUG_ON(!mem_map);
#endif /* CONFIG_FLATMEM */

	set_max_mapnr(max_low_pfn);
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

