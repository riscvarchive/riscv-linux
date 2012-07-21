#ifndef __ASM_RISCV_SIGCONTEXT_H
#define __ASM_RISCV_SIGCONTEXT_H

#include <asm/ptrace.h>
#include <asm/user.h>

/* This struct is saved by setup_frame in signal.c, to keep the current
   context while a signal handler is executed. It's restored by sys_sigreturn.
*/

struct sigcontext {
	struct user_regs_struct regs;  /* needs to be first */
	unsigned long oldmask;
};

#endif /* __ASM_RISCV_SIGCONTEXT_H */
