/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224_reg_tbl.c
 *
 * History:
 *    2014/07/08 - [Long Zhao] Create
 *
 * Copyright (C) 2012-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx224_video_format_reg_table imx224_video_format_tbl = {
	.reg = {
		IMX224_ADBIT,
		IMX224_MODE,
		IMX224_WINMODE,
		IMX224_FRSEL,
		IMX224_VMAX_LSB,
		IMX224_VMAX_MSB,
		IMX224_HMAX_LSB,
		IMX224_HMAX_MSB,
		IMX224_ODBIT,
		IMX224_BLKLEVEL_LSB,
		IMX224_BLKLEVEL_MSB,
		/* DOL related */
		IMX224_WDMODE,
		IMX224_SHS1_LSB,
		IMX224_SHS1_MSB,
		IMX224_SHS1_HSB,
		IMX224_SHS2_LSB,
		IMX224_SHS2_MSB,
		IMX224_SHS2_HSB,
		IMX224_SHS3_LSB,
		IMX224_SHS3_MSB,
		IMX224_SHS3_HSB,
		IMX224_RHS1_LSB,
		IMX224_RHS1_MSB,
		IMX224_RHS1_HSB,
		IMX224_RHS2_LSB,
		IMX224_RHS2_MSB,
		IMX224_RHS2_HSB,
		IMX224_NULL0_SIZE,
		IMX224_PIC_SIZE_LSB,
		IMX224_PIC_SIZE_MSB,
		IMX224_DOL_PAT1,
		IMX224_DOL_PAT2,
		IMX224_XVSCNT_INT,
	},
	.table[0] = {		/* Quad VGAp120 10bits */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* ADBIT */
			0x00, /* MODE */
			0x00, /* WINMODE */
			0x00, /* FRSEL */
			0x4C, /* VMAX_LSB */
			0x04, /* VMAX_MSB */
			0x65, /* HMAX_LSB */
			0x04, /* HMAX_MSB */
			0xE0, /* ODBIT_OPORTSEL */
			0x3C, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
			/* DOL related */
			0x00, /* IMX224_WDMODE */
			0x00, /* IMX224_SHS1_LSB */
			0x00, /* IMX224_SHS1_MSB */
			0x00, /* IMX224_SHS1_HSB */
			0x00, /* IMX224_SHS2_LSB */
			0x00, /* IMX224_SHS2_MSB */
			0x00, /* IMX224_SHS2_HSB */
			0x00, /* IMX224_SHS3_LSB */
			0x00, /* IMX224_SHS3_MSB */
			0x00, /* IMX224_SHS3_HSB */
			0x00, /* IMX224_RHS1_LSB */
			0x00, /* IMX224_RHS1_MSB */
			0x00, /* IMX224_RHS1_HSB */
			0x00, /* IMX224_RHS2_LSB */
			0x00, /* IMX224_RHS2_MSB */
			0x00, /* IMX224_RHS2_HSB */
			0x01, /* IMX224_NULL0_SIZE */
			0xD1, /* IMX224_PIC_SIZE_LSB */
			0x03, /* IMX224_PIC_SIZE_MSB */
			0x01, /* IMX224_DOL_PAT1 */
			0x00, /* IMX224_DOL_PAT2 */
			0x00, /* IMX224_XVSCNT_INT */
		},
		.width		= 1280,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.slvs_eav_col	= 1312,
		.slvs_sav2sav_dist = 1800,
	},
	.table[1] = {		/* 720p120 10bits */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* ADBIT */
			0x00, /* MODE */
			0x10, /* WINMODE */
			0x00, /* FRSEL */
			0xEE, /* VMAX_LSB */
			0x02, /* VMAX_MSB */
			0x72, /* HMAX_LSB */
			0x06, /* HMAX_MSB */
			0xE0, /* ODBIT_OPORTSEL */
			0x3C, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
			/* DOL related */
			0x00, /* IMX224_WDMODE */
			0x00, /* IMX224_SHS1_LSB */
			0x00, /* IMX224_SHS1_MSB */
			0x00, /* IMX224_SHS1_HSB */
			0x00, /* IMX224_SHS2_LSB */
			0x00, /* IMX224_SHS2_MSB */
			0x00, /* IMX224_SHS2_HSB */
			0x00, /* IMX224_SHS3_LSB */
			0x00, /* IMX224_SHS3_MSB */
			0x00, /* IMX224_SHS3_HSB */
			0x00, /* IMX224_RHS1_LSB */
			0x00, /* IMX224_RHS1_MSB */
			0x00, /* IMX224_RHS1_HSB */
			0x00, /* IMX224_RHS2_LSB */
			0x00, /* IMX224_RHS2_MSB */
			0x00, /* IMX224_RHS2_HSB */
			0x01, /* IMX224_NULL0_SIZE */
			0xD9, /* IMX224_PIC_SIZE_LSB */
			0x02, /* IMX224_PIC_SIZE_MSB */
			0x01, /* IMX224_DOL_PAT1 */
			0x00, /* IMX224_DOL_PAT2 */
			0x00, /* IMX224_XVSCNT_INT */
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
		.slvs_eav_col	= 1312,
		.slvs_sav2sav_dist = 2200,
	},
	.table[2] = {		/* 640x480p60 12bits 2x2binning */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* ADBIT */
			0x22, /* MODE */
			0x00, /* WINMODE */
			0x00, /* FRSEL */
			0x26, /* VMAX_LSB */
			0x02, /* VMAX_MSB */
			0x94, /* HMAX_LSB */
			0x11, /* HMAX_MSB */
			0xE1, /* ODBIT_OPORTSEL */
			0xF0, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
			/* DOL related */
			0x00, /* IMX224_WDMODE */
			0x00, /* IMX224_SHS1_LSB */
			0x00, /* IMX224_SHS1_MSB */
			0x00, /* IMX224_SHS1_HSB */
			0x00, /* IMX224_SHS2_LSB */
			0x00, /* IMX224_SHS2_MSB */
			0x00, /* IMX224_SHS2_HSB */
			0x00, /* IMX224_SHS3_LSB */
			0x00, /* IMX224_SHS3_MSB */
			0x00, /* IMX224_SHS3_HSB */
			0x00, /* IMX224_RHS1_LSB */
			0x00, /* IMX224_RHS1_MSB */
			0x00, /* IMX224_RHS1_HSB */
			0x00, /* IMX224_RHS2_LSB */
			0x00, /* IMX224_RHS2_MSB */
			0x00, /* IMX224_RHS2_HSB */
			0x01, /* IMX224_NULL0_SIZE */
			0xD1, /* IMX224_PIC_SIZE_LSB */
			0x03, /* IMX224_PIC_SIZE_MSB */
			0x01, /* IMX224_DOL_PAT1 */
			0x00, /* IMX224_DOL_PAT2 */
			0x00, /* IMX224_XVSCNT_INT */
		},
		.width		= 640,
		.height		= 480,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.slvs_eav_col	= 1312,
		.slvs_sav2sav_dist = 1500,
	},
	.table[3] = {		/* 2x DOL Quad VGAp60 10bits */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* ADBIT */
			0x00, /* MODE */
			0x00, /* WINMODE */
			0x00, /* FRSEL */
			0x4C, /* VMAX_LSB */
			0x04, /* VMAX_MSB */
			0x65, /* HMAX_LSB */
			0x04, /* HMAX_MSB */
			0xE0, /* ODBIT_OPORTSEL */
			0x3C, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
			/* DOL related */
			0x11, /* IMX224_WDMODE */
			0x04, /* IMX224_SHS1_LSB */
			0x00, /* IMX224_SHS1_MSB */
			0x00, /* IMX224_SHS1_HSB */
			0x57, /* IMX224_SHS2_LSB */
			0x00, /* IMX224_SHS2_MSB */
			0x00, /* IMX224_SHS2_HSB */
			0x00, /* IMX224_SHS3_LSB */
			0x00, /* IMX224_SHS3_MSB */
			0x00, /* IMX224_SHS3_HSB */
#ifdef USE_960P_2X_30FPS
			0x51, /* IMX224_RHS1_LSB */
			0x04, /* IMX224_RHS1_MSB */
#else
			0xC1, /* IMX224_RHS1_LSB */
			0x00, /* IMX224_RHS1_MSB */
#endif
			0x00, /* IMX224_RHS1_HSB */
			0x00, /* IMX224_RHS2_LSB */
			0x00, /* IMX224_RHS2_MSB */
			0x00, /* IMX224_RHS2_HSB */
			0x00, /* IMX224_NULL0_SIZE */
			0xA2, /* IMX224_PIC_SIZE_LSB */
			0x07, /* IMX224_PIC_SIZE_MSB */
			0x00, /* IMX224_DOL_PAT1 */
			0x01, /* IMX224_DOL_PAT2 */
			0x01, /* IMX224_XVSCNT_INT */
		},
		.width		= IMX224_H_PIXEL*2+IMX224_HBLANK,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
