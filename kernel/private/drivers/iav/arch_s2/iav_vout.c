/*
 * iav_vout.c
 *
 * History:
 *	2012/04/13 - [Jian Tang] created file
 *
 * Copyright (C) 2009-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "amba_dsp.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "amba_imgproc.h"

#define VOUT_FLAGS_ALL		0xFFFFFFFF

#define VOUT_UPDATE_MIXER	AMBA_VIDEO_SOURCE_UPDATE_MIXER_SETUP
#define VOUT_UPDATE_VIDEO	AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP
#define VOUT_UPDATE_DISPLAY	AMBA_VIDEO_SOURCE_UPDATE_DISPLAY_SETUP
#define VOUT_UPDATE_OSD		AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP

#define VOUT_UPDATE_ALL		(VOUT_UPDATE_MIXER | VOUT_UPDATE_VIDEO | VOUT_UPDATE_DISPLAY | VOUT_UPDATE_OSD)

/*-------------------------------------------------------------------------------------
	vout resource management
-------------------------------------------------------------------------------------*/

typedef struct iav_vout_info_s {
	vout_default_info_t	*default_info;
} iav_vout_info_t;

iav_vout_info_t G_iav_vout_info[IAV_NUM_VOUT];
void (*G_iav_vout_irq_handler)(unsigned) = NULL;

static void issue_vout_cmd(void *cmd, u32 size)
{
	dsp_issue_cmd(cmd, size);
}

int iav_vout_init(void)
{
	int i;
	for (i = 0; i < IAV_NUM_VOUT; i++) {
		amba_vout_video_source_cmd(i,
			AMBA_VIDEO_SOURCE_REGISTER_DSP_CMD_FN,
			issue_vout_cmd);
	}
	return 0;
}

static void set_default_info(VOUT_VIDEO_SETUP_CMD *dsp_cmd, int vout_id)
{
	vout_default_info_t *default_info = G_iav_vout_info[vout_id].default_info;
	dsp_cmd->default_img_info_dram_addr = VIRT_TO_DSP(default_info);
	clean_d_cache(default_info, sizeof(*default_info));
}

static void config_vout(iav_context_t *context, int vout_mask, VOUT_SRC vout_src, u32 flag)
{
	int i;
	VOUT_VIDEO_SETUP_CMD def_vout_data;

	for (i = 0; i < 2; i++) {
		int enable = vout_mask & (1 << i);
		int src;

		if (i == 0) {
			src = vout_src & 0xffff;
		} else {
			src = vout_src >> 16;
		}

		if (enable || i == 0) {
			amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
				&def_vout_data);

			def_vout_data.en = enable ? 1 : 0;
			def_vout_data.src = enable ? src : VOUT_SRC_BACKGROUND;
			if (vout_src == VOUT_SRC_DEFAULT_IMG) {
				set_default_info(&def_vout_data, i);
			}

			amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
				&def_vout_data);
			amba_vout_video_source_cmd(i,
				AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

			G_iav_info.pvoutinfo[i]->active_mode.video_en = enable;
		} else {
			if (context->need_issue_reset_hdmi) {
				amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_RESET, &flag);
			}
		}
	}
}

static int update_vout_info(iav_context_t *context, int chan)
{
	int				errorCode = 0;
	struct amba_video_source_info	voutinfo;

	errorCode = amba_vout_video_source_cmd(chan,
		AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO,
		&voutinfo);
	if (errorCode) {
		iav_error("%s: AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO %d\n",
			__func__, errorCode);
		G_iav_info.pvoutinfo[chan]->enabled = 0;
		goto iav_update_vout_exit;
	}

	G_iav_info.pvoutinfo[chan]->enabled = voutinfo.enabled;
	G_iav_info.pvoutinfo[chan]->video_info = voutinfo.video_info;

iav_update_vout_exit:
	return errorCode;
}

static int stop_vout(iav_context_t *context, int chan)
{
	int	rval;

	rval = amba_vout_video_source_cmd(chan,
		AMBA_VIDEO_SOURCE_RESET, NULL);
	if (rval)
		goto iav_stop_vout_exit;

	rval = update_vout_info(context, chan);
	if (rval)
		goto iav_stop_vout_exit;

iav_stop_vout_exit:
	return rval;
}

