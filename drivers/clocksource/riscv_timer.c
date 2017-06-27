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

/*
 * See <linux/timer_riscv.h> for the rationale behind pre-allocating per-cpu
 * timers on RISC-V systems.
 */
static DEFINE_PER_CPU(struct clocksource, clock_source);
static DEFINE_PER_CPU(struct clock_event_device, clock_event);

struct clock_event_device *timer_riscv_device(int cpu)
{
	return &per_cpu(clock_event, cpu);
}

struct clocksource *timer_riscv_source(int cpu)
{
	return &per_cpu(clock_source, cpu);
}

void timer_riscv_init(int cpu_id,
		      unsigned long riscv_timebase,
		      unsigned long long (*rdtime)(struct clocksource *),
		      int (*next)(unsigned long, struct clock_event_device*))
{
	struct clocksource *cs = &per_cpu(clock_source, cpu_id);
	struct clock_event_device *ce = &per_cpu(clock_event, cpu_id);

	*cs = (struct clocksource) {
		.name = "riscv_clocksource",
		.rating = 300,
		.read = rdtime,
		.mask = CLOCKSOURCE_MASK(BITS_PER_LONG),
		.flags = CLOCK_SOURCE_IS_CONTINUOUS,
	};
	clocksource_register_hz(cs, riscv_timebase);

	*ce = (struct clock_event_device){
		.name = "riscv_timer_clockevent",
		.features = CLOCK_EVT_FEAT_ONESHOT,
		.rating = 300,
		.cpumask = cpumask_of(cpu_id),
		.set_next_event = next,
		.set_state_oneshot  = NULL,
		.set_state_shutdown = NULL,
	};
	clockevents_config_and_register(ce, riscv_timebase, 100, 0x7fffffff);
}
