#ifndef _ASM_RISCV_SWITCH_TO_H
#define _ASM_RISCV_SWITCH_TO_H

#define switch_to(prev, next, last) \
	do { 							\
	__asm__ volatile ("move a0, %0;"			\
	                  "move a1, %1;"			\
	                  "sd sp, 728(a0);" 			\
	                  "la ra, 1f;"				\
			  "sd ra, 744(a0);"			\
	                  "ld sp, 728(a1);" 			\
			  "ld ra, 744(a1);"			\
			  "j __switch_to;"			\
			  "1: move %2, v0"			\
			  : 					\
			  : "r" (prev), "r" (next), "r" (last)	\
			  : "memory");				\
	} while (0)
/*
 * a0 = prev->thread
 * a1 = next->thread
 * prev->thread.sp = sp
 * ra = addr at 1
 * prev->thread.pc = ra
 * sp = next->thread.sp
 * ra = next->thread.pc
 * jump to C __switch_to (just returns prev)
 * 1:
 * last = v0 (prev)
 */

#endif /* _ASM_RISCV_SWITCH_TO */
