#include <linux/fb.h>
#include <linux/module.h>

#include <asm/htif.h>

#define DRIVER_NAME "htifrfb"

#define HTIF_CMD_RFB_CONF (0x00UL)
#define HTIF_CMD_RFB_ADDR (0x01UL)

#define RFB_XRES	(1024UL)
#define RFB_YRES	(768UL)
#define RFB_DEPTH	(24UL)
#define RFB_BPP		(32UL)

#define RFB_ROW_LEN	((RFB_XRES * RFB_BPP) >> 3)
#define RFB_WORD_MASK	((BITS_PER_LONG >> 3) - 1)
#define RFB_LINE_LEN	((RFB_ROW_LEN + RFB_WORD_MASK) & (~RFB_WORD_MASK))
#define RFB_MEM_SIZE	(RFB_LINE_LEN * RFB_YRES)
#define RFB_BUF_ORDER	(9)

static void htifrfb_destroy(struct fb_info *info)
{
	if (info->screen_base)
		free_pages((unsigned long)(info->screen_base), RFB_BUF_ORDER);
	framebuffer_release(info);
}

static struct fb_ops htifrfb_ops = {
	.owner		= THIS_MODULE,
	.fb_destroy     = htifrfb_destroy,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static struct fb_var_screeninfo htifrfb_var = {
	.xres		= RFB_XRES,
	.yres		= RFB_YRES,
	.bits_per_pixel	= RFB_BPP,
	.red = {
		.offset	= 16,
		.length	= 8,
	},
	.green = {
		.offset	= 8,
		.length	= 8,
	},
	.blue = {
		.offset	= 0,
		.length	= 8,
	},
	.transp = {
		.offset	= 24,
		.length	= 8,
	},
	.activate	= FB_ACTIVATE_NOW,
	.height		= -1,
	.width		= -1,
};

static struct fb_fix_screeninfo htifrfb_fix = {
	.id		= "HTIF RFB",
	.smem_len	= RFB_MEM_SIZE,
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.line_length	= RFB_LINE_LEN,
	.accel		= FB_ACCEL_NONE,
};

static int htifrfb_probe(struct device *dev)
{
	struct htif_device *htif_dev;
	sbi_device_message response;
	struct fb_info *info;
	unsigned long flags;
	int ret;

	dev_info(dev, "detected framebuffer\n");
	htif_dev = to_htif_dev(dev);

	info = framebuffer_alloc(0, dev);
	if (unlikely(info == NULL))
		return -ENOMEM;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (unlikely(ret))
		goto err_alloc_cmap;

	info->screen_base = (char *)__get_free_pages(GFP_KERNEL, RFB_BUF_ORDER);
	if (unlikely(info->screen_base == NULL))
		goto err_alloc_buf;

	info->flags = FBINFO_DEFAULT;
	info->fbops = &htifrfb_ops;
	info->var = htifrfb_var;
	info->fix = htifrfb_fix;
	info->fix.smem_start = __pa(info->screen_base);

	ret = register_framebuffer(info);
	if (unlikely(ret))
		goto err_reg_fb;

	/* Disable interrupts and poll for HTIF acknowledgement */
	local_irq_save(flags);

	htif_tohost(htif_dev->index, HTIF_CMD_RFB_CONF,
		(RFB_BPP << 32) | (RFB_YRES << 16) | RFB_XRES);
	htif_fromhost(&response);

	htif_tohost(htif_dev->index, HTIF_CMD_RFB_ADDR,
		info->fix.smem_start);
	htif_fromhost(&response);

	local_irq_restore(flags);
	return 0;

err_reg_fb:
	free_pages((unsigned long)(info->screen_base), RFB_BUF_ORDER);
err_alloc_buf:
	fb_dealloc_cmap(&info->cmap);
err_alloc_cmap:
	framebuffer_release(info);
	return ret;
}

static struct htif_driver htifrfb_driver = {
	.type = "rfb",
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.probe = htifrfb_probe,
	},
};

static int __init htifrfb_init(void)
{
	return htif_register_driver(&htifrfb_driver);
}

static void __exit htifrfb_exit(void)
{
	htif_unregister_driver(&htifrfb_driver);
}

module_init(htifrfb_init);
module_exit(htifrfb_exit);

MODULE_DESCRIPTION("HTIF block driver");
MODULE_LICENSE("GPL");
