/*
 * include/ambas_vout.h
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

#ifndef __AMBAS_VOUT_H
#define __AMBAS_VOUT_H

#define AMBA_VOUT_NAME_LENGTH		(32)

#define AMBA_VOUT_SOURCE_TYPE_DVE	(1 << 0)
#define AMBA_VOUT_SOURCE_TYPE_DIGITAL	(1 << 1)
#define AMBA_VOUT_SOURCE_TYPE_HDMI	(1 << 2)
#define AMBA_VOUT_SOURCE_TYPE_MIPI	(1 << 3)

#define AMBA_VOUT_SOURCE_STARTING_ID	(0)
#define AMBA_VOUT_SINK_STARTING_ID	(0)

#include <ambas_common.h>

/* ========================= IAV VOUT struct ================================ */
enum amba_vout_display_device_type {
	AMBA_VOUT_DISPLAY_DEVICE_LCD		= 0,
	AMBA_VOUT_DISPLAY_DEVICE_HDMI,
};

enum amba_vout_sink_type {
	AMBA_VOUT_SINK_TYPE_AUTO = 0,
	AMBA_VOUT_SINK_TYPE_CVBS = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_SVIDEO = ((1 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_YPBPR = ((2 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_HDMI = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_HDMI),
	AMBA_VOUT_SINK_TYPE_DIGITAL = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_DIGITAL),
	AMBA_VOUT_SINK_TYPE_MIPI = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_MIPI),
};

enum amba_vout_sink_state {
	AMBA_VOUT_SINK_STATE_IDLE = 0,
	AMBA_VOUT_SINK_STATE_RUNNING,
	AMBA_VOUT_SINK_STATE_SUSPENDED,
};

enum amba_vout_sink_plug {
	AMBA_VOUT_SINK_PLUGGED = 0,
	AMBA_VOUT_SINK_REMOVED,
};

enum amba_vout_hdmi_color_space {
	AMBA_VOUT_HDMI_CS_AUTO = 0,
	AMBA_VOUT_HDMI_CS_RGB,
	AMBA_VOUT_HDMI_CS_YCBCR_444,
	AMBA_VOUT_HDMI_CS_YCBCR_422,
};

enum amba_vout_hdmi_overscan {
	AMBA_VOUT_HDMI_OVERSCAN_AUTO = 0,
	AMBA_VOUT_HDMI_NON_FORCE_OVERSCAN,
	AMBA_VOUT_HDMI_FORCE_OVERSCAN,
};

typedef enum {
	DDD_FRAME_PACKING		= 0,
	DDD_FIELD_ALTERNATIVE		= 1,
	DDD_LINE_ALTERNATIVE		= 2,
	DDD_SIDE_BY_SIDE_FULL		= 3,
	DDD_L_DEPTH			= 4,
	DDD_L_DEPTH_GRAPHICS_DEPTH	= 5,
	DDD_TOP_AND_BOTTOM		= 6,
	DDD_RESERVED			= 7,

	DDD_SIDE_BY_SIDE_HALF		= 8,

	DDD_UNSUPPORTED			= 16,
} ddd_structure_t;

struct amba_vout_video_size {
	u32 specified;
	u16 vout_width;		//Vout width
	u16 vout_height;	//Vout height
	u16 video_width;	//Video width
	u16 video_height;	//Video height
};

struct amba_vout_video_offset {
	u32 specified;
	s16 offset_x;
	s16 offset_y;
};

struct amba_vout_osd_size {
	u16 width;
	u16 height;
};

struct amba_vout_osd_rescale {
	u32 enable;
	u16 width;
	u16 height;
};

struct amba_vout_osd_offset {
	u32 specified;
	s16 offset_x;
	s16 offset_y;
};

struct amba_vout_window_info {
	u16 start_x;
	u16 start_y;
	u16 end_x;
	u16 end_y;
	u16 width;
	u16 field_reverse;
};

struct amba_vout_hv_sync_info {
	u16 hsync_start;
	u16 hsync_end;
	u16 vtsync_start;
	u16 vtsync_end;
	u16 vbsync_start;
	u16 vbsync_end;

	u16 vtsync_start_row;
	u16 vtsync_start_col;
	u16 vtsync_end_row;
	u16 vtsync_end_col;
	u16 vbsync_start_row;
	u16 vbsync_start_col;
	u16 vbsync_end_row;
	u16 vbsync_end_col;

