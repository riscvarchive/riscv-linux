#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#include <asm/processor.h>
#include <asm/pcr.h>

#define ARCH_IRQ_DISABLED       0
#define ARCH_IRQ_ENABLED        (SR_ET)

static unsigned long arch_local_save_flags(void)
{
	return (mfpcr(PCR_STATUS) & SR_ET);
}

static void arch_local_irq_restore(unsigned long flags)
{
	unsigned long status;
	status = mfpcr(PCR_STATUS);
	status = (status & ~(SR_ET)) | flags;
	mtpcr(PCR_STATUS, status);
}

#include <asm-generic/irqflags.h>

#endif /* _ASM_RISCV_IRQFLAGS_H */
