#include <linux/kbuild.h>
#include <linux/sched.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>

void asm_offsets(void)
{
	OFFSET(TASK_THREAD_INFO, task_struct, stack);
	OFFSET(THREAD_RA, task_struct, thread.ra);
	OFFSET(THREAD_S0, task_struct, thread.s[0]);
	OFFSET(THREAD_S1, task_struct, thread.s[1]);
	OFFSET(THREAD_S2, task_struct, thread.s[2]);
	OFFSET(THREAD_S3, task_struct, thread.s[3]);
	OFFSET(THREAD_S4, task_struct, thread.s[4]);
	OFFSET(THREAD_S5, task_struct, thread.s[5]);
	OFFSET(THREAD_S6, task_struct, thread.s[6]);
	OFFSET(THREAD_S7, task_struct, thread.s[7]);
	OFFSET(THREAD_S8, task_struct, thread.s[8]);
	OFFSET(THREAD_S9, task_struct, thread.s[9]);
	OFFSET(THREAD_S10, task_struct, thread.s[10]);
	OFFSET(THREAD_S11, task_struct, thread.s[11]);
	OFFSET(THREAD_SP, task_struct, thread.sp);
	OFFSET(TI_FLAGS, thread_info, flags);

	DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_EPC, pt_regs, epc);
	OFFSET(PT_RA, pt_regs, ra);
	OFFSET(PT_FP, pt_regs, s0);
	OFFSET(PT_S0, pt_regs, s0);
	OFFSET(PT_S1, pt_regs, s1);
	OFFSET(PT_S2, pt_regs, s2);
	OFFSET(PT_S3, pt_regs, s3);
	OFFSET(PT_S4, pt_regs, s4);
	OFFSET(PT_S5, pt_regs, s5);
	OFFSET(PT_S6, pt_regs, s6);
	OFFSET(PT_S7, pt_regs, s7);
	OFFSET(PT_S8, pt_regs, s8);
	OFFSET(PT_S9, pt_regs, s9);
	OFFSET(PT_S10, pt_regs, s10);
	OFFSET(PT_S11, pt_regs, s11);
	OFFSET(PT_SP, pt_regs, sp);
	OFFSET(PT_TP, pt_regs, tp);
	OFFSET(PT_A0, pt_regs, a0);
	OFFSET(PT_A1, pt_regs, a1);
	OFFSET(PT_A2, pt_regs, a2);
	OFFSET(PT_A3, pt_regs, a3);
	OFFSET(PT_A4, pt_regs, a4);
	OFFSET(PT_A5, pt_regs, a5);
	OFFSET(PT_A6, pt_regs, a6);
	OFFSET(PT_A7, pt_regs, a7);
	OFFSET(PT_T0, pt_regs, t0);
	OFFSET(PT_T1, pt_regs, t1);
	OFFSET(PT_T2, pt_regs, t2);
	OFFSET(PT_T3, pt_regs, t3);
	OFFSET(PT_T4, pt_regs, t4);
	OFFSET(PT_T5, pt_regs, t5);
	OFFSET(PT_T6, pt_regs, t6);
	OFFSET(PT_GP, pt_regs, gp);
	OFFSET(PT_STATUS, pt_regs, status);
	OFFSET(PT_BADVADDR, pt_regs, badvaddr);
	OFFSET(PT_CAUSE, pt_regs, cause);
	OFFSET(PT_SYSCALLNO, pt_regs, syscallno);
}
