/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx172/imx172_arch_reg_tbl.c
 *
 * History:
 *    2012/08/22 - [Cao Rongrong] Create
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
static const struct imx172_reg_table imx172_share_regs[] = {
	{IMX172_REG_00, 0x06},
	{IMX172_REG_01, 0x11},
	{IMX172_REG_02, 0x01},
	{IMX172_REG_MDSEL3, 0x00},
	{IMX172_REG_MDSEL4, 0x00},
	{IMX172_REG_MDVREV, 0x00},
	{IMX172_REG_MDSEL13, 0x74},
	{IMX172_REG_MDSEL14, 0x74},
	{IMX172_REG_MDSEL15, 0x74},
	{IMX172_REG_MDSEL5_LSB, 0x20},
	{IMX172_REG_MDSEL5_MSB, 0x01},
	{IMX172_REG_MDPLS01, 0x00},
	{IMX172_REG_MDPLS02, 0x00},
	{IMX172_REG_MDPLS03, 0x00},
	{IMX172_REG_MDPLS04, 0x00},
	{IMX172_REG_MDPLS05, 0x00},
	{IMX172_REG_MDPLS06, 0x00},
	{IMX172_REG_MDPLS07, 0x00},
	{IMX172_REG_MDPLS08, 0x00},
	{IMX172_REG_MDPLS09, 0x00},
	{IMX172_REG_MDPLS10, 0x00},
	{IMX172_REG_MDPLS11, 0x00},
	{IMX172_REG_MDPLS12, 0x00},
	{IMX172_REG_MDPLS13, 0x00},
	{IMX172_REG_MDPLS14, 0x00},
	{IMX172_REG_MDPLS15, 0x00},
	{IMX172_REG_MDPLS16, 0x00},
	{IMX172_REG_MDSEL7, 0x00},
	{IMX172_REG_MDSEL8, 0x00},
	{IMX172_REG_MDSEL9, 0x00},
	{IMX172_REG_MDSEL10, 0x00},
	{IMX172_REG_MDSEL11, 0x00},
	{IMX172_REG_MDPLS17, 0x00},
	{IMX172_REG_MDPLS18, 0x00},
	{IMX172_REG_MDPLS19, 0x00},
	{IMX172_REG_MDPLS20, 0x00},
	{IMX172_REG_MDPLS21, 0x00},
	{IMX172_REG_MDPLS22, 0x00},
	{IMX172_REG_MDPLS23, 0x00},
	{IMX172_REG_MDPLS24, 0x00},
	{IMX172_REG_MDPLS25, 0x00},
	{IMX172_REG_MDPLS26, 0x00},
	{IMX172_REG_MDPLS27, 0x00},
	{IMX172_REG_MDPLS28, 0x00},
	{IMX172_REG_MDPLS29, 0x00},
	{IMX172_REG_MDPLS30, 0x00},
	{IMX172_REG_MDPLS31, 0x00},
	{IMX172_REG_MDPLS32, 0x00},
	{IMX172_REG_MDPLS33, 0x00},
	{IMX172_REG_MDSEL12, 0x0E},
	{IMX172_REG_PLSTMG11_LSB, 0x31},
	{IMX172_REG_PLSTMG11_MSB, 0x01},
	{IMX172_REG_PLSTMG00, 0x01},
	{IMX172_REG_PLSTMG01, 0x0E},
	{IMX172_REG_PLSTMG13, 0x0E},
	{IMX172_REG_PLSTMG02, 0x0E},
	{IMX172_REG_PLSTMG14, 0x0E},
	{IMX172_REG_PLSTMG15, 0x10},
	{IMX172_REG_PLSTMG03, 0x00},
	{IMX172_REG_PLSTMG04, 0x10},
	{IMX172_REG_PLSTMG05, 0x0D},
	{IMX172_REG_PLSTMG06, 0x0D},
	{IMX172_REG_PLSTMG07_LSB, 0x00},
	{IMX172_REG_PLSTMG07_MSB, 0x07},
	{IMX172_REG_PLSTMG12, 0x10},
	{IMX172_REG_PLSTMG08, 0x05},
	{IMX172_REG_PLSTMG09_LSB, 0x19},
	{IMX172_REG_PLSTMG09_MSB, 0x19},
};

#define IMX172_SHARE_REG_SIZE		ARRAY_SIZE(imx172_share_regs)

static const struct imx172_pll_reg_table imx172_pll_tbl[] = {
	[0] = { // For 4096x2160/1080p@30fps(8lanes), 2048x1080@59.94 2x2 binning(4lanes)
		.pixclk = 575999424,
		.extclk = 71999928,
		.factor = 8,
		.regs = {
		}
	},
	[1] = { // For 4000x3000 10ch 12bit 30fps
		.pixclk = 576000000,
		.extclk = 72000000,
		.factor = 8,
		.regs = {
		}
	},
	[2] = { // For 4096x2160@60fps(10lanes)
		.pixclk = 575999424,
		.extclk = 72000000,
		.factor = 8,
		.regs = {
		}
	},
};

