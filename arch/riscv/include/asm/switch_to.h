#ifndef _ASM_RISCV_SWITCH_TO_H
#define _ASM_RISCV_SWITCH_TO_H

#include <linux/linkage.h>

extern asmlinkage void switch_to(struct task_struct *prev,
    struct task_struct *next, struct task_struct *last);

#endif /* _ASM_RISCV_SWITCH_TO */
