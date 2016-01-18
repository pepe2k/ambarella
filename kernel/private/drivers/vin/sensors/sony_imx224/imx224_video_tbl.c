/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224_video_tbl.c
 *
 * History:
 *    2014/07/08 - [Long Zhao] Create
 *
 * Copyright (C) 2012-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx224_video_info imx224_video_info_table[] = {
	[0] = {		/* 1280x960 */
		.format_index	= 0,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+14+8,
		.def_width	= 1280,
		.def_height	= 960,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {		/* 1280x720 */
		.format_index	= 1,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+4+2,
		.def_width	= 1280,
		.def_height	= 720,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {		/* 640x480 */
		.format_index	= 2,
		.def_start_x	= 2+2+4,
		.def_start_y	= 1+2+6+4,
		.def_width	= 640,
		.def_height	= 480,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 640,
		.act_height	= 480,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = {		/* 2x DOL QuadVGA */
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX224_H_PIXEL * 2 + IMX224_HBLANK,
#ifdef USE_960P_2X_30FPS
		.def_height	= IMX224_QVGA_BRL + (0x451 - 1)/2, /* BRL + VBP1 */
#else
		.def_height	= IMX224_QVGA_BRL + (0xC1 - 1)/2, /* BRL + VBP1 */
#endif
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	[4] = {		/* 3x DOL QuadVGA */
		.format_index	= 4,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= IMX224_H_PIXEL * 3 + IMX224_HBLANK * 2,
		.def_height	= IMX224_QVGA_BRL + (0x569 - 2)/3, /* BRL + VBP2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GB,
	},
	/* note update IMX224_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX224_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx224_video_info_table)

struct imx224_video_mode imx224_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1280_960,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_VGA,
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
#define IMX224_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx224_video_mode_table)

struct imx224_video_mode imx224_video_mode_2xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1280_960,
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
#define IMX224_VIDEO_MODE_2XDOL_TABLE_SIZE		ARRAY_SIZE(imx224_video_mode_2xdol_table)

struct imx224_video_mode imx224_video_mode_3xdol_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1280_960,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX224_VIDEO_MODE_3XDOL_TABLE_SIZE		ARRAY_SIZE(imx224_video_mode_3xdol_table)

#define IMX224_VIDEO_MODE_TABLE_AUTO		AMBA_VIDEO_MODE_1280_960