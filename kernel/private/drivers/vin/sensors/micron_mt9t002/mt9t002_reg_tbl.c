/*
 * Filename : mt9t002_reg_tbl.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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

/* ========================================================================== */
/*   < rember to update MT9T002_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
/*   < rember to update MT9T002_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct mt9t002_video_format_reg_table mt9t002_video_format_tbl = {
	.reg = {
		/* add video format related register here */
		0x3004,		//X_ADDR_START
		0x3008,		//X_ADDR_END
		0x3002,		//Y_ADDR_START
		0x3006,		//Y_ADDR_END
		0x30A2,		//X_ODD_INCREMENT
		0x30A6,		//Y_ODD_INCREMENT
		0x300C,		//LINE_LENGTH_PCK
		0x300A,		//FRAME_LENGTH_LINE
		0x3014,		//FINE_INTEGRATION_TIME
		0x3012,		//Coarse_Integration_Time
		0x3042,		//EXTRA_DELAY
		0x3040		//
		},
	.table[0] = {		//2304X1296@60p
		.ext_reg_fill = NULL,
		.data = {
			0,
			2303,
			120,
			1415,
			1,
			1,
			1248,
			1308,
			0,
			1308,
			0,
			0
		},
		.width	= 2304,
		.height	= 1296,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
#if (CHIP_REV == A5S)// workaround: A5s can only support 2304x1296@30p
		.max_fps = AMBA_VIDEO_FPS_30,
#else
		.max_fps = AMBA_VIDEO_FPS_60,
#endif
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 3,
		},
	.table[1] = {		//1920x1080@60p
		.ext_reg_fill = NULL,
		.data = {
			192,
			2111,
			228,
			1307,
			1,
			1,
			1484,
			1102,
			0,
			408,
			0,
			0
		},
		.width	= 1920,
		.height	= 1080,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 11,
		},
	.table[2] = {		//2304x1536@30p
		.ext_reg_fill = NULL,
		.data = {
			0,
			2303,
			0,
			1535,
			1,
			1,
			2114,
			1548,
			0,
			408,
			0,
			0
		},
		.width	= 2304,
		.height	= 1536,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 7,
		},
	.table[3] = {		//2048x1536@30p
		.ext_reg_fill = NULL,
		.data = {
			128,
			2175,
			0,
			1535,
			1,
			1,
			1504,
			2174,
			272,
			2169,
			0,
			0
		},
		.width	= 2048,
		.height	= 1536,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 5,
		},
	.table[4] = {		//1152x648@120p
		.ext_reg_fill = NULL,
		.data = {
			0,
			2303,
			120,
			1415,
			3,
			3,
			1248,
			653,
			0,
			1250,
			0,
			0x3000
		},
		.width	= 1152,
		.height	= 648,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 3,
		},
	.table[5] = {		//768x512@30p 3x digital binning
		.ext_reg_fill = NULL,
		.data = {
			0,
			2303,
			0,
			1535,
			0x5,
			0x5,
			2114,
			1548,
			0,
			408,
			0,
			0x3300
		},
		.width	= 768,
		.height	= 512,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits	= AMBA_VIDEO_BITS_12,
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 7,
		},
		/* add video format table here, if necessary */
};


/** MT9T002 global gain table row size */

#define MT9T002_GAIN_COL_AGC		(0)
#define MT9T002_GAIN_COL_REG_AGAIN	(1)
#define MT9T002_GAIN_COL_REG_DGAIN	(2)

#if GAIN_128_STEPS /* 64 steps */

