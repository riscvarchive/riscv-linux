#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/initrd.h>
#include <linux/console.h>
#include <linux/screen_info.h>

#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/pgtable.h>
#include <asm/smp.h>
#include <asm/sbi.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_DUMMY_CONSOLE
struct screen_info screen_info = {
	.orig_video_lines	= 30,
	.orig_video_cols	= 80,
	.orig_video_mode	= 0,
	.orig_video_ega_bx	= 0,
	.orig_video_isVGA	= 1,
	.orig_video_points	= 8
};
#endif

static char __initdata command_line[COMMAND_LINE_SIZE];
#ifdef CONFIG_CMDLINE_BOOL
static char __initdata builtin_cmdline[COMMAND_LINE_SIZE] = CONFIG_CMDLINE;
#endif /* CONFIG_CMDLINE_BOOL */

unsigned long va_pa_offset;
unsigned long pfn_base;

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
	memblock_reserve(__pa(initrd_start), size);
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

static resource_size_t __initdata mem_size;

/* Parse "mem=nn[KkMmGg]" */
static int __init early_mem(char *p)
{
	if (!p)
		return -EINVAL;
	mem_size = memparse(p, &p) & PMD_MASK;
	if (mem_size == 0)
		return -EINVAL;
	return 0;
}
early_param("mem", early_mem);

#define NUM_SWAPPER_PMDS ((uintptr_t)-PAGE_OFFSET >> PGDIR_SHIFT)
pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;
pmd_t swapper_pmd[PTRS_PER_PMD] __page_aligned_bss;

pgd_t trampoline_pg_dir[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);
pmd_t trampoline_pmd[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);

asmlinkage void __init setup_vm(void)
{
	extern char _start;
	uintptr_t i;
	uintptr_t pa = (uintptr_t) &_start;
	uintptr_t size = roundup((uintptr_t) _end - pa, PMD_SIZE);
	uintptr_t pmd_pfn = PFN_DOWN((uintptr_t)swapper_pmd);
	uintptr_t prot = pgprot_val(PAGE_KERNEL) | _PAGE_EXEC;
	pgd_t pmd = __pgd((pmd_pfn << _PAGE_PFN_SHIFT) | _PAGE_TABLE);

	/* Sanity check alignment and size */
	BUG_ON((pa % PMD_SIZE) != 0);
	BUG_ON((PAGE_OFFSET % PGDIR_SIZE) != 0);
	BUG_ON(size > PGDIR_SIZE);

	va_pa_offset = PAGE_OFFSET - pa;
	pfn_base = PFN_DOWN(pa);
	mem_size = size;

	trampoline_pg_dir[(PAGE_OFFSET >> PGDIR_SHIFT) % PTRS_PER_PGD] = pmd;
	trampoline_pmd[0] = __pmd((PFN_DOWN(pa) << _PAGE_PFN_SHIFT) | prot);

	swapper_pg_dir[(PAGE_OFFSET >> PGDIR_SHIFT) % PTRS_PER_PGD] = pmd;
	for (i = 0; i < size / PMD_SIZE; i++, pa += PMD_SIZE)
		swapper_pmd[i] = __pmd((PFN_DOWN(pa) << _PAGE_PFN_SHIFT) | prot);
}

static void __init setup_bootmem(void)
{
	extern char _start;

#warning FIXME CONFIG STRING: enumerate physical memory
	{
		/* Assume we have 4 * (kernel size) memory in total */
		uintptr_t i, extra = 3 * (mem_size / PMD_SIZE);
		uintptr_t va = (uintptr_t) &_start + mem_size;
		mem_size += extra * PMD_SIZE;
		for (i = 0; i < extra; i++, va += PMD_SIZE) {
			uintptr_t prot = pgprot_val(PAGE_KERNEL);
			pmd_t pte = __pmd((PFN_DOWN(__pa(va)) << _PAGE_PFN_SHIFT) | prot);
			swapper_pmd[(va >> PMD_SHIFT) % PTRS_PER_PMD] = pte;
		}
		local_flush_tlb_all();
	}

	set_max_mapnr(PFN_DOWN(mem_size));
	max_low_pfn = pfn_base + PFN_DOWN(mem_size);

#ifdef CONFIG_BLK_DEV_INITRD
	setup_initrd();
#endif /* CONFIG_BLK_DEV_INITRD */

	memblock_reserve(__pa(&_start), (uintptr_t) _end - (uintptr_t) &_start);
	memblock_allow_resize();
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

	parse_early_param();

	init_mm.start_code = (unsigned long) _stext;
	init_mm.end_code   = (unsigned long) _etext;
	init_mm.end_data   = (unsigned long) _edata;
	init_mm.brk        = (unsigned long) _end;

	setup_bootmem();
#ifdef CONFIG_SMP
	setup_smp();
#endif
	paging_init();

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif
}
