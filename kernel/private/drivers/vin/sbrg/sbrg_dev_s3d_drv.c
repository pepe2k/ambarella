/*
 * kernel/private/drivers/ambarella/vin/sbrg/s3d/sbrg_dev_s3d_drv.c
 *
 * Author: Haowei Lo <hwlo@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <sbrg_pri.h>
#include <sbrg/amba_sbrg.h>

/* ========================================================================== */
#define S3D_SADDR_LVL			(1)       /* 1 High, 0 Low */

#ifndef CONFIG_I2C_AMBARELLA_SBRG_S3D_ADDR
#if (S3D_SADDR_LVL == 1)
	#define CONFIG_I2C_AMBARELLA_SBRG_S3D_ADDR	(0xA6>>1)
#else
	#define CONFIG_I2C_AMBARELLA_SBRG_S3D_ADDR	(0xA4>>1)
#endif
#endif

#define S3D_UNUSED			0
#define S3D_LEFT			1
#define S3D_RIGHT			2
#define S3D_BOTH			3

AMBA_SBRG_BASIC_PARAM_CALL(sbrg_s3d, CONFIG_I2C_AMBARELLA_SBRG_S3D_ADDR, 0, 0644);
/* ========================================================================== */

struct amba_sbrg_s3d_dev_info {
	struct __amba_sbrg_adapter	adap;
	struct i2c_client		client;
};

static struct i2c_driver i2c_driver_sbrg_s3d;

static const char sbrg_s3d_name[] = "s3d";



struct pll_rct_clk_obj_s {
	u32 	fref;
	u32 	freq;
	u16	pll_ctrl1;
	u16	pll_ctrl2;
	u16	clksi_prescaler;
	u16	pll_prescaler;
};

static struct pll_rct_clk_obj_s G_sbrg_rct_clk[] = {

	/*clk_si = 24M, clk_pll_out = 96M, dn = 60, dm = 5, dp = 3 */
	{PLL_CLK_24MHZ, PLL_CLK_96MHZ, 0x0c3b, 0x0204, 0x0000, 0x0000},
	/*clk_si = 24M, clk_pll_out = 120M, dn = 50, dm = 5, dp = 2 */
	{PLL_CLK_24MHZ, PLL_CLK_120MHZ, 0x0c31, 0x0104, 0x0000, 0x0000},
	/*clk_si = 24M, clk_pll_out = 144M, dn = 60, dm = 5, dp = 2 */
	{PLL_CLK_24MHZ, PLL_CLK_144MHZ, 0x0c3b, 0x0104, 0x0000, 0x0000},
	/*clk_si = 24M, clk_pll_out = 150M, dn = 50, dm = 4, dp = 2 */
	{PLL_CLK_24MHZ, PLL_CLK_150MHZ, 0x0c31, 0x0103, 0x0000, 0x0000},
	/*clk_si = 24M, clk_pll_out = 156M, dn = 52, dm = 4, dp = 2 */
	{PLL_CLK_24MHZ, PLL_CLK_156MHZ, 0x0c33, 0x0103, 0x0000, 0x0000},
	/*clk_si = 24M, clk_pll_out = 24M, For FPGA */
	{PLL_CLK_24MHZ, PLL_CLK_24MHZ, 0x0c00, 0x0000, 0x0001, 0x0000},
	/*clk_si = 48M, clk_pll_out = 60M, For FPGA */
	{PLL_CLK_48MHZ, PLL_CLK_60MHZ, 0x0c00, 0x0000, 0x0002, 0x0000},
	/* NULL */
        {0x0, 0x0, 0x0000, 0x0000, 0x0010, 0x0000}
};

//S3D IDC
/* ========================================================================== */
static int sbrg_s3d_write_reg( struct __amba_sbrg_adapter *adap, u16 subaddr, u16 data)
{
	int errCode = 0;
	struct amba_sbrg_s3d_dev_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	sbrg_dbg("sbrg s3d write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct amba_sbrg_s3d_dev_info *)adap->pinfo;
	client = &pinfo->client;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data >> 8);
	pbuf[3] = (data & 0xff);

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		sbrg_err("sbrg_s3d_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int sbrg_s3d_read_reg( struct __amba_sbrg_adapter *adap, u16 subaddr, u16 *pdata)
{
	int	errCode = 0;
	struct amba_sbrg_s3d_dev_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	pinfo = (struct amba_sbrg_s3d_dev_info *)adap->pinfo;
	client = &pinfo->client;


	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 2;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)){
		sbrg_err("sbrg_s3d_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = ((pbuf[0] << 8) | pbuf[1]);
		errCode = 0;
	}

	return errCode;
}

/**
 * Write S3D by using IDCS
 */
static void wr_s3d_reg(struct __amba_sbrg_adapter *adap, u16 addr, u16 *data,
			u16 size, u8 burst)
{
	u16 s3d_count = 0;

//	if (burst == 1) {
	//Not support yet
//	} else {
		do {
			sbrg_s3d_write_reg(adap, addr, *data );
			s3d_count++;
			addr++;
			data++;
		} while (s3d_count < size);
//	}
}

/**
 * Read S3D by using IDCS
 */
static void rd_s3d_reg(struct __amba_sbrg_adapter *adap, u16 addr, u16 *data, u16 size)
{
	u16 s3d_count = 0;

	do {
		sbrg_s3d_read_reg(adap, addr, data);
		s3d_count++;
		addr++;
		data++;
	} while (s3d_count < size);
}

/**
 * ACCESS S3D VIN Wrapper
 */
static void s3d_vin_reg_access(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
			       u16 offset, u16 *data, u16 size)
{
	wr_s3d_reg(adap, S3D_REG_ADDR(sub_module_id, offset), data, size, 0);
}

/* ========================================================================== */


/**
 * S3D adjust input_clk value function
 */
static u32 get_fref_index(struct __amba_sbrg_adapter *adap, u32 fref)
{
	u32 clk_si_freq;

	clk_si_freq = fref / 100000;
	clk_si_freq = (clk_si_freq + 1) / 10;
	clk_si_freq = clk_si_freq * 1000000;

	return clk_si_freq;
}

/**
 * config RCT
 */
static int config_s3d_rct(struct __amba_sbrg_adapter *adap, sbrg_rct_3d_cfg_t *rct_3d_cfg)
{
	int i;
	u16 reg;
	u32 input_freq = rct_3d_cfg->input_freq_hz;
	u32 output_freq = rct_3d_cfg->output_freq_hz;
	u32 fref;

	reg = (((rct_3d_cfg->clk_si_sel & 0x1) << 1)	|
	       (rct_3d_cfg->gclk_core_sel & 0x1));

	/* Using PLL CLK */
	if (reg != 0) {
		/* Set clk select */
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_GCLK_CORE_SEL_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		/* lookup pll setting */
		fref = get_fref_index(adap, input_freq);

		for (i = 0; ;i++) {
		        if ((G_sbrg_rct_clk[i].fref == 0) ||
			    ((G_sbrg_rct_clk[i].fref == fref) &&
			     (G_sbrg_rct_clk[i].freq == output_freq)))
				break;
		}

		if (G_sbrg_rct_clk[i].fref != 0) {
			wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
				   &(G_sbrg_rct_clk[i].pll_ctrl1),
				   NWORDS(u16),
				   0);
			wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL2_OFFSET),
				   &(G_sbrg_rct_clk[i].pll_ctrl2),
				   NWORDS(u16),
				   0);
			wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_ED_DIV_CLK_SENSOR_OFFSET),
				   &(G_sbrg_rct_clk[i].clksi_prescaler),
				   NWORDS(u16),
				   0);
			wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_ED_DIV_CLK_PLL_REF_OFFSET),
				   &(G_sbrg_rct_clk[i].pll_prescaler),
				   NWORDS(u16),
				   0);
		} else {
			return -1;
		}

		do {
			rd_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
				   &reg,
				   NWORDS(u16));
		} while ((reg & 0x1000) == 0);
	}

	return 0;
}

