/*
 * iav_encode_drv.h
 *
 * History:
 *	2011/08/02 - [Jay Zhang] created file
 *	2011/08/15 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_ENCODE_DRV_H__
#define __IAV_ENCODE_DRV_H__


/**********************************************************
 *
 *	Encoding APIs - IAV_IOC_ENCODE_MAGIC
 *
 *********************************************************/
enum {
	// Legacy, from 0x00 to 0x1F
	IOC_SET_JPEG_QUALITY = 0x00,
	IOC_GET_JPEG_QUALITY = 0x01,
	IOC_SET_H264_CONFIG = 0x02,
	IOC_SET_H264_CONFIG_ALL = 0x03,
	IOC_GET_H264_CONFIG = 0x04,
	IOC_START_ENCODE = 0x05,
	IOC_STOP_ENCODE = 0x06,
	IOC_GET_PREVIEW_FORMAT = 0x07,
	IOC_SET_ENCODE_FORMAT = 0x08,
	IOC_SET_ENCODE_FORMAT_ALL = 0x09,
	IOC_GET_ENCODE_FORMAT = 0x0A,
	IOC_ENABLE_PREVIEW = 0x0B,
	IOC_DISABLE_PREVIEW = 0x0C,
	IOC_DISABLE_PREVIEW2 = 0x0D,
	IOC_READ_BITSTREAM = 0x0E,
	IOC_GET_BSB_FULLNESS = 0x0F,

	IOC_VBR_MODIFY_BITRATE = 0x10,
	IOC_CBR_MODIFY_BITRATE = 0x11,
	IOC_FORCE_IDR = 0x12,
	IOC_CHANGE_FRAME_RATE = 0x13,
	IOC_CAVLC_GET_PJPEG_BUFFER_INFO = 0x14,
	IOC_CAVLC_REPORT_ENCODED_BITS = 0x15,
	IOC_CAVLC_UPDATE_PJPEG_BUFFER = 0x16,
	IOC_GET_ENCODE_STATE = 0x17,

	// For A5s IPCAM, from 0x20 to 0x5F
	IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX = 0x20,
	IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX = 0x21,
	IOC_SET_SOURCE_BUFFER_FORMAT_EX = 0x22,
	IOC_GET_SOURCE_BUFFER_FORMAT_EX = 0x23,
	IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX = 0x24,
	IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX = 0x25,
	IOC_SET_PREVIEW_BUFFER_FORMAT_ALL_EX = 0x26,
	IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX = 0x27,
	IOC_SET_ENCODE_FORMAT_EX = 0x28,
	IOC_GET_ENCODE_FORMAT_EX = 0x29,
	IOC_GET_SOURCE_BUFFER_INFO_EX = 0x2A,
	IOC_GET_ENCODE_STREAM_INFO_EX = 0x2B,
	IOC_SET_DIGITAL_ZOOM_EX = 0x2C,
	IOC_GET_DIGITAL_ZOOM_EX = 0x2D,
	IOC_SET_2ND_DIGITAL_ZOOM_EX = 0x2E,
	IOC_GET_2ND_DIGITAL_ZOOM_EX = 0x2F,

	IOC_READ_YUV_BUFFER_INFO_EX = 0x30,
	IOC_READ_ME1_BUFFER_INFO_EX = 0x31,
	IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX = 0x32,
	IOC_CHANGE_FRAMERATE_FACTOR_EX = 0x33,
	IOC_SYNC_FRAMERATE_FACTOR_EX = 0x34,
	IOC_GET_FRAMERATE_FACTOR_EX = 0x35,
	IOC_CHANGE_INTRA_MB_ROWS_EX = 0x36,
	IOC_READ_BITSTREAM_EX = 0x37,
	IOC_SET_BITRATE_EX = 0x38,
	IOC_GET_BITRATE_EX = 0x39,
	IOC_CHANGE_GOP_EX = 0x3A,
	IOC_FORCE_IDR_EX = 0x3B,
	IOC_SET_MJPEG_CONFIG_EX = 0x3C,
	IOC_GET_MJPEG_CONFIG_EX = 0x3D,
	IOC_SET_H264_CONFIG_EX = 0x3E,
	IOC_GET_H264_CONFIG_EX = 0x3F,

	IOC_START_ENCODE_EX = 0x40,
	IOC_STOP_ENCODE_EX = 0x41,
	IOC_SET_LOWDELAY_REFRESH_PERIOD_EX = 0x42,
	IOC_CHANGE_QP_LIMIT_EX = 0x43,
	IOC_GET_QP_LIMIT_EX = 0x44,
	IOC_SET_PREV_A_FRAMERATE_DIV_EX = 0x45,
	IOC_GET_QP_HIST_INFO_EX = 0x46,
	IOC_SET_QP_ROI_MATRIX_EX = 0x47,
	IOC_GET_QP_ROI_MATRIX_EX = 0x48,
	IOC_SET_AUDIO_CLK_FREQ_EX = 0x49,
	IOC_SET_PANIC_CONTROL_PARAM_EX = 0x4A,
	IOC_UPDATE_VIN_FRAMERATE_EX = 0x4B,
	IOC_SET_CHROMA_FORMAT_EX = 0x4C,
	IOC_GET_CHROMA_FORMAT_EX = 0x4D,
	IOC_SET_H264_ENC_PARAM_EX = 0x4E,
	IOC_GET_H264_ENC_PARAM_EX = 0x4F,

	// For iOne, from 0x60 to 0x7F
	IOC_ENTER_ENCODE_MODE = 0x60,
	IOC_INIT_ENCODE = 0x61,
	IOC_ENCODE_SETUP = 0x62,
	IOC_GET_ENCODED_FRAME = 0x63,
	IOC_RELEASE_ENCODED_FRAME = 0x64,
	IOC_GET_PREVIEW_BUFFER = 0x65,
	IOC_ENCODE_STATE = 0x66,
	IOC_ADD_OSD_BLEND_AREA = 0x67,
	IOC_REMOVE_OSD_BLEND_AREA = 0x68,
	IOC_UPDATE_OSD_BLEND_AREA = 0x69,
	IOC_SET_ZOOM = 0x6A,
	IOC_ABORT_ENCODE = 0x6B,
	IOC_SET_PREV_SRC_WIN = 0x6C,
	IOC_STILL_JPEG_CAPTURE = 0x6D,
	IOC_UPDATE_PREVIEW_BUFFER_CONFIG = 0x6E,

	// For S2, from 0x80 to 0x9F
	IOC_GET_SHARPEN_FILTER_CONFIG_EX = 0x80,
	IOC_SET_SHARPEN_FILTER_CONFIG_EX = 0x81,
	IOC_GET_WARP_CONTROL_EX = 0x82,
	IOC_SET_WARP_CONTROL_EX = 0x83,
	IOC_GET_STATIS_CONFIG_EX = 0x84,
	IOC_SET_STATIS_CONFIG_EX = 0x85,
	IOC_FETCH_STATIS_INFO_EX = 0x86,
	IOC_RELEASE_STATIS_INFO_EX = 0x87,
	IOC_FETCH_FRAME_EX = 0x88,
	IOC_RELEASE_FRAME_EX = 0x89,
	IOC_ENC_DRAM_ALLOC_FRAME_EX = 0x8A,
	IOC_ENC_DRAM_UPDATE_FRAME_EX = 0x8B,
	IOC_GET_WARP_AREA_DPTZ_EX = 0x8C,
	IOC_SET_WARP_AREA_DPTZ_EX = 0x8D,
	IOC_QUERY_ENCMODE_CAP = 0x8E,
	IOC_QUERY_ENCBUF_CAP = 0x8F,

	IOC_GET_SOURCE_BUFFER_SETUP_EX = 0x90,
	IOC_SET_SOURCE_BUFFER_SETUP_EX = 0x91,
	IOC_VIDEO_PROC = 0x92,
	IOC_STREAM_CFG_EX = 0x93,
	IOC_BUFCAP_EX = 0x94,
	IOC_WARP_PROC = 0x95,
	IOC_VCAP_PROC = 0x96,

	// For sync frame encoding
	IOC_FRAME_SYNC_PROC = 0xA0,

	// Reserved, from 0xB0 to 0xCF

	// Others, from 0xD0 to 0xFF
	// For VIN FPS STAT
	IOC_START_VIN_FPS_STAT = 0xD0,
	IOC_GET_VIN_FPS_STAT = 0xD1,
	IOC_STILL_CHG_VIN = 0xD2,
};

#define _ENC_IO(IOCTL)		_IO(IAV_IOC_ENCODE_MAGIC, IOCTL)
#define _ENC_IOW(IOCTL, param)		_IOW(IAV_IOC_ENCODE_MAGIC, IOCTL, param)
#define _ENC_IOR(IOCTL, param)		_IOR(IAV_IOC_ENCODE_MAGIC, IOCTL, param)
#define _ENC_IOWR(IOCTL, param)		_IOWR(IAV_IOC_ENCODE_MAGIC, IOCTL, param)

#define IAV_IOC_SET_JPEG_QUALITY	_ENC_IOW(IOC_SET_JPEG_QUALITY, struct iav_jpeg_quality_s *)
#define IAV_IOC_GET_JPEG_QUALITY	_ENC_IOW(IOC_GET_JPEG_QUALITY, struct iav_jpeg_quality_s *)

#define IAV_IOC_SET_H264_CONFIG		_ENC_IOW(IOC_SET_H264_CONFIG, struct iav_h264_config_s *)
#define IAV_IOC_SET_H264_CONFIG_ALL	_ENC_IOW(IOC_SET_H264_CONFIG_ALL, struct iav_h264_config_s *)
#define IAV_IOC_GET_H264_CONFIG		_ENC_IOR(IOC_GET_H264_CONFIG, struct iav_h264_config_s *)

#define IAV_IOC_START_ENCODE		_ENC_IOW(IOC_START_ENCODE, int)
#define IAV_IOC_STOP_ENCODE			_ENC_IOW(IOC_STOP_ENCODE, int)
#define IAV_IOC_ABORT_ENCODE		_ENC_IOW(IOC_ABORT_ENCODE, int)

#define IAV_IOC_GET_PREVIEW_FORMAT	_ENC_IOR(IOC_GET_PREVIEW_FORMAT, struct iav_preview_format_s *)

#define IAV_IOC_SET_ENCODE_FORMAT	_ENC_IOW(IOC_SET_ENCODE_FORMAT, struct iav_encode_format_s *)
#define IAV_IOC_GET_ENCODE_FORMAT	_ENC_IOR(IOC_GET_ENCODE_FORMAT, struct iav_encode_format_s *)

#define IAV_IOC_ENABLE_PREVIEW		_ENC_IO(IOC_ENABLE_PREVIEW)
#define IAV_IOC_DISABLE_PREVIEW		_ENC_IO(IOC_DISABLE_PREVIEW)
#define IAV_IOC_DISABLE_PREVIEW2		_ENC_IO(IOC_DISABLE_PREVIEW2)

#define IAV_IOC_READ_BITSTREAM		_ENC_IOR(IOC_READ_BITSTREAM, struct bs_fifo_info_s *)
#define IAV_IOC_GET_BSB_FULLNESS	_ENC_IOR(IOC_GET_BSB_FULLNESS, struct iav_bsb_fullness_s *)
#define IAV_IOC_CAVLC_GET_PJPEG_BUFFER_INFO	_ENC_IOR(IOC_CAVLC_GET_PJPEG_BUFFER_INFO, struct cavlc_pjpeg_buffer_info_s *)
#define IAV_IOC_CAVLC_UPDATE_PJPEG_BUFFER	_ENC_IOW(IOC_CAVLC_UPDATE_PJPEG_BUFFER, u32)
#define IAV_IOC_CAVLC_REPORT_ENCODED_BITS	_ENC_IOW(IOC_CAVLC_REPORT_ENCODED_BITS, struct cavlc_encode_info_s *)

