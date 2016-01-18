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
	[0] = {					//2304X1296@60p
		.format_index	= 0,		/* select mt9t002_video_format_tbl */
		.def_start_x	= 0,		/* tell amba soc the capture x start offset */
		.def_start_y	= 0,		/* tell amba soc the capture y start offset */
		.def_width	= 2304,		/*2304	    */
		.def_height	= 1296,		/*1296      */
		.slvs_eav_col	= 2304,		/*src_width */
		.slvs_sav2sav_dist = (1248*2),	/*line_length_pck*2 */
		.sync_start	= (0+5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[1] = {					//1920x1080@60p
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,		/*1920	    */
		.def_height	= 1080,		/*1088      */
		.slvs_eav_col	= 1920,		/*src_width */
		.slvs_sav2sav_dist = (1484*2),	/*line_length_pck*/
		.sync_start	= (0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[2] = {					//1152x648@120p
		.format_index	= 4,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1152,
		.def_height	= 648,
		.slvs_eav_col	= 1152,		/*src_width */
		.slvs_sav2sav_dist = (1248*2),	/*line_length_pck*/
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[3] = {					//2048X1536@30p
		.format_index	= 3,
		.fps_index	= 0,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2048,		/*2048	   */
		.def_height	= 1536,		/*1536     */
		.slvs_eav_col	= 2048,		/*src_width */
		.slvs_sav2sav_dist = (1504*2),	/*line_length_pck*/
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[4] = {					//2304X1536@30p
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2304,		/*2304      */
		.def_height	= 1536,		/*1536      */
		.slvs_eav_col	= 2304,		/*src_width */
		.slvs_sav2sav_dist = (2114*2),	/*line_length_pck*/
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[5] = {					//768X512@30p 3x binning
		.format_index	= 5,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 768,		/*768      */
		.def_height	= 512,		/*512      */
		.slvs_eav_col	= 2304,		/*src_width */
		.slvs_sav2sav_dist = (2114*2),	/*line_length_pck*/
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
	{AMBA_VIDEO_MODE_1152x648,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2304x1536,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_768_512,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	10,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	10,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	/*  << add other video mode here if necessary >> */
};

#define MT9T002_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(mt9t002_video_mode_table)
#define MT9T002_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_2304x1296

