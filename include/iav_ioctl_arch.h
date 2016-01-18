
/*
 * iav_ioctl_arch.h
 *
 * History:
 *	2008/04/02 - [Oliver Li] created file
 *	2011/06/10 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_IOCTL_ARCH_H__
#define __IAV_IOCTL_ARCH_H__

/*
 * general APIs	- IAV_IOC_MAGIC
 */
enum {
	// For UCODE, from 0x00 to 0x0F
	IOC_GET_UCODE_INFO = 0x00,
	IOC_GET_UCODE_VERSION = 0x01,
	IOC_UPDATE_UCODE = 0x02,
	IOC_GET_DRIVER_INFO = 0x03,
	IOC_SUSPEND_DSP = 0x04,
	IOC_RESUME_DSP = 0x05,
	IOC_DUMP_IDSP_INFO = 0x06,
	IOC_DUMP_IDSP_CFG = 0x07,

	// For MMAP, from 0x10 to 0x18
	IOC_GET_MMAP_INFO = 0x10,
	IOC_MAP_LOGO = 0x11,
	IOC_MAP_DSP = 0x12,
	IOC_MAP_BSB = 0x13,
	IOC_MAP_BSB2 = 0x14,
	IOC_MAP_DECODE_BSB = 0x15,
	IOC_MAP_DECODE_BSB2 = 0x16,
	IOC_MAP_IMGPROC = 0x17,
	IOC_MAP_DSP_PARTITION = 0x18,

	// For UNMAP, from 0x19 to 0x1F
	IOC_UNMAP_LOGO = 0x19,
	IOC_UNMAP_DSP = 0x1A,
	IOC_UNMAP_BSB = 0x1B,
	IOC_UNMAP_DECODE_BSB = 0x1C,

	// For VOUT, from 0x20 to 0x3F
	IOC_VOUT_HALT = 0x20,
	IOC_VOUT_SELECT_DEV = 0x21,
	IOC_VOUT_CONFIGURE_SINK = 0x22,
	IOC_VOUT_SELECT_FB = 0x23,
	IOC_VOUT_ENABLE_VIDEO = 0x24,
	IOC_VOUT_FLIP_VIDEO = 0x25,
	IOC_VOUT_ROTATE_VIDEO = 0x26,
	IOC_VOUT_ENABLE_CSC = 0x27,
	IOC_VOUT_CHANGE_VIDEO_SIZE = 0x28,
	IOC_VOUT_CHANGE_VIDEO_OFFSET = 0x29,
	IOC_VOUT_FLIP_OSD = 0x2A,
	IOC_VOUT_ENABLE_OSD_RESCALER = 0x2B,
	IOC_VOUT_CHANGE_OSD_OFFSET = 0x2C,

	// For Still capture / YUV capture, from 0x40 to 0x4F
	IOC_INIT_STILL_CAPTURE = 0x40,
	IOC_START_STILL_CAPTURE = 0x41,
	IOC_STOP_STILL_CAPTURE = 0x42,
	IOC_STILL_CAPTURE_ADV = 0x43,
	IOC_LEAVE_STILL_CAPTURE = 0x44,
	IOC_INIT_STILL_PROC_MEM = 0x45,
	IOC_STILL_PROC_FROM_MEMORY = 0x46,
	IOC_STOP_STILL_PROC_MEM = 0x47,
	IOC_START_MOTION_CAPTURE = 0x48,
	IOC_READ_MOTION_BUFFER_INFO = 0x49,

	// For system, from 0x50 to 0x6F
	IOC_GET_NUM_CHANNEL = 0x50,
	IOC_SELECT_CHANNEL = 0x51,
	IOC_GET_STATE = 0x52,
	IOC_GET_STATE_INFO = 0x53,
	IOC_ENTER_IDLE = 0x54,
	IOC_READ_PREVIEW_INFO = 0x55,
	IOC_READ_RAW_INFO = 0x56,
	IOC_GDMA_COPY_EX = 0x57,

