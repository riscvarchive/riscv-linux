#include <asm/htif.h>

void write_tohost(unsigned long val)
{
	__asm__ volatile ("mtpcr %0, cr30" : : "r" (val));
}

unsigned long read_tohost(void)
{
	unsigned long val;

	__asm__ volatile ("mfpcr %0, cr30" : "=r" (val));
	return val;
}

void write_fromhost(unsigned long val)
{
	__asm__ volatile ("mtpcr %0, cr31" : : "r" (val));
}

unsigned long read_fromhost(void)
{
	unsigned long val;

	__asm__ volatile ("mfpcr %0, cr31" : "=r" (val));
	return val;
}
