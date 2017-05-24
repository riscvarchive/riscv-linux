/*
 * Copyright (C) 2012 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/of.h>

/* Return -1 if not a valid hart */
int riscv_of_processor_hart(struct device_node *node)
{
	const char *isa, *status;
	u32 hart;

	if (!of_device_is_compatible(node, "riscv"))
		return -(ENODEV);
	if (of_property_read_u32(node, "reg", &hart)
	    || hart >= NR_CPUS)
		return -(ENODEV);
	if (of_property_read_string(node, "status", &status)
	    || strcmp(status, "okay"))
		return -(ENODEV);
	if (of_property_read_string(node, "riscv,isa", &isa)
	    || isa[0] != 'r'
	    || isa[1] != 'v')
		return -(ENODEV);

	return hart;
}

#ifdef CONFIG_PROC_FS

static void *c_start(struct seq_file *m, loff_t *pos)
{
	*pos = cpumask_next(*pos - 1, cpu_online_mask);
	if ((*pos) < nr_cpu_ids)
		return (void *)(uintptr_t)(1 + *pos);
	return NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

static int c_show(struct seq_file *m, void *v)
{
	unsigned long hart_id = (unsigned long)v - 1;
	struct device_node *node = of_get_cpu_node(hart_id, NULL);
	const char *compat, *isa, *mmu;

	seq_printf(m, "hart\t: %lu\n", hart_id);
	if (!of_property_read_string(node, "riscv,isa", &isa)
	    && isa[0] == 'r'
	    && isa[1] == 'v')
		seq_printf(m, "isa\t: %s\n", isa);
	if (!of_property_read_string(node, "mmu-type", &mmu)
	    && !strncmp(mmu, "riscv,", 6))
		seq_printf(m, "mmu\t: %s\n", mmu+6);
	if (!of_property_read_string(node, "compatible", &compat)
	    && strcmp(compat, "riscv"))
		seq_printf(m, "uarch\t: %s\n", compat);
	seq_puts(m, "\n");

	return 0;
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show
};

#endif /* CONFIG_PROC_FS */
