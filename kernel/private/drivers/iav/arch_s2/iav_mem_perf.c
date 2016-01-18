/*
 * iav_mem_perf.c
 *
 * History:
 *	2012/12/12 - [Jian Tang] created file
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

#include "iav_common.h"
#include "iav_drv.h"

#include "utils.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_capture.h"
#include "iav_warp.h"
#include "iav_mem_perf.h"


#define	DRAM_BW_DSP_PER		(55)
#define	DRAM_BW_MARGIN_PER	(0)
#define	DRAM_SIZE_MARGIN_MIN		(1 << 20)

#define	AAA_CFA_SIZE			(16000)
#define	AAA_RGB_SIZE			(5248)

typedef struct iav_system_freq_s {
	u32	cortex_MHz;
	u32	idsp_MHz;
	u32	core_MHz;
	u32	dram_MHz;
} iav_system_freq_t;

typedef struct dsp_mem_usage_s {
	/* Memory bandwidth */
	u8	cfa_fact;
	u8	main_420_fact;
	u8	main_400_fact;
	u8	main_comp_fact;
	u8	pm_fact;
	u8	division_fact;
	u8	reserved;

	/* DRAM size */
	u8	raw_size_fact;
	u8	stream_fact;
	u8	aaa_cfa_fact;
	u8	aaa_rgb_fact;
	u8	pre_main_420_fact;
	u8	buffer_420_fact[IAV_MAX_SOURCE_BUFFER_NUM];
	u8	me1_fact[IAV_MAX_SOURCE_BUFFER_NUM];
} dsp_mem_usage_t;


static iav_system_freq_t G_iav_freq[IAV_CHIP_ID_S2_TOTAL_NUM] = {
	[IAV_CHIP_ID_S2_33] = {
		.cortex_MHz = 500,
		.idsp_MHz = 228,
		.core_MHz = 224,
		.dram_MHz = 408,
	},
	[IAV_CHIP_ID_S2_55] = {
		.cortex_MHz = 500,
		.idsp_MHz = 312,
		.core_MHz = 264,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2_66] = {
		.cortex_MHz = 750,
		.idsp_MHz = 312,
		.core_MHz = 384,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2_88] = {
		.cortex_MHz = 1000,
		.idsp_MHz = 336,
		.core_MHz = 384,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2_99] = {
		.cortex_MHz = 1000,
		.idsp_MHz = 336,
		.core_MHz = 384,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2_22] = {
		.cortex_MHz = 500,
		.idsp_MHz = 192,
		.core_MHz = 168,
		.dram_MHz = 408,
	},
};

