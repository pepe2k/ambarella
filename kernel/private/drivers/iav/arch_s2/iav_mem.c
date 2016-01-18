/*
 * iav_mem.c
 *
 * History:
 *	2011/11/11 - [Jian Tang] created file
 *
 * Copyright (C) 2011-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"

#include "utils.h"
#include "iav_priv.h"
#include "iav_mem.h"

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_imgproc.h"
#endif


#define HIGH_MEM_MASK		(0xF0000000)
#define MEM_MASK_8Gb		(0xE)

static u32 G_total_dram_size = IAV_DRAM_SIZE_4Gb;
static struct iav_mem_info G_iav_mem;

static inline void init_mem_block(struct iav_mem_block *mem_block,
	u32 phys_start, u8 *virt_start, u32 size)
{
	mem_block->phys_start = phys_start;
	mem_block->phys_end = phys_start + size;
	mem_block->kernel_start = virt_start;
	mem_block->kernel_end = virt_start + size;
	mem_block->size = size;
}

static inline int get_mem_block_ptr(int mmap_type,
	struct iav_mem_block ** mem_block_addr)
{
	struct iav_mem_block * mem_block = NULL;
	switch (mmap_type) {
	case IAV_MMAP_BSB:
	case IAV_MMAP_BSB2:
		mem_block = &G_iav_mem.bsb;
		break;
	case IAV_MMAP_DSP:
		mem_block = &G_iav_mem.dsp;
		break;
	case IAV_MMAP_FB:
		mem_block = &G_iav_mem.fb;
		break;
	case IAV_MMAP_USER:
		mem_block = &G_iav_mem.user;
		break;
	case IAV_MMAP_OVERLAY:
		mem_block = &G_iav_mem.overlay;
		break;
	case IAV_MMAP_PRIVACYMASK:
		mem_block = &G_iav_mem.privacymask;
		break;
	case IAV_MMAP_QP_MATRIX:
		mem_block = &G_iav_mem.qp_matrix;
		break;
	case IAV_MMAP_MV:
		mem_block = &G_iav_mem.mv;
		break;
	case IAV_MMAP_IMGPROC:
		mem_block = &G_iav_mem.imgproc;
		break;
	case IAV_MMAP_LOCAL_EXPO:
		mem_block = &G_iav_mem.local_expo;
		break;
	case IAV_MMAP_QP_HIST:
		mem_block = &G_iav_mem.qp_hist;
		break;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	case IAV_MMAP_CMD_SYNC:
		mem_block = &G_iav_mem.cmd_sync;
		break;
#endif
	default:
		return -1;
		break;
	}

	*mem_block_addr = mem_block;
	return 0;
}

static void free_all_mem(void)
{
	u8 * ptr = NULL;

	ptr = G_iav_mem.mv.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.mv.kernel_start = NULL;
	}
	ptr = G_iav_mem.qp_hist.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.qp_hist.kernel_start = NULL;
	}
	ptr = G_iav_mem.q_matrix.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.q_matrix.kernel_start = NULL;
	}
	ptr = G_iav_mem.jpeg_qt.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.jpeg_qt.kernel_start = NULL;
	}
	ptr = G_iav_mem.yuv_warp.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.yuv_warp.kernel_start = NULL;
	}
	ptr = G_iav_mem.enc_cfg.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.enc_cfg.kernel_start = NULL;
	}
	ptr = G_iav_mem.local_expo.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.local_expo.kernel_start = NULL;
	}
	ptr = G_iav_mem.ca_warp.kernel_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.ca_warp.kernel_start = NULL;
	}

	ptr = (u8 *)G_iav_mem.bs_desc.start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.bs_desc.start = NULL;
	}
	ptr = (u8 *)G_iav_mem.bs_desc.ext_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.bs_desc.ext_start = NULL;
	}
	ptr = (u8 *)G_iav_mem.bs_desc.bp_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.bs_desc.bp_start = NULL;
	}
	ptr = (u8 *)G_iav_mem.stat_desc.start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.stat_desc.start = NULL;
	}
	ptr = (u8 *)G_iav_mem.stat_desc.ext_start;
	if (ptr) {
		kfree(ptr);
		G_iav_mem.stat_desc.ext_start = NULL;
	}
	iav_printk("Free all allocated IAV memory !\n");
}

static void iav_mem_ddrc_config(void)
{
	u32 ahb_virt_addr = get_ambarella_ahb_virt();
	u32 apb_virt_addr = get_ambarella_apb_virt();

	amba_writel(ahb_virt_addr + 0x00004044, 0x07);
	amba_writel(ahb_virt_addr + 0x00004048, 0x00000200);
	amba_writel(ahb_virt_addr + 0x0000404C, 0x0);
	amba_writel(ahb_virt_addr + 0x00004050, 0x0);
	amba_writel(ahb_virt_addr + 0x00004054, 0x0);
	amba_writel(ahb_virt_addr + 0x00004058, 0x0);
	amba_writel(ahb_virt_addr + 0x0000405C, 0x0);
	amba_writel(ahb_virt_addr + 0x00004060, 0x0);
	amba_writel(ahb_virt_addr + 0x00004064, 0x0);
	amba_writel(ahb_virt_addr + 0x00004068, 0x0);
	amba_writel(ahb_virt_addr + 0x0000406C, 0x0);
	amba_writel(ahb_virt_addr + 0x00004070, 0x0);
	amba_writel(ahb_virt_addr + 0x00004074, 0x0);
	amba_writel(ahb_virt_addr + 0x00004078, 0x0);
	amba_writel(ahb_virt_addr + 0x0000407C, 0x0);
	amba_writel(ahb_virt_addr + 0x00004080, 0x0);
	amba_writel(ahb_virt_addr + 0x00004084, 0x0);
	amba_writel(ahb_virt_addr + 0x00004088, 0x0);
	amba_writel(ahb_virt_addr + 0x0000408C, 0x0);
	amba_writel(ahb_virt_addr + 0x00004090, 0x0);
	amba_writel(ahb_virt_addr + 0x00004094, 0x0);
	amba_writel(ahb_virt_addr + 0x00004098, 0x0);
	amba_writel(ahb_virt_addr + 0x0000409C, 0x0);
	amba_writel(ahb_virt_addr + 0x000040A0, 0x0);
	amba_writel(ahb_virt_addr + 0x000040A4, 0x0);
	amba_writel(ahb_virt_addr + 0x000040A8, 0x0);
	amba_writel(ahb_virt_addr + 0x000040AC, 0x0);
	amba_writel(ahb_virt_addr + 0x000040B0, 0x0);
	amba_writel(ahb_virt_addr + 0x000040B4, 0x0);
	amba_writel(ahb_virt_addr + 0x000040B8, 0x0);
	amba_writel(ahb_virt_addr + 0x000040BC, 0x0);
	amba_writel(ahb_virt_addr + 0x000040C0, 0x0);
	amba_writel(ahb_virt_addr + 0x000040C4, 0x10002000);

	// For NAND BCH, already done in BST
	amba_writel(ahb_virt_addr + 0x000041C0, 0x00000001);

	amba_writel(apb_virt_addr + 0x0015000C, 0x07);
}

int iav_mem_init(void)
{
	u32 phys_addr = 0;
	u8 *ptr = NULL;
	int size = 0;

	if (BITSTREAM_MEM_SIZE > BITSTREAM_MEM_MAX_SIZE) {
		iav_error("BSB size [%dMB] should not be greater than [%dMB].\n",
			BITSTREAM_MEM_SIZE >> 20, BITSTREAM_MEM_MAX_SIZE >> 20);
		return -1;
	}

	/* Init MEM arbiter */
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	size = USER_MEM_SIZE + OVERLAY_MEM_SIZE + BITSTREAM_MEM_SIZE +
		IMGPROC_MEM_SIZE + PRIVACYMASK_MEM_SIZE_MIN +
		STREAM_QP_MATRIX_MEM_TOTAL_SIZE * DEFAULT_TOGGLED_ENC_CMD_NUM +
		VCAP_ENC_CMD_MEM_TOTAL_SIZE;
