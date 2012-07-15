#include <linux/delay.h>

#define LOOPS_PER_JIFFY 1337

inline void __const_udelay(unsigned long xloops)
{
	__delay((xloops >> 32) * LOOPS_PER_JIFFY * CONFIG_HZ);
}

inline void __delay(unsigned long loops)
{
	unsigned long start, now;

	__asm__ volatile ("rdcycle %0" : "=r" (start));
	do {
		__asm__ volatile ("rdcycle %0" : "=r" (now));
	} while ((now - start) < loops);
}
