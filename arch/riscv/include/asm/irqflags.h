#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#ifndef __ASSEMBLY__

#include <asm/processor.h>
#include <asm/regs.h>

static unsigned long arch_local_save_flags(void)
{
	return mfpcr(PCR_STATUS) & SR_ET;
}

static void arch_local_irq_restore(unsigned long flags)
{
	mtpcr((mfpcr(PCR_STATUS) & ~SR_ET) | !!flags, PCR_STATUS);
}

#endif /* __ASSEMBLY__ */

#include <asm-generic/irqflags.h>

#endif /* _ASM_RISCV_IRQFLAGS_H */
