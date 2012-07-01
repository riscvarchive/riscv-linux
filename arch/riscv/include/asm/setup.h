#ifndef _ASM_RISCV_SETUP_H
#define _ASM_RISCV_SETUP_H

#ifdef __KERNEL__

#ifdef CONFIG_EARLY_PRINTK
extern void setup_early_printk(void);
#else
extern void setup_early_printk(void)
{
}
#endif /* CONFIG_EARLY_PRINTK */

#endif /* __KERNEL__ */

#include <asm-generic/setup.h>

#endif /* _ASM_RISCV_SETUP_H */
