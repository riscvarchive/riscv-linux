#ifndef _ASM_RISCV_THREAD_INFO_H
#define _ASM_RISCV_THREAD_INFO_H

#ifdef __KERNEL__

#include <asm/page.h>
#include <linux/const.h>

/* thread information allocation */
#define THREAD_SIZE_ORDER 	(1)
#define THREAD_SIZE 		(PAGE_SIZE << THREAD_SIZE_ORDER)

#ifndef __ASSEMBLY__

#include <asm/processor.h>
#include <asm/csr.h>

typedef unsigned long mm_segment_t;

/*
 * low level task data that entry.S needs immediate access to
 * - this struct should fit entirely inside of one cache line
 * - this struct resides at the bottom of the supervisor stack
 * - if the members of this struct changes, the assembly constants
 *   in asm-offsets.c must be updated accordingly
 */
struct thread_info {
	struct task_struct	*task;		/* main task structure */
	unsigned long		flags;		/* low level flags */
	__u32			cpu;		/* current CPU */
	int                     preempt_count;  /* 0 => preemptable, <0 => BUG */
	mm_segment_t		addr_limit;
};

/*
 * macros/functions for gaining access to the thread information structure
 *
 * preempt_count needs to be 1 initially, until the scheduler is functional.
 */
#define INIT_THREAD_INFO(tsk)			\
{						\
	.task		= &tsk,			\
	.flags		= 0,			\
	.cpu		= 0,			\
	.preempt_count	= INIT_PREEMPT_COUNT,	\
	.addr_limit	= KERNEL_DS,		\
}

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)

/*
 * Pointer to the thread_info struct of the current process
 */
static inline struct thread_info *current_thread_info(void)
{
	register struct thread_info *tp __asm__ ("tp");
	return tp;
}

#endif /* !__ASSEMBLY__ */

/*
 * thread information flags
 * - these are process state flags that various assembly files may need to
 *   access
 * - pending work-to-be-done flags are in lowest half-word
 * - other flags in upper half-word(s)
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_RESTORE_SIGMASK	4	/* restore signal mask in do_signal() */
#define TIF_MEMDIE		5	/* is terminating due to OOM killer */
#define TIF_SYSCALL_TRACEPOINT  6       /* syscall tracepoint instrumentation */

#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)

#define _TIF_WORK_MASK \
	(_TIF_NOTIFY_RESUME | _TIF_SIGPENDING | _TIF_NEED_RESCHED)

#endif /* __KERNEL__ */

#endif /* _ASM_RISCV_THREAD_INFO_H */
