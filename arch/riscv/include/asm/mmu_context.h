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

#ifndef _ASM_RISCV_MMU_CONTEXT_H
#define _ASM_RISCV_MMU_CONTEXT_H

#include <asm-generic/mm_hooks.h>

#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

static inline void enter_lazy_tlb(struct mm_struct *mm,
	struct task_struct *task)
{
}

/* Initialize context-related info for a new mm_struct */
static inline int init_new_context(struct task_struct *task,
	struct mm_struct *mm)
{
	return 0;
}

static inline void destroy_context(struct mm_struct *mm)
{
}

static inline pgd_t *current_pgdir(void)
{
	return pfn_to_virt(csr_read(sptbr) & SPTBR_PPN);
}

static inline void set_pgdir(pgd_t *pgd)
{
	csr_write(sptbr, virt_to_pfn(pgd) | SPTBR_MODE);
}

static inline void flush_icache_deferred(struct mm_struct *mm)
{
#ifdef CONFIG_SMP
	unsigned int cpu = smp_processor_id();
	cpumask_t *mask = &mm->context.icache_stale_mask;
	if (cpumask_test_cpu(cpu, mask)) {
		cpumask_clear_cpu(cpu, mask);
		smp_mb();
		local_flush_icache_all();
	}
#endif
}

static inline void switch_mm(struct mm_struct *prev,
	struct mm_struct *next, struct task_struct *task)
{
	if (likely(prev != next)) {
		unsigned int cpu = smp_processor_id();
		cpumask_clear_cpu(cpu, mm_cpumask(prev));
		cpumask_set_cpu(cpu, mm_cpumask(next));

		set_pgdir(next->pgd);
		local_flush_tlb_all();

		flush_icache_deferred(next);
	}
}

static inline void activate_mm(struct mm_struct *prev,
			       struct mm_struct *next)
{
	switch_mm(prev, next, NULL);
}

static inline void deactivate_mm(struct task_struct *task,
	struct mm_struct *mm)
{
}

#endif /* _ASM_RISCV_MMU_CONTEXT_H */
