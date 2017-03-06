#ifndef _ASM_RISCV_IRQ_H
#define _ASM_RISCV_IRQ_H

#define NR_IRQS         0

#define INTERRUPT_CAUSE_SOFTWARE    1
#define INTERRUPT_CAUSE_TIMER       5
#define INTERRUPT_CAUSE_EXTERNAL    9

void riscv_timer_interrupt(void);

#include <asm-generic/irq.h>

#endif /* _ASM_RISCV_IRQ_H */
