/*******************************************************************************
 * lbr_api.c
 *
 * History:
 *	2014/05/02 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "basetypes.h"
#include "iav_drv.h"
#include "lbr_api.h"
#include "lbr_param_priv.h"

int G_lbr_log_level = 0;
static lbr_init_t g_lbr_init_data;
static int g_lbr_init_done = 0;
static lbr_control_context_t g_lbr_control_context[LBR_PROFILE_NUM];
static lbr_profile_bitrate_gap_gop_t bitrate_gap;

static inline int calculate_bitrate_target(
            lbr_control_context_t *context,
            lbr_bitrate_case_target_t *case_target)
{
	int width, height;
	u32 bitrate_ceiling;
	u32 final_bitrate;
	int bits_per_MB;

	width = context->encode_config.width;
	height = context->encode_config.height;

	//if the lbr style is CBR alike, then
	if (context->lbr_style == LBR_STYLE_FPS_KEEP_CBR_ALIKE) {
		if (context->bitrate_target.auto_target) {
			final_bitrate = LOW_BITRATE_CEILING_DEFAULT;
		} else {
			final_bitrate = context->bitrate_target.bitrate_ceiling;
		}
		return final_bitrate;
	}

	//if not CBR alike style, then do some calculations
	if ((width == 1920) && (height == 1080)) {
		final_bitrate = case_target->bitrate_1080p;
#ifdef LBR_BITRATE_GAP_ADJ
		/*Adjust bitrate according to GOP length when
		 choose LBR_PROFILE_STATIC or LBR_PROFILE_SMALL_MOTION*/
		if (context->lbr_profile_type == LBR_PROFILE_STATIC) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct
					final_bitrate += bitrate_gap.bg_IBBP.sm_nm.bitrate_1080p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += bitrate_gap.bg_IPPP.sm_nm.bitrate_1080p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		} else if (context->lbr_profile_type == LBR_PROFILE_SMALL_MOTION) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct

					final_bitrate += bitrate_gap.bg_IBBP.bm_sm.bitrate_1080p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += bitrate_gap.bg_IPPP.bm_sm.bitrate_1080p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		}
#endif
	} else if ((width == 1280) && (height == 720)) {
		final_bitrate = case_target->bitrate_720p;
#ifdef LBR_BITRATE_GAP_ADJ
		/*Adjust bitrate according to GOP length when
		 * choose LBR_PROFILE_STATIC or LBR_PROFILE_SMALL_MOTION*/
		if (context->lbr_profile_type == LBR_PROFILE_STATIC) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct
					final_bitrate += bitrate_gap.bg_IBBP.sm_nm.bitrate_720p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += bitrate_gap.bg_IPPP.sm_nm.bitrate_720p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		} else if (context->lbr_profile_type == LBR_PROFILE_SMALL_MOTION) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct
					final_bitrate += bitrate_gap.bg_IBBP.bm_sm.bitrate_720p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += bitrate_gap.bg_IPPP.bm_sm.bitrate_720p
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		}
#endif
	} else {
		final_bitrate = ((width * height) >> 8) * case_target->bitrate_per_MB;
#ifdef LBR_BITRATE_GAP_ADJ
		/*Adjust bitrate according to GOP length when
		 * choose LBR_PROFILE_STATIC or LBR_PROFILE_SMALL_MOTION*/
		if (context->lbr_profile_type == LBR_PROFILE_STATIC) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct
					final_bitrate += ((width * height) >> 8)
					            * bitrate_gap.bg_IBBP.sm_nm.bitrate_per_MB
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += ((width * height) >> 8)
					            * bitrate_gap.bg_IPPP.sm_nm.bitrate_per_MB
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		} else if (context->lbr_profile_type == LBR_PROFILE_SMALL_MOTION) {
			if (context->encode_config.N <= LBR_GOP_LENGTH_PREFER
			            && context->encode_config.N >= LBR_GOP_LENGTH_MIN) {
				if (context->encode_config.M > 1) { //IBBP GOP struct
					final_bitrate += ((width * height) >> 8)
					            * bitrate_gap.bg_IBBP.bm_sm.bitrate_per_MB
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				} else { //IPPP GOP struct
					final_bitrate += ((width * height) >> 8)
					            * bitrate_gap.bg_IPPP.bm_sm.bitrate_per_MB
					            * (LBR_GOP_LENGTH_PREFER
					                        - context->encode_config.N)
					            / (LBR_GOP_LENGTH_PREFER * 2);
				}
			}
		}
