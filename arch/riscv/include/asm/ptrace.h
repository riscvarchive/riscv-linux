#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

#include <asm/processor.h>

typedef struct pt_regs {
	unsigned long zero;
	unsigned long ra;
	unsigned long v[2];
	unsigned long a[8];
	unsigned long t[8];
	unsigned long s[8];
	unsigned long gp; /* aka s8 */
	unsigned long fp; /* aka s9 */
	unsigned long sp;
	unsigned long tp;
	unsigned long usp;
	unsigned long status;
	unsigned long pc; 
} pt_regs;

#define user_mode(regs) (((regs)->status & SR_S) == 0)

#include <asm-generic/ptrace.h>

#endif /* _ASM_RISCV_PTRACE_H */
