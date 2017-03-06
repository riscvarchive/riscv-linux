#ifndef _ASM_RISCV_SERIAL_H
#define _ASM_RISCV_SERIAL_H

/*
 * FIXME: interim serial support for riscv-qemu
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

#define SERIAL_PORT_DFNS 			\
	{	/* ttyS0 */			\
		.baud_base = BASE_BAUD,		\
		.port      = 0x3F8,		\
		.irq       = 4,			\
		.flags     = STD_COM_FLAGS,	\
	},

#endif /* _ASM_RISCV_SERIAL_H */
