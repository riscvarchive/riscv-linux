#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/asm.h>
#include <asm/csr.h>

#define PTIDX_BITS	10
#define PTIDX_MASK	((1 << PTIDX_BITS) - 1)
#define PTIDX_SHIFT(i)	(((i) * PTIDX_BITS) + PAGE_SHIFT)
#define PTIDX(i, va)	(((va) >> PTIDX_SHIFT(i)) & PTIDX_MASK)
#define PTOFF(i, va)	(PTIDX(i, va) << LGREG)

#define PAGE_SUPV	(_PAGE_SR | _PAGE_SW | _PAGE_SX)

__INIT
ENTRY(_start)

#ifdef CONFIG_64BIT
	li s0, (SR_S64 | SR_U64)
	csrs status, s0
#define SR_CLEAR (SR_VM | SR_IM | SR_EI)
#else
#define SR_CLEAR (SR_VM | SR_S64 | SR_U64 | SR_IM | SR_EI)
#endif
	li s0, SR_CLEAR
	csrc status, s0

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

#if (PAGE_OFFSET & (PTIDX_SHIFT(1) - 1))
#error PAGE_OFFSET must be aligned on a superpage
#endif

	/* Initialize provisional page tables */
	.altmacro
	.macro vminit, ptr_base, ptr_ident, ptr_kern, ptr_end, pte, step
	li \pte, (PAGE_SUPV | _PAGE_G | _PAGE_V)
	li \step, (1 << PTIDX_SHIFT(1))

	/* Address of first kernel PTE */
	li \ptr_kern, PTOFF(1, PAGE_OFFSET)
	add \ptr_kern, \ptr_base, \ptr_kern

	/* Address after last kernel PTE */
	li \ptr_end, PTOFF(1, VMALLOC_START)
	add \ptr_end, \ptr_base, \ptr_end

1:
	REG_S \pte, 0(\ptr_ident)  /* Identity mapping */
	REG_S \pte, 0(\ptr_kern)   /* Kernel mapping */
	add \pte, \pte, \step      /* Increment PFN */
	addi \ptr_kern, \ptr_kern, SZREG
	addi \ptr_ident, \ptr_ident, SZREG
	bltu \ptr_kern, \ptr_end, 1b
	.endm

#ifdef CONFIG_64BIT

	la s1, ident_pm_dir
	la s2, kern_pm_dir

	/* PGD entry for identity mapping */
	ori s3, s1, (PAGE_SUPV | _PAGE_T | _PAGE_V)
	sd s3, 0(s0)

	/* PGD entry for kernel mapping */
	ori s3, s2, (PAGE_SUPV | _PAGE_T | _PAGE_V)
	li s4, PTOFF(2, PAGE_OFFSET)
	add s0, s0, s4
	sd s3, 0(s0)

	/* Cover the entire kernel virtual address space
	   using 4 MiB superpages */
	vminit s2, s1, s0, s3, s4, s5

#else /* !CONFIG_64BIT */

	/* Cover the entire kernel virtual address space
	   using 2 MiB superpages */
	vminit s0, s0, s1, s2, s3, s4

#endif /* CONFIG_64BIT */

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

#ifdef CONFIG_64BIT

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

#endif /* CONFIG_64BIT */
