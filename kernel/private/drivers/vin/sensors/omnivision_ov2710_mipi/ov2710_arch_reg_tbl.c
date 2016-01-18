/*
 * Filename : ov2710_arch_reg_tbl.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static const struct ov2710_reg_table ov2710_share_regs[] = {
	// Select clock source
	{ OV2710_PLL_CLOCK_SELECT,	0x03},//0x3103

	// OV2710 software reset
	{ OV2710_SYSTEM_CONTROL00,	0x82},//0x3008
	{ OV2710_SYSTEM_CONTROL00,	0x42},	//sleep mode

	// MIPI setting OFF
	{ OV2710_MIPI_CTRL00,	0x0C},	//MIPI Enable

	// IO control
	{ OV2710_PAD_OUTPUT_ENABLE00,	0x00},//0x3016
	{ OV2710_PAD_OUTPUT_ENABLE01,	0x7F},//0x3017
	{ OV2710_PAD_OUTPUT_ENABLE02,	0xFC},//0x3018
//	{ OV2710_PAD_OUTPUT_SELECT00,	0x00},//0x301C		// def = 0x00
//	{ OV2710_PAD_OUTPUT_SELECT01,	0x00},//0x301D		// def = 0x00
//	{ OV2710_PAD_OUTPUT_SELECT02,	0x00},//0x301E		// def = 0x00
//	{ OV2710_PAD_OUTPUT_DRIVE_CAPABILITY,	0x02},//0x302C		// Output drive capability selection (def = 0x02)

//	{ OV2710_DVP_CTRL00,	0x04},//use HSYNC instead of HREF
//	{ OV2710_DVP_CTRL02,	0x03},

	// ISP control
	{ OV2710_ISP_CONTROL0,	0x00},//0x5000		// [7] LENC on, [2] Black pixel correct en, [1] white pixel correct en
	{ OV2710_ISP_CONTROL1,	0x00},//0x5001		// [0] AWB en
	//{ 0x5002,	0x00},//0x5002		// [2] VAP en	(CAN'T DISABLE !!!!!!!!!!!!)
	//{ 0x503D, 	0x80},
	// Manual AEC/AGC
	{ OV2710_AEC_PK_MANUAL,	0x07},//0x3503		// [2] VTS manual en, [1] AGC manual en, [0] AEC manual en
	{ OV2710_AEC_PK_VTS_H,	0x00},//0x350C
	{ OV2710_AEC_PK_VTS_L,	0x00},//0x350D

	// Black level calibration
	{ OV2710_BLC_CONTROL_00,	0x05},//0x4000		// BLC control
	{ 0x4006,	0x00},//0x4006		// Black level target [9:8]
	{ 0x4007,	0x00},//0x4007		// Black level target [7:0]
	// Reserved registers
	{ 0x302D,	0x90},//0x302D
	{ 0x3600,	0x04},//0x3600
	{ 0x3603,	0xA7},//0x3603
	{ 0x3604,	0x60},//0x3604
	{ 0x3605,	0x05},//0x3605
	{ 0x3606,	0x12},//0x3606
	{ 0x3621,	0x04},//0x3621		// [7] Horizontal binning, [6] Horizontal skipping
	{ 0x3630,	0x6D},//0x3630
	{ 0x3631,	0x26},//0x3631
	{ 0x3702,	0x9E},//0x3702
	{ 0x3703,	0x74},//0x3703
	{ 0x3704,	0x10},//0x3704
	{ 0x3706,	0x61},//0x3706
	{ 0x370B,	0x40},//0x370B
	{ 0x370D,	0x07},//0x370D
	{ 0x3710,	0x9E},//0x3710
	{ 0x3712,	0x0C},//0x3712
	{ 0x3713,	0x8B},//0x3713
	{ 0x3714,	0x74},//0x3714
	{ 0x381a,	0x1a},//0x381a
	{ 0x382e,	0x0f},//0x382e
	{ 0x4301,	0xFF},//0x4301
	{ 0x4303,	0x00},//0x4303
	{ 0x3A1A,	0x06},//0x3A1A		// AVG REG (to be removed?)
	{ 0x5688,	0x03},//0x5688		// AVG REG (to be removed?)
	{ 0x3017, 	0x00},
	{ 0x3018, 	0x00},

	//MIPI
	{0x4801, 	0x0f},
	{0x4800, 	0x24},		//non-continuous clock

};

#define OV2710_SHARE_REG_SZIE		ARRAY_SIZE(ov2710_share_regs)

/*
 *  1.rember to update OV2710_VIDEO_PLL_REG_TABLE_SIZE if you add/remove the regs
 * 	2.see rct.h for pixclk/extclk value
 */
