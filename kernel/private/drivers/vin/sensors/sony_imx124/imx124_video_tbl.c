/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_video_tbl.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static struct imx124_video_info imx124_video_info_table[] = {
	[0] = {		// 3M
		.format_index	= 0,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 2048,
		.def_height	= 1536,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.slvs_eav_col	= 2064,
		.slvs_sav2sav_dist = 2250,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {		// 1080p
		.format_index	= 1,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.slvs_eav_col	= 1952,
		.slvs_sav2sav_dist = 2250,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX124_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX124_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx124_video_info_table)
#define IMX124_DEFAULT_VIDEO_INDEX	(0)

struct imx124_video_mode imx124_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX124_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx124_video_mode_table)
#define IMX124_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_QXGA

