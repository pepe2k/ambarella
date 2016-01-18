/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx172/imx172_reg_tbl.c
 *
 * History:
 *    2012/08/22 - [Cao Rongrong] Create
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
static const struct imx172_video_format_reg_table imx172_video_format_tbl = {
	.reg		= {
		IMX172_REG_03,
		IMX172_REG_MDSEL1,
		IMX172_REG_MDSEL2,
		IMX172_REG_MDSEL6,
	},
	.table[0]	= {// mode 0: 4000x3000 10ch 12bit 34.97fps
		.ext_reg_fill	= NULL,
		.data		= {0x00, 0x00, 0x07, 0x47},
		.width		= 4000,
		.height		= 3000,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,	// CRR: FIXME
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 1,
		.xhs_clk	= 660,
		.h_xhs		= 1,
		.slvs_eav_col	= 4170,
		.lane_num	= 10,
	},
	.table[1]	= {// mode 1: 4000x3000 10ch 10bit 39.96fps
		.ext_reg_fill	= NULL,
		.data		= {0x00, 0x00, 0x01, 0x47},
		.width		= 4000,
		.height		= 3000,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,	// CRR: FIXME
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.xhs_clk	= 585,
		.h_xhs		= 1,
		.slvs_eav_col	= 4170,
		.lane_num	= 10,
	},
	.table[2]	= {// mode 2: 4096X2160 8ch 12bit 29.97fps
		.ext_reg_fill	= NULL,
		.data		= {0x11, 0x80, 0x47, 0x47},
		.width		= 4096,
		.height		= 2160,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.xhs_clk	= 1092,
		.h_xhs		= 1,
		.slvs_eav_col	= 4248,
		.lane_num	= 8,
	},
	.table[3]	= {// mode 3: 4096X2160 4ch 12bit 14.99fps
		.ext_reg_fill	= NULL,
		.data		= {0x33, 0x80, 0x47, 0x47},
		.width		= 4096,
		.height		= 2160,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_15,
		.auto_fps	= AMBA_VIDEO_FPS_15,
		.pll_index	= 0,
		.xhs_clk	= 2184,
		.h_xhs		= 1,
		.slvs_eav_col	= 4248,
		.lane_num	= 4,
	},
	.table[4]	= {// mode 4: 4096X2160 10ch 10bit 59.94fps
		.ext_reg_fill	= NULL,
		.data		= {0x00, 0x80, 0x41, 0x47},
		.width		= 4096,
		.height		= 2160,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,	// CRR: FIXME
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 2,
		.xhs_clk	= 546,
		.h_xhs		= 1,
		.slvs_eav_col	= 4250,
		.lane_num	= 10,
	},
	.table[5]	= {// mode 5: 2x2 binning 2048x1080 4ch 12bit 59.94fps
		.ext_reg_fill	= NULL,
		.data		= {0x33, 0x89, 0x4D, 0x67},
		.width		= 2048,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 1,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.xhs_clk	= 528,
		.h_xhs		= 2,
		.slvs_eav_col	= 2124,
		.lane_num	= 4,
	},
	.table[6]	= {// mode 6: 4096X2160 8ch 10bit 29.97fps, not mentioned in datasheet
		.ext_reg_fill	= NULL,
		.data		= {0x11, 0x80, 0x41, 0x47},
		.width		= 4096,
		.height		= 2160,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.xhs_clk	= 1092,
		.h_xhs		= 1,
		.slvs_eav_col	= 4248,
		.lane_num	= 8,
	},
	.table[7]	= {// mode 7: 2x2 binning 2000x1126 8ch 10bit 120fps
		.ext_reg_fill	= NULL,
		.data		= {0x11, 0x40, 0x21, 0x04},
		.width		= 2000,
		.height		= 1126,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 1,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
		.xhs_clk	= 460,
		.h_xhs		= 1,
		.slvs_eav_col	= 2088,
		.lane_num	= 8,
	},
};

