#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/processor.h>
#include <asm/thread_info.h>

__INIT
ENTRY(_start)
	.align 4
	.global _start
	.ent _start
_start:

	mfpcr x1, cr0
#ifdef CONFIG_64BIT
	ori x1, x1, (SR_S64 | SR_U64)
#else
	andi x1, x1, ~(SR_S64 | SR_U64)
#endif
	mtpcr x1, cr0

	la sp, init_thread_union + THREAD_SIZE

	/* Clear .bss segment */
	la x1, __bss_start
	la x2, _end
1:
	sw zero, 0(x1)
	addi x1, x1, 0x4
	bltu x1, x2, 1b

	la x1, start_kernel
	jr x1

	.end _start
