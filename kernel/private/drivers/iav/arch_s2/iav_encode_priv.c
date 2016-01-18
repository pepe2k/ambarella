/*
 * iav_encode_priv.c
 *
 * History:
 *	2013/01/05 - [Jian Tang] created file
 * Copyright (C) 2013-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

iav_encode_obj_ex_t G_encode_obj =
{
	.total_bits_info_ctr_h264 = 0,
	.total_bits_info_ctr_mjpeg = 0,
	.total_bits_info_ctr_tjpeg = 0,
	.total_pic_encoded_h264_mode = 0,
	.total_pic_encoded_mjpeg_mode = 0,
	.h264_pic_counter = 0,
	.mjpeg_pic_counter = 0,
	.total_pic_counter = 0,
	.encoding_stream_num = 0,
};


iav_encode_stream_ex_t G_encode_stream[IAV_MAX_ENCODE_STREAMS_NUM];

static iav_bitrate_info_ex_t G_rate_control_info[IAV_MAX_ENCODE_STREAMS_NUM];
static iav_stream_encode_config_ex_t G_encode_config[IAV_MAX_ENCODE_STREAMS_NUM];

// stream session variables
static u32 G_stream_session_id[IAV_MAX_ENCODE_STREAMS_NUM][SESSION_ID_INDEX_SIZE];

static iav_system_load_t G_system_load[SYSTEM_LOAD_NUM] =
{
	[IAV_CHIP_ID_S2_33] = {
		.system_load = SYSTEM_LOAD_S2_33,
		.desc = "3MP30 + 480p30",
		.max_enc_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
	},
	[IAV_CHIP_ID_S2_55] = {
		.system_load = SYSTEM_LOAD_S2_55,
		.desc = "1080p60 + 480p60",
		.max_enc_num = IAV_MEDIUM_ENCODE_STREAMS_NUM,
	},
	[IAV_CHIP_ID_S2_66] = {
		.system_load = SYSTEM_LOAD_S2_66,
		.desc = "5MP30 + 480p30",
		.max_enc_num = IAV_MEDIUM_ENCODE_STREAMS_NUM,
	},
	[IAV_CHIP_ID_S2_88] = {
		.system_load = SYSTEM_LOAD_S2_88,
		.desc = "1080p120",
		.max_enc_num = IAV_MAX_ENCODE_STREAMS_NUM,
	},
	[IAV_CHIP_ID_S2_99] = {
		.system_load = SYSTEM_LOAD_S2_99,
		.desc = "1080p120",
		.max_enc_num = IAV_MAX_ENCODE_STREAMS_NUM,
	},
	[IAV_CHIP_ID_S2_22] = {
		.system_load = SYSTEM_LOAD_S2_22,
		.desc = "1080p31 + 480p30",
		.max_enc_num = IAV_DEFAULT_ENCODE_STREAMS_NUM,
	},
};

static CAPTURE_BUFFER_ID G_capture_source_id[PREVIEW_BUFFER_TOTAL_NUM] =
{
	MAIN_BUFFER_ID,			PREVIEW_C_BUFFER_ID,
	PREVIEW_B_BUFFER_ID,	PREVIEW_A_BUFFER_ID,
	DRAM_BUFFER_ID,
};

static u8 * G_qp_matrix_map = NULL;
static u8 * G_qp_matrix_current_daddr = NULL;

// rate control variables
static u32 rc_br_table[H264_RC_LUT_NUM_MAX] =
{
	256000,	 512000,	 768000,				// < 1 Mbps
	1000000,	1500000,	2000000,	 3000000,	// < 4 Mbps
	4000000,	6000000,	8000000,	10000000,	// < 12 Mbps
	16000000,
};

static u32 rc_reso_table[H264_RC_LUT_NUM_MAX] =
{
	3840*2160,	 1920*1080,	 1280*1024,	 1280*960,
	1280*720,	 1024*768,	 800*600,	 720*576,
	720*480,	 640*480,	 352*288,	 352*240,
};

static u32 rc_qp_for_vbr_lut[H264_RC_LUT_NUM_MAX][H264_RC_LUT_NUM_MAX] =
{
	//	4K 1080p SXGA 1280x960 720p XGA SVGA 576p 480p VGA CIF	SIF
	{	36,	31,	29,	27,	27,	26,	23,	23,	22,	22,	17,	16	},	// 256 kbps
	{	34,	30,	26,	25,	25,	24,	22,	21,	20,	20,	16,	15	},	// 512 kbps
	{	31,	28,	25,	24,	24,	23,	21,	20,	19,	19,	15,	14	},	// 768 kbps
	{	30,	27,	24,	23,	23,	22,	20,	19,	18,	18,	14,	13	},	// 1 Mbps
	{	29,	26,	24,	22,	22,	21,	19,	18,	17,	17,	12,	11	},	// 1.5 Mbps
	{	28,	25,	23,	22,	21,	19,	18,	17,	16,	16,	11,	10	},	// 2 Mbps
	{	27,	24,	22,	21,	20,	19,	17,	16,	15,	15,	9,	8	},	// 3 Mbps
	{	26,	23,	21,	20,	19,	18,	16,	15,	14,	14,	8,	7	},	// 4 Mbps
	{	25,	22,	20,	19,	18,	17,	15,	14,	13,	12,	5,	1	},	// 6 Mbps
	{	24,	21,	19,	18,	17,	16,	14,	13,	12,	11,	1,	1	},	// 8 Mbps
	{	24,	21,	18,	17,	16,	15,	13,	12,	11,	10,	1,	1	},	// 10 Mbps
	{	23,	20,	17,	16,	15,	14,	12,	11,	10,	9,	1,	1	},	// 16 Mbps
};

static u8 * G_quant_matrix_addr = NULL;
static u8 * G_q_matrix_addr = NULL;

static DEFINE_IAV_TIMER(G_bsb_polling_read);
static DEFINE_IAV_TIMER(G_statis_polling_read);


/******************************************
 *
 *	Internal helper functions
 *
 ******************************************/

