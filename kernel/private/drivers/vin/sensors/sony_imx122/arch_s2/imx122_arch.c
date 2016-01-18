/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/arch_a2/imx122_arch.c
 *
 * History:
 *    2011/09/23 - [Bingliang Hu] Create
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
static int imx122_init_vin_clock(struct __amba_vin_source *src, const struct imx122_pll_reg_table *pll_tbl)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = pll_tbl->extclk;
	adap_arg.so_pclk_freq_hz = pll_tbl->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int imx122_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	u32	adap_arg = 0;

	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

/*
 * It is advised by sony, that they only guarantee the sync code. However, if we
 * want to use external sync signal, use the DCK sync mode. Try to use sync code
 * first!!!
 */
static int imx122_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct imx122_info		*pinfo;
	struct amba_vin_adap_config	imx122_config;
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

	pinfo = (struct imx122_info *)src->pinfo;
	index = pinfo->current_video_index;

	memset(&imx122_config, 0, sizeof (imx122_config));

	imx122_config.hsync_mask 	= HSYNC_MASK_CLR;
	imx122_config.sony_field_mode 	= AMBA_VIDEO_ADAP_NORMAL_FIELD;
	imx122_config.field0_pol 	= VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	imx122_config.hs_polarity = (phase & 0x1);
	imx122_config.vs_polarity = (phase & 0x2) >> 1;

	imx122_config.emb_sync_loc 	= VIN_DONT_CARE;
	imx122_config.emb_sync 		= VIN_DONT_CARE;
	imx122_config.sync_mode 	= SYNC_MODE_SLAVE;

	imx122_config.data_edge 	= PCLK_RISING_EDGE;
	imx122_config.src_data_width 	= SRC_DATA_12B;
	imx122_config.input_mode 	= VIN_RGB_LVDS_1PEL_SDR_LVCMOS;

	imx122_config.mipi_act_lanes 	= VIN_DONT_CARE;
	imx122_config.serial_mode 	= SERIAL_VIN_MODE_DISABLE;
	imx122_config.sony_slvs_mode 	= SONY_SLVS_MODE_DISABLE;

	imx122_config.clk_select_slvs 	= CLK_SELECT_SPCLK;
	imx122_config.slvs_eav_col 	= VIN_DONT_CARE;
	imx122_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &imx122_config);

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

	min_HV_sync.hs_min = nendx - 20;
	min_HV_sync.vs_min = nendy - 20;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = imx122_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int imx122_post_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	u32	adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int imx122_get_capability( struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int				errCode = 0;
	struct imx122_info *pinfo;
	u32				index = -1;
	u32				format_index;

	pinfo = (struct imx122_info *)src->pinfo;
	if(pinfo->current_video_index == -1) {
		vin_err("imx122 isn't ready!\n");
		errCode = -EINVAL;
		goto imx122_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index	= imx122_video_info_table[index].format_index;

	memset(args, 0, sizeof(struct amba_vin_src_capability));

	args->input_type 	= imx122_video_format_tbl.table[format_index].type;
	args->dev_type 		= AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate 	= pinfo->frame_rate;
	args->video_format 	= imx122_video_format_tbl.table[format_index].format;
	args->bit_resolution 	= imx122_video_format_tbl.table[format_index].bits;
	args->aspect_ratio 	= imx122_video_format_tbl.table[format_index].ratio;
	args->video_system 	= AMBA_VIDEO_SYSTEM_AUTO;

	args->max_width 		= 2080;
	args->max_height 	= 1553;

	args->def_cap_w 	= imx122_video_format_tbl.table[format_index].width;
	args->def_cap_h 	= imx122_video_format_tbl.table[format_index].height;

	args->cap_start_x 	= pinfo->cap_start_x;
	args->cap_start_y 	= pinfo->cap_start_y;
	args->cap_cap_w 	= pinfo->cap_cap_w;
	args->cap_cap_h 	= pinfo->cap_cap_h;

	args->act_start_x = pinfo->act_start_x;
	args->act_start_y = pinfo->act_start_y;
	args->act_width = pinfo->act_width;
	args->act_height = pinfo->act_height;

	args->sensor_id 	= SONYIMX035;
	args->bayer_pattern 	= pinfo->bayer_pattern;
	args->field_format 	= 1;
	args->active_capinfo_num= pinfo->active_capinfo_num;
	args->bin_max 		= 4;
	args->skip_max 		= 8;
	args->sensor_readout_mode= imx122_video_format_tbl.table[format_index].srm;
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

imx122_get_video_preprocessing_exit:
	return errCode;
}