static dsp_mem_usage_t G_dsp_mem_usage[DSP_ENCODE_MODE_TOTAL_NUM] = {
	[DSP_FULL_FRAMERATE_MODE] = {
		.cfa_fact = 0,
		.main_420_fact = 8,
		.main_400_fact = 0,
		.main_comp_fact = 0,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 0,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			13,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		.cfa_fact = 0,
		.main_420_fact = 1,
		.main_400_fact = 0,
		.main_comp_fact = 0,
		.pm_fact = 1,
		.division_fact = 1,

		.raw_size_fact = 0,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 5,
		.buffer_420_fact = {
			15,	9,	0,	9,
		},
		.me1_fact = {
			12,	10,	0,	10,
		},
	},
	[DSP_HIGH_MEGA_PIXEL_MODE] = {
		.cfa_fact = 1,
		.main_420_fact = 6,
		.main_400_fact = 0,
		.main_comp_fact = 1,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			9,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_CALIBRATION_MODE] = {
		.cfa_fact = 1,
		.main_420_fact = 6,
		.main_400_fact = 0,
		.main_comp_fact = 1,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			10,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_HDR_FRAME_INTERLEAVED_MODE] = {
		.cfa_fact = 0,
		.main_420_fact = 2 * 4 + 6,
		.main_400_fact = 0,
		.main_comp_fact = 2 * 4,
		.pm_fact = 0,
		.division_fact = 4,

		.raw_size_fact = 2,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			5,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		.cfa_fact = 2,
		.main_420_fact = 8,
		.main_400_fact = 2,
		.main_comp_fact = 0,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			5,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_HIGH_MP_FULL_PERF_MODE] = {
		.cfa_fact = 1,
		.main_420_fact = 4,
		.main_400_fact = 0,
		.main_comp_fact = 1,
		.pm_fact = 1,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			6,	9,	9,	9,
		},
		.me1_fact = {
			8,	10,	10,	10,
		},
	},
	[DSP_FULL_FPS_FULL_PERF_MODE] = {
		.cfa_fact = 0,
		.main_420_fact = 6,
		.main_400_fact = 0,
		.main_comp_fact = 0,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 0,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			13,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},
	[DSP_MULTI_VIN_MODE] = {
		.cfa_fact = 1,
		.main_420_fact = 4,
		.main_400_fact = 0,
		.main_comp_fact = 1,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			6,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},

	[DSP_HISO_VIDEO_MODE] = {
		.cfa_fact = 1,
		.main_420_fact = 4,
		.main_comp_fact = 1,
		.pm_fact = 0,
		.division_fact = 1,

		.raw_size_fact = 5,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 0,
		.buffer_420_fact = {
			9,	9,	9,	9,
		},
		.me1_fact = {
			11,	10,	10,	10,
		},
	},

	[DSP_HIGH_MP_WARP_MODE] = {
		.cfa_fact = 0,
		.main_420_fact = 1,
		.main_400_fact = 0,
		.main_comp_fact = 0,
		.pm_fact = 1,
		.division_fact = 1,

		.raw_size_fact = 3,
		.stream_fact = 4,
		.aaa_cfa_fact = 6,
		.aaa_rgb_fact = 6,
		.pre_main_420_fact = 5,
		.buffer_420_fact = {
			15,	9,	0,	9,
		},
		.me1_fact = {
			11,	10,	0,	10,
		},
	},
};
static struct notifier_block G_audio_event;

/******************************************
 *
 *	Internal helper functions
 *
 ******************************************/

static int calc_warp_mem_bandwidth(u32 param_addr,
	u32 vin_fps, u32 * bw_in_KBs)
{
#define	MEM_BW_DEWARP_MAIN_NUM		(5)
	int i, rotate;
	u32 mem_bw;
	u32 dewarp_main_420;
	u32 output_h;
	iav_warp_vector_ex_t * warp_area = NULL;
	iav_warp_control_ex_t * warp_ctrl = NULL;

	warp_ctrl = (param_addr == 0) ? &G_warp_control :
		(iav_warp_control_ex_t *)param_addr;
	mem_bw = dewarp_main_420 = 0;
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		warp_area = &warp_ctrl->area[i];
		if (warp_area->enable) {
			if (warp_area->dup) {
				/* Warp copy */
				rotate = 0;
				output_h = 0;
			} else {
				/* Warp transform */
				if (warp_area->rotate_flip & ROTATE_90) {
					rotate = 1;
					output_h = MIN(warp_area->input.width,
						warp_area->output.height);
				} else {
					rotate = 0;
					output_h = MIN(warp_area->input.height,
						warp_area->output.height);
				}
			}
			if (is_hwarp_bypass_enabled() &&
				(warp_area->input.width == warp_area->output.width) &&
				(warp_area->input.height <= warp_area->output.height) &&
				(!warp_area->rotate_flip)) {
				mem_bw += (warp_area->output.width * output_h +
					warp_area->output.width * warp_area->output.height) / 2 * 3;
			} else {
				mem_bw += (warp_area->input.width * warp_area->input.height *
					(1 + rotate) + warp_area->output.width * output_h * 2 +
					 warp_area->output.width * warp_area->output.height) / 2 * 3;
			}
			dewarp_main_420 += warp_area->output.width * warp_area->output.height / 2 * 3;
		}
	}

	mem_bw += MEM_BW_DEWARP_MAIN_NUM * dewarp_main_420;

	*bw_in_KBs += (mem_bw >> 10) * vin_fps;
	return 0;
}

