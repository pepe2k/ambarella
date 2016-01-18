
/*
 * iav_struct_arch.h
 *
 * History:
 *	2008/04/02 - [Oliver Li] created file
 *	2011/06/10 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_STRUCT_ARCH_H__
#define __IAV_STRUCT_ARCH_H__


#define UCODE_LOAD_ITEM_MAX		4

typedef enum {
	IAV_STATE_IDLE = 0,
	IAV_STATE_PREVIEW = 1,
	IAV_STATE_ENCODING = 2,
	IAV_STATE_STILL_CAPTURE = 3,
	IAV_STATE_DECODING = 4,
	IAV_STATE_TRANSCODING = 5,
	IAV_STATE_DUPLEX = 6,
	IAV_STATE_EXITING_PREVIEW = 7,
	IAV_STATE_INIT = 0xFF,
} IAV_STATE;

typedef enum {
	IAV_IOCTL_BLOCK = 0,
	IAV_IOCTL_NONBLOCK = (1 << 0),
} IAV_IOCTL_FLAG;

#define	IAV_BIT(x)			(1 << (x))
typedef enum {
	IAV_DEBUG_CHECK_DISABLE = IAV_BIT(0),
	IAV_DEBUG_PRINT_ENABLE = IAV_BIT(1),
} IAV_DEBUG_MASK;

typedef struct ucode_load_item_s {
	unsigned long		addr_offset;
	char			filename[20];
} ucode_load_item_t;

typedef struct ucode_load_info_s {
	unsigned long		map_size;
	int			nr_item;
	ucode_load_item_t	items[UCODE_LOAD_ITEM_MAX];
} ucode_load_info_t;

typedef struct ucode_version_s {
	u16	year;
	u8	month;
	u8	day;
	u32	edition_num;
	u32	edition_ver;
} ucode_version_t;

typedef struct driver_version_s {
	int	arch;
	int	model;
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
	u32	api_version;
	u32	idsp_version;
	u32	reserved[2];
} driver_version_t;

typedef struct iav_dsp_setup_s {
	int	cmd;
	u32	args[8];
} iav_dsp_setup_t;

typedef struct iav_cache_config_s {
	u8 disable_l2_in_idle_mode;
	u8 disable_l2_in_non_idle_mode;
	u8 try_512k_in_udec_mode;
	u8 reserved0;
} iav_cache_config_t;

typedef struct iav_resolution_s {
	u16	width;
	u16	height;
	u32	max_framerate;
} iav_resolution_t;

typedef struct iav_resolution2_s {
	iav_resolution_t	resolution_main;
	iav_resolution_t	resolution_secondary;
} iav_resolution2_t;

typedef struct iav_mem_info_s {
	u32	bsb_size;
	u32	dsp_size;
} iav_mem_info_t;

typedef struct iav_mmap_s {
	u8	*bsb_addr;
	u32	bsb_size;
	u8	*bsb_dec_addr;
	u32	bsb_dec_size;
	u8	*dsp_addr;
	u32	dsp_size;
} iav_mmap_t;

typedef struct iav_mmap_info_s {
	u8	*addr;
	u32	length;
} iav_mmap_info_t;

typedef struct iav_num_channel_s {
	int	num_enc_channels;
	int	num_dec_channels;
} iav_num_channel_t;

typedef struct overlay_insert_area_s {
	u16	width;
	u16	pitch;
	u16	height;
	u16	start_x;
	u16	start_y;
	u16	enable;
	u8	*y_addr;
	u8	*uv_addr;
	u8	*alpha_addr;
} overlay_insert_area_t;

typedef struct overlay_insert_item_s {
	overlay_insert_area_t	area0;
	overlay_insert_area_t	area1;
	overlay_insert_area_t	area2;
} overlay_insert_item_t;


#define OVERLAY_ENABLE_MAIN		1
#define OVERLAY_ENABLE_SECOND		2

typedef struct overlay_insert_s {
	u32	enable;
	overlay_insert_item_t	h264;
	overlay_insert_item_t	secondary;
} overlay_insert_t;

typedef struct iav_state_info_s {
	int	state;
	int	encoder_state;	// A3 DVR
	int	decoder_state;	// A3 DVR
	int	channel_state;	// A3 DVR
	int	vout_irq_count;
	int	vin_irq_count;
	int	vdsp_irq_count;
	int	dsp_op_mode;
	int	dsp_encode_state;
	int	dsp_encode_mode;
	int	dsp_decode_state;
	int	decode_state;
	int	encode_timecode;
	int	encode_pts;
	int	vout1_irq_count;
} iav_state_info_t;

typedef struct iav_vout_fb_sel_s {
	int		vout_id;
	int		fb_id;
} iav_vout_fb_sel_t;

typedef struct iav_vout_enable_video_s {
	int		vout_id;
	int		video_en;
} iav_vout_enable_video_t;

typedef struct iav_vout_flip_video_s {
	int		vout_id;
	int		flip;
} iav_vout_flip_video_t;

typedef struct iav_vout_rotate_video_s {
	int		vout_id;
	int		rotate;
} iav_vout_rotate_video_t;

typedef struct iav_vout_enable_csc_s {
	int		vout_id;
	int		csc_en;
} iav_vout_enable_csc_t;

typedef struct iav_vout_change_video_size_s {
	int		vout_id;
	int		width;
	int		height;
} iav_vout_change_video_size_t;

typedef struct iav_vout_change_video_offset_s {
	int		vout_id;
	int		specified;
	int		offset_x;
	int		offset_y;
} iav_vout_change_video_offset_t;

typedef struct iav_vout_flip_osd_s {
	int		vout_id;
	int		flip;
} iav_vout_flip_osd_t;

typedef struct iav_vout_enable_osd_rescaler_s {
	int		vout_id;
	int		enable;
	int		width;
	int		height;
} iav_vout_enable_osd_rescaler_t;

typedef struct iav_vout_change_osd_offset_s {
	int		vout_id;
	int		specified;
	int		offset_x;
	int		offset_y;
} iav_vout_change_osd_offset_t;

typedef struct iav_preview_info_s {
	s32		interval;	// input
	u8		*y_addr;
	u8		*uv_addr;
	u32		pitch;
	u32		seqnum;
	u32		PTS;
} iav_preview_info_t;

typedef struct iav_yuv_buffer_info_ex_s{
	u32		source;
	u8		*y_addr;
	u8		*uv_addr;
	u32		width;
	u32		height;
	u32		pitch;
	u32		seqnum;
	IAV_YUV_DATA_FORMAT	format;
	u32		flag;
	u32		reserved;
	u64		mono_pts;
	u64		dsp_pts;
} iav_yuv_buffer_info_ex_t;

typedef struct iav_motion_buffer_info_s {
	u8		*y_addr;
	u32		pitch;
	u32		width;
	u32		height;
	u32		seqnum;
	u32		PTS;
} iav_motion_buffer_info_t;

typedef struct iav_raw_info_s {
	u8		*raw_addr;
	u8		bayer_pattern;
	u8		bit_resolution;
	u8		sensor_id;
	u8		reserved;
	u32		pitch;
	u32		width;
	u32		height;
	u32		flag;
	u64		raw_dsp_pts;
} iav_raw_info_t;

typedef struct iav_dump_idsp_info_s {
	u32		mode;	//0 -disable, 1-enable, 2-get data
	void		*pBuffer;
} iav_dump_idsp_info_t;

typedef struct iav_idsp_config_info_s {
	u8*		addr;
	u32		addr_long;
	u32		id_section;
} iav_idsp_config_info_t;

typedef struct load_ucode_target_s {
	struct {
		u32 code_addr;
		u32 memd_addr;
		u32 sub0_addr;
		u32 sub1_addr;
		u32 dsp_binary_data_addr;
	} dsp;
	struct {
		u32 code_addr;
		u32 data_addr;
	} audio;
} load_ucode_target_t;

typedef struct iav_debug_setup_s {
	u8	enable;
	u8	reserved[3];
	u32	flag;
} iav_debug_setup_t;

#endif // __IAV_STRUCT_ARCH_H__

