#include <linux/kbuild.h>
#include <linux/sched.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>

void asm_offsets(void)
{
	OFFSET(TASK_THREAD_INFO, task_struct, stack);
	OFFSET(THREAD_PC, task_struct, thread.pc);
	OFFSET(THREAD_SP, task_struct, thread.sp);
#ifdef CONFIG_FRAME_POINTER
	OFFSET(THREAD_FP, task_struct, thread.fp);
#endif /* CONFIG_FRAME_POINTER */
	OFFSET(TI_FLAGS, thread_info, flags);

	DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_ZERO, pt_regs, zero);
	OFFSET(PT_RA, pt_regs, ra);
	OFFSET(PT_FP, pt_regs, s[0]);
	OFFSET(PT_S0, pt_regs, s[0]);
	OFFSET(PT_S1, pt_regs, s[1]);
	OFFSET(PT_S2, pt_regs, s[2]);
	OFFSET(PT_S3, pt_regs, s[3]);
	OFFSET(PT_S4, pt_regs, s[4]);
	OFFSET(PT_S5, pt_regs, s[5]);
	OFFSET(PT_S6, pt_regs, s[6]);
	OFFSET(PT_S7, pt_regs, s[7]);
	OFFSET(PT_S8, pt_regs, s[8]);
	OFFSET(PT_S9, pt_regs, s[9]);
	OFFSET(PT_S10, pt_regs, s[10]);
	OFFSET(PT_S11, pt_regs, s[11]);
	OFFSET(PT_SP, pt_regs, sp);
	OFFSET(PT_TP, pt_regs, tp);
	OFFSET(PT_V0, pt_regs, v[0]);
	OFFSET(PT_V1, pt_regs, v[1]);
	OFFSET(PT_A0, pt_regs, a[0]);
	OFFSET(PT_A1, pt_regs, a[1]);
	OFFSET(PT_A2, pt_regs, a[2]);
	OFFSET(PT_A3, pt_regs, a[3]);
	OFFSET(PT_A4, pt_regs, a[4]);
	OFFSET(PT_A5, pt_regs, a[5]);
	OFFSET(PT_A6, pt_regs, a[6]);
	OFFSET(PT_A7, pt_regs, a[7]);
	OFFSET(PT_T0, pt_regs, t[0]);
	OFFSET(PT_T1, pt_regs, t[1]);
	OFFSET(PT_T2, pt_regs, t[2]);
	OFFSET(PT_T3, pt_regs, t[3]);
	OFFSET(PT_T4, pt_regs, t[4]);
	OFFSET(PT_T5, pt_regs, t[5]);
	OFFSET(PT_STATUS, pt_regs, status);
	OFFSET(PT_EPC, pt_regs, epc);
	OFFSET(PT_BADVADDR, pt_regs, badvaddr);
	OFFSET(PT_CAUSE, pt_regs, cause);
}
