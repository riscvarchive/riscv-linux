#include <linux/ptrace.h>
#include <linux/elf.h>
#include <linux/regset.h>

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
	return 0;
}
