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

unsigned long riscv_timebase;

static int next_event(unsigned long delta, struct clock_event_device *ce)
{
	/*
	 * time_init() allocates a timer for each CPU.  Since we're writing the
	 * timer comparison register here we can't allow the timers to cross
	 * harts.
	 */
	BUG_ON(ce != timer_riscv_device(smp_processor_id()));
	sbi_set_timer(get_cycles64() + delta);
	return 0;
}

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

void riscv_timer_interrupt(void)
{
	int cpu = smp_processor_id();
	struct clock_event_device *evdev = timer_riscv_device(cpu);

	evdev->event_handler(evdev);
}

void __init init_clockevent(void)
{
	int cpu_id = smp_processor_id();

	timer_riscv_init(cpu_id, riscv_timebase, &rdtime, &next_event);
	csr_set(sie, SIE_STIE);
}

void __init time_init(void)
{
	struct device_node *cpu;
	u32 prop;

	cpu = of_find_node_by_path("/cpus");
	if (!cpu || of_property_read_u32(cpu, "timebase-frequency", &prop))
		panic(KERN_WARNING "RISC-V system with no 'timebase-frequency' in DTS\n");
	riscv_timebase = prop;

	lpj_fine = riscv_timebase / HZ;

	init_clockevent();
}
