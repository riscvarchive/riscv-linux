#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#include <asm/processor.h>
#include <asm/csr.h>

/* read interrupt enabled status */
static inline unsigned long arch_local_save_flags(void)
{
	return csr_read(status);
}

/* unconditionally enable interrupts */
static inline void arch_local_irq_enable(void)
{
	csr_set(status, SR_EI);
}

/* unconditionally disable interrupts */
static inline void arch_local_irq_disable(void)
{
	csr_clear(status, SR_EI);
}

/* get status and disable interrupts */
static inline unsigned long arch_local_irq_save(void)
{
	return csr_read_clear(status, SR_EI);
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
static inline void arch_local_irq_restore(unsigned long flags)
{
	csr_set(status, flags & SR_EI);
}

#endif /* _ASM_RISCV_IRQFLAGS_H */
