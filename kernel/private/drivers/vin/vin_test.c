/*
 * kernel/private/drivers/ambarella/vin/vin_test.c
 *
 * Author: Louis Sun <lysun@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>
#include "vin_pri.h"
#include <asm/div64.h>

/* below code are enabled only when CONFIG_VIN_FPS_STAT enabled */

#ifdef CONFIG_VIN_FPS_STAT

#define MAX_TIMER_RECORDS	1001
#define MAX_SKIPPED_FRAMES	100
#define MAX_TIMER_VALUE		4294967296 //0xFFFFFFFF

static u32 g_timer_values[MAX_TIMER_RECORDS];
static u32 g_current_timer_count = 0;


static u32 g_skipped_frames = 0;
static u32 g_vin_stat_done = 0;

static u32 g_vin_fps_test_start = 0;

struct completion g_vin_fps_calc_cmpl;

void amba_vin_vsync_calc_fps_wait(void)
{
	wait_for_completion_interruptible(&g_vin_fps_calc_cmpl);
}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps_wait);

void amba_vin_vsync_update_fps_stat(void)
{
	//only work when vin fps is on
	if (!g_vin_fps_test_start)
		return;

	if (g_skipped_frames < MAX_SKIPPED_FRAMES)	{
		g_skipped_frames++;
		//skip some frames just in order to wait for VIN to become fully stable
	} else {
		if (g_current_timer_count < MAX_TIMER_RECORDS) {
			g_timer_values[g_current_timer_count] = (u32)amba_readl(TIMER1_STATUS_REG);
			g_current_timer_count++;
		} else {
			if (!g_vin_stat_done) {
				vin_dbg("VIN stat record is ready with %d entries\n", MAX_TIMER_RECORDS);
				g_vin_stat_done = 1; //stat is done, no more
				g_vin_fps_test_start = 0;	//set fps test to stopped
				complete(&g_vin_fps_calc_cmpl);
			}
		}
	}
}

int amba_vin_vsync_calc_fps_reset(void)
{
	g_skipped_frames = 0;
	g_current_timer_count = 0;
	g_vin_stat_done = 0;
	memset(g_timer_values, 0, sizeof(g_timer_values));
	init_completion(&g_vin_fps_calc_cmpl);

	//turn on the flag
	g_vin_fps_test_start = 1;
	return 0;
}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps_reset);

int amba_vin_vsync_calc_fps(struct amba_fps_report_s *fps_report)
{
	int i = 0 ;
	u32 g_timer_diff_values = 0;
	u64 g_timer_sum = 0;

	u32 avg_diff = 0;
	u32 max_diff = 0;
	u32 apb_clk = 0;
	u32 min_diff = 0;

	avg_diff = 1;
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	apb_clk = get_apb_bus_freq_hz();
#else
	apb_clk = clk_get_rate(clk_get(NULL, "gclk_apb"));
#endif
	max_diff = 1;
	min_diff = apb_clk;

	for (i = 0; i < MAX_TIMER_RECORDS-1 ; i++)
	{
		if (g_timer_values[i] > g_timer_values[i+1]) {
			g_timer_diff_values = g_timer_values[i] - g_timer_values[i+1];
		}else {
			g_timer_diff_values = MAX_TIMER_VALUE - g_timer_values[i+1] + g_timer_values[i];
		}

		g_timer_sum += g_timer_diff_values;

		if (g_timer_diff_values > max_diff)
			max_diff = g_timer_diff_values;

		if (g_timer_diff_values < min_diff)
			min_diff = g_timer_diff_values;
	}
	do_div(g_timer_sum, MAX_TIMER_RECORDS - 1);
	avg_diff = (u32) g_timer_sum;

	if(!fps_report) {
		DRV_PRINT("Null report pointer \n");
		return -1;
	}

	memset(fps_report, 0, sizeof(*fps_report));
	fps_report->avg_frame_diff = avg_diff;
	fps_report->counter_freq = apb_clk;
	fps_report->max_frame_diff = max_diff;
	fps_report->min_frame_diff = min_diff;

	return 0;

}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps);

#else
int amba_vin_vsync_calc_fps_reset(void)
{
	return -1;
}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps_reset);

void amba_vin_vsync_calc_fps_wait(void)
{
}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps_wait);

int amba_vin_vsync_calc_fps(struct amba_fps_report_s *fps_report)
{
	return -1;
}
EXPORT_SYMBOL(amba_vin_vsync_calc_fps);

#endif