/**
 * Config MIPI PHY
 */
static void config_s3d_mipi_phy(struct __amba_sbrg_adapter *adap, u8 s3d_channel,
				sbrg_rct_3d_mipi_phy_cfg_t *rct_3d_mipi_phy)
{
	u16 reg;
	switch (s3d_channel) {
		case S3D_UNUSED:
			break;

		case S3D_LEFT:
		/* config MIPI PHY LEFT */
		reg = rct_3d_mipi_phy->mipi_phy_ctrl_1;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = rct_3d_mipi_phy->mipi_phy_ctrl_2;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = rct_3d_mipi_phy->mipi_phy_ctrl_3;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		/* config MIPI PHY RIGHT */
		reg = LVCMOS_INPUT_PAD;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = 0;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = RCT_MIPI_PHY_3_POWER_DOWN;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
			break;

		case S3D_RIGHT:
		/* config MIPI PHY LEFT */
		reg = LVCMOS_INPUT_PAD;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = 0;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = RCT_MIPI_PHY_3_POWER_DOWN;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		/* config MIPI PHY RIGHT */
		reg = rct_3d_mipi_phy->mipi_phy_ctrl_1;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = rct_3d_mipi_phy->mipi_phy_ctrl_2;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = rct_3d_mipi_phy->mipi_phy_ctrl_3;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
			break;

		case S3D_BOTH:
		/* config MIPI PHY BOTH */
		reg = rct_3d_mipi_phy->mipi_phy_ctrl_1;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_1_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
		reg = rct_3d_mipi_phy->mipi_phy_ctrl_2;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_2_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);

		reg = rct_3d_mipi_phy->mipi_phy_ctrl_3;
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_L_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_MIPI_PHY_CTRL_R_3_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
			break;
	}
}

/**
 * Config s_control and s_inputcfg registers
 */
