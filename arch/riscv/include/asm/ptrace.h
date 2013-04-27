#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

#include <asm/pcr.h>

#ifndef __ASSEMBLY__

typedef struct pt_regs {
	unsigned long zero;
	unsigned long ra;
	unsigned long s[12];
	unsigned long sp;
	unsigned long tp;
	unsigned long v[2];
	unsigned long a[14];
	/* PCRs */
	unsigned long status;
	unsigned long epc;
	unsigned long badvaddr;
	unsigned long cause;
} pt_regs;

#define user_mode(regs) (((regs)->status & SR_PS) == 0)


/* Helpers for working with the instruction pointer */
#ifndef GET_IP
#define GET_IP(regs) ((regs)->epc)
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
#define GET_USP(regs) ((regs)->sp)
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
#define GET_FP(regs) ((regs)->s[0])
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
