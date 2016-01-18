/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/arch_a7/imx078_arch.c
 *
 * History:
 *    2012/01/06 - [Long Zhao] Create
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
static int imx078_init_vin_clock(struct __amba_vin_source *src, const struct imx078_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int imx078_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	u32	adap_arg;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int imx078_set_master_HV(struct __amba_vin_source *src, struct amba_vin_HV master_HV)
{
	int	errCode = 0;
	struct amba_vin_HV_width	master_HV_width;

	master_HV_width.vwidth = 0xa27;//master_HV.line_num_a_field - 1;
	master_HV_width.hwidth = 0xe50;//master_HV.pel_clk_a_line - 64;

	master_HV.line_num_a_field = 0xa28;
	master_HV.pel_clk_a_line = 0xe70;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_HV_WIDTH, &master_HV_width);
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_HV, &master_HV);
	return errCode;
}

/*
 * It is advised by sony, that they only guarantee the sync code. However, if we
 * want to use external sync signal, use the DCK sync mode. Try to use sync code
 * first!!!
 */
static int imx078_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct imx078_info		*pinfo;
	struct amba_vin_adap_config	imx078_config;
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

	pinfo = (struct imx078_info *)src->pinfo;
	index = pinfo->current_video_index;

	memset(&imx078_config, 0, sizeof (imx078_config));

	imx078_config.hsync_mask 	= VIN_DONT_CARE;
	imx078_config.sony_field_mode 	= AMBA_VIDEO_ADAP_NORMAL_FIELD;
	imx078_config.field0_pol 	= VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	imx078_config.hs_polarity = (phase & 0x1);
	imx078_config.vs_polarity = (phase & 0x2) >> 1;

	imx078_config.emb_sync_loc 	= EMB_SYNC_BOTH_PELS;
	imx078_config.emb_sync 		= EMB_SYNC_ON;
	imx078_config.emb_sync_mode = EMB_SYNC_ALL_ZEROS;

	imx078_config.sync_mode 	= SYNC_MODE_SONY_SPECIFIC;

	imx078_config.data_edge 	= PCLK_RISING_EDGE;
	imx078_config.input_mode 	= VIN_RGB_LVDS_1PEL_DDR_LVCMOS;

	imx078_config.serial_mode 	= SERIAL_VIN_MODE_ENABLE;
	imx078_config.sony_slvs_mode 	= SONY_SLVS_MODE_ENABLE;

	imx078_config.clk_select_slvs 	= CLK_SELECT_SPCLK;

	imx078_config.mipi_act_lanes = VIN_DONT_CARE;
	imx078_config.src_data_width = SRC_DATA_12B;
	imx078_config.slvs_control	= 0x6303;
	imx078_config.slvs_lane_mux_select[0]= 0x6134;
	imx078_config.slvs_lane_mux_select[1]= 0x0;
	imx078_config.slvs_lane_mux_select[2]= 0x0;


	imx078_config.slvs_eav_col 	= 0x824;
	/* line_length_pck 528 * 4 serial clk * 2 DDR * 4 lanes / 12 bits * 2(binning) data width */
	imx078_config.slvs_sav2sav_dist = 0x99f;


	imx078_config.slvs_frame_line_w	= imx078_config.slvs_sav2sav_dist;
	imx078_config.slvs_act_frame_line_w	= imx078_config.slvs_eav_col;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &imx078_config);

	nstartx = pinfo->cap_start_x;
	nstarty = pinfo->cap_start_y;
	width 	= pinfo->cap_cap_w;
	height 	= pinfo->cap_cap_h;

	nendx = (width + nstartx);
	nendy = (height + nstarty);

	window_info.start_x = nstartx;
	window_info.start_y = nstarty;
	window_info.end_x = nendx - 1;
	window_info.end_y = nendy - 1;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CAPTURE_WINDOW, &window_info);

	min_HV_sync.hs_min = nendx - nstartx - 20;
	min_HV_sync.vs_min = nendy - nstarty - 20;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = imx078_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int imx078_post_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	u32	adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int imx078_get_capability( struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int				errCode = 0;
	struct imx078_info *pinfo;
	u32				index = -1;
	u32				format_index;

	pinfo = (struct imx078_info *)src->pinfo;
	if(pinfo->current_video_index == -1) {
		vin_err("imx078 isn't ready!\n");
		errCode = -EINVAL;
		goto imx078_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index	= imx078_video_info_table[index].format_index;

	memset(args, 0, sizeof(struct amba_vin_src_capability));

	args->input_type 	= imx078_video_format_tbl.table[format_index].type;
	args->dev_type 		= AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate 	= pinfo->frame_rate;
	args->video_format 	= imx078_video_format_tbl.table[format_index].format;
	args->bit_resolution 	= imx078_video_format_tbl.table[format_index].bits;
	args->aspect_ratio 	= imx078_video_format_tbl.table[format_index].ratio;
	args->video_system 	= AMBA_VIDEO_SYSTEM_AUTO;

	args->max_width 		= 4168;
	args->max_height 	= 3060;

	args->def_cap_w 	= imx078_video_format_tbl.table[format_index].width;
	args->def_cap_h 	= imx078_video_format_tbl.table[format_index].height;

	args->cap_start_x 	= pinfo->cap_start_x;
	args->cap_start_y 	= pinfo->cap_start_y;
	args->cap_cap_w 	= pinfo->cap_cap_w;
	args->cap_cap_h 	= pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;

	args->sensor_id 	= SONYIMX078;
	args->bayer_pattern 	= pinfo->bayer_pattern;
	args->field_format 	= 1;
	args->active_capinfo_num= pinfo->active_capinfo_num;
	args->bin_max 		= 4;
	args->skip_max 		= 8;
	args->sensor_readout_mode= imx078_video_format_tbl.table[format_index].srm;
	args->input_format 	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	args->column_bin 	= 0;
	args->row_bin 		= 0;
	args->column_skip 	= 0;
	args->row_skip 		= 0;

	args->mode_type 	= pinfo->mode_type;
	args->current_vin_mode 	= pinfo->current_vin_mode;
	args->current_shutter_time = pinfo->current_shutter_time;
	args->current_gain_db 	= pinfo->current_gain_db;
	args->current_sw_blc.bl_oo = pinfo->current_sw_blc.bl_oo;
	args->current_sw_blc.bl_oe = pinfo->current_sw_blc.bl_oe;
	args->current_sw_blc.bl_eo = pinfo->current_sw_blc.bl_eo;
	args->current_sw_blc.bl_ee = pinfo->current_sw_blc.bl_ee;
	args->current_fps 		= pinfo->frame_rate;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_VIN_CAP_INFO, &(args->vin_cap_info));

imx078_get_video_preprocessing_exit:
	return errCode;
}

