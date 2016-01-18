/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_reg_tbl.c
 *
 * History:
 *    2013/12/27 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx123_video_format_reg_table imx123_video_format_tbl = {
	.table[0] = {		// 4ch 3M linear
		.ext_reg_fill = NULL,
		.regs = {
			/* 4 lane, QXGA 60fps */
			{0x3007, 0x00}, /* WINMODE */
			{0x3009, 0x01}, /* DRSEL */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0xEE}, /* HMAX_LSB */
			{0x301C, 0x02}, /* HMAX_MSB */
			{0x3044, 0x25}, /* ODBIT */

			{0xFFFF, 0xFF},
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
		.slvs_eav_col	= IMX123_QXGA_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
	},
	.table[1] = {		// 4ch 1080p linear
		.ext_reg_fill = NULL,
		.regs = {
			/* 4 lane, 1080p 60fps */
			{0x3007, 0x10}, /* WINMODE */
			{0x3009, 0x01}, /* DRSEL */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0xEE}, /* HMAX_LSB */
			{0x301C, 0x02}, /* HMAX_MSB */
			{0x3044, 0x25}, /* ODBIT */

			{0xFFFF, 0xFF},
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
		.pll_index = 0,
		.slvs_eav_col	= IMX123_1080P_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
	},
	.table[2] = {		// 3M 2x wdr
		.ext_reg_fill = NULL,
		.regs = {
#if 1
			/* 2X WDR setting QXGA 30 fps */
			{0x3007, 0x00}, /* WINMODE */
			{0x3009, 0x01}, /* DRSEL from 120fps to 60fps */
			{0x300C, 0x14}, /* WDSEL */
			{0x3044, 0x35}, /* ODBIT */
			{0x3046, 0x44}, /* XVSMSKCNT/XHSMSKCNT */
			{0x30C8, 0x90}, /* HBLANK1, Tentative */
			{0x30C9, 0x40}, /* HBLANK1, Tentative */
			{0x30CA, 0xCD}, /* HBLANK2, Tentative */
			{0x30CB, 0x20}, /* HBLANK2, XVSMSKCNT_INT */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0xEE}, /* HMAX_LSB from 120fps to 60fps */
			{0x301C, 0x02}, /* HMAX_MSB from 120fps to 60fps */
#else
			/* 2X WDR setting QXGA 60 fps */
			{0x3007, 0x00}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x300C, 0x14}, /* WDSEL */
			{0x3044, 0x35}, /* ODBIT */
			{0x3046, 0x44}, /* XVSMSKCNT/XHSMSKCNT */
			{0x30C8, 0xA0}, /* HBLANK1, Tentative */
			{0x30C9, 0x40}, /* HBLANK1, Tentative */
			{0x30CA, 0xCD}, /* HBLANK2, Tentative */
			{0x30CB, 0x20}, /* HBLANK2, XVSMSKCNT_INT */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0x77}, /* HMAX_LSB */
			{0x301C, 0x01}, /* HMAX_MSB */
#endif
			/* SHS1 7h */
			{0x301E, 0x07}, /* SHS1_LSB */
			{0x301F, 0x00}, /* SHS1_MSB */
			{0x3020, 0x00}, /* SHS1_HSB */
			/* SHS2 2F0h */
			{0x3021, 0xF0}, /* SHS2_LSB */
			{0x3022, 0x02}, /* SHS2_MSB */
			{0x3023, 0x00}, /* SHS2_HSB */
#ifdef USE_3M_2X_25FPS
			/* RHS1 2BAh */
			{0x302E, 0xBA}, /* RHS1_LSB */
			{0x302F, 0x02}, /* RHS1_MSB */
#else
			/* RHS1 3Ah */
			{0x302E, 0x3A}, /* RHS1_LSB */
			{0x302F, 0x00}, /* RHS1_MSB */
