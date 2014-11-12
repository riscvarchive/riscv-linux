#ifndef _ASM_RISCV_MMU_H
#define _ASM_RISCV_MMU_H

#ifndef __ASSEMBLY__

typedef struct {
	void *vdso;
} mm_context_t;

#endif /* __ASSEMBLY__ */

#include <asm-generic/memory_model.h>

#endif /* _ASM_RISCV_MMU_H */