/* Below APIs are used to dynamically change encoding behavior */
#define IAV_IOC_VBR_MODIFY_BITRATE	_ENC_IOW(IOC_VBR_MODIFY_BITRATE, struct iav_vbr_modify_bitrate_s *)
#define IAV_IOC_CBR_MODIFY_BITRATE	_ENC_IOW(IOC_CBR_MODIFY_BITRATE, struct iav_cbr_modify_bitrate_s *)
#define IAV_IOC_FORCE_IDR			_ENC_IOW(IOC_FORCE_IDR, int)
#define IAV_IOC_CHANGE_FRAME_RATE	_ENC_IOW(IOC_CHANGE_FRAME_RATE, int)

#define IAV_IOC_GET_ENCODE_STATE		_ENC_IOR(IOC_GET_ENCODE_STATE, struct iav_encode_state_s *)


// for A5s IPCam
#define IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX _ENC_IOW(IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX, struct iav_source_buffer_type_all_ex_s *)
#define IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX _ENC_IOR(IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX, struct iav_source_buffer_type_all_ex_s *)

#define IAV_IOC_SET_SOURCE_BUFFER_FORMAT_EX	_ENC_IOW(IOC_SET_SOURCE_BUFFER_FORMAT_EX, struct iav_source_buffer_format_ex_s *)
#define IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX	_ENC_IOWR(IOC_GET_SOURCE_BUFFER_FORMAT_EX, struct iav_source_buffer_format_ex_s *)	//fill stream and get data

#define IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX _ENC_IOW(IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX, struct iav_source_buffer_format_all_ex_s *)
#define IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX _ENC_IOR(IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, struct iav_source_buffer_format_all_ex_s *)

#define IAV_IOC_SET_PREVIEW_BUFFER_FORMAT_ALL_EX _ENC_IOW(IOC_SET_PREVIEW_BUFFER_FORMAT_ALL_EX, struct iav_preview_buffer_format_all_ex_s *)
#define IAV_IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX _ENC_IOR(IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX, struct iav_preview_buffer_format_all_ex_s *)

#define IAV_IOC_SET_ENCODE_FORMAT_EX		_ENC_IOW(IOC_SET_ENCODE_FORMAT_EX, struct iav_encode_format_ex_s *)
#define IAV_IOC_GET_ENCODE_FORMAT_EX		_ENC_IOWR(IOC_GET_ENCODE_FORMAT_EX, struct iav_encode_format_ex_s *)	//fill stream and get data

#define IAV_IOC_GET_SOURCE_BUFFER_INFO_EX	_ENC_IOWR(IOC_GET_SOURCE_BUFFER_INFO_EX, struct iav_source_buffer_info_ex_s *)	//get source buffer info to see whether the buffer can be reconfigured
#define IAV_IOC_GET_ENCODE_STREAM_INFO_EX	_ENC_IOWR(IOC_GET_ENCODE_STREAM_INFO_EX, struct iav_encode_stream_info_ex_s *)	//get encode stream info to see whether we should still display it

#define IAV_IOC_SET_DIGITAL_ZOOM_EX			_ENC_IOW(IOC_SET_DIGITAL_ZOOM_EX, struct iav_digital_zoom_ex_s *)
#define IAV_IOC_GET_DIGITAL_ZOOM_EX			_ENC_IOW(IOC_GET_DIGITAL_ZOOM_EX, struct iav_digital_zoom_ex_s *)

#define IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX		_ENC_IOW(IOC_SET_2ND_DIGITAL_ZOOM_EX, struct iav_digital_zoom_ex_s *)
#define IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX		_ENC_IOWR(IOC_GET_2ND_DIGITAL_ZOOM_EX, struct iav_digital_zoom_ex_s *)

#define IAV_IOC_READ_YUV_BUFFER_INFO_EX		_ENC_IOWR(IOC_READ_YUV_BUFFER_INFO_EX, struct iav_yuv_buffer_info_ex_s *)
#define IAV_IOC_READ_ME1_BUFFER_INFO_EX		_ENC_IOW(IOC_READ_ME1_BUFFER_INFO_EX, struct iav_me1_buffer_info_ex_s *)

#define IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX	_ENC_IOW(IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX, struct iav_digital_zoom_privacy_mask_ex_s *)

/* read bitstream with additional info */
#define IAV_IOC_READ_BITSTREAM_EX			_ENC_IOR(IOC_READ_BITSTREAM_EX, struct bits_info_ex_s *)
/* change bitrate on the fly */
#define IAV_IOC_SET_BITRATE_EX			_ENC_IOW(IOC_SET_BITRATE_EX, struct iav_bitrate_info_ex_s *)
#define IAV_IOC_GET_BITRATE_EX			_ENC_IOWR(IOC_GET_BITRATE_EX, struct iav_bitrate_info_ex_s *)
/* change gop on the fly */
#define IAV_IOC_CHANGE_GOP_EX			_ENC_IOW(IOC_CHANGE_GOP_EX, struct iav_change_gop_ex_s *)
/* set chroma format */
#define IAV_IOC_SET_CHROMA_FORMAT_EX	_ENC_IOW(IOC_SET_CHROMA_FORMAT_EX, struct iav_chroma_format_info_ex_s *)
/* change frame rate factor */
#define IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX	_ENC_IOW(IOC_CHANGE_FRAMERATE_FACTOR_EX, struct iav_change_framerate_factor_ex_s *)
#define IAV_IOC_SYNC_FRAMERATE_FACTOR_EX _ENC_IOW(IOC_SYNC_FRAMERATE_FACTOR_EX, struct iav_sync_framerate_factor_ex_s *)
#define IAV_IOC_GET_FRAMERATE_FACTOR_EX		_ENC_IOWR(IOC_GET_FRAMERATE_FACTOR_EX, struct iav_change_framerate_factor_ex_s *)
/* get/set h264 encode param */
#define IAV_IOC_SET_H264_ENC_PARAM_EX	_ENC_IOW(IOC_SET_H264_ENC_PARAM_EX, struct iav_h264_enc_param_ex_s *)
#define IAV_IOC_GET_H264_ENC_PARAM_EX	_ENC_IOWR(IOC_GET_H264_ENC_PARAM_EX, struct iav_h264_enc_param_ex_s *)

/* change intra refresh MB rows */
#define IAV_IOC_CHANGE_INTRA_MB_ROWS_EX		_ENC_IOW(IOC_CHANGE_INTRA_MB_ROWS_EX, struct iav_change_intra_mb_rows_ex_s *)
/* change H264 qp limit on the fly */
#define IAV_IOC_CHANGE_QP_LIMIT_EX			_ENC_IOW(IOC_CHANGE_QP_LIMIT_EX, struct iav_change_qp_limit_ex_s *)
/* get H264 qp limit on the fly */
#define IAV_IOC_GET_QP_LIMIT_EX				_ENC_IOWR(IOC_GET_QP_LIMIT_EX, struct iav_change_qp_limit_ex_s *)
/* set preview A framerate divisor  */
#define IAV_IOC_SET_PREV_A_FRAMERATE_DIV_EX _ENC_IOW(IOC_SET_PREV_A_FRAMERATE_DIV_EX, u8)

/* force IDR */
#define IAV_IOC_FORCE_IDR_EX				_ENC_IOW(IOC_FORCE_IDR_EX, iav_stream_id_t)
/* MJPEG config */
#define IAV_IOC_SET_JPEG_CONFIG_EX		_ENC_IOW(IOC_SET_MJPEG_CONFIG_EX, struct iav_jpeg_config_ex_s *)
#define IAV_IOC_GET_JPEG_CONFIG_EX		_ENC_IOWR(IOC_GET_MJPEG_CONFIG_EX, struct iav_jpeg_config_ex_s *) 	//fill stream and get data
/*  H264 config;  M, N, IDR can be configured on the fly  */
#define IAV_IOC_SET_H264_CONFIG_EX		_ENC_IOW(IOC_SET_H264_CONFIG_EX, struct iav_h264_config_ex_s *)
#define IAV_IOC_GET_H264_CONFIG_EX		_ENC_IOWR(IOC_GET_H264_CONFIG_EX, struct iav_h264_config_ex_s *)	//fill stream and get data
/* stream encode control,  by using  IAV_MAIN_STREAM|IAV_2ND_STREAM, user can start main and secondary stream simultanuously ,
* or user can choose to start main first, and then start secondary */
#define IAV_IOC_START_ENCODE_EX			_ENC_IOW(IOC_START_ENCODE_EX, iav_stream_id_t)
#define IAV_IOC_STOP_ENCODE_EX			_ENC_IOW(IOC_STOP_ENCODE_EX, iav_stream_id_t)
/* low delay API, for future use */
#define IAV_IOC_SET_LOWDELAY_REFRESH_PERIOD_EX		_ENC_IOW(IOC_SET_LOWDELAY_REFRESH_PERIOD_EX, struct iav_lowdelay_refresh_period_ex_s *)

/* get QP histogram for streams */
#define IAV_IOC_GET_QP_HIST_INFO_EX		_ENC_IOR(IOC_GET_QP_HIST_INFO_EX, struct iav_qp_hist_info_ex_s *)

/* set QP matrix based on macro block */
#define IAV_IOC_SET_QP_ROI_MATRIX_EX		_ENC_IOW(IOC_SET_QP_ROI_MATRIX_EX, struct iav_qp_roi_matrix_ex_s *)
/* get QP matrix based on macro block */
#define IAV_IOC_GET_QP_ROI_MATRIX_EX		_ENC_IOWR(IOC_GET_QP_ROI_MATRIX_EX, struct iav_qp_roi_matrix_ex_s *)

/* set audio clock frequency run time */
#define IAV_IOC_SET_AUDIO_CLK_FREQ_EX		_ENC_IOW(IOC_SET_AUDIO_CLK_FREQ_EX, u32)

#define IAV_IOC_SET_PANIC_CONTROL_PARAM_EX		_ENC_IOW(IOC_SET_PANIC_CONTROL_PARAM_EX, struct iav_panic_control_param_ex_s *)

#define IAV_IOC_UPDATE_VIN_FRAMERATE_EX		_ENC_IO(IOC_UPDATE_VIN_FRAMERATE_EX)

// for iOne
#define IAV_IOC_ENTER_ENCODE_MODE		_ENC_IOW(IOC_ENTER_ENCODE_MODE, struct iav_enc_mode_config_s *)
#define IAV_IOC_INIT_ENCODE				_ENC_IOW(IOC_INIT_ENCODE, struct iav_enc_info_s *)
#define IAV_IOC_INIT_ENCODE_INFO		_ENC_IOR(IOC_INIT_ENCODE, struct iav_enc_info_s *)
#define IAV_IOC_ENCODE_SETUP			_ENC_IOW(IOC_ENCODE_SETUP, struct iav_enc_config_s *)
#define IAV_IOC_ENCODE_SETUP_INFO		_ENC_IOR(IOC_ENCODE_SETUP, struct iav_enc_config_s *)

#define IAV_IOC_GET_ENCODED_FRAME		_ENC_IOR(IOC_GET_ENCODED_FRAME, struct iav_frame_desc_s *)
#define IAV_IOC_RELEASE_ENCODED_FRAME	_ENC_IOW(IOC_RELEASE_ENCODED_FRAME, struct iav_frame_desc_s *)
#define IAV_IOC_GET_PREVIEW_BUFFER		_ENC_IOR(IOC_GET_PREVIEW_BUFFER, struct iav_prev_buf_s *)
#define IAV_IOC_ENCODE_STATE			_ENC_IOR(IOC_ENCODE_STATE, struct iav_enc_state_s *)
#define IAV_IOC_ADD_OSD_BLEND_AREA		_ENC_IOW(IOC_ADD_OSD_BLEND_AREA, struct iav_osd_blend_info_s *)
#define IAV_IOC_REMOVE_OSD_BLEND_AREA	_ENC_IOW(IOC_REMOVE_OSD_BLEND_AREA, u8)
#define IAV_IOC_UPDATE_OSD_BLEND_AREA	_ENC_IOW(IOC_UPDATE_OSD_BLEND_AREA, u8)
#define IAV_IOC_SET_ZOOM				_ENC_IOW(IOC_SET_ZOOM, struct iav_zoom_info_s *)
#define IAV_IOC_SET_PREV_SRC_WIN		_ENC_IOW(IOC_SET_PREV_SRC_WIN, struct iav_prev_src_win_s *)
#define IAV_IOC_STILL_JPEG_CAPTURE		_ENC_IOW(IOC_STILL_JPEG_CAPTURE, struct iav_piv_jpeg_capture_s *)
#define IAV_IOC_UPDATE_PREVIEW_BUFFER_CONFIG	_ENC_IOW(IOC_UPDATE_PREVIEW_BUFFER_CONFIG, struct iav_update_preview_buffer_config_s *)

