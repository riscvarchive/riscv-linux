#ifndef _ASM_RISCV_ATOMIC_H
#define _ASM_RISCV_ATOMIC_H

#include <asm-generic/atomic.h>

#define ATOMIC64_INIT(i)	{ (i) }

static inline long long atomic64_read(const atomic64_t *v)
{
	return 0;
}

static inline void atomic64_set(atomic64_t *v, long long i)
{
}

static inline void atomic64_add(long long a, atomic64_t *v)
{
}

static inline long long atomic64_add_return(long long a, atomic64_t *v)
{
	return 0;
}

static inline void atomic64_sub(long long a, atomic64_t *v)
{
}

static inline long long atomic64_sub_return(long long a, atomic64_t *v)
{
	return 0;
}

static inline long long atomic64_dec_if_positive(atomic64_t *v)
{
	return 0;
}

static inline long long atomic64_cmpxchg(atomic64_t *v, long long o, long long n)
{
	return 0;
}

static inline long long atomic64_xchg(atomic64_t *v, long long new)
{
	return 0;
}

static inline int atomic64_add_unless(atomic64_t *v, long long a, long long u)
{
	return 0;
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
