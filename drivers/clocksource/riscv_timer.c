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

void timer_riscv_init(int cpu_id,
		      unsigned long riscv_timebase,
		      int (*next)(unsigned long, struct clock_event_device*))
{
	struct clock_event_device *ce = per_cpu_ptr(&riscv_clock_event, cpu_id);

	ce->cpumask = cpumask_of(cpu_id);
	clockevents_config_and_register(ce, riscv_timebase, MINDELTA, MAXDELTA);
}

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

static int timer_riscv_init_dt(struct device_node *n)
{
	int cpu_id = hart_of_timer(n);
	struct clock_event_device *ce = per_cpu_ptr(&riscv_clock_event, cpu_id);
	struct clocksource *cs = per_cpu_ptr(&riscv_clocksource, cpu_id);

	if (cpu_id == smp_processor_id()) {
		clocksource_register_hz(cs, riscv_timebase);

		ce->cpumask = cpumask_of(cpu_id);
		clockevents_config_and_register(ce, riscv_timebase, MINDELTA, MAXDELTA);
	}

	return 0;
}

TIMER_OF_DECLARE(riscv_timer, "riscv", timer_riscv_init_dt);
