/*
 * kernel/private/drivers/ambarella/vin/arch_s2/vin_arch.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include "../vin_pri.h"

struct amba_vin_dev_info {
	struct __amba_vin_adapter	adap;

	struct __amba_vin_irq_info	vsync_irq;
	struct __amba_vin_irq_info	vin_irq;
	struct __amba_vin_irq_info	idsp_last_pixel_irq;
	struct __amba_vin_irq_info	idsp_irq;
	struct __amba_vin_irq_info	gpio_irq;
       struct __amba_vin_irq_info	idsp_sof_irq;

	struct amba_vin_cap_win_info	cap_info;
	struct amba_vin_irq_fix		irq_fix;
};
static struct amba_vin_dev_info *vin_dev_info[2];

static void amba_s2_second_vin_last_pixel_irq(void *callback_data, u32 counter)
{
	amba_writel(ambarella_phys_to_virt(0x70118000), 0x8000);
	amba_writel(ambarella_phys_to_virt(0x70107ca0), 0x1008);
}

static void amba_s2_vin_reset(struct __amba_vin_adapter *adap)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;
	pinfo->cap_info.s_ctrl_reg = 0x0001;
}

static int amba_s2_vin_config_info(struct __amba_vin_adapter *adap,
	struct amba_vin_adap_config *args)
{
	u16 reg;
	struct amba_vin_dev_info *pinfo;
	u32 val;

	pinfo = adap->pinfo;

	reg = (((args->hsync_mask & 0x1) << 15) |
	       ((args->ecc_enable & 0x1) << 14) |
	       ((args->sony_field_mode & 0x1) << 13) |
	       ((args->field0_pol & 0x1) << 12) |
	       ((args->hs_polarity & 0x1) << 11) |
	       ((args->vs_polarity & 0x1) << 10) |
	       ((args->emb_sync_loc & 0x3) << 8) |
	       ((args->emb_sync_mode & 0x1) << 7) |
	       ((args->emb_sync & 0x1) << 6) |
	       ((args->sync_mode & 0x3) << 4) |
           ((args->data_edge & 0x1) << 3)
	    );
	pinfo->cap_info.s_ctrl_reg = reg;

	reg = (((args->clk_select_slvs & 0x1) << 13) |
	       ((args->sony_slvs_mode & 0x1) << 12) |
	       ((args->serial_mode & 0x1) << 11) |
	       ((args->mipi_act_lanes & 0x3) << 9) |
	       ((args->src_data_width & 0x3) << 5) |
	       (args->input_mode & 0x19f)
	    );
	pinfo->cap_info.s_inp_cfg_reg = reg;

	pinfo->cap_info.s_timeout_v_high_reg	= 0xffff;
	pinfo->cap_info.s_timeout_v_low_reg	= args->slvs_eav_col;
	pinfo->cap_info.s_timeout_h_high_reg	= 0xffff;
	pinfo->cap_info.s_timeout_h_low_reg	= args->slvs_sav2sav_dist;

	reg = ((args->mipi_decompression_mode << 14) |
	       (args->mipi_s_clksettlectl << 12) |
	       (args->mipi_s_dpdn_swap_data << 11) |
	       (args->mipi_s_dpdn_swap_clk << 10) |
	       (args->mipi_s_hssettlectl << 5) |
	       (args->mipi_enable << 4) |
	       (args->mipi_virtualch_mask << 2) |
	       (args->mipi_virtualch_select)
	    );
	pinfo->cap_info.mipi_cfg1_reg = reg;

	reg = ((args->mipi_decompression_enable << 15) |
	       (args->mipi_pixel_byte_swap << 14) |
	       (args->mipi_data_type_mask << 8) |
	       (args->mipi_data_type_sel)
	    );
	pinfo->cap_info.mipi_cfg2_reg = reg;

	pinfo->cap_info.mipi_bdphyctl_reg = args->mipi_b_dphyctl;
	pinfo->cap_info.mipi_sdphyctl_reg = args->mipi_s_dphyctl;

	pinfo->cap_info.slvs_control = args->slvs_control;
	pinfo->cap_info.slvs_frame_line_w = args->slvs_frame_line_w;
	pinfo->cap_info.slvs_act_frame_line_w = args->slvs_act_frame_line_w;
	pinfo->cap_info.slvs_lane_mux_select[0] = args->slvs_lane_mux_select[0];
	pinfo->cap_info.slvs_lane_mux_select[1] = args->slvs_lane_mux_select[1];
	pinfo->cap_info.slvs_lane_mux_select[2] = args->slvs_lane_mux_select[2];
	pinfo->cap_info.slvs_debug = args->slvs_debug;

	pinfo->cap_info.enhance_mode= args->enhance_mode;
	pinfo->cap_info.syncmap_mode= args->syncmap_mode;
	pinfo->cap_info.s_slvs_ctrl_1 = args->s_slvs_ctrl_1;
	pinfo->cap_info.s_slvs_sav_vzero_map = args->s_slvs_sav_vzero_map;
	pinfo->cap_info.s_slvs_sav_vone_map = args->s_slvs_sav_vone_map;
	pinfo->cap_info.s_slvs_eav_vzero_map = args->s_slvs_eav_vzero_map;
	pinfo->cap_info.s_slvs_eav_vone_map = args->s_slvs_eav_vone_map;

	if (args->input_mode & 0x1) {
		if (args->serial_mode == 0) {
			rct_set_vin_lvds_pad(VIN_LVDS_PAD_MODE_LVDS);
		} else {
			rct_set_vin_lvds_pad(VIN_LVDS_PAD_MODE_SLVS);
		}
	} else {
		if (args->serial_mode == 0) {
			if (args->mipi_enable == 1) {
				rct_set_vin_lvds_pad(VIN_LVDS_PAD_MODE_LVDS);
			} else {
				rct_set_vin_lvds_pad(VIN_LVDS_PAD_MODE_LVCMOS);
			}
		} else {
			rct_set_vin_lvds_pad(VIN_LVDS_PAD_MODE_SLVS);
		}
	}

	/* FIX ME */
	amba_writel(RCT_REG(0x260), 0x10001);// set 2nd VIN to lvcoms mode to save the power
	if (args->serial_mode || args->mipi_enable) {
		/* increase resistor termination to make lvds signal be stable */
		val = amba_readl(RCT_REG(0x0fc));
		val &= ~0xF0000000;
		val |= 0xF0000000;
		amba_writel(RCT_REG(0x0fc), val);
	}

	return 0;
}

