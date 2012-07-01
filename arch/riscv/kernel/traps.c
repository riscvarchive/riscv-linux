#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/init.h>

#include <asm/processor.h>
#include <asm/ptrace.h>

void show_stack(struct task_struct *task, unsigned long *sp)
{
}

void dump_stack(void)
{
}

EXPORT_SYMBOL(dump_stack);

void show_regs(struct pt_regs *regs)
{
}

void __init trap_init(void)
{
}
