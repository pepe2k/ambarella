/*
 * iav_capture.h
 *
 * History:
 *	2012/06/20 - [Jian Tang] modified this file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_CAPTURE_H__
#define __IAV_CAPTURE_H__


typedef enum {
	DSP_FULL_FRAMERATE_MODE = 0,
	DSP_MULTI_REGION_WARP_MODE = 1,
	DSP_HIGH_MEGA_PIXEL_MODE = 2,
	DSP_CALIBRATION_MODE = 3,
	DSP_HDR_FRAME_INTERLEAVED_MODE = 4,
	DSP_HDR_LINE_INTERLEAVED_MODE = 5,
	DSP_HIGH_MP_FULL_PERF_MODE = 6,
	DSP_FULL_FPS_FULL_PERF_MODE = 7,
	DSP_MULTI_VIN_MODE = 8,
	DSP_HISO_VIDEO_MODE = 9,
	DSP_HIGH_MP_WARP_MODE = 10,
	DSP_ENCODE_MODE_TOTAL_NUM,
	DSP_ENCODE_MODE_FIRST = DSP_FULL_FRAMERATE_MODE,
	DSP_ENCODE_MODE_LAST = DSP_ENCODE_MODE_TOTAL_NUM,
} DSP_ENCODE_MODE;


typedef enum {
	DSP_DRAM_BUFFER_POOL_INIT = 0,
	DSP_DRAM_BUFFER_POOL_CREATE_READY = 1,
	DSP_DRAM_BUFFER_POOL_READY = 2,
	DSP_DRAM_BUFFER_POOL_REQUEST = 3,
} DSP_ENC_DRAM_MODE;


typedef enum {
	UPDATE_PREVIEW_FRAME_RATE_FLAG = (1 << 0),
	UPDATE_CUSTOM_VIN_FPS_FLAG = (1 << 1),
	UPDATE_FREEZE_ENABLED_FLAG = (1 << 2),
} UPDATE_CAPTUER_PARAM_FLAG;


typedef enum {
	CHROMA_NOISE_BASE_SHIFT = 5,
	CHROMA_NOISE_32_SHIFT = 0,
	CHROMA_NOISE_64_SHIFT = 1,
	CHROMA_NOISE_128_SHIFT = 2,
	CHROMA_NOISE_SHIFT_TOTAL_NUM,
	CHROMA_NOISE_SHIFT_MIN = 0,
	CHROMA_NOISE_SHIFT_MAX = CHROMA_NOISE_128_SHIFT,
} IAV_CHROMA_NOISE_STRENGTH;


typedef enum {
	IAV_ENC_DRAM_BUFFER_YUV400 = 0,
	IAV_ENC_DRAM_BUFFER_YUV420 = 1,

	IAV_ENC_DRAM_BUFFER_TOTAL_NUM,
	IAV_ENC_DRAM_BUFFER_FIRST = 0,
	IAV_ENC_DRAM_BUFFER_LAST = IAV_ENC_DRAM_BUFFER_TOTAL_NUM,
} IAV_ENC_DRAM_CHROMA_FORMAT;


typedef struct iav_enc_mode_limit_s
{
#define	NAME_LENGTH	(16)
	char	name[NAME_LENGTH];
	u8	max_streams_num;
	u8	max_chroma_noise : 2;
	u8	max_eis_delay_count : 2;
	u8	reserved : 4;
	u16	main_width_min;
	u16	main_height_min;
	u16	main_width_max;
	u16	main_height_max;
	u32	capture_pps_max;	// maximum capture pixel per second
	u32	max_encode_MB;
	u16	min_encode_width;
	u16	min_encode_height;
	u32	sharpen_b_possible : 1;
	u32	rotate_possible : 1;
	u32	raw_cap_possible : 1;
	u32	raw_stat_cap_possible : 1;
	u32	dptz_I_possible : 1;
	u32	dptz_II_possible : 1;
	u32	vin_cap_offset_possible : 1;
	u32	hwarp_bypass_possible : 1;
	u32	svc_t_possible : 1;
	u32	enc_from_yuv_possible : 1;
	u32	enc_from_raw_possible : 1;
	u32	vout_swap_possible : 1;
	u32	mctf_pm_possible : 1;
	u32	hdr_pm_possible : 1;
	u32	video_freeze_possible : 1;
	u32	mixer_b_possible : 1;
	u32	vca_buffer_possible : 1;
	u32	vout_b_letter_box_possible : 1;
	u32	yuv_input_enhanced_possible : 1;
	u32	reserved_possible : 13;
} iav_enc_mode_limit_t;


typedef struct iav_vcap_obj_ex_s
{
	u32	cmd_read_delay;
	u32	eis_update_addr;
	u32	freeze_enable : 1;
	u32	freeze_enable_saved : 1;
	u32	mixer_b_enable : 1;
	u32	vca_src_id : 2;
	s32	debug_chip_id : 4;
	u32	vout_b_letter_boxing_enable : 1;
	u32	vout_b_mixer_enable : 1;	 // from vout setting for mixer_b_possible checking
	u32	yuv_input_enhanced : 1;
	u32	reserved : 20;
	/*
	 * SHARPEN_B filter.
	 * 0: disabled; 1: enabled luma sharpen, don't use other values
	 */
	u8	sharpen_b_enable[DSP_ENCODE_MODE_TOTAL_NUM];
	u8	enc_from_raw_enable[DSP_ENCODE_MODE_TOTAL_NUM];
	u8	vskip_before_encode[DSP_ENCODE_MODE_TOTAL_NUM];
} iav_vcap_obj_ex_t;

#define	DSP_ENC_CFG_EXT_SIZE	(sizeof(DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT))

extern iav_source_buffer_ex_t G_cap_pre_main;
extern DSP_SET_OP_MODE_IPCAM_RECORD_CMD G_system_resource_setup[DSP_ENCODE_MODE_TOTAL_NUM];
extern iav_enc_mode_limit_t G_modes_limit[DSP_ENCODE_MODE_TOTAL_NUM];
extern iav_vcap_obj_ex_t G_iav_vcap;

#endif	// __IAV_CAPTURE_H__