static const struct ov2710_pll_reg_table ov2710_pll_tbl[] = {
	[0] = {
		.pixclk = 80000000,
		.extclk = PLL_CLK_24MHZ,
		.regs = {
			{OV2710_PLL_CTRL00, 0xcb},
			{OV2710_PLL_CTRL01, 0x00},
			{OV2710_PLL_CTRL02, 0x0a},
			{OV2710_PLL_PREDIVEDER, 0x01},
		}
	},
	[1] = {
		.pixclk = 80919080,
		.extclk = PLL_CLK_27D1001MHZ,
		.regs = {
			{OV2710_PLL_CTRL00, 0x88},
			{OV2710_PLL_CTRL01, 0x00},
			{OV2710_PLL_CTRL02, 0x18},
			{OV2710_PLL_PREDIVEDER, 0x00},
		}
	},
	[2] = {
		.pixclk = 40500000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
			{OV2710_PLL_CTRL00, 0x88},			// 1/2
			{OV2710_PLL_CTRL01, 0x00},			// 1
			{OV2710_PLL_CTRL02, 0x12},			// 18
			{OV2710_PLL_PREDIVEDER, 0x01},		// 2/3
		}
	},
	[3] = {
		.pixclk = PLL_CLK_54MHZ,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
			{OV2710_PLL_CTRL00, 0x88},			// 1/2
			{OV2710_PLL_CTRL01, 0x00},			// 1
			{OV2710_PLL_CTRL02, 0x18},			// 24
			{OV2710_PLL_PREDIVEDER, 0x01},		// 2/3
		}
	},

	/* << add pll config here if necessary >> */
};

#if 0

/* ========================================================================== */
static const struct ov2710_video_fps_reg_table ov2710_video_fps_720P =
{
 	.reg		= {
		OV2710_TIMING_CONTROL_DVP_HSIZE_HIGH,//0x3808 DVP output horizontal width [11:8]
		OV2710_TIMING_CONTROL_DVP_HSIZE_LOW,//0x3809 DVP output horizontal width [7:0]
		OV2710_TIMING_CONTROL_DVP_VSIZE_HIGH,//0x380A DVP output vertical height [11:8]
		OV2710_TIMING_CONTROL_DVP_VSIZE_LOW,//0x380B DVP output vertical height [7:0]

		OV2710_TIMING_CONTROL_HTS_HIGHBYTE,
		OV2710_TIMING_CONTROL_HTS_LOWBYTE,
		OV2710_TIMING_CONTROL_VTS_HIGHBYTE,
		OV2710_TIMING_CONTROL_VTS_LOWBYTE,//0x380F Total vertical size [7:0]
	},
	.table		= {
		{ // 30fps
		.pll_reg_table	= &ov2710_pll_tbl[2],
		.fps		= AMBA_VIDEO_FPS_30,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x05,//0x3808 DVP output horizontal width [11:8]
			0x00,//0x3809 DVP output horizontal width [7:0]
			0x02,//0x380A DVP output vertical height [11:8]
			0xD0,//0x380B DVP output vertical height [7:0]

			0x07,
			0x00,
			0x02,
			0xf1,
		}
		},
		{ // 29.97fps
		.pll_reg_table	= &ov2710_pll_tbl[2],
		.fps		= AMBA_VIDEO_FPS_29_97,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x05,//0x3808 DVP output horizontal width [11:8]
			0x00,//0x3809 DVP output horizontal width [7:0]
			0x02,//0x380A DVP output vertical height [11:8]
			0xD0,//0x380B DVP output vertical height [7:0]

			0x07,
			0x00,
			0x02,
			0xf2,
		}
		},
		{
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_60,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x05,//0x3808 DVP output horizontal width [11:8]
			0x00,//0x3809 DVP output horizontal width [7:0]
			0x02,//0x380A DVP output vertical height [11:8]
			0xD0,//0x380B DVP output vertical height [7:0]

			0x07,
			0x00,
			0x02,
			0xf1,
		}
		},
		{
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_59_94,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x05,//0x3808 DVP output horizontal width [11:8]
			0x00,//0x3809 DVP output horizontal width [7:0]
			0x02,//0x380A DVP output vertical height [11:8]
			0xD0,//0x380B DVP output vertical height [7:0]

			0x07,
			0x00,
			0x02,
			0xf2,
		}
		},
		{ // 25fps
		.pll_reg_table	= &ov2710_pll_tbl[2],
		.fps		= AMBA_VIDEO_FPS_25,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x05,//0x3808 DVP output horizontal width [11:8]
			0x00,//0x3809 DVP output horizontal width [7:0]
			0x02,//0x380A DVP output vertical height [11:8]
			0xD0,//0x380B DVP output vertical height [7:0]

			0x07,
			0x00,
			0x03,
			0x88,
		}
		},
		{//End
		.pll_reg_table	= NULL,
		},
		/* << add different fps table if necessary >> */
	},
};

