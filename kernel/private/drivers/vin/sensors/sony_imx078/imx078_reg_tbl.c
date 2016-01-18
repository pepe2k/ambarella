/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/imx078_reg_tbl.c
 *
 * History:
 *    2013/03/11 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx078_video_format_reg_table imx078_video_format_tbl = {
	.reg		= {
		imx078_REG_01,
		imx078_REG_SVR_LSB,
		imx078_REG_SVR_MSB,
		imx078_REG_0D,
		imx078_REG_0E,
		imx078_REG_17,
		imx078_REG_ADBIT,
		imx078_REG_69,
	},
	.table[0]	= {
		.ext_reg_fill	= NULL,
		.data	= {
			0x33,
			0x00,
			0x00,
			0x02,
			0x00,
			0x00,
			0x01,
			0x00,
		},
		.width		= 1984,
		.height		= 1116,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 1,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_59_94,
		.pll_index	= 0,
		.xhs_clk = 462,
	},
};

#define imx078_GAIN_ROWS	449
#define imx078_GAIN_COLS	2

#define imx078_GAIN_COL_AGC	0
#define imx078_GAIN_COL_DGAIN	1
#define imx078_GAIN_0DB 448