static void config_s3d_vin_info(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	/* s3d vin ctrl */
	reg = (((vin_info->hsync_mask & 0x1) << 15) 			|
	       ((vin_info->ecc_enable & 0x1) << 14) 			|
	       ((vin_info->sony_field_mode & 0x1) << 13)		|
	       ((vin_info->field0_pol & 0x1) << 12) 			|
	       ((vin_info->hs_polarity & 0x1) << 11) 			|
	       ((vin_info->vs_polarity & 0x1) << 10) 			|
	       ((vin_info->emb_sync_loc & 0x3) << 8) 			|
	       ((vin_info->emb_sync_mode & 0x1) << 7) 			|
	       ((vin_info->emb_sync & 0x1) << 6) 			|
	       ((vin_info->sync_mode & 0x3) << 4) 			|
	       ((vin_info->data_edge & 0x1) << 3)
	      );

	reg |= 0x6;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_CTRL_OFFSET,
			   &reg,
			   NWORDS(reg));

	/* s3d vin ipcfg */
	reg = (((vin_info->clk_select & 0x1) << 13) 			|
	       ((vin_info->mipi_lanes & 0x3) << 9)	 		|
	       ((vin_info->src_data_width & 0x3) << 5) 			|
	       (vin_info->input_mode & 0x19f )
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_INCFG_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * config VIN unit H-sync pulse width and V-sync pulse width at the master mode
 */
static void config_s3d_vin_vh_width(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				    sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->vsync_w;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_V_WIDTH_OFFSET,
			   &reg,
			    NWORDS(reg));

	reg = vin_info->hsync_w;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_H_WIDTH_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * configure VIN unit horizontal offset from H-sync in sensor clock cycles
 * for V-sync assertion and deassertion in bottom field at the master mode
 */
static void config_s3d_vin_hoffset(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				   sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->top_vsync_offset;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_HOF_TOP_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->bottom_vsync_offset;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_HOF_BTM_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * configure VIN unit the number of sensor clock cycles per line and
 * tbe number of lines per field at the master mode
 */
static void config_s3d_vin_VH(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->pel_clk_a_line - 1;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_V_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->line_num_a_field - 1;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_H_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * configure VIN unit minimum H-sync and V-sync pulse width in slave mode
 * for short line and short field detection
 */
static void config_s3d_vin_min_HV_sync_width(struct __amba_sbrg_adapter *adap,
		u16 sub_module_id, sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->vs_min - 1;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_MIN_V_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->hs_min - 1;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_MIN_H_OFFSET,
		 	   &reg,
			   NWORDS(reg));
}

/**
 * The trigger pin can be programmed to go low or high at given line
 * after V-sync signal asserted by external sensor module
 */
static void config_s3d_vin_trigger_pin0(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
					trigger_pin_info_t *trigger_info)
{
	u16 reg;

	reg = (((trigger_info->enabled & 0x1) << 15) 	|
	       ((trigger_info->polarity & 0x1) << 14) 	|
	       (trigger_info->start_line & 0x3fff)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TRG_0_ST_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = trigger_info->last_line & 0x3fff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TRG_0_ED_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * The trigger pin can be programmed to go low or high at given line
 * after V-sync signal asserted by external sensor module
 */
static void config_s3d_vin_trigger_pin1(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
					trigger_pin_info_t *trigger_info)
{
	u16 reg;

	reg = (((trigger_info->enabled & 0x1) << 15) 	|
	       ((trigger_info->polarity & 0x1) << 14) 	|
	       (trigger_info->start_line & 0x3fff)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TRG_1_ST_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = trigger_info->last_line & 0x3fff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TRG_1_ED_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Set cropping window to capture image data of window
 */
static void config_s3d_vin_set_video_capture_window(struct __amba_sbrg_adapter *adap,
				u16 sub_module_id, sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->start_x;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_CAPSTARTH_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->start_y;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_CAPSTARTV_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->end_x;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_CAPENDH_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_info->end_y;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_CAPENDV_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Set all-zero embedded sync horizontal blank interval length
 */
static void config_s3d_vin_set_hori_blank_length(struct __amba_sbrg_adapter *adap,
				u16 sub_module_id, sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->hb_length;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_BLANKLENGTH_H_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Set the timeout count for detecting Vsync before assertion
 */
static void config_s3d_vin_set_timout_vd_detection(struct __amba_sbrg_adapter *adap,
				u16 sub_module_id, sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->v_timeout_count & 0xffff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TOUT_V_LOW_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = (vin_info->v_timeout_count >> 16) & 0xffff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TOUT_V_HIGH_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Set the timeout count for detecting Hsync before assertion
 */
static void config_s3d_vin_set_timout_hd_detection(struct __amba_sbrg_adapter *adap,
				u16 sub_module_id, sbrg_vin_3d_cfg_t *vin_info)
{
	u16 reg;

	reg = vin_info->h_timeout_count & 0xffff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TOUT_H_LOW_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = (vin_info->h_timeout_count >> 16) & 0xffff;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_TOUT_H_HIGH_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * config slvs_control registers
 */
static void config_s3d_vin_slvs_info(struct __amba_sbrg_adapter *adap,
			u16 sub_module_id, sbrg_vin_3d_slvs_cfg_t *vin_slvs_cfg)
{
	u16 reg;

	reg = (((vin_slvs_cfg->slvs_code_check & 0x3) << 14) 		|
	       ((vin_slvs_cfg->slvs_variable_hblank_en & 0x1) << 13) 	|
	       ((vin_slvs_cfg->slvs_vsync_max_en & 0x1) << 12) 		|
	       ((vin_slvs_cfg->slvs_act_lanes & 0xf) << 8) 		|
	       ((vin_slvs_cfg->slvs_param_at_posedge_en & 0x1) << 7) 	|
	       ((vin_slvs_cfg->slvs_vin_stall_en & 0x1) << 6) 		|
	       ((vin_slvs_cfg->slvs_n_jitter_en & 0x1) << 5)		|
	       ((vin_slvs_cfg->slvs_sync_ecc_en & 0x1) << 4) 		|
	       ((vin_slvs_cfg->slvs_sync_repeat_2lanes & 0x1) << 3) 	|
	       ((vin_slvs_cfg->slvs_sync_repeat_all & 0x1) << 2) 	|
	       ((vin_slvs_cfg->sony_slvs_mode & 0x1) << 1) 		|
	       (vin_slvs_cfg->serial_mode & 0x1)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_SLVS_CTRL_OFFSET,
			   &reg,
			   NWORDS(reg));

	if (vin_slvs_cfg->serial_mode & 0x1) {
		reg = vin_slvs_cfg->slvs_sav2sav_dist;
		s3d_vin_reg_access(adap, sub_module_id,
				   S3D_VIN_SLVS_VH_WIDTH_OFFSET,
				   &reg,
				   NWORDS(reg));


		reg = vin_slvs_cfg->slvs_eav_col;
		s3d_vin_reg_access(adap, sub_module_id,
				   S3D_VIN_SLVS_ACTIVE_VH_WIDTH_OFFSET,
				   &reg,
				   NWORDS(reg));
	}

	if (vin_slvs_cfg->slvs_vsync_max_en & 0x1) {
		reg = vin_slvs_cfg->vsync_max;
		s3d_vin_reg_access(adap, sub_module_id,
				   S3D_VIN_SLVS_VSYNC_MAX_OFFSET,
				   &reg,
				   NWORDS(reg));
	}

	if (vin_slvs_cfg->slvs_variable_hblank_en & 0x1) {
		reg = vin_slvs_cfg->hblank;
		s3d_vin_reg_access(adap, sub_module_id,
				   S3D_VIN_SLVS_LINE_SYNC_TOUT_OFFSET,
				   &reg,
				   NWORDS(reg));
	}
}

/**
 * config mipi_control registers
 */
static void config_s3d_vin_mipi_info(struct __amba_sbrg_adapter *adap,
			u16 sub_module_id, sbrg_vin_3d_mipi_cfg_t *vin_mipi_cfg)
{
	u16 reg;

	reg = (((vin_mipi_cfg->mipi_decompression_mode & 0x3) << 14) 	|
	       ((vin_mipi_cfg->mipi_s_clksettlectl & 0x3) << 12) 	|
	       ((vin_mipi_cfg->mipi_s_dpdn_swap_data & 0x1) << 11) 	|
	       ((vin_mipi_cfg->mipi_s_dpdn_swap_clk & 0x1) << 10) 	|
	       ((vin_mipi_cfg->mipi_s_hssettlectl & 0x3f) << 5) 	|
	       ((vin_mipi_cfg->mipi_enable & 0x1) << 4)	 		|
	       ((vin_mipi_cfg->mipi_virtualch_mask & 0x3) << 2)		|
	       (vin_mipi_cfg->mipi_virtualch_select & 0x3)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_MIPI_CTRL_0_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = (((vin_mipi_cfg->mipi_decompression_enable & 0x1) << 15) 	|
	       ((vin_mipi_cfg->mipi_pixel_byte_swap & 0x1) << 14) 	|
	       ((vin_mipi_cfg->mipi_data_type_mask & 0x3f) << 8) 	|
	       (vin_mipi_cfg->mipi_data_type_sel & 0x3f)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_MIPI_CTRL_1_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_mipi_cfg->mipi_b_dphyctl;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_MIPI_B_DPHYCTL_OFFSET,
			   &reg,
			   NWORDS(reg));

	reg = vin_mipi_cfg->mipi_s_dphyctl;
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_MIPI_S_DPHYCTL_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Set slvs lane mux select
 */
static void config_s3d_vin_set_slvs_lane_mux_select(struct __amba_sbrg_adapter *adap,
				u16 sub_module_id, u8 *lane_mux_sel)
{
	u16 reg[4] = {0, 0, 0, 0};
	u16 addr = 0, act_lanes = 0, data;
	u8  i = 0;

	if ((sub_module_id == S3D_CONFIG_SLVS_LANEMUX_L_MODULE)  ||
	    (sub_module_id == S3D_CONFIG_SLVS_LANEMUX_BOTH_MODULE)) {
		/* get actived lanes number */
		addr = S3D_VIN_L_ADDR(S3D_VIN_SLVS_CTRL_OFFSET);
		rd_s3d_reg(adap, addr, &act_lanes, NWORDS(act_lanes));
		act_lanes = ((act_lanes & 0x0f00) >> 8) + 1;

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_0_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[0] = data;

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_1_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[1] = data;

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_2_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[2] = data;

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_3_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[3] = data;

		for (i = 0; i < act_lanes; i++) {
			reg[i >> 2] =
			(((reg[i >> 2]) & (0xffff ^ (0x000f <<
			  ((i % 4) << 2)))) |
		  	 ((lane_mux_sel[i] & 0xf) << ((i % 4) << 2))
	       	 	);
		}

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_0_OFFSET);
		data = reg[0];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_1_OFFSET);
		data = reg[1];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_2_OFFSET);
		data = reg[2];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_L_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_3_OFFSET);
		data = reg[3];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);
	}

	if ((sub_module_id == S3D_CONFIG_SLVS_LANEMUX_R_MODULE)  ||
	    (sub_module_id == S3D_CONFIG_SLVS_LANEMUX_BOTH_MODULE)) {
		/* get actived lanes number */
		addr = S3D_VIN_R_ADDR(S3D_VIN_SLVS_CTRL_OFFSET);
		rd_s3d_reg(adap, addr, &act_lanes, NWORDS(act_lanes));
		act_lanes = ((act_lanes & 0x0f00) >> 8) + 1;

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_0_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[0] = data;

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_1_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[1] = data;

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_2_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[2] = data;

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_3_OFFSET);
		rd_s3d_reg(adap, addr, &data, NWORDS(u16));
		reg[3] = data;

		for (i = 0; i < act_lanes; i++) {
			reg[i >> 2] =
			(((reg[i >> 2]) & (0xffff ^ (0x000f <<
			  ((i % 4) << 2)))) |
		  	 ((lane_mux_sel[i] & 0xf) << ((i % 4) << 2))
	       	 	);
		}

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_0_OFFSET);
		data = reg[0];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_1_OFFSET);
		data = reg[1];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_2_OFFSET);
		data = reg[2];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);

		addr = S3D_VIN_R_ADDR(S3D_VIN_LANE_MUX_SELECT_REG_3_OFFSET);
		data = reg[3];
		wr_s3d_reg(adap, addr, &data, NWORDS(u16), 0);
	}
}

/**
 * S3D VIN reset
 */
static void vin_s3d_set_vin_reset(struct __amba_sbrg_adapter *adap, u16 sub_module_id)
{
	u16 reg = 0x10;

	switch (sub_module_id) {
	case S3D_RESET_L_VIN:
		wr_s3d_reg(adap, S3D_VIN_L_ADDR(S3D_VIN_MIPI_CTRL_0_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);
		reg = 0x3fe6;
		break;

	case S3D_RESET_R_VIN:
		wr_s3d_reg(adap, S3D_VIN_R_ADDR(S3D_VIN_MIPI_CTRL_0_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);
		reg = 0x3cdf;
		break;

	case S3D_RESET_BOTH_VIN:
		wr_s3d_reg(adap, S3D_VIN_BOTH_ADDR(S3D_VIN_MIPI_CTRL_0_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);
		reg = 0x3cc6;
		break;
	};

	wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_REG_SOFT_RESET_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = 0x3fff;
	wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_REG_SOFT_RESET_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
}

/**
 * Set IDSPcmd register and update done
 */
static void config_s3d_vin_idspcmd(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				   sbrg_vin_3d_idspcmd_cfg_t *vin_idspcmd_cfg)
{
	u16 reg;

	reg = (((vin_idspcmd_cfg->ctl_vin_vsync_en & 0x1) << 12) 	|
	       ((vin_idspcmd_cfg->ctl_vin_hsync_en & 0x1) << 11) 	|
	       ((vin_idspcmd_cfg->mipi_enable3 & 0x1) << 10) 		|
	       ((vin_idspcmd_cfg->mipi_enable2 & 0x1) << 9) 		|
	       ((vin_idspcmd_cfg->mipi_enable1 & 0x1) << 8) 		|
	       ((vin_idspcmd_cfg->mipi_enable0 & 0x1) << 7) 		|
	       ((vin_idspcmd_cfg->lvdsspclk_pr & 0x1) << 6) 		|
	       ((vin_idspcmd_cfg->gclksovin_pr & 0x1) << 5) 		|
	       ((vin_idspcmd_cfg->vinmipi_clk_pr & 0x1) << 4) 		|
	       ((vin_idspcmd_cfg->clkslvs_pol & 0x1) << 3) 		|
	       ((vin_idspcmd_cfg->clkvin_pol & 0x1) << 2) 		|
	       ((vin_idspcmd_cfg->mipi_clk & 0x1) << 1) 		|
	       (vin_idspcmd_cfg->input_clk & 0x1)
	      );
	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_S_IDSPCMD_OFFSET,
			   &reg,
			   NWORDS(reg));
}

static void config_s3d_vin_done(struct __amba_sbrg_adapter *adap, u16 sub_module_id,
				sbrg_vin_3d_update_cfg_t *update_3d_cfg)
{
	u16 reg;

	reg =(((update_3d_cfg->update_done_always & 0x1) << 2)	|
	      ((update_3d_cfg->update_done_once & 0x1) << 1)
	     );

	s3d_vin_reg_access(adap, sub_module_id,
			   S3D_VIN_UPDATE_DONE_OFFSET,
			   &reg,
			   NWORDS(reg));
}

/**
 * Config s3d prescaler
 */

static void config_s3d_prescaler(struct __amba_sbrg_adapter *adap,
			sbrg_prescaler_3d_cfg_t *prescaler_cfg)
{
	u16 reg;

	reg = (((prescaler_cfg->coefficient_shift_en & 0x1) << 1) 	|
	       (prescaler_cfg->h_prescaler & 0x1)
	      );
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_CTRL_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->hout_width;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_HOUT_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->hphase_incr;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_HPHASE_INCR_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->init_c0_int;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_INIT_C0_INT_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->init_c0_frac;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_INIT_C0_FRAC_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->init_c1_int;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_INIT_C1_INT_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->init_c1_frac;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_INIT_C1_FRAC_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = prescaler_cfg->prescaler_update_done;
	wr_s3d_reg(adap, S3D_PRESCALER_ADDR(S3D_PRESCALER_UPDATE_DONE_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
}

/**
 * Config s3d precaler coefficients
 */
static void config_s3d_presc_coef(struct __amba_sbrg_adapter *adap,
				u8 channel, u16 *reg)
{
	u16 data_count = 0;
//	u16 ck_reg[3];
	u16 coef_addr, coef_addr_limit;

	switch (channel) {
		case S3D_UNUSED:
			/* channel warnning */
			coef_addr = 0x840;
			coef_addr_limit = 0x87c;
			break;
		case S3D_LEFT:
			coef_addr = 0x340;
			coef_addr_limit = 0x37c;
			break;
		case S3D_RIGHT:
			coef_addr = 0x440;
			coef_addr_limit = 0x47c;
			break;
		case S3D_BOTH:
			coef_addr = 0x840;
			coef_addr_limit = 0x87c;
			break;
		default:
			sbrg_err("Unkown channel for prescl\n");
			goto config_s3d_presc_coef_exit;
			break;
	}

	for (;coef_addr <= coef_addr_limit;) {
		wr_s3d_reg(adap, coef_addr, &reg[data_count], NWORDS(u16) * 3, 0);
		data_count += 3;
		coef_addr += 0x4;
//		rd_s3d_reg(adap, coef_addr, &ck_reg, NWORDS(u16) * 3);
	}
config_s3d_presc_coef_exit:
	return;
}

/* S3D prescale entrance */
static void s3d_config_prescaler (struct __amba_sbrg_adapter *adap,
			 sbrg_prescaler_obj_t *cfg)
{
	u16 coef_array[] = {		//offset
		0x1dfe, 0x1d4a, 0x00fe,	//0x40 0x41 0x42
		0x19fe, 0x214a, 0x00fe,	//0x44 0x45 0x46
		0x16fe, 0x2449, 0x00ff,	//0x48 0x49 0x4a
		0x12fe, 0x2949, 0xffff,	//0x4c 0x4d 0x4e
		0x0ffe, 0x2d47, 0xff00,	//0x50 0x51 0x52
		0x0cfe, 0x3145, 0xff01,	//0x54 0x55 0x56
		0x0afe, 0x3542, 0xff02,	//0x58 0x59 0x5a
		0x08fe, 0x383f, 0xff04,	//0x5c 0x5d 0x5e
		0x05ff, 0x3c3c, 0xff05,	//0x60 0x61 0x62
		0x04ff, 0x3f38, 0xfe08,	//0x64 0x65 0x66
		0x02ff, 0x4235, 0xfe0a,	//0x68 0x69 0x6a
		0x01ff, 0x4531, 0xfe0c,	//0x6c 0x6d 0x6e
		0x00ff, 0x472d, 0xfe0f,	//0x70 0x71 0x72
		0xffff, 0x4929, 0xfe12,	//0x74 0x75 0x76
		0xff00, 0x4924, 0xfe16,	//0x78 0x79 0x7a
		0xfe00, 0x4a21, 0xfe19	//0x7c 0x7d 0x7e
		};

	u32 shift = 0;
	int h_init_phase_first_color = 0;
	int h_init_phase_second_color = 0;
	sbrg_prescaler_3d_cfg_t s3d_prescale_cfg = {0};

	if (cfg->h_in_width != cfg->h_out_width) {
		s3d_prescale_cfg.hphase_incr		=
				cfg->h_in_width *
				0x2000 /
				cfg->h_out_width;
		s3d_prescale_cfg.h_prescaler		=
						H_PRESCALER_ENABLE;
		s3d_prescale_cfg.coefficient_shift_en	=
						COEF_SHIFT_DISABLE;
		s3d_prescale_cfg.hout_width		=
						cfg->h_out_width - 1;
		s3d_prescale_cfg.prescaler_update_done	=
						PRESCALER_UPDATE_DONE;
	} else {
		s3d_prescale_cfg.h_prescaler		=
						H_PRESCALER_DISABLE;
		s3d_prescale_cfg.coefficient_shift_en	=
						COEF_SHIFT_DISABLE;
		s3d_prescale_cfg.hout_width		=
						cfg->h_out_width - 1;
		s3d_prescale_cfg.prescaler_update_done	=
						PRESCALER_UPDATE_DONE;
	}

	/* select formula */
	switch (cfg->h_sensor_readout_mode) {
		case S3D_PRESCALE_DEFAULT:
			h_init_phase_first_color = 0;
			h_init_phase_second_color = 0;
			break;
		case S3D_PRESCALE_BIN2:
			shift = (2 * s3d_prescale_cfg.hphase_incr
				 - (1 << 13)) / 4;
			h_init_phase_first_color -= shift / 2;
			h_init_phase_second_color += shift / 2;
			break;
		case S3D_PRESCALE_SKIP2:
			shift = (2 * s3d_prescale_cfg.hphase_incr
				 - (1 << 13)) / 4;
			h_init_phase_first_color += shift / 2;
			h_init_phase_second_color -= shift / 2;
			break;
	}

	/* rounding */
	s3d_prescale_cfg.init_c0_int	=
		(h_init_phase_first_color + INITIAL_PHASE_ROUNDING) >> 13;
	s3d_prescale_cfg.init_c0_frac	=
		(h_init_phase_first_color + INITIAL_PHASE_ROUNDING) & 0x1fff;
	s3d_prescale_cfg.init_c1_int	=
		(h_init_phase_second_color + INITIAL_PHASE_ROUNDING) >> 13;
	s3d_prescale_cfg.init_c1_frac	=
		(h_init_phase_second_color + INITIAL_PHASE_ROUNDING) & 0x1fff;
	config_s3d_prescaler(adap, &s3d_prescale_cfg);

	/* config s3d prescaler */
	if (cfg->h_in_width != cfg->h_out_width) {

		config_s3d_presc_coef(adap, S3D_BOTH, coef_array);
	}
}

/**
 * Config s3d voutf
 */
static void config_s3d_voutf(struct __amba_sbrg_adapter *adap,
			sbrg_voutf_3d_cfg_t *voutf_3d_cfg)
{
	u16 reg;

	/* config voutf 3d mode register */
	reg = voutf_3d_cfg->voutf_mode & 0x7;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_MODE_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/*set voutf */
	/* config voutf 3d act pixel register */
	reg = voutf_3d_cfg->voutf_act_pixel & 0x3fff;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_ACT_PIXEL_OFFSET),
	  	   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d blank pixel register */
	reg = voutf_3d_cfg->voutf_blank_pixel & 0x3fff;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_BLANK_PIXEL_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d act line register */
	reg = voutf_3d_cfg->voutf_act_line & 0x1fff;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_ACT_LINE_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d read threshold  register */
	reg = voutf_3d_cfg->voutf_rd_th & 0x3fff;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_RD_TH_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d black border  register */
	reg = voutf_3d_cfg->voutf_black_border & 0xf;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_BLACK_BORDER_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d vd clk polarity  register */
	reg = voutf_3d_cfg->voutf_vd_clk_pol & 0x1;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_VD_CLK_POL_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config voutf 3d read l fst r ch th  register */
	reg = voutf_3d_cfg->voutf_l_ch_fst_r_ch_th & 0x3;
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_L_CH_FST_R_CH_TH_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	if (voutf_3d_cfg->voutf_pg_en & 0x1) {
		/* config voutf 3d pattern generator init register */
		reg = voutf_3d_cfg->voutf_pg_l_ini & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PG_L_INI_OFFSET),
		   	   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf 3d pattern generator init register */
		reg = voutf_3d_cfg->voutf_pg_r_ini & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PG_R_INI_OFFSET),
		   	   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf 3d pattern generator init register */
		reg = voutf_3d_cfg->voutf_pg_l_inc & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PG_L_INC_OFFSET),
		   	   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf 3d pattern generator init register */
		reg = voutf_3d_cfg->voutf_pg_r_inc & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PG_R_INC_OFFSET),
		   	   &reg,
			   NWORDS(reg),
			   0);
	}

	if (voutf_3d_cfg->voutf_mode == 0x4) {
		/*set voutf mode 4 (PIP mode)*/
		/* config voutf PIP select register */
		reg = voutf_3d_cfg->voutf_pip_sel & 0x1;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PIP_SEL_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf PIP start line register */
		reg = voutf_3d_cfg->voutf_pip_start_line & 0x1fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PIP_START_LINE_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf PIP end line register */
		reg = voutf_3d_cfg->voutf_pip_end_line & 0x1fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PIP_END_LINE_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf PIP start pixel register */
		reg = voutf_3d_cfg->voutf_pip_start_pixel & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PIP_START_PIXEL_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		/* config voutf PIP end pixel register */
		reg = voutf_3d_cfg->voutf_pip_end_pixel & 0x3fff;
		wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_PIP_END_PIXEL_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);
	}

	/* config voutf 3d enable register */
	reg = (((voutf_3d_cfg->voutf_pg_en & 0x1) << 1) |
	       ((voutf_3d_cfg->voutf_en & 0x1) << 0)
	      );
	wr_s3d_reg(adap, S3D_VOUTF_ADDR(S3D_VOUTF_3D_EN_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
}

/**
 * Config s3d lcd 3d vout
 */
static void config_s3d_lcd_3d_vout(struct __amba_sbrg_adapter *adap,
			sbrg_lcd_3d_vout_cfg_t *lcd_3d_vout_cfg)
{
	u16 reg;

	/* config lcd 3d vout pattern gen registers */
	reg = lcd_3d_vout_cfg->patgen_en & 0x1;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_PATTERN_GEN_EN_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config lcd 3d vout vsync registers */
	reg = lcd_3d_vout_cfg->num_vsync_low & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_V_LOW_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = lcd_3d_vout_cfg->num_vsync_back & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_V_BACK_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = lcd_3d_vout_cfg->num_vsync_active & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_V_ACTIVE_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = lcd_3d_vout_cfg->num_vsync_front & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_V_FRONT_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config lcd 3d vout hsync registers */
	reg = lcd_3d_vout_cfg->num_hsync_low & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_H_LOW_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = lcd_3d_vout_cfg->num_hsync_back & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_H_BACK_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	reg = lcd_3d_vout_cfg->num_hsync_active & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_H_ACTIVE_OFFSET),
				&reg, NWORDS(reg), 0);

	reg = lcd_3d_vout_cfg->num_hsync_front & 0xffff;
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_H_FRONT_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	if (lcd_3d_vout_cfg->demux_sel != LCD_3D_OUTPUT_INTERFACE_BT601) {
		/* config lcd 3d vout hif2mcu registers */
		reg = (((lcd_3d_vout_cfg->hif2mcu_rw & 0x1) << 15) |
		       ((lcd_3d_vout_cfg->hif2mcu_dcx & 0x1) << 13)
		      );
		wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(
					S3D_LCD_3D_VOUT_HIF2MCU_CTRL_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);

		reg = lcd_3d_vout_cfg->hif2mcu_data & 0xffff;
		wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(
					S3D_LCD_3D_VOUT_HIF2MCU_DATA_OFFSET),
			   &reg,
			   NWORDS(reg),
			   0);
	}

	/* config lcd 3d vout CSB register */
	reg = (((lcd_3d_vout_cfg->num_cssu & 0xff) << 8) |
	       ((lcd_3d_vout_cfg->num_cshd & 0xff) << 0)
	      );
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_CSB_PIN_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config lcd 3d vout E register */
	reg = (((lcd_3d_vout_cfg->num_wrsu & 0xff) << 8) |
	       ((lcd_3d_vout_cfg->num_wrhd & 0xff) << 0)
	      );
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_E_PIN_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config lcd 3d vout write/read enable cycle register */
	reg = (((lcd_3d_vout_cfg->num_wren & 0xff) << 8) |
	       ((lcd_3d_vout_cfg->num_rden & 0xff) << 0)
	      );
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_WR_EN_CYCLE_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);

	/* config lcd 3d vout ctrl register */
	reg = (((lcd_3d_vout_cfg->div3_wait_lock & 0x1) << 14)	|
	       ((lcd_3d_vout_cfg->rgb888_mode & 0x1) << 13)	|
	       ((lcd_3d_vout_cfg->din_wid_is_8b & 0x1) << 12)	|
	       ((lcd_3d_vout_cfg->m8068_cdx_sel & 0x1) << 11)	|
	       ((lcd_3d_vout_cfg->m8068_burst_en & 0x1) << 10)	|
	       ((lcd_3d_vout_cfg->cyc_per_pxl & 0x3) << 8)	|
	       ((lcd_3d_vout_cfg->interleave_seq & 0x3) << 6)	|
	       ((lcd_3d_vout_cfg->demux_sel & 0x1) << 4)	|
	       ((lcd_3d_vout_cfg->bt601_den_sel & 0x1) << 3)	|
	       ((lcd_3d_vout_cfg->dclk_phase_sel & 0x1) << 2)	|
	       ((lcd_3d_vout_cfg->in_phase_sel & 0x1) << 1)	|
	       (lcd_3d_vout_cfg->lcd_3d_brg_en & 0x1)
	      );
	wr_s3d_reg(adap, S3D_LCD_3D_VOUT_ADDR(S3D_LCD_3D_VOUT_CTRL_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
}

/**
 * Config s3d idcs
 */
static void config_s3d_idcs(struct __amba_sbrg_adapter *adap,
			sbrg_idc_3d_cfg_t *idc_3d_cfg)
{
	u16 reg;
	u16 iopad_sonof_data;
	reg = (((idc_3d_cfg->sbrg_wr_sel & 0x3) << 1) |
	       (idc_3d_cfg->sbrg_rd_sel & 0x1)
	      );
	/* set IDC iopad */
	rd_s3d_reg(adap, S3D_RCT_ADDR(S3D_IOPAD_SONOF_CTRL2_OFFSET),
		   &iopad_sonof_data,
		   NWORDS(iopad_sonof_data));

	if ((idc_3d_cfg->sbrg_wr_sel == SBRG_S3D_IDC_DISABLE_WRITE_SDA0) &&
	   (idc_3d_cfg->sbrg_rd_sel == SBRG_S3D_IDC_READ_SDA1) ) {
		iopad_sonof_data = (iopad_sonof_data | 0x200) & 0x2ff;
	}
	if ((idc_3d_cfg->sbrg_wr_sel == SBRG_S3D_IDC_DISABLE_WRITE_SDA1) &&
	   (idc_3d_cfg->sbrg_rd_sel == SBRG_S3D_IDC_READ_SDA0) ) {
		iopad_sonof_data = (iopad_sonof_data | 0x100) & 0x1ff;
	}
	if (idc_3d_cfg->sbrg_wr_sel == SBRG_S3D_IDC_WRITE_BOTH) {
		iopad_sonof_data = (iopad_sonof_data | 0x300) & 0x3ff;
	}
	if (idc_3d_cfg->sbrg_wr_sel == SBRG_S3D_IDC_DISABLE_WRITE_BOTH) {
		iopad_sonof_data = iopad_sonof_data & 0x0ff;
	}
	wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_IOPAD_SONOF_CTRL2_OFFSET),
		&iopad_sonof_data, NWORDS(iopad_sonof_data), 0);
	wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_IDCS_BRG_OFFSET), &reg, NWORDS(reg), 0);
}

