#ifndef _ASM_RISCV_BUG_H
#define _ASM_RISCV_BUG_H

#ifndef __ASSEMBLY__

struct pt_regs;
struct task_struct;

extern void die(const char *str, struct pt_regs *regs, long err);
extern void do_trap(struct pt_regs *regs, int signo, int code,
	unsigned long addr, struct task_struct *tsk);

#endif /* !__ASSEMBLY__ */

#include <asm-generic/bug.h>

#endif /* _ASM_RISCV_BUG_H */
