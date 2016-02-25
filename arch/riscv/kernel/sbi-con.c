#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include <asm/sbi.h>

static DEFINE_SPINLOCK(sbi_tty_port_lock);
static struct tty_port sbi_tty_port;
static struct tty_driver *sbi_tty_driver;

static irqreturn_t sbi_console_isr(int irq, void *dev_id)
{
	int ch = sbi_console_getchar();
	if (ch < 0)
		return IRQ_NONE;

	spin_lock(&sbi_tty_port_lock);
	tty_insert_flip_char(&sbi_tty_port, ch, TTY_NORMAL);
	tty_flip_buffer_push(&sbi_tty_port);
	spin_unlock(&sbi_tty_port_lock);

	return IRQ_HANDLED;
}

static int sbi_tty_open(struct tty_struct *tty, struct file *filp)
{
	return 0;
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

static const struct tty_operations sbi_tty_ops = {
	.open		= sbi_tty_open,
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

	register_console(&sbi_console);

	sbi_tty_driver = tty_alloc_driver(1,
		TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (unlikely(IS_ERR(sbi_tty_driver)))
		return PTR_ERR(sbi_tty_driver);

	sbi_tty_driver->driver_name = "sbi";
	sbi_tty_driver->name = "ttySBI";
	sbi_tty_driver->major = TTY_MAJOR;
	sbi_tty_driver->minor_start = 0;
	sbi_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	sbi_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	sbi_tty_driver->init_termios = tty_std_termios;
	tty_set_operations(sbi_tty_driver, &sbi_tty_ops);

	tty_port_init(&sbi_tty_port);
	tty_port_link_device(&sbi_tty_port, sbi_tty_driver, 0);

	ret = tty_register_driver(sbi_tty_driver);
	if (unlikely(ret))
		goto out_tty_put;

	ret = request_irq(IRQ_SOFTWARE, sbi_console_isr, IRQF_SHARED,
	                  sbi_tty_driver->driver_name, sbi_console_isr);
	if (unlikely(ret))
		goto out_tty_put;

	return ret;

out_tty_put:
	put_tty_driver(sbi_tty_driver);
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
