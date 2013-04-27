#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/tracehook.h>
#include <linux/linkage.h>

#include <asm/ucontext.h>

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
};

asmlinkage long __sys_sigaltstack(const stack_t __user *uss,
	stack_t __user *uoss, struct pt_regs *regs)
{
	return do_sigaltstack(uss, uoss, regs->sp);
}

asmlinkage long __sys_rt_sigreturn(struct pt_regs *regs)
{
	/* TODO */
	return 0;
}


static int handle_signal(unsigned long sig, siginfo_t *info,
	struct k_sigaction *ka, sigset_t *oldset, struct pt_regs *regs)
{
	/* TODO */
	return 0;
}

static void do_signal(struct pt_regs *regs)
{
	siginfo_t info;
	struct k_sigaction ka;
	int signr;

	/*
	 * We want the common case to go fast, which is why
	 * we may in certain cases get here from kernel mode.
	 * Just return without doing anything if so.
	 */
	if (!user_mode(regs))
		return;

	/* TODO: Implement system call restarting */

	signr = get_signal_to_deliver(&info, &ka, regs, NULL);
	if (signr > 0) {
		sigset_t *oldset;

		oldset = (test_thread_flag(TIF_RESTORE_SIGMASK) ?
			&(current->saved_sigmask) : &(current->blocked));

		/* Actually deliver the signal */
		if (!handle_signal(signr, &info, &ka, oldset, regs)) {
			/* A signal was successfully delivered; the saved
			 * sigmask will have been stored in the signal frame
			 * and will be restored by sigreturn, so the
			 * TIF_RESTORE_SIGMASK flag can be cleared.
			 */
			clear_thread_flag(TIF_RESTORE_SIGMASK);
		}
	} else {
		/* No signal to deliver; restore the saved sigmask */
		if (test_thread_flag(TIF_RESTORE_SIGMASK)) {
			clear_thread_flag(TIF_RESTORE_SIGMASK);
			sigprocmask(SIG_SETMASK, &current->saved_sigmask, NULL);
		}
	}
}

/*
 * notification of userspace execution resumption
 * - triggered by the _TIF_WORK_MASK flags
 */
asmlinkage void do_notify_resume(struct pt_regs *regs,
	unsigned long thread_info_flags)
{
	/* Handle pending signal delivery */
	if (thread_info_flags & (_TIF_SIGPENDING | _TIF_RESTORE_SIGMASK)) {
		do_signal(regs);
	}

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
		if (current->replacement_session_keyring)
			key_replace_session_keyring();
	}
}
