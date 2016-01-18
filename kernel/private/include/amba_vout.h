/*
 * kernel/private/include/amba_vout.h
 *
 * History:
 *    2009/05/13 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_VOUT_H
#define __AMBA_VOUT_H

#include <ambas_vout.h>

#define AMBA_VOUT_CLUT_SIZE		(256)

enum amba_video_source_status {
	AMBA_VIDEO_SOURCE_STATUS_IDLE = 0,
	AMBA_VIDEO_SOURCE_STATUS_RUNNING = 1,
	AMBA_VIDEO_SOURCE_STATUS_SUSPENDED = 2,
};

struct amba_video_source_scale_analog_info {
	u16 y_coeff;
	u16 pb_coeff;
	u16 pr_coeff;
	u16 y_cost;
	u16 pb_cost;
	u16 pr_cost;
};

struct amba_video_source_display_info {
	u8 enable_video;
	u8 enable_osd0;
	u8 enable_osd1;
	u8 enable_cursor;
};

struct amba_video_source_info {
	u32 enabled;
	struct amba_video_info video_info;
};

struct amba_video_source_osd_info {
	u32 osd_id;
	u32 gblend;
	u16 width;
	u16 height;
	u16 offset_x;
	u16 offset_y;
	u16 zoom_x;
	u16 zoom_y;
};

struct amba_video_source_osd_clut_info {
	u8 *pclut_table;
	u8 *pblend_table;
};

struct amba_video_source_clock_setup {
	u32				src;
	u32				freq_hz;
};

enum amba_video_source_cmd {
	AMBA_VIDEO_SOURCE_IDLE = 30000,
	AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO,

	AMBA_VIDEO_SOURCE_RESET = 30100,
	AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_RUN,
	AMBA_VIDEO_SOURCE_SUSPEND,
	AMBA_VIDEO_SOURCE_RESUME,
	AMBA_VIDEO_SOURCE_SUSPEND_TOSS,
	AMBA_VIDEO_SOURCE_RESUME_TOSS,
	AMBA_VIDEO_SOURCE_HALT,

	AMBA_VIDEO_SOURCE_REGISTER_SINK = 30200,
	AMBA_VIDEO_SOURCE_UNREGISTER_SINK,
	AMBA_VIDEO_SOURCE_REGISTER_IRQ_CALLBACK,
	AMBA_VIDEO_SOURCE_REGISTER_VOUT_SETUP_PTR,
	AMBA_VIDEO_SOURCE_GET_SINK_NUM,
	AMBA_VIDEO_SOURCE_REGISTER_DSP_CMD_FN,
	AMBA_VIDEO_SOURCE_REGISTER_AR_NOTIFIER,
	AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,

	AMBA_VIDEO_SOURCE_GET_CONFIG = 31000,
	AMBA_VIDEO_SOURCE_GET_VIDEO_SIZE,
	AMBA_VIDEO_SOURCE_GET_DISPLAY,
	AMBA_VIDEO_SOURCE_GET_OSD,
	AMBA_VIDEO_SOURCE_GET_OSD_CLUT,
	AMBA_VIDEO_SOURCE_GET_CURSOR,
	AMBA_VIDEO_SOURCE_GET_CURSOR_BITMAP,
	AMBA_VIDEO_SOURCE_GET_CURSOR_PALETTE,
	AMBA_VIDEO_SOURCE_GET_BG_COLOR,
	AMBA_VIDEO_SOURCE_GET_LCD,
	AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN,
	AMBA_VIDEO_SOURCE_GET_HV,
	AMBA_VIDEO_SOURCE_GET_HVSYNC,
	AMBA_VIDEO_SOURCE_GET_HVSYNC_POLARITY,
	AMBA_VIDEO_SOURCE_GET_SCALE_HD_ANALOG_OUT,
	AMBA_VIDEO_SOURCE_GET_SCALE_SD_ANALOG_OUT,
	AMBA_VIDEO_SOURCE_GET_VBI,
	AMBA_VIDEO_SOURCE_GET_VIDEO_INFO,
	AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_GET_CSC,
	AMBA_VIDEO_SOURCE_GET_CSC_DYNAMICALLY,
	AMBA_VIDEO_SOURCE_GET_DVE,
	AMBA_VIDEO_SOURCE_GET_ACTIVE_SINK_ID,
	AMBA_VIDEO_SOURCE_GET_CLOCK_SETUP,
	AMBA_VIDEO_SOURCE_GET_OSD_BUFFER,
	AMBA_VIDEO_SOURCE_GET_DISPLAY_INPUT,
	AMBA_VIDEO_SOURCE_GET_VIDEO_AR,

	AMBA_VIDEO_SOURCE_SET_CONFIG = 32000,
	AMBA_VIDEO_SOURCE_SET_VIDEO_SIZE,
	AMBA_VIDEO_SOURCE_SET_DISPLAY,
	AMBA_VIDEO_SOURCE_SET_OSD,
	AMBA_VIDEO_SOURCE_SET_OSD_CLUT,
	AMBA_VIDEO_SOURCE_SET_CURSOR,
	AMBA_VIDEO_SOURCE_SET_CURSOR_BITMAP,
	AMBA_VIDEO_SOURCE_SET_CURSOR_PALETTE,
	AMBA_VIDEO_SOURCE_SET_BG_COLOR,
	AMBA_VIDEO_SOURCE_SET_LCD,
	AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN,
	AMBA_VIDEO_SOURCE_SET_HV,
	AMBA_VIDEO_SOURCE_SET_HVSYNC,
	AMBA_VIDEO_SOURCE_SET_HVSYNC_POLARITY,
	AMBA_VIDEO_SOURCE_SET_SCALE_HD_ANALOG_OUT,
	AMBA_VIDEO_SOURCE_SET_SCALE_SD_ANALOG_OUT,
	AMBA_VIDEO_SOURCE_SET_VBI,
	AMBA_VIDEO_SOURCE_SET_VIDEO_INFO,
	AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_SET_CSC,
	AMBA_VIDEO_SOURCE_SET_CSC_DYNAMICALLY,
	AMBA_VIDEO_SOURCE_SET_DVE,
	AMBA_VIDEO_SOURCE_SET_ACTIVE_SINK_ID,
	AMBA_VIDEO_SOURCE_SET_CLOCK_SETUP,
	AMBA_VIDEO_SOURCE_SET_OSD_BUFFER,
	AMBA_VIDEO_SOURCE_SET_DISPLAY_INPUT,
	AMBA_VIDEO_SOURCE_SET_VIDEO_AR,
};
#define AMBA_VIDEO_SOURCE_FORCE_RESET		(1 << 0)
#define AMBA_VIDEO_SOURCE_UPDATE_MIXER_SETUP	(1 << 1)
#define AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP	(1 << 2)
#define AMBA_VIDEO_SOURCE_UPDATE_DISPLAY_SETUP	(1 << 3)
#define AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP	(1 << 4)
#define AMBA_VIDEO_SOURCE_UPDATE_CSC_SETUP	(1 << 5)

enum amba_video_sink_cmd {
	AMBA_VIDEO_SINK_IDLE = 40000,

	AMBA_VIDEO_SINK_RESET = 40100,
	AMBA_VIDEO_SINK_SUSPEND,
	AMBA_VIDEO_SINK_RESUME,
	AMBA_VIDEO_SINK_GET_SOURCE_ID,
	AMBA_VIDEO_SINK_GET_INFO,

	AMBA_VIDEO_SINK_GET_MODE = 41000,

	AMBA_VIDEO_SINK_SET_MODE = 42000,
};

enum amba_video_source_csc_path_info {
	AMBA_VIDEO_SOURCE_CSC_DIGITAL = 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG = 1,
	AMBA_VIDEO_SOURCE_CSC_HDMI = 2,
};

enum amba_video_source_csc_mode_info {
	AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD	= 0,	/* YUV601 -> YUV709 */
	AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD	= 1,	/* YUV601 -> YUV601 */
	AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB		= 2,	/* YUV601 -> RGB    */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD	= 3,	/* YUV709 -> YUV601 */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD	= 4,	/* YUV709 -> YUV709 */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB		= 5,	/* YUV709 -> RGB    */
	AMBA_VIDEO_SOURCE_CSC_RGB2RGB		= 6,	/* RGB    -> RGB */
	AMBA_VIDEO_SOURCE_CSC_RGB2YUV		= 7,	/* RGB    -> YUV */
	AMBA_VIDEO_SOURCE_CSC_RGB2YUV_12BITS	= 8,	/* RGB    -> YUV_12bits */

	AMBA_VIDEO_SOURCE_CSC_ANALOG_SD		= 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_HD		= 1,
};

