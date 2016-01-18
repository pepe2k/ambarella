/*
 * iav_capture.c
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


#include "iav_capture.h"

struct iav_debug_info * G_iav_debug_info = NULL;

iav_source_buffer_ex_t G_cap_pre_main = {
	.type = IAV_SOURCE_BUFFER_TYPE_ENCODE,
	.state = IAV_SOURCE_BUFFER_STATE_IDLE,
	.size = {
		.width = 1920,
		.height = 1080,
	},
	.input = {
		.width = 0,
		.height = 0,
		.x = 0,
		.y = 0,
	},
	.property = {
		.max = {
			.width = MAX_WIDTH_FOR_1ST,
			.height = MAX_HEIGHT_FOR_1ST,
		},
		.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_1ST,
		.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_1ST,
	},
	.ref_count = 0,
};

DSP_SET_OP_MODE_IPCAM_RECORD_CMD G_system_resource_setup[DSP_ENCODE_MODE_TOTAL_NUM] =
{
	[DSP_FULL_FRAMERATE_MODE] = {
		.mode_flags = DSP_FULL_FRAMERATE_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		.mode_flags = DSP_MULTI_REGION_WARP_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 1920,
		.max_warped_main_height = 1080,
		.max_warped_region_input_width = 1920,
		.max_warped_region_output_width = 1920,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 1,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_HIGH_MEGA_PIXEL_MODE] = {
		.mode_flags = DSP_HIGH_MEGA_PIXEL_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 3840,
		.max_main_height = 2160,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_32_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_CALIBRATION_MODE] = {
		.mode_flags = DSP_CALIBRATION_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.raw_compression_disabled = 1,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_32_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_HDR_FRAME_INTERLEAVED_MODE] = {
		.mode_flags = DSP_HDR_FRAME_INTERLEAVED_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.hdr_num_exposures_minus_1 = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		.mode_flags = DSP_HDR_LINE_INTERLEAVED_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.hdr_num_exposures_minus_1 = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_HIGH_MP_FULL_PERF_MODE] = {
		.mode_flags = DSP_HIGH_MP_FULL_PERF_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),


		.max_main_width = 3840,
		.max_main_height = 2160,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_32_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_FULL_FPS_FULL_PERF_MODE] = {
		.mode_flags = DSP_FULL_FPS_FULL_PERF_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_32_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_MULTI_VIN_MODE] = {
		.mode_flags = DSP_MULTI_VIN_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 0,
		.raw_compression_disabled = 0,
		.num_vin_minus_2 = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),


		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height =720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_32_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},

	[DSP_HISO_VIDEO_MODE] = {
		.mode_flags = DSP_HISO_VIDEO_MODE,
		.max_num_enc = 2,	// IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 1,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),

		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height = 720,
		.max_warped_main_width = 0,
		.max_warped_main_height = 0,
		.max_warped_region_input_width = 0,
		.max_warped_region_output_width = 0,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 0,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
	[DSP_HIGH_MP_WARP_MODE] = {
		.mode_flags = DSP_HIGH_MP_WARP_MODE,
		.max_num_enc = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.preview_A_for_enc = 0,
		.preview_B_for_enc = 0,
		.enc_rotation = 0,
		.raw_compression_disabled = 0,
		.extra_dram_buf = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_a = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_a = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_b = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_b = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_dram_buf_prev_c = SET_EXTRA_BUF(IAV_EXTRA_BUF_FRAME_DEFAULT),
		.extra_buf_msb_ext_prev_c = SET_EXTRA_BUF_MSB(IAV_EXTRA_BUF_FRAME_DEFAULT),


		.max_main_width = 1920,
		.max_main_height = 1080,
		.max_preview_C_width = 720,
		.max_preview_C_height = 576,
		.max_preview_B_width = 1280,
		.max_preview_B_height = 720,
		.max_preview_A_width = 1280,
		.max_preview_A_height = 720,
		.max_warped_main_width = 1920,
		.max_warped_main_height = 1080,
		.max_warped_region_input_width = 1920,
		.max_warped_region_output_width = 1920,
		.max_vin_stats_num_lines_top = 0,
		.max_vin_stats_num_lines_bot = 0,
		.max_chroma_filter_radius = CHROMA_NOISE_128_SHIFT,
		.h_warp_bypass = 1,

		.enc_cfg = {
			{1920, 1088, 1},		// stream 0
			{1280, 720, 1},		// stream 1
			{720, 480, 1},			// stream 2
			{720, 480, 1},			// stream 3
			{352, 240, 1},			// stream 4
			{352, 240, 1},			// stream 5
			{352, 240, 1},			// stream 6
			{352, 240, 1},			// stream 7
		},
	},
};

static DSP_ENCODE_MODE G_dsp_enc_mode = DSP_FULL_FRAMERATE_MODE;

iav_enc_mode_limit_t G_modes_limit[DSP_ENCODE_MODE_TOTAL_NUM] =
{
	[DSP_FULL_FRAMERATE_MODE] = {
		// Full frame rate mode
		.name = "Full FPS",
		.main_width_min = MIN_WIDTH_IN_FULL_FPS,
		.main_height_min = MIN_HEIGHT_IN_FULL_FPS,
		.main_width_max = MAX_WIDTH_IN_FULL_FPS,
		.main_height_max = MAX_HEIGHT_IN_FULL_FPS,
		.capture_pps_max = MAX_CAP_PPS_IN_FULL_FPS,
		.rotate_possible = 1,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 1,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 1,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 1,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 1,		/* Use Mixer B to do VCA duplication */
		.max_streams_num = IAV_MAX_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P110,
		.min_encode_width = MIN_WIDTH_IN_FULL_FPS,
		.min_encode_height = MIN_HEIGHT_IN_FULL_FPS,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		// Multi region warping mode
		.name = "Warping",
		.main_width_min = MIN_WIDTH_IN_WARP,
		.main_height_min = MIN_HEIGHT_IN_WARP,
		.main_width_max = MAX_WIDTH_IN_WARP,
		.main_height_max = MAX_HEIGHT_IN_WARP,
		.capture_pps_max = MAX_CAP_PPS_IN_WARP,
		.rotate_possible = 0,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 0,
		.dptz_II_possible = 0,
		.vin_cap_offset_possible = 1,
		.hwarp_bypass_possible = 1,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 1,
		.mctf_pm_possible = 0,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 0,
		.mixer_b_possible = 0,		 /* Mixer B is already occupied by warp engine. */
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_MAX_DEWARP_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_4MP30,
		.min_encode_width = MIN_WIDTH_WITHOUT_ROTATE,
		.min_encode_height = MIN_HEIGHT_IN_WARP,
		.max_eis_delay_count = MAX_EIS_DELAY_COUNT,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_HIGH_MEGA_PIXEL_MODE] = {
		// High mega pixel mode
		.name = "High MP",
		.main_width_min = MIN_WIDTH_IN_HIGH_MP,
		.main_height_min = MIN_HEIGHT_IN_HIGH_MP,
		.main_width_max = MAX_WIDTH_IN_HIGH_MP,
		.main_height_max = MAX_HEIGHT_IN_HIGH_MP,
		.capture_pps_max = MAX_CAP_PPS_IN_HIGH_MP,
		.rotate_possible = 0,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 1,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 1,		/* Use Mixer B to do VCA duplication */
		.max_streams_num = IAV_MAX_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_64_SHIFT,
		.max_encode_MB = LOAD_4KP25_480P30,
		.min_encode_width = MIN_WIDTH_IN_HIGH_MP,
		.min_encode_height = MIN_HEIGHT_IN_HIGH_MP,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_CALIBRATION_MODE] = {
		// Calibration mode
		.name = "Calibration",
		.main_width_min = MIN_WIDTH_IN_CALIB,
		.main_height_min = MIN_HEIGHT_IN_CALIB,
		.main_width_max = MAX_WIDTH_IN_CALIB,
		.main_height_max = MAX_HEIGHT_IN_CALIB,
		.capture_pps_max = MAX_CAP_PPS_IN_CALIB,
		.rotate_possible = 1,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 0,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 0,
		.enc_from_yuv_possible = 0,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 0,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_CALIB,
		.min_encode_height = MIN_HEIGHT_IN_CALIB,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_HDR_FRAME_INTERLEAVED_MODE] = {
		// HDR frame interleaved mode
		.name = "HDR frame",
		.main_width_min = MIN_WIDTH_IN_HDR_FI,
		.main_height_min = MIN_HEIGHT_IN_HDR_FI,
		.main_width_max = MAX_WIDTH_IN_HDR_FI,
		.main_height_max = MAX_HEIGHT_IN_HDR_FI,
		.capture_pps_max = MAX_CAP_PPS_IN_HDR_FI,
		.rotate_possible = 1,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 0,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 0,
		.enc_from_yuv_possible = 0,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 0,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_HDR_FI,
		.min_encode_height = MIN_HEIGHT_IN_HDR_FI,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		// HDR line interleaved mode
		.name = "HDR line",
		.main_width_min = MIN_WIDTH_IN_HDR_LI,
		.main_height_min = MIN_HEIGHT_IN_HDR_LI,
		.main_width_max = MAX_WIDTH_IN_HDR_LI,
		.main_height_max = MAX_HEIGHT_IN_HDR_LI,
		.capture_pps_max = MAX_CAP_PPS_IN_HDR_LI,
		.rotate_possible = 1,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 0,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 1,
		.vout_swap_possible = 1,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 0,
		.video_freeze_possible = 1,
		.mixer_b_possible = 0,
		.vca_buffer_possible = 0,		/* Mixer B is already occupied by HDR engine. */
		.max_streams_num = IAV_MAX_HDR_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_HDR_LI,
		.min_encode_height = MIN_HEIGHT_IN_HDR_LI,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 1,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_HIGH_MP_FULL_PERF_MODE] = {
		// High mega pixel full performance mode
		.name = "High MP (FP)",
		.main_width_min = MIN_WIDTH_IN_HIGH_MP_FP,
		.main_height_min = MIN_HEIGHT_IN_HIGH_MP_FP,
		.main_width_max = MAX_WIDTH_IN_HIGH_MP_FP,
		.main_height_max = MAX_HEIGHT_IN_HIGH_MP_FP,
		.capture_pps_max = MAX_CAP_PPS_IN_HIGH_MP_FP,
		.rotate_possible = 0,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 0,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 1,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 0,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 1,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 1,
		.max_streams_num = IAV_MAX_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_32_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_HIGH_MP_FP,
		.min_encode_height = MIN_HEIGHT_IN_HIGH_MP_FP,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 1,
	},
	[DSP_FULL_FPS_FULL_PERF_MODE] = {
		// Full frame rate & full performance mode
		.name = "Full FPS (FP)",
		.main_width_min = MIN_WIDTH_IN_FULL_FPS_FP,
		.main_height_min = MIN_HEIGHT_IN_FULL_FPS_FP,
		.main_width_max = MAX_WIDTH_IN_FULL_FPS_FP,
		.main_height_max = MAX_HEIGHT_IN_FULL_FPS_FP,
		.capture_pps_max = MAX_CAP_PPS_IN_FULL_FPS_FP,
		.rotate_possible = 1,
		.raw_cap_possible = 0,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 1,
		.dptz_II_possible = 1,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 1,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_32_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_FULL_FPS_FP,
		.min_encode_height = MIN_WIDTH_IN_FULL_FPS_FP,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
	[DSP_MULTI_VIN_MODE] = {
		// Multiple CFA VIN mode
		.name = "Multi VIN",
		.main_width_min = MIN_WIDTH_IN_MULTI_CFA_VIN,
		.main_height_min = MIN_HEIGHT_IN_MULTI_CFA_VIN,
		.main_width_max = MAX_WIDTH_IN_MULTI_CFA_VIN,
		.main_height_max = MAX_HEIGHT_IN_MULTI_CFA_VIN * MAX_CFA_VIN_NUM,
		.capture_pps_max = MAX_CAP_PPS_IN_MULTI_CFA_VIN,
		.rotate_possible = 0,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 0,
		.dptz_I_possible = 0,
		.dptz_II_possible = 0,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 0,
		.enc_from_yuv_possible = 0,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 0,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_MAX_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_64_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_MULTI_CFA_VIN,
		.min_encode_height = MIN_HEIGHT_IN_MULTI_CFA_VIN,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},

	[DSP_HISO_VIDEO_MODE] = {
		// HISO Video mode
		.name = "HISO Video",
		.main_width_min = MIN_WIDTH_IN_HISO,
		.main_height_min = MIN_HEIGHT_IN_HISO,
		.main_width_max = MAX_WIDTH_IN_HISO,
		.main_height_max = MAX_HEIGHT_IN_HISO,
		.capture_pps_max = MAX_CAP_PPS_IN_HISO,
		.rotate_possible = 1,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 0,
		.sharpen_b_possible = 1,
		.dptz_I_possible = 0,
		.dptz_II_possible = 0,
		.vin_cap_offset_possible = 0,
		.hwarp_bypass_possible = 0,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 1,
		.vout_swap_possible = 0,
		.mctf_pm_possible = 1,
		.hdr_pm_possible = 0,
		.video_freeze_possible = 0,
		.mixer_b_possible = 1,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P120,
		.min_encode_width = MIN_WIDTH_IN_FULL_FPS,
		.min_encode_height = MIN_HEIGHT_IN_FULL_FPS,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},

	[DSP_HIGH_MP_WARP_MODE] = {
		// Multi region warping mode
		.name = "High MP Warping",
		.main_width_min = MIN_WIDTH_IN_HIGH_MP_WARP,
		.main_height_min = MIN_HEIGHT_IN_HIGH_MP_WARP,
		.main_width_max = MAX_WIDTH_IN_HIGH_MP_WARP,
		.main_height_max = MAX_HEIGHT_IN_HIGH_MP_WARP,
		.capture_pps_max = MAX_CAP_PPS_IN_HIGH_MP_WARP,
		.rotate_possible = 0,
		.raw_cap_possible = 1,
		.raw_stat_cap_possible = 1,
		.sharpen_b_possible = 1,
		.hwarp_bypass_possible = 1,
		.svc_t_possible = 1,
		.enc_from_yuv_possible = 1,
		.enc_from_raw_possible = 0,
		.vout_swap_possible = 1,
		.mctf_pm_possible = 0,
		.hdr_pm_possible = 1,
		.video_freeze_possible = 0,
		.mixer_b_possible = 0,
		.vca_buffer_possible = 0,
		.max_streams_num = IAV_MAX_DEWARP_STREAMS_NUM,
		.max_chroma_noise = CHROMA_NOISE_128_SHIFT,
		.max_encode_MB = LOAD_1080P61_480P30,
		.min_encode_width = MIN_WIDTH_WITHOUT_ROTATE,
		.min_encode_height = MIN_HEIGHT_IN_HIGH_MP_WARP,
		.max_eis_delay_count = 0,
		.vout_b_letter_box_possible = 0,
		.yuv_input_enhanced_possible = 0,
	},
};

