/*
 * kernel/private/drivers/ambarella/vin/decoders/ambdd/arch_s2/ambdd_arch.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
static int ambdd_init_vin_clock(struct __amba_vin_source *src)
{
	int					errorcode = 0;
	struct amba_vin_clk_info		adap_arg;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = video_clkout;
	adap_arg.so_pclk_freq_hz = video_pixclk;
	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errorcode;
}

static int ambdd_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int					errorcode = 0;
	u32					adap_arg;

	adap_arg = 0;
	errorcode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_INIT, &adap_arg);

	return errorcode;
}

static int ambdd_set_vin_mode(struct __amba_vin_source *src)
{
	int					errorcode = 0;
	struct amba_vin_adap_config		ambdd_config;
	u32					width;
	u32					height;
	u32					nstartx;
	u32					nstarty;
	u32					nendx;
	u32					nendy;
	struct amba_vin_cap_window		window_info;
	struct amba_vin_min_HV_sync		min_HV_sync;
	u32					adap_arg;
	struct ambdd_info			*pinfo;
	u32					brgb = 0;

	pinfo = (struct ambdd_info *)src->pinfo;
	pinfo->input_mode = 0;

	memset(&ambdd_config, 0, sizeof (ambdd_config));

	ambdd_config.hsync_mask = HSYNC_MASK_CLR;
	ambdd_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	ambdd_config.field0_pol = VIN_DONT_CARE;
	ambdd_config.hs_polarity = (video_phase & 0x1);
	ambdd_config.vs_polarity = (video_phase & 0x2) >> 1;
	ambdd_config.sync_mode = SYNC_MODE_SLAVE;
	ambdd_config.data_edge = video_data_edge;

	switch(video_type) {
	case AMBA_VIDEO_TYPE_YUV_601:
		ambdd_config.emb_sync_loc = VIN_DONT_CARE;
		ambdd_config.emb_sync = VIN_DONT_CARE;
		ambdd_config.emb_sync_mode = VIN_DONT_CARE;
		break;

	case AMBA_VIDEO_TYPE_YUV_656:
		ambdd_config.emb_sync_loc = EMB_SYNC_LOWER_PEL;
		ambdd_config.emb_sync = EMB_SYNC_ON;
		ambdd_config.emb_sync_mode = EMB_SYNC_ITU_656;
		break;

	case AMBA_VIDEO_TYPE_YUV_BT1120: //Assume high for 1120
		ambdd_config.emb_sync_loc = EMB_SYNC_UPPER_PEL;
		ambdd_config.emb_sync = EMB_SYNC_ON;
		ambdd_config.emb_sync_mode = EMB_SYNC_ITU_656;
		break;

	case AMBA_VIDEO_TYPE_RGB_601:
		brgb = 1;
		ambdd_config.emb_sync_loc = VIN_DONT_CARE;
		ambdd_config.emb_sync = VIN_DONT_CARE;
		ambdd_config.emb_sync_mode = VIN_DONT_CARE;
		break;

	case AMBA_VIDEO_TYPE_RGB_656:
		brgb = 1;
		ambdd_config.emb_sync_loc = EMB_SYNC_LOWER_PEL;
		ambdd_config.emb_sync = EMB_SYNC_ON;
		ambdd_config.emb_sync_mode = EMB_SYNC_ITU_656;
		break;

	case AMBA_VIDEO_TYPE_RGB_BT1120: //Assume high for 1120
		brgb = 1;
		ambdd_config.emb_sync_loc = EMB_SYNC_UPPER_PEL;
		ambdd_config.emb_sync = EMB_SYNC_ON;
		ambdd_config.emb_sync_mode = EMB_SYNC_ITU_656;
		break;

	case AMBA_VIDEO_TYPE_RGB_RAW:
	default:
		brgb = 1;
		ambdd_config.emb_sync_loc = VIN_DONT_CARE;
		ambdd_config.emb_sync = VIN_DONT_CARE;
		ambdd_config.emb_sync_mode = VIN_DONT_CARE;
		break;
	}

	switch(video_bits) {
	case AMBA_VIDEO_BITS_10:
		ambdd_config.src_data_width = SRC_DATA_10B;
		break;

	case AMBA_VIDEO_BITS_12:
		ambdd_config.src_data_width = SRC_DATA_12B;
		break;

	case AMBA_VIDEO_BITS_14:
		ambdd_config.src_data_width = SRC_DATA_14B;
		break;

	case AMBA_VIDEO_BITS_8:
	case AMBA_VIDEO_BITS_16:
	default:
		ambdd_config.src_data_width = SRC_DATA_8B;
		break;
	}

	if (brgb == 0) {
		pinfo->input_mode |= (0x01 << 4);
		if (video_bits == AMBA_VIDEO_BITS_16)
			pinfo->input_mode |= (0x01 << 2);
	}
	pinfo->input_mode |= ((video_yuv_mode & 0x03) << 7);
	ambdd_config.input_mode = pinfo->input_mode;

	ambdd_config.mipi_act_lanes = VIN_DONT_CARE;
	ambdd_config.serial_mode = SERIAL_VIN_MODE_DISABLE;
	ambdd_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;
	ambdd_config.clk_select_slvs = CLK_SELECT_SPCLK;
	ambdd_config.slvs_eav_col = VIN_DONT_CARE;
	ambdd_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_CONFIG, &ambdd_config);

	nstartx = cap_start_x;
	nstarty = cap_start_y;
	width = cap_cap_w;
	if (video_format == AMBA_VIDEO_FORMAT_INTERLACE)
		height = cap_cap_h >> 1;
	else
		height = cap_cap_h;
	nendx = (width + nstartx);
	nendy = (height + nstarty);

	window_info.start_x = nstartx;
	window_info.start_y = nstarty;
	window_info.end_x = nendx - 1;
	window_info.end_y = nendy - 1;
	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_CAPTURE_WINDOW, &window_info);

	min_HV_sync.hs_min = nendx - 20;
	min_HV_sync.vs_min = nendy - 20;
	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = sync_start;
	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errorcode;
}

static int ambdd_post_set_vin_mode(struct __amba_vin_source *src)
{
	int				errorcode = 0;
	u32				adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW,
		&adap_arg);

	return errorcode;
}

static int ambdd_get_capability(struct __amba_vin_source *src,
	struct amba_vin_src_capability *args)
{
	int					errorcode = 0;
	u32					i;

	memset(args, 0, sizeof(struct amba_vin_src_capability));

	args->input_type = video_type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_DECODER;
	args->frame_rate = video_fps;
	args->video_format = video_format;
	args->bit_resolution = video_bits;
	args->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
	args->video_system = AMBA_VIDEO_SYSTEM_AUTO;
	if (video_format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
		args->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	else
		args->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
	args->current_vin_mode = video_mode;

	args->max_width = cap_cap_w;
	args->max_height = cap_cap_h;

	args->def_cap_w = cap_cap_w;
	args->def_cap_h = cap_cap_h;

	args->cap_start_x = cap_start_x;
	args->cap_start_y = cap_start_y;
	args->cap_cap_w = cap_cap_w;
	args->cap_cap_h = cap_cap_h;

	/*Sensor related setting*/
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

	errorcode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_GET_VIN_CAP_INFO, &(args->vin_cap_info));

	return errorcode;
}