#define MT9T002_GAIN_ROWS		(897)
#define MT9T002_GAIN_COLS		(3)
#define MT9T002_GAIN_DOUBLE		(128)
#define MT9T002_GAIN_0DB		(896)
/* 128 steps*/
const s16 MT9T002_GAIN_TABLE[MT9T002_GAIN_ROWS][MT9T002_GAIN_COLS] =
{
	{ 0x8000 , 0x30 , 0x7FF },		/*index: 0, 42.1442dB */
	{ 0x7F4F , 0x30 , 0x7F5 },		/*index: 1, 42.0972dB */
	{ 0x7E9F , 0x30 , 0x7EA },		/*index: 2, 42.0501dB */
	{ 0x7DF0 , 0x30 , 0x7DF },		/*index: 3, 42.0031dB */
	{ 0x7D42 , 0x30 , 0x7D4 },		/*index: 4, 41.9561dB */
	{ 0x7C95 , 0x30 , 0x7C9 },		/*index: 5, 41.909dB */
	{ 0x7BE8 , 0x30 , 0x7BF },		/*index: 6, 41.862dB */
	{ 0x7B3D , 0x30 , 0x7B4 },		/*index: 7, 41.8149dB */
	{ 0x7A93 , 0x30 , 0x7A9 },		/*index: 8, 41.7679dB */
	{ 0x79E9 , 0x30 , 0x79F },		/*index: 9, 41.7209dB */
	{ 0x7941 , 0x30 , 0x794 },		/*index: 10, 41.6738dB */
	{ 0x7899 , 0x30 , 0x78A },		/*index: 11, 41.6268dB */
	{ 0x77F2 , 0x30 , 0x77F },		/*index: 12, 41.5798dB */
	{ 0x774D , 0x30 , 0x775 },		/*index: 13, 41.5327dB */
	{ 0x76A8 , 0x30 , 0x76A },		/*index: 14, 41.4857dB */
	{ 0x7604 , 0x30 , 0x760 },		/*index: 15, 41.4387dB */
	{ 0x7560 , 0x30 , 0x756 },		/*index: 16, 41.3916dB */
	{ 0x74BE , 0x30 , 0x74C },		/*index: 17, 41.3446dB */
	{ 0x741D , 0x30 , 0x742 },		/*index: 18, 41.2976dB */
	{ 0x737C , 0x30 , 0x738 },		/*index: 19, 41.2505dB */
	{ 0x72DD , 0x30 , 0x72E },		/*index: 20, 41.2035dB */
	{ 0x723E , 0x30 , 0x724 },		/*index: 21, 41.1564dB */
	{ 0x71A0 , 0x30 , 0x71A },		/*index: 22, 41.1094dB */
	{ 0x7103 , 0x30 , 0x710 },		/*index: 23, 41.0624dB */
	{ 0x7066 , 0x30 , 0x706 },		/*index: 24, 41.0153dB */
	{ 0x6FCB , 0x30 , 0x6FD },		/*index: 25, 40.9683dB */
	{ 0x6F30 , 0x30 , 0x6F3 },		/*index: 26, 40.9213dB */
	{ 0x6E97 , 0x30 , 0x6E9 },		/*index: 27, 40.8742dB */
	{ 0x6DFE , 0x30 , 0x6E0 },		/*index: 28, 40.8272dB */
	{ 0x6D66 , 0x30 , 0x6D6 },		/*index: 29, 40.7802dB */
	{ 0x6CCF , 0x30 , 0x6CD },		/*index: 30, 40.7331dB */
	{ 0x6C38 , 0x30 , 0x6C4 },		/*index: 31, 40.6861dB */
	{ 0x6BA2 , 0x30 , 0x6BA },		/*index: 32, 40.639dB */
	{ 0x6B0E , 0x30 , 0x6B1 },		/*index: 33, 40.592dB */
	{ 0x6A7A , 0x30 , 0x6A8 },		/*index: 34, 40.545dB */
	{ 0x69E6 , 0x30 , 0x69E },		/*index: 35, 40.4979dB */
	{ 0x6954 , 0x30 , 0x695 },		/*index: 36, 40.4509dB */
	{ 0x68C2 , 0x30 , 0x68C },		/*index: 37, 40.4039dB */
	{ 0x6832 , 0x30 , 0x683 },		/*index: 38, 40.3568dB */
	{ 0x67A2 , 0x30 , 0x67A },		/*index: 39, 40.3098dB */
	{ 0x6712 , 0x30 , 0x671 },		/*index: 40, 40.2628dB */
	{ 0x6684 , 0x30 , 0x668 },		/*index: 41, 40.2157dB */
	{ 0x65F6 , 0x30 , 0x65F },		/*index: 42, 40.1687dB */
	{ 0x6569 , 0x30 , 0x657 },		/*index: 43, 40.1217dB */
	{ 0x64DD , 0x30 , 0x64E },		/*index: 44, 40.0746dB */
	{ 0x6451 , 0x30 , 0x645 },		/*index: 45, 40.0276dB */
	{ 0x63C7 , 0x30 , 0x63C },		/*index: 46, 39.9805dB */
	{ 0x633D , 0x30 , 0x634 },		/*index: 47, 39.9335dB */
	{ 0x62B4 , 0x30 , 0x62B },		/*index: 48, 39.8865dB */
	{ 0x622B , 0x30 , 0x623 },		/*index: 49, 39.8394dB */
	{ 0x61A3 , 0x30 , 0x61A },		/*index: 50, 39.7924dB */
	{ 0x611C , 0x30 , 0x612 },		/*index: 51, 39.7454dB */
	{ 0x6096 , 0x30 , 0x609 },		/*index: 52, 39.6983dB */
	{ 0x6011 , 0x30 , 0x601 },		/*index: 53, 39.6513dB */
	{ 0x5F8C , 0x30 , 0x5F9 },		/*index: 54, 39.6043dB */
	{ 0x5F08 , 0x30 , 0x5F0 },		/*index: 55, 39.5572dB */
	{ 0x5E84 , 0x30 , 0x5E8 },		/*index: 56, 39.5102dB */
	{ 0x5E02 , 0x30 , 0x5E0 },		/*index: 57, 39.4632dB */
	{ 0x5D80 , 0x30 , 0x5D8 },		/*index: 58, 39.4161dB */
	{ 0x5CFE , 0x30 , 0x5D0 },		/*index: 59, 39.3691dB */
	{ 0x5C7E , 0x30 , 0x5C8 },		/*index: 60, 39.322dB */
	{ 0x5BFE , 0x30 , 0x5C0 },		/*index: 61, 39.275dB */
	{ 0x5B7F , 0x30 , 0x5B8 },		/*index: 62, 39.228dB */
	{ 0x5B00 , 0x30 , 0x5B0 },		/*index: 63, 39.1809dB */
	{ 0x5A82 , 0x30 , 0x5A8 },		/*index: 64, 39.1339dB */
	{ 0x5A05 , 0x30 , 0x5A0 },		/*index: 65, 39.0869dB */
	{ 0x5989 , 0x30 , 0x599 },		/*index: 66, 39.0398dB */
	{ 0x590D , 0x30 , 0x591 },		/*index: 67, 38.9928dB */
	{ 0x5892 , 0x30 , 0x589 },		/*index: 68, 38.9458dB */
	{ 0x5818 , 0x30 , 0x581 },		/*index: 69, 38.8987dB */
	{ 0x579E , 0x30 , 0x57A },		/*index: 70, 38.8517dB */
	{ 0x5725 , 0x30 , 0x572 },		/*index: 71, 38.8046dB */
	{ 0x56AC , 0x30 , 0x56B },		/*index: 72, 38.7576dB */
	{ 0x5634 , 0x30 , 0x563 },		/*index: 73, 38.7106dB */
	{ 0x55BD , 0x30 , 0x55C },		/*index: 74, 38.6635dB */
	{ 0x5547 , 0x30 , 0x554 },		/*index: 75, 38.6165dB */
	{ 0x54D1 , 0x30 , 0x54D },		/*index: 76, 38.5695dB */
	{ 0x545B , 0x30 , 0x546 },		/*index: 77, 38.5224dB */
	{ 0x53E7 , 0x30 , 0x53E },		/*index: 78, 38.4754dB */
	{ 0x5373 , 0x30 , 0x537 },		/*index: 79, 38.4284dB */
	{ 0x52FF , 0x30 , 0x530 },		/*index: 80, 38.3813dB */
	{ 0x528D , 0x30 , 0x529 },		/*index: 81, 38.3343dB */
	{ 0x521B , 0x30 , 0x522 },		/*index: 82, 38.2873dB */
	{ 0x51A9 , 0x30 , 0x51B },		/*index: 83, 38.2402dB */
	{ 0x5138 , 0x30 , 0x514 },		/*index: 84, 38.1932dB */
	{ 0x50C8 , 0x30 , 0x50C },		/*index: 85, 38.1461dB */
	{ 0x5058 , 0x30 , 0x506 },		/*index: 86, 38.0991dB */
	{ 0x4FE9 , 0x30 , 0x4FF },		/*index: 87, 38.0521dB */
	{ 0x4F7B , 0x30 , 0x4F8 },		/*index: 88, 38.005dB */
	{ 0x4F0D , 0x30 , 0x4F1 },		/*index: 89, 37.958dB */
	{ 0x4E9F , 0x30 , 0x4EA },		/*index: 90, 37.911dB */
	{ 0x4E33 , 0x30 , 0x4E3 },		/*index: 91, 37.8639dB */
	{ 0x4DC7 , 0x30 , 0x4DC },		/*index: 92, 37.8169dB */
	{ 0x4D5B , 0x30 , 0x4D6 },		/*index: 93, 37.7699dB */
	{ 0x4CF0 , 0x30 , 0x4CF },		/*index: 94, 37.7228dB */
	{ 0x4C86 , 0x30 , 0x4C8 },		/*index: 95, 37.6758dB */
	{ 0x4C1C , 0x30 , 0x4C2 },		/*index: 96, 37.6287dB */
	{ 0x4BB3 , 0x30 , 0x4BB },		/*index: 97, 37.5817dB */
	{ 0x4B4A , 0x30 , 0x4B5 },		/*index: 98, 37.5347dB */
	{ 0x4AE2 , 0x30 , 0x4AE },		/*index: 99, 37.4876dB */
	{ 0x4A7A , 0x30 , 0x4A8 },		/*index: 100, 37.4406dB */
	{ 0x4A13 , 0x30 , 0x4A1 },		/*index: 101, 37.3936dB */
	{ 0x49AD , 0x30 , 0x49B },		/*index: 102, 37.3465dB */
	{ 0x4947 , 0x30 , 0x494 },		/*index: 103, 37.2995dB */
	{ 0x48E2 , 0x30 , 0x48E },		/*index: 104, 37.2525dB */
	{ 0x487D , 0x30 , 0x488 },		/*index: 105, 37.2054dB */
	{ 0x4819 , 0x30 , 0x482 },		/*index: 106, 37.1584dB */
	{ 0x47B5 , 0x30 , 0x47B },		/*index: 107, 37.1114dB */
	{ 0x4752 , 0x30 , 0x475 },		/*index: 108, 37.0643dB */
	{ 0x46F0 , 0x30 , 0x46F },		/*index: 109, 37.0173dB */
	{ 0x468D , 0x30 , 0x469 },		/*index: 110, 36.9702dB */
	{ 0x462C , 0x30 , 0x463 },		/*index: 111, 36.9232dB */
	{ 0x45CB , 0x30 , 0x45D },		/*index: 112, 36.8762dB */
	{ 0x456A , 0x30 , 0x457 },		/*index: 113, 36.8291dB */
	{ 0x450A , 0x30 , 0x451 },		/*index: 114, 36.7821dB */
	{ 0x44AB , 0x30 , 0x44B },		/*index: 115, 36.7351dB */
	{ 0x444C , 0x30 , 0x445 },		/*index: 116, 36.688dB */
	{ 0x43EE , 0x30 , 0x43F },		/*index: 117, 36.641dB */
	{ 0x4390 , 0x30 , 0x439 },		/*index: 118, 36.594dB */
	{ 0x4332 , 0x30 , 0x433 },		/*index: 119, 36.5469dB */
	{ 0x42D5 , 0x30 , 0x42D },		/*index: 120, 36.4999dB */
	{ 0x4279 , 0x30 , 0x428 },		/*index: 121, 36.4529dB */
	{ 0x421D , 0x30 , 0x422 },		/*index: 122, 36.4058dB */
	{ 0x41C2 , 0x30 , 0x41C },		/*index: 123, 36.3588dB */
	{ 0x4167 , 0x30 , 0x416 },		/*index: 124, 36.3117dB */
	{ 0x410C , 0x30 , 0x411 },		/*index: 125, 36.2647dB */
	{ 0x40B2 , 0x30 , 0x40B },		/*index: 126, 36.2177dB */
	{ 0x4059 , 0x30 , 0x406 },		/*index: 127, 36.1706dB */
	{ 0x4000 , 0x30 , 0x400 },		/*index: 128, 36.1236dB */
	{ 0x3FA8 , 0x30 , 0x3FA },		/*index: 129, 36.0766dB */
	{ 0x3F50 , 0x30 , 0x3F5 },		/*index: 130, 36.0295dB */
	{ 0x3EF8 , 0x30 , 0x3EF },		/*index: 131, 35.9825dB */
	{ 0x3EA1 , 0x30 , 0x3EA },		/*index: 132, 35.9355dB */
	{ 0x3E4A , 0x30 , 0x3E5 },		/*index: 133, 35.8884dB */
	{ 0x3DF4 , 0x30 , 0x3DF },		/*index: 134, 35.8414dB */
	{ 0x3D9F , 0x30 , 0x3DA },		/*index: 135, 35.7943dB */
	{ 0x3D49 , 0x30 , 0x3D5 },		/*index: 136, 35.7473dB */
	{ 0x3CF5 , 0x30 , 0x3CF },		/*index: 137, 35.7003dB */
	{ 0x3CA0 , 0x30 , 0x3CA },		/*index: 138, 35.6532dB */
	{ 0x3C4D , 0x30 , 0x3C5 },		/*index: 139, 35.6062dB */
	{ 0x3BF9 , 0x30 , 0x3C0 },		/*index: 140, 35.5592dB */
	{ 0x3BA6 , 0x30 , 0x3BA },		/*index: 141, 35.5121dB */
	{ 0x3B54 , 0x30 , 0x3B5 },		/*index: 142, 35.4651dB */
	{ 0x3B02 , 0x30 , 0x3B0 },		/*index: 143, 35.4181dB */
	{ 0x3AB0 , 0x30 , 0x3AB },		/*index: 144, 35.371dB */
	{ 0x3A5F , 0x30 , 0x3A6 },		/*index: 145, 35.324dB */
	{ 0x3A0E , 0x30 , 0x3A1 },		/*index: 146, 35.277dB */
	{ 0x39BE , 0x30 , 0x39C },		/*index: 147, 35.2299dB */
	{ 0x396E , 0x30 , 0x397 },		/*index: 148, 35.1829dB */
	{ 0x391F , 0x30 , 0x392 },		/*index: 149, 35.1358dB */
	{ 0x38D0 , 0x30 , 0x38D },		/*index: 150, 35.0888dB */
	{ 0x3881 , 0x30 , 0x388 },		/*index: 151, 35.0418dB */
	{ 0x3833 , 0x30 , 0x383 },		/*index: 152, 34.9947dB */
	{ 0x37E6 , 0x30 , 0x37E },		/*index: 153, 34.9477dB */
	{ 0x3798 , 0x30 , 0x37A },		/*index: 154, 34.9007dB */
	{ 0x374B , 0x30 , 0x375 },		/*index: 155, 34.8536dB */
	{ 0x36FF , 0x30 , 0x370 },		/*index: 156, 34.8066dB */
	{ 0x36B3 , 0x30 , 0x36B },		/*index: 157, 34.7596dB */
	{ 0x3667 , 0x30 , 0x366 },		/*index: 158, 34.7125dB */
	{ 0x361C , 0x30 , 0x362 },		/*index: 159, 34.6655dB */
	{ 0x35D1 , 0x30 , 0x35D },		/*index: 160, 34.6184dB */
	{ 0x3587 , 0x30 , 0x358 },		/*index: 161, 34.5714dB */
	{ 0x353D , 0x30 , 0x354 },		/*index: 162, 34.5244dB */
	{ 0x34F3 , 0x30 , 0x34F },		/*index: 163, 34.4773dB */
	{ 0x34AA , 0x30 , 0x34B },		/*index: 164, 34.4303dB */
	{ 0x3461 , 0x30 , 0x346 },		/*index: 165, 34.3833dB */
	{ 0x3419 , 0x30 , 0x342 },		/*index: 166, 34.3362dB */
	{ 0x33D1 , 0x30 , 0x33D },		/*index: 167, 34.2892dB */
	{ 0x3389 , 0x30 , 0x339 },		/*index: 168, 34.2422dB */
	{ 0x3342 , 0x30 , 0x334 },		/*index: 169, 34.1951dB */
	{ 0x32FB , 0x30 , 0x330 },		/*index: 170, 34.1481dB */
	{ 0x32B5 , 0x30 , 0x32B },		/*index: 171, 34.1011dB */
	{ 0x326E , 0x30 , 0x327 },		/*index: 172, 34.054dB */
	{ 0x3229 , 0x30 , 0x323 },		/*index: 173, 34.007dB */
	{ 0x31E3 , 0x30 , 0x31E },		/*index: 174, 33.9599dB */
	{ 0x319E , 0x30 , 0x31A },		/*index: 175, 33.9129dB */
	{ 0x315A , 0x30 , 0x316 },		/*index: 176, 33.8659dB */
	{ 0x3116 , 0x30 , 0x311 },		/*index: 177, 33.8188dB */
	{ 0x30D2 , 0x30 , 0x30D },		/*index: 178, 33.7718dB */
	{ 0x308E , 0x30 , 0x309 },		/*index: 179, 33.7248dB */
	{ 0x304B , 0x30 , 0x305 },		/*index: 180, 33.6777dB */
	{ 0x3008 , 0x30 , 0x301 },		/*index: 181, 33.6307dB */
	{ 0x2FC6 , 0x30 , 0x2FC },		/*index: 182, 33.5837dB */
	{ 0x2F84 , 0x30 , 0x2F8 },		/*index: 183, 33.5366dB */
	{ 0x2F42 , 0x30 , 0x2F4 },		/*index: 184, 33.4896dB */
	{ 0x2F01 , 0x30 , 0x2F0 },		/*index: 185, 33.4426dB */
	{ 0x2EC0 , 0x30 , 0x2EC },		/*index: 186, 33.3955dB */
	{ 0x2E7F , 0x30 , 0x2E8 },		/*index: 187, 33.3485dB */
	{ 0x2E3F , 0x30 , 0x2E4 },		/*index: 188, 33.3014dB */
	{ 0x2DFF , 0x30 , 0x2E0 },		/*index: 189, 33.2544dB */
	{ 0x2DBF , 0x30 , 0x2DC },		/*index: 190, 33.2074dB */
	{ 0x2D80 , 0x30 , 0x2D8 },		/*index: 191, 33.1603dB */
	{ 0x2D41 , 0x30 , 0x2D4 },		/*index: 192, 33.1133dB */
	{ 0x2D03 , 0x30 , 0x2D0 },		/*index: 193, 33.0663dB */
	{ 0x2CC4 , 0x30 , 0x2CC },		/*index: 194, 33.0192dB */
	{ 0x2C87 , 0x30 , 0x2C8 },		/*index: 195, 32.9722dB */
	{ 0x2C49 , 0x30 , 0x2C5 },		/*index: 196, 32.9252dB */
	{ 0x2C0C , 0x30 , 0x2C1 },		/*index: 197, 32.8781dB */
	{ 0x2BCF , 0x30 , 0x2BD },		/*index: 198, 32.8311dB */
	{ 0x2B92 , 0x30 , 0x2B9 },		/*index: 199, 32.784dB */
	{ 0x2B56 , 0x30 , 0x2B5 },		/*index: 200, 32.737dB */
	{ 0x2B1A , 0x30 , 0x2B2 },		/*index: 201, 32.69dB */
	{ 0x2ADF , 0x30 , 0x2AE },		/*index: 202, 32.6429dB */
	{ 0x2AA3 , 0x30 , 0x2AA },		/*index: 203, 32.5959dB */
	{ 0x2A68 , 0x30 , 0x2A7 },		/*index: 204, 32.5489dB */
	{ 0x2A2E , 0x30 , 0x2A3 },		/*index: 205, 32.5018dB */
	{ 0x29F3 , 0x30 , 0x29F },		/*index: 206, 32.4548dB */
	{ 0x29B9 , 0x30 , 0x29C },		/*index: 207, 32.4078dB */
	{ 0x2980 , 0x30 , 0x298 },		/*index: 208, 32.3607dB */
	{ 0x2946 , 0x30 , 0x294 },		/*index: 209, 32.3137dB */
	{ 0x290D , 0x30 , 0x291 },		/*index: 210, 32.2667dB */
	{ 0x28D5 , 0x30 , 0x28D },		/*index: 211, 32.2196dB */
	{ 0x289C , 0x30 , 0x28A },		/*index: 212, 32.1726dB */
	{ 0x2864 , 0x30 , 0x286 },		/*index: 213, 32.1255dB */
	{ 0x282C , 0x30 , 0x283 },		/*index: 214, 32.0785dB */
	{ 0x27F5 , 0x30 , 0x27F },		/*index: 215, 32.0315dB */
	{ 0x27BD , 0x30 , 0x27C },		/*index: 216, 31.9844dB */
	{ 0x2786 , 0x30 , 0x278 },		/*index: 217, 31.9374dB */
	{ 0x2750 , 0x30 , 0x275 },		/*index: 218, 31.8904dB */
	{ 0x2719 , 0x30 , 0x272 },		/*index: 219, 31.8433dB */
	{ 0x26E3 , 0x30 , 0x26E },		/*index: 220, 31.7963dB */
	{ 0x26AE , 0x30 , 0x26B },		/*index: 221, 31.7493dB */
	{ 0x2678 , 0x30 , 0x268 },		/*index: 222, 31.7022dB */
	{ 0x2643 , 0x30 , 0x264 },		/*index: 223, 31.6552dB */
	{ 0x260E , 0x30 , 0x261 },		/*index: 224, 31.6081dB */
	{ 0x25D9 , 0x30 , 0x25E },		/*index: 225, 31.5611dB */
	{ 0x25A5 , 0x30 , 0x25A },		/*index: 226, 31.5141dB */
	{ 0x2571 , 0x30 , 0x257 },		/*index: 227, 31.467dB */
	{ 0x253D , 0x30 , 0x254 },		/*index: 228, 31.42dB */
	{ 0x250A , 0x30 , 0x251 },		/*index: 229, 31.373dB */
	{ 0x24D7 , 0x30 , 0x24D },		/*index: 230, 31.3259dB */
	{ 0x24A4 , 0x30 , 0x24A },		/*index: 231, 31.2789dB */
	{ 0x2471 , 0x30 , 0x247 },		/*index: 232, 31.2319dB */
	{ 0x243F , 0x30 , 0x244 },		/*index: 233, 31.1848dB */
	{ 0x240C , 0x30 , 0x241 },		/*index: 234, 31.1378dB */
	{ 0x23DB , 0x30 , 0x23E },		/*index: 235, 31.0908dB */
	{ 0x23A9 , 0x30 , 0x23B },		/*index: 236, 31.0437dB */
	{ 0x2378 , 0x30 , 0x237 },		/*index: 237, 30.9967dB */
	{ 0x2347 , 0x30 , 0x234 },		/*index: 238, 30.9496dB */
	{ 0x2316 , 0x30 , 0x231 },		/*index: 239, 30.9026dB */
	{ 0x22E5 , 0x30 , 0x22E },		/*index: 240, 30.8556dB */
	{ 0x22B5 , 0x30 , 0x22B },		/*index: 241, 30.8085dB */
	{ 0x2285 , 0x30 , 0x228 },		/*index: 242, 30.7615dB */
	{ 0x2255 , 0x30 , 0x225 },		/*index: 243, 30.7145dB */
	{ 0x2226 , 0x30 , 0x222 },		/*index: 244, 30.6674dB */
	{ 0x21F7 , 0x30 , 0x21F },		/*index: 245, 30.6204dB */
	{ 0x21C8 , 0x30 , 0x21C },		/*index: 246, 30.5734dB */
	{ 0x2199 , 0x30 , 0x21A },		/*index: 247, 30.5263dB */
	{ 0x216B , 0x30 , 0x217 },		/*index: 248, 30.4793dB */
	{ 0x213C , 0x30 , 0x214 },		/*index: 249, 30.4323dB */
	{ 0x210F , 0x30 , 0x211 },		/*index: 250, 30.3852dB */
	{ 0x20E1 , 0x30 , 0x20E },		/*index: 251, 30.3382dB */
	{ 0x20B3 , 0x30 , 0x20B },		/*index: 252, 30.2911dB */
	{ 0x2086 , 0x30 , 0x208 },		/*index: 253, 30.2441dB */
	{ 0x2059 , 0x30 , 0x206 },		/*index: 254, 30.1971dB */
	{ 0x202C , 0x30 , 0x203 },		/*index: 255, 30.15dB */
	{ 0x2000 , 0x30 , 0x200 },		/*index: 256, 30.103dB */
	{ 0x1FD4 , 0x30 , 0x1FD },		/*index: 257, 30.056dB */
	{ 0x1FA8 , 0x30 , 0x1FA },		/*index: 258, 30.0089dB */
	{ 0x1F7C , 0x30 , 0x1F8 },		/*index: 259, 29.9619dB */
	{ 0x1F50 , 0x30 , 0x1F5 },		/*index: 260, 29.9149dB */
	{ 0x1F25 , 0x30 , 0x1F2 },		/*index: 261, 29.8678dB */
	{ 0x1EFA , 0x30 , 0x1F0 },		/*index: 262, 29.8208dB */
	{ 0x1ECF , 0x30 , 0x1ED },		/*index: 263, 29.7737dB */
	{ 0x1EA5 , 0x30 , 0x1EA },		/*index: 264, 29.7267dB */
	{ 0x1E7A , 0x30 , 0x1E8 },		/*index: 265, 29.6797dB */
	{ 0x1E50 , 0x30 , 0x1E5 },		/*index: 266, 29.6326dB */
	{ 0x1E26 , 0x30 , 0x1E2 },		/*index: 267, 29.5856dB */
	{ 0x1DFD , 0x30 , 0x1E0 },		/*index: 268, 29.5386dB */
	{ 0x1DD3 , 0x30 , 0x1DD },		/*index: 269, 29.4915dB */
	{ 0x1DAA , 0x30 , 0x1DB },		/*index: 270, 29.4445dB */
	{ 0x1D81 , 0x30 , 0x1D8 },		/*index: 271, 29.3975dB */
	{ 0x1D58 , 0x30 , 0x1D6 },		/*index: 272, 29.3504dB */
	{ 0x1D30 , 0x30 , 0x1D3 },		/*index: 273, 29.3034dB */
	{ 0x1D07 , 0x30 , 0x1D0 },		/*index: 274, 29.2564dB */
	{ 0x1CDF , 0x30 , 0x1CE },		/*index: 275, 29.2093dB */
	{ 0x1CB7 , 0x30 , 0x1CB },		/*index: 276, 29.1623dB */
	{ 0x1C8F , 0x30 , 0x1C9 },		/*index: 277, 29.1152dB */
	{ 0x1C68 , 0x30 , 0x1C6 },		/*index: 278, 29.0682dB */
	{ 0x1C41 , 0x30 , 0x1C4 },		/*index: 279, 29.0212dB */
	{ 0x1C1A , 0x30 , 0x1C2 },		/*index: 280, 28.9741dB */
	{ 0x1BF3 , 0x30 , 0x1BF },		/*index: 281, 28.9271dB */
	{ 0x1BCC , 0x30 , 0x1BD },		/*index: 282, 28.8801dB */
	{ 0x1BA6 , 0x30 , 0x1BA },		/*index: 283, 28.833dB */
	{ 0x1B7F , 0x30 , 0x1B8 },		/*index: 284, 28.786dB */
	{ 0x1B59 , 0x30 , 0x1B6 },		/*index: 285, 28.739dB */
	{ 0x1B34 , 0x30 , 0x1B3 },		/*index: 286, 28.6919dB */
	{ 0x1B0E , 0x30 , 0x1B1 },		/*index: 287, 28.6449dB */
	{ 0x1AE9 , 0x30 , 0x1AF },		/*index: 288, 28.5978dB */
	{ 0x1AC3 , 0x30 , 0x1AC },		/*index: 289, 28.5508dB */
	{ 0x1A9E , 0x30 , 0x1AA },		/*index: 290, 28.5038dB */
	{ 0x1A7A , 0x30 , 0x1A8 },		/*index: 291, 28.4567dB */
	{ 0x1A55 , 0x30 , 0x1A5 },		/*index: 292, 28.4097dB */
	{ 0x1A31 , 0x30 , 0x1A3 },		/*index: 293, 28.3627dB */
	{ 0x1A0C , 0x30 , 0x1A1 },		/*index: 294, 28.3156dB */
	{ 0x19E8 , 0x30 , 0x19F },		/*index: 295, 28.2686dB */
	{ 0x19C5 , 0x30 , 0x19C },		/*index: 296, 28.2216dB */
	{ 0x19A1 , 0x30 , 0x19A },		/*index: 297, 28.1745dB */
	{ 0x197E , 0x30 , 0x198 },		/*index: 298, 28.1275dB */
	{ 0x195A , 0x30 , 0x196 },		/*index: 299, 28.0805dB */
	{ 0x1937 , 0x30 , 0x193 },		/*index: 300, 28.0334dB */
	{ 0x1914 , 0x30 , 0x191 },		/*index: 301, 27.9864dB */
	{ 0x18F2 , 0x30 , 0x18F },		/*index: 302, 27.9393dB */
	{ 0x18CF , 0x30 , 0x18D },		/*index: 303, 27.8923dB */
	{ 0x18AD , 0x30 , 0x18B },		/*index: 304, 27.8453dB */
	{ 0x188B , 0x30 , 0x189 },		/*index: 305, 27.7982dB */
	{ 0x1869 , 0x30 , 0x187 },		/*index: 306, 27.7512dB */
	{ 0x1847 , 0x30 , 0x184 },		/*index: 307, 27.7042dB */
	{ 0x1826 , 0x30 , 0x182 },		/*index: 308, 27.6571dB */
	{ 0x1804 , 0x30 , 0x180 },		/*index: 309, 27.6101dB */
	{ 0x17E3 , 0x30 , 0x17E },		/*index: 310, 27.5631dB */
	{ 0x17C2 , 0x30 , 0x17C },		/*index: 311, 27.516dB */
	{ 0x17A1 , 0x30 , 0x17A },		/*index: 312, 27.469dB */
	{ 0x1780 , 0x30 , 0x178 },		/*index: 313, 27.422dB */
	{ 0x1760 , 0x30 , 0x176 },		/*index: 314, 27.3749dB */
	{ 0x1740 , 0x30 , 0x174 },		/*index: 315, 27.3279dB */
	{ 0x171F , 0x30 , 0x172 },		/*index: 316, 27.2808dB */
	{ 0x16FF , 0x30 , 0x170 },		/*index: 317, 27.2338dB */
	{ 0x16E0 , 0x30 , 0x16E },		/*index: 318, 27.1868dB */
	{ 0x16C0 , 0x30 , 0x16C },		/*index: 319, 27.1397dB */
	{ 0x16A1 , 0x30 , 0x16A },		/*index: 320, 27.0927dB */
	{ 0x1681 , 0x30 , 0x168 },		/*index: 321, 27.0457dB */
	{ 0x1662 , 0x30 , 0x166 },		/*index: 322, 26.9986dB */
	{ 0x1643 , 0x30 , 0x164 },		/*index: 323, 26.9516dB */
	{ 0x1624 , 0x30 , 0x162 },		/*index: 324, 26.9046dB */
	{ 0x1606 , 0x30 , 0x160 },		/*index: 325, 26.8575dB */
	{ 0x15E7 , 0x30 , 0x15E },		/*index: 326, 26.8105dB */
	{ 0x15C9 , 0x30 , 0x15D },		/*index: 327, 26.7634dB */
	{ 0x15AB , 0x30 , 0x15B },		/*index: 328, 26.7164dB */
	{ 0x158D , 0x30 , 0x159 },		/*index: 329, 26.6694dB */
	{ 0x156F , 0x30 , 0x157 },		/*index: 330, 26.6223dB */
	{ 0x1552 , 0x30 , 0x155 },		/*index: 331, 26.5753dB */
	{ 0x1534 , 0x30 , 0x153 },		/*index: 332, 26.5283dB */
	{ 0x1517 , 0x30 , 0x151 },		/*index: 333, 26.4812dB */
	{ 0x14FA , 0x30 , 0x150 },		/*index: 334, 26.4342dB */
	{ 0x14DD , 0x30 , 0x14E },		/*index: 335, 26.3872dB */
	{ 0x14C0 , 0x30 , 0x14C },		/*index: 336, 26.3401dB */
	{ 0x14A3 , 0x30 , 0x14A },		/*index: 337, 26.2931dB */
	{ 0x1487 , 0x30 , 0x148 },		/*index: 338, 26.2461dB */
	{ 0x146A , 0x30 , 0x147 },		/*index: 339, 26.199dB */
	{ 0x144E , 0x30 , 0x145 },		/*index: 340, 26.152dB */
	{ 0x1432 , 0x30 , 0x143 },		/*index: 341, 26.1049dB */
	{ 0x1416 , 0x30 , 0x141 },		/*index: 342, 26.0579dB */
	{ 0x13FA , 0x30 , 0x140 },		/*index: 343, 26.0109dB */
	{ 0x13DF , 0x30 , 0x13E },		/*index: 344, 25.9638dB */
	{ 0x13C3 , 0x30 , 0x13C },		/*index: 345, 25.9168dB */
	{ 0x13A8 , 0x30 , 0x13A },		/*index: 346, 25.8698dB */
	{ 0x138D , 0x30 , 0x139 },		/*index: 347, 25.8227dB */
	{ 0x1372 , 0x30 , 0x137 },		/*index: 348, 25.7757dB */
	{ 0x1357 , 0x30 , 0x135 },		/*index: 349, 25.7287dB */
	{ 0x133C , 0x30 , 0x134 },		/*index: 350, 25.6816dB */
	{ 0x1321 , 0x30 , 0x132 },		/*index: 351, 25.6346dB */
	{ 0x1307 , 0x30 , 0x130 },		/*index: 352, 25.5875dB */
	{ 0x12ED , 0x30 , 0x12F },		/*index: 353, 25.5405dB */
	{ 0x12D3 , 0x30 , 0x12D },		/*index: 354, 25.4935dB */
	{ 0x12B8 , 0x30 , 0x12C },		/*index: 355, 25.4464dB */
	{ 0x129F , 0x30 , 0x12A },		/*index: 356, 25.3994dB */
	{ 0x1285 , 0x30 , 0x128 },		/*index: 357, 25.3524dB */
	{ 0x126B , 0x30 , 0x127 },		/*index: 358, 25.3053dB */
	{ 0x1252 , 0x30 , 0x125 },		/*index: 359, 25.2583dB */
	{ 0x1238 , 0x30 , 0x124 },		/*index: 360, 25.2113dB */
	{ 0x121F , 0x30 , 0x122 },		/*index: 361, 25.1642dB */
	{ 0x1206 , 0x30 , 0x120 },		/*index: 362, 25.1172dB */
	{ 0x11ED , 0x30 , 0x11F },		/*index: 363, 25.0702dB */
	{ 0x11D5 , 0x30 , 0x11D },		/*index: 364, 25.0231dB */
	{ 0x11BC , 0x30 , 0x11C },		/*index: 365, 24.9761dB */
	{ 0x11A3 , 0x30 , 0x11A },		/*index: 366, 24.929dB */
	{ 0x118B , 0x30 , 0x119 },		/*index: 367, 24.882dB */
	{ 0x1173 , 0x30 , 0x117 },		/*index: 368, 24.835dB */
	{ 0x115B , 0x30 , 0x116 },		/*index: 369, 24.7879dB */
	{ 0x1143 , 0x30 , 0x114 },		/*index: 370, 24.7409dB */
	{ 0x112B , 0x30 , 0x113 },		/*index: 371, 24.6939dB */
	{ 0x1113 , 0x30 , 0x111 },		/*index: 372, 24.6468dB */
	{ 0x10FB , 0x30 , 0x110 },		/*index: 373, 24.5998dB */
	{ 0x10E4 , 0x30 , 0x10E },		/*index: 374, 24.5528dB */
	{ 0x10CD , 0x30 , 0x10D },		/*index: 375, 24.5057dB */
	{ 0x10B5 , 0x30 , 0x10B },		/*index: 376, 24.4587dB */
	{ 0x109E , 0x30 , 0x10A },		/*index: 377, 24.4117dB */
	{ 0x1087 , 0x30 , 0x108 },		/*index: 378, 24.3646dB */
	{ 0x1070 , 0x30 , 0x107 },		/*index: 379, 24.3176dB */
	{ 0x105A , 0x30 , 0x106 },		/*index: 380, 24.2705dB */
	{ 0x1043 , 0x30 , 0x104 },		/*index: 381, 24.2235dB */
	{ 0x102D , 0x30 , 0x103 },		/*index: 382, 24.1765dB */
	{ 0x1016 , 0x30 , 0x101 },		/*index: 383, 24.1294dB */
	{ 0x1000 , 0x30 , 0x100 },		/*index: 384, 24.0824dB */
	{ 0xFEA , 0x30 , 0xFF },		/*index: 385, 24.0354dB */
	{ 0xFD4 , 0x30 , 0xFD },		/*index: 386, 23.9883dB */
	{ 0xFBE , 0x30 , 0xFC },		/*index: 387, 23.9413dB */
	{ 0xFA8 , 0x30 , 0xFB },		/*index: 388, 23.8943dB */
	{ 0xF93 , 0x30 , 0xF9 },		/*index: 389, 23.8472dB */
	{ 0xF7D , 0x30 , 0xF8 },		/*index: 390, 23.8002dB */
	{ 0xF68 , 0x30 , 0xF6 },		/*index: 391, 23.7531dB */
	{ 0xF52 , 0x30 , 0xF5 },		/*index: 392, 23.7061dB */
	{ 0xF3D , 0x30 , 0xF4 },		/*index: 393, 23.6591dB */
	{ 0xF28 , 0x30 , 0xF3 },		/*index: 394, 23.612dB */
	{ 0xF13 , 0x30 , 0xF1 },		/*index: 395, 23.565dB */
	{ 0xEFE , 0x30 , 0xF0 },		/*index: 396, 23.518dB */
	{ 0xEEA , 0x30 , 0xEF },		/*index: 397, 23.4709dB */
	{ 0xED5 , 0x30 , 0xED },		/*index: 398, 23.4239dB */
	{ 0xEC0 , 0x30 , 0xEC },		/*index: 399, 23.3769dB */
	{ 0xEAC , 0x30 , 0xEB },		/*index: 400, 23.3298dB */
	{ 0xE98 , 0x30 , 0xE9 },		/*index: 401, 23.2828dB */
	{ 0xE84 , 0x30 , 0xE8 },		/*index: 402, 23.2358dB */
	{ 0xE70 , 0x30 , 0xE7 },		/*index: 403, 23.1887dB */
	{ 0xE5C , 0x30 , 0xE6 },		/*index: 404, 23.1417dB */
	{ 0xE48 , 0x30 , 0xE4 },		/*index: 405, 23.0946dB */
	{ 0xE34 , 0x30 , 0xE3 },		/*index: 406, 23.0476dB */
	{ 0xE20 , 0x30 , 0xE2 },		/*index: 407, 23.0006dB */
	{ 0xE0D , 0x30 , 0xE1 },		/*index: 408, 22.9535dB */
	{ 0xDF9 , 0x30 , 0xE0 },		/*index: 409, 22.9065dB */
	{ 0xDE6 , 0x30 , 0xDE },		/*index: 410, 22.8595dB */
	{ 0xDD3 , 0x30 , 0xDD },		/*index: 411, 22.8124dB */
	{ 0xDC0 , 0x30 , 0xDC },		/*index: 412, 22.7654dB */
	{ 0xDAD , 0x30 , 0xDB },		/*index: 413, 22.7184dB */
	{ 0xD9A , 0x30 , 0xDA },		/*index: 414, 22.6713dB */
	{ 0xD87 , 0x30 , 0xD8 },		/*index: 415, 22.6243dB */
	{ 0xD74 , 0x30 , 0xD7 },		/*index: 416, 22.5772dB */
	{ 0xD62 , 0x30 , 0xD6 },		/*index: 417, 22.5302dB */
	{ 0xD4F , 0x30 , 0xD5 },		/*index: 418, 22.4832dB */
	{ 0xD3D , 0x30 , 0xD4 },		/*index: 419, 22.4361dB */
	{ 0xD2B , 0x30 , 0xD3 },		/*index: 420, 22.3891dB */
	{ 0xD18 , 0x30 , 0xD2 },		/*index: 421, 22.3421dB */
	{ 0xD06 , 0x30 , 0xD0 },		/*index: 422, 22.295dB */
	{ 0xCF4 , 0x30 , 0xCF },		/*index: 423, 22.248dB */
	{ 0xCE2 , 0x30 , 0xCE },		/*index: 424, 22.201dB */
	{ 0xCD0 , 0x30 , 0xCD },		/*index: 425, 22.1539dB */
	{ 0xCBF , 0x30 , 0xCC },		/*index: 426, 22.1069dB */
	{ 0xCAD , 0x30 , 0xCB },		/*index: 427, 22.0599dB */
	{ 0xC9C , 0x30 , 0xCA },		/*index: 428, 22.0128dB */
	{ 0xC8A , 0x30 , 0xC9 },		/*index: 429, 21.9658dB */
	{ 0xC79 , 0x30 , 0xC8 },		/*index: 430, 21.9187dB */
	{ 0xC68 , 0x30 , 0xC6 },		/*index: 431, 21.8717dB */
	{ 0xC56 , 0x30 , 0xC5 },		/*index: 432, 21.8247dB */
	{ 0xC45 , 0x30 , 0xC4 },		/*index: 433, 21.7776dB */
	{ 0xC34 , 0x30 , 0xC3 },		/*index: 434, 21.7306dB */
	{ 0xC24 , 0x30 , 0xC2 },		/*index: 435, 21.6836dB */
	{ 0xC13 , 0x30 , 0xC1 },		/*index: 436, 21.6365dB */
	{ 0xC02 , 0x30 , 0xC0 },		/*index: 437, 21.5895dB */
	{ 0xBF1 , 0x30 , 0xBF },		/*index: 438, 21.5425dB */
	{ 0xBE1 , 0x30 , 0xBE },		/*index: 439, 21.4954dB */
	{ 0xBD1 , 0x30 , 0xBD },		/*index: 440, 21.4484dB */
	{ 0xBC0 , 0x30 , 0xBC },		/*index: 441, 21.4014dB */
	{ 0xBB0 , 0x30 , 0xBB },		/*index: 442, 21.3543dB */
	{ 0xBA0 , 0x30 , 0xBA },		/*index: 443, 21.3073dB */
	{ 0xB90 , 0x30 , 0xB9 },		/*index: 444, 21.2602dB */
	{ 0xB80 , 0x30 , 0xB8 },		/*index: 445, 21.2132dB */
	{ 0xB70 , 0x30 , 0xB7 },		/*index: 446, 21.1662dB */
	{ 0xB60 , 0x30 , 0xB6 },		/*index: 447, 21.1191dB */
	{ 0xB50 , 0x30 , 0xB5 },		/*index: 448, 21.0721dB */
	{ 0xB41 , 0x30 , 0xB4 },		/*index: 449, 21.0251dB */
	{ 0xB31 , 0x30 , 0xB3 },		/*index: 450, 20.978dB */
	{ 0xB22 , 0x30 , 0xB2 },		/*index: 451, 20.931dB */
	{ 0xB12 , 0x30 , 0xB1 },		/*index: 452, 20.884dB */
	{ 0xB03 , 0x30 , 0xB0 },		/*index: 453, 20.8369dB */
	{ 0xAF4 , 0x30 , 0xAF },		/*index: 454, 20.7899dB */
	{ 0xAE5 , 0x30 , 0xAE },		/*index: 455, 20.7428dB */
	{ 0xAD6 , 0x30 , 0xAD },		/*index: 456, 20.6958dB */
	{ 0xAC7 , 0x30 , 0xAC },		/*index: 457, 20.6488dB */
	{ 0xAB8 , 0x30 , 0xAB },		/*index: 458, 20.6017dB */
	{ 0xAA9 , 0x30 , 0xAB },		/*index: 459, 20.5547dB */
	{ 0xA9A , 0x30 , 0xAA },		/*index: 460, 20.5077dB */
	{ 0xA8B , 0x30 , 0xA9 },		/*index: 461, 20.4606dB */
	{ 0xA7D , 0x30 , 0xA8 },		/*index: 462, 20.4136dB */
	{ 0xA6E , 0x30 , 0xA7 },		/*index: 463, 20.3666dB */
	{ 0xA60 , 0x30 , 0xA6 },		/*index: 464, 20.3195dB */
	{ 0xA52 , 0x30 , 0xA5 },		/*index: 465, 20.2725dB */
	{ 0xA43 , 0x30 , 0xA4 },		/*index: 466, 20.2255dB */
	{ 0xA35 , 0x30 , 0xA3 },		/*index: 467, 20.1784dB */
	{ 0xA27 , 0x30 , 0xA2 },		/*index: 468, 20.1314dB */
	{ 0xA19 , 0x30 , 0xA2 },		/*index: 469, 20.0843dB */
	{ 0xA0B , 0x30 , 0xA1 },		/*index: 470, 20.0373dB */
	{ 0x9FD , 0x30 , 0xA0 },		/*index: 471, 19.9903dB */
	{ 0x9EF , 0x30 , 0x9F },		/*index: 472, 19.9432dB */
	{ 0x9E2 , 0x30 , 0x9E },		/*index: 473, 19.8962dB */
	{ 0x9D4 , 0x30 , 0x9D },		/*index: 474, 19.8492dB */
	{ 0x9C6 , 0x30 , 0x9C },		/*index: 475, 19.8021dB */
	{ 0x9B9 , 0x30 , 0x9C },		/*index: 476, 19.7551dB */
	{ 0x9AB , 0x30 , 0x9B },		/*index: 477, 19.7081dB */
	{ 0x99E , 0x30 , 0x9A },		/*index: 478, 19.661dB */
	{ 0x991 , 0x30 , 0x99 },		/*index: 479, 19.614dB */
	{ 0x983 , 0x30 , 0x98 },		/*index: 480, 19.5669dB */
	{ 0x976 , 0x30 , 0x97 },		/*index: 481, 19.5199dB */
	{ 0x969 , 0x30 , 0x97 },		/*index: 482, 19.4729dB */
	{ 0x95C , 0x30 , 0x96 },		/*index: 483, 19.4258dB */
	{ 0x94F , 0x30 , 0x95 },		/*index: 484, 19.3788dB */
	{ 0x942 , 0x30 , 0x94 },		/*index: 485, 19.3318dB */
	{ 0x936 , 0x30 , 0x93 },		/*index: 486, 19.2847dB */
	{ 0x929 , 0x30 , 0x93 },		/*index: 487, 19.2377dB */
	{ 0x91C , 0x30 , 0x92 },		/*index: 488, 19.1907dB */
	{ 0x910 , 0x30 , 0x91 },		/*index: 489, 19.1436dB */
	{ 0x903 , 0x30 , 0x90 },		/*index: 490, 19.0966dB */
	{ 0x8F7 , 0x30 , 0x8F },		/*index: 491, 19.0496dB */
	{ 0x8EA , 0x30 , 0x8F },		/*index: 492, 19.0025dB */
	{ 0x8DE , 0x30 , 0x8E },		/*index: 493, 18.9555dB */
	{ 0x8D2 , 0x30 , 0x8D },		/*index: 494, 18.9084dB */
	{ 0x8C5 , 0x30 , 0x8C },		/*index: 495, 18.8614dB */
	{ 0x8B9 , 0x30 , 0x8C },		/*index: 496, 18.8144dB */
	{ 0x8AD , 0x30 , 0x8B },		/*index: 497, 18.7673dB */
	{ 0x8A1 , 0x30 , 0x8A },		/*index: 498, 18.7203dB */
	{ 0x895 , 0x30 , 0x89 },		/*index: 499, 18.6733dB */
	{ 0x88A , 0x30 , 0x89 },		/*index: 500, 18.6262dB */
	{ 0x87E , 0x30 , 0x88 },		/*index: 501, 18.5792dB */
	{ 0x872 , 0x30 , 0x87 },		/*index: 502, 18.5322dB */
	{ 0x866 , 0x30 , 0x86 },		/*index: 503, 18.4851dB */
	{ 0x85B , 0x30 , 0x86 },		/*index: 504, 18.4381dB */
	{ 0x84F , 0x30 , 0x85 },		/*index: 505, 18.3911dB */
	{ 0x844 , 0x30 , 0x84 },		/*index: 506, 18.344dB */
	{ 0x838 , 0x30 , 0x84 },		/*index: 507, 18.297dB */
	{ 0x82D , 0x30 , 0x83 },		/*index: 508, 18.2499dB */
	{ 0x822 , 0x30 , 0x82 },		/*index: 509, 18.2029dB */
	{ 0x816 , 0x30 , 0x81 },		/*index: 510, 18.1559dB */
	{ 0x80B , 0x30 , 0x81 },		/*index: 511, 18.1088dB */
	{ 0x800 , 0x30 , 0x80 },		/*index: 512, 18.0618dB */
	{ 0x7F5 , 0x20 , 0xFF },		/*index: 513, 18.0148dB */
	{ 0x7EA , 0x20 , 0xFD },		/*index: 514, 17.9677dB */
	{ 0x7DF , 0x20 , 0xFC },		/*index: 515, 17.9207dB */
	{ 0x7D4 , 0x20 , 0xFB },		/*index: 516, 17.8737dB */
	{ 0x7C9 , 0x20 , 0xF9 },		/*index: 517, 17.8266dB */
	{ 0x7BF , 0x20 , 0xF8 },		/*index: 518, 17.7796dB */
	{ 0x7B4 , 0x20 , 0xF6 },		/*index: 519, 17.7325dB */
	{ 0x7A9 , 0x20 , 0xF5 },		/*index: 520, 17.6855dB */
	{ 0x79F , 0x20 , 0xF4 },		/*index: 521, 17.6385dB */
	{ 0x794 , 0x20 , 0xF3 },		/*index: 522, 17.5914dB */
	{ 0x78A , 0x20 , 0xF1 },		/*index: 523, 17.5444dB */
	{ 0x77F , 0x20 , 0xF0 },		/*index: 524, 17.4974dB */
	{ 0x775 , 0x20 , 0xEF },		/*index: 525, 17.4503dB */
	{ 0x76A , 0x20 , 0xED },		/*index: 526, 17.4033dB */
	{ 0x760 , 0x20 , 0xEC },		/*index: 527, 17.3563dB */
	{ 0x756 , 0x20 , 0xEB },		/*index: 528, 17.3092dB */
	{ 0x74C , 0x20 , 0xE9 },		/*index: 529, 17.2622dB */
	{ 0x742 , 0x20 , 0xE8 },		/*index: 530, 17.2152dB */
	{ 0x738 , 0x20 , 0xE7 },		/*index: 531, 17.1681dB */
	{ 0x72E , 0x20 , 0xE6 },		/*index: 532, 17.1211dB */
	{ 0x724 , 0x20 , 0xE4 },		/*index: 533, 17.074dB */
	{ 0x71A , 0x20 , 0xE3 },		/*index: 534, 17.027dB */
	{ 0x710 , 0x20 , 0xE2 },		/*index: 535, 16.98dB */
	{ 0x706 , 0x20 , 0xE1 },		/*index: 536, 16.9329dB */
	{ 0x6FD , 0x20 , 0xE0 },		/*index: 537, 16.8859dB */
	{ 0x6F3 , 0x20 , 0xDE },		/*index: 538, 16.8389dB */
	{ 0x6E9 , 0x20 , 0xDD },		/*index: 539, 16.7918dB */
	{ 0x6E0 , 0x20 , 0xDC },		/*index: 540, 16.7448dB */
	{ 0x6D6 , 0x20 , 0xDB },		/*index: 541, 16.6978dB */
	{ 0x6CD , 0x20 , 0xDA },		/*index: 542, 16.6507dB */
	{ 0x6C4 , 0x20 , 0xD8 },		/*index: 543, 16.6037dB */
	{ 0x6BA , 0x20 , 0xD7 },		/*index: 544, 16.5566dB */
	{ 0x6B1 , 0x20 , 0xD6 },		/*index: 545, 16.5096dB */
	{ 0x6A8 , 0x20 , 0xD5 },		/*index: 546, 16.4626dB */
	{ 0x69E , 0x20 , 0xD4 },		/*index: 547, 16.4155dB */
	{ 0x695 , 0x20 , 0xD3 },		/*index: 548, 16.3685dB */
	{ 0x68C , 0x20 , 0xD2 },		/*index: 549, 16.3215dB */
	{ 0x683 , 0x20 , 0xD0 },		/*index: 550, 16.2744dB */
	{ 0x67A , 0x20 , 0xCF },		/*index: 551, 16.2274dB */
	{ 0x671 , 0x20 , 0xCE },		/*index: 552, 16.1804dB */
	{ 0x668 , 0x20 , 0xCD },		/*index: 553, 16.1333dB */
	{ 0x65F , 0x20 , 0xCC },		/*index: 554, 16.0863dB */
	{ 0x657 , 0x20 , 0xCB },		/*index: 555, 16.0393dB */
	{ 0x64E , 0x20 , 0xCA },		/*index: 556, 15.9922dB */
	{ 0x645 , 0x20 , 0xC9 },		/*index: 557, 15.9452dB */
	{ 0x63C , 0x20 , 0xC8 },		/*index: 558, 15.8981dB */
	{ 0x634 , 0x20 , 0xC6 },		/*index: 559, 15.8511dB */
	{ 0x62B , 0x20 , 0xC5 },		/*index: 560, 15.8041dB */
	{ 0x623 , 0x20 , 0xC4 },		/*index: 561, 15.757dB */
	{ 0x61A , 0x20 , 0xC3 },		/*index: 562, 15.71dB */
	{ 0x612 , 0x20 , 0xC2 },		/*index: 563, 15.663dB */
	{ 0x609 , 0x20 , 0xC1 },		/*index: 564, 15.6159dB */
	{ 0x601 , 0x20 , 0xC0 },		/*index: 565, 15.5689dB */
	{ 0x5F9 , 0x20 , 0xBF },		/*index: 566, 15.5219dB */
	{ 0x5F0 , 0x20 , 0xBE },		/*index: 567, 15.4748dB */
	{ 0x5E8 , 0x20 , 0xBD },		/*index: 568, 15.4278dB */
	{ 0x5E0 , 0x20 , 0xBC },		/*index: 569, 15.3808dB */
	{ 0x5D8 , 0x20 , 0xBB },		/*index: 570, 15.3337dB */
	{ 0x5D0 , 0x20 , 0xBA },		/*index: 571, 15.2867dB */
	{ 0x5C8 , 0x20 , 0xB9 },		/*index: 572, 15.2396dB */
	{ 0x5C0 , 0x20 , 0xB8 },		/*index: 573, 15.1926dB */
	{ 0x5B8 , 0x20 , 0xB7 },		/*index: 574, 15.1456dB */
	{ 0x5B0 , 0x20 , 0xB6 },		/*index: 575, 15.0985dB */
	{ 0x5A8 , 0x20 , 0xB5 },		/*index: 576, 15.0515dB */
	{ 0x5A0 , 0x20 , 0xB4 },		/*index: 577, 15.0045dB */
	{ 0x599 , 0x20 , 0xB3 },		/*index: 578, 14.9574dB */
	{ 0x591 , 0x20 , 0xB2 },		/*index: 579, 14.9104dB */
	{ 0x589 , 0x20 , 0xB1 },		/*index: 580, 14.8634dB */
	{ 0x581 , 0x20 , 0xB0 },		/*index: 581, 14.8163dB */
	{ 0x57A , 0x20 , 0xAF },		/*index: 582, 14.7693dB */
	{ 0x572 , 0x20 , 0xAE },		/*index: 583, 14.7222dB */
	{ 0x56B , 0x20 , 0xAD },		/*index: 584, 14.6752dB */
	{ 0x563 , 0x20 , 0xAC },		/*index: 585, 14.6282dB */
	{ 0x55C , 0x20 , 0xAB },		/*index: 586, 14.5811dB */
	{ 0x554 , 0x20 , 0xAB },		/*index: 587, 14.5341dB */
	{ 0x54D , 0x20 , 0xAA },		/*index: 588, 14.4871dB */
	{ 0x546 , 0x20 , 0xA9 },		/*index: 589, 14.44dB */
	{ 0x53E , 0x20 , 0xA8 },		/*index: 590, 14.393dB */
	{ 0x537 , 0x20 , 0xA7 },		/*index: 591, 14.346dB */
	{ 0x530 , 0x20 , 0xA6 },		/*index: 592, 14.2989dB */
	{ 0x529 , 0x20 , 0xA5 },		/*index: 593, 14.2519dB */
	{ 0x522 , 0x20 , 0xA4 },		/*index: 594, 14.2049dB */
	{ 0x51B , 0x20 , 0xA3 },		/*index: 595, 14.1578dB */
	{ 0x514 , 0x20 , 0xA2 },		/*index: 596, 14.1108dB */
	{ 0x50C , 0x20 , 0xA2 },		/*index: 597, 14.0637dB */
	{ 0x506 , 0x20 , 0xA1 },		/*index: 598, 14.0167dB */
	{ 0x4FF , 0x20 , 0xA0 },		/*index: 599, 13.9697dB */
	{ 0x4F8 , 0x20 , 0x9F },		/*index: 600, 13.9226dB */
	{ 0x4F1 , 0x20 , 0x9E },		/*index: 601, 13.8756dB */
	{ 0x4EA , 0x20 , 0x9D },		/*index: 602, 13.8286dB */
	{ 0x4E3 , 0x20 , 0x9C },		/*index: 603, 13.7815dB */
	{ 0x4DC , 0x20 , 0x9C },		/*index: 604, 13.7345dB */
	{ 0x4D6 , 0x20 , 0x9B },		/*index: 605, 13.6875dB */
	{ 0x4CF , 0x20 , 0x9A },		/*index: 606, 13.6404dB */
	{ 0x4C8 , 0x20 , 0x99 },		/*index: 607, 13.5934dB */
	{ 0x4C2 , 0x20 , 0x98 },		/*index: 608, 13.5463dB */
	{ 0x4BB , 0x20 , 0x97 },		/*index: 609, 13.4993dB */
	{ 0x4B5 , 0x20 , 0x97 },		/*index: 610, 13.4523dB */
	{ 0x4AE , 0x20 , 0x96 },		/*index: 611, 13.4052dB */
	{ 0x4A8 , 0x20 , 0x95 },		/*index: 612, 13.3582dB */
	{ 0x4A1 , 0x20 , 0x94 },		/*index: 613, 13.3112dB */
	{ 0x49B , 0x20 , 0x93 },		/*index: 614, 13.2641dB */
	{ 0x494 , 0x20 , 0x93 },		/*index: 615, 13.2171dB */
	{ 0x48E , 0x20 , 0x92 },		/*index: 616, 13.1701dB */
	{ 0x488 , 0x20 , 0x91 },		/*index: 617, 13.123dB */
	{ 0x482 , 0x20 , 0x90 },		/*index: 618, 13.076dB */
	{ 0x47B , 0x20 , 0x8F },		/*index: 619, 13.029dB */
	{ 0x475 , 0x20 , 0x8F },		/*index: 620, 12.9819dB */
	{ 0x46F , 0x20 , 0x8E },		/*index: 621, 12.9349dB */
	{ 0x469 , 0x20 , 0x8D },		/*index: 622, 12.8878dB */
	{ 0x463 , 0x20 , 0x8C },		/*index: 623, 12.8408dB */
	{ 0x45D , 0x20 , 0x8C },		/*index: 624, 12.7938dB */
	{ 0x457 , 0x20 , 0x8B },		/*index: 625, 12.7467dB */
	{ 0x451 , 0x20 , 0x8A },		/*index: 626, 12.6997dB */
	{ 0x44B , 0x20 , 0x89 },		/*index: 627, 12.6527dB */
	{ 0x445 , 0x20 , 0x89 },		/*index: 628, 12.6056dB */
	{ 0x43F , 0x20 , 0x88 },		/*index: 629, 12.5586dB */
	{ 0x439 , 0x20 , 0x87 },		/*index: 630, 12.5116dB */
	{ 0x433 , 0x20 , 0x86 },		/*index: 631, 12.4645dB */
	{ 0x42D , 0x20 , 0x86 },		/*index: 632, 12.4175dB */
	{ 0x428 , 0x20 , 0x85 },		/*index: 633, 12.3705dB */
	{ 0x422 , 0x20 , 0x84 },		/*index: 634, 12.3234dB */
	{ 0x41C , 0x20 , 0x84 },		/*index: 635, 12.2764dB */
	{ 0x416 , 0x20 , 0x83 },		/*index: 636, 12.2293dB */
	{ 0x411 , 0x20 , 0x82 },		/*index: 637, 12.1823dB */
	{ 0x40B , 0x20 , 0x81 },		/*index: 638, 12.1353dB */
	{ 0x406 , 0x20 , 0x81 },		/*index: 639, 12.0882dB */
	{ 0x400 , 0x20 , 0x80 },		/*index: 640, 12.0412dB */
	{ 0x3FA , 0x10 , 0xFF },		/*index: 641, 11.9942dB */
	{ 0x3F5 , 0x10 , 0xFD },		/*index: 642, 11.9471dB */
	{ 0x3EF , 0x10 , 0xFC },		/*index: 643, 11.9001dB */
	{ 0x3EA , 0x10 , 0xFB },		/*index: 644, 11.8531dB */
	{ 0x3E5 , 0x10 , 0xF9 },		/*index: 645, 11.806dB */
	{ 0x3DF , 0x10 , 0xF8 },		/*index: 646, 11.759dB */
	{ 0x3DA , 0x10 , 0xF6 },		/*index: 647, 11.7119dB */
	{ 0x3D5 , 0x10 , 0xF5 },		/*index: 648, 11.6649dB */
	{ 0x3CF , 0x10 , 0xF4 },		/*index: 649, 11.6179dB */
	{ 0x3CA , 0x10 , 0xF3 },		/*index: 650, 11.5708dB */
	{ 0x3C5 , 0x10 , 0xF1 },		/*index: 651, 11.5238dB */
	{ 0x3C0 , 0x10 , 0xF0 },		/*index: 652, 11.4768dB */
	{ 0x3BA , 0x10 , 0xEF },		/*index: 653, 11.4297dB */
	{ 0x3B5 , 0x10 , 0xED },		/*index: 654, 11.3827dB */
	{ 0x3B0 , 0x10 , 0xEC },		/*index: 655, 11.3357dB */
	{ 0x3AB , 0x10 , 0xEB },		/*index: 656, 11.2886dB */
	{ 0x3A6 , 0x10 , 0xE9 },		/*index: 657, 11.2416dB */
	{ 0x3A1 , 0x10 , 0xE8 },		/*index: 658, 11.1946dB */
	{ 0x39C , 0x10 , 0xE7 },		/*index: 659, 11.1475dB */
	{ 0x397 , 0x10 , 0xE6 },		/*index: 660, 11.1005dB */
	{ 0x392 , 0x10 , 0xE4 },		/*index: 661, 11.0534dB */
	{ 0x38D , 0x10 , 0xE3 },		/*index: 662, 11.0064dB */
	{ 0x388 , 0x10 , 0xE2 },		/*index: 663, 10.9594dB */
	{ 0x383 , 0x10 , 0xE1 },		/*index: 664, 10.9123dB */
	{ 0x37E , 0x10 , 0xE0 },		/*index: 665, 10.8653dB */
	{ 0x37A , 0x10 , 0xDE },		/*index: 666, 10.8183dB */
	{ 0x375 , 0x10 , 0xDD },		/*index: 667, 10.7712dB */
	{ 0x370 , 0x10 , 0xDC },		/*index: 668, 10.7242dB */
	{ 0x36B , 0x10 , 0xDB },		/*index: 669, 10.6772dB */
	{ 0x366 , 0x10 , 0xDA },		/*index: 670, 10.6301dB */
	{ 0x362 , 0x10 , 0xD8 },		/*index: 671, 10.5831dB */
	{ 0x35D , 0x10 , 0xD7 },		/*index: 672, 10.536dB */
	{ 0x358 , 0x10 , 0xD6 },		/*index: 673, 10.489dB */
	{ 0x354 , 0x10 , 0xD5 },		/*index: 674, 10.442dB */
	{ 0x34F , 0x10 , 0xD4 },		/*index: 675, 10.3949dB */
	{ 0x34B , 0x10 , 0xD3 },		/*index: 676, 10.3479dB */
	{ 0x346 , 0x10 , 0xD2 },		/*index: 677, 10.3009dB */
	{ 0x342 , 0x10 , 0xD0 },		/*index: 678, 10.2538dB */
	{ 0x33D , 0x10 , 0xCF },		/*index: 679, 10.2068dB */
	{ 0x339 , 0x10 , 0xCE },		/*index: 680, 10.1598dB */
	{ 0x334 , 0x10 , 0xCD },		/*index: 681, 10.1127dB */
	{ 0x330 , 0x10 , 0xCC },		/*index: 682, 10.0657dB */
	{ 0x32B , 0x10 , 0xCB },		/*index: 683, 10.0187dB */
	{ 0x327 , 0x10 , 0xCA },		/*index: 684, 9.9716dB */
	{ 0x323 , 0x10 , 0xC9 },		/*index: 685, 9.9246dB */
	{ 0x31E , 0x10 , 0xC8 },		/*index: 686, 9.8775dB */
	{ 0x31A , 0x10 , 0xC6 },		/*index: 687, 9.8305dB */
	{ 0x316 , 0x10 , 0xC5 },		/*index: 688, 9.7835dB */
	{ 0x311 , 0x10 , 0xC4 },		/*index: 689, 9.7364dB */
	{ 0x30D , 0x10 , 0xC3 },		/*index: 690, 9.6894dB */
	{ 0x309 , 0x10 , 0xC2 },		/*index: 691, 9.6424dB */
	{ 0x305 , 0x10 , 0xC1 },		/*index: 692, 9.5953dB */
	{ 0x301 , 0x10 , 0xC0 },		/*index: 693, 9.5483dB */
	{ 0x2FC , 0x10 , 0xBF },		/*index: 694, 9.5013dB */
	{ 0x2F8 , 0x10 , 0xBE },		/*index: 695, 9.4542dB */
	{ 0x2F4 , 0x10 , 0xBD },		/*index: 696, 9.4072dB */
	{ 0x2F0 , 0x10 , 0xBC },		/*index: 697, 9.3602dB */
	{ 0x2EC , 0x10 , 0xBB },		/*index: 698, 9.3131dB */
	{ 0x2E8 , 0x10 , 0xBA },		/*index: 699, 9.2661dB */
	{ 0x2E4 , 0x10 , 0xB9 },		/*index: 700, 9.219dB */
	{ 0x2E0 , 0x10 , 0xB8 },		/*index: 701, 9.172dB */
	{ 0x2DC , 0x10 , 0xB7 },		/*index: 702, 9.125dB */
	{ 0x2D8 , 0x10 , 0xB6 },		/*index: 703, 9.0779dB */
	{ 0x2D4 , 0x10 , 0xB5 },		/*index: 704, 9.0309dB */
	{ 0x2D0 , 0x10 , 0xB4 },		/*index: 705, 8.9839dB */
	{ 0x2CC , 0x10 , 0xB3 },		/*index: 706, 8.9368dB */
	{ 0x2C8 , 0x10 , 0xB2 },		/*index: 707, 8.8898dB */
	{ 0x2C5 , 0x10 , 0xB1 },		/*index: 708, 8.8428dB */
	{ 0x2C1 , 0x10 , 0xB0 },		/*index: 709, 8.7957dB */
	{ 0x2BD , 0x10 , 0xAF },		/*index: 710, 8.7487dB */
	{ 0x2B9 , 0x10 , 0xAE },		/*index: 711, 8.7016dB */
	{ 0x2B5 , 0x10 , 0xAD },		/*index: 712, 8.6546dB */
	{ 0x2B2 , 0x10 , 0xAC },		/*index: 713, 8.6076dB */
	{ 0x2AE , 0x10 , 0xAB },		/*index: 714, 8.5605dB */
	{ 0x2AA , 0x10 , 0xAB },		/*index: 715, 8.5135dB */
	{ 0x2A7 , 0x10 , 0xAA },		/*index: 716, 8.4665dB */
	{ 0x2A3 , 0x10 , 0xA9 },		/*index: 717, 8.4194dB */
	{ 0x29F , 0x10 , 0xA8 },		/*index: 718, 8.3724dB */
	{ 0x29C , 0x10 , 0xA7 },		/*index: 719, 8.3254dB */
	{ 0x298 , 0x10 , 0xA6 },		/*index: 720, 8.2783dB */
	{ 0x294 , 0x10 , 0xA5 },		/*index: 721, 8.2313dB */
	{ 0x291 , 0x10 , 0xA4 },		/*index: 722, 8.1843dB */
	{ 0x28D , 0x10 , 0xA3 },		/*index: 723, 8.1372dB */
	{ 0x28A , 0x10 , 0xA2 },		/*index: 724, 8.0902dB */
	{ 0x286 , 0x10 , 0xA2 },		/*index: 725, 8.0431dB */
	{ 0x283 , 0x10 , 0xA1 },		/*index: 726, 7.9961dB */
	{ 0x27F , 0x10 , 0xA0 },		/*index: 727, 7.9491dB */
	{ 0x27C , 0x10 , 0x9F },		/*index: 728, 7.902dB */
	{ 0x278 , 0x10 , 0x9E },		/*index: 729, 7.855dB */
	{ 0x275 , 0x10 , 0x9D },		/*index: 730, 7.808dB */
	{ 0x272 , 0x10 , 0x9C },		/*index: 731, 7.7609dB */
	{ 0x26E , 0x10 , 0x9C },		/*index: 732, 7.7139dB */
	{ 0x26B , 0x10 , 0x9B },		/*index: 733, 7.6669dB */
	{ 0x268 , 0x10 , 0x9A },		/*index: 734, 7.6198dB */
	{ 0x264 , 0x10 , 0x99 },		/*index: 735, 7.5728dB */
	{ 0x261 , 0x10 , 0x98 },		/*index: 736, 7.5257dB */
	{ 0x25E , 0x10 , 0x97 },		/*index: 737, 7.4787dB */
	{ 0x25A , 0x10 , 0x97 },		/*index: 738, 7.4317dB */
	{ 0x257 , 0x10 , 0x96 },		/*index: 739, 7.3846dB */
	{ 0x254 , 0x10 , 0x95 },		/*index: 740, 7.3376dB */
	{ 0x251 , 0x10 , 0x94 },		/*index: 741, 7.2906dB */
	{ 0x24D , 0x10 , 0x93 },		/*index: 742, 7.2435dB */
	{ 0x24A , 0x10 , 0x93 },		/*index: 743, 7.1965dB */
	{ 0x247 , 0x10 , 0x92 },		/*index: 744, 7.1495dB */
	{ 0x244 , 0x10 , 0x91 },		/*index: 745, 7.1024dB */
	{ 0x241 , 0x10 , 0x90 },		/*index: 746, 7.0554dB */
	{ 0x23E , 0x10 , 0x8F },		/*index: 747, 7.0084dB */
	{ 0x23B , 0x10 , 0x8F },		/*index: 748, 6.9613dB */
	{ 0x237 , 0x10 , 0x8E },		/*index: 749, 6.9143dB */
	{ 0x234 , 0x10 , 0x8D },		/*index: 750, 6.8672dB */
	{ 0x231 , 0x10 , 0x8C },		/*index: 751, 6.8202dB */
	{ 0x22E , 0x10 , 0x8C },		/*index: 752, 6.7732dB */
	{ 0x22B , 0x10 , 0x8B },		/*index: 753, 6.7261dB */
	{ 0x228 , 0x10 , 0x8A },		/*index: 754, 6.6791dB */
	{ 0x225 , 0x10 , 0x89 },		/*index: 755, 6.6321dB */
	{ 0x222 , 0x10 , 0x89 },		/*index: 756, 6.585dB */
	{ 0x21F , 0x10 , 0x88 },		/*index: 757, 6.538dB */
	{ 0x21C , 0x10 , 0x87 },		/*index: 758, 6.491dB */
	{ 0x21A , 0x10 , 0x86 },		/*index: 759, 6.4439dB */
	{ 0x217 , 0x10 , 0x86 },		/*index: 760, 6.3969dB */
	{ 0x214 , 0x10 , 0x85 },		/*index: 761, 6.3499dB */
	{ 0x211 , 0x10 , 0x84 },		/*index: 762, 6.3028dB */
	{ 0x20E , 0x10 , 0x84 },		/*index: 763, 6.2558dB */
	{ 0x20B , 0x10 , 0x83 },		/*index: 764, 6.2087dB */
	{ 0x208 , 0x10 , 0x82 },		/*index: 765, 6.1617dB */
	{ 0x206 , 0x10 , 0x81 },		/*index: 766, 6.1147dB */
	{ 0x203 , 0x10 , 0x81 },		/*index: 767, 6.0676dB */
	{ 0x200 , 0x10 , 0x80 },		/*index: 768, 6.0206dB */
	{ 0x1FD , 0x0 , 0xFF },		/*index: 769, 5.9736dB */
	{ 0x1FA , 0x0 , 0xFD },		/*index: 770, 5.9265dB */
	{ 0x1F8 , 0x0 , 0xFC },		/*index: 771, 5.8795dB */
	{ 0x1F5 , 0x0 , 0xFB },		/*index: 772, 5.8325dB */
	{ 0x1F2 , 0x0 , 0xF9 },		/*index: 773, 5.7854dB */
	{ 0x1F0 , 0x0 , 0xF8 },		/*index: 774, 5.7384dB */
	{ 0x1ED , 0x0 , 0xF6 },		/*index: 775, 5.6913dB */
	{ 0x1EA , 0x0 , 0xF5 },		/*index: 776, 5.6443dB */
	{ 0x1E8 , 0x0 , 0xF4 },		/*index: 777, 5.5973dB */
	{ 0x1E5 , 0x0 , 0xF3 },		/*index: 778, 5.5502dB */
	{ 0x1E2 , 0x0 , 0xF1 },		/*index: 779, 5.5032dB */
	{ 0x1E0 , 0x0 , 0xF0 },		/*index: 780, 5.4562dB */
	{ 0x1DD , 0x0 , 0xEF },		/*index: 781, 5.4091dB */
	{ 0x1DB , 0x0 , 0xED },		/*index: 782, 5.3621dB */
	{ 0x1D8 , 0x0 , 0xEC },		/*index: 783, 5.3151dB */
	{ 0x1D6 , 0x0 , 0xEB },		/*index: 784, 5.268dB */
	{ 0x1D3 , 0x0 , 0xE9 },		/*index: 785, 5.221dB */
	{ 0x1D0 , 0x0 , 0xE8 },		/*index: 786, 5.174dB */
	{ 0x1CE , 0x0 , 0xE7 },		/*index: 787, 5.1269dB */
	{ 0x1CB , 0x0 , 0xE6 },		/*index: 788, 5.0799dB */
	{ 0x1C9 , 0x0 , 0xE4 },		/*index: 789, 5.0328dB */
	{ 0x1C6 , 0x0 , 0xE3 },		/*index: 790, 4.9858dB */
	{ 0x1C4 , 0x0 , 0xE2 },		/*index: 791, 4.9388dB */
	{ 0x1C2 , 0x0 , 0xE1 },		/*index: 792, 4.8917dB */
	{ 0x1BF , 0x0 , 0xE0 },		/*index: 793, 4.8447dB */
	{ 0x1BD , 0x0 , 0xDE },		/*index: 794, 4.7977dB */
	{ 0x1BA , 0x0 , 0xDD },		/*index: 795, 4.7506dB */
	{ 0x1B8 , 0x0 , 0xDC },		/*index: 796, 4.7036dB */
	{ 0x1B6 , 0x0 , 0xDB },		/*index: 797, 4.6566dB */
	{ 0x1B3 , 0x0 , 0xDA },		/*index: 798, 4.6095dB */
	{ 0x1B1 , 0x0 , 0xD8 },		/*index: 799, 4.5625dB */
	{ 0x1AF , 0x0 , 0xD7 },		/*index: 800, 4.5154dB */
	{ 0x1AC , 0x0 , 0xD6 },		/*index: 801, 4.4684dB */
	{ 0x1AA , 0x0 , 0xD5 },		/*index: 802, 4.4214dB */
	{ 0x1A8 , 0x0 , 0xD4 },		/*index: 803, 4.3743dB */
	{ 0x1A5 , 0x0 , 0xD3 },		/*index: 804, 4.3273dB */
	{ 0x1A3 , 0x0 , 0xD2 },		/*index: 805, 4.2803dB */
	{ 0x1A1 , 0x0 , 0xD0 },		/*index: 806, 4.2332dB */
	{ 0x19F , 0x0 , 0xCF },		/*index: 807, 4.1862dB */
	{ 0x19C , 0x0 , 0xCE },		/*index: 808, 4.1392dB */
	{ 0x19A , 0x0 , 0xCD },		/*index: 809, 4.0921dB */
	{ 0x198 , 0x0 , 0xCC },		/*index: 810, 4.0451dB */
	{ 0x196 , 0x0 , 0xCB },		/*index: 811, 3.9981dB */
	{ 0x193 , 0x0 , 0xCA },		/*index: 812, 3.951dB */
	{ 0x191 , 0x0 , 0xC9 },		/*index: 813, 3.904dB */
	{ 0x18F , 0x0 , 0xC8 },		/*index: 814, 3.8569dB */
	{ 0x18D , 0x0 , 0xC6 },		/*index: 815, 3.8099dB */
	{ 0x18B , 0x0 , 0xC5 },		/*index: 816, 3.7629dB */
	{ 0x189 , 0x0 , 0xC4 },		/*index: 817, 3.7158dB */
	{ 0x187 , 0x0 , 0xC3 },		/*index: 818, 3.6688dB */
	{ 0x184 , 0x0 , 0xC2 },		/*index: 819, 3.6218dB */
	{ 0x182 , 0x0 , 0xC1 },		/*index: 820, 3.5747dB */
	{ 0x180 , 0x0 , 0xC0 },		/*index: 821, 3.5277dB */
	{ 0x17E , 0x0 , 0xBF },		/*index: 822, 3.4807dB */
	{ 0x17C , 0x0 , 0xBE },		/*index: 823, 3.4336dB */
	{ 0x17A , 0x0 , 0xBD },		/*index: 824, 3.3866dB */
	{ 0x178 , 0x0 , 0xBC },		/*index: 825, 3.3396dB */
	{ 0x176 , 0x0 , 0xBB },		/*index: 826, 3.2925dB */
	{ 0x174 , 0x0 , 0xBA },		/*index: 827, 3.2455dB */
	{ 0x172 , 0x0 , 0xB9 },		/*index: 828, 3.1984dB */
	{ 0x170 , 0x0 , 0xB8 },		/*index: 829, 3.1514dB */
	{ 0x16E , 0x0 , 0xB7 },		/*index: 830, 3.1044dB */
	{ 0x16C , 0x0 , 0xB6 },		/*index: 831, 3.0573dB */
	{ 0x16A , 0x0 , 0xB5 },		/*index: 832, 3.0103dB */
	{ 0x168 , 0x0 , 0xB4 },		/*index: 833, 2.9633dB */
	{ 0x166 , 0x0 , 0xB3 },		/*index: 834, 2.9162dB */
	{ 0x164 , 0x0 , 0xB2 },		/*index: 835, 2.8692dB */
	{ 0x162 , 0x0 , 0xB1 },		/*index: 836, 2.8222dB */
	{ 0x160 , 0x0 , 0xB0 },		/*index: 837, 2.7751dB */
	{ 0x15E , 0x0 , 0xAF },		/*index: 838, 2.7281dB */
	{ 0x15D , 0x0 , 0xAE },		/*index: 839, 2.681dB */
	{ 0x15B , 0x0 , 0xAD },		/*index: 840, 2.634dB */
	{ 0x159 , 0x0 , 0xAC },		/*index: 841, 2.587dB */
	{ 0x157 , 0x0 , 0xAB },		/*index: 842, 2.5399dB */
	{ 0x155 , 0x0 , 0xAB },		/*index: 843, 2.4929dB */
	{ 0x153 , 0x0 , 0xAA },		/*index: 844, 2.4459dB */
	{ 0x151 , 0x0 , 0xA9 },		/*index: 845, 2.3988dB */
	{ 0x150 , 0x0 , 0xA8 },		/*index: 846, 2.3518dB */
	{ 0x14E , 0x0 , 0xA7 },		/*index: 847, 2.3048dB */
	{ 0x14C , 0x0 , 0xA6 },		/*index: 848, 2.2577dB */
	{ 0x14A , 0x0 , 0xA5 },		/*index: 849, 2.2107dB */
	{ 0x148 , 0x0 , 0xA4 },		/*index: 850, 2.1637dB */
	{ 0x147 , 0x0 , 0xA3 },		/*index: 851, 2.1166dB */
	{ 0x145 , 0x0 , 0xA2 },		/*index: 852, 2.0696dB */
	{ 0x143 , 0x0 , 0xA2 },		/*index: 853, 2.0225dB */
	{ 0x141 , 0x0 , 0xA1 },		/*index: 854, 1.9755dB */
	{ 0x140 , 0x0 , 0xA0 },		/*index: 855, 1.9285dB */
	{ 0x13E , 0x0 , 0x9F },		/*index: 856, 1.8814dB */
	{ 0x13C , 0x0 , 0x9E },		/*index: 857, 1.8344dB */
	{ 0x13A , 0x0 , 0x9D },		/*index: 858, 1.7874dB */
	{ 0x139 , 0x0 , 0x9C },		/*index: 859, 1.7403dB */
	{ 0x137 , 0x0 , 0x9C },		/*index: 860, 1.6933dB */
	{ 0x135 , 0x0 , 0x9B },		/*index: 861, 1.6463dB */
	{ 0x134 , 0x0 , 0x9A },		/*index: 862, 1.5992dB */
	{ 0x132 , 0x0 , 0x99 },		/*index: 863, 1.5522dB */
	{ 0x130 , 0x0 , 0x98 },		/*index: 864, 1.5051dB */
	{ 0x12F , 0x0 , 0x97 },		/*index: 865, 1.4581dB */
	{ 0x12D , 0x0 , 0x97 },		/*index: 866, 1.4111dB */
	{ 0x12C , 0x0 , 0x96 },		/*index: 867, 1.364dB */
	{ 0x12A , 0x0 , 0x95 },		/*index: 868, 1.317dB */
	{ 0x128 , 0x0 , 0x94 },		/*index: 869, 1.27dB */
	{ 0x127 , 0x0 , 0x93 },		/*index: 870, 1.2229dB */
	{ 0x125 , 0x0 , 0x93 },		/*index: 871, 1.1759dB */
	{ 0x124 , 0x0 , 0x92 },		/*index: 872, 1.1289dB */
	{ 0x122 , 0x0 , 0x91 },		/*index: 873, 1.0818dB */
	{ 0x120 , 0x0 , 0x90 },		/*index: 874, 1.0348dB */
	{ 0x11F , 0x0 , 0x8F },		/*index: 875, 0.9878dB */
	{ 0x11D , 0x0 , 0x8F },		/*index: 876, 0.9407dB */
	{ 0x11C , 0x0 , 0x8E },		/*index: 877, 0.8937dB */
	{ 0x11A , 0x0 , 0x8D },		/*index: 878, 0.8466dB */
	{ 0x119 , 0x0 , 0x8C },		/*index: 879, 0.7996dB */
	{ 0x117 , 0x0 , 0x8C },		/*index: 880, 0.7526dB */
	{ 0x116 , 0x0 , 0x8B },		/*index: 881, 0.7055dB */
	{ 0x114 , 0x0 , 0x8A },		/*index: 882, 0.6585dB */
	{ 0x113 , 0x0 , 0x89 },		/*index: 883, 0.6115dB */
	{ 0x111 , 0x0 , 0x89 },		/*index: 884, 0.5644dB */
	{ 0x110 , 0x0 , 0x88 },		/*index: 885, 0.5174dB */
	{ 0x10E , 0x0 , 0x87 },		/*index: 886, 0.4704dB */
	{ 0x10D , 0x0 , 0x86 },		/*index: 887, 0.4233dB */
	{ 0x10B , 0x0 , 0x86 },		/*index: 888, 0.3763dB */
	{ 0x10A , 0x0 , 0x85 },		/*index: 889, 0.3293dB */
	{ 0x108 , 0x0 , 0x84 },		/*index: 890, 0.2822dB */
	{ 0x107 , 0x0 , 0x84 },		/*index: 891, 0.2352dB */
	{ 0x106 , 0x0 , 0x83 },		/*index: 892, 0.1881dB */
	{ 0x104 , 0x0 , 0x82 },		/*index: 893, 0.1411dB */
	{ 0x103 , 0x0 , 0x81 },		/*index: 894, 0.0941dB */
	{ 0x101 , 0x0 , 0x81 },		/*index: 895, 0.047dB */
	{ 0x100 , 0x0 , 0x80 },		/*index: 896, 0dB */
};

