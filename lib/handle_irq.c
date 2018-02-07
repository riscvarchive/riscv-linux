/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1992 Linus Torvalds
 * Modifications for ARM processor Copyright (C) 1995-2000 Russell King.
 * Support for Dynamic Tick Timer Copyright (C) 2004-2005 Nokia Corporation.
 * Dynamic Tick Timer written by Tony Lindgren <tony@atomide.com> and
 * Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>.
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2017 SiFive, Inc
 */

#include <asm-generic/handle_irq.h>

void (*handle_arch_irq)(struct pt_regs *) = NULL;

int __init set_handle_irq(void (*handle_irq)(struct pt_regs *))
{
	if (!handle_arch_irq)
		handle_arch_irq = handle_irq;

	if (handle_arch_irq != handle_irq)
		return -EBUSY;

	return 0;
}

