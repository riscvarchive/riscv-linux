#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kdebug.h>
#include <linux/mm.h>
#include <linux/module.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/csr.h>

int show_unhandled_signals = 1;

extern asmlinkage void handle_exception(void);

static DEFINE_SPINLOCK(die_lock);

void die(struct pt_regs *regs, const char *str)
{
	static int die_counter;
	int ret;

	oops_enter();

	spin_lock_irq(&die_lock);
	console_verbose();
	bust_spinlocks(1);

	pr_emerg("%s [#%d]\n", str, ++die_counter);
	print_modules();
	show_regs(regs);

	ret = notify_die(DIE_OOPS, str, regs, 0, regs->cause, SIGSEGV);

	bust_spinlocks(0);
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	spin_unlock_irq(&die_lock);
	oops_exit();

	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");
	if (ret != NOTIFY_STOP)
		do_exit(SIGSEGV);
}

static inline void do_trap_siginfo(int signo, int code,
	unsigned long addr, struct task_struct *tsk)
{
	siginfo_t info;

	info.si_signo = signo;
	info.si_errno = 0;
	info.si_code = code;
	info.si_addr = (void __user *)addr;
	force_sig_info(signo, &info, tsk);
}

void do_trap(struct pt_regs *regs, int signo, int code,
	unsigned long addr, struct task_struct *tsk)
{
	if (show_unhandled_signals && unhandled_signal(tsk, signo)
	    && printk_ratelimit()) {
		pr_info("%s[%d]: unhandled signal %d code 0x%x at 0x" REG_FMT,
			tsk->comm, task_pid_nr(tsk), signo, code, addr);
		print_vma_addr(KERN_CONT " in ", GET_IP(regs));
		pr_cont("\n");
		show_regs(regs);
	}

	do_trap_siginfo(signo, code, addr, tsk);
}

static void do_trap_error(struct pt_regs *regs, int signo, int code,
	unsigned long addr, const char *str)
{
	if (user_mode(regs)) {
		do_trap(regs, signo, code, addr, current);
	} else {
		if (!fixup_exception(regs))
			die(regs, str);
	}
}

#define DO_ERROR_INFO(name, signo, code, addr, str)			\
asmlinkage void name(struct pt_regs *regs)				\
{									\
	do_trap_error(regs, signo, code, addr(regs), "Oops - " str);	\
}

#define GET_EPC(regs)		((regs)->epc)
#define GET_BADVADDR(regs)	((regs)->badvaddr)

DO_ERROR_INFO(do_trap_unknown,
	SIGILL, ILL_ILLTRP, GET_EPC, "unknown exception");
DO_ERROR_INFO(do_trap_insn_misaligned,
	SIGBUS, BUS_ADRALN, GET_EPC, "instruction address misaligned");
DO_ERROR_INFO(do_trap_insn_illegal,
	SIGILL, ILL_ILLOPC, GET_EPC, "illegal instruction");
DO_ERROR_INFO(do_trap_insn_privileged,
	SIGILL, ILL_PRVOPC, GET_EPC, "privileged instruction");
DO_ERROR_INFO(do_trap_load_misaligned,
	SIGBUS, BUS_ADRALN, GET_BADVADDR, "load address misaligned");
DO_ERROR_INFO(do_trap_store_misaligned,
	SIGBUS, BUS_ADRALN, GET_BADVADDR, "store address misaligned");

asmlinkage void do_trap_break(struct pt_regs *regs)
{
	do_trap_siginfo(SIGTRAP, TRAP_BRKPT, regs->epc, current);
	regs->epc += 0x4;
}

void __init trap_init(void)
{
	/* Clear the IPI exception that started the processor */
	csr_write(clear_ipi, 0);
	/* Set the exception vector address */
	csr_write(evec, &handle_exception);
}