iav_vcap_obj_ex_t G_iav_vcap =
{
	.cmd_read_delay	= MIN_CMD_READ_DELAY_IN_CLK,
	.eis_update_addr	= 0,
	.freeze_enable	= 0,
	.freeze_enable_saved	= 0,
	.mixer_b_enable	= 1,
	.vca_src_id		= 0,
	.debug_chip_id	= IAV_CHIP_ID_S2_UNKNOWN,
	.vout_b_letter_boxing_enable = 0,
	.yuv_input_enhanced = 0,
	.sharpen_b_enable = {
		[DSP_FULL_FRAMERATE_MODE] = 1,
		[DSP_MULTI_REGION_WARP_MODE] = 1,
		[DSP_HIGH_MEGA_PIXEL_MODE] = 1,
		[DSP_CALIBRATION_MODE] = 0,
		[DSP_HDR_FRAME_INTERLEAVED_MODE] = 1,
		[DSP_HDR_LINE_INTERLEAVED_MODE] = 1,
		[DSP_HIGH_MP_FULL_PERF_MODE] = 0,
		[DSP_FULL_FPS_FULL_PERF_MODE] = 0,
		[DSP_MULTI_VIN_MODE] = 0,
		[DSP_HISO_VIDEO_MODE] = 1,
		[DSP_HIGH_MP_WARP_MODE] = 1,
	},
	.enc_from_raw_enable = {
		[DSP_FULL_FRAMERATE_MODE] = 0,
		[DSP_MULTI_REGION_WARP_MODE] = 0,
		[DSP_HIGH_MEGA_PIXEL_MODE] = 0,
		[DSP_CALIBRATION_MODE] = 0,
		[DSP_HDR_FRAME_INTERLEAVED_MODE] = 0,
		[DSP_HDR_LINE_INTERLEAVED_MODE] = 0,
		[DSP_HIGH_MP_FULL_PERF_MODE] = 0,
		[DSP_FULL_FPS_FULL_PERF_MODE] = 0,
		[DSP_MULTI_VIN_MODE] = 0,
		[DSP_HISO_VIDEO_MODE] = 0,
		[DSP_HIGH_MP_WARP_MODE] = 0,
	},
	.vskip_before_encode = {
		[DSP_FULL_FRAMERATE_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX,
		[DSP_MULTI_REGION_WARP_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE,
		[DSP_HIGH_MEGA_PIXEL_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX,
		[DSP_CALIBRATION_MODE] = 0,
		[DSP_HDR_FRAME_INTERLEAVED_MODE] = 0,
		[DSP_HDR_LINE_INTERLEAVED_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX,
		[DSP_HIGH_MP_FULL_PERF_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX,
		[DSP_FULL_FPS_FULL_PERF_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN,
		[DSP_MULTI_VIN_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN,
		[DSP_HISO_VIDEO_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN,
		[DSP_HIGH_MP_WARP_MODE] = IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX,
	},
};

static iav_dsp_partition_t G_dsp_partition = {
	.id_map = 0,
	.id_map_user = 0,
	.map_flag = 0,
};

static iav_source_buffer_ex_t G_source_buffer[IAV_MAX_SOURCE_BUFFER_NUM] =
{
	[IAV_ENCODE_SOURCE_MAIN_BUFFER] = {	// Main source buffer
		.id = IAV_ENCODE_SOURCE_MAIN_BUFFER,
		.type = IAV_SOURCE_BUFFER_TYPE_ENCODE,
		.state = IAV_SOURCE_BUFFER_STATE_IDLE,
		.size = {
			.width = 1920,
			.height = 1080,
		},
		.input = {
			.width = 0,
			.height = 0,
			.x = 0,
			.y = 0,
		},
		.property = {
			.max = {
				.width = MAX_WIDTH_FOR_1ST,
				.height = MAX_HEIGHT_FOR_1ST * MAX_CFA_VIN_NUM,
			},
			.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_1ST,
			.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_1ST,
		},
		.unwarp = 0,
		.ref_count = 0,
	},
	[IAV_ENCODE_SOURCE_SECOND_BUFFER] = {	// Second source buffer (preview C)
		.id = IAV_ENCODE_SOURCE_SECOND_BUFFER,
		.type = IAV_SOURCE_BUFFER_TYPE_ENCODE,
		.state = IAV_SOURCE_BUFFER_STATE_IDLE,
		.size = {
			.width = 720,
			.height = 480,
		},
		.input = {
			.width = 0,
			.height = 0,
			.x = 0,
			.y = 0,
		},
		.property = {
			.max = {
				.width = MAX_WIDTH_FOR_2ND,
				.height = MAX_HEIGHT_FOR_2ND * MAX_CFA_VIN_NUM,
			},
			.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_2ND,
			.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_2ND,
		},
		.unwarp = 0,
		.ref_count = 0,
	},
	[IAV_ENCODE_SOURCE_THIRD_BUFFER] = {	// Third source buffer (preview B)
		.id = IAV_ENCODE_SOURCE_THIRD_BUFFER,
		.type = IAV_SOURCE_BUFFER_TYPE_PREVIEW,
		.state = IAV_SOURCE_BUFFER_STATE_IDLE,
		.size = {
			.width = 720,
			.height = 480,
		},
		.input = {
			.width = 0,
			.height = 0,
			.x = 0,
			.y = 0,
		},
		.property = {
			.max = {
				.width = MAX_WIDTH_FOR_3RD,
				.height = MAX_HEIGHT_FOR_3RD * MAX_CFA_VIN_NUM,
			},
			.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_3RD,
			.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_3RD,
		},
		.unwarp = 0,
		.preview_framerate_division_factor = 1,
		.ref_count = 0,
	},
	[IAV_ENCODE_SOURCE_FOURTH_BUFFER] = {	// Fourth source buffer (preview A)
		.id = IAV_ENCODE_SOURCE_FOURTH_BUFFER,
		.type = IAV_SOURCE_BUFFER_TYPE_OFF,
		.state = IAV_SOURCE_BUFFER_STATE_IDLE,
		.size = {
			.width = 720,
			.height = 480,
		},
		.input = {
			.width = 0,
			.height = 0,
			.x = 0,
			.y = 0,
		},
		.property = {
			.max = {
				.width = MAX_WIDTH_FOR_4TH,
				.height = MAX_HEIGHT_FOR_4TH * MAX_CFA_VIN_NUM,
			},
			.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_4TH,
			.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_4TH,
		},
		.unwarp = 0,
		.preview_framerate_division_factor = 1,
		.ref_count = 0,
	},
	[IAV_ENCODE_SOURCE_MAIN_DRAM] = {	// Main DRAM source
		.id = IAV_ENCODE_SOURCE_MAIN_DRAM,
		.type = IAV_SOURCE_BUFFER_TYPE_ENCODE,
		.state = IAV_SOURCE_BUFFER_STATE_IDLE,
		.size = {
			.width = MIN_WIDTH_WITHOUT_ROTATE,
			.height = MIN_HEIGHT_IN_STREAM,
		},
		.input = {
			.width = 0,
			.height = 0,
			.x = 0,
			.y = 0,
		},
		.property = {
			.max = {
				.width = MAX_WIDTH_FOR_1ST_DRAM,
				.height = MAX_HEIGHT_FOR_1ST_DRAM * MAX_CFA_VIN_NUM,
			},
			.max_zoom_in_factor = MAX_ZOOM_IN_FACTOR_FOR_1ST_DRAM,
			.max_zoom_out_factor = MAX_ZOOM_OUT_FACTOR_FOR_1ST_DRAM,
		},
		.dram = {
			.max_frame_num = 0,
			.buf_state = DSP_DRAM_BUFFER_POOL_INIT,
			.buf_pitch = 0,
			.buf_max_size = {
				.width = MIN_WIDTH_WITHOUT_ROTATE,
				.height = MIN_HEIGHT_IN_STREAM,
			},
		},
		.unwarp = 0,
		.preview_framerate_division_factor = 1,
		.ref_count = 0,
	}
};

static iav_system_setup_info_ex_t G_system_setup_info[DSP_ENCODE_MODE_TOTAL_NUM] =
{
	[DSP_FULL_FRAMERATE_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 1,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 0,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_HIGH_MEGA_PIXEL_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_CALIBRATION_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_HDR_FRAME_INTERLEAVED_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_HIGH_MP_FULL_PERF_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 0,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_FULL_FPS_FULL_PERF_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
	[DSP_MULTI_VIN_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 0,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},

	[DSP_HISO_VIDEO_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 0,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 1,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},

	[DSP_HIGH_MP_WARP_MODE] = {
		.voutA_osd_blend_enable = 1,
		.voutB_osd_blend_enable = 0,
		.coded_bits_interrupt_enable = 1,
		.vout_swap = 1,
		.cmd_read_delay = MIN_CMD_READ_DELAY_IN_MS,
		.mctf_privacy_mask = 0,
		.eis_delay_count = 0,
		.debug_enc_dummy_latency_count = 0,
	},
};

struct iav_obj G_iav_obj =
{
	.op_mode		= DSP_OP_MODE_IDLE,

	.dsp_encode_mode	= -1,
	.dsp_encode_state	= -1,
	.dsp_encode_state2	= -1,
	.dsp_vcap_mode		= -1,

	.dsp_decode_state	= -1,
	.decode_state		= -1,
};

static CAPTURE_BUFFER_ID G_capture_source[PREVIEW_BUFFER_TOTAL_NUM] =
{
	MAIN_BUFFER_ID, PREVIEW_C_BUFFER_ID - 1,
	PREVIEW_B_BUFFER_ID - 1, PREVIEW_A_BUFFER_ID - 1,
};
static u8 * G_max_enc_cfg_addr = NULL;


/******************************************
 *
 *	External helper functions
 *
 ******************************************/

inline unsigned long __iav_spin_lock(void)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&G_iav_obj.lock, flags);
	return flags;
}

inline void iav_irq_restore(unsigned long flags)
{
	spin_unlock_irqrestore(&G_iav_obj.lock, flags);
}

inline void iav_init_compl(iav_completion_t * compl_info)
{
	compl_info->waiters = 0;
	init_completion(&compl_info->compl);
}

static inline void iav_compl_inc(iav_completion_t * compl)
{
	unsigned long flags;
	iav_irq_save(flags);
	++compl->waiters;
	iav_irq_restore(flags);
}

static inline void iav_compl_dec(iav_completion_t * compl)
{
	unsigned long flags;
	iav_irq_save(flags);
	--compl->waiters;
	iav_irq_restore(flags);
}

inline int iav_wait_compl_interruptible(iav_completion_t * compl_info)
{
	int retv;
	iav_compl_inc(compl_info);
	iav_unlock();
	retv = wait_for_completion_interruptible(&compl_info->compl);
	iav_lock();
	if (retv < 0) {
		iav_compl_dec(compl_info);
	}
	return retv;
}

inline void notify_waiters(iav_completion_t * compl_info)
{
	if (compl_info->waiters > 0) {
		iav_compl_dec(compl_info);
		complete(&compl_info->compl);
	}
}

inline void notify_all_waiters(iav_completion_t * compl_info)
{
	while (compl_info->waiters > 0) {
		iav_compl_dec(compl_info);
		complete(&compl_info->compl);
	}
}

inline void set_iav_state(IAV_STATE state)
{
	G_iav_info.state = state;
}

inline int get_iav_state(void)
{
	return G_iav_info.state;
}

/* G_iav_info.state will be updated in VDSP ISR.
 * Need to call spin lock to protect the data. */
inline int is_iav_state_init(void)
{
	int retv = 0;

	spin_lock_irq(&G_iav_obj.lock);
	retv = (G_iav_info.state == IAV_STATE_INIT);
	spin_unlock_irq(&G_iav_obj.lock);

	return retv;
}

inline int is_iav_state_idle(void)
{
	int retv = 0;

	spin_lock_irq(&G_iav_obj.lock);
	retv = (G_iav_info.state == IAV_STATE_IDLE);
	spin_unlock_irq(&G_iav_obj.lock);

	return retv;
}

inline int is_iav_state_preview(void)
{
	int retv = 0;

	spin_lock_irq(&G_iav_obj.lock);
	retv = (G_iav_info.state == IAV_STATE_PREVIEW);
	spin_unlock_irq(&G_iav_obj.lock);

	return retv;
}

inline int is_iav_state_encoding(void)
{
	int retv = 0;

	spin_lock_irq(&G_iav_obj.lock);
	retv = (G_iav_info.state == IAV_STATE_ENCODING);
	spin_unlock_irq(&G_iav_obj.lock);

	return retv;
}

inline int is_iav_state_prev_or_enc(void)
{
	int retv = 0;

	spin_lock_irq(&G_iav_obj.lock);
	retv = ((G_iav_info.state == IAV_STATE_PREVIEW) ||
		(G_iav_info.state == IAV_STATE_ENCODING));
	spin_unlock_irq(&G_iav_obj.lock);

	return retv;
}

inline int is_valid_buffer_id(int buffer)
{
	return ((buffer >= IAV_ENCODE_SOURCE_BUFFER_FIRST) &&
		(buffer < IAV_ENCODE_SOURCE_BUFFER_LAST));
}

inline int is_valid_dram_buffer_id(int buffer)
{
	return ((buffer >= IAV_ENCODE_SOURCE_DRAM_FIRST) &&
		(buffer < IAV_ENCODE_SOURCE_DRAM_LAST));
}

inline int is_valid_vca_buffer_id(int buffer)
{
	return ((buffer >= IAV_ENCODE_SOURCE_VCA_FIRST) &&
		(buffer < IAV_ENCODE_SOURCE_VCA_LAST));
}

inline int is_invalid_dsp_addr(u32 addr)
{
	return ((addr == 0x0) || (addr == 0xdeadbeef) || (addr == 0xc0000000));
}

inline int is_buf_type_off(int buffer)
{
	return (G_source_buffer[buffer].type == IAV_SOURCE_BUFFER_TYPE_OFF);
}

inline int is_buf_type_enc(int buffer)
{
	return (G_source_buffer[buffer].type == IAV_SOURCE_BUFFER_TYPE_ENCODE);
}

inline int is_buf_type_prev(int buffer)
{
	return (G_source_buffer[buffer].type == IAV_SOURCE_BUFFER_TYPE_PREVIEW);
}

inline int is_buf_unwarped(int buffer)
{
	return (G_source_buffer[buffer].unwarp != 0);
}

inline int is_sub_buf(int buffer)
{
	return ((buffer >= IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST) &&
			(buffer < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST));
}

static inline int __is_iav_enc_mode(DSP_ENCODE_MODE mode)
{
	return (G_dsp_enc_mode == mode);
}

static inline int is_valid_enc_mode(DSP_ENCODE_MODE mode)
{
	return ((mode >= DSP_ENCODE_MODE_FIRST) && (mode < DSP_ENCODE_MODE_LAST));
}

static inline int is_multi_buffer_id(iav_buffer_id_t buffer_id)
{
	return ((buffer_id & (buffer_id - 1)) > 0);
}

inline int get_enc_mode(void)
{
	return G_dsp_enc_mode;
}

inline int is_full_framerate_mode(void)
{
	return __is_iav_enc_mode(DSP_FULL_FRAMERATE_MODE);
}

inline int is_multi_region_warp_mode(void)
{
	return __is_iav_enc_mode(DSP_MULTI_REGION_WARP_MODE);
}

inline int is_high_mega_pixel_mode(void)
{
	return __is_iav_enc_mode(DSP_HIGH_MEGA_PIXEL_MODE);
}

inline int is_calibration_mode(void)
{
	return __is_iav_enc_mode(DSP_CALIBRATION_MODE);
}

inline int is_hdr_frame_interleaved_mode(void)
{
	return __is_iav_enc_mode(DSP_HDR_FRAME_INTERLEAVED_MODE);
}

inline int is_hdr_line_interleaved_mode(void)
{
	return __is_iav_enc_mode(DSP_HDR_LINE_INTERLEAVED_MODE);
}

inline int is_high_mp_full_perf_mode(void)
{
	return __is_iav_enc_mode(DSP_HIGH_MP_FULL_PERF_MODE);
}

inline int is_full_fps_full_perf_mode(void)
{
	return __is_iav_enc_mode(DSP_FULL_FPS_FULL_PERF_MODE);
}

inline int is_multi_vin_mode(void)
{
	return __is_iav_enc_mode(DSP_MULTI_VIN_MODE);
}


inline int is_hiso_video_mode(void)
{
	return __is_iav_enc_mode(DSP_HISO_VIDEO_MODE);
}

inline int is_high_mp_warp_mode(void)
{
	return __is_iav_enc_mode(DSP_HIGH_MP_WARP_MODE);
}

inline int is_hdr_mode(void)
{
	return ((is_hdr_frame_interleaved_mode()) ||
			(is_hdr_line_interleaved_mode()));
}

inline int is_full_performance_mode(void)
{
	return ((is_high_mp_full_perf_mode()) || (is_full_fps_full_perf_mode()));
}

inline int is_high_mp_mode(void)
{
	return ((is_high_mega_pixel_mode()) || (is_high_mp_full_perf_mode()) ||
		(is_high_mp_warp_mode()));
}

inline int is_warp_mode(void)
{
	return ((is_multi_region_warp_mode()) || (is_high_mp_warp_mode()));
}

inline int is_sharpen_b_enabled(void)
{
	return G_iav_vcap.sharpen_b_enable[G_dsp_enc_mode];
}

inline int is_rotate_possible_enabled(void)
{
	return G_system_resource_setup[G_dsp_enc_mode].enc_rotation;
}

inline int is_polling_readout(void)
{
	return (G_system_setup_info[G_dsp_enc_mode].coded_bits_interrupt_enable == 0);
}

inline int is_interrupt_readout(void)
{
	return (G_system_setup_info[G_dsp_enc_mode].coded_bits_interrupt_enable == 1);
}

inline int is_raw_capture_enabled(void)
{
	return G_system_resource_setup[G_dsp_enc_mode].raw_compression_disabled;
}

inline int is_raw_stat_capture_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].raw_stat_cap_possible;
}

inline int is_dptz_I_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].dptz_I_possible;
}

inline int is_dptz_II_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].dptz_II_possible;
}

inline int is_vin_cap_offset_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].vin_cap_offset_possible;
}

