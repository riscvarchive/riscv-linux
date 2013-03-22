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

#define SR_IM_SHIFT     16
#define SR_IM_MASK(n)   ((_AC(1,UL)) << ((n) + SR_IM_SHIFT))

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_SYSCALL             6
#define EXC_LOAD_MISALIGNED     8
#define EXC_STORE_MISALIGNED    9
#define EXC_LOAD_ACCESS         10
#define EXC_STORE_ACCESS        11

#ifdef __ASSEMBLY__

#define PCR_STATUS 	cr0
#define PCR_EPC		cr1
#define PCR_BADVADDR	cr2
#define PCR_EVEC	cr3
#define PCR_COUNT	cr4
#define PCR_COMPARE	cr5
#define PCR_CAUSE	cr6
#define PCR_PTBR	cr7
#define PCR_CLR_IPI	cr9
#define PCR_K0		cr12
#define PCR_K1		cr13
#define PCR_TOSIM	cr27
#define PCR_FROMSIM	cr28
#define PCR_TOHOST	cr30
#define PCR_FROMHOST	cr31

#else /* __ASSEMBLY__ */

#define PCR_STATUS 	0
#define PCR_EPC		1
#define PCR_BADVADDR	2
#define PCR_EVEC	3
#define PCR_COUNT	4
#define PCR_COMPARE	5
#define PCR_CAUSE	6
#define PCR_PTBR	7
#define PCR_CLR_IPI	9
#define PCR_K0		12
#define PCR_K1		13
#define PCR_TOSIM	27
#define PCR_FROMSIM	28
#define PCR_TOHOST	30
#define PCR_FROMHOST	31

#define mtpcr(pcr,val)				\
({						\
	register unsigned long __tmp;		\
	__asm__ __volatile__ (			\
		"mtpcr %0, %1, cr%2"		\
		: "=r" (__tmp)			\
		: "r" (val), "i" (pcr));	\
	__tmp;					\
})

#define mfpcr(pcr)				\
({						\
	register unsigned long __val;		\
	__asm__ __volatile__ (			\
		"mfpcr %0, cr%1"		\
		: "=r" (__val)			\
		:  "i" (pcr));			\
	__val;					\
})

#define setpcr(pcr,val)				\
({						\
	register unsigned long __tmp;		\
	__asm__ __volatile__ (			\
		"setpcr %0, cr%2, %1"		\
		: "=r" (__tmp)			\
		: "i" (val), "i" (pcr));	\
	__tmp;					\
})

#define clearpcr(pcr,val)			\
({						\
	register unsigned long __tmp;		\
	__asm__ __volatile__ (			\
		"clearpcr %0, cr%2, %1"		\
		: "=r" (__tmp)			\
		: "i" (val), "i" (pcr));	\
	__tmp;					\
})

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PCR_H */
