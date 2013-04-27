#include <linux/module.h>
#include <linux/uaccess.h>

int fixup_exception(struct pt_regs *regs, unsigned long epc)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(epc);
	if (fixup) {
		regs->epc = fixup->fixup;
		return 1;
	}
	return 0;
}
