/*
 * Filename : ov5658_video_tbl.c
 *
 * History:
 *    2013/08/28 - [Johnson Diao] Create
 *
 * Copyright (C) 2013-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

/* the table is used to provide video info to application */
static const struct ov5658_video_info ov5658_video_info_table[] = {
	[0] = {
		.format_index	= 0,	/* select ov5658_video_format_tbl */
		.def_start_x	= 0,	/* tell amba soc the capture x start offset */
		.def_start_y	= 2,	/* tell amba soc the capture y start offset */
		.def_width	= 2592,
		.def_height	= 1944,
		.sync_start	= 0,
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
		},
	[1] = {
		.format_index	= 0,	/* select ov5658_video_format_tbl */
		.def_start_x	= 336,	/* tell amba soc the capture x start offset */
		.def_start_y	= 432,	/* tell amba soc the capture y start offset */
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= 0,
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
		},
	[2] = {// 720p 60fs
		.format_index	= 1,	/* select ov5658_video_format_tbl */
		.def_start_x	= 0,	/* tell amba soc the capture x start offset */
		.def_start_y	= 2,	/* tell amba soc the capture y start offset */
		.def_width	= 1280,
		.def_height	= 720,
		.sync_start	= 0,
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
		},

	/*  >> add more video info here, if necessary << */

};

#define OV5658_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(ov5658_video_info_table)
#define OV5658_DEFAULT_VIDEO_INDEX	(0)

struct ov5658_video_mode ov5658_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0, /* select the index from above info table */
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0, /* select the index from above info table */
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
/******************************************************************/
	{AMBA_VIDEO_MODE_QSXGA, // 2592x1944
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
/******************************************************************/
	{AMBA_VIDEO_MODE_1080P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
/******************************************************************/
	{AMBA_VIDEO_MODE_720P,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

/******************************************************************/
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)}

	/*  << add other video mode here if necessary >> */
};

#define OV5658_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(ov5658_video_mode_table)
#define OV5658_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_QSXGA

