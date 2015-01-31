#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/csr.h>
#include <asm/unistd.h>
#include <asm/errno.h>
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>

	.altmacro
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

	/* Save stack pointer */
	csrw sup1, sp
	/* Check if originated from user mode */
	csrr sp, status
	andi sp, sp, SR_PS
	bnez sp, _restore_kernel_sp

	/* Switch to kernel mode stack; load stack
	   pointer from current->thread.sp */
	csrr sp, sup0
	ld sp, THREAD_SP(sp)
	j _save_context

_restore_kernel_sp:
	csrr sp, sup1
_save_context:
	addi sp, sp, -(PT_SIZE)
	sd x1,  PT_RA(sp)
	sd x3,  PT_GP(sp)
	sd x4,  PT_TP(sp)
	sd x5,  PT_T0(sp)
	sd x6,  PT_T1(sp)
	sd x7,  PT_T2(sp)
	sd x8,  PT_S0(sp)
	sd x9,  PT_S1(sp)
	sd x10, PT_A0(sp)
	sd x11, PT_A1(sp)
	sd x12, PT_A2(sp)
	sd x13, PT_A3(sp)
	sd x14, PT_A4(sp)
	sd x15, PT_A5(sp)
	sd x16, PT_A6(sp)
	sd x17, PT_A7(sp)
	sd x18, PT_S2(sp)
	sd x19, PT_S3(sp)
	sd x20, PT_S4(sp)
	sd x21, PT_S5(sp)
	sd x22, PT_S6(sp)
	sd x23, PT_S7(sp)
	sd x24, PT_S8(sp)
	sd x25, PT_S9(sp)
	sd x26, PT_S10(sp)
	sd x27, PT_S11(sp)
	sd x28, PT_T3(sp)
	sd x29, PT_T4(sp)
	sd x30, PT_T5(sp)
	sd x31, PT_T6(sp)

	csrr s0, sup1
	csrr s1, status
	csrr s2, epc
	csrr s3, badvaddr
	csrr s4, cause
	sd s0, PT_SP(sp)
	sd s1, PT_STATUS(sp)
	sd s2, PT_EPC(sp)
	sd s3, PT_BADVADDR(sp)
	sd s4, PT_CAUSE(sp)
	.endm

	.macro RESTORE_ALL
	csrrc a1, status, SR_EI
	ld a0, PT_STATUS(sp)
	li s0, ~(SR_IM | SR_EI)
	ld a2, PT_EPC(sp)
	csrr a3, sup0
	li s1, (SR_IM)
	and a0, a0, s0
	and a1, a1, s1
	/* Retain current IM field */
	or a0, a0, a1
	csrw status, a0

	/* Save unwound kernel stack pointer
	   into current->thread.sp */
	addi s0, sp, PT_SIZE
	sd s0, THREAD_SP(a3)

	csrw epc, a2

	ld x1,  PT_RA(sp)
	ld x3,  PT_GP(sp)
	ld x4,  PT_TP(sp)
	ld x5,  PT_T0(sp)
	ld x6,  PT_T1(sp)
	ld x7,  PT_T2(sp)
	ld x8,  PT_S0(sp)
	ld x9,  PT_S1(sp)
	ld x10, PT_A0(sp)
	ld x11, PT_A1(sp)
	ld x12, PT_A2(sp)
	ld x13, PT_A3(sp)
	ld x14, PT_A4(sp)
	ld x15, PT_A5(sp)
	ld x16, PT_A6(sp)
	ld x17, PT_A7(sp)
	ld x18, PT_S2(sp)
	ld x19, PT_S3(sp)
	ld x20, PT_S4(sp)
	ld x21, PT_S5(sp)
	ld x22, PT_S6(sp)
	ld x23, PT_S7(sp)
	ld x24, PT_S8(sp)
	ld x25, PT_S9(sp)
	ld x26, PT_S10(sp)
	ld x27, PT_S11(sp)
	ld x28, PT_T3(sp)
	ld x29, PT_T4(sp)
	ld x30, PT_T5(sp)
	ld x31, PT_T6(sp)

	ld x2,  PT_SP(sp)
	.endm

ENTRY(handle_exception)
	SAVE_ALL
	csrr s0, cause
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
	slli s0, s0, 3
	add s1, s1, s0
	/* Check if exception code lies within bounds */
	bgeu s1, s2, 1f
	ld s1, 0(s1)
	jr s1
