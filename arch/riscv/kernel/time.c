#include <linux/init.h>

#include <asm/regs.h>

void __init time_init(void)
{
	mtpcr(0, PCR_COUNT);
	mtpcr(1000, PCR_COMPARE);
}
