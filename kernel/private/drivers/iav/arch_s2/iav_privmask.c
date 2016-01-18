/**********************************************************
 * iav_privmask.c
 *
 * History:
 *
 *	2012/02/20 - [Jian Tang] create multiple buffers for Privacy Mask
 *	2013/08/10 - [Jian Tang] add PM support for MB and pixel level
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 **********************************************************/

#include <amba_common.h>
#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "amba_dsp.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_capture.h"
#include "iav_pts.h"
#include "iav_encode.h"
#include "iav_privmask.h"

#define LOCAL_EXPO_GAIN_IMPL_NUM   (4)

static iav_privacy_mask_setup_ex_t G_pm_setup;  // PM param configured by user
static iav_local_exposure_ex_t     G_le_setup;
static u32 G_le_gain_size;
static u8* G_le_gain_addr;      // LE gain from user
static u8* G_le_gain_impl_addr; // LE gain to dsp
static u32 G_le_comp_pm;

/******************************************
 *
 *	Internal helper functions
 *
 ******************************************/

static inline u32 is_valid_pm_user_addr(iav_context_t* context, u8* addr)
{
	return (addr >= context->privacymask.user_start) &&
		(addr < context->privacymask.user_end);
}

inline u32 pm_user_to_phy_addr(iav_context_t* context, u8* addr)
{
	struct iav_mem_block* privacymask;
	iav_get_mem_block(IAV_MMAP_PRIVACYMASK, &privacymask);
	return (u32)(addr - context->privacymask.user_start +
		privacymask->phys_start);
}

static inline u32 pm_user_to_kernel_addr(iav_context_t* context, u8* addr)
{
	struct iav_mem_block* privacymask;
	iav_get_mem_block(IAV_MMAP_PRIVACYMASK, &privacymask);
	return (u32)(addr - context->privacymask.user_start +
		privacymask->kernel_start);
}

static inline u16 get_pm_domain(void)
{
	switch (get_enc_mode()) {
	case DSP_MULTI_REGION_WARP_MODE:
		return IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT;
		break;
	case DSP_HIGH_MP_FULL_PERF_MODE:
		return  is_mctf_pm_enabled() ? IAV_PRIVACY_MASK_DOMAIN_STREAM :
			IAV_PRIVACY_MASK_DOMAIN_VIN;
		break;
	case DSP_MULTI_VIN_MODE:
	case DSP_HIGH_MP_WARP_MODE:
		return IAV_PRIVACY_MASK_DOMAIN_VIN;
		break;
	default:
		return is_mctf_pm_enabled() ? IAV_PRIVACY_MASK_DOMAIN_MAIN :
			IAV_PRIVACY_MASK_DOMAIN_VIN;
		break;
	}
}

static inline u16 get_pm_unit(void)
{
	return is_mctf_pm_enabled() ? IAV_PRIVACY_MASK_UNIT_MB :
		IAV_PRIVACY_MASK_UNIT_PIXEL;
//	return is_mctf_pm_enabled() ? IAV_PRIVACY_MASK_UNIT_MB :
//		(is_raw_capture_enabled() ?  IAV_PRIVACY_MASK_UNIT_PIXELRAW :
//			IAV_PRIVACY_MASK_UNIT_PIXEL);
}

static inline void get_pm_size_in_pixel(u16 domain, iav_reso_ex_t* pm_pixel)
{
	iav_rect_ex_t vin;
	switch (domain) {
	case IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT:
		pm_pixel->width = (G_cap_pre_main.input.width <= MAX_WIDTH_IN_FULL_FPS ?
			G_cap_pre_main.input.width : MAX_WIDTH_IN_FULL_FPS);
		pm_pixel->height = G_cap_pre_main.input.height;
		break;
	case IAV_PRIVACY_MASK_DOMAIN_VIN:
		get_vin_window(&vin);
		pm_pixel->width = vin.width * get_vin_num(get_enc_mode());
		pm_pixel->height = vin.height;
		break;
	case IAV_PRIVACY_MASK_DOMAIN_MAIN:
	default:
		break;
	}
}

static inline u16 get_buffer_pitch_in_pixel(u16 unit, int pixel_count_in_row)
{
	if (unit == IAV_PRIVACY_MASK_UNIT_PIXEL) {
		// align to 32 bytes
		return ALIGN(ALIGN(pixel_count_in_row , 32) / 32 * 27, 32);
	} else if (unit == IAV_PRIVACY_MASK_UNIT_PIXELRAW) {
		// 2 bytes for each pixel and align to 32 bytes
		return ALIGN(pixel_count_in_row * 2, 32);
	}
	return 0;
}

