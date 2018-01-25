#include <asm/ptrace.h>
#include <asm/syscall.h>
#include <asm/thread_info.h>
#include <linux/ptrace.h>
#include <linux/elf.h>
#include <linux/regset.h>
#include <linux/tracehook.h>
#include <trace/events/syscalls.h>

enum riscv_regset {
	REGSET_X,
};

/*
 * Get registers from task and ready the result for userspace.
 */
static char *getregs(struct task_struct *child, struct pt_regs *uregs)
{
	*uregs = *task_pt_regs(child);
	return (char *)uregs;
}

/* Put registers back to task. */
static void putregs(struct task_struct *child, struct pt_regs *uregs)
{
	struct pt_regs *regs = task_pt_regs(child);
	*regs = *uregs;
}

static int riscv_gpr_get(struct task_struct *target,
                         const struct user_regset *regset,
                         unsigned int pos, unsigned int count,
                         void *kbuf, void __user *ubuf)
{
	struct pt_regs regs;

	getregs(target, &regs);

	return user_regset_copyout(&pos, &count, &kbuf, &ubuf, &regs, 0,
				   sizeof(regs));
}

static int riscv_gpr_set(struct task_struct *target,
                         const struct user_regset *regset,
                         unsigned int pos, unsigned int count,
                         const void *kbuf, const void __user *ubuf)
{
	int ret;
	struct pt_regs regs;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &regs, 0,
				 sizeof(regs));
	if (ret)
		return ret;

	putregs(target, &regs);

	return 0;
}


static const struct user_regset riscv_user_regset[] = {
	[REGSET_X] = {
		.core_note_type = NT_PRSTATUS,
		.n = ELF_NGREG,
		.size = sizeof(elf_greg_t),
		.align = sizeof(elf_greg_t),
		.get = &riscv_gpr_get,
		.set = &riscv_gpr_set,
	},
};

static const struct user_regset_view riscv_user_native_view = {
	.name = "riscv",
	.e_machine = EM_RISCV,
	.regsets = riscv_user_regset,
	.n = ARRAY_SIZE(riscv_user_regset),
};

const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
	return &riscv_user_native_view;
}

void ptrace_disable(struct task_struct *child)
{
        clear_tsk_thread_flag(child, TIF_SYSCALL_TRACE);
}

long arch_ptrace(struct task_struct *child, long request,
                 unsigned long addr, unsigned long data)
{
	long ret = -EIO;

	switch (request) {
	default:
		ret = ptrace_request(child, request, addr, data);
		break;
	}

	return ret;
}

/* Allows PTRACE_SYSCALL to work.  These are called from entry.S in
 * {handle,ret_from}_syscall. */
void do_syscall_trace_enter(struct pt_regs *regs)
{
	if (test_thread_flag(TIF_SYSCALL_TRACE)) {
		if (tracehook_report_syscall_entry(regs))
			syscall_set_nr(current, regs, -1);
	}

#ifdef CONFIG_HAVE_SYSCALL_TRACEPOINTS
	if (test_thread_flag(TIF_SYSCALL_TRACEPOINT))
		trace_sys_enter(regs, syscall_get_nr(current, regs));
#endif
}

void do_syscall_trace_exit(struct pt_regs *regs)
{
	if (test_thread_flag(TIF_SYSCALL_TRACE))
		tracehook_report_syscall_exit(regs, 0);

#ifdef CONFIG_HAVE_SYSCALL_TRACEPOINTS
	if (test_thread_flag(TIF_SYSCALL_TRACEPOINT))
		trace_sys_exit(regs, regs->regs[0]);
#endif
}
