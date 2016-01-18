/*
 * kernel/private/drivers/ambarella/vin/sensors/omnivision_ov4689/ov4689_reg_tbl.c
 *
 * History:
 *    2013/05/29 - [Long Zhao] Create
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
static const struct ov4689_video_format_reg_table ov4689_video_format_tbl = {
	.reg = {
	},
	.table[0] = {		//2688x1520(10bits)@60fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2688,
		.height		= 1520,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
	.table[1] = {		// 2x hdr mode: 2688x1920x2(10bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2688,
		.height		= 1920 * 2,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 1,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 0,
			.short1_start_x = 0,
			.short1_start_y = 0x180 * 2 + 1, /* MAX_MIDDLE * 2 + 1 */
		},
	},
	.table[2] = {		// 3x hdr mode: 2368x1558x3(10bits)@30fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2368,
		.height		= 1558 * 3,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 2,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 0,
			.short1_start_x = 0,
			.short1_start_y = 0x80 * 3 + 1, /* MAX_MIDDLE * 3 + 1 */
			.short2_start_x = 0,
			.short2_start_y = (0x80 * 3 + 1) + (0x40 * 3 + 1), /* .short1_start_y + MAX_SHORT * 3 + 1 */
		},
	},
	.table[3] = {		// 2 lane 1080p@60fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 3,
	},
	.table[4] = {		// 2 lane 720p@150fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
#if (CHIP_REV == A5S) // A5s must double the line length for current idsp performance
		.max_fps	= AMBA_VIDEO_FPS(75),
#else
		.max_fps	= AMBA_VIDEO_FPS(150),
#endif
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 4,
	},
	.table[5] = {		// 3x 4M hdr mode: 2688x1520x3(10bits)@15fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2688,
		.height		= 1520 * 3,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_15,
		.auto_fps	= AMBA_VIDEO_FPS_15,
		.pll_index	= 2,
		.hdr_win_offset = {
			.long_start_x = 0,
			.long_start_y = 0,
			.short1_start_x = 0,
			.short1_start_y = 0x1C0 * 3 + 1, /* MAX_MIDDLE * 3 + 1 */
			.short2_start_x = 0,
			.short2_start_y = (0x1C0 * 3+ 1) + (0x40 * 3 + 1), /* .short1_start_y + MAX_SHORT * 3 + 1 */
		},
	},
	/* add video format table here, if necessary */
};


/* Gain table */
/* OV4689 global gain table row size */
#define OV4689_GAIN_ROWS               (385)
#define OV4689_GAIN_COLS               (5)
#define OV4689_GAIN_0DB                (384)

#define OV4689_GAIN_COL_AGC            (0)
#define OV4689_GAIN_COL_REG3508        (1)
#define OV4689_GAIN_COL_REG3509        (2)
#define OV4689_GAIN_COL_DGAIN_MSB      (3)
#define OV4689_GAIN_COL_DGAIN_LSB      (4)


