/*
 * Code borrowed from arch/arm64/kernel/pci.c
 *   which borrowed from powerpc/kernel/pci-common.c
 *   which borrowed from arch/alpha/kernel/pci.c
 *
 * Extruded from code written by
 *      Dave Rusling (david.rusling@reo.mts.dec.com)
 *      David Mosberger (davidm@cs.arizona.edu)
 * Copyright (C) 1999 Andrea Arcangeli <andrea@suse.de>
 * Copyright (C) 2000 Ivan Kokshaysky <ink@jurassic.park.msu.ru>
 * Copyright (C) 2003 Anton Blanchard <anton@au.ibm.com>, IBM
 * Copyright (C) 2014 ARM Ltd.
 * Copyright (C) 2017 SiFive
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/* This file contains weakly bound functions that implement pcibios functions
 * that some architectures have copied verbatim.
 */

#include <linux/pci.h>

/*
 * Called after each bus is probed, but before its children are examined
 */
__attribute__ ((weak))
void pcibios_fixup_bus(struct pci_bus *bus)
{
       /* nothing to do, expected to be removed in the future */
}
/*
 * We don't have to worry about legacy ISA devices, so nothing to do here
 */
__attribute__ ((weak))
resource_size_t pcibios_align_resource(void *data, const struct resource *res,
                               resource_size_t size, resource_size_t align)
{
       return res->start;
}