	// For IPCAM, from 0x70 to 0x8F
	IOC_GET_CHIP_ID_EX = 0x70,
	IOC_SET_VIN_CAPTURE_WINDOW = 0x71,
	IOC_GET_VIN_CAPTURE_WINDOW = 0x72,
	IOC_SET_SYSTEM_RESOURCE_LIMIT_EX = 0x73,
	IOC_GET_SYSTEM_RESOURCE_LIMIT_EX = 0x74,
	IOC_SET_SYSTEM_SETUP_INFO_EX = 0x75,
	IOC_GET_SYSTEM_SETUP_INFO_EX = 0x76,
	IOC_MAP_QP_ROI_MATRIX_EX = 0x77,
	IOC_UNMAP_QP_ROI_MATRIX_EX = 0x78,
	IOC_MAP_MOTION_VECTOR_EX = 0x79,
	IOC_UNMAP_MOTION_VECTOR_EX = 0x7A,
	IOC_MAP_QP_HISTOGRAM_EX = 0x7B,

	// For Misc settings, from 0x90 to 0xAF
	IOC_TEST = 0x90,
	IOC_LOG_SETUP = 0x91,
	IOC_CONFIG_L2CACHE = 0x92,
	IOC_DEBUG_SETUP = 0x93,

	// For reserved, from 0xB0 to 0xFF
};

#define _IAV_IO(IOCTL)				_IO(IAV_IOC_MAGIC, IOCTL)
#define _IAV_IOR(IOCTL, param)		_IOR(IAV_IOC_MAGIC, IOCTL, param)
#define _IAV_IOW(IOCTL, param)		_IOW(IAV_IOC_MAGIC, IOCTL, param)
#define _IAV_IOWR(IOCTL, param)		_IOWR(IAV_IOC_MAGIC, IOCTL, param)

#define IAV_IOC_GET_UCODE_INFO		_IAV_IOR(IOC_GET_UCODE_INFO, struct ucode_load_info_s *)
#define IAV_IOC_GET_UCODE_VERSION	_IAV_IOR(IOC_GET_UCODE_VERSION, struct ucode_version_s *)
#define IAV_IOC_UPDATE_UCODE		_IAV_IOW(IOC_UPDATE_UCODE, int)
#define IAV_IOC_GET_DRIVER_INFO		_IAV_IOR(IOC_GET_DRIVER_INFO, struct driver_version_s *)

#define IAV_IOC_SUSPEND_DSP		_IAV_IOW(IOC_SUSPEND_DSP, int)
#define IAV_IOC_RESUME_DSP		_IAV_IOW(IOC_RESUME_DSP, int)

#define IAV_IOC_GET_MMAP_INFO		_IAV_IOR(IOC_GET_MMAP_INFO, struct iav_mmap_s *)
#define IAV_IOC_MAP_LOGO			_IAV_IOR(IOC_MAP_LOGO, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_BSB				_IAV_IOR(IOC_MAP_BSB, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_IMGPROC			_IAV_IOR(IOC_MAP_IMGPROC, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_DECODE_BSB		_IAV_IOW(IOC_MAP_DECODE_BSB, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_DSP				_IAV_IOR(IOC_MAP_DSP, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_DSP_PARTITION				_IAV_IOR(IOC_MAP_DSP_PARTITION, struct iav_mmap_dsp_partition_info_s *)

