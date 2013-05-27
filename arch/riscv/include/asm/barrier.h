#ifndef _ASM_RISCV_BARRIER_H
#define _ASM_RISCV_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()	__asm__ __volatile__ ("nop")

#define mb()	__asm__ __volatile__ ("fence" : : : "memory")
#define rmb()	mb()
#define wmb()	mb()

#ifdef CONFIG_SMP
#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()
#else
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#endif /* CONFIG_SMP */

#define set_mb(var, value)  do { var = value;  mb(); } while (0)
#define set_wmb(var, value) do { var = value; wmb(); } while (0)

#define read_barrier_depends()		do {} while (0)
#define smp_read_barrier_depends()	do {} while (0)

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_BARRIER_H */
