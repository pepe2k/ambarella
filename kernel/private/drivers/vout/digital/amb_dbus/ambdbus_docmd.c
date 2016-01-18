/*
 * kernel/private/drivers/ambarella/vout/digital/amb_dbus/ambdbus_docmd.c
 *
 * History:
 *    2009/05/21 - [Anthony Ginger] Create
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
static int ambdbus_init_ccir601_525i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_525i)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_525i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV525I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_525i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_525i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_525i)
}

static int ambdbus_init_ccir601_625i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_625i)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_625i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV625I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_625i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_625i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_625i)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_480p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE, 525, 525,
		ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_HVSYNC(0, 62,
		0, 6 * 858, 429, 6 * 858 + 429,
		0, 0, 6, 1,
		0, 429, 6, 429,
		ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_ACTIVE_WIN(122, 122 + 720 - 1,
		36, 36 + 480 - 1, ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV480P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_480p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_480p)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_576p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE, 625, 625,
		ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_HVSYNC(0, 64,
		0, 5 * 864, 432, 5 * 864 + 432,
		0, 0, 5, 0,
		0, 432, 5, 432,
		ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_ACTIVE_WIN(132, 132 + 720 - 1,
		44, 44 + 576 - 1, ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV576P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_576p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_576p)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_HV(YUV720P_Y_PELS_PER_LINE, 750, 750,
		ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_HVSYNC(0, 40,
		0, 5 * 1650, 825, 5 * 1650 + 825,
		0, 0, 5, 0,
		0, 825, 5, 825,
		ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + 1280 - 1,
		25, 25 + 720 - 1, ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_HV(YUV720P50_Y_PELS_PER_LINE, 750, 750,
		ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_HVSYNC(0, 40,
		0, 5 * 1980, 990, 5 * 1980 + 990,
		0, 0, 5, 0,
		0, 990, 5, 990,
		ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + 1280 - 1,
		25, 25 + 720 - 1, ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p50)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p30(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_HV(3300, 750, 750,
		ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_HVSYNC(0, 40,
		0, 5 * 3300, 1650, 5 * 3300 + 1650,
		0, 0, 5, 0,
		0, 1650, 5, 1650,
		ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + 1280 - 1,
		25, 25 + 720 - 1, ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p30)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p30)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p25(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_HV(3960, 750, 750,
		ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_HVSYNC(0, 40,
		0, 5 * 3960, 1980, 5 * 3960 + 1980,
		0, 0, 5, 0,
		0, 1980, 5, 1980,
		ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + 1280 - 1,
		25, 25 + 720 - 1, ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p25)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p25)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_720p24(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 59400000,
		ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_HV(3300, 750, 750,
		ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_HVSYNC(0, 40,
		0, 5 * 3300, 1650, 5 * 3300 + 1650,
		0, 0, 5, 0,
		0, 1650, 5, 1650,
		ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_ACTIVE_WIN(260, 260 + 1280 - 1,
		25, 25 + 720 - 1, ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_720p24)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_720p24)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	if (sink_mode->frame_rate == AMBA_VIDEO_FPS_60) {
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
			ambdbus_init_ccir601_1080i)
	} else {
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
			ambdbus_init_ccir601_1080i)
	}

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_HV(YUV1080I_Y_PELS_PER_LINE, 562, 563,
		ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2200, 1100, 5 * 2200 + 1100,
		0, 0, 5, 0,
		0, 1100, 5, 1100,
		ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		20, 20 + 540 - 1, ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080i)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080i50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_HV(YUV1080I50_Y_PELS_PER_LINE, 562, 563,
		ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2640, 1320, 5 * 2640 + 1320,
		0, 0, 5, 0,
		0, 1320, 5, 1320,
		ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		20, 20 + 540 - 1, ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080I50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080i50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080i50)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_148_5D1001MHZ,
		ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_HV(YUV1080P_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2200, 1100, 5 * 2200 + 1100,
		0, 0, 5, 0,
		0, 1100, 5, 1100,
		ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		41, 41 + 1080 - 1, ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_148_5MHZ,
		ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_HV(YUV1080P50_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2640, 1320, 5 * 2640 + 1320,
		0, 0, 5, 0,
		0, 1320, 5, 1320,
		ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		41, 41 + 1080 -1, ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p50)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p30(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_HV(YUV1080P_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2200, 1100, 5 * 2200 + 1100,
		0, 0, 5, 0,
		0, 1100, 5, 1100,
		ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		41, 41 + 1080 - 1, ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p30)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p30)
}

/* ========================================================================== */
static int ambdbus_init_ccir601_1080p25(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_HV(YUV1080P50_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_HVSYNC(0, 44,
		0, 5 * 2640, 1320, 5 * 2640 + 1320,
		0, 0, 5, 0,
		0, 1320, 5, 1320,
		ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_ACTIVE_WIN(192, 192 + 1920 - 1,
		41, 41 + 1080 - 1, ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_1080p25)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_1080p25)
}

static int ambdbus_init_ccir601_rgb_960_240(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_960_240)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_960_240_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		errorCode = -EINVAL;
		goto ambdbus_init_ccir601_rgb_960_240_exit;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE * 2, 263, 263,
		ambdbus_init_ccir601_rgb_960_240)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_960_240)

	AMBA_VOUT_SET_ACTIVE_WIN(241, 241 + 960 * colors_per_dot - 1,
		21, 21 + YUV525I_ACT_LINES_PER_FLD - 1,
		ambdbus_init_ccir601_rgb_960_240)

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV480P_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_960_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_960_240)
	} else {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV525I_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_960_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_960_240)
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_960_240)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_960_240)
}

