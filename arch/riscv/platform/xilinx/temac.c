#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>

#include <asm/csr.h>

#define XTE_MAX_FRAME_SIZE	1522

#define XTE_RXC0_ADDR		0x400 /* Rx configuration 0 */
#define XTE_RXC1_ADDR		0x404 /* Rx configuration 1 */
#define XTE_RXC1_RST_MASK	(1 << 31) /* Receiver reset */
#define XTE_RXC1_JUMBO_MASK	(1 << 30) /* Jumbo frame enable */
#define XTE_RXC1_FCS_MASK	(1 << 29) /* In-band FCS enable */
#define XTE_RXC1_RXEN_MASK	(1 << 28) /* Receiver enable */
#define XTE_RXC1_VLAN_MASK	(1 << 27) /* VLAN enable */
#define XTE_RXC1_HDX_MASK	(1 << 26) /* Half duplex */
#define XTE_RXC1_LTEC_MASK	(1 << 25) /* Length/type error check disable */
#define XTE_RXC1_CFLC_MASK	(1 << 24) /* Control frame length check disable */

#define XTE_TXC_ADDR		0x408 /* Tx configuration */
#define XTE_TXC_RST_MASK	(1 << 31) /* Transmitter reset */
#define XTE_TXC_JUMBO_MASK	(1 << 30) /* Jumbo frame enable */
#define XTE_TXC_FCS_MASK	(1 << 29) /* In-bad FCS enable */
#define XTE_TXC_TXEN_MASK	(1 << 28) /* Transmitter enable */
#define XTE_TXC_VLAN_MASK	(1 << 27) /* VLAN enable */
#define XTE_TXC_HDX_MASK	(1 << 26) /* Half duplex */
#define XTE_TXC_IFGA_MASK	(1 << 25) /* Interframe gap adjustment enable */

#define XTE_FCC_ADDR		0x40c /* Flow control configuration */
#define XTE_FCC_TXFC_MASK	(1 << 30) /* Tx flow control enable */
#define XTE_FCC_RXFC_MASK	(1 << 29) /* Rx flow control enable */

#define XTE_SPD_ADDR		0x410 /* MAC speed configuration */
#define XTE_SPD_SHIFT		30
#define XTE_SPD_MASK		0x3
#define XTE_SPD_10BASE		0x0 /* 10 Mb/s */
#define XTE_SPD_100BASE		0x1 /* 100 Mb/s */
#define XTE_SPD_1000BASE	0x2 /* 1 Gb/s */

#define XTE_RXM_ADDR		0x400 /* Rx max frame configuration */
#define XTE_RXM_MAXEN_MASK	(1 << 16) /* Rx max frame enable */
#define XTE_RXM_MAXSZ_MASK	0x3fff	  /* Rx max frame length */

#define XTE_TXM_ADDR		0x400 /* Tx max frame configuration */
#define XTE_TXM_MAXEN_MASK	(1 << 16) /* Tx max frame enable */
#define XTE_TXM_MAXSZ_MASK	0x3fff	  /* Tx max frame length */

#define XTE_ID_ADDR		0x4f8 /* ID register */
#define XTE_ABLE_ADDR		0x4fc /* Ability register */


#define DMA_TX_PTR_SIZE		3
#define DMA_TX_PTR_MASK		((1 << DMA_TX_PTR_SIZE) - 1)
#define DMA_RX_PTR_SIZE		3
#define DMA_RX_PTR_MASK		((1 << DMA_RX_PTR_SIZE) - 1)

#define DMA_RX_ALIGN_BYTES	0x40
#define DMA_RX_ALIGN(p)		((typeof(p)) \
	(__ALIGN_KERNEL((unsigned long)(p), DMA_RX_ALIGN_BYTES)))

