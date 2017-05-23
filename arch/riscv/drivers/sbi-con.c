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

#include <linux/init.h>
#include <linux/console.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include <asm/sbi.h>

#define SBI_POLL_PERIOD 1
#define SBI_MAX_GETCHARS 10

static struct tty_driver *sbi_tty_driver;

struct sbi_console_private {
	struct tty_port port;
	struct timer_list timer;

} sbi_console_singleton;

static void sbi_console_getchars(uintptr_t data)
{
	struct sbi_console_private *priv = (struct sbi_console_private *)data;
	struct tty_port *port = &priv->port;
	unsigned long flags;
	int ch;

	spin_lock_irqsave(&port->lock, flags);

	if ((ch = sbi_console_getchar()) >= 0) {
		tty_insert_flip_char(port, ch, TTY_NORMAL);
		tty_flip_buffer_push(port);
	}

	mod_timer(&priv->timer, jiffies + SBI_POLL_PERIOD);

	spin_unlock_irqrestore(&port->lock, flags);
}

static int sbi_tty_write(struct tty_struct *tty,
	const unsigned char *buf, int count)
{
	const unsigned char *end;

	for (end = buf + count; buf < end; buf++) {
		sbi_console_putchar(*buf);
	}
	return count;
}

static int sbi_tty_write_room(struct tty_struct *tty)
{
	return 1024; /* arbitrary */
}

static int sbi_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct sbi_console_private *priv = &sbi_console_singleton;
	struct tty_port *port = &priv->port;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	if (!port->tty) {
		tty->driver_data = priv;
		tty->port = port;
		port->tty = tty;
		mod_timer(&priv->timer, jiffies);
	}

	spin_unlock_irqrestore(&port->lock, flags);

	return 0;
}

static void sbi_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct sbi_console_private *priv = tty->driver_data;
	struct tty_port *port = &priv->port;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	if (tty->count == 1) {
		port->tty = NULL;
		del_timer(&priv->timer);
	}

	spin_unlock_irqrestore(&port->lock, flags);
}

static const struct tty_operations sbi_tty_ops = {
	.open		= sbi_tty_open,
	.close		= sbi_tty_close,
	.write		= sbi_tty_write,
	.write_room	= sbi_tty_write_room,
};


static void sbi_console_write(struct console *co, const char *buf, unsigned n)
{
	for ( ; n > 0; n--, buf++) {
		if (*buf == '\n')
			sbi_console_putchar('\r');
		sbi_console_putchar(*buf);
	}
}

static struct tty_driver *sbi_console_device(struct console *co, int *index)
{
	*index = co->index;
	return sbi_tty_driver;
}

static int sbi_console_setup(struct console *co, char *options)
{
	return co->index != 0 ? -ENODEV : 0;
}

static struct console sbi_console = {
	.name	= "sbi_console",
	.write	= sbi_console_write,
	.device	= sbi_console_device,
	.setup	= sbi_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= -1
};

static int __init sbi_console_init(void)
{
	int ret;
	static struct tty_driver *drv;

	setup_timer(&sbi_console_singleton.timer, sbi_console_getchars,
		    (uintptr_t)&sbi_console_singleton);
	register_console(&sbi_console);

	drv = tty_alloc_driver(1, TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (unlikely(IS_ERR(drv)))
		return PTR_ERR(drv);

	drv->driver_name = "sbi";
	drv->name = "ttySBI";
	drv->major = TTY_MAJOR;
	drv->minor_start = 0;
	drv->type = TTY_DRIVER_TYPE_SERIAL;
	drv->subtype = SERIAL_TYPE_NORMAL;
	drv->init_termios = tty_std_termios;
	tty_set_operations(drv, &sbi_tty_ops);

	tty_port_init(&sbi_console_singleton.port);
	tty_port_link_device(&sbi_console_singleton.port, drv, 0);

	ret = tty_register_driver(drv);
	if (unlikely(ret))
		goto out_tty_put;

	sbi_tty_driver = drv;
	return 0;

out_tty_put:
	put_tty_driver(drv);
	return ret;
}

static void __exit sbi_console_exit(void)
{
	tty_unregister_driver(sbi_tty_driver);
	put_tty_driver(sbi_tty_driver);
}

module_init(sbi_console_init);
module_exit(sbi_console_exit);

MODULE_DESCRIPTION("RISC-V SBI console driver");
MODULE_LICENSE("GPL");

#ifdef CONFIG_EARLY_PRINTK

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
