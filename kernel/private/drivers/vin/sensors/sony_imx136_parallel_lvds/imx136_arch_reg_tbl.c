/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_arch_reg_tbl.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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
static const struct imx136_reg_table imx136_share_regs[] = {
	/* Chip ID=03h, do not change */
	{0x030F, 0x0E},
	{0x0316, 0x02},

	/* Chip ID=04h, do not change */
	{0x0436, 0x71},
	{0x0439, 0xF1},
	{0x0441, 0xF2},
	{0x0442, 0x21},
	{0x0443, 0x21},
	{0x0448, 0xF2},
	{0x0449, 0x21},
	{0x044A, 0x21},
	{0x0452, 0x01},
	{0x0454, 0xB1},

	{0x020A, 0xF0},/* Black level */
	{0x020B, 0x00},
};
#define IMX136_SHARE_REG_SIZE		ARRAY_SIZE(imx136_share_regs)

static const struct imx136_pll_reg_table imx136_pll_tbl[] = {
	[0] = {// for 1080p@30fps
		.pixclk = PLL_CLK_148_5MHZ,
		.extclk = PLL_CLK_37_125MHZ,
		.regs = {
		}
	},

};