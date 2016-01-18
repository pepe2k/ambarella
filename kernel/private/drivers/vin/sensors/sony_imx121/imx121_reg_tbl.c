/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx121/imx121_reg_tbl.c
 *
 * History:
 *    2011/11/25 - [Long Zhao] Create
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
static const struct imx121_video_format_reg_table imx121_video_format_tbl = {
	.reg		= {
		IMX121_REG_01,
		IMX121_REG_0D,
		IMX121_REG_ADBIT,
		IMX121_REG_BLKLEVEL
	},
	.table[0]	= {// mode 1: 4096x2160 10ch 10bit 59.94fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x00,// 10ch
			0x21,// mode 1
			0x01,// A/D 10bits
			0x32
		},
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
		.pll_index	= 4,
		.xhs_clk = 546,
	},
	.table[1]	= {// mode 2: 4096x2160 8ch 12bit 27.97fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x11,// 8ch
			0x20,// mode 2
			0x03,// A/D 12bits
			0xC8
		},
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
		.pll_index	= 6,
		.xhs_clk = 1170,
	},
	.table[2]	= {// mode 4: 2x2 binning 2048x1080 4ch 10bit 59.94fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x33,// 4ch
			0x22,// mode 4
			0x01,// A/D 10bits
			0xC8
		},
		.width		= 2048,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.auto_fps	= AMBA_VIDEO_FPS_59_94,
		.pll_index	= 3,
		.xhs_clk = 528,
	},
	.table[3]	= {// mode 5: 4016x3016 10ch 10bit 41.96fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x00,// 10ch
			0x01,// mode 5
			0x01,// A/D 10bits
			0x32
		},
		.width		= 4016,
		.height		= 3016,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 5,
		.xhs_clk = 550,
	},
	.table[4]	= {// mode 6: 4000x3000 8ch 12bit 19.98fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x11,// 8ch
			0x00,// mode 6
			0x03,// A/D 12bits
			0xC8
		},
		.width		= 4000,
		.height		= 3000,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_20,
		.auto_fps	= AMBA_VIDEO_FPS_20,
		.pll_index	= 8,
		.xhs_clk = 1170,
	},
};

#define IMX121_GAIN_ROWS	449
#define IMX121_GAIN_COLS	2

#define IMX121_GAIN_COL_AGC	0
#define IMX121_GAIN_COL_DGAIN	1
#define IMX121_GAIN_0DB 448

