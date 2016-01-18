/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx183/imx183_video_tbl.c
 *
 * History:
 *    2014/08/13 - [Long Zhao] Create
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
static const struct imx183_video_info imx183_video_info_table[] = {
	[0] = {
		.format_index	= 0,
		.def_start_x	= 24+48+24+24+12,
		.def_start_y	= 16+12+12,
		.def_width	= 5472,
		.def_height	= 3648,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {
		.format_index	= 1,
		.def_start_x	= 12+24+12+12+6,
		.def_start_y	= 4+4+6,
		.def_width	= 2736,
		.def_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {
		.format_index	= 2,
		.def_start_x	= 12+24+12+12+6,
		.def_start_y	= 4+4+6,
		.def_width	= 2736,
		.def_height	= 1824,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = {
		.format_index	= 3,
		.def_start_x	= 8+14+10+8+4,
		.def_start_y	= 4+4+6,
		.def_width	= 1824,
		.def_height	= 1216,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[4] = {
		.format_index	= 4,
		.def_start_x	= 24+48+24+12+16,
		.def_start_y	= 8+4+4,
		.def_width	= 4096,
		.def_height	= 2160,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	/* note update IMX183_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX183_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx183_video_info_table)
#define IMX183_DEFAULT_VIDEO_INDEX	(0)

struct imx183_video_mode imx183_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_5472_3648,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2736_1536,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2736_1824,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1824_1216,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_4096x2160,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	/* off mode */
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define IMX183_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx183_video_mode_table)
#define IMX183_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_5472_3648

