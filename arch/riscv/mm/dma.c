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

	return dma_noop_ops.alloc(dev, size, dma_handle, gfp, attrs);
}

static void dma_riscv_free(struct device *dev, size_t size,
                          void *cpu_addr, dma_addr_t dma_addr,
                          unsigned long attrs)
{
	return dma_noop_ops.free(dev, size, cpu_addr, dma_addr, attrs);
}

static dma_addr_t dma_riscv_map_page(struct device *dev, struct page *page,
                                      unsigned long offset, size_t size,
                                      enum dma_data_direction dir,
                                      unsigned long attrs)
{
	return dma_noop_ops.map_page(dev, page, offset, size, dir, attrs);
}

static int dma_riscv_map_sg(struct device *dev, struct scatterlist *sgl, int nents,
                             enum dma_data_direction dir,
                             unsigned long attrs)
{
	return dma_noop_ops.map_sg(dev, sgl, nents, dir, attrs);
}

static int dma_riscv_supported(struct device *dev, u64 mask)
{
	// Our smallest allocation pool (ZONE_DMA) uses 32 physical address bits
	// (it is common on RISC-V for physical memory to start at the 2GiB mark)
	return mask >= DMA_BIT_MASK(32);
}

const struct dma_map_ops dma_riscv_ops = {
	.alloc			= dma_riscv_alloc,
	.free			= dma_riscv_free,
	.map_page		= dma_riscv_map_page,
	.map_sg			= dma_riscv_map_sg,
	.dma_supported		= dma_riscv_supported,
};

EXPORT_SYMBOL(dma_riscv_ops);
