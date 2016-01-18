
/*
 * iav_encode_drv.h
 *
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * History:
 *	2010/1/28 - [Oliver Li] created file
 *	2010/12/3 - [Oliver Li] modified
 *		IAV_IOC_GET_DECODE_FRAME -> IAV_IOC_GET_DECODED_FRAME
 *		iav_decoded_frame_t	 -> iav_frame_buffer_t
 */

#ifndef __IAV_DECODE_DRV_H__
#define __IAV_DECODE_DRV_H__

/*
 * decoding APIs	- IAV_IOC_DECODE_MAGIC
 */
enum {
	IOC_START_DECODE = 0x00,
	IOC_STOP_DECODE = 0x01,

	IOC_DECODE_H264 = 0x02,
	IOC_DECODE_JPEG = 0x03,
	IOC_DECODE_MULTI = 0x04,

	IOC_DECODE_PAUSE = 0x05,
	IOC_DECODE_RESUME = 0x06,
	IOC_DECODE_STEP = 0x07,
	IOC_TRICK_PLAY = 0x08,

	IOC_GET_DECODE_INFO = 0x09,
	IOC_WAIT_BSB = 0x0a,
	IOC_WAIT_EOS = 0x0b,

	IOC_CONFIG_DECODER = 0x0c,
	IOC_CONFIG_DISPLAY = 0x0d,

	IOC_ENTER_UDEC_MODE = 0x0e,	// udec

	IOC_CONFIG_FB_POOL = 0x0f,	// udec
	IOC_REQUEST_FRAME = 0x10,	// udec
	IOC_GET_DECODED_FRAME = 0x11,	// udec
	IOC_RENDER_FRAME = 0x12,	// udec
	IOC_RELEASE_FRAME = 0x13,	// udec
	IOC_POSTP_FRAME = 0x14,	// udec

	IOC_WAIT_DECODER = 0x15,	// udec
	IOC_DECODE_FLUSH = 0x16,

	IOC_INIT_UDEC = 0x17,		// udec
	IOC_RELEASE_UDEC = 0x18,	// udec

	IOC_UDEC_DECODE = 0x19,	// udec
	IOC_UDEC_STOP = 0x1a,		// udec

	IOC_WAIT_UDEC_STATUS = 0x1b,	// udec
	IOC_GET_UDEC_STATE = 0x1c,	// udec

	IOC_SET_AUDIO_CLK_OFFSET = 0x1d, //udec
	IOC_WAKE_VOUT = 0x1e,		// udec
	IOC_UPDATE_VOUT_CONFIG = 0x1f,	// udec
	IOC_UDEC_TRICKPLAY = 0x20,	// udec

	IOC_UDEC_MAP = 0x21,	// udec
	IOC_UDEC_UNMAP = 0x22,	// udec

	IOC_UDEC_UPDATE_FB_POOL_CONFIG = 0x23, // udec

	IOC_ENTER_MDEC_MODE = 0x24,	// mdec

//	IOC_ENTER_MDEC_MODE_EX,	// mdec ex
	IOC_POSTP_WINDOW_CONFIG = 0x25,
	IOC_POSTP_RENDER_CONFIG = 0x26,
	IOC_POSTP_STREAM_SWITCH = 0x27,
	IOC_WAIT_STREAM_SWITCH_MSG = 0x28,

	IOC_UPDATE_VOUT_DEWARP_CONFIG = 0x29,

	IOC_POSTP_BUFFERING_CONTROL = 0x2a,

	IOC_UDEC_PB_SPEED = 0x2b,	// udec
	IOC_UDEC_CAPTURE = 0x2c,
	IOC_UDEC_ZOOM = 0x2d,

	IOC_POSTP_UPDATE_MW_DISPLAY = 0x2e,
	IOC_RELEASE_UDEC_CAPTURE = 0x2f,

	//udec transcode
	IOC_UDEC_INIT_TRANSCODER = 0x30,
	IOC_UDEC_RELEASE_TRANSCODER = 0x31,

	IOC_UDEC_START_TRANSCODER = 0x32,
	IOC_UDEC_STOP_TRANSCODER = 0x33,

	IOC_UDEC_TRANSCODER_READ_BITS = 0x34,

	//update on the fly
	IOC_UDEC_TRANSCODER_UPDATE_BITRATE = 0x35,
	IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE = 0x36,
	IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE = 0x37,
	IOC_UDEC_TRANSCODER_DEMAND_IDR = 0x38,

	IOC_DECODE_DBG = 0xff,
};


#define IAV_IOC_START_DECODE		_IO(IAV_IOC_DECODE_MAGIC, IOC_START_DECODE)
#define IAV_IOC_STOP_DECODE		_IO(IAV_IOC_DECODE_MAGIC, IOC_STOP_DECODE)

#define IAV_IOC_DECODE_H264		_IOW(IAV_IOC_DECODE_MAGIC, IOC_DECODE_H264, struct iav_h264_decode_s *)
#define IAV_IOC_DECODE_JPEG		_IOW(IAV_IOC_DECODE_MAGIC, IOC_DECODE_JPEG, struct iav_jpeg_info_s *)
#define IAV_IOC_DECODE_MULTI		_IOW(IAV_IOC_DECODE_MAGIC, IOC_DECODE_MULTI, struct iav_multi_info_s *)

#define IAV_IOC_DECODE_PAUSE		_IO(IAV_IOC_DECODE_MAGIC, IOC_DECODE_PAUSE)
#define IAV_IOC_DECODE_RESUME		_IO(IAV_IOC_DECODE_MAGIC, IOC_DECODE_RESUME)
#define IAV_IOC_DECODE_STEP		_IO(IAV_IOC_DECODE_MAGIC, IOC_DECODE_STEP)
#define IAV_IOC_TRICK_PLAY		_IOW(IAV_IOC_DECODE_MAGIC, IOC_TRICK_PLAY, struct iav_trick_play_s *)