/**
 * Config s3d spi
 */
static void config_s3d_spi(struct __amba_sbrg_adapter *adap,
			sbrg_spi_3d_cfg_t *spi_3d_cfg)
{
	u16 reg;

	/* set spi ctrl */
	reg = (((spi_3d_cfg->sbrg_rd_sel & 0x1) << 1) |
	       (spi_3d_cfg->sbrg_spi_en & 0x1)
	      );
	wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_SPI_CTRL_OFFSET), &reg, NWORDS(reg), 0);

	/* set spi program ctrl */
	reg = (spi_3d_cfg->sbrg_wr_sel & 0xf);
	wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_SPI_PROGRAM_CTRL_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
}

/**
 * Config s3d irq
 */
static void config_s3d_vic(struct __amba_sbrg_adapter *adap,
			sbrg_vic_3d_cfg_t *vic_3d_cfg)
{
#if 0	//IRQ
	u16 reg;
	int rval;
	int s3d_int_vec = GPIO_INT_VEC(vic_3d_cfg->s3d_irq);

	/* set vic interrupt */
	reg = vic_3d_cfg->idsp_vic_enable;

	if (reg != 0) {
		gpio_config(vic_3d_cfg->s3d_irq, GPIO_FUNC_SW_INPUT);
	//	vic_set_type(s3d_int_vec, VIRQ_LEVEL_HIGH);
		vic_set_type(s3d_int_vec, VIRQ_RISING_EDGE);
		free_irq(s3d_int_vec, vic_3d_cfg->hdl);
		rval = request_irq(s3d_int_vec,
				   IRQ_FLAG_FAST | IRQ_FLAG_SHARED,
				   vic_3d_cfg->hdl,
				   NULL);
		if (rval < 0) {
			DRV_PRINT("request_irq(%d, ...) error : %d",
			       s3d_int_vec, rval);
		}
		enable_irq(s3d_int_vec, vic_3d_cfg->hdl);
	} else {
		disable_irq(s3d_int_vec, vic_3d_cfg->hdl);
		free_irq(s3d_int_vec, vic_3d_cfg->hdl);
	}

	wr_s3d_reg(adap, S3D_VIC_ADDR(S3D_VIC_ENABLE_OFFSET), &reg, NWORDS(reg), 0);
#endif
}

