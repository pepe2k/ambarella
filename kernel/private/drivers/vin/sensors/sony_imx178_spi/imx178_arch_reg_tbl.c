/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_arch_reg_tbl.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx178_reg_table imx178_share_regs[] = {
	/* do not change */
	{0x0211, 0x00},
	{0x021B, 0x00},
	{0x0237, 0x08},
	{0x0238, 0x00},
	{0x0239, 0x00},
	{0x02AD, 0x49},
	{0x02AF, 0x54},
	{0x02B0, 0x33},
	{0x02B3, 0x0A},
	{0x02C4, 0x30},
	{0x0303, 0x03},
	{0x0304, 0x08},
	{0x0307, 0x10},
	{0x030F, 0x01},
	{0x04E5, 0x06},
	{0x04E6, 0x00},
	{0x04E7, 0x1F},
	{0x04E8, 0x00},
	{0x04E9, 0x00},
	{0x04EA, 0x00},
	{0x04EB, 0x00},
	{0x04EC, 0x00},
	{0x04EE, 0x00},
	{0x04F2, 0x02},
	{0x04F4, 0x00},
	{0x04F5, 0x00},
	{0x04F6, 0x00},
	{0x04F7, 0x00},
	{0x04F8, 0x00},
	{0x04FC, 0x02},
	{0x0510, 0x11},
	{0x0538, 0x81},
	{0x053D, 0x00},
	{0x0562, 0x00},
	{0x056B, 0x02},
	{0x056E, 0x11},
	{0x05B4, 0xFE},
	{0x05B5, 0x06},
	{0x05B9, 0x00},
};
#define IMX178_SHARE_REG_SIZE		ARRAY_SIZE(imx178_share_regs)

static const struct imx178_pll_reg_table imx178_pll_tbl[] = {
	[0] = {
		.pixclk = 432000000,
		.extclk = PLL_CLK_54MHZ,
		.regs = {
			{0x030C, 0x01},
			{0x05BE, 0x21},
			{0x05BF, 0x21},
			{0x05C0, 0x2C},
			{0x05C1, 0x2C},
			{0x05C2, 0x21},
			{0x05C3, 0x2C},
			{0x05C4, 0x10}, //{0x05C4, 0x20}   {0x05C4, 0x10}
			{0x05C5, 0x01}, //{0x05C5, 0x00}   {0x05C5, 0x01}
			{0x031C, 0x1E},
			{0x031D, 0x15},
			{0x031E, 0x72},
			{0x031F, 0x00},
			{0x0320, 0x5C},
			{0x0321, 0x00},
			{0x0322, 0x72},
			{0x0323, 0x00},
			{0x0324, 0xC7},
			{0x0325, 0x01},
			{0x032D, 0x00},
			{0x032E, 0x01},
			{0x032F, 0x15},
			{0x0331, 0x10},
			{0x0332, 0x00},
			{0x0333, 0x72},
			{0x0334, 0x00},
			{0x0337, 0x38},
			{0x0338, 0x00},
			{0x0339, 0x00},
			{0x033A, 0x00},
			{0x033D, 0x00},
			{0x0340, 0x00},
			{0x0420, 0x89},
			{0x0421, 0x00},
			{0x0422, 0x54},
			{0x0423, 0x00},
			{0x0426, 0x8D},
			{0x0427, 0x00},
			{0x04A9, 0x14},
			{0x04AA, 0x00},
			{0x04B3, 0x0A},
			{0x04B4, 0x00},
			{0x05D6, 0x10},
			{0x05D7, 0x0F},
			{0x05D8, 0x0E},
			{0x05D9, 0x0C},
			{0x05DA, 0x06},
			{0xFFFF, 0xFF},//end
		}
	},
	[1] = {
		.pixclk = 594000000,
		.extclk = PLL_CLK_74_25MHZ,
		.regs = {
			{0x030C, 0x00},
			{0x05BE, 0x0C},
			{0x05BF, 0x0C},
			{0x05C0, 0x10},
			{0x05C1, 0x10},
			{0x05C2, 0x0C},
			{0x05C3, 0x10},
			{0x05C4, 0x10},
			{0x05C5, 0x00},
			{0x031C, 0x34},
			{0x031D, 0x28},
			{0x031E, 0xAB},
			{0x031F, 0x00},
			{0x0320, 0x95},
			{0x0321, 0x00},
			{0x0322, 0xB4},
			{0x0323, 0x00},
			{0x0324, 0x8C},
			{0x0325, 0x02},
			{0x032D, 0x03},
			{0x032E, 0x0C},
			{0x032F, 0x28},
			{0x0331, 0x2D},
			{0x0332, 0x00},
			{0x0333, 0xB4},
			{0x0334, 0x00},
			{0x0337, 0x50},
			{0x0338, 0x08},
			{0x0339, 0x00},
			{0x033A, 0x07},
			{0x033D, 0x05},
			{0x0340, 0x06},
			{0x0420, 0x8B},
			{0x0421, 0x00},
			{0x0422, 0x74},
			{0x0423, 0x00},
			{0x0426, 0xC2},
			{0x0427, 0x00},
			{0x04A9, 0x1B},
			{0x04AA, 0x00},
			{0x04B3, 0x0E},
			{0x04B4, 0x00},
			{0x05D6, 0x16},
			{0x05D7, 0x15},
			{0x05D8, 0x14},
			{0x05D9, 0x10},
			{0x05DA, 0x08},
			{0xFFFF, 0xFF},//end
		}
	},
};