#define IAV_IOC_GET_DECODE_INFO		_IOR(IAV_IOC_DECODE_MAGIC, IOC_GET_DECODE_INFO, struct iav_decode_info_s *)
#define IAV_IOC_WAIT_BSB		_IOR(IAV_IOC_DECODE_MAGIC, IOC_WAIT_BSB, struct iav_bsb_emptiness_s *)
#define IAV_IOC_WAIT_EOS		_IOW(IAV_IOC_DECODE_MAGIC, IOC_WAIT_EOS, struct iav_wait_eos_s *)

#define IAV_IOC_CONFIG_DECODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_CONFIG_DECODER, struct iav_config_decoder_s *)
#define IAV_IOC_CONFIG_DISPLAY		_IOW(IAV_IOC_DECODE_MAGIC, IOC_CONFIG_DISPLAY, struct iav_config_display_s *)

#define IAV_IOC_ENTER_UDEC_MODE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_ENTER_UDEC_MODE, struct iav_udec_mode_config_s *)

#define IAV_IOC_CONFIG_FB_POOL		_IOW(IAV_IOC_DECODE_MAGIC, IOC_CONFIG_FB_POOL, struct iav_fbp_config_s *)
#define IAV_IOC_REQUEST_FRAME		_IOR(IAV_IOC_DECODE_MAGIC, IOC_REQUEST_FRAME, struct iav_frame_buffer_s *)
#define IAV_IOC_GET_FRAME_BUFFER	IAV_IOC_REQUEST_FRAME

#define IAV_IOC_GET_DECODED_FRAME	_IOR(IAV_IOC_DECODE_MAGIC, IOC_GET_DECODED_FRAME, struct iav_frame_buffer_s *)
#define IAV_IOC_RENDER_FRAME		_IOW(IAV_IOC_DECODE_MAGIC, IOC_RENDER_FRAME, struct iav_frame_buffer_s *)
#define IAV_IOC_RELEASE_FRAME		_IOW(IAV_IOC_DECODE_MAGIC, IOC_RELEASE_FRAME, struct iav_frame_buffer_s *)
#define IAV_IOC_POSTP_FRAME		_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_FRAME, struct iav_frame_buffer_s *)

#define IAV_IOC_WAIT_DECODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_WAIT_DECODER, struct iav_wait_decoder_s *)
#define IAV_IOC_DECODE_FLUSH		_IO(IAV_IOC_DECODE_MAGIC, IOC_DECODE_FLUSH)

#define IAV_IOC_INIT_UDEC		_IOW(IAV_IOC_DECODE_MAGIC, IOC_INIT_UDEC, struct iav_udec_info_ex_s *)

// note: the parameter is same with IAV_IOC_UDEC_STOP
#define IAV_IOC_RELEASE_UDEC		_IOW(IAV_IOC_DECODE_MAGIC, IOC_RELEASE_UDEC, u32)

#define IAV_IOC_UDEC_DECODE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_DECODE, struct iav_udec_decode_s *)

// note:
//	low byte is udec_id
//	high byte: 0/1 - stop flag
//	high byte: 0xff - reset to READY state after STOP
#define IAV_IOC_UDEC_STOP		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_STOP, u32)

#define IAV_IOC_WAIT_UDEC_STATUS	_IOR(IAV_IOC_DECODE_MAGIC, IOC_WAIT_UDEC_STATUS, struct iav_udec_status_s *)
#define IAV_IOC_GET_UDEC_STATE		_IOR(IAV_IOC_DECODE_MAGIC, IOC_GET_UDEC_STATE, struct iav_udec_state_s *)
#define IAV_IOC_SET_AUDIO_CLK_OFFSET	_IOR(IAV_IOC_DECODE_MAGIC, IOC_SET_AUDIO_CLK_OFFSET, struct iav_audio_clk_offset_s *)
#define IAV_IOC_WAKE_VOUT		_IOR(IAV_IOC_DECODE_MAGIC, IOC_WAKE_VOUT, struct iav_wake_vout_s *)
#define IAV_IOC_UPDATE_VOUT_CONFIG	_IOW(IAV_IOC_DECODE_MAGIC, IOC_UPDATE_VOUT_CONFIG, struct iav_udec_vout_configs_s *)
#define IAV_IOC_UDEC_TRICKPLAY		_IOR(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRICKPLAY, struct iav_udec_trickplay_s *)
#define IAV_IOC_UDEC_PB_SPEED		_IOR(IAV_IOC_DECODE_MAGIC, IOC_UDEC_PB_SPEED, struct iav_udec_pb_speed_s *)

#define IAV_IOC_UDEC_MAP		_IOR(IAV_IOC_DECODE_MAGIC, IOC_UDEC_MAP, struct iav_udec_map_s *)
#define IAV_IOC_UDEC_UNMAP		_IO(IAV_IOC_DECODE_MAGIC, IOC_UDEC_UNMAP)

#define IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG	_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_UPDATE_FB_POOL_CONFIG, struct iav_fbp_config_s *)

#define IAV_IOC_ENTER_MDEC_MODE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_ENTER_MDEC_MODE, struct iav_mdec_mode_config_s *)
//obsolete blow
//#define IAV_IOC_ENTER_MDEC_MODE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_ENTER_MDEC_MODE, struct iav_mdec_mode_config_s *)
//#define IAV_IOC_ENTER_MDEC_MODE_EX		_IOW(IAV_IOC_DECODE_MAGIC, IOC_ENTER_MDEC_MODE_EX, struct iav_mdec_mode_config_ex_s *)
#define IAV_IOC_POSTP_WINDOW_CONFIG		_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_WINDOW_CONFIG, struct iav_postp_window_config_s *)
#define IAV_IOC_POSTP_RENDER_CONFIG		_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_RENDER_CONFIG, struct iav_postp_render_config_s *)
#define IAV_IOC_POSTP_STREAM_SWITCH		_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_STREAM_SWITCH, struct iav_postp_stream_switch_s *)
#define IAV_IOC_WAIT_STREAM_SWITCH_MSG		_IOW(IAV_IOC_DECODE_MAGIC, IOC_WAIT_STREAM_SWITCH_MSG, struct iav_wait_stream_switch_msg_s *)
#define IAV_IOC_POSTP_BUFFERING_CONTROL		_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_BUFFERING_CONTROL, struct iav_postp_buffering_control_s *)
#define IAV_IOC_POSTP_UPDATE_MW_DISPLAY	_IOW(IAV_IOC_DECODE_MAGIC, IOC_POSTP_UPDATE_MW_DISPLAY, struct iav_postp_update_mw_display_s*)

