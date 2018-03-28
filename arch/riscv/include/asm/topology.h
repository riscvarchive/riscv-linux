/* SPDX-License-Identifier: GPL-2.0 */
/* Copied from arch/arm64/include/asm/topology.h */
#ifndef __ASM_TOPOLOGY_H
#define __ASM_TOPOLOGY_H

#include <linux/cpumask.h>

struct cpu_topology {
	int thread_id;
	int core_id;
	int cluster_id;
	cpumask_t thread_sibling;
	cpumask_t core_sibling;
};

extern struct cpu_topology cpu_topology[NR_CPUS];

#define topology_physical_package_id(cpu)	(0)
#define topology_core_id(cpu)			(cpu)
#define topology_core_cpumask(cpu)		(1 << cpu)
#define topology_sibling_cpumask(cpu)		(1 << cpu)

#include <asm-generic/topology.h>

#endif /* _ASM_ARM_TOPOLOGY_H */
