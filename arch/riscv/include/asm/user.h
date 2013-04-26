#ifndef _ASM_RISCV_USER_H
#define _ASM_RISCV_USER_H

/* Mirrors ptrace.h's pt_regs */

typedef struct user_regs_struct {
	unsigned long zero;
	unsigned long ra;
	unsigned long s[12];
	unsigned long sp;
	unsigned long tp;
	unsigned long v[2];
	unsigned long a[14];
	unsigned long usp;
	unsigned long status;
	unsigned long pc; 
	unsigned long epc;
} user_regs_struct;

#endif /* _ASM_RISCV_USER_H */
