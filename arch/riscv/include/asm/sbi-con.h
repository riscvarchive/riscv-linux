#ifndef _ASM_RISCV_SBI_CON_H
#define _ASM_RISCV_SBI_CON_H

#include <linux/irqreturn.h>

irqreturn_t sbi_console_isr(void);

#endif /* _ASM_RISCV_SBI_CON_H */
