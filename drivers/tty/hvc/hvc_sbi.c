/*
 * RISC-V SBI interface to hvc_console.c
 *  based on drivers-tty/hvc/hvc_udbg.c
 *
 * Copyright (C) 2008 David Gibson, IBM Corporation
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

#include <linux/console.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/irq.h>

#include <asm/sbi.h>

#include "hvc_console.h"

static int hvc_sbi_tty_put(uint32_t vtermno, const char *buf, int count)
{
	int i;

	for (i = 0; i < count; i++)
		sbi_console_putchar(buf[i]);

	return i;
}

static int hvc_sbi_tty_get(uint32_t vtermno, char *buf, int count)
{
	int i, c;

	for (i = 0; i < count; i++) {
		if ((c = sbi_console_getchar()) < 0)
			break;
		buf[i] = c;
	}

	return i;
}

static const struct hv_ops hvc_sbi_ops = {
	.get_chars = hvc_sbi_tty_get,
	.put_chars = hvc_sbi_tty_put,
};

static int __init hvc_sbi_init(void)
{
	return PTR_ERR_OR_ZERO(hvc_alloc(0, 0, &hvc_sbi_ops, 16));
}
device_initcall(hvc_sbi_init);

static int __init hvc_sbi_console_init(void)
{
	hvc_instantiate(0, 0, &hvc_sbi_ops);
	add_preferred_console("hvc", 0, NULL);

	return 0;
}
console_initcall(hvc_sbi_console_init);

#ifdef CONFIG_EARLY_PRINTK
static void sbi_console_write(struct console *co, const char *buf,
			      unsigned int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		if (buf[i] == '\n')
			sbi_console_putchar('\r');
		sbi_console_putchar(buf[i]);
	}
}

static struct console early_console_dev __initdata = {
	.name	= "early",
	.write	= sbi_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT,
	.index	= -1
};

static int __init setup_early_printk(char *str)
{
	if (early_console == NULL) {
		early_console = &early_console_dev;
		register_console(early_console);
	}
	return 0;
}
early_param("earlyprintk", setup_early_printk);
#endif