static int enable_default_local_exposure(void)
{
	int i;
	u16* addr = (u16*)G_le_gain_addr;
	for (i = 0; i < LOCAL_EXPO_GAIN_NUM; ++i, ++addr) {
		*addr = 1024;
	}
	memset(&G_le_setup, 0, sizeof(G_le_setup));
	G_le_setup.enable = 1;
	G_le_setup.radius = 0;
	G_le_setup.luma_weight_blue = 16;
	G_le_setup.luma_weight_green = 16;
	G_le_setup.luma_weight_red = 16;
	G_le_setup.luma_weight_sum_shift = 2;
	return 0;
}

static u8* get_next_impl_address(void)
{
	static int index = 0;
	index = (index + 1) % LOCAL_EXPO_GAIN_IMPL_NUM;
	return G_le_gain_impl_addr + index *  G_le_gain_size;
}

void cmd_set_privacy_mask(iav_context_t* context)
{
	u32 addr = 0;
	VCAP_SET_PRIVACY_MASK_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_SET_PRIVACY_MASK;

	if (G_pm_setup.enable) {
		addr = pm_user_to_phy_addr(context, G_pm_setup.buffer_addr);
		dsp_cmd.enabled_flags_dram_addr = PHYS_TO_DSP(addr);
	}

	/* --- FIXME ---
	 * Do not clear dram pitch when PM is not enabled,
	 * because the dram pitch param would reach MCTF control
	 * block in a different timing than enable flag
	 */
	dsp_cmd.enabled_flags_dram_pitch = G_pm_setup.buffer_pitch;

	dsp_cmd.U = G_pm_setup.u;
	dsp_cmd.V = G_pm_setup.v;
	dsp_cmd.Y = G_pm_setup.y;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, enabled_flags_dram_addr);
	iav_dsp(dsp_cmd, enabled_flags_dram_pitch);
	iav_dsp(dsp_cmd, Y);
	iav_dsp(dsp_cmd, U);
	iav_dsp(dsp_cmd, V);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

void cmd_set_local_exposure(void)
{

	local_exposure_t dsp_cmd;
	int i;
	u8* impl_addr = get_next_impl_address();
	u16* src = (u16*)G_le_gain_addr;
	u16* dst = (u16*)impl_addr;

	if (G_le_comp_pm) {
		for (i = 0; i < LOCAL_EXPO_GAIN_NUM; ++i, ++src, ++dst) {
			*dst = (*src << 1);
		}
	} else {
		memcpy(dst, src, G_le_gain_size);
	}
	clean_cache_aligned(impl_addr, G_le_gain_size);


	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = LOCAL_EXPOSURE;
	dsp_cmd.enable = G_le_setup.enable;
	dsp_cmd.group_index = G_le_setup.group_index;
	dsp_cmd.radius = G_le_setup.radius;
	dsp_cmd.luma_weight_red = G_le_setup.luma_weight_red;
	dsp_cmd.luma_weight_green = G_le_setup.luma_weight_green;
	dsp_cmd.luma_weight_blue = G_le_setup.luma_weight_blue;
	dsp_cmd.luma_weight_sum_shift = G_le_setup.luma_weight_sum_shift;
	dsp_cmd.gain_curve_table_addr = VIRT_TO_DSP(impl_addr);
	dsp_cmd.luma_offset = G_le_setup.luma_offset;

//	iav_dsp_hex(dsp_cmd, cmd_code);
//	iav_dsp(dsp_cmd, enable);
//	iav_dsp(dsp_cmd, group_index);
//	iav_dsp(dsp_cmd, radius);
//	iav_dsp(dsp_cmd, luma_weight_red);
//	iav_dsp(dsp_cmd, luma_weight_green);
//	iav_dsp(dsp_cmd, luma_weight_blue);
//	iav_dsp(dsp_cmd, luma_weight_sum_shift);
//	iav_dsp_hex(dsp_cmd, gain_curve_table_addr);
//	iav_dsp(dsp_cmd, luma_offset);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static int check_privacy_mask_in_pixel(iav_context_t* context,
	iav_privacy_mask_setup_ex_t* mask_setup)
{
	iav_reso_ex_t pm_pixel = {0};

	if (mask_setup->enable) {
		if (!is_valid_pm_user_addr(context, mask_setup->buffer_addr)) {
			iav_error("invalid privacy mask address %p.\n", mask_setup->buffer_addr);
			return -1;
		}

		iav_no_check();

		if (check_full_fps_full_perf_cap("PM") < 0) {
			return -1;
		}
		if (is_video_freeze_enabled()) {
			iav_error("Cannot run PM when video freeze is enabled!\n");
			return -1;
		}

		get_pm_size_in_pixel(get_pm_domain(), &pm_pixel);
		if (mask_setup->buffer_pitch !=
			get_buffer_pitch_in_pixel(get_pm_unit(), pm_pixel.width)) {
			iav_error("Wrong buffer pitch %d.\n", mask_setup->buffer_pitch);
			return -1;
		}
		if (mask_setup->buffer_height != pm_pixel.height) {
			iav_error("Wrong buffer height %d.\n", mask_setup->buffer_height);
			return -1;
		}
		if (mask_setup->y != 16 || mask_setup->u != 128
			|| mask_setup->v != 128) {
			mask_setup->y = 16;
			mask_setup->u = 128;
			mask_setup->v = 128;
			iav_printk("Privacy mask in pixel unit supports black only.\n");
		}
	}

	return 0;
}

