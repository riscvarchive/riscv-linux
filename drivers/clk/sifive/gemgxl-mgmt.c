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
#include <linux/err.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

struct sifive_gemgxl_mgmt {
	void __iomem *reg;
	unsigned long rate;
	struct clk_hw hw;
};

#define to_sifive_gemgxl_mgmt(mgmt) container_of(mgmt, struct sifive_gemgxl_mgmt, hw)

static unsigned long sifive_gemgxl_mgmt_recalc_rate(struct clk_hw *hw,
				      unsigned long parent_rate)
{
	struct sifive_gemgxl_mgmt *mgmt = to_sifive_gemgxl_mgmt(hw);
	return mgmt->rate;
}

static long sifive_gemgxl_mgmt_round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *parent_rate)
{
	if (WARN_ON(rate < 2500000)) {
		return 2500000;
	} else if (rate == 2500000) {
		return 2500000;
	} else if (WARN_ON(rate < 13750000)) {
		return 2500000;
	} else if (WARN_ON(rate < 25000000)) {
		return 25000000;
	} else if (rate == 25000000) {
		return 25000000;
	} else if (WARN_ON(rate < 75000000)) {
		return 25000000;
	} else if (WARN_ON(rate < 125000000)) {
		return 125000000;
	} else if (rate == 125000000) {
		return 125000000;
	} else {
		WARN_ON(rate > 125000000);
		return 125000000;
	}
}

static int sifive_gemgxl_mgmt_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct sifive_gemgxl_mgmt *mgmt = to_sifive_gemgxl_mgmt(hw);
	rate = sifive_gemgxl_mgmt_round_rate(hw, rate, &parent_rate);
	iowrite32(rate != 125000000, mgmt->reg);
	mgmt->rate = rate;
	return 0;
}

static const struct clk_ops sifive_gemgxl_mgmt_ops = {
	.recalc_rate = sifive_gemgxl_mgmt_recalc_rate,
	.round_rate = sifive_gemgxl_mgmt_round_rate,
	.set_rate = sifive_gemgxl_mgmt_set_rate,
};

static int sifive_gemgxl_mgmt_probe(struct platform_device *pdev)
{
	struct clk_init_data init;
	struct sifive_gemgxl_mgmt *mgmt;
	struct resource *res;
	struct clk *clk;

	mgmt = devm_kzalloc(&pdev->dev, sizeof(*mgmt), GFP_KERNEL);
	if (!mgmt)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mgmt->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mgmt->reg))
		return PTR_ERR(mgmt->reg);

	init.name = pdev->dev.of_node->name;
	init.ops = &sifive_gemgxl_mgmt_ops;
	init.flags = 0;
	init.num_parents = 0;

	mgmt->rate = 0;
	mgmt->hw.init = &init;

	clk = clk_register(NULL, &mgmt->hw);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	of_clk_add_provider(pdev->dev.of_node, of_clk_src_simple_get, clk);

	dev_info(&pdev->dev, "Registered clock switch '%s'\n", init.name);

	return 0;
}

static const struct of_device_id sifive_gemgxl_mgmt_of_match[] = {
	{ .compatible = "sifive,cadencegemgxlmgmt0", },
	{}
};

static struct platform_driver sifive_gemgxl_mgmt_driver = {
	.driver	= {
		.name = "sifive-gemgxl-mgmt",
		.of_match_table = sifive_gemgxl_mgmt_of_match,
	},
	.probe = sifive_gemgxl_mgmt_probe,
};

static int __init sifive_gemgxl_mgmt_init(void)
{
	return platform_driver_register(&sifive_gemgxl_mgmt_driver);
}
core_initcall(sifive_gemgxl_mgmt_init);
