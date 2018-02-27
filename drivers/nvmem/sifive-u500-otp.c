/*
 * Copyright (C) 2018 SiFive, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

/* Register offsets */
#define EMEMORYOTP_REG_GET(reg)		ioread32(otp->base + reg)
#define EMEMORYOTP_REG_SET(reg, val)	iowrite32(val, otp->base + reg)

#define EMEMORYOTP_PA                 0x00
#define EMEMORYOTP_PAIO               0x04
#define EMEMORYOTP_PAS                0x08
#define EMEMORYOTP_PCE                0x0C
#define EMEMORYOTP_PCLK               0x10
#define EMEMORYOTP_PDIN               0x14
#define EMEMORYOTP_PDOUT              0x18
#define EMEMORYOTP_PDSTB              0x1C
#define EMEMORYOTP_PPROG              0x20
#define EMEMORYOTP_PTC                0x24
#define EMEMORYOTP_PTM                0x28
#define EMEMORYOTP_PTM_REP            0x2C
#define EMEMORYOTP_PTR                0x30
#define EMEMORYOTP_PTRIM              0x34
#define EMEMORYOTP_PWE                0x38

/* Timing delays (in us)
   MIN indicates that there is no maximum.
   TYP indicates that there is a maximum
   that should not be exceeded.
   When the minimums are < 1us, I just put 1us.
*/

#define EMEMORYOTP_MIN_TVDS      1
#define EMEMORYOTP_MIN_TSAS      2
#define EMEMORYOTP_MIN_TTAS      50
#define EMEMORYOTP_MIN_TTAH      1
#define EMEMORYOTP_MIN_TASH      1
#define EMEMORYOTP_MIN_TMS       1
#define EMEMORYOTP_MIN_TCS       10
#define EMEMORYOTP_MIN_TMH       1
#define EMEMORYOTP_MIN_TAS       50

#define EMEMORYOTP_MAX_TCD       1
#define EMEMORYOTP_MIN_TKH       1

// Note: This has an upper limit of 100.
#define EMEMORYOTP_MIN_TCSP      10
#define EMEMORYOTP_TYP_TCSP      11

// This has an upper limit of 20.
#define EMEMORYOTP_MIN_TPPS      5
#define EMEMORYOTP_TYP_TPPS      6

// This has an upper limit of 20.
#define EMEMORYOTP_MIN_TPPH      1
#define EMEMORYOTP_TYP_TPPH      2

// This has upper limit of 100.
#define EMEMORYOTP_MIN_TPPR      5
#define EMEMORYOTP_TYP_TPPR      6

// This has upper limit of 20
#define EMEMORYOTP_MIN_TPW       10
#define EMEMORYOTP_TYP_TPW       11

#define EMEMORYOTP_MIN_TASP      1
#define EMEMORYOTP_MIN_TDSP      1

#define EMEMORYOTP_MIN_TAHP      1
#define EMEMORYOTP_MIN_TDHP      1
// This has a max of 5!
#define EMEMORYOTP_MIN_TPWI      1
#define EMEMORYOTP_TYP_TPWI      2

struct sifive_u500_otp {
	void __iomem		*base;
	raw_spinlock_t		lock;
	struct nvmem_config	config;
};

static void sifive_u500_otp_power_up_sequence(struct sifive_u500_otp *otp)
{
	// Probably don't need to do this, since
	// all the other stuff has been happening.
	// But it is on the wave form.
	udelay(EMEMORYOTP_MIN_TVDS);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PDSTB, 1);
	udelay(EMEMORYOTP_MIN_TSAS);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTRIM, 1);
	udelay(EMEMORYOTP_MIN_TTAS);
}

static void sifive_u500_otp_power_down_sequence(struct sifive_u500_otp *otp)
{
	udelay(EMEMORYOTP_MIN_TTAH);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTRIM, 0);
	udelay(EMEMORYOTP_MIN_TASH);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PDSTB, 0);
	// No delay indicated after this
}

static void sifive_u500_otp_begin_read(struct sifive_u500_otp *otp)
{
	// Initialize
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCLK, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PA, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PDIN, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTM, 0);
	udelay(EMEMORYOTP_MIN_TMS);

	// Enable chip select
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCE, 1);
	udelay(EMEMORYOTP_MIN_TCS);
}

static void sifive_u500_otp_exit_read(struct sifive_u500_otp *otp)
{
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCLK, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PA, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PDIN, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 0);
	// Disable chip select
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCE, 0);
	// Wait before changing PTM
	udelay(EMEMORYOTP_MIN_TMH);
}

static unsigned int sifive_u500_otp_read(struct sifive_u500_otp *otp, int address)
{
	unsigned int read_value;
	int delay;

	EMEMORYOTP_REG_SET(EMEMORYOTP_PA, address);
	// Toggle clock
	udelay(EMEMORYOTP_MIN_TAS);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCLK, 1);

	// Insert delay until data is ready.
	// There are lots of delays
	// on the chart, but I think this is the most relevant.
	delay = max(EMEMORYOTP_MAX_TCD, EMEMORYOTP_MIN_TKH);
	udelay(delay);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCLK, 0);
	read_value = EMEMORYOTP_REG_GET(EMEMORYOTP_PDOUT);

	// Could check here for things like TCYC < TAH + TCD
	return read_value;
}

static void sifive_u500_otp_pgm_entry(struct sifive_u500_otp *otp)
{
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCLK, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PA, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PAS, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PAIO, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PDIN, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 0);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTM, 2);
	udelay(EMEMORYOTP_MIN_TMS);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCE, 1);
	udelay(EMEMORYOTP_TYP_TCSP);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PPROG, 1);
	udelay(EMEMORYOTP_TYP_TPPS);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTRIM, 1);
}