1:
	tail handle_fault_unknown

handle_syscall:
	/* Advance EPC to avoid executing the original
	   scall instruction on sret */
	addi s2, s2, 0x4
	sd s2, PT_EPC(sp)
	/* System calls run with interrupts enabled */
	csrs status, SR_EI
	li t0, __NR_syscalls
	/* Syscall number held in a7 */
	bgeu a7, t0, bad_syscall_number
	la s0, sys_call_table
	slli t0, a7, 3
	add s0, s0, t0
	ld s0, 0(s0)
	sd a7, PT_SYSCALLNO(sp) /* save in case of restart */
	jalr s0

ret_from_syscall:
	/* Set user a0 to kernel a0 */
	sd a0, PT_A0(sp)

ret_from_exception:
	ld s0, PT_STATUS(sp)
	andi s0, s0, SR_PS
	bnez s0, restore_all

resume_userspace:
	csrc status, SR_EI /* Disable interrupts to ensure that thread
	                      info flags are checked atomically */
	csrr s0, sup0
	ld s0, TASK_THREAD_INFO(s0)
	ld s0, TI_FLAGS(s0) /* current_thread_info->flags */
	andi s1, s0, _TIF_WORK_MASK
	bnez s1, work_pending

restore_all:
	RESTORE_ALL
	sret

work_pending:
	/* Enter slow path for supplementary processing */
	la ra, resume_userspace
	andi s1, s0, _TIF_NEED_RESCHED
	bnez s1, work_resched
work_notifysig:
	/* Handle pending signals and notify-resume requests */
	csrs status, SR_EI /* Enable interrupts for do_notify_resume() */
	move a0, sp /* pt_regs */
	move a1, s0 /* current_thread_info->flags */
	tail do_notify_resume
work_resched:
	tail schedule

bad_syscall_number:
	li a0, -ENOSYS
	j ret_from_syscall
END(handle_exception)


ENTRY(ret_from_fork)
	la ra, restore_all
	tail schedule_tail
ENDPROC(ret_from_fork)

ENTRY(ret_from_kernel_thread)
	call schedule_tail
	/* Call fn(arg) */
	la ra, restore_all
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
	sd ra,  THREAD_RA(a0)
	sd s0,  THREAD_S0(a0)
	sd s1,  THREAD_S1(a0)
	sd s2,  THREAD_S2(a0)
	sd s3,  THREAD_S3(a0)
	sd s4,  THREAD_S4(a0)
	sd s5,  THREAD_S5(a0)
	sd s6,  THREAD_S6(a0)
	sd s7,  THREAD_S7(a0)
	sd s8,  THREAD_S8(a0)
	sd s9,  THREAD_S9(a0)
	sd s10, THREAD_S10(a0)
	sd s11, THREAD_S11(a0)
	sd sp,  THREAD_SP(a0)
	/* Restore context from next->thread */
	ld ra,  THREAD_RA(a1)
	ld s0,  THREAD_S0(a1)
	ld s1,  THREAD_S1(a1)
	ld s2,  THREAD_S2(a1)
	ld s3,  THREAD_S3(a1)
	ld s4,  THREAD_S4(a1)
	ld s5,  THREAD_S5(a1)
	ld s6,  THREAD_S6(a1)
	ld s7,  THREAD_S7(a1)
	ld s8,  THREAD_S8(a1)
	ld s9,  THREAD_S9(a1)
	ld s10, THREAD_S10(a1)
	ld s11, THREAD_S11(a1)
	ld sp,  THREAD_SP(a1)
	csrw sup0, a1 /* Next current pointer */
	ret
ENDPROC(__switch_to)


	.section ".rodata"
	/* Exception vector table */
ENTRY(excp_vect_table)
	.quad handle_misaligned_insn
	.quad do_page_fault
	.quad handle_illegal_insn
	.quad handle_privileged_insn
	.quad handle_privileged_insn
	.quad handle_fault_unknown
	.quad 0 /* handle_syscall */
	.quad handle_fault_unknown
	.quad handle_misaligned_data
	.quad handle_misaligned_data
	.quad do_page_fault
	.quad do_page_fault
excp_vect_table_end:
END(excp_vect_table)

