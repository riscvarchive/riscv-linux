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

#define MINDELTA 100
#define MAXDELTA 0x7fffffff

/*
 * See <linux/timer_riscv.h> for the rationale behind pre-allocating per-cpu
 * timers on RISC-V systems.
 */
DEFINE_PER_CPU(struct clock_event_device, riscv_clock_event) = {
	.name           = "riscv_timer_clockevent",
	.features       = CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 300,
	.set_state_oneshot  = NULL,
	.set_state_shutdown = NULL,
};

static struct clocksource cs = {
	.name = "riscv_clocksource",
	.rating = 300,
	.mask = CLOCKSOURCE_MASK(BITS_PER_LONG),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

void clocksource_riscv_init(unsigned long long (*rdtime)(struct clocksource *))
{
	cs.read = rdtime;
	clocksource_register_hz(&cs, riscv_timebase);
}

void timer_riscv_init(int cpu_id,
		      unsigned long riscv_timebase,
		      int (*next)(unsigned long, struct clock_event_device*))
{
	struct clock_event_device *ce = per_cpu_ptr(&riscv_clock_event, cpu_id);

	ce->cpumask = cpumask_of(cpu_id);
	ce->set_next_event = next;

	clockevents_config_and_register(ce, riscv_timebase, MINDELTA, MAXDELTA);
}
