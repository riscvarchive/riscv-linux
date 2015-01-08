#include <linux/delay.h>
#include <linux/param.h>
#include <linux/timex.h>

inline void __const_udelay(unsigned long xloops)
{
	u64 loops;
	loops = (u64)xloops * loops_per_jiffy * HZ;
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
