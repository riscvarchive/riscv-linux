/* adapted from arch/x86/kernel/pci-nommu.c  by Jamey Hicks <jamey.hicks@gmail.com> */

/* Fallback functions when the main IOMMU code is not compiled in. This
   code is roughly equivalent to i386. */
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/pci.h>
#include <linux/mm.h>

//#include <asm/processor.h>
//#include <asm/iommu.h>
#include <asm/dma.h>

// copied from arch/x86/kernel/pci-dma.c
void *dma_generic_alloc_coherent(struct device *dev, size_t size,
				 dma_addr_t *dma_addr, gfp_t flag,
				 struct dma_attrs *attrs)
{
	unsigned long dma_mask;
	struct page *page;
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	dma_addr_t addr;

	dma_mask = dma_alloc_coherent_mask(dev, flag);

	flag &= ~__GFP_ZERO;
again:
	page = NULL;
	/* CMA can be used only in the context which permits sleeping */
	if (flag & __GFP_WAIT) {
		page = dma_alloc_from_contiguous(dev, count, get_order(size));
		if (page && page_to_phys(page) + size > dma_mask) {
			dma_release_from_contiguous(dev, page, count);
			page = NULL;
		}
	}
	/* fallback */
	if (!page)
		page = alloc_pages_node(dev_to_node(dev), flag, get_order(size));
	if (!page)
		return NULL;

	addr = page_to_phys(page);
	if (addr + size > dma_mask) {
		__free_pages(page, get_order(size));

		if (dma_mask < DMA_BIT_MASK(32) && !(flag & GFP_DMA)) {
			flag = (flag & ~GFP_DMA32) | GFP_DMA;
			goto again;
		}

		return NULL;
	}
	memset(page_address(page), 0, size);
	*dma_addr = addr;
	return page_address(page);
}

// copied from arch/x86/kernel/pci-dma.c
void dma_generic_free_coherent(struct device *dev, size_t size, void *vaddr,
			       dma_addr_t dma_addr, struct dma_attrs *attrs)
{
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	struct page *page = virt_to_page(vaddr);

	if (!dma_release_from_contiguous(dev, page, count))
		free_pages((unsigned long)vaddr, get_order(size));
}

static int
check_addr(char *name, struct device *hwdev, dma_addr_t bus, size_t size)
{
	return 1;
}

static dma_addr_t no_iommu_map_page(struct device *dev, struct page *page,
				 unsigned long offset, size_t size,
				 enum dma_data_direction dir,
				 struct dma_attrs *attrs)
{
	dma_addr_t bus = page_to_phys(page) + offset;
	WARN_ON(size == 0);
	if (!check_addr("map_single", dev, bus, size))
		return DMA_ERROR_CODE;
	flush_write_buffers();
	return bus;
}

/* Map a set of buffers described by scatterlist in streaming
 * mode for DMA.  This is the scatter-gather version of the
 * above pci_map_single interface.  Here the scatter gather list
 * elements are each tagged with the appropriate dma address
 * and length.  They are obtained via sg_dma_{address,length}(SG).
 *
 * NOTE: An implementation may be able to use a smaller number of
 *       DMA address/length pairs than there are SG table elements.
 *       (for example via virtual mapping capabilities)
 *       The routine returns the number of addr/length pairs actually
 *       used, at most nents.
 *
 * Device ownership issues as mentioned above for pci_map_single are
 * the same here.
 */
static int no_iommu_map_sg(struct device *hwdev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir,
			struct dma_attrs *attrs)
{
	struct scatterlist *s;
	int i;

	WARN_ON(nents == 0 || sg[0].length == 0);

	for_each_sg(sg, s, nents, i) {
		BUG_ON(!sg_page(s));
		s->dma_address = sg_phys(s);
		if (!check_addr("map_sg", hwdev, s->dma_address, s->length))
			return 0;
#ifdef CONFIG_NEED_SG_DMA_LENGTH
		s->dma_length = s->length;
#endif
	}
	flush_write_buffers();
	return nents;
}

static void no_iommu_sync_single_for_device(struct device *dev,
			dma_addr_t addr, size_t size,
			enum dma_data_direction dir)
{
	flush_write_buffers();
}


static void no_iommu_sync_sg_for_device(struct device *dev,
			struct scatterlist *sg, int nelems,
			enum dma_data_direction dir)
{
	flush_write_buffers();
}

struct dma_map_ops no_iommu_dma_ops = {
	.alloc			= dma_generic_alloc_coherent,
	.free			= dma_generic_free_coherent,
	.map_sg			= no_iommu_map_sg,
	.map_page		= no_iommu_map_page,
	.sync_single_for_device = no_iommu_sync_single_for_device,
	.sync_sg_for_device	= no_iommu_sync_sg_for_device,
	.is_phys		= 1,
};

struct dma_map_ops *dma_ops = &no_iommu_dma_ops;
EXPORT_SYMBOL(dma_ops);
