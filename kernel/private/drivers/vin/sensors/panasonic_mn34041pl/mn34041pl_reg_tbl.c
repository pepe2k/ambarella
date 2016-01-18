/*
 * Filename : mn34041pl_reg_tbl.c
 *
 * History:
 *2011/01/12 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

	/*   < rember to update MN34041PL_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
	/*   < rember to update MN34041PL_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct mn34041pl_video_format_reg_table mn34041pl_video_format_tbl = {
	.reg = {
		/* add video format related register here */
		},
	.table[0] = {		//2016*1108 12bits@60fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2016,
		.height		= 1108,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits		= AMBA_VIDEO_BITS_12,//TODO
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		},
	.table[1] = {		//2016*1108 10bits@120fps
		.ext_reg_fill = NULL,
		.data = {
		},
		.width		= 2016,
		.height		= 1108,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits		= AMBA_VIDEO_BITS_10,//TODO
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.srm		= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
		},
	/* add video format table here, if necessary */
};


/** MN34041PL global gain table row size */
#define MN34041PL_GAIN_COLS		(3)
#if 0
#define MN34041PL_GAIN_ROWS		(321)
#define MN34041PL_GAIN_0DB		(320)
#else
#define MN34041PL_GAIN_ROWS		(321 + 16*8)		/* add 12db gain to gain table */
#define MN34041PL_GAIN_0DB		(320 + 16*8)		/* add 12db gain to gain table */
#endif

#define MN34041PL_GAIN_COL_REG_CGAIN	(0)
#define MN34041PL_GAIN_COL_REG_AGAIN	(1)
#define MN34041PL_GAIN_COL_REG_DGAIN	(2)