static int check_audio_clock(struct notifier_block *nb, unsigned long val, void *data)
{
	int rval = NOTIFY_OK;
	struct ambarella_i2s_interface *i2s_config = data;
	u32 delta, audio_hz, audio_target;

	audio_hz = i2s_config->mclk;
	delta = 100;
	audio_target = AUDIO_CLK_KHZ * 1000;
	if ((audio_hz < (audio_target - delta)) || (audio_hz > (audio_target + delta))) {
		iav_error("Invalid Audio clock [%d], it must be around [%d].\n",
			audio_hz, audio_target);
	}

	return rval;
}

static int check_system_clock(void)
{
	static int done = 0;
	u32 delta, hz, hz_target, chip_id;

	if (!done) {
		delta = 100;

		if (unlikely(G_iav_obj.dsp_chip_id == IAV_CHIP_ID_S2_UNKNOWN)) {
			G_iav_obj.dsp_chip_id = dsp_get_chip_id();
		}
		chip_id = G_iav_obj.dsp_chip_id;

		hz_target = G_iav_freq[chip_id].cortex_MHz * 1000 * 1000;
		hz = get_cortex_freq_hz();
		if (hz > hz_target + delta) {
			iav_error("Cortex clock: target [%d], real [%d].\n", hz_target, hz);
			return -1;
		}
		if (hz < hz_target - delta) {
			iav_warning("Cortex clock real [%d] lower than target [%d].\n",
				hz, hz_target);
		}

		hz_target = G_iav_freq[chip_id].idsp_MHz * 1000 * 1000;
		hz = get_idsp_freq_hz();
		if (hz > hz_target + delta) {
			iav_error("IDSP clock: target [%d], real [%d].\n", hz_target, hz);
			return -1;
		}
		if (hz < hz_target - delta) {
			iav_warning("IDSP clock real [%d] lower than target [%d].\n",
				hz, hz_target);
		}

		hz_target = G_iav_freq[chip_id].core_MHz * 1000 * 1000;
		hz = get_core_bus_freq_hz();
		if (hz > hz_target + delta) {
			iav_error("DSP core clock: target [%d], real [%d].\n", hz_target, hz);
			return -1;
		}
		if (hz < hz_target - delta) {
			iav_warning("DSP core clock real [%d] lower than target [%d].\n",
				hz, hz_target);
		}

		hz_target = G_iav_freq[chip_id].dram_MHz * 1000 * 1000;
		hz = get_dram_freq_hz();
		if (hz > hz_target + delta) {
			iav_error("DRAM clock: target [%d], real [%d].\n", hz_target, hz);
			return -1;
		}
		if (hz < hz_target - delta) {
			iav_warning("DRAM clock real [%d] lower than target [%d].\n",
				hz, hz_target);
		}

		G_audio_event.notifier_call = check_audio_clock;
		ambarella_audio_register_notifier(&G_audio_event);

		done = 1;
	}

	return 0;
}

