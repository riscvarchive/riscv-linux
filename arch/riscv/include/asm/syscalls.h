#ifndef _ASM_RISCV_SYSCALLS_H
#define _ASM_RISCV_SYSCALLS_H

#include <linux/linkage.h>

#include <asm-generic/syscalls.h>

/* kernel/sys_riscv.c */
asmlinkage long sys_sysriscv(unsigned long, unsigned long,
	unsigned long, unsigned long);

#endif /* _ASM_RISCV_SYSCALLS_H */
