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

#ifdef CONFIG_64BIT
static void __init pagetable_init(void)
{
	unsigned int i, start, end;
	unsigned long ppn;
	start = pmd_index(PAGE_OFFSET);
	end = pmd_index((unsigned long)pfn_to_virt(max_low_pfn));

	ppn = 0UL;
	for (i = start; i <= end; i++)
	{
		kern_pm_dir[i] = __pmd(ppn | pgprot_val(PAGE_KERNEL) | _PAGE_V);
		ppn += PMD_SIZE;
	}
	/* Remove identity mapping to catch NULL pointer dereferences */
	swapper_pg_dir[0] = __pgd(0);
}
#else
static void __init pagetable_init(void)
{
}
#endif /* CONFIG_64BIT */

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
	unsigned long code_size, data_size, init_size;
	unsigned long num_resvpages;
	unsigned long pfn;
#ifdef CONFIG_NUMA
	int nid;
#endif /* CONFIG_NUMA */

	high_memory = (void *)(__va(PFN_PHYS(max_low_pfn)));

#ifdef CONFIG_NUMA
	num_physpages = 0;
	totalram_pages = 0;
	for_each_online_node(nid) {
		pg_data_t *pgdat;
		pgdat = NODE_DATA(nid);
		num_physpages += pgdat->node_present_pages;
		totalram_pages += free_all_bootmem_node(pgdat);
	}
#else
	totalram_pages = free_all_bootmem();
	num_physpages = NODE_DATA(0)->node_present_pages;
#endif /* CONFIG_NUMA */

	num_resvpages = 0;
	for (pfn = 0; pfn < max_low_pfn; pfn++) {
		if (page_is_ram(pfn) && PageReserved(pfn_to_page(pfn)))
			num_resvpages++;
	}

	code_size = (unsigned long)(&_etext) - (unsigned long)(&_stext);
	data_size = (unsigned long)(&_edata) - (unsigned long)(&_etext);
	init_size = (unsigned long)(&__init_end) - (unsigned long)(&__init_begin);

	printk(KERN_INFO
		"Memory: %luk/%luk available (%luk kernel code, "
		"%luk reserved, %luk data, %luk init)\n",
		nr_free_pages() << (PAGE_SHIFT - 10),
		num_physpages << (PAGE_SHIFT - 10),
		code_size >> 10,
		num_resvpages << (PAGE_SHIFT - 10),
		data_size >> 10,
		init_size >> 10);

	printk(KERN_INFO "virtual kernel memory layout:\n"
		"    vmalloc : 0x%08lx - 0x%08lx  [%4ld MiB]\n"
		"     lowmem : 0x%08lx - 0x%08lx  [%4ld MiB]\n"
		"      .data : 0x%08lx - 0x%08lx\n"
		"      .text : 0x%08lx - 0x%08lx\n"
		"      .init : 0x%08lx - 0x%08lx\n",
		VMALLOC_START, VMALLOC_END,
		(VMALLOC_END - VMALLOC_START) >> 20,
		PAGE_OFFSET, (unsigned long)(high_memory),
		((unsigned long)(high_memory) - PAGE_OFFSET) >> 20,
		(unsigned long)(&_etext),
		(unsigned long)(&_edata),
		(unsigned long)(&_text),
		(unsigned long)(&_etext),
		(unsigned long)(&__init_begin),
		(unsigned long)(&__init_end));
}

static void free_init_pages(const char *what, unsigned long begin, unsigned long end)
{
	unsigned long pfn;

	for (pfn = PFN_UP(begin); pfn < PFN_DOWN(end); pfn++) {
		struct page *page = pfn_to_page(pfn);
		void *addr = phys_to_virt(PFN_PHYS(pfn));

		ClearPageReserved(page);
		init_page_count(page);
		memset(addr, POISON_FREE_INITMEM, PAGE_SIZE);
		__free_page(page);
		totalram_pages++;
	}
	printk(KERN_INFO "Freeing %s: %ldk freed\n", what, (end - begin) >> 10);
}

void free_initmem(void)
{
	free_init_pages("unused kernel memory",
		__pa(&__init_begin),
		__pa(&__init_end));
}

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
}
#endif /* CONFIG_BLK_DEV_INITRD */