// For S2 IPCAM
#define IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX	_ENC_IOR(IOC_GET_SHARPEN_FILTER_CONFIG_EX, struct iav_sharpen_filter_cfg_s *)
#define IAV_IOC_SET_SHARPEN_FILTER_CONFIG_EX	_ENC_IOW(IOC_SET_SHARPEN_FILTER_CONFIG_EX, struct iav_sharpen_filter_cfg_s *)

#define IAV_IOC_GET_WARP_CONTROL_EX		_ENC_IOR(IOC_GET_WARP_CONTROL_EX, struct iav_warp_control_ex_s *)
#define IAV_IOC_SET_WARP_CONTROL_EX		_ENC_IOW(IOC_SET_WARP_CONTROL_EX, struct iav_warp_control_ex_s *)

#define IAV_IOC_GET_STATIS_CONFIG_EX		_ENC_IOWR(IOC_GET_STATIS_CONFIG_EX, struct iav_enc_statis_config_ex_s *)
#define IAV_IOC_SET_STATIS_CONFIG_EX		_ENC_IOW(IOC_SET_STATIS_CONFIG_EX, struct iav_enc_statis_config_ex_s *)

#define IAV_IOC_FETCH_STATIS_INFO_EX		_ENC_IOWR(IOC_FETCH_STATIS_INFO_EX, struct iav_enc_statis_info_ex_s *)
#define IAV_IOC_RELEASE_STATIS_INFO_EX	_ENC_IOW(IOC_RELEASE_STATIS_INFO_EX, struct iav_enc_statis_info_ex_s *)

#define IAV_IOC_FETCH_FRAME_EX			_ENC_IOWR(IOC_FETCH_FRAME_EX, struct bits_info_ex_s *)
#define IAV_IOC_RELEASE_FRAME_EX			_ENC_IOW(IOC_RELEASE_FRAME_EX, struct bits_info_ex_s *)

#define IAV_IOC_GET_WARP_AREA_DPTZ_EX		_ENC_IOWR(IOC_GET_WARP_AREA_DPTZ_EX, struct iav_warp_dptz_ex_s *)
#define IAV_IOC_SET_WARP_AREA_DPTZ_EX		_ENC_IOW(IOC_SET_WARP_AREA_DPTZ_EX, struct iav_warp_dptz_ex_s *)

#define IAV_IOC_QUERY_ENCMODE_CAP_EX		_ENC_IOWR(IOC_QUERY_ENCMODE_CAP, struct iav_encmode_cap_ex_s *)

#define IAV_IOC_QUERY_ENCBUF_CAP_EX		_ENC_IOWR(IOC_QUERY_ENCBUF_CAP, struct iav_encbuf_cap_ex_s *)

#define IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX		_ENC_IOWR(IOC_GET_SOURCE_BUFFER_SETUP_EX, struct iav_source_buffer_setup_ex_s *)
#define IAV_IOC_SET_SOURCE_BUFFER_SETUP_EX		_ENC_IOW(IOC_SET_SOURCE_BUFFER_SETUP_EX, struct iav_source_buffer_setup_ex_s *)

#define IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX		_ENC_IOWR(IOC_ENC_DRAM_ALLOC_FRAME_EX, struct iav_enc_dram_buf_frame_ex_s *)
#define IAV_IOC_ENC_DRAM_RELEASE_FRAME_EX		_ENC_IOW(IOC_ENC_DRAM_ALLOC_FRAME_EX, struct iav_enc_dram_buf_frame_ex_s *)
#define IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX		_ENC_IOW(IOC_ENC_DRAM_UPDATE_FRAME_EX, struct iav_enc_dram_buf_update_ex_s *)

#define IAV_IOC_GET_VIDEO_PROC		_ENC_IOWR(IOC_VIDEO_PROC, struct iav_video_proc_s)
#define IAV_IOC_CFG_VIDEO_PROC		_ENC_IOW(IOC_VIDEO_PROC, struct iav_video_proc_s)
#define IAV_IOC_APPLY_VIDEO_PROC	_ENC_IOW(IOC_VIDEO_PROC, struct iav_apply_flag_s [IAV_VIDEO_PROC_NUM])

#define IAV_IOC_GET_STREAM_CFG_EX			_ENC_IOWR(IOC_STREAM_CFG_EX, struct iav_stream_cfg_s)
#define IAV_IOC_SET_STREAM_CFG_EX			_ENC_IOW(IOC_STREAM_CFG_EX, struct iav_stream_cfg_s)

#define IAV_IOC_READ_BUF_CAP_EX			_ENC_IOWR(IOC_BUFCAP_EX, struct iav_buf_cap_s)

#define IAV_IOC_GET_WARP_PROC			_ENC_IOWR(IOC_WARP_PROC, struct iav_warp_proc_s)
#define IAV_IOC_CFG_WARP_PROC			_ENC_IOW(IOC_WARP_PROC, struct iav_warp_proc_s)
#define IAV_IOC_APPLY_WARP_PROC		_ENC_IOW(IOC_WARP_PROC, struct iav_apply_flag_s [IAV_WARP_PROC_NUM])

#define IAV_IOC_GET_VCAP_PROC		_ENC_IOWR(IOC_VCAP_PROC, struct iav_vcap_proc_s)
#define IAV_IOC_CFG_VCAP_PROC		_ENC_IOW(IOC_VCAP_PROC, struct iav_vcap_proc_s *)
#define IAV_IOC_APPLY_VCAP_PROC	_ENC_IOW(IOC_VCAP_PROC, struct iav_apply_flag_s [IAV_VCAP_PROC_NUM])

// for VIN FPS statistics
#define IAV_IOC_START_VIN_FPS_STAT		_ENC_IOW(IOC_START_VIN_FPS_STAT, u8)
#define IAV_IOC_GET_VIN_FPS_STAT		_ENC_IOR(IOC_GET_VIN_FPS_STAT, struct amba_fps_report_s *)
#define IAV_IOC_STILL_CHG_VIN			_ENC_IO(IOC_STILL_CHG_VIN)

//For sync frame encoding
#define IAV_IOC_CFG_FRAME_SYNC_PROC		_ENC_IOW(IOC_FRAME_SYNC_PROC, struct iav_stream_cfg_s *)
#define IAV_IOC_GET_FRAME_SYNC_PROC		_ENC_IOWR(IOC_FRAME_SYNC_PROC, struct iav_stream_cfg_s *)
#define IAV_IOC_APPLY_FRAME_SYNC_PROC		_ENC_IOW(IOC_FRAME_SYNC_PROC, u8)

/**********************************************************
 *
 *		Constant, macro & enum definitions
 *
 *********************************************************/

/*encode pic_type */
typedef enum {
	IDR_FRAME = 1,
	I_FRAME = 2,
	P_FRAME = 3,
	B_FRAME = 4,
	JPEG_STREAM = 5,
	JPEG_THUMBNAIL = 6,
	JPEG_THUMBNAIL2 = 7,
	IAV_PIC_TYPE_TOTAL_NUM = 8,
	IAV_PIC_TYPE_FIRST = 1,
	IAV_PIC_TYPE_LAST = IAV_PIC_TYPE_TOTAL_NUM,
} IAV_PIC_TYPE;

/*
 * encode stream id: (LSB is stream 0, and MSB is stream 31)
 * Encode format flags used in IAV_IOC_START_STREAM_ENCODE and etc.
 * It can be or'ed, the IAV_MAIN_STREAM macro and etc.
 */
typedef u32	iav_stream_id_t;
typedef enum {
	IAV_MAIN_STREAM = (1 << 0),
	IAV_2ND_STREAM = (1 << 1),
	IAV_3RD_STREAM = (1 << 2),
	IAV_4TH_STREAM = (1 << 3),
	IAV_5TH_STREAM = (1 << 4),
	IAV_6TH_STREAM = (1 << 5),
	IAV_7TH_STREAM = (1 << 6),
	IAV_8TH_STREAM = (1 << 7),

	/*
	 * IAV Current implementation has set limit to max stream number
	 * up to 8 for S2.
	 */
	IAV_STREAM_MAX_NUM_IMPL = 8,
	IAV_STREAM_MAX_NUM_ALL = 16,
	IAV_ALL_STREAM = ((1 << IAV_STREAM_MAX_NUM_IMPL) - 1),
} IAV_STREAM_ID;

/* source buffer id */
typedef u32 iav_buffer_id_t;
typedef enum {
	IAV_MAIN_BUFFER = (1 << 0),	// encode format flags, can be or'ed, the IAV_MAIN_BUFFER macro and etc can be OR'ed
	IAV_2ND_BUFFER = (1 << 1),	// used in IAV_IOC_SET_SOURCE_BUFFER_FORMAT_EX and etc
	IAV_3RD_BUFFER = (1 << 2),
	IAV_4TH_BUFFER = (1 << 3),
	IAV_BUFFER_ID_TOTAL_NUM = 4,
} IAV_BUFFER_ID;

/* GOP model */
typedef enum {
	IAV_GOP_SIMPLE = 0,	// simpe GOP, MPEG2 alike
	IAV_GOP_ADVANCED = 1,	// hierachical GOP
	IAV_GOP_P2B2REF = 2,	// B has 2 reference, P has 2 reference
	IAV_GOP_P2B3REF = 3,	// B has 3 reference, P has 2 reference
	IAV_GOP_P2B3_ADV = 4,	// B has 3 reference, P has 2 reference, advanced mode
	IAV_GOP_SVCT_FIRST = 2,
	IAV_GOP_SVCT_LAST = 5,
	IAV_GOP_LONGTERM_P1B0REF = 6,  // P refers from I, may drop P in GOP. enable fast reverse play
} IAV_GOP_MODEL;

/* bitrate control */
typedef enum {
	IAV_BRC_CBR = 1,
	IAV_BRC_PCBR = 2,
	IAV_BRC_VBR = 3,
	IAV_BRC_SCBR = 4,		// smart CBR
	IAV_BRC_MODE_FIRST = IAV_BRC_CBR,
	IAV_BRC_MODE_LAST = IAV_BRC_SCBR + 1,
} IAV_BRC_MODE;

#define IAV_ENC_ID_MAIN			0
#define IAV_ENC_ID_SECOND		1
#define IAV_ENC_ID_THIRD		2
#define IAV_ENC_ID_FOURTH		3
#define IAV_ENC_ID_ALL			0xFF

#define IAV_BUFFER_ID_MAIN			0
#define IAV_BUFFER_ID_PREVIEW_A		1
#define IAV_BUFFER_ID_PREVIEW_B		2
#define IAV_BUFFER_ID_PREVIEW_C		3

#define IAV_FLAG_USE_DEFAULT		(1 << 0)
#define IAV_CODED_FRAME_NO_WAIT		(1 << 1)
#define IAV_FLAG_STILL_PICTURE		(1 << 2)

/* Real time encode param limit */
typedef enum {
	INTRABIAS_MIN = 1,
	INTRABIAS_MAX = 4000,
	P_SKIP_MIN = 1,
	P_SKIP_MAX = 4000,
	ZMV_TH_MIN = 0,
	ZMV_TH_MAX = 255,
	MODE_BIAS_MIN = -4,
	MODE_BIAS_MAX = 5,
	USER_BIAS_VALUE_MIN = 0,
	USER_BIAS_VALUE_MAX = 9,
	GOP_REF_P_MIN = 0,
	GOP_REF_P_MAX = 2,
} REALTIME_ENC_PARAM_LIMIT;


/**********************************************************
 *
 *		Extended definitions for IPCAM
 *
 *********************************************************/

