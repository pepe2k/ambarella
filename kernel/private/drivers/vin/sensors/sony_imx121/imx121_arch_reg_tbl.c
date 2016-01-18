/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx121/imx121_arch_reg_tbl.c
 *
 * History:
 *    2011/11/25 - [Long Zhao] Create
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
static const struct imx121_reg_table imx121_share_regs[] = {
	{IMX121_REG_0A, 0x11},
	{IMX121_REG_0E, 0x00},
	{IMX121_REG_FREQ, 0x05},// INCKx4
	{IMX121_REG_4C, 0x74},
	{IMX121_REG_4D, 0x74},
	{IMX121_REG_DCKRST, 0x01},
	{IMX121_REG_330, 0x01},
};
#define IMX121_SHARE_REG_SIZE		ARRAY_SIZE(imx121_share_regs)

static const struct imx121_pll_reg_table imx121_pll_tbl[] = {
	[0] = {// For 1080p@59.94fps(10lanes)
		.pixclk = 575999438,//extclk * 8
		.extclk = 71999929,
		.regs = {
		}
	},
	[1] = {// For 4096x2160@29.97fps(10lanes)
		.pixclk = 287999719,//extclk * 8
		.extclk = 35999964,
		.regs = {
		}
	},
	[2] = { // For 4016x3016@29.97fps(10lanes)
		.pixclk = 411428170,//extclk * 8
		.extclk = 51428521,
		.regs = {
		}
	},
	[3] = {// For 1080p@60fps(10lanes)
		.pixclk = 576576023,//extclk * 8
		.extclk = 72072002,
		.regs = {
		}
	},
	[4] = {// For 4096x2160@30fps(10lanes)
		.pixclk = 288287994,//extclk * 8
		.extclk = 36035999,
		.regs = {
		}
	},
	[5] = { // For 4016x3016@30fps(10lanes)
		.pixclk = 411839992,//extclk * 8
		.extclk = 51479999,
		.regs = {
		}
	},
	[6] = { // For 4096x2160/1080p@30fps(8lanes)
		.pixclk = 617974770,//extclk * 8
		.extclk = 77219995,
		.regs = {
		}
	},
	[7] = { // For 4096x2160/1080p@29.97fps(8lanes)
		.pixclk = 617356795,//extclk * 8
		.extclk = 77142775,
		.regs = {
		}
	},
	[8] = { // For 4000x3000@20fps(8lanes)
		.pixclk = 576701580,//extclk * 8
		.extclk = 72071986,
		.regs = {
		}
	},
};