/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_video_tbl.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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
static const struct imx136_video_info imx136_video_info_table[] = {
	[0] = {		// 1920x1200
		.format_index	= 0,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 4+8+4+8,
		.def_width	= 1920,
		.def_height	= 1200,
		.slvs_eav_col	= 1952,		/*src_width */
		.slvs_sav2sav_dist = 2200,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	[1] = {		// 1920x1080
		.format_index	= 1,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 4+8+4+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.slvs_eav_col	= 1952,		/*src_width */
		.slvs_sav2sav_dist = 2200,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	[2] = {		// 1280x720
		.format_index	= 2,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 3+4+1+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		.slvs_eav_col	= 1312,		/*src_width */
		.slvs_sav2sav_dist = 1650,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX136_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX136_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx136_video_info_table)
#define IMX136_DEFAULT_VIDEO_INDEX	(0)

struct imx136_video_mode imx136_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_WUXGA,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
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
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX136_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx136_video_mode_table)
#define IMX136_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

