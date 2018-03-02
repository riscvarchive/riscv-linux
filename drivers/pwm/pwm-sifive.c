/*
 * Copyright (C) 2018 SiFive, Inc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by
 * the Free Software Foundation.
 */

#include <dt-bindings/pwm/pwm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>

#define MAX_PWM			4

/* Register offsets */
#define REG_PWMCFG		0x0
#define REG_PWMCOUNT		0x8
#define REG_PWMS		0x10
#define	REG_PWMCMP0		0x20

/* PWMCFG fields */
#define BIT_PWM_SCALE		0
#define BIT_PWM_STICKY		8
#define BIT_PWM_ZERO_ZMP	9
#define BIT_PWM_DEGLITCH	10
#define BIT_PWM_EN_ALWAYS	12
#define BIT_PWM_EN_ONCE		13
#define BIT_PWM0_CENTER		16
#define BIT_PWM0_GANG		24
#define BIT_PWM0_IP		28

#define SIZE_PWMCMP		4
#define MASK_PWM_SCALE		0xf

struct sifive_pwm_device {
	struct pwm_chip		chip;
	struct notifier_block	notifier;
	struct clk		*clk;
	void __iomem		*regs;
	int 			irq;
	unsigned int		approx_period;
	unsigned int		real_period;
};

static inline struct sifive_pwm_device *chip_to_sifive(struct pwm_chip *c)
{
	return container_of(c, struct sifive_pwm_device, chip);
}

static inline struct sifive_pwm_device *notifier_to_sifive(struct notifier_block *nb)
{
	return container_of(nb, struct sifive_pwm_device, notifier);
}

static int sifive_pwm_apply(struct pwm_chip *chip, struct pwm_device *dev, struct pwm_state *state)
{
	struct sifive_pwm_device *pwm = chip_to_sifive(chip);
	unsigned int duty_cycle;
	u32 frac;

	duty_cycle = state->duty_cycle;
	if (!state->enabled) duty_cycle = 0;
	if (state->polarity == PWM_POLARITY_NORMAL) duty_cycle = state->period - duty_cycle;

	frac = ((u64)duty_cycle << 16) / state->period;
	frac = min(frac, 0xFFFFU);

	iowrite32(frac, pwm->regs + REG_PWMCMP0 + (dev->hwpwm * SIZE_PWMCMP));

	if (state->enabled) {
		state->period = pwm->real_period;
		state->duty_cycle = ((u64)frac * pwm->real_period) >> 16;
		if (state->polarity == PWM_POLARITY_NORMAL)
			state->duty_cycle = state->period - state->duty_cycle;
	}

	return 0;
}

static void sifive_pwm_get_state(struct pwm_chip *chip, struct pwm_device *dev, struct pwm_state *state)
{
	struct sifive_pwm_device *pwm = chip_to_sifive(chip);
	unsigned long duty;

	duty = ioread32(pwm->regs + REG_PWMCMP0 + (dev->hwpwm * SIZE_PWMCMP));

	state->period     = pwm->real_period;
	state->duty_cycle = ((u64)duty * pwm->real_period) >> 16;
	state->polarity   = PWM_POLARITY_INVERSED;
	state->enabled    = duty > 0;
}

static const struct pwm_ops sifive_pwm_ops = {
	.get_state	= sifive_pwm_get_state,
	.apply		= sifive_pwm_apply,
	.owner		= THIS_MODULE,
};

static struct pwm_device *sifive_pwm_xlate(struct pwm_chip *chip, const struct of_phandle_args *args)
{
	struct sifive_pwm_device *pwm = chip_to_sifive(chip);
	struct pwm_device *dev;

	if (args->args[0] >= chip->npwm)
		return ERR_PTR(-EINVAL);

	dev = pwm_request_from_chip(chip, args->args[0], NULL);
	if (IS_ERR(dev))
		return dev;

	/* The period cannot be changed on a per-PWM basis */
	dev->args.period   = pwm->real_period;
	dev->args.polarity = PWM_POLARITY_NORMAL;
	if (args->args[1] & PWM_POLARITY_INVERTED)
		dev->args.polarity = PWM_POLARITY_INVERSED;

	return dev;
}

