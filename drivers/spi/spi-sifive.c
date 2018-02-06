/*
 * SiFive SPI controller driver (master mode only)
 *
 * Author: SiFive, Inc.
 *	sifive@sifive.com
 *
 * 2018 (c) SiFive Software, Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>

#define SIFIVE_SPI_MAX_CS	32

#define SIFIVE_SPI_NAME "sifive_spi"

#define XSPI_SCDR_OFFSET	0x000	/* Serial Clock Divisor Register */
#define XSPI_SCD_SCALE_MASK	0xFFF

#define XSPI_SCMR_OFFSET        0x004   /* Serial Clock Mode Register */
#define XSPI_SCM_CPHA		1
#define XSPI_SCM_CPOL		2
#define XSPI_SCM_MODE_MASK     (XSPI_SCM_CPHA | XSPI_SCM_CPOL)

#define XSPI_CSIDR_OFFSET       0x010
#define XSPI_CSDR_OFFSET        0x014
#define XSPI_CSMR_OFFSET        0x018
#define XSPI_CSM_MODE_AUTO      0
#define XSPI_CSM_MODE_HOLD      2
#define XSPI_CSM_MODE_OFF       3

#define XSPI_DC0R_OFFSET        0x028
#define XSPI_CS_TO_SCK_MASK     0xFF
#define XSPI_SCK_TO_CS_MASK     (0xFF << 16)
#define XSPI_DC1R_OFFSET        0x02C
#define XSPI_MIN_CS_IATIME_MASK 0xFF
#define XSPI_MAX_IF_DELAY_MASK  (0xFF << 16)

#define XSPI_FFR_OFFSET         0x040
#define XSPI_FF_SINGLE          0
#define XSPI_FF_DUAL            1
#define XSPI_FF_QUAD            2
#define XSPI_FF_SPI_MASK        0x3
#define XSPI_FF_LSB_FIRST       4
#define XSPI_FF_TX_DIR          8
#define XSPI_FF_BPF_MASK        (0xFF << 16)

#define XSPI_TXDR_OFFSET	0x048	/* Data Transmit Register */
#define XSPI_TXD_FIFO_FULL      (8U << 28)
#define XSPI_RXDR_OFFSET	0x04C	/* Data Receive Register */
#define XSPI_RXD_FIFO_EMPTY     (8U << 28)
#define XSPI_DATA_MASK          0xFF
#define XSPI_DATA_SHIFT         8

#define XSPI_TXWMR_OFFSET       0x050   /* TX FIFO Watermark Register */
#define XSPI_RXWMR_OFFSET       0x054   /* RX FIFO Watermark Register */

#define XSPI_FCTRL_OFFSET	0x60

#define XSPI_IPR_OFFSET		0x074	/* Interrupt Pendings Register */
#define XSPI_IER_OFFSET		0x070	/* Interrupt Enable Register */
#define XSPI_TXWM_INTR          0x1
#define XSPI_RXWM_INTR          0x2

struct sifive_spi {
	void __iomem	*regs;		/* virt. address of the control registers */
	struct clk	*clk;		/* bus clock */
	int		irq;		/* watermark irq */
	int 		buffer_size;	/* buffer size in words */
	u32		cs_inactive;	/* Level of the CS pins when inactive*/
	struct completion done;		/* Wake-up from interrupt */
};

static void sifive_spi_write(struct sifive_spi *spi, int offset, u32 value)
{
	iowrite32(value, spi->regs + offset);
}

static u32 sifive_spi_read(struct sifive_spi *spi, int offset)
{
	return ioread32(spi->regs + offset);
}

static void sifive_spi_init(struct sifive_spi *spi)
{
	/* Watermark interrupts are disabled by default */
	sifive_spi_write(spi, XSPI_IER_OFFSET, 0);

	/* Default watermark FIFO threshold values */
	sifive_spi_write(spi, XSPI_TXWMR_OFFSET, 1);
	sifive_spi_write(spi, XSPI_RXWMR_OFFSET, 0);

	/* !!! Program any CS/SCK Delays and Inactive Time */

	/* Exit specialized memory-mapped SPI flash mode */
	sifive_spi_write(spi, XSPI_FCTRL_OFFSET, 0);
}

static void sifive_spi_prep_device(struct sifive_spi *spi, struct spi_device *device)
{
	u32 cr;

	/* Update the chip select polarity */
	if (device->mode & SPI_CS_HIGH)
		spi->cs_inactive &= ~BIT(device->chip_select);
	else
		spi->cs_inactive |= BIT(device->chip_select);
	sifive_spi_write(spi, XSPI_CSDR_OFFSET, spi->cs_inactive);

	/* Select the correct device */
	sifive_spi_write(spi, XSPI_CSIDR_OFFSET, device->chip_select);

	/* Switch clock mode bits */
	cr = sifive_spi_read(spi, XSPI_SCMR_OFFSET) & ~XSPI_SCM_MODE_MASK;
	if (device->mode & SPI_CPHA)
		cr |= XSPI_SCM_CPHA;
	if (device->mode & SPI_CPOL)
		cr |= XSPI_SCM_CPOL;
	sifive_spi_write(spi, XSPI_SCMR_OFFSET, cr);
}

