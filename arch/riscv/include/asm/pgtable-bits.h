#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

#define _PAGE_SPECIAL (1 << 11) /* Not in ISA */
#define _PAGE_PRESENT (1 << 10) /* Not in ISA */

#define _SUPERVISOR_READABLE   (1 << 9)
#define _SUPERVISOR_WRITABLE   (1 << 8)
#define _SUPERVISOR_EXECUTABLE (1 << 7)

#define _USER_READABLE   (1 << 6)
#define _USER_WRITABLE   (1 << 5)
#define _USER_EXECUTABLE (1 << 4)

#define _PAGE_DIRTY      (1 << 3)
#define _PAGE_REFERENCED (1 << 2) /* For LRU, etc., also young/old */
#define _PAGE_ENTRY      (1 << 1) /* Leaf */
#define _PAGE_TABLE_DESC (1 << 0) /* Descriptor */

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
