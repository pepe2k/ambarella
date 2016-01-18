/*
 * Filename : ar0835hs_reg_tbl.c
 *
 * History:
 *    2012/12/26 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

	/*   < rember to update AR0835HS_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
	/*   < rember to update AR0835HS_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct ar0835hs_video_format_reg_table ar0835hs_video_format_tbl = {
	.reg = {
		0x3064,	//SMIA_TEST
		0x3002,	//Y_ADDR_START
		0x3006,	//Y_ADDR_END
		0x3004,	//X_ADDR_START
		0x3008,	//X_ADDR_END
		0x3040,	//READ_MODE
		0x300c,	//LINE_LENGTH_PCK
		0x300a,	//FRAME_LENGTH_LINES
		0x3012,	//integration time
		0x0400,	//Enable horizontal and vertical scaling
		0x0402,	//Enable True bayer sampling
		0x0404,	//scaled multiplier to closest image size
		0x306e,	//True bayer scaling
		0x0408,	//residual of slice for odd and even row
		0x040a,	//Crop start_o
		0x034c,	//X_OUTPUT_SIZE
		0x034e,	//Y_OUTPUT_SIZE
		0x3f3a,	//Quient Time off
	},
	.table[0] = {		//3264x2448@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			8,      //Y_ADDR_START
			2455,   //Y_ADDR_END
			8,      //X_ADDR_START
			3271,   //X_ADDR_END
			0x4041, //READ_MODE
			4300,   //LINE_LENGTH_PCK
			2669,   //FRAME_LENGTH_LINES
			2669,   //integration time
			0x0,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			16,     //scaled multiplier to closest image size
			0x9080, //True bayer scaling
			0x1010, //residual of slice for odd and even row
			0x210,  //Crop start_o
			3264,   //X_OUTPUT_SIZE
			2448,   //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 3264,
		.height = 2448,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
	},
	.table[1] = {		//2304x1836@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			314,    //Y_ADDR_START
			2149,   //Y_ADDR_END
			4,      //X_ADDR_START
			3267,   //X_ADDR_END
			0x4041, //READ_MODE
			7320,   //LINE_LENGTH_PCK
			2042,   //FRAME_LENGTH_LINES
			2042,   //integration time
			0x1,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			22,     //scaled multiplier to closest image size
			0x9090, //True bayer scaling
			0x0b0e, //residual of slice for odd and even row
			0x016b, //Crop start_o
			2304,   //X_OUTPUT_SIZE
			1836,   //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 2304,
		.height = 1836,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	.table[2] = {		//3264x1836@30fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			314,    //Y_ADDR_START
			2149,   //Y_ADDR_END
			8,      //X_ADDR_START
			3271,   //X_ADDR_END
			0x4041, //READ_MODE
			7920,   //LINE_LENGTH_PCK
			1887,   //FRAME_LENGTH_LINES
			1887,   //integration time
			0x0,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			16,     //scaled multiplier to closest image size
			0x9080, //True bayer scaling
			0x1010, //residual of slice for odd and even row
			0x210,  //Crop start_o
			3264,   //X_OUTPUT_SIZE
			1836,   //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 3264,
		.height = 1836,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	.table[3] = {		//2208x1836@60fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			314,    //Y_ADDR_START
			2149,   //Y_ADDR_END
			4,      //X_ADDR_START
			3267,   //X_ADDR_END
			0x4041, //READ_MODE
			3960,   //LINE_LENGTH_PCK
			1887,   //FRAME_LENGTH_LINES
			1887,   //integration time
			0x1,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			23,     //scaled multiplier to closest image size
			0x9090, //True bayer scaling
			0x1501, //residual of slice for odd and even row
			0x014b, //Crop start_o
			2208,   //X_OUTPUT_SIZE
			1836,   //Y_OUTPUT_SIZE
			0x8803, //Quient Time off
		},
		.width = 2208,
		.height = 1836,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	.table[4] = {		//1280x918@120fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			314,    //Y_ADDR_START
			2147,   //Y_ADDR_END
			16,     //X_ADDR_START
			3277,   //X_ADDR_END
			0x68c3, //READ_MODE
			3800,   //LINE_LENGTH_PCK
			983,    //FRAME_LENGTH_LINES
			983,    //integration time
			0x1,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			20,     //scaled multiplier to closest image size
			0x9090, //True bayer scaling
			0x1402, //residual of slice for odd and even row
			0x018d, //Crop start_o
			1280,   //X_OUTPUT_SIZE
			918,    //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 1280,
		.height = 918,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	.table[5] = {		//816x612@120fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			8,      //Y_ADDR_START
			2449,   //Y_ADDR_END
			16,     //X_ADDR_START
			3273,   //X_ADDR_END
			0x61c7, //READ_MODE
			3920,   //LINE_LENGTH_PCK
			952,    //FRAME_LENGTH_LINES
			952,    //integration time
			0x0,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			16,     //scaled multiplier to closest image size
			0x9080, //True bayer scaling
			0x1010, //residual of slice for odd and even row
			0x0210, //Crop start_o
			816,    //X_OUTPUT_SIZE
			612,    //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 816,
		.height = 612,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_120,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
	.table[6] = {		//320x306@240fps
		.ext_reg_fill = NULL,
		.data = {
			0x5800, //SMIA_TEST
			8,      //Y_ADDR_START
			2449,   //Y_ADDR_END
			16,     //X_ADDR_START
			3273,   //X_ADDR_END
			0x61cf, //READ_MODE
			3920,   //LINE_LENGTH_PCK
			476,    //FRAME_LENGTH_LINES
			476,    //integration time
			0x1,    //Enable horizontal and vertical scaling
			0x0,    //Enable True bayer sampling
			40,     //scaled multiplier to closest image size
			0x9080, //True bayer scaling
			0x1824, //residual of slice for odd and even row
			0x00c6, //Crop start_o
			320,    //X_OUTPUT_SIZE
			306,    //Y_OUTPUT_SIZE
			0xff03, //Quient Time off
		},
		.width = 320,
		.height = 306,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS(240),
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 1,
	},
};

/** AGC Gain Table */

