#ifndef _ASM_RISCV_SEGMENT_H
#define _ASM_RISCV_SEGMENT_H

#include <asm-generic/segment.h>

#ifndef __ASSEMBLY__

typedef struct {
	unsigned long seg;
} mm_segment_t;

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_SEGMENT_H */
