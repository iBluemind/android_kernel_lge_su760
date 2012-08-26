/*
 * arch/arm/kernel/topology.c
 *
 * Copyright (C) 2011  vincent.guittot@linaro.org
 *
 * based on arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/sched.h>

#include <asm/cacheflush.h>
#include <asm/topology.h>

#define hard_smp_mpidr() \
	({ \
		unsigned int cpunum; \
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum)); \
		cpunum; \
	})

struct cputopo_arm cpu_topology[NR_CPUS];

const struct cpumask *cpu_coregroup_mask(unsigned int cpu)
{
	return &(cpu_topology[cpu].core_sibling);
}

/*
 * store_cpu_topology is called at boot when only one cpu is running
 * and with the mutex cpu_hotplug.lock locked, when several cpus have booted,
 * which prevents simultaneous write access to cpu_topology array
 */
void store_cpu_topology(unsigned int cpuid)
{
	struct cputopo_arm *cpuid_topo = &(cpu_topology[cpuid]);
	unsigned int mpidr;
	unsigned int cpu;

	/* If the cpu topology has been already set, just return */
	if (cpuid_topo->core_id != -1)
		return;

	mpidr = hard_smp_mpidr();

	/* create cpu topology mapping */
	if (mpidr & (0x3 << 30)) {
		/*
		 * This is a multiprocessor system
		 * multiprocessor format & multiprocessor mode field are set
		 */

		if (mpidr & (0x1 << 24)) {
			/* core performance interdependency */
			cpuid_topo->thread_id = (mpidr & 0x3);
			cpuid_topo->core_id =  ((mpidr >> 8) & 0xF);
			cpuid_topo->socket_id = ((mpidr >> 16) & 0xFF);
		} else {
			/* normal core interdependency */
			cpuid_topo->thread_id = -1;
			cpuid_topo->core_id = (mpidr & 0x3);
			cpuid_topo->socket_id = ((mpidr >> 8) & 0xF);
		}
	} else {
		/*
		 * This is an uniprocessor system
		 * we are in multiprocessor format but uniprocessor system
		 * or in the old uniprocessor format
		 */

		cpuid_topo->thread_id = -1;
		cpuid_topo->core_id = 0;
		cpuid_topo->socket_id = -1;
	}

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		if (cpuid_topo->socket_id == cpu_topo->socket_id) {
			cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
			if (cpu != cpuid)
				cpumask_set_cpu(cpu,
					&cpuid_topo->core_sibling);

			if (cpuid_topo->core_id == cpu_topo->core_id) {
				cpumask_set_cpu(cpuid,
					&cpu_topo->thread_sibling);
				if (cpu != cpuid)
					cpumask_set_cpu(cpu,
						&cpuid_topo->thread_sibling);
			}
		}
	}
	smp_wmb();

	printk(KERN_INFO "cpu %u : thread %d cpu %d, socket %d, mpidr %x\n",
		cpuid, cpu_topology[cpuid].thread_id,
		cpu_topology[cpuid].core_id,
		cpu_topology[cpuid].socket_id, mpidr);

}

/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void init_cpu_topology(void)
{
	unsigned int cpu;

	/* init core mask */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);
	}
	smp_wmb();
}