static int sifive_spi_prep_transfer(struct sifive_spi *spi, struct spi_device *device, struct spi_transfer *t)
{
	u32 hz, scale, cr;
	int mode;

	/* Calculate and program the clock rate */
	hz = t->speed_hz ? t->speed_hz : device->max_speed_hz;
	scale = (DIV_ROUND_UP(clk_get_rate(spi->clk) >> 1, hz) - 1) & XSPI_SCD_SCALE_MASK;
	sifive_spi_write(spi, XSPI_SCDR_OFFSET, scale);

	/* Modify the SPI protocol mode */
	cr = sifive_spi_read(spi, XSPI_FFR_OFFSET);

	/* LSB first? */
	cr &= ~XSPI_FF_LSB_FIRST;
	if (device->mode & SPI_LSB_FIRST)
		cr |= XSPI_FF_LSB_FIRST;

	/* SINGLE/DUAL/QUAD? */
	mode = max((int)t->rx_nbits, (int)t->tx_nbits);
	cr &= ~XSPI_FF_SPI_MASK;
	switch (mode) {
		case SPI_NBITS_QUAD: cr |= XSPI_FF_QUAD;   break;
		case SPI_NBITS_DUAL: cr |= XSPI_FF_DUAL;   break;
		default:             cr |= XSPI_FF_SINGLE; break;
	}

	/* SPI direction */
	cr &= ~XSPI_FF_TX_DIR;
	if (!t->rx_buf)
		cr |= XSPI_FF_TX_DIR;

	sifive_spi_write(spi, XSPI_FFR_OFFSET, cr);

	/* We will want to poll if the time we need to wait is less than the context switching time.
	 * Let's call that threshold 5us. The operation will take:
	 *    (8/mode) * buffer_size / hz <= 5 * 10^-6
	 *    1600000 * buffer_size <= hz * mode
	 */
	return 1600000 * spi->buffer_size <= hz * mode;
}

static void sifive_spi_tx(struct sifive_spi *spi, const u8* tx_ptr)
{
	BUG_ON((sifive_spi_read(spi, XSPI_TXDR_OFFSET) & XSPI_TXD_FIFO_FULL) != 0);
	sifive_spi_write(spi, XSPI_TXDR_OFFSET, *tx_ptr & XSPI_DATA_MASK);
}

static void sifive_spi_rx(struct sifive_spi *spi, u8* rx_ptr)
{
        u32 data = sifive_spi_read(spi, XSPI_RXDR_OFFSET);
        BUG_ON((data & XSPI_RXD_FIFO_EMPTY) != 0);
        *rx_ptr = data & XSPI_DATA_MASK;
}

static void sifive_spi_wait(struct sifive_spi *spi, int bit, int poll)
{
	if (poll) {
		u32 cr;
		do cr = sifive_spi_read(spi, XSPI_IPR_OFFSET);
		while (!(cr & bit));
	} else {
		reinit_completion(&spi->done);
		sifive_spi_write(spi, XSPI_IER_OFFSET, bit);
		wait_for_completion(&spi->done);
	}
}

static void sifive_spi_execute(struct sifive_spi *spi, struct spi_transfer *t, int poll)
{
	int remaining_words = t->len;
	const u8* tx_ptr = t->tx_buf;
	u8* rx_ptr = t->rx_buf;

	while (remaining_words) {
		int n_words, tx_words, rx_words;
		n_words = min(remaining_words, spi->buffer_size);

		/* Enqueue n_words for transmission */
		for (tx_words = 0; tx_words < n_words; ++tx_words)
			sifive_spi_tx(spi, tx_ptr++);

		if (rx_ptr) {
			/* Wait for transmission + reception to complete */
			sifive_spi_write(spi, XSPI_RXWMR_OFFSET, n_words-1);
			sifive_spi_wait(spi, XSPI_RXWM_INTR, poll);

			/* Read out all the data from the RX FIFO */
			for (rx_words = 0; rx_words < n_words; ++rx_words)
				sifive_spi_rx(spi, rx_ptr++);
		} else {
			/* Wait for transmission to complete */
			sifive_spi_wait(spi, XSPI_TXWM_INTR, poll);
		}

		remaining_words -= n_words;
	}
}

static int sifive_spi_transfer_one_message(struct spi_master *master, struct spi_message *msg)
{
	struct sifive_spi *spi = spi_master_get_devdata(master);
	struct spi_device *device = msg->spi;
	struct spi_transfer *t;
	int is_last, cs_change, cs_enable, poll;

	/* Setup device-specific properties */
	sifive_spi_prep_device(spi, device);

	sifive_spi_write(spi, XSPI_CSMR_OFFSET, XSPI_CSM_MODE_HOLD);
	list_for_each_entry(t, &msg->transfers, transfer_list) {
		/* Setup transfer-specific properties */
		poll = sifive_spi_prep_transfer(spi, device, t);
		sifive_spi_execute(spi, t, poll);
		msg->actual_length += t->len;

		is_last = list_is_last(&t->transfer_list, &msg->transfers);
		cs_change = !!t->cs_change;
		cs_enable = !is_last ^ cs_change;
		sifive_spi_write(spi, XSPI_CSMR_OFFSET, cs_enable ? XSPI_CSM_MODE_HOLD : XSPI_CSM_MODE_AUTO);
	}

	msg->status = 0;
	spi_finalize_current_message(master);

	return 0;
}

