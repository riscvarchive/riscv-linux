#include <linux/delay.h>

#include <asm/timex.h>

#define LOOPS_PER_JIFFY 1337

inline void __const_udelay(unsigned long xloops)
{
	unsigned long loops;
	loops = xloops * LOOPS_PER_JIFFY * CONFIG_HZ;
	__delay(loops >> 32);
}

inline void __delay(unsigned long loops)
{
	cycles_t start, now;
	start = get_cycles();
	do {
		now = get_cycles();
	} while ((now - start) < loops);
}
