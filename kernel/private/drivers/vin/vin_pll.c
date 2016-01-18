/*
 * kernel/private/drivers/ambarella/vin/vin_pll.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "vin_pri.h"
#include <plat/clk.h>

/* ========================================================================== */
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
#else

/* ==========================================================================*/
static int ambarella_so_clk_set_rate(struct clk *c, unsigned long rate)
{
	int ret_val = -1;
	u32 pre_scaler;
	u32 ctrl2;
	u32 ctrl3;
	u64 divident;
	u64 divider;
	u32 middle;
	union ctrl_reg_u ctrl_reg;
	union frac_reg_u frac_reg;
	u64 diff;

	if (c->extra_scaler > 1) {
		rate *= c->extra_scaler;
	}

	if (!rate) {
		ret_val = -1;
		goto ambarella_so_clk_set_rate_exit;
	}

	pre_scaler = amba_rct_readl(c->pres_reg);
	middle = ambarella_rct_find_pll_table_index(rate, pre_scaler,
		ambarella_rct_pll_table, AMBARELLA_RCT_PLL_TABLE_SIZE);

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.intp = ambarella_rct_pll_table[middle].intp;
	ctrl_reg.s.sdiv = ambarella_rct_pll_table[middle].sdiv;
	ctrl_reg.s.sout = ambarella_rct_pll_table[middle].sout;
	ctrl_reg.s.bypass = 0;
	ctrl_reg.s.frac_mode = 0;
	ctrl_reg.s.force_reset = 0;
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.tristate = 0;
	ctrl_reg.s.force_lock = 1;
	ctrl_reg.s.force_bypass = 0;
	ctrl_reg.s.write_enable = 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);
	ctrl_reg.s.write_enable = 1;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);
	ctrl_reg.s.write_enable = 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	amba_rct_writel(c->post_reg, ambarella_rct_pll_table[middle].post);

	if (c->frac_mode) {
		c->rate = ambarella_rct_clk_get_rate(c);
		if (c->rate < rate) {
			diff = rate - c->rate;
		} else {
			diff = c->rate - rate;
		}
		vin_dbgv("rate = %lu\n", rate);
		vin_dbgv("c->rate = %llu\n", c->rate);
		vin_dbgv("diff = %llu\n", diff);

		divident = (diff * pre_scaler *
			(ambarella_rct_pll_table[middle].sout + 1) *
			ambarella_rct_pll_table[middle].post);
		divident = divident << 32;
		divider = ((u64)REF_CLK_FREQ *
			(ambarella_rct_pll_table[middle].sdiv + 1));
		AMBCLK_DO_DIV_ROUND(divident, divider);
		if (c->rate <= rate) {
			frac_reg.s.nega	= 0;
			frac_reg.s.frac	= divident;
		} else {
			frac_reg.s.nega	= 1;
			frac_reg.s.frac	= 0x80000000 - divident;
		}
		vin_dbgv("frac_reg.s.nega = 0x%08X\n", frac_reg.s.nega);
		vin_dbgv("frac_reg.s.frac = 0x%08X\n", frac_reg.s.frac);
		amba_rct_writel(c->frac_reg, frac_reg.w);

		ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
		if (diff) {
			ctrl_reg.s.frac_mode = 1;
		} else {
			ctrl_reg.s.frac_mode = 0;
		}
		ctrl_reg.s.force_lock = 1;
		ctrl_reg.s.write_enable = 1;
		amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

		ctrl_reg.s.write_enable	= 0;
		amba_rct_writel(c->ctrl_reg, ctrl_reg.w);
	}
	if (ctrl_reg.s.frac_mode) {
		ctrl2 = 0x3f770000;
		ctrl3 = 0x00069300;
	} else {
		ctrl2 = 0x3f770000;
		ctrl3 = 0x00068300;
	}

	amba_rct_writel(c->ctrl2_reg, ctrl2);
	amba_rct_writel(c->ctrl3_reg, ctrl3);
	ret_val = 0;

