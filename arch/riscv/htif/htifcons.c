#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <asm/htif.h>

#define DRIVER_NAME "htifcons"

#define HTIF_NR_TTY		(1)
#define HTIF_DEV_CONSOLE	(1U)

struct htifcons_port {
	struct tty_port port;
	struct htif_dev *dev;
	unsigned int index;
};

static struct htifcons_port *htifcons_table[HTIF_NR_TTY];
static DEFINE_SPINLOCK(htifcons_table_lock);

static int htifcons_port_add(struct htifcons_port *port)
{
	unsigned int index;
	int ret;

	ret = -EBUSY;
	spin_lock(&htifcons_table_lock);
	for (index = 0; index < HTIF_NR_TTY; index++) {
		if (htifcons_table[index] == NULL) {
			htifcons_table[index] = port;
			port->index = index;
			ret = 0;
			break;
		}
	}
	spin_unlock(&htifcons_table_lock);
	return ret;
}

static void htifcons_port_remove(struct htifcons_port *port)
{
//	BUG_ON(htifcons_port[port->index] != port);
	spin_lock(&htifcons_table_lock);
	htifcons_table[port->index] = NULL;
	spin_unlock(&htifcons_table_lock);
}

static struct htifcons_port *htifcons_port_get(unsigned int index)
{
	struct htifcons_port *port;

	if (index >= HTIF_NR_TTY)
		return NULL;
	spin_lock(&htifcons_table_lock);
	port = htifcons_table[index];
	if (port)
		tty_port_get(&port->port);
	spin_unlock(&htifcons_table_lock);

	return port;
}

static void htifcons_port_put(struct htifcons_port *port)
{
	tty_port_put(&port->port);
}


static void htifcons_write(unsigned int id, const char *buf,
	unsigned int count)
{
	for (; count > 0; count--) {
		unsigned char ch;
		ch = *buf++;
		if (unlikely(ch == '\n'))
			htif_tohost(id, HTIF_CMD_WRITE, '\r');
		htif_tohost(id, HTIF_CMD_WRITE, ch);
	}
}


static int htif_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct htifcons_port *port;
	port = tty->driver_data;
	if (unlikely(port == NULL))
		return -ENODEV;
	return tty_port_open(&port->port, tty, filp);
}

static void htif_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct htifcons_port *port;
	port = tty->driver_data;
	tty_port_close(&port->port, tty, filp);
}

static int htif_tty_write(struct tty_struct *tty,
	const unsigned char *buf, int count)
{
	struct htifcons_port *port;
	port = tty->driver_data;
	htifcons_write(port->dev->minor, buf, count);
	return count;
}

static int htif_tty_write_room(struct tty_struct *tty)
{
	return 64;
}

/**
 * htif_tty_install - install method
 * @driver: the driver in use (htif_tty_driver)
 * @tty: the tty being bound
 *
 * Look up and bind the tty and the driver together.
 * Initialize any needed private data (i.e., the termios).
 */
static int htif_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct htifcons_port *port;
	int ret;

	port = htifcons_port_get(tty->index);
	ret = tty_standard_install(driver, tty);
	if (unlikely(ret)) {
		htifcons_port_put(port);
	} else {
		tty->driver_data = port;
	}
	return ret;
}

static void htifcons_port_destruct(struct tty_port *port)
{
	kfree(container_of(port, struct htifcons_port, port));
}

static const struct tty_port_operations htifcons_port_ops = {
	.destruct = htifcons_port_destruct,
};

static const struct tty_operations htif_tty_ops = {
	.open = htif_tty_open,
	.close = htif_tty_close,
	.write = htif_tty_write,
	.write_room = htif_tty_write_room,
	.install = htif_tty_install,
};

static struct tty_driver *htif_tty_driver;


static void htif_console_write(struct console *con,
		const char *buf, unsigned int count)
{
	/* FIXME: Device ID should not be hard-coded. */
	htifcons_write(HTIF_DEV_CONSOLE, buf, count);
}

static struct tty_driver *htif_console_device(struct console *con,
	int *index)
{
	*index = con->index;
	return htif_tty_driver;
}

