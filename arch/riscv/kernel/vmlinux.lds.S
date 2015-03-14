#define LOAD_OFFSET PAGE_OFFSET
#include <asm/vmlinux.lds.h>
#include <asm/page.h>
#include <asm/thread_info.h>

OUTPUT_ARCH(riscv)
ENTRY(_start)

jiffies = jiffies_64;

SECTIONS
{
	/* Beginning of code and text segment */
	. = LOAD_OFFSET;
	_start = .;
	__init_begin = .;
	HEAD_TEXT_SECTION
	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(16)
	__init_end = .;

	.text : {
		_text = .;
		_stext = .;
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		ENTRY_TEXT
		IRQENTRY_TEXT
		*(.fixup)
		_etext = .;
	}

	/* Start of data section */
	_sdata = .;
	RO_DATA_SECTION(PAGE_SIZE)
	RW_DATA_SECTION(0x40, PAGE_SIZE, THREAD_SIZE)
	.sdata : {
		_gp = . + 0x800;
		*(.sdata*)
	}
	.srodata : {
		*(.srodata*)
	}
	/* End of data section */
	_edata = .;

	BSS_SECTION(0x20, 0, 0x20)

	EXCEPTION_TABLE(0x10)
	NOTES

	.rel.dyn : {
		*(.rel.dyn*)
	}

	_end = .;

	STABS_DEBUG
	DWARF_DEBUG

	DISCARDS
}
