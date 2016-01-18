/*
 * Filename : mn34041pl_video_tbl.c
 *
 * History:
 *    2011/03/08 - [Haowei Lo] Create
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
static const struct mn34041pl_video_info mn34041pl_video_info_table[] = {
	[0] = {							//1920x1080 60p
		.format_index	= 0,				/* select mn34041pl_video_format_tbl */
		.def_start_x	= 64,				/* tell amba soc the capture x start offset */
		.def_start_y	= 8,				/* tell amba soc the capture y start offset */
		.def_width	= 1920,				/*1920	    */
		.def_height	= 1080,				/*1080      */
		.slvs_eav_col	= 2016,				/*src_width */

		/*line_length_pck 3600 serial clk * 2 DDR * 4 lanes / 12 bits data width */
		.slvs_sav2sav_dist = (3600 * 2 * 4 / 12),
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
		},
	[1] = {							//1920x1080 120p
		.format_index	= 1,				/* select mn34041pl_video_format_tbl */
		.def_start_x	= 64,				/* tell amba soc the capture x start offset */
		.def_start_y	= 8,				/* tell amba soc the capture y start offset */
		.def_width	= 1920,				/*1920	    */
		.def_height	= 1080,				/*1080      */
		.slvs_eav_col	= 2016,				/*src_width */

		/*line_length_pck 1800 serial clk * 2 DDR * 6 lanes / 10 bits data width */
		.slvs_sav2sav_dist = 2160,//(1800 * 2 * 6 / 10),
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
		},
	/*  >> add more video info here, if necessary << */
};

#define MN34041PL_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(mn34041pl_video_info_table)
#define MN34041PL_DEFAULT_VIDEO_INDEX	(0)

struct mn34041pl_video_mode mn34041pl_video_mode_table[] = {
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
	{AMBA_VIDEO_MODE_OFF,
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/*  << add other video mode here if necessary >> */
};

#define MN34041PL_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(mn34041pl_video_mode_table)
#define MN34041PL_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P