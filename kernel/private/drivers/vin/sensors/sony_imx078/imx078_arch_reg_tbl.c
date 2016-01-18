/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/imx078_arch_reg_tbl.c
 *
 * History:
 *    2013/03/11 - [Bingliang Hu] Create
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
static const struct imx078_reg_table imx078_share_regs[] = {
	{imx078_REG_DCKRST, 0x01},
	{imx078_REG_00, 0x00},
	{imx078_REG_01, 0x33},
	{imx078_REG_SVR_LSB, 0x00},
	{imx078_REG_SVR_MSB, 0x00},
	{imx078_REG_SPL_LSB, 0x00},
	{imx078_REG_SPL_MSB, 0x00},
	{imx078_REG_SHR_LSB, 0x09},
	{imx078_REG_SHR_MSB, 0x00},
	{imx078_REG_PGC_LSB, 0x00},
	{imx078_REG_PGC_MSB, 0x00},
	{imx078_REG_0A, 0x00},
	{imx078_REG_0D, 0x02},
	{imx078_REG_0E, 0x00},
	{imx078_REG_BLKLEVEL, 0x32},
	{imx078_REG_DGAIN, 0x80},
	{imx078_REG_17, 0x00},
	{imx078_REG_FREQ, 0x05},
	{imx078_REG_ADBIT, 0x01},
	{imx078_REG_4C, 0x74},
	{imx078_REG_4D, 0x74},
	{imx078_REG_DCKRST, 0x00},
	{imx078_REG_69, 0x00},
	{imx078_REG_330, 0x01},
	{imx078_REG_0A, 0x10},
};
#define imx078_SHARE_REG_SIZE		ARRAY_SIZE(imx078_share_regs)

static const struct imx078_pll_reg_table imx078_pll_tbl[] = {
	[0] = {
		.pixclk = 576000000,//extclk * 8
		.extclk = 72000000,
		.regs = {
		}
	},
};