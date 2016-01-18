/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_docmd.c
 *
 * History:
 *    2009/06/02 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include "ambhdmi_vout.c"
#include "ambhdmi_hdmise.c"

/* ========================================================================== */
static int ambhdmi_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;
	struct ambhdmi_sink			*phdmi_sink;

	phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

	psink->pstate = psink->state;
	psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	amba_writel(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET, 0);

	return errorCode;
}

static int ambhdmi_suspend(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;
	struct ambhdmi_sink			*phdmi_sink;

	phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

	psink->pstate = psink->state;
	psink->state = AMBA_VOUT_SINK_STATE_SUSPENDED;
	amba_writel(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET, 0);

	return errorCode;
}

static int ambhdmi_resume(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;
	//struct ambhdmi_sink			*phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

	errorCode = ambhdmi_hw_init();
	if (errorCode) {
		vout_err("%s, %d\n", __func__, __LINE__);
	}

	return errorCode;
}

static int ambhdmi_set_video_mode(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					i, cs, errorCode = 0;
	struct ambhdmi_sink			*phdmi_sink;
	const amba_hdmi_edid_t			*pedid;
	const amba_hdmi_color_space_t		*pcolor_space;
	struct amba_video_sink_mode 		vout_mode;
	const amba_hdmi_video_timing_t		*vt = NULL;

	phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_AUTO)
		sink_mode->sink_type = psink->sink_type;
	if (sink_mode->sink_type != psink->sink_type) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vout_err("%s-%d sink_type is %d, not %d!\n",
			psink->name, psink->id,
			psink->sink_type, sink_mode->sink_type);
		goto ambhdmi_set_video_mode_exit;
	}
	for (i = 0; i < AMBA_VIDEO_MODE_MAX; i++) {
		if (phdmi_sink->mode_list[i] == AMBA_VIDEO_MODE_MAX) {
			errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
			vout_err("%s-%d can not support mode %d!\n",
				psink->name, psink->id, sink_mode->mode);
			goto ambhdmi_set_video_mode_exit;
		}
		if (phdmi_sink->mode_list[i] == sink_mode->mode)
			break;
	}

	/* Find Video Timing */
	vt = ambhdmi_edid_find_video_mode(phdmi_sink, sink_mode->mode);
	if (!vt) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vout_err("%s-%d can not support mode %d!\n",
			psink->name, psink->id, sink_mode->mode);
		goto ambhdmi_set_video_mode_exit;
	}

	vout_info("%s-%d switch to mode %d!\n",
		psink->name, psink->id, sink_mode->mode);

	memcpy(&vout_mode, sink_mode, sizeof(vout_mode));
	vout_mode.ratio = AMBA_VIDEO_RATIO_16_9;
	vout_mode.bits = AMBA_VIDEO_BITS_16;
	vout_mode.type = AMBA_VIDEO_TYPE_YUV_601;
	vout_mode.sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
	if (vt->interlace)
		vout_mode.format = AMBA_VIDEO_FORMAT_INTERLACE;
	else
		vout_mode.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
	vout_mode.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

	/* Select HDMI output color space */
	pedid		= &phdmi_sink->edid;
	pcolor_space	= &pedid->color_space;
	cs		= AMBA_VOUT_HDMI_CS_RGB;
	switch (vout_mode.hdmi_color_space) {
	case AMBA_VOUT_HDMI_CS_AUTO:
		/*if (pedid->interface == HDMI) {
			if (pcolor_space->support_ycbcr444)
				cs = AMBA_VOUT_HDMI_CS_YCBCR_444;
			if (pcolor_space->support_ycbcr422)
				cs = AMBA_VOUT_HDMI_CS_YCBCR_422;
		}*/
		cs = AMBA_VOUT_HDMI_CS_RGB;
		break;

	case AMBA_VOUT_HDMI_CS_YCBCR_444:
		if (pcolor_space->support_ycbcr444) {
			cs = AMBA_VOUT_HDMI_CS_YCBCR_444;
		} else {
			DRV_PRINT("%s: HDMI Sink Device doesn't support "
				"YCBCR444 input! Use RGB444 instead.\n",
				__func__);
		}
		break;

	case AMBA_VOUT_HDMI_CS_YCBCR_422:
		if (pcolor_space->support_ycbcr422) {
			cs = AMBA_VOUT_HDMI_CS_YCBCR_422;
		} else {
			DRV_PRINT("%s: HDMI Sink Device doesn't support "
				"YCBCR422 input! Use RGB444 instead.\n",
				__func__);
		}
		break;

	default:
		break;
	}
	vout_mode.hdmi_color_space = cs;

	errorCode = ambhdmi_vout_init(psink, &vout_mode, vt);
	if (errorCode) {
		vout_errorcode();
		goto ambhdmi_set_video_mode_exit;
	}

	ambhdmi_hdmise_init(phdmi_sink, cs, sink_mode->hdmi_3d_structure, sink_mode->hdmi_overscan, vt);

	if (!errorCode)
		psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

ambhdmi_set_video_mode_exit:
	return errorCode;
}

static int ambhdmi_docmd(struct __amba_vout_video_sink *psink,
	enum amba_video_sink_cmd cmd, void *args)
{
	int					errorCode = 0;

	switch (cmd) {
	case AMBA_VIDEO_SINK_IDLE:
		break;

	case AMBA_VIDEO_SINK_RESET:
		errorCode = ambhdmi_reset(psink, args);
		break;

	case AMBA_VIDEO_SINK_SUSPEND:
		errorCode = ambhdmi_suspend(psink, args);
		break;

	case AMBA_VIDEO_SINK_RESUME:
		errorCode = ambhdmi_resume(psink, args);
		break;

	case AMBA_VIDEO_SINK_GET_SOURCE_ID:
		*(int *)args = psink->source_id;
		break;

	case AMBA_VIDEO_SINK_GET_INFO:
	{
		struct amba_vout_sink_info	*psink_info;

		psink_info = (struct amba_vout_sink_info *)args;
		psink_info->source_id = psink->source_id;
		psink_info->sink_type = psink->sink_type;
		memcpy(psink_info->name, psink->name, sizeof(psink_info->name));
		psink_info->state = psink->state;
		psink_info->hdmi_plug = psink->hdmi_plug;
		memcpy(psink_info->hdmi_modes, psink->hdmi_modes, sizeof(psink->hdmi_modes));
		psink_info->hdmi_native_mode = psink->hdmi_native_mode;
		psink_info->hdmi_native_width = psink->hdmi_native_width;
		psink_info->hdmi_native_height = psink->hdmi_native_height;
		psink_info->hdmi_overscan = psink->hdmi_overscan;
	}
		break;

	case AMBA_VIDEO_SINK_GET_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SINK_SET_MODE:
		errorCode = ambhdmi_set_video_mode(psink,
			(struct amba_video_sink_mode *)args);
		break;

	default:
		vout_err("%s-%d do not support cmd %d!\n",
			psink->name, psink->id, cmd);
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errorCode;
}

