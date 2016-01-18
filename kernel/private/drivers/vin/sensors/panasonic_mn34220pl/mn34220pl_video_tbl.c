/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34220pl/mn34220pl_video_tbl.c
 *
 * History:
 *    2013/06/08 - [Long Zhao] Create
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
static const struct mn34220pl_video_info mn34220pl_video_info_table[] = {
	[0] = {		// 1920x1200
		.format_index	= 0,
		.def_start_x	= 4,
		.def_start_y	= 2+10+4,
		.def_width	= 1920,
		.def_height	= 1212,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1200,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[1] = {		// 1920x1080
		.format_index	= 0,
		.def_start_x	= 4,
		.def_start_y	= 2+10+4,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[2] = {		// 1920x2500, 2x wdr mode
		.format_index	= 1,
		.def_start_x	= 4,
		.def_start_y	= 1,
		.def_width	= 1920,
		.def_height	= 3000 - 20,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[3] = {		// 1920x4500, 3x wdr mode
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 2,
		.def_width	= 1920,
		.def_height	= 4500 - 30,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[4] = {		// 1920x4500, 4x wdr mode
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 3,
		.def_width	= 1920,
		.def_height	= 4500 - 40,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	[5] = {		// 1080p120
		.format_index	= 4,
		.def_start_x	= 4,
		.def_start_y	= 2+10+4,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
	},
	/* note update MN34220PL_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define MN34220PL_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_info_table)

struct mn34220pl_video_mode mn34220pl_video_mode_120fps_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
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
#define MN34220PL_VIDEO_MODE_120FPS_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_mode_120fps_table)

struct mn34220pl_video_mode mn34220pl_video_mode_2x_table[] = {
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
#define MN34220PL_VIDEO_MODE_2X_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_mode_2x_table)

struct mn34220pl_video_mode mn34220pl_video_mode_3x_table[] = {
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
#define MN34220PL_VIDEO_MODE_3X_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_mode_3x_table)

struct mn34220pl_video_mode mn34220pl_video_mode_4x_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	4,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define MN34220PL_VIDEO_MODE_4X_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_mode_4x_table)

struct mn34220pl_video_mode mn34220pl_video_mode_table[] = {
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
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define MN34220PL_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(mn34220pl_video_mode_table)
#define MN34220PL_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

