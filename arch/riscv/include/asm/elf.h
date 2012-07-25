/*
 * OpenRISC Linux
 *
 * Linux architectural port borrowing liberally from similar works of
 * others.  All original copyrights apply as per the original source
 * declaration.
 *
 * OpenRISC implementation:
 * Copyright (C) 2003 Matjaz Breskvar <phoenix@bsemi.com>
 * Copyright (C) 2010-2011 Jonas Bonn <jonas@southpole.se>
 * et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _ASM_RISCV_ELF_H
#define _ASM_RISCV_ELF_H

#include <linux/types.h>
#include <linux/ptrace.h>

/* RISC-V relocation types */
#define R_RISCV_NONE	0
#define R_RISCV_32	2
#define R_RISCV_REL32	3
#define R_RISCV_26	4
#define R_RISCV_HI16	5
#define R_RISCV_LO16	6
#define R_RISCV_GPREL16	7
#define R_RISCV_LITERAL	8
#define R_RISCV_GOT16	9
#define R_RISCV_PC16	10
#define R_RISCV_CALL16	11
#define R_RISCV_GPREL32	12
/* The remaining relocs are defined on Irix, although they are not
   in the RISC-V ELF ABI.  */
#define R_RISCV_UNUSED1	13
#define R_RISCV_UNUSED2	14
#define R_RISCV_UNUSED3	15

#define R_RISCV_64		18
#define R_RISCV_GOT_DISP	19
#define R_RISCV_GOT_HI16	22
#define R_RISCV_GOT_LO16	23
#define R_RISCV_SUB		24
#define R_RISCV_INSERT_A	25
#define R_RISCV_INSERT_B	26
#define R_RISCV_DELETE		27
#define R_RISCV_CALL_HI16	30
#define R_RISCV_CALL_LO16	31
#define R_RISCV_SCN_DISP	32
#define R_RISCV_REL16		33
#define R_RISCV_ADD_IMMEDIATE	34
#define R_RISCV_PJUMP		35
#define R_RISCV_RELGOT		36
#define R_RISCV_JALR		37
/* TLS relocations.  */
#define R_RISCV_TLS_DTPMOD32	38
#define R_RISCV_TLS_DTPREL32	39
#define R_RISCV_TLS_DTPMOD64	40
#define R_RISCV_TLS_DTPREL64	41
#define R_RISCV_TLS_GD		42
#define R_RISCV_TLS_LDM		43
#define R_RISCV_TLS_DTPREL_HI16	44
#define R_RISCV_TLS_DTPREL_LO16	45
#define R_RISCV_TLS_GOTTPREL	46
#define R_RISCV_TLS_TPREL32	47
#define R_RISCV_TLS_TPREL64	48
#define R_RISCV_TLS_TPREL_HI16	49
#define R_RISCV_TLS_TPREL_LO16	50
#define R_RISCV_TLS_GOT_HI16	51
#define R_RISCV_TLS_GOT_LO16 	52
#define R_RISCV_TLS_GD_HI16	53
#define R_RISCV_TLS_GD_LO16	54
#define R_RISCV_TLS_LDM_HI16	55
#define R_RISCV_TLS_LDM_LO16	56
#define R_RISCV_GLOB_DAT	57
/* These relocations are specific to VxWorks.  */
#define R_RISCV_COPY		126
#define R_RISCV_JUMP_SLOT	127

typedef unsigned long elf_greg_t;

/*
 * Note that NGREG is defined to ELF_NGREG in include/linux/elfcore.h, and is
 * thus exposed to user-space.
 */
#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

/* A placeholder; OR32 does not have fp support yes, so no fp regs for now.  */
typedef unsigned long elf_fpregset_t;

/* This should be moved to include/linux/elf.h */
#define EM_RISCV	0xF3

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_ARCH	EM_RISCV
#define ELF_CLASS	ELFCLASS64
#define ELF_DATA	ELFDATA2MSB

#ifdef __KERNEL__

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */

#define elf_check_arch(x) ((x)->e_machine == EM_RISCV)

/* This is the location that an ET_DYN program is loaded if exec'ed.  Typical
   use of this is to invoke "./ld.so someprog" to test out a new version of
   the loader.  We need to make sure that it is out of the way of the program
   that it will "exec", and that there is sufficient room for the brk.  */

#define ELF_ET_DYN_BASE         (0x08000000)

/*
 * Enable dump using regset.
 * This covers all of general/DSP/FPU regs.
 */
#define CORE_DUMP_USE_REGSET

#define ELF_EXEC_PAGESIZE	8192

extern void dump_elf_thread(elf_greg_t *dest, struct pt_regs *pt);
#define ELF_CORE_COPY_REGS(dest, regs) dump_elf_thread(dest, regs);

/* This yields a mask that user programs can use to figure out what
   instruction set this cpu supports.  This could be done in userspace,
   but it's not easy, and we've already done it here.  */

#define ELF_HWCAP	(0)

/* This yields a string that ld.so will use to load implementation
   specific libraries for optimization.  This is more specific in
   intent than poking at uname or /proc/cpuinfo.

   For the moment, we have only optimizations for the Intel generations,
   but that could change... */

#define ELF_PLATFORM	(NULL)

#define SET_PERSONALITY(ex) set_personality(PER_LINUX)

#endif /* __KERNEL__ */
#endif /* _ASM_RISCV_ELF_H */
