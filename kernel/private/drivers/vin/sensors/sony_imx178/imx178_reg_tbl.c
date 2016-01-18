/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_reg_tbl.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
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
static const struct imx178_video_format_reg_table imx178_video_format_tbl = {
	.reg = {
		0x300E, //MODE
		0x300F, //WINMODE
		0x3010, //TCYCLE
		0x3066, //VNDMY
		0x302C, //VMAX_LSB
		0x302D, //VMAX_MSB
		0x302E, //VMAX_HSB
		0x302F, //HMAX_LSB
		0x3030, //HMAX_MSB
		0x300D, //ADBIT_ADBITFREQ
		0x3059, //ODBIT_OPORTSEL
		0x3004, //STBLVDS
		0x3101, //FREQ
		0x3015, //BLKLEVEL_LSB
		0x3016, //BLKLEVEL_MSB
	},
	.table[0] = {		//6.3M: 3072x2048(14bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x00, //MODE
			0x00, //WINMODE
			0x00, //TCYCLE
			0x06, //VNDMY
			0x61, //VMAX_LSB
			0x08, //VMAX_MSB
			0x00, //VMAX_HSB
			0x48, //HMAX_LSB
			0x03, //HMAX_MSB
			0x02, //ADBIT_ADBITFREQ, ADC 14bits
			0x02, //ODBIT_OPORTSEL, 10ch, 14bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0x20, //BLKLEVEL_LSB
			0x03, //BLKLEVEL_MSB
		},
		.width = 3072,
		.height = 2048,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_14,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 3100,
		.slvs_sav2sav_dist = 4800,
		.h_inck = 840,
		.lane_num = 10,
		},
	.table[1] = {		//5.0M: 2592x1944(14bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x00, //MODE
			0x10, //WINMODE
			0x00, //TCYCLE
			0x06, //VNDMY
			0x61, //VMAX_LSB
			0x08, //VMAX_MSB
			0x00, //VMAX_HSB
			0x48, //HMAX_LSB
			0x03, //HMAX_MSB
			0x02, //ADBIT_ADBITFREQ, ADC 14bits
			0x02, //ODBIT_OPORTSEL, 10ch, 14bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0x20, //BLKLEVEL_LSB
			0x03, //BLKLEVEL_MSB
		},
		.width = 2592,
		.height = 1944,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_14,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 2620,
		.slvs_sav2sav_dist = 4800,
		.h_inck = 840,
		.lane_num = 10,
		},
	.table[2] = {		//2560x2048(14bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x00, //MODE
			0x20, //WINMODE
			0x00, //TCYCLE
			0x06, //VNDMY
			0x61, //VMAX_LSB
			0x08, //VMAX_MSB
			0x00, //VMAX_HSB
			0x48, //HMAX_LSB
			0x03, //HMAX_MSB
			0x02, //ADBIT_ADBITFREQ, ADC 14bits
			0x02, //ODBIT_OPORTSEL, 10ch, 14bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0x20, //BLKLEVEL_LSB
			0x03, //BLKLEVEL_MSB
		},
		.width = 2560,
		.height = 2048,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_14,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 2588,
		.slvs_sav2sav_dist = 4800,
		.h_inck = 840,
		.lane_num = 10,
		},
	.table[3] = {		//1.6M: 1536x1024(14bits)@30fps 2x2 binning
		.ext_reg_fill = NULL,
		.data = {
			0x23, //MODE
			0x00, //WINMODE
			0x01, //TCYCLE
			0x06, //VNDMY
			0x08, //VMAX_LSB
			0x0C, //VMAX_MSB
			0x00, //VMAX_HSB
			0x49, //HMAX_LSB
			0x02, //HMAX_MSB
			0x05, //ADBIT_ADBITFREQ, ADC 12bits
			0x02, //ODBIT_OPORTSEL, 10ch, 14bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0x20, //BLKLEVEL_LSB
			0x03, //BLKLEVEL_MSB
		},
		.width = 1536,
		.height = 1024,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_14,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 1550,
		.slvs_sav2sav_dist = 3343,
		.h_inck = 585,
		.lane_num = 10,
		},
	.table[4] = {		//1080P(12bits)@120fps
		.ext_reg_fill = NULL,
		.data = {
			0x01, //MODE
			0x00, //WINMODE
			0x00, //TCYCLE
			0x03, //VNDMY
			0x65, //VMAX_LSB
			0x04, //VMAX_MSB
			0x00, //VMAX_HSB
			0x26, //HMAX_LSB
			0x02, //HMAX_MSB
			0x05, //ADBIT_ADBITFREQ, ADC 12bits
			0x01, //ODBIT_OPORTSEL, 10ch, 12bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0xC8, //BLKLEVEL_LSB
			0x00, //BLKLEVEL_MSB
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
		.pll_index = 1,
		.slvs_eav_col	= 1950,
		.slvs_sav2sav_dist = 3667,
		.h_inck = 550,
		.lane_num = 10,
		},
	.table[5] = {		//720P@120fps 2x2 binning
		.ext_reg_fill = NULL,
		.data = {
			0x24, //MODE
			0x00, //WINMODE
			0x01, //TCYCLE
			0x04, //VNDMY
			0x72, //VMAX_LSB
			0x06, //VMAX_MSB
			0x00, //VMAX_HSB
			0x77, //HMAX_LSB
			0x01, //HMAX_MSB
			0x04, //ADBIT_ADBITFREQ, ADC 10bits
			0x01, //ODBIT_OPORTSEL, 10ch, 12bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0xC8, //BLKLEVEL_LSB
			0x00, //BLKLEVEL_MSB
		},
		.width = 1280,
		.height = 720,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
		.slvs_eav_col	= 1310,
		.slvs_sav2sav_dist = 5000,
		.h_inck = 375,
		.lane_num = 10,
		},
	.table[6] = {		//6.3M: 3072x2048(12bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x00, //MODE
			0x00, //WINMODE
			0x00, //TCYCLE
			0x06, //VNDMY
			0x30, //VMAX_LSB
			0x08, //VMAX_MSB
			0x00, //VMAX_HSB
			0xEF, //HMAX_LSB
			0x01, //HMAX_MSB
			0x05, //ADBIT_ADBITFREQ, ADC 12bits
			0x01, //ODBIT_OPORTSEL, 10ch, 12bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0xC8, //BLKLEVEL_LSB
			0x00, //BLKLEVEL_MSB
		},
		.width = 3072,
		.height = 2048,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 2,
		.slvs_eav_col	= 3100,
		.slvs_sav2sav_dist = 3300,
		.h_inck = 495,
		.lane_num = 10,
		},
	.table[7] = {		//5.0M: 2592x1944(12bits)@60fps
		.ext_reg_fill = NULL,
		.data = {
			0x00, //MODE
			0x10, //WINMODE
			0x00, //TCYCLE
			0x06, //VNDMY
			0x61, //VMAX_LSB
			0x08, //VMAX_MSB
			0x00, //VMAX_HSB
			0xA4, //HMAX_LSB
			0x01, //HMAX_MSB
			0x05, //ADBIT_ADBITFREQ, ADC 12bits
			0x01, //ODBIT_OPORTSEL, 10ch, 12bits
			0x00, //STBLVDS, 10ch active
			0x30, //FREQ
			0xC8, //BLKLEVEL_LSB
			0x00, //BLKLEVEL_MSB
		},
		.width = 2592,
		.height = 1944,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 2620,
		.slvs_sav2sav_dist = 2800,
		.h_inck = 420,
		.lane_num = 10,
		},
	.table[8] = {		/* 6.3M: 3072x2048(10bits)@60fps */
		.ext_reg_fill = NULL,
		.data = {
			0x00, /* MODE */
			0x00, /* WINMODE */
			0x00, /* TCYCLE */
			0x06, /* VNDMY */
			0x61, /* VMAX_LSB */
			0x08, /* VMAX_MSB */
			0x00, /* VMAX_HSB */
			0xA4, /* HMAX_LSB */
			0x01, /* HMAX_MSB */
			0x00, /* ADBIT_ADBITFREQ, ADC 10bits */
			0x00, /* ODBIT_OPORTSEL, 10ch, 10bits */
			0x00, /* STBLVDS, 10ch active */
			0x30, /* FREQ */
			0x32, /* BLKLEVEL_LSB */
			0x00, /* BLKLEVEL_MSB */
		},
		.width = 3072,
		.height = 2048,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 3100,
		.slvs_sav2sav_dist = 3360,
		.h_inck = 420,
		.lane_num = 10,
		},
	/* add video format table here, if necessary */
};


/* Gain table */
#define IMX178_GAIN_MAX_DB  0x1E0 //low light: 48DB, high light:51DB
#define IMX178_GAIN_ROWS 481
