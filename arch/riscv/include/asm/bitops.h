#ifndef _ASM_RISCV_BITOPS_H
#define _ASM_RISCV_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error "Only <linux/bitops.h> can be included directly"
#endif /* _LINUX_BITOPS_H */

#ifdef __KERNEL__

#include <linux/compiler.h>
#include <linux/irqflags.h>
#include <asm/barrier.h>
#include <asm/bitsperlong.h>

#ifdef CONFIG_RV_ATOMIC

#define LONG_MASK(nr) (1UL << ((nr) & (BITS_PER_LONG - 1)))
#ifdef CONFIG_64BIT
#define LONG_WORD(nr) ((nr) >> 6)
#else
#define LONG_WORD(nr) ((nr) >> 5)
#endif /* CONFIG_64BIT */

#ifndef smp_mb__before_clear_bit
#define smp_mb__before_clear_bit()  smp_mb()
#define smp_mb__after_clear_bit()   smp_mb()
#endif /* smp_mb__before_clear_bit */

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
/*
static __always_inline unsigned long __ffs(unsigned long word)
{
	return 0;
}
*/
#include <asm-generic/bitops/__ffs.h>

#include <asm-generic/bitops/ffz.h>

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
/*
static __always_inline int fls(int x)
{
	return 0;
}
*/
#include <asm-generic/bitops/fls.h>

/**
 * __fls - find last (most-significant) set bit in a long word
 * @word: the word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
/*
static __always_inline unsigned long __fls(unsigned long word)
{
	return 0;
}
*/
#include <asm-generic/bitops/__fls.h>

#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/find.h>
#include <asm-generic/bitops/sched.h>

/**
 * ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
/*
static __always_inline int ffs(int x)
{
	return 0;
}
*/
#include <asm-generic/bitops/ffs.h>

#include <asm-generic/bitops/hweight.h>

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It may be reordered on other architectures than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long res;
	unsigned long mask;
	volatile unsigned long *p;

	mask = LONG_MASK(nr);
	p = addr + LONG_WORD(nr);

	__asm__ __volatile__ (
		"amoor.d %0, %2, %1"
		: "=r" (res), "+A" (*p)
		: "r" (mask));

	return ((res & mask) != 0);
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It can be reordered on other architectures other than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long res;
	unsigned long mask;
	volatile unsigned long *p;

	mask = LONG_MASK(nr);
	p = addr + LONG_WORD(nr);

	__asm__ __volatile__ (
		"amoand.d %0, %2, %1"
		: "=r" (res), "+A" (*p)
		: "r" (~mask));

	return ((res & mask) != 0);
}

/**
 * test_and_change_bit - Change a bit and return its old value
 * @nr: Bit to change
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long res;
	unsigned long mask;
	volatile unsigned long *p;

	mask = LONG_MASK(nr);
	p = addr + LONG_WORD(nr);

	__asm__ __volatile__ (
		"amoxor.d %0, %2, %1"
		: "=r" (res), "+A" (*p)
		: "r" (mask));

	return ((res & mask) != 0);
}

/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 *
 * Note: there are no guarantees that this function will not be reordered
 * on non x86 architectures, so if you are writing portable code,
 * make sure not to rely on its reordering guarantees.
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
	(void)test_and_set_bit(nr, addr);
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static inline void clear_bit(int nr, volatile unsigned long *addr)
{
	(void)test_and_clear_bit(nr, addr);
}

/**
 * change_bit - Toggle a bit in memory
 * @nr: Bit to change
 * @addr: Address to start counting from
 *
 * change_bit() is atomic and may not be reordered. It may be
 * reordered on other architectures than x86.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline void change_bit(int nr, volatile unsigned long *addr)
{
	(void)test_and_change_bit(nr, addr);
}

/**
 * test_and_set_bit_lock - Set a bit and return its old value, for lock
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and provides acquire barrier semantics.
 * It can be used to implement bit locks.
 */
static inline int test_and_set_bit_lock(
	unsigned long nr, volatile unsigned long *addr)
{
	return test_and_set_bit(nr, addr);
}

/**
 * clear_bit_unlock - Clear a bit in memory, for unlock
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This operation is atomic and provides release barrier semantics.
 */
static inline void clear_bit_unlock(
	unsigned long nr, volatile unsigned long *addr)
{
	clear_bit(nr, addr);
}

/**
 * __clear_bit_unlock - Clear a bit in memory, for unlock
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This operation is like clear_bit_unlock, however it is not atomic.
 * It does provide release barrier semantics so it can be used to unlock
 * a bit lock, however it would only be used if no other CPU can modify
 * any bits in the memory until the lock is released (a good example is
 * if the bit lock itself protects access to the other bits in the word).
 */
static inline void __clear_bit_unlock(
	unsigned long nr, volatile unsigned long *addr)
{
	clear_bit(nr, addr);
}

#undef LONG_MASK
#undef LONG_WORD

#include <asm-generic/bitops/non-atomic.h>
#include <asm-generic/bitops/le.h>
#include <asm-generic/bitops/ext2-atomic.h>

#else /* !CONFIG_RV_ATOMIC */

#include <asm-generic/bitops.h>

#endif /* CONFIG_RV_ATOMIC */

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_BITOPS_H */