#define IAV_IOC_RELEASE_UDEC_CAPTURE	_IOW(IAV_IOC_DECODE_MAGIC, IOC_RELEASE_UDEC_CAPTURE, u32)

//dewarp related
#define IAV_IOC_UPDATE_VOUT_DEWARP_CONFIG	_IOW(IAV_IOC_DECODE_MAGIC, IOC_UPDATE_VOUT_DEWARP_CONFIG, struct iav_udec_vout_dewarp_config_s *)

//capture related
#define IAV_IOC_UDEC_CAPTURE	_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_CAPTURE, struct iav_udec_capture_s *)
#define IAV_IOC_UDEC_ZOOM	_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_ZOOM, struct iav_udec_zoom_s *)

#define IAV_IOC_DECODE_DBG		_IOW(IAV_IOC_DECODE_MAGIC, IOC_DECODE_DBG, u32)

#define IAV_IOC_GET_DECODE_FRAME	IAV_IOC_GET_DECODED_FRAME

//udec transcode related
#define IAV_IOC_UDEC_INIT_TRANSCODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_INIT_TRANSCODER, struct iav_udec_init_transcoder_s *)
#define IAV_IOC_UDEC_RELEASE_TRANSCODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_RELEASE_TRANSCODER, u32)
#define IAV_IOC_UDEC_START_TRANSCODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_START_TRANSCODER, struct iav_udec_start_transcoder_s *)
#define IAV_IOC_UDEC_STOP_TRANSCODER		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_STOP_TRANSCODER, struct iav_udec_stop_transcoder_s *)
#define IAV_IOC_UDEC_TRANSCODER_READ_BITS		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRANSCODER_READ_BITS, struct iav_udec_transcoder_bs_info_s *)
#define IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRANSCODER_UPDATE_BITRATE, struct iav_udec_transcoder_update_bitrate_s*)
#define IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE, struct iav_udec_transcoder_update_framerate_s *)
#define IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE, struct iav_udec_transcoder_update_gop_s *)
#define IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR		_IOW(IAV_IOC_DECODE_MAGIC, IOC_UDEC_TRANSCODER_DEMAND_IDR, struct iav_udec_transcoder_demand_idr_s *)

// input source type for multi window case
enum {
	MUDEC_INPUT_SRC_BACKGROUND_COLOR = 0,
	MUDEC_INPUT_SRC_UDEC = 1,
	MUDEC_INPUT_SRC_DESIGNATED_IMAGE = 2,
	MUDEC_INPUT_SOURCE_NUM,
};

typedef struct iav_h264_decode_s {
	u8	*start_addr;
	u8	*end_addr;	// exclusive
	u32	first_display_pts;
	u32	num_pics;
	u32	next_size;
	u32	pts;
	u32	pts_high;
	u32	pic_width;
	u32	pic_height;
} iav_h264_decode_t;


typedef struct iav_jpeg_info_s {
	u8	*start_addr;
	u32	size;
} iav_jpeg_info_t;


#define SCENE_JPEG	0
#define SCENE_H264_I	1

typedef struct iav_scene_s {
	u16	offset_x;
	u16	offset_y;
	u16	width;
	u16	height;
	u8	*addr;
	u32	size : 24;
	u32	type : 8;
} iav_scene_t;


#define MAX_SCENE	6
typedef struct iav_multi_info_s {
	u32		scene_count;
	iav_scene_t	scenes[MAX_SCENE];
} iav_multi_info_t;


typedef struct iav_trick_play_s {
	u16		speed;
	u8		scan_mode;
	u8		direction;
} iav_trick_play_t;


typedef struct iav_decode_info_s {
	u32		curr_pts;
	u32		curr_pts_high;
	u32		decoded_frames;
} iav_decode_info_t;


typedef struct iav_bsb_emptiness_s {
	u32		room;		// input & output
	u8		*start_addr;	// input
	u8		*end_addr;	// output
} iav_bsb_emptiness_t;

typedef struct iav_wait_eos_s {
	u32		eos_pts;
	u32		delta;
} iav_wait_eos_t;

#define IAV_DEFAULT_DECODER	0
#define IAV_SIMPLE_DECODER	1
#define IAV_SOFTWARE_DECODER	2

typedef struct iav_config_decoder_s {
	u16		flags;
	u16		decoder_type;
	u16		chroma_format;
	u16		num_frame_buffer;	// out
	u16		pic_width;
	u16		pic_height;
	u16		fb_width;		// out
	u16		fb_height;		// out
} iav_config_decoder_t;

typedef struct iav_rect_s {
	s16		left;
	s16		top;
	s16		right;
	s16		bottom;
} iav_rect_t;


#define DISP_MODE_FIT		0
#define DISP_MODE_FILL		1
#define DISP_MODE_FIT_DEST	2
#define DISP_MODE_FILL_DEST	3
#define DISP_MODE_CUSTOMIZE	4

typedef struct iav_config_display_s {
	u8		mode;
	u8		vout_id;
	u16		flags;
	iav_rect_t	src;
	iav_rect_t	dest;
} iav_config_display_t;

#define IAV_FRAME_NEED_ADDR		(1 << 0)
#define IAV_FRAME_NEED_SYNC		(1 << 1)
#define IAV_FRAME_LAST			(1 << 2)
#define IAV_FRAME_FORCE_RELEASE		(1 << 3)	// deprecated
#define IAV_FRAME_SYNC_VOUT		(1 << 4)	// render every frame
#define IAV_FRAME_NO_WAIT		(1 << 5)	// return error immediately if no fb
#define IAV_FRAME_NO_RELEASE		(1 << 6)	// do not release the frame buffer after rendering
#define IAV_FRAME_PADDR			(1 << 7)	// return physical address for lu_buf_addr/ch_buf_addr

