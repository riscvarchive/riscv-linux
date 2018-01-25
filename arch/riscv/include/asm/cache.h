#ifndef _ASM_RISCV_CACHE_H
#define _ASM_RISCV_CACHE_H

#if defined(CONFIG_CPU_RV_ROCKET)
#define L1_CACHE_SHIFT		6
#else
#define L1_CACHE_SHIFT		5
#endif

#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)

#endif /* _ASM_RISCV_CACHE_H */
