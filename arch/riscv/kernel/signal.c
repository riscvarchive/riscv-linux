#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/tracehook.h>
#include <linux/linkage.h>

#include <asm/ucontext.h>
#include <asm/vdso.h>
#include <asm/csr.h>

#define DEBUG_SIG 0

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
};

static int restore_sigcontext(struct pt_regs *regs,
	struct sigcontext __user *sc)
{
	int err = 0;
	unsigned int i;

	err |= __get_user(regs->epc, &sc->epc);
	err |= __get_user(regs->ra, &sc->ra);
	for (i = 0; i < (sizeof(regs->s) / sizeof(unsigned long)); i++) {
		err |= __get_user(regs->s[i], &sc->s[i]);
	}
	err |= __get_user(regs->sp, &sc->sp);
	err |= __get_user(regs->tp, &sc->tp);
	for (i = 0; i < (sizeof(regs->v) / sizeof(unsigned long)); i++) {
		err |= __get_user(regs->v[i], &sc->v[i]);
	}
	for (i = 0; i < (sizeof(regs->a) / sizeof(unsigned long)); i++) {
		err |= __get_user(regs->a[i], &sc->a[i]);
	}
	for (i = 0; i < (sizeof(regs->t) / sizeof(unsigned long)); i++) {
		err |= __get_user(regs->t[i], &sc->t[i]);
	}
	err |= __get_user(regs->gp, &sc->gp);

	/* Disable sys_rt_sigreturn() restarting */
	regs->syscallno = ~0UL;
	return err;
}

SYSCALL_DEFINE0(rt_sigreturn)
{
	struct pt_regs *regs = current_pt_regs();
	struct rt_sigframe __user *frame;
	struct task_struct *task;
	sigset_t set;

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->restart_block.fn = do_no_restart_syscall;

	frame = (struct rt_sigframe __user *)regs->sp;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;

	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->uc.uc_mcontext))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return regs->v[0];

badframe:
	task = current;
	if (show_unhandled_signals) {
		pr_info_ratelimited("%s[%d]: bad frame in %s: "
			"frame=%p pc=%p sp=%p\n",
			task->comm, task_pid_nr(task), __func__,
			frame, (void *)regs->epc, (void *)regs->sp);
	}
	force_sig(SIGSEGV, task);
	return 0;
}

static int setup_sigcontext(struct sigcontext __user *sc,
	struct pt_regs *regs)
{
	int err = 0;
	unsigned int i;

	err |= __put_user(regs->epc, &sc->epc);
	err |= __put_user(regs->ra, &sc->ra);
	for (i = 0; i < (sizeof(regs->s) / sizeof(unsigned long)); i++) {
		err |= __put_user(regs->s[i], &sc->s[i]);
	}
	err |= __put_user(regs->sp, &sc->sp);
	err |= __put_user(regs->tp, &sc->tp);
	for (i = 0; i < (sizeof(regs->v) / sizeof(unsigned long)); i++) {
		err |= __put_user(regs->v[i], &sc->v[i]);
	}
	for (i = 0; i < (sizeof(regs->a) / sizeof(unsigned long)); i++) {
		err |= __put_user(regs->a[i], &sc->a[i]);
	}
	for (i = 0; i < (sizeof(regs->t) / sizeof(unsigned long)); i++) {
		err |= __put_user(regs->t[i], &sc->t[i]);
	}
	err |= __put_user(regs->gp, &sc->gp);

	return err;
}

static inline void __user *get_sigframe(struct ksignal *ksig,
	struct pt_regs *regs, size_t framesize)
{
	unsigned long sp;
	/* Default to using normal stack */
	sp = regs->sp;

	/*
	 * If we are on the alternate signal stack and would overflow it, don't.
	 * Return an always-bogus address instead so we will die with SIGSEGV.
	 */
	if (on_sig_stack(sp) && !likely(on_sig_stack(sp - framesize)))
		return (void __user __force *)(-1UL);

	/* This is the X/Open sanctioned signal stack switching. */
	sp = sigsp(sp, ksig) - framesize;

	/* Align the stack frame. */
	sp &= ~0xfUL;

	return (void __user *)sp;
}


static int setup_rt_frame(struct ksignal *ksig, sigset_t *set,
	struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	int err = 0;

	frame = get_sigframe(ksig, regs, sizeof(*frame));
	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		return -EFAULT;

	err |= copy_siginfo_to_user(&frame->info, &ksig->info);

	/* Create the ucontext. */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(NULL, &frame->uc.uc_link);
	err |= __save_altstack(&frame->uc.uc_stack, regs->sp);
	err |= setup_sigcontext(&frame->uc.uc_mcontext, regs);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		return -EFAULT;

	/* Set up to return from userspace. */
	regs->ra = (unsigned long)VDSO_SYMBOL(
		current->mm->context.vdso, rt_sigreturn);

	/*
	 * Set up registers for signal handler.
	 * Registers that we don't modify keep the value they had from
	 * user-space at the time we took the signal.
	 * We always pass siginfo and mcontext, regardless of SA_SIGINFO,
	 * since some things rely on this (e.g. glibc's debug/segfault.c).
	 */
	regs->epc = (unsigned long)ksig->ka.sa.sa_handler;
	regs->sp = (unsigned long)frame;
	regs->a[0] = ksig->sig;                     /* a0: signal number */
	regs->a[1] = (unsigned long)(&frame->info); /* a1: siginfo pointer */
	regs->a[2] = (unsigned long)(&frame->uc);   /* a2: ucontext pointer */

#if DEBUG_SIG
	pr_info("SIG deliver (%s:%d): sig=%d pc=%p ra=%p sp=%p\n",
		current->comm, task_pid_nr(current), ksig->sig,
		(void *)regs->epc, (void *)regs->ra, frame);
#endif

	return 0;
}

static void handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	/* Are we from a system call? */
	if (regs->cause == EXC_SYSCALL) {
		/* If so, check system call restarting.. */
		switch (regs->v[0]) {
		case -ERESTART_RESTARTBLOCK:
		case -ERESTARTNOHAND:
			regs->v[0] = -EINTR;
			break;

		case -ERESTARTSYS:
			if (!(ksig->ka.sa.sa_flags & SA_RESTART)) {
				regs->v[0] = -EINTR;
				break;
			}
			/* fallthrough */
		case -ERESTARTNOINTR:
			regs->v[0] = regs->syscallno;
			regs->epc -= 0x4;
			break;
		}
	}

	/* Set up the stack frame */
	ret = setup_rt_frame(ksig, oldset, regs);

	signal_setup_done(ret, ksig, 0);
}

static void do_signal(struct pt_regs *regs)
{
	struct ksignal ksig;

	if (get_signal(&ksig)) {
		/* Actually deliver the signal */
		handle_signal(&ksig, regs);
		return;
	}

	/* Did we come from a system call? */
	if (regs->cause == EXC_SYSCALL) {
		/* Restart the system call - no handlers present */
		switch (regs->v[0]) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs->v[0] = regs->syscallno;
			regs->epc -= 0x4;
			break;
		case -ERESTART_RESTARTBLOCK:
			regs->v[0] = __NR_restart_syscall;
			regs->epc -= 0x4;
			break;
		}
	}

	/* If there is no signal to deliver, we just put the saved
	   sigmask back. */
	restore_saved_sigmask();
}

/*
 * notification of userspace execution resumption
 * - triggered by the _TIF_WORK_MASK flags
 */
asmlinkage void do_notify_resume(struct pt_regs *regs,
	unsigned long thread_info_flags)
{
	/* Handle pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING) {
		do_signal(regs);
	}

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}
