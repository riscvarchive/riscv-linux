#include <linux/init.h>
#include <asm/setup.h>

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_EARLY_PRINTK
	setup_early_printk();
#endif /* CONFIG_EARLY_PRINTK */
}
