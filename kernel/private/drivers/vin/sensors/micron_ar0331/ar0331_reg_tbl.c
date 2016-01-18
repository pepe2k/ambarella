/*
 * Filename : ar0331_reg_tbl.c
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
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

	/*   < rember to update AR0331_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
	/*   < rember to update AR0331_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct ar0331_video_format_reg_table ar0331_video_format_tbl = {
	.reg = {
		/* add video format related register here */
		0x3002,		//Y_ADDR_START
		0x3004,		//X_ADDR_START
		0x3006,		//Y_ADDR_END
		0x3008,		//X_ADDR_END
		0x300A,		//FRAME_LENGTH_LINE
		0x300C,		//LINE_LENGTH_PCK
		0x3012,		//Coarse_Integration_Time
		0x3014,		//FINE_INTEGRATION_TIME
		0x30A2,		//X_ODD_INCREMENT
		0x30A6,		//Y_ODD_INCREMENT
		0x3040,		//READ MODE
		0x31AE		//SERIAL_FORMAT
		},

	.table[0] = {		//2048x1536
		.ext_reg_fill = NULL,
		.data = {
			/* set video format related register here */
			4,
			6,
			1539,
			2053,
			1657,
			1120,
			0x031B,
			0x0000,
			1,
			1,
			0,
			0x0304
		},
		.width	= 2048,
		.height	= 1540,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		},
	.table[1] = {		//1920x1080
		.ext_reg_fill = NULL,
		.data = {
			/* set video format related register here */
			228,
			66,
			1307,
			1993,
			1125,
			2200,
			0x0464,
			0x0000,
			1,
			1,
			0,
			0x0304
		},
		.width	= 1920,
		.height	= 1084,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		},

	.table[2] = {		//1280x720
		.ext_reg_fill = NULL,
		.data = {
			/* set video format related register here */
			412,
			390,
			1131,
			1669,
			757,
			1634,
			0x02C6,
			0x0000,
			1,
			1,
			0,
			0x0304
		},
		.width	= 1280,
		.height	= 724,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_59_94,
		.pll_index = 0,
		},
		/* add video format table here, if necessary */
};

#if 0
/** AR0331 global gain table row size */
#define AR0331_GAIN_ROWS		(897)
#define AR0331_GAIN_COLS		(3)
#define AR0331_GAIN_DOUBLE		(128)
#define AR0331_GAIN_0DB		(896)

#define AR0331_GAIN_COL_AGC		(0)
#define AR0331_GAIN_COL_REG_AGAIN	(1)
#define AR0331_GAIN_COL_REG_DGAIN	(2)

