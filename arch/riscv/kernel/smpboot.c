#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/sbi.h>

void *__cpu_up_stack_pointer[NR_CPUS];

void __init smp_prepare_boot_cpu(void)
{
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
}

void __init setup_smp(void)
{
#warning FIXME CONFIG STRING: number of boot CPUs hard-wired to 1
	int i, num_cpus = 1;

	for (i = 0; i < min(num_cpus, NR_CPUS); i++) {
		set_cpu_possible(i, true);
		set_cpu_present(i, true);
	}
}

int __cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	/* Signal cpu to start */
	mb();
	__cpu_up_stack_pointer[cpu] = task_stack_page(tidle) + THREAD_SIZE;

	while (!cpu_online(cpu))
		cpu_relax();
	
	return 0;
}

void __init smp_cpus_done(unsigned int max_cpus)
{
}

/*
 * C entry point for a secondary processor.
 */
asmlinkage void __init smp_callin(void)
{
	struct mm_struct *mm = &init_mm;

	/* All kernel threads share the same mm context.  */
	atomic_inc(&mm->mm_count);
	current->active_mm = mm;

	trap_init();
	init_clockevent();
	notify_cpu_starting(smp_processor_id());
	set_cpu_online(smp_processor_id(), 1);
	local_flush_tlb_all();
	local_irq_enable();
	preempt_disable();
	cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);
}