#endif
			{0x3030, 0x00}, /* RHS1_HSB */
			/* RHS2 1Ch */
			{0x3031, 0x1C}, /* RHS2_LSB */
			{0x3032, 0x00}, /* RHS2_MSB */
			{0x3033, 0x00}, /* RHS2_HSB */

			{0xFFFF, 0xFF},
		},
		.width = 2048 * 2,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
#ifdef USE_3M_2X_25FPS
		.max_fps = AMBA_VIDEO_FPS_25,
		.auto_fps = AMBA_VIDEO_FPS_25,
#else
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
#endif
		.pll_index = 0,
		.slvs_eav_col	= IMX123_QXGA_H_PIXEL * 2 + 0x90,
		.slvs_sav2sav_dist = IMX123_H_PERIOD * 2,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 20,
			.short1_start_x = IMX123_QXGA_H_PIXEL + 0x90,
#ifdef USE_3M_2X_25FPS
			.short1_start_y = 20 + (0x2BA - 2) / 2,
#else
			.short1_start_y = 20 + (0x3A - 2) / 2,
#endif
		},
	},
	.table[3] = {		// 1080p 2x wdr
		.ext_reg_fill = NULL,
		.regs = {
			/* 2X WDR setting 1080p 60 fps */
			{0x3007, 0x10}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x300C, 0x14}, /* WDSEL */
			{0x3044, 0x35}, /* ODBIT */
			{0x3046, 0x44}, /* XVSMSKCNT/XHSMSKCNT */
			{0x30C8, 0xA0}, /* HBLANK1, Tentative */
			{0x30C9, 0x40}, /* HBLANK1, Tentative */
			{0x30CA, 0xCD}, /* HBLANK2, Tentative */
			{0x30CB, 0x20}, /* HBLANK2, XVSMSKCNT_INT */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0x77}, /* HMAX_LSB */
			{0x301C, 0x01}, /* HMAX_MSB */
			/* SHS1 Eh */
			{0x301E, 0x0E}, /* SHS1_LSB */
			{0x301F, 0x00}, /* SHS1_MSB */
			{0x3020, 0x00}, /* SHS1_HSB */
			/* SHS2 E0h */
			{0x3021, 0xE0}, /* SHS2_LSB */
			{0x3022, 0x00}, /* SHS2_MSB */
			{0x3023, 0x00}, /* SHS2_HSB */
			/* RHS1 2BAh */
			{0x302E, 0xBA}, /* RHS1_LSB */
			{0x302F, 0x02}, /* RHS1_MSB */
			{0x3030, 0x00}, /* RHS1_HSB */
			/* RHS2 1Ch */
			{0x3031, 0x1C}, /* RHS2_LSB */
			{0x3032, 0x00}, /* RHS2_MSB */
			{0x3033, 0x00}, /* RHS2_HSB */

			{0xFFFF, 0xFF},
		},
		.width = 1920 * 2,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_1080P_H_PIXEL * 2 + 0xA0,
		.slvs_sav2sav_dist = IMX123_H_PERIOD * 2,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 20,
			.short1_start_x = IMX123_1080P_H_PIXEL + 0xA0,
			.short1_start_y = 20 + (0x2BA - 2) / 2,
		},
	},
	.table[4] = {		// 3M 3x wdr
		.ext_reg_fill = NULL,
		.regs = {
			/* 3X WDR setting QXGA 30fps */
			{0x3007, 0x00}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x300C, 0x24}, /* WDSEL */
			{0x3044, 0x35}, /* ODBIT */
			{0x3046, 0x8C}, /* XVSMSKCNT/XHSMSKCNT */
			{0x30C8, 0xD0}, /* HBLANK1, Tentative */
			{0x30C9, 0x40}, /* HBLANK1, Tentative */
			{0x30CA, 0xD0}, /* HBLANK2, Tentative */
			{0x30CB, 0x60}, /* HBLANK2, XVSMSKCNT_INT */
			{0x3018, 0x42}, /* VMAX_LSB *///{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0x77}, /* HMAX_LSB */
			{0x301C, 0x01}, /* HMAX_MSB */
			/* SHS1 14h */
			{0x301E, 0x14}, /* SHS1_LSB */
			{0x301F, 0x00}, /* SHS1_MSB */
			{0x3020, 0x00}, /* SHS1_HSB */
			/* SHS2 DEh */
			{0x3021, 0xDE}, /* SHS2_LSB */
			{0x3022, 0x00}, /* SHS2_MSB */
			{0x3023, 0x00}, /* SHS2_HSB */
			/* SHS3 100h */
			{0x3024, 0x00}, /* SHS3_LSB */
			{0x3025, 0x01}, /* SHS3_MSB */
			{0x3026, 0x00}, /* SHS3_HSB */
			/* RHS1 24Eh */
			{0x302E, 0x4E}, /* RHS1_LSB */
			{0x302F, 0x02}, /* RHS1_MSB */
			{0x3030, 0x00}, /* RHS1_HSB */
			/* RHS2 31Ch */
			{0x3031, 0x1C}, /* RHS2_LSB */
			{0x3032, 0x03}, /* RHS2_MSB */
			{0x3033, 0x00}, /* RHS2_HSB */

			{0xFFFF, 0xFF},
		},
		.width = 2048 * 3,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_25,
		.auto_fps = AMBA_VIDEO_FPS_25,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_QXGA_H_PIXEL * 3 + 0xD0 * 2,
		.slvs_sav2sav_dist = IMX123_H_PERIOD * 3,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 20,
			.short1_start_x = IMX123_QXGA_H_PIXEL + 0xD0,
			.short1_start_y = 20 + (0x24E - 2) / 3,
			.short2_start_x = (IMX123_QXGA_H_PIXEL + 0xD0) * 2,
			.short2_start_y = 20 + (0x31C - 4) / 3,
		},
	},
	.table[5] = {		// 1080p 3x wdr
		.ext_reg_fill = NULL,
		.regs = {
			/* 3X WDR setting 1080p 30fps */
			{0x3007, 0x10}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x300C, 0x24}, /* WDSEL */
			{0x3044, 0x35}, /* ODBIT */
			{0x3046, 0x8C}, /* XVSMSKCNT/XHSMSKCNT */
			{0x30C8, 0xCB}, /* HBLANK1, Tentative */
			{0x30C9, 0x40}, /* HBLANK1, Tentative */
			{0x30CA, 0xCD}, /* HBLANK2, Tentative */
			{0x30CB, 0x60}, /* HBLANK2, XVSMSKCNT_INT */
			{0x3018, 0x42}, /* VMAX_LSB *///{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0x77}, /* HMAX_LSB */
			{0x301C, 0x01}, /* HMAX_MSB */
			/* SHS1 14h */
			{0x301E, 0x14}, /* SHS1_LSB */
			{0x301F, 0x00}, /* SHS1_MSB */
			{0x3020, 0x00}, /* SHS1_HSB */
			/* SHS2 DEh */
			{0x3021, 0xDE}, /* SHS2_LSB */
			{0x3022, 0x00}, /* SHS2_MSB */
			{0x3023, 0x00}, /* SHS2_HSB */
			/* SHS3 100h */
			{0x3024, 0x00}, /* SHS3_LSB */
			{0x3025, 0x01}, /* SHS3_MSB */
			{0x3026, 0x00}, /* SHS3_HSB */
			/* RHS1 24Eh */
			{0x302E, 0x4E}, /* RHS1_LSB */
			{0x302F, 0x02}, /* RHS1_MSB */
			{0x3030, 0x00}, /* RHS1_HSB */
			/* RHS2 31Ch */
			{0x3031, 0x1C}, /* RHS2_LSB */
			{0x3032, 0x03}, /* RHS2_MSB */
			{0x3033, 0x00}, /* RHS2_HSB */

			{0xFFFF, 0xFF},
		},
		.width = 2048 * 3,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_25,
		.auto_fps = AMBA_VIDEO_FPS_25,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_1080P_H_PIXEL * 3 + 0xCB + 0xCD,
		.slvs_sav2sav_dist = IMX123_H_PERIOD * 3,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 20,
			.short1_start_x = IMX123_1080P_H_PIXEL + 0xCB,
			.short1_start_y = 20 + (0x2E4 - 2) / 3,
			.short2_start_x = IMX123_1080P_H_PIXEL * 2 + 0xCB + 0xCD,
			.short2_start_y = 20 + (0x31C - 4) / 3,
		},
	},
	.table[6] = {		// 8ch 3M linear
		.ext_reg_fill = NULL,
		.regs = {
			/* 8 lane, QXGA 120fps */
			{0x3007, 0x00}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x301B, 0x77}, /* HMAX_LSB */
			{0x301C, 0x01}, /* HMAX_MSB */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301E, 0x08}, /* SHS1_LSB */
			{0x3021, 0x08}, /* SHS2_LSB */
			{0x3024, 0x08}, /* SHS3_LSB */
			{0x3027, 0x08},
			{0x3044, 0x35}, /* ODBIT */

			{0xFFFF, 0xFF},
		},
		.width = 2048,
		.height = 1536,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_QXGA_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
	},
	.table[7] = {		// 8ch 1080p linear
		.ext_reg_fill = NULL,
		.regs = {
			/* 8 lane 1080p 120 fps */
			{0x3007, 0x10}, /* WINMODE */
			{0x3009, 0x00}, /* DRSEL */
			{0x301B, 0x26}, /* HMAX_LSB */
			{0x301C, 0x02}, /* HMAX_MSB */
			{0x3018, 0x65}, /* VMAX_LSB */
			{0x3019, 0x04}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301E, 0x08}, /* SHS1_LSB */
			{0x3021, 0x08}, /* SHS2_LSB */
			{0x3024, 0x08}, /* SHS3_LSB */
			{0x3027, 0x08},
			{0x3044, 0x35}, /* ODBIT */

			{0xFFFF, 0xFF},
		},
		.width = 1920,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_1080P_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
	},
	.table[8] = {		// 4ch 3M dual gain
		.ext_reg_fill = NULL,
		.regs = {
			{0x3007, 0x0C}, /* WINMODE */
			{0x3009, 0x01}, /* DRSEL */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0xEE}, /* HMAX_LSB */
			{0x301C, 0x02}, /* HMAX_MSB */
			{0x3044, 0x25}, /* ODBIT */

			{0xFFFF, 0xFF},
		},
		.width = 2048,
		.height = 1536*2,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_QXGA_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 0,
			.short1_start_x = 0,
			.short1_start_y = 1,
		},
	},
	.table[9] = {		// 4ch 1080p dual gain
		.ext_reg_fill = NULL,
		.regs = {
			{0x3007, 0x1C}, /* WINMODE */
			{0x3009, 0x01}, /* DRSEL */
			{0x3018, 0x40}, /* VMAX_LSB */
			{0x3019, 0x06}, /* VMAX_MSB */
			{0x301A, 0x00}, /* VMAX_HSB */
			{0x301B, 0xEE}, /* HMAX_LSB */
			{0x301C, 0x02}, /* HMAX_MSB */
			{0x3044, 0x25}, /* ODBIT */

			{0xFFFF, 0xFF},
		},
		.width = 1920,
		.height = 1080*2,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= IMX123_1080P_H_PIXEL,
		.slvs_sav2sav_dist = IMX123_H_PERIOD,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 0,
			.short1_start_x = 0,
			.short1_start_y = 1,
		},
	},
	/* add video format table here, if necessary */
};


/* Gain table */
#define IMX123_GAIN_MAX_IDX  0x1E0 //low light: 48DB
#define IMX123_DUAL_GAIN_MAX_IDX  510 //51DB

