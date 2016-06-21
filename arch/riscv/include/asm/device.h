#ifndef _ASM_RISCV_DEVICE_H
#define _ASM_RISCV_DEVICE_H

#include <linux/sysfs.h>

struct dev_archdata {
	struct dma_map_ops *dma_ops;
};

struct pdev_archdata {
	const char __iomem *config_start;
	const char __iomem *config_end;
	struct bin_attribute config;
};

#endif /* _ASM_RISCV_DEVICE_H */
