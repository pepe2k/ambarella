/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx290/imx290_video_tbl.c
 *
 * History:
 *    2015/05/18 - [Hao Zeng] Create
 *
 * Copyright (C) 2004-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static struct imx290_video_info imx290_video_info_table[] = {
	[0] = {	/* 1080p120 10bits */
		.format_index	= 0,
		.def_start_x	= 4+8,
		.def_start_y	= 1+2+10+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,

		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {	/* 720p120 10bits */
		.format_index	= 1,
		.def_start_x	= 4+8,
		.def_start_y	= 1+2+4+4,
		.def_width	= 1280,
		.def_height	= 720,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,

		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {	/* 1080p 2x wdr */
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX290_1080P_H_PIXEL * 2 + 0x2b4,
		.def_height	= IMX290_1080P_BRL + (IMX290_1080P_2X_RHS1 - 1) / 2 + 1, /* BRL + VBP1 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[3] = {	/* 1080p 3x wdr */
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX290_1080P_H_PIXEL * 3 + 0x2b4 * 2,
		.def_height	= IMX290_1080P_BRL + (IMX290_1080P_3X_RHS2 - 2) / 3 - 1, /* BRL + VBP2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	/* note update IMX290_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX290_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx290_video_info_table)

struct imx290_video_mode imx290_video_mode_table[] = {
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
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX290_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx290_video_mode_table)
#define IMX290_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

struct imx290_video_mode imx290_video_mode_2xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX290_VIDEO_MODE_2XDOL_TABLE_SIZE	ARRAY_SIZE(imx290_video_mode_2xdol_table)

struct imx290_video_mode imx290_video_mode_3xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX290_VIDEO_MODE_3XDOL_TABLE_SIZE	ARRAY_SIZE(imx290_video_mode_3xdol_table)

