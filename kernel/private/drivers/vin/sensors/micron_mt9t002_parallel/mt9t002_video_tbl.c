/*
 * Filename : mt9t002_video_tbl.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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
static const struct mt9t002_video_info mt9t002_video_info_table[] = {
	[0] = {					//2304X1296@30p
		.format_index	= 0,		/* select mt9t002_video_format_tbl */
		.def_start_x	= 0,		/* tell amba soc the capture x start offset */
		.def_start_y	= 0,		/* tell amba soc the capture y start offset */
		.def_width	= 2304,		/*2304	    */
		.def_height	= 1296,		/*1296      */
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[1] = {					//1920x1080@30p
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[2] = {					//2048x1536@30p
		.format_index	= 2,
		.def_start_x	= (2304 - 2048)/2,
		.def_start_y	= 0,
		.def_width	= 2048,
		.def_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	/*  >> add more video info here, if necessary << */
};

#define MT9T002_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE(mt9t002_video_info_table)
#define MT9T002_DEFAULT_VIDEO_INDEX	(0)

struct mt9t002_video_mode mt9t002_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2304x1296,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/*  << add other video mode here if necessary >> */
};

#define MT9T002_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(mt9t002_video_mode_table)
#define MT9T002_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_2304x1296