#define IAV_IOC_VOUT_HALT			_IAV_IOW(IOC_VOUT_HALT, int)
#define IAV_IOC_VOUT_SELECT_DEV		_IAV_IOW(IOC_VOUT_SELECT_DEV, int)
#define IAV_IOC_VOUT_CONFIGURE_SINK	_IAV_IOW(IOC_VOUT_CONFIGURE_SINK, struct amba_video_sink_mode *)
#define IAV_IOC_VOUT_SELECT_FB		_IAV_IOW(IOC_VOUT_SELECT_FB, struct iav_vout_fb_sel_s *)
#define IAV_IOC_VOUT_ENABLE_VIDEO	_IAV_IOW(IOC_VOUT_ENABLE_VIDEO, struct iav_vout_enable_video_s *)
#define IAV_IOC_VOUT_FLIP_VIDEO		_IAV_IOW(IOC_VOUT_FLIP_VIDEO, struct iav_vout_flip_video_s *)
#define IAV_IOC_VOUT_ROTATE_VIDEO	_IAV_IOW(IOC_VOUT_ROTATE_VIDEO, struct iav_vout_rotate_video_s *)
#define IAV_IOC_VOUT_ENABLE_CSC		_IAV_IOW(IOC_VOUT_ENABLE_CSC, struct iav_vout_enable_csc_s *)
#define IAV_IOC_VOUT_CHANGE_VIDEO_SIZE		_IAV_IOW(IOC_VOUT_CHANGE_VIDEO_SIZE, struct iav_vout_change_video_size_s *)
#define IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET	_IAV_IOW(IOC_VOUT_CHANGE_VIDEO_OFFSET, struct iav_vout_change_video_offset_s *)
#define IAV_IOC_VOUT_FLIP_OSD		_IAV_IOW(IOC_VOUT_FLIP_OSD, struct iav_vout_flip_osd_s *)
#define IAV_IOC_VOUT_ENABLE_OSD_RESCALER	_IAV_IOW(IOC_VOUT_ENABLE_OSD_RESCALER, struct iav_vout_enable_osd_rescaler_s *)
#define IAV_IOC_VOUT_CHANGE_OSD_OFFSET		_IAV_IOW(IOC_VOUT_CHANGE_OSD_OFFSET, struct iav_vout_change_osd_offset_s *)

#define IAV_DEC_CHANNEL_FLAG		0x00010000
#define IAV_CHANNEL_MASK		0x0000ffff

#define IAV_ENC_CHANNEL(_channel_id)	((_channel_id) & IAV_CHANNEL_MASK)
#define IAV_DEC_CHANNEL(_channel_id)	(IAV_DEC_CHANNEL_FLAG | ((_channel_id) & IAV_CHANNEL_MASK))

#define IAV_IOC_GET_NUM_CHANNEL		_IAV_IOR(IOC_GET_NUM_CHANNEL, struct iav_num_channel_s *)
#define IAV_IOC_SELECT_CHANNEL		_IAV_IOW(IOC_SELECT_CHANNEL, int)

#define IAV_IOC_GET_STATE			_IAV_IOR(IOC_GET_STATE, int *)
#define IAV_IOC_GET_STATE_INFO		_IAV_IOR(IOC_GET_STATE_INFO, struct iav_state_info_s *)

#define IAV_IOC_ENTER_IDLE			_IAV_IOW(IOC_ENTER_IDLE, int)

#define IAV_IOC_INIT_STILL_CAPTURE	_IAV_IOW(IOC_INIT_STILL_CAPTURE, struct iav_still_init_info *)
#define IAV_IOC_START_STILL_CAPTURE	_IAV_IOW(IOC_START_STILL_CAPTURE, struct iav_still_cap_info *)
#define IAV_IOC_STOP_STILL_CAPTURE	_IAV_IO(IOC_STOP_STILL_CAPTURE)
#define IAV_IOC_STILL_CAPTURE_ADV	_IAV_IO(IOC_STILL_CAPTURE_ADV)
#define IAV_IOC_LEAVE_STILL_CAPTURE	_IAV_IO(IOC_LEAVE_STILL_CAPTURE)

#define IAV_IOC_INIT_STILL_PROC_MEM	_IAV_IOW(IOC_INIT_STILL_PROC_MEM, struct iav_init_still_mem_info *)
#define IAV_IOC_STILL_PROC_FROM_MEMORY _IAV_IOW(IOC_STILL_PROC_FROM_MEMORY,struct iav_still_proc_mem_info*)
#define IAV_IOC_STOP_STILL_PROC_MEM _IAV_IO(IOC_STOP_STILL_PROC_MEM)

#define IAV_IOC_READ_PREVIEW_INFO	_IAV_IOR(IOC_READ_PREVIEW_INFO, struct iav_preview_info_s *)
#define IAV_IOC_READ_RAW_INFO		_IAV_IOWR(IOC_READ_RAW_INFO, struct iav_raw_info_s *)

#define IAV_IOC_DUMP_IDSP_INFO		_IAV_IOW(IOC_DUMP_IDSP_INFO, struct iav_dump_idsp_info_s*)
#define IAV_IOC_DSP_DUMP_CFG        _IOW(IAV_IOC_MAGIC,IOC_DUMP_IDSP_CFG,struct iav_idsp_config_info_s*)

