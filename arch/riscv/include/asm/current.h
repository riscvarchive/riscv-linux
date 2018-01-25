#ifndef _ASM_RISCV_CURRENT_H
#define _ASM_RISCV_CURRENT_H

#include <asm/csr.h>

struct task_struct;

static inline struct task_struct *get_current(void)
{
	register struct task_struct * tp asm("tp");
	return tp;
}

#define current (get_current())

#endif /* _ASM_RISCV_CURRENT_H */
