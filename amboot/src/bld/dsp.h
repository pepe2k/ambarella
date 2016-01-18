/**
 * tcldsp/bld/dsp.h
 *
 * History:
 *    2008/06/03 - [E-John Lien] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __DSP_H__
#define __DSP_H__


#if (VOUT_DIRECT_DSP_INTERFACE == 1)
/* define dsp operating mode */
typedef enum {
	ENCODE_MODE			= 0x00,
	DECODE_MODE			= 0x01,
} dsp_operation_mode;


/* VOUT commands */
#if (CHIP_REV == A7 || CHIP_REV == A6 || CHIP_REV == I1 || CHIP_REV == S2)
#define VOUT_MIXER_SETUP		0x07000001
#define VOUT_VIDEO_SETUP		0x07000002
#define VOUT_DEFAULT_IMG_SETUP		0x07000003
#define VOUT_OSD_SETUP			0x07000004
#define VOUT_OSD_BUFFER_SETUP		0x07000005
#define VOUT_OSD_CLUT_SETUP		0x07000006
#define VOUT_DISPLAY_SETUP		0x07000007
#define VOUT_DVE_SETUP			0x07000008
#define VOUT_RESET			0x07000009
#define VOUT_DISPLAY_CSC_SETUP		0x0700000A
#define VOUT_DIGITAL_OUTPUT_MODE_SETUP	0x0700000B
#define VOUT_GAMMA_SETUP		0x0700000C
#else
#define VOUT_MIXER_SETUP		0x00007001
#define VOUT_VIDEO_SETUP		0x00007002
#define VOUT_DEFAULT_IMG_SETUP		0x00007003
#define VOUT_OSD_SETUP			0x00007004
#define VOUT_OSD_BUFFER_SETUP		0x00007005
#define VOUT_OSD_CLUT_SETUP		0x00007006
#define VOUT_DISPLAY_SETUP		0x00007007
#define VOUT_DVE_SETUP			0x00007008
#define VOUT_RESET			0x00007009
#define VOUT_DISPLAY_CSC_SETUP		0x0000700A
#define VOUT_DIGITAL_OUTPUT_MODE_SETUP	0x0000700B
#endif

typedef enum {
	VOUT_SRC_DEFAULT_IMG = 0,
	VOUT_SRC_BACKGROUND  = 1,
	VOUT_SRC_ENC         = 2,
	VOUT_SRC_DEC         = 3,
	VOUT_SRC_H264_DEC    = 3,
	VOUT_SRC_MPEG2_DEC   = 5,
	VOUT_SRC_MPEG4_DEC   = 6,
	VOUT_SRC_MIXER_A     = 7,
	VOUT_SRC_VCAP        = 8,
} vout_src_t;

typedef enum {
	OSD_SRC_MAPPED_IN    = 0,
	OSD_SRC_DIRECT_IN    = 1,
} osd_src_t;

typedef enum {
	OSD_MODE_UYV565    = 0,
	OSD_MODE_AYUV4444  = 1,
	OSD_MODE_AYUV1555  = 2,
	OSD_MODE_YUV1555   = 3,
} osd_dir_mode_t;

typedef enum {
	CSC_DIGITAL	= 0,
	CSC_ANALOG	= 1,
	CSC_HDMI	= 2,
} csc_type_t;

//#endif
#if (CHIP_REV == A5S)
// (cmd code 0x00007001)
typedef struct vout_mixer_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u8  interlaced;
	u8  frm_rate;
	u16 act_win_width;
	u16 act_win_height;
	u8  back_ground_v;
	u8  back_ground_u;
	u8  back_ground_y;
	u8  reserved;
	u8  highlight_v;
	u8  highlight_u;
	u8  highlight_y;
	u8  highlight_thresh;
} vout_mixer_setup_t;
#else
// (cmd code 0x00007001)
typedef struct vout_mixer_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u8  interlaced;
	u8  frm_rate;
	u16 act_win_width;
	u16 act_win_height;
	u8  back_ground_v;
	u8  back_ground_u;
	u8  back_ground_y;
    	u8  mixer_444;
	u8  highlight_v;
	u8  highlight_u;
	u8  highlight_y;
	u8  highlight_thresh;
	u8  reverse_en;
	u8  csc_en;
	u8  reserved[2];
	u32 csc_parms[9];
} vout_mixer_setup_t;
#endif
// (cmd code 0x00007002)
typedef struct vout_video_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u8  en;
	u8  src;
	u8  flip;
	u8  rotate;
	u16 reserved;
	u16 win_offset_x;
	u16 win_offset_y;
	u16 win_width;
	u16 win_height;
	u32 default_img_y_addr;
	u32 default_img_uv_addr;
	u16 default_img_pitch;
	u8  default_img_repeat_field;
	u8  reserved2;
} vout_video_setup_t;

// (cmd code 0x00007003)
typedef struct vout_default_img_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 reserved;
	u32 default_img_y_addr;
	u32 default_img_uv_addr;
	u16 default_img_pitch;
	u8  default_img_repeat_field;
	u8  reserved2;
} vout_default_img_setup_t;

// (cmd code 0x00007004)
typedef struct vout_osd_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u8  en;
	u8  src;
	u8  flip;
	u8  rescaler_en;
	u8  premultiplied;
	u8  global_blend;
	u16 win_offset_x;
	u16 win_offset_y;
	u16 win_width;
	u16 win_height;
	u16 rescaler_input_width;
	u16 rescaler_input_height;
	u32 osd_buf_dram_addr;
	u16 osd_buf_pitch;
	u8  osd_buf_repeat_field;
	u8  osd_direct_mode;
	u16 osd_transparent_color;
	u8  osd_transparent_color_en;
	u8  osd_swap_bytes;
	u32 osd_buf_info_dram_addr;
} vout_osd_setup_t;

// (cmd code 0x00007005)
typedef struct vout_osd_buf_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 reserved;
	u32 osd_buf_dram_addr;
	u16 osd_buf_pitch;
	u8  osd_buf_repeat_field;
	u8  reserved2;
} vout_osd_buf_setup_t;

// (cmd code 0x00007006)
typedef struct vout_osd_clut_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 reserved;
	u32 clut_dram_addr;
} vout_osd_clut_setup_t;

// (cmd code 0x00007007)
typedef struct vout_display_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 reserved;
	u32 disp_config_dram_addr;
} vout_display_setup_t;

// (cmd code 0x00007008)
typedef struct vout_dve_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 reserved;
	u32 dve_config_dram_addr;
} vout_dve_setup_t;

// (cmd code 0x00007009)
typedef struct vout_reset_s
{
	u32 cmd_code;
	u16 vout_id;
	u8  reset_mixer;
	u8  reset_disp;
} vout_reset_t;

// (cmd code 0x0000700a)
typedef struct vout_display_csc_setup_s
{
	u32 cmd_code;
	u16 vout_id;
	u16 csc_type; // 0: digital; 1: analog; 2: hdmi
	u32 csc_parms[9];
} vout_display_csc_setup_t;

// (cmd code 0x0000700b)
typedef struct vout_digital_output_mode_s
{
        u32 cmd_code;
        u16 vout_id;
        u16 reserved;
        u32 output_mode;
} vout_digital_output_mode_setup_t;
#endif	//#if (VOUT_DIRECT_DSP_INTERFACE == 1)


#endif	/* __DSP_H__ */
