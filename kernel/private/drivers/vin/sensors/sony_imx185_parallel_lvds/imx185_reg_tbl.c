/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185_reg_tbl.c
 *
 * History:
 *    2013/04/02 - [Ken He] Create
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
static const struct imx185_video_format_reg_table imx185_video_format_tbl = {
	.reg = {
		IMX185_ADBIT,
		IMX185_MODE,
		IMX185_WINMODE,
		IMX185_FRSEL,
		IMX185_VMAX_LSB,
		IMX185_VMAX_MSB,
		IMX185_VMAX_HSB,
		IMX185_HMAX_LSB,
		IMX185_HMAX_MSB,
		IMX185_ODBIT,
	},
	.table[0] = {		//1920x1200(12bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x01, //ADBIT
			0x00, //MODE
			0x00, //WINMODE WUXGA MODE
			0x01, //FRSEL
			0x28, //VMAX_LSB
			0x05, //VMAX_MSB
			0x00, //VMAX_HSB
			0xA6, //HMAX_LSB
			0x0E, //HMAX_MSB
			0x61, //ODBIT_OPORTSEL, 12bits
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
		.pll_index	= 0,
		.h_pixel = 3750,
		.data_rate = PLL_CLK_148_5MHZ,
	},
	.table[1] = {		//1920x1080(12bits)@60fps
		.ext_reg_fill = NULL,
		.data = {
			0x01, //ADBIT
			0x00, //MODE
			0x10, //WINMODE 1080P
			0x01, //FRSEL
			0x65, //VMAX_LSB
			0x04, //VMAX_MSB
			0x00, //VMAX_HSB
			0x98, //HMAX_LSB
			0x08, //HMAX_MSB
			0x61, //ODBIT_OPORTSEL, 12bits
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
		.h_pixel = 2200,
		.data_rate = PLL_CLK_148_5MHZ,
	},
	.table[2] = {		//1280x720(12bits)@60fps
		.ext_reg_fill = NULL,
		.data = {
			0x01, //ADBIT
			0x00, //MODE
			0x20, //WINMODE
			0x01, //FRSEL
			0xEE, //VMAX_LSB
			0x02, //VMAX_MSB
			0x00, //VMAX_HSB
			0xE4, //HMAX_LSB
			0x0C, //HMAX_MSB
			0x61, //ODBIT_OPORTSEL, 12bits
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
		.h_pixel = 1650,
		.data_rate = PLL_CLK_74_25MHZ,
	},
	.table[3] = {		//1920x1080(10bits)@120fps
		.ext_reg_fill = NULL,
		.data = {
			0x10, //ADBIT
			0x00, //MODE
			0x10, //WINMODE 1080P
			0x00, //FRSEL
			0x65, //VMAX_LSB
			0x04, //VMAX_MSB
			0x00, //VMAX_HSB
			0x4C, //HMAX_LSB
			0x04, //HMAX_MSB
			0x61, //ODBIT_OPORTSEL, 12bits
		},
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.h_pixel = 2200,
		.data_rate = PLL_CLK_297MHZ,
	},
	/* add video format table here, if necessary */
};


/* Gain table */
/*0.0 dB - 48.0 dB /0.3 dB step */
#define IMX185_GAIN_MAX_DB	0x1E0
#define IMX185_GAIN_ROWS	161
