#ifndef _UAPI_ASM_RISCV_BYTEORDER_H
#define _UAPI_ASM_RISCV_BYTEORDER_H

#if defined(__RISCVEL__)
#include <linux/byteorder/little_endian.h>
#elif defined(__RISCVEB__)
#include <linux/byteorder/big_endian.h>
#else
#error "Unknown endianness"
#endif

#endif /* _UAPI_ASM_RISCV_BYTEORDER_H */