static inline int __is_dsp_vcap_mode(VCAP_SETUP_MODE mode)
{
	return (G_iav_obj.dsp_vcap_mode == mode);
}

static inline void __set_dsp_vcap_mode(VCAP_SETUP_MODE mode)
{
	G_iav_obj.dsp_vcap_mode = mode;
}

static inline int __is_dsp_enc_state(int stream, encode_state_t stat)
{
	return (G_iav_obj.dsp_encode_state_ex[stream] == stat);
}

static inline void __set_dsp_enc_state(int stream, encode_state_t stat)
{
	G_iav_obj.dsp_encode_state_ex[stream] = stat;
}

static inline int __is_enc_op(int stream, iav_encode_op_ex_t op)
{
	return (G_encode_stream[stream].op == op);
}

static inline void __set_enc_op(int stream, iav_encode_op_ex_t op)
{
	G_encode_stream[stream].op = op;
}

static inline int __is_enc_type(int stream, u32 enc_type)
{
	return (G_encode_stream[stream].format.encode_type == enc_type);
}

static inline int __is_enc_h264(int stream)
{
	return __is_enc_type(stream, IAV_ENCODE_H264);
}

static inline int __is_enc_mjpeg(int stream)
{
	return __is_enc_type(stream, IAV_ENCODE_MJPEG);
}

static inline int __is_enc_none(int stream)
{
	return __is_enc_type(stream, IAV_ENCODE_NONE);
}

static inline int __is_invalid_stream(int stream)
{
	return ((stream < 0) || (stream >= IAV_MAX_ENCODE_STREAMS_NUM));
}

static inline int __is_invalid_stream_id(iav_stream_id_t stream_id)
{
	return ((stream_id & STREAM_ID_MASK) == 0);
}

static inline int __is_multi_stream_id(iav_stream_id_t  stream_id)
{
	return ((stream_id & (stream_id - 1)) > 0);
}

static inline int __is_stream_in_starting(int stream)
{
	return (__is_enc_op(stream, IAV_ENCODE_OP_START) &&
			__is_dsp_enc_state(stream, ENC_IDLE_STATE));
}

static inline int __is_stream_in_encoding(int stream)
{
	return (__is_enc_op(stream, IAV_ENCODE_OP_START) &&
			__is_dsp_enc_state(stream, ENC_BUSY_STATE));
}

static inline int __is_stream_in_stopping(int stream)
{
	return (__is_enc_op(stream, IAV_ENCODE_OP_STOP) &&
			__is_dsp_enc_state(stream, ENC_BUSY_STATE));
}

static inline int __is_stream_ready_for_encode(int stream)
{
	return (__is_enc_op(stream, IAV_ENCODE_OP_STOP) &&
			__is_dsp_enc_state(stream, ENC_IDLE_STATE));
}

static inline int __is_stream_in_transition(int stream)
{
	return (__is_stream_in_starting(stream) || __is_stream_in_stopping(stream));
}

static inline int __is_stream_in_h264_encoding(int stream)
{
	return (__is_enc_h264(stream) && __is_stream_in_encoding(stream));
}

static inline int __is_bsb_mapped(iav_context_t * context)
{
	return (context->bsb.user_start && context->bsb.user_end);
}