#define IAV_INVALID_FB_ID		0xFFFF

typedef struct iav_frame_buffer_s {
	u16		flags;		// for ioctl
	u16		fb_id;		// frame buffer id, or IAV_INVALID_FB_ID

	u8		fb_format;
	u8		coded_frame;
	u8		chroma_format;
	u8		if_video;

	u32		pts;
	u32		pts_high;

	u16		buffer_width;
	u16		buffer_height;
	u16		buffer_pitch;
	u16		real_fb_id;	// out - fb_id used by ucode

	u8		decoder_id;	// in
	u8		eos_flag;	// out
	u8		pic_struct;	// in - IAV_IOC_REQUEST_FRAME
	u8		interlaced;	// in - IAV_IOC_POSTP_FRAME

	u16		pic_width;
	u16		pic_height;

	u16		lu_off_x;
	u16		lu_off_y;
	u16		ch_off_x;
	u16		ch_off_y;

	u32		top_or_frm_word;
	u32		bot_word;

	void		*lu_buf_addr;
	void		*ch_buf_addr;
} iav_frame_buffer_t;

typedef iav_frame_buffer_t iav_decoded_frame_t;

#define IAV_WAIT_BSB		(1 << 0)
#define IAV_WAIT_FRAME		(1 << 1)
#define IAV_WAIT_EOS		(1 << 2)
#define IAV_WAIT_BUFFER		(1 << 3)

#define IAV_WAIT_BITS_FIFO	(1 << 4)	// udec
#define IAV_WAIT_MV_FIFO	(1 << 5)	// udec
#define IAV_WAIT_OUTPIC		(1 << 6)	// udec
#define IAV_WAIT_UDEC_EOS	(1 << 7)	// udec
#define IAV_WAIT_UDEC_ERROR	(1 << 8)	// udec
#define IAV_WAIT_VOUT_STATE	(1 << 9)	// udec

//ugly code: each vdsp interrupt
#define IAV_WAIT_VDSP_INTERRUPT  (1 << 10)	// udec

typedef struct iav_wait_decoder_s {
	u32		flags;
	iav_bsb_emptiness_t emptiness;

	int		num_decoded_frames;

	// for udec
	u8		decoder_id;	// in
	u8		vout_state;	// out
	u8		reserved[2];

	iav_bsb_emptiness_t mv_emptiness;
} iav_wait_decoder_t;

// udec_type
enum {
	UDEC_NONE,	// invalid
	UDEC_H264,
	UDEC_MP12,
	UDEC_MP4H,
	UDEC_MP4S,
	UDEC_VC1,
	UDEC_RV40,
	UDEC_JPEG,
	UDEC_SW,
	UDEC_LAST,
};

typedef struct iav_udec_config_s {
	u8	tiled_mode;	// tiled mode, valid value is 0, 4 or 5, if enable
	u8	frm_chroma_fmt_max;	// 0: 420; 1: 422
	u8	dec_types;
	u8	max_frm_num;
	u16	max_frm_width;
	u16	max_frm_height;
	u32	max_fifo_size;
} iav_udec_config_t;

typedef struct iav_udec_deint_config_s {
	u8	init_tff;	// top_field_first
	u8	deint_lu_en;	// 1
	u8	deint_ch_en;	// 1
	u8	osd_en;		// 0

	u8	deint_mode;	// for 1080i60 input, you need to set "1" here, for 480i/576i input, you can set it to "0" or "1".

	u8	deint_spatial_shift;	// 0
	u8	deint_lowpass_shift;	// 7

	u8	deint_lowpass_center_weight;	// 112
	u8	deint_lowpass_hor_weight;	// 2
	u8	deint_lowpass_ver_weight;	// 4

	u8	deint_gradient_bias;	// 15
	u8	deint_predict_bias;	// 15
	u8	deint_candidate_bias;	// 10

	s16	deint_spatial_score_bias;	// 5
	s16	deint_temporal_score_bias;	// 5
} iav_udec_deint_config_t;

typedef struct iav_udec_mode_feature_constrain_s {
	u8	set_constrains_enable;//enable or disable
	u8	always_disable_l2_cache;

	//h264
	u8	h264_no_fmo;
	u8	h264_no_cabac;
	u8	h264_no_B_frame;

	//vout
	u8	vout_no_lcd;
	u8	vout_no_hdmi;

	//postp related
	u8	no_deinterlacer;
} iav_udec_mode_feature_constrain_t;

enum {
	DSP_ENCRM_TYPE_NONE = 0,
	DSP_ENCRM_TYPE_H264 = 2,
};

typedef struct iav_udec_transcoder_config_s {
	u8 main_type;
	u8 pip_type;
	u8 piv_type;
	u8 mctf_flag;

	u16 encoding_width;
	u16 encoding_height;

	u16 pip_encoding_width;
	u16 pip_encoding_height;
} iav_udec_transcoder_config_t;

typedef struct iav_udec_transcoders_config_s {
	u8 total_channel_number;
	u8 reserved0;
	u8 reserved1;
	u8 reserved2;

	iav_udec_transcoder_config_t transcoder_config[2];
} iav_udec_transcoders_config_t;

// IAV_IOC_ENTER_UDEC_MODE
typedef struct iav_udec_mode_config_s {
	u8	postp_mode;		// 0: no pp; 1: decpp; 2: voutpp
	u8	enable_deint;		// 1: enable deinterlacer
	u8	pp_chroma_fmt_max;	// 0: mon; 1: 420; 2: 422; 3: 444
	u8	enable_error_mode;	// obsolete

	//enable dewarp feature
	u8	enable_horizontal_dewarp;
	u8	enable_vertical_dewarp;
	u8	primary_vout;
	u8	enable_transcode;

	u16	pp_max_frm_width;
	u16	pp_max_frm_height;
	u16	pp_max_frm_num;

	u8	vout_mask;		// bit 0: use VOUT 0; bit 1: use VOUT 1
	u8	num_udecs;

	u8 pp_background_Y;
	u8 pp_background_Cb;
	u8 pp_background_Cr;
	u8 cur_display_vout_mask;

	iav_udec_config_t *udec_config;	// array of udec configurations
	iav_udec_mode_feature_constrain_t feature_constrains;//feature constrains

	iav_udec_deint_config_t *deint_config;

	iav_udec_transcoders_config_t udec_transcoder_config;
} iav_udec_mode_config_t;

