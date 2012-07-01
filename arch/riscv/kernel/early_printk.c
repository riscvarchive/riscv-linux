/*
 * Borrowing extensively from MIPS.
 */
#include <linux/console.h>
#include <linux/init.h>

#include <asm/setup.h>
#include <asm/htif.h>

#define FESVR_SYSCALL_WRITE 4
#define FESVR_STDOUT 1

static void __init
early_console_write(struct console *con, const char *s, unsigned n)
{
	static unsigned long packet[4]; 

	packet[0] = FESVR_SYSCALL_WRITE;
	packet[1] = FESVR_STDOUT;
	packet[2] = (unsigned long)s;
	packet[3] = n;

	write_tohost((unsigned long)packet);
	while (read_fromhost() == 0);

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