ambarella_so_clk_set_rate_exit:
	c->rate = ambarella_rct_clk_get_rate(c);

	return ret_val;
}

struct clk_ops ambarella_rct_so_ops = {
	.enable		= ambarella_rct_clk_enable,
	.disable	= ambarella_rct_clk_disable,
	.get_rate	= ambarella_rct_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_so_clk_set_rate,
	.set_parent	= NULL,
};

static struct clk gclk_so = {
	.parent		= NULL,
	.name		= "gclk_so",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_SENSOR_CTRL_REG,
	.pres_reg	= SCALER_SENSOR_PRE_REG,
	.post_reg	= SCALER_SENSOR_POST_REG,
	.frac_reg	= PLL_SENSOR_FRAC_REG,
	.ctrl2_reg	= PLL_SENSOR_CTRL2_REG,
	.ctrl3_reg	= PLL_SENSOR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 3,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_so_ops,
};

static struct clk *rct_register_clk_so(void)
{
	struct clk *pgclk_so = NULL;

	pgclk_so = clk_get(NULL, "gclk_so");
	if (IS_ERR(pgclk_so)) {
		ambarella_register_clk(&gclk_so);
		pgclk_so = &gclk_so;
		pr_info("SYSCLK:SO[%lu]\n", clk_get_rate(pgclk_so));
	}

	return pgclk_so;
}
#endif

/* ========================================================================== */
void rct_set_so_clk_src(u32 mode)
{
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	amb_set_sensor_clock_pad_mode(HAL_BASE_VP, mode);
#else
	amba_rct_writel(USE_CLK_SI_INPUT_MODE_REG, (mode & 0x1));
#endif
}

void rct_set_so_freq_hz(u32 freq_hz)
{
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	amb_set_sensor_clock_frequency(HAL_BASE_VP, freq_hz);
#else
	clk_set_rate(rct_register_clk_so(), freq_hz);
#endif
}

u32 rct_get_so_freq_hz(void)
{
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	return (u32)amb_get_sensor_clock_frequency(HAL_BASE_VP);
#else
	return clk_get_rate(rct_register_clk_so());
#endif
}

void rct_set_vin_lvds_pad(int mode)
{
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	if (mode == VIN_LVDS_PAD_MODE_LVCMOS) {
		amb_set_lvds_pad_mode(HAL_BASE_VP,
			AMB_LVDS_PAD_MODE_LVCMOS);
	} else if (mode == VIN_LVDS_PAD_MODE_LVDS) {
		amb_set_lvds_pad_mode(HAL_BASE_VP,
			AMB_LVDS_PAD_MODE_LVDS);
	} else if (mode == VIN_LVDS_PAD_MODE_SLVS) {
		amb_set_lvds_pad_mode(HAL_BASE_VP,
			AMB_LVDS_PAD_MODE_SLVS);
	} else {
		vin_err("rct_set_vin_lvds_pad(%d) not supported\n", mode);
	}
#else
	if (mode == VIN_LVDS_PAD_MODE_LVCMOS) {
		amba_rct_writel(LVDS_LVCMOS_REG, 0xffffffff);
		amba_rct_clrbitsl(LVDS_ASYNC_REG, 0x0F000000);
		//amba_rct_setbitsl(LVDS_ASYNC_REG, 0x000FFFFF);
	} else if (mode == VIN_LVDS_PAD_MODE_LVDS) {
		amba_rct_writel(LVDS_LVCMOS_REG, 0x00000000);
		amba_rct_clrbitsl(LVDS_ASYNC_REG, 0x0F000000);
	} else if (mode == VIN_LVDS_PAD_MODE_SLVS) {
		amba_rct_writel(LVDS_LVCMOS_REG, 0x00000000);
		amba_rct_setbitsl(LVDS_ASYNC_REG, 0x0F000000);
	} else {
		vin_err("rct_set_vin_lvds_pad(%d) not supported\n", mode);
	}
#endif
}