static inline int __is_mv_mapped(iav_context_t * context)
{
	return (context->mv.user_start && context->mv.user_end);
}

static inline int __is_qp_hist_mapped(iav_context_t * context)
{
	return (context->qp_hist.user_start && context->qp_hist.user_end);
}

static inline int __get_stream_encode_state(int stream)
{
	int stream_state = IAV_STREAM_STATE_UNKNOWN;
	if (__is_stream_in_encoding(stream)) {
		stream_state = IAV_STREAM_STATE_ENCODING;
	} else if (__is_stream_ready_for_encode(stream)) {
		stream_state = IAV_STREAM_STATE_READY_FOR_ENCODING;
	} else if (__is_stream_in_starting(stream)) {
		stream_state = IAV_STREAM_STATE_STARTING;
	} else if (__is_stream_in_stopping(stream)) {
		stream_state = IAV_STREAM_STATE_STOPPING;
	} else {
		stream_state = IAV_STREAM_STATE_ERROR;
	}
	return stream_state;
}

static inline void __update_source_buffer_state(int buffer_id)
{
	iav_source_buffer_ex_t * buffer = &G_iav_obj.source_buffer[buffer_id];
	if (buffer->ref_count == 0) {
		buffer->state = IAV_SOURCE_BUFFER_STATE_IDLE;
	} else {
		buffer->state = IAV_SOURCE_BUFFER_STATE_BUSY;
	}
}

static inline void __inc_source_buffer_ref(int buffer_id, int stream)
{
	G_iav_obj.source_buffer[buffer_id].ref_count |= (1 << stream);
	__update_source_buffer_state(buffer_id);
}

static inline void __rel_source_buffer_ref(int buffer_id, int stream)
{
	G_iav_obj.source_buffer[buffer_id].ref_count &= ~(1 << stream);
	__update_source_buffer_state(buffer_id);
}

static inline void __read_bits_desc(dsp_bits_info_t ** bits_info_ptr)
{
	dsp_bits_info_t *bits_info;

	bits_info = G_encode_obj.bits_desc_read_ptr;
	invalidate_d_cache((void*)bits_info, sizeof(dsp_bits_info_t));
	*bits_info_ptr = bits_info;
}

// return 1 if it's the last "end" frame
inline int is_end_frame(dsp_bits_info_t * bits_info)
{
	return ((bits_info->start_addr == ENCODE_INVALID_ADDRESS) &&
			(bits_info->length == ENCODE_INVALID_LENGTH) &&
			(bits_info->pts_64 == ENCODE_STOPPED_PTS));
}

// return 1 if encode format changed, else return 0
static inline int is_encode_format_changed(
		iav_encode_format_ex_t * encode_format_from,
		iav_encode_format_ex_t * encode_format_to)
{
	return ((encode_format_from->encode_type != encode_format_to ->encode_type) ||
			(encode_format_from->source != encode_format_to->source) ||
			(encode_format_from->encode_width != encode_format_to->encode_width) ||
			(encode_format_from->encode_height != encode_format_to->encode_height) ||
			(encode_format_from->encode_x != encode_format_to->encode_x) ||
			(encode_format_from->encode_y != encode_format_to->encode_y) ||
			(encode_format_from->hflip != encode_format_to->hflip) ||
			(encode_format_from->vflip != encode_format_to->vflip) ||
			(encode_format_from->rotate_clockwise != encode_format_to->rotate_clockwise) ||
			(encode_format_from->negative_offset_disable != encode_format_to->negative_offset_disable) ||
			(encode_format_from->duration != encode_format_to->duration));
}

//return 1 if it's only encoding offset change, else return 0
static inline int is_encode_offset_changed_only(
		iav_encode_format_ex_t * encode_format_from,
		iav_encode_format_ex_t * encode_format_to)
{
	return ((encode_format_from->encode_type == encode_format_to ->encode_type) &&
			(encode_format_from->source == encode_format_to->source) &&
			(encode_format_from->encode_width == encode_format_to->encode_width) &&
			(encode_format_from->encode_height == encode_format_to->encode_height) &&
			(encode_format_from->hflip == encode_format_to->hflip) &&
			(encode_format_from->vflip == encode_format_to->vflip) &&
			(encode_format_from->rotate_clockwise == encode_format_to->rotate_clockwise) &&
			(encode_format_from->negative_offset_disable == encode_format_to->negative_offset_disable));
}

static inline int is_video_encode_interlaced(void)
{
	return (get_vin_capability()->video_format == AMBA_VIDEO_FORMAT_INTERLACE);
}

