#!/bin/sh
#
# arch/riscv/install.sh
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995 by Linus Torvalds
#
# Adapted from code in arch/ia64/install.sh by Shea Levy
#
# "make install" script for riscv architecture
#
# Arguments:
#   $1 - kernel version
#   $2 - kernel image file
#   $3 - kernel map file
#   $4 - default install path (blank if root directory)
#

# User may have a custom install script

if [ -x ~/bin/${INSTALLKERNEL} ]; then exec ~/bin/${INSTALLKERNEL} "$@"; fi
if [ -x /sbin/${INSTALLKERNEL} ]; then exec /sbin/${INSTALLKERNEL} "$@"; fi

# Default install - no bootloader configuration (yet?)
base=$(basename $2)

if [ -f $4/$base ]; then
	mv $4/$base $4/$base.old
fi

if [ -f $4/System.map ]; then
	mv $4/System.map $4/System.old
fi

cat $2 > $4/$base
cp $3 $4/System.map
