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

static struct console early_console __initdata = {
	.name	= "early",
	.write	= early_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT,
	.index	= -1
};

static int early_console_initialized __initdata;

void __init setup_early_printk(void)
{
	if (early_console_initialized)
		return;
	early_console_initialized = 1;

	register_console(&early_console);
}
