/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_video_tbl.c
 *
 * History:
 *    2013/12/27 - [Long Zhao] Create
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
static struct imx123_video_info imx123_video_info_table[] = {
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
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {		// 3M 2x wdr
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX123_QXGA_H_PIXEL * 2 + 0x90,
#ifdef USE_3M_2X_25FPS
		.def_height	= IMX123_QXGA_BRL + (0x2BA - 2) / 2, // BRL + VBP1
#else
		.def_height	= IMX123_QXGA_BRL + (0x3A - 2) / 2, // BRL + VBP1
#endif
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[3] = {		// 1080p 2x wdr
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX123_1080P_H_PIXEL * 2 + 0xA0,
		.def_height	= IMX123_1080P_BRL + (0x2BA - 2) / 2, // BRL + VBP1
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[4] = {		// 3M 3x wdr
		.format_index	= 4,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX123_QXGA_H_PIXEL * 3 + 0xD0 * 2,
		.def_height	= IMX123_QXGA_BRL + (0x31C - 4) / 3, // BRL + VBP2
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[5] = {		// 1080p 3x wdr
		.format_index	= 5,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX123_1080P_H_PIXEL * 3 + 0xCB + 0xCD,
		.def_height	= IMX123_1080P_BRL + (0x31C - 4)/3, // BRL + VBP2
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[6] = {		// 3M 120fps
		.format_index	= 6,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 2048,
		.def_height	= 1536,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[7] = {		// 1080p 120fps
		.format_index	= 7,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[8] = {		// 3M-dual gain
		.format_index	= 8,
		.def_start_x	= 8,
		.def_start_y	= 1+8+16+16,
		.def_width	= 2048,
		.def_height	= 1536*2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[9] = {		// 1080p-dual gain
		.format_index	= 9,
		.def_start_x	= 8,
		.def_start_y	= 1+8+16+16,
		.def_width	= 1920,
		.def_height	= 1080*2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	/* note update IMX123_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX123_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx123_video_info_table)

struct imx123_video_mode imx123_video_mode_120fps_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	6,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX123_VIDEO_MODE_120FPS_TABLE_SIZE	ARRAY_SIZE(imx123_video_mode_120fps_table)

struct imx123_video_mode imx123_video_mode_dualgain_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	8,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	9,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	9,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX123_VIDEO_MODE_DUALGAIN_TABLE_SIZE	ARRAY_SIZE(imx123_video_mode_dualgain_table)

struct imx123_video_mode imx123_video_mode_2xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
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
#define IMX123_VIDEO_MODE_2XDOL_TABLE_SIZE	ARRAY_SIZE(imx123_video_mode_2xdol_table)

struct imx123_video_mode imx123_video_mode_3xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QXGA,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX123_VIDEO_MODE_3XDOL_TABLE_SIZE	ARRAY_SIZE(imx123_video_mode_3xdol_table)

struct imx123_video_mode imx123_video_mode_table[] = {
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
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX123_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx123_video_mode_table)
#define IMX123_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_QXGA

