#include <linux/linkage.h>

	.altmacro
	.macro fixup op reg addr lbl
	LOCAL _epc
_epc:
	\op \reg, \addr
	.section __ex_table,"a"
	.balign 8
	.quad _epc, \lbl
	.previous
	.endm

ENTRY(__copy_user)
	add a3, a1, a2
	/* Use word-oriented copy only if low-order bits match */
	andi t0, a0, 0x7
	andi t1, a1, 0x7
	bne t0, t1, 2f

	addi t0, a1, 7
	andi t1, a3, ~0x7
	andi t0, t0, ~0x7
	/* a3: terminal address of source region
	 * t0: lowest doubleword-aligned address in source
	 * t1: highest doubleword-aligned address in source
	 */
	bgeu t0, t1, 2f
	bltu a1, t0, 3f
1:
	fixup ld, t2, (a1), 10f
	fixup sd, t2, (a0), 10f
	addi a1, a1, 8
	addi a0, a0, 8
	bltu a1, t1, 1b
2:
	bltu a1, a3, 4f
	li a0, 0
	ret
3: /* Edge case: unalignment */
	fixup lbu, t2, (a1), 10f
	fixup sb, t2, (a0), 10f
	addi a1, a1, 1
	addi a0, a0, 1
	bltu a1, t0, 3b
	jump 1b
4: /* Edge case: remainder */
	fixup lbu, t2, (a1), 10f
	fixup sb, t2, (a0), 10f
	addi a1, a1, 1
	addi a0, a0, 1
	bltu a1, a3, 4b
	li a0, 0
	ret
ENDPROC(__copy_user)


ENTRY(__clear_user)
	add a3, a0, a1
	addi t0, a0, 7
	andi t1, a3, ~0x7
	andi t0, t0, ~0x7
	/* a3: terminal address of target region
	 * t0: lowest doubleword-aligned address in target region
	 * t1: highest doubleword-aligned address in target region
	 */
	bgeu t0, t1, 2f
	bltu a0, t0, 3f
1:
	fixup sd, zero, (a0), 10f
	addi a0, a0, 8
	bltu a0, t1, 1b
2:
	bltu a0, a3, 4f
	li a0, 0
	ret
3: /* Edge case: unalignment */
	fixup sb, zero, (a0), 10f
	addi a0, a0, 1
	bltu a0, t0, 3b
	jump 1b
4: /* Edge case: remainder */
	fixup sb, zero, (a0), 10f
	bltu a0, a3, 4b
	li a0, 0
	ret
ENDPROC(__clear_user)

	.section .fixup,"ax"
	.balign 4
10:
	sub a0, a3, a0
	ret
	.previous
