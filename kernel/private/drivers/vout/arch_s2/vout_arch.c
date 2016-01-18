/*
 * kernel/private/drivers/ambarella/vout/arch_s2/vout_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_dsp.h>

#include "../vout_pri.h"
#include "dsp_cmd.h"
#include "utils.h"

/* ========================================================================== */
struct amba_vout_dev_info {
	unsigned int			irq;
	struct __amba_vout_video_source	video_source;
	irq_callback_t			*irq_callback;
	vout_dsp_cmd			*pdsp_cmd_fn;
	spinlock_t			lock;

	u32				irq_counter;
	struct proc_dir_entry		*irq_proc_file;
	struct ambsync_proc_hinfo	irq_proc_hinfo;

	u32				irq_wait_reset_counter;
	u32				irq_wait_reset_flag;
	wait_queue_head_t		irq_wait_reset;

	wait_queue_head_t		osd_update_wq;
	spinlock_t			osd_update_lock;
	u32				osd_update_wait_num;
	u32				osd_update_timeout;

	struct amb_async_proc_info	event_proc;
	struct amb_event_pool		event_pool;

	enum amba_video_source_status	pstatus;	/* Status before suspension */
	enum amba_video_source_status	status;
	struct amba_video_info		video_info;

	VOUT_MIXER_SETUP_CMD		mixer_setup;
	VOUT_VIDEO_SETUP_CMD		video_setup;
	VOUT_DEFAULT_IMG_SETUP_CMD	default_img_setup;
	u32				osd_setup_flag;
	VOUT_OSD_SETUP_CMD		osd_setup;
	VOUT_OSD_BUF_SETUP_CMD		osd_buf_setup;
	u32				osd_clut_setup_flag;
	VOUT_OSD_CLUT_SETUP_CMD		osd_clut_setup;
	VOUT_DISPLAY_SETUP_CMD		display_setup;
	VOUT_DVE_SETUP_CMD		dve_setup;
	VOUT_RESET_CMD			reset;
	u32				csc_setup_flag;
	VOUT_DISPLAY_CSC_SETUP_CMD	csc_setup;
	u32				dram_dve_flag;
	dram_dve_t			dram_dve;
	dram_display_t			dram_display;
	u32				dram_clut[AMBA_VOUT_CLUT_SIZE];
	dram_osd_t			dram_osd;
};

struct amba_vout_system_instance_info {
	struct amba_vout_dev_info	*pvout_source;
	dma_addr_t			dma_address;
	int				source_id;
	u32				source_type;
	const char			name[AMBA_VOUT_NAME_LENGTH];
	struct resource			irq;
};

