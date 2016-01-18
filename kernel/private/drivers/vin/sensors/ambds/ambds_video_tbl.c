/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds_video_tbl.c
 *
 * History:
 *    2014/12/30 - [Long Zhao] Create
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
static struct ambds_video_info ambds_video_info_table[] = {
	[0] = {		// 1080p
		.format_index	= 0,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= (0 + 5),
	},
	[1] = {		// 4K
		.format_index	= 1,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 3840,
		.def_height	= 2160,
		.sync_start	= (0 + 5),
	},
	/* note update AMBDS_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define AMBDS_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(ambds_video_info_table)
#define AMBDS_DEFAULT_VIDEO_INDEX	(0)

struct ambds_video_mode ambds_video_mode_table[] = {
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
	{AMBA_VIDEO_MODE_3840x2160,
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
#define AMBDS_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(ambds_video_mode_table)
#define AMBDS_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