/* This is 64-step gain table, AR0331_GAIN_ROWS = 359, AR0331_GAIN_COLS = 3 */
const s16 AR0331_GAIN_TABLE[AR0331_GAIN_ROWS][AR0331_GAIN_COLS] =
{
	{ 0x8000 , 0x30,	0x7DE},		/*index: 0, 42.1215dB */
	{ 0x7F4F , 0x30,	0x7D3},		/*index: 1, 42.0972dB */
	{ 0x7E9F , 0x30,	0x7C8},		/*index: 2, 42.0501dB */
	{ 0x7DF0 , 0x30,	0x7BD},		/*index: 3, 42.0031dB */
	{ 0x7D42 , 0x30,	0x7B3},		/*index: 4, 41.9561dB */
	{ 0x7C95 , 0x30,	0x7A8},		/*index: 5, 41.909dB */
	{ 0x7BE8 , 0x30,	0x79E},		/*index: 6, 41.862dB */
	{ 0x7B3D , 0x30,	0x793},		/*index: 7, 41.8149dB */
	{ 0x7A93 , 0x30,	0x789},		/*index: 8, 41.7679dB */
	{ 0x79E9 , 0x30,	0x77E},		/*index: 9, 41.7209dB */
	{ 0x7941 , 0x30,	0x774},		/*index: 10, 41.6738dB */
	{ 0x7899 , 0x30,	0x76A},		/*index: 11, 41.6268dB */
	{ 0x77F2 , 0x30,	0x75F},		/*index: 12, 41.5798dB */
	{ 0x774D , 0x30,	0x755},		/*index: 13, 41.5327dB */
	{ 0x76A8 , 0x30,	0x74B},		/*index: 14, 41.4857dB */
	{ 0x7604 , 0x30,	0x741},		/*index: 15, 41.4387dB */
	{ 0x7560 , 0x30,	0x737},		/*index: 16, 41.3916dB */
	{ 0x74BE , 0x30,	0x72D},		/*index: 17, 41.3446dB */
	{ 0x741D , 0x30,	0x723},		/*index: 18, 41.2976dB */
	{ 0x737C , 0x30,	0x719},		/*index: 19, 41.2505dB */
	{ 0x72DD , 0x30,	0x710},		/*index: 20, 41.2035dB */
	{ 0x723E , 0x30,	0x706},		/*index: 21, 41.1564dB */
	{ 0x71A0 , 0x30,	0x6FC},		/*index: 22, 41.1094dB */
	{ 0x7103 , 0x30,	0x6F3},		/*index: 23, 41.0624dB */
	{ 0x7066 , 0x30,	0x6E9},		/*index: 24, 41.0153dB */
	{ 0x6FCB , 0x30,	0x6E0},		/*index: 25, 40.9683dB */
	{ 0x6F30 , 0x30,	0x6D6},		/*index: 26, 40.9213dB */
	{ 0x6E97 , 0x30,	0x6CD},		/*index: 27, 40.8742dB */
	{ 0x6DFE , 0x30,	0x6C3},		/*index: 28, 40.8272dB */
	{ 0x6D66 , 0x30,	0x6BA},		/*index: 29, 40.7802dB */
	{ 0x6CCF , 0x30,	0x6B1},		/*index: 30, 40.7331dB */
	{ 0x6C38 , 0x30,	0x6A7},		/*index: 31, 40.6861dB */
	{ 0x6BA2 , 0x30,	0x69E},		/*index: 32, 40.639dB */
	{ 0x6B0E , 0x30,	0x695},		/*index: 33, 40.592dB */
	{ 0x6A7A , 0x30,	0x68C},		/*index: 34, 40.545dB */
	{ 0x69E6 , 0x30,	0x683},		/*index: 35, 40.4979dB */
	{ 0x6954 , 0x30,	0x67A},		/*index: 36, 40.4509dB */
	{ 0x68C2 , 0x30,	0x671},		/*index: 37, 40.4039dB */
	{ 0x6832 , 0x30,	0x668},		/*index: 38, 40.3568dB */
	{ 0x67A2 , 0x30,	0x65F},		/*index: 39, 40.3098dB */
	{ 0x6712 , 0x30,	0x657},		/*index: 40, 40.2628dB */
	{ 0x6684 , 0x30,	0x64E},		/*index: 41, 40.2157dB */
	{ 0x65F6 , 0x30,	0x645},		/*index: 42, 40.1687dB */
	{ 0x6569 , 0x30,	0x63D},		/*index: 43, 40.1217dB */
	{ 0x64DD , 0x30,	0x634},		/*index: 44, 40.0746dB */
	{ 0x6451 , 0x30,	0x62B},		/*index: 45, 40.0276dB */
	{ 0x63C7 , 0x30,	0x623},		/*index: 46, 39.9805dB */
	{ 0x633D , 0x30,	0x61B},		/*index: 47, 39.9335dB */
	{ 0x62B4 , 0x30,	0x612},		/*index: 48, 39.8865dB */
	{ 0x622B , 0x30,	0x60A},		/*index: 49, 39.8394dB */
	{ 0x61A3 , 0x30,	0x601},		/*index: 50, 39.7924dB */
	{ 0x611C , 0x30,	0x5F9},		/*index: 51, 39.7454dB */
	{ 0x6096 , 0x30,	0x5F1},		/*index: 52, 39.6983dB */
	{ 0x6011 , 0x30,	0x5E9},		/*index: 53, 39.6513dB */
	{ 0x5F8C , 0x30,	0x5E1},		/*index: 54, 39.6043dB */
	{ 0x5F08 , 0x30,	0x5D8},		/*index: 55, 39.5572dB */
	{ 0x5E84 , 0x30,	0x5D0},		/*index: 56, 39.5102dB */
	{ 0x5E02 , 0x30,	0x5C8},		/*index: 57, 39.4632dB */
	{ 0x5D80 , 0x30,	0x5C0},		/*index: 58, 39.4161dB */
	{ 0x5CFE , 0x30,	0x5B9},		/*index: 59, 39.3691dB */
	{ 0x5C7E , 0x30,	0x5B1},		/*index: 60, 39.322dB */
	{ 0x5BFE , 0x30,	0x5A9},		/*index: 61, 39.275dB */
	{ 0x5B7F , 0x30,	0x5A1},		/*index: 62, 39.228dB */
	{ 0x5B00 , 0x30,	0x599},		/*index: 63, 39.1809dB */
	{ 0x5A82 , 0x30,	0x592},		/*index: 64, 39.1339dB */
	{ 0x5A05 , 0x30,	0x58A},		/*index: 65, 39.0869dB */
	{ 0x5989 , 0x30,	0x582},		/*index: 66, 39.0398dB */
	{ 0x590D , 0x30,	0x57B},		/*index: 67, 38.9928dB */
	{ 0x5892 , 0x30,	0x573},		/*index: 68, 38.9458dB */
	{ 0x5818 , 0x30,	0x56C},		/*index: 69, 38.8987dB */
	{ 0x579E , 0x30,	0x564},		/*index: 70, 38.8517dB */
	{ 0x5725 , 0x30,	0x55D},		/*index: 71, 38.8046dB */
	{ 0x56AC , 0x30,	0x555},		/*index: 72, 38.7576dB */
	{ 0x5634 , 0x30,	0x54E},		/*index: 73, 38.7106dB */
	{ 0x55BD , 0x30,	0x547},		/*index: 74, 38.6635dB */
	{ 0x5547 , 0x30,	0x53F},		/*index: 75, 38.6165dB */
	{ 0x54D1 , 0x30,	0x538},		/*index: 76, 38.5695dB */
	{ 0x545B , 0x30,	0x531},		/*index: 77, 38.5224dB */
	{ 0x53E7 , 0x30,	0x52A},		/*index: 78, 38.4754dB */
	{ 0x5373 , 0x30,	0x523},		/*index: 79, 38.4284dB */
	{ 0x52FF , 0x30,	0x51C},		/*index: 80, 38.3813dB */
	{ 0x528D , 0x30,	0x514},		/*index: 81, 38.3343dB */
	{ 0x521B , 0x30,	0x50D},		/*index: 82, 38.2873dB */
	{ 0x51A9 , 0x30,	0x507},		/*index: 83, 38.2402dB */
	{ 0x5138 , 0x30,	0x500},		/*index: 84, 38.1932dB */
	{ 0x50C8 , 0x30,	0x4F9},		/*index: 85, 38.1461dB */
	{ 0x5058 , 0x30,	0x4F2},		/*index: 86, 38.0991dB */
	{ 0x4FE9 , 0x30,	0x4EB},		/*index: 87, 38.0521dB */
	{ 0x4F7B , 0x30,	0x4E4},		/*index: 88, 38.005dB */
	{ 0x4F0D , 0x30,	0x4DE},		/*index: 89, 37.958dB */
	{ 0x4E9F , 0x30,	0x4D7},		/*index: 90, 37.911dB */
	{ 0x4E33 , 0x30,	0x4D0},		/*index: 91, 37.8639dB */
	{ 0x4DC7 , 0x30,	0x4CA},		/*index: 92, 37.8169dB */
	{ 0x4D5B , 0x30,	0x4C3},		/*index: 93, 37.7699dB */
	{ 0x4CF0 , 0x30,	0x4BC},		/*index: 94, 37.7228dB */
	{ 0x4C86 , 0x30,	0x4B6},		/*index: 95, 37.6758dB */
	{ 0x4C1C , 0x30,	0x4AF},		/*index: 96, 37.6287dB */
	{ 0x4BB3 , 0x30,	0x4A9},		/*index: 97, 37.5817dB */
	{ 0x4B4A , 0x30,	0x4A2},		/*index: 98, 37.5347dB */
	{ 0x4AE2 , 0x30,	0x49C},		/*index: 99, 37.4876dB */
	{ 0x4A7A , 0x30,	0x496},		/*index: 100, 37.4406dB */
	{ 0x4A13 , 0x30,	0x48F},		/*index: 101, 37.3936dB */
	{ 0x49AD , 0x30,	0x489},		/*index: 102, 37.3465dB */
	{ 0x4947 , 0x30,	0x483},		/*index: 103, 37.2995dB */
	{ 0x48E2 , 0x30,	0x47D},		/*index: 104, 37.2525dB */
	{ 0x487D , 0x30,	0x476},		/*index: 105, 37.2054dB */
	{ 0x4819 , 0x30,	0x470},		/*index: 106, 37.1584dB */
	{ 0x47B5 , 0x30,	0x46A},		/*index: 107, 37.1114dB */
	{ 0x4752 , 0x30,	0x464},		/*index: 108, 37.0643dB */
	{ 0x46F0 , 0x30,	0x45E},		/*index: 109, 37.0173dB */
	{ 0x468D , 0x30,	0x458},		/*index: 110, 36.9702dB */
	{ 0x462C , 0x30,	0x452},		/*index: 111, 36.9232dB */
	{ 0x45CB , 0x30,	0x44C},		/*index: 112, 36.8762dB */
	{ 0x456A , 0x30,	0x446},		/*index: 113, 36.8291dB */
	{ 0x450A , 0x30,	0x440},		/*index: 114, 36.7821dB */
	{ 0x44AB , 0x30,	0x43A},		/*index: 115, 36.7351dB */
	{ 0x444C , 0x30,	0x435},		/*index: 116, 36.688dB */
	{ 0x43EE , 0x30,	0x42F},		/*index: 117, 36.641dB */
	{ 0x4390 , 0x30,	0x429},		/*index: 118, 36.594dB */
	{ 0x4332 , 0x30,	0x423},		/*index: 119, 36.5469dB */
	{ 0x42D5 , 0x30,	0x41E},		/*index: 120, 36.4999dB */
	{ 0x4279 , 0x30,	0x418},		/*index: 121, 36.4529dB */
	{ 0x421D , 0x30,	0x412},		/*index: 122, 36.4058dB */
	{ 0x41C2 , 0x30,	0x40D},		/*index: 123, 36.3588dB */
	{ 0x4167 , 0x30,	0x407},		/*index: 124, 36.3117dB */
	{ 0x410C , 0x30,	0x402},		/*index: 125, 36.2647dB */
	{ 0x40B2 , 0x30,	0x3FC},		/*index: 126, 36.2177dB */
	{ 0x4059 , 0x30,	0x3F6},	///	/*index: 127, 36.1706dB */
	{ 0x4000 , 0x30,	0x3F1},		/*index: 128, 36.1236dB */
	{ 0x3FA8 , 0x30,	0x3EC},		/*index: 129, 36.0766dB */
	{ 0x3F50 , 0x30,	0x3E6},		/*index: 130, 36.0295dB */
	{ 0x3EF8 , 0x30,	0x3E1},		/*index: 131, 35.9825dB */
	{ 0x3EA1 , 0x30,	0x3DB},		/*index: 132, 35.9355dB */
	{ 0x3E4A , 0x30,	0x3D6},		/*index: 133, 35.8884dB */
	{ 0x3DF4 , 0x30,	0x3D1},		/*index: 134, 35.8414dB */
	{ 0x3D9F , 0x30,	0x3CC},		/*index: 135, 35.7943dB */
	{ 0x3D49 , 0x30,	0x3C6},		/*index: 136, 35.7473dB */
	{ 0x3CF5 , 0x30,	0x3C1},		/*index: 137, 35.7003dB */
	{ 0x3CA0 , 0x30,	0x3BC},		/*index: 138, 35.6532dB */
	{ 0x3C4D , 0x30,	0x3B7},		/*index: 139, 35.6062dB */
	{ 0x3BF9 , 0x30,	0x3B2},		/*index: 140, 35.5592dB */
	{ 0x3BA6 , 0x30,	0x3AD},		/*index: 141, 35.5121dB */
	{ 0x3B54 , 0x30,	0x3A8},		/*index: 142, 35.4651dB */
	{ 0x3B02 , 0x30,	0x3A3},		/*index: 143, 35.4181dB */
	{ 0x3AB0 , 0x30,	0x39E},		/*index: 144, 35.371dB */
	{ 0x3A5F , 0x30,	0x399},		/*index: 145, 35.324dB */
	{ 0x3A0E , 0x30,	0x394},		/*index: 146, 35.277dB */
	{ 0x39BE , 0x30,	0x38F},		/*index: 147, 35.2299dB */
	{ 0x396E , 0x30,	0x38A},		/*index: 148, 35.1829dB */
	{ 0x391F , 0x30,	0x385},		/*index: 149, 35.1358dB */
	{ 0x38D0 , 0x30,	0x380},		/*index: 150, 35.0888dB */
	{ 0x3881 , 0x30,	0x37B},		/*index: 151, 35.0418dB */
	{ 0x3833 , 0x30,	0x376},		/*index: 152, 34.9947dB */
	{ 0x37E6 , 0x30,	0x372},		/*index: 153, 34.9477dB */
	{ 0x3798 , 0x30,	0x36D},		/*index: 154, 34.9007dB */
	{ 0x374B , 0x30,	0x368},		/*index: 155, 34.8536dB */
	{ 0x36FF , 0x30,	0x363},		/*index: 156, 34.8066dB */
	{ 0x36B3 , 0x30,	0x35F},		/*index: 157, 34.7596dB */
	{ 0x3667 , 0x30,	0x35A},		/*index: 158, 34.7125dB */
	{ 0x361C , 0x30,	0x356},		/*index: 159, 34.6655dB */
	{ 0x35D1 , 0x30,	0x351},		/*index: 160, 34.6184dB */
	{ 0x3587 , 0x30,	0x34C},		/*index: 161, 34.5714dB */
	{ 0x353D , 0x30,	0x348},		/*index: 162, 34.5244dB */
	{ 0x34F3 , 0x30,	0x343},		/*index: 163, 34.4773dB */
	{ 0x34AA , 0x30,	0x33F},		/*index: 164, 34.4303dB */
	{ 0x3461 , 0x30,	0x33A},		/*index: 165, 34.3833dB */
	{ 0x3419 , 0x30,	0x336},		/*index: 166, 34.3362dB */
	{ 0x33D1 , 0x30,	0x331},		/*index: 167, 34.2892dB */
	{ 0x3389 , 0x30,	0x32D},		/*index: 168, 34.2422dB */
	{ 0x3342 , 0x30,	0x329},		/*index: 169, 34.1951dB */
	{ 0x32FB , 0x30,	0x324},		/*index: 170, 34.1481dB */
	{ 0x32B5 , 0x30,	0x320},		/*index: 171, 34.1011dB */
	{ 0x326E , 0x30,	0x31C},		/*index: 172, 34.054dB */
	{ 0x3229 , 0x30,	0x317},		/*index: 173, 34.007dB */
	{ 0x31E3 , 0x30,	0x313},		/*index: 174, 33.9599dB */
	{ 0x319E , 0x30,	0x30F},		/*index: 175, 33.9129dB */
	{ 0x315A , 0x30,	0x30B},		/*index: 176, 33.8659dB */
	{ 0x3116 , 0x30,	0x306},		/*index: 177, 33.8188dB */
	{ 0x30D2 , 0x30,	0x302},		/*index: 178, 33.7718dB */
	{ 0x308E , 0x30,	0x2FE},		/*index: 179, 33.7248dB */
	{ 0x304B , 0x30,	0x2FA},		/*index: 180, 33.6777dB */
	{ 0x3008 , 0x30,	0x2F6},		/*index: 181, 33.6307dB */
	{ 0x2FC6 , 0x30,	0x2F2},		/*index: 182, 33.5837dB */
	{ 0x2F84 , 0x30,	0x2EE},		/*index: 183, 33.5366dB */
	{ 0x2F42 , 0x30,	0x2EA},		/*index: 184, 33.4896dB */
	{ 0x2F01 , 0x30,	0x2E6},		/*index: 185, 33.4426dB */
	{ 0x2EC0 , 0x30,	0x2E2},		/*index: 186, 33.3955dB */
	{ 0x2E7F , 0x30,	0x2DE},		/*index: 187, 33.3485dB */
	{ 0x2E3F , 0x30,	0x2DA},		/*index: 188, 33.3014dB */
	{ 0x2DFF , 0x30,	0x2D6},		/*index: 189, 33.2544dB */
	{ 0x2DBF , 0x30,	0x2D2},		/*index: 190, 33.2074dB */
	{ 0x2D80 , 0x30,	0x2CE},		/*index: 191, 33.1603dB */
	{ 0x2D41 , 0x30,	0x2CA},		/*index: 192, 33.1133dB */
	{ 0x2D03 , 0x30,	0x2C6},		/*index: 193, 33.0663dB */
	{ 0x2CC4 , 0x30,	0x2C3},		/*index: 194, 33.0192dB */
	{ 0x2C87 , 0x30,	0x2BF},		/*index: 195, 32.9722dB */
	{ 0x2C49 , 0x30,	0x2BB},		/*index: 196, 32.9252dB */
	{ 0x2C0C , 0x30,	0x2B7},		/*index: 197, 32.8781dB */
	{ 0x2BCF , 0x30,	0x2B3},		/*index: 198, 32.8311dB */
	{ 0x2B92 , 0x30,	0x2B0},		/*index: 199, 32.784dB */
	{ 0x2B56 , 0x30,	0x2AC},		/*index: 200, 32.737dB */
	{ 0x2B1A , 0x30,	0x2A8},		/*index: 201, 32.69dB */
	{ 0x2ADF , 0x30,	0x2A5},		/*index: 202, 32.6429dB */
	{ 0x2AA3 , 0x30,	0x2A1},		/*index: 203, 32.5959dB */
	{ 0x2A68 , 0x30,	0x29D},		/*index: 204, 32.5489dB */
	{ 0x2A2E , 0x30,	0x29A},		/*index: 205, 32.5018dB */
	{ 0x29F3 , 0x30,	0x296},		/*index: 206, 32.4548dB */
	{ 0x29B9 , 0x30,	0x293},		/*index: 207, 32.4078dB */
	{ 0x2980 , 0x30,	0x28F},		/*index: 208, 32.3607dB */
	{ 0x2946 , 0x30,	0x28C},		/*index: 209, 32.3137dB */
	{ 0x290D , 0x30,	0x288},		/*index: 210, 32.2667dB */
	{ 0x28D5 , 0x30,	0x285},		/*index: 211, 32.2196dB */
	{ 0x289C , 0x30,	0x281},		/*index: 212, 32.1726dB */
	{ 0x2864 , 0x30,	0x27E},		/*index: 213, 32.1255dB */
	{ 0x282C , 0x30,	0x27A},		/*index: 214, 32.0785dB */
	{ 0x27F5 , 0x30,	0x277},		/*index: 215, 32.0315dB */
	{ 0x27BD , 0x30,	0x273},		/*index: 216, 31.9844dB */
	{ 0x2786 , 0x30,	0x270},		/*index: 217, 31.9374dB */
	{ 0x2750 , 0x30,	0x26D},		/*index: 218, 31.8904dB */
	{ 0x2719 , 0x30,	0x269},		/*index: 219, 31.8433dB */
	{ 0x26E3 , 0x30,	0x266},		/*index: 220, 31.7963dB */
	{ 0x26AE , 0x30,	0x263},		/*index: 221, 31.7493dB */
	{ 0x2678 , 0x30,	0x25F},		/*index: 222, 31.7022dB */
	{ 0x2643 , 0x30,	0x25C},		/*index: 223, 31.6552dB */
	{ 0x260E , 0x30,	0x259},		/*index: 224, 31.6081dB */
	{ 0x25D9 , 0x30,	0x256},		/*index: 225, 31.5611dB */
	{ 0x25A5 , 0x30,	0x252},		/*index: 226, 31.5141dB */
	{ 0x2571 , 0x30,	0x24F},		/*index: 227, 31.467dB */
	{ 0x253D , 0x30,	0x24C},		/*index: 228, 31.42dB */
	{ 0x250A , 0x30,	0x249},		/*index: 229, 31.373dB */
	{ 0x24D7 , 0x30,	0x246},		/*index: 230, 31.3259dB */
	{ 0x24A4 , 0x30,	0x243},		/*index: 231, 31.2789dB */
	{ 0x2471 , 0x30,	0x23F},		/*index: 232, 31.2319dB */
	{ 0x243F , 0x30,	0x23C},		/*index: 233, 31.1848dB */
	{ 0x240C , 0x30,	0x239},		/*index: 234, 31.1378dB */
	{ 0x23DB , 0x30,	0x236},		/*index: 235, 31.0908dB */
	{ 0x23A9 , 0x30,	0x233},		/*index: 236, 31.0437dB */
	{ 0x2378 , 0x30,	0x230},		/*index: 237, 30.9967dB */
	{ 0x2347 , 0x30,	0x22D},		/*index: 238, 30.9496dB */
	{ 0x2316 , 0x30,	0x22A},		/*index: 239, 30.9026dB */
	{ 0x22E5 , 0x30,	0x227},		/*index: 240, 30.8556dB */
	{ 0x22B5 , 0x30,	0x224},		/*index: 241, 30.8085dB */
	{ 0x2285 , 0x30,	0x221},		/*index: 242, 30.7615dB */
	{ 0x2255 , 0x30,	0x21E},		/*index: 243, 30.7145dB */
	{ 0x2226 , 0x30,	0x21B},		/*index: 244, 30.6674dB */
	{ 0x21F7 , 0x30,	0x218},		/*index: 245, 30.6204dB */
	{ 0x21C8 , 0x30,	0x216},		/*index: 246, 30.5734dB */
	{ 0x2199 , 0x30,	0x213},		/*index: 247, 30.5263dB */
	{ 0x216B , 0x30,	0x210},		/*index: 248, 30.4793dB */
	{ 0x213C , 0x30,	0x20D},		/*index: 249, 30.4323dB */
	{ 0x210F , 0x30,	0x20A},		/*index: 250, 30.3852dB */
	{ 0x20E1 , 0x30,	0x207},		/*index: 251, 30.3382dB */
	{ 0x20B3 , 0x30,	0x205},		/*index: 252, 30.2911dB */
	{ 0x2086 , 0x30,	0x202},		/*index: 253, 30.2441dB */
	{ 0x2059 , 0x30,	0x1FF},		/*index: 254, 30.1971dB */
	{ 0x202C , 0x30,	0x1FC},//	/*index: 255, 30.0579dB */
	{ 0x2000 , 0x30,	0x1F9},		/*index: 256, 30.103dB */
	{ 0x1FD4 , 0x1C,	0x4EA},		/*index: 257, 30.056dB */
	{ 0x1FA8 , 0x1C,	0x4E3},		/*index: 258, 30.0089dB */
	{ 0x1F7C , 0x1C,	0x4DC},		/*index: 259, 29.9619dB */
	{ 0x1F50 , 0x1C,	0x4D5},		/*index: 260, 29.9149dB */
	{ 0x1F25 , 0x1C,	0x4CF},		/*index: 261, 29.8678dB */
	{ 0x1EFA , 0x1C,	0x4C8},		/*index: 262, 29.8208dB */
	{ 0x1ECF , 0x1C,	0x4C2},		/*index: 263, 29.7737dB */
	{ 0x1EA5 , 0x1C,	0x4BB},		/*index: 264, 29.7267dB */
	{ 0x1E7A , 0x1C,	0x4B4},		/*index: 265, 29.6797dB */
	{ 0x1E50 , 0x1C,	0x4AE},		/*index: 266, 29.6326dB */
	{ 0x1E26 , 0x1C,	0x4A8},		/*index: 267, 29.5856dB */
	{ 0x1DFD , 0x1C,	0x4A1},		/*index: 268, 29.5386dB */
	{ 0x1DD3 , 0x1C,	0x49B},		/*index: 269, 29.4915dB */
	{ 0x1DAA , 0x1C,	0x494},		/*index: 270, 29.4445dB */
	{ 0x1D81 , 0x1C,	0x48E},		/*index: 271, 29.3975dB */
	{ 0x1D58 , 0x1C,	0x488},		/*index: 272, 29.3504dB */
	{ 0x1D30 , 0x1C,	0x482},		/*index: 273, 29.3034dB */
	{ 0x1D07 , 0x1C,	0x47B},		/*index: 274, 29.2564dB */
	{ 0x1CDF , 0x1C,	0x475},		/*index: 275, 29.2093dB */
	{ 0x1CB7 , 0x1C,	0x46F},		/*index: 276, 29.1623dB */
	{ 0x1C8F , 0x1C,	0x469},		/*index: 277, 29.1152dB */
	{ 0x1C68 , 0x1C,	0x463},		/*index: 278, 29.0682dB */
	{ 0x1C41 , 0x1C,	0x45D},		/*index: 279, 29.0212dB */
	{ 0x1C1A , 0x1C,	0x457},		/*index: 280, 28.9741dB */
	{ 0x1BF3 , 0x1C,	0x451},		/*index: 281, 28.9271dB */
	{ 0x1BCC , 0x1C,	0x44B},		/*index: 282, 28.8801dB */
	{ 0x1BA6 , 0x1C,	0x445},		/*index: 283, 28.833dB */
	{ 0x1B7F , 0x1C,	0x43F},		/*index: 284, 28.786dB */
	{ 0x1B59 , 0x1C,	0x439},		/*index: 285, 28.739dB */
	{ 0x1B34 , 0x1C,	0x433},		/*index: 286, 28.6919dB */
	{ 0x1B0E , 0x1C,	0x42E},		/*index: 287, 28.6449dB */
	{ 0x1AE9 , 0x1C,	0x428},		/*index: 288, 28.5978dB */
	{ 0x1AC3 , 0x1C,	0x422},		/*index: 289, 28.5508dB */
	{ 0x1A9E , 0x1C,	0x41C},		/*index: 290, 28.5038dB */
	{ 0x1A7A , 0x1C,	0x417},		/*index: 291, 28.4567dB */
	{ 0x1A55 , 0x1C,	0x411},		/*index: 292, 28.4097dB */
	{ 0x1A31 , 0x1C,	0x40B},		/*index: 293, 28.3627dB */
	{ 0x1A0C , 0x1C,	0x406},		/*index: 294, 28.3156dB */
	{ 0x19E8 , 0x1C,	0x400},		/*index: 295, 28.2686dB */
	{ 0x19C5 , 0x1C,	0x3FB},		/*index: 296, 28.2216dB */
	{ 0x19A1 , 0x1C,	0x3F5},		/*index: 297, 28.1745dB */
	{ 0x197E , 0x1C,	0x3F0},		/*index: 298, 28.1275dB */
	{ 0x195A , 0x1C,	0x3EA},		/*index: 299, 28.0805dB */
	{ 0x1937 , 0x1C,	0x3E5},		/*index: 300, 28.0334dB */
	{ 0x1914 , 0x1C,	0x3E0},		/*index: 301, 27.9864dB */
	{ 0x18F2 , 0x1C,	0x3DA},		/*index: 302, 27.9393dB */
	{ 0x18CF , 0x1C,	0x3D5},		/*index: 303, 27.8923dB */
	{ 0x18AD , 0x1C,	0x3D0},		/*index: 304, 27.8453dB */
	{ 0x188B , 0x1C,	0x3CA},		/*index: 305, 27.7982dB */
	{ 0x1869 , 0x1C,	0x3C5},		/*index: 306, 27.7512dB */
	{ 0x1847 , 0x1C,	0x3C0},		/*index: 307, 27.7042dB */
	{ 0x1826 , 0x1C,	0x3BB},		/*index: 308, 27.6571dB */
	{ 0x1804 , 0x1C,	0x3B6},		/*index: 309, 27.6101dB */
	{ 0x17E3 , 0x1C,	0x3B1},		/*index: 310, 27.5631dB */
	{ 0x17C2 , 0x1C,	0x3AC},		/*index: 311, 27.516dB */
	{ 0x17A1 , 0x1C,	0x3A6},		/*index: 312, 27.469dB */
	{ 0x1780 , 0x1C,	0x3A1},		/*index: 313, 27.422dB */
	{ 0x1760 , 0x1C,	0x39C},		/*index: 314, 27.3749dB */
	{ 0x1740 , 0x1C,	0x397},		/*index: 315, 27.3279dB */
	{ 0x171F , 0x1C,	0x393},		/*index: 316, 27.2808dB */
	{ 0x16FF , 0x1C,	0x38E},		/*index: 317, 27.2338dB */
	{ 0x16E0 , 0x1C,	0x389},		/*index: 318, 27.1868dB */
	{ 0x16C0 , 0x1C,	0x384},		/*index: 319, 27.1397dB */
	{ 0x16A1 , 0x1C,	0x37F},		/*index: 320, 27.0927dB */
	{ 0x1681 , 0x1C,	0x37A},		/*index: 321, 27.0457dB */
	{ 0x1662 , 0x1C,	0x375},		/*index: 322, 26.9986dB */
	{ 0x1643 , 0x1C,	0x371},		/*index: 323, 26.9516dB */
	{ 0x1624 , 0x1C,	0x36C},		/*index: 324, 26.9046dB */
	{ 0x1606 , 0x1C,	0x367},		/*index: 325, 26.8575dB */
	{ 0x15E7 , 0x1C,	0x362},		/*index: 326, 26.8105dB */
	{ 0x15C9 , 0x1C,	0x35E},		/*index: 327, 26.7634dB */
	{ 0x15AB , 0x1C,	0x359},		/*index: 328, 26.7164dB */
	{ 0x158D , 0x1C,	0x355},		/*index: 329, 26.6694dB */
	{ 0x156F , 0x1C,	0x350},		/*index: 330, 26.6223dB */
	{ 0x1552 , 0x1C,	0x34B},		/*index: 331, 26.5753dB */
	{ 0x1534 , 0x1C,	0x347},		/*index: 332, 26.5283dB */
	{ 0x1517 , 0x1C,	0x342},		/*index: 333, 26.4812dB */
	{ 0x14FA , 0x1C,	0x33E},		/*index: 334, 26.4342dB */
	{ 0x14DD , 0x1C,	0x339},		/*index: 335, 26.3872dB */
	{ 0x14C0 , 0x1C,	0x335},		/*index: 336, 26.3401dB */
	{ 0x14A3 , 0x1C,	0x330},		/*index: 337, 26.2931dB */
	{ 0x1487 , 0x1C,	0x32C},		/*index: 338, 26.2461dB */
	{ 0x146A , 0x1C,	0x328},		/*index: 339, 26.199dB */
	{ 0x144E , 0x1C,	0x323},		/*index: 340, 26.152dB */
	{ 0x1432 , 0x1C,	0x31F},		/*index: 341, 26.1049dB */
	{ 0x1416 , 0x1C,	0x31B},		/*index: 342, 26.0579dB */
	{ 0x13FA , 0x1C,	0x316},		/*index: 343, 26.0109dB */
	{ 0x13DF , 0x1C,	0x312},		/*index: 344, 25.9638dB */
	{ 0x13C3 , 0x1C,	0x30E},		/*index: 345, 25.9168dB */
	{ 0x13A8 , 0x1C,	0x30A},		/*index: 346, 25.8698dB */
	{ 0x138D , 0x1C,	0x306},		/*index: 347, 25.8227dB */
	{ 0x1372 , 0x1C,	0x301},		/*index: 348, 25.7757dB */
	{ 0x1357 , 0x1C,	0x2FD},		/*index: 349, 25.7287dB */
	{ 0x133C , 0x1C,	0x2F9},		/*index: 350, 25.6816dB */
	{ 0x1321 , 0x1C,	0x2F5},		/*index: 351, 25.6346dB */
	{ 0x1307 , 0x1C,	0x2F1},		/*index: 352, 25.5875dB */
	{ 0x12ED , 0x1C,	0x2ED},		/*index: 353, 25.5405dB */
	{ 0x12D3 , 0x1C,	0x2E9},		/*index: 354, 25.4935dB */
	{ 0x12B8 , 0x1C,	0x2E5},		/*index: 355, 25.4464dB */
	{ 0x129F , 0x1C,	0x2E1},		/*index: 356, 25.3994dB */
	{ 0x1285 , 0x1C,	0x2DD},		/*index: 357, 25.3524dB */
	{ 0x126B , 0x1C,	0x2D9},		/*index: 358, 25.3053dB */
	{ 0x1252 , 0x1C,	0x2D5},		/*index: 359, 25.2583dB */
	{ 0x1238 , 0x1C,	0x2D1},		/*index: 360, 25.2113dB */
	{ 0x121F , 0x1C,	0x2CD},		/*index: 361, 25.1642dB */
	{ 0x1206 , 0x1C,	0x2C9},		/*index: 362, 25.1172dB */
	{ 0x11ED , 0x1C,	0x2C6},		/*index: 363, 25.0702dB */
	{ 0x11D5 , 0x1C,	0x2C2},		/*index: 364, 25.0231dB */
	{ 0x11BC , 0x1C,	0x2BE},		/*index: 365, 24.9761dB */
	{ 0x11A3 , 0x1C,	0x2BA},		/*index: 366, 24.929dB */
	{ 0x118B , 0x1C,	0x2B6},		/*index: 367, 24.882dB */
	{ 0x1173 , 0x1C,	0x2B3},		/*index: 368, 24.835dB */
	{ 0x115B , 0x1C,	0x2AF},		/*index: 369, 24.7879dB */
	{ 0x1143 , 0x1C,	0x2AB},		/*index: 370, 24.7409dB */
	{ 0x112B , 0x1C,	0x2A8},		/*index: 371, 24.6939dB */
	{ 0x1113 , 0x1C,	0x2A4},		/*index: 372, 24.6468dB */
	{ 0x10FB , 0x1C,	0x2A0},		/*index: 373, 24.5998dB */
	{ 0x10E4 , 0x1C,	0x29D},		/*index: 374, 24.5528dB */
	{ 0x10CD , 0x1C,	0x299},		/*index: 375, 24.5057dB */
	{ 0x10B5 , 0x1C,	0x295},		/*index: 376, 24.4587dB */
	{ 0x109E , 0x1C,	0x292},		/*index: 377, 24.4117dB */
	{ 0x1087 , 0x1C,	0x28E},		/*index: 378, 24.3646dB */
	{ 0x1070 , 0x1C,	0x28B},		/*index: 379, 24.3176dB */
	{ 0x105A , 0x1C,	0x287},		/*index: 380, 24.2705dB */
	{ 0x1043 , 0x1C,	0x284},		/*index: 381, 24.2235dB */
	{ 0x102D , 0x1C,	0x280},		/*index: 382, 24.1765dB */
	{ 0x1016 , 0x1C,	0x27D},	//	/*index: 383, 24.1324dB */
	{ 0x1000 , 0x1C,	0x279},		/*index: 384, 24.0824dB */
	{ 0xFEA ,  0x10,	0x3F0},		/*index: 385, 24.0354dB */
	{ 0xFD4 ,  0x10,	0x3EB},		/*index: 386, 23.9883dB */
	{ 0xFBE ,  0x10,	0x3E6},		/*index: 387, 23.9413dB */
	{ 0xFA8 ,  0x10,	0x3E0},		/*index: 388, 23.8943dB */
	{ 0xF93 ,  0x10,	0x3DB},		/*index: 389, 23.8472dB */
	{ 0xF7D ,  0x10,	0x3D6},		/*index: 390, 23.8002dB */
	{ 0xF68 ,  0x10,	0x3D0},		/*index: 391, 23.7531dB */
	{ 0xF52 ,  0x10,	0x3CB},		/*index: 392, 23.7061dB */
	{ 0xF3D ,  0x10,	0x3C6},		/*index: 393, 23.6591dB */
	{ 0xF28 ,  0x10,	0x3C1},		/*index: 394, 23.612dB */
	{ 0xF13 ,  0x10,	0x3BB},		/*index: 395, 23.565dB */
	{ 0xEFE ,  0x10,	0x3B6},		/*index: 396, 23.518dB */
	{ 0xEEA ,  0x10,	0x3B1},		/*index: 397, 23.4709dB */
	{ 0xED5 ,  0x10,	0x3AC},		/*index: 398, 23.4239dB */
	{ 0xEC0 ,  0x10,	0x3A7},		/*index: 399, 23.3769dB */
	{ 0xEAC ,  0x10,	0x3A2},		/*index: 400, 23.3298dB */
	{ 0xE98 ,  0x10,	0x39D},		/*index: 401, 23.2828dB */
	{ 0xE84 ,  0x10,	0x398},		/*index: 402, 23.2358dB */
	{ 0xE70 ,  0x10,	0x393},		/*index: 403, 23.1887dB */
	{ 0xE5C ,  0x10,	0x38E},		/*index: 404, 23.1417dB */
	{ 0xE48 ,  0x10,	0x389},		/*index: 405, 23.0946dB */
	{ 0xE34 ,  0x10,	0x384},		/*index: 406, 23.0476dB */
	{ 0xE20 ,  0x10,	0x37F} ,		/*index: 407, 23.0006dB */
	{ 0xE0D ,  0x10,	0x37B},		/*index: 408, 22.9535dB */
	{ 0xDF9 ,  0x10,	0x376},		/*index: 409, 22.9065dB */
	{ 0xDE6 ,  0x10,	0x371},		/*index: 410, 22.8595dB */
	{ 0xDD3 ,  0x10,	0x36C},		/*index: 411, 22.8124dB */
	{ 0xDC0 ,  0x10,	0x368},		/*index: 412, 22.7654dB */
	{ 0xDAD ,  0x10,	0x363},		/*index: 413, 22.7184dB */
	{ 0xD9A ,  0x10,	0x35E},		/*index: 414, 22.6713dB */
	{ 0xD87 ,  0x10,	0x35A},		/*index: 415, 22.6243dB */
	{ 0xD74 ,  0x10,	0x355},		/*index: 416, 22.5772dB */
	{ 0xD62 ,  0x10,	0x350},		/*index: 417, 22.5302dB */
	{ 0xD4F ,  0x10,	0x34C},		/*index: 418, 22.4832dB */
	{ 0xD3D ,  0x10,	0x347},		/*index: 419, 22.4361dB */
	{ 0xD2B ,  0x10,	0x343},		/*index: 420, 22.3891dB */
	{ 0xD18 ,  0x10,	0x33E},		/*index: 421, 22.3421dB */
	{ 0xD06 ,  0x10,	0x33A},		/*index: 422, 22.295dB */
	{ 0xCF4 ,  0x10,	0x335},		/*index: 423, 22.248dB */
	{ 0xCE2 ,  0x10,	0x331},		/*index: 424, 22.201dB */
	{ 0xCD0 ,  0x10,	0x32C},		/*index: 425, 22.1539dB */
	{ 0xCBF ,  0x10,	0x328},		/*index: 426, 22.1069dB */
	{ 0xCAD ,  0x10,	0x324},		/*index: 427, 22.0599dB */
	{ 0xC9C ,  0x10,	0x31F},		/*index: 428, 22.0128dB */
	{ 0xC8A ,  0x10,	0x31B},		/*index: 429, 21.9658dB */
	{ 0xC79 ,  0x10,	0x317},		/*index: 430, 21.9187dB */
	{ 0xC68 ,  0x10,	0x313},		/*index: 431, 21.8717dB */
	{ 0xC56 ,  0x10,	0x30E},		/*index: 432, 21.8247dB */
	{ 0xC45 ,  0x10,	0x30A},		/*index: 433, 21.7776dB */
	{ 0xC34 ,  0x10,	0x306},		/*index: 434, 21.7306dB */
	{ 0xC24 ,  0x10,	0x302},		/*index: 435, 21.6836dB */
	{ 0xC13 ,  0x10,	0x2FE},		/*index: 436, 21.6365dB */
	{ 0xC02 ,  0x10,	0x2FA},		/*index: 437, 21.5895dB */
	{ 0xBF1 ,  0x10,	0x2F5},		/*index: 438, 21.5425dB */
	{ 0xBE1 ,  0x10,	0x2F1},		/*index: 439, 21.4954dB */
	{ 0xBD1 ,  0x10,	0x2ED},		/*index: 440, 21.4484dB */
	{ 0xBC0 ,  0x10,	0x2E9},		/*index: 441, 21.4014dB */
	{ 0xBB0 ,  0x10,	0x2E5},		/*index: 442, 21.3543dB */
	{ 0xBA0 ,  0x10,	0x2E1},		/*index: 443, 21.3073dB */
	{ 0xB90 ,  0x10,	0x2DD},		/*index: 444, 21.2602dB */
	{ 0xB80 ,  0x10,	0x2D9},		/*index: 445, 21.2132dB */
	{ 0xB70 ,  0x10,	0x2D5},		/*index: 446, 21.1662dB */
	{ 0xB60 ,  0x10,	0x2D1},		/*index: 447, 21.1191dB */
	{ 0xB50 ,  0x10,	0x2CE}, ////	/*index: 448, 20.5547dB */
	{ 0xB41 ,  0x10,	0x2CA},		/*index: 449, 21.0251dB */
	{ 0xB31 ,  0x10,	0x2C6},		/*index: 450, 20.978dB */
	{ 0xB22 ,  0x10,	0x2C2},		/*index: 451, 20.931dB */
	{ 0xB12 ,  0x10,	0x2BE},		/*index: 452, 20.884dB */
	{ 0xB03 ,  0x10,	0x2BA},		/*index: 453, 20.8369dB */
	{ 0xAF4 ,  0x10,	0x2B7},		/*index: 454, 20.7899dB */
	{ 0xAE5 ,  0x10,	0x2B3},		/*index: 455, 20.7428dB */
	{ 0xAD6 ,  0x10,	0x2AF},		/*index: 456, 20.6958dB */
	{ 0xAC7 ,  0x10,	0x2AC},		/*index: 457, 20.6488dB */
	{ 0xAB8 ,  0x10,	0x2A8},		/*index: 458, 20.6017dB */
	{ 0xAA9 ,  0x10,	0x2A4},		/*index: 459, 20.5547dB */
	{ 0xA9A ,  0x10,	0x2A1},		/*index: 460, 20.5077dB */
	{ 0xA8B ,  0x10,	0x29D},		/*index: 461, 20.4606dB */
	{ 0xA7D ,  0x10,	0x299},		/*index: 462, 20.4136dB */
	{ 0xA6E ,  0x10,	0x296},		/*index: 463, 20.3666dB */
	{ 0xA60 ,  0x10,	0x292},		/*index: 464, 20.3195dB */
	{ 0xA52 ,  0x10,	0x28F},		/*index: 465, 20.2725dB */
	{ 0xA43 ,  0x10,	0x28B},		/*index: 466, 20.2255dB */
	{ 0xA35 ,  0x10,	0x288},		/*index: 467, 20.1784dB */
	{ 0xA27 ,  0x10,	0x284},		/*index: 468, 20.1314dB */
	{ 0xA19 ,  0x10,	0x281},		/*index: 469, 20.0843dB */
	{ 0xA0B ,  0x10,	0x27D},		/*index: 470, 20.0373dB */
	{ 0x9FD ,  0x10,	0x27A},		/*index: 471, 19.9903dB */
	{ 0x9EF ,  0x10,	0x276},		/*index: 472, 19.9432dB */
	{ 0x9E2 ,  0x10,	0x273},		/*index: 473, 19.8962dB */
	{ 0x9D4 ,  0x10,	0x270},		/*index: 474, 19.8492dB */
	{ 0x9C6 ,  0x10,	0x26C},		/*index: 475, 19.8021dB */
	{ 0x9B9 ,  0x10,	0x269},		/*index: 476, 19.7551dB */
	{ 0x9AB ,  0x10,	0x266},		/*index: 477, 19.7081dB */
	{ 0x99E ,  0x10,	0x262},		/*index: 478, 19.661dB */
	{ 0x991 ,  0x10,	0x25F},		/*index: 479, 19.614dB */
	{ 0x983 ,  0x10,	0x25C},		/*index: 480, 19.5669dB */
	{ 0x976 ,  0x10,	0x258},		/*index: 481, 19.5199dB */
	{ 0x969 ,  0x10,	0x255},		/*index: 482, 19.4729dB */
	{ 0x95C ,  0x10,	0x252},		/*index: 483, 19.4258dB */
	{ 0x94F ,  0x10,	0x24F},		/*index: 484, 19.3788dB */
	{ 0x942 ,  0x10,	0x24C},		/*index: 485, 19.3318dB */
	{ 0x936 ,  0x10,	0x248},		/*index: 486, 19.2847dB */
	{ 0x929 ,  0x10,	0x245},		/*index: 487, 19.2377dB */
	{ 0x91C ,  0x10,	0x242},		/*index: 488, 19.1907dB */
	{ 0x910 ,  0x10,	0x23F},		/*index: 489, 19.1436dB */
	{ 0x903 ,  0x10,	0x23C},		/*index: 490, 19.0966dB */
	{ 0x8F7 ,  0x10,	0x239},		/*index: 491, 19.0496dB */
	{ 0x8EA ,  0x10,	0x236},		/*index: 492, 19.0025dB */
	{ 0x8DE ,  0x10,	0x233},		/*index: 493, 18.9555dB */
	{ 0x8D2 ,  0x10,	0x230},		/*index: 494, 18.9084dB */
	{ 0x8C5 ,  0x10,	0x22D},		/*index: 495, 18.8614dB */
	{ 0x8B9 ,  0x10,	0x22A},		/*index: 496, 18.8144dB */
	{ 0x8AD ,  0x10,	0x227},		/*index: 497, 18.7673dB */
	{ 0x8A1 ,  0x10,	0x224},		/*index: 498, 18.7203dB */
	{ 0x895 ,  0x10,	0x221},		/*index: 499, 18.6733dB */
	{ 0x88A ,  0x10,	0x21E},		/*index: 500, 18.6262dB */
	{ 0x87E ,  0x10,	0x21B},		/*index: 501, 18.5792dB */
	{ 0x872 ,  0x10,	0x218},		/*index: 502, 18.5322dB */
	{ 0x866 ,  0x10,	0x215},		/*index: 503, 18.4851dB */
	{ 0x85B ,  0x10,	0x212},		/*index: 504, 18.4381dB */
	{ 0x84F ,  0x10,	0x20F},		/*index: 505, 18.3911dB */
	{ 0x844 ,  0x10,	0x20D},		/*index: 506, 18.344dB */
	{ 0x838 ,  0x10,	0x20A},		/*index: 507, 18.297dB */
	{ 0x82D ,  0x10,	0x207},		/*index: 508, 18.2499dB */
	{ 0x822 ,  0x10,	0x204},		/*index: 509, 18.2029dB */
	{ 0x816 ,  0x10,	0x201},		/*index: 510, 18.1559dB */
	{ 0x80B ,  0x10,	0x1FF},		/*index: 511, 18.1088dB */
	{ 0x800 ,  0x10,	0x1FC},	//	/*index: 512, 18.0618dB */
	{ 0x7F5 ,  0x10,	0x1F9},		/*index: 513, 18.0148dB */
	{ 0x7EA ,  0x10,	0x1F6},		/*index: 514, 17.9677dB */
	{ 0x7DF ,  0x10,	0x1F4},		/*index: 515, 17.9207dB */
	{ 0x7D4 ,  0x10,	0x1F1},		/*index: 516, 17.8737dB */
	{ 0x7C9 ,  0x10,	0x1EE},		/*index: 517, 17.8266dB */
	{ 0x7BF ,  0x10,	0x1EC},		/*index: 518, 17.7796dB */
	{ 0x7B4 ,  0x10,	0x1E9},		/*index: 519, 17.7325dB */
	{ 0x7A9 ,  0x10,	0x1E6},		/*index: 520, 17.6855dB */
	{ 0x79F ,  0x10,	0x1E4},		/*index: 521, 17.6385dB */
	{ 0x794 ,  0x10,	0x1E1},		/*index: 522, 17.5914dB */
	{ 0x78A ,  0x10,	0x1DF},		/*index: 523, 17.5444dB */
	{ 0x77F ,  0x10,	0x1DC},		/*index: 524, 17.4974dB */
	{ 0x775 ,  0x10,	0x1D9},		/*index: 525, 17.4503dB */
	{ 0x76A ,  0x10,	0x1D7},		/*index: 526, 17.4033dB */
	{ 0x760 ,  0x10,	0x1D4},		/*index: 527, 17.3563dB */
	{ 0x756 ,  0x10,	0x1D2},		/*index: 528, 17.3092dB */
	{ 0x74C ,  0x10,	0x1CF},		/*index: 529, 17.2622dB */
	{ 0x742 ,  0x10,	0x1CD},		/*index: 530, 17.2152dB */
	{ 0x738 ,  0x10,	0x1CA},		/*index: 531, 17.1681dB */
	{ 0x72E ,  0x10,	0x1C8},		/*index: 532, 17.1211dB */
	{ 0x724 ,  0x10,	0x1C5},		/*index: 533, 17.074dB */
	{ 0x71A ,  0x10,	0x1C3},		/*index: 534, 17.027dB */
	{ 0x710 ,  0x10,	0x1C1},		/*index: 535, 16.98dB */
	{ 0x706 ,  0x10,	0x1BE},		/*index: 536, 16.9329dB */
	{ 0x6FD ,  0x10,	0x1BC},		/*index: 537, 16.8859dB */
	{ 0x6F3 ,  0x10,	0x1B9},		/*index: 538, 16.8389dB */
	{ 0x6E9 ,  0x10,	0x1B7},		/*index: 539, 16.7918dB */
	{ 0x6E0 ,  0x10,	0x1B5},		/*index: 540, 16.7448dB */
	{ 0x6D6 ,  0x10,	0x1B2},		/*index: 541, 16.6978dB */
	{ 0x6CD ,  0x10,	0x1B0},		/*index: 542, 16.6507dB */
	{ 0x6C4 ,  0x10,	0x1AE},		/*index: 543, 16.6037dB */
	{ 0x6BA ,  0x10,	0x1AB},		/*index: 544, 16.5566dB */
	{ 0x6B1 ,  0x10,	0x1A9},	//	/*index: 545, 16.1236dB */
	{ 0x6A8 ,  0x10,	0x1A7},		/*index: 546, 16.4626dB */
	{ 0x69E ,  0x10,	0x1A4},		/*index: 547, 16.4155dB */
	{ 0x695 ,  0x10,	0x1A2},		/*index: 548, 16.3685dB */
	{ 0x68C ,  0x10,	0x1A0},		/*index: 549, 16.3215dB */
	{ 0x683 ,  0x10,	0x19E},		/*index: 550, 16.2744dB */
	{ 0x67A ,  0x10,	0x19B},		/*index: 551, 16.2274dB */
	{ 0x671 ,  0x10,	0x199},		/*index: 552, 16.1804dB */
	{ 0x668 ,  0x10,	0x197},		/*index: 553, 16.1333dB */
	{ 0x65F ,  0x10,	0x195},		/*index: 554, 16.0863dB */
	{ 0x657 ,  0x10,	0x193},		/*index: 555, 16.0393dB */
	{ 0x64E ,  0x10,	0x190},		/*index: 556, 15.9922dB */
	{ 0x645 ,  0x10,	0x18E},		/*index: 557, 15.9452dB */
	{ 0x63C ,  0x10,	0x18C},		/*index: 558, 15.8981dB */
	{ 0x634 ,  0x10,	0x18A},		/*index: 559, 15.8511dB */
	{ 0x62B ,  0x10,	0x188},		/*index: 560, 15.8041dB */
	{ 0x623 ,  0x10,	0x186},		/*index: 561, 15.757dB */
	{ 0x61A ,  0x10,	0x184},		/*index: 562, 15.71dB */
	{ 0x612 ,  0x10,	0x182},		/*index: 563, 15.663dB */
	{ 0x609 ,  0x10,	0x17F},		/*index: 564, 15.6159dB */
	{ 0x601 ,  0x10,	0x17D},		/*index: 565, 15.5689dB */
	{ 0x5F9 ,  0x10,	0x17B},		/*index: 566, 15.5219dB */
	{ 0x5F0 ,  0x10,	0x179},		/*index: 567, 15.4748dB */
	{ 0x5E8 ,  0x10,	0x177},		/*index: 568, 15.4278dB */
	{ 0x5E0 ,  0x10,	0x175},//		/*index: 569, 14.5408dB */
	{ 0x5D8 ,  0x10,	0x173},		/*index: 570, 15.3337dB */
	{ 0x5D0 ,  0x10,	0x171},		/*index: 571, 15.2867dB */
	{ 0x5C8 ,  0x10,	0x16F},		/*index: 572, 15.2396dB */
	{ 0x5C0 ,  0x10,	0x16D},		/*index: 573, 15.1926dB */
	{ 0x5B8 ,  0x10,	0x16B},		/*index: 574, 15.1456dB */
	{ 0x5B0 ,  0x10,	0x169},		/*index: 575, 15.0985dB */
	{ 0x5A8 ,  0x10,	0x167},		/*index: 576, 15.0515dB */
	{ 0x5A0 ,  0x10,	0x165},		/*index: 577, 15.0045dB */
	{ 0x599 ,  0x10,	0x164},		/*index: 578, 14.9574dB */
	{ 0x591 ,  0x10,	0x162},		/*index: 579, 14.9104dB */
	{ 0x589 ,  0x10,	0x160},		/*index: 580, 14.8634dB */
	{ 0x581 ,  0x10,	0x15E},		/*index: 581, 14.8163dB */
	{ 0x57A ,  0x10,	0x15C},		/*index: 582, 14.7693dB */
	{ 0x572 ,  0x10,	0x15A},		/*index: 583, 14.7222dB */
	{ 0x56B ,  0x10,	0x158},		/*index: 584, 14.6752dB */
	{ 0x563 ,  0x10,	0x156},		/*index: 585, 14.6282dB */
	{ 0x55C ,  0x10,	0x154},		/*index: 586, 14.5811dB */
	{ 0x554 ,  0x10,	0x153},		/*index: 587, 14.5341dB */
	{ 0x54D ,  0x10,	0x151},		/*index: 588, 14.4871dB */
	{ 0x546 ,  0x10,	0x14F},		/*index: 589, 14.44dB */
	{ 0x53E ,  0x10,	0x14D},		/*index: 590, 14.393dB */
	{ 0x537 ,  0x10,	0x14B},		/*index: 591, 14.346dB */
	{ 0x530 ,  0x10,	0x14A},		/*index: 592, 14.2989dB */
	{ 0x529 ,  0x10,	0x148},		/*index: 593, 14.2519dB */
	{ 0x522 ,  0x10,	0x146},		/*index: 594, 14.2049dB */
	{ 0x51B ,  0x10,	0x144},		/*index: 595, 14.1578dB */
	{ 0x514 ,  0x10,	0x143},		/*index: 596, 14.1108dB */
	{ 0x50C ,  0x10,	0x141},		/*index: 597, 14.0637dB */
	{ 0x506 ,  0x10,	0x13F},		/*index: 598, 14.0167dB */
	{ 0x4FF ,  0x10,	0x13D},		/*index: 599, 13.9697dB */
	{ 0x4F8 ,  0x10,	0x13C},		/*index: 600, 13.9226dB */
	{ 0x4F1 ,  0x10,	0x13A},		/*index: 601, 13.8756dB */
	{ 0x4EA ,  0x10,	0x138},		/*index: 602, 13.8286dB */
	{ 0x4E3 ,  0x10,	0x137},		/*index: 603, 13.7815dB */
	{ 0x4DC ,  0x10,	0x135},//		/*index: 604, 13.2045dB */
	{ 0x4D6 ,  0x10,	0x133},		/*index: 605, 13.6875dB */
	{ 0x4CF ,  0x10,	0x132},		/*index: 606, 13.6404dB */
	{ 0x4C8 ,  0x10,	0x130},		/*index: 607, 13.5934dB */
	{ 0x4C2 ,  0x10,	0x12E},		/*index: 608, 13.5463dB */
	{ 0x4BB ,  0x10,	0x12D},		/*index: 609, 13.4993dB */
	{ 0x4B5 ,  0x10,	0x12B},		/*index: 610, 13.4523dB */
	{ 0x4AE ,  0x10,	0x129},		/*index: 611, 13.4052dB */
	{ 0x4A8 ,  0x10,	0x128},		/*index: 612, 13.3582dB */
	{ 0x4A1 ,  0x10,	0x126},		/*index: 613, 13.3112dB */
	{ 0x49B ,  0x10,	0x125},		/*index: 614, 13.2641dB */
	{ 0x494 ,  0x10,	0x123},		/*index: 615, 13.2171dB */
	{ 0x48E ,  0x10,	0x122},		/*index: 616, 13.1701dB */
	{ 0x488 ,  0x10,	0x120},		/*index: 617, 13.123dB */
	{ 0x482 ,  0x10,	0x11E},		/*index: 618, 13.076dB */
	{ 0x47B ,  0x10,	0x11D},		/*index: 619, 13.029dB */
	{ 0x475 ,  0x10,	0x11B},		/*index: 620, 12.9819dB */
	{ 0x46F ,  0x10,	0x11A},		/*index: 621, 12.9349dB */
	{ 0x469 ,  0x10,	0x118},		/*index: 622, 12.8878dB */
	{ 0x463 ,  0x10,	0x117},		/*index: 623, 12.8408dB */
	{ 0x45D ,  0x10,	0x115},		/*index: 624, 12.7938dB */
	{ 0x457 ,  0x10,	0x114},		/*index: 625, 12.7467dB */
	{ 0x451 ,  0x10,	0x112},		/*index: 626, 12.6997dB */
	{ 0x44B ,  0x10,	0x111},		/*index: 627, 12.6527dB */
	{ 0x445 ,  0x10,	0x10F},		/*index: 628, 12.6056dB */
	{ 0x43F ,  0x10,	0x10E},		/*index: 629, 12.5586dB */
	{ 0x439 ,  0x10,	0x10C},		/*index: 630, 12.5116dB */
	{ 0x433 ,  0x10,	0x10B},		/*index: 631, 12.4645dB */
	{ 0x42D ,  0x10,	0x10A},		/*index: 632, 12.4175dB */
	{ 0x428 ,  0x10,	0x108},		/*index: 633, 12.3705dB */
	{ 0x422 ,  0x10,	0x107},		/*index: 634, 12.3234dB */
	{ 0x41C ,  0x10,	0x105},		/*index: 635, 12.2764dB */
	{ 0x416 ,  0x10,	0x104},		/*index: 636, 12.2293dB */
	{ 0x411 ,  0x10,	0x102},		/*index: 637, 12.1823dB */
	{ 0x40B ,  0x10,	0x101},		/*index: 638, 12.1353dB */
	{ 0x406 ,  0x10,	0x100},		/*index: 639, 12.0882dB */
	{ 0x400 ,  0x10,	0xFE},		/*index: 640, 12.0412dB */
	{ 0x3FA ,  0xC,	0x13C},		/*index: 641, 11.9942dB */
	{ 0x3F5 ,  0xC,	0x13B},		/*index: 642, 11.9471dB */
	{ 0x3EF ,  0xC,	0x139},		/*index: 643, 11.9001dB */
	{ 0x3EA ,  0xC,	0x137},		/*index: 644, 11.8531dB */
	{ 0x3E5 ,  0xC,	0x136},		/*index: 645, 11.806dB */
	{ 0x3DF ,  0xC,	0x134},		/*index: 646, 11.759dB */
	{ 0x3DA ,  0xC,	0x132},		/*index: 647, 11.7119dB */
	{ 0x3D5 ,  0xC,	0x131},		/*index: 648, 11.6649dB */
	{ 0x3CF ,  0xC,	0x12F},		/*index: 649, 11.6179dB */
	{ 0x3CA ,  0xC,	0x12D},		/*index: 650, 11.5708dB */
	{ 0x3C5 ,  0xC,	0x12C},		/*index: 651, 11.5238dB */
	{ 0x3C0 ,  0xC,	0x12A},		/*index: 652, 11.4768dB */
	{ 0x3BA ,  0xC,	0x128},		/*index: 653, 11.4297dB */
	{ 0x3B5 ,  0xC,	0x127},		/*index: 654, 11.3827dB */
	{ 0x3B0 ,  0xC,	0x125},		/*index: 655, 11.3357dB */
	{ 0x3AB ,  0xC,	0x124},		/*index: 656, 11.2886dB */
	{ 0x3A6 ,  0xC,	0x122},		/*index: 657, 11.2416dB */
	{ 0x3A1 ,  0xC,	0x121},		/*index: 658, 11.1946dB */
	{ 0x39C ,  0xC,	0x11F},		/*index: 659, 11.1475dB */
	{ 0x397 ,  0xC,	0x11D},		/*index: 660, 11.1005dB */
	{ 0x392 ,  0xC,	0x11C},		/*index: 661, 11.0534dB */
	{ 0x38D ,  0xC,	0x11A},		/*index: 662, 11.0064dB */
	{ 0x388 ,  0xC,	0x119},		/*index: 663, 10.9594dB */
	{ 0x383 ,  0xC,	0x117},		/*index: 664, 10.9123dB */
	{ 0x37E ,  0xC,	0x116},		/*index: 665, 10.8653dB */
	{ 0x37A ,  0xC,	0x114},		/*index: 666, 10.8183dB */
	{ 0x375 ,  0xC,	0x113},		/*index: 667, 10.7712dB */
	{ 0x370 ,  0xC,	0x111},		/*index: 668, 10.7242dB */
	{ 0x36B ,  0xC,	0x110},		/*index: 669, 10.6772dB */
	{ 0x366 ,  0xC,	0x10E},		/*index: 670, 10.6301dB */
	{ 0x362 ,  0xC,	0x10D},		/*index: 671, 10.5831dB */
	{ 0x35D ,  0xC,	0x10B},		/*index: 672, 10.536dB */
	{ 0x358 ,  0xC,	0x10A},		/*index: 673, 10.489dB */
	{ 0x354 ,  0xC,	0x109},		/*index: 674, 10.442dB */
	{ 0x34F ,  0xC,	0x107},		/*index: 675, 10.3949dB */
	{ 0x34B ,  0xC,	0x106},		/*index: 676, 10.3479dB */
	{ 0x346 ,  0xC,	0x104},		/*index: 677, 10.3009dB */
	{ 0x342 ,  0xC,	0x103},		/*index: 678, 10.2538dB */
	{ 0x33D ,  0xC,	0x102},		/*index: 679, 10.2068dB */
	{ 0x339 ,  0xC,	0x100},		/*index: 680, 10.1598dB */
	{ 0x334 ,  0xC,	0xFF},	//	/*index: 681, 10.1037dB */
	{ 0x330 ,  0xC,	0xFD},		/*index: 682, 10.0657dB */
	{ 0x32B ,  0xC,	0xFC},		/*index: 683, 10.0187dB */
	{ 0x327 ,  0xC,	0xFB},		/*index: 684, 9.9716dB */
	{ 0x323 ,  0xC,	0xF9},		/*index: 685, 9.9246dB */
	{ 0x31E ,  0xC,	0xF8},		/*index: 686, 9.8775dB */
	{ 0x31A ,  0xC,	0xF7},		/*index: 687, 9.8305dB */
	{ 0x316 ,  0xC,	0xF5},		/*index: 688, 9.7835dB */
	{ 0x311 ,  0xC,	0xF4},		/*index: 689, 9.7364dB */
	{ 0x30D ,  0xC,	0xF3},		/*index: 690, 9.6894dB */
	{ 0x309 ,  0xC,	0xF1},		/*index: 691, 9.6424dB */
	{ 0x305 ,  0xC,	0xF0},		/*index: 692, 9.5953dB */
	{ 0x301 ,  0xC,	0xEF},		/*index: 693, 9.5483dB */
	{ 0x2FC ,  0xC,	0xED},		/*index: 694, 9.5013dB */
	{ 0x2F8 ,  0xC,	0xEC},		/*index: 695, 9.4542dB */
	{ 0x2F4 ,  0xC,	0xEB},		/*index: 696, 9.4072dB */
	{ 0x2F0 ,  0xC,	0xEA},		/*index: 697, 9.3602dB */
	{ 0x2EC ,  0xC,	0xE8},		/*index: 698, 9.3131dB */
	{ 0x2E8 ,  0xC,	0xE7},		/*index: 699, 9.2661dB */
	{ 0x2E4 ,  0xC,	0xE6},		/*index: 700, 9.219dB */
	{ 0x2E0 ,  0xC,	0xE5},		/*index: 701, 9.172dB */
	{ 0x2DC ,  0xC,	0xE3},		/*index: 702, 9.125dB */
	{ 0x2D8 ,  0xC,	0xE2},		/*index: 703, 9.0779dB */
	{ 0x2D4 ,  0xC,	0xE1},		/*index: 704, 9.0309dB */
	{ 0x2D0 ,  0x7,	0x118},		/*index: 705, 8.9839dB */
	{ 0x2CC ,  0x7,	0x116},		/*index: 706, 8.9368dB */
	{ 0x2C8 ,  0x7,	0x115},		/*index: 707, 8.8898dB */
	{ 0x2C5 ,  0x7,	0x113},		/*index: 708, 8.8428dB */
	{ 0x2C1 ,  0x7,	0x112},		/*index: 709, 8.7957dB */
	{ 0x2BD ,  0x7,	0x110},		/*index: 710, 8.7487dB */
	{ 0x2B9 ,  0x7,	0x10F},		/*index: 711, 8.7016dB */
	{ 0x2B5 ,  0x7,	0x10D},		/*index: 712, 8.6546dB */
	{ 0x2B2 ,  0x7,	0x10C},		/*index: 713, 8.6076dB */
	{ 0x2AE ,  0x7,	0x10B},		/*index: 714, 8.5605dB */
	{ 0x2AA ,  0x7,	0x109},		/*index: 715, 8.5135dB */
	{ 0x2A7 ,  0x7,	0x108},		/*index: 716, 8.4665dB */
	{ 0x2A3 ,  0x7,	0x106},		/*index: 717, 8.4194dB */
	{ 0x29F ,  0x7,	0x105},		/*index: 718, 8.3724dB */
	{ 0x29C ,  0x7,	0x103},		/*index: 719, 8.3254dB */
	{ 0x298 ,  0x7,	0x102},		/*index: 720, 8.2783dB */
	{ 0x294 ,  0x7,	0x101},		/*index: 721, 8.2313dB */
	{ 0x291 ,  0x7,	0xFF},		/*index: 722, 8.1843dB */
	{ 0x28D ,  0x7,	0xFE},		/*index: 723, 8.1372dB */
	{ 0x28A ,  0x7,	0xFD},		/*index: 724, 8.0902dB */
	{ 0x286 ,  0x7,	0xFB},		/*index: 725, 8.0431dB */
	{ 0x283 ,  0x7,	0xFA},		/*index: 726, 7.9961dB */
	{ 0x27F ,  0x7,	0xF8},		/*index: 727, 7.9491dB */
	{ 0x27C ,  0x7,	0xF7},		/*index: 728, 7.902dB */
	{ 0x278 ,  0x7,	0xF6},		/*index: 729, 7.855dB */
	{ 0x275 ,  0x7,	0xF4},	//	/*index: 730, 7.8241dB */
	{ 0x272 ,  0x7,	0xF3},		/*index: 731, 7.7609dB */
	{ 0x26E ,  0x7,	0xF2},		/*index: 732, 7.7139dB */
	{ 0x26B ,  0x7,	0xF1},		/*index: 733, 7.6669dB */
	{ 0x268 ,  0x7,	0xEF},		/*index: 734, 7.6198dB */
	{ 0x264 ,  0x7,	0xEE},		/*index: 735, 7.5728dB */
	{ 0x261 ,  0x7,	0xED},		/*index: 736, 7.5257dB */
	{ 0x25E ,  0x7,	0xEB},		/*index: 737, 7.4787dB */
	{ 0x25A ,  0x7,	0xEA},		/*index: 738, 7.4317dB */
	{ 0x257 ,  0x7,	0xE9},		/*index: 739, 7.3846dB */
	{ 0x254 ,  0x7,	0xE8},		/*index: 740, 7.3376dB */
	{ 0x251 ,  0x7,	0xE6},		/*index: 741, 7.2906dB */
	{ 0x24D ,  0x7,	0xE5},		/*index: 742, 7.2435dB */
	{ 0x24A ,  0x7,	0xE4},		/*index: 743, 7.1965dB */
	{ 0x247 ,  0x7,	0xE3},		/*index: 744, 7.1495dB */
	{ 0x244 ,  0x7,	0xE1},		/*index: 745, 7.1024dB */
	{ 0x241 ,  0x7,	0xE0},		/*index: 746, 7.0554dB */
	{ 0x23E ,  0x7,	0xDF},		/*index: 747, 7.0084dB */
	{ 0x23B ,  0x7,	0xDE},		/*index: 748, 6.9613dB */
	{ 0x237 ,  0x7,	0xDD},		/*index: 749, 6.9143dB */
	{ 0x234 ,  0x7,	0xDB},		/*index: 750, 6.8672dB */
	{ 0x231 ,  0x7,	0xDA},		/*index: 751, 6.8202dB */
	{ 0x22E ,  0x7,	0xD9},		/*index: 752, 6.7732dB */
	{ 0x22B ,  0x7,	0xD8},		/*index: 753, 6.7261dB */
	{ 0x228 ,  0x7,	0xD7},		/*index: 754, 6.6791dB */
	{ 0x225 ,  0x7,	0xD6},		/*index: 755, 6.6321dB */
	{ 0x222 ,  0x7,	0xD4},		/*index: 756, 6.585dB */
	{ 0x21F ,  0x7,	0xD3},		/*index: 757, 6.538dB */
	{ 0x21C ,  0x7,	0xD2},		/*index: 758, 6.491dB */
	{ 0x21A ,  0x7,	0xD1},		/*index: 759, 6.4439dB */
	{ 0x217 ,  0x7,	0xD0},		/*index: 760, 6.3969dB */
	{ 0x214 ,  0x7,	0xCF},		/*index: 761, 6.3499dB */
	{ 0x211 ,  0x7,	0xCE},		/*index: 762, 6.3028dB */
	{ 0x20E ,  0x7,	0xCC},		/*index: 763, 6.2558dB */
	{ 0x20B ,  0x7,	0xCB},		/*index: 764, 6.2087dB */
	{ 0x208 ,  0x7,	0xCA},		/*index: 765, 6.1617dB */
	{ 0x206 ,  0x7,	0xC9},		/*index: 766, 6.1147dB */
	{ 0x203 ,  0x7,	0xC8},		/*index: 767, 6.0676dB */
	{ 0x200 ,  0x7,	0xC7},		/*index: 768, 6.0206dB */
	{ 0x1FD ,  0x0,	0xFE},		/*index: 769, 5.9736dB */
	{ 0x1FA ,  0x0,	0xFC},		/*index: 770, 5.9265dB */
	{ 0x1F8 ,  0x0,	0xFB},		/*index: 771, 5.8795dB */
	{ 0x1F5 ,  0x0,	0xF9},		/*index: 772, 5.8325dB */
	{ 0x1F2 ,  0x0,	0xF8},		/*index: 773, 5.7854dB */
	{ 0x1F0 ,  0x0,	0xF7},		/*index: 774, 5.7384dB */
	{ 0x1ED ,  0x0,	0xF5},		/*index: 775, 5.6913dB */
	{ 0x1EA ,  0x0,	0xF4},		/*index: 776, 5.6443dB */
	{ 0x1E8 ,  0x0,	0xF3},		/*index: 777, 5.5973dB */
	{ 0x1E5 ,  0x0,	0xF1},		/*index: 778, 5.5502dB */
	{ 0x1E2 ,  0x0,	0xF0},		/*index: 779, 5.5032dB */
	{ 0x1E0 ,  0x0,	0xEF},		/*index: 780, 5.4562dB */
	{ 0x1DD ,  0x0,	0xEE},		/*index: 781, 5.4091dB */
	{ 0x1DB ,  0x0,	0xEC},		/*index: 782, 5.3621dB */
	{ 0x1D8 ,  0x0,	0xEB},		/*index: 783, 5.3151dB */
	{ 0x1D6 ,  0x0,	0xEA},		/*index: 784, 5.268dB */
	{ 0x1D3 ,  0x0,	0xE9},		/*index: 785, 5.221dB */
	{ 0x1D0 ,  0x0,	0xE7},		/*index: 786, 5.174dB */
	{ 0x1CE ,  0x0,	0xE6},		/*index: 787, 5.1269dB */
	{ 0x1CB ,  0x0,	0xE5},		/*index: 788, 5.0799dB */
	{ 0x1C9 ,  0x0,	0xE4},		/*index: 789, 5.0328dB */
	{ 0x1C6 ,  0x0,	0xE2},		/*index: 790, 4.9858dB */
	{ 0x1C4 ,  0x0,	0xE1},		/*index: 791, 4.9388dB */
	{ 0x1C2 ,  0x0,	0xE0},		/*index: 792, 4.8917dB */
	{ 0x1BF ,  0x0,	0xDF},		/*index: 793, 4.8447dB */
	{ 0x1BD ,  0x0,	0xDD},		/*index: 794, 4.7977dB */
	{ 0x1BA ,  0x0,	0xDC},		/*index: 795, 4.7506dB */
	{ 0x1B8 ,  0x0,	0xDB},		/*index: 796, 4.7036dB */
	{ 0x1B6 ,  0x0,	0xDA},		/*index: 797, 4.6566dB */
	{ 0x1B3 ,  0x0,	0xD9},		/*index: 798, 4.6095dB */
	{ 0x1B1 ,  0x0,	0xD8},		/*index: 799, 4.5625dB */
	{ 0x1AF ,  0x0,	0xD6},		/*index: 800, 4.5154dB */
	{ 0x1AC ,  0x0,	0xD5},		/*index: 801, 4.4684dB */
	{ 0x1AA ,  0x0,	0xD4},		/*index: 802, 4.4214dB */
	{ 0x1A8 ,  0x0,	0xD3},		/*index: 803, 4.3743dB */
	{ 0x1A5 ,  0x0,	0xD2},		/*index: 804, 4.3273dB */
	{ 0x1A3 ,  0x0,	0xD1},		/*index: 805, 4.2803dB */
	{ 0x1A1 ,  0x0,	0xD0},		/*index: 806, 4.2332dB */
	{ 0x19F ,  0x0,	0xCE},		/*index: 807, 4.1862dB */
	{ 0x19C ,  0x0,	0xCD},		/*index: 808, 4.1392dB */
	{ 0x19A ,  0x0,	0xCC},	//	/*index: 809, 4.0824dB */
	{ 0x198 ,  0x0,	0xCB},		/*index: 810, 4.0451dB */
	{ 0x196 ,  0x0,	0xCA},		/*index: 811, 3.9981dB */
	{ 0x193 ,  0x0,	0xC9},		/*index: 812, 3.951dB */
	{ 0x191 ,  0x0,	0xC8},		/*index: 813, 3.904dB */
	{ 0x18F ,  0x0,	0xC7},		/*index: 814, 3.8569dB */
	{ 0x18D ,  0x0,	0xC6},		/*index: 815, 3.8099dB */
	{ 0x18B ,  0x0,	0xC5},		/*index: 816, 3.7629dB */
	{ 0x189 ,  0x0,	0xC4},		/*index: 817, 3.7158dB */
	{ 0x187 ,  0x0,	0xC2},		/*index: 818, 3.6688dB */
	{ 0x184 ,  0x0,	0xC1},		/*index: 819, 3.6218dB */
	{ 0x182 ,  0x0,	0xC0},		/*index: 820, 3.5747dB */
	{ 0x180 ,  0x0,	0xBF},		/*index: 821, 3.5277dB */
	{ 0x17E ,  0x0,	0xBE},		/*index: 822, 3.4807dB */
	{ 0x17C ,  0x0,	0xBD},		/*index: 823, 3.4336dB */
	{ 0x17A ,  0x0,	0xBC},		/*index: 824, 3.3866dB */
	{ 0x178 ,  0x0,	0xBB},		/*index: 825, 3.3396dB */
	{ 0x176 ,  0x0,	0xBA},		/*index: 826, 3.2925dB */
	{ 0x174 ,  0x0,	0xB9},		/*index: 827, 3.2455dB */
	{ 0x172 ,  0x0,	0xB8},		/*index: 828, 3.1984dB */
	{ 0x170 ,  0x0,	0xB7},		/*index: 829, 3.1514dB */
	{ 0x16E ,  0x0,	0xB6},		/*index: 830, 3.1044dB */
	{ 0x16C ,  0x0,	0xB5},		/*index: 831, 3.0573dB */
	{ 0x16A ,  0x0,	0xB4},		/*index: 832, 3.0103dB */
	{ 0x168 ,  0x0,	0xB3},		/*index: 833, 2.9633dB */
	{ 0x166 ,  0x0,	0xB2},		/*index: 834, 2.9162dB */
	{ 0x164 ,  0x0,	0xB1},		/*index: 835, 2.8692dB */
	{ 0x162 ,  0x0,	0xB0},		/*index: 836, 2.8222dB */
	{ 0x160 ,  0x0,	0xAF},		/*index: 837, 2.7751dB */
	{ 0x15E ,  0x0,	0xAF},		/*index: 838, 2.7281dB */
	{ 0x15D ,  0x0,	0xAE},		/*index: 839, 2.681dB */
	{ 0x15B ,  0x0,	0xAD},		/*index: 840, 2.634dB */
	{ 0x159 ,  0x0,	0xAC},		/*index: 841, 2.587dB */
	{ 0x157 ,  0x0,	0xAB},		/*index: 842, 2.5399dB */
	{ 0x155 ,  0x0,	0xAA},		/*index: 843, 2.4929dB */
	{ 0x153 ,  0x0,	0xA9},		/*index: 844, 2.4459dB */
	{ 0x151 ,  0x0,	0xA8},		/*index: 845, 2.3988dB */
	{ 0x150 ,  0x0,	0xA7},		/*index: 846, 2.3518dB */
	{ 0x14E ,  0x0,	0xA6},		/*index: 847, 2.3048dB */
	{ 0x14C ,  0x0,	0xA5},		/*index: 848, 2.2577dB */
	{ 0x14A ,  0x0,	0xA4},		/*index: 849, 2.2107dB */
	{ 0x148 ,  0x0,	0xA4},	//	/*index: 850, 2.1437dB */
	{ 0x147 ,  0x0,	0xA3},		/*index: 851, 2.1166dB */
	{ 0x145 ,  0x0,	0xA2},		/*index: 852, 2.0696dB */
	{ 0x143 ,  0x0,	0xA1},		/*index: 853, 2.0225dB */
	{ 0x141 ,  0x0,	0xA0},		/*index: 854, 1.9755dB */
	{ 0x140 ,  0x0,	0x9F},		/*index: 855, 1.9285dB */
	{ 0x13E ,  0x0,	0x9E},		/*index: 856, 1.8814dB */
	{ 0x13C ,  0x0,	0x9D},		/*index: 857, 1.8344dB */
	{ 0x13A ,  0x0,	0x9D},		/*index: 858, 1.7874dB */
	{ 0x139 ,  0x0,	0x9C},		/*index: 859, 1.7403dB */
	{ 0x137 ,  0x0,	0x9B},		/*index: 860, 1.6933dB */
	{ 0x135 ,  0x0,	0x9A},		/*index: 861, 1.6463dB */
	{ 0x134 ,  0x0,	0x99},		/*index: 862, 1.5992dB */
	{ 0x132 ,  0x0,	0x98},		/*index: 863, 1.5522dB */
	{ 0x130 ,  0x0,	0x98},		/*index: 864, 1.5051dB */
	{ 0x12F ,  0x0,	0x97},		/*index: 865, 1.4581dB */
	{ 0x12D ,  0x0,	0x96},		/*index: 866, 1.4111dB */
	{ 0x12C ,  0x0,	0x95},		/*index: 867, 1.364dB */
	{ 0x12A ,  0x0,	0x94},		/*index: 868, 1.317dB */
	{ 0x128 ,  0x0,	0x94},		/*index: 869, 1.27dB */
	{ 0x127 ,  0x0,	0x93},		/*index: 870, 1.2229dB */
	{ 0x125 ,  0x0,	0x92},		/*index: 871, 1.1759dB */
	{ 0x124 ,  0x0,	0x91},		/*index: 872, 1.1289dB */
	{ 0x122 ,  0x0,	0x90},		/*index: 873, 1.0818dB */
	{ 0x120 ,  0x0,	0x90},		/*index: 874, 1.0348dB */
	{ 0x11F ,  0x0,	0x8F},		/*index: 875, 0.9878dB */
	{ 0x11D ,  0x0,	0x8E},		/*index: 876, 0.9407dB */
	{ 0x11C ,  0x0,	0x8D},		/*index: 877, 0.8937dB */
	{ 0x11A ,  0x0,	0x8D},		/*index: 878, 0.8466dB */
	{ 0x119 ,  0x0,	0x8C},		/*index: 879, 0.7996dB */
	{ 0x117 ,  0x0,	0x8B},		/*index: 880, 0.7526dB */
	{ 0x116 ,  0x0,	0x8A},		/*index: 881, 0.7055dB */
	{ 0x114 ,  0x0,	0x8A},		/*index: 882, 0.6585dB */
	{ 0x113 ,  0x0,	0x89},		/*index: 883, 0.6115dB */
	{ 0x111 ,  0x0,	0x88},		/*index: 884, 0.5644dB */
	{ 0x110 ,  0x0,	0x87},		/*index: 885, 0.5174dB */
	{ 0x10E ,  0x0,	0x87},		/*index: 886, 0.4704dB */
	{ 0x10D ,  0x0,	0x86},		/*index: 887, 0.4233dB */
	{ 0x10B ,  0x0,	0x85},		/*index: 888, 0.3763dB */
	{ 0x10A ,  0x0,	0x85},		/*index: 889, 0.3293dB */
	{ 0x108 ,  0x0,	0x84},		/*index: 890, 0.2822dB */
	{ 0x107 ,  0x0,	0x84},		/*index: 891, 0.2352dB */
	{ 0x106 ,  0x0,	0x83},		/*index: 892, 0.1881dB */
	{ 0x104 ,  0x0,	0x82},		/*index: 893, 0.1411dB */
	{ 0x103 ,  0x0,	0x82},		/*index: 894, 0.0941dB */
	{ 0x101 ,  0x0,	0x81},		/*index: 895, 0.047dB */
	{ 0x100 ,  0x0, 0x80},		/*index: 896, 0dB */

};
#endif