#define DMA_CSR_STATUS		0x530
#define DMA_CSR_TX_ADDR		0x531
#define DMA_CSR_TX_CNT		0x532
#define DMA_CSR_TX_WPTR		0x533
#define DMA_CSR_TX_RPTR		0x534
#define DMA_CSR_RX_ADDR		0x535
#define DMA_CSR_RX_CNT		0x536
#define DMA_CSR_RX_WPTR		0x537
#define DMA_CSR_RX_RPTR		0x538

#define DMA_STAT_IRQ		(1 << 0)
#define DMA_STAT_TX_RDY		(1 << 1)
#define DMA_STAT_RX_RDY		(1 << 2)
#define DMA_STAT_RX_VAL		(1 << 3)

#define IRQ_DMA			4

// read/write values are 32 bits wide, CFGD
// reads will clear CFGD first
// writes will set data 
//
// writes to CFGD do nothing special
// reads from CFGD do nothing special
// writes to CFGA issue the read/write request based on CFGD[32]
// the csr_write to CFGA will stall until the cpu until 
// 1) the axi write is ack'd
// 2) the read data is available in CFGD for csr_read to read plainly


static inline void lots_of_nops(void) {
	int i;
	for (i = 0; i < 100; i++) {
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
		__asm__ __volatile__("nop");
	}
}

static inline unsigned long temac_cfg_read(unsigned long reg)
{
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGD, 0);
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg); // doing the "work"
	lots_of_nops();
	return csr_read(CONFIG_RISCV_TEMAC_CSR_CFGD);
}

static inline void temac_cfg_write(unsigned long reg, unsigned long val)
{
	// make bit 33 the sfp_tx_disable reset
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGD, val | (((unsigned long)0x1) << 32));
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg); // doing the "work"
	lots_of_nops();
}

// remove this
static inline void temac_cfg_clear(unsigned long reg, unsigned long val)
{
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg);
	csr_clear(CONFIG_RISCV_TEMAC_CSR_CFGD, val);
}


struct temac_local {
	spinlock_t lock;
	struct sk_buff *rx_ring[1 << DMA_RX_PTR_SIZE];
	struct sk_buff *tx_ring[1 << DMA_TX_PTR_SIZE];
	unsigned long tx_ptr;
};

static int temac_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct temac_local *lp;
	unsigned long flags;
	int rc;

	lp = netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);

	if (likely(csr_read(DMA_CSR_STATUS) & DMA_STAT_TX_RDY)) {
		unsigned long ptr, addr;

		ndev->trans_start = jiffies;
		skb_tx_timestamp(skb);

		addr = virt_to_phys(skb->data);
		ptr = csr_read(DMA_CSR_TX_WPTR);
		csr_write(DMA_CSR_TX_CNT, skb->len);
		csr_write(DMA_CSR_TX_ADDR, addr);

		lp->tx_ring[ptr] = skb;
		pr_debug("temac_dma: Tx start: ptr=%lu addr=%lx cnt=%u\n",
			ptr, addr, skb->len);
		rc = NETDEV_TX_OK;
	} else {
		netif_stop_queue(ndev);
		pr_debug("temac_dma: Tx busy\n");
		rc = NETDEV_TX_BUSY;
	}

	spin_unlock_irqrestore(&lp->lock, flags);
	return rc;
}

static inline void temac_dma_irq_tx(struct net_device *ndev)
{
	struct temac_local *lp;

	lp = netdev_priv(ndev);
	while (lp->tx_ptr != csr_read(DMA_CSR_TX_RPTR)) {
		struct sk_buff *skb;

		pr_debug("temac_dma: Tx done: ptr=%lu\n", lp->tx_ptr);
		skb = lp->tx_ring[lp->tx_ptr++];
		lp->tx_ptr &= DMA_TX_PTR_MASK;

		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;
		dev_consume_skb_irq(skb);
		netif_wake_queue(ndev);
	}
}

