#ifndef _ASM_RISCV_SBI_H
#define _ASM_RISCV_SBI_H

#define SBI_CALL(which, arg0, arg1, arg2) ({ 		\
	register uintptr_t a0 asm ("a0") = (arg0);	\
	register uintptr_t a1 asm ("a1") = (arg1);	\
	register uintptr_t a2 asm ("a2") = (arg2);	\
	register uintptr_t a7 asm ("a7") = (which);	\
	asm volatile ("ecall"				\
		      : "+r" (a0)			\
		      : "r" (a1), "r" (a2), "r" (a7)	\
		      : "memory");			\
	a0;						\
})

/* Lazy implementations until SBI is finalized */
#define SBI_CALL_0(which) SBI_CALL(which, 0, 0, 0)
#define SBI_CALL_1(which, arg0) SBI_CALL(which, arg0, 0, 0)
#define SBI_CALL_2(which, arg0, arg1) SBI_CALL(which, arg0, arg1, 0)

static inline void sbi_console_putchar(int ch)
{
	SBI_CALL_1(1, ch);
}

static inline int sbi_console_getchar(void)
{
	return SBI_CALL_0(2);
}

static inline void sbi_set_timer(unsigned long long stime_value)
{
#if __riscv_xlen == 32
	SBI_CALL_2(7, stime_value, stime_value >> 32);
#else
	SBI_CALL_1(7, stime_value);
#endif
}

static inline void sbi_shutdown(void)
{
	SBI_CALL_0(6);
}

#endif
