#ifndef _ASM_RISCV_HTIF_H
#define _ASM_RISCV_HTIF_H

#include <linux/device.h>
#include <linux/irqreturn.h>

#include <asm/csr.h>

#define HTIF_DEV_SHIFT      (56)
#define HTIF_CMD_SHIFT      (48)

#define HTIF_CMD_READ       (0x00UL)
#define HTIF_CMD_WRITE      (0x01UL)
#define HTIF_CMD_IDENTIFY   (0xFFUL)

#define HTIF_MAX_DEV        (256)
#define HTIF_MAX_ID         (64)

#define HTIF_ALIGN          (64)

extern struct bus_type htif_bus_type;

static inline void htif_tohost(unsigned long dev,
	unsigned long cmd, unsigned long data)
{
	unsigned long packet;

	packet = (dev << HTIF_DEV_SHIFT) | (cmd << HTIF_CMD_SHIFT) | data;
	while (csr_swap(tohost, packet)) {
		cpu_relax();
	}
}

static inline unsigned long __htif_fromhost(void)
{
	return csr_swap(fromhost, 0);
}

static inline unsigned long htif_fromhost(void)
{
	unsigned long data;

	while (!(data = __htif_fromhost())) {
		cpu_relax();
	}
	return data;
}

struct htif_device {
	struct device dev;
	unsigned int index;
	char id[];
};

#define to_htif_dev(d) container_of(d, struct htif_device, dev)

struct htif_driver {
	struct device_driver driver;
	const char *type;
};

#define to_htif_driver(d) container_of(d, struct htif_driver, driver)

extern int htif_register_driver(struct htif_driver *drv);
extern void htif_unregister_driver(struct htif_driver *drv);

typedef irqreturn_t (*htif_irq_handler_t)(struct htif_device *, unsigned long);

extern int htif_request_irq(struct htif_device *dev, htif_irq_handler_t handler);
extern void htif_free_irq(struct htif_device *dev);

#endif /* _ASM_RISCV_HTIF_H */