/**
 * Reset the S3D
 */
static int config_s3d_reset(struct __amba_sbrg_adapter *adap, void *arg)
{
	u16 *sub_module_id = (u16 *)arg;
	u16 data;

	switch (*sub_module_id) {

	case S3D_RESET_ALL:
		data = 0x2000;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_L_VIN:
	case S3D_RESET_R_VIN:
	case S3D_RESET_BOTH_VIN:
		vin_s3d_set_vin_reset(adap, *sub_module_id);
		break;

	case S3D_RESET_L_IDSP:
		data = 0x3ffd;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_R_IDSP:
		data = 0x3fbf;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_BOTH_IDSP:
		data = 0x3fbd;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_L_MIPI:
		data = 0x3ff7;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3ff5;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_R_MIPI:
		data = 0x3eff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3ebf;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_BOTH_MIPI:
		data = 0x3ef7;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3eb5;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_VOUT:
		data = 0x2fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_L_PRESCALER:
		data = 0x37ff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_R_PRESCALER:
		data = 0x3bff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_BOTH_PRESCALER:
		data = 0x33ff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		data = 0x3fff;
		wr_s3d_reg(adap, S3D_BASE_ADDR(S3D_REG_SOFT_RESET_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	case S3D_RESET_PLL:
		/* reset PLL */
		rd_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
			   &data,
			   NWORDS(data));

		data = (data & 0xfbff);

		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
			   &data,
			   NWORDS(data),
			   0);

		data = (data | 0x0400);

		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
			   &data,
			   NWORDS(data),
			   0);
		break;

	default:
		return -1;
	}

	return 0;
}
/**
 * Enable S3D PLL(default)
 */
