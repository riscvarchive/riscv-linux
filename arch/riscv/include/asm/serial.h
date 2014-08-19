#ifndef _ASM_RISCV_SERIAL_H
#define _ASM_RISCV_SERIAL_H

/*
 * Makeshift serial support, only for use with riscv-qemu.
 *
 * Currently requires that the emulator itself create a hole at addresses
 * 0x3f8 - 0x3ff without looking through page tables.
 *
 * This assumes you have a 1.8432 MHz clock for your UART.
 */
#define BASE_BAUD ( 1843200 / 16 )

/* Standard COM flags */
#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)
#endif

#define SERIAL_PORT_DFNS \
	/* UART CLK   PORT IRQ     FLAGS        */ \
	{ 0, BASE_BAUD, 0x3F8, 1, STD_COM_FLAGS }, /* ttyS0 */

#endif /* _ASM_RISCV_SERIAL_H */