/* interlaced VIN encoding, deinterlacing mode */
#define DEINTLC_OFF				0	//encode as field mode
#define DEINTLC_BOB_MODE		1	//encode as BOB frame mode by dropping one field
#define DEINTLC_WEAVE_MODE	2	//encode as WEAVE frame mode by combining two fields

#define INTLC_OFF				0	//no progressive to interlacing
#define INTLC_ON				1	//enable progressive to interlacing

#define IAV_ENTROPY_CABAC	0
#define IAV_ENTROPY_CAVLC	1

#define IAV_H264_CHROMA_FORMAT_YUV420			0
#define IAV_H264_CHROMA_FORMAT_MONO				1

#define IAV_JPEG_CHROMA_FORMAT_YUV422			0
#define IAV_JPEG_CHROMA_FORMAT_YUV420			1
#define IAV_JPEG_CHROMA_FORMAT_MONO				2

enum {
	IAV_ENCODE_SOURCE_MAIN_BUFFER = 0,
	IAV_ENCODE_SOURCE_SECOND_BUFFER = 1,
	IAV_ENCODE_SOURCE_THIRD_BUFFER = 2,
	IAV_ENCODE_SOURCE_FOURTH_BUFFER = 3,
	IAV_ENCODE_SOURCE_MAIN_DRAM = 4,
	IAV_ENCODE_SOURCE_TOTAL_NUM,
	IAV_ENCODE_SOURCE_MAIN_VCA = 0x08,

	/* Sub source buffers */
	IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST = IAV_ENCODE_SOURCE_SECOND_BUFFER,
	IAV_ENCODE_SUB_SOURCE_BUFFER_LAST = IAV_ENCODE_SOURCE_FOURTH_BUFFER + 1,
	IAV_ENCODE_SUB_SOURCE_BUFFER_TOTAL_NUM = IAV_ENCODE_SUB_SOURCE_BUFFER_LAST - IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST,

	/* DRAM source buffers */
	IAV_ENCODE_SOURCE_DRAM_FIRST = IAV_ENCODE_SOURCE_MAIN_DRAM,
	IAV_ENCODE_SOURCE_DRAM_LAST = IAV_ENCODE_SOURCE_MAIN_DRAM + 1,
	IAV_ENCODE_SOURCE_DRAM_TOTAL_NUM = IAV_ENCODE_SOURCE_DRAM_LAST - IAV_ENCODE_SOURCE_DRAM_FIRST,

	/* VCA source buffers */
	IAV_ENCODE_SOURCE_VCA_FIRST = IAV_ENCODE_SOURCE_MAIN_VCA,
	IAV_ENCODE_SOURCE_VCA_LAST = IAV_ENCODE_SOURCE_MAIN_VCA + 1,
	IAV_ENCODE_SOURCE_VCA_TOTAL_NUM = IAV_ENCODE_SOURCE_VCA_LAST - IAV_ENCODE_SOURCE_VCA_FIRST,

	IAV_ENCODE_SOURCE_BUFFER_FIRST = IAV_ENCODE_SOURCE_MAIN_BUFFER,
	IAV_ENCODE_SOURCE_BUFFER_LAST = IAV_ENCODE_SOURCE_TOTAL_NUM,
};

typedef enum {
	IAV_YUV_400_FORMAT = 0,
	IAV_YUV_420_FORMAT = 1,
	IAV_YUV_422_FORMAT = 2,
	IAV_YUV_DATA_FORMAT_TOTAL_NUM,
	IAV_YUV_DATA_FORMAT_FIRST = IAV_YUV_400_FORMAT,
	IAV_YUV_DATA_FORMAT_LAST = IAV_YUV_DATA_FORMAT_TOTAL_NUM,
} IAV_YUV_DATA_FORMAT;

typedef enum {
	IAV_CBR						= 0,
	IAV_VBR,
	IAV_CBR_QUALITY_KEEPING,
	IAV_VBR_QUALITY_KEEPING,
	IAV_CBR2,
	IAV_VBR2,
} iav_rate_control_mode;

typedef enum {
	IAV_CHIP_ID_S2_UNKNOWN = -1,
	IAV_CHIP_ID_S2_33 = 0,
	IAV_CHIP_ID_S2_55 = 1,
	IAV_CHIP_ID_S2_66 = 2,
	IAV_CHIP_ID_S2_88 = 3,
	IAV_CHIP_ID_S2_99 = 4,
	IAV_CHIP_ID_S2_22 = 5,
	IAV_CHIP_ID_S2_TOTAL_NUM,
	IAV_CHIP_ID_S2_FIRST = IAV_CHIP_ID_S2_33,
	IAV_CHIP_ID_S2_LAST = IAV_CHIP_ID_S2_TOTAL_NUM,
} iav_chip_s2_id;

typedef enum {
	IAV_ENCODE_NONE,	// none
	IAV_ENCODE_H264,	// H.264
	IAV_ENCODE_MJPEG,	// MJPEG
} IAV_ENCODE_TYPE;

typedef enum {
	IAV_ENCODE_CURRENT_MODE = 255,
	IAV_ENCODE_FULL_FRAMERATE_MODE = 0,
	IAV_ENCODE_WARP_MODE = 1,
	IAV_ENCODE_HIGH_MEGA_MODE = 2,
	IAV_ENCODE_CALIBRATION_MODE = 3,
	IAV_ENCODE_HDR_FRAME_MODE = 4,
	IAV_ENCODE_HDR_LINE_MODE = 5,
	IAV_ENCODE_HIGH_MP_FULL_PERF_MODE = 6,
	IAV_ENCODE_FULL_FPS_FULL_PERF_MODE = 7,
	IAV_ENCODE_MULTI_VIN_MODE = 8,
	IAV_ENCODE_HISO_VIDEO_MODE = 9,
	IAV_ENCODE_HIGH_MP_WARP_MODE = 10,
	IAV_ENCODE_MODE_TOTAL_NUM,
	IAV_ENCODE_MODE_FIRST = IAV_ENCODE_FULL_FRAMERATE_MODE,
	IAV_ENCODE_MODE_LAST = IAV_ENCODE_MODE_TOTAL_NUM,
} IAV_ENCODE_MODE;

typedef enum {
	IAV_STREAM_STATE_UNKNOWN = 0,		// uninitialized or unconfigured
	IAV_STREAM_STATE_READY_FOR_ENCODING = 1,		//configured ready, but not started encoding yet
	IAV_STREAM_STATE_ENCODING = 2,		// encoding
	IAV_STREAM_STATE_STARTING = 3,		// transition state: starting to encode
	IAV_STREAM_STATE_STOPPING = 4,		// transition state: stopping encoding
	IAV_STREAM_STATE_ERROR	 = 255,		// known error
} iav_encode_stream_state_ex_t;

typedef enum {
	IAV_SOURCE_BUFFER_STATE_UNKNOWN	 = 0, // uninitialized or unconfigured
	IAV_SOURCE_BUFFER_STATE_IDLE = 1,     //configured, but not used by any stream to encode
	IAV_SOURCE_BUFFER_STATE_BUSY = 2, 	//configured and used by at least one stream to encode
	IAV_SOURCE_BUFFER_STATE_ERROR = 255,	//known error
} iav_source_buffer_state_ex_t;

typedef enum {
	IAV_SOURCE_BUFFER_TYPE_OFF = 0, 		// source buffer disabled
	IAV_SOURCE_BUFFER_TYPE_ENCODE = 1, 	// source buffer for encoding
	IAV_SOURCE_BUFFER_TYPE_PREVIEW = 2,     	// source buffer for preview
} iav_source_buffer_type_ex_t;

typedef enum {
	IAV_DSP_PARTITION_RAW = (1 << 0),
	IAV_DSP_PARTITION_MAIN_CAPTURE = (1 << 1),
	IAV_DSP_PARTITION_PREVIEW_A = (1 << 2),
	IAV_DSP_PARTITION_PREVIEW_B = (1 << 3),
	IAV_DSP_PARTITION_PREVIEW_C = (1 << 4),
	IAV_DSP_PARTITION_MAIN_CAPTURE_ME1 = (1 << 5),
	IAV_DSP_PARTITION_PREVIEW_A_ME1 = (1 << 6),
	IAV_DSP_PARTITION_PREVIEW_B_ME1 = (1 << 7),
	IAV_DSP_PARTITION_PREVIEW_C_ME1 = (1 << 8),
	IAV_DSP_PARTITION_POST_MAIN = (1 << 9),
	IAV_DSP_PARTITION_EFM_ME1 = (1 << 10),
	IAV_DSP_PARTITION_EFM_YUV = (1 << 11),
	IAV_DSP_PARTITION_NUM = 12,
	IAV_DSP_PARTITION_ALL = (1 << IAV_DSP_PARTITION_NUM) - 1,
} IAV_DSP_PARTITION_TYPE;

typedef enum {
	IAV_PREVIEW_STATE_UNKNOWN  = 0, //uninitialized or unconfigured
	IAV_PREVIEW_STATE_DISABLED = 1, //disabled
	IAV_PREVIEW_STATE_ENABLED  = 2,	//enabled normal preview
	IAV_PREVIEW_STATE_LOW_POWER = 3,	 //low power smem preview
} iav_preview_state_ex_t;

typedef enum {
	IAV_NO_AUD_NO_SEI = 0,
	IAV_AUD_BEFORE_SPS_WITH_SEI = 1,
	IAV_AUD_AFTER_SPS_WITH_SEI = 2,
	IAV_NO_AUD_WITH_SEI = 3,
	IAV_AUD_WITH_PPS_SEI = 4,
	IAV_AU_TYPE_TOTAL_NUM,
	IAV_AU_TYPE_FIRST = 0,
	IAV_AU_TYPE_LAST = IAV_AU_TYPE_TOTAL_NUM,
} iav_au_type_ex_t;

typedef enum {
	H264_BASELINE_PROFILE = 0,
	H264_MAIN_PROFILE = 1,
	H264_HIGH_PROFILE = 2,
	H264_PROFILE_TOTAL_NUM,
	H264_PROFILE_FIRST = 0,
	H264_PROFILE_LAST = H264_PROFILE_TOTAL_NUM,
} iav_encode_profile_ex_t;


/**********************************************************
 *
 *			Structure definitions
 *
 *********************************************************/

typedef struct iav_img_size_s {
	u16	width;
	u16	height;
} iav_img_size_t;

// IAV_IOC_ENTER_ENCODE_MODE
typedef struct iav_enc_mode_config_s {
	u8	keep_mode;
	u8	enable_lowres_encoding;// 2rd stream
	u8	enable_piv;// 3rd stream
	u8	reserved;
} iav_enc_mode_config_t;

typedef struct iav_enc_info_s {
	iav_img_size_t		main_img;
	iav_img_size_t		second_img;
	iav_img_size_t		piv_img;
	u32				no_preview_on_vout;

	u8				enable_second_stream;
	u8				enable_piv_stream;
	u8				disable_previewA;
	u8				disable_previewB;
} iav_enc_info_t;

typedef struct iav_enc_config_s {
	u8	flags;			// in - for IOCTL
	u8	enc_id;			// 0 - main; 1 - secondary
	u16	reserved;

	IAV_ENCODE_TYPE	encode_type;

	// depends on encode_type
	union {
		struct i1_h264_config {	// H.264
			u16	M;
			u16	N;

			u8	idr_interval;
			u8	gop_model;
			u8	bitrate_control;
			u8	calibration;

			u8	vbr_ness;
			u8	min_vbr_rate_factor;	// 0 - 100
			u16	max_vbr_rate_factor;	// 0 - 400

			u32	average_bitrate;

			u8	entropy_codec;		//0: (default)CABAC-Main Profile,    1: CAVLC- Baseline Profile
		} h264;

		struct i1_jpeg_config {	// jpeg
			u8	chroma_format;
			u8	init_quality_level;
			u16	thumb_active_w;
			u16	thumb_active_h;
			u16	thumb_dram_w;
			u16	thumb_dram_h;
		} jpeg;
	} u;
} iav_enc_config_t;

