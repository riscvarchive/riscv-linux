#ifndef _ASM_RISCV_ATOMIC_H
#define _ASM_RISCV_ATOMIC_H

#include <asm-generic/atomic.h>

#define ATOMIC64_INIT(i)	{ (i) }

static inline s64 atomic64_read(const atomic64_t *v)
{
	return *((volatile long *)(&(v->counter)));
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

static inline void atomic64_add(s64 a, atomic64_t *v)
{
	(void)atomic64_add_return(a, v);
}

static inline s64 atomic64_sub_return(s64 a, atomic64_t *v)
{
	register s64 c;
	__asm__ __volatile__ (
		"sub %1, x0, %1;"
		"amoadd.d %0, %1, 0(%2);"
		: "=r" (c), "=&r" (a)
		: "r" (&(v->counter))
		: "memory");
	return (c - a);
}

static inline void atomic64_sub(s64 a, atomic64_t *v)
{
	(void)atomic64_sub_return(a, v);
}

static inline s64 atomic64_cmpxchg(atomic64_t *v, s64 o, s64 n)
{
	s64 val;
	if ((val = v->counter) == o)
		v->counter = n;
	return val;
}

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

static inline void atomic64_set(atomic64_t *v, s64 i)
{
	(void)atomic64_xchg(v, i);
}

static inline s64 atomic64_dec_if_positive(atomic64_t *v)
{
	s64 c, d, old;
	c = atomic64_read(v);
	for (;;) {
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

static inline int atomic64_add_unless(atomic64_t *v, s64 a, s64 u)
{
	if (v->counter != u) {
		v->counter += a;
		return 1;
	} else {
		return 0;
	}
}

#define atomic64_add_negative(a, v)	(atomic64_add_return((a), (v)) < 0)
#define atomic64_inc(v)			atomic64_add(1LL, (v))
#define atomic64_inc_return(v)		atomic64_add_return(1LL, (v))
#define atomic64_inc_and_test(v) 	(atomic64_inc_return(v) == 0)
#define atomic64_sub_and_test(a, v)	(atomic64_sub_return((a), (v)) == 0)
#define atomic64_dec(v)			atomic64_sub(1LL, (v))
#define atomic64_dec_return(v)		atomic64_sub_return(1LL, (v))
#define atomic64_dec_and_test(v)	(atomic64_dec_return((v)) == 0)
#define atomic64_inc_not_zero(v) 	atomic64_add_unless((v), 1LL, 0LL)

#endif /* _ASM_RISCV_ATOMIC_H */
