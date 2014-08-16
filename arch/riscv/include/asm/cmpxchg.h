#ifndef _ASM_RISCV_CMPXCHG_H
#define _ASM_RISCV_CMPXCHG_H

#include <linux/bug.h>

#ifdef CONFIG_RV_ATOMIC

#include <asm/barrier.h>

#define __xchg(new, ptr, size)					\
({								\
	__typeof__(ptr) __ptr = (ptr);				\
	__typeof__(new) __new = (new);				\
	__typeof__(*(ptr)) __ret;				\
	switch (size) {						\
	case 4:							\
		__asm__ __volatile__ (				\
			"amoswap.w %0, %1, 0(%2)"		\
			: "=r" (__ret)				\
			: "r" (__new), "r" (__ptr)		\
			: "memory");				\
		break;						\
	case 8:							\
		__asm__ __volatile__ (				\
			"amoswap.d %0, %1, 0(%2)"		\
			: "=r" (__ret)				\
			: "r" (__new), "r" (__ptr)		\
			: "memory");				\
		break;						\
	default:						\
		BUILD_BUG();					\
	}							\
	__ret;							\
})

#define xchg(ptr, x)    (__xchg((x), (ptr), sizeof(*(ptr))))


/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#define __cmpxchg(ptr, old, new, size)				\
({								\
	__typeof__(ptr) __ptr = (ptr);				\
	__typeof__(old) __old = (old);				\
	__typeof__(new) __new = (new);				\
	__typeof__(*(ptr)) __ret;				\
	register unsigned int __rc;				\
	switch (size) {						\
	case 4:							\
		__asm__ __volatile__ (				\
		"0:"						\
			"lr.w %0, 0(%2)\n"			\
			"bne  %0, %3, 1f\n"			\
			"sc.w %1, %4, 0(%2)\n"			\
			"bnez %1, 0b\n"				\
		"1:"						\
			: "=&r" (__ret), "=&r" (__rc)		\
			: "r" (__ptr), "r" (__old), "r" (__new)	\
			: "memory");				\
		break;						\
	case 8:							\
		__asm__ __volatile__ (				\
		"0:"						\
			"lr.d %0, 0(%2)\n"			\
			"bne  %0, %3, 1f\n"			\
			"sc.d %1, %4, 0(%2)\n"			\
			"bnez %1, 0b\n"				\
		"1:"						\
			: "=&r" (__ret), "=&r" (__rc)		\
			: "r" (__ptr), "r" (__old), "r" (__new)	\
			: "memory");				\
		break;						\
	default:						\
		BUILD_BUG();					\
	}							\
	__ret;							\
})

#define __cmpxchg_mb(ptr, old, new, size) 			\
({								\
	__typeof__(*(ptr)) __ret;				\
	smp_mb();						\
	__ret = __cmpxchg((ptr), (old), (new), (size));		\
	smp_mb();						\
	__ret;							\
})

#define cmpxchg(ptr, o, n) \
	(__cmpxchg_mb((ptr), (o), (n), sizeof(*(ptr))))

#define cmpxchg_local(ptr, o, n) \
	(__cmpxchg((ptr), (o), (n), sizeof(*(ptr))))

#define cmpxchg64(ptr, o, n)			\
({						\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);	\
	cmpxchg((ptr), (o), (n));		\
})

#define cmpxchg64_local(ptr, o, n)		\
({						\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);	\
	cmpxchg_local((ptr), (o), (n));		\
})

#else /* !CONFIG_RV_ATOMIC */

static inline unsigned long __cmpxchg_local(volatile void *ptr,
                unsigned long old, unsigned long new, int size)
{
	register unsigned long flags, prev;

	/*
	 * Sanity checking, compile-time.
	 */
	BUILD_BUG_ON(size == 8 && sizeof(unsigned long) != 8);

	switch (size) {
	case 4:
	__asm__ __volatile__ (
	"0:"
                "csrrc %1,status,4\n"
		"lw %0, 0(%2)\n"
		"bne %0, %3, 1f\n"
		"sw %4, 0(%2)\n"
	"1:"
                "andi %1,%1,4\n"
                "beqz %1,2f\n"
                "csrsi status,4\n"
        "2:"
		: "=&r" (prev), "=&r" (flags)
		: "r" (ptr), "r" (old), "r" (new)
		: "memory");
		break;
#if defined(CONFIG_64BIT)
	case 8:
	__asm__ __volatile__ (
	"0:"
                "csrrc %1,status,4\n"
		"ld %0, 0(%2)\n"
		"bne %0, %3, 1f\n"
		"sd %4, 0(%2)\n"
	"1:"
                "andi %1,%1,4\n"
                "beqz %1,2f\n"
                "csrsi status,4\n"
        "2:"
		: "=&r" (prev), "=&r" (flags)
		: "r" (ptr), "r" (old), "r" (new)
		: "memory");
		break;
#endif
	default:
		BUILD_BUG();
	}
	return prev;
}
#define cmpxchg_local(ptr, o, n)					       \
	((__typeof__(*(ptr)))__cmpxchg_local((ptr), (unsigned long)(o),	       \
			(unsigned long)(n), sizeof(*(ptr))))

#include <asm-generic/cmpxchg.h>

#endif /* CONFIG_RV_ATOMIC */

#endif /* _ASM_RISCV_CMPXCHG_H */