/* ========================================================================== */
static struct amba_vout_system_instance_info vout0_instance = {
	.pvout_source	= NULL,
	.source_id	= AMBA_VOUT_SOURCE_STARTING_ID,
	.source_type	= AMBA_VOUT_SOURCE_TYPE_DIGITAL,
	.name		= "vout0",
	.irq		= {
		.start	= VOUT_IRQ,
		.end	= VOUT_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct amba_vout_system_instance_info vout1_instance = {
	.pvout_source	= NULL,
	.source_id	= AMBA_VOUT_SOURCE_STARTING_ID + 1,
	.source_type	= (AMBA_VOUT_SOURCE_TYPE_DVE |
				AMBA_VOUT_SOURCE_TYPE_HDMI),
	.name		= "vout1",
	.irq		= {
		.start	= ORC_VOUT0_IRQ,
		.end	= ORC_VOUT0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static void __amba_s2_vout_reset(struct amba_vout_dev_info *pinfo);

/* ========================================================================== */
#define CACHE_LINE_SIZE		32
inline void clean_cache_aligned(u8 *start, unsigned long size)
{
	unsigned long offset = (unsigned long)start & (CACHE_LINE_SIZE - 1);

	start -= offset;
	size += offset;

	clean_d_cache(start, size);
}

static int amba_s2_update_iav_info(struct __amba_vout_video_source *psrc,
	struct amba_video_source_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	args->enabled = pinfo->status;
	args->video_info = pinfo->video_info;

	return errorCode;
}

static int amba_s2_vout_reset(struct __amba_vout_video_source *psrc, u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	pinfo->reset.reset_mixer = 1;
	pinfo->reset.reset_disp = 1;
	pinfo->osd_setup_flag = AMBA_VOUT_SETUP_NA;
	pinfo->osd_clut_setup_flag = AMBA_VOUT_SETUP_NA;
	pinfo->dram_dve_flag = AMBA_VOUT_SETUP_NA;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_RUNNING) {
		pinfo->status = AMBA_VIDEO_SOURCE_STATUS_IDLE;
		vout_dbg("%s: switch to %d!\n", __func__, pinfo->status);
	} else {
		vout_dbg("%s already stopped %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_run(struct __amba_vout_video_source *psrc, u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->status = AMBA_VIDEO_SOURCE_STATUS_RUNNING;
		vout_dbg("%s: switch to %d!\n", __func__, pinfo->status);
	} else {
		vout_dbg("%s already running %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_halt(struct __amba_vout_video_source *psrc, u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	pinfo->reset.reset_mixer = 1;
	pinfo->reset.reset_disp = 1;
	pinfo->osd_setup_flag = AMBA_VOUT_SETUP_NA;
	pinfo->osd_clut_setup_flag = AMBA_VOUT_SETUP_NA;
	pinfo->dram_dve_flag = AMBA_VOUT_SETUP_NA;

	if (pinfo->pdsp_cmd_fn && pinfo->status == AMBA_VIDEO_SOURCE_STATUS_RUNNING) {
		__amba_s2_vout_reset(pinfo);
	}

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_RUNNING) {
		pinfo->status = AMBA_VIDEO_SOURCE_STATUS_IDLE;
		vout_dbg("%s: switch to %d!\n", __func__, pinfo->status);
	} else {
		vout_dbg("%s already stopped %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_get_config(struct __amba_vout_video_source *psrc,
	vd_config_t *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	args->d_control = pinfo->dram_display.d_control;
	args->d_digital_output_mode = pinfo->dram_display.d_digital_output_mode;
	args->d_analog_output_mode = pinfo->dram_display.d_analog_output_mode;
	args->d_hdmi_output_mode = pinfo->dram_display.d_hdmi_output_mode;

	return errorCode;
}

static int amba_s2_vout_set_config(struct __amba_vout_video_source *psrc,
	vd_config_t *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->dram_display.d_control = args->d_control;
		pinfo->dram_display.d_digital_output_mode =
			args->d_digital_output_mode;
		pinfo->dram_display.d_analog_output_mode =
			args->d_analog_output_mode;
		pinfo->dram_display.d_hdmi_output_mode =
			args->d_hdmi_output_mode;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_set_video_size(struct __amba_vout_video_source *psrc,
	struct amba_vout_window_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->video_setup.win_offset_x = args->start_x;
		pinfo->video_setup.win_offset_y = args->start_y;
		pinfo->video_setup.win_width = args->end_x - args->start_x + 1;
		pinfo->video_setup.win_height = args->end_y - args->start_y + 1;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_get_active_win(struct __amba_vout_video_source *psrc,
	struct amba_vout_window_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	args->start_x = pinfo->dram_display.d_active_region_start_0.s.startcol;
	args->start_y = pinfo->dram_display.d_active_region_start_0.s.startrow;
	args->end_x = pinfo->dram_display.d_active_region_end_0.s.endcol;
	args->end_y = pinfo->dram_display.d_active_region_end_0.s.endrow;
	args->width = pinfo->mixer_setup.act_win_width;

	return errorCode;
}

static int amba_s2_vout_set_active_win(struct __amba_vout_video_source *psrc,
	struct amba_vout_window_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->mixer_setup.act_win_width = args->width;
		pinfo->mixer_setup.act_win_height =
			args->end_y - args->start_y + 1;

		pinfo->dram_display.d_active_region_start_0.s.startrow =
			args->start_y;
		pinfo->dram_display.d_active_region_start_0.s.startcol =
			args->start_x;
		pinfo->dram_display.d_active_region_end_0.s.endrow =
			args->end_y;
		pinfo->dram_display.d_active_region_end_0.s.endcol =
			args->end_x;

		pinfo->dram_display.d_active_region_start_1.s.startrow =
			args->start_y + 1;
		pinfo->dram_display.d_active_region_start_1.s.startcol =
			args->start_x;
		pinfo->dram_display.d_active_region_end_1.s.endrow =
			args->end_y + 1;
		pinfo->dram_display.d_active_region_end_1.s.endcol =
			args->end_x;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_set_hvsync(struct __amba_vout_video_source *psrc,
	struct amba_vout_hv_sync_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		switch (args->sink_type) {
		case AMBA_VOUT_SINK_TYPE_DIGITAL:
		case AMBA_VOUT_SINK_TYPE_MIPI:
			pinfo->dram_display.d_digital_hsync_control.s.hsyncstart
				= args->hsync_start;
			pinfo->dram_display.d_digital_hsync_control.s.hsyncend
				= args->hsync_end;

			pinfo->dram_display.d_digital_vsync_start_0.s.vsyncstart_row
				= args->vtsync_start_row;
			pinfo->dram_display.d_digital_vsync_start_0.s.vsyncstart_column
				= args->vtsync_start_col;
			pinfo->dram_display.d_digital_vsync_end_0.s.vsyncend_row
				= args->vtsync_end_row;
			pinfo->dram_display.d_digital_vsync_end_0.s.vsyncend_column
				= args->vtsync_end_col;

			pinfo->dram_display.d_digital_vsync_start_1.s.vsyncstart_row
				= args->vbsync_start_row;
			pinfo->dram_display.d_digital_vsync_start_1.s.vsyncstart_column
				= args->vbsync_start_col;
			pinfo->dram_display.d_digital_vsync_end_1.s.vsyncend_row
				= args->vbsync_end_row;
			pinfo->dram_display.d_digital_vsync_end_1.s.vsyncend_column
				= args->vbsync_end_col;
			break;

		case AMBA_VOUT_SINK_TYPE_CVBS:
		case AMBA_VOUT_SINK_TYPE_SVIDEO:
		case AMBA_VOUT_SINK_TYPE_YPBPR:
			pinfo->dram_display.d_analog_hsync_control.s.hsyncstart
				= args->hsync_start;
			pinfo->dram_display.d_analog_hsync_control.s.hsyncend
				= args->hsync_end;

			pinfo->dram_display.d_analog_vsync_start_0.s.vsyncstart_row
				= args->vtsync_start_row;
			pinfo->dram_display.d_analog_vsync_start_0.s.vsyncstart_column
				= args->vtsync_start_col;
			pinfo->dram_display.d_analog_vsync_end_0.s.vsyncend_row
				= args->vtsync_end_row;
			pinfo->dram_display.d_analog_vsync_end_0.s.vsyncend_column
				= args->vtsync_end_col;

			pinfo->dram_display.d_analog_vsync_start_1.s.vsyncstart_row
				= args->vbsync_start_row;
			pinfo->dram_display.d_analog_vsync_start_1.s.vsyncstart_column
				= args->vbsync_start_col;
			pinfo->dram_display.d_analog_vsync_end_1.s.vsyncend_row
				= args->vbsync_end_row;
			pinfo->dram_display.d_analog_vsync_end_1.s.vsyncend_column
				= args->vbsync_end_col;
			break;

		case AMBA_VOUT_SINK_TYPE_HDMI:
			pinfo->dram_display.d_hdmi_hsync_control.s.hsyncstart
				= args->hsync_start;
			pinfo->dram_display.d_hdmi_hsync_control.s.hsyncend
				= args->hsync_end;

			pinfo->dram_display.d_hdmi_vsync_start_0.s.vsyncstart_row
				= args->vtsync_start_row;
			pinfo->dram_display.d_hdmi_vsync_start_0.s.vsyncstart_column
				= args->vtsync_start_col;
			pinfo->dram_display.d_hdmi_vsync_end_0.s.vsyncend_row
				= args->vtsync_end_row;
			pinfo->dram_display.d_hdmi_vsync_end_0.s.vsyncend_column
				= args->vtsync_end_col;

			pinfo->dram_display.d_hdmi_vsync_start_1.s.vsyncstart_row
				= args->vbsync_start_row;
			pinfo->dram_display.d_hdmi_vsync_start_1.s.vsyncstart_column
				= args->vbsync_start_col;
			pinfo->dram_display.d_hdmi_vsync_end_1.s.vsyncend_row
				= args->vbsync_end_row;
			pinfo->dram_display.d_hdmi_vsync_end_1.s.vsyncend_column
				= args->vbsync_end_col;
			break;

		default:
			break;
		}
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_set_hv(struct __amba_vout_video_source *psrc,
	struct amba_vout_hv_size_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->dram_display.d_frame_size.s.h = args->hsize - 1;
		pinfo->dram_display.d_frame_size.s.v = args->vtsize - 1;
		pinfo->dram_display.d_frame_height_field_1.s.v =
			args->vbsize - 1;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_scale_sd_analog_out(
	struct __amba_vout_video_source *psrc,
	struct amba_video_source_scale_analog_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->dram_display.d_analog_sd_scale_y.s.scale_en = 1;
		pinfo->dram_display.d_analog_sd_scale_y.s.y = args->y_coeff;
		pinfo->dram_display.d_analog_sd_scale_pbpr.s.pb = args->pb_coeff;
		pinfo->dram_display.d_analog_sd_scale_pbpr.s.pr = args->pr_coeff;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_set_vbi(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *psink_mode)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		if (psink_mode->ratio == AMBA_VIDEO_RATIO_4_3) {
			pinfo->dram_display.d_analog_vbi_control.s.cpb = 0;
			goto amba_s2_vout_set_vbi_exit;
		}

		if ((psink_mode->mode == AMBA_VIDEO_MODE_480I) ||
			(psink_mode->mode == AMBA_VIDEO_MODE_D1_NTSC)) {

			/* for 480P */
			pinfo->dram_display.d_analog_vbi_start_v.s.start_row_0
				= 40 - 6;
			pinfo->dram_display.d_analog_vbi_start_v.s.start_row_1
				= 40 - 6;
			pinfo->dram_display.d_analog_vbi_h.s.start_col
				= 156 - 1;
			pinfo->dram_display.d_analog_vbi_control.s.cpb
				= 26;
			pinfo->dram_display.d_analog_vbi_control.s.sd_comp
				= HD_CVBS_SD;

			if (psink_mode->mode == AMBA_VIDEO_MODE_480I) {
				pinfo->dram_display.d_analog_vbi_start_v.s.start_row_0
					= 20;
				pinfo->dram_display.d_analog_vbi_start_v.s.start_row_1
					= 20;
				pinfo->dram_display.d_analog_vbi_h.s.start_col
					= 375;
				pinfo->dram_display.d_analog_vbi_control.s.cpb
					= 60;

				if (psink_mode->sink_type ==
					AMBA_VOUT_SINK_TYPE_YPBPR) {
					pinfo->dram_display.d_analog_vbi_control.s.sd_comp
						= COMP_SD;
					pinfo->dram_display.d_analog_vbi_h.s.end_col
						= 1701;
				} else {
					pinfo->dram_display.d_analog_vbi_control.s.sd_comp
						= HD_CVBS_SD;
					pinfo->dram_display.d_analog_vbi_h.s.end_col
						= 0;
				}
			}
			pinfo->dram_display.d_analog_vbi_data0 = 0x00180005;

		} else if ((psink_mode->mode == AMBA_VIDEO_MODE_576I) ||
			(psink_mode->mode == AMBA_VIDEO_MODE_D1_PAL)) {
			pinfo->dram_display.d_analog_vbi_start_v.s.start_row_0
				= 42;
			pinfo->dram_display.d_analog_vbi_start_v.s.start_row_1
				= 42;
			pinfo->dram_display.d_analog_vbi_h.s.start_col
				= 148;
			pinfo->dram_display.d_analog_vbi_control.s.cpb
				= 1;
			pinfo->dram_display.d_analog_vbi_control.s.sd_comp = HD_CVBS_SD;

			if (psink_mode->mode == AMBA_VIDEO_MODE_576I) {
				pinfo->dram_display.d_analog_vbi_start_v.s.start_row_0
					= 26;
				pinfo->dram_display.d_analog_vbi_start_v.s.start_row_1
					= 26;
				pinfo->dram_display.d_analog_vbi_h.s.start_col
					= 367;
				pinfo->dram_display.d_analog_vbi_control.s.cpb
					= 2;

				if (psink_mode->sink_type ==
					AMBA_VOUT_SINK_TYPE_YPBPR) {
					pinfo->dram_display.d_analog_vbi_control.s.sd_comp
						= COMP_SD;
					pinfo->dram_display.d_analog_vbi_h.s.end_col
						= 1112;
				} else {
					pinfo->dram_display.d_analog_vbi_control.s.sd_comp
						= HD_CVBS_SD;
					pinfo->dram_display.d_analog_vbi_h.s.end_col
						= 248;
				}
			}

			pinfo->dram_display.d_analog_vbi_data0 = 0x3fc03fff;
			pinfo->dram_display.d_analog_vbi_data1 = 0x3fc03fc0;
			pinfo->dram_display.d_analog_vbi_data2 = 0xffc03fc0;
			pinfo->dram_display.d_analog_vbi_data3 = 0x000ffe01;
			pinfo->dram_display.d_analog_vbi_data4 = 0x00fffffc;
			pinfo->dram_display.d_analog_vbi_data5 = 0x00ff00ff;
			pinfo->dram_display.d_analog_vbi_data6 = 0xff00ff00;
			pinfo->dram_display.d_analog_vbi_data7 = 0xff00ff00;
			pinfo->dram_display.d_analog_vbi_data8 = 0xff00ff00;
			pinfo->dram_display.d_analog_vbi_data9 = 0xfc03fc00;
			pinfo->dram_display.d_analog_vbi_data10 = 0xfc03fc03;
			pinfo->dram_display.d_analog_vbi_data11 = 0x0003fc03;
		}
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

amba_s2_vout_set_vbi_exit:
	return errorCode;
}

static int amba_s2_vout_select_display_input(struct __amba_vout_video_source *psrc,
	enum amba_vout_display_input *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	pinfo->dram_display.d_input_stream_enable.s.input_select = *args;

	return errorCode;
}

static int amba_s2_vout_get_video_info(struct __amba_vout_video_source *psrc,
	struct amba_video_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	*args = pinfo->video_info;

	return errorCode;
}

static int amba_s2_vout_set_video_info(struct __amba_vout_video_source *psrc,
	struct amba_video_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->video_info = *args;

		pinfo->mixer_setup.interlaced =
			amba_iav_format_to_format(args->format);
		pinfo->mixer_setup.frm_rate = amba_iav_fps_to_fps(args->fps);
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_get_osd(struct __amba_vout_video_source *psrc,
	VOUT_OSD_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;
	*args = pinfo->osd_setup;

	return errorCode;
}

static int amba_s2_vout_set_osd(struct __amba_vout_video_source *psrc,
	VOUT_OSD_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	pinfo->osd_setup.en = args->en;
	pinfo->osd_setup.src = args->src;
	pinfo->osd_setup.flip = args->flip;
	pinfo->osd_setup.rescaler_en = args->rescaler_en;
	pinfo->osd_setup.premultiplied = args->premultiplied;
	pinfo->osd_setup.global_blend = args->global_blend;
	pinfo->osd_setup.win_offset_x = args->win_offset_x;
	pinfo->osd_setup.win_offset_y = args->win_offset_y;
	pinfo->osd_setup.win_width = args->win_width;
	pinfo->osd_setup.win_height = args->win_height;
	pinfo->osd_setup.rescaler_input_width =
		args->rescaler_input_width;
	pinfo->osd_setup.rescaler_input_height =
		args->rescaler_input_height;
	pinfo->osd_setup.osd_buf_dram_addr = args->osd_buf_dram_addr;
	pinfo->osd_setup.osd_buf_pitch = args->osd_buf_pitch;
	pinfo->osd_setup.osd_buf_repeat_field =
		args->osd_buf_repeat_field;
	pinfo->osd_setup.osd_direct_mode = args->osd_direct_mode;
	pinfo->osd_setup.osd_transparent_color =
		args->osd_transparent_color;
	pinfo->osd_setup.osd_transparent_color_en =
		args->osd_transparent_color_en;
	pinfo->osd_setup_flag = AMBA_VOUT_SETUP_NEW;
	pinfo->osd_clut_setup_flag = AMBA_VOUT_SETUP_NA;

	return errorCode;
}

static int amba_s2_vout_get_osd_buf(struct __amba_vout_video_source *psrc,
	VOUT_OSD_BUF_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;
	*args = pinfo->osd_buf_setup;

	return errorCode;
}

static int amba_s2_vout_set_osd_buf(struct __amba_vout_video_source *psrc,
	VOUT_OSD_BUF_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;
	unsigned long				flags;

	pinfo = psrc->pinfo;

	pinfo->osd_buf_setup.osd_buf_dram_addr = args->osd_buf_dram_addr;
	pinfo->osd_buf_setup.osd_buf_pitch = args->osd_buf_pitch;
	pinfo->osd_buf_setup.osd_buf_repeat_field = args->osd_buf_repeat_field;

	pinfo->dram_osd.osd_buf_dram_addr = args->osd_buf_dram_addr;
	pinfo->dram_osd.osd_buf_pitch = args->osd_buf_pitch;
	pinfo->dram_osd.osd_buf_repeat_field = args->osd_buf_repeat_field;
	clean_cache_aligned((u8 *)&pinfo->dram_osd, sizeof(pinfo->dram_osd));

	spin_lock_irqsave(&pinfo->osd_update_lock, flags);
	pinfo->osd_update_wait_num = 1;
	spin_unlock_irqrestore(&pinfo->osd_update_lock, flags);
	if (!wait_event_interruptible_timeout(pinfo->osd_update_wq, pinfo->osd_update_wait_num == 0, HZ / 10)) {
		spin_lock_irqsave(&pinfo->osd_update_lock, flags);
		pinfo->osd_update_timeout = 1;
		spin_unlock_irqrestore(&pinfo->osd_update_lock, flags);
		DRV_PRINT("Wait for vout%d irq timeout, it might die!\n", psrc->id);
	}

	return errorCode;
}

static int amba_s2_vout_set_osd_clut(struct __amba_vout_video_source *psrc,
	struct amba_video_source_osd_clut_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;
	u32					i;
	u8					*pclut_table;

	pinfo = psrc->pinfo;
	pclut_table = args->pclut_table;

	for (i = 0; i < AMBA_VOUT_CLUT_SIZE; i++) {
		pinfo->dram_clut[i] = ((args->pblend_table[i] & 0xff) << 24) +
		        ((pclut_table[1] & 0xff) << 16) +
			((pclut_table[2] & 0xff) << 8) +
			 (pclut_table[0] & 0xff);

		vout_dbg("%s: dram_clut[%d] = 0x%08x!\n",
			__func__, i, pinfo->dram_clut[i]);
		pclut_table += 3;
	}
	pinfo->osd_clut_setup_flag = AMBA_VOUT_SETUP_NEW;

	return errorCode;
}

static int amba_s2_vout_get_setup(struct __amba_vout_video_source *psrc,
	VOUT_VIDEO_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	*args = pinfo->video_setup;

	return errorCode;
}

static int amba_s2_vout_set_setup(struct __amba_vout_video_source *psrc,
	VOUT_VIDEO_SETUP_CMD *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	pinfo->video_setup.en = args->en;
	pinfo->video_setup.src = args->src;
	pinfo->video_setup.flip = args->flip;
	pinfo->video_setup.rotate = args->rotate;
	pinfo->video_setup.win_offset_x = args->win_offset_x;
	pinfo->video_setup.win_offset_y = args->win_offset_y;
	pinfo->video_setup.win_width = args->win_width;
	pinfo->video_setup.win_height = args->win_height;
	pinfo->video_setup.default_img_y_addr =
		args->default_img_y_addr;
	pinfo->video_setup.default_img_uv_addr =
		args->default_img_uv_addr;
	pinfo->video_setup.default_img_pitch =
		args->default_img_pitch;
	pinfo->video_setup.default_img_repeat_field =
		args->default_img_repeat_field;

	return errorCode;
}

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
#else

typedef union {
	struct  {
		unsigned int	ctrl_reg	: 16;
		unsigned int	pump_current	: 8;
		unsigned int	reserved	: 8;
	};
	unsigned int w;
} hdmi_4k_ctrl2_reg_t ;

typedef union {
	struct  {
		unsigned int	vco_select	: 1;
		unsigned int	vco_range	: 2;
		unsigned int	clamp		: 2;
		unsigned int	dsm_enable	: 1;
		unsigned int	reserved1	: 1;
		unsigned int	dsm_gain	: 2;
		unsigned int	reserved2	: 3;
		unsigned int	reserved3	: 1;
		unsigned int	resistor	: 4;
		unsigned int	bias_current	: 3;
		unsigned int	bypass_jdiv	: 1;
		unsigned int	bypass_jout	: 1;
		unsigned int	reserved4	: 10;
	};
	unsigned int w;
} hdmi_4k_ctrl3_reg_t ;

typedef union {
	struct  {
		unsigned int	iref_ctat	: 8;
		unsigned int	iref_ptat	: 8;
		unsigned int	bg_ctrl		: 8;
		unsigned int	course_tune	: 8;
	};
	unsigned int w;
} hdmi_4k_ctrl4_reg_t ;

unsigned long amba_s2_hdmi_get_rate(struct clk *c)
{
	union ctrl_reg_u ctrl_reg;
	u32 pre_scaler_reg;
	u32 post_scaler_reg;
	u32 pll_int;
	u32 sdiv;
	u32 sout;
	u64 divident;
	u64 divider;
	u64 ref_clk;

	ctrl_reg.w = amba_readl(c->ctrl_reg);
	if ((ctrl_reg.s.power_down == 1) ||
		(ctrl_reg.s.halt_vco == 1)) {
		c->rate = 0;
		goto amba_s2_hdmi_get_rate_exit;
	}

	if (c->pres_reg != PLL_REG_UNAVAILABLE) {
		pre_scaler_reg = amba_readl(c->pres_reg);
	} else {
		pre_scaler_reg = 1;
	}
	if (c->post_reg != PLL_REG_UNAVAILABLE) {
		post_scaler_reg = amba_readl(c->post_reg);
	} else {
		post_scaler_reg = 1;
	}

	pll_int = ctrl_reg.s.intp;
	sdiv = ctrl_reg.s.sdiv;
	sout = ctrl_reg.s.sout;

	if (amba_tstbitsl(RCT_REG(0x008), 0x1)) {
		ref_clk = REF_CLK_FREQ;
	} else {
		ref_clk = c->parent->ops->get_rate(c->parent);
	}

	divident = (ref_clk * (pll_int + 1) * (sdiv + 1));
	divider = (pre_scaler_reg * (sout + 1) * post_scaler_reg);

	AMBCLK_DO_DIV(divident, divider);
	c->rate = divident;

amba_s2_hdmi_get_rate_exit:
	return c->rate;
}

int amba_s2_hdmi_auto_select_hdmi4k_range(void)
{
	hdmi_4k_ctrl2_reg_t	reg2;
	hdmi_4k_ctrl3_reg_t	reg3;
	hdmi_4k_ctrl4_reg_t	reg4;
	u32			status, lock = 0, low = 0xffff, high = 0xffff;
	u32			i, clamp;
	int			ret;

	reg2.w	= amba_readl(RCT_REG(0x330));
	amba_writel(RCT_REG(0x330), 0x00170000);

	reg3.w	= amba_readl(RCT_REG(0x334));
	clamp	= reg3.clamp;

	reg4.w	= amba_readl(RCT_REG(0x33c));

	for (i = 0; i < 16 ; i++) {
		reg4.course_tune = i;
		amba_writel(RCT_REG(0x33c), reg4.w);

		reg3.clamp = 2;
		amba_writel(RCT_REG(0x334), reg3.w);
		udelay(15);

		reg3.clamp = clamp;
		amba_writel(RCT_REG(0x334), reg3.w);
		msleep(10);

		status = amba_readl(RCT_REG(0x338)) & 0x001f0000;
		if (status == 0x001f0000) {
		    lock = 1 << i;
		    if (low == 0xffff) {
		        low = i;
		    }
		    high = i;
		} else if (low != 0xffff) {
		    break;
		}
	}

	if (lock == 0) {
		ret = -1;
	} else {
		reg4.course_tune = (low + high + 1) / 2;
		amba_writel(RCT_REG(0x33c), reg4.w);
		ret = 0;
	}

	amba_writel(RCT_REG(0x330), reg2.w);

	return ret;
}

int amba_s2_hdmi_set_rate(struct clk *c, unsigned long rate)
{
	union ctrl_reg_u hdmi_ctrl_reg;
	u32 prescaler;
	u32 intprog;

	if (rate < 150000000) {
		amba_clrbitsl(RCT_REG(0x008), 0x1);
		amba_clrbitsl(RCT_REG(0x328), 0x2);

		c->ctrl_reg = PLL_HDMI_CTRL_REG;
		c->pres_reg = SCALER_HDMI_PRE_REG;
		c->post_reg = PLL_REG_UNAVAILABLE;
		c->frac_reg = PLL_REG_UNAVAILABLE;
		c->ctrl2_reg = PLL_VIDEO2_CTRL2_REG;
		c->ctrl3_reg = PLL_VIDEO2_CTRL3_REG;

		if (rate <= 40 * 1000 * 1000) {
			prescaler = 1;
			intprog = (10 - 1);
		} else if (rate <= 80 * 1000 * 1000) {
			prescaler = 2;
			intprog = (20 - 1);
		} else if (rate <= 160 * 1000 * 1000) {
			prescaler = 4;
			intprog = (40 - 1);
		} else {
			prescaler = 8;
			intprog = (80 - 1);
		}

		hdmi_ctrl_reg.w = amba_readl(PLL_HDMI_CTRL_REG);
		hdmi_ctrl_reg.s.intp = intprog;
		hdmi_ctrl_reg.s.sdiv = 0;
		hdmi_ctrl_reg.s.sout = 0;
		hdmi_ctrl_reg.s.frac_mode = 0;
		hdmi_ctrl_reg.s.force_lock = 1;
		hdmi_ctrl_reg.s.write_enable = 1;
		amba_writel(PLL_HDMI_CTRL_REG, hdmi_ctrl_reg.w);
		hdmi_ctrl_reg.s.write_enable = 0;
		amba_writel(PLL_HDMI_CTRL_REG, hdmi_ctrl_reg.w);

		amba_writel(SCALER_HDMI_PRE_REG, prescaler);

		amba_writel(PLL_HDMI_CTRL2_REG, 0x3f770000);
		amba_writel(PLL_HDMI_CTRL3_REG, 0x00068302);
	} else {
		amba_setbitsl(RCT_REG(0x008), 0x5);
		amba_setbitsl(RCT_REG(0x328), 0x2);

		c->ctrl_reg = RCT_REG(0x32c);
		c->pres_reg = PLL_REG_UNAVAILABLE;
		c->post_reg = PLL_REG_UNAVAILABLE;
		c->frac_reg = PLL_REG_UNAVAILABLE;
		c->ctrl2_reg = RCT_REG(0x330);
		c->ctrl3_reg = RCT_REG(0x334);
		c->ctrl4_reg = RCT_REG(0x33c);

		hdmi_ctrl_reg.w = amba_readl(RCT_REG(0x32c));
		hdmi_ctrl_reg.w = 0x06109000;
		amba_writel(RCT_REG(0x32c), hdmi_ctrl_reg.w);
		msleep(1);
		hdmi_ctrl_reg.s.write_enable = 1;
		amba_writel(RCT_REG(0x32c), hdmi_ctrl_reg.w);
		msleep(1);
		hdmi_ctrl_reg.s.write_enable = 0;
		amba_writel(RCT_REG(0x32c), hdmi_ctrl_reg.w);
		msleep(1);
		amba_writel(RCT_REG(0x330), 0x00710000);
		msleep(1);
		amba_writel(RCT_REG(0x33c), 0x08000000);
		msleep(1);
		amba_writel(RCT_REG(0x334), 0x71068300);
		msleep(1);

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
		amb_auto_select_hdmi4k_range(HAL_BASE_VP);
#else
		amba_s2_hdmi_auto_select_hdmi4k_range();
#endif
	}

	return 0;
}

struct clk_ops amba_s2_hdmi_ops = {
	.enable		= NULL,
	.disable	= NULL,
	.get_rate	= amba_s2_hdmi_get_rate,
	.round_rate	= NULL,
	.set_rate	= amba_s2_hdmi_set_rate,
	.set_parent	= NULL,
};

static struct clk gclk_hdmi = {
	.parent		= NULL,
	.name		= "gclk_hdmi",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_VIDEO2_CTRL_REG,
	.pres_reg	= SCALER_VIDEO2_REG,
	.post_reg	= SCALER_VIDEO2_POST_REG,
	.frac_reg	= PLL_VIDEO2_FRAC_REG,
	.ctrl2_reg	= PLL_VIDEO2_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO2_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &amba_s2_hdmi_ops,
};

static struct clk *rct_register_gclk_hdmi(void)
{
	struct clk *pgclk_hdmi = NULL;
	struct clk *pgclk_vo = NULL;

	pgclk_hdmi = clk_get(NULL, "gclk_hdmi");
	if (IS_ERR(pgclk_hdmi)) {
		pgclk_vo = clk_get(NULL, "gclk_vo");
		if (IS_ERR(pgclk_vo)) {
			BUG();
		}
		gclk_hdmi.parent = pgclk_vo;
		ambarella_register_clk(&gclk_hdmi);
		pgclk_hdmi = &gclk_hdmi;
		pr_info("SYSCLK:HDMI[%lu]\n", clk_get_rate(pgclk_hdmi));
	}

	return pgclk_hdmi;
}
#endif

static int amba_s2_vout_set_clock_setup(struct __amba_vout_video_source *psrc,
	struct amba_video_source_clock_setup *args)
{
	int errorCode = 0;

	if (!psrc->id) {
		rct_set_vout2_clk_src(args->src);
		rct_set_vout2_freq_hz(args->freq_hz);
	} else {
		rct_set_vout_clk_src(args->src);
		rct_set_vout_freq_hz(args->freq_hz);
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
#else
		clk_set_rate(rct_register_gclk_hdmi(), args->freq_hz);
#endif
	}

	return errorCode;
}

static void __toss_cmd_check(const void *src, unsigned int n)
{
#if 0
	struct toss_devctx_dsp_s		*dsp_ctx;
	u8					*cmd_buf;
	int					i;
	int					j;

	if (toss == NULL)
		return;

	dsp_ctx = &toss->devctx[toss->oldctx].dsp;
	for (i = 0; i < dsp_ctx->filtered_cmds; i++) {
		if (dsp_ctx->cmd[i].code == (*(u32 *)src)) {
			if (memcmp(&dsp_ctx->cmd[i].param, src, n)) {
				vout_err("toss cmd 0x%08X missmatach...\n",
					dsp_ctx->cmd[i].code);
				cmd_buf = (u8 *)src;
				cmd_buf += 4;
				n -= 4;
				DRV_PRINT("Src:\n\t");
				for (j = 0; j < n; j++)
					DRV_PRINT("0x%02X ", cmd_buf[j]);
				cmd_buf = (u8 *)&dsp_ctx->cmd[i].param;
				DRV_PRINT("\nToss:\n\t");
				for (j = 0; j < n; j++)
					DRV_PRINT("0x%02X ", cmd_buf[j]);
				DRV_PRINT("\n");
			}
			break;
		}
	}
#endif
}

static void __amba_s2_vout_reset(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->reset, sizeof(VOUT_RESET_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->reset, sizeof(VOUT_RESET_CMD));
	vout_dsp(pinfo->reset, cmd_code);
	vout_dsp(pinfo->reset, vout_id);
	vout_dsp(pinfo->reset, reset_mixer);
	vout_dsp(pinfo->reset, reset_disp);
}

static void __amba_s2_vout_mixer_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->mixer_setup, sizeof(VOUT_MIXER_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->mixer_setup, sizeof(VOUT_MIXER_SETUP_CMD));
	vout_dsp(pinfo->mixer_setup, cmd_code);
	vout_dsp(pinfo->mixer_setup, vout_id);
	vout_dsp(pinfo->mixer_setup, interlaced);
	vout_dsp(pinfo->mixer_setup, frm_rate);
	vout_dsp(pinfo->mixer_setup, act_win_width);
	vout_dsp(pinfo->mixer_setup, act_win_height);
	vout_dsp(pinfo->mixer_setup, back_ground_v);
	vout_dsp(pinfo->mixer_setup, back_ground_u);
	vout_dsp(pinfo->mixer_setup, back_ground_y);
	vout_dsp(pinfo->mixer_setup, highlight_v);
	vout_dsp(pinfo->mixer_setup, highlight_u);
	vout_dsp(pinfo->mixer_setup, highlight_y);
	vout_dsp(pinfo->mixer_setup, highlight_thresh);
}

static void __amba_s2_vout_video_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->video_setup, sizeof(VOUT_VIDEO_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->video_setup, sizeof(VOUT_VIDEO_SETUP_CMD));
	vout_dsp(pinfo->video_setup, cmd_code);
	vout_dsp(pinfo->video_setup, vout_id);
	vout_dsp(pinfo->video_setup, en);
	vout_dsp(pinfo->video_setup, src);
	vout_dsp(pinfo->video_setup, flip);
	vout_dsp(pinfo->video_setup, rotate);
	vout_dsp(pinfo->video_setup, win_offset_x);
	vout_dsp(pinfo->video_setup, win_offset_y);
	vout_dsp(pinfo->video_setup, win_width);
	vout_dsp(pinfo->video_setup, win_height);
	vout_dsp(pinfo->video_setup, default_img_y_addr);
	vout_dsp(pinfo->video_setup, default_img_uv_addr);
	vout_dsp(pinfo->video_setup, default_img_pitch);
	vout_dsp(pinfo->video_setup, default_img_repeat_field);
	vout_dsp(pinfo->video_setup, num_stat_lines_at_top);
}

static void __amba_s2_vout_display_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->display_setup, sizeof(VOUT_DISPLAY_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->display_setup, sizeof(VOUT_DISPLAY_SETUP_CMD));
	vout_dsp(pinfo->display_setup, cmd_code);
	vout_dsp(pinfo->display_setup, vout_id);
	vout_dsp(pinfo->display_setup, disp_config_dram_addr);
}

static void __amba_s2_vout_dve_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->dve_setup, sizeof(VOUT_DVE_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->dve_setup, sizeof(VOUT_DVE_SETUP_CMD));
	vout_dsp(pinfo->dve_setup, cmd_code);
	vout_dsp(pinfo->dve_setup, vout_id);
	vout_dsp(pinfo->dve_setup, dve_config_dram_addr);
}

static void __amba_s2_vout_csc_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->csc_setup, sizeof(VOUT_DISPLAY_CSC_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->csc_setup, sizeof(VOUT_DISPLAY_CSC_SETUP_CMD));
	vout_dsp(pinfo->csc_setup, cmd_code);
	vout_dsp(pinfo->csc_setup, vout_id);
	vout_dsp(pinfo->csc_setup, csc_type);
	vout_dsp(pinfo->csc_setup, csc_parms[0]);
	vout_dsp(pinfo->csc_setup, csc_parms[1]);
	vout_dsp(pinfo->csc_setup, csc_parms[2]);
	vout_dsp(pinfo->csc_setup, csc_parms[3]);
	vout_dsp(pinfo->csc_setup, csc_parms[4]);
	vout_dsp(pinfo->csc_setup, csc_parms[5]);
	vout_dsp(pinfo->csc_setup, csc_parms[6]);
	vout_dsp(pinfo->csc_setup, csc_parms[7]);
	vout_dsp(pinfo->csc_setup, csc_parms[8]);
}

static void __amba_s2_vout_osd_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->osd_setup, sizeof(VOUT_OSD_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->osd_setup, sizeof(VOUT_OSD_SETUP_CMD));
	vout_dsp(pinfo->osd_setup, cmd_code);
	vout_dsp(pinfo->osd_setup, vout_id);
	vout_dsp(pinfo->osd_setup, en);
	vout_dsp(pinfo->osd_setup, src);
	vout_dsp(pinfo->osd_setup, flip);
	vout_dsp(pinfo->osd_setup, rescaler_en);
	vout_dsp(pinfo->osd_setup, premultiplied);
	vout_dsp(pinfo->osd_setup, global_blend);
	vout_dsp(pinfo->osd_setup, win_offset_x);
	vout_dsp(pinfo->osd_setup, win_offset_y);
	vout_dsp(pinfo->osd_setup, win_width);
	vout_dsp(pinfo->osd_setup, win_height);
	vout_dsp(pinfo->osd_setup, rescaler_input_width);
	vout_dsp(pinfo->osd_setup, rescaler_input_height);
	vout_dsp(pinfo->osd_setup, osd_buf_dram_addr);
	vout_dsp(pinfo->osd_setup, osd_buf_pitch);
	vout_dsp(pinfo->osd_setup, osd_buf_repeat_field);
	vout_dsp(pinfo->osd_setup, osd_direct_mode);
	vout_dsp(pinfo->osd_setup, osd_transparent_color);
	vout_dsp(pinfo->osd_setup, osd_transparent_color_en);
	vout_dsp(pinfo->osd_setup, osd_buf_info_dram_addr);
}

static void __amba_s2_vout_osd_clut_setup(struct amba_vout_dev_info *pinfo)
{
	__toss_cmd_check(&pinfo->osd_clut_setup, sizeof(VOUT_OSD_CLUT_SETUP_CMD));
	pinfo->pdsp_cmd_fn(&pinfo->osd_clut_setup,
		sizeof(VOUT_OSD_CLUT_SETUP_CMD));
	vout_dsp(pinfo->osd_clut_setup, cmd_code);
	vout_dsp(pinfo->osd_clut_setup, vout_id);
	vout_dsp(pinfo->osd_clut_setup, clut_dram_addr);
}

static int amba_s2_vout_update_setup(struct __amba_vout_video_source *psrc,
	u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;
	u32					flag;

	pinfo = psrc->pinfo;

	if (args == NULL) {
		vout_warn("%s args is NULL!\n", __func__);
		flag = 0xFFFFFFFF;
	} else {
		flag = *args;
	}

	vout_dbg("%s enter with %d, %p, 0x%X\n",
		__func__, pinfo->status, pinfo->pdsp_cmd_fn, flag);

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		goto amba_s2_vout_update_setup_exit;
	}

	if (pinfo->pdsp_cmd_fn == NULL) {
		errorCode = -EINVAL;
		vout_err("pdsp_cmd_fn is NULL!\n");
		goto amba_s2_vout_update_setup_exit;
	}

	if (flag & AMBA_VIDEO_SOURCE_FORCE_RESET) {
		__amba_s2_vout_reset(pinfo);
		flag |= (AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP |
			AMBA_VIDEO_SOURCE_UPDATE_DISPLAY_SETUP |
			AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP);

		/* DO NOT issue MIXER & OSD commands when VOUT input is selected
		 * from SMEM in multiple region warp pipeline for S2.
		 */
		if (pinfo->dram_display.d_input_stream_enable.s.input_select ==
			AMBA_VOUT_INPUT_FROM_SMEM) {
			flag &= ~AMBA_VIDEO_SOURCE_UPDATE_MIXER_SETUP;
			flag &= ~AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
		}
	}

	if (flag & AMBA_VIDEO_SOURCE_UPDATE_MIXER_SETUP) {
		__amba_s2_vout_mixer_setup(pinfo);
	}

	if (flag & AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP) {
		if (pinfo->dram_display.d_input_stream_enable.s.input_select ==
			AMBA_VOUT_INPUT_FROM_SMEM) {
			pinfo->video_setup.num_stat_lines_at_top = pinfo->video_setup.win_offset_y ?
				(pinfo->video_setup.win_offset_y + 1) : 0;
			pinfo->video_setup.win_offset_y = 0;
			pinfo->video_setup.win_height = ((pinfo->video_setup.win_height >> 1) +
				pinfo->video_setup.num_stat_lines_at_top) << 1;
		} else {
			pinfo->video_setup.num_stat_lines_at_top = 0;
		}
		__amba_s2_vout_video_setup(pinfo);
	}

	if (flag & AMBA_VIDEO_SOURCE_UPDATE_DISPLAY_SETUP) {
		__amba_s2_vout_display_setup(pinfo);
		if ((psrc->source_type & AMBA_VOUT_SOURCE_TYPE_DVE) &&
			pinfo->dram_display.d_control.s.analog_out) {
			__amba_s2_vout_dve_setup(pinfo);
			pinfo->dram_dve_flag &= ~AMBA_VOUT_SETUP_CHANGED;
		}
	}

	if (flag & AMBA_VIDEO_SOURCE_UPDATE_CSC_SETUP) {
		if (pinfo->csc_setup_flag & AMBA_VOUT_SETUP_CHANGED) {
			__amba_s2_vout_csc_setup(pinfo);
			pinfo->csc_setup_flag &= ~AMBA_VOUT_SETUP_CHANGED;
		}
	}

	if (flag & AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP) {
		if (pinfo->osd_setup_flag & AMBA_VOUT_SETUP_CHANGED) {
			__amba_s2_vout_osd_setup(pinfo);
			pinfo->osd_setup_flag &= ~AMBA_VOUT_SETUP_CHANGED;
		}

		if (pinfo->osd_clut_setup_flag & AMBA_VOUT_SETUP_CHANGED) {
			__amba_s2_vout_osd_clut_setup(pinfo);
			pinfo->osd_clut_setup_flag &= ~AMBA_VOUT_SETUP_CHANGED;
		}
	}

	/*if (pinfo->mixer_setup.act_win_height == 2160) {
		msleep(300);
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
		amb_auto_select_hdmi4k_range(HAL_BASE_VP);
#else
		// TBD
#endif
	}*/

amba_s2_vout_update_setup_exit:
	return errorCode;
}

static int amba_s2_vout_get_bg_color(struct __amba_vout_video_source *psrc,
	struct amba_vout_bg_color_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	args->cr = pinfo->dram_display.d_background.s.cr;
	args->cb = pinfo->dram_display.d_background.s.cb;
	args->y = pinfo->dram_display.d_background.s.y;

	return errorCode;
}

static int amba_s2_vout_set_bg_color(struct __amba_vout_video_source *psrc,
	struct amba_vout_bg_color_info *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		pinfo->dram_display.d_background.s.cr = args->cr;
		pinfo->dram_display.d_background.s.cb = args->cb;
		pinfo->dram_display.d_background.s.y =
			(args->y > 10) ? args->y : 10;

		pinfo->mixer_setup.back_ground_v =
			pinfo->dram_display.d_background.s.cr;
		pinfo->mixer_setup.back_ground_u =
			pinfo->dram_display.d_background.s.cb;
		pinfo->mixer_setup.back_ground_y =
			pinfo->dram_display.d_background.s.y;
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_set_lcd(struct __amba_vout_video_source *psrc,
	struct amba_vout_lcd_info *plcd_cfg)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;
	int					clk_div;

	pinfo = psrc->pinfo;

	if (pinfo->status != AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
		goto amba_s2_vout_set_lcd_exit;
	}

	switch (plcd_cfg->dclk_edge) {
	case AMBA_VOUT_LCD_CLK_RISING_EDGE:
		pinfo->dram_display.d_digital_output_mode.s.clk_edge
			= LCD_CLK_VLD_RISING;
		break;

	case AMBA_VOUT_LCD_CLK_FALLING_EDGE:
		pinfo->dram_display.d_digital_output_mode.s.clk_edge
			= LCD_CLK_VLD_FALLING;
		break;

	default:
		errorCode = -EINVAL;
		goto amba_s2_vout_set_lcd_exit;
		break;
	}

	switch (plcd_cfg->hvld_pol) {
	case AMBA_VOUT_LCD_HVLD_POL_LOW:
		pinfo->dram_display.d_digital_output_mode.s.hvld_pol
			= LCD_ACT_LOW;
		break;

	case AMBA_VOUT_LCD_HVLD_POL_HIGH:
		pinfo->dram_display.d_digital_output_mode.s.hvld_pol
			= LCD_ACT_HIGH;
		break;

	default:
		errorCode = -EINVAL;
		goto amba_s2_vout_set_lcd_exit;
		break;
	}

	switch (plcd_cfg->mode) {
	case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
		pinfo->dram_display.d_digital_output_mode.s.mode
			= LCD_MODE_1COLOR;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
		pinfo->dram_display.d_digital_output_mode.s.mode
			= LCD_MODE_3COLORS;
		break;

	case AMBA_VOUT_LCD_MODE_RGB565:
		pinfo->dram_display.d_digital_output_mode.s.mode
			= LCD_MODE_RGB565;
		break;

	case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
		pinfo->dram_display.d_digital_output_mode.s.mode
			= LCD_MODE_3COLORS_DUMMY;
		break;

	case AMBA_VOUT_LCD_MODE_RGB888:
		pinfo->dram_display.d_digital_output_mode.s.mode
			= LCD_MODE_601_24BITS;
		break;

	case AMBA_VOUT_LCD_MODE_DISABLE:
		goto amba_s2_vout_set_lcd_exit;
		break;

	default:
		errorCode = -EINVAL;
		goto amba_s2_vout_set_lcd_exit;
		break;
	}

	if (!psrc->id) {
		clk_div = rct_get_vout2_freq_hz();
	} else {
		clk_div = rct_get_vout_freq_hz();
	}
	clk_div /= plcd_cfg->dclk_freq_hz;
	switch (clk_div) {
	case 2:	//PLL_CLK_13_5MHZ
		pinfo->dram_display.d_digital_output_mode.s.divider_en
			= LCD_CLK_DIV_ENA;
		pinfo->dram_display.d_digital_output_mode.s.clk_src
			= LCD_CLK_DIVIDED;
		pinfo->dram_display.d_digital_output_mode.s.div_ptn_len = 2;
		pinfo->dram_display.d_digital_clock_pattern_3 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_2 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_1 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_0 = 0x00000001;
		break;

	case 4:	//PLL_CLK_6_75MHZ
		pinfo->dram_display.d_digital_output_mode.s.divider_en
			= LCD_CLK_DIV_ENA;
		pinfo->dram_display.d_digital_output_mode.s.clk_src
			= LCD_CLK_DIVIDED;
		pinfo->dram_display.d_digital_output_mode.s.div_ptn_len = 4;
		pinfo->dram_display.d_digital_clock_pattern_3 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_2 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_1 = 0x00000000;
		pinfo->dram_display.d_digital_clock_pattern_0 = 0x00000003;
		break;

	default:
		pinfo->dram_display.d_digital_output_mode.s.divider_en
			= LCD_CLK_DIV_DIS;
		pinfo->dram_display.d_digital_output_mode.s.clk_src
			= LCD_CLK_UNDIVIDED;
		break;
	}

	switch (plcd_cfg->seqt) {
	case AMBA_VOUT_LCD_SEQ_R0_G1_B2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_R0_G1_B2;
		break;
	case AMBA_VOUT_LCD_SEQ_R0_B1_G2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_R0_B1_G2;
		break;
	case AMBA_VOUT_LCD_SEQ_G0_R1_B2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_G0_R1_B2;
		break;
	case AMBA_VOUT_LCD_SEQ_G0_B1_R2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_G0_B1_R2;
		break;
	case AMBA_VOUT_LCD_SEQ_B0_R1_G2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_B0_R1_G2;
		break;
	case AMBA_VOUT_LCD_SEQ_B0_G1_R2:
		pinfo->dram_display.d_digital_output_mode.s.seqe
			= VO_SEQ_B0_G1_R2;
		break;
	default:
		errorCode = -EINVAL;
		goto amba_s2_vout_set_lcd_exit;
		break;
	}

	switch (plcd_cfg->seqb) {
	case AMBA_VOUT_LCD_SEQ_R0_G1_B2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_R0_G1_B2;
		break;
	case AMBA_VOUT_LCD_SEQ_R0_B1_G2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_R0_B1_G2;
		break;
	case AMBA_VOUT_LCD_SEQ_G0_R1_B2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_G0_R1_B2;
		break;
	case AMBA_VOUT_LCD_SEQ_G0_B1_R2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_G0_B1_R2;
		break;
	case AMBA_VOUT_LCD_SEQ_B0_R1_G2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_B0_R1_G2;
		break;
	case AMBA_VOUT_LCD_SEQ_B0_G1_R2:
		pinfo->dram_display.d_digital_output_mode.s.seqo
			= VO_SEQ_B0_G1_R2;
		break;
	default:
		errorCode = -EINVAL;
		goto amba_s2_vout_set_lcd_exit;
		break;
	}

amba_s2_vout_set_lcd_exit:
	return errorCode;
}

static int amba_s2_vout_set_csc(struct __amba_vout_video_source *psrc,
	struct amba_video_source_csc_info *pcsc_cfg)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	static u32 digital_color_tbl[7][6] = {
		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD: YUV601 -> YUV709 */
		{ 0x005103a8, 0x1f950008, 0x1fbb04b0,
		  0x00220009, 0x7ff603d5, 0x00017ff4 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD: YUV601 -> YUV601 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB: YUV601 -> RGB */
		{ 0x1e6f04a8, 0x04a81cbf, 0x1fff0812,
		  0x1ffe04a8, 0x00880662, 0x7f217eeb },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD: YUV709 -> YUV601 */
		{ 0x1fb60458, 0x00631ff2, 0x003c0362,
		  0x1fe31ff2, 0x000a042b, 0x7fff000b },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD: YUV709 -> YUV709 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB: YUV709 -> RGB */
		{ 0x1f2604a8, 0x04a81ddd, 0x00010874,
		  0x1fff04a8, 0x004d072b, 0x7f087edf },

		/* AMBA_VIDEO_SOURCE_CSC_RGB2RGB: RGB -> RGB*/
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 } };

	static u32 digital_clamp_tbl[8][3] = {
		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL  */
		{ 0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL   */
		{ 0x03ac0040, 0x03c00040, 0x03c00040 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 } };

	static u32 analog_color_tbl[2][6] = {
		/* VO_CSC_ANALOG_SD */
		{ 0x04000400, 0x00000400, 0x00000000,
		  0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* VO_CSC_ANALOG_HD */
		{ 0x02710280, 0x00d40271, 0x00c700c7,
		  0x03ff0000, 0x03ff0000, 0x03ff0000 } };

	static u32 analog_scale_tbl[2][6] = {
		/* VO_CSC_ANALOG_CLAMP_SD */
		{ 0x04c004c0, 0x000004c0, 0x00000000,
		  0x03ac0040, 0x03c00040, 0x03c00040 },

		/* VO_CSC_ANALOG_CLAMP_HD */
		{ 0x04c004c0, 0x000004c0, 0x00000000,
		  0x03ac0040, 0x03c00040, 0x03c00040 } };

	static u32 hdmi_color_tbl[7][6] = {
		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD: YUV601 -> YUV709 */
		{ 0x005103a8, 0x1f950008, 0x1fbb04b0,
		  0x00220009, 0x7ff603d5, 0x00017ff4 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD: YUV601 -> YUV601 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB: YUV601 -> RGB */
		{ 0x1e6f04a8, 0x04a81cbf, 0x1fff0812,
		  0x1ffe04a8, 0x00880662, 0x7f217eeb },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD: YUV709 -> YUV601 */
		{ 0x1fb60458, 0x00631ff2, 0x003c0362,
		  0x1fe31ff2, 0x000a042b, 0x7fff000b },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD: YUV709 -> YUV709 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB: YUV709 -> RGB */
		{ 0x1f2604a8, 0x04a81ddd, 0x00010874,
		  0x1fff04a8, 0x004d072b, 0x7f087edf },

		/* AMBA_VIDEO_SOURCE_CSC_RGB2RGB: RGB -> RGB*/
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 } };

	static u32 hdmi_clamp_tbl[9][3] = {
		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL  */
		{ 0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL   */
		{ 0x03ac0040, 0x03c00040, 0x03c00040 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP */
		{ 0x0eb00100, 0x0f000100, 0x0f000100 } };

	pinfo = psrc->pinfo;

	if (pinfo->status != AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
		goto amba_s2_vout_set_csc_exit;
	}

	switch (pcsc_cfg->path) {
	case AMBA_VIDEO_SOURCE_CSC_DIGITAL:
		pinfo->dram_display.d_digital_csc_param_0 =
					digital_color_tbl[pcsc_cfg->mode][0];
		pinfo->dram_display.d_digital_csc_param_1 =
					digital_color_tbl[pcsc_cfg->mode][1];
		pinfo->dram_display.d_digital_csc_param_2 =
					digital_color_tbl[pcsc_cfg->mode][2];
		pinfo->dram_display.d_digital_csc_param_3 =
					digital_color_tbl[pcsc_cfg->mode][3];
		pinfo->dram_display.d_digital_csc_param_4 =
					digital_color_tbl[pcsc_cfg->mode][4];
		pinfo->dram_display.d_digital_csc_param_5 =
					digital_color_tbl[pcsc_cfg->mode][5];

		pinfo->dram_display.d_digital_csc_param_6 =
					digital_clamp_tbl[pcsc_cfg->clamp][0];
		pinfo->dram_display.d_digital_csc_param_7 =
					digital_clamp_tbl[pcsc_cfg->clamp][1];
		pinfo->dram_display.d_digital_csc_param_8 =
					digital_clamp_tbl[pcsc_cfg->clamp][2];
		break;

	case AMBA_VIDEO_SOURCE_CSC_ANALOG:
		pinfo->dram_display.d_analog_csc_param_0 =
					analog_color_tbl[pcsc_cfg->mode][0];
		pinfo->dram_display.d_analog_csc_param_1 =
					analog_color_tbl[pcsc_cfg->mode][1];
		pinfo->dram_display.d_analog_csc_param_2 =
					analog_color_tbl[pcsc_cfg->mode][2];
		pinfo->dram_display.d_analog_csc_param_3 =
					analog_color_tbl[pcsc_cfg->mode][3];
		pinfo->dram_display.d_analog_csc_param_4 =
					analog_color_tbl[pcsc_cfg->mode][4];
		pinfo->dram_display.d_analog_csc_param_5 =
					analog_color_tbl[pcsc_cfg->mode][5];

		pinfo->dram_display.d_analog_csc_2_param_0 =
					analog_scale_tbl[pcsc_cfg->clamp][0];
		pinfo->dram_display.d_analog_csc_2_param_1 =
					analog_scale_tbl[pcsc_cfg->clamp][1];
		pinfo->dram_display.d_analog_csc_2_param_2 =
					analog_scale_tbl[pcsc_cfg->clamp][2];
		pinfo->dram_display.d_analog_csc_2_param_3 =
					analog_scale_tbl[pcsc_cfg->clamp][3];
		pinfo->dram_display.d_analog_csc_2_param_4 =
					analog_scale_tbl[pcsc_cfg->clamp][4];
		pinfo->dram_display.d_analog_csc_2_param_5 =
					analog_scale_tbl[pcsc_cfg->clamp][5];
		break;

	case AMBA_VIDEO_SOURCE_CSC_HDMI:
		pinfo->dram_display.d_hdmi_csc_param_0 =
					hdmi_color_tbl[pcsc_cfg->mode][0];
		pinfo->dram_display.d_hdmi_csc_param_1 =
					hdmi_color_tbl[pcsc_cfg->mode][1];
		pinfo->dram_display.d_hdmi_csc_param_2 =
					hdmi_color_tbl[pcsc_cfg->mode][2];
		pinfo->dram_display.d_hdmi_csc_param_3 =
					hdmi_color_tbl[pcsc_cfg->mode][3];
		pinfo->dram_display.d_hdmi_csc_param_4 =
					hdmi_color_tbl[pcsc_cfg->mode][4];
		pinfo->dram_display.d_hdmi_csc_param_5 =
					hdmi_color_tbl[pcsc_cfg->mode][5];

		pinfo->dram_display.d_hdmi_csc_param_6 =
					hdmi_clamp_tbl[pcsc_cfg->clamp][0];
		pinfo->dram_display.d_hdmi_csc_param_7 =
					hdmi_clamp_tbl[pcsc_cfg->clamp][1];
		pinfo->dram_display.d_hdmi_csc_param_8 =
					hdmi_clamp_tbl[pcsc_cfg->clamp][2];
		break;

	default:
		break;
	}

amba_s2_vout_set_csc_exit:
	return errorCode;
}

static int amba_s2_vout_set_csc_dynamically(struct __amba_vout_video_source *psrc,
	struct amba_video_source_csc_info *pcsc_cfg)
{
	struct amba_vout_dev_info		*pinfo;

	static u32 digital_color_tbl[7][6] = {
		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD: YUV601 -> YUV709 */
		{ 0x005103a8, 0x1f950008, 0x1fbb04b0,
		  0x00220009, 0x7ff603d5, 0x00017ff4 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD: YUV601 -> YUV601 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB: YUV601 -> RGB */
		{ 0x1e6f04a8, 0x04a81cbf, 0x1fff0812,
		  0x1ffe04a8, 0x00880662, 0x7f217eeb },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD: YUV709 -> YUV601 */
		{ 0x1fb60458, 0x00631ff2, 0x003c0362,
		  0x1fe31ff2, 0x000a042b, 0x7fff000b },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD: YUV709 -> YUV709 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB: YUV709 -> RGB */
		{ 0x1f2604a8, 0x04a81ddd, 0x00010874,
		  0x1fff04a8, 0x004d072b, 0x7f087edf },

		/* AMBA_VIDEO_SOURCE_CSC_RGB2RGB: RGB -> RGB*/
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 } };

	static u32 digital_clamp_tbl[8][3] = {
		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL  */
		{ 0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL   */
		{ 0x03ac0040, 0x03c00040, 0x03c00040 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 } };

	static u32 analog_color_tbl[2][6] = {
		/* VO_CSC_ANALOG_SD */
		{ 0x04000400, 0x00000400, 0x00000000,
		  0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* VO_CSC_ANALOG_HD */
		{ 0x02710280, 0x00d40271, 0x00c700c7,
		  0x03ff0000, 0x03ff0000, 0x03ff0000 } };

	static u32 hdmi_color_tbl[7][6] = {
		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD: YUV601 -> YUV709 */
		{ 0x005103a8, 0x1f950008, 0x1fbb04b0,
		  0x00220009, 0x7ff603d5, 0x00017ff4 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD: YUV601 -> YUV601 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB: YUV601 -> RGB */
		{ 0x1e6f04a8, 0x04a81cbf, 0x1fff0812,
		  0x1ffe04a8, 0x00880662, 0x7f217eeb },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD: YUV709 -> YUV601 */
		{ 0x1fb60458, 0x00631ff2, 0x003c0362,
		  0x1fe31ff2, 0x000a042b, 0x7fff000b },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD: YUV709 -> YUV709 */
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 },

		/* AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB: YUV709 -> RGB */
		{ 0x1f2604a8, 0x04a81ddd, 0x00010874,
		  0x1fff04a8, 0x004d072b, 0x7f087edf },

		/* AMBA_VIDEO_SOURCE_CSC_RGB2RGB: RGB -> RGB*/
		{ 0x00000400, 0x00000000, 0x00000400,
		  0x00000000, 0x00000400, 0x00000000 } };

	static u32 hdmi_clamp_tbl[8][3] = {
		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL  */
		{ 0x03ff0000, 0x03ff0000, 0x03ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL   */
		{ 0x03ac0040, 0x03c00040, 0x03c00040 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },

		/* AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 } };

	pinfo = psrc->pinfo;

	switch (pcsc_cfg->path) {
	case AMBA_VIDEO_SOURCE_CSC_DIGITAL:
		pinfo->csc_setup.csc_type = AMBA_VIDEO_SOURCE_CSC_DIGITAL;

		pinfo->csc_setup.csc_parms[0] =
					digital_color_tbl[pcsc_cfg->mode][0];
		pinfo->csc_setup.csc_parms[1] =
					digital_color_tbl[pcsc_cfg->mode][1];
		pinfo->csc_setup.csc_parms[2] =
					digital_color_tbl[pcsc_cfg->mode][2];
		pinfo->csc_setup.csc_parms[3] =
					digital_color_tbl[pcsc_cfg->mode][3];
		pinfo->csc_setup.csc_parms[4] =
					digital_color_tbl[pcsc_cfg->mode][4];
		pinfo->csc_setup.csc_parms[5] =
					digital_color_tbl[pcsc_cfg->mode][5];

		pinfo->csc_setup.csc_parms[6] =
					digital_clamp_tbl[pcsc_cfg->clamp][0];
		pinfo->csc_setup.csc_parms[7] =
					digital_clamp_tbl[pcsc_cfg->clamp][1];
		pinfo->csc_setup.csc_parms[8] =
					digital_clamp_tbl[pcsc_cfg->clamp][2];

		pinfo->csc_setup_flag = AMBA_VOUT_SETUP_NEW;

		break;

	case AMBA_VIDEO_SOURCE_CSC_ANALOG:
		pinfo->csc_setup.csc_type = AMBA_VIDEO_SOURCE_CSC_ANALOG;

		pinfo->csc_setup.csc_parms[0] =
					analog_color_tbl[pcsc_cfg->mode][0];
		pinfo->csc_setup.csc_parms[1] =
					analog_color_tbl[pcsc_cfg->mode][1];
		pinfo->csc_setup.csc_parms[2] =
					analog_color_tbl[pcsc_cfg->mode][2];
		pinfo->csc_setup.csc_parms[3] =
					analog_color_tbl[pcsc_cfg->mode][3];
		pinfo->csc_setup.csc_parms[4] =
					analog_color_tbl[pcsc_cfg->mode][4];
		pinfo->csc_setup.csc_parms[5] =
					analog_color_tbl[pcsc_cfg->mode][5];

		pinfo->csc_setup_flag = AMBA_VOUT_SETUP_NEW;

		break;

	case AMBA_VIDEO_SOURCE_CSC_HDMI:
		pinfo->csc_setup.csc_type = AMBA_VIDEO_SOURCE_CSC_HDMI;

		pinfo->csc_setup.csc_parms[0] =
					hdmi_color_tbl[pcsc_cfg->mode][0];
		pinfo->csc_setup.csc_parms[1] =
					hdmi_color_tbl[pcsc_cfg->mode][1];
		pinfo->csc_setup.csc_parms[2] =
					hdmi_color_tbl[pcsc_cfg->mode][2];
		pinfo->csc_setup.csc_parms[3] =
					hdmi_color_tbl[pcsc_cfg->mode][3];
		pinfo->csc_setup.csc_parms[4] =
					hdmi_color_tbl[pcsc_cfg->mode][4];
		pinfo->csc_setup.csc_parms[5] =
					hdmi_color_tbl[pcsc_cfg->mode][5];

		pinfo->csc_setup.csc_parms[6] =
					hdmi_clamp_tbl[pcsc_cfg->clamp][0];
		pinfo->csc_setup.csc_parms[7] =
					hdmi_clamp_tbl[pcsc_cfg->clamp][1];
		pinfo->csc_setup.csc_parms[8] =
					hdmi_clamp_tbl[pcsc_cfg->clamp][2];

		pinfo->csc_setup_flag = AMBA_VOUT_SETUP_NEW;

		break;

	default:
		break;
	}

	return 0;
}

static int amba_s2_vout_get_dve(struct __amba_vout_video_source *psrc,
	dram_dve_t *pdve)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;
	*pdve = pinfo->dram_dve;

	return errorCode;
}

static int amba_s2_vout_set_dve(struct __amba_vout_video_source *psrc,
	dram_dve_t *pdve)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_IDLE) {
		if (pdve) {
			pinfo->dram_dve = *pdve;
			pinfo->dram_dve_flag = AMBA_VOUT_SETUP_NEW;
		} else {
			pinfo->dram_dve_flag = AMBA_VOUT_SETUP_NA;
		}
	} else {
		errorCode = -EINVAL;
		vout_err("%s wrong status %d!\n", __func__, pinfo->status);
	}

	return errorCode;
}

static int amba_s2_vout_suspend(struct __amba_vout_video_source *psrc,
	u32 *args)
{
	int				errorCode = 0;
	struct amba_vout_dev_info	*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status != AMBA_VIDEO_SOURCE_STATUS_SUSPENDED) {
		pinfo->pstatus = pinfo->status;
		pinfo->status = AMBA_VIDEO_SOURCE_STATUS_SUSPENDED;
	} else {
		errorCode = 1;
	}

	return errorCode;
}

static int amba_s2_vout_resume(struct __amba_vout_video_source *psrc,
	u32 *args)
{
	int				errorCode = 0;
	struct amba_vout_dev_info	*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->status == AMBA_VIDEO_SOURCE_STATUS_SUSPENDED) {
		pinfo->status = pinfo->pstatus;
	} else {
		errorCode = 1;
	}

	return errorCode;
}

static int amba_s2_vout_suspend_toss(struct __amba_vout_video_source *psrc,
	u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	disable_irq(pinfo->irq);

	return errorCode;
}

static int amba_s2_vout_resume_toss(struct __amba_vout_video_source *psrc,
	u32 *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	if (pinfo->osd_setup_flag & AMBA_VOUT_SETUP_VALID)
		__amba_s2_vout_osd_setup(pinfo);

	if (pinfo->osd_clut_setup_flag & AMBA_VOUT_SETUP_VALID)
		__amba_s2_vout_osd_clut_setup(pinfo);

	enable_irq(pinfo->irq);

	return errorCode;
}

static int amba_s2_vout_docmd(struct __amba_vout_video_source *psrc,
	enum amba_video_source_cmd cmd, void *args)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*pinfo;

	pinfo = psrc->pinfo;

	switch (cmd) {
	case AMBA_VIDEO_SOURCE_IDLE:
		break;

	case AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO:
		errorCode = amba_s2_update_iav_info(psrc,
			(struct amba_video_source_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_RESET:
		errorCode = amba_s2_vout_reset(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP:
		errorCode = amba_s2_vout_update_setup(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_RUN:
		errorCode = amba_s2_vout_run(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_HALT:
		errorCode = amba_s2_vout_halt(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_SUSPEND:
		errorCode = amba_s2_vout_suspend(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_RESUME:
		errorCode = amba_s2_vout_resume(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_SUSPEND_TOSS:
		errorCode = amba_s2_vout_suspend_toss(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_RESUME_TOSS:
		errorCode = amba_s2_vout_resume_toss(psrc, (u32 *)args);
		break;

	case AMBA_VIDEO_SOURCE_REGISTER_SINK:
		psrc->total_sink_num++;
		break;
	case AMBA_VIDEO_SOURCE_UNREGISTER_SINK:
		if (psrc->total_sink_num)
			psrc->total_sink_num--;
		else
			errorCode = -EINVAL;
		break;
	case AMBA_VIDEO_SOURCE_GET_SINK_NUM:
		*(int *)args = psrc->total_sink_num;
		break;

	case AMBA_VIDEO_SOURCE_GET_ACTIVE_SINK_ID:
		*(int *)args = psrc->active_sink_id;
		break;
	case AMBA_VIDEO_SOURCE_SET_ACTIVE_SINK_ID:
		if (psrc->total_sink_num > *(int *)args)
			psrc->active_sink_id = *(int *)args;
		else
			errorCode = -EINVAL;
		break;

	case AMBA_VIDEO_SOURCE_REGISTER_IRQ_CALLBACK:
		{
		unsigned long		flags;

		spin_lock_irqsave(&pinfo->lock, flags);

		pinfo->irq_callback = (irq_callback_t *)args;

		spin_unlock_irqrestore(&pinfo->lock, flags);
		}
		break;

	case AMBA_VIDEO_SOURCE_REGISTER_VOUT_SETUP_PTR:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_REGISTER_DSP_CMD_FN:
		{
		unsigned long		flags;

		spin_lock_irqsave(&pinfo->lock, flags);

		pinfo->pdsp_cmd_fn = (vout_dsp_cmd *)args;

		spin_unlock_irqrestore(&pinfo->lock, flags);
		}
		break;

	case AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT:
		{
		struct amb_event		event;

		event.type = (enum amb_event_type)args;
		errorCode = amb_event_pool_affuse(&pinfo->event_pool,
				event);
		if (!errorCode && pinfo->event_proc.fasync_queue)
			kill_fasync(&pinfo->event_proc.fasync_queue, SIGIO, POLL_IN);
		}
		break;

	case AMBA_VIDEO_SOURCE_GET_CONFIG:
		errorCode = amba_s2_vout_get_config(psrc, (vd_config_t *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_CONFIG:
		errorCode = amba_s2_vout_set_config(psrc, (vd_config_t *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_VIDEO_SIZE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_VIDEO_SIZE:
		errorCode = amba_s2_vout_set_video_size(psrc,
			(struct amba_vout_window_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_DISPLAY:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_DISPLAY:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_OSD:
		errorCode = amba_s2_vout_get_osd(psrc,
			(VOUT_OSD_SETUP_CMD *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_OSD:
		errorCode = amba_s2_vout_set_osd(psrc,
			(VOUT_OSD_SETUP_CMD *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_OSD_BUFFER:
		errorCode = amba_s2_vout_get_osd_buf(psrc,
			(VOUT_OSD_BUF_SETUP_CMD *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_OSD_BUFFER:
		errorCode = amba_s2_vout_set_osd_buf(psrc,
			(VOUT_OSD_BUF_SETUP_CMD *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_OSD_CLUT:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_OSD_CLUT:
		errorCode = amba_s2_vout_set_osd_clut(psrc,
			(struct amba_video_source_osd_clut_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_CURSOR:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CURSOR:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_CURSOR_BITMAP:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CURSOR_BITMAP:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_CURSOR_PALETTE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CURSOR_PALETTE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_BG_COLOR:
		errorCode = amba_s2_vout_get_bg_color(psrc,
			(struct amba_vout_bg_color_info *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_BG_COLOR:
		errorCode = amba_s2_vout_set_bg_color(psrc,
			(struct amba_vout_bg_color_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_LCD:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_LCD:
		errorCode = amba_s2_vout_set_lcd(psrc,
			(struct amba_vout_lcd_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN:
		errorCode = amba_s2_vout_get_active_win(psrc,
			(struct amba_vout_window_info *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN:
		errorCode = amba_s2_vout_set_active_win(psrc,
			(struct amba_vout_window_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_HV:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_HV:
		errorCode = amba_s2_vout_set_hv(psrc,
			(struct amba_vout_hv_size_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_HVSYNC:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_HVSYNC:
		errorCode = amba_s2_vout_set_hvsync(psrc,
			(struct amba_vout_hv_sync_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_HVSYNC_POLARITY:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_HVSYNC_POLARITY:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_SCALE_HD_ANALOG_OUT:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_SCALE_HD_ANALOG_OUT:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIDEO_SOURCE_GET_SCALE_SD_ANALOG_OUT:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_SCALE_SD_ANALOG_OUT:
		errorCode = amba_s2_vout_scale_sd_analog_out(psrc,
			(struct amba_video_source_scale_analog_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_VBI:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_VBI:
		errorCode = amba_s2_vout_set_vbi(psrc,
			(struct amba_video_sink_mode *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_VIDEO_INFO:
		errorCode = amba_s2_vout_get_video_info(psrc,
			(struct amba_video_info *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_VIDEO_INFO:
		errorCode = amba_s2_vout_set_video_info(psrc,
			(struct amba_video_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_VOUT_SETUP:
		errorCode = amba_s2_vout_get_setup(psrc,
			(VOUT_VIDEO_SETUP_CMD *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_VOUT_SETUP:
		errorCode = amba_s2_vout_set_setup(psrc,
			(VOUT_VIDEO_SETUP_CMD *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_CLOCK_SETUP:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CLOCK_SETUP:
		errorCode = amba_s2_vout_set_clock_setup(psrc,
			(struct amba_video_source_clock_setup *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_CSC:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CSC:
		errorCode = amba_s2_vout_set_csc(psrc,
			(struct amba_video_source_csc_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_CSC_DYNAMICALLY:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_CSC_DYNAMICALLY:
		errorCode = amba_s2_vout_set_csc_dynamically(psrc,
			(struct amba_video_source_csc_info *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_DVE:
		errorCode = amba_s2_vout_get_dve(psrc,
			(dram_dve_t *)args);
		break;
	case AMBA_VIDEO_SOURCE_SET_DVE:
		errorCode = amba_s2_vout_set_dve(psrc,
			(dram_dve_t *)args);
		break;

	case AMBA_VIDEO_SOURCE_GET_DISPLAY_INPUT:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIDEO_SOURCE_SET_DISPLAY_INPUT:
		errorCode = amba_s2_vout_select_display_input(psrc,
			(enum amba_vout_display_input *)args);
		break;

	case AMBA_VIDEO_SOURCE_REGISTER_AR_NOTIFIER:
		break;

	default:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		vout_err("%s-%d do not support cmd %d!\n",
			psrc->name, psrc->id, cmd);

		break;
	}

	return errorCode;
}

static irqreturn_t amba_s2_vout_irq(int irqno, void *dev_id)
{
	struct amba_vout_dev_info		*pinfo;
	unsigned long				flags;

	pinfo = (struct amba_vout_dev_info *)dev_id;

	if (pinfo->irq_callback)
		pinfo->irq_callback();

	pinfo->irq_counter++;
	atomic_set(&pinfo->irq_proc_hinfo.sync_proc_flag, 0xFFFFFFFF);
	wake_up_all(&pinfo->irq_proc_hinfo.sync_proc_head);

	spin_lock_irqsave(&pinfo->osd_update_lock, flags);
	if (pinfo->osd_update_wait_num) {
		pinfo->osd_update_wait_num--;
	}
	pinfo->osd_update_timeout = 0;
	spin_unlock_irqrestore(&pinfo->osd_update_lock, flags);
	wake_up_interruptible(&pinfo->osd_update_wq);

	return IRQ_HANDLED;
}

static int amba_s2_vout_proc_read(char *start, void *data)
{
	struct amba_vout_dev_info	*pinfo;

	pinfo = (struct amba_vout_dev_info *)data;

	return sprintf(start, "%08x", pinfo->irq_counter);
}

static const struct file_operations amba_s2_vout_proc_fops = {
	.open = ambsync_proc_open,
	.read = ambsync_proc_read,
	.release = ambsync_proc_release,
};

static ssize_t amba_s2_vout_event_proc_read(struct file *filp,
	char __user *buf, size_t count, loff_t *offset)
{
	struct amb_event_pool			*pool;
	struct amb_event			event;
	size_t					i;
	int					ret;

	pool = (struct amb_event_pool *)filp->private_data;
	if (!pool) {
		return -EINVAL;
	}

	for (i = 0; i < count / sizeof(struct amb_event); i++) {
		if (amb_event_pool_query_event(pool, &event, *offset))
			break;
		ret = copy_to_user(&buf[i * sizeof(struct amb_event)],
			&event, sizeof(event));
		if (ret)
			return -EFAULT;
		(*offset)++;
	}

	if (i == 0) {
		*offset = amb_event_pool_query_index(pool);
		if (!amb_event_pool_query_event(pool, &event, *offset)) {
			ret = copy_to_user(buf, &event, sizeof(event));
			if (ret)
				return -EFAULT;
			(*offset)++;
			i++;
		}
	}

	return i * sizeof(struct amb_event);
}

/* ========================================================================== */
static int __init amba_s2_vout_probe(
	struct amba_vout_system_instance_info *pvi)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*vout_dev_info;
	struct amba_vout_dev_info		*vout_dma_info;
	struct __amba_vout_video_source		*psrc;
	dma_addr_t				dma_address;

	vout_dbg("amba_s2_vout_init\n");

	vout_dev_info = dma_alloc_coherent(NULL,
		sizeof(struct amba_vout_dev_info),
		&dma_address, GFP_KERNEL);
	if (vout_dev_info == NULL) {
		vout_err("Out of memory!\n");
		errorCode = -ENOMEM;
		goto amba_s2_vout_probe_exit;
	}
	vout_dma_info = (struct amba_vout_dev_info *)dma_address;

	vout_dev_info->dram_display.d_digital_output_mode.s.hvld_pol
		= LCD_ACT_HIGH;

	vout_dev_info->irq = pvi->irq.start;
	vout_dev_info->status = AMBA_VIDEO_SOURCE_STATUS_IDLE;
	spin_lock_init(&vout_dev_info->lock);
	init_waitqueue_head(&vout_dev_info->irq_wait_reset);

	init_waitqueue_head(&vout_dev_info->osd_update_wq);
	spin_lock_init(&vout_dev_info->osd_update_lock);
	vout_dev_info->osd_update_wait_num = 0;
	vout_dev_info->osd_update_timeout = 0;

	vout_dev_info->irq_counter = 0;
	ambsync_proc_hinit(&vout_dev_info->irq_proc_hinfo);
	vout_dev_info->irq_proc_hinfo.sync_read_proc = amba_s2_vout_proc_read;
	vout_dev_info->irq_proc_hinfo.sync_read_data = vout_dev_info;

	amb_event_pool_init(&vout_dev_info->event_pool);
	snprintf(vout_dev_info->event_proc.proc_name,
		sizeof(vout_dev_info->event_proc.proc_name),
		"%s_event", pvi->name);
	vout_dev_info->event_proc.fops.read = amba_s2_vout_event_proc_read;
	vout_dev_info->event_proc.private_data = &vout_dev_info->event_pool;

	vout_dev_info->osd_setup_flag = AMBA_VOUT_SETUP_NA;
	vout_dev_info->osd_clut_setup_flag = AMBA_VOUT_SETUP_NA;
	vout_dev_info->dram_dve_flag = AMBA_VOUT_SETUP_NA;

	vout_dev_info->mixer_setup.cmd_code = CMD_VOUT_MIXER_SETUP;
	vout_dev_info->mixer_setup.vout_id = pvi->source_id;
	vout_dev_info->mixer_setup.highlight_thresh = 0xff;
	vout_dev_info->mixer_setup.highlight_y  = 0xd2;
	vout_dev_info->mixer_setup.highlight_u  = 0x10;
	vout_dev_info->mixer_setup.highlight_v  = 0x92;
	vout_dev_info->mixer_setup.mixer_444	= 0;
	vout_dev_info->mixer_setup.csc_en	= 2;

	vout_dev_info->mixer_setup.csc_parms[0] = 0x00640204;
	vout_dev_info->mixer_setup.csc_parms[1] = 0xFED70108;
	vout_dev_info->mixer_setup.csc_parms[2] = 0xFF6701C0;
	vout_dev_info->mixer_setup.csc_parms[3] = 0xFFB7FE87;
	vout_dev_info->mixer_setup.csc_parms[4] = 0x001001C0;
	vout_dev_info->mixer_setup.csc_parms[5] = 0x00800080;
	vout_dev_info->mixer_setup.csc_parms[6] = 0x00FE0001;
	vout_dev_info->mixer_setup.csc_parms[7] = 0x00FE0001;
	vout_dev_info->mixer_setup.csc_parms[8] = 0x00FE0001;


	vout_dev_info->video_setup.cmd_code = CMD_VOUT_VIDEO_SETUP;
	vout_dev_info->video_setup.vout_id = pvi->source_id;
	vout_dev_info->default_img_setup.cmd_code = CMD_VOUT_DEFAULT_IMG_SETUP;
	vout_dev_info->default_img_setup.vout_id = pvi->source_id;

	vout_dev_info->dram_osd.osd_buf_dram_addr = 0;
	vout_dev_info->dram_osd.osd_buf_pitch = 0;
	vout_dev_info->dram_osd.osd_buf_repeat_field = 0;

	vout_dev_info->osd_setup.cmd_code = CMD_VOUT_OSD_SETUP;
	vout_dev_info->osd_setup.vout_id = pvi->source_id;
	vout_dev_info->osd_setup.osd_buf_info_dram_addr =
		(u32)&vout_dma_info->dram_osd;
	vout_dev_info->osd_buf_setup.cmd_code = CMD_VOUT_OSD_BUFFER_SETUP;
	vout_dev_info->osd_buf_setup.vout_id = pvi->source_id;
	vout_dev_info->osd_clut_setup.cmd_code = CMD_VOUT_OSD_CLUT_SETUP;
	vout_dev_info->osd_clut_setup.vout_id = pvi->source_id;
	vout_dev_info->osd_clut_setup.clut_dram_addr =
		(u32)&vout_dma_info->dram_clut;

	vout_dev_info->display_setup.cmd_code = CMD_VOUT_DISPLAY_SETUP;
	vout_dev_info->display_setup.vout_id = pvi->source_id;
	vout_dev_info->display_setup.disp_config_dram_addr =
		(u32)&vout_dma_info->dram_display;

	vout_dev_info->dve_setup.cmd_code = CMD_VOUT_DVE_SETUP;
	vout_dev_info->dve_setup.vout_id = pvi->source_id;
	vout_dev_info->dve_setup.dve_config_dram_addr =
		(u32)&vout_dma_info->dram_dve;

	vout_dev_info->reset.cmd_code = CMD_VOUT_RESET;
	vout_dev_info->reset.vout_id = pvi->source_id;

	vout_dev_info->csc_setup_flag = AMBA_VOUT_SETUP_NA;
	vout_dev_info->csc_setup.cmd_code = CMD_VOUT_DISPLAY_CSC_SETUP;
	vout_dev_info->csc_setup.vout_id = pvi->source_id;

	psrc = &vout_dev_info->video_source;
	psrc->id = pvi->source_id;
	psrc->source_type = pvi->source_type;
	strlcpy(psrc->name, pvi->name, sizeof(psrc->name));
	psrc->owner = THIS_MODULE;
	psrc->pinfo = vout_dev_info;
	psrc->docmd = amba_s2_vout_docmd;
	errorCode = amba_vout_add_video_source(psrc);
	if (errorCode) {
		vout_err("Adding VOUT adapter%d failed!\n", psrc->id);
		goto amba_s2_vout_probe_free_vout;
	}

	errorCode = request_irq(vout_dev_info->irq, amba_s2_vout_irq,
		IRQF_TRIGGER_RISING | IRQF_DISABLED, psrc->name, vout_dev_info);
	if (errorCode) {
		vout_err("Request IRQ%d failed!\n", vout_dev_info->irq);
		goto amba_s2_vout_probe_free_vout;
	}

	vout_dev_info->irq_proc_file = proc_create_data(pvi->name, S_IRUGO,
		get_ambarella_proc_dir(), &amba_s2_vout_proc_fops,
		&vout_dev_info->irq_proc_hinfo);
	if (vout_dev_info->irq_proc_file == NULL) {
		vout_err("Register %s failed!\n", pvi->name);
	}

	if (amb_async_proc_create(&vout_dev_info->event_proc)) {
		vout_err("Create event proc %s failed!\n", pvi->name);
	}

	vout_notice("%s:%d probed!\n", psrc->name, psrc->id);
	pvi->pvout_source = vout_dev_info;
	pvi->dma_address = dma_address;

	goto amba_s2_vout_probe_exit;

amba_s2_vout_probe_free_vout:
	kfree(vout_dev_info);
	pvi->pvout_source = NULL;

amba_s2_vout_probe_exit:
	return errorCode;
}

static int __exit amba_s2_vout_remove(
	struct amba_vout_system_instance_info *pvi)
{
	int					errorCode = 0;
	struct amba_vout_dev_info		*vout_dev_info;

	if (pvi->pvout_source) {
		vout_dev_info = pvi->pvout_source;
		free_irq(vout_dev_info->irq, vout_dev_info);
		remove_proc_entry(pvi->name, get_ambarella_proc_dir());
		amb_async_proc_remove(&vout_dev_info->event_proc);
		errorCode = amba_vout_del_video_source(
			&vout_dev_info->video_source);
		kfree(vout_dev_info);
		pvi->pvout_source = NULL;
		pvi->dma_address = 0;
	}

	return errorCode;
}

/* ========================================================================== */
static int __init amba_s2_vout_init(void)
{
	int					errorCode = 0;

	vout_dbg("amba_s2_vout_init\n");

	errorCode = amba_s2_vout_probe(&vout0_instance);
	if (errorCode) {
		goto amba_s2_vout_init_exit;
	}

	errorCode = amba_s2_vout_probe(&vout1_instance);
	if (errorCode) {
		goto amba_s2_vout_init_exit;
	}

amba_s2_vout_init_exit:
	return errorCode;
}

static void __exit amba_s2_vout_exit(void)
{
	vout_dbg("amba_s2_vout_exit\n");

	amba_s2_vout_remove(&vout0_instance);
	amba_s2_vout_remove(&vout1_instance);
}

module_init(amba_s2_vout_init);
module_exit(amba_s2_vout_exit);

MODULE_DESCRIPTION("Ambarella S2 Vout driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

