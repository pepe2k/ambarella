/*
 * kernel/private/drivers/ambarella/vin/decoders/ambdd/ambdd_docmd.c
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
static int ambdd_get_video_info(struct __amba_vin_source *src,
	struct amba_video_info *p_video_info)
{
	int					errorcode = 0;

	p_video_info->width = cap_cap_w;
	p_video_info->height = cap_cap_h;
	p_video_info->fps = video_fps;
	p_video_info->format = video_format;
	p_video_info->type = video_type;
	p_video_info->bits = video_bits;
	p_video_info->ratio = AMBA_VIDEO_RATIO_AUTO;
	p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
	p_video_info->rev = 0;

	return errorcode;
}

static int ambdd_check_vidoe_mode(struct __amba_vin_source *src,
	struct amba_vin_source_mode_info *p_mode_info)
{
	int					errorcode = 0;

	p_mode_info->is_supported = 0;

	if (p_mode_info->mode != AMBA_VIDEO_MODE_AUTO) {
		vin_err("AMBDD only supports auto mode.\n");
		errorcode = -EINVAL;
		goto ambdd_check_vidoe_mode_exit;
	}

	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof(p_mode_info->video_info));

	p_mode_info->mode = video_mode;
	p_mode_info->is_supported = 1;
	errorcode = ambdd_get_video_info(src, &p_mode_info->video_info);
	amba_vin_source_set_fps_flag(p_mode_info, video_fps);

ambdd_check_vidoe_mode_exit:
	return errorcode;
}

static int ambdd_set_video_mode(struct __amba_vin_source *src,
	enum amba_video_mode mode)
{
	int					errorcode = 0;

	errorcode = ambdd_init_vin_clock(src);
	if (errorcode)
		goto ambdd_set_video_mode_exit;

	msleep(100);
	errorcode = ambdd_pre_set_vin_mode(src);
	if (errorcode)
		goto ambdd_set_video_mode_exit;

	msleep(100);
	errorcode = ambdd_set_vin_mode(src);
	if (errorcode)
		goto ambdd_set_video_mode_exit;

	msleep(100);
	errorcode = ambdd_post_set_vin_mode(src);

	msleep(100);
	ambdd_print_info(src);

ambdd_set_video_mode_exit:
	return errorcode;
}

/* ========================================================================== */
static int ambdd_docmd(struct __amba_vin_source *src,
	enum amba_vin_src_cmd cmd, void *args)
{
	int					errorcode = 0;
	//struct ambdd_info			*pinfo = (struct ambdd_info *)src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_IDLE:
		break;

	case AMBA_VIN_SRC_RESET:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_POWER:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_INFO:
	{
		struct amba_vin_source_info	*pub_src;

		pub_src = (struct amba_vin_source_info *)args;
		pub_src->id = src->id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof(pub_src->name));
		pub_src->sensor_id = DECODER_AMBDD;
		pub_src->default_mode = video_mode;
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS;
		pub_src->adapter_id = 0;
	}
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errorcode = ambdd_get_video_info(src,
			(struct amba_video_info *)args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errorcode = ambdd_get_capability(src,
			(struct amba_vin_src_capability *)args);
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errorcode = ambdd_check_vidoe_mode(src,
			(struct amba_vin_source_mode_info *)args);
		break;

	case AMBA_VIN_SRC_SELECT_CHANNEL:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errorcode = ambdd_set_video_mode(src,
			*(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
	case AMBA_VIN_SRC_SET_STILL_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_FPN:
	case AMBA_VIN_SRC_SET_FPN:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_BLC:
	case AMBA_VIN_SRC_SET_BLC:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_CAP_WINDOW:
	case AMBA_VIN_SRC_SET_CAP_WINDOW:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_BLANK:
	case AMBA_VIN_SRC_SET_BLANK:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_PIXEL_SKIP_BIN:
	case AMBA_VIN_SRC_SET_PIXEL_SKIP_BIN:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_CAPTURE_MODE:
	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_TRIGGER_MODE:
	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_SLOWSHUTTER_MODE:
	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n",
			src->name, src->id, cmd);
		errorcode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errorcode;
}