static int amba_s2_vin_set_video_capture_window(
	struct __amba_vin_adapter *adap, struct amba_vin_cap_window *args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	pinfo->cap_info.s_cap_start_v_reg = args->start_y;
	pinfo->cap_info.s_cap_start_h_reg = args->start_x;
	pinfo->cap_info.s_cap_end_v_reg = args->end_y;
	pinfo->cap_info.s_cap_end_h_reg = args->end_x;

	return 0;
}

static int amba_s2_vin_config_min_HV_sync_width(
	struct __amba_vin_adapter *adap, struct amba_vin_min_HV_sync *args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	pinfo->cap_info.s_min_h_reg = args->hs_min - 1;
	pinfo->cap_info.s_min_v_reg = args->vs_min - 1;

	return 0;
}

static int amba_s2_vin_enable_video_capture_window(
	struct __amba_vin_adapter *adap, u32 args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	if (args == AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE) {
		pinfo->cap_info.s_ctrl_reg |= 0x0006;
	} else {
		pinfo->cap_info.s_ctrl_reg &= ~(0x0006);
	}

	return 0;
}

static void amba_s2_vin_set_vin_clk(
	struct __amba_vin_adapter *adap, struct amba_vin_clk_info *args)
{
	rct_set_so_clk_src(args->mode);
	rct_set_so_freq_hz(args->so_freq_hz);
}

static void amba_s2_vin_set_H_offset(
	struct __amba_vin_adapter *adap, struct amba_vin_H_offset *args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	pinfo->cap_info.s_h_offset_top_reg = args->top_vsync_offset;
	pinfo->cap_info.s_h_offset_bot_reg = args->bottom_vsync_offset;
}

