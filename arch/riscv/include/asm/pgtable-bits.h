#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

/*
 * RV32Sv32 page table entry:
 * | 31 10 | 9             7 | 6 | 5 | 4  1 | 0
 *    PFN    reserved for SW   D   R   TYPE   V
 *
 * RV64Sv39 / RV64Sv48 page table entry:
 * | 63           48 | 47 10 | 9             7 | 6 | 5 | 4  1 | 0
 *   reserved for HW    PFN    reserved for SW   D   R   TYPE   V
 */

#define _PAGE_PRESENT   (1 << 0)
#define _PAGE_TYPE      (0xF << 1)  /* Page type */
#define _PAGE_WRITE     (1 << 1)    /* Writable (subfield of TYPE field) */
#define _PAGE_ACCESSED  (1 << 5)    /* Set by hardware on any access */
#define _PAGE_DIRTY     (1 << 6)    /* Set by hardware on any write */
#define _PAGE_SOFT      (1 << 7)    /* Reserved for software */

#define _PAGE_TYPE_TABLE    (0x00)  /* Page table */
#define _PAGE_TYPE_TABLE_G  (0x02)  /* Page table, global mapping */
#define _PAGE_TYPE_USER_RO  (0x08)  /* User read-only, Kernel read-only */
#define _PAGE_TYPE_USER_RW  (0x0A)  /* User read-write, Kernel read-write */
#define _PAGE_TYPE_USER_RX  (0x04)  /* User read-execute, Kernel read-only */
#define _PAGE_TYPE_USER_RWX (0x06)  /* User RWX, Kernel read-write */
#define _PAGE_TYPE_KERN_RW  (0x1A)  /* Kernel read-write */

#define _PAGE_SPECIAL   _PAGE_SOFT
#define _PAGE_TABLE     (_PAGE_PRESENT | _PAGE_TYPE_TABLE)

#define _PAGE_PFN_SHIFT 10

/* Set of bits to preserve across pte_modify() */
#define _PAGE_CHG_MASK  (~(_PAGE_PRESENT | _PAGE_TYPE))

/* Advertise support for _PAGE_SPECIAL */
#define __HAVE_ARCH_PTE_SPECIAL

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