static int config_s3d_enable(struct __amba_sbrg_adapter *adap, void *arg)
{
	u16 reg = 0;
	rd_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
		   &reg,
		   NWORDS(reg));

	reg = reg & (~0x200);
	wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
	return 0;
}

/**
 * Disable S3D PLL
 */
static int config_s3d_disable(struct __amba_sbrg_adapter *adap, void *arg)
{
	u16 reg = 0;
	rd_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
		   &reg,
		   NWORDS(reg));

	reg = reg | 0x200;
	wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_PLL_CTRL1_OFFSET),
		   &reg,
		   NWORDS(reg),
		   0);
	return 0;
}



static int amba_sbrg_s3d_docmd(struct __amba_sbrg_adapter *adap,
	int cmd, void *args)
{
	int retval = 0;
	u16 sub_module_id = (u16)cmd;
	u16 reg;
	sbrg_reg_3d_cfg_t	*p_args;

	switch (sub_module_id) {
	case S3D_CONFIG_RCT_MODULE:
		/* config RCT */
		retval = config_s3d_rct(adap, (sbrg_rct_3d_cfg_t *)args);
		break;

	case S3D_MIPI_PHY_CTRL_L_MODULE:
		/* config LEFT MIPI PHY */
		config_s3d_mipi_phy(adap, S3D_LEFT,
				    (sbrg_rct_3d_mipi_phy_cfg_t *)args);
		break;

	case S3D_MIPI_PHY_CTRL_R_MODULE:
		/* config RIGHT MIPI PHY */
		config_s3d_mipi_phy(adap, S3D_RIGHT,
				    (sbrg_rct_3d_mipi_phy_cfg_t *)args);
		break;

	case S3D_MIPI_PHY_CTRL_BOTH_MODULE:
		/* config BOTH MIPI PHY */
		config_s3d_mipi_phy(adap, S3D_BOTH,
				    (sbrg_rct_3d_mipi_phy_cfg_t *)args);
		break;

	case S3D_VOUTF_SSTL_CTRL_MUDULE:
		/* config voutf sstl ctrl */
		reg = *((u16 *)args);

		wr_s3d_reg(adap, S3D_RCT_ADDR(S3D_VOUTF_SSTL_CTRL_OFFSET),
			   &reg,
			   NWORDS(u16),
			   0);
		break;

	case S3D_CONFIG_VIN_L_MODULE:
	case S3D_CONFIG_VIN_R_MODULE:
	case S3D_CONFIG_VIN_BOTH_MODULE:
		/* configure VIN unit minimum H-sync/V-sync width registers */
		config_s3d_vin_min_HV_sync_width(adap, sub_module_id,
						 (sbrg_vin_3d_cfg_t *)args);

		/* Set cropping window to capture image data of window */
		config_s3d_vin_set_video_capture_window(adap, sub_module_id,
						(sbrg_vin_3d_cfg_t *)args);

		/* config H-sync/V-sync pulse width register */
		config_s3d_vin_vh_width(adap, sub_module_id,
					(sbrg_vin_3d_cfg_t *)args);

		/* configure VIN unit S_V/S_H registers */
		config_s3d_vin_VH(adap, sub_module_id, (sbrg_vin_3d_cfg_t *)args);

		/* configure VIN unit horizontal offset register */
		config_s3d_vin_hoffset(adap, sub_module_id,
				       (sbrg_vin_3d_cfg_t *)args);

		/* Set horizontal blank interval length register */
		config_s3d_vin_set_hori_blank_length(adap, sub_module_id,
						(sbrg_vin_3d_cfg_t *)args);

		/* Set the timeout count for detecting Vsync/Hsync */
		config_s3d_vin_set_timout_vd_detection(adap, sub_module_id,
						(sbrg_vin_3d_cfg_t *)args);
		config_s3d_vin_set_timout_hd_detection(adap, sub_module_id,
						(sbrg_vin_3d_cfg_t *)args);

		/* config s_control and s_inputcfg registers */
		config_s3d_vin_info(adap, sub_module_id, (sbrg_vin_3d_cfg_t *)args);
		break;

	case S3D_CONFIG_IDSP_L_MODULE:
	case S3D_CONFIG_IDSP_R_MODULE:
	case S3D_CONFIG_IDSP_BOTH_MODULE:
		/* Set IDSPcmd register */
		config_s3d_vin_idspcmd(adap, sub_module_id,
				       (sbrg_vin_3d_idspcmd_cfg_t *)args);
		break;

	case S3D_CONFIG_VIN_SLVS_L_MODULE:
	case S3D_CONFIG_VIN_SLVS_R_MODULE:
	case S3D_CONFIG_VIN_SLVS_BOTH_MODULE:
		/* config slvs_control registers */
		config_s3d_vin_slvs_info(adap, sub_module_id,
					 (sbrg_vin_3d_slvs_cfg_t *)args);
		break;

	case S3D_CONFIG_SLVS_LANEMUX_L_MODULE:
	case S3D_CONFIG_SLVS_LANEMUX_R_MODULE:
	case S3D_CONFIG_SLVS_LANEMUX_BOTH_MODULE:
		/* set slvs lane mux select */
		config_s3d_vin_set_slvs_lane_mux_select(adap, sub_module_id,
							(u8 *)args);
		break;

	case S3D_CONFIG_VIN_MIPI_L_MODULE:
	case S3D_CONFIG_VIN_MIPI_R_MODULE:
	case S3D_CONFIG_VIN_MIPI_BOTH_MODULE:
		/* config mipi_control registers */
		config_s3d_vin_mipi_info(adap, sub_module_id,
					 (sbrg_vin_3d_mipi_cfg_t *)args);
		break;

	case S3D_CONFIG_VIN_TRIGGER0_L_MODULE:
	case S3D_CONFIG_VIN_TRIGGER0_R_MODULE:
	case S3D_CONFIG_VIN_TRIGGER0_BOTH_MODULE:
		config_s3d_vin_trigger_pin0(adap, sub_module_id,
					    (trigger_pin_info_t *)args);
		break;

	case S3D_CONFIG_VIN_TRIGGER1_L_MODULE:
	case S3D_CONFIG_VIN_TRIGGER1_R_MODULE:
	case S3D_CONFIG_VIN_TRIGGER1_BOTH_MODULE:
		config_s3d_vin_trigger_pin1(adap, sub_module_id,
					    (trigger_pin_info_t *)args);
		break;

	case S3D_CONFIG_UPDATE_L_MODULE:
	case S3D_CONFIG_UPDATE_R_MODULE:
	case S3D_CONFIG_UPDATE_BOTH_MODULE:
		/* update register */
		config_s3d_vin_done(adap, sub_module_id,
				    (sbrg_vin_3d_update_cfg_t *)args);
		break;
	case S3D_CONFIG_PRESCALER:
		/* build up the configuration of S3D prescaler */
		s3d_config_prescaler(adap, (sbrg_prescaler_obj_t *) args);
		break;

	case S3D_CONFIG_PRESCALER_MODULE:
		/* config s3d prescaler */
		config_s3d_prescaler(adap, (sbrg_prescaler_3d_cfg_t *) args);
		break;

	case S3D_CONFIG_L_PRESC_COEF_MODULE:
		/* config s3d precaler coefficients */
		config_s3d_presc_coef(adap, S3D_LEFT, (u16 *)args);
		break;

	case S3D_CONFIG_R_PRESC_COEF_MODULE:
		/* config s3d precaler coefficients */
		config_s3d_presc_coef(adap, S3D_RIGHT, (u16 *)args);
		break;

	case S3D_CONFIG_BOTH_PRESC_COEF_MODULE:
		/* config s3d precaler coefficients */
		config_s3d_presc_coef(adap, S3D_BOTH, (u16 *)args);
		break;

	case S3D_CONFIG_VOUTF_3D_MODULE:
		/* config s3d voutf */
		config_s3d_voutf(adap, (sbrg_voutf_3d_cfg_t *)args);
		break;

	case S3D_CONFIG_LCD_3D_VOUT_MODULE:
		/* config s3d lcd 3d vout */
		config_s3d_lcd_3d_vout(adap, (sbrg_lcd_3d_vout_cfg_t *)args);
		break;

	case S3D_CONFIG_IDCS_MODULE:
		/* config s3d idcs */
		config_s3d_idcs(adap, (sbrg_idc_3d_cfg_t *)args);
		break;

	case S3D_CONFIG_SPI_MODULE:
		/* config s3d spi */
		config_s3d_spi(adap, (sbrg_spi_3d_cfg_t *)args);
		break;

	case S3D_CONFIG_VIC_MODULE:
		config_s3d_vic(adap, (sbrg_vic_3d_cfg_t *)args);
		break;

	case S3D_SET_REG_DATA:
		p_args = (sbrg_reg_3d_cfg_t *)args;
		wr_s3d_reg(adap, p_args->addr, &p_args->data, 1, 0);
		break;
	case S3D_GET_REG_DATA:
		p_args = (sbrg_reg_3d_cfg_t *)args;
		rd_s3d_reg(adap, p_args->addr, &p_args->data, 1);
		break;

	case S3D_CONFIG_ENABLE:
		config_s3d_enable(adap, args);
		break;
	case S3D_CONFIG_DISABLE:
		config_s3d_disable(adap, args);
		break;
	case S3D_CONFIG_RESET:
		config_s3d_reset(adap, args);
		break;
	default:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		sbrg_err("%s-%d do not support cmd %d!\n",
			adap->name, adap->id, sub_module_id);

		break;
	}
	return retval;
}



