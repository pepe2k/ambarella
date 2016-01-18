/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104_reg_tbl.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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
static const struct imx104_video_format_reg_table imx104_video_format_tbl = {
	.reg		= {
		IMX104_ADBIT,
		IMX104_MODE,
		IMX104_WINMODE,
		IMX104_FRSEL,
		IMX104_VMAX_LSB,
		IMX104_VMAX_MSB,
		IMX104_VMAX_HSB,
		IMX104_HMAX_LSB,
		IMX104_HMAX_MSB,
		IMX104_OUTCTRL,
		IMX104_INCKSEL1,
		IMX104_INCKSEL2,
		IMX104_INCKSEL3,
		IMX104_BLKLEVEL_LSB,
		IMX104_BLKLEVEL_MSB
	},
	.table[0]	= {	//1280x1024
		.ext_reg_fill	= NULL,
		.data	= {
			0x01, //IMX104_ADBIT
			0x00, //IMX104_MODE
			0x00, //IMX104_WINMODE
			0x00, //IMX104_FRSEL,FIXME: in TD, should be 0x01
			0x4C, //IMX104_VMAX_LSB
			0x04, //IMX104_VMAX_MSB
			0x00, //IMX104_VMAX_HSB
			0xCA, //IMX104_HMAX_LSB
			0x08, //IMX104_HMAX_MSB
			0xE1, //IMX104_OUTCTRL
			0x00, //IMX104_INCKSEL1
			0x00, //IMX104_INCKSEL2
			0x00, //IMX104_INCKSEL3
			0xF0, //IMX104_BLKLEVEL_LSB
			0x00, //IMX104_BLKLEVEL_MSB
		},
		.width		= 1280,
		.height		= 1024,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
	.table[1]	= {	//1280x720@60fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x01, //IMX104_ADBIT
			0x00, //IMX104_MODE
			0x10, //IMX104_WINMODE
			0x01, //IMX104_FRSEL
			0xEE, //IMX104_VMAX_LSB
			0x02, //IMX104_VMAX_MSB
			0x00, //IMX104_VMAX_HSB
			0xE4, //IMX104_HMAX_LSB
			0x0C, //IMX104_HMAX_MSB
			0xE1, //IMX104_OUTCTRL
			0x00, //IMX104_INCKSEL1
			0x00, //IMX104_INCKSEL2
			0x00, //IMX104_INCKSEL3
			0xF0, //IMX104_BLKLEVEL_LSB
			0x00, //IMX104_BLKLEVEL_MSB
		},
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
	.table[2]	= {	//1280x720@120fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x00, //IMX104_ADBIT
			0x00, //IMX104_MODE
			0x10, //IMX104_WINMODE
			0x00, //IMX104_FRSEL
			0xEE, //IMX104_VMAX_LSB
			0x02, //IMX104_VMAX_MSB
			0x00, //IMX104_VMAX_HSB
			0x72, //IMX104_HMAX_LSB
			0x06, //IMX104_HMAX_MSB
			0xE0, //IMX104_OUTCTRL
			0x00, //IMX104_INCKSEL1
			0x00, //IMX104_INCKSEL2
			0x00, //IMX104_INCKSEL3
			0x3C, //IMX104_BLKLEVEL_LSB
			0x00, //IMX104_BLKLEVEL_MSB
		},
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
};

#define IMX104_GAIN_ROWS  141
#define IMX104_WDR_GAIN_ROWS 41
#define IMX104_GAIN_42DB  0x8C
#define IMX104_GAIN_12DB  0x28 // For WDR mode

