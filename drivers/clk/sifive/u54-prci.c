/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2018 SiFive, Inc.
 */

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/log2.h>

#define CORE_CLOCK 0
#define GEMTX_CLOCK 1
#define PRCI_CLOCKS 2

#define MIN_REF 7000000ULL
#define MAX_REF 200000000ULL
#define MAX_PARENT 600000000ULL
#define MAX_VCO 4800000000ULL
#define MAX_DIV 64ULL
#define MAX_R 64ULL

#define PLL_LOCK 0x80000000ULL
#define NAME_LEN 40ULL

struct sifive_u54_prci_driver;

struct sifive_u54_prci_pll {
	struct clk_hw hw;
	struct sifive_u54_prci_driver *driver;
	char name[NAME_LEN];
	u32 freq;
	u32 glcm;
};

struct sifive_u54_prci_driver {
	struct clk_onecell_data table;
	struct clk *clks[PRCI_CLOCKS];
	struct sifive_u54_prci_pll plls[PRCI_CLOCKS];
	void __iomem *reg;
};

#define to_sifive_u54_prci_pll(hw) container_of(hw, struct sifive_u54_prci_pll, hw)

struct sifive_u54_pll_cfg {
	unsigned long r, f, q, a;
};

static struct sifive_u54_pll_cfg sifive_u54_pll_cfg(u32 reg)
{
	struct sifive_u54_pll_cfg cfg;
	cfg.r = (reg >>  0) & 0x3f;
	cfg.f = (reg >>  6) & 0x1ff;
	cfg.q = (reg >> 15) & 0x7;
	cfg.a = (reg >> 18) & 0x7;
	return cfg;
}

static u32 sifive_u54_pll_reg(struct sifive_u54_pll_cfg cfg)
{
	u32 reg = 0;
	reg |= (cfg.r & 0x3f)  << 0;
	reg |= (cfg.f & 0x1ff) << 6;
	reg |= (cfg.q & 0x7)   << 15;
	reg |= (cfg.a & 0x7)   << 18;
	reg |= 1<<25; // internal feedback
	return reg;
}

static unsigned long sifive_u54_pll_rate(struct sifive_u54_pll_cfg cfg, unsigned long parent)
{
	return (parent*2*(cfg.f+1) / (cfg.r+1)) >> cfg.q;
}

static struct sifive_u54_pll_cfg sifive_u54_pll_configure(unsigned long target, unsigned long parent)
{
	struct sifive_u54_pll_cfg cfg;
	unsigned long scale, ratio, best_delta, filter;
	u32 max_r, best_r, best_f, r;

	/* Confirm input frequency is within bounds */
	if (WARN_ON(parent > MAX_PARENT)) { parent = MAX_PARENT; }
	if (WARN_ON(parent < MIN_REF))    { parent = MIN_REF; }

	/* Calculate the Q shift and target VCO */
	scale = MAX_VCO / target;
	if (scale <= 1) {
		cfg.q = 1;
		target = MAX_VCO;
	} else if (scale > MAX_DIV) {
		cfg.q = ilog2(MAX_DIV);
		target = MAX_VCO/2;
	} else {
		cfg.q = ilog2(scale);
		target = target << cfg.q;
	}

	/* Precalcualte the target ratio */
	ratio = (target << 20) / parent;

	/* Placeholder values */
	best_r = 0;
	best_f = 0;
	best_delta = MAX_VCO;

	/* Consider all values for R which land within [MIN_REF, MAX_REF]; prefer smaller R */
	max_r = min(MAX_R, parent / MIN_REF);
	for (r = DIV_ROUND_UP(parent, MAX_REF); r <= max_r; ++r) {
		/* What is the best F we can pick in this case? */
		u32 f = (ratio*r + (1<<20)) >> 21;
		unsigned long ref = parent / r;
		unsigned long vco = ref * f * 2;
		unsigned long delta;

		/* Ensure rounding didn't take us out of range */
		if (vco > target) --f;
		if (vco < MAX_VCO/2) ++f;
		vco = ref * f * 2;

		delta = abs(target - vco);
		if (delta < best_delta) {
			best_delta = delta;
			best_r = r;
			best_f = f;
		}
	}

	cfg.r = best_r - 1;
	cfg.f = best_f - 1;

	/* Pick the best PLL jitter filter */
	filter = parent / best_r;
	BUG_ON(filter < 7000000);
	if (filter < 11000000) {
		cfg.a = 1;
	} else if (filter < 18000000) {
		cfg.a = 2;
	} else if (filter < 30000000) {
		cfg.a = 3;
	} else if (filter < 50000000) {
		cfg.a = 4;
	} else if (filter < 80000000) {
		cfg.a = 5;
	} else if (filter < 130000000) {
		cfg.a = 6;
	} else {
		BUG_ON (filter > 200000000);
		cfg.a = 7;
	}

	return cfg;
}

static unsigned long sifive_u54_prci_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct sifive_u54_prci_pll *pll = to_sifive_u54_prci_pll(hw);
	struct sifive_u54_prci_driver *driver = pll->driver;

	u32 reg = ioread32(driver->reg + pll->freq);
	struct sifive_u54_pll_cfg cfg = sifive_u54_pll_cfg(reg);

	return sifive_u54_pll_rate(cfg, parent_rate);
}

