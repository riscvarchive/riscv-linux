#ifndef _ASM_RISCV_PCR_H
#define _ASM_RISCV_PCR_H

#include <linux/const.h>

/* Status register flags */
#define SR_ET   _AC(0x00000001,UL) /* Enable exceptions */
#define SR_EF   _AC(0x00000002,UL) /* Enable floating-point */
#define SR_EV   _AC(0x00000004,UL) /* Enable vector unit */
#define SR_PS   _AC(0x00000010,UL) /* Previous supervisor */
#define SR_S    _AC(0x00000020,UL) /* Supervisor */
#define SR_U64  _AC(0x00000040,UL) /* RV64 user mode */
#define SR_S64  _AC(0x00000080,UL) /* RV64 supervisor mode */
#define SR_VM   _AC(0x00000100,UL) /* Enable virtual memory */
#define SR_IM   _AC(0x00FF0000,UL) /* Interrupt mask */

#ifdef __ASSEMBLY__

#define PCR_STATUS 	cr0
#define PCR_EPC		cr1
#define PCR_BADVADDR	cr2
#define PCR_EVEC	cr3
#define PCR_COUNT	cr4
#define PCR_COMPARE	cr5
#define PCR_CAUSE	cr6
#define PCR_PTBR	cr7
#define PCR_TOHOST	cr30
#define PCR_FROMHOST	cr31

#else /* __ASSEMBLY__ */

#define PCR_STATUS 	"cr0"
#define PCR_EPC		"cr1"
#define PCR_BADVADDR	"cr2"
#define PCR_EVEC	"cr3"
#define PCR_COUNT	"cr4"
#define PCR_COMPARE	"cr5"
#define PCR_CAUSE	"cr6"
#define PCR_PTBR	"cr7"
#define PCR_TOHOST	"cr30"
#define PCR_FROMHOST	"cr31"

#define mtpcr(pcr,val)			\
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

#endif /* _ASM_RISCV_PCR_H */
