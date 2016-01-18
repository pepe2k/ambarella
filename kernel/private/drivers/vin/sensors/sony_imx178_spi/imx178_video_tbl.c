/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_video_tbl.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
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
static const struct imx178_video_info imx178_video_info_table[] = {
	[0] = {		// 6.3M: 3072x2048(14bits)@30fps
		.format_index	= 0,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 3072,
		.def_height	= 2048,
		.slvs_eav_col	= 3100,
		.slvs_sav2sav_dist = 4800,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[1] = {		// 5.0M: 2592x1944(14bits)@30fps
		.format_index	= 1,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 2592,
		.def_height	= 1944,
		.slvs_eav_col	= 2620,
		.slvs_sav2sav_dist = 4800,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[2] = {		// 2560x2048(14bits)@30fps
		.format_index	= 2,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 2560,
		.def_height	= 2048,
		.slvs_eav_col	= 2588,
		.slvs_sav2sav_dist = 4800,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[3] = {		// 1.6M: 1536x1024(14bits)@30fps 2x2 binning
		.format_index	= 3,
		.def_start_x	= 2+4,
		.def_start_y	= 1+6+4+4+4,
		.def_width	= 1536,
		.def_height	= 1024,
		.slvs_eav_col	= 1550,
		.slvs_sav2sav_dist = 3343,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[4] = {		// 1080P@120fps
		.format_index	= 4,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 1920,
		.def_height	= 1080,
		.slvs_eav_col	= 1950,
		.slvs_sav2sav_dist = 3667,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},
	[5] = {		// 720P@120fps 2x2 binning
		.format_index	= 5,
		.def_start_x	= 4+8,
		.def_start_y	= 1+4+4+6+4,
		.def_width	= 1280,
		.def_height	= 720,
		.slvs_eav_col	= 1310,
		.slvs_sav2sav_dist = 5000,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
	},

	/* note update IMX178_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define IMX178_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx178_video_info_table)
#define IMX178_DEFAULT_VIDEO_INDEX	(0)

struct imx178_video_mode imx178_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_3072_2048,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_QSXGA,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2560_2048,
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	2,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1536_1024,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	7,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define IMX178_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx178_video_mode_table)
#define IMX178_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_3072_2048

