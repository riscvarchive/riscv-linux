#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#include <asm/processor.h>
#include <asm/pcr.h>

/* read interrupt enabled status */
static unsigned long arch_local_save_flags(void)
{
	return mfpcr(PCR_STATUS);
}

/* unconditionally enable interrupts */
static inline void arch_local_irq_enable(void)
{
	setpcr(PCR_STATUS, SR_ET);
}

/* unconditionally disable interrupts */
static inline void arch_local_irq_disable(void)
{
	clearpcr(PCR_STATUS, SR_ET);
}

/* get status and disable interrupts */
static inline unsigned long arch_local_irq_save(void)
{
	return clearpcr(PCR_STATUS, SR_ET);
}

/* test flags */
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & SR_ET);
}

/* test hardware interrupt enable bit */
static inline int arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

/* set interrupt enabled status */
static void arch_local_irq_restore(unsigned long flags)
{
	if (!arch_irqs_disabled_flags(flags)) {
		arch_local_irq_enable();
	} else {
		/* Not strictly necessary if it can be guaranteed
		   that SR_ET has already been cleared through
		   arch_irqs_disabled_flags() */
		arch_local_irq_disable();
	}
}

#endif /* _ASM_RISCV_IRQFLAGS_H */
