/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441a/arch_a7/adv7441a_arch.c
 *
 * History:
 *    2011/10/19 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static int adv7441a_init_vin_clock(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = psrcinfo->so_freq_hz;
	adap_arg.so_pclk_freq_hz = psrcinfo->so_pclk_freq_hz;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int adv7441a_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int adv7441a_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct amba_vin_adap_config adv7441a_config;
	u32 width;
	u32 height;
	u32 nstartx;
	u32 nstarty;
	u32 nendx;
	u32 nendy;
	struct amba_vin_cap_window window_info;
	struct amba_vin_min_HV_sync min_HV_sync;
	u32 adap_arg;
	struct adv7441a_src_info *psrcinfo;
	u8 phase;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	errCode = adv7441a_init_vin_clock(src);

	memset(&adv7441a_config, 0, sizeof (adv7441a_config));

	adv7441a_config.hsync_mask = HSYNC_MASK_CLR;
	adv7441a_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	adv7441a_config.field0_pol = VIN_DONT_CARE;
	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	adv7441a_config.hs_polarity = (phase & 0x1);
	adv7441a_config.vs_polarity = (phase & 0x2) >> 1;
	if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_656) {
		adv7441a_config.emb_sync_loc = EMB_SYNC_LOWER_PEL;
		adv7441a_config.emb_sync = EMB_SYNC_ON;
		adv7441a_config.emb_sync_mode = EMB_SYNC_ITU_656;
		adv7441a_config.sync_mode = SYNC_MODE_SLAVE;
	} else {
		adv7441a_config.emb_sync_loc = VIN_DONT_CARE;
		adv7441a_config.emb_sync = VIN_DONT_CARE;
		adv7441a_config.emb_sync_mode = VIN_DONT_CARE;
		adv7441a_config.sync_mode = VIN_DONT_CARE;
	}
	adv7441a_config.data_edge = PCLK_RISING_EDGE;
	adv7441a_config.src_data_width = SRC_DATA_8B;
	if (psrcinfo->bit_resolution == AMBA_VIDEO_BITS_8) {
		adv7441a_config.input_mode = VIN_YUV_LVDS_1PEL_SDR_CB_Y0_CR_Y1_LVCMOS;
	} else {
		adv7441a_config.input_mode = VIN_YUV_LVDS_2PELS_SDR_Y0CB_Y1CR_LVCMOS;
	}
	adv7441a_config.mipi_act_lanes = VIN_DONT_CARE;
	adv7441a_config.serial_mode = SERIAL_VIN_MODE_DISABLE;
	adv7441a_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;
	adv7441a_config.clk_select_slvs = CLK_SELECT_SPCLK;
	adv7441a_config.slvs_eav_col = VIN_DONT_CARE;
	adv7441a_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	adv7441a_config.slvs_control = 0x00;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &adv7441a_config);

	nstartx = psrcinfo->cap_start_x;
	nstarty = psrcinfo->cap_start_y;
	width = psrcinfo->cap_cap_w;
	if (psrcinfo->video_format == AMBA_VIDEO_FORMAT_INTERLACE)
		height = psrcinfo->cap_cap_h >> 1;
	else
		height = psrcinfo->cap_cap_h;
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

	adap_arg = psrcinfo->sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}

static int adv7441a_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int adv7441a_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	u32 i;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = psrcinfo->input_type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_DECODER;
	args->frame_rate = psrcinfo->frame_rate;
	args->video_format = psrcinfo->video_format;
	args->bit_resolution = psrcinfo->bit_resolution;
	args->aspect_ratio = psrcinfo->aspect_ratio;
	args->video_system = psrcinfo->video_system;
	args->input_format = psrcinfo->input_format;
	args->current_vin_mode = psrcinfo->cap_vin_mode;
	args->max_width = psrcinfo->cap_cap_w;
	args->max_height = psrcinfo->cap_cap_h;
	args->def_cap_w = psrcinfo->cap_cap_w;
	args->def_cap_h = psrcinfo->cap_cap_h;
	args->cap_start_x = psrcinfo->cap_start_x;
	args->cap_start_y = psrcinfo->cap_start_y;
	args->cap_cap_w = psrcinfo->cap_cap_w;
	args->cap_cap_h = psrcinfo->cap_cap_h;

	/*Sensor related setting */
	args->sensor_id = GENERIC_SENSOR;
	args->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	args->field_format = 1;
	args->active_capinfo_num = 0;
	args->bin_max = 0;
	args->skip_max = 0;
	args->sensor_readout_mode = 0;
	args->column_bin = 0;
	args->row_bin = 0;
	args->column_skip = 0;
	args->row_skip = 0;

	for (i = 0; i < AMBA_VIN_MAX_FPS_TABLE_SIZE; i++) {
		args->ext_fps[i] = AMBA_VIDEO_FPS_AUTO;
	}

	args->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;
	args->current_shutter_time = 0;
	args->current_gain_db = 0;
	args->current_sw_blc.bl_oo = 0;
	args->current_sw_blc.bl_oe = 0;
	args->current_sw_blc.bl_eo = 0;
	args->current_sw_blc.bl_ee = 0;
	args->current_fps = 0;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_VIN_CAP_INFO, &(args->vin_cap_info));

	return errCode;
}
