/*
 * Copyright (C) 2017 SiFive
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

#include <linux/cacheinfo.h>
#include <linux/cpu.h>
#include <linux/of.h>
#include <linux/of_device.h>

static void ci_leaf_init(struct cacheinfo *this_leaf,
			 struct device_node *node,
			 enum cache_type type, unsigned int level)
{
	this_leaf->of_node = of_node_get(node);
	this_leaf->level = level;
	this_leaf->type = type;
	/* not a sector cache */
	this_leaf->physical_line_partition = 1;
	/* TODO: Add to DTS */
	this_leaf->attributes =
		CACHE_WRITE_BACK
		| CACHE_READ_ALLOCATE
		| CACHE_WRITE_ALLOCATE;
}

static void init_cache_level_helper(struct device_node *np, int *levels, int *leaves)
{
	struct of_phandle_iterator it;
	int rc, level;

	of_for_each_phandle(&it, rc, np, "next-level-cache", NULL, 0) {
		if (!of_device_is_compatible(it.node, "cache"))
			continue;
		if (of_property_read_u32(it.node, "cache-level", &level))
			continue;
		if (level > *levels) *levels = level;
		if (of_property_read_bool(it.node, "cache-size")) {
			++*leaves;
			init_cache_level_helper(it.node, levels, leaves);
		}
		if (of_property_read_bool(it.node, "i-cache-size")) {
			++*leaves;
			init_cache_level_helper(it.node, levels, leaves);
		}
		if (of_property_read_bool(it.node, "d-cache-size")) {
			++*leaves;
			init_cache_level_helper(it.node, levels, leaves);
		}
	}
}

static int __init_cache_level(unsigned int cpu)
{
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct device_node *np = of_cpu_device_node_get(cpu);
	int levels = 0, leaves = 0;

	if (of_property_read_bool(np, "cache-size"))
		++leaves;
	if (of_property_read_bool(np, "i-cache-size"))
		++leaves;
	if (of_property_read_bool(np, "d-cache-size"))
		++leaves;
	if (leaves > 0)
		levels = 1;

	init_cache_level_helper(np, &levels, &leaves);

	this_cpu_ci->num_levels = levels;
	this_cpu_ci->num_leaves = leaves;
	return 0;
}

static struct cacheinfo *populate_cache_leaves_helper(struct device_node *np, struct cacheinfo *this_leaf)
{
	struct of_phandle_iterator it;
	int rc, level;

	of_for_each_phandle(&it, rc, np, "next-level-cache", NULL, 0) {
		if (!of_device_is_compatible(it.node, "cache"))
			continue;
		if (of_property_read_u32(it.node, "cache-level", &level))
			continue;
		if (of_property_read_bool(it.node, "cache-size")) {
			ci_leaf_init(this_leaf++, it.node, CACHE_TYPE_UNIFIED, level);
			this_leaf = populate_cache_leaves_helper(it.node, this_leaf);
		}
		if (of_property_read_bool(it.node, "i-cache-size")) {
			ci_leaf_init(this_leaf++, it.node, CACHE_TYPE_INST, level);
			this_leaf = populate_cache_leaves_helper(it.node, this_leaf);
		}
		if (of_property_read_bool(it.node, "d-cache-size")) {
			ci_leaf_init(this_leaf++, it.node, CACHE_TYPE_DATA, level);
			this_leaf = populate_cache_leaves_helper(it.node, this_leaf);
		}
	}

	return this_leaf;
}

static int __populate_cache_leaves(unsigned int cpu)
{
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct cacheinfo *this_leaf = this_cpu_ci->info_list;
	struct device_node *np = of_cpu_device_node_get(cpu);
	int level = 1;

	if (of_property_read_bool(np, "cache-size"))
		ci_leaf_init(this_leaf++, np, CACHE_TYPE_UNIFIED, level);
	if (of_property_read_bool(np, "i-cache-size"))
		ci_leaf_init(this_leaf++, np, CACHE_TYPE_INST, level);
	if (of_property_read_bool(np, "d-cache-size"))
		ci_leaf_init(this_leaf++, np, CACHE_TYPE_DATA, level);

	this_leaf = populate_cache_leaves_helper(np, this_leaf);
	return 0;
}

DEFINE_SMP_CALL_CACHE_FUNCTION(init_cache_level)
DEFINE_SMP_CALL_CACHE_FUNCTION(populate_cache_leaves)
