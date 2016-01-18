/*
 * Filename : ov4689_arch.c
 *
 * History:
 *    2013/05/29 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static int ov4689_init_vin_clock(struct __amba_vin_source *src, const struct ov4689_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int ov4689_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int ov4689_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct ov4689_info *pinfo;
	struct amba_vin_adap_config ov4689_config;
	u32 index;
	u16 phase;
	u32 width;
	u32 height;
	u32 nstartx;
	u32 nstarty;
	u32 nendx;
	u32 nendy;
	struct amba_vin_cap_window window_info;
	struct amba_vin_min_HV_sync min_HV_sync;
	u32 adap_arg;

	pinfo = (struct ov4689_info *) src->pinfo;
	index = pinfo->current_video_index;

	memset(&ov4689_config, 0, sizeof (ov4689_config));

	ov4689_config.hsync_mask = HSYNC_MASK_CLR;
	ov4689_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	ov4689_config.field0_pol = VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	ov4689_config.hs_polarity = (phase & 0x1);
	ov4689_config.vs_polarity = (phase & 0x2) >> 1;

	ov4689_config.ecc_enable = 1;

	ov4689_config.emb_sync_loc = EMB_SYNC_LOWER_PEL;
	ov4689_config.emb_sync_mode   = EMB_SYNC_ITU_656;
	ov4689_config.emb_sync = EMB_SYNC_OFF;

	ov4689_config.sync_mode = SYNC_MODE_SLAVE;

	ov4689_config.data_edge = PCLK_RISING_EDGE;
	ov4689_config.src_data_width = SRC_DATA_10B;
	ov4689_config.input_mode = VIN_RGB_LVDS_2PELS_DDR_MIPI;

	ov4689_config.serial_mode = SERIAL_VIN_MODE_DISABLE;
	ov4689_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;

	ov4689_config.clk_select_slvs = CLK_SELECT_MIPICLK;
	ov4689_config.slvs_eav_col = VIN_DONT_CARE;
	ov4689_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	ov4689_config.mipi_enable = MIPI_VIN_MODE_ENABLE;
	ov4689_config.mipi_data_type_sel = RAW_10BIT;

	if(lane == 4) { // 4 lane
		ov4689_config.mipi_act_lanes = MIPI_4LANE;
		ov4689_config.mipi_s_hssettlectl = 41;
		ov4689_config.mipi_b_dphyctl = 0x8D47;
		ov4689_config.mipi_s_dphyctl = 0x619;
	} else { // 2 lane
		ov4689_config.mipi_act_lanes = MIPI_2LANE;
		ov4689_config.mipi_s_hssettlectl = 34;
		ov4689_config.mipi_b_dphyctl = 0x8D47;
		ov4689_config.mipi_s_dphyctl = 0x619;
	}

	ov4689_config.enhance_mode = 1;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &ov4689_config);

	nstartx = pinfo->cap_start_x;
	nstarty = pinfo->cap_start_y;
	width = pinfo->cap_cap_w;
	/*
	 * for interlace video, height is half of cap_h; for some
	 * source is lines is less than it delcares, so we should
	 * adjust the height
	 */
	height = pinfo->cap_cap_h;

	nendx = (width + nstartx);
	nendy = (height + nstarty);

	window_info.start_x = nstartx;
	window_info.start_y = nstarty;
	window_info.end_x = nendx - 1;
	window_info.end_y = nendy - 1;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CAPTURE_WINDOW, &window_info);

	min_HV_sync.hs_min = width - 1;
	min_HV_sync.vs_min = height - 1;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = ov4689_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int ov4689_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}
/* ----- MIPI API ---------*/

static void ov4689_vin_reset_idsp (struct __amba_vin_source *src)
{
	/* reset all sections */
	amba_writel(DSP_VIN_DEBUG_REG(0x801C), 0xFF);
	amba_writel(DSP_VIN_DEBUG_REG(0x8030), 0x100);

	/* hard code gclk_so_vin to 192MHz, VIN will use gclk_so_vin to caputre data */
	amba_writel(RCT_REG(0x24), 0x17151009);
	amba_writel(RCT_REG(0x24), 0x17151008);
	amba_writel(RCT_REG(0x30), 0x8);
}

static void ov4689_vin_mipi_phy_enable (struct __amba_vin_source *src, u8 lanes)
{
	amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIPI_PHY_ENABLE, &lanes);
}

static void ov4689_vin_mipi_reset (struct __amba_vin_source *src)
{
	amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_CONFIG_MIPI_RESET, 0);
}

static int ov4689_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	struct ov4689_info *pinfo;
	u32 index = -1;
	u32 format_index;

	pinfo = (struct ov4689_info *) src->pinfo;
	if (pinfo->current_video_index == -1) {
		vin_err("ov4689 isn't ready!\n");
		errCode = -EINVAL;
		goto ov4689_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index = ov4689_video_info_table[index].format_index;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = ov4689_video_format_tbl.table[format_index].type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate = pinfo->frame_rate;
	args->video_format = ov4689_video_format_tbl.table[format_index].format;
	args->bit_resolution = ov4689_video_format_tbl.table[format_index].bits;
	args->aspect_ratio = ov4689_video_format_tbl.table[format_index].ratio;
	args->video_system = AMBA_VIDEO_SYSTEM_AUTO;

	/* TODO update it if necessary */
	args->max_width = 2688;
	args->max_height = 1520;

	args->def_cap_w = ov4689_video_format_tbl.table[format_index].width;
	args->def_cap_h = ov4689_video_format_tbl.table[format_index].height;

	args->cap_start_x = pinfo->cap_start_x;
	args->cap_start_y = pinfo->cap_start_y;
	args->cap_cap_w = pinfo->cap_cap_w;
	args->cap_cap_h = pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;
	args->hdr_data_read_protocol = HDR_MULTI_EXPO_IN_SEPARATE_LINE;

	args->sensor_id = GENERIC_SENSOR; /* TODO, some workaround is done in ucode for each sensor, so set it carefully */
	args->bayer_pattern = pinfo->bayer_pattern;
	args->field_format = 1;
	args->active_capinfo_num = pinfo->active_capinfo_num;
	args->bin_max = 4;
	args->skip_max = 8;
	args->sensor_readout_mode = ov4689_video_format_tbl.table[format_index].srm;
	args->input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW; /* TODO update it if it yuv interlaced*/
	args->column_bin = 0;
	args->row_bin = 0;
	args->column_skip = 0;
	args->row_skip = 0;

	args->mode_type = pinfo->mode_type;
	args->current_vin_mode = pinfo->current_vin_mode;
	args->current_shutter_time = pinfo->current_shutter_time;
	args->current_gain_db = pinfo->current_gain_db;
	args->current_sw_blc.bl_oo = pinfo->current_sw_blc.bl_oo;
	args->current_sw_blc.bl_oe = pinfo->current_sw_blc.bl_oe;
	args->current_sw_blc.bl_eo = pinfo->current_sw_blc.bl_eo;
	args->current_sw_blc.bl_ee = pinfo->current_sw_blc.bl_ee;
	args->current_fps = pinfo->frame_rate;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_VIN_CAP_INFO, &(args->vin_cap_info));

ov4689_get_video_preprocessing_exit:
	return errCode;
}
