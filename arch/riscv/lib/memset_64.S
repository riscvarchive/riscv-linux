#include <linux/linkage.h>

/* void *memset(void *, int, size_t) */

ENTRY(memset)
	move t0, a0  /* Preserve return value */

	/* Defer to byte-oriented fill for small sizes */
	sltiu a3, a2, 16
	bnez a3, 4f

	/* Round to nearest double word-aligned address
	   greater than or equal to start address */
	addi a3, t0, 0x7
	andi a3, a3, ~(0x7)
	beq a3, t0, 2f  /* Skip if already aligned */
	/* Handle initial misalignment */
	sub a4, a3, t0
1:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 1b
	sub a2, a2, a4  /* Update count */

2: /* Duff's device with 32 double-word stores per iteration */
	/* Broadcast value into all 8 bytes */
	andi a1, a1, 0xff
	slli a3, a1, 8
	or a1, a3, a1
	slli a3, a1, 16
	or a1, a3, a1
	slli a3, a1, 32
	or a1, a3, a1

	/* Calculate end address */
	andi a4, a2, ~(0x7)
	add a3, t0, a4

	andi a4, a4, 0xf8   /* Calculate remainder */
	beqz a4, 3f         /* Shortcut if no remainder */
	sub a4, zero, a4
	addi a4, a4, 0x100  /* Calculate initial offset */

	/* Adjust start address with offset */
	sub t0, t0, a4

	/* Jump into loop body */
	/* Assumes 32-bit instruction lengths */
	la a5, 3f
	srli a4, a4, 1
	add a5, a5, a4
	jr a5
3:
	sd a1, 0x00(t0)
	sd a1, 0x08(t0)
	sd a1, 0x10(t0)
	sd a1, 0x18(t0)
	sd a1, 0x20(t0)
	sd a1, 0x28(t0)
	sd a1, 0x30(t0)
	sd a1, 0x38(t0)
	sd a1, 0x40(t0)
	sd a1, 0x48(t0)
	sd a1, 0x50(t0)
	sd a1, 0x58(t0)
	sd a1, 0x60(t0)
	sd a1, 0x68(t0)
	sd a1, 0x70(t0)
	sd a1, 0x78(t0)
	sd a1, 0x80(t0)
	sd a1, 0x88(t0)
	sd a1, 0x90(t0)
	sd a1, 0x98(t0)
	sd a1, 0xa0(t0)
	sd a1, 0xa8(t0)
	sd a1, 0xb0(t0)
	sd a1, 0xb8(t0)
	sd a1, 0xc0(t0)
	sd a1, 0xc8(t0)
	sd a1, 0xd0(t0)
	sd a1, 0xd8(t0)
	sd a1, 0xe0(t0)
	sd a1, 0xe8(t0)
	sd a1, 0xf0(t0)
	sd a1, 0xf8(t0)
	addi t0, t0, 0x100
	bltu t0, a3, 3b
	andi a2, a2, 0x7  /* Update count */

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
