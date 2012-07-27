#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

#define PTE_PFN_SHIFT   (PAGE_SHIFT)
/* Extracts the PFN from a pgd/pud/pmd/pte */
#define PTE_PFN_MASK    (PAGE_MASK)
/*
 * RV32S page table entry:
 * | 32  23 | 22  13 | 12  10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[1]   PPN[0]     --    SR  SW  SR  UR  UW  UE   D   R   E   T
 *
 * RV64S page table entry:
 * | 63  33 | 32  23 | 22  13 | 12  10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   PPN[2]   PPN[1]   PPN[0]     --    SR  SW  SR  UR  UW  UE   D   R   E   T
 */

#define _PAGE_SR        (1 << 9) /* Supervisor read */
#define _PAGE_SW        (1 << 8) /* Supervisor write */
#define _PAGE_SE        (1 << 7) /* Supervisor execute */

#define _PAGE_UR        (1 << 6) /* User read */
#define _PAGE_UW        (1 << 5) /* User write */
#define _PAGE_UE        (1 << 4) /* User execute */

#define _PAGE_D         (1 << 3) /* Dirty */
#define _PAGE_R         (1 << 2) /* Referenced */
#define _PAGE_E         (1 << 1) /* Page table entry */
#define _PAGE_T         (1 << 0) /* Page table descriptor */

#define _PAGE_UNUSED1   (1 << 10)
#define _PAGE_UNUSED2   (1 << 11)
#define _PAGE_UNUSED3   (1 << 12)

#define _PAGE_PRESENT   _PAGE_UNUSED1
#define _PAGE_FILE      _PAGE_D /* when !present: non-linear file mapping */

/* Set of bits to retain in pte_modify() */
#define _PAGE_CHG_MASK  (~(_PAGE_SR | _PAGE_SW | _PAGE_SE | \
                           _PAGE_UR | _PAGE_UW | _PAGE_UE))

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
