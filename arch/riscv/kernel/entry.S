#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/pcr.h>
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>

	.altmacro
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

	/* Save stack pointer */
	mtpcr sp, PCR_SUP1
	/* Check if originated from user mode */
	mfpcr sp, PCR_STATUS
	andi sp, sp, SR_PS
	bnez sp, _restore_kernel_sp

	/* Switch to kernel mode stack; load stack
	   pointer from current->thread.sp */
	mfpcr sp, PCR_SUP0
	ld sp, THREAD_SP(sp)
	j _save_context

_restore_kernel_sp:
	mfpcr sp, PCR_SUP1
_save_context:
	addi sp, sp, -(PT_SIZE)
	sd x0,  PT_ZERO(sp)
	sd x1,  PT_RA(sp)
	sd x2,  PT_S0(sp)
	sd x3,  PT_S1(sp)
	sd x4,  PT_S2(sp)
	sd x5,  PT_S3(sp)
	sd x6,  PT_S4(sp)
	sd x7,  PT_S5(sp)
	sd x8,  PT_S6(sp)
	sd x9,  PT_S7(sp)
	sd x10, PT_S8(sp)
	sd x11, PT_S9(sp)
	sd x12, PT_S10(sp)
	sd x13, PT_S11(sp)
	sd x15, PT_TP(sp)
	sd x16, PT_V0(sp)
	sd x17, PT_V1(sp)
	sd x18, PT_A0(sp)
	sd x19, PT_A1(sp)
	sd x20, PT_A2(sp)
	sd x21, PT_A3(sp)
	sd x22, PT_A4(sp)
	sd x23, PT_A5(sp)
	sd x24, PT_A6(sp)
	sd x25, PT_A7(sp)
	sd x26, PT_T0(sp)
	sd x27, PT_T1(sp)
	sd x28, PT_T2(sp)
	sd x29, PT_T3(sp)
	sd x30, PT_T4(sp)
	sd x31, PT_T5(sp)

	mfpcr s0, PCR_SUP1
	sd s0, PT_SP(sp)
	mfpcr s0, PCR_STATUS
	sd s0, PT_STATUS(sp)
	mfpcr s0, PCR_EPC
	sd s0, PT_EPC(sp)
	mfpcr s0, PCR_BADVADDR
	sd s0, PT_BADVADDR(sp)
	mfpcr s0, PCR_CAUSE
	sd s0, PT_CAUSE(sp)
	.endm

	.macro RESTORE_ALL
	clearpcr v1, PCR_STATUS, SR_EI
	ld v0, PT_STATUS(sp)
	li s0, ~(SR_IM | SR_EI)
	li s1, (SR_IM)
	and v0, v0, s0
	and v1, v1, s1
	/* Retain current IM field */
	or v0, v0, v1
	mtpcr v0, PCR_STATUS

	/* Save unwound kernel stack pointer
	   into current->thread.sp */
	addi s0, sp, PT_SIZE
	mfpcr v0, PCR_SUP0
	sd s0, THREAD_SP(v0)

	ld v0, PT_EPC(sp)
	mtpcr v0, PCR_EPC

	ld x1,  PT_RA(sp)
	ld x2,  PT_S0(sp)
	ld x3,  PT_S1(sp)
	ld x4,  PT_S2(sp)
	ld x5,  PT_S3(sp)
	ld x6,  PT_S4(sp)
	ld x7,  PT_S5(sp)
	ld x8,  PT_S6(sp)
	ld x9,  PT_S7(sp)
	ld x10, PT_S8(sp)
	ld x11, PT_S9(sp)
	ld x12, PT_S10(sp)
	ld x13, PT_S11(sp)
	ld x15, PT_TP(sp)
	ld x16, PT_V0(sp)
	ld x17, PT_V1(sp)
	ld x18, PT_A0(sp)
	ld x19, PT_A1(sp)
	ld x20, PT_A2(sp)
	ld x21, PT_A3(sp)
	ld x22, PT_A4(sp)
	ld x23, PT_A5(sp)
	ld x24, PT_A6(sp)
	ld x25, PT_A7(sp)
	ld x26, PT_T0(sp)
	ld x27, PT_T1(sp)
	ld x28, PT_T2(sp)
	ld x29, PT_T3(sp)
	ld x30, PT_T4(sp)
	ld x31, PT_T5(sp)

	ld x14, PT_SP(sp)
	.endm

ENTRY(handle_exception)
	SAVE_ALL
	mfpcr s0, PCR_CAUSE
	la ra, handle_exception_tail
	/* MSB of cause differentiates between
	   interrupts and exceptions */
	bge s0, zero, 1f

	/* Handle interrupts */
	slli a0, s0, 1
	srli a0, a0, 1
	move a1, sp
	j do_IRQ
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
	j handle_fault_unknown