/* This is 64-step gain table, MN34041PL_GAIN_ROWS = 359, MN34041PL_GAIN_COLS = 3 */
const s16 MN34041PL_GAIN_TABLE[MN34041PL_GAIN_ROWS][MN34041PL_GAIN_COLS] =
{
	/* Column Gain,Analog Gain,Digital Gain */
#if 0
	{0xC000, 0x00FF, 0x0140},		/* 42 db =12db column amplifer gain, ~12 db analog gain, 18 db digital gain */

	{0xC000, 0x00FF, 0x013F},
	{0xC000, 0x00FF, 0x013E},
	{0xC000, 0x00FF, 0x013D},
	{0xC000, 0x00FF, 0x013C},
	{0xC000, 0x00FF, 0x013B},
	{0xC000, 0x00FF, 0x013A},
	{0xC000, 0x00FF, 0x0139},
	{0xC000, 0x00FF, 0x0138},
	{0xC000, 0x00FF, 0x0137},
	{0xC000, 0x00FF, 0x0136},
	{0xC000, 0x00FF, 0x0135},
	{0xC000, 0x00FF, 0x0134},
	{0xC000, 0x00FF, 0x0133},
	{0xC000, 0x00FF, 0x0132},
	{0xC000, 0x00FF, 0x0131},
	{0xC000, 0x00FF, 0x0130},

	{0xC000, 0x00FF, 0x012F},
	{0xC000, 0x00FF, 0x012E},
	{0xC000, 0x00FF, 0x012D},
	{0xC000, 0x00FF, 0x012C},
	{0xC000, 0x00FF, 0x012B},
	{0xC000, 0x00FF, 0x012A},
	{0xC000, 0x00FF, 0x0129},
	{0xC000, 0x00FF, 0x0128},
	{0xC000, 0x00FF, 0x0127},
	{0xC000, 0x00FF, 0x0126},
	{0xC000, 0x00FF, 0x0125},
	{0xC000, 0x00FF, 0x0124},
	{0xC000, 0x00FF, 0x0123},
	{0xC000, 0x00FF, 0x0122},
	{0xC000, 0x00FF, 0x0121},
	{0xC000, 0x00FF, 0x0120},		/* 39 db */


	{0xC000, 0x00FF, 0x011F},
	{0xC000, 0x00FF, 0x011E},
	{0xC000, 0x00FF, 0x011D},
	{0xC000, 0x00FF, 0x011C},
	{0xC000, 0x00FF, 0x011B},
	{0xC000, 0x00FF, 0x011A},
	{0xC000, 0x00FF, 0x0119},
	{0xC000, 0x00FF, 0x0118},
	{0xC000, 0x00FF, 0x0117},
	{0xC000, 0x00FF, 0x0116},
	{0xC000, 0x00FF, 0x0115},
	{0xC000, 0x00FF, 0x0114},
	{0xC000, 0x00FF, 0x0113},
	{0xC000, 0x00FF, 0x0112},
	{0xC000, 0x00FF, 0x0111},
	{0xC000, 0x00FF, 0x0110},

	{0xC000, 0x00FF, 0x010F},
	{0xC000, 0x00FF, 0x010E},
	{0xC000, 0x00FF, 0x010D},
	{0xC000, 0x00FF, 0x010C},
	{0xC000, 0x00FF, 0x010B},
	{0xC000, 0x00FF, 0x010A},
	{0xC000, 0x00FF, 0x0109},
	{0xC000, 0x00FF, 0x0108},
	{0xC000, 0x00FF, 0x0107},
	{0xC000, 0x00FF, 0x0106},
	{0xC000, 0x00FF, 0x0105},
	{0xC000, 0x00FF, 0x0104},
	{0xC000, 0x00FF, 0x0103},
	{0xC000, 0x00FF, 0x0102},
	{0xC000, 0x00FF, 0x0101},
	{0xC000, 0x00FF, 0x0100},		/* 36 db = 12db column amplifer gain, ~12 db analog gain, 12 db digital gain */
#else
	{0xC000, 0x00C0, 0x0180},		/* 42 db =12db column amplifer gain, ~6 db analog gain, 24 db digital gain */

	{0xC000, 0x00C0, 0x017F},
	{0xC000, 0x00C0, 0x017E},
	{0xC000, 0x00C0, 0x017D},
	{0xC000, 0x00C0, 0x017C},
	{0xC000, 0x00C0, 0x017B},
	{0xC000, 0x00C0, 0x017A},
	{0xC000, 0x00C0, 0x0179},
	{0xC000, 0x00C0, 0x0178},
	{0xC000, 0x00C0, 0x0177},
	{0xC000, 0x00C0, 0x0176},
	{0xC000, 0x00C0, 0x0175},
	{0xC000, 0x00C0, 0x0174},
	{0xC000, 0x00C0, 0x0173},
	{0xC000, 0x00C0, 0x0172},
	{0xC000, 0x00C0, 0x0171},
	{0xC000, 0x00C0, 0x0170},

	{0xC000, 0x00C0, 0x016F},
	{0xC000, 0x00C0, 0x016E},
	{0xC000, 0x00C0, 0x016D},
	{0xC000, 0x00C0, 0x016C},
	{0xC000, 0x00C0, 0x016B},
	{0xC000, 0x00C0, 0x016A},
	{0xC000, 0x00C0, 0x0169},
	{0xC000, 0x00C0, 0x0168},
	{0xC000, 0x00C0, 0x0167},
	{0xC000, 0x00C0, 0x0166},
	{0xC000, 0x00C0, 0x0165},
	{0xC000, 0x00C0, 0x0164},
	{0xC000, 0x00C0, 0x0163},
	{0xC000, 0x00C0, 0x0162},
	{0xC000, 0x00C0, 0x0161},
	{0xC000, 0x00C0, 0x0160},		/* 39 db */


	{0xC000, 0x00C0, 0x015F},
	{0xC000, 0x00C0, 0x015E},
	{0xC000, 0x00C0, 0x015D},
	{0xC000, 0x00C0, 0x015C},
	{0xC000, 0x00C0, 0x015B},
	{0xC000, 0x00C0, 0x015A},
	{0xC000, 0x00C0, 0x0159},
	{0xC000, 0x00C0, 0x0158},
	{0xC000, 0x00C0, 0x0157},
	{0xC000, 0x00C0, 0x0156},
	{0xC000, 0x00C0, 0x0155},
	{0xC000, 0x00C0, 0x0154},
	{0xC000, 0x00C0, 0x0153},
	{0xC000, 0x00C0, 0x0152},
	{0xC000, 0x00C0, 0x0151},
	{0xC000, 0x00C0, 0x0150},

	{0xC000, 0x00C0, 0x014F},
	{0xC000, 0x00C0, 0x014E},
	{0xC000, 0x00C0, 0x014D},
	{0xC000, 0x00C0, 0x014C},
	{0xC000, 0x00C0, 0x014B},
	{0xC000, 0x00C0, 0x014A},
	{0xC000, 0x00C0, 0x0149},
	{0xC000, 0x00C0, 0x0148},
	{0xC000, 0x00C0, 0x0147},
	{0xC000, 0x00C0, 0x0146},
	{0xC000, 0x00C0, 0x0145},
	{0xC000, 0x00C0, 0x0144},
	{0xC000, 0x00C0, 0x0143},
	{0xC000, 0x00C0, 0x0142},
	{0xC000, 0x00C0, 0x0141},
	{0xC000, 0x00C0, 0x0140},		/* 36 db = 12db column amplifer gain, ~6 db analog gain, 18 db digital gain */
#endif

	{0xC000, 0x00C0, 0x013F},
	{0xC000, 0x00C0, 0x013E},
	{0xC000, 0x00C0, 0x013D},
	{0xC000, 0x00C0, 0x013C},
	{0xC000, 0x00C0, 0x013B},
	{0xC000, 0x00C0, 0x013A},
	{0xC000, 0x00C0, 0x0139},
	{0xC000, 0x00C0, 0x0138},
	{0xC000, 0x00C0, 0x0137},
	{0xC000, 0x00C0, 0x0136},
	{0xC000, 0x00C0, 0x0135},
	{0xC000, 0x00C0, 0x0134},
	{0xC000, 0x00C0, 0x0133},
	{0xC000, 0x00C0, 0x0132},
	{0xC000, 0x00C0, 0x0131},
	{0xC000, 0x00C0, 0x0130},

	{0xC000, 0x00C0, 0x012F},
	{0xC000, 0x00C0, 0x012E},
	{0xC000, 0x00C0, 0x012D},
	{0xC000, 0x00C0, 0x012C},
	{0xC000, 0x00C0, 0x012B},
	{0xC000, 0x00C0, 0x012A},
	{0xC000, 0x00C0, 0x0129},
	{0xC000, 0x00C0, 0x0128},
	{0xC000, 0x00C0, 0x0127},
	{0xC000, 0x00C0, 0x0126},
	{0xC000, 0x00C0, 0x0125},
	{0xC000, 0x00C0, 0x0124},
	{0xC000, 0x00C0, 0x0123},
	{0xC000, 0x00C0, 0x0122},
	{0xC000, 0x00C0, 0x0121},
	{0xC000, 0x00C0, 0x0120},		 /* 33 db*/

	{0xC000, 0x00C0, 0x011F},
	{0xC000, 0x00C0, 0x011E},
	{0xC000, 0x00C0, 0x011D},
	{0xC000, 0x00C0, 0x011C},
	{0xC000, 0x00C0, 0x011B},
	{0xC000, 0x00C0, 0x011A},
	{0xC000, 0x00C0, 0x0119},
	{0xC000, 0x00C0, 0x0118},
	{0xC000, 0x00C0, 0x0117},
	{0xC000, 0x00C0, 0x0116},
	{0xC000, 0x00C0, 0x0115},
	{0xC000, 0x00C0, 0x0114},
	{0xC000, 0x00C0, 0x0113},
	{0xC000, 0x00C0, 0x0112},
	{0xC000, 0x00C0, 0x0111},
	{0xC000, 0x00C0, 0x0110},

	{0xC000, 0x00C0, 0x010F},
	{0xC000, 0x00C0, 0x010E},
	{0xC000, 0x00C0, 0x010D},
	{0xC000, 0x00C0, 0x010C},
	{0xC000, 0x00C0, 0x010B},
	{0xC000, 0x00C0, 0x010A},
	{0xC000, 0x00C0, 0x0109},
	{0xC000, 0x00C0, 0x0108},
	{0xC000, 0x00C0, 0x0107},
	{0xC000, 0x00C0, 0x0106},
	{0xC000, 0x00C0, 0x0105},
	{0xC000, 0x00C0, 0x0104},
	{0xC000, 0x00C0, 0x0103},
	{0xC000, 0x00C0, 0x0102},
	{0xC000, 0x00C0, 0x0101},
	{0xC000, 0x00C0, 0x0100},		/* 30 db*/

	{0xC000, 0x00C0, 0x00FF},
	{0xC000, 0x00C0, 0x00FE},
	{0xC000, 0x00C0, 0x00FD},
	{0xC000, 0x00C0, 0x00FC},
	{0xC000, 0x00C0, 0x00FB},
	{0xC000, 0x00C0, 0x00FA},
	{0xC000, 0x00C0, 0x00F9},
	{0xC000, 0x00C0, 0x00F8},
	{0xC000, 0x00C0, 0x00F7},
	{0xC000, 0x00C0, 0x00F6},
	{0xC000, 0x00C0, 0x00F5},
	{0xC000, 0x00C0, 0x00F4},
	{0xC000, 0x00C0, 0x00F3},
	{0xC000, 0x00C0, 0x00F2},
	{0xC000, 0x00C0, 0x00F1},
	{0xC000, 0x00C0, 0x00F0},

	{0xC000, 0x00C0, 0x00EF},
	{0xC000, 0x00C0, 0x00EE},
	{0xC000, 0x00C0, 0x00ED},
	{0xC000, 0x00C0, 0x00EC},
	{0xC000, 0x00C0, 0x00EB},
	{0xC000, 0x00C0, 0x00EA},
	{0xC000, 0x00C0, 0x00E9},
	{0xC000, 0x00C0, 0x00E8},
	{0xC000, 0x00C0, 0x00E7},
	{0xC000, 0x00C0, 0x00E6},
	{0xC000, 0x00C0, 0x00E5},
	{0xC000, 0x00C0, 0x00E4},
	{0xC000, 0x00C0, 0x00E3},
	{0xC000, 0x00C0, 0x00E2},
	{0xC000, 0x00C0, 0x00E1},
	{0xC000, 0x00C0, 0x00E0},		/*27 db */

	{0xC000, 0x00C0, 0x00DF},
	{0xC000, 0x00C0, 0x00DE},
	{0xC000, 0x00C0, 0x00DD},
	{0xC000, 0x00C0, 0x00DC},
	{0xC000, 0x00C0, 0x00DB},
	{0xC000, 0x00C0, 0x00DA},
	{0xC000, 0x00C0, 0x00D9},
	{0xC000, 0x00C0, 0x00D8},
	{0xC000, 0x00C0, 0x00D7},
	{0xC000, 0x00C0, 0x00D6},
	{0xC000, 0x00C0, 0x00D5},
	{0xC000, 0x00C0, 0x00D4},
	{0xC000, 0x00C0, 0x00D3},
	{0xC000, 0x00C0, 0x00D2},
	{0xC000, 0x00C0, 0x00D1},
	{0xC000, 0x00C0, 0x00D0},

	{0xC000, 0x00C0, 0x00CF},
	{0xC000, 0x00C0, 0x00CE},
	{0xC000, 0x00C0, 0x00CD},
	{0xC000, 0x00C0, 0x00CC},
	{0xC000, 0x00C0, 0x00CB},
	{0xC000, 0x00C0, 0x00CA},
	{0xC000, 0x00C0, 0x00C9},
	{0xC000, 0x00C0, 0x00C8},
	{0xC000, 0x00C0, 0x00C7},
	{0xC000, 0x00C0, 0x00C6},
	{0xC000, 0x00C0, 0x00C5},
	{0xC000, 0x00C0, 0x00C4},
	{0xC000, 0x00C0, 0x00C3},
	{0xC000, 0x00C0, 0x00C2},
	{0xC000, 0x00C0, 0x00C1},
	{0xC000, 0x00C0, 0x00C0},		/* 24 db,*/

	{0xC000, 0x00C0, 0x00BF},
	{0xC000, 0x00C0, 0x00BE},
	{0xC000, 0x00C0, 0x00BD},
	{0xC000, 0x00C0, 0x00BC},
	{0xC000, 0x00C0, 0x00BB},
	{0xC000, 0x00C0, 0x00BA},
	{0xC000, 0x00C0, 0x00B9},
	{0xC000, 0x00C0, 0x00B8},
	{0xC000, 0x00C0, 0x00B7},
	{0xC000, 0x00C0, 0x00B6},
	{0xC000, 0x00C0, 0x00B5},
	{0xC000, 0x00C0, 0x00B4},
	{0xC000, 0x00C0, 0x00B3},
	{0xC000, 0x00C0, 0x00B2},
	{0xC000, 0x00C0, 0x00B1},
	{0xC000, 0x00C0, 0x00B0},

	{0xC000, 0x00C0, 0x00AF},
	{0xC000, 0x00C0, 0x00AE},
	{0xC000, 0x00C0, 0x00AD},
	{0xC000, 0x00C0, 0x00AC},
	{0xC000, 0x00C0, 0x00AB},
	{0xC000, 0x00C0, 0x00AA},
	{0xC000, 0x00C0, 0x00A9},
	{0xC000, 0x00C0, 0x00A8},
	{0xC000, 0x00C0, 0x00A7},
	{0xC000, 0x00C0, 0x00A6},
	{0xC000, 0x00C0, 0x00A5},
	{0xC000, 0x00C0, 0x00A4},
	{0xC000, 0x00C0, 0x00A3},
	{0xC000, 0x00C0, 0x00A2},
	{0xC000, 0x00C0, 0x00A1},
	{0xC000, 0x00C0, 0x00A0},		/* 21 db */

	{0xC000, 0x00C0, 0x009F},
	{0xC000, 0x00C0, 0x009E},
	{0xC000, 0x00C0, 0x009D},
	{0xC000, 0x00C0, 0x009C},
	{0xC000, 0x00C0, 0x009B},
	{0xC000, 0x00C0, 0x009A},
	{0xC000, 0x00C0, 0x0099},
	{0xC000, 0x00C0, 0x0098},
	{0xC000, 0x00C0, 0x0097},
	{0xC000, 0x00C0, 0x0096},
	{0xC000, 0x00C0, 0x0095},
	{0xC000, 0x00C0, 0x0094},
	{0xC000, 0x00C0, 0x0093},
	{0xC000, 0x00C0, 0x0092},
	{0xC000, 0x00C0, 0x0091},
	{0xC000, 0x00C0, 0x0090},

	{0xC000, 0x00C0, 0x008F},
	{0xC000, 0x00C0, 0x008E},
	{0xC000, 0x00C0, 0x008D},
	{0xC000, 0x00C0, 0x008C},
	{0xC000, 0x00C0, 0x008B},
	{0xC000, 0x00C0, 0x008A},
	{0xC000, 0x00C0, 0x0089},
	{0xC000, 0x00C0, 0x0088},
	{0xC000, 0x00C0, 0x0087},
	{0xC000, 0x00C0, 0x0086},
	{0xC000, 0x00C0, 0x0085},
	{0xC000, 0x00C0, 0x0084},
	{0xC000, 0x00C0, 0x0083},
	{0xC000, 0x00C0, 0x0082},
	{0xC000, 0x00C0, 0x0081},
	{0xC000, 0x00C0, 0x0080},		/* 18 db */

	{0xC000, 0x0080, 0x00BF},
	{0xC000, 0x0080, 0x00BE},
	{0xC000, 0x0080, 0x00BD},
	{0xC000, 0x0080, 0x00BC},
	{0xC000, 0x0080, 0x00BB},
	{0xC000, 0x0080, 0x00BA},
	{0xC000, 0x0080, 0x00B9},
	{0xC000, 0x0080, 0x00B8},
	{0xC000, 0x0080, 0x00B7},
	{0xC000, 0x0080, 0x00B6},
	{0xC000, 0x0080, 0x00B5},
	{0xC000, 0x0080, 0x00B4},
	{0xC000, 0x0080, 0x00B3},
	{0xC000, 0x0080, 0x00B2},
	{0xC000, 0x0080, 0x00B1},
	{0xC000, 0x0080, 0x00B0},

	{0xC000, 0x0080, 0x00AF},
	{0xC000, 0x0080, 0x00AE},
	{0xC000, 0x0080, 0x00AD},
	{0xC000, 0x0080, 0x00AC},
	{0xC000, 0x0080, 0x00AB},
	{0xC000, 0x0080, 0x00AA},
	{0xC000, 0x0080, 0x00A9},
	{0xC000, 0x0080, 0x00A8},
	{0xC000, 0x0080, 0x00A7},
	{0xC000, 0x0080, 0x00A6},
	{0xC000, 0x0080, 0x00A5},
	{0xC000, 0x0080, 0x00A4},
	{0xC000, 0x0080, 0x00A3},
	{0xC000, 0x0080, 0x00A2},
	{0xC000, 0x0080, 0x00A1},
	{0xC000, 0x0080, 0x00A0},		/* 15 db */

	{0xC000, 0x0080, 0x009F},
	{0xC000, 0x0080, 0x009E},
	{0xC000, 0x0080, 0x009D},
	{0xC000, 0x0080, 0x009C},
	{0xC000, 0x0080, 0x009B},
	{0xC000, 0x0080, 0x009A},
	{0xC000, 0x0080, 0x0099},
	{0xC000, 0x0080, 0x0098},
	{0xC000, 0x0080, 0x0097},
	{0xC000, 0x0080, 0x0096},
	{0xC000, 0x0080, 0x0095},
	{0xC000, 0x0080, 0x0094},
	{0xC000, 0x0080, 0x0093},
	{0xC000, 0x0080, 0x0092},
	{0xC000, 0x0080, 0x0091},
	{0xC000, 0x0080, 0x0090},

	{0xC000, 0x0080, 0x008F},  
	{0xC000, 0x0080, 0x008E},
	{0xC000, 0x0080, 0x008D},
	{0xC000, 0x0080, 0x008C},
	{0xC000, 0x0080, 0x008B},
	{0xC000, 0x0080, 0x008A},
	{0xC000, 0x0080, 0x0089},
	{0xC000, 0x0080, 0x0088},
	{0xC000, 0x0080, 0x0087},
	{0xC000, 0x0080, 0x0086},
	{0xC000, 0x0080, 0x0085},
	{0xC000, 0x0080, 0x0084},
	{0xC000, 0x0080, 0x0083},
	{0xC000, 0x0080, 0x0082},
	{0xC000, 0x0080, 0x0081},
	{0xC000, 0x0080, 0x0080},		/* 12 db*/

	{0x8000, 0x0080, 0x00BF},
	{0x8000, 0x0080, 0x00BE},
	{0x8000, 0x0080, 0x00BD},
	{0x8000, 0x0080, 0x00BC},
	{0x8000, 0x0080, 0x00BB},
	{0x8000, 0x0080, 0x00BA},
	{0x8000, 0x0080, 0x00B9},
	{0x8000, 0x0080, 0x00B8},
	{0x8000, 0x0080, 0x00B7},
	{0x8000, 0x0080, 0x00B6},
	{0x8000, 0x0080, 0x00B5},
	{0x8000, 0x0080, 0x00B4},
	{0x8000, 0x0080, 0x00B3},
	{0x8000, 0x0080, 0x00B2},
	{0x8000, 0x0080, 0x00B1},
	{0x8000, 0x0080, 0x00B0},

	{0x8000, 0x0080, 0x00AF},
	{0x8000, 0x0080, 0x00AE},
	{0x8000, 0x0080, 0x00AD},
	{0x8000, 0x0080, 0x00AC},
	{0x8000, 0x0080, 0x00AB},
	{0x8000, 0x0080, 0x00AA},
	{0x8000, 0x0080, 0x00A9},
	{0x8000, 0x0080, 0x00A8},
	{0x8000, 0x0080, 0x00A7},
	{0x8000, 0x0080, 0x00A6},
	{0x8000, 0x0080, 0x00A5},
	{0x8000, 0x0080, 0x00A4},
	{0x8000, 0x0080, 0x00A3},
	{0x8000, 0x0080, 0x00A2},
	{0x8000, 0x0080, 0x00A1},
	{0x8000, 0x0080, 0x00A0},		/* 9 db */

	{0x8000, 0x0080, 0x009F},
	{0x8000, 0x0080, 0x009E},
	{0x8000, 0x0080, 0x009D},
	{0x8000, 0x0080, 0x009C},
	{0x8000, 0x0080, 0x009B},
	{0x8000, 0x0080, 0x009A},
	{0x8000, 0x0080, 0x0099},
	{0x8000, 0x0080, 0x0098},
	{0x8000, 0x0080, 0x0097},
	{0x8000, 0x0080, 0x0096},
	{0x8000, 0x0080, 0x0095},
	{0x8000, 0x0080, 0x0094},
	{0x8000, 0x0080, 0x0093},
	{0x8000, 0x0080, 0x0092},
	{0x8000, 0x0080, 0x0091},
	{0x8000, 0x0080, 0x0090},

	{0x8000, 0x0080, 0x008F},
	{0x8000, 0x0080, 0x008E},
	{0x8000, 0x0080, 0x008D},
	{0x8000, 0x0080, 0x008C},
	{0x8000, 0x0080, 0x008B},
	{0x8000, 0x0080, 0x008A},
	{0x8000, 0x0080, 0x0089},
	{0x8000, 0x0080, 0x0088},
	{0x8000, 0x0080, 0x0087},
	{0x8000, 0x0080, 0x0086},
	{0x8000, 0x0080, 0x0085},
	{0x8000, 0x0080, 0x0084},
	{0x8000, 0x0080, 0x0083},
	{0x8000, 0x0080, 0x0082},
	{0x8000, 0x0080, 0x0081},
	{0x8000, 0x0080, 0x0080},		/* 6 db */

	{0x0000, 0x0080, 0x00BF},
	{0x0000, 0x0080, 0x00BE},
	{0x0000, 0x0080, 0x00BD},
	{0x0000, 0x0080, 0x00BC},
	{0x0000, 0x0080, 0x00BB},
	{0x0000, 0x0080, 0x00BA},
	{0x0000, 0x0080, 0x00B9},
	{0x0000, 0x0080, 0x00B8},
	{0x0000, 0x0080, 0x00B7},
	{0x0000, 0x0080, 0x00B6},
	{0x0000, 0x0080, 0x00B5},
	{0x0000, 0x0080, 0x00B4},
	{0x0000, 0x0080, 0x00B3},
	{0x0000, 0x0080, 0x00B2},
	{0x0000, 0x0080, 0x00B1},
	{0x0000, 0x0080, 0x00B0},

	{0x0000, 0x0080, 0x00AF},
	{0x0000, 0x0080, 0x00AE},
	{0x0000, 0x0080, 0x00AD},
	{0x0000, 0x0080, 0x00AC},
	{0x0000, 0x0080, 0x00AB},
	{0x0000, 0x0080, 0x00AA},
	{0x0000, 0x0080, 0x00A9},
	{0x0000, 0x0080, 0x00A8},
	{0x0000, 0x0080, 0x00A7},
	{0x0000, 0x0080, 0x00A6},
	{0x0000, 0x0080, 0x00A5},
	{0x0000, 0x0080, 0x00A4},
	{0x0000, 0x0080, 0x00A3},
	{0x0000, 0x0080, 0x00A2},
	{0x0000, 0x0080, 0x00A1},
	{0x0000, 0x0080, 0x00A0},		/* 3 db */

	{0x0000, 0x0080, 0x009F},
	{0x0000, 0x0080, 0x009E},
	{0x0000, 0x0080, 0x009D},
	{0x0000, 0x0080, 0x009C},
	{0x0000, 0x0080, 0x009B},
	{0x0000, 0x0080, 0x009A},
	{0x0000, 0x0080, 0x0099},
	{0x0000, 0x0080, 0x0098},
	{0x0000, 0x0080, 0x0097},
	{0x0000, 0x0080, 0x0096},
	{0x0000, 0x0080, 0x0095},
	{0x0000, 0x0080, 0x0094},
	{0x0000, 0x0080, 0x0093},
	{0x0000, 0x0080, 0x0092},
	{0x0000, 0x0080, 0x0091},
	{0x0000, 0x0080, 0x0090},

	{0x0000, 0x0080, 0x008F},
	{0x0000, 0x0080, 0x008E},
	{0x0000, 0x0080, 0x008D},
	{0x0000, 0x0080, 0x008C},
	{0x0000, 0x0080, 0x008B},
	{0x0000, 0x0080, 0x008A},
	{0x0000, 0x0080, 0x0089},
	{0x0000, 0x0080, 0x0088},
	{0x0000, 0x0080, 0x0087},
	{0x0000, 0x0080, 0x0086},
	{0x0000, 0x0080, 0x0085},
	{0x0000, 0x0080, 0x0084},
	{0x0000, 0x0080, 0x0083},
	{0x0000, 0x0080, 0x0082},
	{0x0000, 0x0080, 0x0081},
	{0x0000, 0x0080, 0x0080},		/* 0 db */
};