inline int is_mctf_pm_enabled(void)
{
	return G_system_setup_info[G_dsp_enc_mode].mctf_privacy_mask;
}

inline int is_hwarp_bypass_enabled(void)
{
	return G_system_resource_setup[G_dsp_enc_mode].h_warp_bypass;
}

inline int is_svc_t_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].svc_t_possible;
}

inline int is_enc_from_dram_enabled(void)
{
	return G_modes_limit[G_dsp_enc_mode].enc_from_yuv_possible;
}

inline int is_enc_from_raw_enabled(void)
{
	return G_iav_vcap.enc_from_raw_enable[G_dsp_enc_mode];
}

inline int is_video_freeze_enabled(void)
{
	return G_iav_vcap.freeze_enable;
}

inline int is_vout_b_letter_boxing_enabled(void)
{
	return G_iav_vcap.vout_b_letter_boxing_enable &&
		(!(G_system_setup_info[G_dsp_enc_mode].vout_swap ||
		G_iav_vcap.mixer_b_enable));
}

inline int is_map_dsp_partition(void)
{
	return G_dsp_partition.map_flag;
}

u8 * dsp_dsp_to_user(iav_context_t * context, u32 addr)
{
	struct iav_mem_block * dsp;

	iav_get_mem_block(IAV_MMAP_DSP, &dsp);
	addr = DSP_TO_PHYS(addr);
	if (addr < dsp->phys_start || addr >= dsp->phys_end) {
		iav_error("bad address 0x%x, phys start 0x%x, end 0x%x.\n",
			addr, dsp->phys_start, dsp->phys_end);
		return NULL;
	}
	if (context->dsp.user_start == 0 || context->dsp.user_end == 0) {
		iav_error("DSP memory is not mapped!\n");
		return NULL;
	}
	return (addr - dsp->phys_start) + context->dsp.user_start;
}