/** AR0835HS global gain table row size */
#define AR0835HS_GAIN_ROWS		449
#define AR0835HS_GAIN_COLS 		2
#define AR0835HS_GAIN_DOUBLE	64
#define AR0835HS_GAIN_0DB		(AR0835HS_GAIN_ROWS - 1)

#define AR0835_GAIN_COL_AGC		0
#define AR0835_GAIN_COL_REG		1

/* This is 64-step gain table, AR0835_GAIN_ROWS = 449, AR0835_GAIN_COLS = 2 */
const u16 AR0835HS_GLOBAL_GAIN_TABLE[AR0835HS_GAIN_ROWS][AR0835HS_GAIN_COLS] =
{
	/* {gain*256, gain_reg} */
	{0x8000, 0xffe7},    /* index 000, gain = 42.144199 dB */
	{0x7e9f, 0xfd27},    /* index 001, gain = 42.050128 dB */
	{0x7d41, 0xfa87},    /* index 002, gain = 41.956056 dB */
	{0x7be8, 0xf7c7},    /* index 003, gain = 41.861984 dB */
	{0x7a92, 0xf527},    /* index 004, gain = 41.767912 dB */
	{0x7940, 0xf287},    /* index 005, gain = 41.673840 dB */
	{0x77f2, 0xefe7},    /* index 006, gain = 41.579768 dB */
	{0x76a7, 0xed47},    /* index 007, gain = 41.485696 dB */
	{0x7560, 0xeac7},    /* index 008, gain = 41.391624 dB */
	{0x741c, 0xe827},    /* index 009, gain = 41.297553 dB */
	{0x72dc, 0xe5a7},    /* index 010, gain = 41.203481 dB */
	{0x719f, 0xe327},    /* index 011, gain = 41.109409 dB */
	{0x7066, 0xe0c7},    /* index 012, gain = 41.015337 dB */
	{0x6f30, 0xde67},    /* index 013, gain = 40.921265 dB */
	{0x6dfd, 0xdbe7},    /* index 014, gain = 40.827193 dB */
	{0x6cce, 0xd987},    /* index 015, gain = 40.733121 dB */
	{0x6ba2, 0xd747},    /* index 016, gain = 40.639049 dB */
	{0x6a79, 0xd4e7},    /* index 017, gain = 40.544978 dB */
	{0x6954, 0xd2a7},    /* index 018, gain = 40.450906 dB */
	{0x6831, 0xd067},    /* index 019, gain = 40.356834 dB */
	{0x6712, 0xce27},    /* index 020, gain = 40.262762 dB */
	{0x65f6, 0xcbe7},    /* index 021, gain = 40.168690 dB */
	{0x64dc, 0xc9a7},    /* index 022, gain = 40.074618 dB */
	{0x63c6, 0xc787},    /* index 023, gain = 39.980546 dB */
	{0x62b3, 0xc567},    /* index 024, gain = 39.886474 dB */
	{0x61a3, 0xc347},    /* index 025, gain = 39.792403 dB */
	{0x6096, 0xc127},    /* index 026, gain = 39.698331 dB */
	{0x5f8b, 0xbf07},    /* index 027, gain = 39.604259 dB */
	{0x5e84, 0xbd07},    /* index 028, gain = 39.510187 dB */
	{0x5d7f, 0xbae7},    /* index 029, gain = 39.416115 dB */
	{0x5c7d, 0xb8e7},    /* index 030, gain = 39.322043 dB */
	{0x5b7e, 0xb6e7},    /* index 031, gain = 39.227971 dB */
	{0x5a82, 0xb507},    /* index 032, gain = 39.133899 dB */
	{0x5988, 0xb307},    /* index 033, gain = 39.039828 dB */
	{0x5891, 0xb127},    /* index 034, gain = 38.945756 dB */
	{0x579d, 0xaf27},    /* index 035, gain = 38.851684 dB */
	{0x56ac, 0xad47},    /* index 036, gain = 38.757612 dB */
	{0x55bd, 0xab67},    /* index 037, gain = 38.663540 dB */
	{0x54d0, 0xa9a7},    /* index 038, gain = 38.569468 dB */
	{0x53e6, 0xa7c7},    /* index 039, gain = 38.475396 dB */
	{0x52ff, 0xa5e7},    /* index 040, gain = 38.381324 dB */
	{0x521a, 0xa427},    /* index 041, gain = 38.287253 dB */
	{0x5138, 0xa267},    /* index 042, gain = 38.193181 dB */
	{0x5058, 0xa0a7},    /* index 043, gain = 38.099109 dB */
	{0x4f7a, 0x9ee7},    /* index 044, gain = 38.005037 dB */
	{0x4e9f, 0x9d27},    /* index 045, gain = 37.910965 dB */
	{0x4dc6, 0x9b87},    /* index 046, gain = 37.816893 dB */
	{0x4cf0, 0x99e7},    /* index 047, gain = 37.722821 dB */
	{0x4c1b, 0x9827},    /* index 048, gain = 37.628749 dB */
	{0x4b4a, 0x9687},    /* index 049, gain = 37.534678 dB */
	{0x4a7a, 0x94e7},    /* index 050, gain = 37.440606 dB */
	{0x49ad, 0x9347},    /* index 051, gain = 37.346534 dB */
	{0x48e1, 0x91c7},    /* index 052, gain = 37.252462 dB */
	{0x4818, 0x9027},    /* index 053, gain = 37.158390 dB */
	{0x4752, 0x8ea7},    /* index 054, gain = 37.064318 dB */
	{0x468d, 0x8d07},    /* index 055, gain = 36.970246 dB */
	{0x45ca, 0x8b87},    /* index 056, gain = 36.876174 dB */
	{0x450a, 0x8a07},    /* index 057, gain = 36.782103 dB */
	{0x444c, 0x8887},    /* index 058, gain = 36.688031 dB */
	{0x438f, 0x8707},    /* index 059, gain = 36.593959 dB */
	{0x42d5, 0x85a7},    /* index 060, gain = 36.499887 dB */
	{0x421d, 0x8427},    /* index 061, gain = 36.405815 dB */
	{0x4166, 0x82c7},    /* index 062, gain = 36.311743 dB */
	{0x40b2, 0x8167},    /* index 063, gain = 36.217671 dB */
	{0x4000, 0x8007},    /* index 064, gain = 36.123599 dB */
	{0x3f4f, 0x7e87},    /* index 065, gain = 36.029528 dB */
	{0x3ea0, 0x7d47},    /* index 066, gain = 35.935456 dB */
	{0x3df4, 0x7be7},    /* index 067, gain = 35.841384 dB */
	{0x3d49, 0x7a87},    /* index 068, gain = 35.747312 dB */
	{0x3ca0, 0x7947},    /* index 069, gain = 35.653240 dB */
	{0x3bf9, 0x77e7},    /* index 070, gain = 35.559168 dB */
	{0x3b53, 0x76a7},    /* index 071, gain = 35.465096 dB */
	{0x3ab0, 0x7567},    /* index 072, gain = 35.371024 dB */
	{0x3a0e, 0x7407},    /* index 073, gain = 35.276953 dB */
	{0x396e, 0x72c7},    /* index 074, gain = 35.182881 dB */
	{0x38cf, 0x7187},    /* index 075, gain = 35.088809 dB */
	{0x3833, 0x7067},    /* index 076, gain = 34.994737 dB */
	{0x3798, 0x6f27},    /* index 077, gain = 34.900665 dB */
	{0x36fe, 0x6de7},    /* index 078, gain = 34.806593 dB */
	{0x3667, 0x6cc7},    /* index 079, gain = 34.712521 dB */
	{0x35d1, 0x6ba7},    /* index 080, gain = 34.618450 dB */
	{0x353c, 0x6a67},    /* index 081, gain = 34.524378 dB */
	{0x34aa, 0x6947},    /* index 082, gain = 34.430306 dB */
	{0x3418, 0x6827},    /* index 083, gain = 34.336234 dB */
	{0x3389, 0x6707},    /* index 084, gain = 34.242162 dB */
	{0x32fb, 0x65e7},    /* index 085, gain = 34.148090 dB */
	{0x326e, 0x64c7},    /* index 086, gain = 34.054018 dB */
	{0x31e3, 0x63c7},    /* index 087, gain = 33.959946 dB */
	{0x3159, 0x62a7},    /* index 088, gain = 33.865875 dB */
	{0x30d1, 0x61a7},    /* index 089, gain = 33.771803 dB */
	{0x304b, 0x6087},    /* index 090, gain = 33.677731 dB */
	{0x2fc5, 0x5f87},    /* index 091, gain = 33.583659 dB */
	{0x2f42, 0x5e87},    /* index 092, gain = 33.489587 dB */
	{0x2ebf, 0x5d67},    /* index 093, gain = 33.395515 dB */
	{0x2e3e, 0x5c67},    /* index 094, gain = 33.301443 dB */
	{0x2dbf, 0x5b67},    /* index 095, gain = 33.207371 dB */
	{0x2d41, 0x5a87},    /* index 096, gain = 33.113300 dB */
	{0x2cc4, 0x5987},    /* index 097, gain = 33.019228 dB */
	{0x2c48, 0x5887},    /* index 098, gain = 32.925156 dB */
	{0x2bce, 0x5787},    /* index 099, gain = 32.831084 dB */
	{0x2b56, 0x56a7},    /* index 100, gain = 32.737012 dB */
	{0x2ade, 0x55a7},    /* index 101, gain = 32.642940 dB */
	{0x2a68, 0x54c7},    /* index 102, gain = 32.548868 dB */
	{0x29f3, 0x53e7},    /* index 103, gain = 32.454796 dB */
	{0x297f, 0x52e7},    /* index 104, gain = 32.360725 dB */
	{0x290d, 0x5207},    /* index 105, gain = 32.266653 dB */
	{0x289c, 0x5127},    /* index 106, gain = 32.172581 dB */
	{0x282c, 0x5047},    /* index 107, gain = 32.078509 dB */
	{0x27bd, 0x4f67},    /* index 108, gain = 31.984437 dB */
	{0x274f, 0x4e87},    /* index 109, gain = 31.890365 dB */
	{0x26e3, 0x4dc7},    /* index 110, gain = 31.796293 dB */
	{0x2678, 0x4ce7},    /* index 111, gain = 31.702221 dB */
	{0x260d, 0x4c07},    /* index 112, gain = 31.608150 dB */
	{0x25a5, 0x4b47},    /* index 113, gain = 31.514078 dB */
	{0x253d, 0x4a67},    /* index 114, gain = 31.420006 dB */
	{0x24d6, 0x49a7},    /* index 115, gain = 31.325934 dB */
	{0x2470, 0x48e7},    /* index 116, gain = 31.231862 dB */
	{0x240c, 0x4807},    /* index 117, gain = 31.137790 dB */
	{0x23a9, 0x4747},    /* index 118, gain = 31.043718 dB */
	{0x2346, 0x4687},    /* index 119, gain = 30.949646 dB */
	{0x22e5, 0x45c7},    /* index 120, gain = 30.855575 dB */
	{0x2285, 0x4507},    /* index 121, gain = 30.761503 dB */
	{0x2226, 0x4447},    /* index 122, gain = 30.667431 dB */
	{0x21c7, 0x4387},    /* index 123, gain = 30.573359 dB */
	{0x216a, 0x42c7},    /* index 124, gain = 30.479287 dB */
	{0x210e, 0x4207},    /* index 125, gain = 30.385215 dB */
	{0x20b3, 0x4167},    /* index 126, gain = 30.291143 dB */
	{0x2059, 0x40a7},    /* index 127, gain = 30.197071 dB */
	{0x2000, 0x4007},    /* index 128, gain = 30.103000 dB */
	{0x1fa7, 0x3f47},    /* index 129, gain = 30.008928 dB */
	{0x1f50, 0x3ea7},    /* index 130, gain = 29.914856 dB */
	{0x1efa, 0x3de7},    /* index 131, gain = 29.820784 dB */
	{0x1ea4, 0x3d47},    /* index 132, gain = 29.726712 dB */
	{0x1e50, 0x3ca7},    /* index 133, gain = 29.632640 dB */
	{0x1dfc, 0x3be7},    /* index 134, gain = 29.538568 dB */
	{0x1da9, 0x3b47},    /* index 135, gain = 29.444496 dB */
	{0x1d58, 0x3aa7},    /* index 136, gain = 29.350425 dB */
	{0x1d07, 0x3a07},    /* index 137, gain = 29.256353 dB */
	{0x1cb7, 0x3967},    /* index 138, gain = 29.162281 dB */
	{0x1c67, 0x38c7},    /* index 139, gain = 29.068209 dB */
	{0x1c19, 0x3827},    /* index 140, gain = 28.974137 dB */
	{0x1bcc, 0x3787},    /* index 141, gain = 28.880065 dB */
	{0x1b7f, 0x36e7},    /* index 142, gain = 28.785993 dB */
	{0x1b33, 0x3667},    /* index 143, gain = 28.691921 dB */
	{0x1ae8, 0x35c7},    /* index 144, gain = 28.597850 dB */
	{0x1a9e, 0x3527},    /* index 145, gain = 28.503778 dB */
	{0x1a55, 0x34a7},    /* index 146, gain = 28.409706 dB */
	{0x1a0c, 0x3407},    /* index 147, gain = 28.315634 dB */
	{0x19c4, 0x3387},    /* index 148, gain = 28.221562 dB */
	{0x197d, 0x32e7},    /* index 149, gain = 28.127490 dB */
	{0x1937, 0x3267},    /* index 150, gain = 28.033418 dB */
	{0x18f1, 0x31e7},    /* index 151, gain = 27.939346 dB */
	{0x18ac, 0x3147},    /* index 152, gain = 27.845275 dB */
	{0x1868, 0x30c7},    /* index 153, gain = 27.751203 dB */
	{0x1825, 0x3047},    /* index 154, gain = 27.657131 dB */
	{0x17e2, 0x2fc7},    /* index 155, gain = 27.563059 dB */
	{0x17a1, 0x2f47},    /* index 156, gain = 27.468987 dB */
	{0x175f, 0x2ea7},    /* index 157, gain = 27.374915 dB */
	{0x171f, 0x2e27},    /* index 158, gain = 27.280843 dB */
	{0x16df, 0x2da7},    /* index 159, gain = 27.186771 dB */
	{0x16a0, 0x2d47},    /* index 160, gain = 27.092700 dB */
	{0x1662, 0x2cc7},    /* index 161, gain = 26.998628 dB */
	{0x1624, 0x2c47},    /* index 162, gain = 26.904556 dB */
	{0x15e7, 0x2bc7},    /* index 163, gain = 26.810484 dB */
	{0x15ab, 0x2b47},    /* index 164, gain = 26.716412 dB */
	{0x156f, 0x2ac7},    /* index 165, gain = 26.622340 dB */
	{0x1534, 0x2a67},    /* index 166, gain = 26.528268 dB */
	{0x14f9, 0x29e7},    /* index 167, gain = 26.434196 dB */
	{0x14bf, 0x2967},    /* index 168, gain = 26.340125 dB */
	{0x1486, 0x2907},    /* index 169, gain = 26.246053 dB */
	{0x144e, 0x2887},    /* index 170, gain = 26.151981 dB */
	{0x1416, 0x2827},    /* index 171, gain = 26.057909 dB */
	{0x13de, 0x27a7},    /* index 172, gain = 25.963837 dB */
	{0x13a7, 0x2747},    /* index 173, gain = 25.869765 dB */
	{0x1371, 0x26e7},    /* index 174, gain = 25.775693 dB */
	{0x133c, 0x2667},    /* index 175, gain = 25.681622 dB */
	{0x1306, 0x2607},    /* index 176, gain = 25.587550 dB */
	{0x12d2, 0x25a7},    /* index 177, gain = 25.493478 dB */
	{0x129e, 0x2527},    /* index 178, gain = 25.399406 dB */
	{0x126b, 0x24c7},    /* index 179, gain = 25.305334 dB */
	{0x1238, 0x2467},    /* index 180, gain = 25.211262 dB */
	{0x1206, 0x2407},    /* index 181, gain = 25.117190 dB */
	{0x11d4, 0x23a7},    /* index 182, gain = 25.023118 dB */
	{0x11a3, 0x2347},    /* index 183, gain = 24.929047 dB */
	{0x1172, 0x22e7},    /* index 184, gain = 24.834975 dB */
	{0x1142, 0x2287},    /* index 185, gain = 24.740903 dB */
	{0x1113, 0x2227},    /* index 186, gain = 24.646831 dB */
	{0x10e3, 0x21c7},    /* index 187, gain = 24.552759 dB */
	{0x10b5, 0x2167},    /* index 188, gain = 24.458687 dB */
	{0x1087, 0x2107},    /* index 189, gain = 24.364615 dB */
	{0x1059, 0x20a7},    /* index 190, gain = 24.270543 dB */
	{0x102c, 0x2047},    /* index 191, gain = 24.176472 dB */
	{0x1000, 0x2007},    /* index 192, gain = 24.082400 dB */
	{0x0fd3, 0x1fa7},    /* index 193, gain = 23.988328 dB */
	{0x0fa8, 0x1f47},    /* index 194, gain = 23.894256 dB */
	{0x0f7d, 0x1ee7},    /* index 195, gain = 23.800184 dB */
	{0x0f52, 0x1ea7},    /* index 196, gain = 23.706112 dB */
	{0x0f28, 0x1e47},    /* index 197, gain = 23.612040 dB */
	{0x0efe, 0x1de7},    /* index 198, gain = 23.517968 dB */
	{0x0ed4, 0x1da7},    /* index 199, gain = 23.423897 dB */
	{0x0eac, 0x1d47},    /* index 200, gain = 23.329825 dB */
	{0x0e83, 0x1d07},    /* index 201, gain = 23.235753 dB */
	{0x0e5b, 0x1ca7},    /* index 202, gain = 23.141681 dB */
	{0x0e33, 0x1c67},    /* index 203, gain = 23.047609 dB */
	{0x0e0c, 0x1c07},    /* index 204, gain = 22.953537 dB */
	{0x0de6, 0x1bc7},    /* index 205, gain = 22.859465 dB */
	{0x0dbf, 0x1b67},    /* index 206, gain = 22.765393 dB */
	{0x0d99, 0x1b27},    /* index 207, gain = 22.671322 dB */
	{0x0d74, 0x1ae7},    /* index 208, gain = 22.577250 dB */
	{0x0d4f, 0x1a87},    /* index 209, gain = 22.483178 dB */
	{0x0d2a, 0x1a47},    /* index 210, gain = 22.389106 dB */
	{0x0d06, 0x1a07},    /* index 211, gain = 22.295034 dB */
	{0x0ce2, 0x19c7},    /* index 212, gain = 22.200962 dB */
	{0x0cbe, 0x1967},    /* index 213, gain = 22.106890 dB */
	{0x0c9b, 0x1927},    /* index 214, gain = 22.012818 dB */
	{0x0c78, 0x18e7},    /* index 215, gain = 21.918747 dB */
	{0x0c56, 0x18a7},    /* index 216, gain = 21.824675 dB */
	{0x0c34, 0x1867},    /* index 217, gain = 21.730603 dB */
	{0x0c12, 0x1827},    /* index 218, gain = 21.636531 dB */
	{0x0bf1, 0x17e7},    /* index 219, gain = 21.542459 dB */
	{0x0bd0, 0x17a7},    /* index 220, gain = 21.448387 dB */
	{0x0baf, 0x1747},    /* index 221, gain = 21.354315 dB */
	{0x0b8f, 0x1707},    /* index 222, gain = 21.260243 dB */
	{0x0b6f, 0x16c7},    /* index 223, gain = 21.166172 dB */
	{0x0b50, 0x16a7},    /* index 224, gain = 21.072100 dB */
	{0x0b31, 0x1667},    /* index 225, gain = 20.978028 dB */
	{0x0b12, 0x1627},    /* index 226, gain = 20.883956 dB */
	{0x0af3, 0x15e7},    /* index 227, gain = 20.789884 dB */
	{0x0ad5, 0x15a7},    /* index 228, gain = 20.695812 dB */
	{0x0ab7, 0x1567},    /* index 229, gain = 20.601740 dB */
	{0x0a9a, 0x1527},    /* index 230, gain = 20.507668 dB */
	{0x0a7c, 0x14e7},    /* index 231, gain = 20.413597 dB */
	{0x0a5f, 0x14a7},    /* index 232, gain = 20.319525 dB */
	{0x0a43, 0x1487},    /* index 233, gain = 20.225453 dB */
	{0x0a27, 0x1447},    /* index 234, gain = 20.131381 dB */
	{0x0a0b, 0x1407},    /* index 235, gain = 20.037309 dB */
	{0x09ef, 0x13c7},    /* index 236, gain = 19.943237 dB */
	{0x09d3, 0x13a7},    /* index 237, gain = 19.849165 dB */
	{0x09b8, 0x1367},    /* index 238, gain = 19.755093 dB */
	{0x099e, 0x1327},    /* index 239, gain = 19.661022 dB */
	{0x0983, 0x1307},    /* index 240, gain = 19.566950 dB */
	{0x0969, 0x12c7},    /* index 241, gain = 19.472878 dB */
	{0x094f, 0x1287},    /* index 242, gain = 19.378806 dB */
	{0x0935, 0x1267},    /* index 243, gain = 19.284734 dB */
	{0x091c, 0x1227},    /* index 244, gain = 19.190662 dB */
	{0x0903, 0x1207},    /* index 245, gain = 19.096590 dB */
	{0x08ea, 0x11c7},    /* index 246, gain = 19.002518 dB */
	{0x08d1, 0x11a7},    /* index 247, gain = 18.908447 dB */
	{0x08b9, 0x1167},    /* index 248, gain = 18.814375 dB */
	{0x08a1, 0x1147},    /* index 249, gain = 18.720303 dB */
	{0x0889, 0x1107},    /* index 250, gain = 18.626231 dB */
	{0x0871, 0x10e7},    /* index 251, gain = 18.532159 dB */
	{0x085a, 0x10a7},    /* index 252, gain = 18.438087 dB */
	{0x0843, 0x1087},    /* index 253, gain = 18.344015 dB */
	{0x082c, 0x1047},    /* index 254, gain = 18.249943 dB */
	{0x0816, 0x1027},    /* index 255, gain = 18.155872 dB */
	{0x0800, 0x1007},    /* index 256, gain = 18.061800 dB */
	{0x07e9, 0x1506},    /* index 257, gain = 17.967728 dB */
	{0x07d4, 0x14e6},    /* index 258, gain = 17.873656 dB */
	{0x07be, 0x14a6},    /* index 259, gain = 17.779584 dB */
	{0x07a9, 0x1466},    /* index 260, gain = 17.685512 dB */
	{0x0794, 0x1426},    /* index 261, gain = 17.591440 dB */
	{0x077f, 0x13e6},    /* index 262, gain = 17.497368 dB */
	{0x076a, 0x13c6},    /* index 263, gain = 17.403297 dB */
	{0x0756, 0x1386},    /* index 264, gain = 17.309225 dB */
	{0x0741, 0x1346},    /* index 265, gain = 17.215153 dB */
	{0x072d, 0x1326},    /* index 266, gain = 17.121081 dB */
	{0x0719, 0x12e6},    /* index 267, gain = 17.027009 dB */
	{0x0706, 0x12a6},    /* index 268, gain = 16.932937 dB */
	{0x06f3, 0x1286},    /* index 269, gain = 16.838865 dB */
	{0x06df, 0x1246},    /* index 270, gain = 16.744794 dB */
	{0x06cc, 0x1226},    /* index 271, gain = 16.650722 dB */
	{0x06ba, 0x11e6},    /* index 272, gain = 16.556650 dB */
	{0x06a7, 0x11a6},    /* index 273, gain = 16.462578 dB */
	{0x0695, 0x1186},    /* index 274, gain = 16.368506 dB */
	{0x0683, 0x1146},    /* index 275, gain = 16.274434 dB */
	{0x0671, 0x1126},    /* index 276, gain = 16.180362 dB */
	{0x065f, 0x10e6},    /* index 277, gain = 16.086290 dB */
	{0x064d, 0x10c6},    /* index 278, gain = 15.992219 dB */
	{0x063c, 0x10a6},    /* index 279, gain = 15.898147 dB */
	{0x062b, 0x1066},    /* index 280, gain = 15.804075 dB */
	{0x061a, 0x1046},    /* index 281, gain = 15.710003 dB */
	{0x0609, 0x1006},    /* index 282, gain = 15.615931 dB */
	{0x05f8, 0x17e3},    /* index 283, gain = 15.521859 dB */
	{0x05e8, 0x17a3},    /* index 284, gain = 15.427787 dB */
	{0x05d7, 0x1743},    /* index 285, gain = 15.333715 dB */
	{0x05c7, 0x1703},    /* index 286, gain = 15.239644 dB */
	{0x05b7, 0x16c3},    /* index 287, gain = 15.145572 dB */
	{0x05a8, 0x16a3},    /* index 288, gain = 15.051500 dB */
	{0x0598, 0x1663},    /* index 289, gain = 14.957428 dB */
	{0x0589, 0x1623},    /* index 290, gain = 14.863356 dB */
	{0x0579, 0x15e3},    /* index 291, gain = 14.769284 dB */
	{0x056a, 0x15a3},    /* index 292, gain = 14.675212 dB */
	{0x055b, 0x1563},    /* index 293, gain = 14.581140 dB */
	{0x054d, 0x1523},    /* index 294, gain = 14.487069 dB */
	{0x053e, 0x14e3},    /* index 295, gain = 14.392997 dB */
	{0x052f, 0x14a3},    /* index 296, gain = 14.298925 dB */
	{0x0521, 0x1483},    /* index 297, gain = 14.204853 dB */
	{0x0513, 0x1443},    /* index 298, gain = 14.110781 dB */
	{0x0505, 0x1403},    /* index 299, gain = 14.016709 dB */
	{0x04f7, 0x13c3},    /* index 300, gain = 13.922637 dB */
	{0x04e9, 0x13a3},    /* index 301, gain = 13.828565 dB */
	{0x04dc, 0x1363},    /* index 302, gain = 13.734494 dB */
	{0x04cf, 0x1323},    /* index 303, gain = 13.640422 dB */
	{0x04c1, 0x1303},    /* index 304, gain = 13.546350 dB */
	{0x04b4, 0x12c3},    /* index 305, gain = 13.452278 dB */
	{0x04a7, 0x1283},    /* index 306, gain = 13.358206 dB */
	{0x049a, 0x1263},    /* index 307, gain = 13.264134 dB */
	{0x048e, 0x1223},    /* index 308, gain = 13.170062 dB */
	{0x0481, 0x1203},    /* index 309, gain = 13.075990 dB */
	{0x0475, 0x11c3},    /* index 310, gain = 12.981919 dB */
	{0x0468, 0x11a3},    /* index 311, gain = 12.887847 dB */
	{0x045c, 0x1163},    /* index 312, gain = 12.793775 dB */
	{0x0450, 0x1143},    /* index 313, gain = 12.699703 dB */
	{0x0444, 0x1103},    /* index 314, gain = 12.605631 dB */
	{0x0438, 0x10e3},    /* index 315, gain = 12.511559 dB */
	{0x042d, 0x10a3},    /* index 316, gain = 12.417487 dB */
	{0x0421, 0x1083},    /* index 317, gain = 12.323415 dB */
	{0x0416, 0x1043},    /* index 318, gain = 12.229344 dB */
	{0x040b, 0x1023},    /* index 319, gain = 12.135272 dB */
	{0x0400, 0x1003},    /* index 320, gain = 12.041200 dB */
	{0x03f4, 0x1502},    /* index 321, gain = 11.947128 dB */
	{0x03ea, 0x14e2},    /* index 322, gain = 11.853056 dB */
	{0x03df, 0x14a2},    /* index 323, gain = 11.758984 dB */
	{0x03d4, 0x1462},    /* index 324, gain = 11.664912 dB */
	{0x03ca, 0x1422},    /* index 325, gain = 11.570840 dB */
	{0x03bf, 0x13e2},    /* index 326, gain = 11.476769 dB */
	{0x03b5, 0x13c2},    /* index 327, gain = 11.382697 dB */
	{0x03ab, 0x1382},    /* index 328, gain = 11.288625 dB */
	{0x03a0, 0x1342},    /* index 329, gain = 11.194553 dB */
	{0x0396, 0x1322},    /* index 330, gain = 11.100481 dB */
	{0x038c, 0x12e2},    /* index 331, gain = 11.006409 dB */
	{0x0383, 0x12a2},    /* index 332, gain = 10.912337 dB */
	{0x0379, 0x1282},    /* index 333, gain = 10.818265 dB */
	{0x036f, 0x1242},    /* index 334, gain = 10.724194 dB */
	{0x0366, 0x1222},    /* index 335, gain = 10.630122 dB */
	{0x035d, 0x11e2},    /* index 336, gain = 10.536050 dB */
	{0x0353, 0x11a2},    /* index 337, gain = 10.441978 dB */
	{0x034a, 0x1182},    /* index 338, gain = 10.347906 dB */
	{0x0341, 0x1142},    /* index 339, gain = 10.253834 dB */
	{0x0338, 0x1122},    /* index 340, gain = 10.159762 dB */
	{0x032f, 0x10e2},    /* index 341, gain = 10.065690 dB */
	{0x0326, 0x10c2},    /* index 342, gain =  9.971619 dB */
	{0x031e, 0x10a2},    /* index 343, gain =  9.877547 dB */
	{0x0315, 0x1062},    /* index 344, gain =  9.783475 dB */
	{0x030d, 0x1042},    /* index 345, gain =  9.689403 dB */
	{0x0304, 0x1002},    /* index 346, gain =  9.595331 dB */
	{0x02fc, 0x17e1},    /* index 347, gain =  9.501259 dB */
	{0x02f4, 0x17a1},    /* index 348, gain =  9.407187 dB */
	{0x02eb, 0x1741},    /* index 349, gain =  9.313115 dB */
	{0x02e3, 0x1701},    /* index 350, gain =  9.219044 dB */
	{0x02db, 0x16c1},    /* index 351, gain =  9.124972 dB */
	{0x02d4, 0x16a1},    /* index 352, gain =  9.030900 dB */
	{0x02cc, 0x1661},    /* index 353, gain =  8.936828 dB */
	{0x02c4, 0x1621},    /* index 354, gain =  8.842756 dB */
	{0x02bc, 0x15e1},    /* index 355, gain =  8.748684 dB */
	{0x02b5, 0x15a1},    /* index 356, gain =  8.654612 dB */
	{0x02ad, 0x1561},    /* index 357, gain =  8.560541 dB */
	{0x02a6, 0x1521},    /* index 358, gain =  8.466469 dB */
	{0x029f, 0x14e1},    /* index 359, gain =  8.372397 dB */
	{0x0297, 0x14a1},    /* index 360, gain =  8.278325 dB */
	{0x0290, 0x1481},    /* index 361, gain =  8.184253 dB */
	{0x0289, 0x1441},    /* index 362, gain =  8.090181 dB */
	{0x0282, 0x1401},    /* index 363, gain =  7.996109 dB */
	{0x027b, 0x13c1},    /* index 364, gain =  7.902037 dB */
	{0x0274, 0x13a1},    /* index 365, gain =  7.807966 dB */
	{0x026e, 0x1361},    /* index 366, gain =  7.713894 dB */
	{0x0267, 0x1321},    /* index 367, gain =  7.619822 dB */
	{0x0260, 0x1301},    /* index 368, gain =  7.525750 dB */
	{0x025a, 0x12c1},    /* index 369, gain =  7.431678 dB */
	{0x0253, 0x1281},    /* index 370, gain =  7.337606 dB */
	{0x024d, 0x1261},    /* index 371, gain =  7.243534 dB */
	{0x0247, 0x1221},    /* index 372, gain =  7.149462 dB */
	{0x0240, 0x1201},    /* index 373, gain =  7.055391 dB */
	{0x023a, 0x11c1},    /* index 374, gain =  6.961319 dB */
	{0x0234, 0x11a1},    /* index 375, gain =  6.867247 dB */
	{0x022e, 0x1161},    /* index 376, gain =  6.773175 dB */
	{0x0228, 0x1141},    /* index 377, gain =  6.679103 dB */
	{0x0222, 0x1101},    /* index 378, gain =  6.585031 dB */
	{0x021c, 0x10e1},    /* index 379, gain =  6.490959 dB */
	{0x0216, 0x10a1},    /* index 380, gain =  6.396887 dB */
	{0x0210, 0x1081},    /* index 381, gain =  6.302816 dB */
	{0x020b, 0x1041},    /* index 382, gain =  6.208744 dB */
	{0x0205, 0x1021},    /* index 383, gain =  6.114672 dB */
	{0x0200, 0x1001},    /* index 384, gain =  6.020600 dB */
	{0x01fa, 0x1fa0},    /* index 385, gain =  5.926528 dB */
	{0x01f5, 0x1f40},    /* index 386, gain =  5.832456 dB */
	{0x01ef, 0x1ee0},    /* index 387, gain =  5.738384 dB */
	{0x01ea, 0x1ea0},    /* index 388, gain =  5.644312 dB */
	{0x01e5, 0x1e40},    /* index 389, gain =  5.550241 dB */
	{0x01df, 0x1de0},    /* index 390, gain =  5.456169 dB */
	{0x01da, 0x1da0},    /* index 391, gain =  5.362097 dB */
	{0x01d5, 0x1d40},    /* index 392, gain =  5.268025 dB */
	{0x01d0, 0x1d00},    /* index 393, gain =  5.173953 dB */
	{0x01cb, 0x1ca0},    /* index 394, gain =  5.079881 dB */
	{0x01c6, 0x1c60},    /* index 395, gain =  4.985809 dB */
	{0x01c1, 0x1c00},    /* index 396, gain =  4.891737 dB */
	{0x01bc, 0x1bc0},    /* index 397, gain =  4.797666 dB */
	{0x01b7, 0x1b60},    /* index 398, gain =  4.703594 dB */
	{0x01b3, 0x1b20},    /* index 399, gain =  4.609522 dB */
	{0x01ae, 0x1ae0},    /* index 400, gain =  4.515450 dB */
	{0x01a9, 0x1a80},    /* index 401, gain =  4.421378 dB */
	{0x01a5, 0x1a40},    /* index 402, gain =  4.327306 dB */
	{0x01a0, 0x1a00},    /* index 403, gain =  4.233234 dB */
	{0x019c, 0x19c0},    /* index 404, gain =  4.139162 dB */
	{0x0197, 0x1960},    /* index 405, gain =  4.045091 dB */
	{0x0193, 0x1920},    /* index 406, gain =  3.951019 dB */
	{0x018f, 0x18e0},    /* index 407, gain =  3.856947 dB */
	{0x018a, 0x18a0},    /* index 408, gain =  3.762875 dB */
	{0x0186, 0x1860},    /* index 409, gain =  3.668803 dB */
	{0x0182, 0x1820},    /* index 410, gain =  3.574731 dB */
	{0x017e, 0x17e0},    /* index 411, gain =  3.480659 dB */
	{0x017a, 0x17a0},    /* index 412, gain =  3.386587 dB */
	{0x0175, 0x1740},    /* index 413, gain =  3.292516 dB */
	{0x0171, 0x1700},    /* index 414, gain =  3.198444 dB */
	{0x016d, 0x16c0},    /* index 415, gain =  3.104372 dB */
	{0x016a, 0x16a0},    /* index 416, gain =  3.010300 dB */
	{0x0166, 0x1660},    /* index 417, gain =  2.916228 dB */
	{0x0162, 0x1620},    /* index 418, gain =  2.822156 dB */
	{0x015e, 0x15e0},    /* index 419, gain =  2.728084 dB */
	{0x015a, 0x15a0},    /* index 420, gain =  2.634012 dB */
	{0x0156, 0x1560},    /* index 421, gain =  2.539941 dB */
	{0x0153, 0x1520},    /* index 422, gain =  2.445869 dB */
	{0x014f, 0x14e0},    /* index 423, gain =  2.351797 dB */
	{0x014b, 0x14a0},    /* index 424, gain =  2.257725 dB */
	{0x0148, 0x1480},    /* index 425, gain =  2.163653 dB */
	{0x0144, 0x1440},    /* index 426, gain =  2.069581 dB */
	{0x0141, 0x1400},    /* index 427, gain =  1.975509 dB */
	{0x013d, 0x13c0},    /* index 428, gain =  1.881437 dB */
	{0x013a, 0x13a0},    /* index 429, gain =  1.787366 dB */
	{0x0137, 0x1360},    /* index 430, gain =  1.693294 dB */
	{0x0133, 0x1320},    /* index 431, gain =  1.599222 dB */
	{0x0130, 0x1300},    /* index 432, gain =  1.505150 dB */
	{0x012d, 0x12c0},    /* index 433, gain =  1.411078 dB */
	{0x0129, 0x1280},    /* index 434, gain =  1.317006 dB */
	{0x0126, 0x1260},    /* index 435, gain =  1.222934 dB */
	{0x0123, 0x1220},    /* index 436, gain =  1.128862 dB */
	{0x0120, 0x1200},    /* index 437, gain =  1.034791 dB */
	{0x011d, 0x11c0},    /* index 438, gain =  0.940719 dB */
	{0x011a, 0x11a0},    /* index 439, gain =  0.846647 dB */
	{0x0117, 0x1160},    /* index 440, gain =  0.752575 dB */
	{0x0114, 0x1140},    /* index 441, gain =  0.658503 dB */
	{0x0111, 0x1100},    /* index 442, gain =  0.564431 dB */
	{0x010e, 0x10e0},    /* index 443, gain =  0.470359 dB */
	{0x010b, 0x10a0},    /* index 444, gain =  0.376287 dB */
	{0x0108, 0x1080},    /* index 445, gain =  0.282216 dB */
	{0x0105, 0x1040},    /* index 446, gain =  0.188144 dB */
	{0x0102, 0x1020},    /* index 447, gain =  0.094072 dB */
	{0x0100, 0x1000},    /* index 448, gain =  0.000000 dB */
};