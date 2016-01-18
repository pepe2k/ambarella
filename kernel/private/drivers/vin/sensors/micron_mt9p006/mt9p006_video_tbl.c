/*
 * Filename : mt9p006_video_tbl.c
 *
 * History:
 *    2011/05/23 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

/* the table is used to provide video info to application */
static const struct mt9p006_video_info mt9p006_video_info_table[] = {
	[0] = {					//1920x1080p29.97
		.format_index	= 0,		/* select mt9p006_video_format_tbl */
		.def_start_x	= 0,		/* tell amba soc the capture x start offset */
		.def_start_y	= 0,		/* tell amba soc the capture y start offset */
		.def_width	= 1920,		/*1920	    */
		.def_height	= 1080,		/*1080      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[1] = {					//1280x720p59.94
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,		/*1280	    */
		.def_height	= 720,		/*720      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[2] = {					//2560x1440p15
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2560,		/*2560	    */
		.def_height	= 1440,		/*1440      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[3] = {					//2592x1944p12
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2592,		/*2592	    */
		.def_height	= 1944,		/*1944      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[4] = {					//2048x1536p15
		.format_index	= 4,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2048,		/*2048	    */
		.def_height	= 1536,		/*1536      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	/*  >> add more video info here, if necessary << */

};

#define MT9P006_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE(mt9p006_video_info_table)
#define MT9P006_DEFAULT_VIDEO_INDEX	(0)

struct mt9p006_video_mode mt9p006_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	 0, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2560x1440,
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QSXGA,
	 3,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 3,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	 4,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 4,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	/*  << add other video mode here if necessary >> */
};

#define MT9P006_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(mt9p006_video_mode_table)
#define MT9P006_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