typedef struct iav_udec_vout_config_s {
	u8	vout_id;		// 0, 1

	u8	udec_id;		// notice: it is removed!

	u8	disable;		// 1 - disable this vout
	u8	flip;			// 0 - no flip;
					// 1 - flip vertically and horizontally
					// 2 - flip horizontally
					// 3 - flip vertically

	u8	rotate;			// 0 - no rotation
					// 1 - rotate by 90 degree

	u16	target_win_offset_x;
	u16	target_win_offset_y;

	u16	target_win_width;
	u16	target_win_height;

	u16	win_offset_x;
	u16	win_offset_y;
	u16	win_width;
	u16	win_height;

	u32	zoom_factor_x;
	u32	zoom_factor_y;
} iav_udec_vout_config_t;

enum {
	IAV_DEWARP_PARAMS_INVALID = 0x0,
	IAV_DEWARP_PARAMS_CUSTOMIZED_TABLE = 0x1,
	IAV_DEWARP_PARAMS_TOP_BOTTOM_WIDTH = 0x2,
	IAV_DEWARP_PARAMS_FOUR_POINT = 0x3,

	IAV_ALL_UDEC_TAG = 0xff,
};

typedef struct iav_udec_vout_dewarp_config_s {
	u8	udec_id;	// need to be set with IAV_IOC_UPDATE_VOUT_CONFIG
	u8	reserved0[3];

	// horizontal de-warpping
	u8	vout_rotated;
	u8	horz_warp_enable;
	u8	warp_table_type;// 1: customized table; 2: param0/param1 means top/bottom width; 3:specify 4 points of wrap area
	u8	use_customized_dewarp_params;

	s16*	warp_horizontal_table_address;

	u8	grid_array_width;// related to vout dimention
	u8	grid_array_height;// related to vout dimention
	u8	horz_grid_spacing_exponent;
	u8	vert_grid_spacing_exponent;

	u16 warp_param0, warp_param1;
	u16 warp_param2, warp_param3;
	u16 warp_param4, warp_param5;
	u16 warp_param6, warp_param7;
} iav_udec_vout_dewarp_config_t;

typedef struct iav_udec_vout_configs_s {
	u8	udec_id;	// need to be set with IAV_IOC_UPDATE_VOUT_CONFIG

	u32	num_vout;	// in
	iav_udec_vout_config_t *vout_config;

	u32	first_pts_low;
	u32	first_pts_high;

	u16	input_center_x;
	u16	input_center_y;

	//dewarp related:
	u8	dewarp_params_updated;//if vout0's width/height changes, need enable it
	u8	reserved1[3];
	u16	dewarp_vout_width;//after rotated
	u16	dewarp_vout_height;//after rotated
	iav_udec_vout_dewarp_config_t dewarp;
} iav_udec_vout_configs_t;

typedef iav_udec_vout_configs_t iav_udec_display_t;

#define IAV_UDEC_VALIDATION_ONLY	(1 << 0)
#define IAV_UDEC_FORCE_DECODE		(1 << 1)

// IAV_IOC_INIT_UDEC
typedef struct iav_udec_info_ex_s {
	u8	udec_id;
	u8	udec_type;

	u8	enable_pp;
	u8	enable_deint;

	// the vout used by this udec
	u8	interlaced_out;
	u8	packed_out; // 0: planar yuv; 1: packed yuyv
	u8	out_chroma_format; // 0: 420; 1: 422

	u8	enable_err_handle;
	u8	other_flags;	// IAV_UDEC_VALIDATION_ONLY, IAV_UDEC_FORCE_DECODE

	u8	noncachable_buffer;
	u8	reserved0[2];

	// vout config
	iav_udec_vout_configs_t vout_configs;

	u32	bits_fifo_size;
	u32	ref_cache_size;

	u16	concealment_mode;	// 0 - use the last good picture as the concealment reference frame
					// 1 - use the last displayed as the concealment reference frame
					// 2 - use the frame given by ARM as the concealment reference frame
	u16	concealment_ref_frm_buf_id;	// used only when concealment_mode is 2

	// depends on udec_type
	union {
		struct {	// H.264
			u32	pjpeg_buf_size;
		} h264;

		struct {	// MPEG2, MPEG4
			u32	deblocking_flag;
			u32	pquant_mode;
			u8	pquant_table[32];
			u32	is_avi_flag;
		} mpeg;

		struct {	// software MPEG4
			u32	mv_fifo_size;
			u32	deblocking_flag;
			u32	pquant_mode;
			u8	pquant_table[32];
		} mp4s;

		struct {	// RV40
			u32	mv_fifo_size;
		} rv40;

		struct {	// jpeg
			u16	still_bits_circular;
			u16	reserved;
			u16	still_max_decode_width;
			u16	still_max_decode_height;
		} jpeg;
	} u;

	u8	*bits_fifo_start;	// out
	u8	*mv_fifo_start;		// out
} iav_udec_info_ex_t;

// obsolete - please use iav_udec_info_ex_t
// IAV_IOC_INIT_UDEC
typedef struct iav_udec_info_s {
	u8	udec_id;
	u8	udec_type;
	u8	enable_pp;
	u8	enable_deint;

	// the vout used by this udec
	u8	interlaced_out;
	u8	packed_out; // 0: planar yuv; 1: packed yuyv
	u8	reserved[2];

	// vout config
	u32	num_vout;
	iav_udec_vout_config_t *vout_config;

	u32	first_pts_low;
	u32	first_pts_high;

	u16	input_center_x;
	u16	input_center_y;

	u32	bits_fifo_size;
	u32	ref_cache_size;

	u16	concealment_mode;	// 0 - use the last good picture as the concealment reference frame
					// 1 - use the last displayed as the concealment reference frame
					// 2 - use the frame given by ARM as the concealment reference frame
	u16	concealment_ref_frm_buf_id;	// used only when concealment_mode is 2

	// depends on udec_type
	union {
		struct {	// H.264
			u32	pjpeg_buf_size;
		} h264;

		struct {	// MPEG2, MPEG4
			u32	deblocking_flag;
		} mpeg;

		struct {	// software MPEG4
			u32	mv_fifo_size;
			u32	deblocking_flag;
		} mp4s;

		struct {	// RV40
			u32	mv_fifo_size;
		} rv40;

		struct {	// jpeg
			u16	still_bits_circular;
			u16	reserved;
			u16	still_max_decode_width;
			u16	still_max_decode_height;
		} jpeg;
	} u;

	u8	*bits_fifo_start;	// out
	u8	*mv_fifo_start;		// out
} iav_udec_info_t;