static int ambdbus_init_ccir601_rgb_320_240(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_320_240)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_320_240_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE * 2, 263, 263,
		ambdbus_init_ccir601_rgb_320_240)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_320_240)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(241, 241 + 320 * colors_per_dot - 1,
		21, 21 + YUV525I_ACT_LINES_PER_FLD - 1, colors_per_dot,
		ambdbus_init_ccir601_rgb_320_240)

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV480P_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_320_240)
	} else {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV525I_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_320_240)
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_240)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_320_240)
}

static int ambdbus_init_ccir601_rgb_320_288(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_320_288)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_320_288_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE * 2, 313, 313,
		ambdbus_init_ccir601_rgb_320_288)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_320_288)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(241, 241 + 320 * colors_per_dot - 1,
		21, 21 + YUV625I_ACT_LINES_PER_FLD - 1, colors_per_dot,
		ambdbus_init_ccir601_rgb_320_288)

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV480P_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_288)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_320_288)
	} else {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV625I_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_288)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_320_288)
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_320_288)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_320_288)
}

static int ambdbus_init_ccir601_rgb_360_240(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_360_240)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_360_240_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE * 2, 263, 263,
		ambdbus_init_ccir601_rgb_360_240)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_360_240)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(241, 241 + 360 * colors_per_dot - 1,
		21, 21 + YUV525I_ACT_LINES_PER_FLD - 1, colors_per_dot,
		ambdbus_init_ccir601_rgb_360_240)

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV480P_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_360_240)
	} else {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV525I_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_240)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_360_240)
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_240)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_360_240)
}

static int ambdbus_init_ccir601_rgb_360_288(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_360_288)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_360_288_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE * 2, 313, 313,
		ambdbus_init_ccir601_rgb_360_288)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_360_288)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(241, 241 + 360 * colors_per_dot - 1,
		21, 21 + YUV625I_ACT_LINES_PER_FLD - 1, colors_per_dot,
		ambdbus_init_ccir601_rgb_360_288)

	if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV480P_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_288)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_360_288)
	} else {
		AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
			AMBA_VIDEO_SYSTEM_NTSC, YUV625I_DEFAULT_FRAME_RATE,
			sink_mode->format, sink_mode->type, sink_mode->bits,
			sink_mode->ratio, sink_mode->video_flip,
			sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_288)

		AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_360_288)
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_360_288)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_360_288)
}

