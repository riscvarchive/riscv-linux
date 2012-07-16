#ifndef _ASM_RISCV_TIMEX_H
#define _ASM_RISCV_TIMEX_H

#include <asm/param.h>

#define CLOCK_TICK_RATE (HZ * 100UL)

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
	cycles_t n;
	__asm__ __volatile__ (
		"rdcycle %0"
		: "=r" (n));
	return n;
}

#define ARCH_HAS_READ_CURRENT_TIMER

static inline int read_current_timer(unsigned long *timer_val)
{
	*timer_val = get_cycles();
	return 0;
}

#endif /* _ASM_RISCV_TIMEX_H */
