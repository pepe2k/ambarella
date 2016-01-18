/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_reg_tbl.c
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
static const struct imx136_video_format_reg_table imx136_video_format_tbl = {
	.reg		= {
		IMX136_ADBIT,
		IMX136_MODE,
		IMX136_WINMODE,
		IMX136_FRSEL,
		IMX136_VMAX_LSB,
		IMX136_VMAX_MSB,
		IMX136_VMAX_HSB,
		IMX136_HMAX_LSB,
		IMX136_HMAX_MSB,
		IMX136_OUTCTRL,
		IMX136_INCKSEL0,
		IMX136_INCKSEL1,
		IMX136_INCKSEL2,
		IMX136_INCKSEL3,
		IMX136_INCKSEL4,
	},
	.table[0]	= {	//1920x1200
		.ext_reg_fill	= NULL,
		.data	= {
			0x01, //IMX136_ADBIT
			0x00, //IMX136_MODE
			0x00, //IMX136_WINMODE
			0x01, //IMX136_FRSEL
			0xE2, //IMX136_VMAX_LSB
			0x04, //IMX136_VMAX_MSB
			0x00, //IMX136_VMAX_HSB
			0x98, //IMX136_HMAX_LSB
			0x08, //IMX136_HMAX_MSB
			0xE1, //IMX136_OUTCTRL
			0x00, //IMX136_INCKSEL0
			0x30, //IMX136_INCKSEL1
			0x04, //IMX136_INCKSEL2
			0x30, //IMX136_INCKSEL3
			0x04, //IMX136_INCKSEL4
		},
		.width		= 1920,
		.height		= 1200,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 2,
	},
	.table[1]	= {	//1920x1080
		.ext_reg_fill	= NULL,
		.data	= {
			0x01, //IMX136_ADBIT
			0x00, //IMX136_MODE
			0x10, //IMX136_WINMODE,
			0x00, //IMX136_FRSEL
			0x65, //IMX136_VMAX_LSB
			0x04, //IMX136_VMAX_MSB
			0x00, //IMX136_VMAX_HSB
			0x4C, //IMX136_HMAX_LSB
			0x04, //IMX136_HMAX_MSB
			0xE1, //IMX136_OUTCTRL
			0x00, //IMX136_INCKSEL0
			0x30, //IMX136_INCKSEL1
			0x04, //IMX136_INCKSEL2
			0x30, //IMX136_INCKSEL3
			0x04, //IMX136_INCKSEL4
		},
		.width		= 1920,
		.height		= 1080,
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
	.table[2]	= {	//1280x720
		.ext_reg_fill	= NULL,
		.data	= {
			0x01, //IMX136_ADBIT
			0x00, //IMX136_MODE
			0x20, //IMX136_WINMODE,
			0x00, //IMX136_FRSEL
			0xEE, //IMX136_VMAX_LSB
			0x02, //IMX136_VMAX_MSB
			0x00, //IMX136_VMAX_HSB
			0x72, //IMX136_HMAX_LSB
			0x06, //IMX136_HMAX_MSB
			0xE1, //IMX136_OUTCTRL
			0x00, //IMX136_INCKSEL0
			0x30, //IMX136_INCKSEL1
			0x04, //IMX136_INCKSEL2
			0x30, //IMX136_INCKSEL3
			0x04, //IMX136_INCKSEL4
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
};


#define IMX136_GAIN_ROWS  421
#define IMX136_GAIN_42DB  0x1A4
