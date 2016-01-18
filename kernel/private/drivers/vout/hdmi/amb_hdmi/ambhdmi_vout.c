/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_vout.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include "arch/ambhdmi_vout_arch.c"

typedef struct {
	enum amba_video_mode			vmode;
	u8					system;
	u32					fps;
	enum PLL_CLK_HZ				clock;
	enum PLL_CLK_HZ				clock2;
} _vd_ctrl_t;

const static _vd_ctrl_t VD_Ctrl[] = {
	{AMBA_VIDEO_MODE_VGA,       	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_60,    PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_480I,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_576I,      	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_D1_NTSC,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_D1_PAL,    	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_720P,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_74_25D1001MHZ,	PLL_CLK_74_25MHZ},
	{AMBA_VIDEO_MODE_720P_PAL,  	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_74_25MHZ,     	0},
	{AMBA_VIDEO_MODE_1080I,     	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_74_25D1001MHZ,	PLL_CLK_74_25MHZ},
	{AMBA_VIDEO_MODE_1080I_PAL, 	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_74_25MHZ,     	0},
	{AMBA_VIDEO_MODE_1080P24,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_24,    PLL_CLK_74_25MHZ,     	0},
	{AMBA_VIDEO_MODE_1080P25,   	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_25,    PLL_CLK_74_25MHZ,     	0},
	{AMBA_VIDEO_MODE_1080P30,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_30,    PLL_CLK_74_25D1001MHZ,	PLL_CLK_74_25MHZ},
	{AMBA_VIDEO_MODE_1080P,     	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_148_5D1001MHZ,	PLL_CLK_148_5MHZ},
	{AMBA_VIDEO_MODE_1080P_PAL, 	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_148_5MHZ,     	0},
	{AMBA_VIDEO_MODE_HDMI_NATIVE,	AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_AUTO,  PLL_CLK_27MHZ,        	0},
	{AMBA_VIDEO_MODE_720P24,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_24,    59400000,             	0},
	{AMBA_VIDEO_MODE_720P25,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_25,    PLL_CLK_74_25MHZ,     	0},
	{AMBA_VIDEO_MODE_720P30,      	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_30,    PLL_CLK_74_25D1001MHZ,	PLL_CLK_74_25MHZ},
	{AMBA_VIDEO_MODE_2160P30,      	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_30,    PLL_CLK_296_703MHZ,	0},
	{AMBA_VIDEO_MODE_2160P25,      	AMBA_VIDEO_SYSTEM_NTSC,  AMBA_VIDEO_FPS_25,    PLL_CLK_297MHZ,		0},
	{AMBA_VIDEO_MODE_2160P24,      	AMBA_VIDEO_SYSTEM_NTSC,  AMBA_VIDEO_FPS_24,    PLL_CLK_296_703MHZ,	0},
	{AMBA_VIDEO_MODE_2160P24_SE,   	AMBA_VIDEO_SYSTEM_NTSC,  AMBA_VIDEO_FPS_24,    PLL_CLK_297MHZ,		0},


};

#define	VMODE_NUM	ARRAY_SIZE(VD_Ctrl)


/* ========================================================================== */
static int ambhdmi_vout_init(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode,
	const amba_hdmi_video_timing_t *vt)
{
	int					i, errorCode = 0;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_video_info			sink_video_info;
	struct amba_video_source_csc_info	sink_csc;
	struct amba_video_source_clock_setup	clk_setup;
	u16					h_size;

	for (i = 0; i < VMODE_NUM; i++)
		if (VD_Ctrl[i].vmode == sink_mode->mode)
			break;
	if (i >= VMODE_NUM) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		goto ambhdmi_vout_init_exit;
	}

	errorCode = ambhdmi_vout_init_arch(psink, sink_mode, vt);
	if (errorCode) {
		vout_errorcode();
		goto ambhdmi_vout_init_exit;
	}

	if (VD_Ctrl[i].vmode != AMBA_VIDEO_MODE_HDMI_NATIVE) {
		if (sink_mode->hdmi_3d_structure == DDD_FRAME_PACKING || sink_mode->hdmi_3d_structure == DDD_SIDE_BY_SIDE_FULL) {
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, VD_Ctrl[i].clock * 2,
				ambhdmi_vout_init)
		} else {
			if ((sink_mode->frame_rate == AMBA_VIDEO_FPS_60 || sink_mode->frame_rate == AMBA_VIDEO_FPS_30) && VD_Ctrl[i].clock2) {
				AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, VD_Ctrl[i].clock2,
					ambhdmi_vout_init)
			} else {
				AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, VD_Ctrl[i].clock,
					ambhdmi_vout_init)
			}
		}
	} else {
		if (sink_mode->hdmi_3d_structure == DDD_FRAME_PACKING || sink_mode->hdmi_3d_structure == DDD_SIDE_BY_SIDE_FULL) {
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, vt->pixel_clock * 1000 * 2,
				ambhdmi_vout_init)
		} else {
			AMBA_VOUT_SET_CLOCK(VO_CLK_ONCHIP_PLL_27MHZ, vt->pixel_clock * 1000,
				ambhdmi_vout_init)
		}
	}

	h_size = vt->h_blanking + vt->h_active;
	AMBA_VOUT_SET_HVSYNC(0, vt->hsync_width,
		0, vt->vsync_width * h_size,
		sink_sync.vtsync_start + (h_size >> 1),
		sink_sync.vtsync_end + (h_size >> 1),
		0, 0, vt->vsync_width, 0,
		0, h_size >> 1, vt->vsync_width, h_size >> 1,
		ambhdmi_vout_init)

	AMBA_VOUT_SET_VIDEO_INFO(sink_mode->video_size.video_width,
		sink_mode->video_size.video_height,
		VD_Ctrl[i].system, VD_Ctrl[i].fps, sink_mode->format,
		sink_mode->type, sink_mode->bits, sink_mode->ratio,
		sink_mode->video_flip, sink_mode->video_rotate, ambhdmi_vout_init)

	if (sink_mode->csc_en) {
		switch (sink_mode->hdmi_color_space) {
		case AMBA_VOUT_HDMI_CS_RGB:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL,
				ambhdmi_vout_init)
			break;

		case AMBA_VOUT_HDMI_CS_YCBCR_444:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP,
				ambhdmi_vout_init)
			break;

		case AMBA_VOUT_HDMI_CS_YCBCR_422:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP,
				ambhdmi_vout_init)
			break;

		default:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
				ambhdmi_vout_init)
			break;
		}
	} else {
		switch (sink_mode->hdmi_color_space) {
		case AMBA_VOUT_HDMI_CS_RGB:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_RGB2RGB,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
				ambhdmi_vout_init)
			break;

		case AMBA_VOUT_HDMI_CS_YCBCR_444:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_RGB2YUV,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP,
				ambhdmi_vout_init)
			break;

		case AMBA_VOUT_HDMI_CS_YCBCR_422:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_RGB2YUV_12BITS,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP,
				ambhdmi_vout_init)
			break;

		default:
			AMBA_VOUT_SET_CSC_PRE_CHECKED(AMBA_VIDEO_SOURCE_CSC_HDMI,
				AMBA_VIDEO_SOURCE_CSC_RGB2RGB,
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL,
				ambhdmi_vout_init)
			break;
		}
	}

	AMBA_VOUT_SET_VIDEO(sink_mode->video_en, sink_mode->video_flip,
		sink_mode->video_rotate, ambhdmi_vout_init)

	AMBA_VOUT_SET_OSD()

	AMBA_VOUT_FUNC_EXIT(ambhdmi_vout_init)
}

