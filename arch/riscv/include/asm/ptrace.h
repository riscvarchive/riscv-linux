#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

#include <asm/pcr.h>

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
	unsigned long epc;
} pt_regs;

#define user_mode(regs) (((regs)->status & SR_S) == 0)

#ifndef __ASSEMBLY__

/* Helpers for working with the instruction pointer */
#ifndef GET_IP
#define GET_IP(regs) ((regs)->pc)
#endif
#ifndef SET_IP
#define SET_IP(regs, val) (GET_IP(regs) = (val))
#endif

static inline unsigned long instruction_pointer(struct pt_regs *regs)
{
	return GET_IP(regs);
}
static inline void instruction_pointer_set(struct pt_regs *regs,
                                           unsigned long val)
{
	SET_IP(regs, val);
}

#ifndef profile_pc
#define profile_pc(regs) instruction_pointer(regs)
#endif

/* Helpers for working with the user stack pointer */
#ifndef GET_USP
#define GET_USP(regs) ((regs)->usp)
#endif
#ifndef SET_USP
#define SET_USP(regs, val) (GET_USP(regs) = (val))
#endif

static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return GET_USP(regs);
}
static inline void user_stack_pointer_set(struct pt_regs *regs,
                                          unsigned long val)
{
	SET_USP(regs, val);
}

/* Helpers for working with the frame pointer */
#ifndef GET_FP
#define GET_FP(regs) ((regs)->fp)
#endif
#ifndef SET_FP
#define SET_FP(regs, val) (GET_FP(regs) = (val))
#endif

static inline unsigned long frame_pointer(struct pt_regs *regs)
{
	return GET_FP(regs);
}
static inline void frame_pointer_set(struct pt_regs *regs,
                                     unsigned long val)
{
	SET_FP(regs, val);
}

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PTRACE_H */
