/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#ifndef _ASM_RISCV_SMP_H
#define _ASM_RISCV_SMP_H

#include <linux/cpumask.h>
#include <linux/irqreturn.h>

#ifdef CONFIG_SMP

/* SMP initialization hook for setup_arch */
void __init setup_smp(void);

/* Hook for the generic smp_call_function_many() routine. */
void arch_send_call_function_ipi_mask(struct cpumask *mask);

/* Hook for the generic smp_call_function_single() routine. */
void arch_send_call_function_single_ipi(int cpu);

/* Obtains the hart ID of the currently executing task.  This relies on
 * THREAD_INFO_IN_TASK, but we define that unconditionally. */
#ifdef CONFIG_THREAD_INFO_IN_TASK
#define get_ti()                ((struct thread_info *)get_current())
#define raw_smp_processor_id()  (get_ti()->cpu)
#endif

/* Interprocessor interrupt handler */
irqreturn_t handle_ipi(void);

#ifdef CONFIG_HOTPLUG_CPU
extern int __cpu_disable(void);
extern void __cpu_die(unsigned int cpu);
extern void cpu_play_dead(void);
extern void boot_sec_cpu(void);
#endif
#endif /* CONFIG_SMP */

#endif /* _ASM_RISCV_SMP_H */