#endif
	}

	LBR_DEBUG("look up table, bitrate seems to be %d \n", final_bitrate);

	//if bitrate target is full auto, then no need to care about target scaling or clamp
	if (context->bitrate_target.auto_target) {
		LBR_DEBUG("lbr bitrate ceiling is auto\n");
		return final_bitrate;
	}

	//otherwise scale or clamp the bitrate params according to bitrate_ceiling
	bitrate_ceiling = context->bitrate_target.bitrate_ceiling;
	bits_per_MB = bitrate_ceiling / ((width * height) >> 8);

	LBR_DEBUG("bitrate ceiling is %d \n", bitrate_ceiling);

	if (bits_per_MB > HIGH_BITRATE_PER_MB_TRESHOLD) {
		LBR_DEBUG("bitrate ceiling is normal bitrate mode, scale up for target \n");
		final_bitrate = final_bitrate
		            * bits_per_MB/ HIGH_BITRATE_PER_MB_TRESHOLD;
	} else {
		LBR_DEBUG("bitrate ceiling is low bitrate mode, use bitrate ceiling to clamp bitrate\n");
	}
	if (final_bitrate > bitrate_ceiling) {
		final_bitrate = bitrate_ceiling;
	}
	LBR_DEBUG("final bitrate selected is %d \n", final_bitrate);

	return final_bitrate;
}

