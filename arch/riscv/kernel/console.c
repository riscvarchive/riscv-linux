#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/pcr.h>

static struct tty_driver *htif_tty_driver;

static irqreturn_t htif_input_isr(int irq, void *dev_id)
{
	static struct tty_struct *tty;
	unsigned long packet;

	/* mtpcr will return its old value and clear the interrupt */
	packet = mtpcr(PCR_FROMHOST, 0);
	if (unlikely(!packet)) {
		return IRQ_NONE;
	}

	tty = dev_id;
	tty_insert_flip_char(tty, packet, TTY_NORMAL);
	tty_flip_buffer_push(tty);

	/* Send next request for character */
	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_READ);
	while (mtpcr(PCR_TOHOST, packet));

	return IRQ_HANDLED;
}

static inline void htif_put_char(unsigned char ch)
{
	unsigned long packet;

	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_WRITE | ch);
	while (mtpcr(PCR_TOHOST, packet));
}

static void htif_write(const char *buf, unsigned int count)
{
	for (; count > 0; count--) {
		unsigned char ch;
		ch = *buf++;
		if (unlikely(ch == '\n'))
			htif_put_char('\r');
		htif_put_char(ch);
	}
}

static unsigned int htif_tty_count = 0;

static int htif_tty_open(struct tty_struct *tty, struct file *filp)
{
	static struct tty_struct *tty_current = NULL;
	unsigned long packet;
	int ret;

	pr_debug("htif: opening tty %s\n", tty->name);
	if (htif_tty_count > 0) {
		if (unlikely(tty != tty_current)) {
			return -EBUSY;
		} else {
			htif_tty_count++;
			return 0;
		}
	}
	htif_tty_count++;
	tty_current = tty;

	/* Send next request for character */
	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_READ);
	while (mtpcr(PCR_TOHOST, packet));

	ret = request_irq(IRQ_HOST, htif_input_isr, 0, "htif_input", tty);
	if (ret) {
		pr_err("htif: failed to request irq: %d\n", ret);
	}
	return 0;
}

static void htif_tty_close(struct tty_struct *tty, struct file *filp)
{
	if ((htif_tty_count > 0) && (--htif_tty_count == 0)) {
		free_irq(IRQ_HOST, tty);
	}
}

static int htif_tty_write(struct tty_struct *tty,
	const unsigned char *buf, int count)
{
	htif_write(buf, count);
	return count;
}

static int htif_tty_write_room(struct tty_struct *tty)
{
	return 64;
}

static const struct tty_operations htif_tty_ops = {
	.open = htif_tty_open,
	.close = htif_tty_close,
	.write = htif_tty_write,
	.write_room = htif_tty_write_room,
};


static void htif_console_write(struct console *con,
		const char *buf, unsigned int count)
{
	htif_write(buf, count);
}

static struct tty_driver *htif_console_device(struct console *con,
	int *index)
{
	*index = con->index;
	return htif_tty_driver;
}

static struct console htif_console = {
	.name	= "htif",
	.write	= htif_console_write,
	.device = htif_console_device,
	.flags	= CON_PRINTBUFFER,
	.index	= -1
};

static int __init htif_console_init(void)
{
	register_console(&htif_console);
	return 0;
}
console_initcall(htif_console_init);


static int __init htif_tty_init(void)
{
	int ret;

	htif_tty_driver = alloc_tty_driver(1);
	if (htif_tty_driver == NULL) {
		pr_err("htif: tty allocation failed\n");
		return -ENOMEM;
	}

	htif_tty_driver->driver_name = "htif";
	htif_tty_driver->name = "htif";
	htif_tty_driver->type = TTY_DRIVER_TYPE_SYSTEM;
	htif_tty_driver->subtype = SYSTEM_TYPE_TTY;
	htif_tty_driver->major = 0;
	htif_tty_driver->minor_start = 0;
	htif_tty_driver->init_termios = tty_std_termios;
	htif_tty_driver->flags = TTY_DRIVER_REAL_RAW;

	tty_set_operations(htif_tty_driver, &htif_tty_ops);
	ret = tty_register_driver(htif_tty_driver);
	if (ret != 0) {
		pr_err("htif: failed to register tty driver: %d\n", ret);
		put_tty_driver(htif_tty_driver);
		return ret;
	}
	return 0;
}

module_init(htif_tty_init);

