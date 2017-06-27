/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_RISCV_HVC_RISCV_SBI_H
#define _ASM_RISCV_HVC_RISCV_SBI_H

/*
 * We always support CONFIG_EARLY_PRINTK via the SBI console driver because it
 * works well enough that there's no penalty to doing so.
 */
extern struct console riscv_sbi_early_console_dev __initdata;

#endif