/* This is 64-step gain table, OV4689_GAIN_ROWS = 381, OV4689_GAIN_COLS = 5 */
const s16 OV4689_GAIN_TABLE[OV4689_GAIN_ROWS][OV4689_GAIN_COLS] = {
	{0x3E00, 0x07, 0xF0, 0x0f, 0xff}, /* index   0,  gain = 35.8478 dB, actual gain = 35.8478 dB, */
	{0x3D84, 0x07, 0xF0, 0x0f, 0xe0}, /* index   1,  */
	{0x3D08, 0x07, 0xF0, 0x0f, 0xc0}, /* index   2,  */
	{0x3C8C, 0x07, 0xF0, 0x0f, 0xa0}, /* index   3,  */
	{0x3C10, 0x07, 0xF0, 0x0f, 0x80}, /* index   4,  */
	{0x3B94, 0x07, 0xF0, 0x0f, 0x60}, /* index   5,  */
	{0x3B18, 0x07, 0xF0, 0x0f, 0x40}, /* index   6,  */
	{0x3A9C, 0x07, 0xF0, 0x0f, 0x20}, /* index   7,  */
	{0x3A20, 0x07, 0xF0, 0x0f, 0x00}, /* index   8,  */
	{0x39A4, 0x07, 0xF0, 0x0e, 0xe0}, /* index   9,  */
	{0x3928, 0x07, 0xF0, 0x0e, 0xc0}, /* index  10,  */
	{0x38AC, 0x07, 0xF0, 0x0e, 0xa0}, /* index  11,  */
	{0x3830, 0x07, 0xF0, 0x0e, 0x80}, /* index  12,  */
	{0x37B4, 0x07, 0xF0, 0x0e, 0x60}, /* index  13,  */
	{0x3738, 0x07, 0xF0, 0x0e, 0x40}, /* index  14,  */
	{0x36BC, 0x07, 0xF0, 0x0e, 0x20}, /* index  15,  */
	{0x3640, 0x07, 0xF0, 0x0e, 0x00}, /* index  16,  */
	{0x35C4, 0x07, 0xF0, 0x0d, 0xe0}, /* index  17,  */
	{0x3548, 0x07, 0xF0, 0x0d, 0xc0}, /* index  18,  */
	{0x34CC, 0x07, 0xF0, 0x0d, 0xa0}, /* index  19,  */
	{0x3450, 0x07, 0xF0, 0x0d, 0x80}, /* index  20,  */
	{0x33D4, 0x07, 0xF0, 0x0d, 0x60}, /* index  21,  */
	{0x3358, 0x07, 0xF0, 0x0d, 0x40}, /* index  22,  */
	{0x32DC, 0x07, 0xF0, 0x0d, 0x20}, /* index  23,  */
	{0x3260, 0x07, 0xF0, 0x0d, 0x00}, /* index  24,  */
	{0x31E4, 0x07, 0xF0, 0x0c, 0xe0}, /* index  25,  */
	{0x3168, 0x07, 0xF0, 0x0c, 0xc0}, /* index  26,  */
	{0x30EC, 0x07, 0xF0, 0x0c, 0xa0}, /* index  27,  */
	{0x3070, 0x07, 0xF0, 0x0c, 0x80}, /* index  28,  */
	{0x2FF4, 0x07, 0xF0, 0x0c, 0x60}, /* index  29,  */
	{0x2F78, 0x07, 0xF0, 0x0c, 0x40}, /* index  30,  */
	{0x2EFC, 0x07, 0xF0, 0x0c, 0x20}, /* index  31,  */
	{0x2E80, 0x07, 0xF0, 0x0c, 0x00}, /* index  32,  */
	{0x2E04, 0x07, 0xF0, 0x0b, 0xe0}, /* index  33,  */
	{0x2D88, 0x07, 0xF0, 0x0b, 0xc0}, /* index  34,  */
	{0x2D0C, 0x07, 0xF0, 0x0b, 0xa0}, /* index  35,  */
	{0x2C90, 0x07, 0xF0, 0x0b, 0x80}, /* index  36,  */
	{0x2C14, 0x07, 0xF0, 0x0b, 0x60}, /* index  37,  */
	{0x2B98, 0x07, 0xF0, 0x0b, 0x40}, /* index  38,  */
	{0x2B1C, 0x07, 0xF0, 0x0b, 0x20}, /* index  39,  */
	{0x2AA0, 0x07, 0xF0, 0x0b, 0x00}, /* index  40,  */
	{0x2A24, 0x07, 0xF0, 0x0a, 0xe0}, /* index  41,  */
	{0x29A8, 0x07, 0xF0, 0x0a, 0xc0}, /* index  42,  */
	{0x292C, 0x07, 0xF0, 0x0a, 0xa0}, /* index  43,  */
	{0x28B0, 0x07, 0xF0, 0x0a, 0x80}, /* index  44,  */
	{0x2834, 0x07, 0xF0, 0x0a, 0x60}, /* index  45,  */
	{0x27B8, 0x07, 0xF0, 0x0a, 0x40}, /* index  46,  */
	{0x273C, 0x07, 0xF0, 0x0a, 0x20}, /* index  47,  */
	{0x26C0, 0x07, 0xF0, 0x0a, 0x00}, /* index  48,  */
	{0x2644, 0x07, 0xF0, 0x09, 0xe0}, /* index  49,  */
	{0x25C8, 0x07, 0xF0, 0x09, 0xc0}, /* index  50,  */
	{0x254C, 0x07, 0xF0, 0x09, 0xa0}, /* index  51,  */
	{0x24D0, 0x07, 0xF0, 0x09, 0x80}, /* index  52,  */
	{0x2454, 0x07, 0xF0, 0x09, 0x60}, /* index  53,  */
	{0x23D8, 0x07, 0xF0, 0x09, 0x40}, /* index  54,  */
	{0x235C, 0x07, 0xF0, 0x09, 0x20}, /* index  55,  */
	{0x22E0, 0x07, 0xF0, 0x09, 0x00}, /* index  56,  */
	{0x2264, 0x07, 0xF0, 0x08, 0xe0}, /* index  57,  */
	{0x21E8, 0x07, 0xF0, 0x08, 0xc0}, /* index  58,  */
	{0x216C, 0x07, 0xF0, 0x08, 0xa0}, /* index  59,  */
	{0x20F0, 0x07, 0xF0, 0x08, 0x80}, /* index  60,  gain = 30.1030 dB, actual gain = 30.3538 dB, */
	{0x2074, 0x07, 0xF0, 0x08, 0x60}, /* index  61,  */
	{0x1FF8, 0x07, 0xF0, 0x08, 0x40}, /* index  62,  */
	{0x1F7C, 0x07, 0xF0, 0x08, 0x20}, /* index  63,  */
	{0x1F00, 0x07, 0xF0, 0x08, 0x00}, /* index  64,  */
	{0x1EC2, 0x07, 0xF0, 0x07, 0xf0}, /* index  65,  */
	{0x1E84, 0x07, 0xF0, 0x07, 0xe0}, /* index  66,  */
	{0x1E46, 0x07, 0xF0, 0x07, 0xd0}, /* index  67,  */
	{0x1E08, 0x07, 0xF0, 0x07, 0xc0}, /* index  68,  */
	{0x1DCA, 0x07, 0xF0, 0x07, 0xb0}, /* index  69,  */
	{0x1D8C, 0x07, 0xF0, 0x07, 0xa0}, /* index  70,  */
	{0x1D4E, 0x07, 0xF0, 0x07, 0x90}, /* index  71,  */
	{0x1D10, 0x07, 0xF0, 0x07, 0x80}, /* index  72,  */
	{0x1CD2, 0x07, 0xF0, 0x07, 0x70}, /* index  73,  */
	{0x1C94, 0x07, 0xF0, 0x07, 0x60}, /* index  74,  */
	{0x1C56, 0x07, 0xF0, 0x07, 0x50}, /* index  75,  */
	{0x1C18, 0x07, 0xF0, 0x07, 0x40}, /* index  76,  */
	{0x1BDA, 0x07, 0xF0, 0x07, 0x30}, /* index  77,  */
	{0x1B9C, 0x07, 0xF0, 0x07, 0x20}, /* index  78,  */
	{0x1B5E, 0x07, 0xF0, 0x07, 0x10}, /* index  79,  */
	{0x1B20, 0x07, 0xF0, 0x07, 0x00}, /* index  80,  */
	{0x1AE2, 0x07, 0xF0, 0x06, 0xf0}, /* index  81,  */
	{0x1AA4, 0x07, 0xF0, 0x06, 0xe0}, /* index  82,  */
	{0x1A66, 0x07, 0xF0, 0x06, 0xd0}, /* index  83,  */
	{0x1A28, 0x07, 0xF0, 0x06, 0xc0}, /* index  84,  */
	{0x19EA, 0x07, 0xF0, 0x06, 0xb0}, /* index  85,  */
	{0x19AC, 0x07, 0xF0, 0x06, 0xa0}, /* index  86,  */
	{0x196E, 0x07, 0xF0, 0x06, 0x90}, /* index  87,  */
	{0x1930, 0x07, 0xF0, 0x06, 0x80}, /* index  88,  */
	{0x18F2, 0x07, 0xF0, 0x06, 0x70}, /* index  89,  */
	{0x18B4, 0x07, 0xF0, 0x06, 0x60}, /* index  90,  */
	{0x1876, 0x07, 0xF0, 0x06, 0x50}, /* index  91,  */
	{0x1838, 0x07, 0xF0, 0x06, 0x40}, /* index  92,  */
	{0x17FA, 0x07, 0xF0, 0x06, 0x30}, /* index  93,  */
	{0x17BC, 0x07, 0xF0, 0x06, 0x20}, /* index  94,  */
	{0x177E, 0x07, 0xF0, 0x06, 0x10}, /* index  95,  */
	{0x1740, 0x07, 0xF0, 0x06, 0x00}, /* index  96,  */
	{0x1702, 0x07, 0xF0, 0x05, 0xf0}, /* index  97,  */
	{0x16C4, 0x07, 0xF0, 0x05, 0xe0}, /* index  98,  */
	{0x1686, 0x07, 0xF0, 0x05, 0xd0}, /* index  99,  */
	{0x1648, 0x07, 0xF0, 0x05, 0xc0}, /* index 100,  */
	{0x160A, 0x07, 0xF0, 0x05, 0xb0}, /* index 101,  */
	{0x15CC, 0x07, 0xF0, 0x05, 0xa0}, /* index 102,  */
	{0x158E, 0x07, 0xF0, 0x05, 0x90}, /* index 103,  */
	{0x1550, 0x07, 0xF0, 0x05, 0x80}, /* index 104,  */
	{0x1512, 0x07, 0xF0, 0x05, 0x70}, /* index 105,  */
	{0x14D4, 0x07, 0xF0, 0x05, 0x60}, /* index 106,  */
	{0x1496, 0x07, 0xF0, 0x05, 0x50}, /* index 107,  */
	{0x1458, 0x07, 0xF0, 0x05, 0x40}, /* index 108,  */
	{0x141A, 0x07, 0xF0, 0x05, 0x30}, /* index 109,  */
	{0x13DC, 0x07, 0xF0, 0x05, 0x20}, /* index 110,  */
	{0x139E, 0x07, 0xF0, 0x05, 0x10}, /* index 111,  */
	{0x1360, 0x07, 0xF0, 0x05, 0x00}, /* index 112,  */
	{0x1322, 0x07, 0xF0, 0x04, 0xf0}, /* index 113,  */
	{0x12E4, 0x07, 0xF0, 0x04, 0xe0}, /* index 114,  */
	{0x12A6, 0x07, 0xF0, 0x04, 0xd0}, /* index 115,  */
	{0x1268, 0x07, 0xF0, 0x04, 0xc0}, /* index 116,  */
	{0x122A, 0x07, 0xF0, 0x04, 0xb0}, /* index 117,  */
	{0x11EC, 0x07, 0xF0, 0x04, 0xa0}, /* index 118,  */
	{0x11AE, 0x07, 0xF0, 0x04, 0x90}, /* index 119,  */
	{0x1170, 0x07, 0xF0, 0x04, 0x80}, /* index 120,  */
	{0x1132, 0x07, 0xF0, 0x04, 0x70}, /* index 121,  */
	{0x10F4, 0x07, 0xF0, 0x04, 0x60}, /* index 122,  */
	{0x10B6, 0x07, 0xF0, 0x04, 0x50}, /* index 123,  */
	{0x1078, 0x07, 0xF0, 0x04, 0x40}, /* index 124,  gain = 24.0824 dB, actual gain = 24.3332 dB, */
	{0x103A, 0x07, 0xF0, 0x04, 0x30}, /* index 125,  */
	{0x0FFC, 0x07, 0xF0, 0x04, 0x20}, /* index 126,  */
	{0x0FBE, 0x07, 0xF0, 0x04, 0x10}, /* index 127,  */
	{0x0F80, 0x07, 0xF0, 0x04, 0x00}, /* index 128,  */
	{0x0F60, 0x07, 0xEE, 0x04, 0x00}, /* index 129,  */
	{0x0F40, 0x07, 0xEC, 0x04, 0x00}, /* index 130,  */
	{0x0F20, 0x07, 0xEA, 0x04, 0x00}, /* index 131,  */
	{0x0F00, 0x07, 0xE8, 0x04, 0x00}, /* index 132,  */
	{0x0EE0, 0x07, 0xE6, 0x04, 0x00}, /* index 133,  */
	{0x0EC0, 0x07, 0xE4, 0x04, 0x00}, /* index 134,  */
	{0x0EA0, 0x07, 0xE2, 0x04, 0x00}, /* index 135,  */
	{0x0E80, 0x07, 0xE0, 0x04, 0x00}, /* index 136,  */
	{0x0E60, 0x07, 0xDE, 0x04, 0x00}, /* index 137,  */
	{0x0E40, 0x07, 0xDC, 0x04, 0x00}, /* index 138,  */
	{0x0E20, 0x07, 0xDA, 0x04, 0x00}, /* index 139,  */
	{0x0E00, 0x07, 0xD8, 0x04, 0x00}, /* index 140,  */
	{0x0DE0, 0x07, 0xD6, 0x04, 0x00}, /* index 141,  */
	{0x0DC0, 0x07, 0xD4, 0x04, 0x00}, /* index 142,  */
	{0x0DA0, 0x07, 0xD2, 0x04, 0x00}, /* index 143,  */
	{0x0D80, 0x07, 0xD0, 0x04, 0x00}, /* index 144,  */
	{0x0D60, 0x07, 0xCE, 0x04, 0x00}, /* index 145,  */
	{0x0D40, 0x07, 0xCC, 0x04, 0x00}, /* index 146,  */
	{0x0D20, 0x07, 0xCA, 0x04, 0x00}, /* index 147,  */
	{0x0D00, 0x07, 0xC8, 0x04, 0x00}, /* index 148,  */
	{0x0CE0, 0x07, 0xC6, 0x04, 0x00}, /* index 149,  */
	{0x0CC0, 0x07, 0xC4, 0x04, 0x00}, /* index 150,  */
	{0x0CA0, 0x07, 0xC2, 0x04, 0x00}, /* index 151,  */
	{0x0C80, 0x07, 0xC0, 0x04, 0x00}, /* index 152,  */
	{0x0C60, 0x07, 0xBE, 0x04, 0x00}, /* index 153,  */
	{0x0C40, 0x07, 0xBC, 0x04, 0x00}, /* index 154,  */
	{0x0C20, 0x07, 0xBA, 0x04, 0x00}, /* index 155,  */
	{0x0C00, 0x07, 0xB8, 0x04, 0x00}, /* index 156,  */
	{0x0BE0, 0x07, 0xB6, 0x04, 0x00}, /* index 157,  */
	{0x0BC0, 0x07, 0xB4, 0x04, 0x00}, /* index 158,  */
	{0x0BA0, 0x07, 0xB2, 0x04, 0x00}, /* index 159,  */
	{0x0B80, 0x07, 0xB0, 0x04, 0x00}, /* index 160,  */
	{0x0B60, 0x07, 0xAE, 0x04, 0x00}, /* index 161,  */
	{0x0B40, 0x07, 0xAC, 0x04, 0x00}, /* index 162,  */
	{0x0B20, 0x07, 0xAA, 0x04, 0x00}, /* index 163,  */
	{0x0B00, 0x07, 0xA8, 0x04, 0x00}, /* index 164,  */
	{0x0AE0, 0x07, 0xA6, 0x04, 0x00}, /* index 165,  */
	{0x0AC0, 0x07, 0xA4, 0x04, 0x00}, /* index 166,  */
	{0x0AA0, 0x07, 0xA2, 0x04, 0x00}, /* index 167,  */
	{0x0A80, 0x07, 0xA0, 0x04, 0x00}, /* index 168,  */
	{0x0A60, 0x07, 0x9E, 0x04, 0x00}, /* index 169,  */
	{0x0A40, 0x07, 0x9C, 0x04, 0x00}, /* index 170,  */
	{0x0A20, 0x07, 0x9A, 0x04, 0x00}, /* index 171,  */
	{0x0A00, 0x07, 0x98, 0x04, 0x00}, /* index 172,  */
	{0x09E0, 0x07, 0x96, 0x04, 0x00}, /* index 173,  */
	{0x09C0, 0x07, 0x94, 0x04, 0x00}, /* index 174,  */
	{0x09A0, 0x07, 0x92, 0x04, 0x00}, /* index 175,  */
	{0x0980, 0x07, 0x90, 0x04, 0x00}, /* index 176,  */
	{0x0960, 0x07, 0x8E, 0x04, 0x00}, /* index 177,  */
	{0x0940, 0x07, 0x8C, 0x04, 0x00}, /* index 178,  */
	{0x0920, 0x07, 0x8A, 0x04, 0x00}, /* index 179,  */
	{0x0900, 0x07, 0x88, 0x04, 0x00}, /* index 180,  */
	{0x08E0, 0x07, 0x86, 0x04, 0x00}, /* index 181,  */
	{0x08C0, 0x07, 0x84, 0x04, 0x00}, /* index 182,  */
	{0x08A0, 0x07, 0x82, 0x04, 0x00}, /* index 183,  */
	{0x0880, 0x07, 0x80, 0x04, 0x00}, /* index 184,  */
	{0x0860, 0x07, 0x7E, 0x04, 0x00}, /* index 185,  */
	{0x0840, 0x07, 0x7C, 0x04, 0x00}, /* index 186,  */
	{0x0820, 0x07, 0x7A, 0x04, 0x00}, /* index 187,  */
	{0x0800, 0x07, 0x78, 0x04, 0x00}, /* index 188,  gain = 18.0618 dB, actual gain = 18.0618 dB, */
	{0x07F0, 0x03, 0xF4, 0x04, 0x00}, /* index 189,  */
	{0x07E0, 0x03, 0xF2, 0x04, 0x00}, /* index 190,  */
	{0x07D0, 0x03, 0xF0, 0x04, 0x00}, /* index 191,  */
	{0x07C0, 0x03, 0xEE, 0x04, 0x00}, /* index 192,  */
	{0x07B0, 0x03, 0xEC, 0x04, 0x00}, /* index 193,  */
	{0x07A0, 0x03, 0xEA, 0x04, 0x00}, /* index 194,  */
	{0x0790, 0x03, 0xE8, 0x04, 0x00}, /* index 195,  */
	{0x0780, 0x03, 0xE6, 0x04, 0x00}, /* index 196,  */
	{0x0770, 0x03, 0xE4, 0x04, 0x00}, /* index 197,  */
	{0x0760, 0x03, 0xE2, 0x04, 0x00}, /* index 198,  */
	{0x0750, 0x03, 0xE0, 0x04, 0x00}, /* index 199,  */
	{0x0740, 0x03, 0xDE, 0x04, 0x00}, /* index 200,  */
	{0x0730, 0x03, 0xDC, 0x04, 0x00}, /* index 201,  */
	{0x0720, 0x03, 0xDA, 0x04, 0x00}, /* index 202,  */
	{0x0710, 0x03, 0xD8, 0x04, 0x00}, /* index 203,  */
	{0x0700, 0x03, 0xD6, 0x04, 0x00}, /* index 204,  */
	{0x06F0, 0x03, 0xD4, 0x04, 0x00}, /* index 205,  */
	{0x06E0, 0x03, 0xD2, 0x04, 0x00}, /* index 206,  */
	{0x06D0, 0x03, 0xD0, 0x04, 0x00}, /* index 207,  */
	{0x06C0, 0x03, 0xCE, 0x04, 0x00}, /* index 208,  */
	{0x06B0, 0x03, 0xCC, 0x04, 0x00}, /* index 209,  */
	{0x06A0, 0x03, 0xCA, 0x04, 0x00}, /* index 210,  */
	{0x0690, 0x03, 0xC8, 0x04, 0x00}, /* index 211,  */
	{0x0680, 0x03, 0xC6, 0x04, 0x00}, /* index 212,  */
	{0x0670, 0x03, 0xC4, 0x04, 0x00}, /* index 213,  */
	{0x0660, 0x03, 0xC2, 0x04, 0x00}, /* index 214,  */
	{0x0650, 0x03, 0xC0, 0x04, 0x00}, /* index 215,  */
	{0x0640, 0x03, 0xBE, 0x04, 0x00}, /* index 216,  */
	{0x0630, 0x03, 0xBC, 0x04, 0x00}, /* index 217,  */
	{0x0620, 0x03, 0xBA, 0x04, 0x00}, /* index 218,  */
	{0x0610, 0x03, 0xB8, 0x04, 0x00}, /* index 219,  */
	{0x0600, 0x03, 0xB6, 0x04, 0x00}, /* index 220,  */
	{0x05F0, 0x03, 0xB4, 0x04, 0x00}, /* index 221,  */
	{0x05E0, 0x03, 0xB2, 0x04, 0x00}, /* index 222,  */
	{0x05D0, 0x03, 0xB0, 0x04, 0x00}, /* index 223,  */
	{0x05C0, 0x03, 0xAE, 0x04, 0x00}, /* index 224,  */
	{0x05B0, 0x03, 0xAC, 0x04, 0x00}, /* index 225,  */
	{0x05A0, 0x03, 0xAA, 0x04, 0x00}, /* index 226,  */
	{0x0590, 0x03, 0xA8, 0x04, 0x00}, /* index 227,  */
	{0x0580, 0x03, 0xA6, 0x04, 0x00}, /* index 228,  */
	{0x0570, 0x03, 0xA4, 0x04, 0x00}, /* index 229,  */
	{0x0560, 0x03, 0xA2, 0x04, 0x00}, /* index 230,  */
	{0x0550, 0x03, 0xA0, 0x04, 0x00}, /* index 231,  */
	{0x0540, 0x03, 0x9E, 0x04, 0x00}, /* index 232,  */
	{0x0530, 0x03, 0x9C, 0x04, 0x00}, /* index 233,  */
	{0x0520, 0x03, 0x9A, 0x04, 0x00}, /* index 234,  */
	{0x0510, 0x03, 0x98, 0x04, 0x00}, /* index 235,  */
	{0x0500, 0x03, 0x96, 0x04, 0x00}, /* index 236,  */
	{0x04F0, 0x03, 0x94, 0x04, 0x00}, /* index 237,  */
	{0x04E0, 0x03, 0x92, 0x04, 0x00}, /* index 238,  */
	{0x04D0, 0x03, 0x90, 0x04, 0x00}, /* index 239,  */
	{0x04C0, 0x03, 0x8E, 0x04, 0x00}, /* index 240,  */
	{0x04B0, 0x03, 0x8C, 0x04, 0x00}, /* index 241,  */
	{0x04A0, 0x03, 0x8A, 0x04, 0x00}, /* index 242,  */
	{0x0490, 0x03, 0x88, 0x04, 0x00}, /* index 243,  */
	{0x0480, 0x03, 0x86, 0x04, 0x00}, /* index 244,  */
	{0x0470, 0x03, 0x84, 0x04, 0x00}, /* index 245,  */
	{0x0460, 0x03, 0x82, 0x04, 0x00}, /* index 246,  */
	{0x0450, 0x03, 0x80, 0x04, 0x00}, /* index 247,  */
	{0x0440, 0x03, 0x7E, 0x04, 0x00}, /* index 248,  */
	{0x0430, 0x03, 0x7C, 0x04, 0x00}, /* index 249,  */
	{0x0420, 0x03, 0x7A, 0x04, 0x00}, /* index 250,  */
	{0x0410, 0x03, 0x78, 0x04, 0x00}, /* index 251,  */
	{0x0400, 0x03, 0x74, 0x04, 0x00}, /* index 252,  gain = 12.0412 dB, actual gain = 12.0412 dB, */
	{0x03F8, 0x01, 0xF6, 0x04, 0x00}, /* index 253,  */
	{0x03F0, 0x01, 0xF4, 0x04, 0x00}, /* index 254,  */
	{0x03E8, 0x01, 0xF2, 0x04, 0x00}, /* index 255,  */
	{0x03E0, 0x01, 0xF0, 0x04, 0x00}, /* index 256,  */
	{0x03D8, 0x01, 0xEE, 0x04, 0x00}, /* index 257,  */
	{0x03D0, 0x01, 0xEC, 0x04, 0x00}, /* index 258,  */
	{0x03C8, 0x01, 0xEA, 0x04, 0x00}, /* index 259,  */
	{0x03C0, 0x01, 0xE8, 0x04, 0x00}, /* index 260,  */
	{0x03B8, 0x01, 0xE6, 0x04, 0x00}, /* index 261,  */
	{0x03B0, 0x01, 0xE4, 0x04, 0x00}, /* index 262,  */
	{0x03A8, 0x01, 0xE2, 0x04, 0x00}, /* index 263,  */
	{0x03A0, 0x01, 0xE0, 0x04, 0x00}, /* index 264,  */
	{0x0398, 0x01, 0xDE, 0x04, 0x00}, /* index 265,  */
	{0x0390, 0x01, 0xDC, 0x04, 0x00}, /* index 266,  */
	{0x0388, 0x01, 0xDA, 0x04, 0x00}, /* index 267,  */
	{0x0380, 0x01, 0xD8, 0x04, 0x00}, /* index 268,  */
	{0x0378, 0x01, 0xD6, 0x04, 0x00}, /* index 269,  */
	{0x0370, 0x01, 0xD4, 0x04, 0x00}, /* index 270,  */
	{0x0368, 0x01, 0xD2, 0x04, 0x00}, /* index 271,  */
	{0x0360, 0x01, 0xD0, 0x04, 0x00}, /* index 272,  */
	{0x0358, 0x01, 0xCE, 0x04, 0x00}, /* index 273,  */
	{0x0350, 0x01, 0xCC, 0x04, 0x00}, /* index 274,  */
	{0x0348, 0x01, 0xCA, 0x04, 0x00}, /* index 275,  */
	{0x0340, 0x01, 0xC8, 0x04, 0x00}, /* index 276,  */
	{0x0338, 0x01, 0xC6, 0x04, 0x00}, /* index 277,  */
	{0x0330, 0x01, 0xC4, 0x04, 0x00}, /* index 278,  */
	{0x0328, 0x01, 0xC2, 0x04, 0x00}, /* index 279,  */
	{0x0320, 0x01, 0xC0, 0x04, 0x00}, /* index 280,  */
	{0x0318, 0x01, 0xBE, 0x04, 0x00}, /* index 281,  */
	{0x0310, 0x01, 0xBC, 0x04, 0x00}, /* index 282,  */
	{0x0308, 0x01, 0xBA, 0x04, 0x00}, /* index 283,  */
	{0x0300, 0x01, 0xB8, 0x04, 0x00}, /* index 284,  */
	{0x02F8, 0x01, 0xB6, 0x04, 0x00}, /* index 285,  */
	{0x02F0, 0x01, 0xB4, 0x04, 0x00}, /* index 286,  */
	{0x02E8, 0x01, 0xB2, 0x04, 0x00}, /* index 287,  */
	{0x02E0, 0x01, 0xB0, 0x04, 0x00}, /* index 288,  */
	{0x02D8, 0x01, 0xAE, 0x04, 0x00}, /* index 289,  */
	{0x02D0, 0x01, 0xAC, 0x04, 0x00}, /* index 290,  */
	{0x02C8, 0x01, 0xAA, 0x04, 0x00}, /* index 291,  */
	{0x02C0, 0x01, 0xA8, 0x04, 0x00}, /* index 292,  */
	{0x02B8, 0x01, 0xA6, 0x04, 0x00}, /* index 293,  */
	{0x02B0, 0x01, 0xA4, 0x04, 0x00}, /* index 294,  */
	{0x02A8, 0x01, 0xA2, 0x04, 0x00}, /* index 295,  */
	{0x02A0, 0x01, 0xA0, 0x04, 0x00}, /* index 296,  */
	{0x0298, 0x01, 0x9E, 0x04, 0x00}, /* index 297,  */
	{0x0290, 0x01, 0x9C, 0x04, 0x00}, /* index 298,  */
	{0x0288, 0x01, 0x9A, 0x04, 0x00}, /* index 299,  */
	{0x0280, 0x01, 0x98, 0x04, 0x00}, /* index 300,  */
	{0x0278, 0x01, 0x96, 0x04, 0x00}, /* index 301,  */
	{0x0270, 0x01, 0x94, 0x04, 0x00}, /* index 302,  */
	{0x0268, 0x01, 0x92, 0x04, 0x00}, /* index 303,  */
	{0x0260, 0x01, 0x90, 0x04, 0x00}, /* index 304,  */
	{0x0258, 0x01, 0x8E, 0x04, 0x00}, /* index 305,  */
	{0x0250, 0x01, 0x8C, 0x04, 0x00}, /* index 306,  */
	{0x0248, 0x01, 0x8A, 0x04, 0x00}, /* index 307,  */
	{0x0240, 0x01, 0x88, 0x04, 0x00}, /* index 308,  */
	{0x0238, 0x01, 0x86, 0x04, 0x00}, /* index 309,  */
	{0x0230, 0x01, 0x84, 0x04, 0x00}, /* index 310,  */
	{0x0228, 0x01, 0x82, 0x04, 0x00}, /* index 311,  */
	{0x0220, 0x01, 0x80, 0x04, 0x00}, /* index 312,  */
	{0x0218, 0x01, 0x7E, 0x04, 0x00}, /* index 313,  */
	{0x0210, 0x01, 0x7C, 0x04, 0x00}, /* index 314,  */
	{0x0208, 0x01, 0x7A, 0x04, 0x00}, /* index 315,  */
	{0x0200, 0x01, 0x78, 0x04, 0x00}, /* index 316,  gain = 6.0206 dB, actual gain = 6.0206 dB, */
	{0x01FC, 0x00, 0xFE, 0x04, 0x00}, /* index 317,  */
	{0x01F8, 0x00, 0xFC, 0x04, 0x00}, /* index 318,  */
	{0x01F4, 0x00, 0xFA, 0x04, 0x00}, /* index 319,  */
	{0x01F0, 0x00, 0xF8, 0x04, 0x00}, /* index 320,  */
	{0x01EC, 0x00, 0xF6, 0x04, 0x00}, /* index 321,  */
	{0x01E8, 0x00, 0xF4, 0x04, 0x00}, /* index 322,  */
	{0x01E4, 0x00, 0xF2, 0x04, 0x00}, /* index 323,  */
	{0x01E0, 0x00, 0xF0, 0x04, 0x00}, /* index 324,  */
	{0x01DC, 0x00, 0xEE, 0x04, 0x00}, /* index 325,  */
	{0x01D8, 0x00, 0xEC, 0x04, 0x00}, /* index 326,  */
	{0x01D4, 0x00, 0xEA, 0x04, 0x00}, /* index 327,  */
	{0x01D0, 0x00, 0xE8, 0x04, 0x00}, /* index 328,  */
	{0x01CC, 0x00, 0xE6, 0x04, 0x00}, /* index 329,  */
	{0x01C8, 0x00, 0xE4, 0x04, 0x00}, /* index 330,  */
	{0x01C4, 0x00, 0xE2, 0x04, 0x00}, /* index 331,  */
	{0x01C0, 0x00, 0xE0, 0x04, 0x00}, /* index 332,  */
	{0x01BC, 0x00, 0xDE, 0x04, 0x00}, /* index 333,  */
	{0x01B8, 0x00, 0xDC, 0x04, 0x00}, /* index 334,  */
	{0x01B4, 0x00, 0xDA, 0x04, 0x00}, /* index 335,  */
	{0x01B0, 0x00, 0xD8, 0x04, 0x00}, /* index 336,  */
	{0x01AC, 0x00, 0xD6, 0x04, 0x00}, /* index 337,  */
	{0x01A8, 0x00, 0xD4, 0x04, 0x00}, /* index 338,  */
	{0x01A4, 0x00, 0xD2, 0x04, 0x00}, /* index 339,  */
	{0x01A0, 0x00, 0xD0, 0x04, 0x00}, /* index 340,  */
	{0x019C, 0x00, 0xCE, 0x04, 0x00}, /* index 341,  */
	{0x0198, 0x00, 0xCC, 0x04, 0x00}, /* index 342,  */
	{0x0194, 0x00, 0xCA, 0x04, 0x00}, /* index 343,  */
	{0x0190, 0x00, 0xC8, 0x04, 0x00}, /* index 344,  */
	{0x018C, 0x00, 0xC6, 0x04, 0x00}, /* index 345,  */
	{0x0188, 0x00, 0xC4, 0x04, 0x00}, /* index 346,  */
	{0x0184, 0x00, 0xC2, 0x04, 0x00}, /* index 347,  */
	{0x0180, 0x00, 0xC0, 0x04, 0x00}, /* index 348,  */
	{0x017C, 0x00, 0xBE, 0x04, 0x00}, /* index 349,  */
	{0x0178, 0x00, 0xBC, 0x04, 0x00}, /* index 350,  */
	{0x0174, 0x00, 0xBA, 0x04, 0x00}, /* index 351,  */
	{0x0170, 0x00, 0xB8, 0x04, 0x00}, /* index 352,  */
	{0x016C, 0x00, 0xB6, 0x04, 0x00}, /* index 353,  */
	{0x0168, 0x00, 0xB4, 0x04, 0x00}, /* index 354,  */
	{0x0164, 0x00, 0xB2, 0x04, 0x00}, /* index 355,  */
	{0x0160, 0x00, 0xB0, 0x04, 0x00}, /* index 356,  */
	{0x015C, 0x00, 0xAE, 0x04, 0x00}, /* index 357,  */
	{0x0158, 0x00, 0xAC, 0x04, 0x00}, /* index 358,  */
	{0x0154, 0x00, 0xAA, 0x04, 0x00}, /* index 359,  */
	{0x0150, 0x00, 0xA8, 0x04, 0x00}, /* index 360,  */
	{0x014C, 0x00, 0xA6, 0x04, 0x00}, /* index 361,  */
	{0x0148, 0x00, 0xA4, 0x04, 0x00}, /* index 362,  */
	{0x0144, 0x00, 0xA2, 0x04, 0x00}, /* index 363,  */
	{0x0140, 0x00, 0xA0, 0x04, 0x00}, /* index 364,  */
	{0x013C, 0x00, 0x9E, 0x04, 0x00}, /* index 365,  */
	{0x0138, 0x00, 0x9C, 0x04, 0x00}, /* index 366,  */
	{0x0134, 0x00, 0x9A, 0x04, 0x00}, /* index 367,  */
	{0x0130, 0x00, 0x98, 0x04, 0x00}, /* index 368,  */
	{0x012C, 0x00, 0x96, 0x04, 0x00}, /* index 369,  */
	{0x0128, 0x00, 0x94, 0x04, 0x00}, /* index 370,  */
	{0x0124, 0x00, 0x92, 0x04, 0x00}, /* index 371,  */
	{0x0120, 0x00, 0x90, 0x04, 0x00}, /* index 372,  */
	{0x011C, 0x00, 0x8E, 0x04, 0x00}, /* index 373,  */
	{0x0118, 0x00, 0x8C, 0x04, 0x00}, /* index 374,  */
	{0x0114, 0x00, 0x8A, 0x04, 0x00}, /* index 375,  */
	{0x0110, 0x00, 0x88, 0x04, 0x00}, /* index 376,  */
	{0x010C, 0x00, 0x87, 0x04, 0x00}, /* index 377,  */
	{0x0108, 0x00, 0x86, 0x04, 0x00}, /* index 378,  */
	{0x0104, 0x00, 0x85, 0x04, 0x00}, /* index 379,  */
	{0x0100, 0x00, 0x84, 0x04, 0x00}, /* index 380,  */
	{0x0100, 0x00, 0x83, 0x04, 0x00}, /* index 381,  */
	{0x0100, 0x00, 0x82, 0x04, 0x00}, /* index 382,  */
	{0x0100, 0x00, 0x81, 0x04, 0x00}, /* index 383,  */
	{0x0100, 0x00, 0x80, 0x04, 0x00}, /* index 384, gain = 0 dB, actual gain = 0 dB, */
};

