#include <linux/delay.h>

#define LOOPS_PER_JIFFY 1337

inline void __const_udelay(unsigned long xloops)
{
	unsigned long loops;
	loops = xloops * LOOPS_PER_JIFFY * CONFIG_HZ;
	__delay(loops >> 32);
}

inline void __delay(unsigned long loops)
{
	unsigned long start, now;

	__asm__ __volatile__ (
		"rdcycle %0" : "=r" (start));
	do {
		__asm__ __volatile__ (
			"rdcycle %0" : "=r" (now));
	} while ((now - start) < loops);
}
