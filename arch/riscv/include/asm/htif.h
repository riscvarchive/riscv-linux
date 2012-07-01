#ifndef _ASM_RISCV_HTIF_H
#define _ASM_RISCV_HTIF_H

extern void write_tohost(unsigned long);
extern unsigned long read_tohost(void);

extern void write_fromhost(unsigned long);
extern unsigned long read_fromhost(void);

#endif /* _ASM_RISCV_HTIF_H */
