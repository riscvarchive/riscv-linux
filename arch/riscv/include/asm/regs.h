#ifndef _ASM_RISCV_REGS_H
#define _ASM_RISCV_REGS_H

#ifndef __ASSEMBLY__

#define mtpcr(val,pcr)			\
do {                                    \
	__asm__ __volatile__ (          \
		"mtpcr %0, " pcr "\n"   \
		:                       \
		: "r" (val));           \
} while (0)

#define mfpcr(pcr)                      \
({                                      \
	register unsigned long __val;   \
	__asm__ __volatile__ (          \
		"mfpcr %0, " pcr "\n"   \
		: "=r" (__val));        \
	__val;                          \
})

#endif /* __ASSEMBLY__ */

#endif /* _AS_RISCV_REGS_H */
