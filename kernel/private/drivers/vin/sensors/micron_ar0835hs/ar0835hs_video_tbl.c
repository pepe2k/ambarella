/*
 * Filename : ar0835hs_video_tbl.c
 *
 * History:
 *    2012/12/26 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

/* the table is used to provide video info to application */
static const struct ar0835hs_video_info ar0835hs_video_info_table[] = {
	[0] = { //3264x2448@30fps
		.format_index = 0,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 3264,
		.def_height = 2448,
		.slvs_eav_col = 3264,		/*src_width */
		.slvs_sav2sav_dist = 3440,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = { //2304x1836@30fps
		.format_index = 1,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 2304,
		.def_height = 1836,
		.slvs_eav_col = 2304,		/*src_width */
		.slvs_sav2sav_dist = 4392,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = { //3264x1836@30fps
		.format_index = 2,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 3264,
		.def_height = 1836,
		.slvs_eav_col = 3264,		/*src_width */
		.slvs_sav2sav_dist = 4752,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = { //2208x1836@60fps
		.format_index = 3,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 2208,
		.def_height = 1836,
		.slvs_eav_col = 2208,		/*src_width */
		.slvs_sav2sav_dist = 2376,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[4] = { //1280x918@120fps
		.format_index = 4,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1280,
		.def_height = 918,
		.slvs_eav_col = 1280,		/*src_width */
		.slvs_sav2sav_dist = 2276,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[5] = { //816x612@120fps
		.format_index = 5,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 816,
		.def_height = 612,
		.slvs_eav_col = 816,		/*src_width */
		.slvs_sav2sav_dist = 2352,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[6] = { //320x306@240fps
		.format_index = 6,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 320,
		.def_height = 306,
		.slvs_eav_col = 320,		/*src_width */
		.slvs_sav2sav_dist = 2352,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[7] = { //1080p@60fps
		.format_index = 3,
		.def_start_x = 144,
		.def_start_y = 378,
		.def_width = 1920,
		.def_height = 1080,
		.slvs_eav_col = 2208,		/*src_width */
		.slvs_sav2sav_dist = 2376,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[8] = { //720p@120fps
		.format_index = 4,
		.def_start_x = 0,
		.def_start_y = 99,
		.def_width = 1280,
		.def_height = 720,
		.slvs_eav_col = 1280,		/*src_width */
		.slvs_sav2sav_dist = 2276,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[9] = { //2304x1296@30fps
		.format_index = 1,
		.def_start_x = 0,
		.def_start_y = 270,
		.def_width = 2304,
		.def_height = 1296,
		.slvs_eav_col = 2304,		/*src_width */
		.slvs_sav2sav_dist = 4392,	/*line_length_pck*/
		.sync_start = (0 + 5),
		.bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/*  >> add more video info here, if necessary << */
};

#define AR0835HS_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(ar0835hs_video_info_table)
#define AR0835HS_DEFAULT_VIDEO_INDEX	(0)

struct ar0835hs_video_mode ar0835hs_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_3264_2448,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2304_1836,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_3264_1836,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2208_1836,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1280_918,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_816_612,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_320_306,
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2304x1296,
	9,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	9,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	10,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	10,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define AR0835HS_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(ar0835hs_video_mode_table)
#define AR0835HS_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_3264_2448