#else
	size = USER_MEM_SIZE + OVERLAY_MEM_SIZE + BITSTREAM_MEM_SIZE + IMGPROC_MEM_SIZE +
		STREAM_QP_MATRIX_MEM_TOTAL_SIZE + PRIVACYMASK_MEM_SIZE_MIN;
#endif
	if (get_ambarella_bsbmem_size() < size) {
		iav_error("Too small BSB [%d MB]. It need to be greater than [%d MB]."
			" Please change it in menuconfig.\n",
			(get_ambarella_bsbmem_size() >> 20), (size >> 20));
		return -1;
	}

	//rasie DSP DRAM arbitration priority
	iav_mem_ddrc_config();
	iav_printk("set mem arbiter to DSP higher over ARM\n");

	memset(&G_iav_mem, 0, sizeof(G_iav_mem));

	/* DSP buffer */
	phys_addr = get_ambarella_dspmem_phys();
	init_mem_block(&G_iav_mem.dsp,
		phys_addr,
		(u8*)get_ambarella_dspmem_virt(),
		get_ambarella_dspmem_size() - dsp_get_ucode_size());
	if ((phys_addr & HIGH_MEM_MASK) >> 28) {
		G_total_dram_size = IAV_DRAM_SIZE_8Gb;
	}

	/* User, Overlay, BSB and Privacy Mask share same memory pre-allocated in amboot.*/
	/* User memory */
	phys_addr = get_ambarella_bsbmem_phys();
	ptr = (u8*)get_ambarella_bsbmem_virt();
	size = USER_MEM_SIZE;
	init_mem_block(&G_iav_mem.user, phys_addr, ptr, size);

	/* Overlay memory */
	phys_addr += size;
	ptr += size;
	size = OVERLAY_MEM_SIZE;
	init_mem_block(&G_iav_mem.overlay, phys_addr, ptr, size);

	/* Bit stream buffer */
	phys_addr += size;
	ptr += size;
	size = BITSTREAM_MEM_SIZE;
	init_mem_block(&G_iav_mem.bsb, phys_addr, ptr, size);

	/* IMGPROC buffer */
	phys_addr += size;
	ptr += size;
	size = IMGPROC_MEM_SIZE;
	init_mem_block(&G_iav_mem.imgproc, phys_addr, ptr, size);
	img_set_phys_start_ptr(phys_addr);

	/* QP ROI matrix memory */
	phys_addr += size;
	ptr += size;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	size = STREAM_QP_MATRIX_MEM_TOTAL_SIZE * DEFAULT_TOGGLED_ENC_CMD_NUM;