static int ambdbus_init_ccir601_rgb_480_640(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_480_640)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_480_640_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	switch (sink_mode->frame_rate) {
	case AMBA_VIDEO_FPS(50):
		AMBA_VOUT_SET_HV(500, 1080, 1080,
			ambdbus_init_ccir601_rgb_480_640)
		break;

	case AMBA_VIDEO_FPS(75):
		AMBA_VOUT_SET_HV(500, 720, 720,
			ambdbus_init_ccir601_rgb_480_640)
		break;

	case AMBA_VIDEO_FPS(60):
	case AMBA_VIDEO_FPS_AUTO:
	default:
		sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
		AMBA_VOUT_SET_HV(500, 900, 900,
			ambdbus_init_ccir601_rgb_480_640)
		break;
	}

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_480_640)

	switch (sink_mode->frame_rate) {
	case AMBA_VIDEO_FPS(50):
		AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
			220, 220 + 640 - 1, ambdbus_init_ccir601_rgb_480_640)
		break;

	case AMBA_VIDEO_FPS(75):
		AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
			40, 40 + 640 - 1, ambdbus_init_ccir601_rgb_480_640)
		break;

	case AMBA_VIDEO_FPS(60):
	case AMBA_VIDEO_FPS_AUTO:
	default:
		sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
		AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
			130, 130 + 640 - 1, ambdbus_init_ccir601_rgb_480_640)
		break;
	}

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, sink_mode->frame_rate,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_480_640)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_480_640)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_480_640)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_480_640)
}

static int ambdbus_init_ccir601_rgb_vga(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir601_rgb_vga)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_vga_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(500, 900, 900, ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
		130, 130 + 640 - 1, ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_vga)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_vga)
}

static int ambdbus_init_ccir601_rgb_hvga(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	switch (sink_mode->frame_rate) {
	case AMBA_VIDEO_FPS_AUTO:
	case AMBA_VIDEO_FPS(60):
		sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 17982018,
			ambdbus_init_ccir601_rgb_hvga)
		break;

	case AMBA_VIDEO_FPS(50):
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 15000000,
			ambdbus_init_ccir601_rgb_hvga)
		break;

	case AMBA_VIDEO_FPS(30):
		sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 9000000,
			ambdbus_init_ccir601_rgb_hvga)
		break;

	case AMBA_VIDEO_FPS(15):
		sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
		AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 4500000,
			ambdbus_init_ccir601_rgb_hvga)
		break;

	default:
		break;
	}

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_hvga_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(500, 600, 600, ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_HVSYNC(0, 10,
		0, 1, 0, 1,
		0, 0, 2, 0,
		0, 0, 2, 0,
		ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_ACTIVE_WIN(14, 14 + 320 * colors_per_dot - 1,
		4, 4 + 480 - 1, ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, sink_mode->frame_rate,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_hvga)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_hvga)
}

static int ambdbus_init_ccir601_rgb_480_800(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

#ifdef CONFIG_HDMI_VIN_VOUT_SYNC_WORKAROUND_ENABLE
	switch (sink_mode->lcd_cfg.model) {
	case AMBA_VOUT_LCD_MODEL_1P3831:
	case AMBA_VOUT_LCD_MODEL_1P3828:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 25774200,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 22500000,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}

		break;

	default:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(50):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 22500000,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(60):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 23790210,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}

		break;
	}
#else
	switch (sink_mode->lcd_cfg.model) {
	case AMBA_VOUT_LCD_MODEL_1P3831:
	case AMBA_VOUT_LCD_MODEL_1P3828:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 25800000,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 22500000,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}

		break;

	default:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(50):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 22500000,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(60):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 23790210,
				ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}

		break;
	}
