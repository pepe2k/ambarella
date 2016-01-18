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
		IMX136_WINPV_LSB,
		IMX136_WINPV_MSB,
		IMX136_WINWV_LSB,
		IMX136_WINWV_MSB,
		IMX136_WINPH_LSB,
		IMX136_WINPH_MSB,
		IMX136_WINWH_LSB,
		IMX136_WINWH_MSB,
		IMX136_OUTCTRL,
		IMX136_INCKSEL0,
		IMX136_INCKSEL1,
		IMX136_INCKSEL2,
		IMX136_INCKSEL3,
		IMX136_INCKSEL4,
	},
	.table[0]	= {	//1920x1080@120fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x10, //IMX136_ADBIT
			0x00, //IMX136_MODE
			0x10, //IMX136_WINMODE,
			0x00, //IMX136_FRSEL
			0x65, //IMX136_VMAX_LSB
			0x04, //IMX136_VMAX_MSB
			0x00, //IMX136_VMAX_HSB
			0x4C, //IMX136_HMAX_LSB
			0x04, //IMX136_HMAX_MSB
			0x3C, //IMX136_WINPV_LSB
			0x00, //IMX136_WINPV_MSB
			0x50, //IMX136_WINWV_LSB
			0x04, //IMX136_WINWV_MSB
			0x00, //IMX136_WINPH_LSB
			0x00, //IMX136_WINPH_MSB
			0x9C, //IMX136_WINWH_LSB
			0x07, //IMX136_WINWH_MSB
			0x61, //IMX136_OUTCTRL
			0x01, //IMX136_INCKSEL0
			0x20, //IMX136_INCKSEL1
			0x06, //IMX136_INCKSEL2
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
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
	.table[1]	= {	//1280x1024@120fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x10, //IMX136_ADBIT
			0x00, //IMX136_MODE
			0x40, //IMX136_WINMODE,
			0x00, //IMX136_FRSEL
			0x65, //IMX136_VMAX_LSB
			0x04, //IMX136_VMAX_MSB
			0x00, //IMX136_VMAX_HSB
			0x4C, //IMX136_HMAX_LSB
			0x04, //IMX136_HMAX_MSB
			0x58, //IMX136_WINPV_LSB
			0x00, //IMX136_WINPV_MSB
			0x18, //IMX136_WINWV_LSB
			0x04, //IMX136_WINWV_MSB
			0x40, //IMX136_WINPH_LSB
			0x01, //IMX136_WINPH_MSB
			0x18, //IMX136_WINWH_LSB
			0x05, //IMX136_WINWH_MSB
			0x61, //IMX136_OUTCTRL
			0x01, //IMX136_INCKSEL0
			0x20, //IMX136_INCKSEL1
			0x06, //IMX136_INCKSEL2
			0x30, //IMX136_INCKSEL3
			0x04, //IMX136_INCKSEL4
		},
		.width		= 1280,
		.height		= 1024,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
};

static const struct imx136_wdr_mode_table imx136_wdr_mode_tbl = {
	.reg		= {
		IMX136_WDMODE,   //WDMODE, FULLSWING, WDSEL
		IMX136_SHS1_LSB, //SHS1
		IMX136_SHS1_MSB,
		IMX136_SHS1_HSB,
		IMX136_SHS2_LSB, //SHS2
		IMX136_SHS2_MSB,
		IMX136_SHS2_HSB,
		IMX136_SHS3_LSB, //SHS3
		IMX136_SHS3_MSB,
		IMX136_SHS3_HSB,
		IMX136_SHS4_LSB, //SHS4
		IMX136_SHS4_MSB,
		IMX136_SHS4_HSB,
		IMX136_VRSET,
		IMX136_REG65,
		IMX136_REG86,
	},
	.table[0]	= { //Normal mode
		.data	= {
			0x00, //IMX136_WDMODE,
			0x00, //IMX136_SHS1_LSB,
			0x00, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x00, //IMX136_SHS2_LSB,
			0x00, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x00, //IMX136_SHS3_LSB,
			0x00, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x0C, //IMX136_VRSET,
			0x20, //IMX136_REG65,
			0x01, //IMX136_REG86,
		},
	},
	.table[1]	= { //IMX136_NEW_WDR_2FRAME_16X_MODE
		.data	= {
			0x11, //IMX136_WDMODE,
			0xD9, //IMX136_SHS1_LSB,
			0x03, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x00, //IMX136_SHS2_LSB,
			0x00, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x00, //IMX136_SHS3_LSB,
			0x00, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x1D, //IMX136_VRSET,
			0x00, //IMX136_REG65,
			0x10, //IMX136_REG86,
		},
	},
	.table[2]	= { //IMX136_NEW_WDR_2FRAME_32X_MODE
		.data	= {
			0x11, //IMX136_WDMODE,
			0x1F, //IMX136_SHS1_LSB,
			0x04, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x00, //IMX136_SHS2_LSB,
			0x00, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x00, //IMX136_SHS3_LSB,
			0x00, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x1C, //IMX136_VRSET,
			0x00, //IMX136_REG65,
			0x10, //IMX136_REG86,
		},
	},
	.table[3]	= { //IMX136_NEW_WDR_4FRAME_MODE
		.data	= {
			0x31, //IMX136_WDMODE,
			0x63, //IMX136_SHS1_LSB,
			0x04, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x51, //IMX136_SHS2_LSB,
			0x04, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x4D, //IMX136_SHS3_LSB,
			0x03, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x18, //IMX136_VRSET,
			0x00, //IMX136_REG65,
			0x10, //IMX136_REG86,
		},
	},
	.table[4]	= { //IMX136_NEW_WDR_2FRAME_CTM_16X_MODE
		.data	= {
			0x19, //IMX136_WDMODE,
			0x1F, //IMX136_SHS1_LSB,
			0x04, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x6A, //IMX136_SHS2_LSB,
			0x04, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x00, //IMX136_SHS3_LSB,
			0x00, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x1D, //IMX136_VRSET,
			0x00, //IMX136_REG65,
			0x10, //IMX136_REG86,
		},
	},
	.table[5]	= { //IMX136_NEW_WDR_2FRAME_CTM_32X_MODE
		.data	= {
			0x19, //IMX136_WDMODE,
			0x42, //IMX136_SHS1_LSB,
			0x04, //IMX136_SHS1_MSB,
			0x00, //IMX136_SHS1_HSB,
			0x6A, //IMX136_SHS2_LSB,
			0x04, //IMX136_SHS2_MSB,
			0x00, //IMX136_SHS2_HSB,
			0x00, //IMX136_SHS3_LSB,
			0x00, //IMX136_SHS3_MSB,
			0x00, //IMX136_SHS3_HSB,
			0x00, //IMX136_SHS4_LSB,
			0x00, //IMX136_SHS4_MSB,
			0x00, //IMX136_SHS4_HSB,
			0x1D, //IMX136_VRSET,
			0x00, //IMX136_REG65,
			0x10, //IMX136_REG86,
		},
	},
};

#define IMX136_GAIN_ROWS  421
#define IMX136_GAIN_42DB  0x1A4