inline u32 get_vin_num(int encode_mode)
{
	return (encode_mode == DSP_MULTI_VIN_MODE ?
		G_system_resource_setup[encode_mode].num_vin_minus_2 +
		MIN_CFA_VIN_NUM : 1);
}

inline u32 get_expo_num(int encode_mode)
{
	return (encode_mode == DSP_HDR_FRAME_INTERLEAVED_MODE ||
			encode_mode == DSP_HDR_LINE_INTERLEAVED_MODE) ?
		G_system_resource_setup[encode_mode].hdr_num_exposures_minus_1 +
		MIN_HDR_EXPOSURES : 1;
}

inline int get_preview_size(u16 buf_enc_w, u16 buf_enc_h,
	iav_source_buffer_type_ex_t type, struct amba_video_info * video_info,
	u16 * width, u16 * height)
{
	u16 buf_w, buf_h;
	switch (type) {
		case IAV_SOURCE_BUFFER_TYPE_OFF:
			buf_w = buf_h = 0;
			break;
		case IAV_SOURCE_BUFFER_TYPE_ENCODE:
			calc_roundup_size(buf_enc_w, buf_enc_h, IAV_ENCODE_H264, &buf_w,
				&buf_h);
			break;
		case IAV_SOURCE_BUFFER_TYPE_PREVIEW:
			if (video_info->rotate) {
				buf_w = video_info->height;
				buf_h = video_info->width;
			} else {
				buf_w = video_info->width;
				buf_h = video_info->height;
			}
			break;
		default :
			iav_error("Invalid source buffer type [%d]!\n", type);
			return -1;
			break;
	}
	*width = buf_w;
	*height = buf_h;
	return 0;
}

