
/*
 * iav_transcode_drv.h
 *
 * History:
 *	2011/08/16 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_TRANSCODE_DRV_H__
#define __IAV_TRANSCODE_DRV_H__

/*
 * encoding APIs	- IAV_IOC_TRANSCODE_MAGIC
 */
enum {
	IOC_ENTER_TRANSCODE_MODE = 0x00,
	IOC_INIT_ENCODE2 = 0x01,
	IOC_ENCODE_SETUP2 = 0x02,
	IOC_CREATE_FRM_BUF_POOL = 0x03,
	IOC_REQUEST_FRAME_FOR_ENC = 0x04,
	IOC_SEND_INPUT_DATA = 0x05,
	IOC_GET_STREAM_REPORT = 0x06,
	IOC_START_ENCODE2 = 0x07,
	IOC_STOP_ENCODE2 = 0x08,
	IOC_GET_ENCODED_FRAME2 = 0x09,
	IOC_RELEASE_ENCODED_FRAME2 = 0x0a,
	IOC_ENTER_UDEC_MODE2 = 0x0b,
	IOC_INIT_UDEC2 = 0x0c,
	IOC_WAIT_DECODER2 = 0x0d,
	IOC_UDEC_DECODE2 = 0x0e,
	IOC_GET_DECODED_FRAME2 = 0x0f,
	IOC_RENDER_FRAME2 = 0x10,
	IOC_RELEASE_FRAME2 = 0x11,
	IOC_UDEC_STOP2 = 0x12,
	IOC_RELEASE_UDEC2 = 0x13,
	IOC_GET_FRM_BUF_POOL_INFO = 0x14,
};

#define IAV_IOC_ENTER_TRANSCODE_MODE	_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_ENTER_TRANSCODE_MODE, struct iav_transc_mode_config_s *)
#define IAV_IOC_INIT_ENCODE2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_INIT_ENCODE2, struct iav_enc_info2_s *)
#define IAV_IOC_ENCODE_SETUP2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_ENCODE_SETUP2, struct iav_enc_config_s *)
#define IAV_IOC_CREATE_FRM_BUF_POOL	_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_CREATE_FRM_BUF_POOL, struct iav_frm_buf_pool_info_s *)
#define IAV_IOC_REQUEST_FRAME_FOR_ENC	_IOR(IAV_IOC_TRANSCODE_MAGIC, IOC_REQUEST_FRAME_FOR_ENC, struct iav_transc_frame_s *)
#define IAV_IOC_SEND_INPUT_DATA			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_SEND_INPUT_DATA, struct iav_transc_frame_s *)
#define IAV_IOC_GET_STREAM_REPORT		_IOR(IAV_IOC_TRANSCODE_MAGIC, IOC_GET_STREAM_REPORT, struct iav_stream_report_s *)
#define IAV_IOC_START_ENCODE2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_START_ENCODE2, int)
#define IAV_IOC_STOP_ENCODE2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_STOP_ENCODE2, int)
#define IAV_IOC_GET_ENCODED_FRAME2		_IOR(IAV_IOC_TRANSCODE_MAGIC, IOC_GET_ENCODED_FRAME2, struct iav_frame_desc_s *)
#define IAV_IOC_RELEASE_ENCODED_FRAME2	_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_RELEASE_ENCODED_FRAME2, struct iav_frame_desc_s *)
#define IAV_IOC_INIT_UDEC2		            	_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_INIT_UDEC2, struct iav_transc_info_s *)
#define IAV_IOC_WAIT_DECODER2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_WAIT_DECODER2, struct iav_wait_decoder_s *)
#define IAV_IOC_UDEC_DECODE2            		_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_UDEC_DECODE2, struct iav_udec_decode_s *)
#define IAV_IOC_GET_DECODED_FRAME2		_IOR(IAV_IOC_TRANSCODE_MAGIC, IOC_GET_DECODED_FRAME2, struct iav_transc_frame_s *)
#define IAV_IOC_RENDER_FRAME2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_RENDER_FRAME2, struct iav_transc_frame_s *)
#define IAV_IOC_RELEASE_FRAME2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_RELEASE_FRAME2, struct iav_transc_frame_s *)
#define IAV_IOC_UDEC_STOP2				_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_UDEC_STOP, u32)
#define IAV_IOC_RELEASE_UDEC2			_IOW(IAV_IOC_TRANSCODE_MAGIC, IOC_RELEASE_UDEC, u32)
#define IAV_IOC_GET_FRM_BUF_POOL_INFO	_IOR(IAV_IOC_TRANSCODE_MAGIC, IOC_GET_FRM_BUF_POOL_INFO, struct iav_frm_buf_pool_info_s *)

/*
 *  Extended Type definitions for iOne
 */

