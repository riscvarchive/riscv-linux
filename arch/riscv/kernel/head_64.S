#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>
#include <asm/page.h>
#include <asm/pgtable-bits.h>
#include <asm/pcr.h>

#define __pa(x) ((x) - PAGE_OFFSET)

__INIT
ENTRY(_start)

	mfpcr x1, PCR_STATUS
	li x2, (SR_S64 | SR_U64)
	li x3, ~(SR_VM)
	or x1, x1, x2
	and x1, x1, x3
	mtpcr x1, PCR_STATUS

	/* Initialize stack pointer */
	la sp, init_thread_union + THREAD_SIZE
	/* Initialize current thread_struct pointer */
	la x1, init_task
	mtpcr x1, PCR_K0

	lui x29, (PAGE_OFFSET >> 12) & 0xfffff
	/* Clear .bss segment, assumed to start and end
	 * with an 8-byte alignment
	 */
	la x1, __bss_start
	la x2, __bss_stop
	sub x1, x1, x29
	sub x2, x2, x29
1:
	sd zero, 0(x1)
	addi x1, x1, 8
	bltu x1, x2, 1b

	/* Initialize provisional page table */
	la x1, swapper_pg_dir
	sub x1, x1, x29
	/* Set PTBR and flush TLB */
	mtpcr x1, PCR_PTBR

	/* Enable paging */
	mfpcr x1, PCR_STATUS
	ori x1, x1, SR_VM
	mtpcr x1, PCR_STATUS

	la x1, start_kernel
	jr x1

END(_start)


#define PGDIR_SHIFT 33
#define PMDIR_SHIFT 23
#define VPN_MASK 0x3ff
#define PTE_SUPV (_PAGE_SR | _PAGE_SW | _PAGE_SE)

#define PGE_OFF(va) ((((va) >> PGDIR_SHIFT) & VPN_MASK) << 3)
#define PME_OFF(va) ((((va) >> PMDIR_SHIFT) & VPN_MASK) << 3)

	.data
	/*
	 * Empty zero page
	 */
	.balign PAGE_SIZE
ENTRY(empty_zero_page)
	.fill (empty_zero_page + PAGE_SIZE) - ., 1, 0x00
END(empty_zero_page)

	/*
	 * Provisional page tables
	 */
	.balign PAGE_SIZE
ENTRY(swapper_pg_dir)
	.quad __pa(ident_pm_dir) + PTE_SUPV | _PAGE_T
	.org swapper_pg_dir + PGE_OFF(PAGE_OFFSET), 0x00
	.quad __pa(kern_pm_dir) + PTE_SUPV | _PAGE_T
	.fill (swapper_pg_dir + PAGE_SIZE) - ., 1, 0x00
END(swapper_pg_dir)

	/* Provisional PMD for initial identity mapping */
	.balign PAGE_SIZE
ENTRY(ident_pm_dir)
	.quad PTE_SUPV | _PAGE_E
	.quad (1 << PMDIR_SHIFT) + PTE_SUPV | _PAGE_E
	.fill (ident_pm_dir + PAGE_SIZE) - ., 1, 0x00
END(ident_pm_dir)

	/* Provisional PMD for initial kernel mapping */
	.balign PAGE_SIZE
ENTRY(kern_pm_dir)
	.org kern_pm_dir + PME_OFF(PAGE_OFFSET), 0x00
	.quad PTE_SUPV | _PAGE_E
	.quad (1 << PMDIR_SHIFT) + PTE_SUPV | _PAGE_E
	.fill (kern_pm_dir + PAGE_SIZE) - ., 1, 0x00
END(kern_pm_dir)

