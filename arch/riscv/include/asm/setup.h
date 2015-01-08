#ifndef _ASM_RISCV_SETUP_H
#define _ASM_RISCV_SETUP_H

#ifdef __KERNEL__

#define PHYSMEM_SIZE  ((unsigned long)(*((volatile u32 *)(0UL))))
#define PHYSMEM_SHIFT (20)

#ifdef CONFIG_64BIT
#define PHYSMEM_MAX   (0x7fffffUL)
#else
#define PHYSMEM_MAX   (0xfffUL)
#endif

#endif /* __KERNEL__ */

#include <asm-generic/setup.h>

#endif /* _ASM_RISCV_SETUP_H */
