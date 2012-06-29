#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#ifndef __ASSEMBLY__

static unsigned long arch_local_save_flags(void)
{
	return 0;
}

static void arch_local_irq_restore(unsigned long flags)
{
}

#endif /* __ASSEMBLY__ */

#include <asm-generic/irqflags.h>

#endif /* _ASM_RISCV_IRQFLAGS_H */