#endif

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_480_800_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	default:
		colors_per_dot = 1;
		break;
	}

	switch (sink_mode->lcd_cfg.model) {
	case AMBA_VOUT_LCD_MODEL_1P3828:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			AMBA_VOUT_SET_HV(500, 860, 860, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 2,
				0, 1, 0, 1,
				0, 0, 2, 0,
				0, 0, 2, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(5, 5 + 480 * colors_per_dot - 1,
				12, 12 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;
		case AMBA_VIDEO_FPS(50):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
			AMBA_VOUT_SET_HV(500, 900, 900, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 8,
				0, 1, 0, 1,
				0, 0, 8, 0,
				0, 0, 8, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
				50, 50 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;
		default:
			break;
		}
		break;

	case AMBA_VOUT_LCD_MODEL_1P3831:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			AMBA_VOUT_SET_HV(500, 860, 860, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 2,
				0, 1, 0, 1,
				0, 0, 2, 0,
				0, 0, 2, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
				30, 30 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(50):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
			AMBA_VOUT_SET_HV(500, 900, 900, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 8,
				0, 1, 0, 1,
				0, 0, 8, 0,
				0, 0, 8, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
				50, 50 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}
		break;

	default:
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(50):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
			AMBA_VOUT_SET_HV(500, 900, 900, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 8,
				0, 1, 0, 1,
				0, 0, 8, 0,
				0, 0, 8, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 480 * colors_per_dot - 1,
				50, 50 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;

		case AMBA_VIDEO_FPS(60):
			AMBA_VOUT_SET_HV(490, 810, 810, ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_HVSYNC(0, 2,
				0, 1, 0, 1,
				0, 0, 2, 0,
				0, 0, 2, 0,
				ambdbus_init_ccir601_rgb_480_800)
			AMBA_VOUT_SET_ACTIVE_WIN(5, 5 + 480 * colors_per_dot - 1,
				5, 5 + 800 - 1, ambdbus_init_ccir601_rgb_480_800)
			break;

		default:
			break;
		}

		break;
	}

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, sink_mode->frame_rate,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_480_800)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_480_800)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_480_800)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_480_800)
}

static int ambdbus_init_ccir601_rgb_wvga(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	switch (sink_mode->lcd_cfg.model) {
	case AMBA_VOUT_LCD_MODEL_PPGA3:
#ifdef CONFIG_HDMI_VIN_VOUT_SYNC_WORKAROUND_ENABLE
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 29957413,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 24989500,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(30):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 14978706,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(15):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 7496850,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		default:
			break;
		}
#else
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 29987400,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 24989500,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(30):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 14993700,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(15):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 7496850,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		default:
			break;
		}
#endif
		break;

	default:
#ifdef CONFIG_HDMI_VIN_VOUT_SYNC_WORKAROUND_ENABLE
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 28397174,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 23664312,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(30):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 14198587,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(15):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 7099294,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		default:
			break;
		}
#else
		switch (sink_mode->frame_rate) {
		case AMBA_VIDEO_FPS_AUTO:
		case AMBA_VIDEO_FPS(60):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 28397174,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(50):
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 23664312,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(30):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 14198587,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		case AMBA_VIDEO_FPS(15):
			sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 7099294,
				ambdbus_init_ccir601_rgb_wvga)
			break;

		default:
			break;
		}
#endif
		break;
	}

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_wvga_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	case AMBA_VOUT_LCD_MODE_RGB888:
	default:
		colors_per_dot = 1;
		break;
	}

	switch (sink_mode->lcd_cfg.model) {
	case AMBA_VOUT_LCD_MODEL_PPGA3:
		AMBA_VOUT_SET_HV(943, 530, 530, ambdbus_init_ccir601_rgb_wvga)

		AMBA_VOUT_SET_HVSYNC(0, 15,
			0, 1879, 0, 1879,
			0, 0, 5, 0,
			0, 0, 5, 0,
			ambdbus_init_ccir601_rgb_wvga)

		AMBA_VOUT_SET_ACTIVE_WIN(55, 55 + 800 * colors_per_dot - 1,
			18, 18 + 480 - 1, ambdbus_init_ccir601_rgb_wvga)

		break;

	default:
		AMBA_VOUT_SET_HV(940, 504, 504, ambdbus_init_ccir601_rgb_wvga)

		AMBA_VOUT_SET_HVSYNC(0, 2,
			0, 1879, 0, 1879,
			0, 0, 2, 0,
			0, 0, 2, 0,
			ambdbus_init_ccir601_rgb_wvga)

		AMBA_VOUT_SET_ACTIVE_WIN(120, 120 + 800 * colors_per_dot - 1,
			20, 20 + 480 - 1, ambdbus_init_ccir601_rgb_wvga)

		break;
	}

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_wvga)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_wvga)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_wvga)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_wvga)
}

