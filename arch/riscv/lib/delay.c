/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <linux/delay.h>
#include <linux/param.h>
#include <linux/timex.h>
#include <linux/export.h>

void __delay(unsigned long cycles)
{
	u64 t0 = get_cycles();

	while ((unsigned long)(get_cycles() - t0) < cycles)
		cpu_relax();
}

void udelay(unsigned long usecs)
{
	u64 ucycles = (u64)usecs * timebase;
	do_div(ucycles, 1000000U);
	__delay((unsigned long)ucycles);
}
EXPORT_SYMBOL(udelay);

void ndelay(unsigned long nsecs)
{
	u64 ncycles = (u64)nsecs * timebase;
	do_div(ncycles, 1000000000U);
	__delay((unsigned long)ncycles);
}
EXPORT_SYMBOL(ndelay);
