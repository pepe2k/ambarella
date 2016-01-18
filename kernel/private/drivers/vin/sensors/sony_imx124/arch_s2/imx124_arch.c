/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/arch_s2/imx124_arch.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
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
static int imx124_init_vin_clock(struct __amba_vin_source *src, const struct imx124_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int imx124_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32	adap_arg = 0;

	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

/*
 * It is advised by sony, that they only guarantee the sync code. However, if we
 * want to use external sync signal, use the DCK sync mode. Try to use sync code
 * first!!!
 */
static int imx124_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct imx124_info		*pinfo;
	struct amba_vin_adap_config	imx124_config;
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

	pinfo = (struct imx124_info *)src->pinfo;
	index = pinfo->current_video_index;

	memset(&imx124_config, 0, sizeof (imx124_config));

	imx124_config.hsync_mask = VIN_DONT_CARE;
	imx124_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	imx124_config.field0_pol = VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	imx124_config.hs_polarity = (phase & 0x1);
	imx124_config.vs_polarity = (phase & 0x2) >> 1;

	imx124_config.emb_sync_loc = EMB_SYNC_BOTH_PELS;
	imx124_config.emb_sync = EMB_SYNC_ON;
	imx124_config.emb_sync_mode = EMB_SYNC_ALL_ZEROS;

	imx124_config.sync_mode = SYNC_MODE_SLAVE;

	imx124_config.data_edge = PCLK_RISING_EDGE;
	imx124_config.src_data_width = SRC_DATA_12B;
	imx124_config.input_mode = VIN_RGB_LVDS_1PEL_DDR_LVCMOS;

	imx124_config.mipi_act_lanes = VIN_DONT_CARE;
	imx124_config.serial_mode = SERIAL_VIN_MODE_ENABLE;
	imx124_config.sony_slvs_mode = SONY_SLVS_MODE_ENABLE;

	imx124_config.clk_select_slvs = CLK_SELECT_SPCLK;
	imx124_config.slvs_eav_col = pinfo->slvs_eav_col;
	imx124_config.slvs_sav2sav_dist = pinfo->slvs_sav2sav_dist - 1;

	imx124_config.slvs_control = 0xA303;// 4 lanes, allow variable length HBLANK
	imx124_config.slvs_frame_line_w = imx124_config.slvs_sav2sav_dist;
	imx124_config.slvs_act_frame_line_w = imx124_config.slvs_eav_col;
	imx124_config.slvs_lane_mux_select[0]= 0x3210;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &imx124_config);

	nstartx = pinfo->cap_start_x;
	nstarty = pinfo->cap_start_y;
	width = pinfo->cap_cap_w;
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

	adap_arg = imx124_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int imx124_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int imx124_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	struct imx124_info *pinfo;
	u32 index = -1;
	u32 format_index;

	pinfo = (struct imx124_info *) src->pinfo;
	if (pinfo->current_video_index == -1) {
		vin_err("imx124 isn't ready!\n");
		errCode = -EINVAL;
		goto imx124_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index = imx124_video_info_table[index].format_index;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = imx124_video_format_tbl.table[format_index].type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate = pinfo->frame_rate;
	args->video_format = imx124_video_format_tbl.table[format_index].format;
	args->bit_resolution = imx124_video_format_tbl.table[format_index].bits;
	args->aspect_ratio = imx124_video_format_tbl.table[format_index].ratio;
	args->video_system = AMBA_VIDEO_SYSTEM_AUTO;

	args->max_width = 3096;
	args->max_height = 2080;

	args->def_cap_w = imx124_video_format_tbl.table[format_index].width;
	args->def_cap_h = imx124_video_format_tbl.table[format_index].height;

	args->cap_start_x = pinfo->cap_start_x;
	args->cap_start_y = pinfo->cap_start_y;
	args->cap_cap_w = pinfo->cap_cap_w;
	args->cap_cap_h = pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;
	args->hdr_data_read_protocol = HDR_MULTI_EXPO_IN_SINGLE_LINE;

	args->sensor_id = GENERIC_SENSOR;
	args->bayer_pattern = pinfo->bayer_pattern;
	args->field_format = 1;
	args->active_capinfo_num = pinfo->active_capinfo_num;
	args->bin_max = 4;
	args->skip_max = 8;
	args->sensor_readout_mode = imx124_video_format_tbl.table[format_index].srm;
	args->input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
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

imx124_get_video_preprocessing_exit:
	return errCode;
}
