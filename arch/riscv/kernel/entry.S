#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>

	.altmacro
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

	/* If coming from userspace, preserve the user stack pointer and load
	   the kernel stack pointer.  If we came from the kernel, sscratch
	   will contain 0, and we should continue on the current stack. */
	csrrw sp, sscratch, sp
	bnez sp, _save_context

_restore_kernel_sp:
	csrr sp, sscratch
_save_context:
	addi sp, sp, -(PT_SIZE)
	REG_S x1,  PT_RA(sp)
	REG_S x3,  PT_GP(sp)
	REG_S x4,  PT_TP(sp)
	REG_S x5,  PT_T0(sp)
	REG_S x6,  PT_T1(sp)
	REG_S x7,  PT_T2(sp)
	REG_S x8,  PT_S0(sp)
	REG_S x9,  PT_S1(sp)
	REG_S x10, PT_A0(sp)
	REG_S x11, PT_A1(sp)
	REG_S x12, PT_A2(sp)
	REG_S x13, PT_A3(sp)
	REG_S x14, PT_A4(sp)
	REG_S x15, PT_A5(sp)
	REG_S x16, PT_A6(sp)
	REG_S x17, PT_A7(sp)
	REG_S x18, PT_S2(sp)
	REG_S x19, PT_S3(sp)
	REG_S x20, PT_S4(sp)
	REG_S x21, PT_S5(sp)
	REG_S x22, PT_S6(sp)
	REG_S x23, PT_S7(sp)
	REG_S x24, PT_S8(sp)
	REG_S x25, PT_S9(sp)
	REG_S x26, PT_S10(sp)
	REG_S x27, PT_S11(sp)
	REG_S x28, PT_T3(sp)
	REG_S x29, PT_T4(sp)
	REG_S x30, PT_T5(sp)
	REG_S x31, PT_T6(sp)

	csrr s0, sscratch
	csrr s1, sstatus
	csrr s2, sepc
	csrr s3, sbadaddr
	csrr s4, scause
	REG_S s0, PT_SP(sp)
	REG_S s1, PT_STATUS(sp)
	REG_S s2, PT_EPC(sp)
	REG_S s3, PT_BADVADDR(sp)
	REG_S s4, PT_CAUSE(sp)
	.endm

	.macro RESTORE_ALL
	REG_L a0, PT_STATUS(sp)
	li a1, SR_PS | SR_PIE
	REG_L a2, PT_EPC(sp)
	csrc sstatus, a1
	and a0, a0, a1
	csrw sepc, a2
	csrs sstatus, a0

	REG_L x1,  PT_RA(sp)
	REG_L x3,  PT_GP(sp)
	REG_L x4,  PT_TP(sp)
	REG_L x5,  PT_T0(sp)
	REG_L x6,  PT_T1(sp)
	REG_L x7,  PT_T2(sp)
	REG_L x8,  PT_S0(sp)
	REG_L x9,  PT_S1(sp)
	REG_L x10, PT_A0(sp)
	REG_L x11, PT_A1(sp)
	REG_L x12, PT_A2(sp)
	REG_L x13, PT_A3(sp)
	REG_L x14, PT_A4(sp)
	REG_L x15, PT_A5(sp)
	REG_L x16, PT_A6(sp)
	REG_L x17, PT_A7(sp)
	REG_L x18, PT_S2(sp)
	REG_L x19, PT_S3(sp)
	REG_L x20, PT_S4(sp)
	REG_L x21, PT_S5(sp)
	REG_L x22, PT_S6(sp)
	REG_L x23, PT_S7(sp)
	REG_L x24, PT_S8(sp)
	REG_L x25, PT_S9(sp)
	REG_L x26, PT_S10(sp)
	REG_L x27, PT_S11(sp)
	REG_L x28, PT_T3(sp)
	REG_L x29, PT_T4(sp)
	REG_L x30, PT_T5(sp)
	REG_L x31, PT_T6(sp)

	REG_L x2,  PT_SP(sp)
	.endm

ENTRY(handle_exception)
	SAVE_ALL

	/* Set sscratch register to 0, so that if a recursive exception
	   occurs, the exception vector knows it came from the kernel */
	csrw sscratch, x0

	/* Compute address of current thread_info */
	li tp, ~(THREAD_SIZE-1)
	and tp, tp, sp
	/* Set the current thread pointer */
	REG_L tp, 0(tp)

	csrr s0, scause
	la gp, _gp
	la ra, ret_from_exception
	/* MSB of cause differentiates between
	   interrupts and exceptions */
	bge s0, zero, 1f

	/* Handle interrupts */
	slli a0, s0, 1
	srli a0, a0, 1
	move a1, sp
	tail do_IRQ
1:
	/* Handle syscalls */
	li s1, EXC_SYSCALL
	beq s0, s1, handle_syscall

	/* Handle other exceptions */
	move  a0, sp /* pt_regs */
