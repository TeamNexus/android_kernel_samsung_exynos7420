/*
 * drivers/cpufreq/cpufreq_nexus.c
 *
 * Copyright (C) 2017 Lukas Berger
 *
 * Uses code from cpufreq_alucard and cpufreq_interactive
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "cpufreq_governor.h"

static int cpufreq_governor_nexus(struct cpufreq_policy *policy, unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_NEXUS
static
#endif
struct cpufreq_governor cpufreq_gov_nexus = {
	.name = "nexus",
	.governor = cpufreq_governor_nexus,
	.owner = THIS_MODULE,
};

static struct cpufreq_nexus_tunables *global_tunables = NULL;
static DEFINE_MUTEX(cpufreq_governor_nexus_mutex);

struct cpufreq_nexus_cpuinfo {
	int init;

	int cpu;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;

	cputime64_t prev_idle;
	cputime64_t prev_wall;

	struct delayed_work work;
	struct mutex timer_mutex;
};

struct cpufreq_nexus_tunables {
	// load at which the cpugov decides to scale down
	#define DEFAULT_DOWN_LOAD 25
	unsigned int down_load;

	// frequency-steps if cpugov scales down
	#define DEFAULT_DOWN_STEP 2
	unsigned int down_step;

	// load at which the cpugov decides to scale up
	#define DEFAULT_UP_LOAD 50
	unsigned int up_load;

	// interval after which the cpugov can decide to scale up
	#define DEFAULT_UP_STEP 1
	unsigned int up_step;

	// interval after which the cpugov can decide to scale down
	#define DEFAULT_SAMPLING_RATE 25000
	unsigned int sampling_rate;

	// indicates if I/O-time should be added to cputime
	#define DEFAULT_IO_IS_BUSY 1
	int io_is_busy;

	// minimal frequency chosen by the cpugov
	unsigned int freq_min;

	// maximal frequency chosen by the cpugov
	unsigned int freq_max;

	// simple boost to freq_max
	int boost;

	// time in usecs when current boostpulse ends
	u64 boostpulse;
};

static DEFINE_PER_CPU(struct cpufreq_nexus_cpuinfo, gov_cpuinfo);

static void cpufreq_nexus_timer(struct work_struct *work)
{
	struct cpufreq_nexus_cpuinfo *cpuinfo;
	struct cpufreq_policy *policy;
	struct cpufreq_nexus_tunables *tunables;
	int delay, cpu, load;
	unsigned int index = 0;
	unsigned int freq = 0,
				 next_freq = 0;
	cputime64_t curr_idle, curr_wall, idle, wall;

	cpuinfo = container_of(work, struct cpufreq_nexus_cpuinfo, work.work);
	if (!cpuinfo)
		return;

	policy = cpuinfo->policy;
	if (!policy)
		return;

	tunables = policy->governor_data;
	cpu = cpuinfo->cpu;

	if (mutex_lock_interruptible(&cpuinfo->timer_mutex))
		return;

	if (!cpu_online(cpu))
		goto exit;

	// calculate new load
	curr_idle = get_cpu_idle_time(cpu, &curr_wall, tunables->io_is_busy);
	idle = (curr_idle - cpuinfo->prev_idle);
	wall = (curr_wall - cpuinfo->prev_wall);

	cpuinfo->prev_idle = curr_idle;
	cpuinfo->prev_wall = curr_wall;

	if (cpuinfo->init) {
		// apply the current cputimes and skip this sample
		cpuinfo->init = 0;
		goto requeue;
	}

	// calculate frequencies
	freq = policy->cur;

	if (wall >= idle) {
		load = (wall > idle ? (100 * (wall - idle)) / wall : 0);

		if (load >= tunables->up_load) {
			freq = min(policy->cur + (tunables->up_step * 108000), policy->max);
		} else if (load <= tunables->down_load) {
			freq = max(policy->cur - (tunables->down_step * 108000), policy->min);
		}

		// apply tunables
		if (freq < tunables->freq_min) {
			freq = max(policy->min, tunables->freq_min);
		}

		if (freq > tunables->freq_max) {
			freq = min(policy->max, tunables->freq_max);
		}

		if (tunables->boost || ktime_to_us(ktime_get()) < tunables->boostpulse) {
			freq = min(policy->max, tunables->freq_max);
		}

		// apply frequencies
		cpufreq_frequency_table_target(policy, cpuinfo->freq_table, freq,
			CPUFREQ_RELATION_H, &index);
		if (cpuinfo->freq_table[index].frequency != policy->cur) {
			cpufreq_frequency_table_target(policy, cpuinfo->freq_table, freq,
				CPUFREQ_RELATION_L, &index);
		}
		next_freq = cpuinfo->freq_table[index].frequency;

		if (next_freq != policy->cur) {
			__cpufreq_driver_target(policy, next_freq, CPUFREQ_RELATION_L);
		}
	}

	// requeue work-timer
requeue:
	delay = usecs_to_jiffies(tunables->sampling_rate);
	if (num_online_cpus() > 1) {
		delay -= jiffies % delay;
	}

	queue_delayed_work_on(cpu, system_wq, &cpuinfo->work, delay);

exit:
	mutex_unlock(&cpuinfo->timer_mutex);
}

#define gov_sys_pol_show_store(_name)                                         \
	gov_sys_show(_name)                                                       \
	gov_pol_show(_name)                                                       \
	gov_sys_store(_name)                                                      \
	gov_pol_store(_name)                                                      \
	static struct global_attr _name##_gov_sys =                               \
		__ATTR(_name, 0666, show_##_name##_gov_sys, store_##_name##_gov_sys); \
	static struct freq_attr _name##_gov_pol =                                 \
		__ATTR(_name, 0666, show_##_name##_gov_pol, store_##_name##_gov_pol);

#define gov_sys_pol(_name)                                                    \
	static struct global_attr _name##_gov_sys =                               \
		__ATTR(_name, 0666, show_##_name##_gov_sys, store_##_name##_gov_sys); \
	static struct freq_attr _name##_gov_pol =                                 \
		__ATTR(_name, 0666, show_##_name##_gov_pol, store_##_name##_gov_pol);

#define gov_sys_show(file_name)                              \
static ssize_t show_##file_name##_gov_sys                    \
(struct kobject *kobj, struct attribute *attr, char *buf)    \
{                                                            \
	return sprintf(buf, "%u\n", global_tunables->file_name); \
}                                                            \

#define gov_pol_show(file_name)                                                                       \
static ssize_t show_##file_name##_gov_pol                                                             \
(struct cpufreq_policy *policy, char *buf)                                                            \
{                                                                                                     \
	return sprintf(buf, "%u\n", ((struct cpufreq_nexus_tunables *)policy->governor_data)->file_name); \
}

#define gov_sys_store(file_name)                                              \
static ssize_t store_##file_name##_gov_sys                                    \
(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count) \
{                                                                             \
	unsigned long val = 0;                                                    \
	int ret = kstrtoul(buf, 0, &val);                                         \
	if (ret < 0)                                                              \
		return ret;                                                           \
	global_tunables->file_name = val;                                         \
	return count;                                                             \
}

#define gov_pol_store(file_name)                                               \
static ssize_t store_##file_name##_gov_pol                                     \
(struct cpufreq_policy *policy, const char *buf, size_t count)                 \
{                                                                              \
	unsigned long val = 0;                                                     \
	int ret = kstrtoul(buf, 0, &val);                                          \
	if (ret < 0)                                                               \
		return ret;                                                            \
	((struct cpufreq_nexus_tunables *)policy->governor_data)->file_name = val; \
	return count;                                                              \
}

static ssize_t show_boostpulse_gov_sys(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ktime_to_us(ktime_get()) < global_tunables->boostpulse);
}

static ssize_t show_boostpulse_gov_pol(struct cpufreq_policy *policy, char *buf)
{
	return sprintf(buf, "%d\n", ktime_to_us(ktime_get()) < ((struct cpufreq_nexus_tunables *)policy->governor_data)->boostpulse);
}

static ssize_t store_boostpulse_gov_sys(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count) 
{
	unsigned long val = 0;
	int ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	global_tunables->boostpulse = ktime_to_us(ktime_get()) + val;
	return count;
}

static ssize_t store_boostpulse_gov_pol(struct cpufreq_policy *policy, const char *buf, size_t count)                 
{
	unsigned long val = 0;
	int ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	((struct cpufreq_nexus_tunables *)policy->governor_data)->boostpulse = ktime_to_us(ktime_get()) + val; 
	return count;
}

gov_sys_pol_show_store(down_load);
gov_sys_pol_show_store(down_step);
gov_sys_pol_show_store(up_load);
gov_sys_pol_show_store(up_step);
gov_sys_pol_show_store(sampling_rate);
gov_sys_pol_show_store(io_is_busy);
gov_sys_pol_show_store(freq_min);
gov_sys_pol_show_store(freq_max);
gov_sys_pol_show_store(boost);
gov_sys_pol(boostpulse);

static struct attribute *attributes_gov_sys[] = {
	&down_load_gov_sys.attr,
	&down_step_gov_sys.attr,
	&up_load_gov_sys.attr,
	&up_step_gov_sys.attr,
	&sampling_rate_gov_sys.attr,
	&io_is_busy_gov_sys.attr,
	&freq_min_gov_sys.attr,
	&freq_max_gov_sys.attr,
	&boost_gov_sys.attr,
	&boostpulse_gov_sys.attr,
	NULL // NULL has to be terminating entry
};

static struct attribute_group attribute_group_gov_sys = {
	.attrs = attributes_gov_sys,
	.name = "nexus",
};

static struct attribute *attributes_gov_pol[] = {
	&down_load_gov_pol.attr,
	&down_step_gov_pol.attr,
	&up_load_gov_pol.attr,
	&up_step_gov_pol.attr,
	&sampling_rate_gov_pol.attr,
	&io_is_busy_gov_pol.attr,
	&freq_min_gov_pol.attr,
	&freq_max_gov_pol.attr,
	&boost_gov_pol.attr,
	&boostpulse_gov_pol.attr,
	NULL // NULL has to be terminating entry
};

static struct attribute_group attribute_group_gov_pol = {
	.attrs = attributes_gov_pol,
	.name = "nexus",
};

static struct attribute_group *get_attribute_group(void) {
	if (have_governor_per_policy())
		return &attribute_group_gov_pol;
	else
		return &attribute_group_gov_sys;
}

static int cpufreq_governor_nexus(struct cpufreq_policy *policy, unsigned int event) {
	int rc = 0;
	int cpu, delay;
	struct cpufreq_nexus_cpuinfo *cpuinfo;
	struct cpufreq_nexus_tunables *tunables;

	cpu = policy->cpu;

	switch (event) {
		case CPUFREQ_GOV_POLICY_INIT:
			mutex_lock(&cpufreq_governor_nexus_mutex);

			if (!have_governor_per_policy()) {
				tunables = global_tunables;
			}

			tunables = kzalloc(sizeof(struct cpufreq_nexus_tunables), GFP_KERNEL);
			if (!tunables) {
				pr_err("%s: POLICY_INIT: kzalloc failed\n", __func__);
				mutex_unlock(&cpufreq_governor_nexus_mutex);
				return -ENOMEM;
			}

			tunables->down_load = DEFAULT_DOWN_LOAD;
			tunables->down_step = DEFAULT_DOWN_STEP;
			tunables->up_load = DEFAULT_UP_LOAD;
			tunables->up_step = DEFAULT_UP_STEP;
			tunables->sampling_rate = DEFAULT_SAMPLING_RATE;
			tunables->io_is_busy = DEFAULT_IO_IS_BUSY;
			tunables->freq_min = policy->min;
			tunables->freq_max = policy->max;
			tunables->boost = 0;
			tunables->boostpulse = 0;

			rc = sysfs_create_group(get_governor_parent_kobj(policy),
					get_attribute_group());
			if (rc) {
				pr_err("%s: POLICY_INIT: sysfs_create_group failed\n", __func__);
				kfree(tunables);
				mutex_unlock(&cpufreq_governor_nexus_mutex);
				return rc;
			}

			policy->governor_data = tunables;

			mutex_unlock(&cpufreq_governor_nexus_mutex);

			break;

		case CPUFREQ_GOV_POLICY_EXIT:
			cpuinfo = &per_cpu(gov_cpuinfo, cpu);
			tunables = policy->governor_data;

			mutex_lock(&cpufreq_governor_nexus_mutex);
			sysfs_remove_group(get_governor_parent_kobj(policy),
					get_attribute_group());

			if (tunables)
				kfree(tunables);
			mutex_unlock(&cpufreq_governor_nexus_mutex);

			break;

		case CPUFREQ_GOV_START:
			if (!cpu_online(cpu) || !policy->cur)
				return -EINVAL;

			mutex_lock(&cpufreq_governor_nexus_mutex);

			cpuinfo = &per_cpu(gov_cpuinfo, cpu);
			tunables = policy->governor_data;

			cpuinfo->cpu = cpu;
			cpuinfo->freq_table = cpufreq_frequency_get_table(cpu);
			cpuinfo->policy = policy;
			cpuinfo->init = 1;

			mutex_init(&cpuinfo->timer_mutex);

			delay = usecs_to_jiffies(tunables->sampling_rate);
			if (num_online_cpus() > 1) {
				delay -= jiffies % delay;
			}

			mutex_unlock(&cpufreq_governor_nexus_mutex);

			INIT_DEFERRABLE_WORK(&cpuinfo->work, cpufreq_nexus_timer);
			queue_delayed_work_on(cpuinfo->cpu, system_wq, &cpuinfo->work, delay);

			break;

		case CPUFREQ_GOV_STOP:
			cpuinfo = &per_cpu(gov_cpuinfo, cpu);
			tunables = policy->governor_data;

			cancel_delayed_work_sync(&cpuinfo->work);

			break;

		case CPUFREQ_GOV_LIMITS:
			mutex_lock(&cpufreq_governor_nexus_mutex);

			cpuinfo = &per_cpu(gov_cpuinfo, cpu);
			if (policy->max < cpuinfo->policy->cur) {
				__cpufreq_driver_target(cpuinfo->policy,
					policy->max, CPUFREQ_RELATION_H);
			} else if (policy->min > cpuinfo->policy->cur) {
				__cpufreq_driver_target(cpuinfo->policy,
					policy->min, CPUFREQ_RELATION_L);
			}

			mutex_unlock(&cpufreq_governor_nexus_mutex);

			break;
	}

	return 0;
}

static int __init cpufreq_nexus_init(void) {
	return cpufreq_register_governor(&cpufreq_gov_nexus);
}

static void __exit cpufreq_nexus_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_nexus);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_NEXUS
fs_initcall(cpufreq_nexus_init);
#else
module_init(cpufreq_nexus_init);
#endif
module_exit(cpufreq_nexus_exit);

MODULE_AUTHOR("Lukas Berger <mail@lukasberger.at>");
MODULE_DESCRIPTION("'cpufreq_nexus' - cpufreq-governor for interactive "
	"and battery-based devices");
MODULE_LICENSE("GPL");