static void amba_s2_vin_set_HV(
	struct __amba_vin_adapter *adap, struct amba_vin_HV *args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	pinfo->cap_info.s_v_reg = (args->line_num_a_field - 1);
	pinfo->cap_info.s_h_reg = (args->pel_clk_a_line - 1);
}

static void amba_s2_vin_set_HV_width(
	struct __amba_vin_adapter *adap, struct amba_vin_HV_width *args)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	pinfo->cap_info.s_v_width_reg = (args->vwidth - 1);
	pinfo->cap_info.s_h_width_reg = (args->hwidth - 1);
	pinfo->cap_info.s_slvs_vsync_horizontal_start = (args->hstart - 1);
	pinfo->cap_info.s_slvs_vsync_horizontal_end = (args->hend - 1);
}

static void amba_s2_vin_set_vout_sync_start_line(
	struct __amba_vin_adapter *adap, u32 start_line)
{
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;
	pinfo->cap_info.s_vout_start_0_reg = start_line;
	pinfo->cap_info.s_vout_start_1_reg = start_line;
}

static void amba_s2_vin_set_trigger0_pin_info(
	struct __amba_vin_adapter *adap, struct amba_vin_trigger_pin_info *args)
{
	u16 reg;
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	reg = (((args->enabled & 0x1) << 15) |
		((args->polarity & 0x1) << 14) |
		(args->start_line & 0x3fff));

	pinfo->cap_info.s_trigger_0_start_reg = reg;
	pinfo->cap_info.s_trigger_0_end_reg = args->last_line;
}

static void amba_s2_vin_set_trigger1_pin_info(
	struct __amba_vin_adapter *adap, struct amba_vin_trigger_pin_info *args)
{
	u16 reg;
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	reg = (((args->enabled & 0x1) << 15) |
		((args->polarity & 0x1) << 14) |
		(args->start_line & 0x3fff));
	pinfo->cap_info.s_trigger_1_start_reg = reg;
	pinfo->cap_info.s_trigger_1_end_reg = args->last_line;
}

static void amba_s2_vin_set_mipi_phy_enable(
	struct __amba_vin_adapter *adap, u8 lanes)
{
	switch(lanes) {
	case MIPI_1LANE:
		amba_writel(DSP_VIN_DEBUG_REG(0x8030), 0x948);
		break;
	case MIPI_2LANE:
		amba_writel(DSP_VIN_DEBUG_REG(0x8030), 0x1948);
		break;
	case MIPI_3LANE:
		amba_writel(DSP_VIN_DEBUG_REG(0x8030), 0x3948);
		break;
	case MIPI_4LANE:
		amba_writel(DSP_VIN_DEBUG_REG(0x8030), 0x7948);
		break;
	default:
		vin_err("S2 VIN can only support 1~4 lanes MIPI\n");
		break;
	}

	/* choose section 1 */
	amba_writel(DSP_VIN_DEBUG_REG(0x8000), 0x1000);

	mdelay(1);
	amba_writel(DSP_VIN_DEBUG_REG(0x04), lanes << 9);
	/* default mipi phy setting */
	amba_writel(DSP_VIN_DEBUG_REG(0x68), 0x230);
	amba_writel(DSP_VIN_DEBUG_REG(0x6C), 0x2d);
	amba_writel(DSP_VIN_DEBUG_REG(0x70), 0x95a8);
	amba_writel(DSP_VIN_DEBUG_REG(0x74), 0x796);
	mdelay(1);
}

static void amba_s2_vin_config_mipi_reset(
	struct __amba_vin_adapter *adap)
{
	int val;

	do { /* wait for mipi phy to lock the clock */
		val = amba_readl(DSP_VIN_DEBUG_REG(0x818));
	} while ((val&0x01) != 0x01);

	/* choose section 1 */
	amba_writel(DSP_VIN_DEBUG_REG(0x8000), 0x1000);

	/* reset section main VIN and mipi digital phy */
	amba_writel(DSP_VIN_DEBUG_REG(0x801c), 0x10001);
	mdelay(10);
	amba_writel(DSP_VIN_DEBUG_REG(0x801c), 0x0);
}


static int amba_s2_vin_docmd(struct __amba_vin_adapter *adap,
	enum amba_vin_adap_cmd cmd, void *args)
{
	int retval = 0;
	struct amba_vin_dev_info *pinfo;

