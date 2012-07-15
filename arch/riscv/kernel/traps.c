#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/jiffies.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/regs.h>

#define FESVR_EXC_TIMER_INTERRUPT 7

extern asmlinkage void handle_exception(void);

void show_stack(struct task_struct *task, unsigned long *sp)
{
}

void dump_stack(void)
{
}

EXPORT_SYMBOL(dump_stack);

void show_regs(struct pt_regs *regs)
{
}

void do_exception(unsigned long cause,
    unsigned long epc, unsigned long badvaddr)
{
	switch (cause) {
		case FESVR_EXC_TIMER_INTERRUPT:
			mtpcr(0, PCR_COUNT);
			mtpcr(1000, PCR_COMPARE);
			__asm__ volatile ("amoadd.d x0, %0, 0(%1)" \
			    : 
			    : "r" (1UL), "r" (&jiffies_64)
			    : "memory");
			break;
		default:
			panic("Unhandled exception %ld\n", cause);
	}
}

void __init trap_init(void)
{
	/* This clears the IPI exception that started the processor */
	mtpcr(0, "cr9");

	mtpcr(&handle_exception, PCR_EVEC);
}
