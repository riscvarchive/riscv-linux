/*
 * Copyright (C) 2017 SiFive
 *   Wesley Terpstra <wesley@sifive.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 */

#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/swiotlb.h>

static void *dma_riscv_alloc(struct device *dev, size_t size,
                            dma_addr_t *dma_handle, gfp_t gfp,
                            unsigned long attrs)
{
	dma_addr_t end_of_normal = (max_low_pfn << PAGE_SHIFT) - 1;

	// If the device cannot address ZONE_NORMAL, allocate from ZONE_DMA
	gfp &= ~(__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM);
	if (dev == NULL ||
	    end_of_normal > dev->coherent_dma_mask ||
	    end_of_normal > *dev->dma_mask)
	{
		gfp |= GFP_DMA;
	}

	return swiotlb_alloc_coherent(dev, size, dma_handle, gfp);
}

static void dma_riscv_free(struct device *dev, size_t size,
                           void *vaddr, dma_addr_t dma_addr,
                           unsigned long attrs)
{
	swiotlb_free_coherent(dev, size, vaddr, dma_addr);
}

const struct dma_map_ops dma_riscv_ops = {
	.alloc = dma_riscv_alloc,
	.free = dma_riscv_free,
	.map_page = swiotlb_map_page,
	.unmap_page = swiotlb_unmap_page,
	.map_sg = swiotlb_map_sg_attrs,
	.unmap_sg = swiotlb_unmap_sg_attrs,
	.sync_single_for_cpu = swiotlb_sync_single_for_cpu,
	.sync_single_for_device = swiotlb_sync_single_for_device,
	.sync_sg_for_cpu = swiotlb_sync_sg_for_cpu,
	.sync_sg_for_device = swiotlb_sync_sg_for_device,
	.dma_supported = swiotlb_dma_supported,
	.mapping_error = swiotlb_dma_mapping_error,
};

EXPORT_SYMBOL(dma_riscv_ops);