#define IAV_IOC_MAP_BSB2			_IAV_IOR(IOC_MAP_BSB2, struct iav_mmap_info_s *)
#define IAV_IOC_MAP_DECODE_BSB2		_IAV_IOR(IOC_MAP_DECODE_BSB2, struct iav_mmap_info_s *)

#define IAV_IOC_UNMAP_BSB			_IAV_IO(IOC_UNMAP_BSB)
#define IAV_IOC_UNMAP_DECODE_BSB	_IAV_IO(IOC_UNMAP_DECODE_BSB)
#define IAV_IOC_UNMAP_LOGO			_IAV_IO(IOC_UNMAP_LOGO)
#define IAV_IOC_UNMAP_DSP			_IAV_IO(IOC_UNMAP_DSP)

#define IAV_IOC_TEST				_IAV_IO(IOC_TEST)
#define IAV_IOC_LOG_SETUP			_IAV_IOW(IOC_LOG_SETUP, u32)
#define IAV_IOC_DEBUG_SETUP			_IAV_IOW(IOC_DEBUG_SETUP, struct iav_debug_setup_s *)

#define IAV_IOC_START_MOTION_CAPTURE	_IAV_IOW(IOC_START_MOTION_CAPTURE, int)
#define IAV_IOC_READ_MOTION_BUFFER_INFO	_IAV_IOR(IOC_READ_MOTION_BUFFER_INFO, struct iav_motion_buffer_info_s *)

#define IAV_IOC_SET_VIN_CAPTURE_WINDOW		_IAV_IOW(IOC_SET_VIN_CAPTURE_WINDOW, struct iav_rect_ex_s *)
#define IAV_IOC_GET_VIN_CAPTURE_WINDOW		_IAV_IOR(IOC_GET_VIN_CAPTURE_WINDOW, struct iav_rect_ex_s *)

#define IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX		_IAV_IOW(IOC_SET_SYSTEM_RESOURCE_LIMIT_EX	, struct iav_system_resource_setup_ex_s *)
#define IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX	_IAV_IOWR(IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, struct iav_system_resource_setup_ex_s *)

#define IAV_IOC_SET_SYSTEM_SETUP_INFO_EX		_IAV_IOW(IOC_SET_SYSTEM_SETUP_INFO_EX, struct iav_system_setup_info_ex_s *)
#define IAV_IOC_GET_SYSTEM_SETUP_INFO_EX		_IAV_IOR(IOC_GET_SYSTEM_SETUP_INFO_EX, struct iav_system_setup_info_ex_s *)

#define IAV_IOC_GET_CHIP_ID_EX	_IAV_IOR(IOC_GET_CHIP_ID_EX, int *)

/* map QP matrix from kernel space */
#define IAV_IOC_MAP_QP_ROI_MATRIX_EX	_IAV_IOR(IOC_MAP_QP_ROI_MATRIX_EX, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_QP_ROI_MATRIX_EX	_IAV_IO(IOC_UNMAP_QP_ROI_MATRIX_EX)

/* map motion vector from kernel space */
#define IAV_IOC_MAP_MV_EX		_IAV_IOR(IOC_MAP_MOTION_VECTOR_EX, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_MV_EX	_IAV_IO(IOC_UNMAP_MOTION_VECTOR_EX)

/* map QP histogram from kernel space */
#define IAV_IOC_MAP_QP_HIST_EX	_IAV_IOR(IOC_MAP_QP_HISTOGRAM_EX, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_QP_HIST_EX	_IAV_IO(IOC_MAP_QP_HISTOGRAM_EX)

#define IAV_IOC_CONFIG_L2CACHE		_IAV_IOW(IOC_CONFIG_L2CACHE, struct iav_cache_config_s*)

/* gdma copy */
#define IAV_IOC_GDMA_COPY_EX	_IAV_IOW(IOC_GDMA_COPY_EX, struct iav_gdma_copy_ex_s*)

/*
 * overlay insertion APIs	- IAV_IOC_OVERLAY_MAGIC : 'o'
 */
