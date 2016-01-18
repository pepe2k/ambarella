/*
 * Filename : mt9p006_arch_reg_tbl.c
 *
 * History:
 *    2011/05/23 - [Haowei Lo] Create
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

static const struct mt9p006_reg_table mt9p006_share_regs[] = {
	{0x0A, 0x8000}, /* MT9P006_PCLK_CTRL            */
	{0x29, 0x0481}, /* MT9P006_REG_29               */
	{0x2A, 0xFF74}, /* MT9P006_REG_2A               */

	{0x30, 0x0000}, /* MT9P006_REG_30               */

	{0x3E, 0x0087}, /* MT9P006_REG_3E               */
	{0x3F, 0x0007}, /* MT9P006_REG_3F               */

	{0x41, 0x0003}, /* MT9P006_REG_41               */
	{0x42, 0x0003}, /* MT9P006_REG_42               */
	{0x43, 0x0003}, /* MT9P006_REG_43               */

	{0x4B, 0x0028}, /* MT9P006_ROW_BLACK_DEF_OFFSET */
	{0x4F, 0x0014}, /* MT9P006_REG_4F               */
	{0x57, 0x0007}, /* MT9P006_REG_57               */
	{0x5F, 0x1C16}  /* MT9P006_BLC_TARGET_THR       */
};
#define MT9P006_SHARE_REG_SZIE		ARRAY_SIZE(mt9p006_share_regs)

/*
 *  1.rember to update MT9P006_VIDEO_PLL_REG_TABLE_SIZE if you add/remove the regs
 *	2.see rct.h for pixclk/extclk value
 */
static const struct mt9p006_pll_reg_table mt9p006_pll_tbl[] = {
	[0] = {	
		/* Non Low Power mode */
		.pixclk = 96000000, /* clock output from sensor/decoder */
		.extclk = PLL_CLK_24MHZ, /* clock from Amba soc to sensor/decoder, if you use external oscillator, ignore it */
		.regs = {
			{0x10, 0x51},   // MT9P401_PLL_CTRL
			// PLL_n_Divider=1 -> N=PLL_n_Divider+1=2
			// PLL_m_factor=16 ->M=16
			{0x11, 0x1001}, // MT9P401_PLL_CONFIG1  // Aptina suggested setting
			// PLL_p1_Divider=1 -> P1=PLL_p1_Divider+1=2
			{0x12, 0x0001}, // MT9P401_PLL_CONFIG2  // Aptina suggested setting
			// Use PLL
			{0x10, 0x53}, // MT9P401_PLL_CTRL
		}
	},
		/* << add pll config here if necessary >> */
};

#if 0

/* ========================================================================== */
static const struct mt9p006_video_fps_reg_table mt9p006_video_fps_1920x1080 = {
	.reg		= {

	},
	.table		= {
		{ // 30fps
		.pll_reg_table	= &mt9p006_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_29_97,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {

		}
		},	
		{//End
		.pll_reg_table	= NULL,
		}
		/* << add different fps table if necessary >> */
	},
};

static const struct mt9p006_video_fps_reg_table mt9p006_video_fps_1280x720 = {
	.reg		= {

	},
	.table		= {
		{ // 30fps
		.pll_reg_table	= &mt9p006_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_29_97,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {

		}
		},
		{//End
		.pll_reg_table	= NULL,
		}
		/* << add different fps table if necessary >> */
	},
};

#endif



