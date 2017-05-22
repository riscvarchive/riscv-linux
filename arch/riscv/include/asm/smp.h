/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

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
irqreturn_t handle_ipi(void);

#endif /* CONFIG_SMP */

#endif /* _ASM_RISCV_SMP_H */