inline int is_supported_stream(int stream)
{
	int max_num = get_max_enc_num(get_enc_mode());
	if (unlikely(stream >= max_num)) {
		iav_error("Invalid stream [%d], out of supported range"
			" [0~%d] in this encoding mode.\n", stream, max_num - 1);
		return 0;
	}
	return 1;
}

inline int get_single_stream_num(iav_stream_id_t stream_id, int * stream_num)
{
	int i;
	if (__is_multi_stream_id(stream_id)) {
		iav_error("No multi id [0x%x] support in this IOCTL!\n", stream_id);
		return -1;
	}
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i))
			break;
	}
	if (__is_invalid_stream(i)) {
		iav_error("No stream num found from id [0x%x]!\n", stream_id);
		return -1;
	}
	*stream_num = i;
	return 0;
}

inline int get_stream_type(int stream)
{
	return G_encode_stream[stream].stream_type;
}

static inline int get_stream_num_from_type(int stream_type)
{
	int stream;

	if (likely((stream_type >= STRM_TP_ENC_FIRST) &&
			(stream_type < STRM_TP_ENC_LAST))) {
		stream = stream_type - STRM_TP_ENC_FIRST;
	} else {
		iav_error("Invalid stream type [%x].\n", stream_type);
		stream = -1;
	}

	return stream;
}

static inline CAPTURE_BUFFER_ID get_capture_buffer_id(u8 source_buffer)
{
	return G_capture_source_id[source_buffer];
}

static inline int is_invalid_bits_desc(dsp_bits_info_t * info)
{
	return ((info->pts_64 == ENCODE_INVALID_PTS) || (info->length == 0));
}

static inline int is_invalid_frame_index(int frame_index)
{
	return ((frame_index < 0) || (frame_index >= NUM_BS_DESC));
}

static inline int is_invalid_statis_desc(dsp_enc_stat_info_t * info)
{
	return ((info->pts_64 == ENCODE_INVALID_PTS) ||
		((info->mv_start_addr == ENCODE_INVALID_ADDRESS || !info->mv_start_addr) &&
		(info->qp_hist_daddr == ENCODE_INVALID_ADDRESS || !info->qp_hist_daddr)));
}

static inline int is_invalid_statis_index(int statis_index)
{
	return ((statis_index < 0) || (statis_index >= NUM_STATIS_DESC));
}

static inline void set_invalid_bits_desc(dsp_bits_info_t * info)
{
	info->length = 0;
	info->pts_64 = ENCODE_INVALID_PTS;
	info->start_addr = ENCODE_INVALID_ADDRESS;
	clean_cache_aligned((void *)info, sizeof(dsp_bits_info_t));
}

static inline void set_invalid_statis_desc(dsp_enc_stat_info_t * info)
{
	info->frmNo = -1;
	info->pts_64 = ENCODE_INVALID_PTS;
	info->mv_start_addr = ENCODE_INVALID_ADDRESS;
	clean_cache_aligned((void *)info, sizeof(dsp_enc_stat_info_t));
}

static inline int get_profile_idc(int stream)
{
	int profile_idc;
	iav_h264_config_ex_t * config = &G_encode_config[stream].h264_encode_config;
	switch (config->profile) {
	case H264_BASELINE_PROFILE:
		profile_idc = BASELINE_PROFILE_IDC;
		break;
	case H264_MAIN_PROFILE:
		profile_idc = MAIN_PROFILE_IDC;
		break;
	case H264_HIGH_PROFILE:
		profile_idc = HIGH_PROFILE_IDC;
		break;
	default:
		iav_error("Invalid profile [%d] of stream [%d], use default"
			" [Main profile]!\n", config->profile, stream);
		profile_idc = MAIN_PROFILE_IDC;
		break;
	}
	return profile_idc;
}

