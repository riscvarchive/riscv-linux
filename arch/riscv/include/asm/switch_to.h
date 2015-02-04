#ifndef _ASM_RISCV_SWITCH_TO_H
#define _ASM_RISCV_SWITCH_TO_H

#include <asm/processor.h>

static inline void __fpu_switch_to(struct task_struct *prev,
                                   struct task_struct *next)
{
#ifdef CONFIG_RISCV_FPU
	if (task_pt_regs(prev)->status & SR_EF)
		fpu_save(prev);
	if (task_pt_regs(next)->status & SR_EF)
		fpu_restore(next);
#endif /* CONFIG_RISCV_FPU */
}

extern struct task_struct *__switch_to(struct task_struct *,
                                       struct task_struct *);

#define switch_to(prev, next, last)			\
do {							\
	struct task_struct *__prev = (prev);		\
	struct task_struct *__next = (next);		\
	__fpu_switch_to(__prev, __next);		\
	((last) = __switch_to(__prev, __next));		\
} while (0)

#endif /* _ASM_RISCV_SWITCH_TO_H */