// IAV_IOC_UDEC_DECODE
typedef struct iav_udec_decode_s {
	u8	udec_type;
	u8	decoder_id;
	u16	num_pics;

	union {
		struct {		// for H264, MP12, MP4H, VC1, JPEG
			u8 *start_addr;
			u8 *end_addr;	// exclusive
		} fifo;

		struct {	// hybrid MPEG4 - obsolete
			u8 *vop_coef_daddr;
			u32 vop_coef_size;

			u8 *mv_coef_daddr;
			u32 mv_coef_size;

			u32 vop_width;
			u32 vop_height;
			u32 vop_time_incre_res;

			u32 vop_pts_high;
			u32 vop_pts_low;

			u8 vop_code_type;
			u8 vop_chroma;
			u8 vop_round_type;
			u8 vop_interlaced;
			u8 vop_top_field_first;
			u8 end_of_sequence;
		} mp4s;

		struct {	// hybrid MPEG4
			u8 *vop_coef_start_addr;
			u8 *vop_coef_end_addr;
			u8 *vop_mv_start_addr;
			u8 *vop_mv_end_addr;
		} mp4s2;

		struct {	// RV40
			u8 *residual_fifo_start;
			u8 *residual_fifo_end;	// exclusive

			u8 *mv_fifo_start;
			u8 *mv_fifo_end;	// exclusive

			u16 fwd_ref_fb_id;
			u16 bwd_ref_fb_id;
			u16 target_fb_id;

			u16 pic_width;
			u16 pic_height;

			u8 pic_coding_type;
			u8 tiled_mode;

			u8 clean_fwd_ref_fb;	// clean cache for fwd_ref_fb_id
			u8 clean_bwd_ref_fb;	// clean cache for bwd_ref_fb_id
		} rv40;
	} u;
} iav_udec_decode_t;

typedef struct iav_udec_status_s {
	u8	decoder_id;	// in
	u8	nowait;		// in
	u8	only_query_current_pts;	//in and out
	u8	eos_flag;	// out
	u32	clock_counter;	// out
	u32	pts_low;	// out
	u32	pts_high;	// out

	u8	pts_valid;	//out
	u8	udec_state;
	u8	vout_state;
	u8	reserved0;
} iav_udec_status_t;

enum {
	IAV_UDEC_STATE_IDLE = 2,
	IAV_UDEC_STATE_READY = 3,
	IAV_UDEC_STATE_RUN = 4,
	IAV_UDEC_STATE_ERROR = 9,
};

enum {
	IAV_VOUT_STATE_INVALID = 0,
	IAV_VOUT_STATE_INIT = 1,
	IAV_VOUT_STATE_IDLE_2_RUN = 2,
	IAV_VOUT_STATE_IDLE = 3,
	IAV_VOUT_STATE_PRE_RUN = 4,
	IAV_VOUT_STATE_DORMANT = 5,
	IAV_VOUT_STATE_RUN = 6,
	IAV_VOUT_STATE_RUN_2_IDLE = 7,
	IAV_VOUT_STATE_PAUSE = 8,
	IAV_VOUT_STATE_STEP = 9,
};

#define IAV_UDEC_STATE_CLEAR_ERROR	(1 << 1)	// clear the error code in IAV
#define IAV_UDEC_STATE_DSP_READ_POINTER	(1 << 2)
#define IAV_UDEC_STATE_BTIS_FIFO_ROOM	(1 << 3)
#define IAV_UDEC_STATE_ARM_WRITE_POINTER	(1 << 4)

typedef struct iav_udec_state_s {
	u8	decoder_id;	// in
	u8	flags;		// in
	u8	udec_state;	// out
	u8	vout_state;	// out
	u32	error_code;	// out
	u32	error_pic_pts_low;	// out
	u32	error_pic_pts_high;	// out

	u32	current_pts_low;		// out
	u32	current_pts_high;	// out

//blow only for debug purpose

	//usr space
	u8* dsp_current_read_bitsfifo_addr;//out
	u8* dsp_current_read_mvfifo_addr;//out
	u8* arm_last_write_bitsfifo_addr;//out
	u8* arm_last_write_mvfifo_addr;//out

	//physical addr
	u8* dsp_current_read_bitsfifo_addr_phys;//out
	u8* dsp_current_read_mvfifo_addr_phys;//out
	u8* arm_last_write_bitsfifo_addr_phys;//out
	u8* arm_last_write_mvfifo_addr_phys;//out

	//bit-stream fifo info
	u32 bits_fifo_total_size;//out
	u32 bits_fifo_free_size;//out
	u32 bits_fifo_phys_start;//out

	//decode cmd count
	u32 tot_decode_cmd_cnt;//out
	u32 tot_decode_frame_cnt;//out
} iav_udec_state_t;

typedef struct iav_audio_clk_offset_s {
	u8	decoder_id;
	s32	audio_clk_offset;
} iav_audio_clk_offset_t;

typedef struct iav_wake_vout_s {
	u8	decoder_id;
} iav_wake_vout_t;

typedef struct iav_udec_trickplay_s {
	u8	decoder_id;
	u8	mode;		//0: PAUSE; 1: RESUME; 2: STEP
} iav_udec_trickplay_t;