static inline int get_level_idc(int stream)
{
	u16 w_MB, h_MB, frame_MB;
	u32 max_MBps, encode_fps, bps;
	u32 level_idc, level_idc_mb, level_idc_mbps, level_idc_bps;

	encode_fps = DIV_ROUND(512000000, get_vin_capability()->frame_rate) *
		G_encode_config[stream].frame_rate_multiplication_factor /
		G_encode_config[stream].frame_rate_division_factor;
	bps = G_encode_config[stream].h264_encode_config.average_bitrate;
	w_MB = ALIGN(G_encode_stream[stream].format.encode_width, 16) / 16;
	h_MB = ALIGN(G_encode_stream[stream].format.encode_height, 16) / 16;
	frame_MB = w_MB * h_MB;
	max_MBps = frame_MB * encode_fps;

	if (max_MBps <= LEVEL_IDC_11_MBPS) {
		level_idc_mbps = LEVEL_IDC_11;
	} else if  (max_MBps <= LEVEL_IDC_12_MBPS) {
		level_idc_mbps = LEVEL_IDC_12;
	} else if (max_MBps <= LEVEL_IDC_13_MBPS) {
		level_idc_mbps = LEVEL_IDC_13;
	} else if (max_MBps <= LEVEL_IDC_21_MBPS) {
		level_idc_mbps = LEVEL_IDC_21;
	} else if (max_MBps <= LEVEL_IDC_22_MBPS) {
		level_idc_mbps = LEVEL_IDC_22;
	} else if (max_MBps <= LEVEL_IDC_30_MBPS) {
		level_idc_mbps = LEVEL_IDC_30;
	} else if (max_MBps <= LEVEL_IDC_31_MBPS) {
		level_idc_mbps = LEVEL_IDC_31;
	} else if (max_MBps <= LEVEL_IDC_32_MBPS) {
		level_idc_mbps = LEVEL_IDC_32;
	} else if (max_MBps <= LEVEL_IDC_40_MBPS) {
		level_idc_mbps = LEVEL_IDC_40;
	} else if (max_MBps <= LEVEL_IDC_42_MBPS) {
		level_idc_mbps = LEVEL_IDC_42;
	} else if (max_MBps <= LEVEL_IDC_50_MBPS) {
		level_idc_mbps = LEVEL_IDC_50;
	} else if (max_MBps <= LEVEL_IDC_51_MBPS) {
		level_idc_mbps = LEVEL_IDC_51;
	} else {
		iav_printk("Too many MBps for H264 spec, use largest level IDC [5.1].\n");
		level_idc_mbps = LEVEL_IDC_51;
	}

	if (frame_MB <= LEVEL_IDC_11_MB) {
		level_idc_mb = LEVEL_IDC_11;
	} else if (frame_MB <= LEVEL_IDC_21_MB) {
		level_idc_mb = LEVEL_IDC_21;
	} else if (frame_MB <= LEVEL_IDC_22_MB) {
		level_idc_mb = LEVEL_IDC_22;
	} else if (frame_MB <= LEVEL_IDC_31_MB) {
		level_idc_mb = LEVEL_IDC_31;
	} else if (frame_MB <= LEVEL_IDC_32_MB) {
		level_idc_mb = LEVEL_IDC_32;
	} else if (frame_MB <= LEVEL_IDC_40_MB) {
		level_idc_mb = LEVEL_IDC_40;
	} else if (frame_MB <= LEVEL_IDC_42_MB) {
		level_idc_mb = LEVEL_IDC_42;
	} else if (frame_MB <= LEVEL_IDC_50_MB) {
		level_idc_mb = LEVEL_IDC_50;
	} else if (frame_MB <= LEVEL_IDC_51_MB) {
		level_idc_mb = LEVEL_IDC_51;
	} else {
		iav_printk("Too large resolution (MB) for H264 spec, use largest level IDC [5.1].\n");
		level_idc_mb = LEVEL_IDC_51;
	}
	level_idc = max(level_idc_mb, level_idc_mbps);

	if (bps <= LEVEL_IDC_11_BR) {
		level_idc_bps = LEVEL_IDC_11;
	} else if (bps <= LEVEL_IDC_12_BR) {
		level_idc_bps = LEVEL_IDC_12;
	} else if (bps <= LEVEL_IDC_13_BR) {
		level_idc_bps = LEVEL_IDC_13;
	} else if (bps <= LEVEL_IDC_20_BR) {
		level_idc_bps = LEVEL_IDC_20;
	} else if (bps <= LEVEL_IDC_21_BR) {
		level_idc_bps = LEVEL_IDC_21;
	} else if (bps <= LEVEL_IDC_30_BR) {
		level_idc_bps = LEVEL_IDC_30;
	} else if (bps <= LEVEL_IDC_31_BR) {
		level_idc_bps = LEVEL_IDC_31;
	} else if (bps <= LEVEL_IDC_32_BR) {
		level_idc_bps = LEVEL_IDC_32;
	} else if (bps <= LEVEL_IDC_41_BR) {
		level_idc_bps = LEVEL_IDC_41;
	} else if (bps <= LEVEL_IDC_50_BR) {
		level_idc_bps = LEVEL_IDC_50;
	} else if (bps <= LEVEL_IDC_51_BR) {
		level_idc_bps = LEVEL_IDC_51;
	} else {
		iav_printk("Too high bitrate (bps) for H264 spec, use largest level IDC [5.1].\n");
		level_idc_bps = LEVEL_IDC_51;
	}

	level_idc = max(level_idc, level_idc_bps);

	return level_idc;
}

