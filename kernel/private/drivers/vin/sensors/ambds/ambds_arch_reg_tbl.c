/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds_arch_reg_tbl.c
 *
 * History:
 *    2014/12/30 - [Long Zhao] Create
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
static const struct ambds_reg_table ambds_share_regs[] = {
	/* please set registers sensor/fpga used here */
};
#define AMBDS_SHARE_REG_SIZE		ARRAY_SIZE(ambds_share_regs)

static const struct ambds_pll_reg_table ambds_pll_tbl[] = {
	[0] = {
		.pixclk = 72000000,
		/* please set master clock here, amba soc will output this frequency from CLK_SI */
		.extclk = PLL_CLK_27MHZ,
		.regs = {
		}
	},
};
