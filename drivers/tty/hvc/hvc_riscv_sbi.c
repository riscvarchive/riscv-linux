/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/console.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/types.h>

#include <asm/sbi.h>
#include <asm/hvc_riscv_sbi.h>

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
		c = sbi_console_getchar();
		if (c < 0)
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

struct console riscv_sbi_early_console_dev __initdata = {
	.name	= "early",
	.write	= sbi_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT | CON_ANYTIME,
	.index	= -1
};