static const struct imx172_reg_table imx172_1080p120_mode_regs[] = {
	{IMX172_REG_03, 0x11},
	{IMX172_REG_MDSEL1, 0x40},
	{IMX172_REG_MDSEL2, 0x21},
	{IMX172_REG_MDSEL5_LSB, 0x17},
	{IMX172_REG_MDSEL5_MSB, 0x01},
	{IMX172_REG_MDPLS01, 0x12},
	{IMX172_REG_MDPLS03, 0x0C},
	{IMX172_REG_MDPLS05, 0xBE},
	{IMX172_REG_MDPLS06, 0x03},
	{IMX172_REG_MDPLS07, 0x7A},
	{IMX172_REG_MDPLS08, 0x04},
	{IMX172_REG_MNL_VADR08, 0X01},
	{IMX172_REG_MNL_VADR09, 0X07},
	{IMX172_REG_MNL_VADR14, 0X01},
	{IMX172_REG_MNL_VADR15, 0X07},
	{IMX172_REG_MNL_VADR20, 0X11},
	{IMX172_REG_MDPLS09, 0x14},
	{IMX172_REG_MDPLS11, 0x0C},
	{IMX172_REG_MDPLS13, 0xC0},
	{IMX172_REG_MDPLS14, 0x03},
	{IMX172_REG_MDPLS15, 0x7A},
	{IMX172_REG_MDPLS16, 0x04},
	{IMX172_REG_MNL_VADR29, 0X01},
	{IMX172_REG_MNL_VADR30, 0X07},
	{IMX172_REG_MNL_VADR35, 0X01},
	{IMX172_REG_MNL_VADR36, 0X07},
	{IMX172_REG_MNL_VADR41, 0X11},
	{IMX172_REG_MNL_VADR42, 0X04},
	{IMX172_REG_MNL_VADR43, 0X04},
	{IMX172_REG_MNL_VADR44, 0X04},
	{IMX172_REG_MNL_VADR45, 0X05},
	{IMX172_REG_MNL_VADR46, 0X04},
	{IMX172_REG_MNL_VADR47, 0X04},
	{IMX172_REG_MNL_VADR48, 0X04},
	{IMX172_REG_MNL_VADR49, 0X08},
	{IMX172_REG_MNL_VADR50, 0X08},
	{IMX172_REG_MNL_VADR51, 0X05},
	{IMX172_REG_MNL_VADR52, 0X08},
	{IMX172_REG_MNL_VADR53, 0X08},
	{IMX172_REG_MDSEL6, 0x04},
	{IMX172_REG_MDSEL7, 0x0A},
	{IMX172_REG_MDSEL8, 0x14},
	{IMX172_REG_MDSEL9, 0x04},
	{IMX172_REG_MDSEL10, 0x0A},
	{IMX172_REG_MDSEL11, 0x14},
	{IMX172_REG_MDPLS19, 0x09},
	{IMX172_REG_MDPLS21, 0x48},
	{IMX172_REG_MDPLS23, 0xBB},
	{IMX172_REG_MDPLS24, 0x01},
	{IMX172_REG_MDPLS25, 0xAC},
	{IMX172_REG_MDPLS26, 0x15},
	{IMX172_REG_MDPLS27, 0x9D},
	{IMX172_REG_MDPLS28, 0x01},
	{IMX172_REG_MDPLS33, 0x02},
	{IMX172_REG_MDSEL12, 0x10},
	{IMX172_REG_MNL_VADR83, 0X02},
};

#define IMX172_1080P120_REG_NUM	ARRAY_SIZE(imx172_1080p120_mode_regs)

#define IMX172_GAIN_ROWS	428
#define IMX172_GAIN_COLS	2

#define IMX172_GAIN_COL_AGC	0
#define IMX172_GAIN_COL_DGAIN	1
#define IMX172_GAIN_0DB 427