#define OV4689_AGAIN_ROWS		(257)
#define OV4689_AGAIN_COLS		(2)
#define OV4689_AGAIN_MSB		(0)
#define OV4689_AGAIN_LSB		(1)
#define OV4689_AGAIN_0DB		(256)

const s16 OV4689_WDR_AGAIN_TABLE[OV4689_AGAIN_ROWS][OV4689_AGAIN_COLS] = {
	{0x07, 0xF0}, /* index   0,  */
	{0x07, 0xEE}, /* index   1,  */
	{0x07, 0xEC}, /* index   2,  */
	{0x07, 0xEA}, /* index   3,  */
	{0x07, 0xE8}, /* index   4,  */
	{0x07, 0xE6}, /* index   5,  */
	{0x07, 0xE4}, /* index   6,  */
	{0x07, 0xE2}, /* index   7,  */
	{0x07, 0xE0}, /* index   8,  */
	{0x07, 0xDE}, /* index   9,  */
	{0x07, 0xDC}, /* index  10,  */
	{0x07, 0xDA}, /* index  11,  */
	{0x07, 0xD8}, /* index  12,  */
	{0x07, 0xD6}, /* index  13,  */
	{0x07, 0xD4}, /* index  14,  */
	{0x07, 0xD2}, /* index  15,  */
	{0x07, 0xD0}, /* index  16,  */
	{0x07, 0xCE}, /* index  17,  */
	{0x07, 0xCC}, /* index  18,  */
	{0x07, 0xCA}, /* index  19,  */
	{0x07, 0xC8}, /* index  20,  */
	{0x07, 0xC6}, /* index  21,  */
	{0x07, 0xC4}, /* index  22,  */
	{0x07, 0xC2}, /* index  23,  */
	{0x07, 0xC0}, /* index  24,  */
	{0x07, 0xBE}, /* index  25,  */
	{0x07, 0xBC}, /* index  26,  */
	{0x07, 0xBA}, /* index  27,  */
	{0x07, 0xB8}, /* index  28,  */
	{0x07, 0xB6}, /* index  29,  */
	{0x07, 0xB4}, /* index  30,  */
	{0x07, 0xB2}, /* index  31,  */
	{0x07, 0xB0}, /* index  32,  */
	{0x07, 0xAE}, /* index  33,  */
	{0x07, 0xAC}, /* index  34,  */
	{0x07, 0xAA}, /* index  35,  */
	{0x07, 0xA8}, /* index  36,  */
	{0x07, 0xA6}, /* index  37,  */
	{0x07, 0xA4}, /* index  38,  */
	{0x07, 0xA2}, /* index  39,  */
	{0x07, 0xA0}, /* index  40,  */
	{0x07, 0x9E}, /* index  41,  */
	{0x07, 0x9C}, /* index  42,  */
	{0x07, 0x9A}, /* index  43,  */
	{0x07, 0x98}, /* index  44,  */
	{0x07, 0x96}, /* index  45,  */
	{0x07, 0x94}, /* index  46,  */
	{0x07, 0x92}, /* index  47,  */
	{0x07, 0x90}, /* index  48,  */
	{0x07, 0x8E}, /* index  49,  */
	{0x07, 0x8C}, /* index  50,  */
	{0x07, 0x8A}, /* index  51,  */
	{0x07, 0x88}, /* index  52,  */
	{0x07, 0x86}, /* index  53,  */
	{0x07, 0x84}, /* index  54,  */
	{0x07, 0x82}, /* index  55,  */
	{0x07, 0x80}, /* index  56,  */
	{0x07, 0x7E}, /* index  57,  */
	{0x07, 0x7C}, /* index  58,  */
	{0x07, 0x7A}, /* index  59,  */
	{0x07, 0x78}, /* index  60,  */
	{0x03, 0xF4}, /* index  61,  */
	{0x03, 0xF2}, /* index  62,  */
	{0x03, 0xF0}, /* index  63,  */
	{0x03, 0xEE}, /* index  64,  */
	{0x03, 0xEC}, /* index  65,  */
	{0x03, 0xEA}, /* index  66,  */
	{0x03, 0xE8}, /* index  67,  */
	{0x03, 0xE6}, /* index  68,  */
	{0x03, 0xE4}, /* index  69,  */
	{0x03, 0xE2}, /* index  70,  */
	{0x03, 0xE0}, /* index  71,  */
	{0x03, 0xDE}, /* index  72,  */
	{0x03, 0xDC}, /* index  73,  */
	{0x03, 0xDA}, /* index  74,  */
	{0x03, 0xD8}, /* index  75,  */
	{0x03, 0xD6}, /* index  76,  */
	{0x03, 0xD4}, /* index  77,  */
	{0x03, 0xD2}, /* index  78,  */
	{0x03, 0xD0}, /* index  79,  */
	{0x03, 0xCE}, /* index  80,  */
	{0x03, 0xCC}, /* index  81,  */
	{0x03, 0xCA}, /* index  82,  */
	{0x03, 0xC8}, /* index  83,  */
	{0x03, 0xC6}, /* index  84,  */
	{0x03, 0xC4}, /* index  85,  */
	{0x03, 0xC2}, /* index  86,  */
	{0x03, 0xC0}, /* index  87,  */
	{0x03, 0xBE}, /* index  88,  */
	{0x03, 0xBC}, /* index  89,  */
	{0x03, 0xBA}, /* index  90,  */
	{0x03, 0xB8}, /* index  91,  */
	{0x03, 0xB6}, /* index  92,  */
	{0x03, 0xB4}, /* index  93,  */
	{0x03, 0xB2}, /* index  94,  */
	{0x03, 0xB0}, /* index  95,  */
	{0x03, 0xAE}, /* index  96,  */
	{0x03, 0xAC}, /* index  97,  */
	{0x03, 0xAA}, /* index  98,  */
	{0x03, 0xA8}, /* index  99,  */
	{0x03, 0xA6}, /* index 100,  */
	{0x03, 0xA4}, /* index 101,  */
	{0x03, 0xA2}, /* index 102,  */
	{0x03, 0xA0}, /* index 103,  */
	{0x03, 0x9E}, /* index 104,  */
	{0x03, 0x9C}, /* index 105,  */
	{0x03, 0x9A}, /* index 106,  */
	{0x03, 0x98}, /* index 107,  */
	{0x03, 0x96}, /* index 108,  */
	{0x03, 0x94}, /* index 109,  */
	{0x03, 0x92}, /* index 110,  */
	{0x03, 0x90}, /* index 111,  */
	{0x03, 0x8E}, /* index 112,  */
	{0x03, 0x8C}, /* index 113,  */
	{0x03, 0x8A}, /* index 114,  */
	{0x03, 0x88}, /* index 115,  */
	{0x03, 0x86}, /* index 116,  */
	{0x03, 0x84}, /* index 117,  */
	{0x03, 0x82}, /* index 118,  */
	{0x03, 0x80}, /* index 119,  */
	{0x03, 0x7E}, /* index 120,  */
	{0x03, 0x7C}, /* index 121,  */
	{0x03, 0x7A}, /* index 122,  */
	{0x03, 0x78}, /* index 123,  */
	{0x03, 0x74}, /* index 124,  */
	{0x01, 0xF6}, /* index 125,  */
	{0x01, 0xF4}, /* index 126,  */
	{0x01, 0xF2}, /* index 127,  */
	{0x01, 0xF0}, /* index 128,  */
	{0x01, 0xEE}, /* index 129  */
	{0x01, 0xEC}, /* index 130  */
	{0x01, 0xEA}, /* index 131  */
	{0x01, 0xE8}, /* index 132  */
	{0x01, 0xE6}, /* index 133  */
	{0x01, 0xE4}, /* index 134  */
	{0x01, 0xE2}, /* index 135  */
	{0x01, 0xE0}, /* index 136  */
	{0x01, 0xDE}, /* index 137  */
	{0x01, 0xDC}, /* index 138  */
	{0x01, 0xDA}, /* index 139  */
	{0x01, 0xD8}, /* index 140  */
	{0x01, 0xD6}, /* index 141  */
	{0x01, 0xD4}, /* index 142  */
	{0x01, 0xD2}, /* index 143  */
	{0x01, 0xD0}, /* index 144  */
	{0x01, 0xCE}, /* index 145  */
	{0x01, 0xCC}, /* index 146  */
	{0x01, 0xCA}, /* index 147  */
	{0x01, 0xC8}, /* index 148  */
	{0x01, 0xC6}, /* index 149  */
	{0x01, 0xC4}, /* index 150  */
	{0x01, 0xC2}, /* index 151  */
	{0x01, 0xC0}, /* index 152  */
	{0x01, 0xBE}, /* index 153  */
	{0x01, 0xBC}, /* index 154  */
	{0x01, 0xBA}, /* index 155  */
	{0x01, 0xB8}, /* index 156  */
	{0x01, 0xB6}, /* index 157  */
	{0x01, 0xB4}, /* index 158  */
	{0x01, 0xB2}, /* index 159  */
	{0x01, 0xB0}, /* index 160  */
	{0x01, 0xAE}, /* index 161  */
	{0x01, 0xAC}, /* index 162  */
	{0x01, 0xAA}, /* index 163  */
	{0x01, 0xA8}, /* index 164  */
	{0x01, 0xA6}, /* index 165  */
	{0x01, 0xA4}, /* index 166  */
	{0x01, 0xA2}, /* index 167  */
	{0x01, 0xA0}, /* index 168  */
	{0x01, 0x9E}, /* index 169  */
	{0x01, 0x9C}, /* index 170  */
	{0x01, 0x9A}, /* index 171  */
	{0x01, 0x98}, /* index 172  */
	{0x01, 0x96}, /* index 173  */
	{0x01, 0x94}, /* index 174  */
	{0x01, 0x92}, /* index 175  */
	{0x01, 0x90}, /* index 176  */
	{0x01, 0x8E}, /* index 177  */
	{0x01, 0x8C}, /* index 178  */
	{0x01, 0x8A}, /* index 179  */
	{0x01, 0x88}, /* index 180  */
	{0x01, 0x86}, /* index 181  */
	{0x01, 0x84}, /* index 182  */
	{0x01, 0x82}, /* index 183  */
	{0x01, 0x80}, /* index 184  */
	{0x01, 0x7E}, /* index 185  */
	{0x01, 0x7C}, /* index 186  */
	{0x01, 0x7A}, /* index 187  */
	{0x01, 0x78}, /* index 188  */
	{0x00, 0xFE}, /* index 189  */
	{0x00, 0xFC}, /* index 190  */
	{0x00, 0xFA}, /* index 191  */
	{0x00, 0xF8}, /* index 192  */
	{0x00, 0xF6}, /* index 193  */
	{0x00, 0xF4}, /* index 194  */
	{0x00, 0xF2}, /* index 195  */
	{0x00, 0xF0}, /* index 196  */
	{0x00, 0xEE}, /* index 197  */
	{0x00, 0xEC}, /* index 198  */
	{0x00, 0xEA}, /* index 199  */
	{0x00, 0xE8}, /* index 200  */
	{0x00, 0xE6}, /* index 201  */
	{0x00, 0xE4}, /* index 202  */
	{0x00, 0xE2}, /* index 203  */
	{0x00, 0xE0}, /* index 204  */
	{0x00, 0xDE}, /* index 205  */
	{0x00, 0xDC}, /* index 206  */
	{0x00, 0xDA}, /* index 207  */
	{0x00, 0xD8}, /* index 208  */
	{0x00, 0xD6}, /* index 209  */
	{0x00, 0xD4}, /* index 210  */
	{0x00, 0xD2}, /* index 211  */
	{0x00, 0xD0}, /* index 212  */
	{0x00, 0xCE}, /* index 213  */
	{0x00, 0xCC}, /* index 214  */
	{0x00, 0xCA}, /* index 215  */
	{0x00, 0xC8}, /* index 216  */
	{0x00, 0xC6}, /* index 217  */
	{0x00, 0xC4}, /* index 218  */
	{0x00, 0xC2}, /* index 219  */
	{0x00, 0xC0}, /* index 220  */
	{0x00, 0xBE}, /* index 221  */
	{0x00, 0xBC}, /* index 222  */
	{0x00, 0xBA}, /* index 223  */
	{0x00, 0xB8}, /* index 224  */
	{0x00, 0xB6}, /* index 225  */
	{0x00, 0xB4}, /* index 226  */
	{0x00, 0xB2}, /* index 227  */
	{0x00, 0xB0}, /* index 228  */
	{0x00, 0xAE}, /* index 229  */
	{0x00, 0xAC}, /* index 230  */
	{0x00, 0xAA}, /* index 231  */
	{0x00, 0xA8}, /* index 232  */
	{0x00, 0xA6}, /* index 233  */
	{0x00, 0xA4}, /* index 234  */
	{0x00, 0xA2}, /* index 235  */
	{0x00, 0xA0}, /* index 236  */
	{0x00, 0x9E}, /* index 237  */
	{0x00, 0x9C}, /* index 238  */
	{0x00, 0x9A}, /* index 239  */
	{0x00, 0x98}, /* index 240  */
	{0x00, 0x96}, /* index 241  */
	{0x00, 0x94}, /* index 242  */
	{0x00, 0x92}, /* index 243  */
	{0x00, 0x90}, /* index 244  */
	{0x00, 0x8E}, /* index 245  */
	{0x00, 0x8C}, /* index 246  */
	{0x00, 0x8A}, /* index 247  */
	{0x00, 0x88}, /* index 248  */
	{0x00, 0x87}, /* index 249  */
	{0x00, 0x86}, /* index 250  */
	{0x00, 0x85}, /* index 251  */
	{0x00, 0x84}, /* index 252  */
	{0x00, 0x83}, /* index 253  */
	{0x00, 0x82}, /* index 254  */
	{0x00, 0x81}, /* index 255  */
	{0x00, 0x80}, /* index 256  0DB */
};

