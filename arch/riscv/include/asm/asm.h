#ifndef _ASM_RISCV_ASM_H
#define _ASM_RISCV_ASM_H

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#ifdef __riscv64
#define __REG_SEL(a,b)	__ASM_STR(a)
#else
#define __REG_SEL(a,b)	__ASM_STR(b)
#endif

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)
#define SZREG		__REG_SEL(8, 4)
#define LGREG		__REG_SEL(3, 2)

#if (_RISCV_SZPTR == 64)
#define __PTR_SEL(a,b)	__ASM_STR(a)
#elif (_RISCV_SZPTR == 32)
#define __PTR_SEL(a,b)	__ASM_STR(b)
#else
#error "Unexpected _RISCV_SZPTR"
#endif

#define PTR		__PTR_SEL(.dword, .word)
#define SZPTR		__PTR_SEL(8, 4)
#define LGPTR		__PTR_SEL(3, 2)

#if (__SIZEOF_INT__ == 4)
#define INT		__ASM_STR(.word)
#define SZINT		__ASM_STR(4)
#define LGINT		__ASM_STR(2)
#else
#error "Unexpected __SIZEOF_INT__"
#endif

#if (__SIZEOF_SHORT__ == 2)
#define SHORT		__ASM_STR(.half)
#define SZSHORT		__ASM_STR(2)
#define LGSHORT		__ASM_STR(1)
#else
#error "Unexpected __SIZEOF_SHORT__"
#endif

#endif /* _ASM_RISCV_ASM_H */
