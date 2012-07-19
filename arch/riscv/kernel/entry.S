#include <linux/init.h>

#include <asm/pcr.h>

.macro SAVE_ALL
	addi sp, sp, -288
	sd x0, 0(sp)
	sd x1, 8(sp)
	sd x2, 16(sp)
	sd x3, 24(sp)
	sd x4, 32(sp)
	sd x5, 40(sp)
	sd x6, 48(sp)
	sd x7, 56(sp)
	sd x8, 64(sp)
	sd x9, 72(sp)
	sd x10, 80(sp)
	sd x11, 88(sp)
	sd x12, 96(sp)
	sd x13, 104(sp)
	sd x14, 112(sp)
	sd x15, 120(sp)
	sd x16, 128(sp)
	sd x17, 136(sp)
	sd x18, 144(sp)
	sd x19, 152(sp)
	sd x20, 160(sp)
	sd x21, 168(sp)
	sd x22, 176(sp)
	sd x23, 184(sp)
	sd x24, 192(sp)
	sd x25, 200(sp)
	sd x26, 208(sp)
	sd x27, 216(sp)
	sd x28, 224(sp)
	sd x29, 232(sp)
	sd x31, 248(sp)
	mfpcr x1, PCR_STATUS
	sd x1, 264(sp)
	mfpcr x1, PCR_EPC
	sd x1, 280(sp)
.endm

.macro RESTORE_ALL
	ld x1, 280(sp)
	mtpcr x1, PCR_EPC
	ld x1, 264(sp)
	mtpcr x1, PCR_STATUS
	ld x1, 8(sp)
	ld x2, 16(sp)
	ld x3, 24(sp)
	ld x4, 32(sp)
	ld x5, 40(sp)
	ld x6, 48(sp)
	ld x7, 56(sp)
	ld x8, 64(sp)
	ld x9, 72(sp)
	ld x10, 80(sp)
	ld x11, 88(sp)
	ld x12, 96(sp)
	ld x13, 104(sp)
	ld x14, 112(sp)
	ld x15, 120(sp)
	ld x16, 128(sp)
	ld x17, 136(sp)
	ld x18, 144(sp)
	ld x19, 152(sp)
	ld x20, 160(sp)
	ld x21, 168(sp)
	ld x22, 176(sp)
	ld x23, 184(sp)
	ld x24, 192(sp)
	ld x25, 200(sp)
	ld x26, 208(sp)
	ld x27, 216(sp)
	ld x28, 224(sp)
	ld x29, 232(sp)
	ld x31, 248(sp)
	addi sp, sp, 288
.endm

	.global handle_exception
	.ent handle_exception
handle_exception:
	SAVE_ALL
	mfpcr a0, PCR_CAUSE

	/* Temporary workaround to accommodate simulator's
	   different exception cause values */
	addi v0, x0, 1
	slli v0, v0, 63
	andi a0, a0, 0xf
	beqz v0, 1f

	move a1, sp
	jal do_IRQ
	j 2f
1:
	mfpcr a1, PCR_EPC
	mfpcr a2, PCR_BADVADDR
	jal do_exception
2:
	mfpcr t0, PCR_STATUS
	andi t0, t0, SR_PS
	beqz t0, resume_userspace
	j restore_and_return
resume_userspace:
	jal schedule
restore_and_return:
	RESTORE_ALL
	eret
	.end handle_exception

	.global ret_from_fork
	.ent ret_from_fork
ret_from_fork:
	jal schedule_tail
	/* Change back to user mode? */
	RESTORE_ALL
	ret
	.end ret_from_fork

/* Not the entire context, just the callee save registers, and a2 */
.macro SAVE_SWITCH_CONTEXT
	addi sp, sp, -96
	sd ra, 0(sp)
	sd s0, 8(sp)
	sd s1, 16(sp)
	sd s2, 24(sp)
	sd s3, 32(sp)
	sd s4, 40(sp)
	sd s5, 48(sp)
	sd s6, 56(sp)
	sd s7, 64(sp)
	sd s8, 72(sp)
	sd s9, 80(sp)
	sd a2, 88(sp)
.endm

.macro RESTORE_SWITCH_CONTEXT
	ld ra, 0(sp)
	ld s0, 8(sp)
	ld s1, 16(sp)
	ld s2, 24(sp)
	ld s3, 32(sp)
	ld s4, 40(sp)
	ld s5, 48(sp)
	ld s6, 56(sp)
	ld s7, 64(sp)
	ld s8, 72(sp)
	ld s9, 80(sp)
	ld a2, 88(sp)
	addi sp, sp, 96
.endm


/* 
 * void switch_to(struct task_struct *prev,
 *     struct task_struct *next, struct task_struct *last);
 */
.global switch_to
.ent switch_to
switch_to:
	SAVE_SWITCH_CONTEXT /* Save ra, s[0-9], and a2 */
	sd sp, 728(a0)      /* prev->thread.sp = sp */
	la ra, 1f           /* Create return addr when we come back to prev */
	sd ra, 744(a0)      /* prev->thread.pc = 1f */ 
	sd tp, 752(a0)      /* prev->thread.tp = tp */
	ld sp, 728(a1)      /* sp = next->thread.sp */
	ld ra, 744(a1)      /* ra = next->thread.pc */
	ld tp, 752(a1)      /* tp = next->thread.tp */
	j __switch_to       /* == { mov v0, a0; ret } */
1: 
	RESTORE_SWITCH_CONTEXT /* Restore ra, s[0-9], and a2 */
	sd v0, 0(a2)        /* *last = prev */
	ret
.end switch_to