static int sbrg_s3d_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int					errCode = 0;
	struct __amba_sbrg_adapter		*adap;
	struct amba_sbrg_s3d_dev_info		*pinfo;

	pinfo = kzalloc(sizeof (struct amba_sbrg_s3d_dev_info), GFP_KERNEL);
	if (!pinfo) {
		sbrg_err("Out of memory!\n");
		errCode = -ENOMEM;
		goto sbrg_s3d_errorCode_na;
	}
	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(*client));
	pinfo->client.addr = sbrg_s3d_addr;
	strlcpy(pinfo->client.name, sbrg_s3d_name, sizeof(pinfo->client.name));
	//Do something intialization

	adap = &pinfo->adap;
	adap->id = SBRG_S3D_ADAPTER_ID;		//Fix me
	adap->dev_type = 1; 	//Fix me
	adap->pinfo = pinfo;
	adap->docmd = amba_sbrg_s3d_docmd;
	errCode = amba_sbrg_add_adapter(adap);
	if (errCode) {
		sbrg_err("Adding Sbrg S3D adapter%d failed!\n", adap->id);
		goto sbrg_s3d_errorCode_kzalloc;
	}

	sbrg_notice("%s:%d probed!\n", adap->name, adap->id);
	goto sbrg_s3d_errorCode_na;