static int start_vout(iav_context_t *context, int chan)
{
	int	rval;
	int	vout_enabled = 0;

	rval = update_vout_info(context, chan);
	if (rval)
		goto iav_start_vout_exit;

	vout_enabled = G_iav_info.pvoutinfo[chan]->enabled;

	if (!vout_enabled) {
		rval = amba_vout_video_source_cmd(chan,
			AMBA_VIDEO_SOURCE_RUN, NULL);
		if (rval)
			goto iav_start_vout_exit;

		rval = update_vout_info(context, chan);
		if (rval)
			goto iav_start_vout_exit;
	} else {
		iav_error("%s: bad vout state %d:%d\n", __func__,
			chan, vout_enabled);
		rval = -EAGAIN;
	}

iav_start_vout_exit:
	return rval;
}

int iav_halt_vout(iav_context_t *context, int vout_id)
{
	int	rval;

	rval = amba_vout_video_source_cmd(vout_id,
		AMBA_VIDEO_SOURCE_HALT, NULL);
	if (rval)
		goto iav_halt_vout_exit;

	rval = update_vout_info(context, vout_id);
	if (rval)
		goto iav_halt_vout_exit;

iav_halt_vout_exit:
	return rval;
}

int iav_select_output_dev(iav_context_t *context, int dev)
{
	int	rval;
	int	source_id;

	rval = amba_vout_video_sink_cmd(dev,
		AMBA_VIDEO_SINK_GET_SOURCE_ID, &source_id);
	if (rval)
		goto iav_select_output_dev_exit;

	if (source_id) {
		G_iav_info.pvoutinfo[1]->active_sink_id = dev;
	} else {
		G_iav_info.pvoutinfo[0]->active_sink_id = dev;
	}

iav_select_output_dev_exit:
	return rval;
}

int iav_configure_sink(iav_context_t *context,
	struct amba_video_sink_mode __user *pcfg)
{
	struct amba_video_sink_mode		sink_cfg, vout_mode;
	int					rval;
	int					source_id;

	if (copy_from_user(&sink_cfg, pcfg, sizeof(sink_cfg))) {
		rval = -EFAULT;
		goto iav_configure_sink_exit;
	}

	// check state
	if (iav_vout_cross_check(context, &sink_cfg) < 0) {
		iav_error("iav_configure_sink: iav_vout_cross_check error\n");
		rval = -EAGAIN;
		goto iav_configure_sink_exit;
	}

	if (sink_cfg.id == -1)
		sink_cfg.id = G_iav_info.pvoutinfo[1]->active_sink_id;
	rval = amba_vout_video_sink_cmd(sink_cfg.id,
		AMBA_VIDEO_SINK_GET_SOURCE_ID, &source_id);
	if (rval)
		goto iav_configure_sink_exit;

	rval = stop_vout(context, source_id);
	if (rval)
		goto iav_configure_sink_exit;

	memcpy(&vout_mode, &sink_cfg, sizeof(vout_mode));
	rval = amba_vout_video_sink_cmd(sink_cfg.id,
		AMBA_VIDEO_SINK_SET_MODE, &vout_mode);
	if (rval)
		goto iav_configure_sink_exit;
	if (source_id) {
		G_iav_info.pvoutinfo[1]->active_mode = sink_cfg;
		G_iav_info.pvoutinfo[1]->active_mode.direct_to_dsp = 0;
		G_iav_info.pvoutinfo[1]->active_sink_id = sink_cfg.id;
	} else {
		G_iav_info.pvoutinfo[0]->active_mode = sink_cfg;
		G_iav_info.pvoutinfo[1]->active_mode.direct_to_dsp = 0;
		G_iav_info.pvoutinfo[0]->active_sink_id = sink_cfg.id;
	}

	rval = amba_vout_video_source_cmd(source_id,
		AMBA_VIDEO_SOURCE_SET_DISPLAY_INPUT, &sink_cfg.display_input);
	if (rval)
		goto iav_configure_sink_exit;

	rval = amba_vout_video_source_cmd(source_id,
		AMBA_VIDEO_SOURCE_SET_BG_COLOR, &sink_cfg.bg_color);
	if (rval)
		goto iav_configure_sink_exit;

	rval = start_vout(context, source_id);

