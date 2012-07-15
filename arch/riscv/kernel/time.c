#include <linux/init.h>

#include <asm/pcr.h>

void __init time_init(void)
{
	mtpcr(PCR_COUNT, 0);
	mtpcr(PCR_COMPARE, 1000);
}
