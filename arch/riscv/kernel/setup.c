#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/pgtable.h>

#define MEMORY_START 0

static char __initdata command_line[COMMAND_LINE_SIZE];

static void __init setup_bootmem(void)
{
	unsigned long start_pfn, end_pfn;
	unsigned long mem_size, bootmap_size;

	mem_size = MEMORY_SIZE;
	printk(KERN_INFO "Detected 0x%lx bytes of physical memory\n", mem_size);
	end_pfn = PFN_DOWN(min(VMALLOC_END - PAGE_OFFSET, mem_size));

	/* First page after kernel image */
	start_pfn = PFN_UP(__pa(&_end));

	bootmap_size = init_bootmem(start_pfn, end_pfn);
	free_bootmem(PFN_PHYS(start_pfn), (end_pfn - start_pfn) << PAGE_SHIFT);
	reserve_bootmem(PFN_PHYS(start_pfn), bootmap_size, BOOTMEM_DEFAULT);
}

void __init setup_arch(char **cmdline_p)
{
	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

#ifdef CONFIG_EARLY_PRINTK
	setup_early_printk();
#endif /* CONFIG_EARLY_PRINTK */
	setup_bootmem();
	paging_init();
}