static inline void temac_dma_irq_rx(struct net_device *ndev)
{
	struct temac_local *lp;

	lp = netdev_priv(ndev);
	while (csr_read(DMA_CSR_STATUS) & DMA_STAT_RX_VAL) {
		struct sk_buff *skb;
		unsigned long ptr, len, align;

		ptr = csr_read(DMA_CSR_RX_RPTR);
		len = csr_read(DMA_CSR_RX_CNT);

		skb = lp->rx_ring[ptr];
		align = DMA_RX_ALIGN(skb->data) - skb->data + NET_IP_ALIGN;
		skb_put(skb, len + align);
		skb_pull(skb, align); /* remove alignment */

		skb->dev = ndev;
		skb->protocol = eth_type_trans(skb, ndev);
		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += len;
		netif_rx(skb);

		pr_debug("temac_dma: Rx done: ptr=%lu cnt=%lu align=%lu\n",
			ptr, len, align);
	}
}

static void temac_dma_alloc_rx(struct net_device *ndev)
{
	struct temac_local *lp;

	lp = netdev_priv(ndev);
	while (csr_read(DMA_CSR_STATUS) & DMA_STAT_RX_RDY) {
		struct sk_buff *skb;
		unsigned long ptr, buf;

		/* Allocate two extra L2 cachelines to ensure that the
		   buffer does not share a cacheline with other data */
		skb = netdev_alloc_skb(ndev, XTE_MAX_FRAME_SIZE +
			(2 * DMA_RX_ALIGN_BYTES) + NET_IP_ALIGN);
		if (!skb) {
			/* FIXME: potential receiver deadlock from
			   memory exhaustion (?) */
			break;
		}

		buf = virt_to_phys(DMA_RX_ALIGN(skb->data));
		ptr = csr_read(DMA_CSR_RX_WPTR);
		csr_write(DMA_CSR_RX_ADDR, buf);

		lp->rx_ring[ptr] = skb;
		pr_debug("temac_dma: Rx alloc: ptr=%lu vaddr=%p paddr=%lu\n",
			ptr, skb->data, buf);
	}
}

static irqreturn_t temac_dma_irq(int irq, void *dev)
{
	csr_clear(DMA_CSR_STATUS, DMA_STAT_IRQ); /* acknowledge */

	temac_dma_irq_tx(dev);
	temac_dma_irq_rx(dev);
	temac_dma_alloc_rx(dev);

	return IRQ_HANDLED;
}

static void temac_reset(struct net_device *ndev)
{
	unsigned int timeout;
	long testresult;
	int i;
//	  printk(KERN_ERR "Running temac_reset----------------------------------\n");

	// attempt to init the sfp
	// start the MDIO interface and set MDIO clock
	temac_cfg_write(0x500, 0x4A);
	temac_cfg_write(0x504, 0x06008800);
	testresult = temac_cfg_read(0x504);
//	  printk(KERN_ERR "TEMAC MDIO control word contains the value 0x%llx\n", testresult);
	testresult = temac_cfg_read(0x50C);
//	  printk(KERN_ERR "TEMAC MDIO Read data contains the value 0x%llx\n", testresult);
	testresult &= 0xFFFF;

	testresult |= 0x8000;
	temac_cfg_write(0x508, testresult); // write reset for pcs/pma
	temac_cfg_write(0x504, 0x06004800); // issue MDIO write

	temac_cfg_write(0x504, 0x06008800);
	testresult = temac_cfg_read(0x50C);
//	  printk(KERN_ERR "After reset, TEMAC MDIO Read data contains the value 0x%llx\n", testresult);

	testresult &= 0xFBFF; // clear isolate bit
//	  printk(KERN_ERR "TESTRESULT SHOULD BE: 0x%llx\n", testresult);

	temac_cfg_write(0x508, testresult); // clear isolate for pcs/pma

	// now, issue MDIO write, 0x1 << 33 is to trigger sfp_tx_disable on/off
	temac_cfg_write(0x504, 0x06004800 | (((unsigned long)0x1) << 33));

	for (i = 0; i < 10; i++) {
		lots_of_nops();
	}

	temac_cfg_write(XTE_RXC1_ADDR, XTE_RXC1_RST_MASK);
	timeout = 100;
	while (temac_cfg_read(XTE_RXC1_ADDR) & XTE_RXC1_RST_MASK) {
		udelay(1);
		if (--timeout == 0) {
			dev_err(&ndev->dev, "Rx reset timeout\n");
			break;
		}
	}

	temac_cfg_write(XTE_TXC_ADDR, XTE_TXC_RST_MASK);
	timeout = 100;
	while (temac_cfg_read(XTE_TXC_ADDR) & XTE_TXC_RST_MASK) {
		udelay(1);
		if (--timeout == 0) {
			dev_err(&ndev->dev, "Tx reset timeout\n");
			break;
		}
	}

	temac_cfg_write(0x404, 0x10000000);
	temac_cfg_write(0x408, 0x10000000);

/*	  for (i = 0; i < 50000*5; i++) {
		lots_of_nops();
	}
*/
	// issue read to check that SFP is out of isolate
/*	  temac_cfg_write(0x504, 0x06008800);
	testresult = temac_cfg_read(0x50C) & 0xFFFF;
	if (testresult != 0x1140) {
		printk(KERN_ERR "After clearing isolate, MDIO read contains incorrect value 0x%lx\n", testresult);
	}*/
}