enum{
	TRANSCODE_INPUT_YUV422P = 6,
	TRANSCODE_INPUT_RGB_RAW = 7,
};

typedef enum {
    IAV_INPUT_INVALID                               = -1,
    IAV_INPUT_CFA_RAW                             = 0,
    IAV_INPUT_YUV_422_INTLC                  = 1,
    IAV_INPUT_YUV_422_PROG                   = 2,
    IAV_INPUT_DRAM_420_INTLC               = 3,
    IAV_INPUT_DRAM_420_PROG                = 4,
    IAV_INPUT_DRAM_422_INTLC_IDSP     = 5,
    IAV_INPUT_DRAM_422_PROG_IDSP      = 6,
    IAV_INPUT_DRAM_420_INTLC_IDSP     = 7,
    IAV_INPUT_DRAM_420_PROG_IDSP      = 8,
    IAV_INPUT_RGB_RAW            = 9,
} IAV_VCAP_INPUT_FORMAT;


//-------------------------------------------------------------------
//                      structure definitions
//-------------------------------------------------------------------
// IAV_IOC_ENTER_TRANSCODE_MODE
typedef struct iav_transc_mode_config_s {
	iav_udec_mode_config_t udec_mode;
	u8					virtual_mem_enable;
} iav_transc_mode_config_t;

// IAV_IOC_INIT_UDEC2
typedef struct iav_udec_info2_s {
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

	u8	*bits_fifo_start;		// out
	u8	*mv_fifo_start;		// out

	// decpp config
	u16 input_center_x;
        u16 input_center_y;

	// for transcode mode only
        u16  out_win_offset_x;
        u16  out_win_offset_y;
        u16  out_win_width;
        u16  out_win_height;
} iav_udec_info2_t;

typedef struct iav_enc_info2_s {
	IAV_VCAP_INPUT_FORMAT 	input_format;		// 6: YUV_422P; 9: RGB_RAW
	u32						input_frame_rate;
	iav_img_size_t				input_img;
	iav_img_size_t				encode_img;
	u32						preview_enable;
} iav_enc_info2_t;

#define IAV_FRAME2_NEED_ADDR		(1 << 0)
#define IAV_FRAME2_NEED_SYNC		(1 << 1)
#define IAV_FRAME2_LAST			(1 << 2)
#define IAV_FRAME2_FORCE_RELEASE		(1 << 3)	// deprecated
#define IAV_FRAME2_SYNC_VOUT		(1 << 4)	// render every frame
#define IAV_FRAME2_NO_WAIT		(1 << 5)	// return error immediately if no fb
#define IAV_FRAME2_NO_RELEASE		(1 << 6)	// do not release the frame buffer after rendering
#define IAV_FRAME2_PADDR			(1 << 7)	// return physical address for lu_buf_addr/ch_buf_addr
#define IAV_FRAME2_FALSE_RLEASE		(1 << 8)	// (obselete) release the frame buffer only at iav layer

typedef struct iav_transc_frame_s {
	u16		flags;		// for ioctl
	u16		real_fb_id;	// out - fb_id used by ucode
	u16		pic_struct;     // in - IAV_IOC_REQUEST_FRAME
        u16          stop_cntl;      // in - stop encoding after this last frame

	u16		buffer_width;
	u16		buffer_height;
	u16		buffer_pitch;

	u16		pic_width;
	u16		pic_height;

	u16		lu_off_x;
	u16		lu_off_y;
	u16		ch_off_x;
	u16		ch_off_y;

	void		*lu_buf_addr;
	void		*ch_buf_addr;

	void		*lu_buf_virt_addr;
	void		*ch_buf_virt_addr;

        /* special for udec */
	u16		fb_id;		// frame buffer id, or IAV_INVALID_FB_ID

	u8		fb_format;
	u8		coded_frame;
	u8		chroma_format;
	u8		if_video;

	u32		pts;
	u32		pts_high;

	u8		decoder_id;	// in
	u8		eos_flag;	        // out
	u8		interlaced;	// in - IAV_IOC_POSTP_FRAME

	u32		top_or_frm_word;
	u32		bot_word;
} iav_transc_frame_t;

typedef struct iav_stream_report_s {
        void        *main_pict_luma_addr;
        void        *main_pict_chroma_addr;
        u16         main_pict_dram_pitch;
} iav_stream_report_t;

typedef struct iav_frm_buf_pool_info_s {
	u16		frm_buf_pool_type;		// in, 0, 1 - decpp pool, 1 <<8- enc input pool
	u16		num_frm_bufs;			// out
	u16		buffer_width;				// out
	u16		buffer_height;				// out
	u8		chroma_format;			// out, 0 - ARGB, 1 - YUV420, 2 - YUV422
	u16		frm_buf_id[32];			// out
	void		*luma_img_base_addr[32];	// out
} iav_frm_buf_pool_info_t;


#endif

