#ifndef _ASM_RISCV_ATOMIC_H
#define _ASM_RISCV_ATOMIC_H

#ifdef CONFIG_RV_ATOMIC

#include <asm/cmpxchg.h>
#include <asm/barrier.h>

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
static inline int atomic_read(const atomic_t *v)
{
	return *((volatile int *)(&(v->counter)));
}

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static inline void atomic_set(atomic_t *v, int i)
{
	v->counter = i;
}

/**
 * atomic_add_return - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns the result
 */
static inline int atomic_add_return(int i, atomic_t *v)
{
	return __atomic_fetch_add(&(v->counter), i, 0) + i;
}

/**
 * atomic_sub_return - subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns the result
 */
static inline int atomic_sub_return(int i, atomic_t *v)
{
	return atomic_add_return(-i, v);
}

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.
 */
static inline void atomic_add(int i, atomic_t *v)
{
	(void) atomic_add_return(i, v);
}

/**
 * atomic_sub - subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.
 */
static inline void atomic_sub(int i, atomic_t *v)
{
	atomic_add(-i, v);
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_inc(atomic_t *v)
{
	atomic_add(1, v);
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.
 */
static inline void atomic_dec(atomic_t *v)
{
	atomic_add(-1, v);
}

static inline int atomic_inc_return(atomic_t *v)
{
	return atomic_add_return(1, v);
}

static inline int atomic_dec_return(atomic_t *v)
{
	return atomic_sub_return(1, v);
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_sub_and_test(int i, atomic_t *v)
{
	return (atomic_sub_return(i, v) == 0);
}

/**
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_inc_and_test(atomic_t *v)
{
	return (atomic_inc_return(v) == 0);
}

/**
 * atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int atomic_dec_and_test(atomic_t *v)
{
	return (atomic_dec_return(v) == 0);
}

/**
 * atomic_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer of type atomic_t
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic_add_negative(int i, atomic_t *v)
{
	return (atomic_add_return(i, v) < 0);
}


static inline int atomic_xchg(atomic_t *v, int n)
{
	return xchg(&(v->counter), n);
}

static inline int atomic_cmpxchg(atomic_t *v, int o, int n)
{
	register int prev, rc;
	__asm__ __volatile__ (
	"0:"
		"lr.w %0, 0(%2)\n"
		"bne  %0, %3, 1f\n"
		"sc.w %1, %4, 0(%2)\n"
		"bnez %1, 0b\n"
	"1:"
		: "=&r" (prev), "=&r" (rc)
		: "r" (&(v->counter)), "r" (o), "r" (n)
		: "memory");
	return prev;
}

/**
 * __atomic_add_unless - add unless the number is already a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as @v was not already @u.
 * Returns the old value of @v.
 */
static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	register int prev, rc;
	__asm__ __volatile__ (
	"0:"
		"lr.w %0, 0(%3)\n"
		"beq  %0, %4, 1f\n"
#ifdef CONFIG_64BIT
		"addw %1, %0, %2\n"
#else
		"add  %1, %0, %2\n"
#endif /* CONFIG_64BIT */
		"sc.w %1, %1, 0(%3)\n"
		"bnez %1, 0b\n"
	"1:"
		: "=&r" (prev), "=&r" (rc)
		: "r" (a), "r" (&(v->counter)), "r" (u)
		: "memory");
	return prev;
}

/**
 * atomic_clear_mask - Atomically clear bits in atomic variable
 * @mask: Mask of the bits to be cleared
 * @v: pointer of type atomic_t
 *
 * Atomically clears the bits set in @mask from @v
 */
static inline void atomic_clear_mask(unsigned int mask, atomic_t *v)
{
	__atomic_fetch_and(&(v->counter), ~mask, 0);
}

/**
 * atomic_set_mask - Atomically set bits in atomic variable
 * @mask: Mask of the bits to be set
 * @v: pointer of type atomic_t
 *
 * Atomically sets the bits set in @mask in @v
 */
static inline void atomic_set_mask(unsigned int mask, atomic_t *v)
{
	__atomic_fetch_or(&(v->counter), mask, 0);
}

/* Assume that atomic operations are already serializing */
#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()
#define smp_mb__after_atomic_inc()	barrier()

#else /* !CONFIG_RV_ATOMIC */

#include <asm-generic/atomic.h>

#endif /* CONFIG_RV_ATOMIC */

#include <asm/atomic64.h>

#endif /* _ASM_RISCV_ATOMIC_H */
