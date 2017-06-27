/*
 * RISC-V SBI console interface
 *
 * Copyright (C) 2017 SiFive
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#ifndef _ASM_RISCV_HVC_RISCV_SBI_H
#define _ASM_RISCV_HVC_RISCV_SBI_H

/*
 * We always support CONFIG_EARLY_PRINTK via the SBI console driver because it
 * works well enough that there's no penalty to doing so.
 */
extern struct console riscv_sbi_early_console_dev __initdata;

#endif
