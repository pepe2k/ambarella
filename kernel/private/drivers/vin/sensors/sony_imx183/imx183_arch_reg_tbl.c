/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx183/imx183_arch_reg_tbl.c
 *
 * History:
 *    2014/08/13 - [Long Zhao] Create
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
static const struct imx183_reg_table imx183_share_regs[] = {
	{0x0058, 0x37},
	{0x005a, 0x2b},
	{0x0312, 0x11},
	{0x0355, 0x00},
	{0x0381, 0x00},
	{0x052e, 0x02},
	{0x0530, 0x0b},
	{0x0531, 0x0b},
	{0x0532, 0x0b},
	{0x0533, 0x0b},
	{0x0534, 0x0b},
	{0x0535, 0x0b},
	{0x053f, 0x1d},
	{0x0541, 0x1d},
	{0x0545, 0x00},
	{0x0549, 0x02},
	{0x054b, 0x00},
	{0x0555, 0x02},
	{0x0563, 0x05},
	{0x05a4, 0x00},
	{0x05a5, 0x07},
	{0x05aa, 0x00},
	{0x05d1, 0x16},
	{0x05d2, 0x15},
	{0x05d3, 0x14},
	{0x065c, 0x01},
	{0x065e, 0x01},
};

#define IMX183_SHARE_REG_SIZE		ARRAY_SIZE(imx183_share_regs)

static const struct imx183_pll_reg_table imx183_pll_tbl[] = {
	[0] = {
		.pixclk = 576000000,
		.extclk = PLL_CLK_72MHZ,
		.regs = {
		}
	},
};

