/*
 * iav_drv.c
 *
 * History:
 *	2011/10/12 - [Jian Tang] created file
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
#include <linux/proc_fs.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "iav_drv.h"
#include "dsp_api.h"
#include "iav_mem.h"
#include "amba_iav.h"
#include "iav_common.h"
#include "iav_api.h"
#include "iav_decode.h"
#include "utils.h"
#include "iav_feature.h"
#include "iav_config.h"
#include "iav_priv.h"
#include "iav_pts.h"
#include "iav_encode.h"
#include "iav_privmask.h"
#include "iav_bufcap.h"
#include "amba_devnum.h"

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_imgproc.h"
#endif


MODULE_AUTHOR("Jian Tang<jtang@ambarella.com>");
MODULE_DESCRIPTION("Ambarella iav driver");
MODULE_LICENSE("Proprietary");

static const char *iav_name = "iav";
static struct cdev iav_cdev;

static const char *ucode_name = "ucode";
static struct cdev ucode_cdev;

DEFINE_MUTEX(iav_mutex);

struct iav_global_info G_iav_info;

struct amba_iav_vout_info G_vout0info = {
	.enabled = 0,
	.active_sink_id = 0,
};
struct amba_iav_vout_info G_vout1info = {
	.enabled = 0,
	.active_sink_id = 0,
};
struct amba_iav_vin_info G_vininfo;

static u32 ucode_user_start;

void clean_cache_aligned(u8 *kernel_start, unsigned long size)
{
	unsigned long offset = (unsigned long)kernel_start & (CACHE_LINE_SIZE - 1);
	kernel_start -= offset;
	size += offset;
	clean_d_cache(kernel_start, size);
}

void iav_lock(void)
{
	mutex_lock(&iav_mutex);
}

void iav_unlock(void)
{
	mutex_unlock(&iav_mutex);
}

static long ucode_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	iav_lock();

	switch (cmd) {
	case IAV_IOC_GET_UCODE_INFO:
		rval = copy_to_user((ucode_load_info_t __user*)arg,
			dsp_get_ucode_info(), sizeof(ucode_load_info_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_GET_UCODE_VERSION:
		rval = copy_to_user((ucode_version_t __user*)arg,
			dsp_get_ucode_version(), sizeof(ucode_version_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_UPDATE_UCODE:
		if (ucode_user_start == 0) {
			rval = -EPERM;
		} else {
			clean_cache_aligned((void*)ucode_user_start, dsp_get_ucode_size());
			rval = 0;
		}
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	iav_unlock();
	return rval;
}

static int ucode_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	unsigned long size;
	iav_lock();

	size = vma->vm_end - vma->vm_start;
	if (size != dsp_get_ucode_size()) {
		rval = -EINVAL;
		goto Done;
	}

	vma->vm_pgoff = dsp_get_ucode_start() >> PAGE_SHIFT;
	if ((rval = remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) < 0) {
		goto Done;
	}

	ucode_user_start = vma->vm_start;
	rval = 0;

Done:
	iav_unlock();
	return rval;
}

static int ucode_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ucode_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations ucode_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ucode_ioctl,
	.mmap = ucode_mmap,
	.open = ucode_open,
	.release = ucode_release
};

int __iav_get_mmap(iav_context_t *context, iav_mmap_t __user *arg)
{
	iav_mmap_t mmap;

	mmap.bsb_addr = context->bsb.user_start;
	mmap.bsb_size = context->bsb.user_end - context->bsb.user_start;

	mmap.bsb_dec_addr = context->bsb.user_start;
	mmap.bsb_dec_size = context->bsb.user_end - context->bsb.user_start;

	mmap.dsp_addr = context->dsp.user_start;
	mmap.dsp_size = context->dsp.user_end - context->dsp.user_start;

	return copy_to_user(arg, &mmap, sizeof(mmap)) ? -EFAULT : 0;
}

static int __iav_map_mem(iav_context_t *context, int mmap_type,
	u32 double_map, unsigned long prot, u32 noncached,
	struct iav_addr_map *map, iav_mmap_info_t __user *arg)
{
	u32 size;
	unsigned long rval;
	iav_mmap_info_t info;
	struct iav_mem_block *block;

	if (map->user_start == NULL) {
		iav_get_mem_block(mmap_type, &block);
		size = double_map ? block->size * 2 : block->size;

		context->g_info->mmap_type = mmap_type;
		context->g_info->called_by_ioc = 1;
		context->g_info->noncached = noncached;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
		down_write(&current->mm->mmap_sem);
		rval = do_mmap(context->file, 0, size, prot, MAP_SHARED, 0);
		up_write(&current->mm->mmap_sem);
#else
		rval = vm_mmap(context->file, 0, size, prot, MAP_SHARED, 0);
#endif
		context->g_info->mmap_type = IAV_MMAP_NONE;
		context->g_info->called_by_ioc = 0;
		context->g_info->noncached = 0;

		if (rval & ~PAGE_MASK) {
			return -ENOMEM;
		}

		info.addr = (u8*)rval;
		info.length = block->size;

		map->ref_count = 1;
	} else {
		info.addr = map->user_start;
		info.length = map->user_end - map->user_start;
		map->ref_count++;
	}

	if (arg != NULL && copy_to_user(arg, &info, sizeof(info))) {
		return -EFAULT;
	}

	return 0;
}

static int map_dsp_partition(iav_context_t *context, int mmap_type,
	u32 noncached, iav_mmap_dsp_partition_info_t __user *arg)
{
	unsigned long rval;
	iav_mmap_dsp_partition_info_t dsp_partition_info;
	int i = 0;
	u32 phys_start = 0, size = 0;
	int map_flag = PROT_READ;

	memset(&dsp_partition_info, 0, sizeof(iav_mmap_dsp_partition_info_t));

	if (!is_iav_state_prev_or_enc()) {
		iav_error("DSP should be mapped in Preview or Encode state\n");
		return -EFAULT;
	}

	if (arg) {
		if (copy_from_user(&dsp_partition_info, arg, sizeof(iav_mmap_dsp_partition_info_t))) {
			return -EFAULT;
		}
	} else {
		dsp_partition_info.id_map = DEFAULT_ALL_PARTITION;
	}
	context->dsp_partition_id_map = dsp_partition_info.id_map;

	if (iav_get_frm_buf_pool_info(&dsp_partition_info.id_map) < 0) {
		return -EFAULT;
	}

	for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
		if (dsp_partition_info.id_map & (1 << i)) {
			iav_get_dsp_partition(i, &phys_start, &size);
			if (context->dsp_partition[i].user_start == NULL) {
				context->g_info->mmap_type = mmap_type;
				context->g_info->called_by_ioc = 1;
				// DSP now is noncached always.
				context->g_info->noncached = 1;
				context->curr_partition = i;

				if (i >= partition_id_to_index(IAV_DSP_PARTITION_MAIN_CAPTURE)) {
					map_flag |= PROT_WRITE;
				}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
				down_write(&current->mm->mmap_sem);
				rval = do_mmap(context->file, 0, size, map_flag, MAP_SHARED, 0);
				up_write(&current->mm->mmap_sem);
#else
				rval = vm_mmap(context->file, 0, size, map_flag, MAP_SHARED, 0);
#endif
				context->g_info->mmap_type = IAV_MMAP_NONE;
				context->g_info->called_by_ioc = 0;
				context->g_info->noncached = 0;

				if (rval & ~PAGE_MASK) {
					return -ENOMEM;
				}
				dsp_partition_info.addr[i] = (u8*)rval;
				dsp_partition_info.length[i] = size;

			} else {
				dsp_partition_info.addr[i] = context->dsp_partition[i].user_start;
				dsp_partition_info.length[i] = size;
			}

		}
	}

	if (arg) {
		if (copy_to_user(arg, &dsp_partition_info, sizeof(dsp_partition_info))) {
			return -EFAULT;
		}
	}

	return 0;
}

static int __iav_unmap_mem(iav_context_t *context, struct iav_addr_map *map)
{
	int rval = 0;
	struct mm_struct *mm;

	if (map->user_start == NULL || map->user_end == NULL)
		return -EINVAL;

	if (map->ref_count > 0 && --map->ref_count == 0) {
		mm = current->mm;
		if (mm != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
			down_write(&mm->mmap_sem);
			rval = do_munmap(mm, (unsigned long)map->user_start,
				(size_t)(map->user_end - map->user_start));
			up_write(&mm->mmap_sem);
#else
			rval = vm_munmap((unsigned long)map->user_start,
				(size_t)(map->user_end - map->user_start));
#endif
		}

		if (rval == 0) {
			map->user_start = NULL;
			map->user_end = NULL;
		}
	}

	return rval;
}

static int __iav_get_driver_info(iav_context_t *context, driver_version_t __user *arg)
{
	extern driver_version_t iav_driver_info;
	return copy_to_user(arg, &iav_driver_info, sizeof(iav_driver_info)) ? -EFAULT : 0;
}

static void boot_to_idle_mode(void *context)
{
	iav_config_vout(context, -1, VOUT_SRC_BACKGROUND | (VOUT_SRC_BACKGROUND << 16));
}

static int __iav_idle_action(iav_context_t *context)
{
	int rval;

	if ((rval = dsp_set_mode(DSP_OP_MODE_IDLE, boot_to_idle_mode, context)) < 0)
		return rval;

	G_iav_info.state = IAV_STATE_IDLE;

	return 0;
}

static int __iav_goto_idle(iav_context_t *context)
{
	int rval;

	switch (G_iav_info.state) {
	case IAV_STATE_ENCODING:
		iav_printk("Goto Idle from STATE_ENCODING.\n");
		if ((rval = iav_stop_encode_ex(context, STREAM_ID_MASK)) < 0)
			break;
		/* fall through */

	case IAV_STATE_PREVIEW:
		iav_printk("Goto Idle from STATE_PREVIEW \n");
		rval = iav_disable_preview(context);
		break;

	case IAV_STATE_STILL_CAPTURE:
		iav_printk("Goto Idle from STATE_STILL_CAP \n");
		rval = -EPERM;
		break;

	case IAV_STATE_DECODING:
		iav_printk("Goto Idle from STATE_DECODING \n");
		rval = iav_leave_decode_mode(context);
		break;

	case IAV_STATE_INIT:
	case IAV_STATE_IDLE:
		iav_printk("Goto Idle from STATE_IDLE \n");
		rval = 0;
		break;

	default:
		iav_printk("Goto Idle from STATE %d\n", G_iav_info.state);
		rval = -EPERM;
		break;
	}

	if (rval == 0 && (G_iav_info.state == IAV_STATE_IDLE ||
			G_iav_info.state == IAV_STATE_INIT))
		rval = __iav_idle_action(context);

	return rval;
}

