/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/imx122_reg_tbl.c
 *
 * History:
 *    2011/09/23 - [Bingliang Hu] Create
 *    2012/06/29 - [Long Zhao] Add 720p60 mode
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
static const struct imx122_video_format_reg_table imx122_video_format_tbl = {
	.reg	= {
		0x0202,// MODE
		0x0203,// HMAX_LSB
		0x0204,// HMAX_MSB
		0x0205,// VMAX_LSB
		0x0206,// VMAX_MSB
		0x0211,// FRSEL/OPORTSEL/M12BEN
		0x0212,// ADRES
		0x0214,// WINPH_LSB
		0x0215,// WINPH_MSB
		0x0216,// WINPV_LSB
		0x0217,// WINPV_MSB
		0x0218,// WINWH_LSB
		0x0219,// WINWH_MSB
		0x021A,// WINWV_LSB
		0x021B,// WINWV_MSB
		0x0220,// BLKLEVEL
		0x0221,// 10BITA
		0x0222,// 720PMODE
		0x027A,// 10BITB
		0x027B,// 10BITC
		0x0298,// 10B1080P_LSB
		0x0299,// 10B1080P_MSB
		0x029A,// 12B1080P_LSB
		0x029B,// 12B1080P_MSB
		0x02CE,// PRES
		0x02CF,// DRES_LSB
		0x02D0,// DRES_MSB
		0x022C,// MASTER
		0x0200,// STANDBY
	},
	.table[0]	= {	//1920x1080
		.ext_reg_fill	= NULL,
		.data	= {
			0x0F,// MODE
			0x4C,// HMAX_LSB
			0x04,// HMAX_MSB
			0x65,// VMAX_LSB
			0x04,// VMAX_MSB
			0x00,// FRSEL/OPORTSEL/M12BEN
			0x82,// ADRES
			0x00,// WINPH_LSB
			0x00,// WINPH_MSB
			0x3C,// WINPV_LSB
			0x00,// WINPV_MSB
			0xC0,// WINWH_LSB
			0x07,// WINWH_MSB
			0x51,// WINWV_LSB
			0x04,// WINWV_MSB
			0xF0,// BLKLEVEL
			0x00,// 10BITA
			0x00,// 720PMODE
			0x00,// 10BITB
			0x00,// 10BITC
			0x26,// 10B1080P_LSB
			0x02,// 10B1080P_MSB
			0x26,// 12B1080P_LSB
			0x02,// 12B1080P_MSB
			0x16,// PRES
			0x82,// DRES_LSB
			0x00,// DRES_MSB
			0x00,// MASTER
			0x30,// STANDBY
		},
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.auto_fps	= AMBA_VIDEO_FPS_29_97,
		.pll_index	= 0,
	},
	.table[1]	= {	//1280x720@60fps
		.ext_reg_fill	= NULL,
		.data	= {
			0x01,// MODE
			0x39,// HMAX_LSB
			0x03,// HMAX_MSB
			0xEE,// VMAX_LSB
			0x02,// VMAX_MSB
			0x00,// FRSEL/OPORTSEL/M12BEN
			0x80,// ADRES
			0x40,// WINPH_LSB
			0x01,// WINPH_MSB
			0xF0,// WINPV_LSB
			0x00,// WINPV_MSB
			0x40,// WINWH_LSB
			0x05,// WINWH_MSB
			0xE9,// WINWV_LSB
			0x02,// WINWV_MSB
			0x3C,// BLKLEVEL
			0x00,// 10BITA
			0x80,// 720PMODE
			0x00,// 10BITB
			0x00,// 10BITC
			0x26,// 10B1080P_LSB
			0x02,// 10B1080P_MSB
			0x4C,// 12B1080P_LSB
			0x04,// 12B1080P_MSB
			0x00,// PRES
			0x00,// DRES_LSB
			0x00,// DRES_MSB
			0x00,// MASTER
			0x30,// STANDBY
		},
		.width		= 1280,
		.height		= 720,
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
};


#define IMX122_GAIN_ROWS	121
#define IMX122_GAIN_COLS	2
#define IMX122_GAIN_DOUBLE	20
#define IMX122_GAIN_0DB		120

#define IMX122_ANALOG_GAIN_0DB		120
#define IMX122_ANALOG_GAIN_18DB		120


#if 0
/* This is analog gain table, IMX122_ANALOG_GAIN_ROWS = 61, The step is 0.3dB */
/* For digital gain, the step is 6dB, Max = 18dB */
const u16 IMX122_ANALOG_GAIN_TABLE[IMX122_ANALOG_GAIN_0DB + 1][2] =
{
	{0x037F,	/*18.0dB   */0x3f},
	{0x037B,	/*17.7dB   */0x3f},
	{0x0376,	/*17.4dB   */0x3f},
	{0x0371,	/*17.1dB   */0x3f},
	{0x122C,	/*16.8dB   */0x3f},
	{0x1227,	/*16.5dB   */0x3f},
	{0x1221,	/*16.2dB   */0x3f},
	{0x035C,	/*15.9dB   */0x3f},
	{0x0356,	/*15.6dB   */0x3f},
	{0x0350,	/*15.3dB   */0x3f},
	{0x034A,	/*15.0dB   */0x3f},
	{0x0344,	/*14.7dB   */0x3f},
	{0x033D,	/*14.4dB   */0x3f},
	{0x032F,	/*13.8dB   */0x3f},
	{0x0328,	/*13.5dB   */0x3f},
	{0x0320,	/*13.2dB   */0x3f},
	{0x0318,	/*12.9dB   */0x3f},
	{0x0310,	/*12.6dB   */0x3f},
	{0x0308,	/*12.3dB   */0x3f},
	{0x02FF,	/*12.0dB   */0x3f},
	{0x02F6,	/*11.7dB   */0x3f},
	{0x02EC,	/*11.4dB   */0x3f},
	{0x02E3,	/*11.1dB   */0x3f},
	{0x02D9,	/*10.8dB   */0x3f},
	{0x02CE,	/*10.5dB   */0x3f},
	{0x02C4,	/*10.2dB   */0x3f},
	{0x02B8,	/*9.9dB   */0x3f},
	{0x02AD,	/*9.6dB   */0x3f},
	{0x02A1,	/*9.3dB   */0x3f},
	{0x0295,	/*9.0dB   */0x3f},
	{0x0288,	/*8.7dB   */0x3f},
	{0x027B,	/*8.4dB   */0x3f},
	{0x026D,	/*8.1dB   */0x3f},
	{0x025F,	/*7.8dB   */0x3f},
	{0x0250,	/*7.5dB   */0x3f},
	{0x0241,	/*7.2dB   */0x3f},
	{0x0231,	/*6.9dB   */0x3f},
	{0x0221,	/*6.6dB   */0x3f},
	{0x0210,	/*6.3dB   */0x3f},
	{0x01FF,	/*6.0dB   */0x3f},
	{0x01ED,	/*5.7dB   */0x3f},
	{0x01DA,	/*5.4dB   */0x3f},
	{0x01C7,	/*5.1dB   */0x3f},
	{0x01B3,	/*4.8dB   */0x3f},
	{0x019E,	/*4.5dB   */0x3f},
	{0x0189,	/*4.2dB   */0x3f},
	{0x0172,	/*3.9dB   */0x3f},
	{0x015B,	/*3.6dB   */0x3f},
	{0x0144,	/*3.3dB   */0x3f},
	{0x012B,	/*3.0dB   */0x3f},
	{0x0112,	/*2.7dB   */0x3f},
	{0x00F7,	/*2.4dB   */0x3f},
	{0x00DC,	/*2.1dB   */0x3f},
	{0x00C0,	/*1.8dB   */0x3f},
	{0x00A2,	/*1.5dB   */0x3f},
	{0x0084,	/*1.2dB   */0x3f},
	{0x0065,	/*0.9dB   */0x3f},
	{0x0044,	/*0.6dB   */0x3f},
	{0x0023,	/*0.3dB   */0x3f},
	{0x0000,	/*0.0dB   */0x3f},

	{0x01ED,	/*5.7dB   */0x2a},
	{0x01DA,	/*5.4dB   */0x2a},
	{0x01C7,	/*5.1dB   */0x2a},
	{0x01B3,	/*4.8dB   */0x2a},
	{0x019E,	/*4.5dB   */0x2a},
	{0x0189,	/*4.2dB   */0x2a},
	{0x0172,	/*3.9dB   */0x2a},
	{0x015B,	/*3.6dB   */0x2a},
	{0x0144,	/*3.3dB   */0x2a},
	{0x012B,	/*3.0dB   */0x2a},
	{0x0112,	/*2.7dB   */0x2a},
	{0x00F7,	/*2.4dB   */0x2a},
	{0x00DC,	/*2.1dB   */0x2a},
	{0x00C0,	/*1.8dB   */0x2a},
	{0x00A2,	/*1.5dB   */0x2a},
	{0x0084,	/*1.2dB   */0x2a},
	{0x0065,	/*0.9dB   */0x2a},
	{0x0044,	/*0.6dB   */0x2a},
	{0x0023,	/*0.3dB   */0x2a},
	{0x0000,	/*0.0dB   */0x2a},

	{0x01ED,	/*5.7dB   */0x15},
	{0x01DA,	/*5.4dB   */0x15},
	{0x01C7,	/*5.1dB   */0x15},
	{0x01B3,	/*4.8dB   */0x15},
	{0x019E,	/*4.5dB   */0x15},
	{0x0189,	/*4.2dB   */0x15},
	{0x0172,	/*3.9dB   */0x15},
	{0x015B,	/*3.6dB   */0x15},
	{0x0144,	/*3.3dB   */0x15},
	{0x012B,	/*3.0dB   */0x15},
	{0x0112,	/*2.7dB   */0x15},
	{0x00F7,	/*2.4dB   */0x15},
	{0x00DC,	/*2.1dB   */0x15},
	{0x00C0,	/*1.8dB   */0x15},
	{0x00A2,	/*1.5dB   */0x15},
	{0x0084,	/*1.2dB   */0x15},
	{0x0065,	/*0.9dB   */0x15},
	{0x0044,	/*0.6dB   */0x15},
	{0x0023,	/*0.3dB   */0x15},
	{0x0000,	/*0.0dB   */0x15},

	{0x01ED,	/*5.7dB   */0x00},
	{0x01DA,	/*5.4dB   */0x00},
	{0x01C7,	/*5.1dB   */0x00},
	{0x01B3,	/*4.8dB   */0x00},
	{0x019E,	/*4.5dB   */0x00},
	{0x0189,	/*4.2dB   */0x00},
	{0x0172,	/*3.9dB   */0x00},
	{0x015B,	/*3.6dB   */0x00},
	{0x0144,	/*3.3dB   */0x00},
	{0x012B,	/*3.0dB   */0x00},
	{0x0112,	/*2.7dB   */0x00},
	{0x00F7,	/*2.4dB   */0x00},
	{0x00DC,	/*2.1dB   */0x00},
	{0x00C0,	/*1.8dB   */0x00},
	{0x00A2,	/*1.5dB   */0x00},
	{0x0084,	/*1.2dB   */0x00},
	{0x0065,	/*0.9dB   */0x00},
	{0x0044,	/*0.6dB   */0x00},
	{0x0023,	/*0.3dB   */0x00},
	{0x0000,	/*0.0dB   */0x00},
};
#endif