	pinfo = adap->pinfo;

	switch (cmd) {
	case AMBA_VIN_ADAP_IDLE:
		break;

	case AMBA_VIN_ADAP_SUSPEND:
		amba_vin_vsync_unbind(adap);
		disable_irq(VIN_IRQ);
		disable_irq(IDSP_LAST_PIXEL_IRQ);
		disable_irq(IDSP_SENSOR_VSYNC_IRQ);
		break;

	case AMBA_VIN_ADAP_RESUME:
		enable_irq(VIN_IRQ);
		enable_irq(IDSP_LAST_PIXEL_IRQ);
		enable_irq(IDSP_SENSOR_VSYNC_IRQ);
		amba_vin_vsync_bind(adap, &pinfo->irq_fix);
		break;

	case AMBA_VIN_ADAP_SET_SHADOW_REG_PTR:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_ADAP_INIT:
		amba_s2_vin_reset(adap);
		break;

	case AMBA_VIN_ADAP_GET_INFO:
		{
			struct amba_vin_adapter *pub_adap;

			pub_adap = (struct amba_vin_adapter *) args;
			pub_adap->id = adap->id;
			pub_adap->dev_type = adap->dev_type;
			strlcpy(pub_adap->name, adap->name,
				sizeof (pub_adap->name));
		}
		break;

	case AMBA_VIN_ADAP_REGISTER_SOURCE:
		adap->total_source_num++;
		break;
	case AMBA_VIN_ADAP_UNREGISTER_SOURCE:
		if (adap->total_source_num)
			adap->total_source_num--;
		else
			retval = -EINVAL;
		break;
	case AMBA_VIN_ADAP_GET_SOURCE_NUM:
		*(int *) args = adap->total_source_num;
		break;
	case AMBA_VIN_ADAP_FIX_ARCH_VSYNC:
		{
			/* FIXME: S2 uses idsp soft intterrupt to bind to soft vsync */
			struct amba_vin_irq_fix *pirq_fix;

			pirq_fix = (struct amba_vin_irq_fix *)args;
			pirq_fix->mode = AMBA_VIN_VSYNC_IRQ_IDSP_SOF;
			pirq_fix->delay = 0;

			amba_vin_vsync_unbind(adap);
			memcpy(&pinfo->irq_fix, args, sizeof(pinfo->irq_fix));
			amba_vin_vsync_bind(adap, &pinfo->irq_fix);
		}
		break;

	case AMBA_VIN_ADAP_GET_ACTIVE_SOURCE_ID:
		*(int *) args = adap->active_source_id;
		break;
	case AMBA_VIN_ADAP_SET_ACTIVE_SOURCE_ID:
		adap->active_source_id = *(int *) args;
		break;

	case AMBA_VIN_ADAP_GET_CONFIG:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_CONFIG:
		retval = amba_s2_vin_config_info(adap,
			(struct amba_vin_adap_config *) args);
		break;

	case AMBA_VIN_ADAP_GET_CAPTURE_WINDOW:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_CAPTURE_WINDOW:
		retval = amba_s2_vin_set_video_capture_window(adap,
			(struct amba_vin_cap_window *) args);
		break;

	case AMBA_VIN_ADAP_GET_MIN_HW_SYNC_WIDTH:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH:
		retval = amba_s2_vin_config_min_HV_sync_width(adap,
			(struct amba_vin_min_HV_sync *) args);
		break;

	case AMBA_VIN_ADAP_GET_VIDEO_CAPTURE_WINDOW:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW:
		retval = amba_s2_vin_enable_video_capture_window(
			adap, *(u32 *)args);
		break;

	case AMBA_VIN_ADAP_GET_SW_BLC:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_SW_BLC:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_ADAP_GET_VIN_CLOCK:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_VIN_CLOCK:
		amba_s2_vin_set_vin_clk(adap,
			(struct amba_vin_clk_info *)args);
		break;

	case AMBA_VIN_ADAP_GET_VOUT_SYNC_START_LINE:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE:
		amba_s2_vin_set_vout_sync_start_line(adap, *(u32 *)args);
		break;

	case AMBA_VIN_ADAP_GET_H_OFFSET:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_H_OFFSET:
		amba_s2_vin_set_H_offset(adap,
			(struct amba_vin_H_offset *)args);
		break;

	case AMBA_VIN_ADAP_GET_HV:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_HV:
		amba_s2_vin_set_HV(adap, (struct amba_vin_HV *) args);
		break;

	case AMBA_VIN_ADAP_GET_HV_WIDTH:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_HV_WIDTH:
		amba_s2_vin_set_HV_width(adap, (struct amba_vin_HV_width *) args);
		break;

	case AMBA_VIN_ADAP_GET_TRIGGER0_PIN_INFO:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_TRIGGER0_PIN_INFO:
		amba_s2_vin_set_trigger0_pin_info(adap,
			(struct amba_vin_trigger_pin_info *)args);
		break;

	case AMBA_VIN_ADAP_GET_TRIGGER1_PIN_INFO:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_TRIGGER1_PIN_INFO:
		amba_s2_vin_set_trigger1_pin_info(adap,
			(struct amba_vin_trigger_pin_info *)args);
		break;

	case AMBA_VIN_ADAP_GET_BLC_INFO:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_BLC_INFO:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_ADAP_GET_HW_BLC:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_HW_BLC:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_ADAP_SET_MIPI_PHY_ENABLE:
		amba_s2_vin_set_mipi_phy_enable(adap, *(u8 *)args);
		break;
	case AMBA_VIN_ADAP_CONFIG_MIPI_RESET:
		amba_s2_vin_config_mipi_reset(adap);
		break;

	case AMBA_VIN_ADAP_GET_VIN_CAP_INFO:
		vin_dbg("s_ctrl_reg = 0x%x\n", pinfo->cap_info.s_ctrl_reg);
		vin_dbg("s_inp_cfg_reg = 0x%x\n", pinfo->cap_info.s_inp_cfg_reg);
		vin_dbg("s_v_width_reg = 0x%x\n", pinfo->cap_info.s_v_width_reg);
		vin_dbg("s_h_width_reg = 0x%x\n", pinfo->cap_info.s_h_width_reg);
		vin_dbg("s_h_offset_top_reg = 0x%x\n", pinfo->cap_info.s_h_offset_top_reg);
		vin_dbg("s_h_offset_bot_reg = 0x%x\n", pinfo->cap_info.s_h_offset_bot_reg);
		vin_dbg("s_v_reg = 0x%x\n", pinfo->cap_info.s_v_reg);
		vin_dbg("s_h_reg = 0x%x\n", pinfo->cap_info.s_h_reg);
		vin_dbg("s_min_v_reg = 0x%x\n", pinfo->cap_info.s_min_v_reg);
		vin_dbg("s_min_h_reg = 0x%x\n", pinfo->cap_info.s_min_h_reg);
		vin_dbg("s_trigger_0_start_reg = 0x%x\n", pinfo->cap_info.s_trigger_0_start_reg);
		vin_dbg("s_trigger_0_end_reg = 0x%x\n", pinfo->cap_info.s_trigger_0_end_reg);
		vin_dbg("s_trigger_1_start_reg = 0x%x\n", pinfo->cap_info.s_trigger_1_start_reg);
		vin_dbg("s_trigger_1_end_reg = 0x%x\n", pinfo->cap_info.s_trigger_1_end_reg);
		vin_dbg("s_vout_start_0_reg = 0x%x\n", pinfo->cap_info.s_vout_start_0_reg);
		vin_dbg("s_vout_start_1_reg = 0x%x\n", pinfo->cap_info.s_vout_start_1_reg);
		vin_dbg("s_cap_start_v_reg = 0x%x\n", pinfo->cap_info.s_cap_start_v_reg);
		vin_dbg("s_cap_start_h_reg = 0x%x\n", pinfo->cap_info.s_cap_start_h_reg);
		vin_dbg("s_cap_end_v_reg = 0x%x\n", pinfo->cap_info.s_cap_end_v_reg);
		vin_dbg("s_cap_end_h_reg = 0x%x\n", pinfo->cap_info.s_cap_end_h_reg);
		vin_dbg("s_blank_leng_h_reg = 0x%x\n", pinfo->cap_info.s_blank_leng_h_reg);

		memcpy(args, &(pinfo->cap_info), sizeof (pinfo->cap_info));
		break;
	case AMBA_VIN_ADAP_SET_VIN_CAP_INFO:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	default:
		retval = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vin_err("%s-%d do not support cmd %d!\n",
			adap->name, adap->id, cmd);

		break;
	}