	enum amba_vout_sink_type sink_type;
};

struct amba_vout_hv_size_info {
	u16 hsize;
	u16 vtsize;	//vsize for progressive
	u16 vbsize;
};

struct amba_vout_bg_color_info {
	u8 y;
	u8 cb;
	u8 cr;
};

enum amba_vout_display_input {
	AMBA_VOUT_INPUT_FROM_MIXER = 0,
	AMBA_VOUT_INPUT_FROM_SMEM,
};

enum amba_vout_lcd_mode_info {
	AMBA_VOUT_LCD_MODE_DISABLE = 0,
	AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT,
	AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT,
	AMBA_VOUT_LCD_MODE_RGB565,
	AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT,
	AMBA_VOUT_LCD_MODE_RGB888,
};

enum amba_vout_lcd_seq_info {
	AMBA_VOUT_LCD_SEQ_R0_G1_B2 = 0,
	AMBA_VOUT_LCD_SEQ_R0_B1_G2,
	AMBA_VOUT_LCD_SEQ_G0_R1_B2,
	AMBA_VOUT_LCD_SEQ_G0_B1_R2,
	AMBA_VOUT_LCD_SEQ_B0_R1_G2,
	AMBA_VOUT_LCD_SEQ_B0_G1_R2,
};

enum amba_vout_lcd_clk_edge_info {
	AMBA_VOUT_LCD_CLK_RISING_EDGE	= 0,
	AMBA_VOUT_LCD_CLK_FALLING_EDGE,
};

enum amba_vout_lcd_hvld_pol_info {
	AMBA_VOUT_LCD_HVLD_POL_HIGH	= 0,
	AMBA_VOUT_LCD_HVLD_POL_LOW,
};

enum amba_vout_lcd_model {
	AMBA_VOUT_LCD_MODEL_DIGITAL	= 0,
	AMBA_VOUT_LCD_MODEL_AUO27,
	AMBA_VOUT_LCD_MODEL_P28K,
	AMBA_VOUT_LCD_MODEL_TPO489,
	AMBA_VOUT_LCD_MODEL_TPO648,
	AMBA_VOUT_LCD_MODEL_TD043,
	AMBA_VOUT_LCD_MODEL_WDF2440,
	AMBA_VOUT_LCD_MODEL_1P3831,
	AMBA_VOUT_LCD_MODEL_1P3828,
	AMBA_VOUT_LCD_MODEL_EJ080NA,
	AMBA_VOUT_LCD_MODEL_AT070TNA2,
	AMBA_VOUT_LCD_MODEL_E330QHD,
	AMBA_VOUT_LCD_MODEL_PPGA3,
	AMBA_VOUT_LCD_MODEL_TJ030,
};

enum amba_vout_flip_info {
	AMBA_VOUT_FLIP_NORMAL = 0,
	AMBA_VOUT_FLIP_HV,
	AMBA_VOUT_FLIP_HORIZONTAL,
	AMBA_VOUT_FLIP_VERTICAL,
};

enum amba_vout_rotate_info {
	AMBA_VOUT_ROTATE_NORMAL,
	AMBA_VOUT_ROTATE_90,
};

enum amba_vout_tailored_info {
	AMBA_VOUT_OSD_NO_CSC	= 0x01,			//No Software CSC
	AMBA_VOUT_OSD_AUTO_COPY	= 0x02,			//Auto copy to other fb
};

struct amba_vout_lcd_info {
	enum amba_vout_lcd_mode_info		mode;
	enum amba_vout_lcd_seq_info		seqt;
	enum amba_vout_lcd_seq_info		seqb;
	enum amba_vout_lcd_clk_edge_info	dclk_edge;
	enum amba_vout_lcd_hvld_pol_info	hvld_pol;
	u32					dclk_freq_hz;	/* PLL_CLK_XXX */
	enum amba_vout_lcd_model		model;
};

struct amba_video_sink_mode {
	/* Sink */
	int				id;		//Sink ID
	u32				mode;		//enum amba_video_mode
	u32				ratio;		//AMBA_VIDEO_RATIO
	u32				bits;		//AMBA_VIDEO_BITS
	u32				type;		//AMBA_VIDEO_TYPE
	u32				format;		//AMBA_VIDEO_FORMAT
	u32				frame_rate;	//AMBA_VIDEO_FPS
	int				csc_en;		//enable csc or not
	struct amba_vout_bg_color_info	bg_color;
	enum amba_vout_display_input	display_input;	// input from SMEM or Mixer
	enum amba_vout_sink_type	sink_type;

	/* Video */
	int				video_en;	//enable video or not
	struct amba_vout_video_size	video_size;	//video size
	struct amba_vout_video_offset	video_offset;	//video offset
	enum amba_vout_flip_info	video_flip;	//flip
	enum amba_vout_rotate_info	video_rotate;	//rotate

	/* OSD */
	int				fb_id;		//frame buffer id
	struct amba_vout_osd_size	osd_size;	//OSD size
	struct amba_vout_osd_rescale	osd_rescale;	//OSD rescale
	struct amba_vout_osd_offset	osd_offset;	//OSD offset
	enum amba_vout_flip_info	osd_flip;	//flip
	enum amba_vout_rotate_info	osd_rotate;	//rotate
	enum amba_vout_tailored_info	osd_tailor;	//no csc, auto copy

	/* Misc */
	u32				direct_to_dsp;	//bypass iav
	struct amba_vout_lcd_info	lcd_cfg;	//LCD only
	enum amba_vout_hdmi_color_space hdmi_color_space;//HDMI only
	ddd_structure_t			hdmi_3d_structure;//HDMI only
	enum amba_vout_hdmi_overscan	hdmi_overscan;	//HDMI only

};

/* ========================= IAV VOUT IO defines ============================= */
#include <linux/ioctl.h>

#define IAV_IOC_VOUT_SOURCE_MAGIC	'$'

#define IAV_IOC_VOUT_GET_SINK_NUM	_IOR(IAV_IOC_VOUT_SOURCE_MAGIC, 1, int *)

#define IAV_IOC_VOUT_SINK_MAGIC		'#'

struct amba_vout_sink_info {
	int					id;		//Sink ID
	int					source_id;
	char					name[AMBA_VOUT_NAME_LENGTH];
	enum amba_vout_sink_type		sink_type;
	struct amba_video_sink_mode		sink_mode;
	enum amba_vout_sink_state		state;
	enum amba_vout_sink_plug		hdmi_plug;
	enum amba_video_mode			hdmi_modes[32];
	enum amba_video_mode			hdmi_native_mode;
	u16					hdmi_native_width;
	u16					hdmi_native_height;
	enum amba_vout_hdmi_overscan		hdmi_overscan;

};
#define IAV_IOC_VOUT_GET_SINK_INFO	_IOR(IAV_IOC_VOUT_SINK_MAGIC, 1, struct amba_vout_sink_info *)

#endif

