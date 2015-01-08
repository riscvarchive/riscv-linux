#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/csr.h>

#define PGDIR_SHIFT 33
#define PMDIR_SHIFT 23
#define VPN_MASK    (0x3ff)
#define PAGE_SUPV (_PAGE_SR | _PAGE_SW | _PAGE_SX)

#define PGE_OFF(va) ((((va) >> PGDIR_SHIFT) & VPN_MASK) << 3)
#define PME_OFF(va) ((((va) >> PMDIR_SHIFT) & VPN_MASK) << 3)


__INIT
ENTRY(_start)

	/* Enable RV64 mode; temporarily disable virtual memory */
	li s0, (SR_S64 | SR_U64)
	li s1, (SR_VM | SR_IM | SR_EI)
	csrs status, s0
	csrc status, s1

	/* Load the global pointer (before any other use of la) */
	la gp, _gp

	/* Clear the .bss segment */
	la a0, __bss_start
	li a1, 0
	la a2, __bss_stop
	sub a2, a2, a0
	call memset

	/* Set PTBR and flush TLB */
	la s0, swapper_pg_dir
	csrw ptbr, s0
	csrw fatc, 0

	/* Initialize provisional page tables */
	la s1, ident_pm_dir
	la s2, kern_pm_dir

#if (PAGE_OFFSET & (PMDIR_SHIFT - 1))
#error PAGE_OFFSET must be aligned on a 4 MiB superpage
#endif

	/* PGD entry for identity mapping */
	ori s3, s1, (PAGE_SUPV | _PAGE_T | _PAGE_V)
	sd s3, 0(s0)

	/* PGD entry for kernel mapping */
	ori s3, s2, (PAGE_SUPV | _PAGE_T | _PAGE_V)
	li s4, PGE_OFF(PAGE_OFFSET)
	add s0, s0, s4
	sd s3, 0(s0)

	/* PMD entries to cover the entire kernel virtual
	   address space using 4 MiB superpages */
	li s0, PME_OFF(VMALLOC_START)
	add s0, s2, s0  /* End address of kern_pm_dir table */
	li s3, PME_OFF(PAGE_OFFSET)
	add s2, s2, s3  /* Address of first PTE in kern_pm_dir */
	li s3, (PAGE_SUPV | _PAGE_G | _PAGE_V)
	li s4, (1 << PMDIR_SHIFT)
1:
	sd s3, 0(s1)    /* Identity mapping */
	sd s3, 0(s2)    /* Kernel mapping */
	add s3, s3, s4  /* Increment PFN */
	addi s1, s1, 0x8
	addi s2, s2, 0x8
	bltu s2, s0, 1b

	/* Enable paging */
	li s0, SR_VM
	csrs status, s0

	/* Relocate to kernel mapping */
	li s0, PAGE_OFFSET
1:	auipc t0, %pcrel_hi(1f)
	add t0, t0, s0
	jr t0, %pcrel_lo(1b)
1:	add gp, gp, s0

	/* Initialize stack pointer */
	la sp, init_thread_union + THREAD_SIZE
	/* Initialize current thread_struct pointer */
	la s0, init_task
	csrw sup0, s0

	tail start_kernel

END(_start)


__PAGE_ALIGNED_BSS
	/* Empty zero page */
	.balign PAGE_SIZE
ENTRY(empty_zero_page)
	.fill (empty_zero_page + PAGE_SIZE) - ., 1, 0x00
END(empty_zero_page)

	/* Provisional PGD */
	.balign PAGE_SIZE
ENTRY(swapper_pg_dir)
	.fill (swapper_pg_dir + PAGE_SIZE) - ., 1, 0x00
END(swapper_pg_dir)

	/* Provisional PMD for initial identity mapping */
	.balign PAGE_SIZE
ENTRY(ident_pm_dir)
	.fill (ident_pm_dir + PAGE_SIZE) - ., 1, 0x00
END(ident_pm_dir)

	/* Provisional PMD for initial kernel mapping */
	.balign PAGE_SIZE
ENTRY(kern_pm_dir)
	.fill (kern_pm_dir + PAGE_SIZE) - ., 1, 0x00
END(kern_pm_dir)

