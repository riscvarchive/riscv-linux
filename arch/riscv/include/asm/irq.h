#ifndef _ASM_RISCV_IRQ_H
#define _ASM_RISCV_IRQ_H

#define NR_IRQS         8
#define IRQ_IPI		5
#define IRQ_HOST	6
#define IRQ_TIMER       7

extern void input_irq_init(void);

#include <asm-generic/irq.h>

#endif /* _ASM_RISCV_IRQ_H */
