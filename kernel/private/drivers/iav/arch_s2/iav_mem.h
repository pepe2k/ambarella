/*
 * iav_mem.h
 *
 * History:
 *	2011/11/11 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_MEM_H
#define __IAV_MEM_H

#define	MERGE_BITS_FIFO

typedef enum IAV_BITS_DESC_TYPE_S
{
	IAV_BITS_MJPEG = 0,
	IAV_BITS_H264 = 1,
	IAV_BITS_BSB = 2,
	IAV_BITS_TYPE_NUM = 2,
	IAV_BITS_TYPE_INVALID = IAV_BITS_TYPE_NUM + 1,
} IAV_BITS_DESC_TYPE;

struct iav_mem_block
{
	u32		phys_start;		// physical start address
	u32		phys_end;		// physical end address
	u8		*kernel_start;	// kernal virtual start address
	u8		*kernel_end;	// kernal virtual end address (exclusive)
	u32		size;			// size in bytes
};

struct iav_bits_desc
{
	struct dsp_bits_info_s	*start;
	struct dsp_bits_info_s	*end;
	struct dsp_extend_bits_info_s	*ext_start;
	struct dsp_extend_bits_info_s	*ext_end;
	struct dsp_bits_partition_info_s	*bp_start;
	struct dsp_bits_partition_info_s	*bp_end;
};

struct iav_stat_desc
{
	struct dsp_enc_stat_info_s * start;
	struct dsp_enc_stat_info_s * end;
	struct dsp_extend_enc_stat_info_s	*ext_start;
	struct dsp_extend_enc_stat_info_s	*ext_end;
};

struct iav_mem_info
{
	struct iav_mem_block	bsb;		// bitstream buffer
	struct iav_mem_block	dsp;		// DSP memory
	struct iav_mem_block	mv;			// motion vector buffer
	struct iav_mem_block	fb;			// frame buffer
	struct iav_mem_block	dsp_partition[IAV_DSP_PARTITION_NUM];		//DSP partitions

	struct iav_mem_block	user;		// user memory for EFM DMA
	struct iav_mem_block	overlay;		// overlay memory for OSD insert
	struct iav_mem_block	privacymask;	// privacy mask memory
	struct iav_mem_block	qp_matrix;	// QP ROI matrix per MB for streams
	struct iav_mem_block	qp_hist;		// QP histogram for streams
	struct iav_mem_block	q_matrix;	// Q matrix for scaling list of high profile
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	struct iav_mem_block	cmd_sync;	//cmd mem for sync frame in streams
#endif
	struct iav_mem_block	jpeg_qt;		// JPEG QT matrix
	struct iav_mem_block	yuv_warp;	// Warp vector table
	struct iav_mem_block	enc_cfg;	// Encode stream config
	struct iav_mem_block	imgproc;	// IMGPROC buffer
	struct iav_mem_block	local_expo;	// Local Exposure
	struct iav_mem_block	ca_warp;	// Chroma Aberration

	struct iav_bits_desc	bs_desc;	// bitstream descriptors
	struct iav_stat_desc	stat_desc;	// encoder statistics descriptors
};



/******************************************
 *
 *		External API
 *
 ******************************************/

int iav_mem_init(void);
void iav_mem_cleanup(void);
int iav_get_mem_block(int mmap_type, struct iav_mem_block ** block);
int iav_set_mem_block(int mmap_type, struct iav_mem_block * block);
int iav_get_bits_desc(struct iav_bits_desc ** desc);
int iav_get_stat_desc(struct iav_stat_desc ** desc);
void iav_set_dsp_partition(int partition, u32 phys_start, u32 size);
void iav_get_dsp_partition(int partition, u32 *phys_start, u32 *size);
int partition_id_to_index(int id);
void iav_reset_dsp_partition(void);

int get_total_dram_size(u32 *total_size);

int mem_alloc_enc_cfg(u8 **ptr, int *alloc_size);
int mem_alloc_yuv_warp(u8 ** ptr, int *alloc_size);
int mem_alloc_jpeg_qt_matrix(u8 **ptr, int *alloc_size);
int mem_alloc_local_exposure(u8 **ptr, int *alloc_size);
int mem_alloc_ca_warp(u8 **ptr, int *alloc_size);
int mem_alloc_q_matrix(u8 **ptr, int *alloc_size);


void *iav_create_mmap(struct iav_context_s *context, unsigned long phys_start, unsigned long virt_start,
	unsigned long size, unsigned long prot,
	int noncached, struct dsp_vm_space_s *vms);
void iav_destroy_mmap(struct iav_context_s *context, unsigned long user_start, unsigned long size);

int __iav_do_map_phys(struct iav_context_s *context, struct vm_area_struct *vma);
int __iav_do_map_phys_2times(struct iav_context_s *context, struct vm_area_struct *vma);

int iav_gdma_copy(struct iav_context_s * context, iav_gdma_copy_ex_t __user * arg);

#endif	//  __IAV_MEM_H

