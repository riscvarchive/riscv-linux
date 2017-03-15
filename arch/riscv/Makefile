# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

LDFLAGS         :=
OBJCOPYFLAGS    := -O binary
LDFLAGS_vmlinux :=
KBUILD_AFLAGS_MODULE += -fPIC
KBUILD_CFLAGS_MODULE += -fPIC

ifeq ($(ARCH),riscv)
	KBUILD_DEFCONFIG = riscv64_spike
else
	KBUILD_DEFCONFIG = $(ARCH)_spike
endif

export BITS
ifeq ($(CONFIG_64BIT),y)
	BITS := 64
	UTS_MACHINE := riscv64

	KBUILD_CFLAGS += -mabi=lp64
	KBUILD_AFLAGS += -mabi=lp64
	KBUILD_MARCH = rv64im
	LDFLAGS += -melf64lriscv
else
	BITS := 32
	UTS_MACHINE := riscv32

	KBUILD_CFLAGS += -mabi=ilp32
	KBUILD_AFLAGS += -mabi=ilp32
	KBUILD_MARCH = rv32im
	LDFLAGS += -melf32lriscv
endif

ifeq ($(CONFIG_RV_ATOMIC),y)
	KBUILD_RV_ATOMIC = a
endif

KBUILD_CFLAGS += -Wall

ifeq ($(CONFIG_RVC),y)
	KBUILD_RVC = c
endif

KBUILD_AFLAGS += -march=$(KBUILD_MARCH)$(KBUILD_RV_ATOMIC)fd$(KBUILD_RVC)

KBUILD_CFLAGS += -march=$(KBUILD_MARCH)$(KBUILD_RV_ATOMIC)$(KBUILD_RVC)
KBUILD_CFLAGS += -mno-save-restore

head-y := arch/riscv/kernel/head.o

core-y += arch/riscv/kernel/ arch/riscv/mm/

libs-y += arch/riscv/lib/

all: vmlinux
