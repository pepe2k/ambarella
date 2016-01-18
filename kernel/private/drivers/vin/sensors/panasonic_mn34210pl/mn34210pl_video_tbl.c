/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34210pl/mn34210pl_video_tbl.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct mn34210pl_video_info mn34210pl_video_info_table[] = {
	[0] = {		// 1296x1032
		.format_index	= 0,
		.def_start_x	= 4,
		.def_start_y	= 18,
		.def_width	= 1296,
		.def_height	= 1032,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1296,
		.act_height	= 1032,
		.slvs_eav_col	= 1304,		/*src_width */
		.slvs_sav2sav_dist = 2640,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[1] = {		// 1280x720
		.format_index	= 4,
		.def_start_x	= 4,
		.def_start_y	= 18,
		.def_width	= 1280,
		.def_height	= 720,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.slvs_eav_col	= 1304,		/*src_width */
		.slvs_sav2sav_dist = 1760,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[2] = {		// 1296x3000, 2x wdr mode
		.format_index	= 1,
		.def_start_x	= 4,
		.def_start_y	= 1,
		.def_width	= 1296,
		.def_height	= 3000 - 6,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.slvs_eav_col	= 1304,		/*src_width */
		.slvs_sav2sav_dist = 2640,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[3] = {		// 1296x4500, 3x wdr mode
		.format_index	= 2,
		.def_start_x	= 4,
		.def_start_y	= 2,
		.def_width	= 1296,
		.def_height	= 4500 - 3,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.slvs_eav_col	= 1304,		/*src_width */
		.slvs_sav2sav_dist = 1760,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[4] = {		// 1296x4500, 4x wdr mode
		.format_index	= 3,
		.def_start_x	= 4,
		.def_start_y	= 3,
		.def_width	= 1296,
		.def_height	= 4500 - 8,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.slvs_eav_col	= 1304,		/*src_width */
		.slvs_sav2sav_dist = 1760,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	/* note update MN34210PL_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define MN34210PL_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(mn34210pl_video_info_table)
#define MN34210PL_DEFAULT_VIDEO_INDEX	(0)

struct mn34210pl_video_mode mn34210pl_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1296_1032,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define MN34210PL_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(mn34210pl_video_mode_table)
#define MN34210PL_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_720P

