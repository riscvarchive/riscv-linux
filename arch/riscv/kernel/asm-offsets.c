/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <linux/kbuild.h>
#include <linux/sched.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>

void asm_offsets(void)
{
	OFFSET(TASK_THREAD_INFO, task_struct, stack);
	OFFSET(THREAD_RA, task_struct, thread.ra);
	OFFSET(THREAD_SP, task_struct, thread.sp);
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
	OFFSET(TI_TASK, thread_info, task);
	OFFSET(TI_FLAGS, thread_info, flags);
	OFFSET(TI_CPU, thread_info, cpu);

	OFFSET(THREAD_F0,  task_struct, thread.fstate.f[0]);
	OFFSET(THREAD_F1,  task_struct, thread.fstate.f[1]);
	OFFSET(THREAD_F2,  task_struct, thread.fstate.f[2]);
	OFFSET(THREAD_F3,  task_struct, thread.fstate.f[3]);
	OFFSET(THREAD_F4,  task_struct, thread.fstate.f[4]);
	OFFSET(THREAD_F5,  task_struct, thread.fstate.f[5]);
	OFFSET(THREAD_F6,  task_struct, thread.fstate.f[6]);
	OFFSET(THREAD_F7,  task_struct, thread.fstate.f[7]);
	OFFSET(THREAD_F8,  task_struct, thread.fstate.f[8]);
	OFFSET(THREAD_F9,  task_struct, thread.fstate.f[9]);
	OFFSET(THREAD_F10, task_struct, thread.fstate.f[10]);
	OFFSET(THREAD_F11, task_struct, thread.fstate.f[11]);
	OFFSET(THREAD_F12, task_struct, thread.fstate.f[12]);
	OFFSET(THREAD_F13, task_struct, thread.fstate.f[13]);
	OFFSET(THREAD_F14, task_struct, thread.fstate.f[14]);
	OFFSET(THREAD_F15, task_struct, thread.fstate.f[15]);
	OFFSET(THREAD_F16, task_struct, thread.fstate.f[16]);
	OFFSET(THREAD_F17, task_struct, thread.fstate.f[17]);
	OFFSET(THREAD_F18, task_struct, thread.fstate.f[18]);
	OFFSET(THREAD_F19, task_struct, thread.fstate.f[19]);
	OFFSET(THREAD_F20, task_struct, thread.fstate.f[20]);
	OFFSET(THREAD_F21, task_struct, thread.fstate.f[21]);
	OFFSET(THREAD_F22, task_struct, thread.fstate.f[22]);
	OFFSET(THREAD_F23, task_struct, thread.fstate.f[23]);
	OFFSET(THREAD_F24, task_struct, thread.fstate.f[24]);
	OFFSET(THREAD_F25, task_struct, thread.fstate.f[25]);
	OFFSET(THREAD_F26, task_struct, thread.fstate.f[26]);
	OFFSET(THREAD_F27, task_struct, thread.fstate.f[27]);
	OFFSET(THREAD_F28, task_struct, thread.fstate.f[28]);
	OFFSET(THREAD_F29, task_struct, thread.fstate.f[29]);
	OFFSET(THREAD_F30, task_struct, thread.fstate.f[30]);
	OFFSET(THREAD_F31, task_struct, thread.fstate.f[31]);
	OFFSET(THREAD_FCSR, task_struct, thread.fstate.fcsr);

	DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_SEPC, pt_regs, sepc);
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
	OFFSET(PT_SSTATUS, pt_regs, sstatus);
	OFFSET(PT_SBADADDR, pt_regs, sbadaddr);
	OFFSET(PT_SCAUSE, pt_regs, scause);
}
