#include <linux/ptrace.h>
#include <linux/elf.h>
#include <linux/regset.h>

static const struct user_regset_view user_riscv_native_view = {
	.name = "riscv",
	.e_machine = EM_RISCV,
	.regsets = NULL,
	.n = 0,
};

const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
	return &user_riscv_native_view;
}

void ptrace_disable(struct task_struct *child)
{
}

long arch_ptrace(struct task_struct *child, long request,
                 unsigned long addr, unsigned long data)
{
	return 0;
}