	//Send cmds to dsp directly if needed
	if (pcfg->direct_to_dsp) {
		u32				flag = VOUT_FLAGS_ALL;

		amba_vout_video_source_cmd(source_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

	}

	iav_vout_update_sink_cfg(context, &sink_cfg);

iav_configure_sink_exit:
	return rval;
}

int iav_enable_vout_csc(iav_context_t *context, struct iav_vout_enable_csc_s * arg)
{
	int				rval;
	struct iav_vout_enable_csc_s	vout_enable_csc;
	u32				flag;

	if (copy_from_user(&vout_enable_csc, arg,
		sizeof(struct iav_vout_enable_csc_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_csc_exit;
	}

	if (vout_enable_csc.vout_id < 0 || vout_enable_csc.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_csc_exit;
	}

	rval = amba_osd_on_csc_change(vout_enable_csc.vout_id,
		vout_enable_csc.csc_en);
	if (rval)
		goto iav_enable_vout_csc_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_CSC_SETUP |
			AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_csc.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_enable_vout_csc_exit:
	return rval;
}

int iav_enable_vout_video(iav_context_t *context,
	struct iav_vout_enable_video_s * arg)
{
	int				rval;
	VOUT_VIDEO_SETUP_CMD		video_setup;
	struct iav_vout_enable_video_s	vout_enable_video;
	u32				flag;

	if (copy_from_user(&vout_enable_video, arg,
		sizeof(struct iav_vout_enable_video_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_video_exit;
	}

	if (vout_enable_video.vout_id < 0 || vout_enable_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	video_setup.en = vout_enable_video.video_en;
	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_enable_vout_video_exit:
	return rval;
}

int iav_flip_vout_video(iav_context_t *context,
	struct iav_vout_flip_video_s * arg)
{
	int				rval;
	VOUT_VIDEO_SETUP_CMD		video_setup;
	struct iav_vout_flip_video_s	vout_flip_video;
	u32				flag;

	if (copy_from_user(&vout_flip_video, arg,
		sizeof(struct iav_vout_flip_video_s))) {
		rval = -EFAULT;
		goto iav_flip_vout_video_exit;
	}

	if (vout_flip_video.vout_id < 0 || vout_flip_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_flip_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_flip_vout_video_exit;

	video_setup.flip = vout_flip_video.flip;
	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_flip_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_flip_vout_video_exit;

	G_iav_info.pvoutinfo[vout_flip_video.vout_id]->active_mode.video_flip
		= vout_flip_video.flip;

iav_flip_vout_video_exit:
	return rval;
}

int iav_rotate_vout_video(iav_context_t *context,
	struct iav_vout_rotate_video_s * arg)
{
	int				rval;
	VOUT_VIDEO_SETUP_CMD		video_setup;
	struct iav_vout_rotate_video_s	vout_rotate_video;
	u32				flag;

	if (copy_from_user(&vout_rotate_video, arg,
		sizeof(struct iav_vout_rotate_video_s))) {
		rval = -EFAULT;
		goto iav_rotate_vout_video_exit;
	}

	if (vout_rotate_video.vout_id < 0 || vout_rotate_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_rotate_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_rotate_vout_video_exit;

	video_setup.rotate = vout_rotate_video.rotate;
	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_rotate_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_rotate_vout_video_exit;

	G_iav_info.pvoutinfo[vout_rotate_video.vout_id]->active_mode.video_rotate
		= vout_rotate_video.rotate;

iav_rotate_vout_video_exit:
	return rval;
}

int iav_change_vout_video_size(iav_context_t *context,
	struct iav_vout_change_video_size_s *arg)
{
	int					rval;
	VOUT_VIDEO_SETUP_CMD			video_setup;
	struct iav_vout_change_video_size_s	cvs;
	VCAP_PREV_SETUP_CMD			video_preview;
	u32					flag;
	int					video_en;

	if (copy_from_user(&cvs, arg,
		sizeof(struct iav_vout_change_video_size_s))) {
		rval = -EFAULT;
		goto iav_change_vout_video_size_exit;
	}

	if (cvs.vout_id < 0 || cvs.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_video_size_exit;
	}

	/* Change video size and disable video temporarily */
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	video_en = video_setup.en;
	video_setup.en = 0;
	video_setup.win_width = cvs.width;
	video_setup.win_height = cvs.height;
	if (G_iav_info.pvoutinfo[cvs.vout_id]->video_info.format
		== AMBA_VIDEO_FORMAT_INTERLACE)
		video_setup.win_height >>= 1;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

	/* Chage preview video size */
	memset(&video_preview, 0, sizeof(VCAP_PREV_SETUP_CMD));
	video_preview.cmd_code = CMD_VCAP_PREV_SETUP;
	video_preview.preview_id = cvs.vout_id;
//	video_preview.preview_en = 1;
	video_preview.preview_w = cvs.width;
	video_preview.preview_h = cvs.height;
	video_preview.preview_format = amba_iav_format_to_format(G_iav_info.pvoutinfo[cvs.vout_id]->video_info.format);
	video_preview.preview_frame_rate = amba_iav_fps_to_fps(G_iav_info.pvoutinfo[cvs.vout_id]->video_info.fps);
	dsp_issue_cmd(&video_preview, sizeof(VCAP_PREV_SETUP_CMD));

	/* Enable video again if enabled before */
	if (!video_en)
		goto iav_change_vout_video_size_exit;

	msleep(100);
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	video_setup.en = video_en;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_change_vout_video_size_exit:
	return rval;
}

int iav_change_vout_video_offset(iav_context_t *context,
	struct iav_vout_change_video_offset_s *arg)
{
	int					rval;
	VOUT_VIDEO_SETUP_CMD			video_setup;
	struct iav_vout_change_video_offset_s	cvo;
	struct amba_vout_window_info		active_window;
	int					height;
	u32					flag;

	if (copy_from_user(&cvo, arg,
		sizeof(struct iav_vout_change_video_offset_s))) {
		rval = -EFAULT;
		goto iav_change_vout_video_offset_exit;
	}

	if (cvo.vout_id < 0 || cvo.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_video_offset_exit;
	}

	/* Change video size and disable video temporarily */
	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN, &active_window);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	height = active_window.end_y - active_window.start_y + 1;
	if (cvo.specified) {
		video_setup.win_offset_x = cvo.offset_x;
		video_setup.win_offset_y = cvo.offset_y;
		if (G_iav_info.pvoutinfo[cvo.vout_id]->video_info.format
			== AMBA_VIDEO_FORMAT_INTERLACE)
			video_setup.win_offset_y >>= 1;

		if (video_setup.win_offset_x + video_setup.win_width > active_window.width)
			video_setup.win_offset_x = active_window.width - video_setup.win_width;
		if (video_setup.win_offset_y + video_setup.win_height > height)
			video_setup.win_offset_y = height - video_setup.win_height;
	} else {
		video_setup.win_offset_x = (active_window.width - video_setup.win_width) >> 1;
		video_setup.win_offset_y = (height - video_setup.win_height) >> 1;
	}

	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(cvo.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_change_vout_video_offset_exit:
	return rval;
}

int iav_select_vout_fb(iav_context_t *context, struct iav_vout_fb_sel_s * arg)
{
	int				rval;
	struct iav_vout_fb_sel_s	vout_fb_sel;
	u32				flag;

	if (copy_from_user(&vout_fb_sel, arg, sizeof(struct iav_vout_fb_sel_s))) {
		rval = -EFAULT;
		goto iav_select_vout_fb_exit;
	}

	if (vout_fb_sel.vout_id < 0 || vout_fb_sel.vout_id > 1) {
		rval = -EPERM;
		goto iav_select_vout_fb_exit;
	}

	rval = amba_osd_on_fb_switch(vout_fb_sel.vout_id, vout_fb_sel.fb_id);
	if (rval)
		goto iav_select_vout_fb_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_fb_sel.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_select_vout_fb_exit:
	return rval;
}

int iav_enable_vout_osd_rescaler(iav_context_t *context,
	struct iav_vout_enable_osd_rescaler_s * arg)
{
	int					rval;
	struct iav_vout_enable_osd_rescaler_s	vout_enable_rescaler;
	u32					flag;

	if (copy_from_user(&vout_enable_rescaler, arg,
		sizeof(struct iav_vout_enable_osd_rescaler_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_osd_rescaler_exit;
	}

	if (vout_enable_rescaler.vout_id < 0
		|| vout_enable_rescaler.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_osd_rescaler_exit;
	}

	rval = amba_osd_on_rescaler_change(vout_enable_rescaler.vout_id,
			vout_enable_rescaler.enable, vout_enable_rescaler.width,
			vout_enable_rescaler.height);
	if (rval)
		goto iav_enable_vout_osd_rescaler_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_rescaler.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_enable_vout_osd_rescaler_exit:
	return rval;
}

int iav_change_vout_osd_offset(iav_context_t *context,
	struct iav_vout_change_osd_offset_s *arg)
{
	int					rval;
	struct iav_vout_change_osd_offset_s	coo;
	u32					flag;

	if (copy_from_user(&coo, arg,
		sizeof(struct iav_vout_change_osd_offset_s))) {
		rval = -EFAULT;
		goto iav_change_vout_osd_offset_exit;
	}

	if (coo.vout_id < 0 || coo.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_osd_offset_exit;
	}

	rval = amba_osd_on_offset_change(coo.vout_id, coo.specified,
		coo.offset_x, coo.offset_y);
	if (rval)
		goto iav_change_vout_osd_offset_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(coo.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

iav_change_vout_osd_offset_exit:
	return rval;
}

void iav_config_vout(iav_context_t *context, int vout_mask, VOUT_SRC vout_src)
{
	config_vout(context, vout_mask, vout_src, VOUT_FLAGS_ALL);
}

void iav_change_vout_src(iav_context_t *context, VOUT_SRC vout_src)
{
	config_vout(context, -1, vout_src | (vout_src << 16), VOUT_UPDATE_VIDEO);
}

void iav_config_vout_osd(iav_context_t *context)
{
	int i;
	u32 flag = VOUT_UPDATE_OSD;

	for (i = 0; i < 2; i++) {
		amba_vout_video_source_cmd(i,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	}
}

//for decoding
void iav_vout_set_irq_handler(void (*handler)(unsigned))
{
	unsigned long flags;
	dsp_lock(flags);
	G_iav_vout_irq_handler = handler;
	dsp_unlock(flags);
}

iav_vout_preproc G_vout_preproc;

// set preproc when entering a mode
// set to NULL when leaving the mode
void iav_vout_set_preproc(iav_vout_preproc preproc)
{
	G_vout_preproc = preproc;
}

vout_default_info_t *iav_vout_default_info(unsigned vout_id)
{
	vout_default_info_t *default_info = G_iav_vout_info[vout_id].default_info;
	invalidate_d_cache(default_info, sizeof(*default_info));
	return default_info;
}

static irqreturn_t iav_vout_irq(int irqno, void *dev_id)
{
	unsigned long flags;

	dsp_lock(flags);

	if (G_iav_vout_irq_handler) {
		if (irqno == VOUT_IRQ)
			G_iav_vout_irq_handler(0);
		//else if (irqno == ORC_VOUT1_IRQ)
		//	G_iav_vout_irq_handler(1);
	}

	dsp_unlock(flags);

	return IRQ_HANDLED;
}

int iav_init_vout_irq(void *dev)
{
	int rval;

	rval = request_irq(VOUT_IRQ, iav_vout_irq,
		IRQF_TRIGGER_RISING | IRQF_SHARED, "decoder", dev);
	if (rval < 0) {
		LOG_ERROR("iav request vout irq failed %d\n", rval);
		return rval;
	}

/*
	rval = request_irq(ORC_VOUT1_IRQ, iav_vout_irq,
		IRQF_TRIGGER_RISING | IRQF_SHARED, "decoder", dev);
	if (rval < 0) {
		LOG_ERROR("iav request vout irq failed %d\n", rval);
		return rval;
	}
*/

	return 0;
}

int iav_update_vout_video_src_ar(u8 ar)
{
	return amba_vout_video_source_cmd(1,
			AMBA_VIDEO_SOURCE_SET_VIDEO_AR, (void *)&ar);
}

void iav_change_vout_src_ex(iav_context_t *context, int vout_mask, VOUT_SRC vout_src)
{
	config_vout(context, vout_mask, vout_src | (vout_src << 16), VOUT_UPDATE_VIDEO);
}

int __iav_enable_vout_video(iav_context_t *context,
	struct iav_vout_enable_video_s *vout_enable_video)
{
	int				rval;
	VOUT_VIDEO_SETUP_CMD		video_setup;
	u32				flag;

	if (vout_enable_video->vout_id < 0 || vout_enable_video->vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_enable_video->vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	video_setup.en = vout_enable_video->video_en;
	rval = amba_vout_video_source_cmd(vout_enable_video->vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_video->vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_enable_vout_video_exit;

	G_iav_info.pvoutinfo[vout_enable_video->vout_id]->active_mode.video_en
		= vout_enable_video->video_en;

iav_enable_vout_video_exit:
	return rval;
}

void iav_set_vout_src(iav_context_t *context, unsigned vout_id, VOUT_SRC vout_src)
{
	VOUT_VIDEO_SETUP_CMD def_vout_data;
	u32 flag = VOUT_UPDATE_VIDEO;

	amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
				&def_vout_data);

	def_vout_data.en = 1;
	def_vout_data.src = vout_src;
	if (vout_src == VOUT_SRC_DEFAULT_IMG) {
		set_default_info(&def_vout_data, vout_id);
	}

	amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
		&def_vout_data);
	amba_vout_video_source_cmd(vout_id,
		AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

	G_iav_info.pvoutinfo[vout_id]->active_mode.video_en = 1;
}