#else

#define MT9T002_GAIN_ROWS		(449)
#define MT9T002_GAIN_COLS		(4)
#define MT9T002_GAIN_DOUBLE		(64)
#define MT9T002_GAIN_0DB		(448)
const s16 MT9T002_GAIN_TABLE[MT9T002_GAIN_ROWS][MT9T002_GAIN_COLS] =
{
/* gain_value*256,  log2(gain)*1024,  register */
	{0x8000, 0x0030, 0x07FF},       /* index 000, 42db*/
	{0x7e9f, 0x0030, 0x07F0},       /* index 001,*/
	{0x7d42, 0x0030, 0x07E0},       /* index 002,*/
	{0x7be8, 0x0030, 0x07D0},       /* index 003,*/
	{0x7a93, 0x0030, 0x07C0},       /* index 004,*/
	{0x7941, 0x0030, 0x07B0},       /* index 005,*/
	{0x77f2, 0x0030, 0x07A0},       /* index 006,*/
	{0x76a8, 0x0030, 0x0790},       /* index 007,*/
	{0x7560, 0x0030, 0x0780},       /* index 008,*/
	{0x741d, 0x0030, 0x0770},       /* index 009,*/
	{0x72dd, 0x0030, 0x0760},       /* index 010,*/
	{0x71a0, 0x0030, 0x0750},       /* index 011,*/
	{0x7066, 0x0030, 0x0740},       /* index 012,*/
	{0x6f30, 0x0030, 0x0730},       /* index 013,*/
	{0x6dfe, 0x0030, 0x0720},       /* index 014,*/
	{0x6ccf, 0x0030, 0x0710},       /* index 015,*/
	{0x6ba2, 0x0030, 0x0700},       /* index 016,*/
	{0x6a7a, 0x0030, 0x06F0},       /* index 017,*/
	{0x6954, 0x0030, 0x06E0},       /* index 018,*/
	{0x6832, 0x0030, 0x06D0},       /* index 019,*/
	{0x6712, 0x0030, 0x06C0},       /* index 020,*/
	{0x65f6, 0x0030, 0x06B0},       /* index 021,*/
	{0x64dd, 0x0030, 0x06A0},       /* index 022,*/
	{0x63c7, 0x0030, 0x0690},       /* index 023,*/
	{0x62b4, 0x0030, 0x0680},       /* index 024,*/
	{0x61a3, 0x0030, 0x0670},       /* index 025,*/
	{0x6096, 0x0030, 0x0660},       /* index 026,*/
	{0x5f8c, 0x0030, 0x0650},       /* index 027,*/
	{0x5e84, 0x0030, 0x0640},       /* index 028,*/
	{0x5d80, 0x0030, 0x0630},       /* index 029,*/
	{0x5c7e, 0x0030, 0x0620},       /* index 030,*/
	{0x5b7f, 0x0030, 0x0610},       /* index 031,*/
	{0x5a82, 0x0030, 0x0600},       /* index 032,*/
	{0x5989, 0x0030, 0x05F0},       /* index 033,*/
	{0x5892, 0x0030, 0x05E0},       /* index 034,*/
	{0x579e, 0x0030, 0x05D0},       /* index 035,*/
	{0x56ac, 0x0030, 0x05C0},       /* index 036,*/
	{0x55bd, 0x0030, 0x05B0},       /* index 037,*/
	{0x54d1, 0x0030, 0x05A0},       /* index 038,*/
	{0x53e7, 0x0030, 0x0590},       /* index 039,*/
	{0x52ff, 0x0030, 0x0580},       /* index 040,*/
	{0x521b, 0x0030, 0x0570},       /* index 041,*/
	{0x5138, 0x0030, 0x0560},       /* index 042,*/
	{0x5058, 0x0030, 0x0550},       /* index 043,*/
	{0x4f7b, 0x0030, 0x0540},       /* index 044,*/
	{0x4e9f, 0x0030, 0x0530},       /* index 045,*/
	{0x4dc7, 0x0030, 0x0520},       /* index 046,*/
	{0x4cf0, 0x0030, 0x0510},       /* index 047,*/
	{0x4c1c, 0x0030, 0x0500},       /* index 048,*/
	{0x4b4a, 0x0030, 0x04F0},       /* index 049,*/
	{0x4a7a, 0x0030, 0x04E0},       /* index 050,*/
	{0x49ad, 0x0030, 0x04D0},       /* index 051,*/
	{0x48e2, 0x0030, 0x04C0},       /* index 052,*/
	{0x4819, 0x0030, 0x04B0},       /* index 053,*/
	{0x4752, 0x0030, 0x04A0},       /* index 054,*/
	{0x468d, 0x0030, 0x0490},       /* index 055,*/
	{0x45cb, 0x0030, 0x0480},       /* index 056,*/
	{0x450a, 0x0030, 0x0470},       /* index 057,*/
	{0x444c, 0x0030, 0x0460},       /* index 058,*/
	{0x4390, 0x0030, 0x0450},       /* index 059,*/
	{0x42d5, 0x0030, 0x0440},       /* index 060,*/
	{0x421d, 0x0030, 0x0430},       /* index 061,*/
	{0x4167, 0x0030, 0x0420},       /* index 062,*/
	{0x40b2, 0x0030, 0x0410},       /* index 063,*/
	{0x4000, 0x0030, 0x0400},       /* index 064,*/

	{0x3f50, 0x0030, 0x03F8},       /* index 065, gain = 36.055475 dB */
	{0x3ea1, 0x0030, 0x03F0},       /* index 066, gain = 35.986811 dB */
	{0x3df4, 0x0030, 0x03E8},       /* index 067, gain = 35.917600 dB */
	{0x3d49, 0x0030, 0x03E0},       /* index 068, gain = 35.847834 dB */
	{0x3ca0, 0x0030, 0x03D8},       /* index 069, gain = 35.777502 dB */
	{0x3bf9, 0x0030, 0x03D0},       /* index 070, gain = 35.706597 dB */
	{0x3b54, 0x0030, 0x03C8},       /* index 071, gain = 35.635107 dB */
	{0x3ab0, 0x0030, 0x03C0},       /* index 072, gain = 35.563025 dB */
	{0x3a0e, 0x0030, 0x03B8},       /* index 073, gain = 35.490339 dB */
	{0x396e, 0x0030, 0x03B0},       /* index 074, gain = 35.417040 dB */
	{0x38d0, 0x0030, 0x03A8},       /* index 075, gain = 35.343117 dB */
	{0x3833, 0x0030, 0x03A0},       /* index 076, gain = 35.268560 dB */
	{0x3798, 0x0030, 0x0398},       /* index 077, gain = 35.193357 dB */
	{0x36ff, 0x0030, 0x0390},       /* index 078, gain = 35.117497 dB */
	{0x3667, 0x0030, 0x0388},       /* index 079, gain = 35.040969 dB */
	{0x35d1, 0x0030, 0x0380},       /* index 080, gain = 34.963761 dB */
	{0x353d, 0x0030, 0x0378},       /* index 081, gain = 34.885860 dB */
	{0x34aa, 0x0030, 0x0370},       /* index 082, gain = 34.807254 dB */
	{0x3419, 0x0030, 0x0368},       /* index 083, gain = 34.727930 dB */
	{0x3389, 0x0030, 0x0360},       /* index 084, gain = 34.647875 dB */
	{0x32fb, 0x0030, 0x0358},       /* index 085, gain = 34.567076 dB */
	{0x326e, 0x0030, 0x0350},       /* index 086, gain = 34.485517 dB */
	{0x31e3, 0x0030, 0x0348},       /* index 087, gain = 34.403186 dB */
	{0x315a, 0x0030, 0x0340},       /* index 088, gain = 34.320067 dB */
	{0x30d2, 0x0030, 0x0338},       /* index 089, gain = 34.236145 dB */
	{0x304b, 0x0030, 0x0330},       /* index 090, gain = 34.151404 dB */
	{0x2fc6, 0x0030, 0x0328},       /* index 091, gain = 34.065828 dB */
	{0x2f42, 0x0030, 0x0320},       /* index 092, gain = 33.979400 dB */
	{0x2ec0, 0x0030, 0x0318},       /* index 093, gain = 33.892104 dB */
	{0x2e3f, 0x0030, 0x0310},       /* index 094, gain = 33.803922 dB */
	{0x2dbf, 0x0030, 0x0308},       /* index 095, gain = 33.714835 dB */
	{0x2d41, 0x0030, 0x0300},       /* index 096, gain = 33.624825 dB */
	{0x2cc4, 0x0030, 0x02F8},       /* index 097, gain = 33.533872 dB */
	{0x2c49, 0x0030, 0x02F0},       /* index 098, gain = 33.441957 dB */
	{0x2bcf, 0x0030, 0x02E8},       /* index 099, gain = 33.349059 dB */
	{0x2b56, 0x0030, 0x02E0},       /* index 100, gain = 33.255157 dB */
	{0x2adf, 0x0030, 0x02D8},       /* index 101, gain = 33.160228 dB */
	{0x2a68, 0x0030, 0x02D0},       /* index 102, gain = 33.064250 dB */
	{0x29f3, 0x0030, 0x02C8},       /* index 103, gain = 32.967200 dB */
	{0x2980, 0x0030, 0x02C0},       /* index 104, gain = 32.869054 dB */
	{0x290d, 0x0030, 0x02B8},       /* index 105, gain = 32.769785 dB */
	{0x289c, 0x0030, 0x02B0},       /* index 106, gain = 32.669369 dB */
	{0x282c, 0x0030, 0x02A8},       /* index 107, gain = 32.567779 dB */
	{0x27bd, 0x0030, 0x02A0},       /* index 108, gain = 32.464986 dB */
	{0x2750, 0x0030, 0x0298},       /* index 109, gain = 32.360962 dB */
	{0x26e3, 0x0030, 0x0290},       /* index 110, gain = 32.255677 dB */
	{0x2678, 0x0030, 0x0288},       /* index 111, gain = 32.149100 dB */
	{0x260e, 0x0030, 0x0280},       /* index 112, gain = 32.041200 dB */
	{0x25a5, 0x0030, 0x0278},       /* index 113, gain = 31.931942 dB */
	{0x253d, 0x0030, 0x0270},       /* index 114, gain = 31.821292 dB */
	{0x24d7, 0x0030, 0x0268},       /* index 115, gain = 31.709215 dB */
	{0x2471, 0x0030, 0x0260},       /* index 116, gain = 31.595672 dB */
	{0x240c, 0x0030, 0x0258},       /* index 117, gain = 31.480625 dB */
	{0x23a9, 0x0030, 0x0250},       /* index 118, gain = 31.364034 dB */
	{0x2347, 0x0030, 0x0248},       /* index 119, gain = 31.245857 dB */
	{0x22e5, 0x0030, 0x0240},       /* index 120, gain = 31.126050 dB */
	{0x2285, 0x0030, 0x0238},       /* index 121, gain = 31.004567 dB */
	{0x2226, 0x0030, 0x0230},       /* index 122, gain = 30.881361 dB */
	{0x21c8, 0x0030, 0x0228},       /* index 123, gain = 30.756382 dB */
	{0x216b, 0x0030, 0x0220},       /* index 124, gain = 30.629578 dB */
	{0x210f, 0x0030, 0x0218},       /* index 125, gain = 30.500896 dB */
	{0x20b3, 0x0030, 0x0210},       /* index 126, gain = 30.370279 dB */
	{0x2059, 0x0030, 0x0208},       /* index 127, gain = 30.237667 dB */
	{0x2000, 0x0030, 0x0200},       /* index 128, gain = 30.103000 dB */

	{0x1fa8, 0x0030, 0x01FC},       /* index 129, gain = 30.034875 dB */
	{0x1f50, 0x0030, 0x01F8},       /* index 130, gain = 29.966211 dB */
	{0x1efa, 0x0030, 0x01F4},       /* index 131, gain = 29.897000 dB */
	{0x1ea5, 0x0030, 0x01F0},       /* index 132, gain = 29.827234 dB */
	{0x1e50, 0x0030, 0x01EC},       /* index 133, gain = 29.756902 dB */
	{0x1dfd, 0x0030, 0x01E8},       /* index 134, gain = 29.685997 dB */
	{0x1daa, 0x0030, 0x01E4},       /* index 135, gain = 29.614508 dB */
	{0x1d58, 0x0030, 0x01E0},       /* index 136, gain = 29.542425 dB */
	{0x1d07, 0x0030, 0x01DC},       /* index 137, gain = 29.469739 dB */
	{0x1cb7, 0x0030, 0x01D8},       /* index 138, gain = 29.396440 dB */
	{0x1c68, 0x0030, 0x01D4},       /* index 139, gain = 29.322517 dB */
	{0x1c1a, 0x0030, 0x01D0},       /* index 140, gain = 29.247960 dB */
	{0x1bcc, 0x0030, 0x01CC},       /* index 141, gain = 29.172757 dB */
	{0x1b7f, 0x0030, 0x01C8},       /* index 142, gain = 29.096897 dB */
	{0x1b34, 0x0030, 0x01C4},       /* index 143, gain = 29.020369 dB */
	{0x1ae9, 0x0030, 0x01C0},       /* index 144, gain = 28.943161 dB */
	{0x1a9e, 0x0030, 0x01BC},       /* index 145, gain = 28.865260 dB */
	{0x1a55, 0x0030, 0x01B8},       /* index 146, gain = 28.786654 dB */
	{0x1a0c, 0x0030, 0x01B4},       /* index 147, gain = 28.707330 dB */
	{0x19c5, 0x0030, 0x01B0},       /* index 148, gain = 28.627275 dB */
	{0x197e, 0x0030, 0x01AC},       /* index 149, gain = 28.546476 dB */
	{0x1937, 0x0030, 0x01A8},       /* index 150, gain = 28.464917 dB */
	{0x18f2, 0x0030, 0x01A4},       /* index 151, gain = 28.382586 dB */
	{0x18ad, 0x0030, 0x01A0},       /* index 152, gain = 28.299467 dB */
	{0x1869, 0x0030, 0x019C},       /* index 153, gain = 28.215545 dB */
	{0x1826, 0x0030, 0x0198},       /* index 154, gain = 28.130804 dB */
	{0x17e3, 0x0030, 0x0194},       /* index 155, gain = 28.045228 dB */
	{0x17a1, 0x0030, 0x0190},       /* index 156, gain = 27.958800 dB */
	{0x1760, 0x0030, 0x018C},       /* index 157, gain = 27.871504 dB */
	{0x171f, 0x0030, 0x0188},       /* index 158, gain = 27.783322 dB */
	{0x16e0, 0x0030, 0x0184},       /* index 159, gain = 27.694235 dB */
	{0x16a1, 0x0030, 0x0180},       /* index 160, gain = 27.604225 dB */
	{0x1662, 0x0030, 0x017C},       /* index 161, gain = 27.513272 dB */
	{0x1624, 0x0030, 0x0178},       /* index 162, gain = 27.421357 dB */
	{0x15e7, 0x0030, 0x0174},       /* index 163, gain = 27.328459 dB */
	{0x15ab, 0x0030, 0x0170},       /* index 164, gain = 27.234557 dB */
	{0x156f, 0x0030, 0x016C},       /* index 165, gain = 27.139628 dB */
	{0x1534, 0x0030, 0x0168},       /* index 166, gain = 27.043650 dB */
	{0x14fa, 0x0030, 0x0164},       /* index 167, gain = 26.946600 dB */
	{0x14c0, 0x0030, 0x0160},       /* index 168, gain = 26.848454 dB */
	{0x1487, 0x0030, 0x015C},       /* index 169, gain = 26.749185 dB */
	{0x144e, 0x0030, 0x0158},       /* index 170, gain = 26.648769 dB */
	{0x1416, 0x0030, 0x0154},       /* index 171, gain = 26.547179 dB */
	{0x13df, 0x0030, 0x0150},       /* index 172, gain = 26.444386 dB */
	{0x13a8, 0x0030, 0x014C},       /* index 173, gain = 26.340362 dB */
	{0x1372, 0x0030, 0x0148},       /* index 174, gain = 26.235077 dB */
	{0x133c, 0x0030, 0x0144},       /* index 175, gain = 26.128501 dB */
	{0x1307, 0x0030, 0x0140},       /* index 176, gain = 26.020600 dB */
	{0x12d3, 0x0030, 0x013C},       /* index 177, gain = 25.911342 dB */
	{0x129f, 0x0030, 0x0138},       /* index 178, gain = 25.800692 dB */
	{0x126b, 0x0030, 0x0134},       /* index 179, gain = 25.688615 dB */
	{0x1238, 0x0030, 0x0130},       /* index 180, gain = 25.575072 dB */
	{0x1206, 0x0030, 0x012C},       /* index 181, gain = 25.460025 dB */
	{0x11d5, 0x0030, 0x0128},       /* index 182, gain = 25.343435 dB */
	{0x11a3, 0x0030, 0x0124},       /* index 183, gain = 25.225257 dB */
	{0x1173, 0x0030, 0x0120},       /* index 184, gain = 25.105450 dB */
	{0x1143, 0x0030, 0x011C},       /* index 185, gain = 24.983967 dB */
	{0x1113, 0x0030, 0x0118},       /* index 186, gain = 24.860761 dB */
	{0x10e4, 0x0030, 0x0114},       /* index 187, gain = 24.735782 dB */
	{0x10b5, 0x0030, 0x0110},       /* index 188, gain = 24.608978 dB */
	{0x1087, 0x0030, 0x010C},       /* index 189, gain = 24.480296 dB */
	{0x105a, 0x0030, 0x0108},       /* index 190, gain = 24.349679 dB */
	{0x102d, 0x0030, 0x0104},       /* index 191, gain = 24.217067 dB */
	{0x1000, 0x0030, 0x0100},       /* index 192, gain = 24.082400 dB */

	{0x0fd4, 0x0030, 0x00FE},       /* index 193, gain = 24.014275 dB */
	{0x0fa8, 0x0030, 0x00FC},       /* index 194, gain = 23.945611 dB */
	{0x0f7d, 0x0030, 0x00FA},       /* index 195, gain = 23.876401 dB */
	{0x0f52, 0x0030, 0x00F8},       /* index 196, gain = 23.806634 dB */
	{0x0f28, 0x0030, 0x00F6},       /* index 197, gain = 23.736302 dB */
	{0x0efe, 0x0030, 0x00F4},       /* index 198, gain = 23.665397 dB */
	{0x0ed5, 0x0030, 0x00F2},       /* index 199, gain = 23.593908 dB */
	{0x0eac, 0x0030, 0x00F0},       /* index 200, gain = 23.521825 dB */
	{0x0e84, 0x0030, 0x00EE},       /* index 201, gain = 23.449139 dB */
	{0x0e5c, 0x0030, 0x00EC},       /* index 202, gain = 23.375840 dB */
	{0x0e34, 0x0030, 0x00EA},       /* index 203, gain = 23.301917 dB */
	{0x0e0d, 0x0030, 0x00E8},       /* index 204, gain = 23.227360 dB */
	{0x0de6, 0x0030, 0x00E6},       /* index 205, gain = 23.152157 dB */
	{0x0dc0, 0x0030, 0x00E4},       /* index 206, gain = 23.076297 dB */
	{0x0d9a, 0x0030, 0x00E2},       /* index 207, gain = 22.999769 dB */
	{0x0d74, 0x0030, 0x00E0},       /* index 208, gain = 22.922561 dB */
	{0x0d4f, 0x0030, 0x00DE},       /* index 209, gain = 22.844660 dB */
	{0x0d2b, 0x0030, 0x00DC},       /* index 210, gain = 22.766054 dB */
	{0x0d06, 0x0030, 0x00DA},       /* index 211, gain = 22.686730 dB */
	{0x0ce2, 0x0030, 0x00D8},       /* index 212, gain = 22.606675 dB */
	{0x0cbf, 0x0030, 0x00D6},       /* index 213, gain = 22.525876 dB */
	{0x0c9c, 0x0030, 0x00D4},       /* index 214, gain = 22.444318 dB */
	{0x0c79, 0x0030, 0x00D2},       /* index 215, gain = 22.361986 dB */
	{0x0c56, 0x0030, 0x00D0},       /* index 216, gain = 22.278867 dB */
	{0x0c34, 0x0030, 0x00CE},       /* index 217, gain = 22.194945 dB */
	{0x0c13, 0x0030, 0x00CC},       /* index 218, gain = 22.110204 dB */
	{0x0bf1, 0x0030, 0x00CA},       /* index 219, gain = 22.024628 dB */
	{0x0bd1, 0x0030, 0x00C8},       /* index 220, gain = 21.938200 dB */
	{0x0bb0, 0x0030, 0x00C6},       /* index 221, gain = 21.850904 dB */
	{0x0b90, 0x0030, 0x00C4},       /* index 222, gain = 21.762722 dB */
	{0x0b70, 0x0030, 0x00C2},       /* index 223, gain = 21.673635 dB */
	{0x0b50, 0x0030, 0x00C0},       /* index 224, gain = 21.583625 dB */
	{0x0b31, 0x0030, 0x00BE},       /* index 225, gain = 21.492672 dB */
	{0x0b12, 0x0030, 0x00BC},       /* index 226, gain = 21.400757 dB */
	{0x0af4, 0x0030, 0x00BA},       /* index 227, gain = 21.307859 dB */
	{0x0ad6, 0x0030, 0x00B8},       /* index 228, gain = 21.213957 dB */
	{0x0ab8, 0x0030, 0x00B6},       /* index 229, gain = 21.119028 dB */
	{0x0a9a, 0x0030, 0x00B4},       /* index 230, gain = 21.023050 dB */
	{0x0a7d, 0x0030, 0x00B2},       /* index 231, gain = 20.926000 dB */
	{0x0a60, 0x0030, 0x00B0},       /* index 232, gain = 20.827854 dB */
	{0x0a43, 0x0030, 0x00AE},       /* index 233, gain = 20.728585 dB */
	{0x0a27, 0x0030, 0x00AC},       /* index 234, gain = 20.628169 dB */
	{0x0a0b, 0x0030, 0x00AA},       /* index 235, gain = 20.526579 dB */
	{0x09ef, 0x0030, 0x00A8},       /* index 236, gain = 20.423786 dB */
	{0x09d4, 0x0030, 0x00A6},       /* index 237, gain = 20.319762 dB */
	{0x09b9, 0x0030, 0x00A4},       /* index 238, gain = 20.214477 dB */
	{0x099e, 0x0030, 0x00A2},       /* index 239, gain = 20.107901 dB */
	{0x0983, 0x0030, 0x00A0},       /* index 240, gain = 20.000000 dB */
	{0x0969, 0x0030, 0x009E},       /* index 241, gain = 19.890742 dB */
	{0x094f, 0x0030, 0x009C},       /* index 242, gain = 19.780092 dB */
	{0x0936, 0x0030, 0x009A},       /* index 243, gain = 19.668015 dB */
	{0x091c, 0x0030, 0x0098},       /* index 244, gain = 19.554472 dB */
	{0x0903, 0x0030, 0x0096},       /* index 245, gain = 19.439426 dB */
	{0x08ea, 0x0030, 0x0094},       /* index 246, gain = 19.322835 dB */
	{0x08d2, 0x0030, 0x0092},       /* index 247, gain = 19.204657 dB */
	{0x08b9, 0x0030, 0x0090},       /* index 248, gain = 19.084850 dB */
	{0x08a1, 0x0030, 0x008E},       /* index 249, gain = 18.963367 dB */
	{0x088a, 0x0030, 0x008C},       /* index 250, gain = 18.840161 dB */
	{0x0872, 0x0030, 0x008A},       /* index 251, gain = 18.715182 dB */
	{0x085b, 0x0030, 0x0088},       /* index 252, gain = 18.588379 dB */
	{0x0844, 0x0030, 0x0086},       /* index 253, gain = 18.459696 dB */
	{0x082d, 0x0030, 0x0084},       /* index 254, gain = 18.329079 dB */
	{0x0816, 0x0030, 0x0082},       /* index 255, gain = 18.196467 dB */
	{0x0800, 0x0030, 0x0080},       /* index 256, gain = 18.061800 dB */
	{0x07ea, 0x0020, 0x00FE},       /* index 257, gain = 17.993675 dB */
	{0x07d4, 0x0020, 0x00FC},       /* index 258, gain = 17.925011 dB */
	{0x07bf, 0x0020, 0x00FA},       /* index 259, gain = 17.855801 dB */
	{0x07a9, 0x0020, 0x00F8},       /* index 260, gain = 17.786034 dB */
	{0x0794, 0x0020, 0x00F6},       /* index 261, gain = 17.715703 dB */
	{0x077f, 0x0020, 0x00F4},       /* index 262, gain = 17.644797 dB */
	{0x076a, 0x0020, 0x00F2},       /* index 263, gain = 17.573308 dB */
	{0x0756, 0x0020, 0x00F0},       /* index 264, gain = 17.501225 dB */
	{0x0742, 0x0020, 0x00EE},       /* index 265, gain = 17.428540 dB */
	{0x072e, 0x0020, 0x00EC},       /* index 266, gain = 17.355240 dB */
	{0x071a, 0x0020, 0x00EA},       /* index 267, gain = 17.281318 dB */
	{0x0706, 0x0020, 0x00E8},       /* index 268, gain = 17.206760 dB */
	{0x06f3, 0x0020, 0x00E6},       /* index 269, gain = 17.131557 dB */
	{0x06e0, 0x0020, 0x00E4},       /* index 270, gain = 17.055697 dB */
	{0x06cd, 0x0020, 0x00E2},       /* index 271, gain = 16.979169 dB */
	{0x06ba, 0x0020, 0x00E0},       /* index 272, gain = 16.901961 dB */
	{0x06a8, 0x0020, 0x00DE},       /* index 273, gain = 16.824060 dB */
	{0x0695, 0x0020, 0x00DC},       /* index 274, gain = 16.745454 dB */
	{0x0683, 0x0020, 0x00DA},       /* index 275, gain = 16.666130 dB */
	{0x0671, 0x0020, 0x00D8},       /* index 276, gain = 16.586075 dB */
	{0x065f, 0x0020, 0x00D6},       /* index 277, gain = 16.505276 dB */
	{0x064e, 0x0020, 0x00D4},       /* index 278, gain = 16.423718 dB */
	{0x063c, 0x0020, 0x00D2},       /* index 279, gain = 16.341386 dB */
	{0x062b, 0x0020, 0x00D0},       /* index 280, gain = 16.258267 dB */
	{0x061a, 0x0020, 0x00CE},       /* index 281, gain = 16.174345 dB */
	{0x0609, 0x0020, 0x00CC},       /* index 282, gain = 16.089604 dB */
	{0x05f9, 0x0020, 0x00CA},       /* index 283, gain = 16.004028 dB */
	{0x05e8, 0x0020, 0x00C8},       /* index 284, gain = 15.917600 dB */
	{0x05d8, 0x0020, 0x00C6},       /* index 285, gain = 15.830304 dB */
	{0x05c8, 0x0020, 0x00C4},       /* index 286, gain = 15.742122 dB */
	{0x05b8, 0x0020, 0x00C2},       /* index 287, gain = 15.653035 dB */
	{0x05a8, 0x0020, 0x00C0},       /* index 288, gain = 15.563025 dB */
	{0x0599, 0x0020, 0x00BE},       /* index 289, gain = 15.472072 dB */
	{0x0589, 0x0020, 0x00BC},       /* index 290, gain = 15.380157 dB */
	{0x057a, 0x0020, 0x00BA},       /* index 291, gain = 15.287259 dB */
	{0x056b, 0x0020, 0x00B8},       /* index 292, gain = 15.193357 dB */
	{0x055c, 0x0020, 0x00B6},       /* index 293, gain = 15.098428 dB */
	{0x054d, 0x0020, 0x00B4},       /* index 294, gain = 15.002451 dB */
	{0x053e, 0x0020, 0x00B2},       /* index 295, gain = 14.905400 dB */
	{0x0530, 0x0020, 0x00B0},       /* index 296, gain = 14.807254 dB */
	{0x0522, 0x0020, 0x00AE},       /* index 297, gain = 14.707985 dB */
	{0x0514, 0x0020, 0x00AC},       /* index 298, gain = 14.607569 dB */
	{0x0506, 0x0020, 0x00AA},       /* index 299, gain = 14.505979 dB */
	{0x04f8, 0x0020, 0x00A8},       /* index 300, gain = 14.403186 dB */
	{0x04ea, 0x0020, 0x00A6},       /* index 301, gain = 14.299162 dB */
	{0x04dc, 0x0020, 0x00A4},       /* index 302, gain = 14.193877 dB */
	{0x04cf, 0x0020, 0x00A2},       /* index 303, gain = 14.087301 dB */
	{0x04c2, 0x0020, 0x00A0},       /* index 304, gain = 13.979400 dB */
	{0x04b5, 0x0020, 0x009E},       /* index 305, gain = 13.870142 dB */
	{0x04a8, 0x0020, 0x009C},       /* index 306, gain = 13.759492 dB */
	{0x049b, 0x0020, 0x009A},       /* index 307, gain = 13.647415 dB */
	{0x048e, 0x0020, 0x0098},       /* index 308, gain = 13.533872 dB */
	{0x0482, 0x0020, 0x0096},       /* index 309, gain = 13.418826 dB */
	{0x0475, 0x0020, 0x0094},       /* index 310, gain = 13.302235 dB */
	{0x0469, 0x0020, 0x0092},       /* index 311, gain = 13.184058 dB */
	{0x045d, 0x0020, 0x0090},       /* index 312, gain = 13.064250 dB */
	{0x0451, 0x0020, 0x008E},       /* index 313, gain = 12.942767 dB */
	{0x0445, 0x0020, 0x008C},       /* index 314, gain = 12.819561 dB */
	{0x0439, 0x0020, 0x008A},       /* index 315, gain = 12.694582 dB */
	{0x042d, 0x0020, 0x0088},       /* index 316, gain = 12.567779 dB */
	{0x0422, 0x0020, 0x0086},       /* index 317, gain = 12.439096 dB */
	{0x0416, 0x0020, 0x0084},       /* index 318, gain = 12.308479 dB */
	{0x040b, 0x0020, 0x0082},       /* index 319, gain = 12.175867 dB */
	{0x0400, 0x0020, 0x0080},       /* index 320, gain = 12.041200 dB */
	{0x03f5, 0x0010, 0x00FE},       /* index 321, gain = 11.973075 dB */
	{0x03ea, 0x0010, 0x00FC},       /* index 322, gain = 11.904411 dB */
	{0x03df, 0x0010, 0x00FA},       /* index 323, gain = 11.835201 dB */
	{0x03d5, 0x0010, 0x00F8},       /* index 324, gain = 11.765434 dB */
	{0x03ca, 0x0010, 0x00F6},       /* index 325, gain = 11.695103 dB */
	{0x03c0, 0x0010, 0x00F4},       /* index 326, gain = 11.624197 dB */
	{0x03b5, 0x0010, 0x00F2},       /* index 327, gain = 11.552708 dB */
	{0x03ab, 0x0010, 0x00F0},       /* index 328, gain = 11.480625 dB */
	{0x03a1, 0x0010, 0x00EE},       /* index 329, gain = 11.407940 dB */
	{0x0397, 0x0010, 0x00EC},       /* index 330, gain = 11.334641 dB */
	{0x038d, 0x0010, 0x00EA},       /* index 331, gain = 11.260718 dB */
	{0x0383, 0x0010, 0x00E8},       /* index 332, gain = 11.186160 dB */
	{0x037a, 0x0010, 0x00E6},       /* index 333, gain = 11.110957 dB */
	{0x0370, 0x0010, 0x00E4},       /* index 334, gain = 11.035097 dB */
	{0x0366, 0x0010, 0x00E2},       /* index 335, gain = 10.958569 dB */
	{0x035d, 0x0010, 0x00E0},       /* index 336, gain = 10.881361 dB */
	{0x0354, 0x0010, 0x00DE},       /* index 337, gain = 10.803460 dB */
	{0x034b, 0x0010, 0x00DC},       /* index 338, gain = 10.724854 dB */
	{0x0342, 0x0010, 0x00DA},       /* index 339, gain = 10.645530 dB */
	{0x0339, 0x0010, 0x00D8},       /* index 340, gain = 10.565476 dB */
	{0x0330, 0x0010, 0x00D6},       /* index 341, gain = 10.484676 dB */
	{0x0327, 0x0010, 0x00D4},       /* index 342, gain = 10.403118 dB */
	{0x031e, 0x0010, 0x00D2},       /* index 343, gain = 10.320786 dB */
	{0x0316, 0x0010, 0x00D0},       /* index 344, gain = 10.237667 dB */
	{0x030d, 0x0010, 0x00CE},       /* index 345, gain = 10.153745 dB */
	{0x0305, 0x0010, 0x00CC},       /* index 346, gain = 10.069004 dB */
	{0x02fc, 0x0010, 0x00CA},       /* index 347, gain = 9.983428 dB */
	{0x02f4, 0x0010, 0x00C8},       /* index 348, gain = 9.897000 dB */
	{0x02ec, 0x0010, 0x00C6},       /* index 349, gain = 9.809704 dB */
	{0x02e4, 0x0010, 0x00C4},       /* index 350, gain = 9.721522 dB */
	{0x02dc, 0x0010, 0x00C2},       /* index 351, gain = 9.632435 dB */
	{0x02d4, 0x0010, 0x00C0},       /* index 352, gain = 9.542425 dB */
	{0x02cc, 0x0010, 0x00BE},       /* index 353, gain = 9.451473 dB */
	{0x02c5, 0x0010, 0x00BC},       /* index 354, gain = 9.359558 dB */
	{0x02bd, 0x0010, 0x00BA},       /* index 355, gain = 9.266659 dB */
	{0x02b5, 0x0010, 0x00B8},       /* index 356, gain = 9.172757 dB */
	{0x02ae, 0x0010, 0x00B6},       /* index 357, gain = 9.077828 dB */
	{0x02a7, 0x0010, 0x00B4},       /* index 358, gain = 8.981851 dB */
	{0x029f, 0x0010, 0x00B2},       /* index 359, gain = 8.884801 dB */
	{0x0298, 0x0010, 0x00B0},       /* index 360, gain = 8.786654 dB */
	{0x0291, 0x0010, 0x00AE},       /* index 361, gain = 8.687385 dB */
	{0x028a, 0x0010, 0x00AC},       /* index 362, gain = 8.586969 dB */
	{0x0283, 0x0010, 0x00AA},       /* index 363, gain = 8.485379 dB */
	{0x027c, 0x0010, 0x00A8},       /* index 364, gain = 8.382586 dB */
	{0x0275, 0x0010, 0x00A6},       /* index 365, gain = 8.278562 dB */
	{0x026e, 0x0010, 0x00A4},       /* index 366, gain = 8.173277 dB */
	{0x0268, 0x0010, 0x00A2},       /* index 367, gain = 8.066701 dB */
	{0x0261, 0x0010, 0x00A0},       /* index 368, gain = 7.958800 dB */
	{0x025a, 0x0010, 0x009E},       /* index 369, gain = 7.849542 dB */
	{0x0254, 0x0010, 0x009C},       /* index 370, gain = 7.738892 dB */
	{0x024d, 0x0010, 0x009A},       /* index 371, gain = 7.626815 dB */
	{0x0247, 0x0010, 0x0098},       /* index 372, gain = 7.513272 dB */
	{0x0241, 0x0010, 0x0096},       /* index 373, gain = 7.398226 dB */
	{0x023b, 0x0010, 0x0094},       /* index 374, gain = 7.281635 dB */
	{0x0234, 0x0010, 0x0092},       /* index 375, gain = 7.163458 dB */
	{0x022e, 0x0010, 0x0090},       /* index 376, gain = 7.043650 dB */
	{0x0228, 0x0010, 0x008E},       /* index 377, gain = 6.922167 dB */
	{0x0222, 0x0010, 0x008C},       /* index 378, gain = 6.798961 dB */
	{0x021c, 0x0010, 0x008A},       /* index 379, gain = 6.673982 dB */
	{0x0217, 0x0010, 0x0088},       /* index 380, gain = 6.547179 dB */
	{0x0211, 0x0010, 0x0086},       /* index 381, gain = 6.418496 dB */
	{0x020b, 0x0010, 0x0084},       /* index 382, gain = 6.287879 dB */
	{0x0206, 0x0010, 0x0082},       /* index 383, gain = 6.155268 dB */
	{0x0200, 0x0010, 0x0080},       /* index 384, gain = 6.020600 dB */
	{0x01fa, 0x0000, 0x00FE},       /* index 385, gain = 5.952475 dB */
	{0x01f5, 0x0000, 0x00FC},       /* index 386, gain = 5.883811 dB */
	{0x01f0, 0x0000, 0x00FA},       /* index 387, gain = 5.814601 dB */
	{0x01ea, 0x0000, 0x00F8},       /* index 388, gain = 5.744834 dB */
	{0x01e5, 0x0000, 0x00F6},       /* index 389, gain = 5.674503 dB */
	{0x01e0, 0x0000, 0x00F4},       /* index 390, gain = 5.603597 dB */
	{0x01db, 0x0000, 0x00F2},       /* index 391, gain = 5.532108 dB */
	{0x01d6, 0x0000, 0x00F0},       /* index 392, gain = 5.460025 dB */
	{0x01d0, 0x0000, 0x00EE},       /* index 393, gain = 5.387340 dB */
	{0x01cb, 0x0000, 0x00EC},       /* index 394, gain = 5.314041 dB */
	{0x01c6, 0x0000, 0x00EA},       /* index 395, gain = 5.240118 dB */
	{0x01c2, 0x0000, 0x00E8},       /* index 396, gain = 5.165560 dB */
	{0x01bd, 0x0000, 0x00E6},       /* index 397, gain = 5.090357 dB */
	{0x01b8, 0x0000, 0x00E4},       /* index 398, gain = 5.014498 dB */
	{0x01b3, 0x0000, 0x00E2},       /* index 399, gain = 4.937969 dB */
	{0x01af, 0x0000, 0x00E0},       /* index 400, gain = 4.860761 dB */
	{0x01aa, 0x0000, 0x00DE},       /* index 401, gain = 4.782860 dB */
	{0x01a5, 0x0000, 0x00DC},       /* index 402, gain = 4.704254 dB */
	{0x01a1, 0x0000, 0x00DA},       /* index 403, gain = 4.624930 dB */
	{0x019c, 0x0000, 0x00D8},       /* index 404, gain = 4.544876 dB */
	{0x0198, 0x0000, 0x00D6},       /* index 405, gain = 4.464076 dB */
	{0x0193, 0x0000, 0x00D4},       /* index 406, gain = 4.382518 dB */
	{0x018f, 0x0000, 0x00D2},       /* index 407, gain = 4.300187 dB */
	{0x018b, 0x0000, 0x00D0},       /* index 408, gain = 4.217067 dB */
	{0x0187, 0x0000, 0x00CE},       /* index 409, gain = 4.133145 dB */
	{0x0182, 0x0000, 0x00CC},       /* index 410, gain = 4.048404 dB */
	{0x017e, 0x0000, 0x00CA},       /* index 411, gain = 3.962828 dB */
	{0x017a, 0x0000, 0x00C8},       /* index 412, gain = 3.876401 dB */
	{0x0176, 0x0000, 0x00C6},       /* index 413, gain = 3.789104 dB */
	{0x0172, 0x0000, 0x00C4},       /* index 414, gain = 3.700922 dB */
	{0x016e, 0x0000, 0x00C2},       /* index 415, gain = 3.611835 dB */
	{0x016a, 0x0000, 0x00C0},       /* index 416, gain = 3.521825 dB */
	{0x0166, 0x0000, 0x00BE},       /* index 417, gain = 3.430873 dB */
	{0x0162, 0x0000, 0x00BC},       /* index 418, gain = 3.338958 dB */
	{0x015e, 0x0000, 0x00BA},       /* index 419, gain = 3.246059 dB */
	{0x015b, 0x0000, 0x00B8},       /* index 420, gain = 3.152157 dB */
	{0x0157, 0x0000, 0x00B6},       /* index 421, gain = 3.057228 dB */
	{0x0153, 0x0000, 0x00B4},       /* index 422, gain = 2.961251 dB */
	{0x0150, 0x0000, 0x00B2},       /* index 423, gain = 2.864201 dB */
	{0x014c, 0x0000, 0x00B0},       /* index 424, gain = 2.766054 dB */
	{0x0148, 0x0000, 0x00AE},       /* index 425, gain = 2.666786 dB */
	{0x0145, 0x0000, 0x00AC},       /* index 426, gain = 2.566370 dB */
	{0x0141, 0x0000, 0x00AA},       /* index 427, gain = 2.464779 dB */
	{0x013e, 0x0000, 0x00A8},       /* index 428, gain = 2.361986 dB */
	{0x013a, 0x0000, 0x00A6},       /* index 429, gain = 2.257962 dB */
	{0x0137, 0x0000, 0x00A4},       /* index 430, gain = 2.152678 dB */
	{0x0134, 0x0000, 0x00A2},       /* index 431, gain = 2.046101 dB */
	{0x0130, 0x0000, 0x00A0},       /* index 432, gain = 1.938200 dB */
	{0x012d, 0x0000, 0x009E},       /* index 433, gain = 1.828942 dB */
	{0x012a, 0x0000, 0x009C},       /* index 434, gain = 1.718293 dB */
	{0x0127, 0x0000, 0x009A},       /* index 435, gain = 1.606215 dB */
	{0x0124, 0x0000, 0x0098},       /* index 436, gain = 1.492672 dB */
	{0x0120, 0x0000, 0x0096},       /* index 437, gain = 1.377626 dB */
	{0x011d, 0x0000, 0x0094},       /* index 438, gain = 1.261035 dB */
	{0x011a, 0x0000, 0x0092},       /* index 439, gain = 1.142858 dB */
	{0x0117, 0x0000, 0x0090},       /* index 440, gain = 1.023050 dB */
	{0x0114, 0x0000, 0x008E},       /* index 441, gain = 0.901567 dB */
	{0x0111, 0x0000, 0x008C},       /* index 442, gain = 0.778361 dB */
	{0x010e, 0x0000, 0x008A},       /* index 443, gain = 0.653382 dB */
	{0x010b, 0x0000, 0x0088},       /* index 444, gain = 0.526579 dB */
	{0x0108, 0x0000, 0x0086},       /* index 445, gain = 0.397897 dB */
	{0x0106, 0x0000, 0x0084},       /* index 446, gain = 0.267279 dB */
	{0x0103, 0x0000, 0x0082},       /* index 447, gain = 0.134668 dB */
	{0x0100, 0x0000, 0x0080},       /* index 448, gain = 0.000000 dB */
};
#endif