static int temac_open(struct net_device *ndev)
{
	int rc;
	temac_reset(ndev);
	temac_dma_alloc_rx(ndev);
	rc = request_irq(IRQ_DMA, temac_dma_irq, 0, ndev->name, ndev);
	if (rc) {
		dev_err(&ndev->dev, "unable to grab IRQ\n");
		return rc;
	}
	netif_start_queue(ndev);
	return 0;
}

static int temac_stop(struct net_device *ndev)
{
	// TODO FIX
	free_irq(IRQ_DMA, ndev);
	temac_cfg_clear(XTE_RXC1_ADDR, XTE_RXC1_RXEN_MASK);
	temac_cfg_clear(XTE_TXC_ADDR, XTE_TXC_TXEN_MASK);
	netif_stop_queue(ndev);
	return 0;
}

static const struct net_device_ops temac_netdev_ops = {
	.ndo_open = temac_open,
	.ndo_stop = temac_stop,
	.ndo_start_xmit = temac_start_xmit,
};

static int riscv_temac_probe(struct platform_device *op)
{
	struct net_device *ndev;
	struct temac_local *lp;

	ndev = alloc_etherdev(sizeof(struct temac_local));
	if (!ndev)
		return -ENOMEM;

	ether_setup(ndev);
	ndev->flags &= ~IFF_MULTICAST;
//	ndev->features = NETIF_F_SG;
	ndev->netdev_ops = &temac_netdev_ops;

	lp = netdev_priv(ndev);
	spin_lock_init(&lp->lock);
	lp->tx_ptr = csr_read(DMA_CSR_TX_RPTR);

	platform_set_drvdata(op, ndev);
	SET_NETDEV_DEV(ndev, &op->dev);

	eth_hw_addr_random(ndev);
	return register_netdev(ndev);
}

static int riscv_temac_remove(struct platform_device *op)
{
	struct net_device *ndev;
	ndev = platform_get_drvdata(op);
	unregister_netdev(ndev);
	free_netdev(ndev);
	return 0;
}

static struct platform_driver riscv_temac_driver = {
	.probe = riscv_temac_probe,
	.remove = riscv_temac_remove,
	.driver = {
		.name = "riscv_temac",
	},
};

module_platform_driver(riscv_temac_driver);


static struct platform_device riscv_temac_device = {
	.name = "riscv_temac",
	.id = -1,
};

static int __init riscv_temac_init(void)
{
	return platform_device_register(&riscv_temac_device);
}

module_init(riscv_temac_init);

MODULE_DESCRIPTION("RISC-V Tri-Mode Ethernet MAC driver");
MODULE_AUTHOR("Albert Ou");
MODULE_LICENSE("GPL");
