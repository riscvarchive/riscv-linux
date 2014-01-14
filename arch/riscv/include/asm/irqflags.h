#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#include <asm/processor.h>
#include <asm/pcr.h>

/* read interrupt enabled status */
static unsigned long arch_local_save_flags(void)
{
	return read_csr(status);
}

/* unconditionally enable interrupts */
static inline void arch_local_irq_enable(void)
{
	set_csr(status, SR_EI);
}

/* unconditionally disable interrupts */
static inline void arch_local_irq_disable(void)
{
	clear_csr(status, SR_EI);
}

/* get status and disable interrupts */
static inline unsigned long arch_local_irq_save(void)
{
	return clear_csr(status, SR_EI);
}

/* test flags */
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & SR_EI);
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
		   that SR_EI has already been cleared through
		   arch_irqs_disabled_flags() */
		arch_local_irq_disable();
	}
}

#endif /* _ASM_RISCV_IRQFLAGS_H */