static int ambdbus_init_ccir601_rgb_240_400(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 19316264,
		ambdbus_init_ccir601_rgb_240_400)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_240_400_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	case AMBA_VOUT_LCD_MODE_RGB888:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(786, 410, 410, ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_HVSYNC(0, 3,
		0, 1571, 0, 1571,
		0, 0, 1, 785,
		0, 0, 1, 785,
		ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(9, 9 + 240 * colors_per_dot - 1,
		3, 3 + 400 - 1, colors_per_dot, ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_240_400)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_240_400)
}

static int ambdbus_init_ccir601_rgb_xga(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

#ifdef CONFIG_HDMI_VIN_VOUT_SYNC_WORKAROUND_ENABLE
	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 64930844,
		ambdbus_init_ccir601_rgb_xga)
#else
	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_65D1001MHZ,
		ambdbus_init_ccir601_rgb_xga)
#endif

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_xga_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	case AMBA_VOUT_LCD_MODE_RGB888:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(1344, 806, 806, ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_HVSYNC(0, 3,
		0, 2199, 0, 2199,
		0, 0, 1, 1099,
		0, 0, 1, 1099,
		ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(160, 160 + 1024 * colors_per_dot - 1,
		19, 19 + 768 - 1, colors_per_dot, ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_xga)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_xga)
}

static int ambdbus_init_ccir601_rgb_wsvga(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
        u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 51200000,
		ambdbus_init_ccir601_rgb_wsvga)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_wsvga_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	case AMBA_VOUT_LCD_MODE_RGB888:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(1344, 635, 635, ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_HVSYNC(0, 4,
		0, 3, 0, 3,
		0, 0, 2, 0,
		0, 0, 2, 0,
		ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 1024 * colors_per_dot - 1,
		10, 10 + 600 - 1, ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_wsvga)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_wsvga)
}

static int ambdbus_init_ccir601_rgb_960_540(
	struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()
	u32			colors_per_dot;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 25000000,
		ambdbus_init_ccir601_rgb_960_540)

	errorCode = ambdbus_init_ccir601_rgb_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_init_ccir601_rgb_960_540_exit;
	}

	switch (sink_mode->lcd_cfg.mode) {
	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		colors_per_dot = 3;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		colors_per_dot = 4;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
	case AMBA_VOUT_LCD_MODE_RGB565:
	case AMBA_VOUT_LCD_MODE_RGB888:
	default:
		colors_per_dot = 1;
		break;
	}

	AMBA_VOUT_SET_HV(980, 600, 600, ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_HVSYNC(0, 4,
		0, 3, 0, 3,
		0, 0, 2, 0,
		0, 0, 2, 0,
		ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_ACTIVE_WIN(10, 10 + 960 * colors_per_dot - 1,
		30, 30 + 540 - 1, ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width, sink_mode->video_size.video_height,
		AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_60,
		sink_mode->format, sink_mode->type, sink_mode->bits,
		sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir601_rgb_960_540)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir601_rgb_960_540)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_525i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir656_525i)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_525i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV525I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_525i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_525i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_525i)
}