enum amba_video_source_csc_clamp_info {
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL		= 0,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL		= 1,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL		= 2,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL		= 3,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP		= 4,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP		= 5,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP	= 6,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP	= 7,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP	= 8,

	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD			= 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD			= 1,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD_NTSC		= 2,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD_PAL		= 3,
};

struct amba_video_source_csc_info {
	enum amba_video_source_csc_path_info path;
	enum amba_video_source_csc_mode_info mode;
	enum amba_video_source_csc_clamp_info clamp;
};

#include "amba_arch_vout.h"

typedef void irq_callback_t(void);
typedef void vout_dsp_cmd(void *cmd, u32 size);
typedef void (*vout_ar_notifier_t)(u8 video_src, u8 ar);

#define AMBA_VOUT_SINK_DECLARE_COMMON_VARIABLES()	\
	int					errorCode = 0;	\
	struct amba_vout_hv_size_info		sink_hv;	\
	struct amba_vout_hv_sync_info		sink_sync;	\
	struct amba_vout_window_info		sink_window;	\
        struct amba_video_info			sink_video_info;	\
	struct amba_video_source_clock_setup	clk_setup;

#define AMBA_VOUT_SET_CLOCK(_src, _freq_hz, func)	\
	clk_setup.src = _src;	\
	clk_setup.freq_hz = _freq_hz;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_CLOCK_SETUP, &clk_setup);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_ARCH(func)	\
	errorCode = func##_arch(psink, sink_mode);		\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_HV(h_size, vt_size, vb_size, func)	\
	sink_hv.hsize = h_size;	\
	sink_hv.vtsize = vt_size;	\
	sink_hv.vbsize = vb_size;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_HV, &sink_hv);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_HVSYNC(hs, he, vts, vte, vbs, vbe, vtsr, vtsc, vter,	\
	vtec, vbsr, vbsc, vber, vbec, func)	\
	sink_sync.hsync_start = hs;	\
	sink_sync.hsync_end = he;	\
	sink_sync.vtsync_start = vts;	\
	sink_sync.vtsync_end = vte;	\
	sink_sync.vbsync_start = vbs;	\
	sink_sync.vbsync_end = vbe;	\
	sink_sync.vtsync_start_row = vtsr;	\
	sink_sync.vtsync_start_col = vtsc;	\
	sink_sync.vtsync_end_row = vter;	\
	sink_sync.vtsync_end_col = vtec;	\
	sink_sync.vbsync_start_row = vbsr;	\
	sink_sync.vbsync_start_col = vbsc;	\
	sink_sync.vbsync_end_row = vber;	\
	sink_sync.vbsync_end_col = vbec;	\
	sink_sync.sink_type = psink->sink_type;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_HVSYNC, &sink_sync);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_ACTIVE_WIN(sx, ex, sy, ey, func)	\
	sink_window.start_x = sx;	\
	sink_window.end_x = ex;	\
	sink_window.start_y = sy;	\
	sink_window.end_y = ey;	\
	sink_window.width = ex - (sx) + 1;	\
	sink_window.field_reverse = 0;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN, &sink_window);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(sx, ex, sy, ey, fr, func)	\
	sink_window.start_x = sx;	\
	sink_window.end_x = ex;	\
	sink_window.start_y = sy;	\
	sink_window.end_y = ey;	\
	sink_window.width = (ex - (sx) + 1) >> 1;	\
	sink_window.field_reverse = fr;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN, &sink_window);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_DIGITAL_ACTIVE_WIN(sx, ex, sy, ey, cpd, func)	\
	sink_window.start_x = sx;	\
	sink_window.end_x = ex;	\
	sink_window.start_y = sy;	\
	sink_window.end_y = ey;	\
	sink_window.width = (ex - (sx) + 1) / (cpd);	\
	sink_window.field_reverse = 0;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN, &sink_window);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_VBI(func)	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_VBI, sink_mode);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_VIDEO_INFO(_width, _height, _system, _fps, _format,	\
	_type, _bits, _ratio, _flip, _rotate, func)	\
	sink_video_info.width = _width;	\
	sink_video_info.height = _height;	\
	sink_video_info.system = _system;	\
	sink_video_info.fps = _fps;	\
	sink_video_info.format = _format;	\
	sink_video_info.type = _type;	\
	sink_video_info.bits = _bits;	\
	sink_video_info.ratio = _ratio;	\
	sink_video_info.flip = _flip;	\
	sink_video_info.rotate = _rotate;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_VIDEO_INFO, &sink_video_info);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_VIDEO_SIZE(func)	\
	sink_window.start_x = sink_mode->video_offset.offset_x;	\
	sink_window.start_y = sink_mode->video_offset.offset_y;	\
	sink_window.end_x = sink_window.start_x + sink_mode->video_size.video_width - 1;	\
	sink_window.end_y = sink_window.start_y + sink_mode->video_size.video_height - 1;	\
	if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {	\
		sink_window.start_y >>= 1;	\
		sink_window.end_y >>= 1;	\
	}	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_VIDEO_SIZE, &sink_window);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_LCD(func)	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_LCD, &sink_mode->lcd_cfg);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_CSC(_path, _mode, _clamp, func)	\
	sink_csc.path = _path;	\
	if (sink_mode->csc_en)	\
		sink_csc.mode = _mode;	\
	else	\
		sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;	\
	sink_csc.clamp = _clamp;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_CSC,	\
		&sink_csc);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_CSC_PRE_CHECKED(_path, _mode, _clamp, func)	\
	sink_csc.path = _path;	\
	sink_csc.mode = _mode;	\
	sink_csc.clamp = _clamp;	\
	errorCode = amba_vout_video_source_cmd(psink->source_id,	\
		AMBA_VIDEO_SOURCE_SET_CSC,	\
		&sink_csc);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_DVE_NTSC_CONFIG(func)	\
	errorCode = ambtve_dve_ntsc_config(psink, sink_mode);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_DVE_PAL_CONFIG(func)	\
	errorCode = ambtve_dve_pal_config(psink, sink_mode);	\
	if (errorCode) {	\
		vout_errorcode();	\
		goto func##_exit;	\
	}

#define AMBA_VOUT_SET_OSD()	\
	amba_osd_on_vout_change(psink->source_id, sink_mode);

#define AMBA_VOUT_FUNC_EXIT(func)	\
func##_exit:	\
	return errorCode;

/* ========================================================================== */
int amba_vout_pm(u32 pmval);

int amba_vout_video_source_cmd(int id,
	enum amba_video_source_cmd cmd, void *args);
int amba_vout_video_sink_cmd(int id,
	enum amba_video_sink_cmd cmd, void *args);
extern int amba_osd_on_vout_change(int vout_id,	\
	struct amba_video_sink_mode *sink_mode);
extern int amba_osd_on_fb_switch(int vout_id, int fb_id);
extern int amba_osd_on_csc_change(int vout_id, int csc_en);
extern int amba_osd_on_rescaler_change(int vout_id, int enable,
	int width, int height);
extern int amba_osd_on_offset_change(int vout_id, int change,
	int x, int y);

#endif

