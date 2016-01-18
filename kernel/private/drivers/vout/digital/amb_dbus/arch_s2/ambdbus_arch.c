/*
 * kernel/private/drivers/ambarella/vout/digital/amb_dbus/arch_s2/ambdbus_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
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
static struct ambdbus_instance_info dbusall_instance = {
	.name		= "DBus-All",
	.pdbus_sink	= NULL,
	.source_id	= AMBA_VOUT_SOURCE_STARTING_ID,
	.sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL,
	.mode_list	= {
		AMBA_VIDEO_MODE_480I,
		AMBA_VIDEO_MODE_576I,
		AMBA_VIDEO_MODE_D1_NTSC,
		AMBA_VIDEO_MODE_D1_PAL,
		AMBA_VIDEO_MODE_720P,
		AMBA_VIDEO_MODE_720P_PAL,
		AMBA_VIDEO_MODE_720P30,
		AMBA_VIDEO_MODE_720P25,
		AMBA_VIDEO_MODE_720P24,
		AMBA_VIDEO_MODE_1080I,
		AMBA_VIDEO_MODE_1080I_PAL,
		AMBA_VIDEO_MODE_1080P,
		AMBA_VIDEO_MODE_1080P_PAL,
		AMBA_VIDEO_MODE_1080P25,
		AMBA_VIDEO_MODE_1080P30,

		AMBA_VIDEO_MODE_960_240,
		AMBA_VIDEO_MODE_320_240,
		AMBA_VIDEO_MODE_320_288,
		AMBA_VIDEO_MODE_360_240,
		AMBA_VIDEO_MODE_360_288,
		AMBA_VIDEO_MODE_480_640,
		AMBA_VIDEO_MODE_VGA,
		AMBA_VIDEO_MODE_HVGA,
		AMBA_VIDEO_MODE_480_800,
		AMBA_VIDEO_MODE_WVGA,
		AMBA_VIDEO_MODE_240_400,
		AMBA_VIDEO_MODE_XGA,
		AMBA_VIDEO_MODE_960_540,

		AMBA_VIDEO_MODE_MAX
	}
};

static struct ambdbus_instance_info dbuslcd_instance = {
	.name		= "DBus-LCD",
	.pdbus_sink	= NULL,
	.source_id	= AMBA_VOUT_SOURCE_STARTING_ID + 1,
	.sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL,
	.mode_list	= {
		AMBA_VIDEO_MODE_480I,
		AMBA_VIDEO_MODE_576I,
		AMBA_VIDEO_MODE_D1_NTSC,
		AMBA_VIDEO_MODE_D1_PAL,
		AMBA_VIDEO_MODE_720P,
		AMBA_VIDEO_MODE_720P_PAL,
		AMBA_VIDEO_MODE_720P30,
		AMBA_VIDEO_MODE_720P25,
		AMBA_VIDEO_MODE_720P24,
		AMBA_VIDEO_MODE_1080I,
		AMBA_VIDEO_MODE_1080I_PAL,
		AMBA_VIDEO_MODE_1080P,
		AMBA_VIDEO_MODE_1080P_PAL,
		AMBA_VIDEO_MODE_1080P25,
		AMBA_VIDEO_MODE_1080P30,

		AMBA_VIDEO_MODE_960_240,
		AMBA_VIDEO_MODE_320_240,
		AMBA_VIDEO_MODE_320_288,
		AMBA_VIDEO_MODE_360_240,
		AMBA_VIDEO_MODE_360_288,
		AMBA_VIDEO_MODE_480_640,
		AMBA_VIDEO_MODE_VGA,
		AMBA_VIDEO_MODE_HVGA,
		AMBA_VIDEO_MODE_480_800,
		AMBA_VIDEO_MODE_WVGA,
		AMBA_VIDEO_MODE_240_400,
		AMBA_VIDEO_MODE_XGA,
		AMBA_VIDEO_MODE_960_540,

		AMBA_VIDEO_MODE_MAX
	}
};

/* ========================================================================== */
static int ambdbus_add_s2dbus(struct ambdbus_instance_info *pdbus)
{
	int				errorCode = 0;
	struct ambdbus_info		*pdbus_info;
	struct __amba_vout_video_sink	*psink;

	pdbus_info = kzalloc(sizeof(struct ambdbus_info), GFP_KERNEL);
	if (!pdbus_info) {
		errorCode = -ENOMEM;
		goto ambdbus_add_s2dbus_exit;
	}
	pdbus_info->mode_list = pdbus->mode_list;

	psink = &pdbus_info->video_sink;
	psink->source_id = pdbus->source_id;
	psink->sink_type = pdbus->sink_type;
	strlcpy(psink->name, pdbus->name, sizeof(psink->name));
	psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	psink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
	psink->hdmi_native_mode = AMBA_VIDEO_MODE_AUTO;
	psink->owner = THIS_MODULE;
	psink->pinfo = pdbus_info;
	psink->docmd = ambdbus_docmd;
	errorCode = amba_vout_add_video_sink(psink);
	if (errorCode)
		goto ambdbus_add_s2dbus_free_pdbus_info;

	vout_notice("%s:%d@%d probed!\n", psink->name,
		psink->id, psink->source_id);
	pdbus->pdbus_sink = pdbus_info;

	goto ambdbus_add_s2dbus_exit;

ambdbus_add_s2dbus_free_pdbus_info:
	kfree(pdbus_info);
	pdbus->pdbus_sink = NULL;

	AMBA_VOUT_FUNC_EXIT(ambdbus_add_s2dbus)
}

