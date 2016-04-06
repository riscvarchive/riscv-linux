#ifndef _ASM_RISCV_SMP_H
#define _ASM_RISCV_SMP_H

#include <linux/cpumask.h>
#include <linux/irqreturn.h>

#ifdef CONFIG_SMP

/* SMP initialization hook for setup_arch */
void __init init_clockevent(void);

/* SMP initialization hook for setup_arch */
void __init setup_smp(void);

/* Hook for the generic smp_call_function_many() routine. */
void arch_send_call_function_ipi_mask(struct cpumask *mask);

/* Hook for the generic smp_call_function_single() routine. */
void arch_send_call_function_single_ipi(int cpu);

#define raw_smp_processor_id() (current_thread_info()->cpu)

/* Interprocessor interrupt handler */
irqreturn_t ipi_isr(int irq, void *dev_id);

#endif /* CONFIG_SMP */

#endif /* _ASM_RISCV_SMP_H */