static irqreturn_t sifive_spi_irq(int irq, void *dev_id)
{
	struct sifive_spi *spi = dev_id;
	u32 ip;

	ip = sifive_spi_read(spi, XSPI_IPR_OFFSET) & (XSPI_TXWM_INTR | XSPI_RXWM_INTR);
	if (ip != 0) {
		/* Disable interrupts until next transfer */
		sifive_spi_write(spi, XSPI_IER_OFFSET, 0);
		complete(&spi->done);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int sifive_spi_probe(struct platform_device *pdev)
{
	struct sifive_spi *spi;
	struct resource *res;
	int ret, num_cs, bits_per_word = 8;
	u32 cs_bits;
	struct spi_master *master;

	master = spi_alloc_master(&pdev->dev, sizeof(struct sifive_spi));
	if (!master) {
		dev_err(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	spi = spi_master_get_devdata(master);
	init_completion(&spi->done);
	platform_set_drvdata(pdev, master);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	spi->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(spi->regs)) {
		dev_err(&pdev->dev, "Unable to map IO resources\n");
		ret = PTR_ERR(spi->regs);
		goto put_master;
	}

	spi->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(spi->clk)) {
		dev_err(&pdev->dev, "Unable to find bus clock\n");
		ret = PTR_ERR(spi->clk);
		goto put_master;
	}

	spi->irq = platform_get_irq(pdev, 0);
	if (spi->irq < 0) {
		dev_err(&pdev->dev, "Unable to find interrupt\n");
		ret = spi->irq;
		goto put_master;
	}

	/* !!! This should be in DTS? */
	spi->buffer_size = 8;

	/* Spin up the bus clock before hitting registers */
	ret = clk_prepare_enable(spi->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable bus clock\n");
		goto put_master;
	}

	/* probe the number of CS lines */
	spi->cs_inactive = sifive_spi_read(spi, XSPI_CSDR_OFFSET);
	sifive_spi_write(spi, XSPI_CSDR_OFFSET, 0xffffffffU);
	cs_bits = sifive_spi_read(spi, XSPI_CSDR_OFFSET);
	sifive_spi_write(spi, XSPI_CSDR_OFFSET, spi->cs_inactive);
	if (!cs_bits) {
		dev_err(&pdev->dev, "Could not auto probe CS lines\n");
		ret = -EINVAL;
		goto put_master;
	}

	num_cs = ilog2(cs_bits) + 1;
	if (num_cs > SIFIVE_SPI_MAX_CS) {
		dev_err(&pdev->dev, "Invalid number of spi slaves\n");
		ret = -EINVAL;
		goto put_master;
	}

	/* Define our master */
	master->bus_num = pdev->id;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_CS_HIGH |
	                    SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD;
	master->flags = SPI_CONTROLLER_MUST_TX;
	master->dev.of_node = pdev->dev.of_node;
	master->bits_per_word_mask = SPI_BPW_MASK(bits_per_word);
	master->num_chipselect = num_cs;
	master->transfer_one_message = sifive_spi_transfer_one_message;

	/* Configure the SPI master hardware */
	sifive_spi_init(spi);

	/* Register for SPI Interrupt */
	ret = devm_request_irq(&pdev->dev, spi->irq, sifive_spi_irq, 0,
				dev_name(&pdev->dev), spi);
	if (ret) {
		dev_err(&pdev->dev, "Unable to bind to interrupt\n");
		goto put_master;
	}

	dev_info(&pdev->dev, "mapped; irq=%d, cs=%d\n",
		spi->irq, master->num_chipselect);

	ret = devm_spi_register_master(&pdev->dev, master);
	if (ret < 0) {
		dev_err(&pdev->dev, "spi_register_master failed\n");
		goto put_master;
	}

	return 0;

put_master:
	spi_master_put(master);

	return ret;
}

static int sifive_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct sifive_spi *spi = spi_master_get_devdata(master);

	/* Disable all the interrupts just in case */
	sifive_spi_write(spi, XSPI_IER_OFFSET, 0);
	spi_master_put(master);

	return 0;
}

static const struct of_device_id sifive_spi_of_match[] = {
	{ .compatible = "sifive,spi0", },
	{}
};
MODULE_DEVICE_TABLE(of, sifive_spi_of_match);

static struct platform_driver sifive_spi_driver = {
	.probe = sifive_spi_probe,
	.remove = sifive_spi_remove,
	.driver = {
		.name = SIFIVE_SPI_NAME,
		.of_match_table = sifive_spi_of_match,
	},
};
module_platform_driver(sifive_spi_driver);

MODULE_AUTHOR("SiFive, Inc. <sifive@sifive.com>");
MODULE_DESCRIPTION("SiFive SPI driver");
MODULE_LICENSE("GPL");
