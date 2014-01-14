#ifndef _ASM_RISCV_PCR_H
#define _ASM_RISCV_PCR_H

#include <linux/const.h>

/* Status register flags */
#define SR_S    _AC(0x00000001,UL) /* Supervisor */
#define SR_PS   _AC(0x00000002,UL) /* Previous supervisor */
#define SR_EI   _AC(0x00000004,UL) /* Enable interrupts */
#define SR_PEI  _AC(0x00000008,UL) /* Previous EI */
#define SR_EF   _AC(0x00000010,UL) /* Enable floating-point */
#define SR_U64  _AC(0x00000020,UL) /* RV64 user mode */
#define SR_S64  _AC(0x00000040,UL) /* RV64 supervisor mode */
#define SR_VM   _AC(0x00000080,UL) /* Enable virtual memory */
#define SR_IM   _AC(0x00FF0000,UL) /* Interrupt mask */
#define SR_IP   _AC(0xFF000000,UL) /* Pending interrupts */

#define SR_IM_SHIFT     16
#define SR_IM_MASK(n)   ((_AC(1,UL)) << ((n) + SR_IM_SHIFT))

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_SYSCALL             6
#define EXC_LOAD_MISALIGNED     8
#define EXC_STORE_MISALIGNED    9
#define EXC_LOAD_ACCESS         10
#define EXC_STORE_ACCESS        11

#ifndef __ASSEMBLY__

#define read_csr(reg) ({ long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) \
  asm volatile ("csrw " #reg ", %0" :: "r"(val))

#define swap_csr(reg, val) ({ long __tmp; \
  asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "r"(val)); \
  __tmp; })

#define set_csr(reg, bit) ({ long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
    asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define clear_csr(reg, bit) ({ long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
    asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define rdcycle() ({ unsigned long __tmp; \
  asm volatile ("rdcycle %0" : "=r"(__tmp)); \
  __tmp; })

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PCR_H */