#else
	size = STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
#endif
	init_mem_block(&G_iav_mem.qp_matrix, phys_addr, ptr, size);

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	/* sync frame cmd memory */
	phys_addr += size;
	ptr += size;
	size = VCAP_ENC_CMD_MEM_TOTAL_SIZE;
	init_mem_block(&G_iav_mem.cmd_sync, phys_addr, ptr, size);
#endif

	/* Privacy Mask memory */
	phys_addr += size;
	ptr += size;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	size = get_ambarella_bsbmem_size() - USER_MEM_SIZE - OVERLAY_MEM_SIZE -
		BITSTREAM_MEM_SIZE - IMGPROC_MEM_SIZE -
		STREAM_QP_MATRIX_MEM_TOTAL_SIZE * DEFAULT_TOGGLED_ENC_CMD_NUM -
		VCAP_ENC_CMD_MEM_TOTAL_SIZE;
#else
	size = get_ambarella_bsbmem_size() - USER_MEM_SIZE - OVERLAY_MEM_SIZE -
		BITSTREAM_MEM_SIZE - IMGPROC_MEM_SIZE -
		STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
#endif
	init_mem_block(&G_iav_mem.privacymask, phys_addr, ptr, size);

	/* Erase all memory, otherwise cause variable uninitialized */
	memset((u8*)get_ambarella_bsbmem_virt(), 0,
		get_ambarella_bsbmem_size());

	/* Motion vector buffer */
	ptr = kzalloc(DEFAULT_MV_MALLOC_SIZE, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for motion vector (MV) !\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.mv,
		virt_to_phys(ptr), ptr, DEFAULT_MV_MALLOC_SIZE);

	/* QP histogram buffer */
	ptr = kzalloc(DEFAULT_QP_HIST_SIZE, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for QP histogram!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.qp_hist,
		virt_to_phys(ptr), ptr, DEFAULT_QP_HIST_SIZE);

	/* JPEG QT matrix memory */
	if (mem_alloc_jpeg_qt_matrix(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for JPEG QT matrix!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.jpeg_qt,
		virt_to_phys(ptr), ptr, size);

	/* Warp vector table memory */
	if (mem_alloc_yuv_warp(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for warp vector table!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.yuv_warp,
		virt_to_phys(ptr), ptr, size);

	/* Encode stream config memory */
	if (mem_alloc_enc_cfg(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for encode config!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.enc_cfg,
		virt_to_phys(ptr), ptr, size);

	/* Local exposure memory */
	if (mem_alloc_local_exposure(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for local exposure!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.local_expo, virt_to_phys(ptr), ptr, size);

	/* Chroma aberration memory */
	if (mem_alloc_ca_warp(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for chroma aberration!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.ca_warp, virt_to_phys(ptr), ptr, size);

	/* Q matrix for scaling list */
	if (mem_alloc_q_matrix(&ptr, &size) < 0) {
		iav_error("iav_mem_init: out of memory for Q matrix!\n");
		goto iav_mem_init_free_memory;
	}
	init_mem_block(&G_iav_mem.q_matrix, virt_to_phys(ptr), ptr, size);

	/* bit stream descriptors */
	ptr = kzalloc(sizeof(dsp_bits_info_t) * NUM_BS_DESC, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for bits info !\n");
		goto iav_mem_init_free_memory;
	}
	G_iav_mem.bs_desc.start = (dsp_bits_info_t *)ptr;
	G_iav_mem.bs_desc.end = (dsp_bits_info_t *)ptr + NUM_BS_DESC;

	/* Extend bit stream descriptors */
	ptr = kzalloc(sizeof(dsp_extend_bits_info_t) * NUM_BS_DESC, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for extension bits info !\n");
		goto iav_mem_init_free_memory;
	}
	G_iav_mem.bs_desc.ext_start = (dsp_extend_bits_info_t *)ptr;
	G_iav_mem.bs_desc.ext_end = (dsp_extend_bits_info_t *)ptr + NUM_BS_DESC;

	/* bits partition descriptors */
	ptr = kzalloc(sizeof(dsp_bits_partition_info_t) * NUM_BS_DESC, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for bits partition info!\n");
		goto iav_mem_init_free_memory;
	}
	G_iav_mem.bs_desc.bp_start = (dsp_bits_partition_info_t *)ptr;
	G_iav_mem.bs_desc.bp_end = (dsp_bits_partition_info_t *)ptr + NUM_BS_DESC;

	/* Encoder statistics descriptors */
	ptr = kzalloc(sizeof(dsp_enc_stat_info_t) * NUM_STATIS_DESC, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for encode statistics info!\n");
		goto iav_mem_init_free_memory;
	}
	G_iav_mem.stat_desc.start = (dsp_enc_stat_info_t *)ptr;
	G_iav_mem.stat_desc.end = (dsp_enc_stat_info_t *)ptr + NUM_STATIS_DESC;

	/* Extend encode statistics descriptors */
	ptr = kzalloc(sizeof(dsp_extend_enc_stat_info_t) * NUM_STATIS_DESC, GFP_KERNEL);
	if (ptr == NULL) {
		iav_error("iav_mem_init: out of memory for extension statistics info!\n");
		goto iav_mem_init_free_memory;
	}
	G_iav_mem.stat_desc.ext_start = (dsp_extend_enc_stat_info_t *)ptr;
	G_iav_mem.stat_desc.ext_end = (dsp_extend_enc_stat_info_t *)ptr +
		NUM_STATIS_DESC;

	return 0;

iav_mem_init_free_memory:
	free_all_mem();

	return -ENOMEM;
}

void iav_mem_cleanup(void)
{
	free_all_mem();
}

int iav_get_mem_block(int mmap_type, struct iav_mem_block ** block)
{
	if (block == NULL) {
		iav_error("Invalid pointer!\n");
		return -EINVAL;
	}

	if (get_mem_block_ptr(mmap_type, block) < 0) {
		iav_error("Invalid mmap type [%d]. Failed to get mem block info.\n",
			mmap_type);
		return -EINVAL;
	}

	return 0;
}

int iav_set_mem_block(int mmap_type, struct iav_mem_block * block)
{
	struct iav_mem_block * mem_block = NULL;

	if (get_mem_block_ptr(mmap_type, &mem_block) < 0) {
		iav_error("Invalid mmap type [%d]. Failed to set mem block info.\n",
			mmap_type);
		return -EINVAL;
	}
	mem_block->phys_start = block->phys_start;
	mem_block->phys_end = block->phys_end;
	mem_block->kernel_start = block->kernel_start;
	mem_block->kernel_end = block->kernel_end;
	mem_block->size = block->size;

	return 0;
}

inline int iav_get_bits_desc(struct iav_bits_desc ** desc)
{
	*desc = &G_iav_mem.bs_desc;
	return 0;
}

inline int iav_get_stat_desc(struct iav_stat_desc ** desc)
{
	*desc = &G_iav_mem.stat_desc;
	return 0;
}

inline int get_total_dram_size(u32 * total_size)
{
	*total_size = G_total_dram_size;
	return 0;
}

void *iav_create_mmap(iav_context_t *context, unsigned long phys_start, unsigned long virt_start,
	unsigned long size, unsigned long prot, int noncached, struct dsp_vm_space_s *vms)
{
	unsigned long page_offset;
	unsigned long rval;
	struct iav_global_info *info = context->g_info;

	page_offset = phys_start & ~PAGE_MASK;
	size += page_offset;
	size = PAGE_ALIGN(size);
	phys_start &= PAGE_MASK;

	if (!info->map2times) {
		info->mmap_type = IAV_MMAP_PHYS;
	} else {
		info->mmap_type = IAV_MMAP_PHYS2;
	}
	info->called_by_ioc = 1;
	info->noncached = noncached;

	info->mmap_page_offset = page_offset;
	info->mmap_phys_start = phys_start;
	info->mmap_size = size;
	info->mmap_prot = prot;
	info->mmap_user_start = NULL;
	info->mmap_vms = vms;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
	down_write(&current->mm->mmap_sem);
	rval = do_mmap(context->file, virt_start,
		size, prot, MAP_SHARED | MAP_NORESERVE, 0);
	up_write(&current->mm->mmap_sem);
#else
	rval = vm_mmap(context->file, virt_start,
		size, prot, MAP_SHARED | MAP_NORESERVE, 0);
#endif

	if (rval & ~PAGE_MASK) {
		iav_error("NO mem\n");
	} else {
		info->mmap_user_start = (u8*)rval + page_offset;
	}

	info->mmap_type = IAV_MMAP_NONE;
	info->called_by_ioc = 0;
	info->noncached = 0;

	return info->mmap_user_start;
}

void iav_destroy_mmap(iav_context_t *context, unsigned long user_start, unsigned long size)
{
	struct mm_struct *mm;
	int rval = 0;

	iav_printk("unmap: 0x%lx, size 0x%lx\n", user_start, size);

	if ((mm = current->mm) != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
		down_write(&mm->mmap_sem);
		rval = do_munmap(mm, user_start, size);
		up_write(&mm->mmap_sem);
#else
		rval = vm_munmap(user_start, size);
#endif
	}

	iav_printk("unmap: done\n");

	if (rval < 0) {
		iav_error("destroy_mmap failed %d\n", rval);
	}
}

int __iav_do_map_phys(iav_context_t *context, struct vm_area_struct *vma)
{
	struct iav_global_info *info = context->g_info;
	if (info->noncached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (info->mmap_vms) {
		if (__dsp_map_vms(vma, info->mmap_vms, vma->vm_start,
				info->mmap_phys_start,
				info->mmap_size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
	} else {
		if (remap_pfn_range(vma, vma->vm_start,
				info->mmap_phys_start >> PAGE_SHIFT,
				info->mmap_size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
	}

	return 0;
}

int __iav_do_map_phys_2times(iav_context_t *context, struct vm_area_struct *vma)
{
	struct iav_global_info *info = context->g_info;
	if (info->noncached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (info->mmap_vms) {
		if (__dsp_map_vms(vma, info->mmap_vms, vma->vm_start,
				info->mmap_phys_start,
				info->mmap_size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
	} else {
		if ((vma->vm_end - vma->vm_start) != (info->mmap_size)) {
			return -ENOMEM;
		}
		if (remap_pfn_range(vma, vma->vm_start,
				info->mmap_phys_start >> PAGE_SHIFT,
				info->mmap_size/2, vma->vm_page_prot)) {

			return -ENOMEM;
		}
		if (remap_pfn_range(vma, vma->vm_start + info->mmap_size/2,
				info->mmap_phys_start >> PAGE_SHIFT,
				info->mmap_size/2, vma->vm_page_prot)) {

			return -ENOMEM;
		}
	}

	return 0;
}

extern int dma_pitch_memcpy(u8 *dest_addr, u8 *src_addr, u16 src_pitch,
	u16 dest_pitch, u16 width, u16 height, u8 src_non_cached, u8 dst_non_cached);

int iav_get_usr_addr(iav_context_t * context, int type, u32 *start_addr)
{
	switch (type) {
		case IAV_MMAP_IMGPROC:
			*start_addr = (u32)context->imgproc.user_start;
			break;
		case IAV_MMAP_PRIVACYMASK:
			*start_addr = (u32)context->privacymask.user_start;
			break;
		case IAV_MMAP_USER:
			*start_addr = (u32)context->user.user_start;
			break;
		case IAV_MMAP_OVERLAY:
			*start_addr = (u32)context->overlay.user_start;
			break;
		case IAV_MMAP_FB:
			*start_addr = (u32)context->fb.user_start;
			break;
		case IAV_MMAP_BSB2:
		case IAV_MMAP_BSB:
			*start_addr = (u32)context->bsb.user_start;
			break;
		case IAV_MMAP_DSP:
			*start_addr = (u32)context->dsp.user_start;
			break;
		default:
			iav_error("No support mmap type:%d\n", type);
			return -EINVAL;
	}

	return 0;
}

static inline int get_dsp_partition(iav_context_t * context, u32 user_addr)
{
	int i;
	for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
		if (context->dsp_partition[i].user_start && user_addr >= (u32)context->dsp_partition[i].user_start
			&& user_addr < (u32)context->dsp_partition[i].user_end) {
			return i;
		}
	}

	return -1;
}

int iav_gdma_copy(iav_context_t * context, iav_gdma_copy_ex_t __user * arg)
{
	u32	dst_addr;
	u32	src_addr;
	u32	usr_src_addr = 0;
	u32	usr_dst_addr = 0;
	struct iav_mem_block *src;
	struct iav_mem_block *dst;
	iav_gdma_copy_ex_t gdma;
	int src_dsp_partition = -1;
	int dst_dsp_partition = -1;

	if (copy_from_user(&gdma, arg, sizeof(iav_gdma_copy_ex_t)))
		return -EFAULT;

	if ((gdma.usr_src_addr & 0x3)  || (gdma.usr_dst_addr & 0x3)) {
		iav_error("usr_src_addr or usr_dst_addr not algin to 4bytes");
		return -EINVAL;
	}

	if (is_map_dsp_partition() && gdma.src_mmap_type == IAV_MMAP_DSP) {
		src_dsp_partition = get_dsp_partition(context, gdma.usr_src_addr);
		if (src_dsp_partition < 0) {
			iav_error("get_dsp_partition\n");
			return -EINVAL;
		}
		usr_src_addr = (u32)context->dsp_partition[src_dsp_partition].user_start;
		gdma.src_non_cached = 1;
	} else {
		if (iav_get_usr_addr(context, gdma.src_mmap_type, &usr_src_addr) < 0) {
			iav_error("iav_get_usr_addr\n");
			return -EINVAL;
		}
	}

	if (is_map_dsp_partition() && gdma.dst_mmap_type == IAV_MMAP_DSP) {
		dst_dsp_partition = get_dsp_partition(context, gdma.usr_dst_addr);
		if (dst_dsp_partition < 0) {
			iav_error("get_dsp_partition\n");
			return -EINVAL;
		}
		usr_dst_addr = (u32)context->dsp_partition[dst_dsp_partition].user_start;
		gdma.dst_non_cached = 1;
	} else {
		if (iav_get_usr_addr(context, gdma.dst_mmap_type, &usr_dst_addr) < 0) {
			iav_error("iav_get_usr_addr\n");
			return -EINVAL;
		}
	}
	iav_get_mem_block(gdma.src_mmap_type, &src);
	iav_get_mem_block(gdma.dst_mmap_type, &dst);

	if (usr_src_addr > gdma.usr_src_addr) {
		iav_error("usr: src_addr:0x%08x, addr:0x%08x\n",
			gdma.usr_src_addr, usr_src_addr);
		iav_error(" Out of range\n");
		return -EINVAL;
	}

	if (usr_dst_addr > gdma.usr_dst_addr) {
		iav_error("usr: dst_addr:0x%08x, addr:0x%08x\n",
			gdma.usr_dst_addr, usr_dst_addr);
		iav_error(" Out of range\n");
		return -EINVAL;
	}

	if (usr_src_addr == usr_dst_addr) {
		if (gdma.usr_dst_addr >= gdma.usr_src_addr) {
			if (gdma.usr_dst_addr < gdma.usr_src_addr +
				gdma.src_pitch * gdma.height) {
				iav_error("usr: src_addr:0x%08x, dst_addr:0x%x, size:0x%08x\n",
					gdma.usr_src_addr, gdma.usr_dst_addr,
					gdma.src_pitch * gdma.height);
				iav_error(" Out of range\n");
				return -EINVAL;
			}
		} else {
			if (gdma.usr_src_addr < gdma.usr_dst_addr +
				gdma.dst_pitch * gdma.height) {
				iav_error("usr: src_addr:0x%08x, dst_addr:0x%x, size:0x%08x\n",
					gdma.usr_src_addr, gdma.usr_dst_addr,
					gdma.src_pitch * gdma.height);
				iav_error(" Out of range\n");
				return -EINVAL;
			}
		}
	}

	if (is_map_dsp_partition() && gdma.dst_mmap_type == IAV_MMAP_DSP) {
		dst_addr = G_iav_mem.dsp_partition[dst_dsp_partition].phys_start + gdma.usr_dst_addr - usr_dst_addr;
	} else {
		dst_addr = dst->phys_start + gdma.usr_dst_addr - usr_dst_addr;
	}

	if (is_map_dsp_partition() && gdma.dst_mmap_type == IAV_MMAP_DSP) {
		src_addr = G_iav_mem.dsp_partition[src_dsp_partition].phys_start + gdma.usr_src_addr - usr_src_addr;
	} else {
		src_addr = src->phys_start + gdma.usr_src_addr - usr_src_addr;
	}

	if (is_map_dsp_partition()) {
		if ((src_addr + gdma.src_pitch * gdma.height > G_iav_mem.dsp_partition[src_dsp_partition].phys_end) ||
			(dst_addr + gdma.dst_pitch * gdma.height > G_iav_mem.dsp_partition[dst_dsp_partition].phys_end)) {
			iav_error("There is no enough phys space\n");
			return -EINVAL;
		}
	} else {
		if ((src_addr + gdma.src_pitch * gdma.height > src->phys_end) ||
			(dst_addr + gdma.dst_pitch * gdma.height > dst->phys_end)) {
			iav_error("There is no enough phys space\n");
			return -EINVAL;
		}
	}
	if (dma_pitch_memcpy((u8 *)dst_addr, (u8 *)src_addr, gdma.src_pitch,
		gdma.dst_pitch, gdma.width, gdma.height,
		gdma.src_non_cached, gdma.dst_non_cached) < 0) {
		iav_error("dma_pitch_memcpy error\n");
		return -EFAULT;
	}

	return 0;
}

void iav_set_dsp_partition(int partition, u32 phys_start, u32 size)
{
	if (size > 0 && phys_start != 0xFFFFFFFF) {
		G_iav_mem.dsp_partition[partition].phys_start = round_down(phys_start, PAGE_SIZE);
		G_iav_mem.dsp_partition[partition].phys_end = round_up(phys_start + size, PAGE_SIZE);
		G_iav_mem.dsp_partition[partition].size = G_iav_mem.dsp_partition[partition].phys_end -
			G_iav_mem.dsp_partition[partition].phys_start;
	}
}

void iav_get_dsp_partition(int partition, u32 *phys_start, u32 *size)
{
	*phys_start = G_iav_mem.dsp_partition[partition].phys_start;
	*size = G_iav_mem.dsp_partition[partition].size;
}

void iav_reset_dsp_partition()
{
	memset(&G_iav_mem.dsp_partition, 0, sizeof(G_iav_mem.dsp_partition));
}

int partition_id_to_index(int id) {
	int i = 0;
	for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
		if (id & (1 << i)) {
			return i;
		}
	}
	return -1;
}