/* For digital gain, the step is 6dB, Max = 18dB */
const u16 IMX121_GAIN_TABLE[IMX121_GAIN_ROWS][IMX121_GAIN_COLS] = {
	{0x0780, 0x83}, /* index 0, gain=42.000000 DB */
	{0x077d, 0x83}, /* index 1, gain=41.906250 DB */
	{0x077b, 0x83}, /* index 2, gain=41.812500 DB */
	{0x077a, 0x83}, /* index 3, gain=41.718750 DB */
	{0x0779, 0x83}, /* index 4, gain=41.625000 DB */
	{0x0777, 0x83}, /* index 5, gain=41.531250 DB */
	{0x0776, 0x83}, /* index 6, gain=41.437500 DB */
	{0x0774, 0x83}, /* index 7, gain=41.343750 DB */
	{0x0773, 0x83}, /* index 8, gain=41.250000 DB */
	{0x0771, 0x83}, /* index 9, gain=41.156250 DB */
	{0x0770, 0x83}, /* index 10, gain=41.062500 DB */
	{0x076e, 0x83}, /* index 11, gain=40.968750 DB */
	{0x076c, 0x83}, /* index 12, gain=40.875000 DB */
	{0x076b, 0x83}, /* index 13, gain=40.781250 DB */
	{0x0769, 0x83}, /* index 14, gain=40.687500 DB */
	{0x0768, 0x83}, /* index 15, gain=40.593750 DB */
	{0x0766, 0x83}, /* index 16, gain=40.500000 DB */
	{0x0764, 0x83}, /* index 17, gain=40.406250 DB */
	{0x0763, 0x83}, /* index 18, gain=40.312500 DB */
	{0x0761, 0x83}, /* index 19, gain=40.218750 DB */
	{0x075f, 0x83}, /* index 20, gain=40.125000 DB */
	{0x075d, 0x83}, /* index 21, gain=40.031250 DB */
	{0x075c, 0x83}, /* index 22, gain=39.937500 DB */
	{0x075a, 0x83}, /* index 23, gain=39.843750 DB */
	{0x0758, 0x83}, /* index 24, gain=39.750000 DB */
	{0x0756, 0x83}, /* index 25, gain=39.656250 DB */
	{0x0754, 0x83}, /* index 26, gain=39.562500 DB */
	{0x0753, 0x83}, /* index 27, gain=39.468750 DB */
	{0x0751, 0x83}, /* index 28, gain=39.375000 DB */
	{0x074f, 0x83}, /* index 29, gain=39.281250 DB */
	{0x074d, 0x83}, /* index 30, gain=39.187500 DB */
	{0x074b, 0x83}, /* index 31, gain=39.093750 DB */
	{0x0749, 0x83}, /* index 32, gain=39.000000 DB */
	{0x0747, 0x83}, /* index 33, gain=38.906250 DB */
	{0x0745, 0x83}, /* index 34, gain=38.812500 DB */
	{0x0743, 0x83}, /* index 35, gain=38.718750 DB */
	{0x0741, 0x83}, /* index 36, gain=38.625000 DB */
	{0x073f, 0x83}, /* index 37, gain=38.531250 DB */
	{0x073d, 0x83}, /* index 38, gain=38.437500 DB */
	{0x073b, 0x83}, /* index 39, gain=38.343750 DB */
	{0x0739, 0x83}, /* index 40, gain=38.250000 DB */
	{0x0736, 0x83}, /* index 41, gain=38.156250 DB */
	{0x0734, 0x83}, /* index 42, gain=38.062500 DB */
	{0x0732, 0x83}, /* index 43, gain=37.968750 DB */
	{0x0730, 0x83}, /* index 44, gain=37.875000 DB */
	{0x072d, 0x83}, /* index 45, gain=37.781250 DB */
	{0x072b, 0x83}, /* index 46, gain=37.687500 DB */
	{0x0729, 0x83}, /* index 47, gain=37.593750 DB */
	{0x0727, 0x83}, /* index 48, gain=37.500000 DB */
	{0x0724, 0x83}, /* index 49, gain=37.406250 DB */
	{0x0722, 0x83}, /* index 50, gain=37.312500 DB */
	{0x071f, 0x83}, /* index 51, gain=37.218750 DB */
	{0x071d, 0x83}, /* index 52, gain=37.125000 DB */
	{0x071b, 0x83}, /* index 53, gain=37.031250 DB */
	{0x0718, 0x83}, /* index 54, gain=36.937500 DB */
	{0x0716, 0x83}, /* index 55, gain=36.843750 DB */
	{0x0713, 0x83}, /* index 56, gain=36.750000 DB */
	{0x0710, 0x83}, /* index 57, gain=36.656250 DB */
	{0x070e, 0x83}, /* index 58, gain=36.562500 DB */
	{0x070b, 0x83}, /* index 59, gain=36.468750 DB */
	{0x0709, 0x83}, /* index 60, gain=36.375000 DB */
	{0x0706, 0x83}, /* index 61, gain=36.281250 DB */
	{0x0703, 0x83}, /* index 62, gain=36.187500 DB */
	{0x0700, 0x83}, /* index 63, gain=36.093750 DB */
	{0x0780, 0x82}, /* index 64, gain=36.000000 DB */
	{0x077d, 0x82}, /* index 65, gain=35.906250 DB */
	{0x077b, 0x82}, /* index 66, gain=35.812500 DB */
	{0x077a, 0x82}, /* index 67, gain=35.718750 DB */
	{0x0779, 0x82}, /* index 68, gain=35.625000 DB */
	{0x0777, 0x82}, /* index 69, gain=35.531250 DB */
	{0x0776, 0x82}, /* index 70, gain=35.437500 DB */
	{0x0774, 0x82}, /* index 71, gain=35.343750 DB */
	{0x0773, 0x82}, /* index 72, gain=35.250000 DB */
	{0x0771, 0x82}, /* index 73, gain=35.156250 DB */
	{0x0770, 0x82}, /* index 74, gain=35.062500 DB */
	{0x076e, 0x82}, /* index 75, gain=34.968750 DB */
	{0x076c, 0x82}, /* index 76, gain=34.875000 DB */
	{0x076b, 0x82}, /* index 77, gain=34.781250 DB */
	{0x0769, 0x82}, /* index 78, gain=34.687500 DB */
	{0x0768, 0x82}, /* index 79, gain=34.593750 DB */
	{0x0766, 0x82}, /* index 80, gain=34.500000 DB */
	{0x0764, 0x82}, /* index 81, gain=34.406250 DB */
	{0x0763, 0x82}, /* index 82, gain=34.312500 DB */
	{0x0761, 0x82}, /* index 83, gain=34.218750 DB */
	{0x075f, 0x82}, /* index 84, gain=34.125000 DB */
	{0x075d, 0x82}, /* index 85, gain=34.031250 DB */
	{0x075c, 0x82}, /* index 86, gain=33.937500 DB */
	{0x075a, 0x82}, /* index 87, gain=33.843750 DB */
	{0x0758, 0x82}, /* index 88, gain=33.750000 DB */
	{0x0756, 0x82}, /* index 89, gain=33.656250 DB */
	{0x0754, 0x82}, /* index 90, gain=33.562500 DB */
	{0x0753, 0x82}, /* index 91, gain=33.468750 DB */
	{0x0751, 0x82}, /* index 92, gain=33.375000 DB */
	{0x074f, 0x82}, /* index 93, gain=33.281250 DB */
	{0x074d, 0x82}, /* index 94, gain=33.187500 DB */
	{0x074b, 0x82}, /* index 95, gain=33.093750 DB */
	{0x0749, 0x82}, /* index 96, gain=33.000000 DB */
	{0x0747, 0x82}, /* index 97, gain=32.906250 DB */
	{0x0745, 0x82}, /* index 98, gain=32.812500 DB */
	{0x0743, 0x82}, /* index 99, gain=32.718750 DB */
	{0x0741, 0x82}, /* index 100, gain=32.625000 DB */
	{0x073f, 0x82}, /* index 101, gain=32.531250 DB */
	{0x073d, 0x82}, /* index 102, gain=32.437500 DB */
	{0x073b, 0x82}, /* index 103, gain=32.343750 DB */
	{0x0739, 0x82}, /* index 104, gain=32.250000 DB */
	{0x0736, 0x82}, /* index 105, gain=32.156250 DB */
	{0x0734, 0x82}, /* index 106, gain=32.062500 DB */
	{0x0732, 0x82}, /* index 107, gain=31.968750 DB */
	{0x0730, 0x82}, /* index 108, gain=31.875000 DB */
	{0x072d, 0x82}, /* index 109, gain=31.781250 DB */
	{0x072b, 0x82}, /* index 110, gain=31.687500 DB */
	{0x0729, 0x82}, /* index 111, gain=31.593750 DB */
	{0x0727, 0x82}, /* index 112, gain=31.500000 DB */
	{0x0724, 0x82}, /* index 113, gain=31.406250 DB */
	{0x0722, 0x82}, /* index 114, gain=31.312500 DB */
	{0x071f, 0x82}, /* index 115, gain=31.218750 DB */
	{0x071d, 0x82}, /* index 116, gain=31.125000 DB */
	{0x071b, 0x82}, /* index 117, gain=31.031250 DB */
	{0x0718, 0x82}, /* index 118, gain=30.937500 DB */
	{0x0716, 0x82}, /* index 119, gain=30.843750 DB */
	{0x0713, 0x82}, /* index 120, gain=30.750000 DB */
	{0x0710, 0x82}, /* index 121, gain=30.656250 DB */
	{0x070e, 0x82}, /* index 122, gain=30.562500 DB */
	{0x070b, 0x82}, /* index 123, gain=30.468750 DB */
	{0x0709, 0x82}, /* index 124, gain=30.375000 DB */
	{0x0706, 0x82}, /* index 125, gain=30.281250 DB */
	{0x0703, 0x82}, /* index 126, gain=30.187500 DB */
	{0x0700, 0x82}, /* index 127, gain=30.093750 DB */
	{0x0780, 0x81}, /* index 128, gain=30.000000 DB */
	{0x077d, 0x81}, /* index 129, gain=29.906250 DB */
	{0x077b, 0x81}, /* index 130, gain=29.812500 DB */
	{0x077a, 0x81}, /* index 131, gain=29.718750 DB */
	{0x0779, 0x81}, /* index 132, gain=29.625000 DB */
	{0x0777, 0x81}, /* index 133, gain=29.531250 DB */
	{0x0776, 0x81}, /* index 134, gain=29.437500 DB */
	{0x0774, 0x81}, /* index 135, gain=29.343750 DB */
	{0x0773, 0x81}, /* index 136, gain=29.250000 DB */
	{0x0771, 0x81}, /* index 137, gain=29.156250 DB */
	{0x0770, 0x81}, /* index 138, gain=29.062500 DB */
	{0x076e, 0x81}, /* index 139, gain=28.968750 DB */
	{0x076c, 0x81}, /* index 140, gain=28.875000 DB */
	{0x076b, 0x81}, /* index 141, gain=28.781250 DB */
	{0x0769, 0x81}, /* index 142, gain=28.687500 DB */
	{0x0768, 0x81}, /* index 143, gain=28.593750 DB */
	{0x0766, 0x81}, /* index 144, gain=28.500000 DB */
	{0x0764, 0x81}, /* index 145, gain=28.406250 DB */
	{0x0763, 0x81}, /* index 146, gain=28.312500 DB */
	{0x0761, 0x81}, /* index 147, gain=28.218750 DB */
	{0x075f, 0x81}, /* index 148, gain=28.125000 DB */
	{0x075d, 0x81}, /* index 149, gain=28.031250 DB */
	{0x075c, 0x81}, /* index 150, gain=27.937500 DB */
	{0x075a, 0x81}, /* index 151, gain=27.843750 DB */
	{0x0758, 0x81}, /* index 152, gain=27.750000 DB */
	{0x0756, 0x81}, /* index 153, gain=27.656250 DB */
	{0x0754, 0x81}, /* index 154, gain=27.562500 DB */
	{0x0753, 0x81}, /* index 155, gain=27.468750 DB */
	{0x0751, 0x81}, /* index 156, gain=27.375000 DB */
	{0x074f, 0x81}, /* index 157, gain=27.281250 DB */
	{0x074d, 0x81}, /* index 158, gain=27.187500 DB */
	{0x074b, 0x81}, /* index 159, gain=27.093750 DB */
	{0x0749, 0x81}, /* index 160, gain=27.000000 DB */
	{0x0747, 0x81}, /* index 161, gain=26.906250 DB */
	{0x0745, 0x81}, /* index 162, gain=26.812500 DB */
	{0x0743, 0x81}, /* index 163, gain=26.718750 DB */
	{0x0741, 0x81}, /* index 164, gain=26.625000 DB */
	{0x073f, 0x81}, /* index 165, gain=26.531250 DB */
	{0x073d, 0x81}, /* index 166, gain=26.437500 DB */
	{0x073b, 0x81}, /* index 167, gain=26.343750 DB */
	{0x0739, 0x81}, /* index 168, gain=26.250000 DB */
	{0x0736, 0x81}, /* index 169, gain=26.156250 DB */
	{0x0734, 0x81}, /* index 170, gain=26.062500 DB */
	{0x0732, 0x81}, /* index 171, gain=25.968750 DB */
	{0x0730, 0x81}, /* index 172, gain=25.875000 DB */
	{0x072d, 0x81}, /* index 173, gain=25.781250 DB */
	{0x072b, 0x81}, /* index 174, gain=25.687500 DB */
	{0x0729, 0x81}, /* index 175, gain=25.593750 DB */
	{0x0727, 0x81}, /* index 176, gain=25.500000 DB */
	{0x0724, 0x81}, /* index 177, gain=25.406250 DB */
	{0x0722, 0x81}, /* index 178, gain=25.312500 DB */
	{0x071f, 0x81}, /* index 179, gain=25.218750 DB */
	{0x071d, 0x81}, /* index 180, gain=25.125000 DB */
	{0x071b, 0x81}, /* index 181, gain=25.031250 DB */
	{0x0718, 0x81}, /* index 182, gain=24.937500 DB */
	{0x0716, 0x81}, /* index 183, gain=24.843750 DB */
	{0x0713, 0x81}, /* index 184, gain=24.750000 DB */
	{0x0710, 0x81}, /* index 185, gain=24.656250 DB */
	{0x070e, 0x81}, /* index 186, gain=24.562500 DB */
	{0x070b, 0x81}, /* index 187, gain=24.468750 DB */
	{0x0709, 0x81}, /* index 188, gain=24.375000 DB */
	{0x0706, 0x81}, /* index 189, gain=24.281250 DB */
	{0x0703, 0x81}, /* index 190, gain=24.187500 DB */
	{0x0700, 0x81}, /* index 191, gain=24.093750 DB */
	{0x0780, 0x80}, /* index 192, gain=24.000000 DB */
	{0x077d, 0x80}, /* index 193, gain=23.906250 DB */
	{0x077b, 0x80}, /* index 194, gain=23.812500 DB */
	{0x077a, 0x80}, /* index 195, gain=23.718750 DB */
	{0x0779, 0x80}, /* index 196, gain=23.625000 DB */
	{0x0777, 0x80}, /* index 197, gain=23.531250 DB */
	{0x0776, 0x80}, /* index 198, gain=23.437500 DB */
	{0x0774, 0x80}, /* index 199, gain=23.343750 DB */
	{0x0773, 0x80}, /* index 200, gain=23.250000 DB */
	{0x0771, 0x80}, /* index 201, gain=23.156250 DB */
	{0x0770, 0x80}, /* index 202, gain=23.062500 DB */
	{0x076e, 0x80}, /* index 203, gain=22.968750 DB */
	{0x076c, 0x80}, /* index 204, gain=22.875000 DB */
	{0x076b, 0x80}, /* index 205, gain=22.781250 DB */
	{0x0769, 0x80}, /* index 206, gain=22.687500 DB */
	{0x0768, 0x80}, /* index 207, gain=22.593750 DB */
	{0x0766, 0x80}, /* index 208, gain=22.500000 DB */
	{0x0764, 0x80}, /* index 209, gain=22.406250 DB */
	{0x0763, 0x80}, /* index 210, gain=22.312500 DB */
	{0x0761, 0x80}, /* index 211, gain=22.218750 DB */
	{0x075f, 0x80}, /* index 212, gain=22.125000 DB */
	{0x075d, 0x80}, /* index 213, gain=22.031250 DB */
	{0x075c, 0x80}, /* index 214, gain=21.937500 DB */
	{0x075a, 0x80}, /* index 215, gain=21.843750 DB */
	{0x0758, 0x80}, /* index 216, gain=21.750000 DB */
	{0x0756, 0x80}, /* index 217, gain=21.656250 DB */
	{0x0754, 0x80}, /* index 218, gain=21.562500 DB */
	{0x0753, 0x80}, /* index 219, gain=21.468750 DB */
	{0x0751, 0x80}, /* index 220, gain=21.375000 DB */
	{0x074f, 0x80}, /* index 221, gain=21.281250 DB */
	{0x074d, 0x80}, /* index 222, gain=21.187500 DB */
	{0x074b, 0x80}, /* index 223, gain=21.093750 DB */
	{0x0749, 0x80}, /* index 224, gain=21.000000 DB */
	{0x0747, 0x80}, /* index 225, gain=20.906250 DB */
	{0x0745, 0x80}, /* index 226, gain=20.812500 DB */
	{0x0743, 0x80}, /* index 227, gain=20.718750 DB */
	{0x0741, 0x80}, /* index 228, gain=20.625000 DB */
	{0x073f, 0x80}, /* index 229, gain=20.531250 DB */
	{0x073d, 0x80}, /* index 230, gain=20.437500 DB */
	{0x073b, 0x80}, /* index 231, gain=20.343750 DB */
	{0x0739, 0x80}, /* index 232, gain=20.250000 DB */
	{0x0736, 0x80}, /* index 233, gain=20.156250 DB */
	{0x0734, 0x80}, /* index 234, gain=20.062500 DB */
	{0x0732, 0x80}, /* index 235, gain=19.968750 DB */
	{0x0730, 0x80}, /* index 236, gain=19.875000 DB */
	{0x072d, 0x80}, /* index 237, gain=19.781250 DB */
	{0x072b, 0x80}, /* index 238, gain=19.687500 DB */
	{0x0729, 0x80}, /* index 239, gain=19.593750 DB */
	{0x0727, 0x80}, /* index 240, gain=19.500000 DB */
	{0x0724, 0x80}, /* index 241, gain=19.406250 DB */
	{0x0722, 0x80}, /* index 242, gain=19.312500 DB */
	{0x071f, 0x80}, /* index 243, gain=19.218750 DB */
	{0x071d, 0x80}, /* index 244, gain=19.125000 DB */
	{0x071b, 0x80}, /* index 245, gain=19.031250 DB */
	{0x0718, 0x80}, /* index 246, gain=18.937500 DB */
	{0x0716, 0x80}, /* index 247, gain=18.843750 DB */
	{0x0713, 0x80}, /* index 248, gain=18.750000 DB */
	{0x0710, 0x80}, /* index 249, gain=18.656250 DB */
	{0x070e, 0x80}, /* index 250, gain=18.562500 DB */
	{0x070b, 0x80}, /* index 251, gain=18.468750 DB */
	{0x0709, 0x80}, /* index 252, gain=18.375000 DB */
	{0x0706, 0x80}, /* index 253, gain=18.281250 DB */
	{0x0703, 0x80}, /* index 254, gain=18.187500 DB */
	{0x0700, 0x80}, /* index 255, gain=18.093750 DB */
	{0x06fe, 0x80}, /* index 256, gain=18.000000 DB */
	{0x06fb, 0x80}, /* index 257, gain=17.906250 DB */
	{0x06f8, 0x80}, /* index 258, gain=17.812500 DB */
	{0x06f5, 0x80}, /* index 259, gain=17.718750 DB */
	{0x06f2, 0x80}, /* index 260, gain=17.625000 DB */
	{0x06ef, 0x80}, /* index 261, gain=17.531250 DB */
	{0x06ec, 0x80}, /* index 262, gain=17.437500 DB */
	{0x06e9, 0x80}, /* index 263, gain=17.343750 DB */
	{0x06e6, 0x80}, /* index 264, gain=17.250000 DB */
	{0x06e3, 0x80}, /* index 265, gain=17.156250 DB */
	{0x06e0, 0x80}, /* index 266, gain=17.062500 DB */
	{0x06dd, 0x80}, /* index 267, gain=16.968750 DB */
	{0x06da, 0x80}, /* index 268, gain=16.875000 DB */
	{0x06d7, 0x80}, /* index 269, gain=16.781250 DB */
	{0x06d4, 0x80}, /* index 270, gain=16.687500 DB */
	{0x06d0, 0x80}, /* index 271, gain=16.593750 DB */
	{0x06cd, 0x80}, /* index 272, gain=16.500000 DB */
	{0x06ca, 0x80}, /* index 273, gain=16.406250 DB */
	{0x06c6, 0x80}, /* index 274, gain=16.312500 DB */
	{0x06c3, 0x80}, /* index 275, gain=16.218750 DB */
	{0x06c0, 0x80}, /* index 276, gain=16.125000 DB */
	{0x06bc, 0x80}, /* index 277, gain=16.031250 DB */
	{0x06b9, 0x80}, /* index 278, gain=15.937500 DB */
	{0x06b5, 0x80}, /* index 279, gain=15.843750 DB */
	{0x06b1, 0x80}, /* index 280, gain=15.750000 DB */
	{0x06ae, 0x80}, /* index 281, gain=15.656250 DB */
	{0x06aa, 0x80}, /* index 282, gain=15.562500 DB */
	{0x06a6, 0x80}, /* index 283, gain=15.468750 DB */
	{0x06a3, 0x80}, /* index 284, gain=15.375000 DB */
	{0x069f, 0x80}, /* index 285, gain=15.281250 DB */
	{0x069b, 0x80}, /* index 286, gain=15.187500 DB */
	{0x0697, 0x80}, /* index 287, gain=15.093750 DB */
	{0x0693, 0x80}, /* index 288, gain=15.000000 DB */
	{0x068f, 0x80}, /* index 289, gain=14.906250 DB */
	{0x068b, 0x80}, /* index 290, gain=14.812500 DB */
	{0x0687, 0x80}, /* index 291, gain=14.718750 DB */
	{0x0683, 0x80}, /* index 292, gain=14.625000 DB */
	{0x067f, 0x80}, /* index 293, gain=14.531250 DB */
	{0x067b, 0x80}, /* index 294, gain=14.437500 DB */
	{0x0677, 0x80}, /* index 295, gain=14.343750 DB */
	{0x0672, 0x80}, /* index 296, gain=14.250000 DB */
	{0x066e, 0x80}, /* index 297, gain=14.156250 DB */
	{0x066a, 0x80}, /* index 298, gain=14.062500 DB */
	{0x0665, 0x80}, /* index 299, gain=13.968750 DB */
	{0x0661, 0x80}, /* index 300, gain=13.875000 DB */
	{0x065c, 0x80}, /* index 301, gain=13.781250 DB */
	{0x0658, 0x80}, /* index 302, gain=13.687500 DB */
	{0x0653, 0x80}, /* index 303, gain=13.593750 DB */
	{0x064f, 0x80}, /* index 304, gain=13.500000 DB */
	{0x064a, 0x80}, /* index 305, gain=13.406250 DB */
	{0x0645, 0x80}, /* index 306, gain=13.312500 DB */
	{0x0640, 0x80}, /* index 307, gain=13.218750 DB */
	{0x063c, 0x80}, /* index 308, gain=13.125000 DB */
	{0x0637, 0x80}, /* index 309, gain=13.031250 DB */
	{0x0632, 0x80}, /* index 310, gain=12.937500 DB */
	{0x062d, 0x80}, /* index 311, gain=12.843750 DB */
	{0x0628, 0x80}, /* index 312, gain=12.750000 DB */
	{0x0623, 0x80}, /* index 313, gain=12.656250 DB */
	{0x061d, 0x80}, /* index 314, gain=12.562500 DB */
	{0x0618, 0x80}, /* index 315, gain=12.468750 DB */
	{0x0613, 0x80}, /* index 316, gain=12.375000 DB */
	{0x060d, 0x80}, /* index 317, gain=12.281250 DB */
	{0x0608, 0x80}, /* index 318, gain=12.187500 DB */
	{0x0603, 0x80}, /* index 319, gain=12.093750 DB */
	{0x05fd, 0x80}, /* index 320, gain=12.000000 DB */
	{0x05f7, 0x80}, /* index 321, gain=11.906250 DB */
	{0x05f2, 0x80}, /* index 322, gain=11.812500 DB */
	{0x05ec, 0x80}, /* index 323, gain=11.718750 DB */
	{0x05e6, 0x80}, /* index 324, gain=11.625000 DB */
	{0x05e1, 0x80}, /* index 325, gain=11.531250 DB */
	{0x05db, 0x80}, /* index 326, gain=11.437500 DB */
	{0x05d5, 0x80}, /* index 327, gain=11.343750 DB */
	{0x05cf, 0x80}, /* index 328, gain=11.250000 DB */
	{0x05c9, 0x80}, /* index 329, gain=11.156250 DB */
	{0x05c2, 0x80}, /* index 330, gain=11.062500 DB */
	{0x05bc, 0x80}, /* index 331, gain=10.968750 DB */
	{0x05b6, 0x80}, /* index 332, gain=10.875000 DB */
	{0x05b0, 0x80}, /* index 333, gain=10.781250 DB */
	{0x05a9, 0x80}, /* index 334, gain=10.687500 DB */
	{0x05a3, 0x80}, /* index 335, gain=10.593750 DB */
	{0x059c, 0x80}, /* index 336, gain=10.500000 DB */
	{0x0595, 0x80}, /* index 337, gain=10.406250 DB */
	{0x058f, 0x80}, /* index 338, gain=10.312500 DB */
	{0x0588, 0x80}, /* index 339, gain=10.218750 DB */
	{0x0581, 0x80}, /* index 340, gain=10.125000 DB */
	{0x057a, 0x80}, /* index 341, gain=10.031250 DB */
	{0x0573, 0x80}, /* index 342, gain=9.937500 DB */
	{0x056c, 0x80}, /* index 343, gain=9.843750 DB */
	{0x0565, 0x80}, /* index 344, gain=9.750000 DB */
	{0x055e, 0x80}, /* index 345, gain=9.656250 DB */
	{0x0556, 0x80}, /* index 346, gain=9.562500 DB */
	{0x054f, 0x80}, /* index 347, gain=9.468750 DB */
	{0x0548, 0x80}, /* index 348, gain=9.375000 DB */
	{0x0540, 0x80}, /* index 349, gain=9.281250 DB */
	{0x0538, 0x80}, /* index 350, gain=9.187500 DB */
	{0x0531, 0x80}, /* index 351, gain=9.093750 DB */
	{0x0529, 0x80}, /* index 352, gain=9.000000 DB */
	{0x0521, 0x80}, /* index 353, gain=8.906250 DB */
	{0x0519, 0x80}, /* index 354, gain=8.812500 DB */
	{0x0511, 0x80}, /* index 355, gain=8.718750 DB */
	{0x0509, 0x80}, /* index 356, gain=8.625000 DB */
	{0x0501, 0x80}, /* index 357, gain=8.531250 DB */
	{0x04f8, 0x80}, /* index 358, gain=8.437500 DB */
	{0x04f0, 0x80}, /* index 359, gain=8.343750 DB */
	{0x04e7, 0x80}, /* index 360, gain=8.250000 DB */
	{0x04df, 0x80}, /* index 361, gain=8.156250 DB */
	{0x04d6, 0x80}, /* index 362, gain=8.062500 DB */
	{0x04cd, 0x80}, /* index 363, gain=7.968750 DB */
	{0x04c4, 0x80}, /* index 364, gain=7.875000 DB */
	{0x04bb, 0x80}, /* index 365, gain=7.781250 DB */
	{0x04b2, 0x80}, /* index 366, gain=7.687500 DB */
	{0x04a9, 0x80}, /* index 367, gain=7.593750 DB */
	{0x04a0, 0x80}, /* index 368, gain=7.500000 DB */
	{0x0496, 0x80}, /* index 369, gain=7.406250 DB */
	{0x048d, 0x80}, /* index 370, gain=7.312500 DB */
	{0x0483, 0x80}, /* index 371, gain=7.218750 DB */
	{0x047a, 0x80}, /* index 372, gain=7.125000 DB */
	{0x0470, 0x80}, /* index 373, gain=7.031250 DB */
	{0x0466, 0x80}, /* index 374, gain=6.937500 DB */
	{0x045c, 0x80}, /* index 375, gain=6.843750 DB */
	{0x0452, 0x80}, /* index 376, gain=6.750000 DB */
	{0x0448, 0x80}, /* index 377, gain=6.656250 DB */
	{0x043d, 0x80}, /* index 378, gain=6.562500 DB */
	{0x0433, 0x80}, /* index 379, gain=6.468750 DB */
	{0x0428, 0x80}, /* index 380, gain=6.375000 DB */
	{0x041e, 0x80}, /* index 381, gain=6.281250 DB */
	{0x0413, 0x80}, /* index 382, gain=6.187500 DB */
	{0x0408, 0x80}, /* index 383, gain=6.093750 DB */
	{0x03fd, 0x80}, /* index 384, gain=6.000000 DB */
	{0x03f2, 0x80}, /* index 385, gain=5.906250 DB */
	{0x03e7, 0x80}, /* index 386, gain=5.812500 DB */
	{0x03db, 0x80}, /* index 387, gain=5.718750 DB */
	{0x03d0, 0x80}, /* index 388, gain=5.625000 DB */
	{0x03c4, 0x80}, /* index 389, gain=5.531250 DB */
	{0x03b8, 0x80}, /* index 390, gain=5.437500 DB */
	{0x03ad, 0x80}, /* index 391, gain=5.343750 DB */
	{0x03a1, 0x80}, /* index 392, gain=5.250000 DB */
	{0x0394, 0x80}, /* index 393, gain=5.156250 DB */
	{0x0388, 0x80}, /* index 394, gain=5.062500 DB */
	{0x037c, 0x80}, /* index 395, gain=4.968750 DB */
	{0x036f, 0x80}, /* index 396, gain=4.875000 DB */
	{0x0362, 0x80}, /* index 397, gain=4.781250 DB */
	{0x0356, 0x80}, /* index 398, gain=4.687500 DB */
	{0x0349, 0x80}, /* index 399, gain=4.593750 DB */
	{0x033c, 0x80}, /* index 400, gain=4.500000 DB */
	{0x032e, 0x80}, /* index 401, gain=4.406250 DB */
	{0x0321, 0x80}, /* index 402, gain=4.312500 DB */
	{0x0313, 0x80}, /* index 403, gain=4.218750 DB */
	{0x0306, 0x80}, /* index 404, gain=4.125000 DB */
	{0x02f8, 0x80}, /* index 405, gain=4.031250 DB */
	{0x02ea, 0x80}, /* index 406, gain=3.937500 DB */
	{0x02dc, 0x80}, /* index 407, gain=3.843750 DB */
	{0x02ce, 0x80}, /* index 408, gain=3.750000 DB */
	{0x02bf, 0x80}, /* index 409, gain=3.656250 DB */
	{0x02b1, 0x80}, /* index 410, gain=3.562500 DB */
	{0x02a2, 0x80}, /* index 411, gain=3.468750 DB */
	{0x0293, 0x80}, /* index 412, gain=3.375000 DB */
	{0x0284, 0x80}, /* index 413, gain=3.281250 DB */
	{0x0275, 0x80}, /* index 414, gain=3.187500 DB */
	{0x0265, 0x80}, /* index 415, gain=3.093750 DB */
	{0x0256, 0x80}, /* index 416, gain=3.000000 DB */
	{0x0246, 0x80}, /* index 417, gain=2.906250 DB */
	{0x0236, 0x80}, /* index 418, gain=2.812500 DB */
	{0x0226, 0x80}, /* index 419, gain=2.718750 DB */
	{0x0216, 0x80}, /* index 420, gain=2.625000 DB */
	{0x0205, 0x80}, /* index 421, gain=2.531250 DB */
	{0x01f5, 0x80}, /* index 422, gain=2.437500 DB */
	{0x01e4, 0x80}, /* index 423, gain=2.343750 DB */
	{0x01d3, 0x80}, /* index 424, gain=2.250000 DB */
	{0x01c2, 0x80}, /* index 425, gain=2.156250 DB */
	{0x01b0, 0x80}, /* index 426, gain=2.062500 DB */
	{0x019f, 0x80}, /* index 427, gain=1.968750 DB */
	{0x018d, 0x80}, /* index 428, gain=1.875000 DB */
	{0x017b, 0x80}, /* index 429, gain=1.781250 DB */
	{0x0169, 0x80}, /* index 430, gain=1.687500 DB */
	{0x0157, 0x80}, /* index 431, gain=1.593750 DB */
	{0x0144, 0x80}, /* index 432, gain=1.500000 DB */
	{0x0132, 0x80}, /* index 433, gain=1.406250 DB */
	{0x011f, 0x80}, /* index 434, gain=1.312500 DB */
	{0x010c, 0x80}, /* index 435, gain=1.218750 DB */
	{0x00f8, 0x80}, /* index 436, gain=1.125000 DB */
	{0x00e5, 0x80}, /* index 437, gain=1.031250 DB */
	{0x00d1, 0x80}, /* index 438, gain=0.937500 DB */
	{0x00bd, 0x80}, /* index 439, gain=0.843750 DB */
	{0x00a9, 0x80}, /* index 440, gain=0.750000 DB */
	{0x0095, 0x80}, /* index 441, gain=0.656250 DB */
	{0x0080, 0x80}, /* index 442, gain=0.562500 DB */
	{0x006b, 0x80}, /* index 443, gain=0.468750 DB */
	{0x0056, 0x80}, /* index 444, gain=0.375000 DB */
	{0x0041, 0x80}, /* index 445, gain=0.281250 DB */
	{0x002b, 0x80}, /* index 446, gain=0.187500 DB */
	{0x0015, 0x80}, /* index 447, gain=0.093750 DB */
	{0x0000, 0x80}, /* index 448, gain=0.000000 DB */
};


