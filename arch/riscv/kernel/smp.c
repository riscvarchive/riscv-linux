#include <linux/smp.h>
#include <linux/sched.h>
#include <asm/sbi.h>

/* A collection of single bit ipi messages.  */
static struct {
	unsigned long bits ____cacheline_aligned;
} ipi_data[NR_CPUS] __cacheline_aligned;

enum ipi_message_type {
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CALL_FUNC_SINGLE,
	IPI_MAX
};

bool handle_ipi(void)
{
	unsigned long *pending_ipis = &ipi_data[smp_processor_id()].bits;
	unsigned long ops;

	/* Clear pending IPI */
	if (!sbi_clear_ipi())
	  return false;

	mb();	/* Order interrupt and bit testing. */
	while ((ops = xchg(pending_ipis, 0)) != 0) {
		mb();	/* Order bit clearing and data access. */

		if (ops & (1 << IPI_RESCHEDULE))
			scheduler_ipi();

		if (ops & (1 << IPI_CALL_FUNC))
			generic_smp_call_function_interrupt();

		if (ops & (1 << IPI_CALL_FUNC_SINGLE))
			generic_smp_call_function_single_interrupt();

		BUG_ON((ops >> IPI_MAX) != 0);

		mb();	/* Order data access and bit testing. */
	}

	return true;
}

static void
send_ipi_message(const struct cpumask *to_whom, enum ipi_message_type operation)
{
	int i;

	mb();
	for_each_cpu(i, to_whom)
		set_bit(operation, &ipi_data[i].bits);

	mb();
	for_each_cpu(i, to_whom)
		sbi_send_ipi(i);
}

void arch_send_call_function_ipi_mask(struct cpumask *mask)
{
	panic("TODO arch_send_call_function_ipi_mask");
}

void arch_send_call_function_single_ipi(int cpu)
{
	panic("TODO arch_send_call_function_single_ipi");
}

void smp_send_stop(void)
{
	panic("TODO smp_send_stop");
}

void smp_send_reschedule(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_RESCHEDULE);
}

void flush_icache_range(unsigned long start, unsigned long end)
{
	panic("TODO flush_icache_range");
}
