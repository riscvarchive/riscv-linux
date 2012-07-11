#include <linux/init.h>
#include <linux/bootmem.h>

#include <asm-generic/sections.h>
#include <asm/setup.h>

#define MEMORY_START 0
#define MEMORY_SIZE 0x20000000 /* = 512 MiB */

void __init bootmem_init(void)
{
  unsigned long start_pfn, bootmap_size;
  unsigned long ram_start_pfn, ram_end_pfn;

  /* The first page after kernel */
  start_pfn = PFN_UP((u32)__pa(&_end));
  ram_start_pfn = PFN_UP(MEMORY_START);
  ram_end_pfn = PFN_UP(MEMORY_START + MEMORY_SIZE);

  bootmap_size = init_bootmem(start_pfn, ram_end_pfn - ram_start_pfn);
  free_bootmem(PFN_PHYS(start_pfn),
          (ram_end_pfn - start_pfn) << PAGE_SHIFT);
  reserve_bootmem(PFN_PHYS(start_pfn), bootmap_size, BOOTMEM_DEFAULT);
}

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_EARLY_PRINTK
	setup_early_printk();
#endif /* CONFIG_EARLY_PRINTK */
//	bootmem_init();
}