	return retval;
}

static int __init amba_s2_vin_init(void)
{
	int					retval = 0;
	struct __amba_vin_adapter		*adap;

	vin_dbg("amba_s2_vin_probe\n");

	vin_dev_info[0] = kzalloc(sizeof(struct amba_vin_dev_info), GFP_KERNEL);
	if (vin_dev_info[0] == NULL) {
		vin_err("Out of memory!\n");
		retval = -ENOMEM;
		goto vin_errorCode_na;
	}

	vin_dev_info[1] = kzalloc(sizeof(struct amba_vin_dev_info), GFP_KERNEL);
	if (vin_dev_info[1] == NULL) {
		vin_err("Out of memory!\n");
		retval = -ENOMEM;
		goto vin_errorCode_na;
	}

	adap = &vin_dev_info[0]->adap;
	adap->id = AMBA_VIN_ADAPTER_STARTING_ID;
	adap->dev_type = (AMBA_VIN_SRC_DEV_TYPE_CMOS |
		AMBA_VIN_SRC_DEV_TYPE_CCD | AMBA_VIN_SRC_DEV_TYPE_DECODER);
	adap->pinfo = vin_dev_info[0];
	adap->docmd = amba_s2_vin_docmd;
	retval = amba_vin_add_adapter(adap);
	if (retval) {
		vin_err("Adding VIN adapter%d failed!\n", adap->id);
		goto vin_errorCode_kzalloc;
	}

	retval = amba_vin_irq_add(&vin_dev_info[0]->adap, -1, 0,
		AMBA_VIN_VSYNC_IRQ_VSYNC, &vin_dev_info[0]->vsync_irq, 1);
	if (retval)
		goto vin_errorCode_kzalloc;


        //VIN_IRQ (VIC1.2  or GIC.2) Edge
	retval = amba_vin_irq_add(&vin_dev_info[0]->adap,
		VIN_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_VIN, &vin_dev_info[0]->vin_irq, 0);
	if (retval)
		goto vin_errorCode_free_vsync_irq;


        //Last pixel (vic 2.5) Edge
	retval = amba_vin_irq_add(&vin_dev_info[0]->adap,
		IDSP_LAST_PIXEL_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_IDSP_LAST_PIXEL,
		&vin_dev_info[0]->idsp_last_pixel_irq, 0);
	if (retval)
		goto vin_errorCode_free_vin_irq;

        //sensor vsync (vic 2.7) Edge
	retval = amba_vin_irq_add(&vin_dev_info[0]->adap,
		IDSP_SENSOR_VSYNC_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_IDSP, &vin_dev_info[0]->idsp_irq, 0);
	if (retval)
		goto vin_errorCode_free_idsp_last_pixel_irq;

        //vin sw (vic 2.3)  Edge
	retval = amba_vin_irq_add(&vin_dev_info[0]->adap,
		IDSP_VIN_SOFT_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_IDSP_SOF, &vin_dev_info[0]->idsp_sof_irq, 0);
	if (retval)
		goto vin_errorCode_free_idsp_irq;

	if (ambarella_is_valid_gpio_irq(&ambarella_board_generic.vin0_vsync)) {
		ambarella_gpio_config(
			ambarella_board_generic.vin0_vsync.irq_gpio,
			ambarella_board_generic.vin0_vsync.irq_gpio_mode);
		retval = amba_vin_irq_add(&vin_dev_info[0]->adap,
			ambarella_board_generic.vin0_vsync.irq_line,
			ambarella_board_generic.vin0_vsync.irq_type,
			AMBA_VIN_VSYNC_IRQ_GPIO | IRQF_SHARED, &vin_dev_info[0]->gpio_irq, 0);
		if (retval)
			goto vin_errorCode_free_idsp_sof_irq;
		adap->pgpio_irq = &vin_dev_info[0]->gpio_irq;
	}

	adap->pvsync_irq = &vin_dev_info[0]->vsync_irq;
	adap->pvin_irq = &vin_dev_info[0]->vin_irq;
	adap->pidsp_last_pixel_irq = &vin_dev_info[0]->idsp_last_pixel_irq;
	adap->pidsp_irq = &vin_dev_info[0]->idsp_irq;
	adap->pidsp_sof_irq = &vin_dev_info[0]->idsp_sof_irq;
	vin_dev_info[0]->irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP_SOF;
	vin_dev_info[0]->irq_fix.delay = 0;
	retval = amba_vin_vsync_bind(adap, &vin_dev_info[0]->irq_fix);
	if (retval)
		goto vin_errorCode_free_gpio_irq;

	vin_notice("%s:%d probed!\n", adap->name, adap->id);

	adap = &vin_dev_info[1]->adap;
	adap->id = AMBA_VIN_ADAPTER_STARTING_ID + 1;
	adap->dev_type = (AMBA_VIN_SRC_DEV_TYPE_CMOS |
		AMBA_VIN_SRC_DEV_TYPE_CCD | AMBA_VIN_SRC_DEV_TYPE_DECODER);
	adap->pinfo = vin_dev_info[1];
	adap->docmd = amba_s2_vin_docmd;
	retval = amba_vin_add_adapter(adap);
	if (retval) {
		vin_err("Adding VIN adapter%d failed!\n", adap->id);
		goto vin_errorCode_kzalloc;
	}

	retval = amba_vin_irq_add(&vin_dev_info[1]->adap, -1, 0,
		AMBA_VIN_VSYNC_IRQ_VSYNC, &vin_dev_info[1]->vsync_irq, 0);
	if (retval)
		goto vin_errorCode_kzalloc;

	retval = amba_vin_irq_add(&vin_dev_info[1]->adap,
		VDSP_PIP_SVSYNC_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_VIN, &vin_dev_info[1]->vsync_irq, 0);
	if (retval)
		goto vin_errorCode_kzalloc;

	vin_dev_info[1]->idsp_last_pixel_irq.callback = amba_s2_second_vin_last_pixel_irq;
	retval = amba_vin_irq_add(&vin_dev_info[1]->adap,
		IDSP_PIP_LAST_PIXEL_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_IDSP_LAST_PIXEL,
		&vin_dev_info[1]->idsp_last_pixel_irq, 0);
	if (retval)
		goto vin_errorCode_free_vsync_irq;

	retval = amba_vin_irq_add(&vin_dev_info[1]->adap,
		IDSP_PIP_SVSYNC_IRQ, IRQF_TRIGGER_RISING | IRQF_SHARED,
		AMBA_VIN_VSYNC_IRQ_IDSP, &vin_dev_info[1]->idsp_irq, 0);
	if (retval)
		goto vin_errorCode_free_idsp_last_pixel_irq;

	if (ambarella_is_valid_gpio_irq(&ambarella_board_generic.vin1_vsync)) {
		ambarella_gpio_config(
			ambarella_board_generic.vin1_vsync.irq_gpio,
			ambarella_board_generic.vin1_vsync.irq_gpio_mode);
		retval = amba_vin_irq_add(&vin_dev_info[1]->adap,
			ambarella_board_generic.vin1_vsync.irq_line,
			ambarella_board_generic.vin1_vsync.irq_type,
			AMBA_VIN_VSYNC_IRQ_GPIO | IRQF_SHARED,
			&vin_dev_info[1]->gpio_irq, 0);
		if (retval)
			goto vin_errorCode_free_idsp_irq;
		adap->pgpio_irq = &vin_dev_info[1]->gpio_irq;
	}

	adap->pvsync_irq = &vin_dev_info[1]->vsync_irq;
	adap->pvin_irq = &vin_dev_info[1]->vin_irq;
	adap->pidsp_last_pixel_irq = &vin_dev_info[1]->idsp_last_pixel_irq;
	adap->pidsp_irq = &vin_dev_info[1]->idsp_irq;
	vin_dev_info[1]->irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_dev_info[1]->irq_fix.delay = 0;
	retval = amba_vin_vsync_bind(adap, &vin_dev_info[1]->irq_fix);
	if (retval)
		goto vin_errorCode_free_gpio_irq;

	init_waitqueue_head(&vsync_irq_wait);

	vin_notice("%s:%d probed!\n", adap->name, adap->id);
	goto vin_errorCode_na;

vin_errorCode_free_gpio_irq:
	if (ambarella_is_valid_gpio_irq(&ambarella_board_generic.vin0_vsync))
		amba_vin_irq_remove(&vin_dev_info[0]->gpio_irq);
	if (ambarella_is_valid_gpio_irq(&ambarella_board_generic.vin1_vsync))
		amba_vin_irq_remove(&vin_dev_info[1]->gpio_irq);

vin_errorCode_free_idsp_sof_irq:
	amba_vin_irq_remove(&vin_dev_info[0]->idsp_sof_irq);

vin_errorCode_free_idsp_irq:
	amba_vin_irq_remove(&vin_dev_info[0]->idsp_irq);
	amba_vin_irq_remove(&vin_dev_info[1]->idsp_irq);

vin_errorCode_free_idsp_last_pixel_irq:
	amba_vin_irq_remove(&vin_dev_info[0]->idsp_last_pixel_irq);
	amba_vin_irq_remove(&vin_dev_info[1]->idsp_last_pixel_irq);

vin_errorCode_free_vin_irq:
	amba_vin_irq_remove(&vin_dev_info[0]->vin_irq);
	amba_vin_irq_remove(&vin_dev_info[1]->vin_irq);

vin_errorCode_free_vsync_irq:
	amba_vin_irq_remove(&vin_dev_info[0]->vsync_irq);
	amba_vin_irq_remove(&vin_dev_info[1]->vsync_irq);

vin_errorCode_kzalloc:
	if (vin_dev_info[0]) {
		kfree(vin_dev_info[0]);
		vin_dev_info[0] = NULL;
	}
	if (vin_dev_info[1]) {
		kfree(vin_dev_info[1]);
		vin_dev_info[1] = NULL;
	}

vin_errorCode_na:
	return retval;
}

