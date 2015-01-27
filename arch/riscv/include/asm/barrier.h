#ifndef _ASM_RISCV_BARRIER_H
#define _ASM_RISCV_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()	__asm__ __volatile__ ("nop")

#define mb()	__asm__ __volatile__ ("fence" : : : "memory")

#include <asm-generic/barrier.h>

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_BARRIER_H */
