#include <linux/console.h>
#include <linux/init.h>

#include <asm/setup.h>
#include <asm/page.h>
#include <asm/htif.h>

#define HTIF_DEV_CONSOLE	(1U)

static inline void __init early_htif_putc(unsigned char ch)
{
	/* FIXME: Device ID should not be hard-coded. */
	htif_tohost(HTIF_DEV_CONSOLE, HTIF_CMD_WRITE, ch);
}

static void __init early_console_write(struct console *con,
		const char *s, unsigned int n)
{
	for (; n > 0; n--) {
		unsigned char ch;
		ch = *s++;
		if (ch == '\n')
			early_htif_putc('\r');
		early_htif_putc(ch);
	}
}

static struct console early_console_dev __initdata = {
	.name	= "early",
	.write	= early_console_write,
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
