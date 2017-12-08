/*
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

#ifndef _LINUX_TIMER_RISCV_H
#define _LINUX_TIMER_RISCV_H

/*
 * All RISC-V systems have a timer attached to every hart.  These timers can be
 * read by the 'rdcycle' pseudo instruction, and can use the SBI to setup
 * events.  In order to abstract the architecture-specific timer reading and
 * setting functions away from the clock event insertion code, we provide
 * function pointers to the clockevent subsystem that perform two basic operations:
 * rdtime() reads the timer on the current CPU, and next_event(delta) sets the
 * next timer event to 'delta' cycles in the future.  As the timers are
 * inherently a per-cpu resource, these callbacks perform operations on the
 * current hart.  There is guaranteed to be exactly one timer per hart on all
 * RISC-V systems.
 */
void timer_riscv_init(int cpu_id,
		      unsigned long riscv_timebase,
		      int (*next_event)(unsigned long, struct clock_event_device *));

void clocksource_riscv_init(unsigned long long (*rdtime)(struct clocksource *));
#endif
