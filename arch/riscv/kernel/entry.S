#include <linux/init.h>

#include <asm/pcr.h>

	.altmacro
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

	/* Save stack pointer */
	mtpcr sp, PCR_K1
	/* Check if originated from user mode */
	mfpcr sp, PCR_STATUS
	andi sp, sp, SR_PS
	bnez sp, _restore_kernel_sp

	/* Switch to kernel mode stack; load stack
	   pointer from current->thread.sp */
	mfpcr sp, PCR_K0
	ld sp, 0(sp)
	ld sp, 728(sp)
	j _save_context

_restore_kernel_sp:
	mfpcr sp, PCR_K1
_save_context:
	addi sp, sp, -0x120
	sd x0,  0x000(sp)
	sd x1,  0x008(sp)
	sd x2,  0x010(sp)
	sd x3,  0x018(sp)
	sd x4,  0x020(sp)
	sd x5,  0x028(sp)
	sd x6,  0x030(sp)
	sd x7,  0x038(sp)
	sd x8,  0x040(sp)
	sd x9,  0x048(sp)
	sd x10, 0x050(sp)
	sd x11, 0x058(sp)
	sd x12, 0x060(sp)
	sd x13, 0x068(sp)
	sd x14, 0x070(sp)
	sd x15, 0x078(sp)
	sd x16, 0x080(sp)
	sd x17, 0x088(sp)
	sd x18, 0x090(sp)
	sd x19, 0x098(sp)
	sd x20, 0x0a0(sp)
	sd x21, 0x0a8(sp)
	sd x22, 0x0b0(sp)
	sd x23, 0x0b8(sp)
	sd x24, 0x0c0(sp)
	sd x25, 0x0c8(sp)
	sd x26, 0x0d0(sp)
	sd x27, 0x0d8(sp)
	sd x28, 0x0e0(sp)
	sd x29, 0x0e8(sp)
	mfpcr x1, PCR_K1
	sd x1,  0x0f0(sp)
	sd x31, 0x0f8(sp)
	mfpcr x1, PCR_STATUS
	sd x1,  0x108(sp)
	mfpcr x1, PCR_EPC
	sd x1,  0x118(sp)
	.endm

	.macro RESTORE_ALL
	/* Save kernel stack pointer 
	   into current->thread.sp */
	mfpcr v0, PCR_K0
	ld v0, 0(v0)
	addi sp, sp, 0x120
	sd sp, 728(v0)

	ld x1, -0x08(sp)
	mtpcr x1, PCR_EPC
	ld x1, -0x18(sp)
	/* eret requires SR_ET to be clear */
	andi x1, x1, ~(SR_ET)
	mtpcr x1, PCR_STATUS

	ld x1,  -0x118(sp)
	ld x2,  -0x110(sp)
	ld x3,  -0x108(sp)
	ld x4,  -0x100(sp)
	ld x5,  -0x0f8(sp)
	ld x6,  -0x0f0(sp)
	ld x7,  -0x0e8(sp)
	ld x8,  -0x0e0(sp)
	ld x9,  -0x0d8(sp)
	ld x10, -0x0d0(sp)
	ld x11, -0x0c8(sp)
	ld x12, -0x0c0(sp)
	ld x13, -0x0b8(sp)
	ld x14, -0x0b0(sp)
	ld x15, -0x0a8(sp)
	ld x16, -0x0a0(sp)
	ld x17, -0x098(sp)
	ld x18, -0x090(sp)
	ld x19, -0x088(sp)
	ld x20, -0x080(sp)
	ld x21, -0x078(sp)
	ld x22, -0x070(sp)
	ld x23, -0x068(sp)
	ld x24, -0x060(sp)
	ld x25, -0x058(sp)
	ld x26, -0x050(sp)
	ld x27, -0x048(sp)
	ld x28, -0x040(sp)
	ld x29, -0x038(sp)
	ld x31, -0x028(sp)
	ld x30, -0x030(sp)
	.endm

	.global handle_exception
	.ent handle_exception
handle_exception:
	SAVE_ALL
	mfpcr t0, PCR_CAUSE
	/* MSB of cause differentiates between
	   interrupts and exceptions */
	bge t0, x0, 1f

	/* Handle interrupts */
	slli a0, t0, 1
	srli a0, a0, 1
	move a1, sp
	jal do_IRQ
	j handle_exception_tail
