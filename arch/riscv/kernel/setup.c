#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/sched.h>
#include <linux/initrd.h>

#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/pgtable.h>

static char __initdata command_line[COMMAND_LINE_SIZE];
#ifdef CONFIG_CMDLINE_BOOL
static char __initdata builtin_cmdline[COMMAND_LINE_SIZE] = CONFIG_CMDLINE;
#endif /* CONFIG_CMDLINE_BOOL */

#ifdef CONFIG_BLK_DEV_INITRD
static void __init setup_initrd(void)
{
	extern char __initramfs_start[];
	extern unsigned long __initramfs_size;
	unsigned long size;

	if (__initramfs_size > 0) {
		initrd_start = (unsigned long)(&__initramfs_start);
		initrd_end = initrd_start + __initramfs_size;
	}

	if (initrd_start >= initrd_end) {
		printk(KERN_INFO "initrd not found or empty");
		goto disable;
	}
	if (__pa(initrd_end) > PFN_PHYS(max_low_pfn)) {
		printk(KERN_ERR "initrd extends beyond end of memory");
		goto disable;
	}

	size =  initrd_end - initrd_start;
	reserve_bootmem(__pa(initrd_start), size, BOOTMEM_DEFAULT);
	initrd_below_start_ok = 1;

	printk(KERN_INFO "Initial ramdisk at: 0x%p (%lu bytes)\n",
		(void *)(initrd_start), size);
	return;
disable:
	printk(KERN_CONT " - disabling initrd\n");
	initrd_start = 0;
	initrd_end = 0;
}
#endif /* CONFIG_BLK_DEV_INITRD */

static void __init setup_bootmem(void)
{
	unsigned long start_pfn, end_pfn;
	unsigned long mem_size, bootmap_size;

	mem_size = min(PHYSMEM_SIZE, PHYSMEM_MAX) << PHYSMEM_SHIFT;
	printk(KERN_INFO "Detected 0x%lx bytes of physical memory\n", mem_size);
	end_pfn = PFN_DOWN(min(VMALLOC_START - PAGE_OFFSET, mem_size));

	/* First page after kernel image */
	start_pfn = PFN_UP(__pa(&_end));

	bootmap_size = init_bootmem(start_pfn, end_pfn);
	free_bootmem(PFN_PHYS(start_pfn), (end_pfn - start_pfn) << PAGE_SHIFT);
	reserve_bootmem(PFN_PHYS(start_pfn), bootmap_size, BOOTMEM_DEFAULT);

#ifdef CONFIG_BLK_DEV_INITRD
	setup_initrd();
#endif /* CONFIG_BLK_DEV_INITRD */
}

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_CMDLINE_BOOL
#ifdef CONFIG_CMDLINE_OVERRIDE
	strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
#else
	if (builtin_cmdline[0] != '\0') {
		/* Append bootloader command line to built-in */
		strlcat(builtin_cmdline, " ", COMMAND_LINE_SIZE);
		strlcat(builtin_cmdline, boot_command_line, COMMAND_LINE_SIZE);
		strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	}
#endif /* CONFIG_CMDLINE_OVERRIDE */
#endif /* CONFIG_CMDLINE_BOOL */
	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

	init_mm.start_code = (unsigned long) _stext;
	init_mm.end_code   = (unsigned long) _etext;
	init_mm.end_data   = (unsigned long) _edata;
	init_mm.brk        = (unsigned long) _end;

	setup_bootmem();
	paging_init();
}