static int check_dsp_mem_bandwidth(u32 encode_mode, u32 param_addr)
{
	u8 expo_fact;
	u32 mem_bw_KBs, mem_limit_KBs;
	u32 vin_fps, pre_main_w, pre_main_h;
	u32 cfa_size, main_420, main_400, main_comp, privacy_mask;
	u32 vin_framerate = get_vin_capability()->frame_rate;
	iav_rect_ex_t vin;
	dsp_mem_usage_t * perf = &G_dsp_mem_usage[encode_mode];

	vin_fps = DIV_ROUND(512000000, vin_framerate);
	pre_main_w = ALIGN(G_cap_pre_main.size.width, PIXEL_IN_MB);
	pre_main_h = ALIGN(G_cap_pre_main.size.height, PIXEL_IN_MB);

	mem_bw_KBs = 0;
	if ((encode_mode == DSP_MULTI_REGION_WARP_MODE) ||
		(encode_mode == DSP_HIGH_MP_WARP_MODE)) {
		if (calc_warp_mem_bandwidth(param_addr, vin_fps, &mem_bw_KBs) < 0) {
			return -1;
		}
	}

	get_vin_window(&vin);
	cfa_size = ALIGN(vin.width, PIXEL_IN_MB) * ALIGN(vin.height, PIXEL_IN_MB);
	if (is_raw_capture_enabled()) {
		cfa_size <<= 1;
	} else {
		cfa_size = (cfa_size >> 5) * 27;
	}
	main_400 = pre_main_w * pre_main_h;
	main_420 = main_400 / 2 * 3;
	main_comp = cfa_size;
	privacy_mask = ALIGN(G_cap_pre_main.input.width, PIXEL_IN_MB) *
		ALIGN(G_cap_pre_main.input.height, PIXEL_IN_MB);

	expo_fact = get_expo_num(encode_mode);
	mem_bw_KBs += ((perf->cfa_fact * cfa_size * expo_fact +
		(perf->main_420_fact + 2 * (expo_fact - 1)) * main_420 +
		perf->main_400_fact * main_400 +
		perf->main_comp_fact * main_comp +
		perf->pm_fact * (privacy_mask >> 5) * 27) >> 10) *
		vin_fps / perf->division_fact;

	mem_limit_KBs = ((G_iav_freq[G_iav_obj.dsp_chip_id].dram_MHz * 8 *
		DRAM_BW_DSP_PER * 100u) >> 10) * (100 + DRAM_BW_MARGIN_PER);

	iav_printk("DSP memory bandwidth [%u MB/s], limit [%u MB/s].\n",
		(mem_bw_KBs >> 10), (mem_limit_KBs >> 10));
	if (mem_bw_KBs > mem_limit_KBs) {
		iav_error("DSP memory bandwidth [%u MB/s] is out of the limit [%u MB/s]"
			" for mode [%d]. Please reduce VIN size or main buffer size!\n",
			(mem_bw_KBs >> 10), (mem_limit_KBs >> 10), encode_mode);
		return -1;
	}

	return 0;
}

