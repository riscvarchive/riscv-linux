#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/irq.h>
#include <asm/csr.h>

static int riscv_timer_set_next_event(unsigned long delta,
	struct clock_event_device *evdev)
{
	/* Set comparator */
	csr_write(stimecmp, csr_read(stime) + delta);
	return 0;
}

static void riscv_timer_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evdev)
{
	switch (mode) {
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	case CLOCK_EVT_MODE_PERIODIC:
	default:
		BUG();
	}
}

static struct clock_event_device riscv_clockevent = {
	.name = "riscv_timer_clockevent",
	.features = CLOCK_EVT_FEAT_ONESHOT,
	.rating = 300,
	.set_next_event = riscv_timer_set_next_event,
	.set_mode = riscv_timer_set_mode,
};

static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evdev;

	evdev = dev_id;
	evdev->event_handler(evdev);
	return IRQ_HANDLED;
}

static struct irqaction timer_irq = {
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED | IRQF_TIMER,
	.name = "timer",
	.dev_id = &riscv_clockevent,
};


static cycle_t riscv_rdcycle(struct clocksource *cs)
{
	return csr_read(cycle);
}

static struct clocksource riscv_clocksource = {
	.name = "riscv_clocksource",
	.rating = 300,
	.read = riscv_rdcycle,
#ifdef CONFIG_64BITS
	.mask = CLOCKSOURCE_MASK(64),
#else
	.mask = CLOCKSOURCE_MASK(32),
#endif /* CONFIG_64BITS */
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};


void __init time_init(void)
{
	u32 freq;
	freq = 100000000UL;

	csr_write(stime, 0);

	clocksource_register_hz(&riscv_clocksource, freq);
	setup_irq(IRQ_TIMER, &timer_irq);
	riscv_clockevent.cpumask = cpumask_of(0);
	clockevents_config_and_register(&riscv_clockevent,
		freq, 100, 0xffffffff);
}
