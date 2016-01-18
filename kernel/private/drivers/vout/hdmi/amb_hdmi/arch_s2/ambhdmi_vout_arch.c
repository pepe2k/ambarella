/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/arch_s2/ambhdmi_vout_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

typedef struct {
	enum amba_video_mode			vmode;
	u8					vid_format;
} _vid_format_t;

const static _vid_format_t VD_Ctrl_Arch_S2[] = {
	{AMBA_VIDEO_MODE_VGA,	    	VD_NON_FIXED },
	{AMBA_VIDEO_MODE_480I,	    	VD_480I60    },
	{AMBA_VIDEO_MODE_576I, 	    	VD_576I50    },
	{AMBA_VIDEO_MODE_D1_NTSC,   	VD_480P60    },
	{AMBA_VIDEO_MODE_D1_PAL,    	VD_576P50    },
	{AMBA_VIDEO_MODE_720P,      	VD_720P60    },
	{AMBA_VIDEO_MODE_720P_PAL,  	VD_720P50    },
	{AMBA_VIDEO_MODE_1080I,     	VD_1080I60   },
	{AMBA_VIDEO_MODE_1080I_PAL, 	VD_1080I50   },
	{AMBA_VIDEO_MODE_1080P24,   	VD_1080P24   },
	{AMBA_VIDEO_MODE_1080P25,   	VD_1080P25   },
	{AMBA_VIDEO_MODE_1080P30,   	VD_1080P30   },
	{AMBA_VIDEO_MODE_1080P,     	VD_1080P60   },
	{AMBA_VIDEO_MODE_1080P_PAL, 	VD_1080P50   },
	{AMBA_VIDEO_MODE_HDMI_NATIVE,	VD_NON_FIXED },
	{AMBA_VIDEO_MODE_720P24,	VD_NON_FIXED },
	{AMBA_VIDEO_MODE_720P25,	VD_NON_FIXED },
	{AMBA_VIDEO_MODE_720P30,	VD_NON_FIXED },
	{AMBA_VIDEO_MODE_2160P30,   	VD_NON_FIXED},
	{AMBA_VIDEO_MODE_2160P25,   	VD_NON_FIXED},
	{AMBA_VIDEO_MODE_2160P24,   	VD_NON_FIXED},
	{AMBA_VIDEO_MODE_2160P24_SE,	VD_NON_FIXED},
};

#define	VMODE_NUM_ARCH_S2	ARRAY_SIZE(VD_Ctrl_Arch_S2)


/* ========================================================================== */
static int ambhdmi_vout_init_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode,
	const amba_hdmi_video_timing_t *vt)
{
	int					i, errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_window_info		sink_window;
	vd_config_t				sink_cfg;

	for (i = 0; i < VMODE_NUM_ARCH_S2; i++)
		if (VD_Ctrl_Arch_S2[i].vmode == sink_mode->mode)
			break;

	if (i >= VMODE_NUM_ARCH_S2) {
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		goto ambhdmi_vout_init_arch_exit;
	}

	if (vt->interlace) {
		AMBA_VOUT_SET_HV(vt->h_blanking + vt->h_active,
			vt->v_blanking + vt->v_active,
			sink_hv.vtsize + 1,
			ambhdmi_vout_init_arch)
	} else {
		AMBA_VOUT_SET_HV(vt->h_blanking + vt->h_active,
			vt->v_blanking + vt->v_active,
			sink_hv.vtsize,
			ambhdmi_vout_init_arch)
	}

	if (sink_mode->mode == AMBA_VIDEO_MODE_480I ||
		sink_mode->mode == AMBA_VIDEO_MODE_576I) {
		AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(
			vt->h_blanking - vt->hsync_offset,
			sink_window.start_x + vt->h_active - 1,
			vt->v_blanking - vt->vsync_offset,
			sink_window.start_y + vt->v_active - 1,
			0,
			ambhdmi_vout_init_arch)
	} else {
		AMBA_VOUT_SET_ACTIVE_WIN(vt->h_blanking - vt->hsync_offset,
			sink_window.start_x + vt->h_active - 1,
			vt->v_blanking - vt->vsync_offset,
			sink_window.start_y + vt->v_active - 1,
			ambhdmi_vout_init_arch)


	}

	AMBA_VOUT_SET_VIDEO_SIZE(ambhdmi_vout_init_arch)

	AMBA_VOUT_HDMI_CONFIG(vt->interlace, VD_Ctrl_Arch_S2[i].vid_format,
		vt->hsync_polarity, vt->vsync_polarity,
		sink_mode->hdmi_color_space, ambhdmi_vout_init_arch)

	AMBA_VOUT_FUNC_EXIT(ambhdmi_vout_init_arch)
}