int check_stream_privacy_mask_param(iav_context_t* context,
	iav_stream_privacy_mask_t* param)
{
	if (!is_high_mp_full_perf_mode()) {
		iav_error("Stream PM must be applied in [High MP FP] mode.\n");
		return -1;
	}
	if (is_video_freeze_enabled()) {
		iav_error("Cannot run PM when video freeze is enabled!\n");
		return -1;
	}
	return 0;
}

/******************************************
 *
 *	External helper functions
 *
 ******************************************/
inline int get_MB_num(int pixel)
{
	return (ALIGN(pixel, PIXEL_IN_MB) / PIXEL_IN_MB);
}

inline int get_buffer_pitch_in_MB(int MB_count_in_row)
{
	return ALIGN(((MB_count_in_row) * sizeof(u32)), 32);
}

int check_privacy_mask_in_MB(iav_context_t* context,
	iav_privacy_mask_setup_ex_t* mask_setup)
{
	u32 row_pitch = 0;
	u8* addr, *src, *dst;

	if (mask_setup->enable) {
		if (!is_valid_pm_user_addr(context, mask_setup->buffer_addr)) {
			iav_error("invalid privacy mask address %p.\n", mask_setup->buffer_addr);
			return -1;
		}
		/*
		 * Repeat last row pitch if sharpen B is enabled. It will make privacy
		 * mask shift 4x4 pixel to up-left corner.
		 */
		if (is_sharpen_b_enabled()) {
			addr = (u8 *) pm_user_to_kernel_addr(context,
			    mask_setup->buffer_addr);
			src = addr
			    + mask_setup->buffer_pitch * (mask_setup->buffer_height - 1);
			dst = src + mask_setup->buffer_pitch;
			memcpy(dst, src, mask_setup->buffer_pitch);
			iav_printk("Sharpen B enabled, repeat last MB row !\n");
		}

		iav_no_check();

		if (check_full_fps_full_perf_cap("PM") < 0) {
			return -1;
		}
		if (is_video_freeze_enabled()) {
			iav_error("Cannot run PM when video freeze is enabled!\n");
			return -1;
		}

		row_pitch = get_buffer_pitch_in_MB(
			get_MB_num(G_iav_obj.source_buffer[0].size.width));
		// if privacy mask setup, check buffer pitch/height with main buffer
		if (mask_setup->buffer_pitch != row_pitch) {
			iav_error("Wrong buffer pitch %d.\n", mask_setup->buffer_pitch);
			return -1;
		}
		if (mask_setup->buffer_height !=
			get_MB_num(G_iav_obj.source_buffer[0].size.height)) {
			iav_error("Wrong buffer height %d.\n", mask_setup->buffer_height);
			return -1;
		}
	}

	return 0;
}

int check_hdr_pm_width(void)
{
	iav_rect_ex_t vin;

	if (!is_mctf_pm_enabled()) {
		get_vin_window(&vin);
		if (vin.width > MAX_WIDTH_IN_FULL_FPS) {
			iav_error("VIN width [%d] CANNOT be wider than [%d] in HDR "
				"based PM method. Please either use MCTF based PM or smaller"
				" VIN.\n", vin.width, MAX_WIDTH_IN_FULL_FPS);
			return -1;
		}
	}

	return 0;
}

void set_digital_zoom_privacy_mask(iav_context_t* context,
	iav_digital_zoom_privacy_mask_ex_t* dptz_pm)
{
	G_pm_setup = dptz_pm->privacy_mask;
	//Guarantee privacy mask and dptz I commands are issued in the same Vsync.
	dsp_start_cmdblk(DSP_CMD_PORT_VCAP);
	cmd_warp_control(&dptz_pm->zoom.input);
	cmd_set_privacy_mask(context);
	dsp_end_cmdblk(DSP_CMD_PORT_VCAP);
}

