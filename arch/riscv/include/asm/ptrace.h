#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

typedef struct pt_regs {
	unsigned long regs[32];
	unsigned long pc; 
	unsigned long usp;
	unsigned long fp; 
	unsigned long sp; 
	unsigned long status;
} pt_regs;

#define user_mode(regs) (((regs)->status & 0x04) == 0)

#include <asm-generic/ptrace.h>

#endif /* _ASM_RISCV_PTRACE_H */
