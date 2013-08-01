#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

#define PTE_PFN_SHIFT   (PAGE_SHIFT)
/* Extracts the PFN from a pgd/pud/pmd/pte */
#define PTE_PFN_MASK    (PAGE_MASK)
/*
 * RV32Sv32 page table entry:
 * | 31 22  | 21  12 | 11  9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[1]   PPN[0]     --    SX  SW  SR  UX  UW  UR   G   T   V
 *
 * RV64Sv43 page table entry:
 * | 63  33 | 32  23 | 22  13 | 12  9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[2]   PPN[1]   PPN[0]     --   SX  SW  SR  UX  UW  UR   G   T   V
 */

#define _PAGE_V         (1 << 0) /* Valid */
#define _PAGE_T         (1 << 1) /* Page table descriptor */
#define _PAGE_G         (1 << 2) /* Global */

#define _PAGE_SR        (1 << 6) /* Supervisor read */
#define _PAGE_SW        (1 << 7) /* Supervisor write */
#define _PAGE_SX        (1 << 8) /* Supervisor execute */

#define _PAGE_UR        (1 << 3) /* User read */
#define _PAGE_UW        (1 << 4) /* User write */
#define _PAGE_UX        (1 << 5) /* User execute */

#define _PAGE_SOFT1     (1 <<  9) /* Reserved for software */
#define _PAGE_SOFT2     (1 << 10) /* Reserved for software */
#define _PAGE_SOFT3     (1 << 11) /* Reserved for software */
#define _PAGE_SOFT4     (1 << 12) /* Reserved for software */

#define _PAGE_D         _PAGE_SOFT1 /* Dirty */
#define _PAGE_R         _PAGE_SOFT2 /* Referenced */
#define _PAGE_PRESENT   _PAGE_V
#define _PAGE_FILE      _PAGE_D /* when !present: non-linear file mapping */

/* Set of bits to retain in pte_modify() */
#define _PAGE_CHG_MASK  (~(_PAGE_SR | _PAGE_SW | _PAGE_SX | \
                           _PAGE_UR | _PAGE_UW | _PAGE_UX))

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
