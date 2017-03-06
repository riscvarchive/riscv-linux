#ifndef _ASM_RISCV_BUG_H
#define _ASM_RISCV_BUG_H

#include <linux/compiler.h>
#include <linux/const.h>
#include <linux/types.h>

#include <asm/asm.h>

#ifdef CONFIG_GENERIC_BUG
#define __BUG_INSN	_AC(0x00100073,UL) /* sbreak */

#ifndef __ASSEMBLY__
typedef u32 bug_insn_t;

#ifdef CONFIG_GENERIC_BUG_RELATIVE_POINTERS
#define __BUG_ENTRY_ADDR	INT " 1b - 2b"
#define __BUG_ENTRY_FILE	INT " %0 - 2b"
#else
#define __BUG_ENTRY_ADDR	PTR " 1b"
#define __BUG_ENTRY_FILE	PTR " %0"
#endif

#ifdef CONFIG_DEBUG_BUGVERBOSE
#define __BUG_ENTRY			\
	__BUG_ENTRY_ADDR "\n\t"		\
	__BUG_ENTRY_FILE "\n\t"		\
	SHORT " %1"
#else
#define __BUG_ENTRY			\
	__BUG_ENTRY_ADDR
#endif

#define BUG()							\
do {								\
	__asm__ __volatile__ (					\
		"1:\n\t"					\
			"sbreak\n"				\
			".pushsection __bug_table,\"a\"\n\t"	\
		"2:\n\t"					\
			__BUG_ENTRY "\n\t"			\
			".org 2b + %2\n\t"			\
			".popsection"				\
		:						\
		: "i" (__FILE__), "i" (__LINE__),		\
		  "i" (sizeof(struct bug_entry)));		\
	unreachable();						\
} while (0)

#define HAVE_ARCH_BUG
#endif /* !__ASSEMBLY__ */
#endif /* CONFIG_GENERIC_BUG */

#include <asm-generic/bug.h>

#ifndef __ASSEMBLY__

struct pt_regs;
struct task_struct;

extern void die(struct pt_regs *regs, const char *str);
extern void do_trap(struct pt_regs *regs, int signo, int code,
	unsigned long addr, struct task_struct *tsk);

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_RISCV_BUG_H */