#define AR0331_GAIN_COL_REG_AGAIN	(0)
#define AR0331_GAIN_COL_REG_DGAIN	(1)

/** AR0331 linear gain table row size */
#define AR0331_LINEAR_GAIN_ROWS		(858)
#define AR0331_LINEAR_GAIN_COLS		(2)
#define AR0331_LINEAR_GAIN_30DB		(601)
#define AR0331_LINEAR_GAIN_42DB		(857)

const s16 AR0331_LINEAR_GAIN_TABLE[AR0331_LINEAR_GAIN_ROWS][AR0331_LINEAR_GAIN_COLS] =
{
	{0x06, 0x80}, //index: 0, gain:1.833000DB->x1.234952
	{0x06, 0x81}, //index: 1, gain:1.880000DB->x1.241652
	{0x06, 0x81}, //index: 2, gain:1.927000DB->x1.248389
	{0x06, 0x82}, //index: 3, gain:1.974000DB->x1.255163
	{0x06, 0x83}, //index: 4, gain:2.021000DB->x1.261973
	{0x06, 0x84}, //index: 5, gain:2.068000DB->x1.268820
	{0x06, 0x84}, //index: 6, gain:2.115000DB->x1.275704
	{0x07, 0x80}, //index: 7, gain:2.162000DB->x1.282626
	{0x07, 0x80}, //index: 8, gain:2.209000DB->x1.289585
	{0x07, 0x81}, //index: 9, gain:2.256000DB->x1.296582
	{0x07, 0x82}, //index: 10, gain:2.303000DB->x1.303617
	{0x07, 0x83}, //index: 11, gain:2.350000DB->x1.310690
	{0x07, 0x83}, //index: 12, gain:2.397000DB->x1.317802
	{0x07, 0x84}, //index: 13, gain:2.444000DB->x1.324952
	{0x07, 0x85}, //index: 14, gain:2.491000DB->x1.332140
	{0x07, 0x85}, //index: 15, gain:2.538000DB->x1.339368
	{0x08, 0x80}, //index: 16, gain:2.585000DB->x1.346635
	{0x08, 0x81}, //index: 17, gain:2.632000DB->x1.353942
	{0x08, 0x82}, //index: 18, gain:2.679000DB->x1.361288
	{0x08, 0x82}, //index: 19, gain:2.726000DB->x1.368674
	{0x08, 0x83}, //index: 20, gain:2.773000DB->x1.376100
	{0x08, 0x84}, //index: 21, gain:2.820000DB->x1.383566
	{0x09, 0x80}, //index: 22, gain:2.867000DB->x1.391073
	{0x09, 0x80}, //index: 23, gain:2.914000DB->x1.398621
	{0x09, 0x81}, //index: 24, gain:2.961000DB->x1.406209
	{0x09, 0x82}, //index: 25, gain:3.008000DB->x1.413839
	{0x09, 0x82}, //index: 26, gain:3.055000DB->x1.421510
	{0x09, 0x83}, //index: 27, gain:3.102000DB->x1.429223
	{0x09, 0x84}, //index: 28, gain:3.149000DB->x1.436978
	{0x09, 0x85}, //index: 29, gain:3.196000DB->x1.444774
	{0x0a, 0x80}, //index: 30, gain:3.243000DB->x1.452613
	{0x0a, 0x80}, //index: 31, gain:3.290000DB->x1.460495
	{0x0a, 0x81}, //index: 32, gain:3.337000DB->x1.468419
	{0x0a, 0x82}, //index: 33, gain:3.384000DB->x1.476386
	{0x0a, 0x83}, //index: 34, gain:3.431000DB->x1.484397
	{0x0a, 0x83}, //index: 35, gain:3.478000DB->x1.492451
	{0x0a, 0x84}, //index: 36, gain:3.525000DB->x1.500548
	{0x0a, 0x85}, //index: 37, gain:3.572000DB->x1.508690
	{0x0a, 0x85}, //index: 38, gain:3.619000DB->x1.516876
	{0x0b, 0x80}, //index: 39, gain:3.666000DB->x1.525106
	{0x0b, 0x81}, //index: 40, gain:3.713000DB->x1.533381
	{0x0b, 0x81}, //index: 41, gain:3.760000DB->x1.541700
	{0x0b, 0x82}, //index: 42, gain:3.807000DB->x1.550065
	{0x0b, 0x83}, //index: 43, gain:3.854000DB->x1.558476
	{0x0b, 0x83}, //index: 44, gain:3.901000DB->x1.566931
	{0x0b, 0x84}, //index: 45, gain:3.948000DB->x1.575433
	{0x0b, 0x85}, //index: 46, gain:3.995000DB->x1.583981
	{0x0b, 0x86}, //index: 47, gain:4.042000DB->x1.592575
	{0x0c, 0x80}, //index: 48, gain:4.089000DB->x1.601216
	{0x0c, 0x80}, //index: 49, gain:4.136000DB->x1.609904
	{0x0c, 0x81}, //index: 50, gain:4.183000DB->x1.618639
	{0x0c, 0x82}, //index: 51, gain:4.230000DB->x1.627421
	{0x0c, 0x82}, //index: 52, gain:4.277000DB->x1.636251
	{0x0c, 0x83}, //index: 53, gain:4.324000DB->x1.645129
	{0x0c, 0x84}, //index: 54, gain:4.371000DB->x1.654055
	{0x0c, 0x85}, //index: 55, gain:4.418000DB->x1.663030
	{0x0c, 0x85}, //index: 56, gain:4.465000DB->x1.672053
	{0x0c, 0x86}, //index: 57, gain:4.512000DB->x1.681125
	{0x0d, 0x80}, //index: 58, gain:4.559000DB->x1.690246
	{0x0d, 0x80}, //index: 59, gain:4.606000DB->x1.699417
	{0x0d, 0x81}, //index: 60, gain:4.653000DB->x1.708638
	{0x0d, 0x82}, //index: 61, gain:4.700000DB->x1.717908
	{0x0d, 0x82}, //index: 62, gain:4.747000DB->x1.727229
	{0x0d, 0x83}, //index: 63, gain:4.794000DB->x1.736601
	{0x0d, 0x84}, //index: 64, gain:4.841000DB->x1.746023
	{0x0d, 0x84}, //index: 65, gain:4.888000DB->x1.755497
	{0x0d, 0x85}, //index: 66, gain:4.935000DB->x1.765022
	{0x0d, 0x86}, //index: 67, gain:4.982000DB->x1.774598
	{0x0e, 0x80}, //index: 68, gain:5.029000DB->x1.784227
	{0x0e, 0x81}, //index: 69, gain:5.076000DB->x1.793907
	{0x0e, 0x81}, //index: 70, gain:5.123000DB->x1.803641
	{0x0e, 0x82}, //index: 71, gain:5.170000DB->x1.813427
	{0x0e, 0x83}, //index: 72, gain:5.217000DB->x1.823266
	{0x0e, 0x83}, //index: 73, gain:5.264000DB->x1.833158
	{0x0e, 0x84}, //index: 74, gain:5.311000DB->x1.843105
	{0x0e, 0x85}, //index: 75, gain:5.358000DB->x1.853105
	{0x0e, 0x85}, //index: 76, gain:5.405000DB->x1.863159
	{0x0e, 0x86}, //index: 77, gain:5.452000DB->x1.873268
	{0x0f, 0x80}, //index: 78, gain:5.499000DB->x1.883432
	{0x0f, 0x80}, //index: 79, gain:5.546000DB->x1.893651
	{0x0f, 0x81}, //index: 80, gain:5.593000DB->x1.903926
	{0x0f, 0x82}, //index: 81, gain:5.640000DB->x1.914256
	{0x0f, 0x83}, //index: 82, gain:5.687000DB->x1.924642
	{0x0f, 0x83}, //index: 83, gain:5.734000DB->x1.935085
	{0x0f, 0x84}, //index: 84, gain:5.781000DB->x1.945584
	{0x0f, 0x85}, //index: 85, gain:5.828000DB->x1.956140
	{0x0f, 0x85}, //index: 86, gain:5.875000DB->x1.966754
	{0x0f, 0x86}, //index: 87, gain:5.922000DB->x1.977425
	{0x0f, 0x87}, //index: 88, gain:5.969000DB->x1.988154
	{0x0f, 0x88}, //index: 89, gain:6.016000DB->x1.998941
	{0x10, 0x80}, //index: 90, gain:6.063000DB->x2.009787
	{0x10, 0x81}, //index: 91, gain:6.110000DB->x2.020691
	{0x10, 0x82}, //index: 92, gain:6.157000DB->x2.031655
	{0x10, 0x82}, //index: 93, gain:6.204000DB->x2.042678
	{0x10, 0x83}, //index: 94, gain:6.251000DB->x2.053761
	{0x10, 0x84}, //index: 95, gain:6.298000DB->x2.064905
	{0x10, 0x84}, //index: 96, gain:6.345000DB->x2.076108
	{0x10, 0x85}, //index: 97, gain:6.392000DB->x2.087373
	{0x10, 0x86}, //index: 98, gain:6.439000DB->x2.098698
	{0x10, 0x87}, //index: 99, gain:6.486000DB->x2.110085
	{0x10, 0x87}, //index: 100, gain:6.533000DB->x2.121534
	{0x10, 0x88}, //index: 101, gain:6.580000DB->x2.133045
	{0x12, 0x80}, //index: 102, gain:6.627000DB->x2.144618
	{0x12, 0x80}, //index: 103, gain:6.674000DB->x2.156254
	{0x12, 0x81}, //index: 104, gain:6.721000DB->x2.167954
	{0x12, 0x82}, //index: 105, gain:6.768000DB->x2.179716
	{0x12, 0x83}, //index: 106, gain:6.815000DB->x2.191543
	{0x12, 0x83}, //index: 107, gain:6.862000DB->x2.203434
	{0x12, 0x84}, //index: 108, gain:6.909000DB->x2.215389
	{0x12, 0x85}, //index: 109, gain:6.956000DB->x2.227409
	{0x12, 0x85}, //index: 110, gain:7.003000DB->x2.239494
	{0x12, 0x86}, //index: 111, gain:7.050000DB->x2.251645
	{0x12, 0x87}, //index: 112, gain:7.097000DB->x2.263862
	{0x12, 0x88}, //index: 113, gain:7.144000DB->x2.276145
	{0x14, 0x80}, //index: 114, gain:7.191000DB->x2.288495
	{0x14, 0x81}, //index: 115, gain:7.238000DB->x2.300912
	{0x14, 0x81}, //index: 116, gain:7.285000DB->x2.313396
	{0x14, 0x82}, //index: 117, gain:7.332000DB->x2.325948
	{0x14, 0x83}, //index: 118, gain:7.379000DB->x2.338568
	{0x14, 0x84}, //index: 119, gain:7.426000DB->x2.351256
	{0x14, 0x84}, //index: 120, gain:7.473000DB->x2.364014
	{0x14, 0x85}, //index: 121, gain:7.520000DB->x2.376840
	{0x14, 0x86}, //index: 122, gain:7.567000DB->x2.389736
	{0x14, 0x86}, //index: 123, gain:7.614000DB->x2.402702
	{0x14, 0x87}, //index: 124, gain:7.661000DB->x2.415739
	{0x14, 0x88}, //index: 125, gain:7.708000DB->x2.428846
	{0x14, 0x89}, //index: 126, gain:7.755000DB->x2.442024
	{0x14, 0x89}, //index: 127, gain:7.802000DB->x2.455274
	{0x14, 0x8a}, //index: 128, gain:7.849000DB->x2.468596
	{0x16, 0x80}, //index: 129, gain:7.896000DB->x2.481990
	{0x16, 0x81}, //index: 130, gain:7.943000DB->x2.495456
	{0x16, 0x82}, //index: 131, gain:7.990000DB->x2.508996
	{0x16, 0x82}, //index: 132, gain:8.037000DB->x2.522609
	{0x16, 0x83}, //index: 133, gain:8.084000DB->x2.536296
	{0x16, 0x84}, //index: 134, gain:8.131000DB->x2.550058
	{0x16, 0x84}, //index: 135, gain:8.178000DB->x2.563894
	{0x16, 0x85}, //index: 136, gain:8.225000DB->x2.577805
	{0x16, 0x86}, //index: 137, gain:8.272000DB->x2.591791
	{0x16, 0x87}, //index: 138, gain:8.319000DB->x2.605854
	{0x16, 0x87}, //index: 139, gain:8.366000DB->x2.619992
	{0x16, 0x88}, //index: 140, gain:8.413000DB->x2.634208
	{0x16, 0x89}, //index: 141, gain:8.460000DB->x2.648500
	{0x16, 0x89}, //index: 142, gain:8.507000DB->x2.662870
	{0x18, 0x80}, //index: 143, gain:8.554000DB->x2.677318
	{0x18, 0x81}, //index: 144, gain:8.601000DB->x2.691845
	{0x18, 0x81}, //index: 145, gain:8.648000DB->x2.706450
	{0x18, 0x82}, //index: 146, gain:8.695000DB->x2.721134
	{0x18, 0x83}, //index: 147, gain:8.742000DB->x2.735899
	{0x18, 0x83}, //index: 148, gain:8.789000DB->x2.750743
	{0x18, 0x84}, //index: 149, gain:8.836000DB->x2.765668
	{0x18, 0x85}, //index: 150, gain:8.883000DB->x2.780674
	{0x18, 0x86}, //index: 151, gain:8.930000DB->x2.795761
	{0x18, 0x86}, //index: 152, gain:8.977000DB->x2.810930
	{0x18, 0x87}, //index: 153, gain:9.024000DB->x2.826181
	{0x18, 0x88}, //index: 154, gain:9.071000DB->x2.841515
	{0x18, 0x88}, //index: 155, gain:9.118000DB->x2.856933
	{0x18, 0x89}, //index: 156, gain:9.165000DB->x2.872434
	{0x18, 0x8a}, //index: 157, gain:9.212000DB->x2.888019
	{0x18, 0x8b}, //index: 158, gain:9.259000DB->x2.903688
	{0x1a, 0x80}, //index: 159, gain:9.306000DB->x2.919443
	{0x1a, 0x81}, //index: 160, gain:9.353000DB->x2.935283
	{0x1a, 0x81}, //index: 161, gain:9.400000DB->x2.951209
	{0x1a, 0x82}, //index: 162, gain:9.447000DB->x2.967222
	{0x1a, 0x83}, //index: 163, gain:9.494000DB->x2.983321
	{0x1a, 0x83}, //index: 164, gain:9.541000DB->x2.999508
	{0x1a, 0x84}, //index: 165, gain:9.588000DB->x3.015782
	{0x1a, 0x85}, //index: 166, gain:9.635000DB->x3.032145
	{0x1a, 0x86}, //index: 167, gain:9.682000DB->x3.048597
	{0x1a, 0x86}, //index: 168, gain:9.729000DB->x3.065138
	{0x1a, 0x87}, //index: 169, gain:9.776000DB->x3.081768
	{0x1a, 0x88}, //index: 170, gain:9.823000DB->x3.098489
	{0x1a, 0x89}, //index: 171, gain:9.870000DB->x3.115301
	{0x1a, 0x89}, //index: 172, gain:9.917000DB->x3.132204
	{0x1a, 0x8a}, //index: 173, gain:9.964000DB->x3.149198
	{0x1a, 0x8b}, //index: 174, gain:10.011000DB->x3.166285
	{0x1a, 0x8c}, //index: 175, gain:10.058000DB->x3.183464
	{0x1c, 0x80}, //index: 176, gain:10.105000DB->x3.200737
	{0x1c, 0x80}, //index: 177, gain:10.152000DB->x3.218103
	{0x1c, 0x81}, //index: 178, gain:10.199000DB->x3.235564
	{0x1c, 0x82}, //index: 179, gain:10.246000DB->x3.253119
	{0x1c, 0x82}, //index: 180, gain:10.293000DB->x3.270770
	{0x1c, 0x83}, //index: 181, gain:10.340000DB->x3.288516
	{0x1c, 0x84}, //index: 182, gain:10.387000DB->x3.306359
	{0x1c, 0x84}, //index: 183, gain:10.434000DB->x3.324298
	{0x1c, 0x85}, //index: 184, gain:10.481000DB->x3.342335
	{0x1c, 0x86}, //index: 185, gain:10.528000DB->x3.360470
	{0x1c, 0x87}, //index: 186, gain:10.575000DB->x3.378703
	{0x1c, 0x87}, //index: 187, gain:10.622000DB->x3.397035
	{0x1c, 0x88}, //index: 188, gain:10.669000DB->x3.415466
	{0x1c, 0x89}, //index: 189, gain:10.716000DB->x3.433998
	{0x1c, 0x8a}, //index: 190, gain:10.763000DB->x3.452630
	{0x1c, 0x8a}, //index: 191, gain:10.810000DB->x3.471363
	{0x1c, 0x8b}, //index: 192, gain:10.857000DB->x3.490197
	{0x1c, 0x8c}, //index: 193, gain:10.904000DB->x3.509134
	{0x1c, 0x8d}, //index: 194, gain:10.951000DB->x3.528174
	{0x1c, 0x8d}, //index: 195, gain:10.998000DB->x3.547317
	{0x1e, 0x80}, //index: 196, gain:11.045000DB->x3.566564
	{0x1e, 0x80}, //index: 197, gain:11.092000DB->x3.585915
	{0x1e, 0x81}, //index: 198, gain:11.139000DB->x3.605371
	{0x1e, 0x82}, //index: 199, gain:11.186000DB->x3.624933
	{0x1e, 0x83}, //index: 200, gain:11.233000DB->x3.644601
	{0x1e, 0x83}, //index: 201, gain:11.280000DB->x3.664376
	{0x1e, 0x84}, //index: 202, gain:11.327000DB->x3.684258
	{0x1e, 0x85}, //index: 203, gain:11.374000DB->x3.704248
	{0x1e, 0x85}, //index: 204, gain:11.421000DB->x3.724346
	{0x1e, 0x86}, //index: 205, gain:11.468000DB->x3.744553
	{0x1e, 0x87}, //index: 206, gain:11.515000DB->x3.764870
	{0x1e, 0x88}, //index: 207, gain:11.562000DB->x3.785297
	{0x1e, 0x88}, //index: 208, gain:11.609000DB->x3.805835
	{0x1e, 0x89}, //index: 209, gain:11.656000DB->x3.826485
	{0x1e, 0x8a}, //index: 210, gain:11.703000DB->x3.847246
	{0x1e, 0x8b}, //index: 211, gain:11.750000DB->x3.868121
	{0x1e, 0x8b}, //index: 212, gain:11.797000DB->x3.889108
	{0x1e, 0x8c}, //index: 213, gain:11.844000DB->x3.910209
	{0x1e, 0x8d}, //index: 214, gain:11.891000DB->x3.931425
	{0x1e, 0x8e}, //index: 215, gain:11.938000DB->x3.952756
	{0x1e, 0x8e}, //index: 216, gain:11.985000DB->x3.974203
	{0x1e, 0x8f}, //index: 217, gain:12.032000DB->x3.995766
	{0x20, 0x80}, //index: 218, gain:12.079000DB->x4.017446
	{0x20, 0x81}, //index: 219, gain:12.126000DB->x4.039243
	{0x20, 0x81}, //index: 220, gain:12.173000DB->x4.061159
	{0x20, 0x82}, //index: 221, gain:12.220000DB->x4.083194
	{0x20, 0x83}, //index: 222, gain:12.267000DB->x4.105348
	{0x20, 0x84}, //index: 223, gain:12.314000DB->x4.127623
	{0x20, 0x84}, //index: 224, gain:12.361000DB->x4.150018
	{0x20, 0x85}, //index: 225, gain:12.408000DB->x4.172535
	{0x20, 0x86}, //index: 226, gain:12.455000DB->x4.195174
	{0x20, 0x86}, //index: 227, gain:12.502000DB->x4.217936
	{0x20, 0x87}, //index: 228, gain:12.549000DB->x4.240822
	{0x20, 0x88}, //index: 229, gain:12.596000DB->x4.263831
	{0x20, 0x89}, //index: 230, gain:12.643000DB->x4.286966
	{0x20, 0x89}, //index: 231, gain:12.690000DB->x4.310226
	{0x20, 0x8a}, //index: 232, gain:12.737000DB->x4.333612
	{0x20, 0x8b}, //index: 233, gain:12.784000DB->x4.357125
	{0x20, 0x8c}, //index: 234, gain:12.831000DB->x4.380765
	{0x20, 0x8c}, //index: 235, gain:12.878000DB->x4.404534
	{0x20, 0x8d}, //index: 236, gain:12.925000DB->x4.428432
	{0x20, 0x8e}, //index: 237, gain:12.972000DB->x4.452460
	{0x20, 0x8f}, //index: 238, gain:13.019000DB->x4.476618
	{0x20, 0x90}, //index: 239, gain:13.066000DB->x4.500907
	{0x20, 0x90}, //index: 240, gain:13.113000DB->x4.525327
	{0x20, 0x91}, //index: 241, gain:13.160000DB->x4.549881
	{0x24, 0x80}, //index: 242, gain:13.207000DB->x4.574567
	{0x24, 0x81}, //index: 243, gain:13.254000DB->x4.599387
	{0x24, 0x81}, //index: 244, gain:13.301000DB->x4.624343
	{0x24, 0x82}, //index: 245, gain:13.348000DB->x4.649433
	{0x24, 0x83}, //index: 246, gain:13.395000DB->x4.674660
	{0x24, 0x83}, //index: 247, gain:13.442000DB->x4.700023
	{0x24, 0x84}, //index: 248, gain:13.489000DB->x4.725524
	{0x24, 0x85}, //index: 249, gain:13.536000DB->x4.751164
	{0x24, 0x86}, //index: 250, gain:13.583000DB->x4.776942
	{0x24, 0x86}, //index: 251, gain:13.630000DB->x4.802861
	{0x24, 0x87}, //index: 252, gain:13.677000DB->x4.828920
	{0x24, 0x88}, //index: 253, gain:13.724000DB->x4.855120
	{0x24, 0x89}, //index: 254, gain:13.771000DB->x4.881463
	{0x24, 0x89}, //index: 255, gain:13.818000DB->x4.907949
	{0x24, 0x8a}, //index: 256, gain:13.865000DB->x4.934578
	{0x24, 0x8b}, //index: 257, gain:13.912000DB->x4.961352
	{0x24, 0x8c}, //index: 258, gain:13.959000DB->x4.988271
	{0x24, 0x8c}, //index: 259, gain:14.006000DB->x5.015336
	{0x24, 0x8d}, //index: 260, gain:14.053000DB->x5.042548
	{0x24, 0x8e}, //index: 261, gain:14.100000DB->x5.069907
	{0x24, 0x8f}, //index: 262, gain:14.147000DB->x5.097415
	{0x24, 0x8f}, //index: 263, gain:14.194000DB->x5.125072
	{0x24, 0x90}, //index: 264, gain:14.241000DB->x5.152880
	{0x24, 0x91}, //index: 265, gain:14.288000DB->x5.180838
	{0x24, 0x92}, //index: 266, gain:14.335000DB->x5.208948
	{0x24, 0x93}, //index: 267, gain:14.382000DB->x5.237210
	{0x24, 0x93}, //index: 268, gain:14.429000DB->x5.265626
	{0x24, 0x94}, //index: 269, gain:14.476000DB->x5.294196
	{0x24, 0x95}, //index: 270, gain:14.523000DB->x5.322921
	{0x28, 0x80}, //index: 271, gain:14.570000DB->x5.351802
	{0x28, 0x80}, //index: 272, gain:14.617000DB->x5.380839
	{0x28, 0x81}, //index: 273, gain:14.664000DB->x5.410034
	{0x28, 0x82}, //index: 274, gain:14.711000DB->x5.439388
	{0x28, 0x83}, //index: 275, gain:14.758000DB->x5.468900
	{0x28, 0x83}, //index: 276, gain:14.805000DB->x5.498573
	{0x28, 0x84}, //index: 277, gain:14.852000DB->x5.528407
	{0x28, 0x85}, //index: 278, gain:14.899000DB->x5.558403
	{0x28, 0x85}, //index: 279, gain:14.946000DB->x5.588561
	{0x28, 0x86}, //index: 280, gain:14.993000DB->x5.618883
	{0x28, 0x87}, //index: 281, gain:15.040000DB->x5.649370
	{0x28, 0x88}, //index: 282, gain:15.087000DB->x5.680022
	{0x28, 0x88}, //index: 283, gain:15.134000DB->x5.710840
	{0x28, 0x89}, //index: 284, gain:15.181000DB->x5.741826
	{0x28, 0x8a}, //index: 285, gain:15.228000DB->x5.772979
	{0x28, 0x8b}, //index: 286, gain:15.275000DB->x5.804302
	{0x28, 0x8b}, //index: 287, gain:15.322000DB->x5.835795
	{0x28, 0x8c}, //index: 288, gain:15.369000DB->x5.867458
	{0x28, 0x8d}, //index: 289, gain:15.416000DB->x5.899293
	{0x28, 0x8e}, //index: 290, gain:15.463000DB->x5.931301
	{0x28, 0x8e}, //index: 291, gain:15.510000DB->x5.963483
	{0x28, 0x8f}, //index: 292, gain:15.557000DB->x5.995840
	{0x28, 0x90}, //index: 293, gain:15.604000DB->x6.028371
	{0x28, 0x91}, //index: 294, gain:15.651000DB->x6.061080
	{0x28, 0x92}, //index: 295, gain:15.698000DB->x6.093966
	{0x28, 0x92}, //index: 296, gain:15.745000DB->x6.127030
	{0x28, 0x93}, //index: 297, gain:15.792000DB->x6.160274
	{0x28, 0x94}, //index: 298, gain:15.839000DB->x6.193698
	{0x28, 0x95}, //index: 299, gain:15.886000DB->x6.227303
	{0x28, 0x96}, //index: 300, gain:15.933000DB->x6.261091
	{0x28, 0x96}, //index: 301, gain:15.980000DB->x6.295062
	{0x28, 0x97}, //index: 302, gain:16.027000DB->x6.329217
	{0x28, 0x98}, //index: 303, gain:16.074000DB->x6.363558
	{0x28, 0x99}, //index: 304, gain:16.121000DB->x6.398085
	{0x2c, 0x80}, //index: 305, gain:16.168000DB->x6.432799
	{0x2c, 0x81}, //index: 306, gain:16.215000DB->x6.467702
	{0x2c, 0x81}, //index: 307, gain:16.262000DB->x6.502794
	{0x2c, 0x82}, //index: 308, gain:16.309000DB->x6.538077
	{0x2c, 0x83}, //index: 309, gain:16.356000DB->x6.573550
	{0x2c, 0x83}, //index: 310, gain:16.403000DB->x6.609217
	{0x2c, 0x84}, //index: 311, gain:16.450000DB->x6.645077
	{0x2c, 0x85}, //index: 312, gain:16.497000DB->x6.681131
	{0x2c, 0x86}, //index: 313, gain:16.544000DB->x6.717381
	{0x2c, 0x86}, //index: 314, gain:16.591000DB->x6.753828
	{0x2c, 0x87}, //index: 315, gain:16.638000DB->x6.790473
	{0x2c, 0x88}, //index: 316, gain:16.685000DB->x6.827316
	{0x2c, 0x89}, //index: 317, gain:16.732000DB->x6.864359
	{0x2c, 0x89}, //index: 318, gain:16.779000DB->x6.901603
	{0x2c, 0x8a}, //index: 319, gain:16.826000DB->x6.939050
	{0x2c, 0x8b}, //index: 320, gain:16.873000DB->x6.976699
	{0x2c, 0x8c}, //index: 321, gain:16.920000DB->x7.014553
	{0x2c, 0x8c}, //index: 322, gain:16.967000DB->x7.052612
	{0x2c, 0x8d}, //index: 323, gain:17.014000DB->x7.090878
	{0x2c, 0x8e}, //index: 324, gain:17.061000DB->x7.129351
	{0x2c, 0x8f}, //index: 325, gain:17.108000DB->x7.168033
	{0x2c, 0x8f}, //index: 326, gain:17.155000DB->x7.206925
	{0x2c, 0x90}, //index: 327, gain:17.202000DB->x7.246028
	{0x2c, 0x91}, //index: 328, gain:17.249000DB->x7.285343
	{0x2c, 0x92}, //index: 329, gain:17.296000DB->x7.324871
	{0x2c, 0x93}, //index: 330, gain:17.343000DB->x7.364614
	{0x2c, 0x93}, //index: 331, gain:17.390000DB->x7.404573
	{0x2c, 0x94}, //index: 332, gain:17.437000DB->x7.444748
	{0x2c, 0x95}, //index: 333, gain:17.484000DB->x7.485141
	{0x2c, 0x96}, //index: 334, gain:17.531000DB->x7.525754
	{0x2c, 0x97}, //index: 335, gain:17.578000DB->x7.566586
	{0x2c, 0x97}, //index: 336, gain:17.625000DB->x7.607641
	{0x2c, 0x98}, //index: 337, gain:17.672000DB->x7.648918
	{0x2c, 0x99}, //index: 338, gain:17.719000DB->x7.690419
	{0x2c, 0x9a}, //index: 339, gain:17.766000DB->x7.732145
	{0x2c, 0x9b}, //index: 340, gain:17.813000DB->x7.774098
	{0x2c, 0x9c}, //index: 341, gain:17.860000DB->x7.816278
	{0x2c, 0x9c}, //index: 342, gain:17.907000DB->x7.858687
	{0x2c, 0x9d}, //index: 343, gain:17.954000DB->x7.901326
	{0x2c, 0x9e}, //index: 344, gain:18.001000DB->x7.944197
	{0x2c, 0x9f}, //index: 345, gain:18.048000DB->x7.987300
	{0x30, 0x80}, //index: 346, gain:18.095000DB->x8.030637
	{0x30, 0x81}, //index: 347, gain:18.142000DB->x8.074209
	{0x30, 0x81}, //index: 348, gain:18.189000DB->x8.118018
	{0x30, 0x82}, //index: 349, gain:18.236000DB->x8.162064
	{0x30, 0x83}, //index: 350, gain:18.283000DB->x8.206349
	{0x30, 0x84}, //index: 351, gain:18.330000DB->x8.250875
	{0x30, 0x84}, //index: 352, gain:18.377000DB->x8.295642
	{0x30, 0x85}, //index: 353, gain:18.424000DB->x8.340652
	{0x30, 0x86}, //index: 354, gain:18.471000DB->x8.385906
	{0x30, 0x86}, //index: 355, gain:18.518000DB->x8.431406
	{0x30, 0x87}, //index: 356, gain:18.565000DB->x8.477153
	{0x30, 0x88}, //index: 357, gain:18.612000DB->x8.523147
	{0x30, 0x89}, //index: 358, gain:18.659000DB->x8.569392
	{0x30, 0x89}, //index: 359, gain:18.706000DB->x8.615887
	{0x30, 0x8a}, //index: 360, gain:18.753000DB->x8.662635
	{0x30, 0x8b}, //index: 361, gain:18.800000DB->x8.709636
	{0x30, 0x8c}, //index: 362, gain:18.847000DB->x8.756892
	{0x30, 0x8c}, //index: 363, gain:18.894000DB->x8.804405
	{0x30, 0x8d}, //index: 364, gain:18.941000DB->x8.852175
	{0x30, 0x8e}, //index: 365, gain:18.988000DB->x8.900205
	{0x30, 0x8f}, //index: 366, gain:19.035000DB->x8.948495
	{0x30, 0x8f}, //index: 367, gain:19.082000DB->x8.997047
	{0x30, 0x90}, //index: 368, gain:19.129000DB->x9.045863
	{0x30, 0x91}, //index: 369, gain:19.176000DB->x9.094943
	{0x30, 0x92}, //index: 370, gain:19.223000DB->x9.144290
	{0x30, 0x93}, //index: 371, gain:19.270000DB->x9.193905
	{0x30, 0x93}, //index: 372, gain:19.317000DB->x9.243788
	{0x30, 0x94}, //index: 373, gain:19.364000DB->x9.293943
	{0x30, 0x95}, //index: 374, gain:19.411000DB->x9.344369
	{0x30, 0x96}, //index: 375, gain:19.458000DB->x9.395070
	{0x30, 0x97}, //index: 376, gain:19.505000DB->x9.446045
	{0x30, 0x97}, //index: 377, gain:19.552000DB->x9.497297
	{0x30, 0x98}, //index: 378, gain:19.599000DB->x9.548826
	{0x30, 0x99}, //index: 379, gain:19.646000DB->x9.600636
	{0x30, 0x9a}, //index: 380, gain:19.693000DB->x9.652726
	{0x30, 0x9b}, //index: 381, gain:19.740000DB->x9.705100
	{0x30, 0x9c}, //index: 382, gain:19.787000DB->x9.757757
	{0x30, 0x9c}, //index: 383, gain:19.834000DB->x9.810700
	{0x30, 0x9d}, //index: 384, gain:19.881000DB->x9.863930
	{0x30, 0x9e}, //index: 385, gain:19.928000DB->x9.917450
	{0x30, 0x9f}, //index: 386, gain:19.975000DB->x9.971259
	{0x30, 0xa0}, //index: 387, gain:20.022000DB->x10.025361
	{0x30, 0xa1}, //index: 388, gain:20.069000DB->x10.079756
	{0x30, 0xa2}, //index: 389, gain:20.116000DB->x10.134446
	{0x30, 0xa3}, //index: 390, gain:20.163000DB->x10.189433
	{0x30, 0xa3}, //index: 391, gain:20.210000DB->x10.244718
	{0x30, 0xa4}, //index: 392, gain:20.257000DB->x10.300303
	{0x30, 0xa5}, //index: 393, gain:20.304000DB->x10.356190
	{0x30, 0xa6}, //index: 394, gain:20.351000DB->x10.412380
	{0x30, 0xa7}, //index: 395, gain:20.398000DB->x10.468875
	{0x30, 0xa8}, //index: 396, gain:20.445000DB->x10.525676
	{0x30, 0xa9}, //index: 397, gain:20.492000DB->x10.582786
	{0x30, 0xaa}, //index: 398, gain:20.539000DB->x10.640205
	{0x30, 0xab}, //index: 399, gain:20.586000DB->x10.697936
	{0x30, 0xac}, //index: 400, gain:20.633000DB->x10.755980
	{0x30, 0xad}, //index: 401, gain:20.680000DB->x10.814340
	{0x30, 0xad}, //index: 402, gain:20.727000DB->x10.873015
	{0x30, 0xae}, //index: 403, gain:20.774000DB->x10.932009
	{0x30, 0xaf}, //index: 404, gain:20.821000DB->x10.991324
	{0x30, 0xb0}, //index: 405, gain:20.868000DB->x11.050960
	{0x30, 0xb1}, //index: 406, gain:20.915000DB->x11.110919
	{0x30, 0xb2}, //index: 407, gain:20.962000DB->x11.171204
	{0x30, 0xb3}, //index: 408, gain:21.009000DB->x11.231817
	{0x30, 0xb4}, //index: 409, gain:21.056000DB->x11.292757
	{0x30, 0xb5}, //index: 410, gain:21.103000DB->x11.354029
	{0x30, 0xb6}, //index: 411, gain:21.150000DB->x11.415633
	{0x30, 0xb7}, //index: 412, gain:21.197000DB->x11.477571
	{0x30, 0xb8}, //index: 413, gain:21.244000DB->x11.539846
	{0x30, 0xb9}, //index: 414, gain:21.291000DB->x11.602458
	{0x30, 0xba}, //index: 415, gain:21.338000DB->x11.665410
	{0x30, 0xbb}, //index: 416, gain:21.385000DB->x11.728703
	{0x30, 0xbc}, //index: 417, gain:21.432000DB->x11.792340
	{0x30, 0xbd}, //index: 418, gain:21.479000DB->x11.856322
	{0x30, 0xbe}, //index: 419, gain:21.526000DB->x11.920652
	{0x30, 0xbf}, //index: 420, gain:21.573000DB->x11.985330
	{0x30, 0xc0}, //index: 421, gain:21.620000DB->x12.050359
	{0x30, 0xc1}, //index: 422, gain:21.667000DB->x12.115742
	{0x30, 0xc2}, //index: 423, gain:21.714000DB->x12.181478
	{0x30, 0xc3}, //index: 424, gain:21.761000DB->x12.247572
	{0x30, 0xc5}, //index: 425, gain:21.808000DB->x12.314024
	{0x30, 0xc6}, //index: 426, gain:21.855000DB->x12.380837
	{0x30, 0xc7}, //index: 427, gain:21.902000DB->x12.448012
	{0x30, 0xc8}, //index: 428, gain:21.949000DB->x12.515552
	{0x30, 0xc9}, //index: 429, gain:21.996000DB->x12.583458
	{0x30, 0xca}, //index: 430, gain:22.043000DB->x12.651732
	{0x30, 0xcb}, //index: 431, gain:22.090000DB->x12.720378
	{0x30, 0xcc}, //index: 432, gain:22.137000DB->x12.789395
	{0x30, 0xcd}, //index: 433, gain:22.184000DB->x12.858787
	{0x30, 0xce}, //index: 434, gain:22.231000DB->x12.928555
	{0x30, 0xcf}, //index: 435, gain:22.278000DB->x12.998702
	{0x30, 0xd1}, //index: 436, gain:22.325000DB->x13.069230
	{0x30, 0xd2}, //index: 437, gain:22.372000DB->x13.140140
	{0x30, 0xd3}, //index: 438, gain:22.419000DB->x13.211435
	{0x30, 0xd4}, //index: 439, gain:22.466000DB->x13.283117
	{0x30, 0xd5}, //index: 440, gain:22.513000DB->x13.355188
	{0x30, 0xd6}, //index: 441, gain:22.560000DB->x13.427650
	{0x30, 0xd8}, //index: 442, gain:22.607000DB->x13.500505
	{0x30, 0xd9}, //index: 443, gain:22.654000DB->x13.573755
	{0x30, 0xda}, //index: 444, gain:22.701000DB->x13.647402
	{0x30, 0xdb}, //index: 445, gain:22.748000DB->x13.721450
	{0x30, 0xdc}, //index: 446, gain:22.795000DB->x13.795899
	{0x30, 0xdd}, //index: 447, gain:22.842000DB->x13.870752
	{0x30, 0xdf}, //index: 448, gain:22.889000DB->x13.946011
	{0x30, 0xe0}, //index: 449, gain:22.936000DB->x14.021678
	{0x30, 0xe1}, //index: 450, gain:22.983000DB->x14.097756
	{0x30, 0xe2}, //index: 451, gain:23.030000DB->x14.174247
	{0x30, 0xe4}, //index: 452, gain:23.077000DB->x14.251153
	{0x30, 0xe5}, //index: 453, gain:23.124000DB->x14.328476
	{0x30, 0xe6}, //index: 454, gain:23.171000DB->x14.406219
	{0x30, 0xe7}, //index: 455, gain:23.218000DB->x14.484383
	{0x30, 0xe9}, //index: 456, gain:23.265000DB->x14.562972
	{0x30, 0xea}, //index: 457, gain:23.312000DB->x14.641986
	{0x30, 0xeb}, //index: 458, gain:23.359000DB->x14.721430
	{0x30, 0xec}, //index: 459, gain:23.406000DB->x14.801305
	{0x30, 0xee}, //index: 460, gain:23.453000DB->x14.881613
	{0x30, 0xef}, //index: 461, gain:23.500000DB->x14.962357
	{0x30, 0xf0}, //index: 462, gain:23.547000DB->x15.043538
	{0x30, 0xf2}, //index: 463, gain:23.594000DB->x15.125161
	{0x30, 0xf3}, //index: 464, gain:23.641000DB->x15.207226
	{0x30, 0xf4}, //index: 465, gain:23.688000DB->x15.289736
	{0x30, 0xf5}, //index: 466, gain:23.735000DB->x15.372695
	{0x30, 0xf7}, //index: 467, gain:23.782000DB->x15.456103
	{0x30, 0xf8}, //index: 468, gain:23.829000DB->x15.539964
	{0x30, 0xf9}, //index: 469, gain:23.876000DB->x15.624280
	{0x30, 0xfb}, //index: 470, gain:23.923000DB->x15.709053
	{0x30, 0xfc}, //index: 471, gain:23.970000DB->x15.794286
	{0x30, 0xfe}, //index: 472, gain:24.017000DB->x15.879982
	{0x30, 0xff}, //index: 473, gain:24.064000DB->x15.966142
	{0x30, 0x100}, //index: 474, gain:24.111000DB->x16.052771
	{0x30, 0x102}, //index: 475, gain:24.158000DB->x16.139869
	{0x30, 0x103}, //index: 476, gain:24.205000DB->x16.227440
	{0x30, 0x105}, //index: 477, gain:24.252000DB->x16.315485
	{0x30, 0x106}, //index: 478, gain:24.299000DB->x16.404009
	{0x30, 0x107}, //index: 479, gain:24.346000DB->x16.493013
	{0x30, 0x109}, //index: 480, gain:24.393000DB->x16.582500
	{0x30, 0x10a}, //index: 481, gain:24.440000DB->x16.672472
	{0x30, 0x10c}, //index: 482, gain:24.487000DB->x16.762933
	{0x30, 0x10d}, //index: 483, gain:24.534000DB->x16.853884
	{0x30, 0x10f}, //index: 484, gain:24.581000DB->x16.945329
	{0x30, 0x110}, //index: 485, gain:24.628000DB->x17.037270
	{0x30, 0x112}, //index: 486, gain:24.675000DB->x17.129710
	{0x30, 0x113}, //index: 487, gain:24.722000DB->x17.222651
	{0x30, 0x115}, //index: 488, gain:24.769000DB->x17.316097
	{0x30, 0x116}, //index: 489, gain:24.816000DB->x17.410049
	{0x30, 0x118}, //index: 490, gain:24.863000DB->x17.504512
	{0x30, 0x119}, //index: 491, gain:24.910000DB->x17.599487
	{0x30, 0x11b}, //index: 492, gain:24.957000DB->x17.694977
	{0x30, 0x11c}, //index: 493, gain:25.004000DB->x17.790985
	{0x30, 0x11e}, //index: 494, gain:25.051000DB->x17.887515
	{0x30, 0x11f}, //index: 495, gain:25.098000DB->x17.984568
	{0x30, 0x121}, //index: 496, gain:25.145000DB->x18.082147
	{0x30, 0x122}, //index: 497, gain:25.192000DB->x18.180256
	{0x30, 0x124}, //index: 498, gain:25.239000DB->x18.278898
	{0x30, 0x126}, //index: 499, gain:25.286000DB->x18.378074
	{0x30, 0x127}, //index: 500, gain:25.333000DB->x18.477789
	{0x30, 0x129}, //index: 501, gain:25.380000DB->x18.578045
	{0x30, 0x12a}, //index: 502, gain:25.427000DB->x18.678844
	{0x30, 0x12c}, //index: 503, gain:25.474000DB->x18.780191
	{0x30, 0x12e}, //index: 504, gain:25.521000DB->x18.882087
	{0x30, 0x12f}, //index: 505, gain:25.568000DB->x18.984537
	{0x30, 0x131}, //index: 506, gain:25.615000DB->x19.087542
	{0x30, 0x133}, //index: 507, gain:25.662000DB->x19.191106
	{0x30, 0x134}, //index: 508, gain:25.709000DB->x19.295232
	{0x30, 0x136}, //index: 509, gain:25.756000DB->x19.399923
	{0x30, 0x138}, //index: 510, gain:25.803000DB->x19.505182
	{0x30, 0x139}, //index: 511, gain:25.850000DB->x19.611012
	{0x30, 0x13b}, //index: 512, gain:25.897000DB->x19.717416
	{0x30, 0x13d}, //index: 513, gain:25.944000DB->x19.824398
	{0x30, 0x13e}, //index: 514, gain:25.991000DB->x19.931960
	{0x30, 0x140}, //index: 515, gain:26.038000DB->x20.040105
	{0x30, 0x142}, //index: 516, gain:26.085000DB->x20.148838
	{0x30, 0x144}, //index: 517, gain:26.132000DB->x20.258160
	{0x30, 0x145}, //index: 518, gain:26.179000DB->x20.368076
	{0x30, 0x147}, //index: 519, gain:26.226000DB->x20.478588
	{0x30, 0x149}, //index: 520, gain:26.273000DB->x20.589699
	{0x30, 0x14b}, //index: 521, gain:26.320000DB->x20.701413
	{0x30, 0x14d}, //index: 522, gain:26.367000DB->x20.813734
	{0x30, 0x14e}, //index: 523, gain:26.414000DB->x20.926664
	{0x30, 0x150}, //index: 524, gain:26.461000DB->x21.040207
	{0x30, 0x152}, //index: 525, gain:26.508000DB->x21.154365
	{0x30, 0x154}, //index: 526, gain:26.555000DB->x21.269143
	{0x30, 0x156}, //index: 527, gain:26.602000DB->x21.384544
	{0x30, 0x158}, //index: 528, gain:26.649000DB->x21.500571
	{0x30, 0x159}, //index: 529, gain:26.696000DB->x21.617228
	{0x30, 0x15b}, //index: 530, gain:26.743000DB->x21.734517
	{0x30, 0x15d}, //index: 531, gain:26.790000DB->x21.852443
	{0x30, 0x15f}, //index: 532, gain:26.837000DB->x21.971009
	{0x30, 0x161}, //index: 533, gain:26.884000DB->x22.090218
	{0x30, 0x163}, //index: 534, gain:26.931000DB->x22.210074
	{0x30, 0x165}, //index: 535, gain:26.978000DB->x22.330580
	{0x30, 0x167}, //index: 536, gain:27.025000DB->x22.451740
	{0x30, 0x169}, //index: 537, gain:27.072000DB->x22.573557
	{0x30, 0x16b}, //index: 538, gain:27.119000DB->x22.696035
	{0x30, 0x16d}, //index: 539, gain:27.166000DB->x22.819178
	{0x30, 0x16f}, //index: 540, gain:27.213000DB->x22.942989
	{0x30, 0x171}, //index: 541, gain:27.260000DB->x23.067472
	{0x30, 0x173}, //index: 542, gain:27.307000DB->x23.192630
	{0x30, 0x175}, //index: 543, gain:27.354000DB->x23.318467
	{0x30, 0x177}, //index: 544, gain:27.401000DB->x23.444987
	{0x30, 0x179}, //index: 545, gain:27.448000DB->x23.572194
	{0x30, 0x17b}, //index: 546, gain:27.495000DB->x23.700090
	{0x30, 0x17d}, //index: 547, gain:27.542000DB->x23.828681
	{0x30, 0x17f}, //index: 548, gain:27.589000DB->x23.957969
	{0x30, 0x181}, //index: 549, gain:27.636000DB->x24.087959
	{0x30, 0x183}, //index: 550, gain:27.683000DB->x24.218654
	{0x30, 0x185}, //index: 551, gain:27.730000DB->x24.350058
	{0x30, 0x187}, //index: 552, gain:27.777000DB->x24.482175
	{0x30, 0x189}, //index: 553, gain:27.824000DB->x24.615009
	{0x30, 0x18b}, //index: 554, gain:27.871000DB->x24.748564
	{0x30, 0x18e}, //index: 555, gain:27.918000DB->x24.882843
	{0x30, 0x190}, //index: 556, gain:27.965000DB->x25.017851
	{0x30, 0x192}, //index: 557, gain:28.012000DB->x25.153591
	{0x30, 0x194}, //index: 558, gain:28.059000DB->x25.290068
	{0x30, 0x196}, //index: 559, gain:28.106000DB->x25.427286
	{0x30, 0x199}, //index: 560, gain:28.153000DB->x25.565247
	{0x30, 0x19b}, //index: 561, gain:28.200000DB->x25.703958
	{0x30, 0x19d}, //index: 562, gain:28.247000DB->x25.843421
	{0x30, 0x19f}, //index: 563, gain:28.294000DB->x25.983641
	{0x30, 0x1a1}, //index: 564, gain:28.341000DB->x26.124621
	{0x30, 0x1a4}, //index: 565, gain:28.388000DB->x26.266367
	{0x30, 0x1a6}, //index: 566, gain:28.435000DB->x26.408881
	{0x30, 0x1a8}, //index: 567, gain:28.482000DB->x26.552169
	{0x30, 0x1ab}, //index: 568, gain:28.529000DB->x26.696234
	{0x30, 0x1ad}, //index: 569, gain:28.576000DB->x26.841081
	{0x30, 0x1af}, //index: 570, gain:28.623000DB->x26.986714
	{0x30, 0x1b2}, //index: 571, gain:28.670000DB->x27.133137
	{0x30, 0x1b4}, //index: 572, gain:28.717000DB->x27.280354
	{0x30, 0x1b6}, //index: 573, gain:28.764000DB->x27.428370
	{0x30, 0x1b9}, //index: 574, gain:28.811000DB->x27.577189
	{0x30, 0x1bb}, //index: 575, gain:28.858000DB->x27.726816
	{0x30, 0x1be}, //index: 576, gain:28.905000DB->x27.877255
	{0x30, 0x1c0}, //index: 577, gain:28.952000DB->x28.028509
	{0x30, 0x1c2}, //index: 578, gain:28.999000DB->x28.180585
	{0x30, 0x1c5}, //index: 579, gain:29.046000DB->x28.333485
	{0x30, 0x1c7}, //index: 580, gain:29.093000DB->x28.487215
	{0x30, 0x1ca}, //index: 581, gain:29.140000DB->x28.641780
	{0x30, 0x1cc}, //index: 582, gain:29.187000DB->x28.797183
	{0x30, 0x1cf}, //index: 583, gain:29.234000DB->x28.953429
	{0x30, 0x1d1}, //index: 584, gain:29.281000DB->x29.110522
	{0x30, 0x1d4}, //index: 585, gain:29.328000DB->x29.268469
	{0x30, 0x1d6}, //index: 586, gain:29.375000DB->x29.427272
	{0x30, 0x1d9}, //index: 587, gain:29.422000DB->x29.586937
	{0x30, 0x1db}, //index: 588, gain:29.469000DB->x29.747468
	{0x30, 0x1de}, //index: 589, gain:29.516000DB->x29.908870
	{0x30, 0x1e1}, //index: 590, gain:29.563000DB->x30.071147
	{0x30, 0x1e3}, //index: 591, gain:29.610000DB->x30.234306
	{0x30, 0x1e6}, //index: 592, gain:29.657000DB->x30.398349
	{0x30, 0x1e9}, //index: 593, gain:29.704000DB->x30.563283
	{0x30, 0x1eb}, //index: 594, gain:29.751000DB->x30.729111
	{0x30, 0x1ee}, //index: 595, gain:29.798000DB->x30.895839
	{0x30, 0x1f1}, //index: 596, gain:29.845000DB->x31.063472
	{0x30, 0x1f3}, //index: 597, gain:29.892000DB->x31.232015
	{0x30, 0x1f6}, //index: 598, gain:29.939000DB->x31.401472
	{0x30, 0x1f9}, //index: 599, gain:29.986000DB->x31.571848
	{0x30, 0x1fb}, //index: 600, gain:30.033000DB->x31.743148
	{0x30, 0x1fe}, //index: 601, gain:30.080000DB->x31.915379
	{0x30, 0x201}, //index: 602, gain:30.127000DB->x32.088543
	{0x30, 0x204}, //index: 603, gain:30.174000DB->x32.262647
	{0x30, 0x207}, //index: 604, gain:30.221000DB->x32.437696
	{0x30, 0x209}, //index: 605, gain:30.268000DB->x32.613695
	{0x30, 0x20c}, //index: 606, gain:30.315000DB->x32.790648
	{0x30, 0x20f}, //index: 607, gain:30.362000DB->x32.968562
	{0x30, 0x212}, //index: 608, gain:30.409000DB->x33.147441
	{0x30, 0x215}, //index: 609, gain:30.456000DB->x33.327290
	{0x30, 0x218}, //index: 610, gain:30.503000DB->x33.508115
	{0x30, 0x21b}, //index: 611, gain:30.550000DB->x33.689922
	{0x30, 0x21d}, //index: 612, gain:30.597000DB->x33.872714
	{0x30, 0x220}, //index: 613, gain:30.644000DB->x34.056499
	{0x30, 0x223}, //index: 614, gain:30.691000DB->x34.241281
	{0x30, 0x226}, //index: 615, gain:30.738000DB->x34.427065
	{0x30, 0x229}, //index: 616, gain:30.785000DB->x34.613857
	{0x30, 0x22c}, //index: 617, gain:30.832000DB->x34.801663
	{0x30, 0x22f}, //index: 618, gain:30.879000DB->x34.990488
	{0x30, 0x232}, //index: 619, gain:30.926000DB->x35.180337
	{0x30, 0x235}, //index: 620, gain:30.973000DB->x35.371217
	{0x30, 0x239}, //index: 621, gain:31.020000DB->x35.563132
	{0x30, 0x23c}, //index: 622, gain:31.067000DB->x35.756088
	{0x30, 0x23f}, //index: 623, gain:31.114000DB->x35.950091
	{0x30, 0x242}, //index: 624, gain:31.161000DB->x36.145147
	{0x30, 0x245}, //index: 625, gain:31.208000DB->x36.341262
	{0x30, 0x248}, //index: 626, gain:31.255000DB->x36.538440
	{0x30, 0x24b}, //index: 627, gain:31.302000DB->x36.736688
	{0x30, 0x24e}, //index: 628, gain:31.349000DB->x36.936012
	{0x30, 0x252}, //index: 629, gain:31.396000DB->x37.136417
	{0x30, 0x255}, //index: 630, gain:31.443000DB->x37.337910
	{0x30, 0x258}, //index: 631, gain:31.490000DB->x37.540495
	{0x30, 0x25b}, //index: 632, gain:31.537000DB->x37.744180
	{0x30, 0x25f}, //index: 633, gain:31.584000DB->x37.948971
	{0x30, 0x262}, //index: 634, gain:31.631000DB->x38.154872
	{0x30, 0x265}, //index: 635, gain:31.678000DB->x38.361890
	{0x30, 0x269}, //index: 636, gain:31.725000DB->x38.570032
	{0x30, 0x26c}, //index: 637, gain:31.772000DB->x38.779303
	{0x30, 0x26f}, //index: 638, gain:31.819000DB->x38.989710
	{0x30, 0x273}, //index: 639, gain:31.866000DB->x39.201258
	{0x30, 0x276}, //index: 640, gain:31.913000DB->x39.413954
	{0x30, 0x27a}, //index: 641, gain:31.960000DB->x39.627803
	{0x30, 0x27d}, //index: 642, gain:32.007000DB->x39.842814
	{0x30, 0x280}, //index: 643, gain:32.054000DB->x40.058990
	{0x30, 0x284}, //index: 644, gain:32.101000DB->x40.276340
	{0x30, 0x287}, //index: 645, gain:32.148000DB->x40.494869
	{0x30, 0x28b}, //index: 646, gain:32.195000DB->x40.714584
	{0x30, 0x28e}, //index: 647, gain:32.242000DB->x40.935491
	{0x30, 0x292}, //index: 648, gain:32.289000DB->x41.157596
	{0x30, 0x296}, //index: 649, gain:32.336000DB->x41.380906
	{0x30, 0x299}, //index: 650, gain:32.383000DB->x41.605429
	{0x30, 0x29d}, //index: 651, gain:32.430000DB->x41.831169
	{0x30, 0x2a0}, //index: 652, gain:32.477000DB->x42.058134
	{0x30, 0x2a4}, //index: 653, gain:32.524000DB->x42.286331
	{0x30, 0x2a8}, //index: 654, gain:32.571000DB->x42.515765
	{0x30, 0x2ab}, //index: 655, gain:32.618000DB->x42.746445
	{0x30, 0x2af}, //index: 656, gain:32.665000DB->x42.978376
	{0x30, 0x2b3}, //index: 657, gain:32.712000DB->x43.211565
	{0x30, 0x2b7}, //index: 658, gain:32.759000DB->x43.446020
	{0x30, 0x2ba}, //index: 659, gain:32.806000DB->x43.681747
	{0x30, 0x2be}, //index: 660, gain:32.853000DB->x43.918753
	{0x30, 0x2c2}, //index: 661, gain:32.900000DB->x44.157045
	{0x30, 0x2c6}, //index: 662, gain:32.947000DB->x44.396629
	{0x30, 0x2ca}, //index: 663, gain:32.994000DB->x44.637514
	{0x30, 0x2ce}, //index: 664, gain:33.041000DB->x44.879706
	{0x30, 0x2d1}, //index: 665, gain:33.088000DB->x45.123211
	{0x30, 0x2d5}, //index: 666, gain:33.135000DB->x45.368038
	{0x30, 0x2d9}, //index: 667, gain:33.182000DB->x45.614193
	{0x30, 0x2dd}, //index: 668, gain:33.229000DB->x45.861684
	{0x30, 0x2e1}, //index: 669, gain:33.276000DB->x46.110518
	{0x30, 0x2e5}, //index: 670, gain:33.323000DB->x46.360702
	{0x30, 0x2e9}, //index: 671, gain:33.370000DB->x46.612243
	{0x30, 0x2ed}, //index: 672, gain:33.417000DB->x46.865149
	{0x30, 0x2f1}, //index: 673, gain:33.464000DB->x47.119427
	{0x30, 0x2f6}, //index: 674, gain:33.511000DB->x47.375085
	{0x30, 0x2fa}, //index: 675, gain:33.558000DB->x47.632130
	{0x30, 0x2fe}, //index: 676, gain:33.605000DB->x47.890569
	{0x30, 0x302}, //index: 677, gain:33.652000DB->x48.150411
	{0x30, 0x306}, //index: 678, gain:33.699000DB->x48.411663
	{0x30, 0x30a}, //index: 679, gain:33.746000DB->x48.674332
	{0x30, 0x30f}, //index: 680, gain:33.793000DB->x48.938426
	{0x30, 0x313}, //index: 681, gain:33.840000DB->x49.203954
	{0x30, 0x317}, //index: 682, gain:33.887000DB->x49.470921
	{0x30, 0x31b}, //index: 683, gain:33.934000DB->x49.739338
	{0x30, 0x320}, //index: 684, gain:33.981000DB->x50.009211
	{0x30, 0x324}, //index: 685, gain:34.028000DB->x50.280548
	{0x30, 0x328}, //index: 686, gain:34.075000DB->x50.553357
	{0x30, 0x32d}, //index: 687, gain:34.122000DB->x50.827646
	{0x30, 0x331}, //index: 688, gain:34.169000DB->x51.103424
	{0x30, 0x336}, //index: 689, gain:34.216000DB->x51.380698
	{0x30, 0x33a}, //index: 690, gain:34.263000DB->x51.659476
	{0x30, 0x33f}, //index: 691, gain:34.310000DB->x51.939767
	{0x30, 0x343}, //index: 692, gain:34.357000DB->x52.221579
	{0x30, 0x348}, //index: 693, gain:34.404000DB->x52.504920
	{0x30, 0x34c}, //index: 694, gain:34.451000DB->x52.789798
	{0x30, 0x351}, //index: 695, gain:34.498000DB->x53.076222
	{0x30, 0x355}, //index: 696, gain:34.545000DB->x53.364200
	{0x30, 0x35a}, //index: 697, gain:34.592000DB->x53.653740
	{0x30, 0x35f}, //index: 698, gain:34.639000DB->x53.944851
	{0x30, 0x363}, //index: 699, gain:34.686000DB->x54.237542
	{0x30, 0x368}, //index: 700, gain:34.733000DB->x54.531821
	{0x30, 0x36d}, //index: 701, gain:34.780000DB->x54.827696
	{0x30, 0x372}, //index: 702, gain:34.827000DB->x55.125177
	{0x30, 0x376}, //index: 703, gain:34.874000DB->x55.424272
	{0x30, 0x37b}, //index: 704, gain:34.921000DB->x55.724990
	{0x30, 0x380}, //index: 705, gain:34.968000DB->x56.027339
	{0x30, 0x385}, //index: 706, gain:35.015000DB->x56.331329
	{0x30, 0x38a}, //index: 707, gain:35.062000DB->x56.636969
	{0x30, 0x38f}, //index: 708, gain:35.109000DB->x56.944266
	{0x30, 0x394}, //index: 709, gain:35.156000DB->x57.253231
	{0x30, 0x399}, //index: 710, gain:35.203000DB->x57.563872
	{0x30, 0x39e}, //index: 711, gain:35.250000DB->x57.876199
	{0x30, 0x3a3}, //index: 712, gain:35.297000DB->x58.190220
	{0x30, 0x3a8}, //index: 713, gain:35.344000DB->x58.505945
	{0x30, 0x3ad}, //index: 714, gain:35.391000DB->x58.823383
	{0x30, 0x3b2}, //index: 715, gain:35.438000DB->x59.142544
	{0x30, 0x3b7}, //index: 716, gain:35.485000DB->x59.463436
	{0x30, 0x3bc}, //index: 717, gain:35.532000DB->x59.786069
	{0x30, 0x3c1}, //index: 718, gain:35.579000DB->x60.110453
	{0x30, 0x3c6}, //index: 719, gain:35.626000DB->x60.436597
	{0x30, 0x3cc}, //index: 720, gain:35.673000DB->x60.764510
	{0x30, 0x3d1}, //index: 721, gain:35.720000DB->x61.094202
	{0x30, 0x3d6}, //index: 722, gain:35.767000DB->x61.425684
	{0x30, 0x3dc}, //index: 723, gain:35.814000DB->x61.758964
	{0x30, 0x3e1}, //index: 724, gain:35.861000DB->x62.094052
	{0x30, 0x3e6}, //index: 725, gain:35.908000DB->x62.430958
	{0x30, 0x3ec}, //index: 726, gain:35.955000DB->x62.769692
	{0x30, 0x3f1}, //index: 727, gain:36.002000DB->x63.110264
	{0x30, 0x3f7}, //index: 728, gain:36.049000DB->x63.452684
	{0x30, 0x3fc}, //index: 729, gain:36.096000DB->x63.796962
	{0x30, 0x402}, //index: 730, gain:36.143000DB->x64.143108
	{0x30, 0x407}, //index: 731, gain:36.190000DB->x64.491132
	{0x30, 0x40d}, //index: 732, gain:36.237000DB->x64.841044
	{0x30, 0x413}, //index: 733, gain:36.284000DB->x65.192855
	{0x30, 0x418}, //index: 734, gain:36.331000DB->x65.546574
	{0x30, 0x41e}, //index: 735, gain:36.378000DB->x65.902213
	{0x30, 0x424}, //index: 736, gain:36.425000DB->x66.259782
	{0x30, 0x429}, //index: 737, gain:36.472000DB->x66.619290
	{0x30, 0x42f}, //index: 738, gain:36.519000DB->x66.980749
	{0x30, 0x435}, //index: 739, gain:36.566000DB->x67.344169
	{0x30, 0x43b}, //index: 740, gain:36.613000DB->x67.709561
	{0x30, 0x441}, //index: 741, gain:36.660000DB->x68.076936
	{0x30, 0x447}, //index: 742, gain:36.707000DB->x68.446304
	{0x30, 0x44d}, //index: 743, gain:36.754000DB->x68.817676
	{0x30, 0x453}, //index: 744, gain:36.801000DB->x69.191063
	{0x30, 0x459}, //index: 745, gain:36.848000DB->x69.566475
	{0x30, 0x45f}, //index: 746, gain:36.895000DB->x69.943925
	{0x30, 0x465}, //index: 747, gain:36.942000DB->x70.323423
	{0x30, 0x46b}, //index: 748, gain:36.989000DB->x70.704979
	{0x30, 0x471}, //index: 749, gain:37.036000DB->x71.088606
	{0x30, 0x477}, //index: 750, gain:37.083000DB->x71.474315
	{0x30, 0x47d}, //index: 751, gain:37.130000DB->x71.862116
	{0x30, 0x484}, //index: 752, gain:37.177000DB->x72.252021
	{0x30, 0x48a}, //index: 753, gain:37.224000DB->x72.644042
	{0x30, 0x490}, //index: 754, gain:37.271000DB->x73.038190
	{0x30, 0x496}, //index: 755, gain:37.318000DB->x73.434476
	{0x30, 0x49d}, //index: 756, gain:37.365000DB->x73.832912
	{0x30, 0x4a3}, //index: 757, gain:37.412000DB->x74.233511
	{0x30, 0x4aa}, //index: 758, gain:37.459000DB->x74.636283
	{0x30, 0x4b0}, //index: 759, gain:37.506000DB->x75.041240
	{0x30, 0x4b7}, //index: 760, gain:37.553000DB->x75.448394
	{0x30, 0x4bd}, //index: 761, gain:37.600000DB->x75.857758
	{0x30, 0x4c4}, //index: 762, gain:37.647000DB->x76.269342
	{0x30, 0x4ca}, //index: 763, gain:37.694000DB->x76.683160
	{0x30, 0x4d1}, //index: 764, gain:37.741000DB->x77.099223
	{0x30, 0x4d8}, //index: 765, gain:37.788000DB->x77.517543
	{0x30, 0x4df}, //index: 766, gain:37.835000DB->x77.938133
	{0x30, 0x4e5}, //index: 767, gain:37.882000DB->x78.361005
	{0x30, 0x4ec}, //index: 768, gain:37.929000DB->x78.786172
	{0x30, 0x4f3}, //index: 769, gain:37.976000DB->x79.213645
	{0x30, 0x4fa}, //index: 770, gain:38.023000DB->x79.643438
	{0x30, 0x501}, //index: 771, gain:38.070000DB->x80.075563
	{0x30, 0x508}, //index: 772, gain:38.117000DB->x80.510032
	{0x30, 0x50f}, //index: 773, gain:38.164000DB->x80.946859
	{0x30, 0x516}, //index: 774, gain:38.211000DB->x81.386055
	{0x30, 0x51d}, //index: 775, gain:38.258000DB->x81.827635
	{0x30, 0x524}, //index: 776, gain:38.305000DB->x82.271611
	{0x30, 0x52b}, //index: 777, gain:38.352000DB->x82.717995
	{0x30, 0x532}, //index: 778, gain:38.399000DB->x83.166802
	{0x30, 0x539}, //index: 779, gain:38.446000DB->x83.618043
	{0x30, 0x541}, //index: 780, gain:38.493000DB->x84.071733
	{0x30, 0x548}, //index: 781, gain:38.540000DB->x84.527885
	{0x30, 0x54f}, //index: 782, gain:38.587000DB->x84.986511
	{0x30, 0x557}, //index: 783, gain:38.634000DB->x85.447626
	{0x30, 0x55e}, //index: 784, gain:38.681000DB->x85.911242
	{0x30, 0x566}, //index: 785, gain:38.728000DB->x86.377375
	{0x30, 0x56d}, //index: 786, gain:38.775000DB->x86.846036
	{0x30, 0x575}, //index: 787, gain:38.822000DB->x87.317240
	{0x30, 0x57c}, //index: 788, gain:38.869000DB->x87.791001
	{0x30, 0x584}, //index: 789, gain:38.916000DB->x88.267332
	{0x30, 0x58b}, //index: 790, gain:38.963000DB->x88.746248
	{0x30, 0x593}, //index: 791, gain:39.010000DB->x89.227762
	{0x30, 0x59b}, //index: 792, gain:39.057000DB->x89.711889
	{0x30, 0x5a3}, //index: 793, gain:39.104000DB->x90.198642
	{0x30, 0x5ab}, //index: 794, gain:39.151000DB->x90.688037
	{0x30, 0x5b2}, //index: 795, gain:39.198000DB->x91.180087
	{0x30, 0x5ba}, //index: 796, gain:39.245000DB->x91.674806
	{0x30, 0x5c2}, //index: 797, gain:39.292000DB->x92.172210
	{0x30, 0x5ca}, //index: 798, gain:39.339000DB->x92.672312
	{0x30, 0x5d2}, //index: 799, gain:39.386000DB->x93.175128
	{0x30, 0x5da}, //index: 800, gain:39.433000DB->x93.680673
	{0x30, 0x5e3}, //index: 801, gain:39.480000DB->x94.188960
	{0x30, 0x5eb}, //index: 802, gain:39.527000DB->x94.700005
	{0x30, 0x5f3}, //index: 803, gain:39.574000DB->x95.213822
	{0x30, 0x5fb}, //index: 804, gain:39.621000DB->x95.730428
	{0x30, 0x603}, //index: 805, gain:39.668000DB->x96.249836
	{0x30, 0x60c}, //index: 806, gain:39.715000DB->x96.772063
	{0x30, 0x614}, //index: 807, gain:39.762000DB->x97.297123
	{0x30, 0x61d}, //index: 808, gain:39.809000DB->x97.825032
	{0x30, 0x625}, //index: 809, gain:39.856000DB->x98.355806
	{0x30, 0x62e}, //index: 810, gain:39.903000DB->x98.889459
	{0x30, 0x636}, //index: 811, gain:39.950000DB->x99.426007
	{0x30, 0x63f}, //index: 812, gain:39.997000DB->x99.965467
	{0x30, 0x648}, //index: 813, gain:40.044000DB->x100.507854
	{0x30, 0x650}, //index: 814, gain:40.091000DB->x101.053184
	{0x30, 0x659}, //index: 815, gain:40.138000DB->x101.601472
	{0x30, 0x662}, //index: 816, gain:40.185000DB->x102.152735
	{0x30, 0x66b}, //index: 817, gain:40.232000DB->x102.706990
	{0x30, 0x674}, //index: 818, gain:40.279000DB->x103.264251
	{0x30, 0x67d}, //index: 819, gain:40.326000DB->x103.824536
	{0x30, 0x686}, //index: 820, gain:40.373000DB->x104.387861
	{0x30, 0x68f}, //index: 821, gain:40.420000DB->x104.954243
	{0x30, 0x698}, //index: 822, gain:40.467000DB->x105.523697
	{0x30, 0x6a1}, //index: 823, gain:40.514000DB->x106.096242
	{0x30, 0x6aa}, //index: 824, gain:40.561000DB->x106.671892
	{0x30, 0x6b4}, //index: 825, gain:40.608000DB->x107.250667
	{0x30, 0x6bd}, //index: 826, gain:40.655000DB->x107.832581
	{0x30, 0x6c6}, //index: 827, gain:40.702000DB->x108.417653
	{0x30, 0x6d0}, //index: 828, gain:40.749000DB->x109.005899
	{0x30, 0x6d9}, //index: 829, gain:40.796000DB->x109.597337
	{0x30, 0x6e3}, //index: 830, gain:40.843000DB->x110.191983
	{0x30, 0x6ec}, //index: 831, gain:40.890000DB->x110.789857
	{0x30, 0x6f6}, //index: 832, gain:40.937000DB->x111.390974
	{0x30, 0x6ff}, //index: 833, gain:40.984000DB->x111.995352
	{0x30, 0x709}, //index: 834, gain:41.031000DB->x112.603010
	{0x30, 0x713}, //index: 835, gain:41.078000DB->x113.213965
	{0x30, 0x71d}, //index: 836, gain:41.125000DB->x113.828235
	{0x30, 0x727}, //index: 837, gain:41.172000DB->x114.445837
	{0x30, 0x731}, //index: 838, gain:41.219000DB->x115.066791
	{0x30, 0x73b}, //index: 839, gain:41.266000DB->x115.691113
	{0x30, 0x745}, //index: 840, gain:41.313000DB->x116.318823
	{0x30, 0x74f}, //index: 841, gain:41.360000DB->x116.949939
	{0x30, 0x759}, //index: 842, gain:41.407000DB->x117.584479
	{0x30, 0x763}, //index: 843, gain:41.454000DB->x118.222462
	{0x30, 0x76d}, //index: 844, gain:41.501000DB->x118.863907
	{0x30, 0x778}, //index: 845, gain:41.548000DB->x119.508831
	{0x30, 0x782}, //index: 846, gain:41.595000DB->x120.157255
	{0x30, 0x78c}, //index: 847, gain:41.642000DB->x120.809198
	{0x30, 0x797}, //index: 848, gain:41.689000DB->x121.464677
	{0x30, 0x7a1}, //index: 849, gain:41.736000DB->x122.123713
	{0x30, 0x7ac}, //index: 850, gain:41.783000DB->x122.786325
	{0x30, 0x7b7}, //index: 851, gain:41.830000DB->x123.452532
	{0x30, 0x7c1}, //index: 852, gain:41.877000DB->x124.122353
	{0x30, 0x7cc}, //index: 853, gain:41.924000DB->x124.795809
	{0x30, 0x7d7}, //index: 854, gain:41.971000DB->x125.472919
	{0x30, 0x7e2}, //index: 855, gain:42.018000DB->x126.153702
	{0x30, 0x7ed}, //index: 856, gain:42.065000DB->x126.838180
	{0x30, 0x7f8}, //index: 857, gain:42.112000DB->x127.526371
};

