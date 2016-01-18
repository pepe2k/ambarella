/*
 * Filename : ov9718_arch.c
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static int ov9718_init_vin_clock(struct __amba_vin_source *src, const struct ov9718_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int ov9718_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int ov9718_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct ov9718_info		*pinfo;
	struct amba_vin_adap_config	ov9718_config;
	u32				index;
	u16				phase;
	u32				width;
	u32				height;
	u32				nstartx;
	u32				nstarty;
	u32				nendx;
	u32				nendy;
	u32				adap_arg;

	struct amba_vin_cap_window	window_info;
	struct amba_vin_min_HV_sync	min_HV_sync;

	pinfo = (struct ov9718_info *)src->pinfo;
	index = pinfo->current_video_index;

	pinfo = (struct ov9718_info *)src->pinfo;

	memset(&ov9718_config, 0, sizeof (ov9718_config));

	ov9718_config.hsync_mask = VIN_DONT_CARE;
	ov9718_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	ov9718_config.field0_pol = VIN_DONT_CARE;

	ov9718_config.ecc_enable = 1;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	ov9718_config.hs_polarity = (phase & 0x1);
	ov9718_config.vs_polarity = (phase & 0x2) >> 1;

	ov9718_config.emb_sync_loc = EMB_SYNC_LOWER_PEL;
	ov9718_config.emb_sync = EMB_SYNC_OFF;
	ov9718_config.emb_sync_mode = EMB_SYNC_ITU_656;

	ov9718_config.sync_mode = SYNC_MODE_SLAVE;

	ov9718_config.data_edge = PCLK_RISING_EDGE;
	ov9718_config.src_data_width = SRC_DATA_12B;
	ov9718_config.input_mode = VIN_RGB_LVDS_2PELS_DDR_MIPI;

	ov9718_config.mipi_act_lanes = MIPI_2LANE;
	ov9718_config.serial_mode = SERIAL_VIN_MODE_DISABLE;
	ov9718_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;

	ov9718_config.clk_select_slvs = CLK_SELECT_MIPICLK;
	ov9718_config.slvs_eav_col = VIN_DONT_CARE;
	ov9718_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	ov9718_config.mipi_enable = MIPI_VIN_MODE_ENABLE;
	ov9718_config.mipi_data_type_sel = RAW_12BIT;
	ov9718_config.mipi_s_hssettlectl = 0xe;
	ov9718_config.mipi_b_dphyctl = 0x95a8;
	ov9718_config.mipi_s_dphyctl = 0x0796;

	ov9718_config.enhance_mode = 1;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &ov9718_config);

	nstartx = pinfo->cap_start_x;
	nstarty = pinfo->cap_start_y;
	/* for 656 mode we need double the capture width */
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

	adap_arg = ov9718_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int ov9718_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static void ov9718_vin_reset_idsp (struct __amba_vin_source *src)
{
	/* reset all sections */
	amba_writel(DSP_VIN_DEBUG_REG(0x801C), 0xFF);
}

/* ----- MIPI API ---------*/
static void ov9718_vin_mipi_phy_enable (struct __amba_vin_source *src, u8 lanes)
{
	amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIPI_PHY_ENABLE, &lanes);
}

static void ov9718_vin_mipi_reset (struct __amba_vin_source *src)
{
	amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_CONFIG_MIPI_RESET, 0);
}

static int ov9718_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	struct ov9718_info *pinfo;
	u32 index = -1;
	u32 format_index;

	pinfo = (struct ov9718_info *) src->pinfo;
	if (pinfo->current_video_index == -1) {
		vin_err("ov9718 isn't ready!\n");
		errCode = -EINVAL;
		goto ov9718_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index = ov9718_video_info_table[index].format_index;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = ov9718_video_format_tbl.table[format_index].type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate =  pinfo->frame_rate;
	args->video_format = ov9718_video_format_tbl.table[format_index].format;
	args->bit_resolution = ov9718_video_format_tbl.table[format_index].bits;
	args->aspect_ratio = ov9718_video_format_tbl.table[format_index].ratio;
	args->video_system = AMBA_VIDEO_SYSTEM_AUTO;

	args->max_width = 1296;
	args->max_height = 812;

	args->def_cap_w = ov9718_video_format_tbl.table[format_index].width;
	args->def_cap_h = ov9718_video_format_tbl.table[format_index].height;

	args->cap_start_x = pinfo->cap_start_x;
	args->cap_start_y = pinfo->cap_start_y;
	args->cap_cap_w = pinfo->cap_cap_w;
	args->cap_cap_h = pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;

	args->sensor_id = GENERIC_SENSOR; /* TODO, some workaround is done in ucode for each sensor, so set it carefully */
	args->bayer_pattern = pinfo->bayer_pattern;
	args->field_format = 1;
	args->active_capinfo_num = pinfo->active_capinfo_num;
	args->bin_max = 4;
	args->skip_max = 8;
	args->sensor_readout_mode = ov9718_video_format_tbl.table[format_index].srm;
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

ov9718_get_video_preprocessing_exit:
	return errCode;
}

