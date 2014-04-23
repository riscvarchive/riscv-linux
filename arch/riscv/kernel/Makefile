#
# Makefile for the RISC-V Linux kernel
#

extra-y := head_$(BITS).o init_task.o vmlinux.lds

obj-y	:= cpu.o entry.o irq.o process.o ptrace.o reset.o setup.o \
	   signal.o syscall_table.o sys_riscv.o time.o traps.o hwacha.o

obj-$(CONFIG_EARLY_PRINTK)      += early_printk.o

clean:
