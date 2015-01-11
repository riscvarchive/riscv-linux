#include <linux/delay.h>

#include <asm/timex.h>

inline void __const_udelay(unsigned long xloops)
{
	unsigned long loops;
	loops = xloops * loops_per_jiffy * CONFIG_HZ;
	__delay(loops >> 32);
}

inline void __delay(unsigned long loops)
{
	cycles_t start, now;
	start = get_cycles();
	do {
		cpu_relax();
		now = get_cycles();
	} while ((now - start) < loops);
}