static long sifive_u54_prci_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *parent_rate)
{
	struct sifive_u54_pll_cfg cfg = sifive_u54_pll_configure(rate, *parent_rate);
	return sifive_u54_pll_rate(cfg, *parent_rate);
}

static int sifive_u54_prci_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	struct sifive_u54_prci_pll *pll = to_sifive_u54_prci_pll(hw);
	struct sifive_u54_prci_driver *driver = pll->driver;

	struct sifive_u54_pll_cfg cfg = sifive_u54_pll_configure(rate, parent_rate);
	u32 reg = sifive_u54_pll_reg(cfg);

	/* Switch to reg clock and reconfigure PLL */
	iowrite32(1, driver->reg + pll->glcm);
	iowrite32(reg, driver->reg + pll->freq);

	/* Wait for lock and switch back to PLL */
	while (!(ioread32(driver->reg + pll->freq) & PLL_LOCK));
	iowrite32(0, driver->reg + pll->glcm);

	return 0;
}

static const struct clk_ops sifive_u54_prci_ops_rw = {
	.recalc_rate = sifive_u54_prci_recalc_rate,
	.round_rate = sifive_u54_prci_round_rate,
	.set_rate = sifive_u54_prci_set_rate,
};

static const struct clk_ops sifive_u54_prci_ops_ro = {
	.recalc_rate = sifive_u54_prci_recalc_rate,
};

static ssize_t sifive_u54_pll_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sifive_u54_prci_driver *driver = dev_get_drvdata(dev);
	return sprintf(buf, "%ld", clk_get_rate(driver->clks[0]));
}

static ssize_t sifive_u54_pll_rate_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sifive_u54_prci_driver *driver = dev_get_drvdata(dev);
	unsigned long rate;
	char *endp;

	rate = simple_strtoul(buf, &endp, 0);
	if (*endp != 0 && *endp != '\n')
		return -EINVAL;

	clk_set_rate(driver->clks[0], rate);
	return count;
}

static DEVICE_ATTR(rate, 0644, sifive_u54_pll_show, sifive_u54_pll_rate_store);

static int sifive_u54_prci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk_init_data init;
	struct sifive_u54_prci_driver *driver;
	struct resource *res;
	const char *parent;
	int i;

	parent = of_clk_get_parent_name(dev->of_node, 0);
	if (!parent) {
		dev_err(dev, "No OF parent clocks found\n");
		return -EINVAL;
	}

	driver = devm_kzalloc(dev, sizeof(*driver), GFP_KERNEL);
	if (!driver) {
		dev_err(dev, "Out of memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	driver->reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(driver->reg))
		return PTR_ERR(driver->reg);

	/* Link the data structure */
	driver->table.clk_num = PRCI_CLOCKS;
	driver->table.clks = &driver->clks[0];
	dev_set_drvdata(dev, driver);

	/* Describe the clocks */
	snprintf(driver->plls[CORE_CLOCK].name, NAME_LEN, "%s.core", dev->of_node->name);
	driver->plls[CORE_CLOCK].freq = 0x4;
	driver->plls[CORE_CLOCK].glcm = 0x24;
	snprintf(driver->plls[GEMTX_CLOCK].name, NAME_LEN, "%s.gemtx", dev->of_node->name);
	driver->plls[GEMTX_CLOCK].freq = 0x1c;
	driver->plls[GEMTX_CLOCK].glcm = 0; /* None; cannot be set_rate */

	/* Export the clocks */
	for (i = 0; i < PRCI_CLOCKS; ++i) {
		init.name = &driver->plls[i].name[0];
		init.ops = driver->plls[i].glcm ? &sifive_u54_prci_ops_rw : &sifive_u54_prci_ops_ro;
		init.num_parents = 1;
		init.parent_names = &parent;
		init.flags = 0;

		driver->plls[i].driver = driver;
		driver->plls[i].hw.init = &init;

		driver->clks[i] = devm_clk_register(dev, &driver->plls[i].hw);
		if (IS_ERR(driver->clks[i])) {
			dev_err(dev, "Failed to register clock %d, %ld\n", i, PTR_ERR(driver->clks[i]));
			return PTR_ERR(driver->clks[i]);
		}
	}

	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &driver->table);
	device_create_file(dev, &dev_attr_rate);
	dev_info(dev, "Registered U54 core clocks\n");

	return 0;
}

static const struct of_device_id sifive_u54_prci_of_match[] = {
	{ .compatible = "sifive,aloeprci0", },
	{ .compatible = "sifive,ux00prci0", },
	{}
};

static struct platform_driver sifive_u54_prci_driver = {
	.driver	= {
		.name = "sifive-u54-prci",
		.of_match_table = sifive_u54_prci_of_match,
	},
	.probe = sifive_u54_prci_probe,
};

static int __init sifive_u54_prci_init(void)
{
	return platform_driver_register(&sifive_u54_prci_driver);
}
core_initcall(sifive_u54_prci_init);
