#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/kdebug.h>
#include <linux/init.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/pcr.h>

extern asmlinkage void handle_exception(void);

static void show_backtrace(struct task_struct *task, const struct pt_regs *regs)
{
#ifdef CONFIG_FRAME_POINTER
	unsigned long sp, fp, pc;

	printk("Call Trace:\n");
	if (regs) {
		sp = GET_USP(regs);
		fp = GET_FP(regs);
		pc = GET_IP(regs);
	} else if (!task || task == current) {
		const register unsigned long current_sp __asm__ ("sp");
		const register unsigned long current_fp __asm__ ("s0");
		sp = current_sp;
		fp = current_fp;
		pc = (unsigned long)(&show_backtrace);
	} else {
		sp = task->thread.sp;
		fp = task->thread.fp;
		pc = task->thread.pc;
	}

	for (;;) {
		if (!kernel_text_address(pc))
			return;
		printk(" [<%08lx>] %pS\n", pc, (void *)pc);
		if (fp < sp + 0x10 || fp >= ALIGN(sp, THREAD_SIZE))
			return;
		/* Unwind stack frame */
		sp = fp;
		pc = *(unsigned long *)(fp - 0x8) - 0x4;
		fp = *(unsigned long *)(fp - 0x10);
	}
#endif /* CONFIG_FRAME_POINTER */
}

void show_stack(struct task_struct *task, unsigned long *sp)
{
	show_backtrace(task, NULL);
}

void dump_stack(void)
{
	struct task_struct *task;
	task = current;
	printk("pid: %d, comm: %.20s %s\n",
		task->pid, task->comm, print_tainted());
	show_backtrace(NULL, NULL);
}
EXPORT_SYMBOL(dump_stack);

void show_regs(struct pt_regs *regs)
{
}

extern void *sys_call_table[__NR_syscalls];


void die(const char *str, struct pt_regs *regs, long err)
{
	int ret;

	oops_enter();
	console_verbose();
	ret = notify_die(DIE_OOPS, str, regs, err, 0, SIGSEGV);
	oops_exit();

	if (panic_on_oops)
		panic("Fatal exception");
	if (ret != NOTIFY_STOP)
		do_exit(SIGSEGV);
}


asmlinkage void handle_fault_unknown(struct pt_regs *regs)
{
	siginfo_t info;

	if (user_mode(regs)) {
		/* Send a SIGILL */
		info.si_signo = SIGILL;
		info.si_errno = 0;
		info.si_code = ILL_ILLTRP;
		info.si_addr = (void *)(regs->epc);
		force_sig_info(SIGILL, &info, current);
	} else { /* Kernel mode */
		panic("unknown exception %ld: epc=0x%016lx badvaddr=0x%016lx",
			regs->cause, regs->epc, regs->badvaddr);
	}
}

static void __handle_misaligned_addr(struct pt_regs *regs,
	unsigned long addr)
{
	siginfo_t info;

	if (user_mode(regs)) {
		/* Send a SIGBUS */
		info.si_signo = SIGBUS;
		info.si_errno = 0;
		info.si_code = BUS_ADRALN;
		info.si_addr = (void *)addr;
		force_sig_info(SIGBUS, &info, current);
	} else { /* Kernel mode */
		die("SIGBUS", regs, 0);
	}
}

asmlinkage void handle_misaligned_insn(struct pt_regs *regs)
{
	__handle_misaligned_addr(regs, regs->epc);
}

asmlinkage void handle_misaligned_data(struct pt_regs *regs)
{
	__handle_misaligned_addr(regs, regs->badvaddr);
}

asmlinkage void handle_break(struct pt_regs *regs)
{
	siginfo_t info;

	info.si_signo = SIGTRAP;
	info.si_errno = 0;
	info.si_code = TRAP_BRKPT;
	info.si_addr = (void *)(regs->epc);
	force_sig_info(SIGTRAP, &info, current);

	regs->epc += 0x4;
}

asmlinkage void handle_privileged_insn(struct pt_regs *regs)
{
	siginfo_t info;

	if (user_mode(regs)) {
		/* Send a SIGILL */
		info.si_signo = SIGILL;
		info.si_errno = 0;
		info.si_code = ILL_PRVOPC;
		info.si_addr = (void *)(regs->epc);
		force_sig_info(SIGILL, &info, current);
	} else { /* Kernel mode */
		die("SIGILL", regs, 0);
	}

}

asmlinkage void handle_illegal_insn(struct pt_regs *regs)
{
	siginfo_t info;

	if (user_mode(regs)) {
		/* Send a SIGILL */
		info.si_signo = SIGILL;
		info.si_errno = 0;
		info.si_code = ILL_ILLOPC;
		info.si_addr = (void *)(regs->epc);
		force_sig_info(SIGILL, &info, current);
	} else { /* Kernel mode */
		die("SIGILL", regs, 0);
	}
}

void __init trap_init(void)
{
	/* Clear the IPI exception that started the processor */
	mtpcr(PCR_CLEAR_IPI, 0);
	/* Set the exception vector address */
	mtpcr(PCR_EVEC, &handle_exception);
}
