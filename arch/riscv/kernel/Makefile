#
# Makefile for the RISC-V Linux kernel
#

extra-y := head.o vmlinux.lds

obj-y	:= cpu.o entry.o irq.o process.o ptrace.o reset.o setup.o \
	   sbi.o signal.o syscall_table.o sys_riscv.o time.o traps.o \
	   stacktrace.o vdso.o vdso/

obj-$(CONFIG_SMP)		+= smpboot.o smp.o
obj-$(CONFIG_SBI_CONSOLE)	+= sbi-con.o

clean:
