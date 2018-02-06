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
#include <linux/spi/spi_bitbang.h>
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
	/* bitbang has to be first */
	struct spi_bitbang bitbang;
	struct completion done;
	void __iomem	*regs;	/* virt. address of the control registers */

	int		irq;

	u8 *rx_ptr;		/* pointer in the Tx buffer */
	const u8 *tx_ptr;	/* pointer in the Rx buffer */
	struct clk *clk;	/* bus clock */
	int buffer_size;	/* buffer size in words */
	u32 cs_inactive;	/* Level of the CS pins when inactive*/
	unsigned int (*read_fn)(void __iomem *);
	void (*write_fn)(u32, void __iomem *);
};

static void xspi_write32(u32 val, void __iomem *addr)
{
	iowrite32(val, addr);
}

static unsigned int xspi_read32(void __iomem *addr)
{
	return ioread32(addr);
}

static void sifive_spi_tx(struct sifive_spi *xspi)
{
	if (!xspi->tx_ptr) {
		xspi->write_fn(0, xspi->regs + XSPI_TXDR_OFFSET);
		return;
	}

	BUG_ON((xspi->read_fn(xspi->regs + XSPI_TXDR_OFFSET) & XSPI_TXD_FIFO_FULL) != 0);

	xspi->write_fn(*(u8 *)(xspi->tx_ptr) & XSPI_DATA_MASK, xspi->regs + XSPI_TXDR_OFFSET);
	++xspi->tx_ptr;
}

static void sifive_spi_rx(struct sifive_spi *xspi)
{
        u32 data = xspi->read_fn(xspi->regs + XSPI_RXDR_OFFSET);
        BUG_ON((data & XSPI_RXD_FIFO_EMPTY) != 0);

	if (!xspi->rx_ptr)
		return;

        *(u8 *)(xspi->rx_ptr) = data & XSPI_DATA_MASK;
	++xspi->rx_ptr;
}

static void xspi_init_hw(struct sifive_spi *xspi)
{
	void __iomem *regs_base = xspi->regs;

	/* Watermark interrupts are disabled by default */
	xspi->write_fn(0, regs_base + XSPI_IER_OFFSET);

	/* Default watermark FIFO threshold values */
	xspi->write_fn(1, regs_base + XSPI_TXWMR_OFFSET);
	xspi->write_fn(0, regs_base + XSPI_RXWMR_OFFSET);

	/* Programe any CS/SCK Delays and Inactive Time*/
        //xspi->write_fn(0x10001, regs_base + XSPI_DCR0_OFFSET);
        //xspi->write_fn(0x1, regs_base + XSPI_DCR1_OFFSET);

	/* Exit specialized memory-mapped SPI flash mode */
	xspi->write_fn(0, regs_base + XSPI_FCTRL_OFFSET);
}

static void sifive_spi_chipselect(struct spi_device *spi, int is_on)
{
	struct sifive_spi *xspi = spi_master_get_devdata(spi->master);

	if (is_on == BITBANG_CS_INACTIVE) return;

	/* Activate the chip select */
	xspi->write_fn(spi->chip_select, xspi->regs + XSPI_CSIDR_OFFSET);
}

/* spi_bitbang requires custom setup_transfer() to be defined if there is a
 * custom txrx_bufs().
 */
static int sifive_spi_setup_transfer(struct spi_device *spi,
		struct spi_transfer *t)
{
	struct sifive_spi *xspi = spi_master_get_devdata(spi->master);

	if (spi->mode & SPI_CS_HIGH)
		xspi->cs_inactive &= ~BIT(spi->chip_select);
	else
		xspi->cs_inactive |= BIT(spi->chip_select);

	return 0;
}

static int sifive_spi_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct sifive_spi *xspi = spi_master_get_devdata(spi->master);
	int remaining_words;	/* the number of words left to transfer */
	u32 cr = 0;
	u8 bpw = 0;
	u32 hz = 0;
	int scale;

	/* Calculate and program the clock rate */
	if (t) {
	  bpw = t->bits_per_word;
	  hz = t->speed_hz;
	}
	hz = hz ? : spi->max_speed_hz;
	scale = (DIV_ROUND_UP(clk_get_rate(xspi->clk), 2 * hz) - 1) & XSPI_SCD_SCALE_MASK;
	xspi->write_fn(scale, xspi->regs + XSPI_SCDR_OFFSET);

	/* Set the SPI clock phase and polarity */
	cr = xspi->read_fn(xspi->regs + XSPI_SCMR_OFFSET) & ~XSPI_SCM_MODE_MASK;
	if (spi->mode & SPI_CPHA)
	  cr |= XSPI_SCM_CPHA;
	if (spi->mode & SPI_CPOL)
	  cr |= XSPI_SCM_CPOL;
	xspi->write_fn(cr, xspi->regs + XSPI_SCMR_OFFSET);

	cr = xspi->read_fn(xspi->regs + XSPI_FFR_OFFSET) & ~XSPI_FF_LSB_FIRST;
	if (spi->mode & SPI_LSB_FIRST)
	  cr |= XSPI_FF_LSB_FIRST;
	xspi->write_fn(cr, xspi->regs + XSPI_FFR_OFFSET);

	xspi->tx_ptr = t->tx_buf;
	xspi->rx_ptr = t->rx_buf;
	remaining_words = t->len;

	/* RXM and TXM interrupts must already be disabled */
	cr = xspi->read_fn(xspi->regs + XSPI_IER_OFFSET);
	BUG_ON((cr & (XSPI_TXWM_INTR|XSPI_RXWM_INTR)) != 0);

	/* Don't lower CS mid-message */
	xspi->write_fn(XSPI_CSM_MODE_HOLD, xspi->regs + XSPI_CSMR_OFFSET);

	while (remaining_words) {
		int n_words, tx_words, rx_words;

		n_words = min(remaining_words, xspi->buffer_size);

		/* Enqueue n_words for transmission */
		for (tx_words = 0; tx_words < n_words; ++tx_words)
			sifive_spi_tx(xspi);

		/* Wait for transmission + reception to complete */
		reinit_completion(&xspi->done);
		xspi->write_fn(n_words-1, xspi->regs + XSPI_RXWMR_OFFSET);
		xspi->write_fn(XSPI_RXWM_INTR, xspi->regs + XSPI_IER_OFFSET);
		wait_for_completion(&xspi->done);

		/* Read out all the data from the Rx FIFO */
		for (rx_words = 0; rx_words < n_words; ++rx_words)
			sifive_spi_rx(xspi);

		remaining_words -= n_words;
	}

	/* If this is the last transfer in the message and we're done, lower CS */
	if (t->cs_change)
		xspi->write_fn(XSPI_CSM_MODE_AUTO, xspi->regs + XSPI_CSMR_OFFSET);

	return t->len;
}


/*
 * Watermark threshold interrupts
 */
static irqreturn_t sifive_spi_irq(int irq, void *dev_id)
{
	struct sifive_spi *xspi = dev_id;
	u32 ip;

	ip = xspi->read_fn(xspi->regs + XSPI_IPR_OFFSET) & (XSPI_TXWM_INTR | XSPI_RXWM_INTR);
	if (ip != 0) {
		/* Disable interrupts until next transfer */
		xspi->write_fn(0, xspi->regs + XSPI_IER_OFFSET);
		complete(&xspi->done);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static const struct of_device_id sifive_spi_of_match[] = {
	{ .compatible = "sifive,spi0", },
	{}
};
MODULE_DEVICE_TABLE(of, sifive_spi_of_match);

static int sifive_spi_probe(struct platform_device *pdev)
{
	struct sifive_spi *xspi;
	struct resource *res;
	int ret, num_cs, bits_per_word = 8;
	long cs_bits;
	struct spi_master *master;

	master = spi_alloc_master(&pdev->dev, sizeof(struct sifive_spi));
	if (!master) {
		dev_err(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	xspi = spi_master_get_devdata(master);
	xspi->cs_inactive = 0xffffffff;
	xspi->bitbang.master = master;
	xspi->bitbang.chipselect = sifive_spi_chipselect;
	xspi->bitbang.setup_transfer = sifive_spi_setup_transfer;
	xspi->bitbang.txrx_bufs = sifive_spi_txrx_bufs;
	init_completion(&xspi->done);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xspi->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(xspi->regs)) {
		dev_err(&pdev->dev, "Unable to map IO resources\n");
		ret = PTR_ERR(xspi->regs);
		goto put_master;
	}

	xspi->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(xspi->clk)) {
		dev_err(&pdev->dev, "Unable to find bus clock\n");
		ret = PTR_ERR(xspi->clk);
		goto put_master;
	}

	ret = clk_prepare_enable(xspi->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable bus clock\n");
		goto put_master;
	}

	/* probe the number of CS lines */
	xspi_write32(0xffffffffU, xspi->regs + XSPI_CSDR_OFFSET);
	cs_bits = ioread32(xspi->regs + XSPI_CSDR_OFFSET);
	if (!cs_bits) {
		dev_err(&pdev->dev, "Could not auto probe CS lines\n");
		ret = -EINVAL;
		goto put_master;
	}

	num_cs = order_base_2(rounddown_pow_of_two(cs_bits)) + 1;
	if (num_cs > SIFIVE_SPI_MAX_CS) {
		dev_err(&pdev->dev, "Invalid number of spi slaves\n");
		ret = -EINVAL;
		goto put_master;
	}

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_MASK(bits_per_word);
	master->bus_num = pdev->id;
	master->num_chipselect = num_cs;
	master->dev.of_node = pdev->dev.of_node;

	xspi->read_fn = xspi_read32;
	xspi->write_fn = xspi_write32;

	/*
	 * Ensure our default match the implementation
	 */
	xspi->cs_inactive &= xspi->read_fn(xspi->regs + XSPI_CSDR_OFFSET);
	xspi->buffer_size = 8;  /* Both TXWM and RXWM are 3bits wide */

	/* SPI controller initializations => disables interrupts */
	xspi_init_hw(xspi);

	xspi->irq = platform_get_irq(pdev, 0);
	if (xspi->irq < 0 && xspi->irq != -ENXIO) {
		ret = xspi->irq;
		goto put_master;
	} else if (xspi->irq >= 0) {
		/* Register for SPI Interrupt */
		ret = devm_request_irq(&pdev->dev, xspi->irq, sifive_spi_irq, 0,
				dev_name(&pdev->dev), xspi);
		if (ret)
			goto put_master;
	}

	dev_info(&pdev->dev, "at 0x%08llX mapped to 0x%p, irq=%d, cs=%d\n",
		(unsigned long long)res->start, xspi->regs, xspi->irq, master->num_chipselect);

	ret = spi_bitbang_start(&xspi->bitbang);
	if (ret) {
		dev_err(&pdev->dev, "spi_bitbang_start FAILED\n");
		goto put_master;
	}

	platform_set_drvdata(pdev, master);
	return 0;

put_master:
	spi_master_put(master);

	return ret;
}

static int sifive_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct sifive_spi *xspi = spi_master_get_devdata(master);
	void __iomem *regs_base = xspi->regs;

	spi_bitbang_stop(&xspi->bitbang);

	/* Disable all the interrupts just in case */
	xspi->write_fn(0, regs_base + XSPI_IER_OFFSET);

	spi_master_put(xspi->bitbang.master);

	return 0;
}

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
