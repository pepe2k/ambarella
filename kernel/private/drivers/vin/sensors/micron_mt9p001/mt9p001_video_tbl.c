/*
 * kernel/private/drivers/ambarella/vin/sensors/micron_mt9p001/mt9p001_video_tbl.c
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#ifndef CONFIG_SENSOR_MT9PXXX_IO_1_8
static const struct mt9p001_video_info mt9p001_video_info_table[] = {
	[0] = {			//1280x720P29.97 or 1280x720P30, default
	       .format_index = 6,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1280,
	       .def_height = 720,
	       .sync_start = (0 + 5),
	       },
	[1] = {			//1280x720P60
	       .format_index = 10,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1280,
	       .def_height = 720,
	       .sync_start = (0 + 5),
	       },

	[2] = {			//This is 4:3 full array readout mode 1: 2752x2004 Manual BLC
	       .format_index = 0,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 2752,
	       .def_height = 2004,
	       .sync_start = (0 + 5),
	       },

	[3] = {			//This is 4:3 full array readout mode 2: 2752x1992 Auto BLC
	       .format_index = 1,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 2752,
	       .def_height = 1992,
	       .sync_start = (0 + 5),
	       },

	[4] = {			//This is 4:3 5M still picture
	       .format_index = 2,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 2592,
	       .def_height = 1944,
	       .sync_start = (0 + 5),
	       },

	[5] = {			//This is 16:9 3.68M still picture
	       .format_index = 3,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 2560,
	       .def_height = 1440,
	       .sync_start = (0 + 5),
	       },

	[6] = {			//1920x1080P30
	       .format_index = 4,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1920,
	       .def_height = 1080,
	       .sync_start = (0 + 5),
	       },

	[7] = {			//640x480P120
	       .format_index = 7,	//7
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 640,
	       .def_height = 480,
	       .sync_start = (0 + 5),
	       },

	[8] = {			//1280x960P30
	       .format_index = 9,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1280,
	       .def_height = 960,
	       .sync_start = (0 + 5),
	       },

	[9] = {			//This is 4:3 3M still picture
	       .format_index = 11,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 2048,
	       .def_height = 1536,
	       .sync_start = (0 + 5),
	       },

	[10] = {		//1440x1080P30
		.format_index = 4,
		.fps_index = 0,
		.def_start_x = 240,
		.def_start_y = 0,
		.def_width = 1440,
		.def_height = 1080,
		.sync_start = (0 + 5),
		},

	[11] = {		//1024x768
		.format_index = 14,
		.fps_index = 0,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1024,
		.def_height = 768,
		.sync_start = (0 + 5),
		},

	[12] = {		//720x480
		.format_index = 14,
		.fps_index = 0,
		.def_start_x = 152,
		.def_start_y = 144,
		.def_width = 720,
		.def_height = 480,
		.sync_start = (144 + 5),
		},

	[13] = {		//720x576
		.format_index = 14,
		.fps_index = 2,
		.def_start_x = 152,
		.def_start_y = 96,
		.def_width = 720,
		.def_height = 576,
		.sync_start = (96 + 5),
		},

	[14] = {		//800x480
		.format_index = 6,
		.fps_index = 0,
		.def_start_x = 240,
		.def_start_y = 120,
		.def_width = 800,
		.def_height = 480,
		.sync_start = (120 + 5),
		},

	[15] = {		//1024x600
		.format_index = 14,
		.fps_index = 0,
		.def_start_x = 128,
		.def_start_y = 60,
		.def_width = 1024,
		.def_height = 600,
		.sync_start = (60 + 5),
		},
	[16] = {		//1280x1024 qiao_ls
		.format_index = 16,
		.fps_index = 0,
		.def_start_x = 128,
		.def_start_y = 60,
		.def_width = 1280,
		.def_height = 1024,
		.sync_start = (60 + 5),
		},
	[17] = {			//320x240 120fps
	       .format_index = 18,	//12,   //7
	       .fps_index = 0,
	       .def_start_x = 0,	//0
	       .def_start_y = 0,	//0
	       .def_width = 320,
	       .def_height = 240,
	       .sync_start = (0 + 5),	//(0 + 5)
	       },
};

struct mt9p001_video_mode mt9p001_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_320_240,
	 17,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 17,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_VGA,
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_D1_NTSC,
	 12,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 12,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_D1_PAL,
	 13,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 13,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_WVGA,
	 14,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 14,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_WSVGA,
	 15,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 15,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_XGA,
	 11,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 11,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_720P,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_1080P,
	 6,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 6,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_QXGA,
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 9,
	 AMBA_VIN_SRC_ENABLED_FOR_STILL},

	{AMBA_VIDEO_MODE_3M_16_9,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 5,
	 AMBA_VIN_SRC_ENABLED_FOR_STILL},

	{AMBA_VIDEO_MODE_5M_4_3,
	 7,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 4,
	 AMBA_VIN_SRC_ENABLED_FOR_STILL},

	{AMBA_VIDEO_MODE_1280_960,
	 8,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 8,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_SXGA,	//qiao_ls
	 16,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 16,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#else				//CONFIG_SENSOR_MT9PXXX_IO_1_8
static const struct mt9p001_video_info mt9p001_video_info_table[] = {
	[0] = {			//1280x720P29.97 or 1280x720P30, default
	       .format_index = 5,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1280,
	       .def_height = 720,
	       .sync_start = (0 + 5),
	       },

	[1] = {			//640x480P30
	       .format_index = 7,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 640,
	       .def_height = 480,
	       .sync_start = (0 + 5),
	       },

	[2] = {			//1024x768
	       .format_index = 13,
	       .fps_index = 0,
	       .def_start_x = 0,
	       .def_start_y = 0,
	       .def_width = 1024,
	       .def_height = 768,
	       .sync_start = (0 + 5),
	       },
};

struct mt9p001_video_mode mt9p001_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_VGA,
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 1,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_XGA,
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 2,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_720P,
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	 0,
	 (AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#endif				//CONFIG_SENSOR_MT9PXXX_IO_1_8

#define MT9P001_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE(mt9p001_video_info_table)
#define MT9P001_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE(mt9p001_video_mode_table)
#define MT9P001_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_720P