inline int get_vin_window(iav_rect_ex_t* vin)
{
	struct amba_vin_src_capability *vin_info = get_vin_capability();

	if (is_hdr_line_interleaved_mode() &&
			vin_info->act_width && vin_info->act_height) {
		vin->width = vin_info->act_width;
		vin->height = vin_info->act_height;
		vin->x = vin_info->act_start_x;
		vin->y = vin_info->act_start_y;
	} else {
		vin->width = vin_info->cap_cap_w / get_vin_num(G_dsp_enc_mode);
		vin->height = vin_info->cap_cap_h;
		vin->x = vin_info->cap_start_x;
		vin->y = vin_info->cap_start_y;
	}
	return 0;
}

inline u32 get_max_enc_num(int encode_mode)
{
	return (G_system_resource_setup[encode_mode].max_num_enc_msb ?
		IPCAM_RECORD_MAX_NUM_ENC_ALL :
		G_system_resource_setup[encode_mode].max_num_enc);
}

inline DSP_ENC_CFG * get_enc_cfg(int encode_mode, int stream)
{
	DSP_ENC_CFG * enc_cfg = NULL;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT * ext = NULL;
	if (likely(stream < IPCAM_RECORD_MAX_NUM_ENC)) {
		enc_cfg = &G_system_resource_setup[encode_mode].enc_cfg[stream];
	} else {
		ext = (DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT *)G_max_enc_cfg_addr;
		enc_cfg = &ext[encode_mode].enc_cfg[stream - IPCAM_RECORD_MAX_NUM_ENC];
	}
	return enc_cfg;
}

