/*
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <linux/linkage.h>
#include <asm/unistd.h>

	.text
ENTRY(__vdso_cmpxchg32)
	.cfi_startproc
#ifdef CONFIG_ISA_A
/*
 * a0: ptr
 * a1: compare value
 * a2: exchange value
 */
0:
	lr.w.aqrl t0, 0(a0)
	bne       t0, a1, 1f
	sc.w.aqrl t1, a2, 0(a0)
	bnez      t1, 0b
1:
	mv        a0, t0
	ret
#else
	li a7, __NR_sysriscv_cmpxchg32
	scall
	ret
#endif
	.cfi_endproc
ENDPROC(__vdso_cmpxchg32)