#define OV4689_DGAIN_ROWS		(129)
#define OV4689_DGAIN_COLS		(2)
#define OV4689_DGAIN_MSB		(0)
#define OV4689_DGAIN_LSB		(1)
#define OV4689_DGAIN_0DB		(128)

const s16 OV4689_WDR_DGAIN_TABLE[OV4689_DGAIN_ROWS][OV4689_DGAIN_COLS] = {
	{0x0f, 0xff}, /* index   0,  */
	{0x0f, 0xe0}, /* index   1,  */
	{0x0f, 0xc0}, /* index   2,  */
	{0x0f, 0xa0}, /* index   3,  */
	{0x0f, 0x80}, /* index   4,  */
	{0x0f, 0x60}, /* index   5,  */
	{0x0f, 0x40}, /* index   6,  */
	{0x0f, 0x20}, /* index   7,  */
	{0x0f, 0x00}, /* index   8,  */
	{0x0e, 0xe0}, /* index   9,  */
	{0x0e, 0xc0}, /* index  10,  */
	{0x0e, 0xa0}, /* index  11,  */
	{0x0e, 0x80}, /* index  12,  */
	{0x0e, 0x60}, /* index  13,  */
	{0x0e, 0x40}, /* index  14,  */
	{0x0e, 0x20}, /* index  15,  */
	{0x0e, 0x00}, /* index  16,  */
	{0x0d, 0xe0}, /* index  17,  */
	{0x0d, 0xc0}, /* index  18,  */
	{0x0d, 0xa0}, /* index  19,  */
	{0x0d, 0x80}, /* index  20,  */
	{0x0d, 0x60}, /* index  21,  */
	{0x0d, 0x40}, /* index  22,  */
	{0x0d, 0x20}, /* index  23,  */
	{0x0d, 0x00}, /* index  24,  */
	{0x0c, 0xe0}, /* index  25,  */
	{0x0c, 0xc0}, /* index  26,  */
	{0x0c, 0xa0}, /* index  27,  */
	{0x0c, 0x80}, /* index  28,  */
	{0x0c, 0x60}, /* index  29,  */
	{0x0c, 0x40}, /* index  30,  */
	{0x0c, 0x20}, /* index  31,  */
	{0x0c, 0x00}, /* index  32,  */
	{0x0b, 0xe0}, /* index  33,  */
	{0x0b, 0xc0}, /* index  34,  */
	{0x0b, 0xa0}, /* index  35,  */
	{0x0b, 0x80}, /* index  36,  */
	{0x0b, 0x60}, /* index  37,  */
	{0x0b, 0x40}, /* index  38,  */
	{0x0b, 0x20}, /* index  39,  */
	{0x0b, 0x00}, /* index  40,  */
	{0x0a, 0xe0}, /* index  41,  */
	{0x0a, 0xc0}, /* index  42,  */
	{0x0a, 0xa0}, /* index  43,  */
	{0x0a, 0x80}, /* index  44,  */
	{0x0a, 0x60}, /* index  45,  */
	{0x0a, 0x40}, /* index  46,  */
	{0x0a, 0x20}, /* index  47,  */
	{0x0a, 0x00}, /* index  48,  */
	{0x09, 0xe0}, /* index  49,  */
	{0x09, 0xc0}, /* index  50,  */
	{0x09, 0xa0}, /* index  51,  */
	{0x09, 0x80}, /* index  52,  */
	{0x09, 0x60}, /* index  53,  */
	{0x09, 0x40}, /* index  54,  */
	{0x09, 0x20}, /* index  55,  */
	{0x09, 0x00}, /* index  56,  */
	{0x08, 0xe0}, /* index  57,  */
	{0x08, 0xc0}, /* index  58,  */
	{0x08, 0xa0}, /* index  59,  */
	{0x08, 0x80}, /* index  60,  */
	{0x08, 0x60}, /* index  61,  */
	{0x08, 0x40}, /* index  62,  */
	{0x08, 0x20}, /* index  63,  */
	{0x08, 0x00}, /* index  64,  */
	{0x07, 0xf0}, /* index  65,  */
	{0x07, 0xe0}, /* index  66,  */
	{0x07, 0xd0}, /* index  67,  */
	{0x07, 0xc0}, /* index  68,  */
	{0x07, 0xb0}, /* index  69,  */
	{0x07, 0xa0}, /* index  70,  */
	{0x07, 0x90}, /* index  71,  */
	{0x07, 0x80}, /* index  72,  */
	{0x07, 0x70}, /* index  73,  */
	{0x07, 0x60}, /* index  74,  */
	{0x07, 0x50}, /* index  75,  */
	{0x07, 0x40}, /* index  76,  */
	{0x07, 0x30}, /* index  77,  */
	{0x07, 0x20}, /* index  78,  */
	{0x07, 0x10}, /* index  79,  */
	{0x07, 0x00}, /* index  80,  */
	{0x06, 0xf0}, /* index  81,  */
	{0x06, 0xe0}, /* index  82,  */
	{0x06, 0xd0}, /* index  83,  */
	{0x06, 0xc0}, /* index  84,  */
	{0x06, 0xb0}, /* index  85,  */
	{0x06, 0xa0}, /* index  86,  */
	{0x06, 0x90}, /* index  87,  */
	{0x06, 0x80}, /* index  88,  */
	{0x06, 0x70}, /* index  89,  */
	{0x06, 0x60}, /* index  90,  */
	{0x06, 0x50}, /* index  91,  */
	{0x06, 0x40}, /* index  92,  */
	{0x06, 0x30}, /* index  93,  */
	{0x06, 0x20}, /* index  94,  */
	{0x06, 0x10}, /* index  95,  */
	{0x06, 0x00}, /* index  96,  */
	{0x05, 0xf0}, /* index  97,  */
	{0x05, 0xe0}, /* index  98,  */
	{0x05, 0xd0}, /* index  99,  */
	{0x05, 0xc0}, /* index 100,  */
	{0x05, 0xb0}, /* index 101,  */
	{0x05, 0xa0}, /* index 102,  */
	{0x05, 0x90}, /* index 103,  */
	{0x05, 0x80}, /* index 104,  */
	{0x05, 0x70}, /* index 105,  */
	{0x05, 0x60}, /* index 106,  */
	{0x05, 0x50}, /* index 107,  */
	{0x05, 0x40}, /* index 108,  */
	{0x05, 0x30}, /* index 109,  */
	{0x05, 0x20}, /* index 110,  */
	{0x05, 0x10}, /* index 111,  */
	{0x05, 0x00}, /* index 112,  */
	{0x04, 0xf0}, /* index 113,  */
	{0x04, 0xe0}, /* index 114,  */
	{0x04, 0xd0}, /* index 115,  */
	{0x04, 0xc0}, /* index 116,  */
	{0x04, 0xb0}, /* index 117,  */
	{0x04, 0xa0}, /* index 118,  */
	{0x04, 0x90}, /* index 119,  */
	{0x04, 0x80}, /* index 120,  */
	{0x04, 0x70}, /* index 121,  */
	{0x04, 0x60}, /* index 122,  */
	{0x04, 0x50}, /* index 123,  */
	{0x04, 0x40}, /* index 124,  */
	{0x04, 0x30}, /* index 125,  */
	{0x04, 0x20}, /* index 126,  */
	{0x04, 0x10}, /* index 127,  */
	{0x04, 0x00}, /* index 128,  1X */
};