typedef struct iav_udec_pb_speed_s {
	u16	speed;      // 0: play as fast as possible; otherwise: 8bits.8bits fractional supported speed control
	u8	decoder_id;
	u8	scan_mode;  // 0: IPB_ALL; 1: REF_ONLY; 2: I_ONLY
	u8	direction;  // 0: DIR_FWD; 1: DIR_BWD
	u8	reserved[3];
} iav_udec_pb_speed_t;

//[31:26] - reserved
//[25] - if set, the current value of the counter will not change
//[24] - if set, the maximum value of the counter will not change
//[23:12] - new count value of the counter            // this indicates how many MB rows have been decoded. DSP will increase the field by one when one MB row is decoded.
//[11:0] - new maximum value for the counter
typedef struct iav_udec_map_s {
	u32	*rv_sync_counter;	// address of the sync counter
} iav_udec_map_t;

typedef struct iav_fbp_config_s {
	u8	decoder_id;
	u8	chroma_format;	// 1 - 4:2:0
	u8	tiled_mode;

	u16	fbp_id;		// should be 0 for now

	u16	buf_width;
	u16	buf_height;

	u16	lu_width;
	u16	lu_height;

	u16	lu_row_offset;
	u16	lu_col_offset;
	u16	ch_row_offset;
	u16	ch_col_offset;

	u16	num_frm_bufs;	// out
} iav_fbp_config_t;

typedef struct iav_fbp_info_s {
	u16	max_frm_num;

	u16	buffer_width;
	u16	buffer_height;

	u8	decoder_id;
	u8	chroma_format;	// 1 - 4:2:0
	u8	tile_mode;	// 0 - no tiling

	u16	fbp_id;		// out
	u16	num_fb;		// out
} iav_fbp_info_t;

//obsolete structure
#if 0
typedef struct iav_udec_window_s {
	u8	udec_id;
	u8	input_src_type;
	u8	av_sync;
	u8	luma_gain;

	u32	first_pts_low;
	u32	first_pts_high;
	u16	input_offset_x;
	u16	input_offset_y;
	u16	input_width;
	u16	input_height;

	u8	voutA_rotation;
	u8	voutA_flip;
	u16	voutA_target_win_offset_x;
	u16	voutA_target_win_offset_y;
	u16	voutA_target_win_width;
	u16	voutA_target_win_height;

	u8	voutB_rotation;
	u8	voutB_flip;
	u16	voutB_target_win_offset_x;
	u16	voutB_target_win_offset_y;
	u16	voutB_target_win_width;
	u16	voutB_target_win_height;
} iav_udec_window_t;

typedef struct iav_mdec_mode_config_s {
	iav_udec_mode_config_t super;
	int num_windows;
	iav_udec_window_t *windows;
} iav_mdec_mode_config_t;
#endif

//new api from dsp
typedef struct udec_window_s {
	u8	win_config_id;
	u8	reserved0;

	u16	input_offset_x;
	u16	input_offset_y;
	u16	input_width;
	u16	input_height;

	u16	target_win_offset_x;
	u16	target_win_offset_y;
	u16	target_win_width;
	u16	target_win_height;
} udec_window_t;

typedef struct udec_render_s {
	u8	render_id;
	u8	win_config_id;
	u8	win_config_id_2nd;
	u8	udec_id;

	u32	first_pts_low;
	u32	first_pts_high;

	u8	input_source_type;
	u8	reserved[3];
} udec_render_t;

typedef struct iav_postp_render_config_s {
	u8	total_num_windows_to_render;
	u8	num_configs;
	u8	reserved0[2];

	udec_render_t	configs[12];
} iav_postp_render_config_t;

typedef struct iav_postp_window_config_s {
	u8	num_configs;
	u8	reserved0[3];

	udec_window_t	configs[12];
} iav_postp_window_config_t;

typedef struct iav_mdec_mode_config_s {
	iav_udec_mode_config_t super;

	u8	max_num_windows;
	u8	total_num_render_configs;
	u8	reserved0[2];

	u32	total_num_win_configs : 8;
	u32	audio_on_win_id : 8;
	u32	pre_buffer_len : 8;
	u32	enable_buffering_ctrl : 1;
	u32	av_sync_enabled : 1;
	u32	voutB_enabled : 1;
	u32	voutA_enabled : 1;
	u32	resereved : 4;

	udec_window_t *windows_config;
	udec_render_t *render_config;

	u16 video_win_width; //The overall width of output video window
	u16 video_win_height;//The overall height of output video window

	u32 frame_rate_in_ticks; //postp framerate: 3000: 30fps; 3003; 29.97fps; 1500: 60fps; 9000: 10fps
} iav_mdec_mode_config_t;

typedef struct udec_stream_switch_s {
	u8	render_id;
	u8	new_udec_id;
	u8	seamless;
	u8	reserved;
} udec_stream_switch_t;

typedef struct iav_postp_stream_switch_s {
	u8	num_config;
	udec_stream_switch_t	switch_config[16];
} iav_postp_stream_switch_t;

#define IAV_FLAGS_WAIT_NON_BLOCK (1<<0)
typedef struct iav_wait_stream_switch_msg_s {
	u32	switch_status;
	u8	render_id;
	u8	reserved0;
	u16	wait_flags;
} iav_wait_stream_switch_msg_t;

typedef struct iav_postp_buffering_control_s {
	u32	stream_id :8;
	u32	control_direction : 8; // 0: repeat one frame; 1: drop one frame;
	u32	frame_time : 16;   // frame time in 90kHz;e.g. for 30fps stream, it should be 3000.
} iav_postp_buffering_control_t;

typedef struct udec_capture_s {
	u32	buffer_base;
	u32	buffer_limit;

	u32	quality;
	u32	quant_table_addr;

	u16	target_pic_width;
	u16	target_pic_height;

	u8	specify_letterbox_related;
	u8	letterbox_strip_y;
	u8	letterbox_strip_cb;
	u8	letterbox_strip_cr;

	u16	letterbox_strip_width;
	u16	letterbox_strip_height;
} udec_capture_t;

enum {
	CAPTURE_CODED = 0,
	CAPTURE_THUMBNAIL,
	CAPTURE_SCREENNAIL,

	CAPTURE_TOT_NUM,
};