typedef struct iav_enc_state_s {
	u8	enc_id;			// 0 - main; 1 - secondary
	u8	encode_state;		// 0 - IDLE, 1 - BUSY
	u32	encode_state_hist;		// buffer at most 32 last enc state
} iav_enc_state_t;

typedef struct iav_frame_desc_s {
	u8		flags;		// in -for ioctl
	u8		enc_id;		// in
	u16		fd_id;		// out - entry offset in bits info fifo

	u8		pic_type;
	u8		pic_struct;
	u8		is_eos;
	u8		reserved;

	u32		frame_num;
	u32		pic_size;

	void		*start_addr;
	void		*usr_start_addr;
	u64		pts_64;
} iav_frame_desc_t;

typedef struct iav_osd_blend_info_s {
	u8		enc_id;			// input
	u8		area_id;			// output
	u16		reserved;

	void		*osd_addr_y;		// output, user address of osd y planar
	void		*osd_addr_uv;	// output, user address of osd uv planar
	void		*alpha_addr_y;	// output
	void		*alpha_addr_uv;	// output

	u16		osd_width;		// input
	u16		osd_height;		// input
	u16		osd_start_x;		// input
	u16		osd_start_y;		// input

	u32		osd_size;			// internal use
	void		*buffer_start[2];	// internal use, virtual address of osd double buffer
	u32		buffer_id;		// internal use
} iav_osd_blend_info_t;

typedef struct iav_zoom_info_s {
	u32		zoom_x;			// 16.16 format
	u32		zoom_y;			// 16.16 format
} iav_zoom_info_t;

typedef struct iav_prev_src_win_s {
	u16		prev_src_w;
	u16		prev_src_h;
	u16		prev_src_x_offset;
	u16		prev_src_y_offset;
} iav_prev_src_win_t;

typedef struct iav_piv_jpeg_capture_s {
	u8		is_mjpeg;
	u8		reserved[3];
	u16		main_jpeg_w;
	u16		main_jpeg_h;
	u16		thumbnail_w;
	u16		thumbnail_h;
	u16		screem_thumbnail_w;
	u16		screem_thumbnail_h;
} iav_piv_jpeg_capture_t;

typedef struct iav_update_preview_buffer_config_s {
	u8		buffer_id;//preview A, B, C
	u8		reserved[3];

	u16		src_off_x;
	u16		src_off_y;
	u16		src_size_x;
	u16		src_size_y;

	u16		scaled_size_x;
	u16		scaled_size_y;
} iav_update_preview_buffer_config_t;

/* Followings are deprecated structures */
typedef struct iav_encode_format_s {
	u8				enc_id;                     // 0 - main, 1 - secondary
	u8				flags;			// in - for IOCTL
	u16				reserved;

	IAV_ENCODE_TYPE	encode_type;
	u16				encode_width;	//encode width
	u16				encode_height;	//encode height
	u16				encode_x;		//crop start x
	u16				encode_y;		//crop start y
} iav_encode_format_t;

typedef struct iav_preview_format_s {
	u16	width;
	u16	height;
	u16	format;
	u16	frame_rate;
} iav_preview_format_t;

typedef struct iav_jpeg_quality_s {
	u32	stream;		// 0 - main, 1 - secondary
	u32	quality;	// 1 - 100
} iav_jpeg_quality_t;

typedef struct iav_pic_info_s {
	u16	ar_x;
	u16	ar_y;
	u8	frame_mode;
	u8	reserved[3];
	u32	rate;
	u32	scale;
	u16	width;
	u16	height;
	u32	reserved2[4];	// for future use
} iav_pic_info_t;

typedef struct iav_h264_config_s {
	u16	stream;	// 0 - main; 1 - secondary
	u16	reserved;
	u16	M;
	u16	N;

	u8	idr_interval;
	u8	gop_model;
	u8	bitrate_control;
	u8	calibration;

	u8	vbr_ness;
	u8	min_vbr_rate_factor;	// 0 - 100
	u16	max_vbr_rate_factor;	// 0 - 400

	u32	average_bitrate;

	s8	slice_alpha_c0_offset_div2;  //alpha value for deblocking filter,   -6 to 6 inclusive,   from weak to strong filtering
	s8	slice_beta_offset_div2;      //beta value for deblocking filter,  -6 to 6 inclusive,  from weak to strong filtering
	u8	deblocking_filter_disable;   //deblocking filter disable 1: disable to use manual control   0:enable
	u8	entropy_codec;		//0: (default)CABAC-Main Profile,    1: CAVLC- Baseline Profile

	u8	force_intlc_tb_iframe;	//0: (default), first top field to be I-frame in interlaced encoding,   1: force first two fields to be I-frame in interlaced encoding
	u8	deintlc_for_intlc_vin;	//0: (DEINTLC_OFF), interlaced VIN will be encoded into interlaced video(field encoding)  1: DEINTLC_WEAVE_MODE 2 :DEINTLC_BOB_MODE
	u8	reserved2;
	u8	reserved3;

	iav_pic_info_t	pic_info;	// output
} iav_h264_config_t;

typedef struct iav_prev_buf_s {
	u8		buffer_id;		// 0 - main, 1 - prev A, 2 - prev B, 3 -prev C
	u8		chroma_format;
	u16		buf_width;
	u16		buf_height;
	u16		buf_pitch;		// dynamic

	void		*luma_addr;		// dynamic
	void		*chroma_addr;	// dynamic
} iav_img_buf_t;

typedef struct bits_info_s {
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
} bits_info_t;

#define NUM_USER_DESC		32
typedef struct bs_fifo_info_s {
	u32		count;
	int		more;		// more encoded frames in driver
	u32		rt_counter;	// real time timer, for internal use only
	bits_info_t	desc[NUM_USER_DESC];
} bs_fifo_info_t;

typedef struct iav_bsb_fullness_s {
	u32		total_size;
	u32		fullness;
	u32		total_pictures;
	u32		num_pictures;
} iav_bsb_fullness_t;

typedef struct iav_encode_state_s {
	int	still_encode_state;
} iav_encode_state_t;


/**********************************************************
 *
 *			Following definitions are for IPCam
 *
 *********************************************************/

typedef struct bits_info_ex_s {
	u32	frame_num;
	u32	PTS;
	u32	start_addr;
	u32	pic_type	: 3;
	u32	level_idc	: 3;
	u32	ref_idc		: 1;
	u32	pic_struct	: 1;
	u32	pic_size	: 24;

	u32	channel_id	: 8;
	u32	stream_id	: 8;

	u32	cavlc_pjpeg	: 1;			// no used.
	u32	stream_end : 1;  			// 0: normal stream frames,  1: stream end null frame
	u32	top_field_first		: 1;	// 0: non-CAVLC   1: CAVLC pjpeg
	u32	repeat_first_field	: 1;	// 0: normal stream frames,  1: stream end null frame
	u32	progressive_sequence :1;
	u32	pts_minus_dts_in_pic :5;	//(pts-dts) in frame or field unit. (if progressive_sequence=0, in field unit. If progressive_sequence=1, in frame unit.)
	u32	res_1		: 6;
	u32	session_id;				//use 32-bit session id
	u32	pjpeg_start_addr;		//not used
	u16	pic_width;
	u16	pic_height;

	u16	addr_offset_sliceheader;//slice header address offset in
	u16	res_2;     				//slice header address offset in

	/* cpb fullness with considering real cabac /cavlc bits. use sign int,
	 * such that negative value indicate cpb underflow.
	 */
	s32	cpb_fullness;
	u64	stream_pts;				//64-bit pts for that stream
	u64	monotonic_pts;			//64-bit pts monotonic value (since Linux boot)
	u32	frame_index;
	u8  jpeg_quality;			// quality value for jpeg stream
	u8  res_3[3];
} bits_info_ex_t;

typedef struct iav_rect_ex_s {
	u16	x;
	u16	y;
	u16	width;
	u16	height;
} iav_rect_ex_t;

typedef struct iav_bitrate_info_ex_s {
	iav_stream_id_t		id;
	iav_rate_control_mode	rate_control_mode;
	u32	cbr_avg_bitrate;
	u32	vbr_min_bitrate;
	u32	vbr_max_bitrate;
} iav_bitrate_info_ex_t;

typedef struct iav_change_gop_ex_s {
	iav_stream_id_t		id;
	u32  N;
	u32  idr_interval;
} iav_change_gop_ex_t;

typedef struct iav_chroma_format_info_ex_s {
	iav_stream_id_t		id;
	u8	chroma_format;
	u8	reserved[3];
} iav_chroma_format_info_ex_t;

typedef struct iav_frame_drop_info_ex_s {
	iav_stream_id_t	id;
	u16	drop_frames_num : 9;
	u16	reserved : 7;
	u16	reserved1;
} iav_frame_drop_info_ex_t;

typedef struct iav_change_framerate_factor_ex_s {
	iav_stream_id_t		id;
	u8	ratio_numerator;
	u8	ratio_denominator;
	u8	keep_fps_in_ss : 1;
	u16	reserved : 15;
} iav_change_framerate_factor_ex_t;

typedef struct iav_h264_enc_param_ex_s {
	iav_stream_id_t		id;
	u16	intrabias_P;
	u16	intrabias_B;
	u16	nonSkipCandidate_bias;
	u16	skipCandidate_threshold;
	u16	cpb_underflow_num;
	u16	cpb_underflow_den;
	u8	zmv_threshold;
	s8	mode_bias_I4Add;
	s8	mode_bias_I16Add;
	s8	mode_bias_Inter8Add;
	s8	mode_bias_Inter16Add;
	s8	mode_bias_DirectAdd;
	s8	user1_intra_bias;
	s8	user1_direct_bias;
	s8	user2_intra_bias;
	s8	user2_direct_bias;
	s8	user3_intra_bias;
	s8	user3_direct_bias;
} iav_h264_enc_param_ex_t;

typedef struct iav_change_qp_limit_ex_s {
	iav_stream_id_t		id;
	u8	qp_min_on_I;
	u8	qp_max_on_I;
	u8	qp_min_on_P;
	u8	qp_max_on_P;
	u8	qp_min_on_B;
	u8	qp_max_on_B;
	u8	qp_min_on_C;
	u8	qp_max_on_C;
	u8	qp_max_on_D;
	u8	qp_min_on_D;
	u8	qp_min_on_Q;
	u8	qp_max_on_Q;
	u8	i_qp_reduce;
	u8	p_qp_reduce;
	u8	b_qp_reduce;
	u8	c_qp_reduce;
	u8	q_qp_reduce;
	u8	adapt_qp;
	u8	skip_flag;
	u8	log_q_num_minus_1;
} iav_change_qp_limit_ex_t;

typedef struct iav_change_intra_mb_rows_ex_s {
	iav_stream_id_t		id;
	u8	intra_refresh_mb_rows;
	u8	reserved[3];
} iav_change_intra_mb_rows_ex_t;

typedef struct iav_panic_control_param_ex_s {
	iav_stream_id_t	id;
	u8	panic_num;
	u8	panic_den;
	u8	reserved[2];
} iav_panic_control_param_ex_t;

typedef enum {
	QP_FRAME_I = 0,
	QP_FRAME_P = 1,
	QP_FRAME_B = 2,
	QP_FRAME_TYPE_NUM = 3,
} QP_FRAME_TYPES;

#define QP_MATRIX_ADV_DEFAULT_VALUE	   (64)
#define QP_MATRIX_VALUE_MIN		(0)
#define QP_MATRIX_VALUE_MAX		(3)
#define QP_MATRIX_VALUE_MIN_ADV		(QP_MATRIX_ADV_DEFAULT_VALUE - 51)
#define QP_MATRIX_VALUE_MAX_ADV		(QP_MATRIX_ADV_DEFAULT_VALUE + 51)

typedef enum {
	QP_ROI_TYPE_BASIC = 0,
	QP_ROI_TYPE_ADV,
	QP_ROI_TYPE_TOTAL_NUM,
	QP_ROI_TYPE_FIRST = QP_ROI_TYPE_BASIC,
	QP_ROI_TYPE_LAST = QP_ROI_TYPE_TOTAL_NUM,
} QP_ROI_TYPE;