/* For digital gain, the step is 6dB, Max = 18dB */
const u16 IMX172_GAIN_TABLE[IMX172_GAIN_ROWS][IMX172_GAIN_COLS] = {
	{0x07a4, 0x03}, /* index 0, gain = 45.000000 DB */
	{0x07a3, 0x03}, /* index 1, gain = 44.894531 DB */
	{0x07a2, 0x03}, /* index 2, gain = 44.789062 DB */
	{0x07a1, 0x03}, /* index 3, gain = 44.683594 DB */
	{0x079f, 0x03}, /* index 4, gain = 44.578125 DB */
	{0x079e, 0x03}, /* index 5, gain = 44.472656 DB */
	{0x079d, 0x03}, /* index 6, gain = 44.367188 DB */
	{0x079c, 0x03}, /* index 7, gain = 44.261719 DB */
	{0x079b, 0x03}, /* index 8, gain = 44.156250 DB */
	{0x0799, 0x03}, /* index 9, gain = 44.050781 DB */
	{0x0798, 0x03}, /* index 10, gain = 43.945312 DB */
	{0x0797, 0x03}, /* index 11, gain = 43.839844 DB */
	{0x0796, 0x03}, /* index 12, gain = 43.734375 DB */
	{0x0794, 0x03}, /* index 13, gain = 43.628906 DB */
	{0x0793, 0x03}, /* index 14, gain = 43.523438 DB */
	{0x0792, 0x03}, /* index 15, gain = 43.417969 DB */
	{0x0790, 0x03}, /* index 16, gain = 43.312500 DB */
	{0x078f, 0x03}, /* index 17, gain = 43.207031 DB */
	{0x078e, 0x03}, /* index 18, gain = 43.101562 DB */
	{0x078c, 0x03}, /* index 19, gain = 42.996094 DB */
	{0x078b, 0x03}, /* index 20, gain = 42.890625 DB */
	{0x0789, 0x03}, /* index 21, gain = 42.785156 DB */
	{0x0788, 0x03}, /* index 22, gain = 42.679688 DB */
	{0x0787, 0x03}, /* index 23, gain = 42.574219 DB */
	{0x0785, 0x03}, /* index 24, gain = 42.468750 DB */
	{0x0784, 0x03}, /* index 25, gain = 42.363281 DB */
	{0x0782, 0x03}, /* index 26, gain = 42.257812 DB */
	{0x0781, 0x03}, /* index 27, gain = 42.152344 DB */
	{0x077f, 0x03}, /* index 28, gain = 42.046875 DB */
	{0x077d, 0x03}, /* index 29, gain = 41.941406 DB */
	{0x077c, 0x03}, /* index 30, gain = 41.835938 DB */
	{0x077a, 0x03}, /* index 31, gain = 41.730469 DB */
	{0x0779, 0x03}, /* index 32, gain = 41.625000 DB */
	{0x0777, 0x03}, /* index 33, gain = 41.519531 DB */
	{0x0775, 0x03}, /* index 34, gain = 41.414062 DB */
	{0x0774, 0x03}, /* index 35, gain = 41.308594 DB */
	{0x0772, 0x03}, /* index 36, gain = 41.203125 DB */
	{0x0770, 0x03}, /* index 37, gain = 41.097656 DB */
	{0x076e, 0x03}, /* index 38, gain = 40.992188 DB */
	{0x076d, 0x03}, /* index 39, gain = 40.886719 DB */
	{0x076b, 0x03}, /* index 40, gain = 40.781250 DB */
	{0x0769, 0x03}, /* index 41, gain = 40.675781 DB */
	{0x0767, 0x03}, /* index 42, gain = 40.570312 DB */
	{0x0765, 0x03}, /* index 43, gain = 40.464844 DB */
	{0x0763, 0x03}, /* index 44, gain = 40.359375 DB */
	{0x0762, 0x03}, /* index 45, gain = 40.253906 DB */
	{0x0760, 0x03}, /* index 46, gain = 40.148438 DB */
	{0x075e, 0x03}, /* index 47, gain = 40.042969 DB */
	{0x075c, 0x03}, /* index 48, gain = 39.937500 DB */
	{0x075a, 0x03}, /* index 49, gain = 39.832031 DB */
	{0x0758, 0x03}, /* index 50, gain = 39.726562 DB */
	{0x0756, 0x03}, /* index 51, gain = 39.621094 DB */
	{0x0753, 0x03}, /* index 52, gain = 39.515625 DB */
	{0x0751, 0x03}, /* index 53, gain = 39.410156 DB */
	{0x074f, 0x03}, /* index 54, gain = 39.304688 DB */
	{0x074d, 0x03}, /* index 55, gain = 39.199219 DB */
	{0x074b, 0x03}, /* index 56, gain = 39.093750 DB */
	{0x07a4, 0x02}, /* index 57, gain = 38.988281 DB */
	{0x07a3, 0x02}, /* index 58, gain = 38.882812 DB */
	{0x07a2, 0x02}, /* index 59, gain = 38.777344 DB */
	{0x07a0, 0x02}, /* index 60, gain = 38.671875 DB */
	{0x079f, 0x02}, /* index 61, gain = 38.566406 DB */
	{0x079e, 0x02}, /* index 62, gain = 38.460938 DB */
	{0x079d, 0x02}, /* index 63, gain = 38.355469 DB */
	{0x079c, 0x02}, /* index 64, gain = 38.250000 DB */
	{0x079b, 0x02}, /* index 65, gain = 38.144531 DB */
	{0x0799, 0x02}, /* index 66, gain = 38.039062 DB */
	{0x0798, 0x02}, /* index 67, gain = 37.933594 DB */
	{0x0797, 0x02}, /* index 68, gain = 37.828125 DB */
	{0x0796, 0x02}, /* index 69, gain = 37.722656 DB */
	{0x0794, 0x02}, /* index 70, gain = 37.617188 DB */
	{0x0793, 0x02}, /* index 71, gain = 37.511719 DB */
	{0x0792, 0x02}, /* index 72, gain = 37.406250 DB */
	{0x0790, 0x02}, /* index 73, gain = 37.300781 DB */
	{0x078f, 0x02}, /* index 74, gain = 37.195312 DB */
	{0x078e, 0x02}, /* index 75, gain = 37.089844 DB */
	{0x078c, 0x02}, /* index 76, gain = 36.984375 DB */
	{0x078b, 0x02}, /* index 77, gain = 36.878906 DB */
	{0x0789, 0x02}, /* index 78, gain = 36.773438 DB */
	{0x0788, 0x02}, /* index 79, gain = 36.667969 DB */
	{0x0786, 0x02}, /* index 80, gain = 36.562500 DB */
	{0x0785, 0x02}, /* index 81, gain = 36.457031 DB */
	{0x0783, 0x02}, /* index 82, gain = 36.351562 DB */
	{0x0782, 0x02}, /* index 83, gain = 36.246094 DB */
	{0x0780, 0x02}, /* index 84, gain = 36.140625 DB */
	{0x077f, 0x02}, /* index 85, gain = 36.035156 DB */
	{0x077d, 0x02}, /* index 86, gain = 35.929688 DB */
	{0x077c, 0x02}, /* index 87, gain = 35.824219 DB */
	{0x077a, 0x02}, /* index 88, gain = 35.718750 DB */
	{0x0778, 0x02}, /* index 89, gain = 35.613281 DB */
	{0x0777, 0x02}, /* index 90, gain = 35.507812 DB */
	{0x0775, 0x02}, /* index 91, gain = 35.402344 DB */
	{0x0773, 0x02}, /* index 92, gain = 35.296875 DB */
	{0x0772, 0x02}, /* index 93, gain = 35.191406 DB */
	{0x0770, 0x02}, /* index 94, gain = 35.085938 DB */
	{0x076e, 0x02}, /* index 95, gain = 34.980469 DB */
	{0x076c, 0x02}, /* index 96, gain = 34.875000 DB */
	{0x076b, 0x02}, /* index 97, gain = 34.769531 DB */
	{0x0769, 0x02}, /* index 98, gain = 34.664062 DB */
	{0x0767, 0x02}, /* index 99, gain = 34.558594 DB */
	{0x0765, 0x02}, /* index 100, gain = 34.453125 DB */
	{0x0763, 0x02}, /* index 101, gain = 34.347656 DB */
	{0x0761, 0x02}, /* index 102, gain = 34.242188 DB */
	{0x075f, 0x02}, /* index 103, gain = 34.136719 DB */
	{0x075d, 0x02}, /* index 104, gain = 34.031250 DB */
	{0x075b, 0x02}, /* index 105, gain = 33.925781 DB */
	{0x0759, 0x02}, /* index 106, gain = 33.820312 DB */
	{0x0757, 0x02}, /* index 107, gain = 33.714844 DB */
	{0x0755, 0x02}, /* index 108, gain = 33.609375 DB */
	{0x0753, 0x02}, /* index 109, gain = 33.503906 DB */
	{0x0751, 0x02}, /* index 110, gain = 33.398438 DB */
	{0x074f, 0x02}, /* index 111, gain = 33.292969 DB */
	{0x074d, 0x02}, /* index 112, gain = 33.187500 DB */
	{0x074b, 0x02}, /* index 113, gain = 33.082031 DB */
	{0x07a4, 0x01}, /* index 114, gain = 32.976562 DB */
	{0x07a3, 0x01}, /* index 115, gain = 32.871094 DB */
	{0x07a2, 0x01}, /* index 116, gain = 32.765625 DB */
	{0x07a0, 0x01}, /* index 117, gain = 32.660156 DB */
	{0x079f, 0x01}, /* index 118, gain = 32.554688 DB */
	{0x079e, 0x01}, /* index 119, gain = 32.449219 DB */
	{0x079d, 0x01}, /* index 120, gain = 32.343750 DB */
	{0x079c, 0x01}, /* index 121, gain = 32.238281 DB */
	{0x079a, 0x01}, /* index 122, gain = 32.132812 DB */
	{0x0799, 0x01}, /* index 123, gain = 32.027344 DB */
	{0x0798, 0x01}, /* index 124, gain = 31.921873 DB */
	{0x0797, 0x01}, /* index 125, gain = 31.816402 DB */
	{0x0795, 0x01}, /* index 126, gain = 31.710932 DB */
	{0x0794, 0x01}, /* index 127, gain = 31.605461 DB */
	{0x0793, 0x01}, /* index 128, gain = 31.499990 DB */
	{0x0791, 0x01}, /* index 129, gain = 31.394520 DB */
	{0x0790, 0x01}, /* index 130, gain = 31.289049 DB */
	{0x078f, 0x01}, /* index 131, gain = 31.183578 DB */
	{0x078d, 0x01}, /* index 132, gain = 31.078108 DB */
	{0x078c, 0x01}, /* index 133, gain = 30.972637 DB */
	{0x078b, 0x01}, /* index 134, gain = 30.867167 DB */
	{0x0789, 0x01}, /* index 135, gain = 30.761696 DB */
	{0x0788, 0x01}, /* index 136, gain = 30.656225 DB */
	{0x0786, 0x01}, /* index 137, gain = 30.550755 DB */
	{0x0785, 0x01}, /* index 138, gain = 30.445284 DB */
	{0x0783, 0x01}, /* index 139, gain = 30.339813 DB */
	{0x0782, 0x01}, /* index 140, gain = 30.234343 DB */
	{0x0780, 0x01}, /* index 141, gain = 30.128872 DB */
	{0x077f, 0x01}, /* index 142, gain = 30.023401 DB */
	{0x077d, 0x01}, /* index 143, gain = 29.917931 DB */
	{0x077b, 0x01}, /* index 144, gain = 29.812460 DB */
	{0x077a, 0x01}, /* index 145, gain = 29.706989 DB */
	{0x0778, 0x01}, /* index 146, gain = 29.601519 DB */
	{0x0777, 0x01}, /* index 147, gain = 29.496048 DB */
	{0x0775, 0x01}, /* index 148, gain = 29.390577 DB */
	{0x0773, 0x01}, /* index 149, gain = 29.285107 DB */
	{0x0771, 0x01}, /* index 150, gain = 29.179636 DB */
	{0x0770, 0x01}, /* index 151, gain = 29.074165 DB */
	{0x076e, 0x01}, /* index 152, gain = 28.968695 DB */
	{0x076c, 0x01}, /* index 153, gain = 28.863224 DB */
	{0x076a, 0x01}, /* index 154, gain = 28.757753 DB */
	{0x0769, 0x01}, /* index 155, gain = 28.652283 DB */
	{0x0767, 0x01}, /* index 156, gain = 28.546812 DB */
	{0x0765, 0x01}, /* index 157, gain = 28.441341 DB */
	{0x0763, 0x01}, /* index 158, gain = 28.335871 DB */
	{0x0761, 0x01}, /* index 159, gain = 28.230400 DB */
	{0x075f, 0x01}, /* index 160, gain = 28.124929 DB */
	{0x075d, 0x01}, /* index 161, gain = 28.019459 DB */
	{0x075b, 0x01}, /* index 162, gain = 27.913988 DB */
	{0x0759, 0x01}, /* index 163, gain = 27.808517 DB */
	{0x0757, 0x01}, /* index 164, gain = 27.703047 DB */
	{0x0755, 0x01}, /* index 165, gain = 27.597576 DB */
	{0x0753, 0x01}, /* index 166, gain = 27.492105 DB */
	{0x0751, 0x01}, /* index 167, gain = 27.386635 DB */
	{0x074f, 0x01}, /* index 168, gain = 27.281164 DB */
	{0x074d, 0x01}, /* index 169, gain = 27.175694 DB */
	{0x074a, 0x01}, /* index 170, gain = 27.070223 DB */
	{0x07a4, 0x00}, /* index 171, gain = 26.964752 DB */
	{0x07a3, 0x00}, /* index 172, gain = 26.859282 DB */
	{0x07a1, 0x00}, /* index 173, gain = 26.753811 DB */
	{0x07a0, 0x00}, /* index 174, gain = 26.648340 DB */
	{0x079f, 0x00}, /* index 175, gain = 26.542870 DB */
	{0x079e, 0x00}, /* index 176, gain = 26.437399 DB */
	{0x079d, 0x00}, /* index 177, gain = 26.331928 DB */
	{0x079b, 0x00}, /* index 178, gain = 26.226458 DB */
	{0x079a, 0x00}, /* index 179, gain = 26.120987 DB */
	{0x0799, 0x00}, /* index 180, gain = 26.015516 DB */
	{0x0798, 0x00}, /* index 181, gain = 25.910046 DB */
	{0x0797, 0x00}, /* index 182, gain = 25.804575 DB */
	{0x0795, 0x00}, /* index 183, gain = 25.699104 DB */
	{0x0794, 0x00}, /* index 184, gain = 25.593634 DB */
	{0x0793, 0x00}, /* index 185, gain = 25.488163 DB */
	{0x0791, 0x00}, /* index 186, gain = 25.382692 DB */
	{0x0790, 0x00}, /* index 187, gain = 25.277222 DB */
	{0x078f, 0x00}, /* index 188, gain = 25.171751 DB */
	{0x078d, 0x00}, /* index 189, gain = 25.066280 DB */
	{0x078c, 0x00}, /* index 190, gain = 24.960810 DB */
	{0x078a, 0x00}, /* index 191, gain = 24.855339 DB */
	{0x0789, 0x00}, /* index 192, gain = 24.749868 DB */
	{0x0788, 0x00}, /* index 193, gain = 24.644398 DB */
	{0x0786, 0x00}, /* index 194, gain = 24.538927 DB */
	{0x0785, 0x00}, /* index 195, gain = 24.433456 DB */
	{0x0783, 0x00}, /* index 196, gain = 24.327986 DB */
	{0x0782, 0x00}, /* index 197, gain = 24.222515 DB */
	{0x0780, 0x00}, /* index 198, gain = 24.117044 DB */
	{0x077e, 0x00}, /* index 199, gain = 24.011574 DB */
	{0x077d, 0x00}, /* index 200, gain = 23.906103 DB */
	{0x077b, 0x00}, /* index 201, gain = 23.800632 DB */
	{0x077a, 0x00}, /* index 202, gain = 23.695162 DB */
	{0x0778, 0x00}, /* index 203, gain = 23.589691 DB */
	{0x0776, 0x00}, /* index 204, gain = 23.484221 DB */
	{0x0775, 0x00}, /* index 205, gain = 23.378750 DB */
	{0x0773, 0x00}, /* index 206, gain = 23.273279 DB */
	{0x0771, 0x00}, /* index 207, gain = 23.167809 DB */
	{0x0770, 0x00}, /* index 208, gain = 23.062338 DB */
	{0x076e, 0x00}, /* index 209, gain = 22.956867 DB */
	{0x076c, 0x00}, /* index 210, gain = 22.851397 DB */
	{0x076a, 0x00}, /* index 211, gain = 22.745926 DB */
	{0x0768, 0x00}, /* index 212, gain = 22.640455 DB */
	{0x0767, 0x00}, /* index 213, gain = 22.534985 DB */
	{0x0765, 0x00}, /* index 214, gain = 22.429514 DB */
	{0x0763, 0x00}, /* index 215, gain = 22.324043 DB */
	{0x0761, 0x00}, /* index 216, gain = 22.218573 DB */
	{0x075f, 0x00}, /* index 217, gain = 22.113102 DB */
	{0x075d, 0x00}, /* index 218, gain = 22.007631 DB */
	{0x075b, 0x00}, /* index 219, gain = 21.902161 DB */
	{0x0759, 0x00}, /* index 220, gain = 21.796690 DB */
	{0x0757, 0x00}, /* index 221, gain = 21.691219 DB */
	{0x0755, 0x00}, /* index 222, gain = 21.585749 DB */
	{0x0753, 0x00}, /* index 223, gain = 21.480278 DB */
	{0x0751, 0x00}, /* index 224, gain = 21.374807 DB */
	{0x074f, 0x00}, /* index 225, gain = 21.269337 DB */
	{0x074c, 0x00}, /* index 226, gain = 21.163866 DB */
	{0x074a, 0x00}, /* index 227, gain = 21.058395 DB */
	{0x0748, 0x00}, /* index 228, gain = 20.952925 DB */
	{0x0746, 0x00}, /* index 229, gain = 20.847454 DB */
	{0x0743, 0x00}, /* index 230, gain = 20.741983 DB */
	{0x0741, 0x00}, /* index 231, gain = 20.636513 DB */
	{0x073f, 0x00}, /* index 232, gain = 20.531042 DB */
	{0x073c, 0x00}, /* index 233, gain = 20.425571 DB */
	{0x073a, 0x00}, /* index 234, gain = 20.320101 DB */
	{0x0738, 0x00}, /* index 235, gain = 20.214630 DB */
	{0x0735, 0x00}, /* index 236, gain = 20.109159 DB */
	{0x0733, 0x00}, /* index 237, gain = 20.003689 DB */
	{0x0730, 0x00}, /* index 238, gain = 19.898218 DB */
	{0x072e, 0x00}, /* index 239, gain = 19.792747 DB */
	{0x072b, 0x00}, /* index 240, gain = 19.687277 DB */
	{0x0729, 0x00}, /* index 241, gain = 19.581806 DB */
	{0x0726, 0x00}, /* index 242, gain = 19.476336 DB */
	{0x0723, 0x00}, /* index 243, gain = 19.370865 DB */
	{0x0721, 0x00}, /* index 244, gain = 19.265394 DB */
	{0x071e, 0x00}, /* index 245, gain = 19.159924 DB */
	{0x071b, 0x00}, /* index 246, gain = 19.054453 DB */
	{0x0718, 0x00}, /* index 247, gain = 18.948982 DB */
	{0x0716, 0x00}, /* index 248, gain = 18.843512 DB */
	{0x0713, 0x00}, /* index 249, gain = 18.738041 DB */
	{0x0710, 0x00}, /* index 250, gain = 18.632570 DB */
	{0x070d, 0x00}, /* index 251, gain = 18.527100 DB */
	{0x070a, 0x00}, /* index 252, gain = 18.421629 DB */
	{0x0707, 0x00}, /* index 253, gain = 18.316158 DB */
	{0x0704, 0x00}, /* index 254, gain = 18.210688 DB */
	{0x0701, 0x00}, /* index 255, gain = 18.105217 DB */
	{0x06fe, 0x00}, /* index 256, gain = 17.999746 DB */
	{0x06fb, 0x00}, /* index 257, gain = 17.894276 DB */
	{0x06f7, 0x00}, /* index 258, gain = 17.788805 DB */
	{0x06f4, 0x00}, /* index 259, gain = 17.683334 DB */
	{0x06f1, 0x00}, /* index 260, gain = 17.577864 DB */
	{0x06ee, 0x00}, /* index 261, gain = 17.472393 DB */
	{0x06ea, 0x00}, /* index 262, gain = 17.366922 DB */
	{0x06e7, 0x00}, /* index 263, gain = 17.261452 DB */
	{0x06e3, 0x00}, /* index 264, gain = 17.155981 DB */
	{0x06e0, 0x00}, /* index 265, gain = 17.050510 DB */
	{0x06dc, 0x00}, /* index 266, gain = 16.945040 DB */
	{0x06d9, 0x00}, /* index 267, gain = 16.839569 DB */
	{0x06d5, 0x00}, /* index 268, gain = 16.734098 DB */
	{0x06d2, 0x00}, /* index 269, gain = 16.628628 DB */
	{0x06ce, 0x00}, /* index 270, gain = 16.523157 DB */
	{0x06ca, 0x00}, /* index 271, gain = 16.417686 DB */
	{0x06c6, 0x00}, /* index 272, gain = 16.312216 DB */
	{0x06c3, 0x00}, /* index 273, gain = 16.206745 DB */
	{0x06bf, 0x00}, /* index 274, gain = 16.101274 DB */
	{0x06bb, 0x00}, /* index 275, gain = 15.995805 DB */
	{0x06b7, 0x00}, /* index 276, gain = 15.890335 DB */
	{0x06b3, 0x00}, /* index 277, gain = 15.784865 DB */
	{0x06af, 0x00}, /* index 278, gain = 15.679396 DB */
	{0x06ab, 0x00}, /* index 279, gain = 15.573926 DB */
	{0x06a6, 0x00}, /* index 280, gain = 15.468456 DB */
	{0x06a2, 0x00}, /* index 281, gain = 15.362987 DB */
	{0x069e, 0x00}, /* index 282, gain = 15.257517 DB */
	{0x069a, 0x00}, /* index 283, gain = 15.152047 DB */
	{0x0695, 0x00}, /* index 284, gain = 15.046577 DB */
	{0x0691, 0x00}, /* index 285, gain = 14.941108 DB */
	{0x068c, 0x00}, /* index 286, gain = 14.835638 DB */
	{0x0688, 0x00}, /* index 287, gain = 14.730168 DB */
	{0x0683, 0x00}, /* index 288, gain = 14.624699 DB */
	{0x067f, 0x00}, /* index 289, gain = 14.519229 DB */
	{0x067a, 0x00}, /* index 290, gain = 14.413759 DB */
	{0x0675, 0x00}, /* index 291, gain = 14.308290 DB */
	{0x0670, 0x00}, /* index 292, gain = 14.202820 DB */
	{0x066b, 0x00}, /* index 293, gain = 14.097350 DB */
	{0x0666, 0x00}, /* index 294, gain = 13.991880 DB */
	{0x0661, 0x00}, /* index 295, gain = 13.886411 DB */
	{0x065c, 0x00}, /* index 296, gain = 13.780941 DB */
	{0x0657, 0x00}, /* index 297, gain = 13.675471 DB */
	{0x0652, 0x00}, /* index 298, gain = 13.570002 DB */
	{0x064d, 0x00}, /* index 299, gain = 13.464532 DB */
	{0x0648, 0x00}, /* index 300, gain = 13.359062 DB */
	{0x0642, 0x00}, /* index 301, gain = 13.253592 DB */
	{0x063d, 0x00}, /* index 302, gain = 13.148123 DB */
	{0x0637, 0x00}, /* index 303, gain = 13.042653 DB */
	{0x0632, 0x00}, /* index 304, gain = 12.937183 DB */
	{0x062c, 0x00}, /* index 305, gain = 12.831714 DB */
	{0x0626, 0x00}, /* index 306, gain = 12.726244 DB */
	{0x0621, 0x00}, /* index 307, gain = 12.620774 DB */
	{0x061b, 0x00}, /* index 308, gain = 12.515305 DB */
	{0x0615, 0x00}, /* index 309, gain = 12.409835 DB */
	{0x060f, 0x00}, /* index 310, gain = 12.304365 DB */
	{0x0609, 0x00}, /* index 311, gain = 12.198895 DB */
	{0x0603, 0x00}, /* index 312, gain = 12.093426 DB */
	{0x05fc, 0x00}, /* index 313, gain = 11.987956 DB */
	{0x05f6, 0x00}, /* index 314, gain = 11.882486 DB */
	{0x05f0, 0x00}, /* index 315, gain = 11.777017 DB */
	{0x05e9, 0x00}, /* index 316, gain = 11.671547 DB */
	{0x05e3, 0x00}, /* index 317, gain = 11.566077 DB */
	{0x05dc, 0x00}, /* index 318, gain = 11.460608 DB */
	{0x05d5, 0x00}, /* index 319, gain = 11.355138 DB */
	{0x05cf, 0x00}, /* index 320, gain = 11.249668 DB */
	{0x05c8, 0x00}, /* index 321, gain = 11.144198 DB */
	{0x05c1, 0x00}, /* index 322, gain = 11.038729 DB */
	{0x05ba, 0x00}, /* index 323, gain = 10.933259 DB */
	{0x05b3, 0x00}, /* index 324, gain = 10.827789 DB */
	{0x05ac, 0x00}, /* index 325, gain = 10.722320 DB */
	{0x05a4, 0x00}, /* index 326, gain = 10.616850 DB */
	{0x059d, 0x00}, /* index 327, gain = 10.511380 DB */
	{0x0595, 0x00}, /* index 328, gain = 10.405910 DB */
	{0x058e, 0x00}, /* index 329, gain = 10.300441 DB */
	{0x0586, 0x00}, /* index 330, gain = 10.194971 DB */
	{0x057f, 0x00}, /* index 331, gain = 10.089501 DB */
	{0x0577, 0x00}, /* index 332, gain = 9.984032 DB */
	{0x056f, 0x00}, /* index 333, gain = 9.878562 DB */
	{0x0567, 0x00}, /* index 334, gain = 9.773092 DB */
	{0x055f, 0x00}, /* index 335, gain = 9.667623 DB */
	{0x0556, 0x00}, /* index 336, gain = 9.562153 DB */
	{0x054e, 0x00}, /* index 337, gain = 9.456683 DB */
	{0x0546, 0x00}, /* index 338, gain = 9.351213 DB */
	{0x053d, 0x00}, /* index 339, gain = 9.245744 DB */
	{0x0534, 0x00}, /* index 340, gain = 9.140274 DB */
	{0x052c, 0x00}, /* index 341, gain = 9.034804 DB */
	{0x0523, 0x00}, /* index 342, gain = 8.929335 DB */
	{0x051a, 0x00}, /* index 343, gain = 8.823865 DB */
	{0x0511, 0x00}, /* index 344, gain = 8.718395 DB */
	{0x0508, 0x00}, /* index 345, gain = 8.612926 DB */
	{0x04fe, 0x00}, /* index 346, gain = 8.507456 DB */
	{0x04f5, 0x00}, /* index 347, gain = 8.401986 DB */
	{0x04ec, 0x00}, /* index 348, gain = 8.296516 DB */
	{0x04e2, 0x00}, /* index 349, gain = 8.191047 DB */
	{0x04d8, 0x00}, /* index 350, gain = 8.085577 DB */
	{0x04ce, 0x00}, /* index 351, gain = 7.980107 DB */
	{0x04c4, 0x00}, /* index 352, gain = 7.874637 DB */
	{0x04ba, 0x00}, /* index 353, gain = 7.769166 DB */
	{0x04b0, 0x00}, /* index 354, gain = 7.663696 DB */
	{0x04a6, 0x00}, /* index 355, gain = 7.558226 DB */
	{0x049b, 0x00}, /* index 356, gain = 7.452756 DB */
	{0x0491, 0x00}, /* index 357, gain = 7.347286 DB */
	{0x0486, 0x00}, /* index 358, gain = 7.241816 DB */
	{0x047b, 0x00}, /* index 359, gain = 7.136345 DB */
	{0x0470, 0x00}, /* index 360, gain = 7.030875 DB */
	{0x0465, 0x00}, /* index 361, gain = 6.925405 DB */
	{0x045a, 0x00}, /* index 362, gain = 6.819935 DB */
	{0x044e, 0x00}, /* index 363, gain = 6.714465 DB */
	{0x0443, 0x00}, /* index 364, gain = 6.608994 DB */
	{0x0437, 0x00}, /* index 365, gain = 6.503524 DB */
	{0x042b, 0x00}, /* index 366, gain = 6.398054 DB */
	{0x041f, 0x00}, /* index 367, gain = 6.292584 DB */
	{0x0413, 0x00}, /* index 368, gain = 6.187114 DB */
	{0x0407, 0x00}, /* index 369, gain = 6.081644 DB */
	{0x03fa, 0x00}, /* index 370, gain = 5.976173 DB */
	{0x03ee, 0x00}, /* index 371, gain = 5.870703 DB */
	{0x03e1, 0x00}, /* index 372, gain = 5.765233 DB */
	{0x03d4, 0x00}, /* index 373, gain = 5.659763 DB */
	{0x03c7, 0x00}, /* index 374, gain = 5.554293 DB */
	{0x03ba, 0x00}, /* index 375, gain = 5.448822 DB */
	{0x03ac, 0x00}, /* index 376, gain = 5.343352 DB */
	{0x039f, 0x00}, /* index 377, gain = 5.237882 DB */
	{0x0391, 0x00}, /* index 378, gain = 5.132412 DB */
	{0x0383, 0x00}, /* index 379, gain = 5.026942 DB */
	{0x0375, 0x00}, /* index 380, gain = 4.921472 DB */
	{0x0367, 0x00}, /* index 381, gain = 4.816001 DB */
	{0x0359, 0x00}, /* index 382, gain = 4.710531 DB */
	{0x034a, 0x00}, /* index 383, gain = 4.605061 DB */
	{0x033c, 0x00}, /* index 384, gain = 4.499591 DB */
	{0x032d, 0x00}, /* index 385, gain = 4.394121 DB */
	{0x031e, 0x00}, /* index 386, gain = 4.288651 DB */
	{0x030e, 0x00}, /* index 387, gain = 4.183180 DB */
	{0x02ff, 0x00}, /* index 388, gain = 4.077710 DB */
	{0x02ef, 0x00}, /* index 389, gain = 3.972240 DB */
	{0x02df, 0x00}, /* index 390, gain = 3.866770 DB */
	{0x02cf, 0x00}, /* index 391, gain = 3.761300 DB */
	{0x02bf, 0x00}, /* index 392, gain = 3.655830 DB */
	{0x02af, 0x00}, /* index 393, gain = 3.550360 DB */
	{0x029e, 0x00}, /* index 394, gain = 3.444890 DB */
	{0x028d, 0x00}, /* index 395, gain = 3.339421 DB */
	{0x027c, 0x00}, /* index 396, gain = 3.233951 DB */
	{0x026b, 0x00}, /* index 397, gain = 3.128481 DB */
	{0x0259, 0x00}, /* index 398, gain = 3.023011 DB */
	{0x0248, 0x00}, /* index 399, gain = 2.917541 DB */
	{0x0236, 0x00}, /* index 400, gain = 2.812071 DB */
	{0x0224, 0x00}, /* index 401, gain = 2.706601 DB */
	{0x0211, 0x00}, /* index 402, gain = 2.601131 DB */
	{0x01ff, 0x00}, /* index 403, gain = 2.495661 DB */
	{0x01ec, 0x00}, /* index 404, gain = 2.390191 DB */
	{0x01d9, 0x00}, /* index 405, gain = 2.284721 DB */
	{0x01c6, 0x00}, /* index 406, gain = 2.179251 DB */
	{0x01b2, 0x00}, /* index 407, gain = 2.073781 DB */
	{0x019f, 0x00}, /* index 408, gain = 1.968311 DB */
	{0x018b, 0x00}, /* index 409, gain = 1.862841 DB */
	{0x0177, 0x00}, /* index 410, gain = 1.757371 DB */
	{0x0162, 0x00}, /* index 411, gain = 1.651901 DB */
	{0x014e, 0x00}, /* index 412, gain = 1.546432 DB */
	{0x0139, 0x00}, /* index 413, gain = 1.440962 DB */
	{0x0123, 0x00}, /* index 414, gain = 1.335492 DB */
	{0x010e, 0x00}, /* index 415, gain = 1.230022 DB */
	{0x00f8, 0x00}, /* index 416, gain = 1.124552 DB */
	{0x00e2, 0x00}, /* index 417, gain = 1.019082 DB */
	{0x00cc, 0x00}, /* index 418, gain = 0.913612 DB */
	{0x00b5, 0x00}, /* index 419, gain = 0.808142 DB */
	{0x009f, 0x00}, /* index 420, gain = 0.702672 DB */
	{0x0088, 0x00}, /* index 421, gain = 0.597202 DB */
	{0x0070, 0x00}, /* index 422, gain = 0.491732 DB */
	{0x0059, 0x00}, /* index 423, gain = 0.386262 DB */
	{0x0041, 0x00}, /* index 424, gain = 0.280792 DB */
	{0x0028, 0x00}, /* index 425, gain = 0.175322 DB */
	{0x0010, 0x00}, /* index 426, gain = 0.069852 DB */
	{0x0000, 0x00}, /* index 427, gain = 0.000000 DB */
};

