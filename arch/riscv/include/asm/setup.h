#ifndef _ASM_RISCV_SETUP_H
#define _ASM_RISCV_SETUP_H

#ifdef __KERNEL__

#define MEMORY_SIZE ((unsigned long)(*((volatile u32 *)(0UL))) << 20)

extern void setup_early_printk(void);

#endif /* __KERNEL__ */

#include <asm-generic/setup.h>

#endif /* _ASM_RISCV_SETUP_H */