#define SLICE_ALPHA_VALUE_MAX 	6
#define SLICE_ALPHA_VALUE_MIN	-6
#define SLICE_BETA_VALUE_MAX	6
#define SLICE_BETA_VALUE_MIN	-6
typedef struct iav_h264_config_ex_s {
	iav_stream_id_t		id;
	u32	average_bitrate;
	iav_pic_info_t	pic_info;	// output

	u16	M;
	u16	N;
	u8	idr_interval;
	u8	gop_model;
	u8	bitrate_control;

	s8	slice_alpha_c0_offset_div2;  //alpha value for deblocking filter,   -6 to 6 inclusive,   from weak to strong filtering
	s8	slice_beta_offset_div2;      //beta value for deblocking filter,  -6 to 6 inclusive,  from weak to strong filtering
	u8	deblocking_filter_disable;   //deblocking filter disable 1: disable to use manual control   0:enable
	u8	entropy_codec;		//0: (default)CABAC-Main Profile, 1: CAVLC- Baseline Profile

	u8	force_intlc_tb_iframe;	//0: (default), first top field to be I-frame in interlaced encoding,   1: force first two fields to be I-frame in interlaced encoding
	u8	allow_i_adv;		//TOREVIEW: all below items are added to enable advanced control on encoding parameters
	u8	cpb_buf_idc;

	u8	en_panic_rc :2;
	u8	cpb_cmp_idc :2;
	u8	fast_rc_idc :4;
	u8	chroma_format : 1;	//0: IAV_H264_CHROMA_FORMAT_YUV420; 1: IAV_H264_CHROMA_FORMAT_MONO
	u8	high_profile : 1;
	u8	qp_roi_type : 2;
	u8	reserved1 : 4;

	// rate control related
	u8	qp_min_on_I;
	u8	qp_max_on_I;
	u8	qp_min_on_P;
	u8	qp_max_on_P;
	u8	qp_min_on_B;
	u8	qp_max_on_B;
	u8	i_qp_reduce;
	u8	p_qp_reduce;
	u8	adapt_qp;
	u8	skip_flag;
	u8	zmv_threshold;
	u8	qp_max_on_D;
	u8	qp_min_on_D;
	u8	c_qp_reduce;

	// intra refresh MB rows
	u8	intra_refresh_mb_rows;

	// h264 syntax config
	/* 0: No AUD, No SEI; 1: AUD before SPS, PPS, with SEI;
	 * 2: AUD after SPS, PPS, with SEI; 3: No AUD, with SEI
	 * 4: No AUD, with PPS per frame */
	u8	au_type;
	u32	cpb_user_size;
	s8	qp_delta[QP_FRAME_TYPE_NUM][4];
	u8	qp_roi_enable;
	u8	profile;
	u8	panic_num;
	u8	panic_den;
	u32	pic_size_control;

	//extend rate control related
	u16	intrabias_P;
	u16	intrabias_B;
	u16	nonSkipCandidate_bias;
	u16	skipCandidate_threshold;
	u16	cpb_underflow_num;
	u16	cpb_underflow_den;

	u8	qp_min_on_C;
	u8	qp_max_on_C;
	u8	b_qp_reduce;
	u8	qmatrix_4x4_enable;
	u8	*qmatrix_4x4_daddr;
	u8	qp_min_on_Q;
	u8	qp_max_on_Q;
	u8	q_qp_reduce;
	u8	log_q_num_minus_1;
	s8	mode_bias_I4Add;
	s8	mode_bias_I16Add;
	u8	debug_long_p_interval;
	s8	mode_bias_Inter8Add;
	s8	mode_bias_Inter16Add;
	s8	mode_bias_DirectAdd;

	s8	user1_intra_bias;
	s8	user1_direct_bias;
	s8	user2_intra_bias;
	s8	user2_direct_bias;
	s8	user3_intra_bias;
	s8	user3_direct_bias;
	u8	numRef_P;
	u8	reserved2[3];
}iav_h264_config_ex_t;

typedef struct iav_qp_roi_matrix_ex_s {
	iav_stream_id_t	id;
	s8				delta[QP_FRAME_TYPE_NUM][4];
	u8				enable;
	u8				type;
	u8				reserved[2];
#ifdef CONFIG_IAV_FOR_DEBUG_DSP
	u8				*daddr;
#else
	u8				*daddr[QP_FRAME_TYPE_NUM];
#endif
	u32				size;
} iav_qp_roi_matrix_ex_t;

typedef enum {
	IAV_QM_INTRA_Y = 0,
	IAV_QM_INTRA_U = 1,
	IAV_QM_INTRA_V = 2,
	IAV_QM_INTER_Y = 3,
	IAV_QM_INTER_U = 4,
	IAV_QM_INTER_V = 5,
	IAV_QM_TYPE_NUM,
	IAV_QM_TYPE_FIRST = 0,
	IAV_QM_TYPE_LAST = IAV_QM_TYPE_NUM,
} IAV_H264_QMATRIX_TYPES;

typedef struct iav_h264_q_matrix_ex_s {
	iav_stream_id_t		id;
	u8	qm_4x4[IAV_QM_TYPE_NUM][16];
} iav_h264_q_matrix_ex_t;

typedef struct iav_jpeg_config_ex_s {
	iav_stream_id_t			id;

	/*  0: IAV_JPEG_CHROMA_FORMAT_YUV422;
	 *  1: IAV_JPEG_CHROMA_FORMAT_YUV420;
	 *  2: IAV_JPEG_CHROMA_FORMAT_MONO
	 */
	u8	chroma_format;
	u8	quality;		// 1 ~ 100 ,  100 is best quality
	u8	reserved[2];
	u8	*jpeg_quant_matrix;
} iav_jpeg_config_ex_t;

typedef enum {
	IAV_ENC_DURATION_FOREVER = 0,
	IAV_ENC_DURATION_MIN = 0,
	IAV_ENC_DURATION_MAX = 0xFFFF,		// The maximum duration frame number is about (1<<16).
} IAV_ENCODE_DURATION;
/*
  encode format config for each stream. The configuration for one frame is independent
  from another, unless the total resource is out of limit.
*/
typedef struct iav_encode_format_ex_s {
	iav_stream_id_t			id;
	u8	encode_type;		//0: none; 1: H.264 (IAV_ENCODE_H264); 2: MJPEG  (IAV_ENCODE_MJPEG)
	u8	source;
	u8	hflip : 1;
	u8	vflip : 1;
	u8	rotate_clockwise : 1;
	u8	negative_offset_disable : 1;
	u8	reserved1 : 4;
	u8	reserved2;
	u16	encode_width;		//encode width
	u16	encode_height;		//encode height
	u16	encode_x;		//crop start x
	u16	encode_y;		//crop start y
	u32	duration;		//0: encode for ever until stop command; non-zero: encode frame numbers
} iav_encode_format_ex_t;

/*get select stream's state */
typedef struct iav_source_buffer_info_ex_s{
	iav_buffer_id_t					id;
	iav_source_buffer_state_ex_t	state;
} iav_source_buffer_info_ex_t;

/*get select stream's state */
typedef struct iav_encode_stream_info_ex_s{
	iav_stream_id_t					id;
	iav_encode_stream_state_ex_t	state;
} iav_encode_stream_info_ex_t;

typedef struct iav_preview_format_ex_s {
	u8	id;		//0 : preview-A,  1: preview-B
	u8 	format;		//0: progressive 1: interlaced
	u16	width;
	u16	height;
	u8	frame_rate;	//0: 29.97  1: 59.94 ,  30, 24, 15, etc
	u8	enable;		//0: disable 1:enable 2:low power enable
} iav_preview_format_ex_t;

typedef struct iav_me1_buffer_info_ex_s {
	u32	source;
	u8	*addr;
	u32	width;
	u32	height;
	u32	pitch;
	u32	seqnum;
	u64	mono_pts;
	u64	dsp_pts;
	u32	flag;
	u32	reserved;
} iav_me1_buffer_info_ex_t;

typedef struct iav_digital_zoom_ex_s {
	u32	source;
	iav_rect_ex_t	input;
} iav_digital_zoom_ex_t;

typedef struct iav_source_buffer_format_all_ex_s {
	u16	main_width;					// width of main source buffer
	u16	main_height;					// height of main source buffer
	u16	main_deintlc_for_intlc_vin;	 	// deintlc mode of main source buffer
	u16	second_width;				// width of secondary source buffer
	u16	second_height;				// height of secondary source buffer
	u16	second_input_width;			// width of second input window
	u16	second_input_height;			// height of second input window
	u16	second_deintlc_for_intlc_vin;	// deintlc mode of second source buffer
	u16	third_width;					// width of third source buffer
	u16	third_height;					// height of third source buffer
	u16	third_input_width;				// width of main input window
	u16	third_input_height;			// width of main input window
	u16	third_deintlc_for_intlc_vin;		// deintlc mode of third source buffer
	u16	fourth_width;					// width of fourth source buffer
	u16	fourth_height;					// height of fourth source buffer
	u16	fourth_input_width;			// width of fourth source buffer
	u16	fourth_input_height;			// height of fourth source buffer
	u16	fourth_deintlc_for_intlc_vin;	// deintlc mode of fourth source buffer

	u8	intlc_scan;						//0: OFF  1: use progressive VIN to encode interlaced video
	u8	second_unwarp;
	u8	third_unwarp;
	u8	fourth_unwarp;
	u16	pre_main_width;
	u16	pre_main_height;
	iav_rect_ex_t	pre_main_input;
} iav_source_buffer_format_all_ex_t;

typedef struct iav_preview_buffer_format_all_ex_s {
	u16	main_preview_width;			// width of main preview buffer (preview B)
	u16	main_preview_height;			// height of main preview buffer (preview B)
	u16	second_preview_width;		// width of second preview buffer (preview A)
	u16	second_preview_height;		// height of second preview buffer (preview A)
} iav_preview_buffer_format_all_ex_t;

typedef struct iav_source_buffer_type_all_ex_s {
	iav_source_buffer_type_ex_t  main_buffer_type;
	iav_source_buffer_type_ex_t  second_buffer_type;
	iav_source_buffer_type_ex_t  third_buffer_type;
	iav_source_buffer_type_ex_t  fourth_buffer_type;
} iav_source_buffer_type_all_ex_t;

typedef struct iav_reso_ex_s {
	u16 width;
	u16 height;
} iav_reso_ex_t;

typedef struct iav_source_buffer_format_ex_s {
	u32					source;	//source buffer id
	iav_reso_ex_t		size;
	iav_rect_ex_t		input;
} iav_source_buffer_format_ex_t;