static int ambdbus_del_s2dbus(struct ambdbus_instance_info *pdbus)
{
	int				errorCode = 0;
	struct ambdbus_info		*pdbus_info;

	if (pdbus->pdbus_sink) {
		pdbus_info = pdbus->pdbus_sink;
		errorCode = amba_vout_del_video_sink(&pdbus_info->video_sink);
		vout_notice("%s removed!\n", pdbus_info->video_sink.name);
		kfree(pdbus_info);
		pdbus->pdbus_sink = NULL;
	}

	return errorCode;
}

static int ambdbus_probe_arch(void)
{
	int				errorCode = 0;

	errorCode = ambdbus_add_s2dbus(&dbusall_instance);
	if (errorCode)
		goto ambdbus_probe_arch_exit;

	errorCode = ambdbus_add_s2dbus(&dbuslcd_instance);
	if (errorCode)
		goto ambdbus_probe_arch_exit;

	AMBA_VOUT_FUNC_EXIT(ambdbus_probe_arch)
}

static int ambdbus_remove_arch(void)
{
	int				errorCode = 0;

	errorCode = ambdbus_del_s2dbus(&dbusall_instance);
	errorCode |= ambdbus_del_s2dbus(&dbuslcd_instance);

	return errorCode;
}

static int ambdbus_init_ccir601_rgb_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED,
			LCD_ACT_LOW, LCD_ACT_LOW,
			sink_cfg.d_digital_output_mode.s.mode,
			ambdbus_init_ccir601_rgb_arch)
	} else {
		AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_NON_FIXED,
			LCD_ACT_LOW, LCD_ACT_LOW,
			sink_cfg.d_digital_output_mode.s.mode,
			ambdbus_init_ccir601_rgb_arch)
	}

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_rgb_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_525i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE * 2, 262, 263,
		ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 124,
		0, 3 * 1716, 858, 3 * 1716 + 858,
		0, 0, 3, 0,
		0, 858, 3, 858,
		ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(238, 238 + 1440 - 1, 18, 18 + 240 - 1, 0,
		ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_480I60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_601_8BITS,
		ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_525i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_525i_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_625i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE * 2, 312, 313,
		ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 126,
		0, 3 * 1728, 864, 3 * 1728 + 864,
		0, 0, 3, 0,
		0, 864, 3, 864,
		ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(264, 264 + 1440 - 1, 22, 22 + 288 - 1, 0,
		ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_576I50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_601_8BITS,
		ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_625i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_625i_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_480p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_480P60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_480p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_480p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_480p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_576p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_576P50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_576p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_576p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_576p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_720P60, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_720p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_720p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_720P50, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_720p50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_720p50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p30_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_720p30_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_720p30_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p30_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p25_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_720p25_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_720p25_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p25_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p24_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_720p24_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_720p24_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p24_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_1080I60, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080i_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080i50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_1080I50, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080i50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080i50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080i50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P60, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080p60_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080p60_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p60_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P50, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080p50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080p50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p30_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P30, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080p30_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080p30_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p30_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p25_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P25, LCD_ACT_HIGH,
		LCD_ACT_HIGH, LCD_MODE_601_16BITS,
		ambdbus_init_ccir601_1080p25_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir601_1080p25_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p25_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_525i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(1716, 262, 263,
		ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(276, 1715, 20, 259, 0,
		ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_480I60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_525i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_525i_arch)
}

static int ambdbus_init_ccir656_625i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(1728, 312, 313,
		ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(288, 1727, 22, 309, 0,
		ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_576I50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_625i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_625i_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_480p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_480P60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_480p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_480p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_480p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_576p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_576P50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_576p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_576p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_576p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_720P60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_720p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_720p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_720P50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_720p50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_720p50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p30_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_720p30_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_720p30_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p30_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p25_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_720p25_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_720p25_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p25_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p24_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_NON_FIXED, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_720p24_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_720p24_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p24_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_1080I60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080i_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080i_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080i50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_INTERLACE, VD_1080I50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080i50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080i50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080i50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080p_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p50_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080p50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080p50_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p50_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p30_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P60, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080p30_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080p30_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p30_arch)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p25_arch(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_DIGITAL_CONFIG(VD_PROGRESSIVE, VD_1080P50, LCD_ACT_LOW,
		LCD_ACT_LOW, LCD_MODE_656, ambdbus_init_ccir656_1080p25_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_DIGITAL,
		AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,
		AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
		ambdbus_init_ccir656_1080p25_arch)

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p25_arch)
}

static int ambdbus_init_ccir656_1280_960_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	return AMBA_ERR_FUNC_NOT_SUPPORTED;
}

/* ========================================================================== */
static int ambdbus_pre_setmode_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;

	return errorCode;
}

static int ambdbus_post_setmode_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;

	AMBA_VOUT_SET_LCD(ambdbus_post_setmode_arch)

AMBA_VOUT_FUNC_EXIT(ambdbus_post_setmode_arch)
}

