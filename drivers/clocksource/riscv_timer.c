/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
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

#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/timer_riscv.h>
#include <linux/sched_clock.h>
#include <linux/cpu.h>
#include <asm/sbi.h>

#define MINDELTA 100
#define MAXDELTA 0x7fffffff

/*
 * See <linux/timer_riscv.h> for the rationale behind pre-allocating per-cpu
 * timers on RISC-V systems.
 */
DECLARE_PER_CPU(struct clock_event_device, riscv_clock_event);
DECLARE_PER_CPU(struct clocksource, riscv_clocksource);

static int next_event(unsigned long delta, struct clock_event_device *ce)
{
	/*
	 * time_init() allocates a timer for each CPU.  Since we're writing the
	 * timer comparison register here we can't allow the timers to cross
	 * harts.
	 */
	BUG_ON(ce != this_cpu_ptr(&riscv_clock_event));
	csr_set(sie, SIE_STIE);
	sbi_set_timer(get_cycles64() + delta);
	return 0;
}

DEFINE_PER_CPU(struct clock_event_device, riscv_clock_event) = {
	.name           = "riscv_timer_clockevent",
	.features       = CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 100,
	.set_state_oneshot  = NULL,
	.set_state_shutdown = NULL,
	.set_next_event = next_event,
};

DEFINE_PER_CPU(bool, riscv_clock_event_enabled) = false;

static unsigned long long rdtime(struct clocksource *cs)
{
	/*
	 * It's guarnteed that all the timers across all the harts are
	 * synchronized within one tick of each other, so while this could
	 * technically go backwards when hopping between CPUs, practically it
	 * won't happen.
	 */
	return get_cycles64();
}

DEFINE_PER_CPU(struct clocksource, riscv_clocksource) = {
	.name = "riscv_clocksource",
	.rating = 300,
	.mask = CLOCKSOURCE_MASK(BITS_PER_LONG),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
	.read = rdtime,
};

static int hart_of_timer(struct device_node *dev)
{
	u32 hart;

	if (!dev)
		return -1;
	if (!of_device_is_compatible(dev, "riscv"))
		return -1;
	if (of_property_read_u32(dev, "reg", &hart))
		return -1;

	return hart;
}

static u64 notrace timer_riscv_sched_read(void)
{
	return get_cycles64();
}

static int timer_riscv_starting_cpu(unsigned int cpu)
{
	struct clock_event_device *ce = per_cpu_ptr(&riscv_clock_event, cpu);

	ce->cpumask = cpumask_of(cpu);
	clockevents_config_and_register(ce, riscv_timebase, MINDELTA, MAXDELTA);
	/* Enable timer interrupt for this cpu */
	csr_set(sie, SIE_STIE);

	return 0;
}

static int timer_riscv_dying_cpu(unsigned int cpu)
{
	/* Disable timer interrupt for this cpu */
	csr_clear(sie, SIE_STIE);

	return 0;
}

static int __init timer_riscv_init_dt(struct device_node *n)
{
	int err = 0;
	int cpu_id = hart_of_timer(n);
	struct clocksource *cs = per_cpu_ptr(&riscv_clocksource, cpu_id);

	if (cpu_id == smp_processor_id()) {
		clocksource_register_hz(cs, riscv_timebase);
		sched_clock_register(timer_riscv_sched_read, 64, riscv_timebase);

		err = cpuhp_setup_state(CPUHP_AP_RISCV_TIMER_STARTING,
			 "clockevents/riscv/timer:starting",
			 timer_riscv_starting_cpu, timer_riscv_dying_cpu);
		if (err)
			pr_err("RISCV timer register failed [%d] for cpu = [%d]\n",
			       err, cpu_id);
	}
	return err;
}

TIMER_OF_DECLARE(riscv_timer, "riscv", timer_riscv_init_dt);