int reset_privacy_mask(void)
{
	struct iav_mem_block* pm;

	if (is_mctf_pm_enabled()) {
		iav_get_mem_block(IAV_MMAP_PRIVACYMASK, &pm);
		memset(pm->kernel_start, 0, pm->size);
	}
	memset(&G_pm_setup, 0, sizeof(G_pm_setup));
	memset(&G_le_setup, 0, sizeof(G_le_setup));
	memset(G_le_gain_addr, 0, G_le_gain_size);
	G_le_comp_pm = 0;
	return 0;
}

int mem_alloc_local_exposure(u8 ** ptr,
	int *alloc_size)
{
	u8* addr = NULL;
	int total_size;

	G_le_gain_size = LOCAL_EXPO_GAIN_NUM * sizeof(u16);
	total_size = G_le_gain_size * (1 + LOCAL_EXPO_GAIN_IMPL_NUM);

	if ((addr = kzalloc(total_size, GFP_KERNEL)) == NULL) {
		iav_error("Not enough memory to allocate local exposure gain!\n");
		return -1;
	}
	*ptr = addr;
	*alloc_size = total_size;
	G_le_gain_addr = addr;
	G_le_gain_impl_addr = addr + G_le_gain_size;
	return 0;
}

int sync_le_with_pm(void)
{
	/*
	 * Issue local exposure command when
	 * 1) Enable PM while LE compensation has not been set
	 * 2) Disable PM while LE compensation has been set
	 */
	if (get_pm_unit() == IAV_PRIVACY_MASK_UNIT_PIXEL) {
		if ((G_le_comp_pm && !G_pm_setup.enable)
		    || (!G_le_comp_pm && G_pm_setup.enable)) {
			if (G_pm_setup.enable && !G_le_setup.enable) {
				enable_default_local_exposure();
			}
			G_le_comp_pm = G_pm_setup.enable;
			return 1;
		}
	}
	return 0;
}

/******************************************
 *
 *	External IAV IOCTLs functions
 *
 ******************************************/