static int check_dsp_dram_size(u32 encode_mode)
{
	int i, max_enc_num;
	u32 dram, dram_limit, raw_size;
	dsp_mem_usage_t * perf = &G_dsp_mem_usage[encode_mode];
	DSP_ENC_CFG * enc_cfg = NULL;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;
	iav_source_buffer_dram_ex_t * buf = NULL;
	iav_rect_ex_t vin;
	struct iav_mem_block * dsp = NULL;
	u8 extra_dewarp_hight[IAV_ENCODE_SOURCE_TOTAL_NUM] = {0};

	get_vin_window(&vin);
	raw_size = ALIGN(vin.width, PIXEL_IN_MB) * ALIGN(vin.height, PIXEL_IN_MB);
	if (is_raw_capture_enabled()) {
		raw_size <<= 1;
	} else {
		raw_size = (raw_size >> 5) * 27;
	}
	resource = &G_system_resource_setup[encode_mode];
	if ((encode_mode == DSP_MULTI_REGION_WARP_MODE) ||
		(encode_mode == DSP_HIGH_MP_WARP_MODE)) {
		extra_dewarp_hight[IAV_ENCODE_SOURCE_MAIN_BUFFER] = PIXEL_IN_MB;
		if (resource->max_preview_C_height > 0) {
			extra_dewarp_hight[IAV_ENCODE_SOURCE_SECOND_BUFFER] = PIXEL_IN_MB;
		}
		if (resource->max_preview_A_height > 0) {
			extra_dewarp_hight[IAV_ENCODE_SOURCE_FOURTH_BUFFER] = PIXEL_IN_MB;
		}
	}
	dram = perf->raw_size_fact * raw_size;
	dram += ALIGN(G_cap_pre_main.size.width, PIXEL_IN_MB) / 16 *
		ALIGN(G_cap_pre_main.size.height, PIXEL_IN_MB) *
			perf->pre_main_420_fact * 24 +
		ALIGN(resource->max_main_width, PIXEL_IN_MB) / 16 *
		(ALIGN(resource->max_main_height, PIXEL_IN_MB) +
		extra_dewarp_hight[IAV_ENCODE_SOURCE_MAIN_BUFFER]) *
			((perf->buffer_420_fact[0] +
			Get_EXTRA_BUF_NUM(resource->extra_dram_buf,
			resource->extra_buf_msb_ext)) * 24 +
				perf->me1_fact[0]) +
		ALIGN(resource->max_preview_C_width, PIXEL_IN_MB) / 16 *
		(ALIGN(resource->max_preview_C_height, PIXEL_IN_MB) +
		extra_dewarp_hight[IAV_ENCODE_SOURCE_SECOND_BUFFER]) *
			((perf->buffer_420_fact[1] +
			Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_c,
			resource->extra_buf_msb_ext_prev_c)) * 24 +
				perf->me1_fact[1]) +
		ALIGN(resource->max_preview_B_width, PIXEL_IN_MB) / 16 *
		ALIGN(resource->max_preview_B_height, PIXEL_IN_MB) *
			((perf->buffer_420_fact[2] +
			Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_b,
			resource->extra_buf_msb_ext_prev_b)) * 24 +
				perf->me1_fact[2]) +
		ALIGN(resource->max_preview_A_width, PIXEL_IN_MB) / 16 *
		(ALIGN(resource->max_preview_A_height, PIXEL_IN_MB) +
		extra_dewarp_hight[IAV_ENCODE_SOURCE_FOURTH_BUFFER])*
			((perf->buffer_420_fact[3] +
			Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_a,
			resource->extra_buf_msb_ext_prev_a)) * 24 +
				perf->me1_fact[3]);
	for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
		i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
		buf = &G_iav_obj.source_buffer[i].dram;
		dram += buf->buf_pitch / 16 * buf->buf_max_size.height *
			(24 + 1) * buf->max_frame_num;
	}

	max_enc_num = get_max_enc_num(encode_mode);
	for (i = 0; i < max_enc_num; ++i) {
		enc_cfg = get_enc_cfg(encode_mode, i);
		dram += ALIGN(enc_cfg->max_enc_width, PIXEL_IN_MB) *
			ALIGN(enc_cfg->max_enc_height, PIXEL_IN_MB) /
			2 * perf->stream_fact * 3;
	}

	dram += perf->aaa_cfa_fact * AAA_CFA_SIZE +
		perf->aaa_rgb_fact * AAA_RGB_SIZE;
	dram += DRAM_SIZE_MARGIN_MIN;

	iav_get_mem_block(IAV_MMAP_DSP, &dsp);
	dram_limit = dsp->size;
	iav_printk("DSP DRAM size used [%u MB], limit [%u MB].\n",
		(dram >> 20), (dram_limit >> 20));
	if (dram > dram_limit) {
		iav_error("DSP DRAM [%u MB] exceeds the limit  [%u MB] for mode [%d]."
			" Please reduce VIN size / max buffer size / max stream size!\n",
			(dram >> 20), (dram_limit >> 20), encode_mode);
		return -1;
	}

	return 0;
}


/******************************************
 *
 *	External IAV IOCTLs functions
 *
 ******************************************/

int iav_check_system_config(u32 mode, u32 param_addr)
{
	iav_no_check();

	if (check_system_clock() < 0) {
		iav_error("Invalid system clock for S2 chip [%d].\n", G_iav_obj.dsp_chip_id);
		return -1;
	}
	if (check_dsp_mem_bandwidth(mode, param_addr) < 0) {
		iav_error("DSP memory bandwidth is NOT enough in mode [%d].\n", mode);
		return -1;
	}
	if (check_dsp_dram_size(mode) < 0) {
		iav_error("DSP DRAM size is not enough in mode [%d].\n", mode);
		return -1;
	}

	return 0;
}