handle_syscall:
	/* System calls run with interrupts enabled */
	setpcr zero, PCR_STATUS, SR_EI
	/* Advance EPC to avoid executing the original
	   syscall instruction on eret */
	ld s0, PT_EPC(sp)
	addi s0, s0, 0x4
	sd s0, PT_EPC(sp)
	la s0, sys_call_table
	/* Syscall number held in v0 */
	slli v0, v0, 3
	add s0, s0, v0
	ld s0, 0(s0)
	jalr s0
	/* Set user v0 to kernel v0 */
	sd v0, PT_V0(sp)
	/* Set user a3 to 0/1 for success/error
	   (per MIPS ABI convention) */
	slt s0, v0, zero
	sd s0, PT_A3(sp)

handle_exception_tail:
	clearpcr s0, PCR_STATUS, SR_EI
	andi s0, s0, SR_PS
	bnez s0, restore_and_return
	/* Otherwise resume userspace */

resume_userspace:
	mfpcr s0, PCR_SUP0
	ld s0, TASK_THREAD_INFO(s0)
	ld s0, TI_FLAGS(s0) /* current_thread_info->flags */

	li s1, (_TIF_WORK_MASK)
	and s1, s0, s1
	beqz s1, restore_and_return
	la ra, resume_userspace

	andi s1, s0, (_TIF_NEED_RESCHED)
	beqz s1, work_notifysig
	j schedule

work_notifysig:
	/* Handle pending signals and notify-resume requests */
	move a0, sp /* pt_regs */
	move a1, s0 /* current_thread_info->flags */
	j do_notify_resume

ENTRY(ret_from_fork)
	jal schedule_tail
restore_and_return:
	RESTORE_ALL
	eret
END(ret_from_fork)

END(handle_exception)

/* 
 * Wrapper routines are necessary to accommodate system calls
 * that require a pt_regs pointer, provided as an additional
 * argument at the position specified by \index.
 * The original arguments are passed through unmodified.
 * 
 * The address of the pt_regs struct on the kernel mode stack
 * is assumed to be the current value of sp.
 *
 * Note: These routines are intended to be called exclusively
 * by the syscall trap handler.
 */
	.macro PTREGS_SYSCALL func index
ENTRY(\func)
	move a\index, sp /* pt_regs pointer */
	j __\func
END(\func)
	.endm

	PTREGS_SYSCALL sys_sigaltstack, 2
	PTREGS_SYSCALL sys_rt_sigreturn, 0
	PTREGS_SYSCALL sys_clone, 5
	PTREGS_SYSCALL sys_execve, 3

	/* Callee-save registers only */
	.macro SAVE_SWITCH_CONTEXT
	addi sp, sp, -0x68
	sd ra, 0x00(sp)
	sd s0, 0x08(sp)
	sd s1, 0x10(sp)
	sd s2, 0x18(sp)
	sd s3, 0x20(sp)
	sd s4, 0x28(sp)
	sd s5, 0x30(sp)
	sd s6, 0x38(sp)
	sd s7, 0x40(sp)
	sd s8, 0x48(sp)
	sd s9, 0x50(sp)
	sd s10, 0x58(sp)
	sd s11, 0x60(sp)
	.endm

	.macro RESTORE_SWITCH_CONTEXT
	ld ra, 0x00(sp)
	ld s0, 0x08(sp)
	ld s1, 0x10(sp)
	ld s2, 0x18(sp)
	ld s3, 0x20(sp)
	ld s4, 0x28(sp)
	ld s5, 0x30(sp)
	ld s6, 0x38(sp)
	ld s7, 0x40(sp)
	ld s8, 0x48(sp)
	ld s9, 0x50(sp)
	ld s10, 0x58(sp)
	ld s11, 0x60(sp)
	addi sp, sp, 0x68
	.endm

/*
 * Register context switch
 * The callee-saved registers must be saved and restored.
 * 
 *   a0: previous task_struct (must be preserved across the switch)
 *   a1: next task_struct
 */
ENTRY(__switch_to)
	SAVE_SWITCH_CONTEXT
	la ra, 1f             /* Return address when resuming prev */
	sd sp, THREAD_SP(a0)  /* prev->thread.sp = sp */
#ifdef CONFIG_FRAME_POINTER
	sd s0, THREAD_FP(a0)  /* prev->thread.sp = sp */
#endif
	sd ra, THREAD_PC(a0)  /* prev->thread.pc = 1f */ 
	ld sp, THREAD_SP(a1)  /* sp = next->thread.sp */
	ld ra, THREAD_PC(a1)  /* ra = next->thread.pc */
	mtpcr a1, PCR_SUP0    /* Next current pointer */
	ret
1: 
	RESTORE_SWITCH_CONTEXT
	move v0, a0  /* Preserve reference */
	ret
END(__switch_to)


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

