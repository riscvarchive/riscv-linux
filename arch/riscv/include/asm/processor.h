#ifndef _ASM_RISCV_PROCESSOR_H
#define _ASM_RISCV_PROCESSOR_H

#include <linux/const.h>

#include <asm/ptrace.h>

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	PAGE_ALIGN(TASK_SIZE >> 1)

#ifdef __KERNEL__
#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		STACK_TOP
#endif /* __KERNEL__ */

#ifndef __ASSEMBLY__

struct task_struct;
struct pt_regs;

/*
 * Default implementation of macro that returns current
 * instruction pointer ("program counter").
 */
#define current_text_addr()	({ __label__ _l; _l: &&_l;})

/* CPU-specific state of a task */
struct thread_struct {
	/* Callee-saved registers */
	unsigned long ra;
	unsigned long sp;	/* Kernel mode stack */
	unsigned long s[12];	/* s[0]: frame pointer */
	struct user_fpregs_struct fstate;
};

#define INIT_THREAD {					\
	.sp = sizeof(init_stack) + (long)&init_stack,	\
}

/* Return saved (kernel) PC of a blocked thread. */
#define thread_saved_pc(t)	((t)->thread.ra)
#define thread_saved_sp(t)	((t)->thread.sp)
#define thread_saved_fp(t)	((t)->thread.s[0])

#define task_pt_regs(tsk) \
	((struct pt_regs *)(task_stack_page(tsk) + THREAD_SIZE) - 1)

#define KSTK_EIP(tsk)		(task_pt_regs(tsk)->sepc)
#define KSTK_ESP(tsk)		(task_pt_regs(tsk)->sp)


/* Do necessary setup to start up a newly executed thread. */
extern void start_thread(struct pt_regs *regs,
			unsigned long pc, unsigned long sp);

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

/* Free current thread data structures, etc. */
static inline void exit_thread(void)
{
}

extern unsigned long get_wchan(struct task_struct *p);


static inline void cpu_relax(void)
{
#ifdef __riscv_muldiv
	int dummy;
	/* In lieu of a halt instruction, induce a long-latency stall. */
	__asm__ __volatile__ ("div %0, %0, zero" : "=r" (dummy));
#endif
	barrier();
}

static inline void wait_for_interrupt(void)
{
	__asm__ __volatile__ ("wfi");
}

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PROCESSOR_H */