static inline int get_colour_primaries(int stream, u32 * color_primaries,
		u32 * transfer_characteristics, u32 * matrix_coefficients)
{
	int width, height;
	width = G_encode_stream[stream].format.encode_width;
	height = G_encode_stream[stream].format.encode_height;
	if (((width == 720) && (height == 480)) ||
			((width == 704) && (height == 480))  ||
			((width == 352) && (height == 240))) {
		//NTSC
		*color_primaries = 6;
		*transfer_characteristics = 6;
		*matrix_coefficients = 6;
	} else if (((width == 720) && (height == 576)) ||
			((width == 704) && (height == 576)) ||
			((width == 352) && (height == 288))) {
		//PAL
		*color_primaries = 5;
		*transfer_characteristics = 5;
		*matrix_coefficients = 5;
	} else {
		//default
		*color_primaries = 1;
		*transfer_characteristics = 1;
		*matrix_coefficients = 1;
	}
	return 0;
}

inline int get_gcd(int a, int b)
{
	if ((a == 0) || (b == 0)) {
		iav_printk("wrong input for gcd \n");
		return 1;
	}
	while ((a != 0) && (b != 0)) {
		if (a > b) {
			a = a%b;
		} else {
			b = b%a;
		}
	}
	return (a == 0) ? b : a;
}

static inline int get_aspect_ratio(iav_reso_ex_t* buffer, iav_rect_ex_t* input,
	u8 *aspect_ratio_idc, u16 *sar_width, u16 *sar_height)
{
	int gcd, num, den;

	num = input->width * buffer->height;
	den = input->height * buffer->width;
	gcd = get_gcd(num, den);
	num = num / gcd * (*sar_width);
	den = den / gcd * (*sar_height);
	if (num == den) {
		//square pixel, 1:1
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL;
		*sar_width = 1;
		*sar_height = 1;
	} else {
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_CUSTOM;
		gcd = get_gcd(num, den);
		*sar_width = num / gcd;
		*sar_height = den / gcd;
	}
	return 0;
}

static inline void get_pic_info_in_h264(int stream, iav_h264_config_ex_t * config)
{
	u8 vfr, multi_frames;
	struct amba_vin_src_capability * vin = NULL;

	config->pic_info.ar_x = 0;
	config->pic_info.ar_y = 0;
	config->pic_info.frame_mode = PAFF_ALL_FRM;
	config->pic_info.width = G_encode_stream[stream].format.encode_width;
	config->pic_info.height = G_encode_stream[stream].format.encode_height;

	if (G_iav_info.pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) {
		vin = get_vin_capability();
		multi_frames = is_hdr_frame_interleaved_mode() ?
			get_expo_num(get_enc_mode()) : 1;
		vfr = amba_iav_fps_format_to_vfr(vin->frame_rate, vin->video_format,
			multi_frames);
		if (vfr < 2) {
			config->pic_info.rate = 6006;
		} else if (vfr < 4) {
			config->pic_info.rate = 3003;
		} else {
			config->pic_info.rate = 180000 / vfr;
		}
		config->pic_info.scale = 180000;
	}
}

static inline u8 * prepare_h264_q_matrix(u32 id)
{
	int i;
	u8 intra[16] = {
		6, 13, 20, 28, 13, 20, 28, 32,
		20, 28, 32, 37, 28, 32, 37, 42};
	u8 inter[16] = {
		10, 14, 20, 24, 14, 20, 24, 27,
		20, 24, 27, 30, 24, 27, 30, 34};
	u8 *addr = G_q_matrix_addr + id * Q_MATRIX_SIZE;

	for (i = 0; i < 3; ++i) {
		memcpy((addr + i * 16), intra, 16);
		memcpy((addr + i * 16 + Q_MATRIX_SIZE / 2), inter, 16);
	}

	return addr;
}

