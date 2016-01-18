/*
 * kernel/private/drivers/ambarella/vout/dve/amb_tve/ambtve_docmd.c
 *
 * History:
 *    2009/05/14 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static int ambtve_init_480i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambtve_init_480i)

	AMBA_VOUT_SET_ARCH(ambtve_init_480i)

	AMBA_VOUT_SET_VBI(ambtve_init_480i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV525I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_480i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_480i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_480i)
}

static int ambtve_init_576i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambtve_init_576i)

	AMBA_VOUT_SET_ARCH(ambtve_init_576i)

	AMBA_VOUT_SET_VBI(ambtve_init_576i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV625I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_576i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_576i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_576i)
}

static int ambtve_init_480p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambtve_init_480p)

	AMBA_VOUT_SET_ARCH(ambtve_init_480p)

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE, 525, 525, ambtve_init_480p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, YUV525I_Y_PELS_PER_LINE, 0, YUV525I_Y_PELS_PER_LINE,
		0, 0, 1, 0,
		0, 0, 1, 0,
		ambtve_init_480p)

	AMBA_VOUT_SET_ACTIVE_WIN(122, 122 + SD_Y_ACT_PELS_PER_LINE - 1,
		36, 36 + 483 - 1, ambtve_init_480p)

	AMBA_VOUT_SET_VBI(ambtve_init_480p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV480P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_480p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_480p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_480p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_480p)
}

static int ambtve_init_576p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambtve_init_576p)

	AMBA_VOUT_SET_ARCH(ambtve_init_576p)

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE, 625, 625, ambtve_init_576p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, YUV625I_Y_PELS_PER_LINE, 0, YUV625I_Y_PELS_PER_LINE,
		0, 0, 1, 0,
		0, 0, 1, 0,
		ambtve_init_576p)

	AMBA_VOUT_SET_ACTIVE_WIN(132, 132 + SD_Y_ACT_PELS_PER_LINE - 1,
		44, 44 + 576 - 1, ambtve_init_576p)

	AMBA_VOUT_SET_VBI(ambtve_init_576p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV576P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_576p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_576p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_576p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_576p)
}

static int ambtve_init_720p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambtve_init_720p)

	AMBA_VOUT_SET_ARCH(ambtve_init_720p)

	AMBA_VOUT_SET_HV(YUV720P_Y_PELS_PER_LINE, 750, 750, ambtve_init_720p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, YUV720P_Y_PELS_PER_LINE, 0, YUV720P_Y_PELS_PER_LINE,
		0, 0, 1, 0,
		0, 0, 1, 0,
		ambtve_init_720p)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + HD720P_Y_ACT_PELS_PER_LINE - 1,
		25, 25 + YUV720P_ACT_LINES_PER_FRM - 1, ambtve_init_720p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_720p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_720p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_720p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_720p)
}

static int ambtve_init_720p50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambtve_init_720p50)

	AMBA_VOUT_SET_ARCH(ambtve_init_720p50)

	AMBA_VOUT_SET_HV(YUV720P_Y_PELS_PER_LINE, 750, 750, ambtve_init_720p50)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, YUV720P_Y_PELS_PER_LINE, 0, YUV720P_Y_PELS_PER_LINE,
		0, 0, 1, 0,
		0, 0, 1, 0,
		ambtve_init_720p50)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + HD720P_Y_ACT_PELS_PER_LINE - 1,
		25, 25 + YUV720P_ACT_LINES_PER_FRM - 1, ambtve_init_720p50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate, ambtve_init_720p50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_720p50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_720p50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_720p50)
}

static int ambtve_init_1080i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambtve_init_1080i)

	AMBA_VOUT_SET_ARCH(ambtve_init_1080i)

	AMBA_VOUT_SET_HV(YUV1080I_Y_PELS_PER_LINE, 563, 563, ambtve_init_1080i)

	AMBA_VOUT_SET_HVSYNC(44, 87,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambtve_init_1080i)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + HD1080I_Y_ACT_PELS_PER_LINE - 1,
		21, 21 + YUV1080I_ACT_LINES_PER_FLD - 1, ambtve_init_1080i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_1080i)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_1080i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_1080i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_1080i)
}

static int ambtve_init_1080i50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambtve_init_1080i50)

	AMBA_VOUT_SET_ARCH(ambtve_init_1080i50)

	AMBA_VOUT_SET_HV(YUV1080I_Y_PELS_PER_LINE, 563, 563, ambtve_init_1080i50)

	AMBA_VOUT_SET_HVSYNC(44, 87,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambtve_init_1080i50)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + HD1080I_Y_ACT_PELS_PER_LINE - 1,
		21, 21 + YUV1080I_ACT_LINES_PER_FLD - 1, ambtve_init_1080i50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080I50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate, ambtve_init_1080i50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_1080i50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambtve_init_1080i50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambtve_init_1080i50)
}

static int ambtve_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;

	if (psink->state != AMBA_VOUT_SINK_STATE_IDLE) {
		psink->pstate = psink->state;
		psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	}

	return errorCode;
}

static int ambtve_suspend(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;

	if (psink->state != AMBA_VOUT_SINK_STATE_SUSPENDED) {
		psink->pstate = psink->state;
		psink->state = AMBA_VOUT_SINK_STATE_SUSPENDED;
	} else {
		errorCode = 1;
	}

	return errorCode;
}

static int ambtve_resume(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;

	if (psink->state == AMBA_VOUT_SINK_STATE_SUSPENDED) {
		psink->state = psink->pstate;
	} else {
		errorCode = 1;
	}

	return errorCode;
}

/* ========================================================================== */
static int ambtve_set_video_mode(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct ambtve_info			*pinfo;
	int					i;

	pinfo = (struct ambtve_info *)psink->pinfo;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_AUTO)
		sink_mode->sink_type = psink->sink_type;
	if (sink_mode->sink_type != psink->sink_type) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vout_err("%s-%d sink_type is %d, not %d!\n",
			psink->name, psink->id,
			psink->sink_type, sink_mode->sink_type);
		goto ambtve_set_video_mode_exit;
	}
	for (i = 0; i < AMBA_VIDEO_MODE_MAX; i++)	{
		if (pinfo->format_list[i] == AMBA_VIDEO_MODE_MAX) {
			errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
			vout_err("%s-%d can not support mode %d!\n",
				psink->name, psink->id, sink_mode->mode);
			goto ambtve_set_video_mode_exit;
		}
		if (pinfo->format_list[i] == sink_mode->mode)
			break;
	}

	vout_info("%s-%d switch to mode %d!\n",
		psink->name, psink->id, sink_mode->mode);

	errorCode = ambtve_pre_setmode_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambtve_set_video_mode_exit;
	}

	errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	switch(sink_mode->mode) {
	case AMBA_VIDEO_MODE_480I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_480i(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_576I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_576i(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_D1_NTSC:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambtve_init_480p(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_D1_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambtve_init_576p(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_16_9;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambtve_init_720p(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_16_9;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambtve_init_720p50(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_16_9;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_1080i(psink, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080I_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_16_9;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_1080i50(psink, sink_mode);
		}
		break;

	default:
		vout_err("%s-%d do not support mode %d!\n",
			psink->name, psink->id, sink_mode->mode);
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	}

	if (!errorCode)
		errorCode = ambtve_post_setmode_arch(psink, sink_mode);

	if (!errorCode)
		psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

	AMBA_VOUT_FUNC_EXIT(ambtve_set_video_mode)
}

static int ambtve_docmd(struct __amba_vout_video_sink *psink,
	enum amba_video_sink_cmd cmd, void *args)
{
	int					errorCode = 0;

	switch (cmd) {
	case AMBA_VIDEO_SINK_IDLE:
		break;

	case AMBA_VIDEO_SINK_RESET:
		errorCode = ambtve_reset(psink, args);
		break;

	case AMBA_VIDEO_SINK_SUSPEND:
		errorCode = ambtve_suspend(psink, args);
		break;

	case AMBA_VIDEO_SINK_RESUME:
		errorCode = ambtve_resume(psink, args);
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
		psink_info->hdmi_overscan = psink->hdmi_overscan;
	}
		break;

	case AMBA_VIDEO_SINK_GET_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SINK_SET_MODE:
		errorCode = ambtve_set_video_mode(psink,
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

