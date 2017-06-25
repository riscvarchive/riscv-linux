/*
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#ifndef _ASM_RISCV_ATOMIC_H
#define _ASM_RISCV_ATOMIC_H

#ifdef CONFIG_ISA_A

#include <asm/cmpxchg.h>
#include <asm/barrier.h>

#define ATOMIC_INIT(i)	{ (i) }

static __always_inline int atomic_read(const atomic_t *v)
{
	return READ_ONCE(v->counter);
}

static __always_inline void atomic_set(atomic_t *v, int i)
{
	WRITE_ONCE(v->counter, i);
}

/* The atomic operations that are supported via AMOs.  atomic_fetch_* is AQ+RL
 * as it's required to include a barrier, while atomic_* doesn't have any
 * ordering constraints.  We don't need any fences here, as the RISC-V spec
 * says that the AQ and RL bits enforce ordering with all memory operations.
 */
#define ATOMIC_OP(op, asm_op, c_op, I)					\
static __always_inline void atomic_##op(int i, atomic_t *v)		\
{									\
	__asm__ __volatile__ (						\
		"amo" #asm_op ".w zero, %1, %0"				\
		: "+A" (v->counter)					\
		: "r" (I));						\
}

#define ATOMIC_FETCH_OP(op, asm_op, c_op, I)				\
static __always_inline int atomic_fetch_##op(int i, atomic_t *v)	\
{									\
	register int ret;						\
	__asm__ __volatile__ (						\
		"amo" #asm_op ".w.aqrl %2, %1, %0"			\
		: "+A" (v->counter), "=r" (ret)				\
		: "r" (I));						\
	return ret;							\
}

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I)				\
static __always_inline int atomic_##op##_return(int i, atomic_t *v)	\
{									\
        return atomic_fetch_##op(i, v) c_op I;				\
}

#define ATOMIC_OPS(op, asm_op, c_op, I)					\
        ATOMIC_OP(op, asm_op, c_op, I)					\
        ATOMIC_FETCH_OP(op, asm_op, c_op, I)				\
        ATOMIC_OP_RETURN(op, asm_op, c_op, I)

ATOMIC_OPS(add, add, +, i)
ATOMIC_OPS(sub, add, +, -i)

#undef ATOMIC_OPS

#define ATOMIC_OPS(op, asm_op, c_op, I)					\
        ATOMIC_OP(op, asm_op, c_op, I)					\
        ATOMIC_FETCH_OP(op, asm_op, c_op, I)

/* FIXME: I could only find documentation that atomic_{add,sub,inc,dec} are
 * barrier-free.  I'm assuming that and/or/xor have the same constraints as the
 * others.
 */
ATOMIC_OPS(and, and, &, i)
ATOMIC_OPS(or, or, |, i)
ATOMIC_OPS(xor, xor, ^, i)

#undef ATOMIC_OPS

#undef ATOMIC_OP
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN

/* The extra atomic operations that are constructed from one of the core
 * AMO-based operations above (aside from sub, which is easier to fit above).
 * These are required to perform a barrier, but they're OK this way because
 * atomic_*_return is also required to perform a barrier.
 */
#define ATOMIC_OP(op, func_op, comp_op, I)				\
static __always_inline bool atomic_##op(int i, atomic_t *v)		\
{									\
	return atomic_##func_op##_return(i, v) comp_op I;		\
}

ATOMIC_OP(add_and_test, add, ==, 0)
ATOMIC_OP(sub_and_test, sub, ==, 0)
ATOMIC_OP(add_negative, add, <, 0)

#undef ATOMIC_OP

#define ATOMIC_OP(op, func_op, c_op, I)					\
static __always_inline void atomic_##op(atomic_t *v)			\
{									\
	atomic_##func_op(I, v);						\
}

#define ATOMIC_FETCH_OP(op, func_op, c_op, I)				\
static __always_inline int atomic_fetch_##op(atomic_t *v)		\
{									\
	return atomic_fetch_##func_op(I, v);				\
}

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I)				\
static __always_inline int atomic_##op##_return(atomic_t *v)		\
{									\
        return atomic_fetch_##op(v) c_op I;				\
}

#define ATOMIC_OPS(op, asm_op, c_op, I)					\
        ATOMIC_OP(op, asm_op, c_op, I)					\
        ATOMIC_FETCH_OP(op, asm_op, c_op, I)				\
        ATOMIC_OP_RETURN(op, asm_op, c_op, I)

ATOMIC_OPS(inc, add, +, 1)
ATOMIC_OPS(dec, add, +, -1)

#undef ATOMIC_OPS
#undef ATOMIC_OP
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN

#define ATOMIC_OP(op, func_op, comp_op, I)				\
static __always_inline bool atomic_##op(atomic_t *v)			\
{									\
	return atomic_##func_op##_return(v) comp_op I;			\
}

ATOMIC_OP(inc_and_test, inc, ==, 0)
ATOMIC_OP(dec_and_test, dec, ==, 0)

#undef ATOMIC_OP

/* This is required to provide a barrier on success. */
static __always_inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
       register int prev, rc;

	__asm__ __volatile__ (
		"0:\n\t"
		"lr.w.aqrl %0, %2\n\t"
		"beq       %0, %4, 1f\n\t"
		"add       %1, %0, %3\n\t"
		"sc.w.aqrl %1, %1, %2\n\t"
		"bnez      %1, 0b\n\t"
		"1:"
		: "=&r" (prev), "=&r" (rc), "+A" (v->counter)
		: "r" (a), "r" (u));
	return prev;
}

/* The extra atomic64 operations that ore constructed from one of the core
 * LR/SC-based operations above.
 */
static __always_inline int atomic_inc_not_zero(atomic_t *v)
{
        return __atomic_add_unless(v, 1, 0);
}

/* atomic_{cmp,}xchg is required to have exactly the same ordering semantics as
 * {cmp,}xchg and the operations that return, so they need a barrier.  We just
 * use the other implementations directly.
 */
static __always_inline int atomic_cmpxchg(atomic_t *v, int o, int n)
{
	return cmpxchg32(&(v->counter), o, n);
}

static __always_inline int atomic_xchg(atomic_t *v, int n)
{
	return xchg32(&(v->counter), n);
}


#else /* !CONFIG_ISA_A */

#include <asm-generic/atomic.h>

#endif /* CONFIG_ISA_A */

#include <asm/atomic64.h>

#endif /* _ASM_RISCV_ATOMIC_H */
