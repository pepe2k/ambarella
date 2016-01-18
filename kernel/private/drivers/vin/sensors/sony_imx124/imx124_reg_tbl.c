/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_reg_tbl.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
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
static const struct imx124_video_format_reg_table imx124_video_format_tbl = {
	.reg = {
		IMX124_WINMODE,
		IMX124_FRSEL,
		IMX124_VMAX_LSB,
		IMX124_VMAX_MSB,
		IMX124_VMAX_HSB,
		IMX124_HMAX_LSB,
		IMX124_HMAX_MSB,
	},
	.table[0] = {		// 4ch 3M linear
		.ext_reg_fill = NULL,
		.data = {
			0x08, //WINMODE
			0x01, //FRSEL
			0x40, //VMAX_LSB
			0x06, //VMAX_MSB
			0x00, //VMAX_HSB
			0xEE, //HMAX_LSB
			0x02, //HMAX_MSB
		},
		.width = 2048,
		.height = 1536,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
	},
	.table[1] = {		// 4ch 1080p linear
		.ext_reg_fill = NULL,
		.data = {
			0x18, //WINMODE
			0x01, //FRSEL
			0x65, //VMAX_LSB
			0x04, //VMAX_MSB
			0x00, //VMAX_HSB
			0x4C, //HMAX_LSB
			0x04, //HMAX_MSB
		},
		.width = 1920,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	/* add video format table here, if necessary */
};


/* Gain table */
#define IMX124_GAIN_MAX_DB  0x1E0 //low light: 48DB
#define IMX124_GAIN_ROWS 481
