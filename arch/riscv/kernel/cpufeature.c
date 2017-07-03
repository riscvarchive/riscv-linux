/*
 * Copied from arch/arm64/kernel/cpufeature.c
 *
 * Copyright (C) 2015 ARM Ltd.
 * Copyright (C) 2017 SiFive
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/of.h>
#include <asm/processor.h>
#include <asm/hwcap.h>

unsigned long elf_hwcap __read_mostly;

void riscv_fill_hwcap(void)
{
	struct device_node *node;
	const char *isa;
	size_t i;

	elf_hwcap = 0;

	/*
	 * We don't support running Linux on hertergenous ISA systems.  For
	 * now, we just check the ISA of the first processor.
	 */
	node = of_find_node_by_type(NULL, "cpu");
	if (!node) {	
		pr_warning("Unable to find \"cpu\" devicetree entry");
		return;
	}

	if (of_property_read_string(node, "riscv,isa", &isa)) {
		pr_warning("Unablet to find \"riscv,isa\" devicetree entry");
		return;
	}

	for (i = 0; i < strlen(isa); ++i) {
		switch (isa[i]) {
		case 'f':
			elf_hwcap |= COMPAT_HWCAP_ISA_F;
			break;
		case 'd':
			elf_hwcap |= COMPAT_HWCAP_ISA_D;
			break;
		case 'q':
			elf_hwcap |= COMPAT_HWCAP_ISA_Q;
			break;
		}
	}

	pr_info("elf_hwcap is 0x%lx", elf_hwcap);
}