static int __iav_ioctl(iav_context_t *context, unsigned int cmd, unsigned long arg)
{
	int rval = -ENOIOCTLCMD;
	static int whole_dsp_mapped = 0;
	struct iav_mem_block *dsp_block = NULL;

	switch (cmd) {
	case IAV_IOC_GET_DRIVER_INFO:
		rval = __iav_get_driver_info(context, (driver_version_t __user *)arg);
		break;

	case IAV_IOC_GET_MMAP_INFO:
		rval = __iav_get_mmap(context, (iav_mmap_t __user *)arg);
		break;

	case IAV_IOC_MAP_LOGO:
		rval = -EINVAL;
		break;

	case IAV_IOC_MAP_DECODE_BSB:
		rval = __iav_map_mem(context,
			IAV_MMAP_BSB, 0,
			PROT_READ | PROT_WRITE, 0,
			&context->bsb,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_DSP:
		if (is_map_dsp_partition()) {
			if (whole_dsp_mapped && get_ambarella_dspmem_virt()) {
				iounmap((void *)get_ambarella_dspmem_virt());
				whole_dsp_mapped = 0;
			}
		} else {
			if (!whole_dsp_mapped) {
				iav_get_mem_block(IAV_MMAP_DSP, &dsp_block);
				dsp_block->kernel_start = (u8 *)ioremap(get_ambarella_dspmem_phys(), dsp_block->size);
				if (!dsp_block->kernel_start) {
					iav_error("Can not Map Whole DSP!\n");
					return -ENOMEM;
				}
				dsp_block->kernel_end = dsp_block->kernel_start + dsp_block->size;
				set_ambarella_dspmem_virt((u32)dsp_block->kernel_start);
				whole_dsp_mapped = 1;
			}
		}
		rval = is_map_dsp_partition() ? map_dsp_partition(context,
			IAV_MMAP_DSP_PARTITION, G_iav_obj.dsp_noncached, NULL)
			: __iav_map_mem(context,
			IAV_MMAP_DSP, 0,
			PROT_READ | PROT_WRITE, G_iav_obj.dsp_noncached,
			&context->dsp,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_DSP_PARTITION:
		rval = map_dsp_partition(context, IAV_MMAP_DSP_PARTITION,
			G_iav_obj.dsp_noncached, (iav_mmap_dsp_partition_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_IMGPROC:
		rval = __iav_map_mem(context,
			IAV_MMAP_IMGPROC, 0,
			PROT_READ | PROT_WRITE, 1,
			&context->imgproc,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_BSB:
	case IAV_IOC_MAP_BSB2:
		rval = __iav_map_mem(context,
			IAV_MMAP_BSB2, 1,
			PROT_READ | PROT_WRITE, 1,
			&context->bsb,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_DECODE_BSB2:
		rval = __iav_map_mem(context,
			IAV_MMAP_BSB2, 1,
			PROT_READ | PROT_WRITE, 0,
			&context->bsb,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_QP_ROI_MATRIX_EX:
		rval = __iav_map_mem(context,
			IAV_MMAP_QP_MATRIX, 0,
			PROT_READ | PROT_WRITE, 1,
			&context->qp_matrix,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_MV_EX:
		rval = __iav_map_mem(context,
			IAV_MMAP_MV, 0,
			PROT_READ, 0,
			&context->mv,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_MAP_QP_HIST_EX:
		rval = __iav_map_mem(context,
			IAV_MMAP_QP_HIST, 0,
			PROT_READ, 0,
			&context->qp_hist,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_UNMAP_BSB:
	case IAV_IOC_UNMAP_DECODE_BSB:
		rval = __iav_unmap_mem(context, &context->bsb);
		break;

	case IAV_IOC_UNMAP_DSP:
		rval = __iav_unmap_mem(context, &context->dsp);
		break;

	case IAV_IOC_UNMAP_QP_ROI_MATRIX_EX:
		rval = __iav_unmap_mem(context, &context->qp_matrix);
		break;

	case IAV_IOC_UNMAP_MV_EX:
		rval = __iav_unmap_mem(context, &context->mv);
		break;

	case IAV_IOC_UNMAP_QP_HIST_EX:
		rval = __iav_unmap_mem(context, &context->qp_hist);
		break;

	case IAV_IOC_VOUT_HALT:
		rval = iav_halt_vout(context, (int)arg);
		break;

	case IAV_IOC_VOUT_SELECT_DEV:
		rval = iav_select_output_dev(context, (int)arg);
		break;

	case IAV_IOC_VOUT_CONFIGURE_SINK:
		rval = iav_configure_sink(context,
			(struct amba_video_sink_mode __user *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_CSC:
		rval = iav_enable_vout_csc(context,
			(struct iav_vout_enable_csc_s *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_VIDEO:
		rval = iav_enable_vout_video(context,
			(struct iav_vout_enable_video_s *)arg);
		break;

	case IAV_IOC_VOUT_FLIP_VIDEO:
		rval = iav_flip_vout_video(context,
			(struct iav_vout_flip_video_s *)arg);
		break;

	case IAV_IOC_VOUT_ROTATE_VIDEO:
		rval = iav_rotate_vout_video(context,
			(struct iav_vout_rotate_video_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_VIDEO_SIZE:
		rval = iav_change_vout_video_size(context,
			(struct iav_vout_change_video_size_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET:
		rval = iav_change_vout_video_offset(context,
			(struct iav_vout_change_video_offset_s *)arg);
		break;

	case IAV_IOC_VOUT_SELECT_FB:
		rval = iav_select_vout_fb(context, (struct iav_vout_fb_sel_s *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_OSD_RESCALER:
		rval = iav_enable_vout_osd_rescaler(context,
			(struct iav_vout_enable_osd_rescaler_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_OSD_OFFSET:
		rval = iav_change_vout_osd_offset(context,
			(struct iav_vout_change_osd_offset_s *)arg);
		break;

	case IAV_IOC_GET_NUM_CHANNEL: {
			iav_num_channel_t channel;
			channel.num_enc_channels = 1;
			channel.num_dec_channels = 1;
			rval = copy_to_user((void*)arg, &channel, sizeof(channel)) ?
				-EFAULT : 0;
		}
		break;

	case IAV_IOC_SELECT_CHANNEL:
		rval = 0;
		break;

	case IAV_IOC_GET_STATE:
		rval = put_user(G_iav_info.state, (int __user *)arg) ? -EFAULT : 0;
		break;

	case IAV_IOC_GET_STATE_INFO:
		rval = iav_get_state_info(context, (iav_state_info_t __user *)arg);
		break;

	case IAV_IOC_ENTER_IDLE:
		rval = __iav_goto_idle(context);
		break;

	case IAV_IOC_INIT_STILL_CAPTURE:
		rval = iav_init_still_capture(context,
			(struct iav_still_init_info __user*)arg);
		break;

	case IAV_IOC_START_STILL_CAPTURE:
		rval = iav_start_still_capture(context,
			(struct iav_still_cap_info __user*)arg);
		break;

	case IAV_IOC_STOP_STILL_CAPTURE:
		rval = iav_stop_still_capture(context);
		break;

	case IAV_IOC_READ_PREVIEW_INFO:
		iav_error("Obsolete, please use IAV_IOC_READ_YUV_BUFFER_INFO_EX"
			" instead!\n");
		break;

	case IAV_IOC_READ_RAW_INFO:
		rval = iav_read_raw_info_ex(context,
			(struct iav_raw_info_s __user *)arg);
		break;

	case IAV_IOC_SET_VIN_CAPTURE_WINDOW:
		rval = iav_set_vin_capture_win(context,
			(struct iav_rect_ex_s __user *)arg);
		break;

	case IAV_IOC_GET_VIN_CAPTURE_WINDOW:
		rval = iav_get_vin_capture_win(context,
			(struct iav_rect_ex_s __user *)arg);
		break;

	case IAV_IOC_SET_SYSTEM_SETUP_INFO_EX:
		rval = iav_set_system_setup_info(context,
			(iav_system_setup_info_ex_t __user *)arg);
		break;

	case IAV_IOC_GET_SYSTEM_SETUP_INFO_EX:
		rval = iav_get_system_setup_info(context,
			(iav_system_setup_info_ex_t __user *)arg);
		break;

	case IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX:
		rval = iav_set_system_resource_limit_ex(context,
			(iav_system_resource_setup_ex_t __user *)arg);
		break;

	case IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX:
		rval = iav_get_system_resource_limit_ex(context,
			(iav_system_resource_setup_ex_t __user *)arg);
		break;

	case IAV_IOC_GET_CHIP_ID_EX:
		rval = iav_get_chip_id_ex(context, (int __user *)arg);
		break;

	case IAV_IOC_GDMA_COPY_EX:
		rval = iav_gdma_copy(context,
			(struct iav_gdma_copy_ex_s __user *)arg);
		break;

	case IAV_IOC_TEST:
		rval = iav_test(context, arg);
		break;
	case IAV_IOC_LOG_SETUP:
		rval = iav_log_setup(context, (struct iav_dsp_setup_s __user *)arg);
		break;
	case IAV_IOC_DEBUG_SETUP:
		rval = iav_debug_setup(context, (struct iav_debug_setup_s __user *)arg);
		break;

	default:
		iav_error("Unknown IOCTL: (0x%x) %d:%c:%d:%d\n", cmd,
			_IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

static int __iav_overlay_ioctl(iav_context_t * context,
	unsigned int cmd, unsigned long arg)
{
	int rval;

	switch (cmd) {
	case IAV_IOC_MAP_USER:
		rval = __iav_map_mem(context,
			IAV_MMAP_USER, 0,
			PROT_READ | PROT_WRITE, 1,
			&context->user,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_UNMAP_USER:
		rval = __iav_unmap_mem(context, &context->user);
		break;

	case IAV_IOC_MAP_OVERLAY:
		rval = __iav_map_mem(context,
			IAV_MMAP_OVERLAY, 0,
			PROT_READ | PROT_WRITE, 1,
			&context->overlay,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_UNMAP_OVERLAY:
		rval = __iav_unmap_mem(context, &context->overlay);
		break;

	case IAV_IOC_OVERLAY_INSERT_EX:
		rval = iav_overlay_insert_ex(context,
			(struct overlay_insert_ex_s __user *)arg);
		break;

	case IAV_IOC_GET_OVERLAY_INSERT_EX:
		rval = iav_get_overlay_insert_ex(context,
			(struct overlay_insert_ex_s __user *)arg);
		break;

	case IAV_IOC_MAP_PRIVACY_MASK_EX:
		rval = __iav_map_mem(context,
			IAV_MMAP_PRIVACYMASK, 0,
			PROT_READ | PROT_WRITE, 1,
			&context->privacymask,
			(iav_mmap_info_t __user *)arg);
		break;

	case IAV_IOC_UNMAP_PRIVACY_MASK_EX:
		rval = __iav_unmap_mem(context, &context->privacymask);
		break;

	case IAV_IOC_SET_PRIVACY_MASK_EX:
		rval = iav_set_privacy_mask_ex(context,
			(struct iav_privacy_mask_setup_ex_s __user *)arg);
		break;

	case IAV_IOC_GET_PRIVACY_MASK_EX:
		rval = iav_get_privacy_mask_ex(context,
			(struct iav_privacy_mask_setup_ex_s __user *)arg);
		break;

	case IAV_IOC_GET_PRIVACY_MASK_INFO_EX:
		rval = iav_get_privacy_mask_info_ex(context,
			(struct iav_privacy_mask_info_ex_s __user *)arg);
		break;

	case IAV_IOC_GET_LOCAL_EXPOSURE_EX:
		rval = iav_get_local_exposure_ex(context,
			(struct iav_local_exposure_ex_s __user *)arg);
		break;

	case IAV_IOC_SET_LOCAL_EXPOSURE_EX:
		rval = iav_set_local_exposure_ex(context,
			(struct iav_local_exposure_ex_s __user *)arg);
		break;

	default:
		iav_printk("Unknown cmd 0x%x\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

int iav_update_vin(iav_context_t *context, int rval)
{
	int				errorCode = 0;
	struct amba_vin_src_capability	src_cap;
	struct iav_global_info		*g_info = context->g_info;

	g_info->pvininfo->enabled = IAV_VIN_DISABLED;

	if (rval) {
		errorCode = rval;
		goto iav_update_vin_exit;
	}

	errorCode = amba_vin_source_cmd(
		context->g_info->pvininfo->active_src_id,
		context->active_channel_id,
		AMBA_VIN_SRC_GET_CAPABILITY,
		&src_cap);

	if (!errorCode) {
		if (src_cap.mode_type & AMBA_VIN_SRC_ENABLED_FOR_VIDEO)
			g_info->pvininfo->enabled |= IAV_VIN_ENABLED_FOR_VIDEO;

		if (src_cap.mode_type & AMBA_VIN_SRC_ENABLED_FOR_STILL)
			g_info->pvininfo->enabled |= IAV_VIN_ENABLED_FOR_STILL;

		memcpy((void *)(&g_info->pvininfo->capability),
			(void *)&src_cap,
			sizeof(struct amba_vin_src_capability));
	}

	if (src_cap.slave_mode && g_info->state != IAV_STATE_IDLE)
		iav_config_vin();

iav_update_vin_exit:
	return errorCode;
}

static int iav_vin_src_cmd(
	iav_context_t *context,
	unsigned int cmd,
	unsigned long args)
{
	int				errorCode = 0;
	struct iav_global_info		*g_info = context->g_info;

	switch (cmd) {
	case IAV_IOC_VIN_SRC_GET_INFO:
	{
		struct amba_vin_source_info	src_info;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_INFO,
			&src_info);
		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_source_info __user *)args,
				&src_info, sizeof(src_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE:
	{
		struct amba_vin_source_mode_info	mode_info;
		struct amba_vin_source_mode_info	kmode_info;
		u32					*pfps_table = NULL;

		errorCode = copy_from_user(&mode_info,
		(struct amba_vin_source_mode_info __user *)args,
			sizeof(mode_info)) ? -EFAULT : 0;
		if (errorCode)
			goto IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE_exit;
		pfps_table = (u32 *)kmalloc((sizeof(u32) *
			mode_info.fps_table_size), GFP_KERNEL);
		if (pfps_table == NULL) {
			errorCode = -ENOMEM;
			goto IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE_exit;
		}
		memcpy(&kmode_info, &mode_info, sizeof(mode_info));
		kmode_info.fps_table = pfps_table;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_CHECK_VIDEO_MODE,
			&kmode_info);
		if (!errorCode) {
			kmode_info.fps_table = mode_info.fps_table;
			errorCode = copy_to_user(
				(struct amba_vin_source_mode_info __user *)args,
				&kmode_info, sizeof(mode_info)) ? -EFAULT : 0;
			errorCode = copy_to_user(
				(u32 __user *)mode_info.fps_table,
				pfps_table, (sizeof(u32) *
				mode_info.fps_table_size)) ? -EFAULT : 0;
		}
		kfree(pfps_table);
	}
IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE_exit:
		break;

	case IAV_IOC_VIN_SRC_SET_VIDEO_MODE:
	{
		enum amba_video_mode	mode =
			AMBA_VIDEO_STANDARD_MODE(args);

		if (g_info->state == IAV_STATE_IDLE) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_VIDEO_MODE,
				&mode);
		} else {
			DRV_PRINT("IAV_IOC_VIN_SRC_SET_VIDEO_MODE:"
				"bad state %d\n", g_info->state);
			errorCode = -EPERM;
		}
		errorCode = iav_update_vin(context, errorCode);
	}
		break;

	case IAV_IOC_VIN_SRC_GET_VIDEO_MODE:
	{
		enum amba_video_mode	mode;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_VIDEO_MODE,
			&mode);

		if (!errorCode) {
			errorCode = copy_to_user(
				(enum amba_video_mode __user *)args,
				&mode, sizeof(mode)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_VIDEO_INFO:
	{
		struct amba_video_info	src_video_info;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_VIDEO_INFO,
			&src_video_info);
		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_video_info __user *)args,
				&src_video_info,
				sizeof(src_video_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_AGC_INFO:
	{
		amba_vin_agc_info_t	agc_info;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_AGC_INFO,
			&agc_info);
		if (!errorCode) {
			errorCode = copy_to_user(
				(amba_vin_agc_info_t __user *)args,
				&agc_info,
				sizeof(agc_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_SHUTTER_INFO:
	{
		amba_vin_shutter_info_t	shutter_info;
//		vin_warn("The command IAV_IOC_VIN_SRC_GET_SHUTTER_INFO will be removed!\n");
		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_SHUTTER_INFO,
			&shutter_info);
		if (!errorCode) {
			errorCode = copy_to_user(
				(amba_vin_shutter_info_t __user *)args,
				&shutter_info,
				sizeof(shutter_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_BLC:
	{
		struct amba_vin_black_level_compensation	src_blc;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&src_blc,
			(struct amba_vin_black_level_compensation __user *)args,
				sizeof(src_blc)) ? -EFAULT : 0;

			if (!errorCode) {
				errorCode = amba_vin_source_cmd(
					context->g_info->pvininfo->active_src_id,
					context->active_channel_id,
					AMBA_VIN_SRC_SET_BLC,
					&src_blc);
			}
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_FRAME_RATE:
	{
		int	fps = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_FRAME_RATE,
				&fps);
		} else {
			DRV_PRINT("IAV_IOC_VIN_SRC_SET_FRAME_RATE:"
				"bad state %d\n", g_info->state);
			errorCode = -EPERM;
		}
		errorCode = iav_update_vin(context, errorCode);
	}
		break;

	case IAV_IOC_VIN_SRC_GET_FRAME_RATE:
	{
		u32 		fps;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_FRAME_RATE,
			&fps);

		if (!errorCode) {
			errorCode = copy_to_user(
				(u32 __user *)args,
				&fps, sizeof(fps)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_SHUTTER_TIME:
	{
		u32		shutter = (u32)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_SHUTTER_TIME,
				&shutter);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_SET_SHUTTER_AND_AGC_SYNC:
	{
		struct sensor_cmd_info_t	info;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&info,
				(struct amba_vin_wdr_gain_gp_info __user *)args,
			sizeof(info)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_SHUTTER_AND_GAIN_SYNC,
				&info);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_GET_SHUTTER_WIDTH:
		{
		u32 	shutter;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&shutter,
				(u32 __user *)args,
			sizeof(u32)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH,
				&shutter);

			if (!errorCode) {
			errorCode = copy_to_user(
				(u32 __user *)args,
				&shutter, sizeof(u32)) ? -EFAULT : 0;
		}
		} else {
			errorCode = -EFAULT;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_GET_SHUTTER_TIME:
	{
		u32 		shutter_time;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_SHUTTER_TIME,
			&shutter_time);

		if (!errorCode) {
			errorCode = copy_to_user(
				(u32 __user *)args,
				&shutter_time, sizeof(shutter_time)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_AGC_DB:
	{
		int 		agc = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_GAIN_DB,
				&agc);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_AGC_DB:
	{
		int 		agc_db;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_GAIN_DB,
			&agc_db);

		if (!errorCode) {
			errorCode = copy_to_user(
				(s32 __user *)args,
				&agc_db, sizeof(agc_db)) ? -EFAULT : 0;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_SET_DGAIN_RATIO:
	{
		struct amba_vin_dgain_info		dgain;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&dgain,
				(struct amba_vin_dgain_info __user *)args,
			sizeof(dgain)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_DGAIN_RATIO,
				&dgain);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_DGAIN_RATIO:
	{
		struct amba_vin_dgain_info 		dgain;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_DGAIN_RATIO,
			&dgain);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_dgain_info __user *)args,
				&dgain, sizeof(dgain)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SET_TRIGGER_MODE:
	{
		struct amba_vin_trigger_mode	trigger_mode;

		if (g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL) {
			errorCode = copy_from_user(&trigger_mode,
				(struct amba_vin_trigger_mode*)args,
				sizeof(struct amba_vin_trigger_mode));
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_TRIGGER_MODE,
				&trigger_mode);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_GET_OPERATION_MODE:
	{
		int 	op_mode;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_OPERATION_MODE,
			&op_mode);

		if (!errorCode) {
			errorCode = copy_to_user(
				(s32 __user *)args,
				&op_mode, sizeof(op_mode)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SET_OPERATION_MODE:
	{
		int	op_mode = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
				(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_SET_OPERATION_MODE,
			&op_mode);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_MIRROR_MODE:
	{
		struct amba_vin_src_mirror_mode  mirror_mode;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&mirror_mode,
			(struct amba_vin_src_mirror_mode __user *)args,
				sizeof(mirror_mode)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_MIRROR_MODE,
				&mirror_mode);
		} else {
			errorCode = -EFAULT;
		}

		errorCode = iav_update_vin(context, errorCode);
	}
		break;

	case IAV_IOC_VIN_SRC_SET_ANTI_FLICKER:
	{
		int tmp = (int)args;

		if (g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO){
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_ANTI_FLICKER,
				&tmp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_EIS_INFO:
	{
		struct amba_vin_eis_info 		eis_info;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_EIS_INFO,
			&eis_info);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_eis_info __user *)args,
				&eis_info, sizeof(struct amba_vin_eis_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_AGAIN:
	{
		int 	agc = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_AGAIN,
				&agc);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_WDR_AGAIN:
	{
		int 	agc;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_WDR_AGAIN,
			&agc);

		if (!errorCode) {
			errorCode = copy_to_user(
				(s32 __user *)args,
				&agc, sizeof(agc)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_AGAIN_INDEX:
	{
		int 	agc = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX,
				&agc);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_DGAIN_GP:
	{
		struct amba_vin_wdr_gain_gp_info		dgain_gp;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&dgain_gp,
				(struct amba_vin_wdr_gain_gp_info __user *)args,
			sizeof(dgain_gp)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_DGAIN_GROUP,
				&dgain_gp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_WDR_DGAIN_GP:
	{
		struct amba_vin_wdr_gain_gp_info 		dgain_gp;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_WDR_DGAIN_GROUP,
			&dgain_gp);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_wdr_gain_gp_info __user *)args,
				&dgain_gp, sizeof(dgain_gp)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_DGAIN_INDEX_GP:
	{
		struct amba_vin_wdr_gain_gp_info		dgain_gp;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&dgain_gp,
				(struct amba_vin_wdr_gain_gp_info __user *)args,
			sizeof(dgain_gp)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP,
				&dgain_gp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_AGAIN_INDEX_GP:
	{
		struct amba_vin_wdr_gain_gp_info		again_gp;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&again_gp,
				(struct amba_vin_wdr_gain_gp_info __user *)args,
			sizeof(again_gp)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX_GROUP,
				&again_gp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_GP:
	{
		struct amba_vin_wdr_shutter_gp_info		shutter_gp;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&shutter_gp,
				(struct amba_vin_wdr_shutter_gp_info __user *)args,
			sizeof(shutter_gp)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_SHUTTER_GROUP,
				&shutter_gp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_ROW_GP:
	{
		struct amba_vin_wdr_shutter_gp_info		shutter_gp;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&shutter_gp,
				(struct amba_vin_wdr_shutter_gp_info __user *)args,
			sizeof(shutter_gp)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP,
				&shutter_gp);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_WDR_SHUTTER_GP:
	{
		struct amba_vin_wdr_shutter_gp_info 		shutter_gp;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_WDR_SHUTTER_GROUP,
			&shutter_gp);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_wdr_shutter_gp_info __user *)args,
				&shutter_gp, sizeof(shutter_gp)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_WDR_SHUTTER2ROW:
	{
		struct amba_vin_wdr_shutter_gp_info		shutter2row;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = copy_from_user(&shutter2row,
				(struct amba_vin_wdr_shutter_gp_info __user *)args,
			sizeof(shutter2row)) ? -EFAULT : 0;

			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_WDR_SHUTTER2ROW,
				&shutter2row);

			if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_wdr_shutter_gp_info __user *)args,
				&shutter2row, sizeof(shutter2row)) ? -EFAULT : 0;
		}
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_SENSOR_TEMP:
	{
		u64 		temperature;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_SENSOR_TEMPERATURE,
			&temperature);

		if (!errorCode) {
			errorCode = copy_to_user(
				(u64 __user *)args,
				&temperature, sizeof(temperature)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_SHUTTER_TIME_ROW:
	{
		u32		shutter_row = (u32)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW,
				&shutter_row);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC:
	{
		u32		shutter_row = (u32)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC,
				&shutter_row);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_SET_AGC_INDEX:
	{
		int 		agc_index = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_GAIN_INDEX,
				&agc_index);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;
	case IAV_IOC_VIN_SRC_SET_AGC_INDEX_SYNC:
	{
		int 		agc_index = (int)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_GAIN_INDEX_SYNC,
				&agc_index);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_STOP_SENSOR_STREAM:
	{
		struct amba_vin_clk_info adap_arg;
		struct amba_vin_source_info	src_info;

		/* do sensor reset to make sure sensor stop */
		errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_RESET,
				NULL);
		if (errorCode) {
			break;
		}

		errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_GET_INFO, &src_info);
		if (errorCode) {
			break;
		}
		/* stop CLK_SI to simulate vsync loss */
		adap_arg.mode = 0;
		adap_arg.so_freq_hz = 0;
		adap_arg.so_pclk_freq_hz = 0;
		errorCode = amba_vin_adapter_cmd(src_info.adapter_id,
			AMBA_VIN_ADAP_SET_VIN_CLOCK,
			&adap_arg);

		iav_printk("sensor stop!\n");
	}
		break;

	case IAV_IOC_VIN_SRC_SET_LOW_LIGHT_MODE:
	{
		u8 		ll_mode = (u8)args;

		if ((g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO) ||
			(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
			errorCode = amba_vin_source_cmd(
				context->g_info->pvininfo->active_src_id,
				context->active_channel_id,
				AMBA_VIN_SRC_SET_LOW_LIGHT_MODE,
				&ll_mode);
		} else {
			errorCode = -EFAULT;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_AAAINFO:
	{
		struct amba_vin_aaa_info aaa_info;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_AAA_INFO,
			&aaa_info);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_aaa_info __user *)args,
				&aaa_info, sizeof(aaa_info)) ? -EFAULT : 0;
		}
	}
		break;

	case IAV_IOC_VIN_SRC_GET_WDR_WIN_OFFSET:
	{
		struct amba_vin_wdr_win_offset 		wdr_win_offset;

		errorCode = amba_vin_source_cmd(
			context->g_info->pvininfo->active_src_id,
			context->active_channel_id,
			AMBA_VIN_SRC_GET_WDR_WIN_OFFSET,
			&wdr_win_offset);

		if (!errorCode) {
			errorCode = copy_to_user(
				(struct amba_vin_wdr_win_offset __user *)args,
				&wdr_win_offset, sizeof(wdr_win_offset)) ? -EFAULT : 0;
		}
	}
		break;

	default:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		DRV_PRINT("%s: do not support cmd %d!\n", __func__, cmd);
		break;
	}

	return errorCode;
}

int iav_vin_core_cmd(
	iav_context_t *context,
	unsigned int cmd,
	unsigned long args)
{
	int				errorCode = 0;
	struct iav_global_info		*g_info = context->g_info;

	switch (cmd) {
	case IAV_IOC_VIN_GET_SOURCE_NUM:
	{
		int			counter1, counter2;

		errorCode = amba_vin_adapter_cmd(0,
			AMBA_VIN_ADAP_GET_SOURCE_NUM, &counter1);
		errorCode |= amba_vin_adapter_cmd(1,
			AMBA_VIN_ADAP_GET_SOURCE_NUM, &counter2);
		if (!errorCode) {
			counter1 += counter2;
			errorCode = put_user(counter1, (int __user *)args);
		}
	}
		break;

	case IAV_IOC_VIN_GET_CURRENT_SRC:
	{
		int		srcid = context->g_info->pvininfo->active_src_id;
		errorCode = put_user(srcid, (int __user *)args);
	}
		break;

	case IAV_IOC_VIN_SET_CURRENT_SRC:
		if (g_info->state == IAV_STATE_IDLE) {
			int				srcid;
			struct amba_vin_source_info	src_info;

			errorCode = get_user(srcid, (int __user *)args);
			if (errorCode) {
				break;
			}

			errorCode = amba_vin_source_cmd(
				srcid,	context->active_channel_id,
				AMBA_VIN_SRC_GET_INFO, &src_info);
			if (errorCode) {
				break;
			}

			errorCode = amba_vin_adapter_cmd(src_info.adapter_id,
				AMBA_VIN_ADAP_SET_ACTIVE_SOURCE_ID,
				&srcid);
			if (errorCode)
				break;

			errorCode = amba_vin_adapter_cmd(src_info.adapter_id,
				AMBA_VIN_ADAP_GET_ACTIVE_SOURCE_ID,
				&context->g_info->pvininfo->active_src_id);
		} else {
			DRV_PRINT("IAV_IOC_VIN_SET_CURRENT_SRC:"
				"bad state %d\n", g_info->state);
			errorCode = -EPERM;
		}
		break;

	default:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		DRV_PRINT("%s do not support cmd %d!\n", __func__, cmd);
		break;
	}

	return errorCode;
}

int iav_vout_source_cmd(iav_context_t *context,
	unsigned int cmd, unsigned long args)
{
	int				errorCode = 0;

	switch (cmd) {
	case IAV_IOC_VOUT_GET_SINK_NUM:
	{
		int			counter = 0;
		int			tmp;

		errorCode = amba_vout_video_source_cmd(0,
			AMBA_VIDEO_SOURCE_GET_SINK_NUM, &tmp);
		if (errorCode)
			goto IAV_IOC_VOUT_GET_SINK_NUM_exit;

		counter += tmp;
		errorCode = amba_vout_video_source_cmd(1,
			AMBA_VIDEO_SOURCE_GET_SINK_NUM, &tmp);
		if (errorCode)
			goto IAV_IOC_VOUT_GET_SINK_NUM_exit;

		counter += tmp;
		if (!errorCode) {
			errorCode = put_user(counter, (int __user *)args);
		}
	}
IAV_IOC_VOUT_GET_SINK_NUM_exit:
		break;

	default:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		DRV_PRINT("amba_vout_source_cmd do not support cmd %d!\n", cmd);
		break;
	}

	return errorCode;
}

int iav_vout_sink_cmd(iav_context_t *context,
	unsigned int cmd, unsigned long args)
{
	int				errorCode = 0;

	switch (cmd) {
	case IAV_IOC_VOUT_GET_SINK_INFO:
	{
		struct amba_vout_sink_info	sink_info;

		errorCode = copy_from_user(&sink_info,
			(struct amba_vout_sink_info __user *)args,
			sizeof(struct amba_vout_sink_info)) ? -EFAULT : 0;
		if (errorCode)
			goto IAV_IOC_VOUT_GET_SINK_INFO_exit;

		errorCode = amba_vout_video_sink_cmd(sink_info.id,
			AMBA_VIDEO_SINK_GET_INFO, &sink_info);
		if (errorCode)
			goto IAV_IOC_VOUT_GET_SINK_INFO_exit;

		if (sink_info.source_id) {
			sink_info.state = G_iav_info.pvoutinfo[1]->enabled;
			sink_info.sink_mode = G_iav_info.pvoutinfo[1]->active_mode;

		} else {
			sink_info.state = G_iav_info.pvoutinfo[0]->enabled;
			sink_info.sink_mode = G_iav_info.pvoutinfo[0]->active_mode;
		}

		errorCode = copy_to_user(
			(struct amba_vout_sink_info __user *)args,
			&sink_info,
			sizeof(struct amba_vout_sink_info)) ? -EFAULT : 0;
	}
IAV_IOC_VOUT_GET_SINK_INFO_exit:
		break;

	default:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		DRV_PRINT("amba_vout_sink_cmd do not support cmd %d!\n", cmd);
		break;
	}

	return errorCode;
}

static long iav_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval = 0;

	iav_lock();

	switch (_IOC_TYPE(cmd)) {
	case IAV_IOC_VIN_MAGIC:
		rval = iav_vin_core_cmd(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_VIN_SRC_MAGIC:
		rval = iav_vin_src_cmd(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_VOUT_SOURCE_MAGIC:
		rval = iav_vout_source_cmd(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_VOUT_SINK_MAGIC:
		rval = iav_vout_sink_cmd(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_MAGIC:
		rval = __iav_ioctl(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_ENCODE_MAGIC:
		rval = __iav_encode_ioctl(filp->private_data, cmd, arg);
		break;

	case IAV_IOC_DECODE_MAGIC:
		rval = __iav_decode_ioctl(filp->private_data, cmd, arg);
		break;

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
	case IAV_IOC_IMAGE_MAGIC:
		rval = amba_imgproc_cmd(filp->private_data, cmd, arg);
		break;
#endif

	case IAV_IOC_OVERLAY_MAGIC:
		rval = __iav_overlay_ioctl(filp->private_data, cmd, arg);
		break;

	default:
		iav_error("Unknown cmd 0x%x\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	iav_unlock();

	return rval;
}

static int iav_do_map(iav_context_t *context,
	struct vm_area_struct *vma, int mmap_type,
	struct iav_addr_map *map)
{
	struct iav_mem_block *mem_block;
	unsigned long vma_size = vma->vm_end - vma->vm_start;

	iav_get_mem_block(mmap_type, &mem_block);

	if (context->g_info->noncached) {
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	}

	if (vma_size == mem_block->size) {
		if (remap_pfn_range(vma, vma->vm_start,
			mem_block->phys_start >> PAGE_SHIFT,
			mem_block->size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
		map->flags = 0;
	} else if (vma_size == mem_block->size * 2) {
		if (remap_pfn_range(vma, vma->vm_start,
			mem_block->phys_start >> PAGE_SHIFT,
			mem_block->size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
		if (remap_pfn_range(vma, vma->vm_start + mem_block->size,
			mem_block->phys_start >> PAGE_SHIFT,
			mem_block->size, vma->vm_page_prot)) {

			return -ENOMEM;
		}
		map->flags = IAV_MAP2;
	} else {
		return -EINVAL;
	}

	map->user_start = (u8*)vma->vm_start;
	map->user_end = (u8*)vma->vm_end;

	return 0;
}

static int iav_do_dsp_map(iav_context_t *context, struct vm_area_struct *vma)
{
	u32 phys_start = 0, size = 0;
	unsigned long vma_size = vma->vm_end - vma->vm_start;

	iav_get_dsp_partition(context->curr_partition, &phys_start, &size);
	if (context->g_info->noncached) {
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	}

	if (vma_size == size) {
		if (remap_pfn_range(vma, vma->vm_start,
			phys_start >> PAGE_SHIFT,
			size, vma->vm_page_prot)) {
			return -ENOMEM;
		}

	} else {
		return -EINVAL;
	}

	context->dsp_partition[context->curr_partition].user_start= (u8*)vma->vm_start;
	context->dsp_partition[context->curr_partition].user_end = (u8*)vma->vm_end;

	return 0;
}

static int iav_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	iav_context_t *context = filp->private_data;

	if (!context->g_info->called_by_ioc)
		iav_lock();

	switch (context->g_info->mmap_type) {
	case IAV_MMAP_BSB:
	case IAV_MMAP_BSB2:
		rval = iav_do_map(context, vma, IAV_MMAP_BSB2, &context->bsb);
		break;

	case IAV_MMAP_DSP:
		rval = iav_do_map(context, vma, IAV_MMAP_DSP, &context->dsp);
		break;

	case IAV_MMAP_FB:
		rval = iav_do_map(context, vma, IAV_MMAP_FB, &context->fb);
		break;

	case IAV_MMAP_USER:
		rval = iav_do_map(context, vma, IAV_MMAP_USER, &context->user);
		break;

	case IAV_MMAP_OVERLAY:
		rval = iav_do_map(context, vma, IAV_MMAP_OVERLAY, &context->overlay);
		break;

	case IAV_MMAP_PRIVACYMASK:
		rval = iav_do_map(context, vma, IAV_MMAP_PRIVACYMASK,
			&context->privacymask);
		break;

	case IAV_MMAP_IMGPROC:
		rval = iav_do_map(context, vma, IAV_MMAP_IMGPROC,
			&context->imgproc);
		break;

	case IAV_MMAP_QP_MATRIX:
		rval = iav_do_map(context, vma, IAV_MMAP_QP_MATRIX,
			&context->qp_matrix);
		break;

	case IAV_MMAP_MV:
		rval = iav_do_map(context, vma, IAV_MMAP_MV, &context->mv);
		break;

	case IAV_MMAP_QP_HIST:
		rval = iav_do_map(context, vma, IAV_MMAP_QP_HIST, &context->qp_hist);
		break;

	case IAV_MMAP_PHYS:
		rval = __iav_do_map_phys(context, vma);
		break;

	case IAV_MMAP_PHYS2:
		rval = __iav_do_map_phys_2times(context, vma);
		break;

	case IAV_MMAP_DSP_PARTITION:
		rval = iav_do_dsp_map(context, vma);
		break;

	default:
		LOG_ERROR("BAD mmap type %d in iav_mmap!\n", context->g_info->mmap_type);
		rval = -EPERM;
		break;
	}

	if (!context->g_info->called_by_ioc)
		iav_unlock();

	return rval;
}

static int iav_open(struct inode *inode, struct file *filp)
{
	iav_context_t * context = NULL;

	context = kzalloc(sizeof(iav_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}

	context->file = filp;
	context->mutex = &iav_mutex;
	context->g_info = &G_iav_info;
	filp->private_data = context;

	iav_lock();
	iav_encode_open(context);
	iav_unlock();

	return 0;
}

static int iav_release(struct inode *inode, struct file *filp)
{
	iav_context_t * context = (iav_context_t *)filp->private_data;

	iav_lock();
	iav_encode_release(context);
	iav_decode_release(context);
	iav_unlock();

	kfree(context);

	return 0;
}

static struct file_operations iav_fops = {
	.owner = THIS_MODULE,
	.open = iav_open,
	.release = iav_release,
	.mmap = iav_mmap,
	.unlocked_ioctl = iav_ioctl,
};

static int __init iav_create_dev(int major, int minor,
	const char *name, struct cdev *cdev, struct file_operations *fops)
{
	dev_t dev_id;
	int rval;

	dev_id = MKDEV(major, minor);

	rval = register_chrdev_region(dev_id, 1, name);
	if (rval) {
		iav_error("failed to get dev region for %s\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		iav_error("cdev_add failed for %s, error = %d\n", name, rval);
		unregister_chrdev_region(cdev->dev, 1);
		return rval;
	}

	iav_printk("%s dev init done, dev_id = %d:%d\n",
		name, MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static void iav_destroy_dev(struct cdev * cdev)
{
	cdev_del(cdev);
	unregister_chrdev_region(cdev->dev, 1);
}

static int iav_state_proc_read(char * page, char ** start,
	off_t off, int count, int * eof, void * data)
{
	int rval = 0;
	iav_state_info_t info;

	if (off != 0)
		goto iav_state_proc_exit;

	// Todo
	info.dsp_op_mode = 0;
	info.dsp_encode_state = 0;
	info.dsp_encode_mode = 0;
	info.dsp_decode_state = 0;
	info.decode_state = 0;
	info.encode_timecode = 0;
	info.encode_pts = 0;

	info.state = G_iav_info.state;

	rval += scnprintf(page + rval, count - rval,
		"IAV state: %d\n", G_iav_info.state);
	rval += scnprintf(page + rval, count - rval,
		"dsp_op_mode: %d\n", info.dsp_op_mode);
	rval += scnprintf(page + rval, count - rval,
		"dsp_encode_state: %d\n", info.dsp_encode_state);
	rval += scnprintf(page + rval, count - rval,
		"dsp_encode_mode: %d\n", info.dsp_encode_mode);
	rval += scnprintf(page + rval, count - rval,
		"dsp_decode_state: %d\n", info.dsp_decode_state);
	rval += scnprintf(page + rval, count - rval,
		"decode_state: %d\n", info.decode_state);
	rval += scnprintf(page + rval, count - rval,
		"encode timecode: %d\n", info.encode_timecode);
	rval += scnprintf(page + rval, count - rval,
		"encode pts: %d\n", info.encode_pts);

	*eof = 1;

iav_state_proc_exit:
	return rval;
}

static void __iav_exit(void)
{
	// Todo
	iav_mem_cleanup();
	iav_encode_exit();
	iav_destroy_dev(&iav_cdev);
	remove_proc_entry("iav_state", get_ambarella_proc_dir());
}

static int __init __iav_init(void)
{
	int rval = 0;
	struct proc_dir_entry *iav_state_proc = NULL;

	/* All memory allocation for IAV module */
	if ((rval = iav_mem_init()) < 0)
		return rval;

	if ((rval = iav_vout_init()) < 0)
		goto iav_init_error_free_mem;

	if ((rval = iav_debug_init()) < 0)
		goto iav_init_error_free_mem;

	if ((rval = iav_init(&G_iav_info, &iav_cdev)) < 0)
		goto iav_init_error_free_mem;

	if ((rval = iav_bufcap_init()) < 0)
		goto iav_init_error_free_mem;

	if ((rval = iav_obj_init()) < 0)
		goto iav_init_error_free_mem;

	//init encoder
	if ((rval = iav_encode_init()) < 0)
		goto iav_init_error_free_mem;

	//init decoder
	if ((rval = iav_decode_init(&iav_cdev)) < 0)
		goto iav_init_error_free_mem;

	// the /dev/ucode device
	if ((rval = iav_create_dev(UCODE_DEV_MAJOR, UCODE_DEV_MINOR,
		ucode_name, &ucode_cdev, &ucode_fops)) < 0) {
		goto iav_init_error;
	}

	// the /dev/iav device
	if ((rval = iav_create_dev(IAV_DEV_MAJOR, IAV_DEV_MINOR,
		iav_name, &iav_cdev, &iav_fops)) < 0) {
		goto iav_init_error;
	}

	G_iav_info.state = IAV_STATE_INIT;
	G_iav_info.dsp_owner = IAV_CORTEX;
	G_iav_info.map2times = 0;
	G_iav_info.duplex_ref_cnt = 0;
	G_iav_info.pvoutinfo[0] = &G_vout0info;
	G_iav_info.pvoutinfo[1] = &G_vout1info;
	G_iav_info.pvininfo = &G_vininfo;
	G_iav_info.high_mega_pixel_enable = 0;
	G_iav_info.hdr_mode = IAV_HDR_NONE;
	G_iav_info.vin_num = 1;

	iav_state_proc = create_proc_entry("iav_state", S_IRUGO,
		get_ambarella_proc_dir());
	if (iav_state_proc == NULL) {
		rval = -ENOMEM;
		iav_error("Failed to create proc file (iav_state)!\n");
		goto iav_init_error;
	}
	iav_state_proc->read_proc = iav_state_proc_read;

	return 0;

iav_init_error_free_mem:
	iav_mem_cleanup();
	return rval;

iav_init_error:
	__iav_exit();
	return rval;
}

module_init(__iav_init);
module_exit(__iav_exit);

