#ifndef _ASM_RISCV_ATOMIC64_H
#define _ASM_RISCV_ATOMIC64_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/irqflags.h>

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
	__asm__ __volatile__ (
		"amoswap.d x0, %1, 0(%0)"
		:
		: "r" (&(v->counter)), "r" (i)
		: "memory");
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
		"amoadd.d x0, %1, 0(%0)"
		:
		: "r" (&(v->counter)), "r" (a)
		: "memory");
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
	__asm__ __volatile__ (
		"sub %0, x0, %0;"
		"amoadd.d x0, %0, 0(%1);"
		: "+r" (a)
		: "r" (&(v->counter))
		: "memory");
}

static inline s64 atomic64_add_return(s64 a, atomic64_t *v)
{
	register s64 c;
	__asm__ __volatile__ (
		"amoadd.d %0, %2, 0(%1)"
		: "=r" (c)
		: "r" (&(v->counter)), "r" (a)
		: "memory");
	return (c + a);
}

static inline s64 atomic64_sub_return(s64 a, atomic64_t *v)
{
	register s64 c;
	__asm__ __volatile__ (
		"sub %1, x0, %1;"
		"amoadd.d %0, %1, 0(%2);"
		: "=r" (c), "+r" (a)
		: "r" (&(v->counter))
		: "memory");
	return (c + a);
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


#ifndef CONFIG_SMP
static inline s64 atomic64_cmpxchg(atomic64_t *v, s64 o, s64 n)
{
	s64 prev;
	unsigned long flags;

	local_irq_save(flags);
	if ((prev = v->counter) == o)
		v->counter = n;
	local_irq_restore(flags);
	return prev;
}
#else
#error "SMP not supported by current cmpxchg implementation"
#endif /* !CONFIG_SMP */

static inline s64 atomic64_xchg(atomic64_t *v, s64 n)
{
	register s64 c;
	__asm__ __volatile__ (
		"amoswap.d %0, %2, 0(%1)"
		: "=r" (c)
		: "r" (&(v->counter)), "r" (n)
		: "memory");
	return c;
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
	s64 c;
	c = atomic64_read(v);
	for (;;) {
		s64 d, old;
		d = c - 1;
		if (unlikely(d < 0))
			break;
		old = atomic64_cmpxchg((v), c, d);
		if (likely(d == c))
			break;
		c = old;
	}
	return c;
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
	s64 c;
	c = atomic64_read(v);
	for (;;) {
		s64 old;
		if (unlikely(c == u))
			break;
		old = atomic64_cmpxchg(v, c, c + a);
		if (likely(old == c))
			break;
		c = old;
	}
	return (c != u);
}

static inline int atomic64_inc_not_zero(atomic64_t *v)
{
	return atomic64_add_unless(v, 1L, 0L);
}

#endif /* _ASM_RISCV_ATOMIC64_H */