static inline int calc_dsp_vin_num(int encode_mode, int vin_num)
{
	return (encode_mode == DSP_MULTI_VIN_MODE ? vin_num - MIN_CFA_VIN_NUM : 0);
}

static inline int calc_dsp_expo_num(int encode_mode, int expo_num)
{
	return (encode_mode == DSP_HDR_FRAME_INTERLEAVED_MODE ||
			encode_mode == DSP_HDR_LINE_INTERLEAVED_MODE) ?
		expo_num - MIN_HDR_EXPOSURES : 0;
}

inline int check_zoom_property(int buffer_id,
	u16 in_w, u16 in_h, u16 out_w, u16 out_h, char * str)
{
	u16 zm_in = G_source_buffer[buffer_id].property.max_zoom_in_factor;
	u16 zm_out = G_source_buffer[buffer_id].property.max_zoom_out_factor;

	if (((zm_out * out_w) <= in_w) || ((zm_out * out_h) <= in_h)) {
		iav_error("%s downscaled from %dx%d to %dx%d, cannot be larger"
			" or equal to the max zoom out factor [%d].\n", str, in_w,
			in_h, out_w, out_h, zm_out);
		return -1;
	}
	if (((zm_in * in_w) < out_w) || ((zm_in * in_h) < out_h)) {
		iav_error("%s upscaled from %dx%d to %dx%d, out of the max zoom in"
			" factor [%d].\n", str, in_w, in_h, out_w, out_h, zm_in);
		return -1;
	}

	return 0;
}

