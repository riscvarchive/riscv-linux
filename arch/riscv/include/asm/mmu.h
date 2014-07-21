#ifndef __ASM_RISCV_MMU_H
#define __ASM_RISCV_MMU_H

#ifndef __ASSEMBLY__

typedef struct {
	void *vdso;
} mm_context_t;

#endif /* __ASSEMBLY__ */

#include <asm-generic/memory_model.h>

#endif /* __ASM_RISCV_MMU_H */
