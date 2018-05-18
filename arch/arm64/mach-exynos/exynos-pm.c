/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Author: Hyosang Jung
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>

#include <mach/exynos-pm.h>

static DEFINE_RWLOCK(exynos_pm_notifier_lock);
static RAW_NOTIFIER_HEAD(exynos_pm_notifier_chain);

static int exynos_pm_notify(enum exynos_pm_event event, int nr_to_call, int *nr_calls)
{
	int ret;

	ret = __raw_notifier_call_chain(&exynos_pm_notifier_chain, event, NULL,
		nr_to_call, nr_calls);

	return notifier_to_errno(ret);
}

int exynos_pm_register_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&exynos_pm_notifier_lock, flags);
	ret = raw_notifier_chain_register(&exynos_pm_notifier_chain, nb);
	write_unlock_irqrestore(&exynos_pm_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_pm_register_notifier);

int exynos_pm_unregister_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&exynos_pm_notifier_lock, flags);
	ret = raw_notifier_chain_unregister(&exynos_pm_notifier_chain, nb);
	write_unlock_irqrestore(&exynos_pm_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_pm_unregister_notifier);

#ifdef CONFIG_SEC_PM_DEBUG
static unsigned int lpa_log_en;
module_param(lpa_log_en, uint, 0644);
#endif

static const char *exynos_critical_irqs[] =
{
	// Exynos
	"exynos-pcie",
	"exynos_tmu",
	"bt_host_wake",
	"mobicore",
	"sec-nfc",
	"sec-pmic-irq",

	// storage
	"ufshcd",

	// USB
	"dwc3",

	// GPS
	"ttyBCM",

	// display
	"13900000.dsim",
	"13930000.decon_fb",
	"14ac0000.mali",

	// input
	"fts_touch",
	"sec_touchkey",

	// DMA
	"10e10000.pdma0",
	"10eb0000.pdma1",

	// VPP
	"13e02000.vpp",
	"13e03000.vpp",
	"13e04000.vpp",
	"13e05000.vpp",

	// SCALER
	"15000000.vpp",
	"15010000.vpp",

	// media
	"15020000.jpeg0",
	"15100000.fimg2d",
	"152e0000.mfc0",

	// sound
	"arizona",

	// MIPI-LLI
	"10f24000.mipi-lli",
	"lli_cp2ap_status",
	"lli_cp2ap_wakeup",

	// Modem
	"ss333_active",

	// MMU
	"114e0000.sysmmu",
	"13a00000.sysmmu",
	"13a10000.sysmmu",
	"13e20000.sysmmu",
	"13e30000.sysmmu",
	"140e0000.sysmmu",
	"141a0000.sysmmu",
	"141b0000.sysmmu",
	"141c0000.sysmmu",
	"141d0000.sysmmu",
	"141e0000.sysmmu",
	"14270000.sysmmu",
	"14280000.sysmmu",
	"15040000.sysmmu",
	"15140000.sysmmu",
	"15160000.sysmmu",
	"15200000.sysmmu",
	"15210000.sysmmu",

	// Circuits
	"13620000.adc",
	"13640000.hsi2c",
	"13650000.hsi2c",
	"13660000.hsi2c",
	"13670000.hsi2c",
	"136a0000.hsi2c",
	"14e00000.hsi2c",
	"14e10000.hsi2c",
	"14e60000.hsi2c",
	"14e70000.hsi2c",
	"10580000.pinctrl",
	"10e60000.pinctrl",
	"13470000.pinctrl",
	"14870000.pinctrl",
	"14cd0000.pinctrl",
	"14ce0000.pinctrl",
	"15690000.pinctrl",

	NULL
};

static const char *exynos_critical_threads[] =
{
	// display
	"decon0",
	"decon1",

	// system
	"zygote",
	"zygote64",
	"surfaceflinger",

	// camera
	"kfimg2dd",
	"cameraserver",

	NULL
};

bool exynos_is_critical_irq(const char *irq) {
	int i;

	for (i = 0; exynos_critical_irqs[i]; i++) {
		if (!strcmp(irq, exynos_critical_irqs[i])) {
			return true;
		}
	}

	return false;
}

bool exynos_is_critical_thread(const char *thread) {
	int i;

	for (i = 0; exynos_critical_threads[i]; i++) {
		if (!strcmp(thread, exynos_critical_threads[i])) {
			return true;
		}
	}

	return false;
}

int exynos_lpa_prepare(void)
{
	int nr_calls = 0;
	int ret = 0;

	read_lock(&exynos_pm_notifier_lock);
	ret = exynos_pm_notify(LPA_PREPARE, -1, &nr_calls);
#ifdef CONFIG_SEC_PM_DEBUG
	if (unlikely(lpa_log_en) && ret < 0) {
		struct notifier_block *nb, *next_nb;
		struct notifier_block *nh = exynos_pm_notifier_chain.head;
		int nr_to_call = nr_calls - 1;

		nb = rcu_dereference_raw(nh);

		while (nb && nr_to_call) {
			next_nb = rcu_dereference_raw(nb->next);
			nb = next_nb;
			nr_to_call--;
		}

		if (nb)
			pr_info("%s: failed at %ps\n", __func__,
					(void *)nb->notifier_call);
	}
#endif
	read_unlock(&exynos_pm_notifier_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_lpa_prepare);

int exynos_lpa_enter(void)
{
	int nr_calls;
	int ret = 0;

	read_lock(&exynos_pm_notifier_lock);
	ret = exynos_pm_notify(LPA_ENTER, -1, &nr_calls);
	if (ret)
		/*
		 * Inform listeners (nr_calls - 1) about failure of CPU PM
		 * PM entry who are notified earlier to prepare for it.
		 */
		exynos_pm_notify(LPA_ENTER_FAIL, nr_calls - 1, NULL);
	read_unlock(&exynos_pm_notifier_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_lpa_enter);

int exynos_lpa_exit(void)
{
	int ret;

	read_lock(&exynos_pm_notifier_lock);
	ret = exynos_pm_notify(LPA_EXIT, -1, NULL);
	read_unlock(&exynos_pm_notifier_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_lpa_exit);

#ifdef CONFIG_SEC_PM_DEBUG
static unsigned int lpc_log_en;
module_param(lpc_log_en, uint, 0644);
#endif

int exynos_lpc_prepare(void)
{
	int nr_calls = 0;
	int ret = 0;

	read_lock(&exynos_pm_notifier_lock);
	ret = exynos_pm_notify(LPC_PREPARE, -1, &nr_calls);
#ifdef CONFIG_SEC_PM_DEBUG
	if (unlikely(lpc_log_en) && ret < 0) {
		struct notifier_block *nb, *next_nb;
		struct notifier_block *nh = exynos_pm_notifier_chain.head;
		int nr_to_call = nr_calls - 1;

		nb = rcu_dereference_raw(nh);

		while (nb && nr_to_call) {
			next_nb = rcu_dereference_raw(nb->next);
			nb = next_nb;
			nr_to_call--;
		}

		if (nb)
			pr_info("%s: failed at %ps\n", __func__,
					(void *)nb->notifier_call);
	}
#endif
	read_unlock(&exynos_pm_notifier_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_lpc_prepare);
