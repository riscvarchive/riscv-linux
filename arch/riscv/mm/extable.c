#include <linux/module.h>
#include <linux/uaccess.h>

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(regs->epc);
	if (fixup) {
		regs->epc = fixup->fixup;
		return 1;
	}
	return 0;
}