/** AR0331 hdr gain table row size */
#define AR0331_HDR_GAIN_ROWS		(473)
#define AR0331_HDR_GAIN_COLS		(3)
#define AR0331_HDR_GAIN_24DB		(472)
#define AR0331_HDR_GAIN_COL_REG_MOTION	(2)

const s16 AR0331_HDR_GAIN_TABLE[AR0331_HDR_GAIN_ROWS][AR0331_HDR_GAIN_COLS] =
{
	{0x06, 0x80, 0x201e}, //index: 0, gain:1.833000DB->x1.234952, motion_detect_q:0x201e
	{0x06, 0x81, 0x201e}, //index: 1, gain:1.880000DB->x1.241652, motion_detect_q:0x201e
	{0x06, 0x81, 0x201e}, //index: 2, gain:1.927000DB->x1.248389, motion_detect_q:0x201e
	{0x06, 0x82, 0x201e}, //index: 3, gain:1.974000DB->x1.255163, motion_detect_q:0x201e
	{0x06, 0x83, 0x201e}, //index: 4, gain:2.021000DB->x1.261973, motion_detect_q:0x201e
	{0x06, 0x84, 0x201e}, //index: 5, gain:2.068000DB->x1.268820, motion_detect_q:0x201e
	{0x06, 0x84, 0x201e}, //index: 6, gain:2.115000DB->x1.275704, motion_detect_q:0x201e
	{0x07, 0x80, 0x203a}, //index: 7, gain:2.162000DB->x1.282626, motion_detect_q:0x203a
	{0x07, 0x80, 0x203a}, //index: 8, gain:2.209000DB->x1.289585, motion_detect_q:0x203a
	{0x07, 0x81, 0x203a}, //index: 9, gain:2.256000DB->x1.296582, motion_detect_q:0x203a
	{0x07, 0x82, 0x203a}, //index: 10, gain:2.303000DB->x1.303617, motion_detect_q:0x203a
	{0x07, 0x83, 0x203a}, //index: 11, gain:2.350000DB->x1.310690, motion_detect_q:0x203a
	{0x07, 0x83, 0x203a}, //index: 12, gain:2.397000DB->x1.317802, motion_detect_q:0x203a
	{0x07, 0x84, 0x203a}, //index: 13, gain:2.444000DB->x1.324952, motion_detect_q:0x203a
	{0x07, 0x85, 0x203a}, //index: 14, gain:2.491000DB->x1.332140, motion_detect_q:0x203a
	{0x07, 0x85, 0x203a}, //index: 15, gain:2.538000DB->x1.339368, motion_detect_q:0x203a
	{0x08, 0x80, 0x203b}, //index: 16, gain:2.585000DB->x1.346635, motion_detect_q:0x203b
	{0x08, 0x81, 0x203b}, //index: 17, gain:2.632000DB->x1.353942, motion_detect_q:0x203b
	{0x08, 0x82, 0x203b}, //index: 18, gain:2.679000DB->x1.361288, motion_detect_q:0x203b
	{0x08, 0x82, 0x203b}, //index: 19, gain:2.726000DB->x1.368674, motion_detect_q:0x203b
	{0x08, 0x83, 0x203b}, //index: 20, gain:2.773000DB->x1.376100, motion_detect_q:0x203b
	{0x08, 0x84, 0x203b}, //index: 21, gain:2.820000DB->x1.383566, motion_detect_q:0x203b
	{0x09, 0x80, 0x203d}, //index: 22, gain:2.867000DB->x1.391073, motion_detect_q:0x203d
	{0x09, 0x80, 0x203d}, //index: 23, gain:2.914000DB->x1.398621, motion_detect_q:0x203d
	{0x09, 0x81, 0x203d}, //index: 24, gain:2.961000DB->x1.406209, motion_detect_q:0x203d
	{0x09, 0x82, 0x203d}, //index: 25, gain:3.008000DB->x1.413839, motion_detect_q:0x203d
	{0x09, 0x82, 0x203d}, //index: 26, gain:3.055000DB->x1.421510, motion_detect_q:0x203d
	{0x09, 0x83, 0x203d}, //index: 27, gain:3.102000DB->x1.429223, motion_detect_q:0x203d
	{0x09, 0x84, 0x203d}, //index: 28, gain:3.149000DB->x1.436978, motion_detect_q:0x203d
	{0x09, 0x85, 0x203d}, //index: 29, gain:3.196000DB->x1.444774, motion_detect_q:0x203d
	{0x0a, 0x80, 0x203e}, //index: 30, gain:3.243000DB->x1.452613, motion_detect_q:0x203e
	{0x0a, 0x80, 0x203e}, //index: 31, gain:3.290000DB->x1.460495, motion_detect_q:0x203e
	{0x0a, 0x81, 0x203e}, //index: 32, gain:3.337000DB->x1.468419, motion_detect_q:0x203e
	{0x0a, 0x82, 0x203e}, //index: 33, gain:3.384000DB->x1.476386, motion_detect_q:0x203e
	{0x0a, 0x83, 0x203e}, //index: 34, gain:3.431000DB->x1.484397, motion_detect_q:0x203e
	{0x0a, 0x83, 0x203e}, //index: 35, gain:3.478000DB->x1.492451, motion_detect_q:0x203e
	{0x0a, 0x84, 0x203e}, //index: 36, gain:3.525000DB->x1.500548, motion_detect_q:0x203e
	{0x0a, 0x85, 0x203e}, //index: 37, gain:3.572000DB->x1.508690, motion_detect_q:0x203e
	{0x0a, 0x85, 0x203e}, //index: 38, gain:3.619000DB->x1.516876, motion_detect_q:0x203e
	{0x0b, 0x80, 0x203f}, //index: 39, gain:3.666000DB->x1.525106, motion_detect_q:0x203f
	{0x0b, 0x81, 0x203f}, //index: 40, gain:3.713000DB->x1.533381, motion_detect_q:0x203f
	{0x0b, 0x81, 0x203f}, //index: 41, gain:3.760000DB->x1.541700, motion_detect_q:0x203f
	{0x0b, 0x82, 0x203f}, //index: 42, gain:3.807000DB->x1.550065, motion_detect_q:0x203f
	{0x0b, 0x83, 0x203f}, //index: 43, gain:3.854000DB->x1.558476, motion_detect_q:0x203f
	{0x0b, 0x83, 0x203f}, //index: 44, gain:3.901000DB->x1.566931, motion_detect_q:0x203f
	{0x0b, 0x84, 0x203f}, //index: 45, gain:3.948000DB->x1.575433, motion_detect_q:0x203f
	{0x0b, 0x85, 0x203f}, //index: 46, gain:3.995000DB->x1.583981, motion_detect_q:0x203f
	{0x0b, 0x86, 0x203f}, //index: 47, gain:4.042000DB->x1.592575, motion_detect_q:0x203f
	{0x0c, 0x80, 0x2041}, //index: 48, gain:4.089000DB->x1.601216, motion_detect_q:0x2041
	{0x0c, 0x80, 0x2041}, //index: 49, gain:4.136000DB->x1.609904, motion_detect_q:0x2041
	{0x0c, 0x81, 0x2041}, //index: 50, gain:4.183000DB->x1.618639, motion_detect_q:0x2041
	{0x0c, 0x82, 0x2041}, //index: 51, gain:4.230000DB->x1.627421, motion_detect_q:0x2041
	{0x0c, 0x82, 0x2041}, //index: 52, gain:4.277000DB->x1.636251, motion_detect_q:0x2041
	{0x0c, 0x83, 0x2041}, //index: 53, gain:4.324000DB->x1.645129, motion_detect_q:0x2041
	{0x0c, 0x84, 0x2041}, //index: 54, gain:4.371000DB->x1.654055, motion_detect_q:0x2041
	{0x0c, 0x85, 0x2041}, //index: 55, gain:4.418000DB->x1.663030, motion_detect_q:0x2041
	{0x0c, 0x85, 0x2041}, //index: 56, gain:4.465000DB->x1.672053, motion_detect_q:0x2041
	{0x0c, 0x86, 0x2041}, //index: 57, gain:4.512000DB->x1.681125, motion_detect_q:0x2041
	{0x0d, 0x80, 0x2043}, //index: 58, gain:4.559000DB->x1.690246, motion_detect_q:0x2043
	{0x0d, 0x80, 0x2043}, //index: 59, gain:4.606000DB->x1.699417, motion_detect_q:0x2043
	{0x0d, 0x81, 0x2043}, //index: 60, gain:4.653000DB->x1.708638, motion_detect_q:0x2043
	{0x0d, 0x82, 0x2043}, //index: 61, gain:4.700000DB->x1.717908, motion_detect_q:0x2043
	{0x0d, 0x82, 0x2043}, //index: 62, gain:4.747000DB->x1.727229, motion_detect_q:0x2043
	{0x0d, 0x83, 0x2043}, //index: 63, gain:4.794000DB->x1.736601, motion_detect_q:0x2043
	{0x0d, 0x84, 0x2043}, //index: 64, gain:4.841000DB->x1.746023, motion_detect_q:0x2043
	{0x0d, 0x84, 0x2043}, //index: 65, gain:4.888000DB->x1.755497, motion_detect_q:0x2043
	{0x0d, 0x85, 0x2043}, //index: 66, gain:4.935000DB->x1.765022, motion_detect_q:0x2043
	{0x0d, 0x86, 0x2043}, //index: 67, gain:4.982000DB->x1.774598, motion_detect_q:0x2043
	{0x0e, 0x80, 0x2045}, //index: 68, gain:5.029000DB->x1.784227, motion_detect_q:0x2045
	{0x0e, 0x81, 0x2045}, //index: 69, gain:5.076000DB->x1.793907, motion_detect_q:0x2045
	{0x0e, 0x81, 0x2045}, //index: 70, gain:5.123000DB->x1.803641, motion_detect_q:0x2045
	{0x0e, 0x82, 0x2045}, //index: 71, gain:5.170000DB->x1.813427, motion_detect_q:0x2045
	{0x0e, 0x83, 0x2045}, //index: 72, gain:5.217000DB->x1.823266, motion_detect_q:0x2045
	{0x0e, 0x83, 0x2045}, //index: 73, gain:5.264000DB->x1.833158, motion_detect_q:0x2045
	{0x0e, 0x84, 0x2045}, //index: 74, gain:5.311000DB->x1.843105, motion_detect_q:0x2045
	{0x0e, 0x85, 0x2045}, //index: 75, gain:5.358000DB->x1.853105, motion_detect_q:0x2045
	{0x0e, 0x85, 0x2045}, //index: 76, gain:5.405000DB->x1.863159, motion_detect_q:0x2045
	{0x0e, 0x86, 0x2045}, //index: 77, gain:5.452000DB->x1.873268, motion_detect_q:0x2045
	{0x0f, 0x80, 0x2046}, //index: 78, gain:5.499000DB->x1.883432, motion_detect_q:0x2046
	{0x0f, 0x80, 0x2046}, //index: 79, gain:5.546000DB->x1.893651, motion_detect_q:0x2046
	{0x0f, 0x81, 0x2046}, //index: 80, gain:5.593000DB->x1.903926, motion_detect_q:0x2046
	{0x0f, 0x82, 0x2046}, //index: 81, gain:5.640000DB->x1.914256, motion_detect_q:0x2046
	{0x0f, 0x83, 0x2046}, //index: 82, gain:5.687000DB->x1.924642, motion_detect_q:0x2046
	{0x0f, 0x83, 0x2046}, //index: 83, gain:5.734000DB->x1.935085, motion_detect_q:0x2046
	{0x0f, 0x84, 0x2046}, //index: 84, gain:5.781000DB->x1.945584, motion_detect_q:0x2046
	{0x0f, 0x85, 0x2046}, //index: 85, gain:5.828000DB->x1.956140, motion_detect_q:0x2046
	{0x0f, 0x85, 0x2046}, //index: 86, gain:5.875000DB->x1.966754, motion_detect_q:0x2046
	{0x0f, 0x86, 0x2046}, //index: 87, gain:5.922000DB->x1.977425, motion_detect_q:0x2046
	{0x0f, 0x87, 0x2046}, //index: 88, gain:5.969000DB->x1.988154, motion_detect_q:0x2046
	{0x0f, 0x88, 0x2046}, //index: 89, gain:6.016000DB->x1.998941, motion_detect_q:0x2046
	{0x10, 0x80, 0x2049}, //index: 90, gain:6.063000DB->x2.009787, motion_detect_q:0x2049
	{0x10, 0x81, 0x2049}, //index: 91, gain:6.110000DB->x2.020691, motion_detect_q:0x2049
	{0x10, 0x82, 0x2049}, //index: 92, gain:6.157000DB->x2.031655, motion_detect_q:0x2049
	{0x10, 0x82, 0x2049}, //index: 93, gain:6.204000DB->x2.042678, motion_detect_q:0x2049
	{0x10, 0x83, 0x2049}, //index: 94, gain:6.251000DB->x2.053761, motion_detect_q:0x2049
	{0x10, 0x84, 0x2049}, //index: 95, gain:6.298000DB->x2.064905, motion_detect_q:0x2049
	{0x10, 0x84, 0x2049}, //index: 96, gain:6.345000DB->x2.076108, motion_detect_q:0x2049
	{0x10, 0x85, 0x2049}, //index: 97, gain:6.392000DB->x2.087373, motion_detect_q:0x2049
	{0x10, 0x86, 0x2049}, //index: 98, gain:6.439000DB->x2.098698, motion_detect_q:0x2049
	{0x10, 0x87, 0x2049}, //index: 99, gain:6.486000DB->x2.110085, motion_detect_q:0x2049
	{0x10, 0x87, 0x2049}, //index: 100, gain:6.533000DB->x2.121534, motion_detect_q:0x2049
	{0x10, 0x88, 0x2049}, //index: 101, gain:6.580000DB->x2.133045, motion_detect_q:0x2049
	{0x12, 0x80, 0x204b}, //index: 102, gain:6.627000DB->x2.144618, motion_detect_q:0x204b
	{0x12, 0x80, 0x204b}, //index: 103, gain:6.674000DB->x2.156254, motion_detect_q:0x204b
	{0x12, 0x81, 0x204b}, //index: 104, gain:6.721000DB->x2.167954, motion_detect_q:0x204b
	{0x12, 0x82, 0x204b}, //index: 105, gain:6.768000DB->x2.179716, motion_detect_q:0x204b
	{0x12, 0x83, 0x204b}, //index: 106, gain:6.815000DB->x2.191543, motion_detect_q:0x204b
	{0x12, 0x83, 0x204b}, //index: 107, gain:6.862000DB->x2.203434, motion_detect_q:0x204b
	{0x12, 0x84, 0x204b}, //index: 108, gain:6.909000DB->x2.215389, motion_detect_q:0x204b
	{0x12, 0x85, 0x204b}, //index: 109, gain:6.956000DB->x2.227409, motion_detect_q:0x204b
	{0x12, 0x85, 0x204b}, //index: 110, gain:7.003000DB->x2.239494, motion_detect_q:0x204b
	{0x12, 0x86, 0x204b}, //index: 111, gain:7.050000DB->x2.251645, motion_detect_q:0x204b
	{0x12, 0x87, 0x204b}, //index: 112, gain:7.097000DB->x2.263862, motion_detect_q:0x204b
	{0x12, 0x88, 0x204b}, //index: 113, gain:7.144000DB->x2.276145, motion_detect_q:0x204b
	{0x14, 0x80, 0x204e}, //index: 114, gain:7.191000DB->x2.288495, motion_detect_q:0x204e
	{0x14, 0x81, 0x204e}, //index: 115, gain:7.238000DB->x2.300912, motion_detect_q:0x204e
	{0x14, 0x81, 0x204e}, //index: 116, gain:7.285000DB->x2.313396, motion_detect_q:0x204e
	{0x14, 0x82, 0x204e}, //index: 117, gain:7.332000DB->x2.325948, motion_detect_q:0x204e
	{0x14, 0x83, 0x204e}, //index: 118, gain:7.379000DB->x2.338568, motion_detect_q:0x204e
	{0x14, 0x84, 0x204e}, //index: 119, gain:7.426000DB->x2.351256, motion_detect_q:0x204e
	{0x14, 0x84, 0x204e}, //index: 120, gain:7.473000DB->x2.364014, motion_detect_q:0x204e
	{0x14, 0x85, 0x204e}, //index: 121, gain:7.520000DB->x2.376840, motion_detect_q:0x204e
	{0x14, 0x86, 0x204e}, //index: 122, gain:7.567000DB->x2.389736, motion_detect_q:0x204e
	{0x14, 0x86, 0x204e}, //index: 123, gain:7.614000DB->x2.402702, motion_detect_q:0x204e
	{0x14, 0x87, 0x204e}, //index: 124, gain:7.661000DB->x2.415739, motion_detect_q:0x204e
	{0x14, 0x88, 0x204e}, //index: 125, gain:7.708000DB->x2.428846, motion_detect_q:0x204e
	{0x14, 0x89, 0x204e}, //index: 126, gain:7.755000DB->x2.442024, motion_detect_q:0x204e
	{0x14, 0x89, 0x204e}, //index: 127, gain:7.802000DB->x2.455274, motion_detect_q:0x204e
	{0x14, 0x8a, 0x204e}, //index: 128, gain:7.849000DB->x2.468596, motion_detect_q:0x204e
	{0x16, 0x80, 0x2051}, //index: 129, gain:7.896000DB->x2.481990, motion_detect_q:0x2051
	{0x16, 0x81, 0x2051}, //index: 130, gain:7.943000DB->x2.495456, motion_detect_q:0x2051
	{0x16, 0x82, 0x2051}, //index: 131, gain:7.990000DB->x2.508996, motion_detect_q:0x2051
	{0x16, 0x82, 0x2051}, //index: 132, gain:8.037000DB->x2.522609, motion_detect_q:0x2051
	{0x16, 0x83, 0x2051}, //index: 133, gain:8.084000DB->x2.536296, motion_detect_q:0x2051
	{0x16, 0x84, 0x2051}, //index: 134, gain:8.131000DB->x2.550058, motion_detect_q:0x2051
	{0x16, 0x84, 0x2051}, //index: 135, gain:8.178000DB->x2.563894, motion_detect_q:0x2051
	{0x16, 0x85, 0x2051}, //index: 136, gain:8.225000DB->x2.577805, motion_detect_q:0x2051
	{0x16, 0x86, 0x2051}, //index: 137, gain:8.272000DB->x2.591791, motion_detect_q:0x2051
	{0x16, 0x87, 0x2051}, //index: 138, gain:8.319000DB->x2.605854, motion_detect_q:0x2051
	{0x16, 0x87, 0x2051}, //index: 139, gain:8.366000DB->x2.619992, motion_detect_q:0x2051
	{0x16, 0x88, 0x2051}, //index: 140, gain:8.413000DB->x2.634208, motion_detect_q:0x2051
	{0x16, 0x89, 0x2051}, //index: 141, gain:8.460000DB->x2.648500, motion_detect_q:0x2051
	{0x16, 0x89, 0x2051}, //index: 142, gain:8.507000DB->x2.662870, motion_detect_q:0x2051
	{0x18, 0x80, 0x2054}, //index: 143, gain:8.554000DB->x2.677318, motion_detect_q:0x2054
	{0x18, 0x81, 0x2054}, //index: 144, gain:8.601000DB->x2.691845, motion_detect_q:0x2054
	{0x18, 0x81, 0x2054}, //index: 145, gain:8.648000DB->x2.706450, motion_detect_q:0x2054
	{0x18, 0x82, 0x2054}, //index: 146, gain:8.695000DB->x2.721134, motion_detect_q:0x2054
	{0x18, 0x83, 0x2054}, //index: 147, gain:8.742000DB->x2.735899, motion_detect_q:0x2054
	{0x18, 0x83, 0x2054}, //index: 148, gain:8.789000DB->x2.750743, motion_detect_q:0x2054
	{0x18, 0x84, 0x2054}, //index: 149, gain:8.836000DB->x2.765668, motion_detect_q:0x2054
	{0x18, 0x85, 0x2054}, //index: 150, gain:8.883000DB->x2.780674, motion_detect_q:0x2054
	{0x18, 0x86, 0x2054}, //index: 151, gain:8.930000DB->x2.795761, motion_detect_q:0x2054
	{0x18, 0x86, 0x2054}, //index: 152, gain:8.977000DB->x2.810930, motion_detect_q:0x2054
	{0x18, 0x87, 0x2054}, //index: 153, gain:9.024000DB->x2.826181, motion_detect_q:0x2054
	{0x18, 0x88, 0x2054}, //index: 154, gain:9.071000DB->x2.841515, motion_detect_q:0x2054
	{0x18, 0x88, 0x2054}, //index: 155, gain:9.118000DB->x2.856933, motion_detect_q:0x2054
	{0x18, 0x89, 0x2054}, //index: 156, gain:9.165000DB->x2.872434, motion_detect_q:0x2054
	{0x18, 0x8a, 0x2054}, //index: 157, gain:9.212000DB->x2.888019, motion_detect_q:0x2054
	{0x18, 0x8b, 0x2054}, //index: 158, gain:9.259000DB->x2.903688, motion_detect_q:0x2054
	{0x1a, 0x80, 0x2058}, //index: 159, gain:9.306000DB->x2.919443, motion_detect_q:0x2058
	{0x1a, 0x81, 0x2058}, //index: 160, gain:9.353000DB->x2.935283, motion_detect_q:0x2058
	{0x1a, 0x81, 0x2058}, //index: 161, gain:9.400000DB->x2.951209, motion_detect_q:0x2058
	{0x1a, 0x82, 0x2058}, //index: 162, gain:9.447000DB->x2.967222, motion_detect_q:0x2058
	{0x1a, 0x83, 0x2058}, //index: 163, gain:9.494000DB->x2.983321, motion_detect_q:0x2058
	{0x1a, 0x83, 0x2058}, //index: 164, gain:9.541000DB->x2.999508, motion_detect_q:0x2058
	{0x1a, 0x84, 0x2058}, //index: 165, gain:9.588000DB->x3.015782, motion_detect_q:0x2058
	{0x1a, 0x85, 0x2058}, //index: 166, gain:9.635000DB->x3.032145, motion_detect_q:0x2058
	{0x1a, 0x86, 0x2058}, //index: 167, gain:9.682000DB->x3.048597, motion_detect_q:0x2058
	{0x1a, 0x86, 0x2058}, //index: 168, gain:9.729000DB->x3.065138, motion_detect_q:0x2058
	{0x1a, 0x87, 0x2058}, //index: 169, gain:9.776000DB->x3.081768, motion_detect_q:0x2058
	{0x1a, 0x88, 0x2058}, //index: 170, gain:9.823000DB->x3.098489, motion_detect_q:0x2058
	{0x1a, 0x89, 0x2058}, //index: 171, gain:9.870000DB->x3.115301, motion_detect_q:0x2058
	{0x1a, 0x89, 0x2058}, //index: 172, gain:9.917000DB->x3.132204, motion_detect_q:0x2058
	{0x1a, 0x8a, 0x2058}, //index: 173, gain:9.964000DB->x3.149198, motion_detect_q:0x2058
	{0x1a, 0x8b, 0x2058}, //index: 174, gain:10.011000DB->x3.166285, motion_detect_q:0x2058
	{0x1a, 0x8c, 0x2058}, //index: 175, gain:10.058000DB->x3.183464, motion_detect_q:0x2058
	{0x1c, 0x80, 0x205c}, //index: 176, gain:10.105000DB->x3.200737, motion_detect_q:0x205c
	{0x1c, 0x80, 0x205c}, //index: 177, gain:10.152000DB->x3.218103, motion_detect_q:0x205c
	{0x1c, 0x81, 0x205c}, //index: 178, gain:10.199000DB->x3.235564, motion_detect_q:0x205c
	{0x1c, 0x82, 0x205c}, //index: 179, gain:10.246000DB->x3.253119, motion_detect_q:0x205c
	{0x1c, 0x82, 0x205c}, //index: 180, gain:10.293000DB->x3.270770, motion_detect_q:0x205c
	{0x1c, 0x83, 0x205c}, //index: 181, gain:10.340000DB->x3.288516, motion_detect_q:0x205c
	{0x1c, 0x84, 0x205c}, //index: 182, gain:10.387000DB->x3.306359, motion_detect_q:0x205c
	{0x1c, 0x84, 0x205c}, //index: 183, gain:10.434000DB->x3.324298, motion_detect_q:0x205c
	{0x1c, 0x85, 0x205c}, //index: 184, gain:10.481000DB->x3.342335, motion_detect_q:0x205c
	{0x1c, 0x86, 0x205c}, //index: 185, gain:10.528000DB->x3.360470, motion_detect_q:0x205c
	{0x1c, 0x87, 0x205c}, //index: 186, gain:10.575000DB->x3.378703, motion_detect_q:0x205c
	{0x1c, 0x87, 0x205c}, //index: 187, gain:10.622000DB->x3.397035, motion_detect_q:0x205c
	{0x1c, 0x88, 0x205c}, //index: 188, gain:10.669000DB->x3.415466, motion_detect_q:0x205c
	{0x1c, 0x89, 0x205c}, //index: 189, gain:10.716000DB->x3.433998, motion_detect_q:0x205c
	{0x1c, 0x8a, 0x205c}, //index: 190, gain:10.763000DB->x3.452630, motion_detect_q:0x205c
	{0x1c, 0x8a, 0x205c}, //index: 191, gain:10.810000DB->x3.471363, motion_detect_q:0x205c
	{0x1c, 0x8b, 0x205c}, //index: 192, gain:10.857000DB->x3.490197, motion_detect_q:0x205c
	{0x1c, 0x8c, 0x205c}, //index: 193, gain:10.904000DB->x3.509134, motion_detect_q:0x205c
	{0x1c, 0x8d, 0x205c}, //index: 194, gain:10.951000DB->x3.528174, motion_detect_q:0x205c
	{0x1c, 0x8d, 0x205c}, //index: 195, gain:10.998000DB->x3.547317, motion_detect_q:0x205c
	{0x1e, 0x80, 0x2061}, //index: 196, gain:11.045000DB->x3.566564, motion_detect_q:0x2061
	{0x1e, 0x80, 0x2061}, //index: 197, gain:11.092000DB->x3.585915, motion_detect_q:0x2061
	{0x1e, 0x81, 0x2061}, //index: 198, gain:11.139000DB->x3.605371, motion_detect_q:0x2061
	{0x1e, 0x82, 0x2061}, //index: 199, gain:11.186000DB->x3.624933, motion_detect_q:0x2061
	{0x1e, 0x83, 0x2061}, //index: 200, gain:11.233000DB->x3.644601, motion_detect_q:0x2061
	{0x1e, 0x83, 0x2061}, //index: 201, gain:11.280000DB->x3.664376, motion_detect_q:0x2061
	{0x1e, 0x84, 0x2061}, //index: 202, gain:11.327000DB->x3.684258, motion_detect_q:0x2061
	{0x1e, 0x85, 0x2061}, //index: 203, gain:11.374000DB->x3.704248, motion_detect_q:0x2061
	{0x1e, 0x85, 0x2061}, //index: 204, gain:11.421000DB->x3.724346, motion_detect_q:0x2061
	{0x1e, 0x86, 0x2061}, //index: 205, gain:11.468000DB->x3.744553, motion_detect_q:0x2061
	{0x1e, 0x87, 0x2061}, //index: 206, gain:11.515000DB->x3.764870, motion_detect_q:0x2061
	{0x1e, 0x88, 0x2061}, //index: 207, gain:11.562000DB->x3.785297, motion_detect_q:0x2061
	{0x1e, 0x88, 0x2061}, //index: 208, gain:11.609000DB->x3.805835, motion_detect_q:0x2061
	{0x1e, 0x89, 0x2061}, //index: 209, gain:11.656000DB->x3.826485, motion_detect_q:0x2061
	{0x1e, 0x8a, 0x2061}, //index: 210, gain:11.703000DB->x3.847246, motion_detect_q:0x2061
	{0x1e, 0x8b, 0x2061}, //index: 211, gain:11.750000DB->x3.868121, motion_detect_q:0x2061
	{0x1e, 0x8b, 0x2061}, //index: 212, gain:11.797000DB->x3.889108, motion_detect_q:0x2061
	{0x1e, 0x8c, 0x2061}, //index: 213, gain:11.844000DB->x3.910209, motion_detect_q:0x2061
	{0x1e, 0x8d, 0x2061}, //index: 214, gain:11.891000DB->x3.931425, motion_detect_q:0x2061
	{0x1e, 0x8e, 0x2061}, //index: 215, gain:11.938000DB->x3.952756, motion_detect_q:0x2061
	{0x1e, 0x8e, 0x2061}, //index: 216, gain:11.985000DB->x3.974203, motion_detect_q:0x2061
	{0x1e, 0x8f, 0x2061}, //index: 217, gain:12.032000DB->x3.995766, motion_detect_q:0x2061
	{0x20, 0x80, 0x2067}, //index: 218, gain:12.079000DB->x4.017446, motion_detect_q:0x2067
	{0x20, 0x81, 0x2067}, //index: 219, gain:12.126000DB->x4.039243, motion_detect_q:0x2067
	{0x20, 0x81, 0x2067}, //index: 220, gain:12.173000DB->x4.061159, motion_detect_q:0x2067
	{0x20, 0x82, 0x2067}, //index: 221, gain:12.220000DB->x4.083194, motion_detect_q:0x2067
	{0x20, 0x83, 0x2067}, //index: 222, gain:12.267000DB->x4.105348, motion_detect_q:0x2067
	{0x20, 0x84, 0x2067}, //index: 223, gain:12.314000DB->x4.127623, motion_detect_q:0x2067
	{0x20, 0x84, 0x2067}, //index: 224, gain:12.361000DB->x4.150018, motion_detect_q:0x2067
	{0x20, 0x85, 0x2067}, //index: 225, gain:12.408000DB->x4.172535, motion_detect_q:0x2067
	{0x20, 0x86, 0x2067}, //index: 226, gain:12.455000DB->x4.195174, motion_detect_q:0x2067
	{0x20, 0x86, 0x2067}, //index: 227, gain:12.502000DB->x4.217936, motion_detect_q:0x2067
	{0x20, 0x87, 0x2067}, //index: 228, gain:12.549000DB->x4.240822, motion_detect_q:0x2067
	{0x20, 0x88, 0x2067}, //index: 229, gain:12.596000DB->x4.263831, motion_detect_q:0x2067
	{0x20, 0x89, 0x2067}, //index: 230, gain:12.643000DB->x4.286966, motion_detect_q:0x2067
	{0x20, 0x89, 0x2067}, //index: 231, gain:12.690000DB->x4.310226, motion_detect_q:0x2067
	{0x20, 0x8a, 0x2067}, //index: 232, gain:12.737000DB->x4.333612, motion_detect_q:0x2067
	{0x20, 0x8b, 0x2067}, //index: 233, gain:12.784000DB->x4.357125, motion_detect_q:0x2067
	{0x20, 0x8c, 0x2067}, //index: 234, gain:12.831000DB->x4.380765, motion_detect_q:0x2067
	{0x20, 0x8c, 0x2067}, //index: 235, gain:12.878000DB->x4.404534, motion_detect_q:0x2067
	{0x20, 0x8d, 0x2067}, //index: 236, gain:12.925000DB->x4.428432, motion_detect_q:0x2067
	{0x20, 0x8e, 0x2067}, //index: 237, gain:12.972000DB->x4.452460, motion_detect_q:0x2067
	{0x20, 0x8f, 0x2067}, //index: 238, gain:13.019000DB->x4.476618, motion_detect_q:0x2067
	{0x20, 0x90, 0x2067}, //index: 239, gain:13.066000DB->x4.500907, motion_detect_q:0x2067
	{0x20, 0x90, 0x2067}, //index: 240, gain:13.113000DB->x4.525327, motion_detect_q:0x2067
	{0x20, 0x91, 0x2067}, //index: 241, gain:13.160000DB->x4.549881, motion_detect_q:0x2067
	{0x24, 0x80, 0x206e}, //index: 242, gain:13.207000DB->x4.574567, motion_detect_q:0x206e
	{0x24, 0x81, 0x206e}, //index: 243, gain:13.254000DB->x4.599387, motion_detect_q:0x206e
	{0x24, 0x81, 0x206e}, //index: 244, gain:13.301000DB->x4.624343, motion_detect_q:0x206e
	{0x24, 0x82, 0x206e}, //index: 245, gain:13.348000DB->x4.649433, motion_detect_q:0x206e
	{0x24, 0x83, 0x206e}, //index: 246, gain:13.395000DB->x4.674660, motion_detect_q:0x206e
	{0x24, 0x83, 0x206e}, //index: 247, gain:13.442000DB->x4.700023, motion_detect_q:0x206e
	{0x24, 0x84, 0x206e}, //index: 248, gain:13.489000DB->x4.725524, motion_detect_q:0x206e
	{0x24, 0x85, 0x206e}, //index: 249, gain:13.536000DB->x4.751164, motion_detect_q:0x206e
	{0x24, 0x86, 0x206e}, //index: 250, gain:13.583000DB->x4.776942, motion_detect_q:0x206e
	{0x24, 0x86, 0x206e}, //index: 251, gain:13.630000DB->x4.802861, motion_detect_q:0x206e
	{0x24, 0x87, 0x206e}, //index: 252, gain:13.677000DB->x4.828920, motion_detect_q:0x206e
	{0x24, 0x88, 0x206e}, //index: 253, gain:13.724000DB->x4.855120, motion_detect_q:0x206e
	{0x24, 0x89, 0x206e}, //index: 254, gain:13.771000DB->x4.881463, motion_detect_q:0x206e
	{0x24, 0x89, 0x206e}, //index: 255, gain:13.818000DB->x4.907949, motion_detect_q:0x206e
	{0x24, 0x8a, 0x206e}, //index: 256, gain:13.865000DB->x4.934578, motion_detect_q:0x206e
	{0x24, 0x8b, 0x206e}, //index: 257, gain:13.912000DB->x4.961352, motion_detect_q:0x206e
	{0x24, 0x8c, 0x206e}, //index: 258, gain:13.959000DB->x4.988271, motion_detect_q:0x206e
	{0x24, 0x8c, 0x206e}, //index: 259, gain:14.006000DB->x5.015336, motion_detect_q:0x206e
	{0x24, 0x8d, 0x206e}, //index: 260, gain:14.053000DB->x5.042548, motion_detect_q:0x206e
	{0x24, 0x8e, 0x206e}, //index: 261, gain:14.100000DB->x5.069907, motion_detect_q:0x206e
	{0x24, 0x8f, 0x206e}, //index: 262, gain:14.147000DB->x5.097415, motion_detect_q:0x206e
	{0x24, 0x8f, 0x206e}, //index: 263, gain:14.194000DB->x5.125072, motion_detect_q:0x206e
	{0x24, 0x90, 0x206e}, //index: 264, gain:14.241000DB->x5.152880, motion_detect_q:0x206e
	{0x24, 0x91, 0x206e}, //index: 265, gain:14.288000DB->x5.180838, motion_detect_q:0x206e
	{0x24, 0x92, 0x206e}, //index: 266, gain:14.335000DB->x5.208948, motion_detect_q:0x206e
	{0x24, 0x93, 0x206e}, //index: 267, gain:14.382000DB->x5.237210, motion_detect_q:0x206e
	{0x24, 0x93, 0x206e}, //index: 268, gain:14.429000DB->x5.265626, motion_detect_q:0x206e
	{0x24, 0x94, 0x206e}, //index: 269, gain:14.476000DB->x5.294196, motion_detect_q:0x206e
	{0x24, 0x95, 0x206e}, //index: 270, gain:14.523000DB->x5.322921, motion_detect_q:0x206e
	{0x28, 0x80, 0x2077}, //index: 271, gain:14.570000DB->x5.351802, motion_detect_q:0x2077
	{0x28, 0x80, 0x2077}, //index: 272, gain:14.617000DB->x5.380839, motion_detect_q:0x2077
	{0x28, 0x81, 0x2077}, //index: 273, gain:14.664000DB->x5.410034, motion_detect_q:0x2077
	{0x28, 0x82, 0x2077}, //index: 274, gain:14.711000DB->x5.439388, motion_detect_q:0x2077
	{0x28, 0x83, 0x2077}, //index: 275, gain:14.758000DB->x5.468900, motion_detect_q:0x2077
	{0x28, 0x83, 0x2077}, //index: 276, gain:14.805000DB->x5.498573, motion_detect_q:0x2077
	{0x28, 0x84, 0x2077}, //index: 277, gain:14.852000DB->x5.528407, motion_detect_q:0x2077
	{0x28, 0x85, 0x2077}, //index: 278, gain:14.899000DB->x5.558403, motion_detect_q:0x2077
	{0x28, 0x85, 0x2077}, //index: 279, gain:14.946000DB->x5.588561, motion_detect_q:0x2077
	{0x28, 0x86, 0x2077}, //index: 280, gain:14.993000DB->x5.618883, motion_detect_q:0x2077
	{0x28, 0x87, 0x2077}, //index: 281, gain:15.040000DB->x5.649370, motion_detect_q:0x2077
	{0x28, 0x88, 0x2077}, //index: 282, gain:15.087000DB->x5.680022, motion_detect_q:0x2077
	{0x28, 0x88, 0x2077}, //index: 283, gain:15.134000DB->x5.710840, motion_detect_q:0x2077
	{0x28, 0x89, 0x2077}, //index: 284, gain:15.181000DB->x5.741826, motion_detect_q:0x2077
	{0x28, 0x8a, 0x2077}, //index: 285, gain:15.228000DB->x5.772979, motion_detect_q:0x2077
	{0x28, 0x8b, 0x2077}, //index: 286, gain:15.275000DB->x5.804302, motion_detect_q:0x2077
	{0x28, 0x8b, 0x2077}, //index: 287, gain:15.322000DB->x5.835795, motion_detect_q:0x2077
	{0x28, 0x8c, 0x2077}, //index: 288, gain:15.369000DB->x5.867458, motion_detect_q:0x2077
	{0x28, 0x8d, 0x2077}, //index: 289, gain:15.416000DB->x5.899293, motion_detect_q:0x2077
	{0x28, 0x8e, 0x2077}, //index: 290, gain:15.463000DB->x5.931301, motion_detect_q:0x2077
	{0x28, 0x8e, 0x2077}, //index: 291, gain:15.510000DB->x5.963483, motion_detect_q:0x2077
	{0x28, 0x8f, 0x2077}, //index: 292, gain:15.557000DB->x5.995840, motion_detect_q:0x2077
	{0x28, 0x90, 0x2077}, //index: 293, gain:15.604000DB->x6.028371, motion_detect_q:0x2077
	{0x28, 0x91, 0x2077}, //index: 294, gain:15.651000DB->x6.061080, motion_detect_q:0x2077
	{0x28, 0x92, 0x2077}, //index: 295, gain:15.698000DB->x6.093966, motion_detect_q:0x2077
	{0x28, 0x92, 0x2077}, //index: 296, gain:15.745000DB->x6.127030, motion_detect_q:0x2077
	{0x28, 0x93, 0x2077}, //index: 297, gain:15.792000DB->x6.160274, motion_detect_q:0x2077
	{0x28, 0x94, 0x2077}, //index: 298, gain:15.839000DB->x6.193698, motion_detect_q:0x2077
	{0x28, 0x95, 0x2077}, //index: 299, gain:15.886000DB->x6.227303, motion_detect_q:0x2077
	{0x28, 0x96, 0x2077}, //index: 300, gain:15.933000DB->x6.261091, motion_detect_q:0x2077
	{0x28, 0x96, 0x2077}, //index: 301, gain:15.980000DB->x6.295062, motion_detect_q:0x2077
	{0x28, 0x97, 0x2077}, //index: 302, gain:16.027000DB->x6.329217, motion_detect_q:0x2077
	{0x28, 0x98, 0x2077}, //index: 303, gain:16.074000DB->x6.363558, motion_detect_q:0x2077
	{0x28, 0x99, 0x2077}, //index: 304, gain:16.121000DB->x6.398085, motion_detect_q:0x2077
	{0x2c, 0x80, 0x2082}, //index: 305, gain:16.168000DB->x6.432799, motion_detect_q:0x2082
	{0x2c, 0x81, 0x2082}, //index: 306, gain:16.215000DB->x6.467702, motion_detect_q:0x2082
	{0x2c, 0x81, 0x2082}, //index: 307, gain:16.262000DB->x6.502794, motion_detect_q:0x2082
	{0x2c, 0x82, 0x2082}, //index: 308, gain:16.309000DB->x6.538077, motion_detect_q:0x2082
	{0x2c, 0x83, 0x2082}, //index: 309, gain:16.356000DB->x6.573550, motion_detect_q:0x2082
	{0x2c, 0x83, 0x2082}, //index: 310, gain:16.403000DB->x6.609217, motion_detect_q:0x2082
	{0x2c, 0x84, 0x2082}, //index: 311, gain:16.450000DB->x6.645077, motion_detect_q:0x2082
	{0x2c, 0x85, 0x2082}, //index: 312, gain:16.497000DB->x6.681131, motion_detect_q:0x2082
	{0x2c, 0x86, 0x2082}, //index: 313, gain:16.544000DB->x6.717381, motion_detect_q:0x2082
	{0x2c, 0x86, 0x2082}, //index: 314, gain:16.591000DB->x6.753828, motion_detect_q:0x2082
	{0x2c, 0x87, 0x2082}, //index: 315, gain:16.638000DB->x6.790473, motion_detect_q:0x2082
	{0x2c, 0x88, 0x2082}, //index: 316, gain:16.685000DB->x6.827316, motion_detect_q:0x2082
	{0x2c, 0x89, 0x2082}, //index: 317, gain:16.732000DB->x6.864359, motion_detect_q:0x2082
	{0x2c, 0x89, 0x2082}, //index: 318, gain:16.779000DB->x6.901603, motion_detect_q:0x2082
	{0x2c, 0x8a, 0x2082}, //index: 319, gain:16.826000DB->x6.939050, motion_detect_q:0x2082
	{0x2c, 0x8b, 0x2082}, //index: 320, gain:16.873000DB->x6.976699, motion_detect_q:0x2082
	{0x2c, 0x8c, 0x2082}, //index: 321, gain:16.920000DB->x7.014553, motion_detect_q:0x2082
	{0x2c, 0x8c, 0x2082}, //index: 322, gain:16.967000DB->x7.052612, motion_detect_q:0x2082
	{0x2c, 0x8d, 0x2082}, //index: 323, gain:17.014000DB->x7.090878, motion_detect_q:0x2082
	{0x2c, 0x8e, 0x2082}, //index: 324, gain:17.061000DB->x7.129351, motion_detect_q:0x2082
	{0x2c, 0x8f, 0x2082}, //index: 325, gain:17.108000DB->x7.168033, motion_detect_q:0x2082
	{0x2c, 0x8f, 0x2082}, //index: 326, gain:17.155000DB->x7.206925, motion_detect_q:0x2082
	{0x2c, 0x90, 0x2082}, //index: 327, gain:17.202000DB->x7.246028, motion_detect_q:0x2082
	{0x2c, 0x91, 0x2082}, //index: 328, gain:17.249000DB->x7.285343, motion_detect_q:0x2082
	{0x2c, 0x92, 0x2082}, //index: 329, gain:17.296000DB->x7.324871, motion_detect_q:0x2082
	{0x2c, 0x93, 0x2082}, //index: 330, gain:17.343000DB->x7.364614, motion_detect_q:0x2082
	{0x2c, 0x93, 0x2082}, //index: 331, gain:17.390000DB->x7.404573, motion_detect_q:0x2082
	{0x2c, 0x94, 0x2082}, //index: 332, gain:17.437000DB->x7.444748, motion_detect_q:0x2082
	{0x2c, 0x95, 0x2082}, //index: 333, gain:17.484000DB->x7.485141, motion_detect_q:0x2082
	{0x2c, 0x96, 0x2082}, //index: 334, gain:17.531000DB->x7.525754, motion_detect_q:0x2082
	{0x2c, 0x97, 0x2082}, //index: 335, gain:17.578000DB->x7.566586, motion_detect_q:0x2082
	{0x2c, 0x97, 0x2082}, //index: 336, gain:17.625000DB->x7.607641, motion_detect_q:0x2082
	{0x2c, 0x98, 0x2082}, //index: 337, gain:17.672000DB->x7.648918, motion_detect_q:0x2082
	{0x2c, 0x99, 0x2082}, //index: 338, gain:17.719000DB->x7.690419, motion_detect_q:0x2082
	{0x2c, 0x9a, 0x2082}, //index: 339, gain:17.766000DB->x7.732145, motion_detect_q:0x2082
	{0x2c, 0x9b, 0x2082}, //index: 340, gain:17.813000DB->x7.774098, motion_detect_q:0x2082
	{0x2c, 0x9c, 0x2082}, //index: 341, gain:17.860000DB->x7.816278, motion_detect_q:0x2082
	{0x2c, 0x9c, 0x2082}, //index: 342, gain:17.907000DB->x7.858687, motion_detect_q:0x2082
	{0x2c, 0x9d, 0x2082}, //index: 343, gain:17.954000DB->x7.901326, motion_detect_q:0x2082
	{0x2c, 0x9e, 0x2082}, //index: 344, gain:18.001000DB->x7.944197, motion_detect_q:0x2082
	{0x2c, 0x9f, 0x2082}, //index: 345, gain:18.048000DB->x7.987300, motion_detect_q:0x2082
	{0x30, 0x80, 0x2092}, //index: 346, gain:18.095000DB->x8.030637, motion_detect_q:0x2092
	{0x30, 0x81, 0x2092}, //index: 347, gain:18.142000DB->x8.074209, motion_detect_q:0x2092
	{0x30, 0x81, 0x2092}, //index: 348, gain:18.189000DB->x8.118018, motion_detect_q:0x2092
	{0x30, 0x82, 0x2092}, //index: 349, gain:18.236000DB->x8.162064, motion_detect_q:0x2092
	{0x30, 0x83, 0x2092}, //index: 350, gain:18.283000DB->x8.206349, motion_detect_q:0x2092
	{0x30, 0x84, 0x2092}, //index: 351, gain:18.330000DB->x8.250875, motion_detect_q:0x2092
	{0x30, 0x84, 0x2092}, //index: 352, gain:18.377000DB->x8.295642, motion_detect_q:0x2092
	{0x30, 0x85, 0x2092}, //index: 353, gain:18.424000DB->x8.340652, motion_detect_q:0x2092
	{0x30, 0x86, 0x2092}, //index: 354, gain:18.471000DB->x8.385906, motion_detect_q:0x2092
	{0x30, 0x86, 0x2092}, //index: 355, gain:18.518000DB->x8.431406, motion_detect_q:0x2092
	{0x30, 0x87, 0x2092}, //index: 356, gain:18.565000DB->x8.477153, motion_detect_q:0x2092
	{0x30, 0x88, 0x2092}, //index: 357, gain:18.612000DB->x8.523147, motion_detect_q:0x2092
	{0x30, 0x89, 0x2092}, //index: 358, gain:18.659000DB->x8.569392, motion_detect_q:0x2092
	{0x30, 0x89, 0x2092}, //index: 359, gain:18.706000DB->x8.615887, motion_detect_q:0x2092
	{0x30, 0x8a, 0x2092}, //index: 360, gain:18.753000DB->x8.662635, motion_detect_q:0x2092
	{0x30, 0x8b, 0x2092}, //index: 361, gain:18.800000DB->x8.709636, motion_detect_q:0x2092
	{0x30, 0x8c, 0x2092}, //index: 362, gain:18.847000DB->x8.756892, motion_detect_q:0x2092
	{0x30, 0x8c, 0x2092}, //index: 363, gain:18.894000DB->x8.804405, motion_detect_q:0x2092
	{0x30, 0x8d, 0x2092}, //index: 364, gain:18.941000DB->x8.852175, motion_detect_q:0x2092
	{0x30, 0x8e, 0x2092}, //index: 365, gain:18.988000DB->x8.900205, motion_detect_q:0x2092
	{0x30, 0x8f, 0x2092}, //index: 366, gain:19.035000DB->x8.948495, motion_detect_q:0x2092
	{0x30, 0x8f, 0x2092}, //index: 367, gain:19.082000DB->x8.997047, motion_detect_q:0x2092
	{0x30, 0x90, 0x2092}, //index: 368, gain:19.129000DB->x9.045863, motion_detect_q:0x2092
	{0x30, 0x91, 0x2092}, //index: 369, gain:19.176000DB->x9.094943, motion_detect_q:0x2092
	{0x30, 0x92, 0x2092}, //index: 370, gain:19.223000DB->x9.144290, motion_detect_q:0x2092
	{0x30, 0x93, 0x2092}, //index: 371, gain:19.270000DB->x9.193905, motion_detect_q:0x2092
	{0x30, 0x93, 0x2092}, //index: 372, gain:19.317000DB->x9.243788, motion_detect_q:0x2092
	{0x30, 0x94, 0x2092}, //index: 373, gain:19.364000DB->x9.293943, motion_detect_q:0x2092
	{0x30, 0x95, 0x2092}, //index: 374, gain:19.411000DB->x9.344369, motion_detect_q:0x2092
	{0x30, 0x96, 0x2092}, //index: 375, gain:19.458000DB->x9.395070, motion_detect_q:0x2092
	{0x30, 0x97, 0x2092}, //index: 376, gain:19.505000DB->x9.446045, motion_detect_q:0x2092
	{0x30, 0x97, 0x2092}, //index: 377, gain:19.552000DB->x9.497297, motion_detect_q:0x2092
	{0x30, 0x98, 0x2092}, //index: 378, gain:19.599000DB->x9.548826, motion_detect_q:0x2092
	{0x30, 0x99, 0x2092}, //index: 379, gain:19.646000DB->x9.600636, motion_detect_q:0x2092
	{0x30, 0x9a, 0x2092}, //index: 380, gain:19.693000DB->x9.652726, motion_detect_q:0x2092
	{0x30, 0x9b, 0x2092}, //index: 381, gain:19.740000DB->x9.705100, motion_detect_q:0x2092
	{0x30, 0x9c, 0x2092}, //index: 382, gain:19.787000DB->x9.757757, motion_detect_q:0x2092
	{0x30, 0x9c, 0x2092}, //index: 383, gain:19.834000DB->x9.810700, motion_detect_q:0x2092
	{0x30, 0x9d, 0x2092}, //index: 384, gain:19.881000DB->x9.863930, motion_detect_q:0x2092
	{0x30, 0x9e, 0x2092}, //index: 385, gain:19.928000DB->x9.917450, motion_detect_q:0x2092
	{0x30, 0x9f, 0x2092}, //index: 386, gain:19.975000DB->x9.971259, motion_detect_q:0x2092
	{0x30, 0xa0, 0x2092}, //index: 387, gain:20.022000DB->x10.025361, motion_detect_q:0x2092
	{0x30, 0xa1, 0x2092}, //index: 388, gain:20.069000DB->x10.079756, motion_detect_q:0x2092
	{0x30, 0xa2, 0x2092}, //index: 389, gain:20.116000DB->x10.134446, motion_detect_q:0x2092
	{0x30, 0xa3, 0x2092}, //index: 390, gain:20.163000DB->x10.189433, motion_detect_q:0x2092
	{0x30, 0xa3, 0x2092}, //index: 391, gain:20.210000DB->x10.244718, motion_detect_q:0x2092
	{0x30, 0xa4, 0x2092}, //index: 392, gain:20.257000DB->x10.300303, motion_detect_q:0x2092
	{0x30, 0xa5, 0x2092}, //index: 393, gain:20.304000DB->x10.356190, motion_detect_q:0x2092
	{0x30, 0xa6, 0x2092}, //index: 394, gain:20.351000DB->x10.412380, motion_detect_q:0x2092
	{0x30, 0xa7, 0x2092}, //index: 395, gain:20.398000DB->x10.468875, motion_detect_q:0x2092
	{0x30, 0xa8, 0x2092}, //index: 396, gain:20.445000DB->x10.525676, motion_detect_q:0x2092
	{0x30, 0xa9, 0x2092}, //index: 397, gain:20.492000DB->x10.582786, motion_detect_q:0x2092
	{0x30, 0xaa, 0x2092}, //index: 398, gain:20.539000DB->x10.640205, motion_detect_q:0x2092
	{0x30, 0xab, 0x2092}, //index: 399, gain:20.586000DB->x10.697936, motion_detect_q:0x2092
	{0x30, 0xac, 0x2092}, //index: 400, gain:20.633000DB->x10.755980, motion_detect_q:0x2092
	{0x30, 0xad, 0x2092}, //index: 401, gain:20.680000DB->x10.814340, motion_detect_q:0x2092
	{0x30, 0xad, 0x2092}, //index: 402, gain:20.727000DB->x10.873015, motion_detect_q:0x2092
	{0x30, 0xae, 0x2092}, //index: 403, gain:20.774000DB->x10.932009, motion_detect_q:0x2092
	{0x30, 0xaf, 0x2092}, //index: 404, gain:20.821000DB->x10.991324, motion_detect_q:0x2092
	{0x30, 0xb0, 0x2092}, //index: 405, gain:20.868000DB->x11.050960, motion_detect_q:0x2092
	{0x30, 0xb1, 0x2092}, //index: 406, gain:20.915000DB->x11.110919, motion_detect_q:0x2092
	{0x30, 0xb2, 0x2092}, //index: 407, gain:20.962000DB->x11.171204, motion_detect_q:0x2092
	{0x30, 0xb3, 0x2092}, //index: 408, gain:21.009000DB->x11.231817, motion_detect_q:0x2092
	{0x30, 0xb4, 0x2092}, //index: 409, gain:21.056000DB->x11.292757, motion_detect_q:0x2092
	{0x30, 0xb5, 0x2092}, //index: 410, gain:21.103000DB->x11.354029, motion_detect_q:0x2092
	{0x30, 0xb6, 0x2092}, //index: 411, gain:21.150000DB->x11.415633, motion_detect_q:0x2092
	{0x30, 0xb7, 0x2092}, //index: 412, gain:21.197000DB->x11.477571, motion_detect_q:0x2092
	{0x30, 0xb8, 0x2092}, //index: 413, gain:21.244000DB->x11.539846, motion_detect_q:0x2092
	{0x30, 0xb9, 0x2092}, //index: 414, gain:21.291000DB->x11.602458, motion_detect_q:0x2092
	{0x30, 0xba, 0x2092}, //index: 415, gain:21.338000DB->x11.665410, motion_detect_q:0x2092
	{0x30, 0xbb, 0x2092}, //index: 416, gain:21.385000DB->x11.728703, motion_detect_q:0x2092
	{0x30, 0xbc, 0x2092}, //index: 417, gain:21.432000DB->x11.792340, motion_detect_q:0x2092
	{0x30, 0xbd, 0x2092}, //index: 418, gain:21.479000DB->x11.856322, motion_detect_q:0x2092
	{0x30, 0xbe, 0x2092}, //index: 419, gain:21.526000DB->x11.920652, motion_detect_q:0x2092
	{0x30, 0xbf, 0x2092}, //index: 420, gain:21.573000DB->x11.985330, motion_detect_q:0x2092
	{0x30, 0xc0, 0x2092}, //index: 421, gain:21.620000DB->x12.050359, motion_detect_q:0x2092
	{0x30, 0xc1, 0x2092}, //index: 422, gain:21.667000DB->x12.115742, motion_detect_q:0x2092
	{0x30, 0xc2, 0x2092}, //index: 423, gain:21.714000DB->x12.181478, motion_detect_q:0x2092
	{0x30, 0xc3, 0x2092}, //index: 424, gain:21.761000DB->x12.247572, motion_detect_q:0x2092
	{0x30, 0xc5, 0x2092}, //index: 425, gain:21.808000DB->x12.314024, motion_detect_q:0x2092
	{0x30, 0xc6, 0x2092}, //index: 426, gain:21.855000DB->x12.380837, motion_detect_q:0x2092
	{0x30, 0xc7, 0x2092}, //index: 427, gain:21.902000DB->x12.448012, motion_detect_q:0x2092
	{0x30, 0xc8, 0x2092}, //index: 428, gain:21.949000DB->x12.515552, motion_detect_q:0x2092
	{0x30, 0xc9, 0x2092}, //index: 429, gain:21.996000DB->x12.583458, motion_detect_q:0x2092
	{0x30, 0xca, 0x2092}, //index: 430, gain:22.043000DB->x12.651732, motion_detect_q:0x2092
	{0x30, 0xcb, 0x2092}, //index: 431, gain:22.090000DB->x12.720378, motion_detect_q:0x2092
	{0x30, 0xcc, 0x2092}, //index: 432, gain:22.137000DB->x12.789395, motion_detect_q:0x2092
	{0x30, 0xcd, 0x2092}, //index: 433, gain:22.184000DB->x12.858787, motion_detect_q:0x2092
	{0x30, 0xce, 0x2092}, //index: 434, gain:22.231000DB->x12.928555, motion_detect_q:0x2092
	{0x30, 0xcf, 0x2092}, //index: 435, gain:22.278000DB->x12.998702, motion_detect_q:0x2092
	{0x30, 0xd1, 0x2092}, //index: 436, gain:22.325000DB->x13.069230, motion_detect_q:0x2092
	{0x30, 0xd2, 0x2092}, //index: 437, gain:22.372000DB->x13.140140, motion_detect_q:0x2092
	{0x30, 0xd3, 0x2092}, //index: 438, gain:22.419000DB->x13.211435, motion_detect_q:0x2092
	{0x30, 0xd4, 0x2092}, //index: 439, gain:22.466000DB->x13.283117, motion_detect_q:0x2092
	{0x30, 0xd5, 0x2092}, //index: 440, gain:22.513000DB->x13.355188, motion_detect_q:0x2092
	{0x30, 0xd6, 0x2092}, //index: 441, gain:22.560000DB->x13.427650, motion_detect_q:0x2092
	{0x30, 0xd8, 0x2092}, //index: 442, gain:22.607000DB->x13.500505, motion_detect_q:0x2092
	{0x30, 0xd9, 0x2092}, //index: 443, gain:22.654000DB->x13.573755, motion_detect_q:0x2092
	{0x30, 0xda, 0x2092}, //index: 444, gain:22.701000DB->x13.647402, motion_detect_q:0x2092
	{0x30, 0xdb, 0x2092}, //index: 445, gain:22.748000DB->x13.721450, motion_detect_q:0x2092
	{0x30, 0xdc, 0x2092}, //index: 446, gain:22.795000DB->x13.795899, motion_detect_q:0x2092
	{0x30, 0xdd, 0x2092}, //index: 447, gain:22.842000DB->x13.870752, motion_detect_q:0x2092
	{0x30, 0xdf, 0x2092}, //index: 448, gain:22.889000DB->x13.946011, motion_detect_q:0x2092
	{0x30, 0xe0, 0x2092}, //index: 449, gain:22.936000DB->x14.021678, motion_detect_q:0x2092
	{0x30, 0xe1, 0x2092}, //index: 450, gain:22.983000DB->x14.097756, motion_detect_q:0x2092
	{0x30, 0xe2, 0x2092}, //index: 451, gain:23.030000DB->x14.174247, motion_detect_q:0x2092
	{0x30, 0xe4, 0x2092}, //index: 452, gain:23.077000DB->x14.251153, motion_detect_q:0x2092
	{0x30, 0xe5, 0x2092}, //index: 453, gain:23.124000DB->x14.328476, motion_detect_q:0x2092
	{0x30, 0xe6, 0x2092}, //index: 454, gain:23.171000DB->x14.406219, motion_detect_q:0x2092
	{0x30, 0xe7, 0x2092}, //index: 455, gain:23.218000DB->x14.484383, motion_detect_q:0x2092
	{0x30, 0xe9, 0x2092}, //index: 456, gain:23.265000DB->x14.562972, motion_detect_q:0x2092
	{0x30, 0xea, 0x2092}, //index: 457, gain:23.312000DB->x14.641986, motion_detect_q:0x2092
	{0x30, 0xeb, 0x2092}, //index: 458, gain:23.359000DB->x14.721430, motion_detect_q:0x2092
	{0x30, 0xec, 0x2092}, //index: 459, gain:23.406000DB->x14.801305, motion_detect_q:0x2092
	{0x30, 0xee, 0x2092}, //index: 460, gain:23.453000DB->x14.881613, motion_detect_q:0x2092
	{0x30, 0xef, 0x2092}, //index: 461, gain:23.500000DB->x14.962357, motion_detect_q:0x2092
	{0x30, 0xf0, 0x2092}, //index: 462, gain:23.547000DB->x15.043538, motion_detect_q:0x2092
	{0x30, 0xf2, 0x2092}, //index: 463, gain:23.594000DB->x15.125161, motion_detect_q:0x2092
	{0x30, 0xf3, 0x2092}, //index: 464, gain:23.641000DB->x15.207226, motion_detect_q:0x2092
	{0x30, 0xf4, 0x2092}, //index: 465, gain:23.688000DB->x15.289736, motion_detect_q:0x2092
	{0x30, 0xf5, 0x2092}, //index: 466, gain:23.735000DB->x15.372695, motion_detect_q:0x2092
	{0x30, 0xf7, 0x2092}, //index: 467, gain:23.782000DB->x15.456103, motion_detect_q:0x2092
	{0x30, 0xf8, 0x2092}, //index: 468, gain:23.829000DB->x15.539964, motion_detect_q:0x2092
	{0x30, 0xf9, 0x2092}, //index: 469, gain:23.876000DB->x15.624280, motion_detect_q:0x2092
	{0x30, 0xfb, 0x2092}, //index: 470, gain:23.923000DB->x15.709053, motion_detect_q:0x2092
	{0x30, 0xfc, 0x2092}, //index: 471, gain:23.970000DB->x15.794286, motion_detect_q:0x2092
	{0x30, 0xfe, 0x2092}, //index: 472, gain:24.017000DB->x15.879982, motion_detect_q:0x2092
};

