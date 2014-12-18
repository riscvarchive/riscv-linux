#include <linux/linkage.h>

/* void *memcpy(void *, const void *, size_t) */

ENTRY(memcpy)
	move t6, a0  /* Preserve return value */

	/* Defer to byte-oriented copy for small sizes */
	sltiu a3, a2, 128
	bnez a3, 4f
	/* Use word-oriented copy only if low-order bits match */
	andi a3, t6, 0x7
	andi a4, a1, 0x7
	bne a3, a4, 4f

	beqz a3, 2f  /* Skip if already aligned */
	/* Round to nearest double word-aligned address
	   greater than or equal to start address */
	andi a3, a1, ~(0x7)
	addi a3, a3, 0x8
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
	andi a4, a2, ~(0x7f)
	beqz a4, 4f
	add a3, a1, a4
3:
	ld a4, 0x00(a1)
	ld a5, 0x08(a1)
	ld a6, 0x10(a1)
	ld a7, 0x18(a1)
	ld t0, 0x20(a1)
	ld t1, 0x28(a1)
	ld t2, 0x30(a1)
	ld t3, 0x38(a1)
	ld t4, 0x40(a1)
	ld t5, 0x48(a1)
	sd a4, 0x00(t6)
	sd a5, 0x08(t6)
	sd a6, 0x10(t6)
	sd a7, 0x18(t6)
	sd t0, 0x20(t6)
	sd t1, 0x28(t6)
	sd t2, 0x30(t6)
	sd t3, 0x38(t6)
	sd t4, 0x40(t6)
	sd t5, 0x48(t6)
	ld a4, 0x50(a1)
	ld a5, 0x58(a1)
	ld a6, 0x60(a1)
	ld a7, 0x68(a1)
	ld t0, 0x70(a1)
	ld t1, 0x78(a1)
	addi a1, a1, 0x80
	sd a4, 0x50(t6)
	sd a5, 0x58(t6)
	sd a6, 0x60(t6)
	sd a7, 0x68(t6)
	sd t0, 0x70(t6)
	sd t1, 0x78(t6)
	addi t6, t6, 0x80
	bltu a1, a3, 3b
	andi a2, a2, 0x7f  /* Update count */

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
