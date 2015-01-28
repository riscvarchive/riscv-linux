#include <linux/delay.h>
#include <linux/param.h>
#include <linux/timex.h>
#include <linux/export.h>

inline void __delay(unsigned long loops)
{
	cycles_t start;

	start = get_cycles();
	do {
		cpu_relax();
	} while ((get_cycles() - start) < loops);
}
EXPORT_SYMBOL(__delay);

inline void __const_udelay(unsigned long xloops)
{
	u64 loops;

	loops = (u64)xloops * loops_per_jiffy * HZ;
	__delay(loops >> 32);
}
EXPORT_SYMBOL(__const_udelay);

void __udelay(unsigned long usecs)
{
	__const_udelay(usecs * 0x10C7UL); /* 2**32 / 1000000 (rounded up) */
}
EXPORT_SYMBOL(__udelay);

void __ndelay(unsigned long nsecs)
{
	__const_udelay(nsecs * 0x5UL); /* 2**32 / 1000000000 (rounded up) */
}
EXPORT_SYMBOL(__ndelay);
