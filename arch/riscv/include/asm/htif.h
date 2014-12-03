#ifndef _ASM_RISCV_HTIF_H
#define _ASM_RISCV_HTIF_H

#include <linux/kernel.h>
#include <linux/device.h>

#include <asm/csr.h>

#ifdef CONFIG_64BIT
#define HTIF_DEV_SHIFT      (56)
#define HTIF_CMD_SHIFT      (48)
#else
#define HTIF_DEV_SHIFT      (24)
#define HTIF_CMD_SHIFT      (16)
#endif

#define HTIF_CMD_READ       (0x00UL)
#define HTIF_CMD_WRITE      (0x01UL)
#define HTIF_CMD_IDENTITY   (0xFFUL)

#define HTIF_NR_DEV         (256UL)

static inline void htif_tohost(unsigned long dev,
	unsigned long cmd, unsigned long data)
{
	unsigned long packet;
	packet = (dev << HTIF_DEV_SHIFT) | (cmd << HTIF_CMD_SHIFT) | data;
	while (csr_swap(tohost, packet) != 0);
}

static inline unsigned long htif_fromhost(void)
{
	unsigned long data;
	while ((data = csr_swap(fromhost, 0)) == 0);
	return data;
}

extern struct bus_type htif_bus_type;

struct htif_dev {
	unsigned int minor;
	const char *type;
	const char *spec;
	struct device dev;
};

#define to_htif_dev(d) container_of(d, struct htif_dev, dev)

struct htif_driver {
	const char *type;
	struct device_driver driver;
};

#define to_htif_driver(d) container_of(d, struct htif_driver, driver)

extern int htif_register_driver(struct htif_driver *drv);
extern void htif_unregister_driver(struct htif_driver *drv);

#endif /* _ASM_RISCV_HTIF_H */
