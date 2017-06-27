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

#ifndef _UAPI_ASM_RISCV_SIGCONTEXT_H
#define _UAPI_ASM_RISCV_SIGCONTEXT_H

#include <asm/ptrace.h>

/* Extension context structure
 *
 * This extends the signal context below with architectural state for
 * an ISA extension.  The state extends from the end of this struct
 * to size bytes thereafter.  The tag identifies the extension type.
 */
struct __riscv_ext_context {
	struct __riscv_ext_context *next;
	__u32 tag;
	__u32 size;
};

/* Signal context structure
 *
 * This contains the context saved before a signal handler is invoked;
 * it is restored by sys_sigreturn / sys_rt_sigreturn.
 */
struct sigcontext {
	struct user_regs_struct sc_regs;
	struct __riscv_ext_context *sc_ext;
};

/* Double-precision floating-point extension context */
#define __riscv_d_ext_tag 0x00000003

struct __riscv_d_ext_context {
  struct __riscv_ext_context head;
  struct __riscv_d_ext_state state;
};

#endif /* _UAPI_ASM_RISCV_SIGCONTEXT_H */
