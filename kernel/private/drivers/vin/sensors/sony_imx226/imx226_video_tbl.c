/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx226/imx226_video_tbl.c
 *
 * History:
 *    2013/12/20 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx226_video_info imx226_video_info_table[] = {
	[0] = { //4096x2160p59.94 (mode 4)
		.format_index	= 4,
		.def_start_x	= 124,
		.def_start_y	= 18,
		.def_width	= 4096,
		.def_height	= 2160,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = { //2048x1080@60 2x2 binning (mode 5)
		.format_index	= 5,
		.def_start_x	= 62,
		.def_start_y	= 14,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = { //4000x3000p34.97 (mode 0)
		.format_index	= 0,
		.def_start_x	= 132,
		.def_start_y	= 40,
		.def_width	= 4000,
		.def_height	= 3000,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = { //3840x2160p30 (mode 4)
		.format_index	= 4,
		.def_start_x	= 124 + (4096 - 3840)/2,
		.def_start_y	= 18,
		.def_width	= 3840,
		.def_height	= 2160,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[4] = { //4096x2160p29.97 (mode 6)
		.format_index	= 6,
		.def_start_x	= 124,
		.def_start_y	= 18,
		.def_width	= 4096,
		.def_height	= 2160,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX226_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX226_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx226_video_info_table)
#define IMX226_DEFAULT_VIDEO_INDEX	(0)

struct imx226_video_mode imx226_video_mode_table[] = {
	/* auto mode */
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* 1080p  */
	{AMBA_VIDEO_MODE_1080P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* 4kx2k */
	{AMBA_VIDEO_MODE_4096x2160,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* 4000x3000 */
	{AMBA_VIDEO_MODE_4000x3000,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* 3840x2160 */
	{AMBA_VIDEO_MODE_3840x2160,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* off mode */
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define IMX226_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx226_video_mode_table)
#define IMX226_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_4096x2160