typedef struct iav_source_buffer_setup_ex_s {
	iav_reso_ex_t	size[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_rect_ex_t	input[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_source_buffer_type_ex_t	type[IAV_ENCODE_SOURCE_TOTAL_NUM];
	u8	unwarp[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_reso_ex_t	pre_main;
	iav_rect_ex_t	pre_main_input;
} iav_source_buffer_setup_ex_t;

typedef struct iav_lowdelay_refresh_period_ex_s {
	iav_stream_id_t			id;
	u32	refresh_period;
} iav_lowdelay_refresh_period_ex_t;

typedef struct stream_encode_size_limit_s{
	int width;
	int height;
} stream_encode_size_limit_t;

typedef struct iav_sync_framerate_factor_ex_s {
	int enable[IAV_STREAM_MAX_NUM_ALL];
	iav_change_framerate_factor_ex_t framefactor[IAV_STREAM_MAX_NUM_ALL];
} iav_sync_framerate_factor_ex_t;

typedef struct iav_system_resource_setup_ex_s {
	iav_reso_ex_t buffer_max_size[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_reso_ex_t  stream_max_size[IAV_STREAM_MAX_NUM_ALL] ;
	u8	stream_max_GOP_M[IAV_STREAM_MAX_NUM_ALL] ;
	u8	stream_max_GOP_N[IAV_STREAM_MAX_NUM_ALL] ;
	u8	stream_max_advanced_quality_model[IAV_STREAM_MAX_NUM_ALL];
	u8	stream_2x_search_range[IAV_STREAM_MAX_NUM_ALL];
	iav_reso_ex_t voutA;  // Read-only;
	iav_reso_ex_t voutB;  // Read-only;
	u16	max_warp_input_width;
	u16	max_warp_output_width;
	u8	max_num_encode_streams;
	u8	max_num_cap_sources;
	u8	total_memory_size;	// Read-only;
	u8	encode_mode;

	u8	exposure_num;	// 2-5 for HDR frame/line mode, 1 for other mode
	u8	vin_num;		// 2-4 for Multi CFA VIN mode, 1 for other mode
	u8	rotate_possible;
	u8	raw_capture_enable;			// Only works in some modes

	u8	sharpen_b_enable;
	u8	max_vin_stats_lines_top;		// Only works in some modes
	u8	max_vin_stats_lines_bottom;		// Only works in some modes
	u8	max_chroma_noise_shift;			// 0-2. 0 = 32, 1 = 64, 2 = 128

	u8	vskip_before_encode;
	u8	vca_buf_src_id;
	u8	hwarp_bypass_possible : 1;
	u8	enc_from_raw_enable : 1;
	u8	extra_2x_zoom_enable : 1;
	u8	mixer_b_enable : 1;
	u8	map_dsp_partition : 1;
	u8	vout_b_letter_box_enable : 1;
	u8	yuv_input_enhanced : 1;
	u8	reserved : 1;
	u8	max_dram_frame[IAV_ENCODE_SOURCE_DRAM_TOTAL_NUM];

	s8	extra_dram_buf[4];
	u8	debug_max_ref_P[IAV_STREAM_MAX_NUM_ALL] ;
	s32	debug_chip_id : 4;
	u32	reserved1 : 28;
} iav_system_resource_setup_ex_t;

typedef enum {
	IAV_PRIVACY_MASK_SHIFT = 31,
} IAV_MASK_BIT_SHIFT;	// For S2 only

typedef enum {
	IAV_PRIVACY_MASK_UNIT_MB = 0,
	IAV_PRIVACY_MASK_UNIT_PIXEL = 1,
	IAV_PRIVACY_MASK_UNIT_PIXELRAW = 2,
} IAV_PRIVACY_MASK_UNIT;

typedef enum {
	IAV_PRIVACY_MASK_DOMAIN_MAIN = 0,
	IAV_PRIVACY_MASK_DOMAIN_VIN = 1,
	IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT = 2,
	IAV_PRIVACY_MASK_DOMAIN_STREAM = 3,
} IAV_PRIVACY_MASK_DOMAIN;

typedef struct iav_privacy_mask_setup_ex_s {
	u8	enable;	// 0: disable  1: enable
	u8	y;	// y of mask color
	u8	u;	// u of mask color
	u8	v;	// v of mask color
	u16	buffer_pitch;	// x direction (horizontal) buffer pitch
	u16	buffer_height;	// y direction (vertical ) buffer height
	u8*	buffer_addr;
} iav_privacy_mask_setup_ex_t;

typedef struct iav_privacy_mask_info_ex_s {
	u16	unit;
	u16	domain;
	u16	buffer_pitch;
	u16	pixel_width;	// Only used in pixel & pixel raw mask unit
	u8	stream_id;		// Only used when domain is stream
} iav_privacy_mask_info_ex_t;

typedef struct iav_system_setup_info_ex_s {
	u16	voutA_osd_blend_enable;	// for A5s
	u16	voutB_osd_blend_enable;	// for A5s
	u16	coded_bits_interrupt_enable;
	u8	pip_size_enable;		// for A5s

	/* "low_delay_cap_enable" only works when M = 1 mode, to reduce
	 * total encoding delay from capture stage.
	 */
	u8	low_delay_cap_enable :1;	// for A5s

	/* Determines the mapping from preview buffer to VOUT
	 * 0 - preview A for digital VOUT, preview B for HDMI / CVBS VOUT
	 * 1 - preview A for HDMI / CVBS VOUT, preview B for digital VOUT
	 */
	u8	vout_swap :1;

	/* Choose Privacy Mask type
	 * 0 - HDR based PM (pixel level), 1 - MCTF based PM (MB level) */
	u8	mctf_privacy_mask :1;
	u8	eis_delay_count :2;
	u8	dsp_noncached :1;
	u8	reserved :2;
	u32	audio_clk_freq;		// for A5s
	int	cmd_read_delay;
	u8	debug_enc_dummy_latency_count;	//Just for s2 verify enc_dummy_latency.
	u8	reserved1[3];
} iav_system_setup_info_ex_t;

typedef struct iav_digital_zoom_privacy_mask_ex_s {
	iav_digital_zoom_ex_t	zoom;
	iav_privacy_mask_setup_ex_t privacy_mask;
} iav_digital_zoom_privacy_mask_ex_t;

/* Total number of bins for QP histogram */
#define	IAV_QP_HIST_BIN_MAX_NUM		(16)
typedef struct iav_qp_hist_ex_s {
	u8	qp[IAV_QP_HIST_BIN_MAX_NUM];	// QP value for each bin
	u16	mb[IAV_QP_HIST_BIN_MAX_NUM];	// Macroblocks used per each bin
	u32	pic_type;
} iav_qp_hist_ex_t;


/*********************************************************
 *
 *	 New structures and enumerations for S2
 *
 *********************************************************/

typedef enum {
	NO_ROTATE_FLIP = 0,
	HORIZONTAL_FLIP = (1 << 0),
	VERTICAL_FLIP = (1 << 1),
	ROTATE_90 = (1 << 2),

	CW_ROTATE_90 = ROTATE_90,
	CW_ROTATE_180 = HORIZONTAL_FLIP | VERTICAL_FLIP,
	CW_ROTATE_270 = CW_ROTATE_90 | CW_ROTATE_180,
} IAV_WARP_ROTATE_FLIP_MODE;

typedef enum {
	MAX_NUM_WARP_AREAS = 8,
	MAX_GRID_WIDTH = 32,
	MAX_GRID_HEIGHT = 48,
	MAX_WARP_TABLE_SIZE = (MAX_GRID_WIDTH * MAX_GRID_HEIGHT),

	GRID_SPACING_PIXEL_16 = 0,
	GRID_SPACING_PIXEL_32 = 1,
	GRID_SPACING_PIXEL_64 = 2,
	GRID_SPACING_PIXEL_128 = 3,
	GRID_SPACING_PIXEL_256 = 4,
	GRID_SPACING_PIXEL_512 = 5,
} IAV_MULTI_REGION_WARPING_PARAMS;

typedef enum {
	CA_TABLE_COL_NUM = 32,
	CA_TABLE_ROW_NUM = 48,
	CA_TABLE_MAX_SIZE = (CA_TABLE_COL_NUM * CA_TABLE_ROW_NUM),

	CA_GRID_SPACING_16 = 0,
	CA_GRID_SPACING_32 = 1,
	CA_GRID_SPACING_64 = 2,
	CA_GRID_SPACING_128 = 3,
	CA_GRID_SPACING_256 = 4,
	CA_GRID_SPACING_512 = 5,
} IAV_CA_WARPATION_PARAMS;

typedef struct iav_sharpen_filter_cfg_s {
	u8	sharpen_b_enable;
	u8	reserved[3];
} iav_sharpen_filter_cfg_t;

typedef struct iav_mctf_filter_strength_s {
	u32	v : 4;		// Bit [3:0] is the temporal adjustment for V
	u32	u : 4;		// Bit [7:4] is the temporal adjustment for U
	u32	y : 4;		// Bit [11:8] is the temporal adjustment for Y
	u32	reserved : 19;
	u32	privacy_mask : 1;	// Bit [31] is the privacy mask enabled flag
} iav_mctf_filter_strength_t;

typedef struct iav_warp_map_ex_s {
	u8	enable;
	u8	output_grid_col;
	u8	output_grid_row;
	u8	horizontal_spacing : 4;
	u8	vertical_spacing : 4;
	u32	addr;
} iav_warp_map_ex_t;

typedef struct iav_warp_vector_ex_s {
	u8	enable;
	u8	rotate_flip;
	u8	dup : 1;
	u8	src_rid : 5;
	u8	reserved0 : 2;
	u8	reserved1;
	iav_rect_ex_t	input;
	iav_rect_ex_t	output;
	iav_warp_map_ex_t hor_map;
	iav_warp_map_ex_t ver_map;
} iav_warp_vector_ex_t;

typedef struct iav_region_dptz_ex_s {
	iav_rect_ex_t	input;
	iav_rect_ex_t	output;
} iav_region_dptz_ex_t;

typedef struct iav_warp_dptz_ex_s {
	u16	buffer_id;	// Only support 2nd and 4th source buffer
	iav_region_dptz_ex_t	dptz[MAX_NUM_WARP_AREAS];
} iav_warp_dptz_ex_t;

typedef struct iav_warp_control_ex_s {
	u8	keep_dptz[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_warp_vector_ex_t	area[MAX_NUM_WARP_AREAS];
} iav_warp_control_ex_t;

typedef struct iav_enc_statis_config_ex_s {
	iav_stream_id_t	id;
	u32	enable_mv_dump : 1;
	u32	enable_qp_hist : 1;
	u32	reserved0 : 30;
	u8	mvdump_division_factor;
	u8	reserved1[3];
} iav_enc_statis_config_ex_t;

typedef enum {
	IAV_MV_DUMP_FLAG = (1 << 0),
	IAV_QP_HIST_DUMP_FLAG = (1 << 1),
} iav_statis_info_data_flag;

typedef enum {
	FT_P_SLICE = 0,
	FT_B_SLICE = 1,
	FT_I_SLICE = 2,
	FT_C_SLICE = 3,
} iav_statis_info_frame_type;

/*
 * Encode statistics info descriptor reported by DSP
 */
typedef struct iav_enc_statis_info_ex_s {
	/* Read / write parameters */
	u32	stream;
	u32	data_type;
	u16	flag;

	/* Read only parameters */
	u16	statis_index;
	u32	frame_num;
	u64	stream_pts;
	u64	mono_pts;
	u16	frame_type : 3;
	u16	reserved : 13;
	u16	mvdump_pitch;
	u32	mvdump_unit_size;
	u32	mv_start_addr;
	u32	qp_hist_addr;
} iav_enc_statis_info_ex_t;

typedef struct iav_motion_vector_ex_s {
	int	x : 14;			// Bit [13:0] is the x component in signed 14-bit format
	int	y : 13;			// Bit [26:14] is the y component in signed 13-bit format
	int	reserved : 5;
} iav_motion_vector_ex_t;

typedef struct iav_encmode_cap_ex_s {
	u8	encode_mode;
	u8	max_streams_num;
	u8	max_chroma_noise_strength;
	u8	reserved0;
	u32	sharpen_b_possible : 1;
	u32	rotate_possible : 1;
	u32	raw_cap_possible : 1;
	u32	raw_stat_cap_possible : 1;
	u32	dptz_I_possible : 1;
	u32	dptz_II_possible : 1;
	u32	vin_cap_offset_possible : 1;
	u32	hwarp_bypass_possible : 1;
	u32	svc_t_possible : 1;
	u32	enc_from_yuv_possible : 1;
	u32	enc_from_raw_possible : 1;
	u32	vout_swap_possible : 1;
	u32	mctf_pm_possible : 1;
	u32	hdr_pm_possible : 1;
	u32	video_freeze_possible : 1;
	u32	vca_buffer_possible : 1;
	u32	mixer_b_possible : 1;
	u32	map_dsp_partition : 1;
	u32	vout_b_letter_box_possible : 1;
	u32	yuv_input_enhanced_possible : 1;
	u32	reserved_possible : 12;
	u16	main_width_min;
	u16	main_height_min;
	u16	main_width_max;
	u16	main_height_max;
	u32	max_encode_MB;
	u16	min_encode_width;
	u16	min_encode_height;
} iav_encmode_cap_ex_t;

typedef struct iav_encbuf_cap_ex_s {
	u32	buf_id;
	u32	max_width;
	u32	max_height;
	u32	max_zoom_in_factor;
	u32	max_zoom_out_factor;
} iav_encbuf_cap_ex_t;

enum IAV_MMAP
{
	IAV_MMAP_NONE = 0,
	IAV_MMAP_BSB = 1,
	IAV_MMAP_BSB2 = 2,
	IAV_MMAP_DSP = 3,
	IAV_MMAP_FB = 4,
	IAV_MMAP_OVERLAY = 5,
	IAV_MMAP_PRIVACYMASK = 6,
	IAV_MMAP_QP_MATRIX = 7,
	IAV_MMAP_MV = 8,
	IAV_MMAP_PHYS = 9,
	IAV_MMAP_PHYS2 = 10,
	IAV_MMAP_IMGPROC = 11,
	IAV_MMAP_LOCAL_EXPO = 12,
	IAV_MMAP_QP_HIST = 13,
	IAV_MMAP_CMD_SYNC = 14,
	IAV_MMAP_DSP_PARTITION = 15,
	IAV_MMAP_USER	= 16,
};

typedef struct iav_gdma_copy_ex_s {
	u32	usr_src_addr;
	u32	usr_dst_addr;
	u16	src_pitch;
	u16	dst_pitch;
	u16	width;
	u16	height;
	u16	src_mmap_type;
	u16	dst_mmap_type;

	/* "non_cached" flag works for those memory not controlled by kernel,
	 * like BSB and DSP memory. */
	u8	src_non_cached;
	u8	dst_non_cached;
	u8	reserved[2];
} iav_gdma_copy_ex_t;


#define LOCAL_EXPO_GAIN_NUM    (256)
typedef struct iav_local_exposure_ex_s {
	u8  group_index;
	u8  enable;
	u16 radius;					/* 0-7 */
	u8  luma_weight_red;		/* 4:0 */
	u8  luma_weight_green;		/* 4:0 */
	u8  luma_weight_blue;		/* 4:0 */
	u8  luma_weight_sum_shift;	/* 4:0 */
	u32 gain_curve_table_addr;	/* 11:0 */
	u16 luma_offset;
	u16 reserved;
} iav_local_exposure_ex_t;

typedef struct iav_enc_dram_buf_frame_ex_s {
	/* The buf_id must be in the range of
	 * [IAV_ENCODE_SOURCE_DRAM_FIRST, IAV_ENCODE_SOURCE_DRAM_LAST) */
	u16	buf_id;
	u16	frame_id;
	iav_reso_ex_t	max_size;
	u8	* y_addr;
	u8	* uv_addr;
	u8	* me1_addr;
} iav_enc_dram_buf_frame_ex_t;

typedef struct iav_enc_dram_buf_update_ex_s {
	/* The buf_id must be in the range of
	 * [IAV_ENCODE_SOURCE_DRAM_FIRST, IAV_ENCODE_SOURCE_DRAM_LAST) */
	u16	buf_id;
	u16	frame_id;
	u32	frame_pts;
	iav_reso_ex_t size;
} iav_enc_dram_buf_update_ex_t;

typedef struct iav_ca_warp_s {
	iav_warp_map_ex_t	hor_map;
	iav_warp_map_ex_t	ver_map;
	u16	red_scale_factor;
	u16	blue_scale_factor;
} iav_ca_warp_t;

typedef struct iav_stream_privacy_mask_s {
	iav_stream_id_t id;
	u8	enable;	// 0: disable  1: enable
	u8	y;	// y of mask color
	u8	u;	// u of mask color
	u8	v;	// v of mask color
	u16	buffer_pitch;	// x direction (horizontal) buffer pitch
	u16	buffer_height;	// y direction (vertical ) buffer height
	u8*	buffer_addr;
} iav_stream_privacy_mask_t;

typedef struct iav_stream_offset_s {
	iav_stream_id_t id;
	u16 x;
	u16 y;
} iav_stream_offset_t;

typedef enum {
	IAV_DPTZ = 0x00,
	IAV_PM = 0x01,
	IAV_CAWARP = 0x02,
	IAV_STREAM_PM = 0x03,
	IAV_STREAM_OFFSET = 0x04,
	IAV_STREAM_OVERLAY = 0x05,
	IAV_VIDEO_PROC_NUM,
	IAV_VIDEO_PROC_FIRST = IAV_DPTZ,
	IAV_VIDEO_PROC_LAST = IAV_VIDEO_PROC_NUM
} iav_video_proc_id;

typedef struct overlay_insert_area_ex_s {	//extended type OSD insert
	u16	enable;
	u16	width;
	u16	pitch;
	u16	height;
	u32	total_size;
	u16	start_x;
	u16	start_y;
	u32	clut_id;
	u8	*data;	//overlay data buffer (without CLUT)
} overlay_insert_area_ex_t;

typedef enum iav_osd_type {
	IAV_OSD_CLUT_8BIT = 0,
	IAV_OSD_AYUV_16BIT = 1, //1:5:5:5 (AYUV)
	IAV_OSD_ARGB_16BIT = 2, //1:5:5:5 (ARGB)
	IAV_OSD_TYPE_NUM,
	IAV_OSD_TYPE_FIRST = IAV_OSD_CLUT_8BIT,
	IAV_OSD_TYPE_LAST = IAV_OSD_TYPE_NUM,
} iav_osd_type_t;

typedef struct overlay_insert_ex_s {
	#define MAX_NUM_OVERLAY_AREA	(6)
	iav_stream_id_t  id;
	u16	enable;
	u16	type : 8;
	u16	insert_always : 1;
	u16	reserved : 7;
	overlay_insert_area_ex_t	area[MAX_NUM_OVERLAY_AREA];
} overlay_insert_ex_t;

typedef struct iav_video_proc_s {
	int cid;
	union {
		iav_digital_zoom_ex_t	dptz;
		iav_privacy_mask_setup_ex_t	pm;
		iav_ca_warp_t	cawarp;
		iav_stream_privacy_mask_t	stream_pm;
		iav_stream_offset_t	stream_offset;
		overlay_insert_ex_t	stream_overlay;
	} arg;
} iav_video_proc_t;

typedef struct iav_apply_flag_s {
	int apply;
	u32 param;
} iav_apply_flag_t;

enum IAV_NALU_TYPE {
	NT_NON_IDR = 1,
	NT_IDR = 5,
	NT_SEI = 6,
	NT_SPS = 7,
	NT_PPS = 8,
	NT_AUD = 9,
};

typedef enum {
	/* stream config for H264 & MJPEG (0x000 ~ 0x0FF) */
	IAV_STMCFG_FPS = 0x00,
	IAV_STMCFG_OFFSET = 0x01,
	IAV_STMCFG_CHROMA = 0x02,
	IAV_STMCFG_FRAME_DROP = 0x03,
	IAV_STMCFG_NUM,
	IAV_STMCFG_FIRST = IAV_STMCFG_FPS,
	IAV_STMCFG_LAST = IAV_STMCFG_NUM,

	/* H264 config (0x100 ~ 0x1FF) */
	IAV_H264CFG_GOP = 0x100,
	IAV_H264CFG_BITRATE = 0x101,
	IAV_H264CFG_FORCE_IDR = 0x102,
	IAV_H264CFG_INTRA_MB_ROW = 0x103,
	IAV_H264CFG_QP_LIMIT = 0x104,
	IAV_H264CFG_ENC_PARAM = 0x105,
	IAV_H264CFG_QP_ROI = 0x106,
	IAV_H264CFG_Q_MATRIX = 0x107,
	IAV_H264CFG_PANIC = 0x108,
	IAV_H264CFG_NUM,
	IAV_H264CFG_FIRST = IAV_H264CFG_GOP,
	IAV_H264CFG_LAST = IAV_H264CFG_NUM,

	/* MJPEG config (0x200 ~ 0x2FF) */
	IAV_MJPEGCFG_QUALITY = 0x200,
	IAV_MJPEGCFG_NUM,
	IAV_MJPEGCFG_FIRST = IAV_MJPEGCFG_QUALITY,
	IAV_MJPEGCFG_LAST = IAV_MJPEGCFG_NUM,
} iav_streamcfg_id;

typedef struct iav_stream_cfg_s {
	iav_streamcfg_id	cid;
	union {
		iav_change_framerate_factor_ex_t fps;
		iav_stream_offset_t enc_offset;
		iav_chroma_format_info_ex_t chroma;
		iav_frame_drop_info_ex_t frame_drop;

		iav_change_gop_ex_t h_gop;
		iav_bitrate_info_ex_t h_rc;
		u32 h_force_idr_sid;
		iav_change_intra_mb_rows_ex_t h_intra_mb;
		iav_change_qp_limit_ex_t h_qp_limit;
		iav_h264_enc_param_ex_t h_enc_param;
		iav_qp_roi_matrix_ex_t h_qp_roi;
		iav_h264_q_matrix_ex_t h_qm;
		iav_panic_control_param_ex_t h_panic;

		u32 m_quality;
	} arg;
	u32	dsp_pts;
} iav_stream_cfg_t;


typedef struct iav_yuv_cap_s {
	u8	*y_addr;
	u8	*uv_addr;
	u32	width;
	u32	height;
	u32	pitch;
	u32	seqnum;
	IAV_YUV_DATA_FORMAT	format;
	u32	reserved;
	u64	mono_pts;
	u64	dsp_pts;
} iav_yuv_cap_t;

typedef struct iav_me1_cap_s {
	u8	*addr;
	u32	width;
	u32	height;
	u32	pitch;
	u32	seqnum;
	u32	reserved;
	u64	mono_pts;
	u64	dsp_pts;
} iav_me1_cap_t;

typedef struct iav_buf_cap_s {
	u32	flag;
	iav_yuv_cap_t		vca;
	iav_yuv_cap_t		yuv[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_me1_cap_t	me1[IAV_ENCODE_SOURCE_TOTAL_NUM];
} iav_buf_cap_t;

typedef enum {
	IAV_WARP_TRANSFORM = 0x00,
	IAV_WARP_COPY = 0x01,
	IAV_WARP_SUB_DPTZ = 0x02,
	IAV_WARP_OFFSET = 0x03,
	IAV_WARP_PROC_NUM,
	IAV_WARP_PROC_FIRST = IAV_WARP_TRANSFORM,
	IAV_WARP_PROC_LAST = IAV_WARP_PROC_NUM,
} iav_warp_proc_id;

typedef struct iav_warp_copy_s {
	u8	enable;
	u8	src_rid;
	u8	reserved[2];
	u16	dst_x;
	u16	dst_y;
} iav_warp_copy_t;

typedef struct iav_warp_sub_dptz_s {
	u32	sub_buf;
	iav_region_dptz_ex_t	dptz;
} iav_warp_sub_dptz_t;

typedef struct iav_warp_proc_s {
	iav_warp_proc_id	wid;
	u8	rid;		/* region id */
	u8	keep_dptz;
	u8	reserved[2];
	union {
		/* Input is in pre-main domain, output is in post-main domain */
		iav_warp_vector_ex_t transform;

		/* Input and out are in post-main domain */
		iav_warp_copy_t copy;

		/* Input is in post-main domain, output is in sub buffer domain */
		iav_warp_sub_dptz_t sub_dptz;

		/* Stream offset in post-main domain & sub buffer domain */
		iav_stream_offset_t offset;
	} arg;
} iav_warp_proc_t;

typedef enum {
	IAV_VCAP_FREEZE = 0x00,
	IAV_VCAP_PROC_NUM,
	IAV_VCAP_PROC_FIRST = IAV_VCAP_FREEZE,
	IAV_VCAP_PROC_LAST = IAV_VCAP_PROC_NUM,
} iav_vcap_proc_id;

typedef struct iav_vcap_proc_s {
	iav_vcap_proc_id	vid;
	union {
		u8	video_freeze;
	} arg;
} iav_vcap_proc_t;

typedef struct iav_mmap_dsp_partition_info_s {
	u32 id_map;
	u8	*addr[IAV_DSP_PARTITION_NUM];
	u32	length[IAV_DSP_PARTITION_NUM];
} iav_mmap_dsp_partition_info_t;

#endif	// __IAV_ENCODE_DRV_H__

