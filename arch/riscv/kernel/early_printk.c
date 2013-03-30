#include <linux/console.h>
#include <linux/init.h>

#include <asm/setup.h>
#include <asm/page.h>
#include <asm/pcr.h>

static inline void __init early_htif_put_char(unsigned char ch)
{
	unsigned long packet;
	packet = (HTIF_DEVICE_CONSOLE | HTIF_COMMAND_WRITE | ch);
	while (mtpcr(PCR_TOHOST, packet));
}

static void __init early_console_write(struct console *con,
		const char *s, unsigned int n)
{
	for (; n > 0; n--) {
		unsigned char ch;
		ch = *s++;
		if (ch == '\n')
			early_htif_put_char('\r');
		early_htif_put_char(ch);
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
