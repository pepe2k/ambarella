/*
 * Filename : ar0331_video_tbl.c
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
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
static const struct ar0331_video_info ar0331_video_info_table[] = {
	[0] = {					//2048x1536 
		.format_index	= 0,		/* select ar0331_video_format_tbl */
		.def_start_x	= 0,		/* tell amba soc the capture x start offset */
		.def_start_y	= 0,		/* tell amba soc the capture y start offset */
		.def_width	= 2048,		/*2048	    */
		.def_height	= 1540,		/*1540      */
		.slvs_eav_col	= 2048,		/*src_width */
		.slvs_sav2sav_dist = (1120*2),	/*line_length_pck*/
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[1] = {					//1920x1080
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,		/*1920	    */
		.def_height	= 1084,		/*1084      */
		.slvs_eav_col	= 1920,		/*src_width */
		.slvs_sav2sav_dist = (1100*2*2),	/*line_length_pck*/
		.sync_start	= (0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	[2] = {					//1280x720
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 724,
		.slvs_eav_col	= 1280,		/*src_width */
		.slvs_sav2sav_dist = (1634*2),	/*line_length_pck*/
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	/*  >> add more video info here, if necessary << */

};

#define AR0331_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE(ar0331_video_info_table)
#define AR0331_DEFAULT_VIDEO_INDEX	(0)

struct ar0331_video_mode ar0331_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	 1, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	

	/*  << add other video mode here if necessary >> */
};

#define AR0331_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(ar0331_video_mode_table)
#define AR0331_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

