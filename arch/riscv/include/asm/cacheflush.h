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

#define flush_icache_range local_flush_icache_all()
#define flush_icache_user_range(vma, pg, addr, len) local_flush_icache_all()

#else /* CONFIG_SMP */

void flush_icache_range(uintptr_t start, uintptr_t end);
#define flush_icache_user_range(vma, pg, addr, len) \
	flush_icache_range((addr), (addr) + (len))

#endif /* CONFIG_SMP */

#endif /* _ASM_RISCV_CACHEFLUSH_H */
