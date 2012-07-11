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
	. = LOAD_OFFSET + 0x2000;
	_start = 0x2000;
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
		_etext = .;
	}

	_sdata = .;
	RO_DATA_SECTION(PAGE_SIZE)
	RW_DATA_SECTION(64, PAGE_SIZE, THREAD_SIZE)
	_edata = .;

	EXCEPTION_TABLE(16)
	NOTES

#ifdef CONFIG_64BIT
	BSS_SECTION(0, 8, 8)
#else
	BSS_SECTION(0, 4, 4)
#endif /* CONFIG_64BIT */
	_end = .;

	STABS_DEBUG
	DWARF_DEBUG

	DISCARDS
}