static inline void prepare_encode_init(void)
{
	int i;
	u16 width, height, source;
	u32 bitrate;
	iav_bitrate_info_ex_t * rc = NULL;
	iav_encode_stream_ex_t * stream = NULL;
	iav_stream_encode_config_ex_t * config = NULL;
	iav_h264_config_ex_t * h264 = NULL;
	iav_jpeg_config_ex_t * jpeg = NULL;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (i == 0) {
			width = 1920;
			height = 1080;
			source = IAV_ENCODE_SOURCE_MAIN_BUFFER;
			bitrate = 8000000;
		} else if (i < IPCAM_RECORD_MAX_NUM_ENC) {
			width = 720;
			height = 480;
			source = IAV_ENCODE_SOURCE_SECOND_BUFFER;
			bitrate = 4000000;
		} else {
			width = 352;
			height = 288;
			source = IAV_ENCODE_SOURCE_SECOND_BUFFER;
			bitrate = 1000000;
		}

		rc = &G_rate_control_info[i];
		rc->id = (1 << i);
		rc->rate_control_mode = IAV_CBR;
		rc->cbr_avg_bitrate = bitrate;
		rc->vbr_min_bitrate = 1000000;
		rc->vbr_max_bitrate = 6000000;

		stream = &G_encode_stream[i];
		stream->stream_type = STRM_TP_ENC_FIRST + i;
		stream->op = IAV_ENCODE_OP_STOP;
		stream->format.id = (1 << i);
		stream->format.hflip = 0;
		stream->format.vflip = 0;
		stream->format.rotate_clockwise = 0;
		stream->format.negative_offset_disable = 0;
		stream->format.encode_width = width;
		stream->format.encode_height = height;
		stream->format.encode_type = IAV_ENCODE_NONE;
		stream->format.source = source;
		stream->format.duration = IAV_ENC_DURATION_FOREVER;
		stream->fake_dsp_state = ENC_IDLE_STATE;
		init_waitqueue_head(&stream->venc_wq);

		config = &G_encode_config[i];
		config->frame_rate_multiplication_factor = 1;
		config->frame_rate_division_factor = 1;
		h264 = &config->h264_encode_config;
		h264->id = (1 << i);
		h264->M = 1;
		h264->N = 30;
		h264->gop_model = IAV_GOP_SIMPLE;
		h264->debug_long_p_interval = 0;
		h264->idr_interval = 1;
		h264->bitrate_control = IAV_BRC_SCBR;
		h264->slice_alpha_c0_offset_div2 = 0;
		h264->slice_beta_offset_div2 = 0;
		h264->deblocking_filter_disable = 0;
		h264->average_bitrate = bitrate;
		h264->qp_min_on_I = 14;
		h264->qp_max_on_I = 51;
		h264->qp_min_on_P = 17;
		h264->qp_max_on_P = 51;
		h264->qp_min_on_B = 21;
		h264->qp_max_on_B = 51;
		h264->qp_min_on_C = 21;
		h264->qp_max_on_C = 51;
		h264->qp_min_on_D = 21;
		h264->qp_max_on_D = 51;
		h264->qp_min_on_Q = 14;
		h264->qp_max_on_Q = 51;
		h264->adapt_qp  = 2;
		h264->i_qp_reduce = 6;
		h264->p_qp_reduce = 3;
		h264->b_qp_reduce = 3;
		h264->c_qp_reduce = 3;
		h264->q_qp_reduce = 6;
		h264->log_q_num_minus_1 = 0;
		h264->skip_flag = 0;
		h264->zmv_threshold = 0;
		h264->mode_bias_I4Add = 0;
		h264->mode_bias_I16Add = -4;
		h264->mode_bias_Inter8Add = -2;
		h264->mode_bias_Inter16Add = -2;
		h264->mode_bias_DirectAdd = -2;
		h264->chroma_format = IAV_H264_CHROMA_FORMAT_YUV420;
		h264->intra_refresh_mb_rows = 0;
		h264->au_type = IAV_AUD_BEFORE_SPS_WITH_SEI;
		h264->qp_roi_enable = 0;
		h264->qp_roi_type = IAV_QP_ROI_TYPE_BASIC;
		h264->profile = H264_MAIN_PROFILE;
		h264->panic_num = 2;
		h264->panic_den = 1;
		h264->intrabias_P = 1;
		h264->intrabias_B = 1;
		h264->nonSkipCandidate_bias = 1;
		h264->skipCandidate_threshold = 1;
		h264->cpb_underflow_num = 1;
		h264->cpb_underflow_den = 1;
		h264->qmatrix_4x4_daddr = prepare_h264_q_matrix(i);
		h264->user1_intra_bias = 0;
		h264->user1_direct_bias = 0;
		h264->user2_intra_bias = 0;
		h264->user2_direct_bias = 0;
		h264->user3_intra_bias = 0;
		h264->user3_direct_bias = 0;
		h264->numRef_P = 0;

		jpeg = &config->jpeg_encode_config;
		jpeg->id = (1 << i);
		jpeg->chroma_format = IAV_JPEG_CHROMA_FORMAT_YUV420;
		jpeg->quality = 50;
		jpeg->jpeg_quant_matrix = NULL;
	}
}

static int calc_vbr_cntl(iav_h264_config_ex_t *config)
{
	int max_vbr_rate_factor, min_vbr_rate_factor;
	u32 max_int, max_fraction;
	int calibration, vbr_ness;

	calibration = 100;
	vbr_ness = 90;
	max_vbr_rate_factor = 400;
	min_vbr_rate_factor = 100;
	max_int = max_vbr_rate_factor / 100;
	max_fraction = max_vbr_rate_factor % 100;

	max_fraction = (max_fraction * (1 << 6) / 100) & 0x3F;

	return calibration |
		(vbr_ness << 8) |
		(min_vbr_rate_factor << 16) |
		(max_fraction << 23) | (max_int << 29);
}

