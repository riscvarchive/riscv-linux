#include <asm/thread_info.h>
#include <asm/asm-offsets.h>
#include <asm/asm.h>
#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>
#include <asm/page.h>
#include <asm/csr.h>

__INIT
ENTRY(_start)
	/* Disable FPU to detect illegal usage of
	   floating point in kernel space */
	li t0, SR_FS
	csrc sstatus, t0

#ifdef CONFIG_RV_PUM
	/* Prevent inadvertent access to user memory */
	li t0, SR_PUM
	csrs sstatus, t0
#endif

	/* By SBI convention, hart ID is in a0 */
	bnez a0, .Lsecondary_start

	/* Initialize page tables and relocate to virtual addresses */
	la sp, init_thread_union + THREAD_SIZE
	call setup_vm
	call relocate

	/* Start the kernel */
	la tp, init_thread_union
	li sp, THREAD_SIZE
	add sp, sp, tp
	tail start_kernel

relocate:
	/* Relocate return address */
	li a1, PAGE_OFFSET
	la a0, _start
	sub a1, a1, a0
	add ra, ra, a1

	/* Point stvec to virtual address of intruction after sptbr write */
	la a0, 1f
	add a0, a0, a1
	csrw stvec, a0

	/* Compute sptbr for kernel page tables, but don't load it yet */
	la a2, swapper_pg_dir
	srl a2, a2, PAGE_SHIFT
	li a1, SPTBR_MODE
	or a2, a2, a1

	/* Load trampoline page directory, which will cause us to trap to
	   stvec if VA != PA, or simply fall through if VA == PA */
	la a0, trampoline_pg_dir
	srl a0, a0, PAGE_SHIFT
	or a0, a0, a1
	sfence.vma
	csrw sptbr, a0
1:
	/* Set trap vector to spin forever to help debug */
	la a0, .Lsecondary_park
	csrw stvec, a0

	/* Load the global pointer */
	la gp, _gp

	/* Switch to kernel page tables */
	csrw sptbr, a2

	ret

.Lsecondary_start:
#ifdef CONFIG_SMP
	li a1, CONFIG_NR_CPUS
	bgeu a0, a1, .Lsecondary_park

	la a1, __cpu_up_stack_pointer
	slli a0, a0, LGREG
	add a0, a0, a1

.Lwait_for_cpu_up:
	REG_L sp, (a0)
	beqz sp, .Lwait_for_cpu_up
	fence

	/* Enable virtual memory and relocate to virtual address */
	call relocate

	/* Initialize task_struct pointer */
	li tp, -THREAD_SIZE
	add tp, tp, sp

	tail smp_callin
#endif

.Lsecondary_park:
	/* We lack SMP support or have too many harts, so park this hart */
	wfi
	j .Lsecondary_park
END(_start)

__PAGE_ALIGNED_BSS
	/* Empty zero page */
	.balign PAGE_SIZE
ENTRY(empty_zero_page)
	.fill (empty_zero_page + PAGE_SIZE) - ., 1, 0x00
END(empty_zero_page)
