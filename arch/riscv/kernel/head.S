#include <asm/thread_info.h>
#include <linux/init.h>
#include <linux/linkage.h>

__INIT
ENTRY(_start)
	/* Load the global pointer (before any other use of la) */
	la gp, _gp

	/* Clear the .bss segment */
	la a0, __bss_start
	li a1, 0
	la a2, __bss_stop
	sub a2, a2, a0
	call memset

	/* Initialize stack pointer */
	la sp, init_thread_union + THREAD_SIZE
	/* Initialize current task_struct pointer */
	la tp, init_task

	tail start_kernel
END(_start)


__PAGE_ALIGNED_BSS
	/* Empty zero page */
	.balign PAGE_SIZE
ENTRY(empty_zero_page)
	.fill (empty_zero_page + PAGE_SIZE) - ., 1, 0x00
END(empty_zero_page)