inline void calc_roundup_size(u16 width_in, u16 height_in,
		u32 enc_type, u16 *width_out, u16 *height_out)
{
	u16 round_h;

	if (enc_type == IAV_ENCODE_H264) {
		round_h = (G_iav_info.pvininfo->capability.video_format ==
			AMBA_VIDEO_FORMAT_INTERLACE) ? (PIXEL_IN_MB << 1) : PIXEL_IN_MB;
		*width_out = ALIGN(width_in, PIXEL_IN_MB);
		*height_out = ALIGN(height_in, round_h);
	} else {
		*width_out = width_in;
		*height_out = height_in;
	}
}

int calc_encode_frame_rate(u32 vin_frame_rate,
	u32 multiplication_factor, u32 division_factor,
	u32 * dsp_encode_frame_rate)
{
	int interlaced = is_video_encode_interlaced();
	u8 scan_format = 0;
	u8 den_encode_frame_rate = 0;
	u32 custom_encoder_frame_rate;

	switch (vin_frame_rate) {
		case AMBA_VIDEO_FPS_59_94:
			den_encode_frame_rate = 1;
			if (interlaced)
				custom_encoder_frame_rate = 30000;		// 59.94i --> 29.97p
			else
				custom_encoder_frame_rate = 60000;		// 59.94p
			break;
		case AMBA_VIDEO_FPS_50:
			if (interlaced)
				custom_encoder_frame_rate = 25000;		// 50i --> 25p
			else
				custom_encoder_frame_rate = 50000;
			break;
		case AMBA_VIDEO_FPS_29_97:
			den_encode_frame_rate = 1;
			custom_encoder_frame_rate = 30000;
			break;
		case AMBA_VIDEO_FPS_23_976:
			den_encode_frame_rate = 1;
			custom_encoder_frame_rate = 24000;
			break;
		case AMBA_VIDEO_FPS_12_5:
			custom_encoder_frame_rate = 12500;
			break;
		case AMBA_VIDEO_FPS_6_25:
			custom_encoder_frame_rate = 6250;
			break;
		case AMBA_VIDEO_FPS_3_125:
			custom_encoder_frame_rate = 3125;
			break;
		case AMBA_VIDEO_FPS_7_5:
			custom_encoder_frame_rate = 7500;
			break;
		case AMBA_VIDEO_FPS_3_75:
			custom_encoder_frame_rate = 3750;
			break;
		default:
			custom_encoder_frame_rate = DIV_ROUND(512000000, vin_frame_rate) * 1000;
			break;
	}

	scan_format = interlaced;
	custom_encoder_frame_rate = custom_encoder_frame_rate *
		multiplication_factor / division_factor;
	*dsp_encode_frame_rate = ((scan_format << SCAN_FORMAT_BIT_SHIFT) |
			(den_encode_frame_rate << DENOMINATOR_BIT_SHIFT) |
			custom_encoder_frame_rate);

	iav_printk("vin fps %d (scan format: %d, den: %d), encode fps %d\n",
			vin_frame_rate, scan_format,
			den_encode_frame_rate ? 1001 : 1000, custom_encoder_frame_rate);

	return 0;
}

inline void get_round_encode_format(int stream, u16* width, u16* height,
	s16* offset_y_shift)
{
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;
	u16 round_width, round_height;
	u16* enc_width, *enc_height;

	if (__is_enc_h264(stream)) {
		calc_roundup_size(format->encode_width, format->encode_height,
			IAV_ENCODE_H264, &round_width, &round_height);
		enc_width = &round_width;
		enc_height = &round_height;
	} else {
		calc_roundup_size(format->encode_width, format->encode_height,
			is_high_mp_full_perf_mode() ? IAV_ENCODE_H264 : IAV_ENCODE_MJPEG,
				&round_width, &round_height);
		enc_width = &format->encode_width;
		enc_height = &format->encode_height;
	}

	if (format->rotate_clockwise) {
		*width = *enc_height;
		*height = *enc_width;
		*offset_y_shift =
			(format->hflip ? 0 : (format->encode_height - round_height));
	} else {
		*width = *enc_width;
		*height = *enc_height;
		*offset_y_shift =
			(format->vflip ? (format->encode_height - round_height) : 0);
	}
	if (format->negative_offset_disable) {
		*offset_y_shift = 0;
	}
}