static int ambdbus_init_ccir656_625i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir656_625i)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_625i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV625I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_625i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_625i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_625i)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_480p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE, 525, 525,
		ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_ACTIVE_WIN(138, 857,
		41, 520, ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV480P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_480p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_480p)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_576p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_27MHZ,
		ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE, 625, 625,
		ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_ACTIVE_WIN(144, 863,
		44, 619, ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV576P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_576p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_576p)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_HV(YUV720P_Y_PELS_PER_LINE, 750, 750,
		ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_ACTIVE_WIN(370, 1649,
		25, 744, ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_HV(YUV720P50_Y_PELS_PER_LINE, 750, 750,
		ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_ACTIVE_WIN(700, 1979,
		25, 744, ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p50)
}

static int ambdbus_init_ccir656_720p30(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_HV(3300, 750, 750,
		ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_ACTIVE_WIN(2020, 3299,
		25, 744, ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p30)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p30)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p25(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_HV(3960, 750, 750,
		ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_ACTIVE_WIN(2680, 3959,
		25, 744, ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p25)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p25)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_720p24(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, 59400000,
		ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_HV(3300, 750, 750,
		ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_ACTIVE_WIN(2020, 3299,
		25, 744, ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_720p24)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_720p24)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080i(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_HV(YUV1080I_Y_PELS_PER_LINE, 562, 563,
		ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_ACTIVE_WIN(280, 2199,
		20, 559, ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080I_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080i)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080i)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080i50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_HV(YUV1080I50_Y_PELS_PER_LINE, 562, 563,
		ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_ACTIVE_WIN(720, 2639,
		20, 559, ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080I50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080i50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080i50)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_148_5D1001MHZ,
		ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_HV(YUV1080P_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir656_1080p);

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_ACTIVE_WIN(280, 2199,
		41, 1120, ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p50(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_148_5MHZ,
		ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_HV(YUV1080P50_Y_PELS_PER_LINE, 1125, 1125,
		ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_ACTIVE_WIN(720, 2639,
		41, 1120, ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080P50_DEFAULT_FRAME_RATE, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate,
		ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p50)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p50)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p30(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25D1001MHZ,
		ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_HV(2200, 1125, 1125,
		ambdbus_init_ccir656_1080p30);

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_ACTIVE_WIN(280, 2199,
		41, 1120, ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p30)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p30)
}

/* ========================================================================== */
static int ambdbus_init_ccir656_1080p25(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_HV(2640, 1125, 1125,
		ambdbus_init_ccir656_1080p25);

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_ACTIVE_WIN(720, 2639,
		41, 1120, ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_PAL,
		YUV1080P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1080p25)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1080p25)
}

static int ambdbus_init_ccir656_1280_960(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()

	AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, PLL_CLK_74_25MHZ,
		ambdbus_init_ccir656_1280_960)

	AMBA_VOUT_SET_ARCH(ambdbus_init_ccir656_1280_960)

	switch (sink_mode->frame_rate) {
	case AMBA_VIDEO_FPS_AUTO:
	case AMBA_VIDEO_FPS(30):
		AMBA_VOUT_SET_HV(2475, 1000, 1000,
			ambdbus_init_ccir656_1280_960)
		break;

	default:
		AMBA_VOUT_SET_HV(2970, 1000, 1000,
			ambdbus_init_ccir656_1280_960)
		break;
	}

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 0, 1,
		0, 0, 0, 1,
		ambdbus_init_ccir656_1280_960)

	switch (sink_mode->frame_rate) {
	case AMBA_VIDEO_FPS_AUTO:
	case AMBA_VIDEO_FPS(30):
		AMBA_VOUT_SET_ACTIVE_WIN(2475 - 1280, 2474,
			995 - 960, 994, ambdbus_init_ccir656_1280_960)
		break;

	default:
		AMBA_VOUT_SET_ACTIVE_WIN(2970 - 1280, 2969,
			995 - 960, 994, ambdbus_init_ccir656_1280_960)
		break;
	}

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height, AMBA_VIDEO_SYSTEM_NTSC,
		YUV720P_DEFAULT_FRAME_RATE, sink_mode->format, sink_mode->type,
		sink_mode->bits, sink_mode->ratio, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1280_960)

	AMBA_VOUT_SET_VIDEO_SIZE(ambdbus_init_ccir656_1280_960)

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambdbus_init_ccir656_1280_960)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambdbus_init_ccir656_1280_960)
}

static int ambdbus_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
	int					errorCode = 0;

	if (psink->state != AMBA_VOUT_SINK_STATE_IDLE) {
		psink->pstate = psink->state;
		psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	}

	return errorCode;
}

static int ambdbus_suspend(struct __amba_vout_video_sink *psink, u32 *args)
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

static int ambdbus_resume(struct __amba_vout_video_sink *psink, u32 *args)
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
static int ambdbus_set_video_mode(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct ambdbus_info			*pinfo;
	int					i;

	pinfo = (struct ambdbus_info *)psink->pinfo;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_AUTO)
		sink_mode->sink_type = psink->sink_type;
	if (sink_mode->sink_type != psink->sink_type) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vout_err("%s-%d sink_type is %d, not %d!\n",
			psink->name, psink->id,
			psink->sink_type, sink_mode->sink_type);
		goto ambdbus_set_video_mode_exit;
	}
	for (i = 0; i < AMBA_VIDEO_MODE_MAX; i++) {
		if (pinfo->mode_list[i] == AMBA_VIDEO_MODE_MAX) {
			errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
			vout_err("%s-%d can not support mode %d!\n",
				psink->name, psink->id, sink_mode->mode);
			goto ambdbus_set_video_mode_exit;
		}
		if (pinfo->mode_list[i] == sink_mode->mode)
			break;
	}
	vout_info("%s-%d switch to mode %d!\n",
		psink->name, psink->id, sink_mode->mode);

	errorCode = ambdbus_pre_setmode_arch(psink, sink_mode);
	if (errorCode) {
		vout_errorcode();
		goto ambdbus_set_video_mode_exit;
	}

	errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	switch(sink_mode->mode) {
	case AMBA_VIDEO_MODE_480I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir656_525i(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir601_525i(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_576I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir656_625i(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir601_625i(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_D1_NTSC:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_480p(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_480p(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_D1_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_576p(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_576p(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_720p(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_720p(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_720p50(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_720p50(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P30:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_720p30(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_720p30(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P25:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_720p25(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_720p25(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_720P24:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_720p24(psink, sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_720p24(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir656_1080i(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir601_1080i(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080I_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir656_1080i50(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambdbus_init_ccir601_1080i50(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080P:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_1080p(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_1080p(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080P_PAL:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_1080p50(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_1080p50(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080P30:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_1080p30(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_1080p30(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1080P25:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_8;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_1080p25(psink,
				sink_mode);
		} else
		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_1080p25(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_960_240:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			((sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) ||
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE))) {
			errorCode = ambdbus_init_ccir601_rgb_960_240(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_320_240:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			((sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) ||
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE))) {
			errorCode = ambdbus_init_ccir601_rgb_320_240(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_320_288:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			((sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) ||
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE))) {
			errorCode = ambdbus_init_ccir601_rgb_320_288(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_360_240:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			((sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) ||
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE))) {
			errorCode = ambdbus_init_ccir601_rgb_360_240(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_360_288:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			((sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) ||
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE))) {
			errorCode = ambdbus_init_ccir601_rgb_360_288(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_480_640:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_480_640(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_VGA:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_vga(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_HVGA:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_hvga(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_480_800:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_480_800(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_WVGA:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_wvga(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_240_400:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_240_400(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_XGA:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_xga(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_WSVGA:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_RAW) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_wsvga(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_960_540:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_RGB_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_RGB_RAW) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir601_rgb_960_540(psink,
				sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_1280_960:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_656;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

		if ((sink_mode->type == AMBA_VIDEO_TYPE_YUV_656) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
			errorCode = ambdbus_init_ccir656_1280_960(psink,
				sink_mode);
		}
		break;

	default:
		vout_err("%s-%d do not support mode %d!\n",
			psink->name, psink->id, sink_mode->mode);
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	}

	if (!errorCode)
		errorCode = ambdbus_post_setmode_arch(psink, sink_mode);

	if (!errorCode)
		psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

	AMBA_VOUT_FUNC_EXIT(ambdbus_set_video_mode)
}

static int ambdbus_docmd(struct __amba_vout_video_sink *psink,
	enum amba_video_sink_cmd cmd, void *args)
{
	int					errorCode = 0;

	switch (cmd) {
	case AMBA_VIDEO_SINK_IDLE:
		break;

	case AMBA_VIDEO_SINK_RESET:
		errorCode = ambdbus_reset(psink, args);
		break;

	case AMBA_VIDEO_SINK_SUSPEND:
		errorCode = ambdbus_suspend(psink, args);
		break;

	case AMBA_VIDEO_SINK_RESUME:
		errorCode = ambdbus_resume(psink, args);
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
		errorCode = ambdbus_set_video_mode(psink,
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

