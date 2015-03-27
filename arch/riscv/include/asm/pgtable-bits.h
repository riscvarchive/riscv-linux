#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

/*
 * RV32Sv32 page table entry:
 * | 31 22  | 21  12 | 11  9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[1]   PPN[0]     --    SX  SW  SR  UX  UW  UR   G   T   V
 *
 * RV64Sv43 page table entry:
 * | 63  33 | 32  23 | 22  13 | 12  9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[2]   PPN[1]   PPN[0]     --   SX  SW  SR  UX  UW  UR   G   T   V
 */

#define _PAGE_TYPE      (7 << 0)
#define _PAGE_USER      (1 << 2) /* Readable by both user and kernel */

/* The permission bits apply to _PAGE_USER pages.  _PAGE_U[WX] additionally
   apply to _PAGE_TYPE_USER_ONLY pages.  For _PAGE_TYPE_KERNEL_ONLY pages,
   use _PAGE_UW instead of _PAGE_SW and _PAGE_UX instead of _PAGE_SX. */

#define _PAGE_SW        (1 << 0) /* Supervisor write */
#define _PAGE_SX        (1 << 1) /* Supervisor execute */
#define _PAGE_UW        (1 << 3) /* User write */
#define _PAGE_UX        (1 << 4) /* User execute */

#define _PAGE_G         (1 << 5) /* Global */
#define _PAGE_ACCESSED  (1 << 6) /* Set by hardware on any access */
#define _PAGE_DIRTY     (1 << 7) /* Set by hardware on any write */
#define _PAGE_SOFT      (1 << 8) /* Reserved for software */

#define _PAGE_PFN_SHIFT 10

#define _PAGE_PRESENT   _PAGE_TYPE
#define _PAGE_SPECIAL   _PAGE_SOFT
#define _PAGE_FILE      _PAGE_UW /* when !present: non-linear file mapping */

/* Values for _PAGE_TYPE field.  _PAGE_USER subsumes types 4-7. */
#define _PAGE_TYPE_INVALID     0
#define _PAGE_TYPE_TABLE       1
#define _PAGE_TYPE_USER_ONLY   2
#define _PAGE_TYPE_KERNEL_ONLY 3

/* Set of bits to preserve across pte_modify() */
#define _PAGE_CHG_MASK  (~(_PAGE_TYPE | _PAGE_UW | _PAGE_UX))

/* Advertise support for _PAGE_SPECIAL */
#define __HAVE_ARCH_PTE_SPECIAL

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
