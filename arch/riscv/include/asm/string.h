#ifndef _ASM_RISCV_STRING_H
#define _ASM_RISCV_STRING_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/linkage.h>

#define __HAVE_ARCH_MEMSET
extern asmlinkage void *memset(void *, int, size_t);

#define __HAVE_ARCH_MEMCPY
extern asmlinkage void *memcpy(void *, const void *, size_t);

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_STRING_H */
