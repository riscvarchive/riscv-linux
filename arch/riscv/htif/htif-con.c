#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <asm/htif.h>

#define DRIVER_NAME "htifcon"

#define NR_PORTS CONFIG_HTIF_CONSOLE_PORTS

struct htifcon_port {
	struct device *dev;
	sbi_device_message output_buf;
	sbi_device_message input_buf;
	struct tty_port port;
	spinlock_t lock;
};

static struct htifcon_port htifcon_ports[NR_PORTS];
static DEFINE_SPINLOCK(htifcon_ports_lock);

static void htifcon_port_destruct(struct tty_port *port)
{
	struct htifcon_port *p;
	unsigned long flags;

	p = container_of(port, struct htifcon_port, port);
	spin_lock(&htifcon_ports_lock);
	spin_lock_irqsave(&p->lock, flags);
	p->dev = NULL;
	spin_unlock_irqrestore(&p->lock, flags);
	spin_unlock(&htifcon_ports_lock);
}

static const struct tty_port_operations htifcon_port_ops = {
	.destruct = htifcon_port_destruct,
};


static inline struct htif_device *htifcon_port_htifdev(
	const struct htifcon_port *port)
{
	return to_htif_dev(port->dev->parent);
}

static struct htifcon_port *htifcon_port_get(unsigned int minor)
{
	struct htifcon_port *port;

	if (unlikely(WARN_ON(minor >= NR_PORTS)))
		return NULL;

	port = NULL;
	spin_lock(&htifcon_ports_lock);
	if (htifcon_ports[minor].dev != NULL) {
		port = &htifcon_ports[minor];
		tty_port_get(&port->port);
	}
	spin_unlock(&htifcon_ports_lock);
	return port;
}

static void htifcon_port_put(struct htifcon_port *port)
{
	tty_port_put(&port->port);
}

static int htif_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct htifcon_port *port;
	int ret;

	port = htifcon_port_get(tty->index);
	if (port == NULL)
		return -ENODEV;

	ret = tty_standard_install(driver, tty);
	if (likely(!ret)) {
		tty->driver_data = port;
	} else {
		htifcon_port_put(port);
	}
	return ret;
}

static int htif_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct htifcon_port *port = tty->driver_data;

	return tty_port_open(&port->port, tty, filp);
}

static void htif_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct htifcon_port *port = tty->driver_data;

	tty_port_close(&port->port, tty, filp);
}

static void htif_tty_cleanup(struct tty_struct *tty)
{
	struct htifcon_port *port = tty->driver_data;

	tty_port_put(&port->port);
	tty->driver_data = NULL;
}

static inline void htifcon_put_char(struct htifcon_port *port,
	unsigned int id, unsigned char ch)
{
	while (port->output_buf.data != 0)
		cpu_relax();
	port->output_buf.dev = id;
	port->output_buf.cmd = HTIF_CMD_WRITE;
	port->output_buf.data = ch;
	sbi_send_device_request(__pa(&port->output_buf));
}

static int htif_tty_write(struct tty_struct *tty,
	const unsigned char *buf, int count)
{
	struct htifcon_port *port = tty->driver_data;
	const unsigned char *end;
	unsigned int id;

	id = htifcon_port_htifdev(port)->index;
	for (end = buf + count; buf < end; buf++) {
		htifcon_put_char(port, id, *buf);
	}
	return count;
}

static int htif_tty_write_room(struct tty_struct *tty)
{
	return 1024; /* arbitrary */
}

static void htif_tty_hangup(struct tty_struct *tty)
{
	struct htifcon_port *port = tty->driver_data;

	tty_port_hangup(&port->port);
}

static const struct tty_operations htif_tty_ops = {
	.install	= htif_tty_install,
	.open		= htif_tty_open,
	.close		= htif_tty_close,
	.cleanup	= htif_tty_cleanup,
	.write		= htif_tty_write,
	.write_room	= htif_tty_write_room,
	.hangup		= htif_tty_hangup,
};

static struct tty_driver *htif_tty_driver;

