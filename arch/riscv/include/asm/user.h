#ifndef _ASM_RISCV_USER_H
#define _ASM_RISCV_USER_H

typedef struct user_regs_struct {
	unsigned long lols[32];
	unsigned long pc; 
	unsigned long usp;
	unsigned long fp; 
} user_regs_struct;

#endif /* _ASM_RISCV_USER_H */
