#ifndef __ASM_RISCV_MMU_H
#define __ASM_RISCV_MMU_H

#ifndef __ASSEMBLY__
typedef struct {
	struct vm_list_struct *vmlist;
	unsigned long end_brk;
} mm_context_t;

static inline struct vm_struct **
pcpu_get_vm_areas(const unsigned long *offsets,
		const size_t *sizes, int nr_vms,
		size_t align)
{
	return NULL;
}

static inline void
pcpu_free_vm_areas(struct vm_struct **vms, int nr_vms)
{
}

#endif

#include <asm-generic/memory_model.h>

#endif /* __ASM_RISCV_MMU_H */
