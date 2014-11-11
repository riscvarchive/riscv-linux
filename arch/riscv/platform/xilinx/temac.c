#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>

#include <asm/csr.h>

#define TRXD_LAST_MASK		(1 << 9)

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
#define XTE_RXM_MAXSZ_MASK	0x3fff    /* Rx max frame length */

#define XTE_TXM_ADDR		0x400 /* Tx max frame configuration */
#define XTE_TXM_MAXEN_MASK	(1 << 16) /* Tx max frame enable */
#define XTE_TXM_MAXSZ_MASK	0x3fff    /* Tx max frame length */

#define XTE_ID_ADDR		0x4f8 /* ID register */
#define XTE_ABLE_ADDR		0x4fc /* Ability register */


static inline unsigned long temac_cfg_read(unsigned long reg)
{
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg);
	return csr_read(CONFIG_RISCV_TEMAC_CSR_CFGD);
}

static inline void temac_cfg_set(unsigned long reg, unsigned long val)
{
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg);
	csr_set(CONFIG_RISCV_TEMAC_CSR_CFGD, val);
}

static inline void temac_cfg_clear(unsigned long reg, unsigned long val)
{
	csr_write(CONFIG_RISCV_TEMAC_CSR_CFGA, reg);
	csr_clear(CONFIG_RISCV_TEMAC_CSR_CFGD, val);
}

static int temac_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	const unsigned char *ptr;
	const unsigned char *end;

	ndev->trans_start = jiffies;
	skb_tx_timestamp(skb);

	for (ptr = skb->data, end = ptr + skb->len; ptr < end;) {
		unsigned long data;
		data = *ptr++;
		if (ptr == end)
			data |= TRXD_LAST_MASK;
		csr_write(CONFIG_RISCV_TEMAC_CSR_TXD, data);
	}

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += skb->len;
	return NETDEV_TX_OK;
}

static irqreturn_t temac_irq(int irq, void *dev)
{
	struct net_device *ndev = dev;
	struct sk_buff *skb;
	unsigned long v;

	skb = netdev_alloc_skb_ip_align(ndev, XTE_MAX_FRAME_SIZE);
	if (!skb)
		return IRQ_NONE;
	do {
		unsigned char *p;
		v = csr_read(CONFIG_RISCV_TEMAC_CSR_RXD);
		p = skb_put(skb, 1);
		*p = v;
	} while(!(v & TRXD_LAST_MASK));

	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, ndev);
	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += skb->len;
	netif_rx(skb);

	return IRQ_HANDLED;
}

static void temac_reset(struct net_device *ndev)
{
	unsigned int timeout;

	temac_cfg_set(XTE_RXC1_ADDR, XTE_RXC1_RST_MASK);
	timeout = 100;
	while (temac_cfg_read(XTE_RXC1_ADDR) & XTE_RXC1_RST_MASK) {
		udelay(1);
		if (--timeout == 0) {
			dev_err(&ndev->dev, "Rx reset timeout\n");
			break;
		}
	}

	temac_cfg_set(XTE_TXC_ADDR, XTE_TXC_RST_MASK);
	timeout = 100;
	while (temac_cfg_read(XTE_TXC_ADDR) & XTE_TXC_RST_MASK) {
		udelay(1);
		if (--timeout == 0) {
			dev_err(&ndev->dev, "Tx reset timeout\n");
			break;
		}
	}
}

static int temac_open(struct net_device *ndev)
{
	int rc;
	temac_reset(ndev);
	rc = request_irq(CONFIG_RISCV_TEMAC_IRQ, temac_irq, 0, ndev->name, ndev);
	if (rc) {
		dev_err(&ndev->dev, "unable to grab IRQ\n");
		return rc;
	}
	netif_start_queue(ndev);
	return 0;
}

static int temac_stop(struct net_device *ndev)
{
	free_irq(CONFIG_RISCV_TEMAC_IRQ, ndev);
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

	ndev = alloc_etherdev(0);
	if (!ndev)
		return -ENOMEM;

	ether_setup(ndev);
	ndev->flags &= ~IFF_MULTICAST;
//	ndev->features = NETIF_F_SG;
	ndev->netdev_ops = &temac_netdev_ops;

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
