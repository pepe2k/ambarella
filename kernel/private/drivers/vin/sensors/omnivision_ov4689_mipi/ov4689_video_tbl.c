/*
 * kernel/private/drivers/ambarella/vin/sensors/omnivision_ov4689/ov4689_video_tbl.c
 *
 * History:
 *    2013/05/29 - [Long Zhao] Create
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
static const struct ov4689_video_info ov4689_video_info_table[] = {
	[0] = {		// linear mode: 4M
		.format_index	= 0,
		.def_start_x	= 0,
		.def_start_y	= 2, // for a5s mipi bug, must skip 1 line
		.def_width	= 2688,
		.def_height	= 1520,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2688,
		.act_height	= 1520,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	[1] = {		// 2x HDR mode
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2688,
		.def_height	= (1520 + 0x180) * 2, /* 1520 + MAX_MIDDLE */
		.act_start_x	= (2688 - 2560)/2,
		.act_start_y	= (1520 - 1440)/2,
		.act_width	= 2560,
		.act_height	= 1440,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	[2] = {		// 3x HDR mode
		.format_index	= 2,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2368,
		.def_height	= (1332 + 0x80 + 0x40) * 3,/* 1332 + MAX_MIDDLE + MAX_SHORT */
		.act_start_x	= (2368 - 1920) / 2,
		.act_start_y	= (1332 - 1080) / 2,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	[3] = {		// linear mode: 1080p
		.format_index	= 3,
		.def_start_x	= 0,
		.def_start_y	= 2, // for a5s mipi bug, must skip 1 line
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	[4] = {		// linear mode: 720p
		.format_index	= 4,
		.def_start_x	= 0,
		.def_start_y	= 2, // for a5s mipi bug, must skip 1 line
		.def_width	= 1280,
		.def_height	= 720,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	[5] = {		// 3x 4M HDR mode
		.format_index	= 5,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2688,
		.def_height	= (1520 + 0x1C0 + 0x40) * 3 - 21,/* 1520 + MAX_MIDDLE + MAX_SHORT */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2688,
		.act_height	= 1520,
		.sync_start	= (0 + 5),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	},
	/* note update OV4689_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define OV4689_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(ov4689_video_info_table)

/* HDR mode table */
struct ov4689_video_mode ov4689_video_mode_3x_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2688x1520,
	5,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	5,
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
#define OV4689_VIDEO_MODE_3X_TABLE_SIZE	ARRAY_SIZE(ov4689_video_mode_3x_table)

struct ov4689_video_mode ov4689_video_mode_2x_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2688x1520,
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
#define OV4689_VIDEO_MODE_2X_TABLE_SIZE	ARRAY_SIZE(ov4689_video_mode_2x_table)

/* linear mode table */
struct ov4689_video_mode ov4689_video_mode_4lane_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2688x1520,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	255,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};
#define OV4689_VIDEO_MODE_4LANE_TABLE_SIZE	ARRAY_SIZE(ov4689_video_mode_4lane_table)

struct ov4689_video_mode ov4689_video_mode_2lane_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_2688x1520,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1080P,
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	3,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_720P,
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
#define OV4689_VIDEO_MODE_2LANE_TABLE_SIZE	ARRAY_SIZE(ov4689_video_mode_2lane_table)
#define OV4689_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_2688x1520

