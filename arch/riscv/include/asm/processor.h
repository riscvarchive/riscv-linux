#ifndef _ASM_RISCV_PROCESSOR_H
#define _ASM_RISCV_PROCESSOR_H

#ifndef __ASSEMBLY__

#include <linux/threads.h>
#include <asm/segment.h>

struct task_struct;
struct pt_regs;

/*
 * System setup and hardware flags.
 */
extern void (*cpu_wait)(void);

extern long kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);
extern unsigned long thread_saved_pc(struct task_struct *tsk);
extern void start_thread(struct pt_regs *regs,
			unsigned long pc, unsigned long sp);
extern unsigned long get_wchan(struct task_struct *p);

/*
 * Return current * instruction pointer ("program counter").
 */
#define current_text_addr() ({ __label__ _l; _l: &&_l; })

#define cpu_relax()		barrier()
#define release_thread(thread)	do {} while (0)
#define prepare_to_copy(tsk)	do {} while (0)

/*
 * User space process size: 2GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.
 */
/* Highest virtual address below the sign-extension hole */
#define TASK_SIZE	(1UL << (PGDIR_SHIFT + 9))

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	0x40000000

#ifdef __KERNEL__
#define STACK_TOP	TASK_SIZE
#define STACK_TOP_MAX	TASK_SIZE
#endif

/* CPU-specific state of a task */
struct thread_struct {
	unsigned long sp;	/* Kernel mode stack */
#ifdef CONFIG_FRAME_POINTER
	unsigned long fp;
#endif /* CONFIG_FRAME_POINTER */
	unsigned long pc;
	unsigned long status;
};

#define INIT_THREAD {		\
	.sp = sizeof(init_stack) + (long)&init_stack, \
}

#define kstk_tos(tsk) \
	((unsigned long)task_stack_page(tsk) + THREAD_SIZE)
#define task_pt_regs(tsk)	((struct pt_regs *)kstk_tos(tsk) - 1)

#define KSTK_EIP(tsk)		(task_pt_regs(tsk)->epc)
#define KSTK_ESP(tsk)		(task_pt_regs(tsk)->sp)

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PROCESSOR_H */