static void __exit amba_s2_vin_exit(void)
{
	if (vin_dev_info[0]) {
		amba_vin_vsync_unbind(&vin_dev_info[0]->adap);
		amba_vin_irq_remove(&vin_dev_info[0]->vsync_irq);
		amba_vin_irq_remove(&vin_dev_info[0]->vin_irq);
		amba_vin_irq_remove(&vin_dev_info[0]->idsp_last_pixel_irq);
		amba_vin_irq_remove(&vin_dev_info[0]->idsp_irq);
		amba_vin_irq_remove(&vin_dev_info[0]->idsp_sof_irq);
		if (ambarella_is_valid_gpio_irq(
			&ambarella_board_generic.vin0_vsync)) {
			amba_vin_irq_remove(&vin_dev_info[0]->gpio_irq);
		}
		amba_vin_del_adapter(&vin_dev_info[0]->adap);
		kfree(vin_dev_info[0]);
		vin_dev_info[0] = NULL;
	}

	if (vin_dev_info[1]) {
		amba_vin_vsync_unbind(&vin_dev_info[1]->adap);
		amba_vin_irq_remove(&vin_dev_info[1]->vsync_irq);
		amba_vin_irq_remove(&vin_dev_info[1]->vin_irq);
		amba_vin_irq_remove(&vin_dev_info[1]->idsp_last_pixel_irq);
		amba_vin_irq_remove(&vin_dev_info[1]->idsp_irq);
		if (ambarella_is_valid_gpio_irq(
			&ambarella_board_generic.vin1_vsync)) {
			amba_vin_irq_remove(&vin_dev_info[1]->gpio_irq);
		}
		amba_vin_del_adapter(&vin_dev_info[1]->adap);
		kfree(vin_dev_info[1]);
		vin_dev_info[1] = NULL;
	}

	vin_notice("amba_s2_vin_remove\n");
}

module_init(amba_s2_vin_init);
module_exit(amba_s2_vin_exit);

MODULE_DESCRIPTION("Ambarella S2 VIN Driver");
MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

