/*
 * Based on arch/arm/include/asm/barrier.h
 *
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2013 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ASM_RISCV_BARRIER_H
#define _ASM_RISCV_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()		__asm__ __volatile__ ("nop")

#define RISCV_FENCE(p, s) \
	__asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barries need to enforce ordering on both devices or memory. */
#define mb()		RISCV_FENCE(iorw,iorw)
#define rmb()		RISCV_FENCE(ir,ir)
#define wmb()		RISCV_FENCE(ow,ow)

/* These barries do not need to enforce ordering on devices, just memory. */
#define smp_mb()	RISCV_FENCE(rw,rw)
#define smp_rmb()	RISCV_FENCE(r,r)
#define smp_wmb()	RISCV_FENCE(w,w)

/*
 * Our atomic operations set the AQ and RL bits and therefor we don't need to
 * fence around atomics.
 */
#define __smb_mb__before_atomic()	barrier()
#define __smb_mb__after_atomic()	barrier()

/*
 * These barries are meant to prevent memory operations inside a spinlock from
 * moving outside of that spinlock.  Since we set the AQ and RL bits when
 * entering or leaving spinlocks, no additional fence needs to be performed.
 */
#define smb_mb__before_spinlock()	barrier()
#define smb_mb__after_spinlock()	barrier()

/* FIXME: I don't think RISC-V is allowed to perform a speculative load. */
#define smp_acquire__after_ctrl_dep()	barrier()

/*
 * The RISC-V ISA doesn't support byte or half-word AMOs, so we fall back to a
 * regular store and a fence here.  Otherwise we emit an AMO with an AQ or RL
 * bit set and allow the microarchitecture to avoid the other half of the AMO.
 */
#define __smp_store_release(p, v)					\
do {									\
	union { typeof(*p) __val; char __c[1]; } __u =			\
		{ .__val = (__force typeof(*p)) (v) };			\
	compiletime_assert_atomic_type(*p);				\
	switch (sizeof(*p)) {						\
	case 1:								\
	case 2:								\
		smb_mb();						\
		WRITE_ONCE(*p, __u.__val);				\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"amoswap.w.rl zero, %1, %0"			\
			: "+A" (*p), "r" (__u.__val)			\
			: 						\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"amoswap.d.rl zero, %1, %0"			\
			: "+A" (*p), "r" (__u.__val)			\
			: 						\
			: "memory");					\
		break;							\
	}								\
} while (0)

#define __smp_load_acquire(p)						\
do {									\
	union { typeof(*p) __val; char __c[1]; } __u =			\
		{ .__val = (__force typeof(*p)) (v) };			\
	compiletime_assert_atomic_type(*p);				\
	switch (sizeof(*p)) {						\
	case 1:								\
	case 2:								\
		__u.__val = READ_ONCE(*p);				\
		smb_mb();						\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"amoor.w.aq %1, zero, %0"			\
			: "+A" (*p)					\
			: "=r" (__u.__val)				\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"amoor.d.aq %1, zero, %0"			\
			: "+A" (*p)					\
			: "=r" (__u.__val)				\
			: "memory");					\
		break;							\
	}								\
	__u.__val;							\
} while (0)

/*
 * The default implementation of this uses READ_ONCE and
 * smp_acquire__after_ctrl_dep, but since we can directly do an ACQUIRE load we
 * can avoid the extra barrier.
 */
#define smp_cond_load_acquire(ptr, cond_expr) ({			\
	typeof(ptr) __PTR = (ptr);					\
	typeof(*ptr) VAL;						\
	for (;;) {							\
		VAL = __smp_load_acquire(__PTR);			\
		if (cond_expr)						\
			break;						\
		cpu_relax();						\
	}								\
	smp_acquire__after_ctrl_dep();					\
	VAL;								\
})

#include <asm-generic/barrier.h>

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_BARRIER_H */
