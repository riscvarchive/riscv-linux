#ifndef _ASM_RISCV_REGS_H
#define _ASM_RISCV_REGS_H

#ifdef __ASSEMBLY__

#define PCR_STATUS 	cr0
#define PCR_EPC		cr1
#define PCR_BADVADDR	cr2
#define PCR_EVEC	cr3
#define PCR_COUNT	cr4
#define PCR_COMPARE	cr5
#define PCR_CAUSE	cr6
#define PCR_PTBR	cr7

#else /* __ASSEMBLY__ */

#define PCR_STATUS 	"cr0"
#define PCR_EPC		"cr1"
#define PCR_BADVADDR	"cr2"
#define PCR_EVEC	"cr3"
#define PCR_COUNT	"cr4"
#define PCR_COMPARE	"cr5"
#define PCR_CAUSE	"cr6"
#define PCR_PTBR	"cr7"

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
