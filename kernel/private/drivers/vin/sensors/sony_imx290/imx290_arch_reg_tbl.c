/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx290/imx290_arch_reg_tbl.c
 *
 * History:
 *    2015/03/24 - [Hao Zeng] Create
 *
 * Copyright (C) 2004-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx290_reg_table imx290_share_regs[] = {
	/* chip ID: 02h */
	{0x300F, 0x00},
	{0x3010, 0x21},
	{0x3012, 0x64},
	{0x3016, 0x09},
	{0x3070, 0x02},
	{0x3071, 0x11},
	{0x309B, 0x10},
	{0x309C, 0x22},
	{0x30A2, 0x02},
	{0x30A6, 0x20},
	{0x30A8, 0x20},
	{0x30AA, 0x20},
	{0x30AC, 0x20},
	{0x30B0, 0x43},

	/* chip ID: 03h */
	{0x3119, 0x9E},
	{0x311C, 0x1E},
	{0x311E, 0x08},
	{0x3128, 0x05},
	{0x313D, 0x83},
	{0x3150, 0x03},
	{0x317E, 0x00},

	/* chip ID: 04h */
	{0x32B8, 0x50},
	{0x32B9, 0x10},
	{0x32BA, 0x00},
	{0x32BB, 0x04},
	{0x32C8, 0x50},
	{0x32C9, 0x10},
	{0x32CA, 0x00},
	{0x32CB, 0x04},

	/* chip ID: 05h */
	{0x332C, 0xD3},
	{0x332D, 0x10},
	{0x332E, 0x0D},
	{0x3358, 0x06},
	{0x3359, 0xE1},
	{0x335A, 0x11},
	{0x3360, 0x1E},
	{0x3361, 0x61},
	{0x3362, 0x10},
	{0x33B0, 0x50},
	{0x33B2, 0x1A},
	{0x33B3, 0x04},
};
#define IMX290_SHARE_REG_SIZE		ARRAY_SIZE(imx290_share_regs)

static const struct imx290_pll_reg_table imx290_pll_tbl[] = {
	[0] = {	/* for 1080p */
		.pixclk = 148500000,
		.extclk = 37125000,
		.regs = {
			{IMX290_INCKSEL1, 0x18},
			{IMX290_INCKSEL2, 0x00},
			{IMX290_INCKSEL3, 0x20},
			{IMX290_INCKSEL4, 0x01},
			{IMX290_INCKSEL5, 0x1A},
			{IMX290_INCKSEL6, 0x1A},
		}
	},
	[1] = {	/* for 720p */
		.pixclk = 148500000,
		.extclk = 37125000,
		.regs = {
			{IMX290_INCKSEL1, 0x20},
			{IMX290_INCKSEL2, 0x00},
			{IMX290_INCKSEL3, 0x20},
			{IMX290_INCKSEL4, 0x01},
			{IMX290_INCKSEL5, 0x1A},
			{IMX290_INCKSEL6, 0x1A},
		}
	},
};
