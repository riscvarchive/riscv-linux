#include <asm/ptrace.h>
#include <asm/syscall.h>
#include <asm/thread_info.h>
#include <linux/ptrace.h>
#include <linux/elf.h>
#include <linux/regset.h>
#include <linux/tracehook.h>
#include <trace/events/syscalls.h>

static int gpr_get(struct task_struct *target,
	const struct user_regset *regset, unsigned int pos,
	unsigned int count, void *kbuf, void __user *ubuf)
{
	struct pt_regs *regs = task_pt_regs(target);
	/* NOTE: Ensure that sensitive CSR contents are not exposed */
	return user_regset_copyout(&pos, &count, &kbuf, &ubuf, regs, 0, -1);
}

static int gpr_set(struct task_struct *target,
	const struct user_regset *regset, unsigned int pos,
	unsigned int count, const void *kbuf, const void __user *ubuf)
{
	struct pt_regs *regs = task_pt_regs(target);
	/* NOTE: Ensure that entries for privileged CSRs are not overwritten */
	return user_regset_copyin(&pos, &count, &kbuf, &ubuf, regs, 0, -1);
}

enum riscv_regset {
	REGSET_GPR,
};

static const struct user_regset riscv_regsets[] = {
	[REGSET_GPR] = {
		.core_note_type = NT_PRSTATUS,
		.n = sizeof(struct user_regs_struct) / sizeof(unsigned long),
		.size = sizeof(unsigned long),
		.align = sizeof(unsigned long),
		.get = gpr_get,
		.set = gpr_set
	},
};

static const struct user_regset_view user_riscv_view = {
	.name = "riscv", .e_machine = EM_RISCV,
	.regsets = riscv_regsets, .n = ARRAY_SIZE(riscv_regsets)
};

const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
	return &user_riscv_view;
}

void ptrace_disable(struct task_struct *child)
{
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