static void sifive_u500_otp_pgm_exit(struct sifive_u500_otp *otp)
{
	EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 0);
	udelay(EMEMORYOTP_TYP_TPPH);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PPROG, 0);
	udelay(EMEMORYOTP_TYP_TPPR);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PCE, 0);
	udelay(EMEMORYOTP_MIN_TMH);
	EMEMORYOTP_REG_SET(EMEMORYOTP_PTM, 0);
}

static void sifive_u500_otp_pgm_access(struct sifive_u500_otp *otp, int address, unsigned int write_data)
{
	int i, pas, delay;

	EMEMORYOTP_REG_SET(EMEMORYOTP_PA, address);
	for (pas = 0; pas < 2; pas ++) {
		EMEMORYOTP_REG_SET(EMEMORYOTP_PAS, pas);
		for (i = 0; i < 32; i++) {
			EMEMORYOTP_REG_SET(EMEMORYOTP_PAIO, i);
			EMEMORYOTP_REG_SET(EMEMORYOTP_PDIN, (write_data >> i) & 1);
			delay = max(EMEMORYOTP_MIN_TASP, EMEMORYOTP_MIN_TDSP);
			udelay(delay);
			EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 1);
			udelay(EMEMORYOTP_TYP_TPW);
			EMEMORYOTP_REG_SET(EMEMORYOTP_PWE, 0);
			delay = max(EMEMORYOTP_MIN_TAHP, EMEMORYOTP_MIN_TDHP);
			delay = max(delay, EMEMORYOTP_TYP_TPWI);
			udelay(delay);
		}
	}
	EMEMORYOTP_REG_SET(EMEMORYOTP_PAS, 0);
}

static int sifive_u500_otp_reg_read(void *context, unsigned int offset, void *val, size_t bytes)
{
	struct sifive_u500_otp *otp = context;
	unsigned long flags;
	u32 *buf = val;
	int i;

	raw_spin_lock_irqsave(&otp->lock, flags);
	sifive_u500_otp_power_up_sequence(otp);
	sifive_u500_otp_begin_read(otp);

	for (i = 0; i < bytes/4; ++i)
		buf[i] = sifive_u500_otp_read(otp, i+offset/4);

	sifive_u500_otp_exit_read(otp);
	sifive_u500_otp_power_down_sequence(otp);
	raw_spin_unlock_irqrestore(&otp->lock, flags);

	return 0;
}

static int sifive_u500_otp_reg_write(void *context, unsigned int offset, void *val, size_t bytes)
{
	struct sifive_u500_otp *otp = context;
	unsigned long flags;
	u32 *buf = val;
	int i;

	if (offset % 4 || bytes % 4)
		return -EINVAL;

	raw_spin_lock_irqsave(&otp->lock, flags);
	sifive_u500_otp_power_up_sequence(otp);
	sifive_u500_otp_pgm_entry(otp);

	for (i = 0; i < bytes/4; ++i)
		sifive_u500_otp_pgm_access(otp, i+offset/4, buf[i]);

	sifive_u500_otp_pgm_exit(otp);
	sifive_u500_otp_power_down_sequence(otp);
	raw_spin_unlock_irqrestore(&otp->lock, flags);

	return 0;
}

static int sifive_u500_otp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct sifive_u500_otp *otp;
	struct nvmem_device *nvmem;

	otp = devm_kzalloc(dev, sizeof(*otp), GFP_KERNEL);
	if (!otp) {
		dev_err(dev, "out of memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	otp->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(otp->base)) {
		dev_err(dev, "unable to map device memory\n");
		return PTR_ERR(otp->base);
	}

	otp->config.dev       = dev;
	otp->config.name      = "sifive-u500-otp";
	otp->config.read_only = false;
	otp->config.root_only = false; // true; /* writes permanently modify the SoC! */
	otp->config.reg_read  = sifive_u500_otp_reg_read;
	otp->config.reg_write = sifive_u500_otp_reg_write;
	otp->config.size      = 16384; /* in bytes */
	otp->config.word_size = 4;
	otp->config.stride    = 4;
	otp->config.priv      = otp;

	raw_spin_lock_init(&otp->lock);
	nvmem = nvmem_register(&otp->config);
	if (IS_ERR(nvmem)) {
		dev_err(dev, "error registering nvmem config\n");
		return PTR_ERR(nvmem);
	}

	platform_set_drvdata(pdev, nvmem);
	return 0;
}

static int sifive_u500_otp_remove(struct platform_device *pdev)
{
	struct nvmem_device *nvmem = platform_get_drvdata(pdev);

	return nvmem_unregister(nvmem);
}

static const struct of_device_id sifive_u500_otp_dt_ids[] = {
	{ .compatible = "sifive,ememoryotp0" },
	{ },
};
MODULE_DEVICE_TABLE(of, sifive_u500_otp_dt_ids);

static struct platform_driver sifive_u500_otp_driver = {
	.probe	= sifive_u500_otp_probe,
	.remove	= sifive_u500_otp_remove,
	.driver = {
		.name	= "sifive-u500-otp",
		.of_match_table = sifive_u500_otp_dt_ids,
	},
};
module_platform_driver(sifive_u500_otp_driver);

MODULE_DESCRIPTION("SiFive U500 OTP driver");
MODULE_LICENSE("GPL v2");
