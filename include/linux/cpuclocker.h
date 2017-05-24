/*
 * Copyright (c) 2017  Lukas Berger
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <mach/cpufreq.h>

#ifdef CONFIG_EXYNOS7420_UNDERCLOCK
    #define EXYNOS7420_CLUSTER0_MIN_LEVEL    L18 // 200 MHz
    #define EXYNOS7420_CLUSTER1_MIN_LEVEL    L23 // 200 MHz
#else
    #define EXYNOS7420_CLUSTER0_MIN_LEVEL    L16 // 400 MHz
    #define EXYNOS7420_CLUSTER1_MIN_LEVEL    L17 // 800 MHz
#endif

#ifdef CONFIG_EXYNOS7420_OVERCLOCK

	/*
	 * Some overclock-informations:
	 *
	 *  * If overclock is enabled, bus-tables (memory throughput MB/sec) get increased
	 *
	 *  * If overclock is enabled, voltages get increased up to
	 *     - +0.25V (+0.05V per level) compared to default voltage on cluster0
	 *     - +0.20V (+0.05V per level) compared to default voltage on cluster1
	 */

    #define EXYNOS7420_CLUSTER0_MAX_LEVEL    L3 // 1704 MHz
    #define EXYNOS7420_CLUSTER1_MAX_LEVEL    L2 // 2300 MHz
#else
    #define EXYNOS7420_CLUSTER0_MAX_LEVEL    L5 // 1500 MHz
    #define EXYNOS7420_CLUSTER1_MAX_LEVEL    L4 // 2100 MHz
#endif