1:
	/* Handle other exceptions */
	la t1, excp_vect_tab
	la t2, excp_vect_tab_end
	slli t0, t0, 3
	add t1, t1, t0
	/* Check if exception code lies within bounds */
	bgeu t1, t2, handle_fault_generic
	ld t1, 0(t1)
	jalr t1
	j handle_exception_tail

handle_fault_generic:
	mfpcr a0, PCR_CAUSE
	mfpcr a1, PCR_EPC
	mfpcr a2, PCR_BADVADDR
	jal report_exception
	j handle_exception_tail

handle_page_fault:
	mfpcr a0, PCR_CAUSE
	mfpcr a1, PCR_EPC
	mfpcr a2, PCR_BADVADDR
	jal do_page_fault
	j handle_exception_tail

handle_syscall:
	la t0, sys_call_table
	/* Syscall number held in v0 */
	slli v0, v0, 3
	add t0, t0, v0
	ld t0, 0(t0)
	jalr t0
handle_syscall_tail:
	/* Advance EPC to avoid executing the original
	   syscall instruction on eret */
	ld t0, 0x118(sp)
	addi t0, t0, 0x4
	sw t0, 0x118(sp)
#	j handle_exception_tail

handle_exception_tail:
	mfpcr t0, PCR_STATUS
	andi t0, t0, SR_PS
	bnez t0, restore_and_return
	/* Resume userspace */
	jal schedule
restore_and_return:
	RESTORE_ALL
	eret
	.end handle_exception

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
	.global \func
	.ent \func
\func:
	move a\index, sp /* pt_regs pointer */
	jal __\func
	/* Hard-code return destination to avoid saving ra */
	j handle_syscall_tail
	.end \func
	.endm

	PTREGS_SYSCALL sys_sigaltstack, 2
	PTREGS_SYSCALL sys_rt_sigreturn, 0
	PTREGS_SYSCALL sys_clone, 4
	PTREGS_SYSCALL sys_execve, 3


	.global ret_from_fork
	.ent ret_from_fork
ret_from_fork:
	jal schedule_tail
	/* Change back to user mode? */
	RESTORE_ALL
	ret
	.end ret_from_fork


	/* Callee-save registers and a2 only */
	.macro SAVE_SWITCH_CONTEXT
	addi sp, sp, -0x60
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
	sd a2, 0x58(sp)
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
	ld a2, 0x58(sp)
	addi sp, sp, 0x60
	.endm


/* 
 * void switch_to(struct task_struct *prev,
 *     struct task_struct *next, struct task_struct *last);
 */
	.global switch_to
	.ent switch_to
switch_to:
	SAVE_SWITCH_CONTEXT /* Save ra, s[0-9], and a2 */
	la ra, 1f           /* Create return addr when we come back to prev */
	mfpcr v0, PCR_K0    /* Previous thread_info struct pointer */
	sd sp, 728(a0)      /* prev->thread.sp = sp */
	sd ra, 744(a0)      /* prev->thread.pc = 1f */ 
	sd v0, 752(a0)      /* prev->thread.tp = tp */
	ld sp, 728(a1)      /* sp = next->thread.sp */
	ld ra, 744(a1)      /* ra = next->thread.pc */
	ld v0, 752(a1)      /* tp = next->thread.tp */
	mtpcr v0, PCR_K0    /* Next thread_info struct pointer */
	ret
1: 
	move v0, a0
	RESTORE_SWITCH_CONTEXT /* Restore ra, s[0-9], and a2 */
	sd v0, 0(a2)        /* *last = prev */
	ret
	.end switch_to


	.section ".rodata"
	/* Exception vector table */
	.global excp_vect_tab
excp_vect_tab:
	.quad handle_fault_generic
	.quad handle_page_fault
	.quad handle_fault_generic
	.quad handle_fault_generic
	.quad handle_fault_generic
	.quad handle_fault_generic
	.quad handle_syscall
	.quad handle_fault_generic
	.quad handle_fault_generic
	.quad handle_fault_generic
	.quad handle_page_fault
	.quad handle_page_fault
	.quad handle_fault_generic
excp_vect_tab_end:

