#
# Makefile for the RISC-V Linux kernel
#

extra-y := head.o init_task.o vmlinux.lds

obj-y   := irq.o process.o ptrace.o reset.o setup.o time.o traps.o

obj-$(CONFIG_EARLY_PRINTK)      += early_printk.o

clean:
