/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx121/imx121_video_tbl.c
 *
 * History:
 *    2011/11/25 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx121_video_info imx121_video_info_table[] = {
	[0] = { //4096x2160p29.97 (mode 2)
		.format_index	= 1,
		.def_start_x	= 108,
		.def_start_y	= 16,
		.def_width	= 4096,
		.def_height	= 2160,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = { //1920x1080p@60 2x2 binning(mode 4)
		.format_index	= 2,
		.def_start_x	= 54 + (2048 - 1920)/2,
		.def_start_y	= 8,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = { //4000x3000p20 (mode 6)
		.format_index	= 4,
		.def_start_x	= 108,
		.def_start_y	= 16,
		.def_width	= 4000,
		.def_height	= 3000,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = { //3840x2160p29.97 (mode 2)
		.format_index	= 1,
		.def_start_x	= 108 + (4096 - 3840)/2,
		.def_start_y	= 16,
		.def_width	= 3840,
		.def_height	= 2160,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX121_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX121_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx121_video_info_table)
#define IMX121_DEFAULT_VIDEO_INDEX	(0)

struct imx121_video_mode imx121_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_4096x2160,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_4000x3000,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_3840x2160,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define IMX121_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx121_video_mode_table)
#define IMX121_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_4096x2160

