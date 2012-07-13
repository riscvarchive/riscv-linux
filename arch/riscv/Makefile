# BK Id: %F% %I% %G% %U% %#%
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
# Modifications for the RISC-V architecture:
# Quan Nguyen <quannguyen@eecs.berkeley.edu>
# Albert Ou <a_ou@eecs.berkeley.edu>
#
# Based on:
# arch/score/Makefile

KBUILD_DEFCONFIG 	:= riscv_defconfig

LDFLAGS         :=
OBJCOPYFLAGS    := -O binary
LDFLAGS_vmlinux :=

ifeq ($(CONFIG_CPU_LITTLE_ENDIAN),y)
	KBUILD_CFLAGS += -mel
else
	KBUILD_CFLAGS += -meb
endif

export BITS
ifeq ($(CONFIG_64BIT),y)
	BITS := 64
	KBUILD_CFLAGS += -mabi=64
else
	BITS := 32
	KBUILD_CFLAGS += -mabi=32
endif

KBUILD_CFLAGS += -Wall

head-y := arch/riscv/kernel/head_$(BITS).o \
          arch/riscv/kernel/init_task.o

core-y += arch/riscv/kernel/ arch/riscv/mm/

libs-y += arch/riscv/lib/

all: vmlinux
