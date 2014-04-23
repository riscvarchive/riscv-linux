#ifndef _ASM_RISCV_HWACHA_H
#define _ASM_RISCV_HWACHA_H

#define hwacha_vxcptcause() \
({ \
	long __cause; \
	__asm__ __volatile__ ( \
		"vxcptcause %0" \
		: "=r" (__cause)); \
	__cause; \
})

#define hwacha_vxcptaux() \
({ \
	long __aux; \
	__asm__ __volatile__ ( \
		"vxcptaux %0" \
		: "=r" (__aux)); \
	__aux; \
})

#define HWACHA_CAUSE_ILLEGAL_CFG 0 // AUX: 0=illegal nxpr, 1=illegal nfpr
#define HWACHA_CAUSE_ILLEGAL_INSTRUCTION 1 // AUX: instruction
#define HWACHA_CAUSE_PRIVILEGED_INSTRUCTION 2 // AUX: instruction
#define HWACHA_CAUSE_TVEC_ILLEGAL_REGID 3 // AUX: instruction
#define HWACHA_CAUSE_VF_MISALIGNED_FETCH 4 // AUX: pc
#define HWACHA_CAUSE_VF_FAULT_FETCH 5 // AUX: pc
#define HWACHA_CAUSE_VF_ILLEGAL_INSTRUCTION 6 // AUX: pc
#define HWACHA_CAUSE_VF_ILLEGAL_REGID 7 // AUX: pc
#define HWACHA_CAUSE_MISALIGNED_LOAD 8 // AUX: badvaddr
#define HWACHA_CAUSE_MISALIGNED_STORE 9 // AUX: badvaddr
#define HWACHA_CAUSE_FAULT_LOAD 10 // AUX: badvaddr
#define HWACHA_CAUSE_FAULT_STORE 11 // AUX: badvaddr

extern void hwacha_init(void);

#endif /* _ASM_RISCV_HWACHA_H */
