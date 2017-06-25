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

/* This file is mostly the same as <asm/atomic.h>.  See the comments there for
 * more information. */
#ifndef _ASM_RISCV_ATOMIC64_H
#define _ASM_RISCV_ATOMIC64_H

#ifdef CONFIG_GENERIC_ATOMIC64

#include <asm-generic/atomic64.h>

#else /* !CONFIG_GENERIC_ATOMIC64 */

#if defined(CONFIG_ISA_A) && (__riscv_xlen >= 64)

#include <asm/cmpxchg.h>
#include <asm/barrier.h>

#define ATOMIC64_INIT(i)	{ (i) }

static __always_inline s64 atomic64_read(const atomic64_t *v)
{
	return READ_ONCE(v->counter);
}

static __always_inline void atomic64_set(atomic64_t *v, s64 i)
{
	WRITE_ONCE(v->counter, i);
}

#define ATOMIC_OP(op, asm_op, c_op, I)					\
static __always_inline void atomic64_##op(s64 i, atomic64_t *v)		\
{									\
	__asm__ __volatile__ (						\
		"amo" #asm_op ".d zero, %1, %0"				\
		: "+A" (v->counter)					\
		: "r" (I));						\
}

#define ATOMIC_FETCH_OP(op, asm_op, c_op, I)				\
static __always_inline s64 atomic64_fetch_##op(s64 i, atomic64_t *v)	\
{									\
	register s64 ret;						\
	__asm__ __volatile__ (						\
		"amo" #asm_op ".d.aqrl %2, %1, %0"			\
		: "+A" (v->counter), "=r" (ret)				\
		: "r" (I));						\
	return ret;							\
}

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I)				\
static __always_inline s64 atomic64_##op##_return(s64 i, atomic64_t *v)	\
{									\
        return atomic64_fetch_##op(i, v) c_op I;			\
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

ATOMIC_OPS(and, and, &, i)
ATOMIC_OPS(or, or, |, i)
ATOMIC_OPS(xor, xor, ^, i)

#undef ATOMIC_OPS

#undef ATOMIC_OP
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN

#define ATOMIC_OP(op, func_op, comp_op, I)				\
static __always_inline bool atomic64_##op(s64 i, atomic64_t *v)		\
{									\
	return atomic64_##func_op##_return(i, v) comp_op I;		\
}

ATOMIC_OP(add_and_test, add, ==, 0)
ATOMIC_OP(sub_and_test, sub, ==, 0)
ATOMIC_OP(add_negative, add, <, 0)

#undef ATOMIC_OP

#define ATOMIC_OP(op, func_op, c_op, I)					\
static __always_inline void atomic64_##op(atomic64_t *v)		\
{									\
	atomic64_##func_op(I, v);					\
}

#define ATOMIC_FETCH_OP(op, func_op, c_op, I)				\
static __always_inline s64 atomic64_fetch_##op(atomic64_t *v)		\
{									\
	return atomic64_fetch_##func_op(I, v);				\
}

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I)				\
static __always_inline s64 atomic64_##op##_return(atomic64_t *v)	\
{									\
        return atomic64_fetch_##op(v) c_op I;				\
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
static __always_inline bool atomic64_##op(atomic64_t *v)		\
{									\
	return atomic64_##func_op##_return(v) comp_op I;		\
}

ATOMIC_OP(inc_and_test, inc, ==, 0)
ATOMIC_OP(dec_and_test, dec, ==, 0)

#undef ATOMIC_OP

static __always_inline s64 atomic64_add_unless(atomic64_t *v, s64 a, s64 u)
{
       register s64 prev, rc;

	__asm__ __volatile__ (
		"0:\n\t"
		"lr.d.aqrl %0, %2\n\t"
		"beq       %0, %4, 1f\n\t"
		"add       %1, %0, %3\n\t"
		"sc.d.aqrl %1, %1, %2\n\t"
		"bnez      %1, 0b\n\t"
		"1:"
		: "=&r" (prev), "=&r" (rc), "+A" (v->counter)
		: "r" (a), "r" (u));
	return prev;
}

static __always_inline s64 atomic64_inc_not_zero(atomic64_t *v)
{
	return atomic64_add_unless(v, 1, 0);
}

static __always_inline s64 atomic64_cmpxchg(atomic64_t *v, s64 o, s64 n)
{
	return cmpxchg64(&(v->counter), o, n);
}

static __always_inline s64 atomic64_xchg(atomic64_t *v, s64 n)
{
	return xchg64(&(v->counter), n);
}


#endif /* defined(CONFIG_ISA_A) && (__riscv_xlen >= 64) */

#endif /* CONFIG_GENERIC_ATOMIC64 */

#endif /* _ASM_RISCV_ATOMIC64_H */