typedef struct iav_udec_capture_s {
	u8	dec_id;
	u8	capture_coded;
	u8	capture_thumbnail;
	u8	capture_screennail;

	udec_capture_t	capture[CAPTURE_TOT_NUM];
} iav_udec_capture_t;

typedef struct iav_udec_zoom_s {
	u32	render_id :8;
	u32	reserved :24;
	u16	input_center_x;
	u16	input_center_y;
	u32	zoom_factor_x;
	u32	zoom_factor_y;
	u16	input_width;
	u16	input_height;
} iav_udec_zoom_t;

typedef struct iav_postp_update_mw_display_s {
	u8	max_num_windows;
	u8	total_num_render_configs;
	u8	reserved1[2];

	u32	total_num_win_configs : 8;
	u32	audio_on_win_id : 8;
	u32	pre_buffer_len  : 8;
	u32	enable_buffering_ctrl : 1;
	u32	av_sync_enabled : 1;
	u32	voutB_enabled   : 1;
	u32	voutA_enabled   : 1;
	u32	resereved : 4;

	udec_window_t *windows_config;
	udec_render_t *render_config;

	u16 video_win_width; //The overall width of output video window
	u16 video_win_height;//The overall height of output video window

	u32 frame_rate_in_ticks; //postp framerate: 3000: 30fps; 3003; 29.97fps; 1500: 60fps; 9000: 10fps
} iav_postp_update_mw_display_t;

// transcoding
enum {
	IAV_UDEC_TRANSCODER_STATE_INIT = 0,
	IAV_UDEC_TRANSCODER_STATE_IDLE,
	IAV_UDEC_TRANSCODER_STATE_STOPPED,
	IAV_UDEC_TRANSCODER_STATE_RUNNING,
	IAV_UDEC_TRANSCODER_STATE_STOPPING,
};

enum {
	STREAM_TYPE_FULL_RESOLUTION = 0x11,
//	STREAM_TYPE_PIP_RESOLUTION = 0x12,
};

typedef struct iav_udec_init_transcoder_s {
	u32	flags;

	u8	id;
	u8	stream_type;
	u8	profile_idc;
	u8	level_idc;

	u16	encoding_width;
	u16	encoding_height;

	u16	source_window_width;
	u16	source_window_height;
	u16	source_window_offset_x;
	u16	source_window_offset_y;

	u8	num_mbrows_per_bitspart;
	u8	M;
	u8	N;
	u8	idr_interval;
	u8	gop_structure;
	u8	numRef_P;
	u8	numRef_B;
	u8	use_cabac;
	u16	quality_level;

	u32	average_bitrate;

	u8	vbr_setting;		// 1 - CBR, 2 - Pseudo CBR, 3 - VBR
	u8	calibration;
	u8	vbr_ness;
	u8	min_vbr_rate_factor;	// 0 - 100
	u16	max_vbr_rate_factor;	// 0 - 400

	// out
	u8	*bits_fifo_start;
	u32	bits_fifo_size;
} iav_udec_init_transcoder_t;

typedef struct iav_udec_start_transcoder_s {
	u8	transcoder_id;
	u8	reserved[3];
} iav_udec_start_transcoder_t;

typedef struct iav_udec_stop_transcoder_s {
	u8	transcoder_id;
	u8	stop_flag;
	u8	reserved[2];
} iav_udec_stop_transcoder_t;

typedef struct __read_bits_info_s {
	u32	frame_num;
	u32	start_addr;
	u64	PTS;
	u32	pic_type	: 3;
	u32	level_idc	: 3;
	u32	ref_idc		: 1;
	u32	pic_struct	: 1;
	u32	pic_size	: 24;
	u32	channel_id	: 8;
	u32	stream_id	: 8;
	u32	cavlc_pjpeg	: 1;	// 0: non-CAVLC   1: CAVLC pjpeg
	u32	res_1		: 15;
} __read_bits_info_t;

#define IAV_UDEC_TRANSCODER_NUM_USER_DESC 8
typedef struct iav_udec_transcoder_bs_info_s {
	u8		id;		// in
	u8		count;		// out. valid entries of desc
	u8		more;		// more encoded frames in driver
	u8		reserved0;
	u32		reserved;
	__read_bits_info_t	desc[IAV_UDEC_TRANSCODER_NUM_USER_DESC];
} iav_udec_transcoder_bs_info_t;

//update on the fly
typedef struct iav_udec_transcoder_update_bitrate_s {
	u8		id;		// in
	u8		reserved0;	//in
	u8		reserved1;
	u8		reserved2;
	u32		average_bitrate;	//in
	u64		pts_to_change_bitrate;//in
} iav_udec_transcoder_update_bitrate_t;

typedef struct iav_udec_transcoder_update_framerate_s {
	u8		id;		// in
	u8		reserved0;	//in
	u8		framerate_reduction_factor;		//vin frame rate/factor = encodng frame rate
	u8		framerate_code;	//in 0: 29.97 interlaced, 1: 29.97 progressive, 2: 59.94 interlaced, 3: 59.94 progressive, 4-255 integer frame rate
} iav_udec_transcoder_update_framerate_t;

typedef struct iav_udec_transcoder_update_gop_s {
	u8		id;		// in
	u8		reserved0;	//in

	u8		change_gop_option;//0: not change, 1: change gop, 2: change gop and force first I/P to be IDR
	u8		follow_gop;

	u8		fgop_max_N;
	u8		fgop_min_N;
	u8		M;
	u8		N;

	u8		gop_structure;
	u8		idr_interval;
	u16		reserved1;

	u64		pts_to_change;
} iav_udec_transcoder_update_gop_t;

typedef struct iav_udec_transcoder_demand_idr_s {
	u8		id;		// in
	u8		reserved0;	//in

	u8		on_demand_idr;//0: not change, 1: change next I/P, 2: change next I/P when pts>pts_to_change
	u8		reserved1;

	u64		pts_to_change;
} iav_udec_transcoder_demand_idr_t;

typedef struct iav_debug_s {
	int	cmd;
	int	value;
} iav_debug_t;

#endif

