/*
 * Filename : ov2710_video_tbl.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

/* the table is used to provide video info to application */
static const struct ov2710_video_info ov2710_video_info_table[] = {
	[0] = {
	       .format_index 	= 0,
	       .def_start_x 	= 0,
	       .def_start_y 	= 2,
	       .def_width 	= 1280,
	       .def_height 	= 720,
	       .sync_start 	= (0 + 0),
	       .bayer_pattern 	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	       },
	[1] = {
	       .format_index 	= 1,
	       .def_start_x 	= 0,
	       .def_start_y 	= 2,
	       .def_width 	= 1920,
	       .def_height 	= 1080,
	       .sync_start 	= (0 + 0),
	       .bayer_pattern 	= AMBA_VIN_SRC_BAYER_PATTERN_BG,
	       },
};

#define OV2710_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE(ov2710_video_info_table)
#define OV2710_DEFAULT_VIDEO_INDEX	(0)

struct ov2710_video_mode ov2710_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	 1, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1, /* select the index from above info table */
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_720P,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_1080P,
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define OV2710_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(ov2710_video_mode_table)
#define OV2710_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_1080P