int iav_set_privacy_mask_ex(iav_context_t *context,
	iav_privacy_mask_setup_ex_t __user *arg)
{
	iav_privacy_mask_setup_ex_t mask_setup;
	int sync_le;

	memset(&mask_setup, 0, sizeof(mask_setup));
	if (copy_from_user(&mask_setup, arg, sizeof(mask_setup)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set privacy mask when not in preview/encoding state\n");
		return -EPERM;
	}

	if (is_mctf_pm_enabled()) {
		iav_error("Cannot set privacy mask only in [%s] mode. "
			"Please call IAV_IOCTL_SET_DIGITAL_ZOOM_PRIVACY_MASK.\n",
			G_modes_limit[get_enc_mode()].name);
		return -EPERM;
	}

	if (check_privacy_mask_in_pixel(context, &mask_setup) < 0) {
		return -EINVAL;
	}

	/*
	 * LE compensates the luma change brought by pixel privacy mask.
	 * Save PM parameters before deciding whether LE compensation occurs.
	 */
	G_pm_setup = mask_setup;
	sync_le = sync_le_with_pm();

	if (sync_le) {
		dsp_start_cmdblk(DSP_CMD_PORT_VCAP);
		cmd_set_local_exposure();
	}
	cmd_set_privacy_mask(context);
	if (sync_le) {
		dsp_end_cmdblk(DSP_CMD_PORT_VCAP);
	}


	return 0;
}

int iav_get_privacy_mask_ex(iav_context_t * context,
	struct iav_privacy_mask_setup_ex_s __user * arg)
{
	iav_privacy_mask_setup_ex_t  pm;
	get_privacy_mask_param(&pm);
	return copy_to_user(arg, &pm, sizeof(pm)) ? -EFAULT : 0;
}

int iav_get_privacy_mask_info_ex(iav_context_t *context,
	struct iav_privacy_mask_info_ex_s __user *arg)
{
	iav_privacy_mask_info_ex_t mask_info;
	iav_reso_ex_t pm_pixel  = {0};

	memset(&mask_info, 0, sizeof(iav_privacy_mask_info_ex_t));

	mask_info.domain = get_pm_domain();
	mask_info.unit = get_pm_unit();

	switch(mask_info.unit) {
		case IAV_PRIVACY_MASK_UNIT_PIXEL:
		case IAV_PRIVACY_MASK_UNIT_PIXELRAW:
			get_pm_size_in_pixel(mask_info.domain, &pm_pixel);
			mask_info.pixel_width = pm_pixel.width;
			mask_info.buffer_pitch = get_buffer_pitch_in_pixel(mask_info.unit,
				mask_info.pixel_width);
			break;
		case IAV_PRIVACY_MASK_UNIT_MB:
		default:
			mask_info.pixel_width = 0;
			mask_info.buffer_pitch = get_buffer_pitch_in_MB(
				get_MB_num(G_iav_obj.source_buffer[0].size.width));
			break;
	}
	return copy_to_user(arg, &mask_info, sizeof(mask_info)) ? -EFAULT : 0;
}

int iav_get_local_exposure_ex(iav_context_t *context,
	struct iav_local_exposure_ex_s __user *arg)
{
	iav_local_exposure_ex_t  le;
	memset(&le, 0, sizeof(le));
	if (copy_from_user(&le.gain_curve_table_addr, &arg->gain_curve_table_addr,
		sizeof(u32))) {
		return -EFAULT;
	}
	if (!le.gain_curve_table_addr) {
		iav_error("Invalid local exposure gain curve table addr 0x%x.\n",
			le.gain_curve_table_addr);
		return -EINVAL;
	}

	if (copy_to_user((void *)le.gain_curve_table_addr, G_le_gain_addr,
		G_le_gain_size)) {
		return -EFAULT;
	}
	le.enable = G_le_setup.enable;
	le.group_index = G_le_setup.group_index;
	le.radius = G_le_setup.radius;
	le.luma_weight_red = G_le_setup.luma_weight_red;
	le.luma_weight_green = G_le_setup.luma_weight_green;
	le.luma_weight_blue = G_le_setup.luma_weight_blue;
	le.luma_weight_sum_shift = G_le_setup.luma_weight_sum_shift;
	le.luma_offset = G_le_setup.luma_offset;

	return copy_to_user(arg, &le, sizeof(le)) ? -EFAULT : 0;
}

int iav_set_local_exposure_ex(iav_context_t *context,
	struct iav_local_exposure_ex_s __user *arg)
{
	iav_local_exposure_ex_t le;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set local exposure when not in preview/encoding state\n");
		return -EPERM;
	}

	memset(&le, 0, sizeof(le));
	if (copy_from_user(&le, arg, sizeof(le)))
		return -EFAULT;
	if (le.enable && !le.gain_curve_table_addr) {
		iav_error("invalid gain curve table address 0x%x\n",
			le.gain_curve_table_addr);
		return -EINVAL;
	}
	if (copy_from_user(G_le_gain_addr, (void *)le.gain_curve_table_addr,
		G_le_gain_size)) {
		return -EFAULT;
	}
	G_le_setup.enable = le.enable;
	G_le_setup.group_index = le.group_index;
	G_le_setup.radius = le.radius;
	G_le_setup.luma_weight_red = le.luma_weight_red;
	G_le_setup.luma_weight_green = le.luma_weight_green;
	G_le_setup.luma_weight_blue = le.luma_weight_blue;
	G_le_setup.luma_weight_sum_shift = le.luma_weight_sum_shift;
	G_le_setup.gain_curve_table_addr = le.gain_curve_table_addr;
	G_le_setup.luma_offset = le.luma_offset;

	cmd_set_local_exposure();
	return 0;
}

int get_privacy_mask_param(iav_privacy_mask_setup_ex_t* pm_setup)
{
	memset(pm_setup, 0, sizeof(iav_privacy_mask_setup_ex_t));
	*pm_setup = G_pm_setup;
	return 0;
}

int cfg_privacy_mask_param(iav_context_t* context,
	iav_privacy_mask_setup_ex_t* pm_setup)
{
	int rval = (get_pm_unit() == IAV_PRIVACY_MASK_UNIT_MB ?
		check_privacy_mask_in_MB(context, pm_setup) :
		check_privacy_mask_in_pixel(context, pm_setup));

	if (!rval) {
		G_pm_setup = *pm_setup;
	}

	return rval;
}

int get_stream_privacy_mask_param(iav_stream_privacy_mask_t* param)
{
	int stream_id;
	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	if (stream_id >= IAV_MAX_ENCODE_STREAMS_NUM) {
		iav_error("Invalid stream id [%d]\n", stream_id);
		return -1;
	}
	*param = G_encode_stream[stream_id].pm;
	return 0;
}

int cfg_stream_privacy_mask_param(iav_context_t* context,
	iav_stream_privacy_mask_t* param)
{
	int stream_id;
	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	if (check_stream_privacy_mask_param(context, param) < 0) {
		return -1;
	}
	G_encode_stream[stream_id].pm = *param;
	return 0;
}

