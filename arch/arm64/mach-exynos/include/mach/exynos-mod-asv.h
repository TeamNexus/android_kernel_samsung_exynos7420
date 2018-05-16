/*
 * include/linux/exynos-mod-asv.h - Exynos ASV modding table
 *
 * Copyright (C) 2018 Lukas Berger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/asv-exynos_cal.h>

#define EXYNOS_MOD_ASV_TABLE

#ifndef SYSC_DVFS_END_LVL_BIG
	#define SYSC_DVFS_END_LVL_BIG    SYSC_DVFS_L24
#endif

#ifndef SYSC_DVFS_END_LVL_LIT
	#define SYSC_DVFS_END_LVL_LIT    SYSC_DVFS_L19
#endif

#ifndef SYSC_DVFS_END_LVL_G3D
	#define SYSC_DVFS_END_LVL_G3D    SYSC_DVFS_L10
#endif

#ifndef MAX_ASV_GROUP
	#define MAX_ASV_GROUP    16
#endif

#define __EMDV(v, d) ((v - d) <= 0 ? 0 : (v - d))

/*
 *  ASV0                ASV1                ASV2                ASV3                ASV4
 *  ASV5                ASV6                ASV7                ASV8                ASV9
 *  ASV10               ASV11               ASV12               ASV13               ASV14
 *  ASV15
 */
#define __EXYNOS_MOD_DEFINE_ASV(volt) \
	__EMDV(volt, 7500), __EMDV(volt, 6750), __EMDV(volt, 6000), __EMDV(volt, 5250), __EMDV(volt, 4500), \
	__EMDV(volt, 3750), __EMDV(volt, 3000), __EMDV(volt, 2250), __EMDV(volt, 1500), __EMDV(volt, 750), \
	__EMDV(volt, 0),    __EMDV(volt, 0),    __EMDV(volt, 0),    __EMDV(volt, 0),    __EMDV(volt, 0), \
	__EMDV(volt, 0),

#define EXYNOS_MOD_VOLT_ASV_GROUPS    (MAX_ASV_GROUP + 1)

#define EXYNOS_MOD_VOLT_TABLE_BIG_SIZE    (SYSC_DVFS_END_LVL_BIG + 1)
const u32 exynos_mod_volt_table_big[EXYNOS_MOD_VOLT_TABLE_BIG_SIZE][EXYNOS_MOD_VOLT_ASV_GROUPS] = {
	/* FREQ   ASV */
	{  2496,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  2400,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  2304,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  2200,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  2100,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  2000,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1896,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1800,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1704,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1600,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1500,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1400,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1300,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1200,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1100,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1000,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   900,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   800,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   700,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   600,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   500,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   400,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   300,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   200,  __EXYNOS_MOD_DEFINE_ASV(50000) },
};

#define EXYNOS_MOD_VOLT_TABLE_LIT_SIZE    (SYSC_DVFS_END_LVL_LIT + 1)
const u32 exynos_mod_volt_table_lit[EXYNOS_MOD_VOLT_TABLE_LIT_SIZE][EXYNOS_MOD_VOLT_ASV_GROUPS] = {
	/* FREQ   ASV */
	{  2000,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1900,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1800,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1704,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1600,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1500,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1400,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1296,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1200,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1104,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{  1000,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   900,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   800,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   700,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   600,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   500,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   400,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   300,  __EXYNOS_MOD_DEFINE_ASV(50000) },
	{   200,  __EXYNOS_MOD_DEFINE_ASV(50000) },
};

#define EXYNOS_MOD_VOLT_TABLE_G3D_SIZE    (SYSC_DVFS_END_LVL_G3D + 1)
const u32 exynos_mod_volt_table_g3d[EXYNOS_MOD_VOLT_TABLE_G3D_SIZE][EXYNOS_MOD_VOLT_ASV_GROUPS] = {
	/* FREQ   ASV */
	{   852,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   772,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   700,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   600,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   544,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   420,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   350,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   266,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   160,  __EXYNOS_MOD_DEFINE_ASV(0) },
	{   100,  __EXYNOS_MOD_DEFINE_ASV(0) },
};
