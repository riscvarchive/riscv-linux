/*
 * SMP initialisation and IPI support
 * Based on arch/arm64/kernel/smp.c
 *
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2015 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/kernel_stat.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/sched/task_stack.h>
#include <linux/sched/hotplug.h>
#include <asm/irq.h>
#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/sbi.h>

void *__cpu_up_stack_pointer[NR_CPUS];
void *__cpu_up_task_pointer[NR_CPUS];

void __init smp_prepare_boot_cpu(void)
{
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
}

void __init setup_smp(void)
{
	struct device_node *dn = NULL;
	int hart, found_boot_cpu = 0;

	while ((dn = of_find_node_by_type(dn, "cpu"))) {
		hart = riscv_of_processor_hartid(dn);
		if (hart >= 0) {
			set_cpu_possible(hart, true);
			set_cpu_present(hart, true);
			if (hart == smp_processor_id()) {
				BUG_ON(found_boot_cpu);
				found_boot_cpu = 1;
			}
		}
	}

	BUG_ON(!found_boot_cpu);
}

int __cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	tidle->thread_info.cpu = cpu;

	/*
	 * On RISC-V systems, all harts boot on their own accord.  Our _start
	 * selects the first hart to boot the kernel and causes the remainder
	 * of the harts to spin in a loop waiting for their stack pointer to be
	 * setup by that main hart.  Writing __cpu_up_stack_pointer signals to
	 * the spinning harts that they can continue the boot process.
	 */
	smp_mb();
	WRITE_ONCE(__cpu_up_stack_pointer[cpu], task_stack_page(tidle) + THREAD_SIZE);
	WRITE_ONCE(__cpu_up_task_pointer[cpu], tidle);

	arch_send_call_function_single_ipi(cpu);
	while (!cpu_online(cpu))
		cpu_relax();

	pr_notice("CPU%u: online\n", cpu);
	return 0;
}

void __init smp_cpus_done(unsigned int max_cpus)
{
}

#ifdef CONFIG_HOTPLUG_CPU
/*
 * __cpu_disable runs on the processor to be shutdown.
 */
int __cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();
	int ret;

	set_cpu_online(cpu, false);
	irq_migrate_all_off_this_cpu();

	return 0;
}
/*
 * called on the thread which is asking for a CPU to be shutdown -
 * waits until shutdown has completed, or it is timed out.
 */
void __cpu_die(unsigned int cpu)
{
	int err = 0;

	if (!cpu_wait_death(cpu, 5)) {
		pr_err("CPU %u: didn't die\n", cpu);
		return;
	}
	pr_notice("CPU%u: shutdown\n", cpu);
}
/*
 * Called from the idle thread for the CPU which has been shutdown.
 */
void cpu_play_dead(void)
{
	int sipval, sieval, scauseval;
	int cpu = smp_processor_id();

	idle_task_exit();

	(void)cpu_report_death();

	/* Do not disable software interrupt to restart cpu after WFI */
	csr_clear(sie, SIE_STIE | SIE_SEIE);

	/* clear all pending flags */
	csr_write(sip, 0);
	/* clear any previous scause data */
	csr_write(scause, 0);

	do {
		wait_for_interrupt();
		sipval = csr_read(sip);
		sieval = csr_read(sie);
		scauseval = csr_read(scause);
	/* only break if wfi returns for an enabled interrupt */
	} while ((sipval & sieval) == 0 &&
		 scauseval != INTERRUPT_CAUSE_SOFTWARE);

	boot_sec_cpu();
}


#endif
/*
 * C entry point for a secondary processor.
 */
asmlinkage void smp_callin(void)
{
	struct mm_struct *mm = &init_mm;

	/* All kernel threads share the same mm context.  */
	mmgrab(mm);
	current->active_mm = mm;

	trap_init();
	notify_cpu_starting(smp_processor_id());
	/* Remote TLB flushes are ignored while the CPU is offline, so emit a local
	 * TLB flush right now just in case. */
	set_cpu_online(smp_processor_id(), true);
	local_flush_tlb_all();
	/* Disable preemption before enabling interrupts, so we don't try to
	 * schedule a CPU that hasn't actually started yet. */
	preempt_disable();
	local_irq_enable();
	cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);
}
