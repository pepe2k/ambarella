/*******************************************************************************
 * lbr_param_priv.h
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
#ifndef __LBR_PARAM_PRIV_H__
#define __LBR_PARAM_PRIV_H__

#define MAX_LBR_STREAM_NUM 4

static lbr_control_t g_lbr_control_param_IPPP_GOP[LBR_PROFILE_NUM] = {
	{	//PROFILE 0 : LBR_PROFILE_STATIC
		.bitrate_target = {
			.bitrate_1080p = 100* 1024,
			.bitrate_720p = 80 * 1024,
			.bitrate_per_MB = 23,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 1 : LBR_PROFILE_SMALL_MOTION
		.bitrate_target = {
			.bitrate_1080p = 600* 1024,
			.bitrate_720p = 400 * 1024,
			.bitrate_per_MB = 114,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 2 : LBR_PROFILE_BIG_MOTION
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 3 : LBR_PROFILE_LOW_LIGHT
		.bitrate_target = {
			.bitrate_1080p = 800* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 32,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 4 : LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 41,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 41,
		.skip_frame_mode = 4,
	},
	{	//PROFILE 5 : LBR_PROFILE_SECURIY_IPCAM_CBR
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 0,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
	},
	{	//PROFILE 6 : LBR_PROFILE_RESERVE2
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
	},
	{	//PROFILE 7 : LBR_PROFILE_RESERVE3
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
};

static lbr_control_t g_lbr_control_param_IBBP_GOP[LBR_PROFILE_NUM] = {
	{	//PROFILE 0 : LBR_PROFILE_STATIC
		.bitrate_target = {
			.bitrate_1080p = 100* 1024,
			.bitrate_720p = 80 * 1024,
			.bitrate_per_MB = 23,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 1 : LBR_PROFILE_SMALL_MOTION
		.bitrate_target = {
			.bitrate_1080p = 500* 1024,
			.bitrate_720p = 300 * 1024,
			.bitrate_per_MB = 85,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 2 : LBR_PROFILE_BIG_MOTION
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 3 : LBR_PROFILE_LOW_LIGHT
		.bitrate_target = {
			.bitrate_1080p = 600* 1024,
			.bitrate_720p = 400 * 1024,
			.bitrate_per_MB = 114,
		},
		.zmv_threshold = 32,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 4 : LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 41,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 41,
		.skip_frame_mode = 4,
		.adapt_qp = 0,
	},
	{	//PROFILE 5 : LBR_PROFILE_SECURIY_IPCAM_CBR
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 0,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 2,
	},
	{	//PROFILE 6 : LBR_PROFILE_RESERVE2
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
	{	//PROFILE 7 : LBR_PROFILE_RESERVE3
		.bitrate_target = {
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.P_qp_limit_min = 0,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 0,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 0,
	},
};

#endif	// __LBR_PARAM_PRIV_H__
