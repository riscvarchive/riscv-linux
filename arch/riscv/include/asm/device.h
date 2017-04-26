/*
 * Copyright (C) 2016 SiFive
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


#ifndef _ASM_RISCV_DEVICE_H
#define _ASM_RISCV_DEVICE_H

#include <linux/sysfs.h>

struct dev_archdata {
	struct dma_map_ops *dma_ops;
};

struct pdev_archdata {
};

#endif /* _ASM_RISCV_DEVICE_H */