1:
	la s1, excp_vect_table
	la s2, excp_vect_table_end
	slli s0, s0, LGPTR
	add s1, s1, s0
	/* Check if exception code lies within bounds */
	bgeu s1, s2, 1f
	REG_L s1, 0(s1)
	jr s1
1:
	tail do_trap_unknown

handle_syscall:
	/* Advance EPC to avoid executing the original
	   scall instruction on sret */
	addi s2, s2, 0x4
	REG_S s2, PT_EPC(sp)
	/* System calls run with interrupts enabled */
	csrs sstatus, SR_IE
	li t0, __NR_syscalls
	la s0, sys_ni_syscall
	/* Syscall number held in a7 */
	bgeu a7, t0, 1f
	la s0, sys_call_table
	slli t0, a7, LGPTR
	add s0, s0, t0
	REG_L s0, 0(s0)
1:
	jalr s0

ret_from_syscall:
	/* Set user a0 to kernel a0 */
	REG_S a0, PT_A0(sp)

ret_from_exception:
	REG_L s0, PT_STATUS(sp)
	csrc sstatus, SR_IE
	andi s0, s0, SR_PS
	bnez s0, restore_all

resume_userspace:
	/* Interrupts must be disabled here so flags are checked atomically */
	REG_L s0, TASK_THREAD_INFO(tp)
	REG_L s0, TI_FLAGS(s0) /* current_thread_info->flags */
	andi s1, s0, _TIF_WORK_MASK
	bnez s1, work_pending

	/* Save unwound kernel stack pointer in sscratch */
	addi s0, sp, PT_SIZE
	csrw sscratch, s0
restore_all:
	RESTORE_ALL
	sret

work_pending:
	/* Enter slow path for supplementary processing */
	la ra, ret_from_exception
	andi s1, s0, _TIF_NEED_RESCHED
	bnez s1, work_resched
work_notifysig:
	/* Handle pending signals and notify-resume requests */
	csrs sstatus, SR_IE /* Enable interrupts for do_notify_resume() */
	move a0, sp /* pt_regs */
	move a1, s0 /* current_thread_info->flags */
	tail do_notify_resume
work_resched:
	tail schedule

END(handle_exception)


ENTRY(ret_from_fork)
	la ra, ret_from_exception
	tail schedule_tail
ENDPROC(ret_from_fork)

ENTRY(ret_from_kernel_thread)
	call schedule_tail
	/* Call fn(arg) */
	la ra, ret_from_exception
	move a0, s1
	jr s0
ENDPROC(ret_from_kernel_thread)


/*
 * Register context switch
 * The callee-saved registers must be saved and restored.
 * 
 *   a0: previous task_struct (must be preserved across the switch)
 *   a1: next task_struct
 */
ENTRY(__switch_to)
	/* Save context into prev->thread */
	REG_S ra,  THREAD_RA(a0)
	REG_S s0,  THREAD_S0(a0)
	REG_S s1,  THREAD_S1(a0)
	REG_S s2,  THREAD_S2(a0)
	REG_S s3,  THREAD_S3(a0)
	REG_S s4,  THREAD_S4(a0)
	REG_S s5,  THREAD_S5(a0)
	REG_S s6,  THREAD_S6(a0)
	REG_S s7,  THREAD_S7(a0)
	REG_S s8,  THREAD_S8(a0)
	REG_S s9,  THREAD_S9(a0)
	REG_S s10, THREAD_S10(a0)
	REG_S s11, THREAD_S11(a0)
	REG_S sp,  THREAD_SP(a0)
	/* Restore context from next->thread */
	REG_L ra,  THREAD_RA(a1)
	REG_L s0,  THREAD_S0(a1)
	REG_L s1,  THREAD_S1(a1)
	REG_L s2,  THREAD_S2(a1)
	REG_L s3,  THREAD_S3(a1)
	REG_L s4,  THREAD_S4(a1)
	REG_L s5,  THREAD_S5(a1)
	REG_L s6,  THREAD_S6(a1)
	REG_L s7,  THREAD_S7(a1)
	REG_L s8,  THREAD_S8(a1)
	REG_L s9,  THREAD_S9(a1)
	REG_L s10, THREAD_S10(a1)
	REG_L s11, THREAD_S11(a1)
	REG_L sp,  THREAD_SP(a1)
	mv tp, a1 /* Next current pointer */
	ret
ENDPROC(__switch_to)


	.section ".rodata"
	/* Exception vector table */
ENTRY(excp_vect_table)
	PTR do_trap_insn_misaligned
	PTR do_page_fault
	PTR do_trap_insn_illegal
	PTR do_trap_insn_privileged
	PTR 0 /* handle_syscall */
	PTR do_trap_unknown
	PTR do_trap_unknown
	PTR do_trap_break
	PTR do_trap_load_misaligned
	PTR do_page_fault
	PTR do_trap_store_misaligned
	PTR do_page_fault
excp_vect_table_end:
END(excp_vect_table)