static const struct ov2710_video_fps_reg_table ov2710_video_fps_1080P =
{
 	.reg		= {
		OV2710_TIMING_CONTROL_DVP_HSIZE_HIGH,//0x3808 DVP output horizontal width [11:8]
		OV2710_TIMING_CONTROL_DVP_HSIZE_LOW,//0x3809 DVP output horizontal width [7:0]
		OV2710_TIMING_CONTROL_DVP_VSIZE_HIGH,//0x380A DVP output vertical height [11:8]
		OV2710_TIMING_CONTROL_DVP_VSIZE_LOW,//0x380B DVP output vertical height [7:0]

		OV2710_TIMING_CONTROL_HTS_HIGHBYTE,
		OV2710_TIMING_CONTROL_HTS_LOWBYTE,
		OV2710_TIMING_CONTROL_VTS_HIGHBYTE,
		OV2710_TIMING_CONTROL_VTS_LOWBYTE,//0x380F Total vertical size [7:0]
	},
	.table		= {
		{ // 30fps
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_30,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x07,//0x3808 DVP output horizontal width [11:8]
			0x80,//0x3809 DVP output horizontal width [7:0]
			0x04,//0x380A DVP output vertical height [11:8]
			0x40,//0x380B DVP output vertical height [7:0]

			0x09,
			0x74,
			0x04,
			0x5c,
		}
		},
		{ // 29.97fps
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_29_97,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x07,//0x3808 DVP output horizontal width [11:8]
			0x80,//0x3809 DVP output horizontal width [7:0]
			0x04,//0x380A DVP output vertical height [11:8]
			0x40,//0x380B DVP output vertical height [7:0]

			0x09,
			0x74,
			0x04,
			0x5d,
		}
		},
		{ // 25fps
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_25,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x07,//0x3808 DVP output horizontal width [11:8]
			0x80,//0x3809 DVP output horizontal width [7:0]
			0x04,//0x380A DVP output vertical height [11:8]
			0x40,//0x380B DVP output vertical height [7:0]

			0x09,
			0x74,
			0x05,
			0x3b,
		}
		},
		{ // 15fps
		.pll_reg_table	= &ov2710_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_15,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
			0x07,//0x3808 DVP output horizontal width [11:8]
			0x80,//0x3809 DVP output horizontal width [7:0]
			0x04,//0x380A DVP output vertical height [11:8]
			0x40,//0x380B DVP output vertical height [7:0]

			0x09,
			0x74,
			0x08,
			0xb8,
		}
		},
		{//End
		.pll_reg_table	= NULL,
		},
		/* << add different fps table if necessary >> */
	},
};
	/* << add other fps tables for differnent video format here if necessary >> */
#endif
