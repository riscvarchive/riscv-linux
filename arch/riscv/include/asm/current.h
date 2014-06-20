#ifndef _ASM_RISCV_CURRENT_H
#define _ASM_RISCV_CURRENT_H

#include <asm/csr.h>

struct task_struct;

static inline struct task_struct *get_current(void)
{
	return (struct task_struct *)(csr_read(sup0));
}

#define current (get_current())

#endif /* _ASM_RISCV_CURRENT_H */