static void sifive_pwm_update_clock(struct sifive_pwm_device *pwm, unsigned long rate)
{
	/* (1 << (16+scale)) * 10^9/rate = real_period */
	unsigned long scalePow = (pwm->approx_period * (u64)rate) / 1000000000;
	int scale = ilog2(scalePow) - 16;

	scale = clamp(scale, 0, 0xf);
	iowrite32((1 << BIT_PWM_EN_ALWAYS) | (scale << BIT_PWM_SCALE), pwm->regs + REG_PWMCFG);

	pwm->real_period = (1000000000ULL << (16 + scale)) / rate;
}

static int sifive_pwm_clock_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct sifive_pwm_device *pwm = notifier_to_sifive(nb);

	if (event == POST_RATE_CHANGE)
		sifive_pwm_update_clock(pwm, ndata->new_rate);

	return NOTIFY_OK;
}

static int sifive_pwm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct sifive_pwm_device *pwm;
	struct pwm_chip *chip;
	struct resource *res;
	int ret;

	pwm = devm_kzalloc(dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(dev, "Out of memory\n");
		return -ENOMEM;
	}

	chip = &pwm->chip;
	chip->dev = dev;
	chip->ops = &sifive_pwm_ops;
	chip->of_xlate = sifive_pwm_xlate;
	chip->of_pwm_n_cells = 2;
	chip->base = -1;

	ret = of_property_read_u32(node, "sifive,npwm", &chip->npwm);
	if (ret < 0 || chip->npwm > MAX_PWM) chip->npwm = MAX_PWM;

	ret = of_property_read_u32(node, "sifive,approx-period", &pwm->approx_period);
	if (ret < 0) {
		dev_err(dev, "Unable to read sifive,approx-period from DTS\n");
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pwm->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(pwm->regs)) {
		dev_err(dev, "Unable to map IO resources\n");
		return PTR_ERR(pwm->regs);
	}

	pwm->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(pwm->clk)) {
		dev_err(dev, "Unable to find controller clock\n");
		return PTR_ERR(pwm->clk);
	}

	pwm->irq = platform_get_irq(pdev, 0);
	if (pwm->irq < 0) {
		dev_err(dev, "Unable to find interrupt\n");
		return pwm->irq;
	}

	/* Watch for changes to underlying clock frequency */
	pwm->notifier.notifier_call = sifive_pwm_clock_notifier;
	clk_notifier_register(pwm->clk, &pwm->notifier);

	/* Initialize PWM config */
	sifive_pwm_update_clock(pwm, clk_get_rate(pwm->clk));

	/* No interrupt handler needed yet */
/*
	ret = devm_request_irq(dev, pwm->irq, sifive_pwm_irq, 0, dev_name(dev), pwm);
	if (ret) {
		dev_err(dev, "Unable to bind interrupt\n");
		return ret;
	}
*/

	ret = pwmchip_add(chip);
	if (ret < 0) {
		dev_err(dev, "cannot register PWM: %d\n", ret);
		clk_notifier_unregister(pwm->clk, &pwm->notifier);
		return ret;
	}

	platform_set_drvdata(pdev, pwm);
	dev_info(dev, "SiFive PWM chip registered %d PWMs\n", chip->npwm);

	return 0;
}

static int sifive_pwm_remove(struct platform_device *dev)
{
	struct sifive_pwm_device *pwm = platform_get_drvdata(dev);
	struct pwm_chip *chip = &pwm->chip;

	clk_notifier_unregister(pwm->clk, &pwm->notifier);
	return pwmchip_remove(chip);
}

static const struct of_device_id sifive_pwm_of_match[] = {
	{ .compatible = "sifive,pwm0" },
	{},
};
MODULE_DEVICE_TABLE(of, sifive_pwm_of_match);

static struct platform_driver sifive_pwm_driver = {
	.probe = sifive_pwm_probe,
	.remove = sifive_pwm_remove,
	.driver = {
		.name = "pwm-sifivem",
		.of_match_table = of_match_ptr(sifive_pwm_of_match),
	},
};
module_platform_driver(sifive_pwm_driver);

MODULE_DESCRIPTION("SiFive PWM driver");
MODULE_LICENSE("GPL v2");
