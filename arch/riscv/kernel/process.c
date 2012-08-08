#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tick.h>

#include <asm/unistd.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/pcr.h>
#include <asm/string.h>

#include <linux/elf.h>
#include <linux/auxvec.h>
/* We don't have Elf{32,64}_auxv_t, so make a fake one instead, that is,
 * a faux auxp struct. Hurr.
 */
struct fauxp {
	unsigned long type;
	unsigned long value;
};

extern asmlinkage void ret_from_fork(void);

void __noreturn cpu_idle(void)
{
	for (;;) {
		tick_nohz_idle_enter();
		rcu_idle_enter();
		while (!need_resched())
			cpu_relax();
		rcu_idle_exit();
		tick_nohz_idle_exit();
		schedule_preempt_disabled();
	}
}

void set_thread_pointer(struct fauxp *auxp, struct pt_regs *regs)
{
	Elf64_Phdr *phdr;
	int phnum;

	phdr = NULL;
	phnum = 0;
	for (; auxp->type != AT_NULL; auxp++) {
		switch (auxp->type) {
			case AT_PHDR:
				phdr = (Elf64_Phdr *)(auxp->value);
				break;
			case AT_PHNUM:
				phnum = (int)(auxp->value);
				break;
			default:
				break;
		}
		// printk("type %d, value 0x%0lx\n", (int)(auxp->type), auxp->value);
	}

	for (; phnum > 0; phnum--, phdr++) {
		if (phdr->p_type == PT_TLS) {
			regs->tp = phdr->p_paddr + phdr->p_memsz + 0x7000;
			printk("tp for process set to 0x%0lx\n", regs->tp);
		}
		// printk("phdr type %d, paddr 0x%lx\n", phdr->p_type, phdr->p_paddr);
	}
}

unsigned long get_wchan(struct task_struct *task)
{
  unsigned long pc;

  if (!task || task == current || task->state == TASK_RUNNING)
    return 0;
  if (!task_stack_page(task))
    return 0;

  pc = task->thread.pc;

  return pc;
}

void start_thread(struct pt_regs *regs, unsigned long pc, 
	unsigned long sp)
{
	unsigned long argc;
	unsigned long *envp;
	unsigned long *auxp;

	/* Remove supervisor privileges */
	regs->status = mfpcr(PCR_STATUS) & (~SR_PS);
	/* Compensate for advancing of EPC in syscall handler */
	regs->epc = pc - 0x4;
	regs->sp = sp;
	
	argc = *(unsigned long *)sp;
	envp = (unsigned long *)sp + (argc + 2);
	for (auxp = envp; *auxp != NULL; auxp++);
	set_thread_pointer((struct fauxp *)(auxp + 1), regs);
}

void exit_thread(void)
{
}

void flush_thread(void)
{
}

int copy_thread(unsigned long clone_flags, unsigned long usp,
	unsigned long unused,
	struct task_struct *p, struct pt_regs *regs)
{
	struct pt_regs *childregs;
	unsigned long childksp;

	childksp = (unsigned long)task_stack_page(p) + THREAD_SIZE;
	childregs = (struct pt_regs *)childksp - 1;

	memcpy(childregs, regs, sizeof(struct pt_regs));

	childregs->sp = (user_mode(regs) ? usp : (unsigned long)childregs);
	childregs->v[0] = 0; /* Set return value for child: v0 */
	childregs->tp = (unsigned long)task_stack_page(p);

	p->thread.status = regs->status & (SR_IM | SR_VM | SR_S64 | SR_U64 | SR_PS | SR_S);
	p->thread.sp = (unsigned long)childregs; /* ksp */
	p->thread.pc = (unsigned long)ret_from_fork; /* pc */
	p->thread.tp = (unsigned long)task_stack_page(p);

	return 0;
}

static void __noreturn kernel_thread_helper(void *arg, int(*fn)(void *))
{
	do_exit(fn(arg));
}

long kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct pt_regs kregs;

	memset(&kregs, 0, sizeof(kregs));

	kregs.a[0] = (unsigned long)arg;
	kregs.a[1] = (unsigned long)fn;
	kregs.ra = (unsigned long)kernel_thread_helper;
	kregs.status = (SR_IM | SR_VM | SR_S64 | SR_U64 | SR_S | SR_PS | SR_EF);

	return do_fork(flags | CLONE_VM | CLONE_UNTRACED,
		0, &kregs, 0, NULL, NULL);
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return tsk->thread.pc;
}

/*
 * Invoke the system call from the kernel instead of calling
 * sys_execve() directly so that pt_regs is correctly populated.
 */
int kernel_execve(const char *filename,
	char *const argv[], char *const envp[])
{
	register unsigned long __a0 __asm__ ("a0");
	register unsigned long __a1 __asm__ ("a1");
	register unsigned long __a2 __asm__ ("a2");
	register int __res __asm__ ("v0");

	__res = __NR_execve;
	__a0 = (unsigned long)(filename);
	__a1 = (unsigned long)(argv);
	__a2 = (unsigned long)(envp);

	__asm__ __volatile__ (
		"syscall;"
		: "+r" (__res)
		: "r" (__a0), "r" (__a1), "r" (__a2)
		: "memory");
	return __res;
}
