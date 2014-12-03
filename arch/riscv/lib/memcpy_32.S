#include <linux/linkage.h>

/* void *memcpy(void *, const void *, size_t) */

ENTRY(memcpy)
	move t6, a0  /* Preserve return value */

	/* Defer to byte-oriented copy for small sizes */
	sltiu a3, a2, 64
	bnez a3, 4f
	/* Use word-oriented copy only if low-order bits match */
	andi a3, t6, 0x3
	andi a4, a1, 0x3
	bne a3, a4, 4f

	beqz a3, 2f  /* Skip if already aligned */
	/* Round to nearest word-aligned address
	   greater than or equal to start address */
	andi a3, a1, ~(0x3)
	addi a3, a3, 0x4
	/* Handle initial misalignment */
	sub a4, a3, a1
1:
	lb a5, 0(a1)
	addi a1, a1, 1
	sb a5, 0(t6)
	addi t6, t6, 1
	bltu a1, a3, 1b
	sub a2, a2, a4  /* Update count */

2:
	andi a4, a2, ~(0x3f)
	beqz a4, 4f
	add a3, a1, a4
3:
	lw a4, 0x00(a1)
	lw a5, 0x04(a1)
	lw a6, 0x08(a1)
	lw a7, 0x0c(a1)
	lw t0, 0x10(a1)
	lw t1, 0x14(a1)
	lw t2, 0x18(a1)
	lw t3, 0x1c(a1)
	lw t4, 0x20(a1)
	lw t5, 0x24(a1)
	sw a4, 0x00(t6)
	sw a5, 0x04(t6)
	sw a6, 0x08(t6)
	sw a7, 0x0c(t6)
	sw t0, 0x10(t6)
	sw t1, 0x14(t6)
	sw t2, 0x18(t6)
	sw t3, 0x1c(t6)
	sw t4, 0x20(t6)
	sw t5, 0x24(t6)
	lw a4, 0x28(a1)
	lw a5, 0x2c(a1)
	lw a6, 0x30(a1)
	lw a7, 0x34(a1)
	lw t0, 0x38(a1)
	lw t1, 0x3c(a1)
	addi a1, a1, 0x40
	sw a4, 0x28(t6)
	sw a5, 0x2c(t6)
	sw a6, 0x30(t6)
	sw a7, 0x34(t6)
	sw t0, 0x38(t6)
	sw t1, 0x3c(t6)
	addi t6, t6, 0x40
	bltu a1, a3, 3b
	andi a2, a2, 0x3f  /* Update count */

4:
	/* Handle trailing misalignment */
	beqz a2, 6f
	add a3, a1, a2
5:
	lb a4, 0(a1)
	addi a1, a1, 1
	sb a4, 0(t6)
	addi t6, t6, 1
	bltu a1, a3, 5b
6:
	ret
END(memcpy)
