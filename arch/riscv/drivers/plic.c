#include <linux/interrupt.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>
#include <linux/types.h>

#include <asm/ptrace.h>
#include <asm/sbi.h>
#include <asm/sbi-con.h>
#include <asm/smp.h>
#include <asm/config-string.h>

struct plic_context {
	volatile u32 priority_threshold;
	volatile u32 claim;
};

/* There can only be one PLIC per coreplex, so statically allocate it. */
static int plic_irqs;
static DEFINE_PER_CPU(struct plic_context *, plic_context);

void plic_interrupt(void)
{
	unsigned int cpu, irq;
	struct plic_context *plic;

	cpu = smp_processor_id();
	plic = per_cpu(plic_context, cpu);

	if (plic) {
		/* Atomically claim the IRQ */
		irq = plic->claim;
		if (irq) {
			generic_handle_irq(irq-1); /* PLIC counts from 1 */
			plic->claim = irq;
		}
	}
}
EXPORT_SYMBOL_GPL(plic_interrupt)

static void plic_irq_mask(struct irq_data *d)
{
	/* mask PLIC via SBI */
	/* once implemented, power-up PLIC with all interrupts masked */
}

static void plic_irq_unmask(struct irq_data *d)
{
	/* unmask PLIC via SBI */
}

static struct irq_chip plic_irq_chip = {
	.name		= "riscv",
	.irq_mask	= plic_irq_mask,
	.irq_unmask	= plic_irq_unmask,
};

static int plic_probe(struct platform_device *pdev)
{
	int hart, irq;
	struct resource *res;
	void *plic;
	char name[40];

	/* Allocate our IRQs */
	plic_irqs = config_string_u64(pdev, "ndevs");
	irq = irq_alloc_descs(0, 0, plic_irqs, 0);

	if (irq != 0) {
		dev_err(&pdev->dev, "could not allocate %d PLIC IRQs at 0!\n", plic_irqs);
		return -ENODEV;
	}

	for (irq = 0; irq < plic_irqs; ++irq) {
		irq_set_chip_and_handler(irq, &plic_irq_chip, handle_simple_irq);
	}

	/* Configure the base addresses for the PLIC */
	for_each_cpu(hart, cpu_possible_mask) {
		per_cpu(plic_context, hart) = 0;

		sprintf(name, "%d.%d.s.ctl", 0, hart);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
		if (!res) {
			dev_warn(&pdev->dev, "could not find PLIC for hart %d\n", hart);
			continue;
		}

		plic = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(plic)) {
			dev_warn(&pdev->dev, "could not map PLIC for hart %d\n", hart);
			continue;
		}

		per_cpu(plic_context, hart) = plic;
	}

	/* Enable external interrupts */
	dev_info(&pdev->dev, "enabling %d IRQs\n", plic_irqs);
	csr_set(sie, SIE_SEIE);

	return 0;
}

static int plic_remove(struct platform_device *pdev)
{
	int hart;

	/* Disable external interrupts */
	csr_clear(sie, SIE_SEIE);

	/* Wipe out the global mapping table; actual unmap is automatic */
	for_each_cpu(hart, cpu_possible_mask) {
		per_cpu(plic_context, hart) = 0;
	}

	/* Release the descriptors */
	irq_free_descs(0, plic_irqs);

	return 0;
}

static struct platform_driver plic_driver = {
	.probe		= plic_probe,
	.remove		= plic_remove,
	.driver		= {
		.name	= "plic",
	},
};

static int __init plic_init(void)
{
	platform_driver_register(&plic_driver);
	return 0;
}

arch_initcall(plic_init)