sbrg_s3d_errorCode_kzalloc:
	kfree(pinfo);

sbrg_s3d_errorCode_na:
	return errCode;
}


static int sbrg_s3d_remove(struct i2c_client *client)
{
	int				errCode = 0;
	struct amba_sbrg_s3d_dev_info	*pinfo;
	struct __amba_sbrg_adapter	*adap;

	adap = amba_sbrg_get_adapter(SBRG_S3D_ADAPTER_ID);
	pinfo = adap->pinfo;

	pinfo = (struct amba_sbrg_s3d_dev_info *)i2c_get_clientdata(client);

	if (pinfo) {

		i2c_set_clientdata(client, NULL);

		errCode = amba_sbrg_del_adapter(adap);
		kfree(pinfo);
	} else {
		sbrg_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	sbrg_notice("amba_sbrg_s3d_remove\n");

	return errCode;
}


static struct i2c_device_id sbrg_s3d_idtable[] = {
	{ "amb_vin1", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sbrg_s3d_idtable);

static struct i2c_driver i2c_driver_sbrg_s3d = {
	.driver = {
		.name	= "amb_vin1",
	},
	.id_table	= sbrg_s3d_idtable,
	.probe		= sbrg_s3d_probe,
	.remove		= sbrg_s3d_remove,
};

static int __init amba_sbrg_s3d_init(void)
{
	return i2c_add_driver(&i2c_driver_sbrg_s3d);
}

static void __exit amba_sbrg_s3d_exit(void)
{
	i2c_del_driver(&i2c_driver_sbrg_s3d);
}

module_init(amba_sbrg_s3d_init);
module_exit(amba_sbrg_s3d_exit);

MODULE_DESCRIPTION("Ambarella Sbrg S3D Driver");
MODULE_AUTHOR("Haowei Lo <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