enum {
	// For overlay blending, from 0x00 to 0x1F
	IOC_MAP_OVERLAY = 0x00,
	IOC_UNMAP_OVERLAY = 0x01,
	IOC_MAP_ALPHA_MASK = 0x02,
	IOC_OVERLAY_INSERT = 0x03,
	IOC_OVERLAY_INSERT_EX = 0x04,
	IOC_GET_OVERLAY_INSERT_EX = 0x05,
	IOC_MAP_USER = 0x6,
	IOC_UNMAP_USER = 0x7,

	// For privacy mask, from 0x20 to 0x3F
	IOC_MAP_PRIVACY_MASK_EX = 0x20,
	IOC_UNMAP_PRIVACY_MASK_EX = 0x21,
	IOC_SET_PRIVACY_MASK_EX = 0x22,
	IOC_GET_PRIVACY_MASK_EX = 0x23,
	IOC_GET_PRIVACY_MASK_INFO_EX = 0x24,
	IOC_GET_LOCAL_EXPOSURE_EX = 0x25,
	IOC_SET_LOCAL_EXPOSURE_EX = 0x26,
};

#define _OVERLAY_IO(IOCTL)				_IO(IAV_IOC_OVERLAY_MAGIC, IOCTL)
#define _OVERLAY_IOR(IOCTL, param)		_IOR(IAV_IOC_OVERLAY_MAGIC, IOCTL, param)
#define _OVERLAY_IOW(IOCTL, param)		_IOW(IAV_IOC_OVERLAY_MAGIC, IOCTL, param)
#define _OVERLAY_IOWR(IOCTL, param)	_IOWR(IAV_IOC_OVERLAY_MAGIC, IOCTL, param)
#define IAV_IOC_MAP_USER				_OVERLAY_IOR(IOC_MAP_USER, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_USER				_OVERLAY_IO(IOC_UNMAP_USER)

#define IAV_IOC_MAP_OVERLAY				_OVERLAY_IOR(IOC_MAP_OVERLAY, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_OVERLAY			_OVERLAY_IO(IOC_UNMAP_OVERLAY)
#define IAV_IOC_MAP_ALPHA_MASK			_OVERLAY_IOR(IOC_MAP_ALPHA_MASK, struct iav_mmap_info_s *)
#define IAV_IOC_OVERLAY_INSERT			_OVERLAY_IOW(IOC_OVERLAY_INSERT, struct overlay_insert_s *)
#define IAV_IOC_OVERLAY_INSERT_EX		_OVERLAY_IOW(IOC_OVERLAY_INSERT_EX, struct overlay_insert_ex_s *)
#define IAV_IOC_GET_OVERLAY_INSERT_EX		_OVERLAY_IOWR(IOC_GET_OVERLAY_INSERT_EX, struct overlay_insert_ex_s *)

//privacy mask IOCTLs
#define IAV_IOC_MAP_PRIVACY_MASK_EX		_OVERLAY_IOR(IOC_MAP_PRIVACY_MASK_EX, struct iav_mmap_info_s *)
#define IAV_IOC_UNMAP_PRIVACY_MASK_EX	_OVERLAY_IO(IOC_UNMAP_PRIVACY_MASK_EX)
#define IAV_IOC_SET_PRIVACY_MASK_EX		_OVERLAY_IOW(IOC_SET_PRIVACY_MASK_EX, struct iav_privacy_mask_setup_ex_s *)
#define IAV_IOC_GET_PRIVACY_MASK_EX		_OVERLAY_IOR(IOC_GET_PRIVACY_MASK_EX, struct iav_privacy_mask_setup_ex_s *)
#define IAV_IOC_GET_PRIVACY_MASK_INFO_EX		_OVERLAY_IOR(IOC_GET_PRIVACY_MASK_INFO_EX, struct iav_privacy_mask_info_ex_s *)
#define IAV_IOC_GET_LOCAL_EXPOSURE_EX	_OVERLAY_IOWR(IOC_GET_LOCAL_EXPOSURE_EX, struct iav_local_exposure_ex_s *)
#define IAV_IOC_SET_LOCAL_EXPOSURE_EX	_OVERLAY_IOW(IOC_SET_LOCAL_EXPOSURE_EX, struct iav_local_exposure_ex_s *)

#endif // __IAV_IOCTL_ARCH_H__


