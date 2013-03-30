#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/setup.h>
#include <asm/page.h>
#include <asm/pcr.h>
#include <asm/irq.h>

static struct tty_driver *htif_tty_driver;
static struct tty_struct *tty_zero;

static irqreturn_t htif_input_isr(int irq, void *dev_id)
{
	unsigned long packet;
	unsigned char c;

	/* mtpcr will return its old value and clear the interrupt */
	packet = mtpcr(PCR_FROMHOST, 0);
	if (!packet) {
		return IRQ_NONE;
	}

	c = packet & 0xFF;
	tty_insert_flip_char(tty_zero, c, TTY_NORMAL);
	tty_flip_buffer_push(tty_zero);

	/* Send next request for character */
	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_READ);
	while (mtpcr(PCR_TOHOST, packet));

	return IRQ_HANDLED;
}

static void htif_put_char(struct tty_struct *tty, unsigned char ch)
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
		if (ch == '\n')
			htif_put_char(tty_zero, '\r');
		htif_put_char(tty_zero, ch);
	}
}


static int htif_tty_open(struct tty_struct *tty, struct file *filp)
{
	printk(KERN_DEBUG "HTIF: opening tty %s\n", tty->name);
	tty_zero = tty;
	return 0;
}

static void htif_tty_close(struct tty_struct *tty, struct file *filp)
{
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

void __init input_irq_init(void)
{
	int ret;
	unsigned long packet;

	ret = request_irq(IRQ_HOST, htif_input_isr, 0, "htif_input", NULL);
	/* Send next request for character */
	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_READ);
	while (mtpcr(PCR_TOHOST, packet));
	if (ret)
		printk(KERN_ERR "HTIF: failed to request htif_input irq\n");
}

static int __init htif_init(void)
{
	int ret;

	htif_tty_driver = alloc_tty_driver(1);
	if (htif_tty_driver == NULL)
		return -ENOMEM;

	htif_tty_driver->driver_name = "htif";
	htif_tty_driver->name = "htif";
	htif_tty_driver->type = TTY_DRIVER_TYPE_SYSTEM;
	htif_tty_driver->subtype = SYSTEM_TYPE_TTY;
	htif_tty_driver->major = 42; /* cannot use dynamic */
	htif_tty_driver->minor_start = 0;
	htif_tty_driver->init_termios = tty_std_termios;
	htif_tty_driver->flags = TTY_DRIVER_REAL_RAW;

	tty_set_operations(htif_tty_driver, &htif_tty_ops);
	ret = tty_register_driver(htif_tty_driver);
	if (ret != 0) {
		printk(KERN_ERR "HTIF: failed to register tty driver: %d\n", ret);
		put_tty_driver(htif_tty_driver);
		return ret;
	}
	tty_register_device(htif_tty_driver, 0, NULL);
	register_console(&htif_console);
	return 0;
}

module_init(htif_init);
