/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185_video_tbl.c
 *
 * History:
 *    2013/04/02 - [Ken He] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx185_video_info imx185_video_info_table[] = {
	[0] = {		//1920x1200
		.format_index	= 0,
		.fps_index	= 0,
		.def_start_x	= 4+8,
		.def_start_y	= 1+2+14+4+8,
		.def_width	= 1920,
		.def_height	= 1200,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {		//1920x1080
		.format_index	= 3,
		.fps_index	= 0,
		.def_start_x	= 4+8,
		.def_start_y	= 1+2+8+4+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {		//1280x720
		.format_index	= 2,
		.fps_index	= 0,
		.def_start_x	= 4+8,
		.def_start_y	= 1+2+4+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX185_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX185_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx185_video_info_table)
#define IMX185_DEFAULT_VIDEO_INDEX	(0)

struct imx185_video_mode imx185_video_mode_table[] = {
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
#define IMX185_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx185_video_mode_table)
#define IMX185_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

