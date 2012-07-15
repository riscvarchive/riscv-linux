#include <linux/init.h>

#include <asm/processor.h>
#include <asm/regs.h>

.macro SAVE_ALL
	addi sp, sp, -248
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
	sd x30, 240(sp)
.endm

.macro RESTORE_ALL
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
	ld x30, 240(sp)
	addi sp, sp, 248
.endm

.global handle_exception
.ent handle_exception
handle_exception:
	SAVE_ALL
	mfpcr a0, PCR_CAUSE
	andi a0, a0, 0x1f /* Cause only [4:0] */
	mfpcr a1, PCR_EPC
	mfpcr a2, PCR_BADVADDR
	jal do_exception

	mfpcr t0, PCR_STATUS
	li t1, SR_PS
	and t0, t0, t1
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

.global kernel_thread_helper
.ent kernel_thread_helper
kernel_thread_helper:
	jalr t0
	move a0, v0
	j do_exit
.end kernel_thread_helper

