/*
 * iav_pts.c
 *
 * History:
 *	2013/03/04 - [Zhaoyang Chen] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "amba_dsp.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_pts.h"
#include "iav_encode.h"

#define AMBA_HW_TIMER_FILE				"/proc/ambarella/ambarella_hwtimer"
#define MAX_LEADING_ZERO_FOR_HW_PTS		8
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
#define AMBA_AUDIO_CLOCK_FREQ			(amb_get_audio_clock_frequency(HAL_BASE_VP))
#else
#define AMBA_AUDIO_CLOCK_FREQ			12288000
#endif

#if !defined(BUILD_AMBARELLA_HW_TIMER)
int get_hwtimer_output_freq(u32 *out_freq)
{
	*out_freq = 0;
	return 0;
}
int get_hwtimer_output_ticks(u64 *out_tick)
{
	*out_tick = 0;
	return 0;
}
#endif

u64 get_hw_pts(u32 audio_pts)
{
	int audio_diff;
	u64 mono_pts = 0;
	static u32 leading = 1;
	static u32 count = 0;
	static u32 prev_pts = 0;
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;
	iav_hwtimer_pts_info_t *hw_pts_info = &pts_info->hw_pts_info;

	audio_diff = abs(hw_pts_info->audio_tick - audio_pts);
	if (unlikely(!hw_pts_info->audio_freq)) {
		iav_printk("HW PTS error: invalid audio frequency: %u!\n",
			hw_pts_info->audio_freq);
	} else if (unlikely(!audio_pts)) {
		// check leading zero pts case and continuous zero pts case
		if (leading && ++count > MAX_LEADING_ZERO_FOR_HW_PTS) {
			iav_printk("HW PTS error: too many leading zero PTS, Count: %d!\n",
				count);
		} else if (!leading && !prev_pts){
			iav_printk("HW PTS error: continuous 2 zero PTS!\n");
		}
	} else if (unlikely(audio_diff > 2 * hw_pts_info->audio_freq)) {
		iav_printk("HW PTS error: too big gap [%u] between sync point [%u] "
			"and pts [%u]!\n", audio_diff, hw_pts_info->audio_tick, audio_pts);
	} else {
		// got valid pts from dsp
		leading = 0;
		mono_pts = (u64)audio_diff * hw_pts_info->hwtimer_freq +
			hw_pts_info->audio_freq / 2;
		do_div(mono_pts, hw_pts_info->audio_freq);
		mono_pts = hw_pts_info->hwtimer_tick - mono_pts;
	}
	prev_pts = audio_pts;

	return mono_pts;
}

int hw_pts_init(void)
{
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;
	iav_hwtimer_pts_info_t *hw_pts_info = &pts_info->hw_pts_info;

	hw_pts_info->fp_timer = filp_open(AMBA_HW_TIMER_FILE, O_RDONLY, 0);
	if(IS_ERR(hw_pts_info->fp_timer)) {
		pts_info->hwtimer_enabled = 0;
		hw_pts_info->fp_timer = NULL;
		iav_printk("HW timer for pts is NOT enabled!\n");
	} else {
		pts_info->hwtimer_enabled = 1;
		filp_close(hw_pts_info->fp_timer, 0);
		iav_printk("HW timer for pts is enabled.\n");
	}

	if (pts_info->hwtimer_enabled == 1) {
		hw_pts_info->audio_freq = AMBA_AUDIO_CLOCK_FREQ;
		get_hwtimer_output_freq(&hw_pts_info->hwtimer_freq);
		iav_printk("Audio clock freq: %u, HW timer clock freq: %u!",
			hw_pts_info->audio_freq, hw_pts_info->hwtimer_freq);
	}

	return 0;
}

int hw_pts_deinit(void)
{
	return 0;
}