static int update_lbr_profile(LBR_PROFILE_TYPE next_profile, int stream_id)
{
	lbr_control_context_t *context = &g_lbr_control_context[stream_id];
	lbr_control_t *lbr_control_param = NULL;
	u32 bitrate_choice;
	int fd_iav = g_lbr_init_data.fd_iav;
	iav_bitrate_info_ex_t bitrate_info;
	iav_encode_format_ex_t format;
	iav_h264_config_ex_t h264_config;
	iav_h264_enc_param_ex_t h264_enc;

	iav_change_qp_limit_ex_t qp_limit;

	if (!g_lbr_init_done) {
		return -1;
	}
	/*convert the lbr profile to IAV IOCTLs.
	look up table g_lbr_control_param_IPPP_GOP  or g_lbr_control_param_IBBP_GOP
	and issue DSP cmds
	update GOP length here*/

	//do basic check on the stream
	format.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
	if (format.encode_type != IAV_ENCODE_H264) {
		LBR_ERROR("stream %d is not H.264 \n", stream_id);
		return -1;
	}

	//check encode width/height valid
	if ((format.encode_width < LBR_ENCODE_WIDTH_MIN)
	            || (format.encode_width > LBR_ENCODE_WIDTH_MAX)) {
		LBR_ERROR("encode width invalid %d \n", format.encode_width);
		return -1;
	}

	if ((format.encode_height < LBR_ENCODE_HEIGHT_MIN)
	            || (format.encode_height > LBR_ENCODE_HEIGHT_MAX)) {
		LBR_ERROR("encode height invalid %d \n", format.encode_height);
		return -1;
	}
	context->encode_config.width = format.encode_width;
	context->encode_config.height = format.encode_height;

	//get current H264 config
	h264_config.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &h264_config);
	context->encode_config.N = h264_config.N;
	context->encode_config.M = h264_config.M;
	context->lbr_profile_type = next_profile;

	if (h264_config.M > 1) {
		lbr_control_param = &g_lbr_control_param_IBBP_GOP[next_profile];
	} else {
		lbr_control_param = &g_lbr_control_param_IPPP_GOP[next_profile];
	}

	if (h264_config.N > LBR_GOP_LENGTH_MAX || h264_config.N < LBR_GOP_LENGTH_MIN) {
		LBR_MSG("warning: current GOP lenght is %d for stream %d ,"
		        "range of GOP length is [%d, %d]. Prefer GOP length is %d.\n",
		        h264_config.N, stream_id, LBR_GOP_LENGTH_MIN,
		        LBR_GOP_LENGTH_MAX, LBR_GOP_LENGTH_PREFER);
	}

	//set bitrate new target
	bitrate_info.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_BITRATE_EX, &bitrate_info);
	LBR_DEBUG("before applying new value, previous lbr DSP bitrate for stream " "%d  is %d\n",
	          stream_id, bitrate_info.cbr_avg_bitrate);
	bitrate_choice = calculate_bitrate_target(
	            context, &lbr_control_param->bitrate_target);

	LBR_DEBUG("stream %d 's bitrate change to %d\n", stream_id, bitrate_choice);

	if (context->lbr_style != LBR_STYLE_FPS_KEEP_CBR_ALIKE) {
		if ((bitrate_choice != bitrate_info.cbr_avg_bitrate)
		            || (bitrate_info.rate_control_mode == IAV_CBR2)) {
			//only change bitrate if new target bitrate is different
			bitrate_info.cbr_avg_bitrate = bitrate_choice;
			bitrate_info.rate_control_mode = IAV_CBR;
			AM_IOCTL(fd_iav, IAV_IOC_SET_BITRATE_EX, &bitrate_info);
		}
	} else {
		//CBR style
		if ((bitrate_choice != bitrate_info.cbr_avg_bitrate)
		            || (bitrate_info.rate_control_mode != IAV_CBR2)) {
			//only change bitrate if new target bitrate is different
			bitrate_info.cbr_avg_bitrate = bitrate_choice;
			bitrate_info.rate_control_mode = IAV_CBR2;
			AM_IOCTL(fd_iav, IAV_IOC_SET_BITRATE_EX, &bitrate_info);
		}
	}

	//zmv threshold
	h264_enc.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_ENC_PARAM_EX, &h264_enc);
	if (h264_enc.zmv_threshold != lbr_control_param->zmv_threshold) {
		h264_enc.zmv_threshold = lbr_control_param->zmv_threshold;
		AM_IOCTL(fd_iav, IAV_IOC_SET_H264_ENC_PARAM_EX, &h264_enc);
	}

	//compare diff
	qp_limit.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_QP_LIMIT_EX, &qp_limit);
	if ((qp_limit.i_qp_reduce != lbr_control_param->I_qp_reduce)
	            || (qp_limit.qp_min_on_P != lbr_control_param->P_qp_limit_min)
	            || (qp_limit.qp_max_on_P != lbr_control_param->P_qp_limit_max)
	            || (qp_limit.qp_min_on_B != lbr_control_param->B_qp_limit_min)
	            || (qp_limit.qp_max_on_B != lbr_control_param->B_qp_limit_max)
	            || (qp_limit.skip_flag != lbr_control_param->skip_frame_mode)
	            || (qp_limit.adapt_qp != lbr_control_param->adapt_qp)) {

		qp_limit.i_qp_reduce = lbr_control_param->I_qp_reduce;
		qp_limit.qp_min_on_P = lbr_control_param->P_qp_limit_min;
		qp_limit.qp_max_on_P = lbr_control_param->P_qp_limit_max;
		qp_limit.qp_min_on_B = lbr_control_param->B_qp_limit_min;
		qp_limit.qp_max_on_B = lbr_control_param->B_qp_limit_max;
		qp_limit.skip_flag = lbr_control_param->skip_frame_mode;
		qp_limit.adapt_qp = lbr_control_param->adapt_qp;

		LBR_DEBUG("qp limit skip flag is %d \n", qp_limit.skip_flag);

		AM_IOCTL(fd_iav, IAV_IOC_CHANGE_QP_LIMIT_EX, &qp_limit);
	}

	return 0;
}

