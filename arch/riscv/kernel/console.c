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

#define SIM_CALL_WRITE 4
#define SIM_CALL_READ 3

#define BUF_SIZE 64 /* Must corroborate with riscv-isa-sim */

static struct tty_driver *htif_tty_driver;
static struct tty_struct *tty_zero;

static irqreturn_t htif_input_isr(int irq, void *dev_id)
{
	volatile char buf[64];
	volatile unsigned long packet[4];
	unsigned long pkt_pa, count;

	packet[0] = SIM_CALL_READ;
	packet[1] = 0;
	packet[2] = (unsigned long)(__pa(buf));
	packet[3] = 0;  // will be clobbered with bytes received

	pkt_pa = (unsigned long)(__pa(packet));
	mtpcr(PCR_TOSIM, pkt_pa);

	count = packet[3];

	tty_insert_flip_string(tty_zero, (char *)buf, count);
	tty_flip_buffer_push(tty_zero);

	return IRQ_HANDLED;
}

static void htif_write(const char *buf, unsigned int count)
{
	volatile unsigned long packet[4]; 
	unsigned long pkt_pa;

	packet[0] = SIM_CALL_WRITE;
	packet[1] = 1;
	packet[2] = (unsigned long)(__pa(buf));
	packet[3] = count;


#ifdef CONFIG_USE_HTIF_CONSOLE
	pkt_pa = (unsigned long)(__pa(packet));
	mtpcr(PCR_TOHOST, pkt_pa);
	while (mfpcr(PCR_FROMHOST) == 0);
	mtpcr(PCR_FROMHOST, 0);
#else
	pkt_pa = (unsigned long)(__pa(packet));
	mtpcr(PCR_TOSIM, pkt_pa);
#endif
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

	ret = request_irq(IRQ_KB, htif_input_isr, 0, "htif_input", NULL);
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