static irqreturn_t htifcon_isr(struct htif_device *dev, sbi_device_message *msg)
{
	struct htifcon_port *port = dev_get_drvdata(&dev->dev);

	if (msg->cmd == HTIF_CMD_READ) {
		spin_lock(&port->lock);
		tty_insert_flip_char(&port->port, msg->data, TTY_NORMAL);
		tty_flip_buffer_push(&port->port);
		spin_unlock(&port->lock);

		/* Request next character */
		sbi_send_device_request(__pa(msg));
	} else if (msg->cmd == HTIF_CMD_WRITE) {
		msg->data = 0;
	} else {
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int htifcon_port_init(struct htif_device *dev)
{
	struct htifcon_port *port;
	unsigned int minor;
	int ret;

	ret = -EBUSY;
	port = htifcon_ports;
	spin_lock(&htifcon_ports_lock);
	for (minor = 0; minor < NR_PORTS; minor++, port++) {
		struct device *tty_dev;

		if (port->dev != NULL)
			continue;

		tty_port_init(&port->port);
		port->port.ops = &htifcon_port_ops;
		spin_lock_init(&port->lock);

		tty_dev = tty_port_register_device(&port->port,
			htif_tty_driver, minor, &dev->dev);
		if (unlikely(IS_ERR(tty_dev))) {
			ret = PTR_ERR(tty_dev);
		} else {
			port->dev = tty_dev;
			dev_set_drvdata(&dev->dev, port);
			ret = 0;
		}
	}
	spin_unlock(&htifcon_ports_lock);
	return ret;
}

static void htif_console_init_late(void);

static int htifcon_probe(struct device *dev)
{
	struct htif_device *htif_dev;
	struct htifcon_port *port;
	int ret;

	dev_info(dev, "detected console\n");
	htif_dev = to_htif_dev(dev);

	ret = htifcon_port_init(htif_dev);
	if (unlikely(ret))
		return ret;

	ret = htif_request_irq(htif_dev, htifcon_isr);
	if (unlikely(ret))
		return ret;

	htif_console_init_late();

	/* Request next character */
	port = dev_get_drvdata(&htif_dev->dev);
	port->input_buf.dev = htif_dev->index;
	port->input_buf.cmd = HTIF_CMD_READ;
	sbi_send_device_request(__pa(&port->input_buf));
	return 0;
}

static struct htif_driver htifcon_driver = {
	.type = "bcd",
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.probe = htifcon_probe,
	},
};

static int __init htifcon_init(void)
{
	int ret;

	htif_tty_driver = tty_alloc_driver(NR_PORTS,
		TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (unlikely(IS_ERR(htif_tty_driver)))
		return PTR_ERR(htif_tty_driver);

	htif_tty_driver->driver_name = "htif_tty";
	htif_tty_driver->name = "ttyHTIF";
	htif_tty_driver->major = TTY_MAJOR;
	htif_tty_driver->minor_start = 0;
	htif_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	htif_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	htif_tty_driver->init_termios = tty_std_termios;
	tty_set_operations(htif_tty_driver, &htif_tty_ops);

	ret = tty_register_driver(htif_tty_driver);
	if (unlikely(ret))
		goto out_tty_put;

	ret = htif_register_driver(&htifcon_driver);
	if (unlikely(ret))
		goto out_tty_unregister;

	return 0;

out_tty_unregister:
	tty_unregister_driver(htif_tty_driver);
out_tty_put:
	put_tty_driver(htif_tty_driver);
	return ret;
}

static void __exit htifcon_exit(void)
{
	htif_unregister_driver(&htifcon_driver);
	tty_unregister_driver(htif_tty_driver);
	put_tty_driver(htif_tty_driver);
}

module_init(htifcon_init);
module_exit(htifcon_exit);

MODULE_DESCRIPTION("HTIF console driver");
MODULE_LICENSE("GPL");


static void htif_console_write(struct console *co,
		const char *buf, unsigned int count)
{
	struct htifcon_port *port = htifcon_ports + co->index;
	//unsigned int id = htifcon_port_htifdev(port)->index;
	const char *end;

	for (end = buf + count; buf < end; buf++) {
		unsigned char ch = *buf;

		if (ch == '\n')
			sbi_console_putchar('\r');
		sbi_console_putchar(ch);
	}
}

static struct tty_driver *htif_console_device(struct console *co, int *index)
{
	*index = co->index;
	return htif_tty_driver;
}

static int __init htif_console_setup(struct console *co, char *options)
{
	return (co->index < 0 || co->index >= NR_PORTS
		|| htifcon_ports[co->index].dev == NULL) ?
		-ENODEV : 0;
}

static struct console htif_console = {
	.name	= "htifcon",
	.write	= htif_console_write,
	.device = htif_console_device,
	.setup	= htif_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= -1
};

/*
 * Early console initialization will fail in the typical case when setup
 * precedes driver initialization.  Note that the driver will later
 * re-attempt to register the console when probing the device.
 */
static int __init htif_console_init(void)
{
	register_console(&htif_console);
	return 0;
}
console_initcall(htif_console_init);

static inline void htif_console_init_late(void)
{
	if (!(htif_console.flags & CON_ENABLED)) {
		register_console(&htif_console);
	}
}