static int switch_lbr_profile(int stream_id)
{
	int ret = 0;
	lbr_control_context_t *context = &g_lbr_control_context[stream_id];
	LBR_PROFILE_TYPE current_profile = context->lbr_profile_type;
	LBR_PROFILE_TYPE next_profile = current_profile;

	do {
		//special handling for Security IPCAM style CBR alike
		if (context->lbr_style == LBR_STYLE_FPS_KEEP_CBR_ALIKE) {
			next_profile = LBR_PROFILE_SECURIY_IPCAM_CBR;
			break;
		}
		if (context->motion_level == LBR_MOTION_NONE) {
			if (context->noise_level == LBR_NOISE_NONE) {
				next_profile = LBR_PROFILE_STATIC;
			} else if (context->noise_level == LBR_NOISE_LOW) {
				next_profile = LBR_PROFILE_STATIC;
			} else {
				next_profile = LBR_PROFILE_LOW_LIGHT;
			}
		} else if (context->motion_level == LBR_MOTION_LOW) {
			if (context->noise_level == LBR_NOISE_NONE) {
				next_profile = LBR_PROFILE_SMALL_MOTION;
			} else if (context->noise_level == LBR_NOISE_LOW) {
				next_profile = LBR_PROFILE_SMALL_MOTION;
			} else {
				next_profile = LBR_PROFILE_LOW_LIGHT;
			}
		} else {
			//high motion
			if ((context->lbr_style == LBR_STYLE_FPS_KEEP_BITRATE_AUTO)
			            || (context->lbr_style == LBR_STYLE_FPS_KEEP_CBR_ALIKE)) {
				if (context->noise_level == LBR_NOISE_HIGH) {
					next_profile = LBR_PROFILE_LOW_LIGHT;
				} else {
					next_profile = LBR_PROFILE_BIG_MOTION;
				}
			} else {
				next_profile = LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP;
			}
		}
	} while (0);

	if (update_lbr_profile(next_profile, stream_id) < 0) {
		LBR_ERROR("fail to switch lbr profile for stream %d\n", stream_id);
		ret = -1;
	} else {
		context->lbr_profile_type = next_profile;
		LBR_DEBUG("stream%d: lbr current profile is: ", stream_id);
		switch (next_profile) {
			case LBR_PROFILE_STATIC:
				LBR_DEBUG("STATIC");
				break;
			case LBR_PROFILE_SMALL_MOTION:
				LBR_DEBUG("SMALL MOTION");
				break;
			case LBR_PROFILE_BIG_MOTION:
				LBR_DEBUG("BIG MOTION");
				break;
			case LBR_PROFILE_LOW_LIGHT:
				LBR_DEBUG("LOW LIGHT");
				break;
			case LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP:
				LBR_DEBUG("BIG MOTION WITH FRAME DROP");
				break;
			case LBR_PROFILE_SECURIY_IPCAM_CBR:
				LBR_DEBUG("SECURITY IPCAM STYLE CBR");
				break;
			default:
				LBR_DEBUG("Error Profile, unknown");
				break;
		}
		LBR_DEBUG("\n");
	}

	return ret;
}

int lbr_open()
{
	g_lbr_init_done = 0;
	return 0;
}

//init lbr library with iav file handle and etc., must call init after open
int lbr_init(lbr_init_t *rc_init)
{
	int i;
	lbr_control_context_t *context;

	g_lbr_init_data = *rc_init;

	for (i = 0; i < LBR_MAX_STREAM_NUM; i ++) {
		context = &g_lbr_control_context[i];
		//set all to default '0'
		memset(context, 0, sizeof(*context));
		context->motion_level = LBR_MOTION_HIGH;
		context->noise_level = LBR_NOISE_HIGH;
		context->bitrate_target.auto_target = 1; //enable auto mode by default
		context->bitrate_target.bitrate_ceiling = LOW_BITRATE_CEILING_DEFAULT;
	}

	bitrate_gap.bg_IPPP.bm_sm.bitrate_1080p =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_1080p
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_1080p;
	bitrate_gap.bg_IPPP.sm_nm.bitrate_1080p =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_1080p
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_1080p;
	bitrate_gap.bg_IPPP.bm_sm.bitrate_720p =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_720p
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_720p;
	bitrate_gap.bg_IPPP.sm_nm.bitrate_720p =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_720p
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_720p;
	bitrate_gap.bg_IPPP.bm_sm.bitrate_per_MB =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_per_MB
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_per_MB;
	bitrate_gap.bg_IPPP.sm_nm.bitrate_per_MB =
	            g_lbr_control_param_IPPP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_per_MB
	                        - g_lbr_control_param_IPPP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_per_MB;

	bitrate_gap.bg_IBBP.bm_sm.bitrate_1080p =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_1080p
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_1080p;
	bitrate_gap.bg_IBBP.sm_nm.bitrate_1080p =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_1080p
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_1080p;
	bitrate_gap.bg_IBBP.bm_sm.bitrate_720p =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_720p
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_720p;
	bitrate_gap.bg_IBBP.sm_nm.bitrate_720p =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_720p
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_720p;
	bitrate_gap.bg_IBBP.bm_sm.bitrate_per_MB =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_BIG_MOTION]
	                        .bitrate_target.bitrate_per_MB
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                                    .bitrate_target.bitrate_per_MB;
	bitrate_gap.bg_IBBP.sm_nm.bitrate_per_MB =
	            g_lbr_control_param_IBBP_GOP[LBR_PROFILE_SMALL_MOTION]
	                        .bitrate_target.bitrate_per_MB
	                        - g_lbr_control_param_IBBP_GOP[LBR_PROFILE_STATIC]
	                                    .bitrate_target.bitrate_per_MB;

	g_lbr_init_done = 1;
	return 0;
}

