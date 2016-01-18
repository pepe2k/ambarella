/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/imx122_video_tbl.c
 *
 * History:
 *    2011/09/23 - [Bingliang Hu] Create
 *    2012/06/29 - [Long Zhao] Add 720p60 mode
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
static const struct imx122_video_info imx122_video_info_table[] = {
	[0] = {
		.format_index	= 0,
		.def_start_x	= (4 + 4 + 36 +8 +8+16) + 64 ,	// + (2048-1920)/2,
		.def_start_y	= (12 + 1 + 4 + 8 +2 +6),	// + (1080-1080)/2,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[1] = {
		.format_index	= 1,
		.def_start_x	= (4 + 4 + 36 +8 +8+16) + 64 ,
		.def_start_y	= 6+1+4+6+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	/* note update IMX122_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX122_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx122_video_info_table)
#define IMX122_DEFAULT_VIDEO_INDEX	(0)

struct imx122_video_mode imx122_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
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
#define IMX122_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx122_video_mode_table)
#define IMX122_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