#define AR0331_GAIN_1_23DB	(601)
#define AR0331_GAIN_ROWS		(602)
#define AR0331_GAIN_COLS		(2)
/* This is 64-step gain table, AR0331_GAIN_ROWS = 359, AR0331_GAIN_COLS = 3 */
const s16 AR0331_GAIN_TABLE[AR0331_GAIN_ROWS][AR0331_GAIN_COLS] =
{
	{0x30, 0x1fe}, //index: 0, gain:30.080000DB->x31.915379
	{0x30, 0x1fb}, //index: 1, gain:30.033000DB->x31.743148
	{0x30, 0x1f9}, //index: 2, gain:29.986000DB->x31.571848
	{0x30, 0x1f6}, //index: 3, gain:29.939000DB->x31.401472
	{0x30, 0x1f3}, //index: 4, gain:29.892000DB->x31.232015
	{0x30, 0x1f1}, //index: 5, gain:29.845000DB->x31.063472
	{0x30, 0x1ee}, //index: 6, gain:29.798000DB->x30.895839
	{0x30, 0x1eb}, //index: 7, gain:29.751000DB->x30.729111
	{0x30, 0x1e9}, //index: 8, gain:29.704000DB->x30.563283
	{0x30, 0x1e6}, //index: 9, gain:29.657000DB->x30.398349
	{0x30, 0x1e3}, //index: 10, gain:29.610000DB->x30.234306
	{0x30, 0x1e1}, //index: 11, gain:29.563000DB->x30.071147
	{0x30, 0x1de}, //index: 12, gain:29.516000DB->x29.908870
	{0x30, 0x1db}, //index: 13, gain:29.469000DB->x29.747468
	{0x30, 0x1d9}, //index: 14, gain:29.422000DB->x29.586937
	{0x30, 0x1d6}, //index: 15, gain:29.375000DB->x29.427272
	{0x30, 0x1d4}, //index: 16, gain:29.328000DB->x29.268469
	{0x30, 0x1d1}, //index: 17, gain:29.281000DB->x29.110522
	{0x30, 0x1cf}, //index: 18, gain:29.234000DB->x28.953429
	{0x30, 0x1cc}, //index: 19, gain:29.187000DB->x28.797183
	{0x30, 0x1ca}, //index: 20, gain:29.140000DB->x28.641780
	{0x30, 0x1c7}, //index: 21, gain:29.093000DB->x28.487215
	{0x30, 0x1c5}, //index: 22, gain:29.046000DB->x28.333485
	{0x30, 0x1c2}, //index: 23, gain:28.999000DB->x28.180585
	{0x30, 0x1c0}, //index: 24, gain:28.952000DB->x28.028509
	{0x30, 0x1be}, //index: 25, gain:28.905000DB->x27.877255
	{0x30, 0x1bb}, //index: 26, gain:28.858000DB->x27.726816
	{0x30, 0x1b9}, //index: 27, gain:28.811000DB->x27.577189
	{0x30, 0x1b6}, //index: 28, gain:28.764000DB->x27.428370
	{0x30, 0x1b4}, //index: 29, gain:28.717000DB->x27.280354
	{0x30, 0x1b2}, //index: 30, gain:28.670000DB->x27.133137
	{0x30, 0x1af}, //index: 31, gain:28.623000DB->x26.986714
	{0x30, 0x1ad}, //index: 32, gain:28.576000DB->x26.841081
	{0x30, 0x1ab}, //index: 33, gain:28.529000DB->x26.696234
	{0x30, 0x1a8}, //index: 34, gain:28.482000DB->x26.552169
	{0x30, 0x1a6}, //index: 35, gain:28.435000DB->x26.408881
	{0x30, 0x1a4}, //index: 36, gain:28.388000DB->x26.266367
	{0x30, 0x1a1}, //index: 37, gain:28.341000DB->x26.124621
	{0x30, 0x19f}, //index: 38, gain:28.294000DB->x25.983641
	{0x30, 0x19d}, //index: 39, gain:28.247000DB->x25.843421
	{0x30, 0x19b}, //index: 40, gain:28.200000DB->x25.703958
	{0x30, 0x199}, //index: 41, gain:28.153000DB->x25.565247
	{0x30, 0x196}, //index: 42, gain:28.106000DB->x25.427286
	{0x30, 0x194}, //index: 43, gain:28.059000DB->x25.290068
	{0x30, 0x192}, //index: 44, gain:28.012000DB->x25.153591
	{0x30, 0x190}, //index: 45, gain:27.965000DB->x25.017851
	{0x30, 0x18e}, //index: 46, gain:27.918000DB->x24.882843
	{0x30, 0x18b}, //index: 47, gain:27.871000DB->x24.748564
	{0x30, 0x189}, //index: 48, gain:27.824000DB->x24.615009
	{0x30, 0x187}, //index: 49, gain:27.777000DB->x24.482175
	{0x30, 0x185}, //index: 50, gain:27.730000DB->x24.350058
	{0x30, 0x183}, //index: 51, gain:27.683000DB->x24.218654
	{0x30, 0x181}, //index: 52, gain:27.636000DB->x24.087959
	{0x30, 0x17f}, //index: 53, gain:27.589000DB->x23.957969
	{0x30, 0x17d}, //index: 54, gain:27.542000DB->x23.828681
	{0x30, 0x17b}, //index: 55, gain:27.495000DB->x23.700090
	{0x30, 0x179}, //index: 56, gain:27.448000DB->x23.572194
	{0x30, 0x177}, //index: 57, gain:27.401000DB->x23.444987
	{0x30, 0x175}, //index: 58, gain:27.354000DB->x23.318467
	{0x30, 0x173}, //index: 59, gain:27.307000DB->x23.192630
	{0x30, 0x171}, //index: 60, gain:27.260000DB->x23.067472
	{0x30, 0x16f}, //index: 61, gain:27.213000DB->x22.942989
	{0x30, 0x16d}, //index: 62, gain:27.166000DB->x22.819178
	{0x30, 0x16b}, //index: 63, gain:27.119000DB->x22.696035
	{0x30, 0x169}, //index: 64, gain:27.072000DB->x22.573557
	{0x30, 0x167}, //index: 65, gain:27.025000DB->x22.451740
	{0x30, 0x165}, //index: 66, gain:26.978000DB->x22.330580
	{0x30, 0x163}, //index: 67, gain:26.931000DB->x22.210074
	{0x30, 0x161}, //index: 68, gain:26.884000DB->x22.090218
	{0x30, 0x15f}, //index: 69, gain:26.837000DB->x21.971009
	{0x30, 0x15d}, //index: 70, gain:26.790000DB->x21.852443
	{0x30, 0x15b}, //index: 71, gain:26.743000DB->x21.734517
	{0x30, 0x159}, //index: 72, gain:26.696000DB->x21.617228
	{0x30, 0x158}, //index: 73, gain:26.649000DB->x21.500571
	{0x30, 0x156}, //index: 74, gain:26.602000DB->x21.384544
	{0x30, 0x154}, //index: 75, gain:26.555000DB->x21.269143
	{0x30, 0x152}, //index: 76, gain:26.508000DB->x21.154365
	{0x30, 0x150}, //index: 77, gain:26.461000DB->x21.040207
	{0x30, 0x14e}, //index: 78, gain:26.414000DB->x20.926664
	{0x30, 0x14d}, //index: 79, gain:26.367000DB->x20.813734
	{0x30, 0x14b}, //index: 80, gain:26.320000DB->x20.701413
	{0x30, 0x149}, //index: 81, gain:26.273000DB->x20.589699
	{0x30, 0x147}, //index: 82, gain:26.226000DB->x20.478588
	{0x30, 0x145}, //index: 83, gain:26.179000DB->x20.368076
	{0x30, 0x144}, //index: 84, gain:26.132000DB->x20.258160
	{0x30, 0x142}, //index: 85, gain:26.085000DB->x20.148838
	{0x30, 0x140}, //index: 86, gain:26.038000DB->x20.040105
	{0x30, 0x13e}, //index: 87, gain:25.991000DB->x19.931960
	{0x30, 0x13d}, //index: 88, gain:25.944000DB->x19.824398
	{0x30, 0x13b}, //index: 89, gain:25.897000DB->x19.717416
	{0x30, 0x139}, //index: 90, gain:25.850000DB->x19.611012
	{0x30, 0x138}, //index: 91, gain:25.803000DB->x19.505182
	{0x30, 0x136}, //index: 92, gain:25.756000DB->x19.399923
	{0x30, 0x134}, //index: 93, gain:25.709000DB->x19.295232
	{0x30, 0x133}, //index: 94, gain:25.662000DB->x19.191106
	{0x30, 0x131}, //index: 95, gain:25.615000DB->x19.087542
	{0x30, 0x12f}, //index: 96, gain:25.568000DB->x18.984537
	{0x30, 0x12e}, //index: 97, gain:25.521000DB->x18.882087
	{0x30, 0x12c}, //index: 98, gain:25.474000DB->x18.780191
	{0x30, 0x12a}, //index: 99, gain:25.427000DB->x18.678844
	{0x30, 0x129}, //index: 100, gain:25.380000DB->x18.578045
	{0x30, 0x127}, //index: 101, gain:25.333000DB->x18.477789
	{0x30, 0x126}, //index: 102, gain:25.286000DB->x18.378074
	{0x30, 0x124}, //index: 103, gain:25.239000DB->x18.278898
	{0x30, 0x122}, //index: 104, gain:25.192000DB->x18.180256
	{0x30, 0x121}, //index: 105, gain:25.145000DB->x18.082147
	{0x30, 0x11f}, //index: 106, gain:25.098000DB->x17.984568
	{0x30, 0x11e}, //index: 107, gain:25.051000DB->x17.887515
	{0x30, 0x11c}, //index: 108, gain:25.004000DB->x17.790985
	{0x30, 0x11b}, //index: 109, gain:24.957000DB->x17.694977
	{0x30, 0x119}, //index: 110, gain:24.910000DB->x17.599487
	{0x30, 0x118}, //index: 111, gain:24.863000DB->x17.504512
	{0x30, 0x116}, //index: 112, gain:24.816000DB->x17.410049
	{0x30, 0x115}, //index: 113, gain:24.769000DB->x17.316097
	{0x30, 0x113}, //index: 114, gain:24.722000DB->x17.222651
	{0x30, 0x112}, //index: 115, gain:24.675000DB->x17.129710
	{0x30, 0x110}, //index: 116, gain:24.628000DB->x17.037270
	{0x30, 0x10f}, //index: 117, gain:24.581000DB->x16.945329
	{0x30, 0x10d}, //index: 118, gain:24.534000DB->x16.853884
	{0x30, 0x10c}, //index: 119, gain:24.487000DB->x16.762933
	{0x30, 0x10a}, //index: 120, gain:24.440000DB->x16.672472
	{0x30, 0x109}, //index: 121, gain:24.393000DB->x16.582500
	{0x30, 0x107}, //index: 122, gain:24.346000DB->x16.493013
	{0x30, 0x106}, //index: 123, gain:24.299000DB->x16.404009
	{0x30, 0x105}, //index: 124, gain:24.252000DB->x16.315485
	{0x30, 0x103}, //index: 125, gain:24.205000DB->x16.227440
	{0x30, 0x102}, //index: 126, gain:24.158000DB->x16.139869
	{0x30, 0x100}, //index: 127, gain:24.111000DB->x16.052771
	{0x30, 0xff}, //index: 128, gain:24.064000DB->x15.966142
	{0x30, 0xfe}, //index: 129, gain:24.017000DB->x15.879982
	{0x30, 0xfc}, //index: 130, gain:23.970000DB->x15.794286
	{0x30, 0xfb}, //index: 131, gain:23.923000DB->x15.709053
	{0x30, 0xf9}, //index: 132, gain:23.876000DB->x15.624280
	{0x30, 0xf8}, //index: 133, gain:23.829000DB->x15.539964
	{0x30, 0xf7}, //index: 134, gain:23.782000DB->x15.456103
	{0x30, 0xf5}, //index: 135, gain:23.735000DB->x15.372695
	{0x30, 0xf4}, //index: 136, gain:23.688000DB->x15.289736
	{0x30, 0xf3}, //index: 137, gain:23.641000DB->x15.207226
	{0x30, 0xf2}, //index: 138, gain:23.594000DB->x15.125161
	{0x30, 0xf0}, //index: 139, gain:23.547000DB->x15.043538
	{0x30, 0xef}, //index: 140, gain:23.500000DB->x14.962357
	{0x30, 0xee}, //index: 141, gain:23.453000DB->x14.881613
	{0x30, 0xec}, //index: 142, gain:23.406000DB->x14.801305
	{0x30, 0xeb}, //index: 143, gain:23.359000DB->x14.721430
	{0x30, 0xea}, //index: 144, gain:23.312000DB->x14.641986
	{0x30, 0xe9}, //index: 145, gain:23.265000DB->x14.562972
	{0x30, 0xe7}, //index: 146, gain:23.218000DB->x14.484383
	{0x30, 0xe6}, //index: 147, gain:23.171000DB->x14.406219
	{0x30, 0xe5}, //index: 148, gain:23.124000DB->x14.328476
	{0x30, 0xe4}, //index: 149, gain:23.077000DB->x14.251153
	{0x30, 0xe2}, //index: 150, gain:23.030000DB->x14.174247
	{0x30, 0xe1}, //index: 151, gain:22.983000DB->x14.097756
	{0x30, 0xe0}, //index: 152, gain:22.936000DB->x14.021678
	{0x30, 0xdf}, //index: 153, gain:22.889000DB->x13.946011
	{0x30, 0xdd}, //index: 154, gain:22.842000DB->x13.870752
	{0x30, 0xdc}, //index: 155, gain:22.795000DB->x13.795899
	{0x30, 0xdb}, //index: 156, gain:22.748000DB->x13.721450
	{0x30, 0xda}, //index: 157, gain:22.701000DB->x13.647402
	{0x30, 0xd9}, //index: 158, gain:22.654000DB->x13.573755
	{0x30, 0xd8}, //index: 159, gain:22.607000DB->x13.500505
	{0x30, 0xd6}, //index: 160, gain:22.560000DB->x13.427650
	{0x30, 0xd5}, //index: 161, gain:22.513000DB->x13.355188
	{0x30, 0xd4}, //index: 162, gain:22.466000DB->x13.283117
	{0x30, 0xd3}, //index: 163, gain:22.419000DB->x13.211435
	{0x30, 0xd2}, //index: 164, gain:22.372000DB->x13.140140
	{0x30, 0xd1}, //index: 165, gain:22.325000DB->x13.069230
	{0x30, 0xcf}, //index: 166, gain:22.278000DB->x12.998702
	{0x30, 0xce}, //index: 167, gain:22.231000DB->x12.928555
	{0x30, 0xcd}, //index: 168, gain:22.184000DB->x12.858787
	{0x30, 0xcc}, //index: 169, gain:22.137000DB->x12.789395
	{0x30, 0xcb}, //index: 170, gain:22.090000DB->x12.720378
	{0x30, 0xca}, //index: 171, gain:22.043000DB->x12.651732
	{0x30, 0xc9}, //index: 172, gain:21.996000DB->x12.583458
	{0x30, 0xc8}, //index: 173, gain:21.949000DB->x12.515552
	{0x30, 0xc7}, //index: 174, gain:21.902000DB->x12.448012
	{0x30, 0xc6}, //index: 175, gain:21.855000DB->x12.380837
	{0x30, 0xc5}, //index: 176, gain:21.808000DB->x12.314024
	{0x30, 0xc3}, //index: 177, gain:21.761000DB->x12.247572
	{0x30, 0xc2}, //index: 178, gain:21.714000DB->x12.181478
	{0x30, 0xc1}, //index: 179, gain:21.667000DB->x12.115742
	{0x30, 0xc0}, //index: 180, gain:21.620000DB->x12.050359
	{0x30, 0xbf}, //index: 181, gain:21.573000DB->x11.985330
	{0x30, 0xbe}, //index: 182, gain:21.526000DB->x11.920652
	{0x30, 0xbd}, //index: 183, gain:21.479000DB->x11.856322
	{0x30, 0xbc}, //index: 184, gain:21.432000DB->x11.792340
	{0x30, 0xbb}, //index: 185, gain:21.385000DB->x11.728703
	{0x30, 0xba}, //index: 186, gain:21.338000DB->x11.665410
	{0x30, 0xb9}, //index: 187, gain:21.291000DB->x11.602458
	{0x30, 0xb8}, //index: 188, gain:21.244000DB->x11.539846
	{0x30, 0xb7}, //index: 189, gain:21.197000DB->x11.477571
	{0x30, 0xb6}, //index: 190, gain:21.150000DB->x11.415633
	{0x30, 0xb5}, //index: 191, gain:21.103000DB->x11.354029
	{0x30, 0xb4}, //index: 192, gain:21.056000DB->x11.292757
	{0x30, 0xb3}, //index: 193, gain:21.009000DB->x11.231817
	{0x30, 0xb2}, //index: 194, gain:20.962000DB->x11.171204
	{0x30, 0xb1}, //index: 195, gain:20.915000DB->x11.110919
	{0x30, 0xb0}, //index: 196, gain:20.868000DB->x11.050960
	{0x30, 0xaf}, //index: 197, gain:20.821000DB->x10.991324
	{0x30, 0xae}, //index: 198, gain:20.774000DB->x10.932009
	{0x30, 0xad}, //index: 199, gain:20.727000DB->x10.873015
	{0x30, 0xad}, //index: 200, gain:20.680000DB->x10.814340
	{0x30, 0xac}, //index: 201, gain:20.633000DB->x10.755980
	{0x30, 0xab}, //index: 202, gain:20.586000DB->x10.697936
	{0x30, 0xaa}, //index: 203, gain:20.539000DB->x10.640205
	{0x30, 0xa9}, //index: 204, gain:20.492000DB->x10.582786
	{0x30, 0xa8}, //index: 205, gain:20.445000DB->x10.525676
	{0x30, 0xa7}, //index: 206, gain:20.398000DB->x10.468875
	{0x30, 0xa6}, //index: 207, gain:20.351000DB->x10.412380
	{0x30, 0xa5}, //index: 208, gain:20.304000DB->x10.356190
	{0x30, 0xa4}, //index: 209, gain:20.257000DB->x10.300303
	{0x30, 0xa3}, //index: 210, gain:20.210000DB->x10.244718
	{0x30, 0xa3}, //index: 211, gain:20.163000DB->x10.189433
	{0x30, 0xa2}, //index: 212, gain:20.116000DB->x10.134446
	{0x30, 0xa1}, //index: 213, gain:20.069000DB->x10.079756
	{0x30, 0xa0}, //index: 214, gain:20.022000DB->x10.025361
	{0x30, 0x9f}, //index: 215, gain:19.975000DB->x9.971259
	{0x30, 0x9e}, //index: 216, gain:19.928000DB->x9.917450
	{0x30, 0x9d}, //index: 217, gain:19.881000DB->x9.863930
	{0x30, 0x9c}, //index: 218, gain:19.834000DB->x9.810700
	{0x30, 0x9c}, //index: 219, gain:19.787000DB->x9.757757
	{0x30, 0x9b}, //index: 220, gain:19.740000DB->x9.705100
	{0x30, 0x9a}, //index: 221, gain:19.693000DB->x9.652726
	{0x30, 0x99}, //index: 222, gain:19.646000DB->x9.600636
	{0x30, 0x98}, //index: 223, gain:19.599000DB->x9.548826
	{0x30, 0x97}, //index: 224, gain:19.552000DB->x9.497297
	{0x30, 0x97}, //index: 225, gain:19.505000DB->x9.446045
	{0x30, 0x96}, //index: 226, gain:19.458000DB->x9.395070
	{0x30, 0x95}, //index: 227, gain:19.411000DB->x9.344369
	{0x30, 0x94}, //index: 228, gain:19.364000DB->x9.293943
	{0x30, 0x93}, //index: 229, gain:19.317000DB->x9.243788
	{0x30, 0x93}, //index: 230, gain:19.270000DB->x9.193905
	{0x30, 0x92}, //index: 231, gain:19.223000DB->x9.144290
	{0x30, 0x91}, //index: 232, gain:19.176000DB->x9.094943
	{0x30, 0x90}, //index: 233, gain:19.129000DB->x9.045863
	{0x30, 0x8f}, //index: 234, gain:19.082000DB->x8.997047
	{0x30, 0x8f}, //index: 235, gain:19.035000DB->x8.948495
	{0x30, 0x8e}, //index: 236, gain:18.988000DB->x8.900205
	{0x30, 0x8d}, //index: 237, gain:18.941000DB->x8.852175
	{0x30, 0x8c}, //index: 238, gain:18.894000DB->x8.804405
	{0x30, 0x8c}, //index: 239, gain:18.847000DB->x8.756892
	{0x30, 0x8b}, //index: 240, gain:18.800000DB->x8.709636
	{0x30, 0x8a}, //index: 241, gain:18.753000DB->x8.662635
	{0x30, 0x89}, //index: 242, gain:18.706000DB->x8.615887
	{0x30, 0x89}, //index: 243, gain:18.659000DB->x8.569392
	{0x30, 0x88}, //index: 244, gain:18.612000DB->x8.523147
	{0x30, 0x87}, //index: 245, gain:18.565000DB->x8.477153
	{0x30, 0x86}, //index: 246, gain:18.518000DB->x8.431406
	{0x30, 0x86}, //index: 247, gain:18.471000DB->x8.385906
	{0x30, 0x85}, //index: 248, gain:18.424000DB->x8.340652
	{0x30, 0x84}, //index: 249, gain:18.377000DB->x8.295642
	{0x30, 0x84}, //index: 250, gain:18.330000DB->x8.250875
	{0x30, 0x83}, //index: 251, gain:18.283000DB->x8.206349
	{0x30, 0x82}, //index: 252, gain:18.236000DB->x8.162064
	{0x30, 0x81}, //index: 253, gain:18.189000DB->x8.118018
	{0x30, 0x81}, //index: 254, gain:18.142000DB->x8.074209
	{0x30, 0x80}, //index: 255, gain:18.095000DB->x8.030637
	{0x2c, 0x9f}, //index: 256, gain:18.048000DB->x7.987300
	{0x2c, 0x9e}, //index: 257, gain:18.001000DB->x7.944197
	{0x2c, 0x9d}, //index: 258, gain:17.954000DB->x7.901326
	{0x2c, 0x9c}, //index: 259, gain:17.907000DB->x7.858687
	{0x2c, 0x9c}, //index: 260, gain:17.860000DB->x7.816278
	{0x2c, 0x9b}, //index: 261, gain:17.813000DB->x7.774098
	{0x2c, 0x9a}, //index: 262, gain:17.766000DB->x7.732145
	{0x2c, 0x99}, //index: 263, gain:17.719000DB->x7.690419
	{0x2c, 0x98}, //index: 264, gain:17.672000DB->x7.648918
	{0x2c, 0x97}, //index: 265, gain:17.625000DB->x7.607641
	{0x2c, 0x97}, //index: 266, gain:17.578000DB->x7.566586
	{0x2c, 0x96}, //index: 267, gain:17.531000DB->x7.525754
	{0x2c, 0x95}, //index: 268, gain:17.484000DB->x7.485141
	{0x2c, 0x94}, //index: 269, gain:17.437000DB->x7.444748
	{0x2c, 0x93}, //index: 270, gain:17.390000DB->x7.404573
	{0x2c, 0x93}, //index: 271, gain:17.343000DB->x7.364614
	{0x2c, 0x92}, //index: 272, gain:17.296000DB->x7.324871
	{0x2c, 0x91}, //index: 273, gain:17.249000DB->x7.285343
	{0x2c, 0x90}, //index: 274, gain:17.202000DB->x7.246028
	{0x2c, 0x8f}, //index: 275, gain:17.155000DB->x7.206925
	{0x2c, 0x8f}, //index: 276, gain:17.108000DB->x7.168033
	{0x2c, 0x8e}, //index: 277, gain:17.061000DB->x7.129351
	{0x2c, 0x8d}, //index: 278, gain:17.014000DB->x7.090878
	{0x2c, 0x8c}, //index: 279, gain:16.967000DB->x7.052612
	{0x2c, 0x8c}, //index: 280, gain:16.920000DB->x7.014553
	{0x2c, 0x8b}, //index: 281, gain:16.873000DB->x6.976699
	{0x2c, 0x8a}, //index: 282, gain:16.826000DB->x6.939050
	{0x2c, 0x89}, //index: 283, gain:16.779000DB->x6.901603
	{0x2c, 0x89}, //index: 284, gain:16.732000DB->x6.864359
	{0x2c, 0x88}, //index: 285, gain:16.685000DB->x6.827316
	{0x2c, 0x87}, //index: 286, gain:16.638000DB->x6.790473
	{0x2c, 0x86}, //index: 287, gain:16.591000DB->x6.753828
	{0x2c, 0x86}, //index: 288, gain:16.544000DB->x6.717381
	{0x2c, 0x85}, //index: 289, gain:16.497000DB->x6.681131
	{0x2c, 0x84}, //index: 290, gain:16.450000DB->x6.645077
	{0x2c, 0x83}, //index: 291, gain:16.403000DB->x6.609217
	{0x2c, 0x83}, //index: 292, gain:16.356000DB->x6.573550
	{0x2c, 0x82}, //index: 293, gain:16.309000DB->x6.538077
	{0x2c, 0x81}, //index: 294, gain:16.262000DB->x6.502794
	{0x2c, 0x81}, //index: 295, gain:16.215000DB->x6.467702
	{0x2c, 0x80}, //index: 296, gain:16.168000DB->x6.432799
	{0x28, 0x99}, //index: 297, gain:16.121000DB->x6.398085
	{0x28, 0x98}, //index: 298, gain:16.074000DB->x6.363558
	{0x28, 0x97}, //index: 299, gain:16.027000DB->x6.329217
	{0x28, 0x96}, //index: 300, gain:15.980000DB->x6.295062
	{0x28, 0x96}, //index: 301, gain:15.933000DB->x6.261091
	{0x28, 0x95}, //index: 302, gain:15.886000DB->x6.227303
	{0x28, 0x94}, //index: 303, gain:15.839000DB->x6.193698
	{0x28, 0x93}, //index: 304, gain:15.792000DB->x6.160274
	{0x28, 0x92}, //index: 305, gain:15.745000DB->x6.127030
	{0x28, 0x92}, //index: 306, gain:15.698000DB->x6.093966
	{0x28, 0x91}, //index: 307, gain:15.651000DB->x6.061080
	{0x28, 0x90}, //index: 308, gain:15.604000DB->x6.028371
	{0x28, 0x8f}, //index: 309, gain:15.557000DB->x5.995840
	{0x28, 0x8e}, //index: 310, gain:15.510000DB->x5.963483
	{0x28, 0x8e}, //index: 311, gain:15.463000DB->x5.931301
	{0x28, 0x8d}, //index: 312, gain:15.416000DB->x5.899293
	{0x28, 0x8c}, //index: 313, gain:15.369000DB->x5.867458
	{0x28, 0x8b}, //index: 314, gain:15.322000DB->x5.835795
	{0x28, 0x8b}, //index: 315, gain:15.275000DB->x5.804302
	{0x28, 0x8a}, //index: 316, gain:15.228000DB->x5.772979
	{0x28, 0x89}, //index: 317, gain:15.181000DB->x5.741826
	{0x28, 0x88}, //index: 318, gain:15.134000DB->x5.710840
	{0x28, 0x88}, //index: 319, gain:15.087000DB->x5.680022
	{0x28, 0x87}, //index: 320, gain:15.040000DB->x5.649370
	{0x28, 0x86}, //index: 321, gain:14.993000DB->x5.618883
	{0x28, 0x85}, //index: 322, gain:14.946000DB->x5.588561
	{0x28, 0x85}, //index: 323, gain:14.899000DB->x5.558403
	{0x28, 0x84}, //index: 324, gain:14.852000DB->x5.528407
	{0x28, 0x83}, //index: 325, gain:14.805000DB->x5.498573
	{0x28, 0x83}, //index: 326, gain:14.758000DB->x5.468900
	{0x28, 0x82}, //index: 327, gain:14.711000DB->x5.439388
	{0x28, 0x81}, //index: 328, gain:14.664000DB->x5.410034
	{0x28, 0x80}, //index: 329, gain:14.617000DB->x5.380839
	{0x28, 0x80}, //index: 330, gain:14.570000DB->x5.351802
	{0x24, 0x95}, //index: 331, gain:14.523000DB->x5.322921
	{0x24, 0x94}, //index: 332, gain:14.476000DB->x5.294196
	{0x24, 0x93}, //index: 333, gain:14.429000DB->x5.265626
	{0x24, 0x93}, //index: 334, gain:14.382000DB->x5.237210
	{0x24, 0x92}, //index: 335, gain:14.335000DB->x5.208948
	{0x24, 0x91}, //index: 336, gain:14.288000DB->x5.180838
	{0x24, 0x90}, //index: 337, gain:14.241000DB->x5.152880
	{0x24, 0x8f}, //index: 338, gain:14.194000DB->x5.125072
	{0x24, 0x8f}, //index: 339, gain:14.147000DB->x5.097415
	{0x24, 0x8e}, //index: 340, gain:14.100000DB->x5.069907
	{0x24, 0x8d}, //index: 341, gain:14.053000DB->x5.042548
	{0x24, 0x8c}, //index: 342, gain:14.006000DB->x5.015336
	{0x24, 0x8c}, //index: 343, gain:13.959000DB->x4.988271
	{0x24, 0x8b}, //index: 344, gain:13.912000DB->x4.961352
	{0x24, 0x8a}, //index: 345, gain:13.865000DB->x4.934578
	{0x24, 0x89}, //index: 346, gain:13.818000DB->x4.907949
	{0x24, 0x89}, //index: 347, gain:13.771000DB->x4.881463
	{0x24, 0x88}, //index: 348, gain:13.724000DB->x4.855120
	{0x24, 0x87}, //index: 349, gain:13.677000DB->x4.828920
	{0x24, 0x86}, //index: 350, gain:13.630000DB->x4.802861
	{0x24, 0x86}, //index: 351, gain:13.583000DB->x4.776942
	{0x24, 0x85}, //index: 352, gain:13.536000DB->x4.751164
	{0x24, 0x84}, //index: 353, gain:13.489000DB->x4.725524
	{0x24, 0x83}, //index: 354, gain:13.442000DB->x4.700023
	{0x24, 0x83}, //index: 355, gain:13.395000DB->x4.674660
	{0x24, 0x82}, //index: 356, gain:13.348000DB->x4.649433
	{0x24, 0x81}, //index: 357, gain:13.301000DB->x4.624343
	{0x24, 0x81}, //index: 358, gain:13.254000DB->x4.599387
	{0x24, 0x80}, //index: 359, gain:13.207000DB->x4.574567
	{0x20, 0x91}, //index: 360, gain:13.160000DB->x4.549881
	{0x20, 0x90}, //index: 361, gain:13.113000DB->x4.525327
	{0x20, 0x90}, //index: 362, gain:13.066000DB->x4.500907
	{0x20, 0x8f}, //index: 363, gain:13.019000DB->x4.476618
	{0x20, 0x8e}, //index: 364, gain:12.972000DB->x4.452460
	{0x20, 0x8d}, //index: 365, gain:12.925000DB->x4.428432
	{0x20, 0x8c}, //index: 366, gain:12.878000DB->x4.404534
	{0x20, 0x8c}, //index: 367, gain:12.831000DB->x4.380765
	{0x20, 0x8b}, //index: 368, gain:12.784000DB->x4.357125
	{0x20, 0x8a}, //index: 369, gain:12.737000DB->x4.333612
	{0x20, 0x89}, //index: 370, gain:12.690000DB->x4.310226
	{0x20, 0x89}, //index: 371, gain:12.643000DB->x4.286966
	{0x20, 0x88}, //index: 372, gain:12.596000DB->x4.263831
	{0x20, 0x87}, //index: 373, gain:12.549000DB->x4.240822
	{0x20, 0x86}, //index: 374, gain:12.502000DB->x4.217936
	{0x20, 0x86}, //index: 375, gain:12.455000DB->x4.195174
	{0x20, 0x85}, //index: 376, gain:12.408000DB->x4.172535
	{0x20, 0x84}, //index: 377, gain:12.361000DB->x4.150018
	{0x20, 0x84}, //index: 378, gain:12.314000DB->x4.127623
	{0x20, 0x83}, //index: 379, gain:12.267000DB->x4.105348
	{0x20, 0x82}, //index: 380, gain:12.220000DB->x4.083194
	{0x20, 0x81}, //index: 381, gain:12.173000DB->x4.061159
	{0x20, 0x81}, //index: 382, gain:12.126000DB->x4.039243
	{0x20, 0x80}, //index: 383, gain:12.079000DB->x4.017446
	{0x1e, 0x8f}, //index: 384, gain:12.032000DB->x3.995766
	{0x1e, 0x8e}, //index: 385, gain:11.985000DB->x3.974203
	{0x1e, 0x8e}, //index: 386, gain:11.938000DB->x3.952756
	{0x1e, 0x8d}, //index: 387, gain:11.891000DB->x3.931425
	{0x1e, 0x8c}, //index: 388, gain:11.844000DB->x3.910209
	{0x1e, 0x8b}, //index: 389, gain:11.797000DB->x3.889108
	{0x1e, 0x8b}, //index: 390, gain:11.750000DB->x3.868121
	{0x1e, 0x8a}, //index: 391, gain:11.703000DB->x3.847246
	{0x1e, 0x89}, //index: 392, gain:11.656000DB->x3.826485
	{0x1e, 0x88}, //index: 393, gain:11.609000DB->x3.805835
	{0x1e, 0x88}, //index: 394, gain:11.562000DB->x3.785297
	{0x1e, 0x87}, //index: 395, gain:11.515000DB->x3.764870
	{0x1e, 0x86}, //index: 396, gain:11.468000DB->x3.744553
	{0x1e, 0x85}, //index: 397, gain:11.421000DB->x3.724346
	{0x1e, 0x85}, //index: 398, gain:11.374000DB->x3.704248
	{0x1e, 0x84}, //index: 399, gain:11.327000DB->x3.684258
	{0x1e, 0x83}, //index: 400, gain:11.280000DB->x3.664376
	{0x1e, 0x83}, //index: 401, gain:11.233000DB->x3.644601
	{0x1e, 0x82}, //index: 402, gain:11.186000DB->x3.624933
	{0x1e, 0x81}, //index: 403, gain:11.139000DB->x3.605371
	{0x1e, 0x80}, //index: 404, gain:11.092000DB->x3.585915
	{0x1e, 0x80}, //index: 405, gain:11.045000DB->x3.566564
	{0x1c, 0x8d}, //index: 406, gain:10.998000DB->x3.547317
	{0x1c, 0x8d}, //index: 407, gain:10.951000DB->x3.528174
	{0x1c, 0x8c}, //index: 408, gain:10.904000DB->x3.509134
	{0x1c, 0x8b}, //index: 409, gain:10.857000DB->x3.490197
	{0x1c, 0x8a}, //index: 410, gain:10.810000DB->x3.471363
	{0x1c, 0x8a}, //index: 411, gain:10.763000DB->x3.452630
	{0x1c, 0x89}, //index: 412, gain:10.716000DB->x3.433998
	{0x1c, 0x88}, //index: 413, gain:10.669000DB->x3.415466
	{0x1c, 0x87}, //index: 414, gain:10.622000DB->x3.397035
	{0x1c, 0x87}, //index: 415, gain:10.575000DB->x3.378703
	{0x1c, 0x86}, //index: 416, gain:10.528000DB->x3.360470
	{0x1c, 0x85}, //index: 417, gain:10.481000DB->x3.342335
	{0x1c, 0x84}, //index: 418, gain:10.434000DB->x3.324298
	{0x1c, 0x84}, //index: 419, gain:10.387000DB->x3.306359
	{0x1c, 0x83}, //index: 420, gain:10.340000DB->x3.288516
	{0x1c, 0x82}, //index: 421, gain:10.293000DB->x3.270770
	{0x1c, 0x82}, //index: 422, gain:10.246000DB->x3.253119
	{0x1c, 0x81}, //index: 423, gain:10.199000DB->x3.235564
	{0x1c, 0x80}, //index: 424, gain:10.152000DB->x3.218103
	{0x1c, 0x80}, //index: 425, gain:10.105000DB->x3.200737
	{0x1a, 0x8c}, //index: 426, gain:10.058000DB->x3.183464
	{0x1a, 0x8b}, //index: 427, gain:10.011000DB->x3.166285
	{0x1a, 0x8a}, //index: 428, gain:9.964000DB->x3.149198
	{0x1a, 0x89}, //index: 429, gain:9.917000DB->x3.132204
	{0x1a, 0x89}, //index: 430, gain:9.870000DB->x3.115301
	{0x1a, 0x88}, //index: 431, gain:9.823000DB->x3.098489
	{0x1a, 0x87}, //index: 432, gain:9.776000DB->x3.081768
	{0x1a, 0x86}, //index: 433, gain:9.729000DB->x3.065138
	{0x1a, 0x86}, //index: 434, gain:9.682000DB->x3.048597
	{0x1a, 0x85}, //index: 435, gain:9.635000DB->x3.032145
	{0x1a, 0x84}, //index: 436, gain:9.588000DB->x3.015782
	{0x1a, 0x83}, //index: 437, gain:9.541000DB->x2.999508
	{0x1a, 0x83}, //index: 438, gain:9.494000DB->x2.983321
	{0x1a, 0x82}, //index: 439, gain:9.447000DB->x2.967222
	{0x1a, 0x81}, //index: 440, gain:9.400000DB->x2.951209
	{0x1a, 0x81}, //index: 441, gain:9.353000DB->x2.935283
	{0x1a, 0x80}, //index: 442, gain:9.306000DB->x2.919443
	{0x18, 0x8b}, //index: 443, gain:9.259000DB->x2.903688
	{0x18, 0x8a}, //index: 444, gain:9.212000DB->x2.888019
	{0x18, 0x89}, //index: 445, gain:9.165000DB->x2.872434
	{0x18, 0x88}, //index: 446, gain:9.118000DB->x2.856933
	{0x18, 0x88}, //index: 447, gain:9.071000DB->x2.841515
	{0x18, 0x87}, //index: 448, gain:9.024000DB->x2.826181
	{0x18, 0x86}, //index: 449, gain:8.977000DB->x2.810930
	{0x18, 0x86}, //index: 450, gain:8.930000DB->x2.795761
	{0x18, 0x85}, //index: 451, gain:8.883000DB->x2.780674
	{0x18, 0x84}, //index: 452, gain:8.836000DB->x2.765668
	{0x18, 0x83}, //index: 453, gain:8.789000DB->x2.750743
	{0x18, 0x83}, //index: 454, gain:8.742000DB->x2.735899
	{0x18, 0x82}, //index: 455, gain:8.695000DB->x2.721134
	{0x18, 0x81}, //index: 456, gain:8.648000DB->x2.706450
	{0x18, 0x81}, //index: 457, gain:8.601000DB->x2.691845
	{0x18, 0x80}, //index: 458, gain:8.554000DB->x2.677318
	{0x16, 0x89}, //index: 459, gain:8.507000DB->x2.662870
	{0x16, 0x89}, //index: 460, gain:8.460000DB->x2.648500
	{0x16, 0x88}, //index: 461, gain:8.413000DB->x2.634208
	{0x16, 0x87}, //index: 462, gain:8.366000DB->x2.619992
	{0x16, 0x87}, //index: 463, gain:8.319000DB->x2.605854
	{0x16, 0x86}, //index: 464, gain:8.272000DB->x2.591791
	{0x16, 0x85}, //index: 465, gain:8.225000DB->x2.577805
	{0x16, 0x84}, //index: 466, gain:8.178000DB->x2.563894
	{0x16, 0x84}, //index: 467, gain:8.131000DB->x2.550058
	{0x16, 0x83}, //index: 468, gain:8.084000DB->x2.536296
	{0x16, 0x82}, //index: 469, gain:8.037000DB->x2.522609
	{0x16, 0x82}, //index: 470, gain:7.990000DB->x2.508996
	{0x16, 0x81}, //index: 471, gain:7.943000DB->x2.495456
	{0x16, 0x80}, //index: 472, gain:7.896000DB->x2.481990
	{0x14, 0x8a}, //index: 473, gain:7.849000DB->x2.468596
	{0x14, 0x89}, //index: 474, gain:7.802000DB->x2.455274
	{0x14, 0x89}, //index: 475, gain:7.755000DB->x2.442024
	{0x14, 0x88}, //index: 476, gain:7.708000DB->x2.428846
	{0x14, 0x87}, //index: 477, gain:7.661000DB->x2.415739
	{0x14, 0x86}, //index: 478, gain:7.614000DB->x2.402702
	{0x14, 0x86}, //index: 479, gain:7.567000DB->x2.389736
	{0x14, 0x85}, //index: 480, gain:7.520000DB->x2.376840
	{0x14, 0x84}, //index: 481, gain:7.473000DB->x2.364014
	{0x14, 0x84}, //index: 482, gain:7.426000DB->x2.351256
	{0x14, 0x83}, //index: 483, gain:7.379000DB->x2.338568
	{0x14, 0x82}, //index: 484, gain:7.332000DB->x2.325948
	{0x14, 0x81}, //index: 485, gain:7.285000DB->x2.313396
	{0x14, 0x81}, //index: 486, gain:7.238000DB->x2.300912
	{0x14, 0x80}, //index: 487, gain:7.191000DB->x2.288495
	{0x12, 0x88}, //index: 488, gain:7.144000DB->x2.276145
	{0x12, 0x87}, //index: 489, gain:7.097000DB->x2.263862
	{0x12, 0x86}, //index: 490, gain:7.050000DB->x2.251645
	{0x12, 0x85}, //index: 491, gain:7.003000DB->x2.239494
	{0x12, 0x85}, //index: 492, gain:6.956000DB->x2.227409
	{0x12, 0x84}, //index: 493, gain:6.909000DB->x2.215389
	{0x12, 0x83}, //index: 494, gain:6.862000DB->x2.203434
	{0x12, 0x83}, //index: 495, gain:6.815000DB->x2.191543
	{0x12, 0x82}, //index: 496, gain:6.768000DB->x2.179716
	{0x12, 0x81}, //index: 497, gain:6.721000DB->x2.167954
	{0x12, 0x80}, //index: 498, gain:6.674000DB->x2.156254
	{0x12, 0x80}, //index: 499, gain:6.627000DB->x2.144618
	{0x10, 0x88}, //index: 500, gain:6.580000DB->x2.133045
	{0x10, 0x87}, //index: 501, gain:6.533000DB->x2.121534
	{0x10, 0x87}, //index: 502, gain:6.486000DB->x2.110085
	{0x10, 0x86}, //index: 503, gain:6.439000DB->x2.098698
	{0x10, 0x85}, //index: 504, gain:6.392000DB->x2.087373
	{0x10, 0x84}, //index: 505, gain:6.345000DB->x2.076108
	{0x10, 0x84}, //index: 506, gain:6.298000DB->x2.064905
	{0x10, 0x83}, //index: 507, gain:6.251000DB->x2.053761
	{0x10, 0x82}, //index: 508, gain:6.204000DB->x2.042678
	{0x10, 0x82}, //index: 509, gain:6.157000DB->x2.031655
	{0x10, 0x81}, //index: 510, gain:6.110000DB->x2.020691
	{0x10, 0x80}, //index: 511, gain:6.063000DB->x2.009787
	{0x0f, 0x88}, //index: 512, gain:6.016000DB->x1.998941
	{0x0f, 0x87}, //index: 513, gain:5.969000DB->x1.988154
	{0x0f, 0x86}, //index: 514, gain:5.922000DB->x1.977425
	{0x0f, 0x85}, //index: 515, gain:5.875000DB->x1.966754
	{0x0f, 0x85}, //index: 516, gain:5.828000DB->x1.956140
	{0x0f, 0x84}, //index: 517, gain:5.781000DB->x1.945584
	{0x0f, 0x83}, //index: 518, gain:5.734000DB->x1.935085
	{0x0f, 0x83}, //index: 519, gain:5.687000DB->x1.924642
	{0x0f, 0x82}, //index: 520, gain:5.640000DB->x1.914256
	{0x0f, 0x81}, //index: 521, gain:5.593000DB->x1.903926
	{0x0f, 0x80}, //index: 522, gain:5.546000DB->x1.893651
	{0x0f, 0x80}, //index: 523, gain:5.499000DB->x1.883432
	{0x0e, 0x86}, //index: 524, gain:5.452000DB->x1.873268
	{0x0e, 0x85}, //index: 525, gain:5.405000DB->x1.863159
	{0x0e, 0x85}, //index: 526, gain:5.358000DB->x1.853105
	{0x0e, 0x84}, //index: 527, gain:5.311000DB->x1.843105
	{0x0e, 0x83}, //index: 528, gain:5.264000DB->x1.833158
	{0x0e, 0x83}, //index: 529, gain:5.217000DB->x1.823266
	{0x0e, 0x82}, //index: 530, gain:5.170000DB->x1.813427
	{0x0e, 0x81}, //index: 531, gain:5.123000DB->x1.803641
	{0x0e, 0x81}, //index: 532, gain:5.076000DB->x1.793907
	{0x0e, 0x80}, //index: 533, gain:5.029000DB->x1.784227
	{0x0d, 0x86}, //index: 534, gain:4.982000DB->x1.774598
	{0x0d, 0x85}, //index: 535, gain:4.935000DB->x1.765022
	{0x0d, 0x84}, //index: 536, gain:4.888000DB->x1.755497
	{0x0d, 0x84}, //index: 537, gain:4.841000DB->x1.746023
	{0x0d, 0x83}, //index: 538, gain:4.794000DB->x1.736601
	{0x0d, 0x82}, //index: 539, gain:4.747000DB->x1.727229
	{0x0d, 0x82}, //index: 540, gain:4.700000DB->x1.717908
	{0x0d, 0x81}, //index: 541, gain:4.653000DB->x1.708638
	{0x0d, 0x80}, //index: 542, gain:4.606000DB->x1.699417
	{0x0d, 0x80}, //index: 543, gain:4.559000DB->x1.690246
	{0x0c, 0x86}, //index: 544, gain:4.512000DB->x1.681125
	{0x0c, 0x85}, //index: 545, gain:4.465000DB->x1.672053
	{0x0c, 0x85}, //index: 546, gain:4.418000DB->x1.663030
	{0x0c, 0x84}, //index: 547, gain:4.371000DB->x1.654055
	{0x0c, 0x83}, //index: 548, gain:4.324000DB->x1.645129
	{0x0c, 0x82}, //index: 549, gain:4.277000DB->x1.636251
	{0x0c, 0x82}, //index: 550, gain:4.230000DB->x1.627421
	{0x0c, 0x81}, //index: 551, gain:4.183000DB->x1.618639
	{0x0c, 0x80}, //index: 552, gain:4.136000DB->x1.609904
	{0x0c, 0x80}, //index: 553, gain:4.089000DB->x1.601216
	{0x0b, 0x86}, //index: 554, gain:4.042000DB->x1.592575
	{0x0b, 0x85}, //index: 555, gain:3.995000DB->x1.583981
	{0x0b, 0x84}, //index: 556, gain:3.948000DB->x1.575433
	{0x0b, 0x83}, //index: 557, gain:3.901000DB->x1.566931
	{0x0b, 0x83}, //index: 558, gain:3.854000DB->x1.558476
	{0x0b, 0x82}, //index: 559, gain:3.807000DB->x1.550065
	{0x0b, 0x81}, //index: 560, gain:3.760000DB->x1.541700
	{0x0b, 0x81}, //index: 561, gain:3.713000DB->x1.533381
	{0x0b, 0x80}, //index: 562, gain:3.666000DB->x1.525106
	{0x0a, 0x85}, //index: 563, gain:3.619000DB->x1.516876
	{0x0a, 0x85}, //index: 564, gain:3.572000DB->x1.508690
	{0x0a, 0x84}, //index: 565, gain:3.525000DB->x1.500548
	{0x0a, 0x83}, //index: 566, gain:3.478000DB->x1.492451
	{0x0a, 0x83}, //index: 567, gain:3.431000DB->x1.484397
	{0x0a, 0x82}, //index: 568, gain:3.384000DB->x1.476386
	{0x0a, 0x81}, //index: 569, gain:3.337000DB->x1.468419
	{0x0a, 0x80}, //index: 570, gain:3.290000DB->x1.460495
	{0x0a, 0x80}, //index: 571, gain:3.243000DB->x1.452613
	{0x09, 0x85}, //index: 572, gain:3.196000DB->x1.444774
	{0x09, 0x84}, //index: 573, gain:3.149000DB->x1.436978
	{0x09, 0x83}, //index: 574, gain:3.102000DB->x1.429223
	{0x09, 0x82}, //index: 575, gain:3.055000DB->x1.421510
	{0x09, 0x82}, //index: 576, gain:3.008000DB->x1.413839
	{0x09, 0x81}, //index: 577, gain:2.961000DB->x1.406209
	{0x09, 0x80}, //index: 578, gain:2.914000DB->x1.398621
	{0x09, 0x80}, //index: 579, gain:2.867000DB->x1.391073
	{0x08, 0x84}, //index: 580, gain:2.820000DB->x1.383566
	{0x08, 0x83}, //index: 581, gain:2.773000DB->x1.376100
	{0x08, 0x82}, //index: 582, gain:2.726000DB->x1.368674
	{0x08, 0x82}, //index: 583, gain:2.679000DB->x1.361288
	{0x08, 0x81}, //index: 584, gain:2.632000DB->x1.353942
	{0x08, 0x80}, //index: 585, gain:2.585000DB->x1.346635
	{0x07, 0x85}, //index: 586, gain:2.538000DB->x1.339368
	{0x07, 0x85}, //index: 587, gain:2.491000DB->x1.332140
	{0x07, 0x84}, //index: 588, gain:2.444000DB->x1.324952
	{0x07, 0x83}, //index: 589, gain:2.397000DB->x1.317802
	{0x07, 0x83}, //index: 590, gain:2.350000DB->x1.310690
	{0x07, 0x82}, //index: 591, gain:2.303000DB->x1.303617
	{0x07, 0x81}, //index: 592, gain:2.256000DB->x1.296582
	{0x07, 0x80}, //index: 593, gain:2.209000DB->x1.289585
	{0x07, 0x80}, //index: 594, gain:2.162000DB->x1.282626
	{0x06, 0x84}, //index: 595, gain:2.115000DB->x1.275704
	{0x06, 0x84}, //index: 596, gain:2.068000DB->x1.268820
	{0x06, 0x83}, //index: 597, gain:2.021000DB->x1.261973
	{0x06, 0x82}, //index: 598, gain:1.974000DB->x1.255163
	{0x06, 0x81}, //index: 599, gain:1.927000DB->x1.248389
	{0x06, 0x81}, //index: 600, gain:1.880000DB->x1.241652
	{0x06, 0x80}, //index: 601, gain:1.833000DB->x1.234952
};

