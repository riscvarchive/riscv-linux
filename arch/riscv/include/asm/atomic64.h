#ifndef _ASM_RISCV_ATOMIC64_H
#define _ASM_RISCV_ATOMIC64_H

#ifdef CONFIG_GENERIC_ATOMIC64
#include <asm-generic/atomic64.h>
#else /* !CONFIG_GENERIC_ATOMIC64 */

#include <linux/types.h>

#define ATOMIC64_INIT(i)	{ (i) }

/**
 * atomic64_read - read atomic64 variable
 * @v: pointer of type atomic64_t
 *
 * Atomically reads the value of @v.
 */
static inline s64 atomic64_read(const atomic64_t *v)
{
	return *((volatile long *)(&(v->counter)));
}

/**
 * atomic64_set - set atomic64 variable
 * @v: pointer to type atomic64_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static inline void atomic64_set(atomic64_t *v, s64 i)
{
	v->counter = i;
}

/**
 * atomic64_add - add integer to atomic64 variable
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v.
 */
static inline void atomic64_add(s64 a, atomic64_t *v)
{
	__asm__ __volatile__ (
		"amoadd.d zero, %1, %0"
		: "+A" (v->counter)
		: "r" (a));
}

/**
 * atomic64_sub - subtract the atomic64 variable
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v.
 */
static inline void atomic64_sub(s64 a, atomic64_t *v)
{
	atomic64_add(-a, v);
}

/**
 * atomic64_add_return - add and return
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static inline s64 atomic64_add_return(s64 a, atomic64_t *v)
{
	register s64 c;
	__asm__ __volatile__ (
		"amoadd.d %0, %2, %1"
		: "=r" (c), "+A" (v->counter)
		: "r" (a));
	return (c + a);
}

static inline s64 atomic64_sub_return(s64 a, atomic64_t *v)
{
	return atomic64_add_return(-a, v);
}

/**
 * atomic64_inc - increment atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic64_inc(atomic64_t *v)
{
	atomic64_add(1L, v);
}

/**
 * atomic64_dec - decrement atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1.
 */
static inline void atomic64_dec(atomic64_t *v)
{
	atomic64_add(-1L, v);
}

static inline s64 atomic64_inc_return(atomic64_t *v)
{
	return atomic64_add_return(1L, v);
}

static inline s64 atomic64_dec_return(atomic64_t *v)
{
	return atomic64_add_return(-1L, v);
}

/**
 * atomic64_inc_and_test - increment and test
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic64_inc_and_test(atomic64_t *v)
{
	return (atomic64_inc_return(v) == 0);
}

/**
 * atomic64_dec_and_test - decrement and test
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int atomic64_dec_and_test(atomic64_t *v)
{
	return (atomic64_dec_return(v) == 0);
}

/**
 * atomic64_sub_and_test - subtract value from variable and test result
 * @a: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @a from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic64_sub_and_test(s64 a, atomic64_t *v)
{
	return (atomic64_sub_return(a, v) == 0);
}

/**
 * atomic64_add_negative - add and test if negative
 * @a: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @a to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic64_add_negative(s64 a, atomic64_t *v)
{
	return (atomic64_add_return(a, v) < 0);
}


static inline s64 atomic64_xchg(atomic64_t *v, s64 n)
{
	register s64 c;
	__asm__ __volatile__ (
		"amoswap.d %0, %2, %1"
		: "=r" (c), "+A" (v->counter)
		: "r" (n));
	return c;
}

static inline s64 atomic64_cmpxchg(atomic64_t *v, s64 o, s64 n)
{
	return cmpxchg(&(v->counter), o, n);
}

/*
 * atomic64_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic_t
 *
 * The function returns the old value of *v minus 1, even if
 * the atomic variable, v, was not decremented.
 */
static inline s64 atomic64_dec_if_positive(atomic64_t *v)
{
	register s64 prev, rc;
	__asm__ __volatile__ (
	"0:"
		"lr.d %0, %2\n"
		"add  %0, %0, -1\n"
		"bltz %0, 1f\n"
		"sc.w %1, %0, %2\n"
		"bnez %1, 0b\n"
	"1:"
		: "=&r" (prev), "=r" (rc), "+A" (v->counter));
	return prev;
}

/**
 * atomic64_add_unless - add unless the number is a given value
 * @v: pointer of type atomic64_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns true if the addition occurred and false otherwise.
 */
static inline int atomic64_add_unless(atomic64_t *v, s64 a, s64 u)
{
	register s64 tmp;
	register int rc = 1;

	__asm__ __volatile__ (
	"0:"
		"lr.d %0, %2\n"
		"beq  %0, %z4, 1f\n"
		"add  %0, %0, %3\n"
		"sc.d %1, %0, %2\n"
		"bnez %1, 0b\n"
	"1:"
		: "=&r" (tmp), "=&r" (rc), "+A" (v->counter)
		: "rI" (a), "rJ" (u));
	return !rc;
}

static inline int atomic64_inc_not_zero(atomic64_t *v)
{
	return atomic64_add_unless(v, 1, 0);
}

#endif /* CONFIG_GENERIC_ATOMIC64 */

#endif /* _ASM_RISCV_ATOMIC64_H */
