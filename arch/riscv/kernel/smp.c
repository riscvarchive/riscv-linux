#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/sched.h>

#include <asm/sbi.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

/* A collection of single bit ipi messages.  */
static struct {
	unsigned long bits ____cacheline_aligned;
} ipi_data[NR_CPUS] __cacheline_aligned;

enum ipi_message_type {
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_MAX
};

irqreturn_t handle_ipi(void)
{
	unsigned long *pending_ipis = &ipi_data[smp_processor_id()].bits;

	/* Clear pending IPI */
	csr_clear(sip, SIE_SSIE);

	while (true) {
		unsigned long ops;

		mb();	/* Order bit clearing and data access. */

		if ((ops = xchg(pending_ipis, 0)) == 0)
			return IRQ_HANDLED;

		if (ops & (1 << IPI_RESCHEDULE))
			scheduler_ipi();

		if (ops & (1 << IPI_CALL_FUNC))
			generic_smp_call_function_interrupt();

		BUG_ON((ops >> IPI_MAX) != 0);

		mb();	/* Order data access and bit testing. */
	}

	return IRQ_HANDLED;
}

static void
send_ipi_message(const struct cpumask *to_whom, enum ipi_message_type operation)
{
	int i;

	mb();
	for_each_cpu(i, to_whom)
		set_bit(operation, &ipi_data[i].bits);

	mb();
	sbi_send_ipi(cpumask_bits(to_whom));
}

void arch_send_call_function_ipi_mask(struct cpumask *mask)
{
	send_ipi_message(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_CALL_FUNC);
}

static void ipi_stop(void *unused)
{
	while (1)
		wait_for_interrupt();
}

void smp_send_stop(void)
{
	on_each_cpu(ipi_stop, NULL, 1);
}

void smp_send_reschedule(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_RESCHEDULE);
}
