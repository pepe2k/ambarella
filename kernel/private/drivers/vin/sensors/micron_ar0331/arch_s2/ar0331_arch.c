/*
 * Filename : ar0331_arch.c
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static int ar0331_init_vin_clock(struct __amba_vin_source *src, const struct ar0331_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int ar0331_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
//	struct ar0331_info *pinfo;
	u32 adap_arg;

//	pinfo = (struct ar0331_info *) src->pinfo;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int ar0331_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct ar0331_info		*pinfo;
	struct amba_vin_adap_config	ar0331_config;
	u32				index;
	u16				phase;
	u32				width;
	u32				height;
	u32				nstartx;
	u32				nstarty;
	u32				nendx;
	u32				nendy;
	struct amba_vin_cap_window	window_info;
	struct amba_vin_min_HV_sync	min_HV_sync;
	u32				adap_arg;

	pinfo = (struct ar0331_info *)src->pinfo;
	index = pinfo->current_video_index;

	memset(&ar0331_config, 0, sizeof (ar0331_config));

	ar0331_config.hsync_mask = VIN_DONT_CARE;
	ar0331_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	ar0331_config.field0_pol = VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	ar0331_config.hs_polarity = (phase & 0x1);
	ar0331_config.vs_polarity = (phase & 0x2) >> 1;

	ar0331_config.emb_sync_loc = EMB_SYNC_BOTH_PELS;
	ar0331_config.emb_sync = EMB_SYNC_ON;
	ar0331_config.emb_sync_mode = EMB_SYNC_ALL_ZEROS;

	ar0331_config.sync_mode = SYNC_MODE_SLAVE;

	ar0331_config.data_edge = PCLK_RISING_EDGE;
	ar0331_config.src_data_width = SRC_DATA_12B;
	ar0331_config.input_mode = VIN_RGB_LVDS_2PELS_DDR_SLVS;

	ar0331_config.mipi_act_lanes = VIN_DONT_CARE;
	ar0331_config.serial_mode = SERIAL_VIN_MODE_ENABLE;
	ar0331_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;

	ar0331_config.clk_select_slvs = CLK_SELECT_SPCLK;
	ar0331_config.slvs_eav_col = pinfo->slvs_eav_col;
	ar0331_config.slvs_sav2sav_dist = pinfo->slvs_sav2sav_dist - 1;

	ar0331_config.slvs_control		= 0x301;
	ar0331_config.slvs_frame_line_w	= ar0331_config.slvs_sav2sav_dist;
	ar0331_config.slvs_act_frame_line_w	= ar0331_config.slvs_eav_col	;
	ar0331_config.slvs_lane_mux_select[0]= 0x3210;
	ar0331_config.slvs_lane_mux_select[1]= 0xc06c;
	ar0331_config.slvs_lane_mux_select[2]= 0x0001;
	ar0331_config.slvs_debug		= 0x3807;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &ar0331_config);


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

	min_HV_sync.hs_min = nendx - 20;
	min_HV_sync.vs_min = nendy - 20;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = ar0331_video_info_table[index].sync_start;
	adap_arg = 0x80808080;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}
static int ar0331_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int ar0331_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	struct ar0331_info *pinfo;
	u32 index = -1;
	u32 format_index;

	pinfo = (struct ar0331_info *) src->pinfo;
	if (pinfo->current_video_index == -1) {
		vin_err("ar0331 isn't ready!\n");
		errCode = -EINVAL;
		goto ar0331_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index = ar0331_video_info_table[index].format_index;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = ar0331_video_format_tbl.table[format_index].type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate =  pinfo->frame_rate;
	args->video_format = ar0331_video_format_tbl.table[format_index].format;
	/* FIX ME */
	if(pinfo->op_mode == AR0331_16BIT_HDR_MODE) {
		args->bit_resolution = AMBA_VIDEO_BITS_14;
	} else {
		args->bit_resolution = ar0331_video_format_tbl.table[format_index].bits;
	}
	args->aspect_ratio = ar0331_video_format_tbl.table[format_index].ratio;
	args->video_system = AMBA_VIDEO_SYSTEM_AUTO;
	//args->type_ext = ar0331_video_info_table[index].type_ext;

	args->max_width = 2048;
	args->max_height = 1536;

	args->def_cap_w = ar0331_video_format_tbl.table[format_index].width;
	args->def_cap_h = ar0331_video_format_tbl.table[format_index].height;

	args->cap_start_x = pinfo->cap_start_x;
	args->cap_start_y = pinfo->cap_start_y;
	args->cap_cap_w = pinfo->cap_cap_w;
	args->cap_cap_h = pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;

	args->sensor_id = MICRON_AR0331; /* TODO, some workaround is done in ucode for each sensor, so set it carefully */
	args->bayer_pattern = pinfo->bayer_pattern;
	args->field_format = 1;
	args->active_capinfo_num = pinfo->active_capinfo_num;
	args->bin_max = 4;
	args->skip_max = 8;
	args->sensor_readout_mode = ar0331_video_format_tbl.table[format_index].srm;
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

ar0331_get_video_preprocessing_exit:
	return errCode;
}

