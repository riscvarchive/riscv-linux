#include <linux/linkage.h>

/* void *memset(void *, int, size_t) */

ENTRY(memset)
	move t0, a0  /* Preserve return value */

	/* Defer to byte-oriented fill for small sizes */
	sltiu a3, a2, 16
	bnez a3, 4f

	/* Round to nearest word-aligned address
	   greater than or equal to start address */
	addi a3, t0, 0x3
	andi a3, a3, ~(0x3)
	beq a3, t0, 2f  /* Skip if already aligned */
	/* Handle initial misalignment */
	sub a4, a3, t0
1:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 1b
	sub a2, a2, a4  /* Update count */

2: /* Duff's device with 32 double-word stores per iteration */
	/* Broadcast value into all 4 bytes */
	andi a1, a1, 0xff
	slli a3, a1, 8
	or a1, a3, a1
	slli a3, a1, 16
	or a1, a3, a1

	/* Calculate end address */
	andi a4, a2, ~(0x3)
	add a3, t0, a4

	andi a4, a4, 0x7c   /* Calculate remainder */
	beqz a4, 3f         /* Shortcut if no remainder */
	sub a4, zero, a4
	addi a4, a4, 0x80  /* Calculate initial offset */

	/* Adjust start address with offset */
	sub t0, t0, a4

	/* Jump into loop body */
	/* Assumes 32-bit instruction lengths */
	la a5, 3f
	add a5, a5, a4
	jr a5
3:
	sw a1, 0x00(t0)
	sw a1, 0x04(t0)
	sw a1, 0x08(t0)
	sw a1, 0x0c(t0)
	sw a1, 0x10(t0)
	sw a1, 0x14(t0)
	sw a1, 0x18(t0)
	sw a1, 0x1c(t0)
	sw a1, 0x20(t0)
	sw a1, 0x24(t0)
	sw a1, 0x28(t0)
	sw a1, 0x2c(t0)
	sw a1, 0x30(t0)
	sw a1, 0x34(t0)
	sw a1, 0x38(t0)
	sw a1, 0x3c(t0)
	sw a1, 0x40(t0)
	sw a1, 0x44(t0)
	sw a1, 0x48(t0)
	sw a1, 0x4c(t0)
	sw a1, 0x50(t0)
	sw a1, 0x54(t0)
	sw a1, 0x58(t0)
	sw a1, 0x5c(t0)
	sw a1, 0x60(t0)
	sw a1, 0x64(t0)
	sw a1, 0x68(t0)
	sw a1, 0x6c(t0)
	sw a1, 0x70(t0)
	sw a1, 0x74(t0)
	sw a1, 0x78(t0)
	sw a1, 0x7c(t0)
	addi t0, t0, 0x80
	bltu t0, a3, 3b
	andi a2, a2, 0x3  /* Update count */

4:
	/* Handle trailing misalignment */
	beqz a2, 6f
	add a3, t0, a2
5:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 5b
6:
	ret
END(memset)