inline int check_full_fps_full_perf_cap(char * str)
{
	u32 vin_fps;
	u16 m_w, m_h;
	struct amba_vin_src_capability *vin = get_vin_capability();

	if (DSP_FULL_FPS_FULL_PERF_MODE == G_dsp_enc_mode) {
		m_w = G_source_buffer[0].size.width;
		m_h = G_source_buffer[0].size.height;
		vin_fps = DIV_ROUND(512000000, vin->frame_rate);
		if ((m_w >= 1920 && m_h >= 1080) &&
			(vin_fps >= MAX_VIN_FPS_FOR_FULL_FEATURE)) {
			iav_error("Cannot enable [%s] when FPS higher than [%d] in"
				" [low delay] mode.\n", str, MAX_VIN_FPS_FOR_FULL_FEATURE);
			return -1;
		}
	}
	return 0;
}

int mem_alloc_enc_cfg(u8 * * ptr, int * alloc_size)
{
	int i, total_size, index;
	u8 * addr = NULL;
	DSP_ENC_CFG * cfg = NULL;

	total_size = DSP_ENC_CFG_EXT_SIZE * DSP_ENCODE_MODE_TOTAL_NUM;
	if ((addr = kzalloc(total_size, GFP_KERNEL)) == NULL) {
		iav_error("Not enough memory to allocate ENC config!\n");
		return -1;
	}
	*ptr = addr;
	*alloc_size = total_size;
	G_max_enc_cfg_addr = addr;

	for (i = 0; i < DSP_ENCODE_MODE_TOTAL_NUM;
		++i, addr += DSP_ENC_CFG_EXT_SIZE) {
		G_system_resource_setup[i].set_op_mode_ext_daddr = (u32)addr;
		cfg = (DSP_ENC_CFG *)addr;
		for (index = 0; index < IPCAM_RECORD_MAX_NUM_ENC_EXT; ++index) {
			cfg[index].max_enc_width = 352;
			cfg[index].max_enc_height = 288;
			cfg[index].max_GOP_M = 1;
			cfg[index].vert_search_range_2x = 0;
		}
	}

	return 0;
}

