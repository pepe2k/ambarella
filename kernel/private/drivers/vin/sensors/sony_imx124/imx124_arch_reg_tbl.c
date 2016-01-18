/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_arch_reg_tbl.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx124_reg_table imx124_share_regs[] = {
	/* chip ID: 02h */
	{0x305A, 0x01},

	/* chip ID: 03h */
	{0x3130, 0x44},
	{0x3133, 0x12},
	{0x3134, 0x10},
	{0x3135, 0x12},
	{0x3136, 0x10},
	{0x3149, 0x55},
	{0x3179, 0xFE},
	{0x31EB, 0x40},

	/* chip ID: 04h */
	{0x3201, 0x3C},
	{0x3202, 0x01},
	{0x3203, 0x0E},
	{0x325F, 0x03},
	{0x3269, 0x03},
	{0x32B6, 0x03},
	{0x32BA, 0x01},
	{0x32C4, 0x01},
	{0x32CB, 0x01},

	/* chip ID: 05h */
	{0x332A, 0xFF},
	{0x332B, 0xFF},
	{0x332C, 0xFF},
	{0x332D, 0xFF},
	{0x332E, 0xFF},
	{0x332F, 0xFF},
};
#define IMX124_SHARE_REG_SIZE		ARRAY_SIZE(imx124_share_regs)

static const struct imx124_pll_reg_table imx124_pll_tbl[] = {
	[0] = {// for 3M
		.pixclk = 72000000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
			{IMX124_INCKSEL1, 0xB1},
			{IMX124_INCKSEL2, 0x00},
			{IMX124_INCKSEL3, 0x2C},
			{IMX124_INCKSEL4, 0x09},
			{IMX124_INCKSEL5, 0x61},
			{IMX124_INCKSEL6, 0x30},
		}
	},
	[1] = {// for 1080p
		.pixclk = 74250000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
			{IMX124_INCKSEL1, 0xA1},
			{IMX124_INCKSEL2, 0x00},
			{IMX124_INCKSEL3, 0x2C},
			{IMX124_INCKSEL4, 0x09},
			{IMX124_INCKSEL5, 0x61},
			{IMX124_INCKSEL6, 0x2C},
		}
	},
};
