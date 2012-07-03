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

  /* Set up two-entry page table: zero out first page*/
  la x2, swapper_pg_dir
	lui x1, 0x2
  add x1, x2, x1 
2:
  addi x1, x1, -8
  sd x0, 0(x1)
  bne x2, x1, 2b

  /* Load entries:
   * ppn[2] = 0x380 (page_offset) -> 0x000
   * ppn[2] = 0x000 (identity)    -> 0x000
   */
  ori x1, x0, 0x386 /* Supervisor RWX + PTE_E */
  sd x1, 0(x2)
  li x3, 0x1c00
  add x3, x2, x3
  sd x1, 0(x3)

  /* Set PTBR (clears TLB) */
  mtpcr x2, cr7

  /* Enable VM */
  mfpcr x1, cr0
  ori x1, x1, (SR_VM)
  mtpcr x1, cr0

	la x1, start_kernel
	jr x1

	.end _start
