/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104_video_tbl.c
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
static const struct imx104_video_info imx104_video_info_table[] = {
	[0] = {		// 1280x1024
		.format_index	= 0,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+4+16+4+8,
		.def_width	= 1280,
		.def_height	= 1024,
		.slvs_eav_col	= 1312,		/*src_width */
		.slvs_sav2sav_dist = 3000,//1500,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	[1] = {		// 1280x720@60fps
		.format_index	= 1,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+4+4+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		.slvs_eav_col	= 1312,		/*src_width */
		.slvs_sav2sav_dist = 2200,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	[2] = {		// 1280x720@120fps
		.format_index	= 2,
		.fps_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+4+4+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		.slvs_eav_col	= 1312,		/*src_width */
		.slvs_sav2sav_dist = 2640,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX104_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX104_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx104_video_info_table)
#define IMX104_DEFAULT_VIDEO_INDEX	(0)

struct imx104_video_mode imx104_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_SXGA,
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
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX104_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx104_video_mode_table)
#define IMX104_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_720P