static struct console htif_console = {
	.name	= "htifcons",
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


static irqreturn_t htif_input_isr(int irq, void *dev_id)
{
	unsigned long data;
	unsigned int index, id;

	data = htif_fromhost();
	if (unlikely(!data))
		return IRQ_NONE;
	id = (data >> HTIF_DEV_SHIFT);

	for (index = 0; index < HTIF_NR_TTY; index++) {
		struct htifcons_port *p;
		p = htifcons_table[index];
		if (p != NULL && p->dev->minor == id) {
			struct tty_port *port;
			port = &(p->port);

			tty_insert_flip_char(port, data, TTY_NORMAL);
			tty_flip_buffer_push(port);

			/* Send next request for character */
			htif_tohost(id, HTIF_CMD_READ, 0);
			return IRQ_HANDLED;
		}
	}
	return IRQ_NONE;
}


static int htifcons_probe(struct device *dev)
{
	static int irq_requested = 0;

	struct htif_dev *htif_dev;
	struct htifcons_port *port;
	int ret;

	htif_dev = to_htif_dev(dev);
	pr_info(DRIVER_NAME ": detected console with ID %u\n", htif_dev->minor);

	port = kzalloc(sizeof(struct htifcons_port), GFP_KERNEL);
	if (port == NULL)
		return -ENOMEM;
	tty_port_init(&port->port);
	port->port.ops = &htifcons_port_ops;
	port->dev = htif_dev;

	ret = htifcons_port_add(port);
	if (unlikely(ret))
		goto err_port_add;

	dev = tty_port_register_device(&port->port, htif_tty_driver, port->index, dev);
	if (unlikely(IS_ERR(dev))) {
		ret = PTR_ERR(dev);
		goto err_port_reg;
	}

	if (!irq_requested) {
		/* Send next request for character */
		htif_tohost(htif_dev->minor, HTIF_CMD_READ, 0);
		ret = request_irq(IRQ_HOST, htif_input_isr, 0, "htif_input", NULL);
		if (ret) {
			pr_err(DRIVER_NAME ": failed to request irq: %d\n", ret);
			goto err_isr_req;
		}
		irq_requested = 1;
	}
	return 0;

err_isr_req:
	tty_unregister_device(htif_tty_driver, port->index);
err_port_reg:
	htifcons_port_remove(port);
err_port_add:
	kfree(port);
	return ret;
}

static struct htif_driver htifcons_driver = {
	.type = "bcd",
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.probe = htifcons_probe,
	},
};


static int __init htifcons_init(void)
{
	int ret;

	htif_tty_driver = alloc_tty_driver(HTIF_NR_TTY);
	if (unlikely(htif_tty_driver == NULL))
		return -ENOMEM;

	htif_tty_driver->driver_name = "htif_tty";
	htif_tty_driver->name = "ttyHTIF";
	htif_tty_driver->type = TTY_DRIVER_TYPE_SYSTEM;
	htif_tty_driver->subtype = SYSTEM_TYPE_TTY;
	htif_tty_driver->major = 0; /* dynamically allocated */
	htif_tty_driver->minor_start = 0;
	htif_tty_driver->init_termios = tty_std_termios;
	htif_tty_driver->flags = TTY_DRIVER_REAL_RAW;
	tty_set_operations(htif_tty_driver, &htif_tty_ops);

	ret = tty_register_driver(htif_tty_driver);
	if (unlikely(ret)) {
		pr_err(DRIVER_NAME ": registering tty driver failed: %d\n", ret);
		goto err_tty_reg;
	}

	ret = htif_register_driver(&htifcons_driver);
	if (unlikely(ret)) {
		pr_err(DRIVER_NAME ": registering htif driver failed: %d\n", ret);
		goto err_htif_reg;
	}
	return 0;

err_htif_reg:
	tty_unregister_driver(htif_tty_driver);
err_tty_reg:
	put_tty_driver(htif_tty_driver);
	return ret;
}

static void __exit htifcons_exit(void)
{
	htif_unregister_driver(&htifcons_driver);
	tty_unregister_driver(htif_tty_driver);
	put_tty_driver(htif_tty_driver);
}

postcore_initcall(htifcons_init);
module_exit(htifcons_exit);
