/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224_arch_reg_tbl.c
 *
 * History:
 *    2014/07/08 - [Long Zhao] Create
 *
 * Copyright (C) 2012-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx224_reg_table imx224_share_regs[] = {
	/* chip ID = 02h, do not change */
	{0x300F, 0x00},
	{0x3012, 0x2C},
	{0x3013, 0x01},
	{0x301D, 0xC2},
	{0x3070, 0x02},
	{0x3071, 0x01},
	{0x309E, 0x22},
	{0x30A5, 0xFB},
	{0x30A6, 0x02},
	{0x30B3, 0xFF},
	{0x30B4, 0x01},
	{0x30B5, 0x42},
	{0x30C2, 0x01},

	/* chip ID = 03h, do not change */
	{0x310F, 0x0F},
	{0x3110, 0x0E},
	{0x3111, 0xE7},
	{0x3112, 0x9C},
	{0x3113, 0x83},
	{0x3114, 0x10},
	{0x3115, 0x42},
	{0x3128, 0x1E},
	{0x31E9, 0x53},
	{0x31EA, 0x0A},
	{0x31ED, 0x38},

	/* chip ID = 04h, do not change */
	{0x320C, 0xCF},
	{0x324C, 0x40},
	{0x324D, 0x03},
	{0x3261, 0xE0},
	{0x3262, 0x02},
	{0x326E, 0x2F},
	{0x326F, 0x30},
	{0x3270, 0x03},
	{0x3298, 0x00},
	{0x329A, 0x12},
	{0x329B, 0xF1},
	{0x329C, 0x0C},

	/* chip ID = 05h, do not change */
	{0x335A, 0x33},
};
#define IMX224_SHARE_REG_SIZE		ARRAY_SIZE(imx224_share_regs)

static const struct imx224_pll_reg_table imx224_pll_tbl[] = {
	[0] = {
		.pixclk = PLL_CLK_148_5MHZ,
		.extclk = PLL_CLK_37_125MHZ,
		.regs = {
			{IMX224_INCKSEL1, 0x20},
			{IMX224_INCKSEL2, 0x00},
			{IMX224_INCKSEL3, 0x20},
			{IMX224_INCKSEL4, 0x00},
		}
	},
};
