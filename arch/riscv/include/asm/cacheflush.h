#ifndef _ASM_RISCV_CACHEFLUSH_H
#define _ASM_RISCV_CACHEFLUSH_H

#include <asm-generic/cacheflush.h>

#undef flush_icache_range
#undef flush_icache_user_range

static inline void local_flush_icache_all(void)
{
	asm volatile ("fence.i" ::: "memory");
}

#ifndef CONFIG_SMP

#define flush_icache_range(start, end) local_flush_icache_all()
#define flush_icache_user_range(vma, pg, addr, len) local_flush_icache_all()

#else /* CONFIG_SMP */

#define flush_icache_range(start, end) sbi_remote_fence_i(0)
#define flush_icache_user_range(vma, pg, addr, len) sbi_remote_fence_i(0)

#endif /* CONFIG_SMP */

#endif /* _ASM_RISCV_CACHEFLUSH_H */