int lbr_close() //close will also deinit it
{
	g_lbr_init_done = 0;
	return 0;
}

int lbr_set_style(LBR_STYLE style, int stream_id) //let LBR to setup style
{
	lbr_control_context_t *context;
	if (!g_lbr_init_done) {
		return -1;
	}
	VERIFY_STREAM_ID(stream_id);
	context = &g_lbr_control_context[stream_id];
	context->lbr_style = style;

	return switch_lbr_profile(stream_id);
}

int lbr_set_motion_level(LBR_MOTION_LEVEL motion_level, int stream_id)
{
	lbr_control_context_t *context;
	if (!g_lbr_init_done) {
		return -1;
	}
	VERIFY_STREAM_ID(stream_id);
	context = &g_lbr_control_context[stream_id];
	context->motion_level = motion_level;
	return switch_lbr_profile(stream_id);
}

int lbr_set_noise_level(LBR_NOISE_LEVEL noise_level, int stream_id)
{
	lbr_control_context_t *context;
	if (!g_lbr_init_done) {
		return -1;
	}
	VERIFY_STREAM_ID(stream_id);
	context = &g_lbr_control_context[stream_id];
	context->noise_level = noise_level;

	return switch_lbr_profile(stream_id);
}

int lbr_set_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id)
{
	lbr_control_context_t *context;
	if (!g_lbr_init_done) {
		return -1;
	}

	if (!bitrate_target) {
		return -1;
	}
	VERIFY_STREAM_ID(stream_id);

	if (!(bitrate_target->auto_target)) {
		if (bitrate_target->bitrate_ceiling < LBR_BITRATE_MIN) {
			LBR_ERROR("bitrate ceiling cannot be set to very low %d \n",
			          bitrate_target->bitrate_ceiling);
			return -1;
		}

		if (bitrate_target->bitrate_ceiling > LBR_BITRATE_MAX) {
			LBR_ERROR("bitrate ceiling cannot be set to very high %d \n",
			          bitrate_target->bitrate_ceiling);
			return -1;
		}
	} else {
		LBR_DEBUG("bitrate ceiling is set to auto \n");
	}

	context = &g_lbr_control_context[stream_id];
	context->bitrate_target = *bitrate_target;

	return switch_lbr_profile(stream_id);
}

u32 lbr_get_version()
{
	return ((LBR_VERSION_MAJOR << 16) | (LBR_VERSION_MINOR));
}

int lbr_get_style(LBR_STYLE *style, int stream_id)
{
	lbr_control_context_t *context;
	VERIFY_STREAM_ID(stream_id);
	if (!style) {
		return -1;
	}
	context = &g_lbr_control_context[stream_id];
	*style = context->lbr_style;

	return 0;
}

int lbr_get_motion_level(LBR_MOTION_LEVEL *motion_level, int stream_id)
{
	lbr_control_context_t *context;
	VERIFY_STREAM_ID(stream_id);
	if (!motion_level) {
		return -1;
	}
	context = &g_lbr_control_context[stream_id];
	*motion_level = context->motion_level;
	return 0;
}

int lbr_get_noise_level(LBR_NOISE_LEVEL *noise_level, int stream_id)
{
	lbr_control_context_t *context;
	VERIFY_STREAM_ID(stream_id);
	if (!noise_level) {
		return -1;
	}
	context = &g_lbr_control_context[stream_id];
	*noise_level = context->noise_level;
	return 0;
}

int lbr_get_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id)
{
	lbr_control_context_t *context;
	VERIFY_STREAM_ID(stream_id);
	if (!bitrate_target) {
		return -1;
	}
	context = &g_lbr_control_context[stream_id];
	*bitrate_target = context->bitrate_target;
	return 0;
}

int lbr_get_log_level(int *pLog)
{
	if (pLog == NULL ) {
		LBR_MSG("[lbr_get_log_level] : Pointer is NULL");
		return -1;
	}
	*pLog = G_lbr_log_level;
	return 0;
}

int lbr_set_log_level(int log)
{
	if (log < LBR_ERROR_LEVEL || log > (LBR_LOG_LEVEL_NUM - 1)) {
		LBR_MSG("Invalid log level, please set it in the range of (0~%d).\n",
		        (LBR_LOG_LEVEL_NUM - 1));
		return -1;
	}
	G_lbr_log_level = log;
	return 0;
}
