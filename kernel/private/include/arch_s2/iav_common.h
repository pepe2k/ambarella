/*
 * iav_common.h
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
 *	2012/04/13 - [Jian Tang] modified file for S2
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_COMMON_H__
#define __IAV_COMMON_H__

#define IAV_NUM_VOUT	(2)
#define IAV_NUM_VIN		(2)

typedef enum {
	IAV_HDR_NONE = 0,
	IAV_HDR_FRM_INTERLEAVED = 1,
	IAV_HDR_LINE_INTERLEAVED = 2,
	IAV_HDR_TOTAL_NUM,
} IAV_HDR_MODE;

struct iav_global_info
{
	// iav_state_t
	int	state;
	int	dsp_booted;
	int	ucode_loaded;
	int	dsp_owner; // 0: Cortex; 1: ARM11

	// vout related
	struct amba_iav_vout_info	*pvoutinfo[IAV_NUM_VOUT];

	// vin related
	struct amba_iav_vin_info	*pvininfo;

	// memory map related
	int	mmap_type;
	u8	called_by_ioc;
	u8	noncached;
	u8	map2times;
	u8	duplex_ref_cnt;//for decouple

	unsigned long		mmap_phys_start;
	unsigned long		mmap_page_offset;
	unsigned long		mmap_size;
	unsigned long		mmap_prot;

	u8			*mmap_user_start;
	struct dsp_vm_space_s	*mmap_vms;
	unsigned long		mmap_virt_start;

	// 3A pipeline related, export for 3A usage
	u8	high_mega_pixel_enable : 1;
	u8	hdr_mode : 2;
	u8	vin_num : 3;
	u8	reserved0 : 2;
	u8	reserved1;
};


#define IAV_MAP2	(1 << 0)

struct iav_addr_map {
	u8 *user_start;	// user space start address
	u8 *user_end;	// user space end address (exclusive)
	u32 ref_count;	// ++ when map; -- when unmap
	u32 flags;
};

enum {
	IAV_MODE_UDEC = (1 << 0),
	IAV_MODE_DUPLEX = (1 << 1),

	// for duplex decouple
	IAV_OWNER_DUPLEX_ENCODING = (1 << 4),
	IAV_OWNER_DUPLEX_DECODING = (1 << 5),

	IAV_MODE_UDEC_CAPTURE = (1 << 6),
};

// 1 raw + 4 YUV + 4 ME1 + 1 POST Main + 2 EFM == 12
#define DSP_PARTITION_NUM (12)
#define DEFAULT_ALL_PARTITION (0x3FF)
typedef struct iav_context_s
{
	void 			*file;

	struct iav_addr_map	bsb;
	struct iav_addr_map	dsp;
	struct iav_addr_map	fb;
	struct iav_addr_map	user;
	struct iav_addr_map	overlay;
	struct iav_addr_map	bpc_map;
	struct iav_addr_map	privacymask;
	struct iav_addr_map	qp_matrix;
	struct iav_addr_map	qp_hist;
	struct iav_addr_map	mv;
	struct iav_addr_map	imgproc;
	struct iav_addr_map	dsp_partition[DSP_PARTITION_NUM];		//DSP partitions

	int			active_src_id;
	int			active_channel_id;

	struct mutex		*mutex;
	struct iav_global_info	*g_info;

	unsigned long		mode_flags;	// user in a mode except IDLE
	u8			vout_flag[IAV_NUM_VOUT];	// vout resource
	u8			need_issue_reset_hdmi;//hack code for RESET HDMI, before udec playback in LCD only mode
	u8			reserved;
	u32			udec_flags;
	void			*udec_mapping;
	u32			enc_flags;
	u32			duplex_flags;
	u32			dsp_partition_id_map;
	u32			curr_partition;
} iav_context_t;

#define set_mode_flag(_context, _flag) \
	do { (_context)->mode_flags |= (_flag); } while (0)

#define clear_mode_flag(_context, _flag) \
	do { (_context)->mode_flags &= ~(_flag); } while (0)

#define is_mode_flag_set(_context, _flag) \
	((_context)->mode_flags & (_flag))

#define	CACHE_LINE_SIZE		(32)
void clean_cache_aligned(u8 *start, unsigned long size);

void get_eis_info_ex(void * eis_info_addr);
void set_eis_info_ex(void * eis_info_addr);



#endif	// __IAV_COMMON_H__

