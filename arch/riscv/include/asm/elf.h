#ifndef _ASM_RISCV_ELF_H
#define _ASM_RISCV_ELF_H

#include <asm/ptrace.h>

/* ELF register definitions */
typedef unsigned long elf_greg_t;

#define ELF_NGREG (sizeof(struct pt_regs) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

#define ELF_NFPREG	0
typedef double elf_fpreg_t;
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];


/* RISC-V relocation types */
#define R_RISCV_NONE		0
#define R_RISCV_32		2
#define R_RISCV_REL32		3
#define R_RISCV_JAL		4
#define R_RISCV_HI20		5
#define R_RISCV_LO12_I		6
#define R_RISCV_LO12_S		7
#define R_RISCV_PCREL_LO12_I	8
#define R_RISCV_PCREL_LO12_S	9
#define R_RISCV_BRANCH		10
#define R_RISCV_CALL		11
#define R_RISCV_PCREL_HI20	12
#define R_RISCV_CALL_PLT	13
#define R_RISCV_64		18
#define R_RISCV_GOT_HI20	22
#define R_RISCV_GOT_LO12	23
#define R_RISCV_COPY		24
#define R_RISCV_JUMP_SLOT	25
/* TLS relocations */
#define R_RISCV_TPREL_HI20	30
#define R_RISCV_TPREL_LO12_I	31
#define R_RISCV_TPREL_LO12_S	32
#define R_RISCV_TLS_DTPMOD32	38
#define R_RISCV_TLS_DTPREL32	39
#define R_RISCV_TLS_DTPMOD64	40
#define R_RISCV_TLS_DTPREL64	41
#define R_RISCV_TLS_GD		42
#define R_RISCV_TLS_DTPREL_HI16	44
#define R_RISCV_TLS_DTPREL_LO16	45
#define R_RISCV_TLS_GOTTPREL	46
#define R_RISCV_TLS_TPREL32	47
#define R_RISCV_TLS_TPREL64	48
#define R_RISCV_TLS_GOT_HI20	51
#define R_RISCV_TLS_GOT_LO12	52
#define R_RISCV_TLS_GD_HI20	53
#define R_RISCV_TLS_GD_LO12	54
#define R_RISCV_GLOB_DAT	57
#define R_RISCV_ADD32		58
#define R_RISCV_ADD64		59
#define R_RISCV_SUB32		60
#define R_RISCV_SUB64		61

/* TODO: Move definition into include/uapi/linux/elf-em.h */
#define EM_RISCV	0xF3

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_ARCH	EM_RISCV
#if defined(CONFIG_64BIT)
#define ELF_CLASS	ELFCLASS64
#else
#define ELF_CLASS	ELFCLASS32
#endif
#define ELF_DATA	ELFDATA2MSB

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == EM_RISCV)

#define CORE_DUMP_USE_REGSET
#define ELF_EXEC_PAGESIZE	(PAGE_SIZE)

/*
 * This is the location that an ET_DYN program is loaded if exec'ed.  Typical
 * use of this is to invoke "./ld.so someprog" to test out a new version of
 * the loader.  We need to make sure that it is out of the way of the program
 * that it will "exec", and that there is sufficient room for the brk.
 */
#define ELF_ET_DYN_BASE		((TASK_SIZE / 3) * 2)

/*
 * This yields a mask that user programs can use to figure out what
 * instruction set this CPU supports.  This could be done in user space,
 * but it's not easy, and we've already done it here.
 */
#define ELF_HWCAP	(0)

/*
 * This yields a string that ld.so will use to load implementation
 * specific libraries for optimization.  This is more specific in
 * intent than poking at uname or /proc/cpuinfo.
 */
#define ELF_PLATFORM	(NULL)

#define AT_SYSINFO_EHDR 33
#define ARCH_DLINFO						\
do {								\
	NEW_AUX_ENT(AT_SYSINFO_EHDR,				\
		(elf_addr_t)current->mm->context.vdso);		\
} while (0)


#define ARCH_HAS_SETUP_ADDITIONAL_PAGES
struct linux_binprm;
extern int arch_setup_additional_pages(struct linux_binprm *bprm,
	int uses_interp);


#endif /* _ASM_RISCV_ELF_H */