#ifdef USE_960P_2X_30FPS
		.max_fps	= AMBA_VIDEO_FPS_30,
#else
		.max_fps	= AMBA_VIDEO_FPS_60,
#endif
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.slvs_eav_col	= IMX224_H_PIXEL*2+IMX224_HBLANK,
		.slvs_sav2sav_dist = IMX224_H_PERIOD*2,
		.hdr_win_offset = {
			.long_start_x = 16,
			.long_start_y = 24,
			.short1_start_x = IMX224_H_PIXEL+IMX224_HBLANK+16,
#ifdef USE_960P_2X_30FPS
			.short1_start_y = 24 + (0x451 - 1)/2,
#else
			.short1_start_y = 24 + (0xC1 - 1)/2,
#endif
		},
	},
	.table[4] = {		/* 3x DOL Quad VGAp30 10bits */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* ADBIT */
			0x00, /* MODE */
			0x00, /* WINMODE */
			0x00, /* FRSEL */
			0x4C, /* VMAX_LSB */
			0x04, /* VMAX_MSB */
			0x65, /* HMAX_LSB */
			0x04, /* HMAX_MSB */
			0xE0, /* ODBIT_OPORTSEL */
			0x3C, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
			/* DOL related */
			0x21, /* IMX224_WDMODE */
			0x04, /* IMX224_SHS1_LSB */
			0x00, /* IMX224_SHS1_MSB */
			0x00, /* IMX224_SHS1_HSB */
			0x89, /* IMX224_SHS2_LSB */
			0x00, /* IMX224_SHS2_MSB */
			0x00, /* IMX224_SHS2_HSB */
			0x2F, /* IMX224_SHS3_LSB */
			0x01, /* IMX224_SHS3_MSB */
			0x00, /* IMX224_SHS3_HSB */
			0x51, /* IMX224_RHS1_LSB */
			0x04, /* IMX224_RHS1_MSB */
			0x00, /* IMX224_RHS1_HSB */
			0x69, /* IMX224_RHS2_LSB */
			0x05, /* IMX224_RHS2_MSB */
			0x00, /* IMX224_RHS2_HSB */
			0x00, /* IMX224_NULL0_SIZE */
			0x73, /* IMX224_PIC_SIZE_LSB */
			0x0B, /* IMX224_PIC_SIZE_MSB */
			0x00, /* IMX224_DOL_PAT1 */
			0x01, /* IMX224_DOL_PAT2 */
			0x03, /* IMX224_XVSCNT_INT */
		},
		.width		= IMX224_H_PIXEL*3+IMX224_HBLANK*2,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.slvs_eav_col	= IMX224_H_PIXEL*3+IMX224_HBLANK*2,
		.slvs_sav2sav_dist = IMX224_H_PERIOD*3,
		.hdr_win_offset = {
			.long_start_x = 16,
			.long_start_y = 24,
			.short1_start_x = IMX224_H_PIXEL+IMX224_HBLANK+16,
			.short1_start_y = 24 + (0x451 - 1) / 3,
			.short2_start_x = (IMX224_H_PIXEL+IMX224_HBLANK+16) * 2,
			.short2_start_y = 24 + (0x569 - 2) / 3,
		},
	},
	/* add video format table here, if necessary */
};


/* Gain table */
/*0.0 dB - 48.0 dB /0.3 dB step */
#define IMX224_GAIN_MAX_DB	480
#define IMX224_GAIN_ROWS	481
