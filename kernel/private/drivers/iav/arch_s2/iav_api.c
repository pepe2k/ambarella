/*
 * iav_api.c
 *
 * History:
 *	2012/05/12 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
#include "amba_dsp.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_mem_perf.h"
#include "iav_privmask.h"
#include "iav_warp.h"
#include "iav_netlink.h"
#include "iav_pts.h"
#include "iav_encode.h"
#include "iav_capture.c"
#include "iav_bufcap.h"

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_imgproc.h"
#endif
#ifdef CONFIG_IAV_CONTROL_AAA
extern iav_nl_image_config_t G_nl_image_config;
#endif

/******************************************
 *	Internal helper functions
 ******************************************/

static int iav_dsp_init(void)
{
	int rval = 0;
	if (dsp_check_status()) {
		if (dsp_hot_init() == 0) {
			G_iav_obj.op_mode = DSP_OP_MODE_IDLE;
			G_iav_obj.dsp_encode_mode = -1;
			G_iav_obj.dsp_encode_state = -1;
			G_iav_obj.dsp_encode_state2 = -1;
			G_iav_obj.dsp_decode_state = 0,
				G_iav_obj.decode_state = -1,
				G_iav_info.state = IAV_STATE_PREVIEW;
		} else if (dsp_hot_init() == 1) {
			G_iav_obj.op_mode = DSP_OP_MODE_IP_CAMERA_RECORD;
			G_iav_obj.dsp_decode_state = 0,
				G_iav_obj.decode_state = -1,
				G_iav_info.state = IAV_STATE_DECODING;
		} else {
			return -1;
		}
	} else {
		if ((rval = dsp_init()) < 0) {
			return rval;
		}
	}
	G_iav_info.dsp_booted = 1;
	return 0;
}

static int iav_setup_irq(void *dev)
{
	int rval;

	rval = dsp_init_irq(dev);
	if (rval < 0) {
		iav_error("iav_setup_irq error \n");
		return -1;
	}

	return rval;
}

static int iav_system_event(struct notifier_block *nb,
		unsigned long val,
		void *data)
{
	int errorCode = NOTIFY_OK;

	iav_printk("%s:0x%lx\n", __func__, val);

	return errorCode;
}

int iav_debug_init(void)
{
	struct iav_bits_desc *bs_desc = NULL;

	if (ambarella_debug_info == 0)
		return 0;

	iav_get_bits_desc(&bs_desc);
	G_iav_debug_info = (struct iav_debug_info *) ambarella_debug_info;
	G_iav_debug_info->global_info_addr = virt_to_phys(&G_iav_info);
	G_iav_debug_info->bs_desc_base = virt_to_phys(bs_desc->start);
	G_iav_debug_info->bs_desc_size = (u32) bs_desc->end - (u32) bs_desc->start;
	G_iav_debug_info->bsb_mem_addr = get_ambarella_bsbmem_phys();
	G_iav_debug_info->bsb_mem_size = get_ambarella_bsbmem_size();
	G_iav_debug_info->dsp_mem_addr = get_ambarella_dspmem_phys();
	G_iav_debug_info->dsp_mem_size = get_ambarella_dspmem_size()
		- dsp_get_ucode_size();
	return 0;
}

int iav_obj_init(void)
{
	int i;
	int encode_mode = get_enc_mode();

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		G_iav_obj.dsp_encode_state_ex[i] = ENC_IDLE_STATE;
	}
	G_iav_obj.source_buffer = &G_source_buffer[0];
	G_iav_obj.system_resource = &G_system_resource_setup[encode_mode];
	G_iav_obj.system_setup_info = &G_system_setup_info[encode_mode];

	G_iav_obj.system_event.notifier_call = iav_system_event;
	ambarella_register_event_notifier(&G_iav_obj.system_event);

	G_iav_obj.dsp_chip_id = IAV_CHIP_ID_S2_UNKNOWN;
	G_iav_obj.dsp_noncached = 0;

	spin_lock_init(&G_iav_obj.lock);
	init_waitqueue_head(&G_iav_obj.vcap_wq);
	init_waitqueue_head(&G_iav_obj.vout_b_update_wq);

	return 0;
}

static void calc_preview_param(iav_context_t *context,
		iav_preview_format_t *param)
{
#if 0
	iav_encode_format_t format;
	get_encode_format(&format);

	if (format.secondary_encode_type != IAV_ENCODE_NONE) {
		param->width = format.secondary_width;
		param->height = format.secondary_height;
		param->format = 0;
		param->frame_rate = amba_iav_fps_to_fps(G_iav_info.pvoutinfo[1]->video_info.fps);
	} else if (format.specify_preview) {
		param->width = format.preview_width;
		param->height = format.preview_height;
		param->format = 0;
		param->frame_rate = format.preview_framerate;
	} else {
		param->width = G_iav_info.pvoutinfo[1]->video_info.width;
		param->height = G_iav_info.pvoutinfo[1]->video_info.height;
		param->format = amba_iav_format_to_format(G_iav_info.pvoutinfo[1]->video_info.format);
		param->frame_rate = amba_iav_fps_to_fps(G_iav_info.pvoutinfo[1]->video_info.fps);
	}
#endif
}

static inline int get_single_buffer_num(iav_buffer_id_t buffer_id,
		int * buffer_num)
{
	int i;
	if (is_multi_buffer_id(buffer_id)) {
		iav_error("No multiple id [0x%x] support in this IOCTL.\n", buffer_id);
		return -1;
	}
	for (i = 0; i < IAV_MAX_SOURCE_BUFFER_NUM; ++i) {
		if (buffer_id & (1 << i))
			break;
	}
	if (i == IAV_MAX_SOURCE_BUFFER_NUM) {
		iav_error("No source buffer num found from id [0x%x]!\n", buffer_id);
		return -1;
	}
	*buffer_num = i;
	return 0;
}

static inline int get_max_encode_buffer_size(int buffer_id,
	iav_reso_ex_t* max)
{
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD* resource =
		&G_system_resource_setup[G_dsp_enc_mode];
	u32 max_w, max_h;
	switch (buffer_id) {
	case IAV_ENCODE_SOURCE_MAIN_BUFFER:
		max_w = resource->max_main_width;
		max_h = resource->max_main_height;
		break;
	case IAV_ENCODE_SOURCE_SECOND_BUFFER:
		max_w = resource->max_preview_C_width;
		max_h = resource->max_preview_C_height;
		break;
	case IAV_ENCODE_SOURCE_THIRD_BUFFER:
		max_w = resource->max_preview_B_width;
		max_h = resource->max_preview_B_height;
		break;
	case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
		max_w = resource->max_preview_A_width;
		max_h = resource->max_preview_A_height;
		break;
	case IAV_ENCODE_SOURCE_MAIN_DRAM:
		max_w = G_source_buffer[buffer_id].dram.buf_max_size.width;
		max_h = G_source_buffer[buffer_id].dram.buf_max_size.height;
		break;
	default:
		iav_error("Invalid source buffer id [%d].\n", buffer_id);
		return -1;
		break;
	}
	if (max) {
		max->width = max_w;
		max->height = max_h;
	}
	return 0;
}


/*
 *  Check the property about source buffer type.
 */

static int check_source_buffer_type(int buffer_id,
		iav_source_buffer_type_ex_t type)
{
	switch (buffer_id) {
	case IAV_ENCODE_SOURCE_MAIN_BUFFER:
		if (type == IAV_SOURCE_BUFFER_TYPE_OFF ||
			type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
			iav_error("Source buffer [%d] type cannot be OFF or PREVIEW.\n",
				buffer_id);
			return -1;
		}
		break;
	case IAV_ENCODE_SOURCE_SECOND_BUFFER:
		if (type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
			iav_error("Source buffer [%d] type cannot be PREVIEW.\n",
				buffer_id);
			return -1;
		}
		break;
	case IAV_ENCODE_SOURCE_THIRD_BUFFER:
		if (is_warp_mode() &&
			type != IAV_SOURCE_BUFFER_TYPE_OFF) {
			iav_error("Source buffer [%d] is unused in [%s] mode.\n",
				buffer_id, G_modes_limit[G_dsp_enc_mode].name);
		}
		break;
	case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
		break;
	case IAV_ENCODE_SOURCE_MAIN_DRAM:
		if (type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
			iav_error("DRAM buffer [%d] type cannot be PREVIEW.\n", buffer_id);
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

/*
 *  Check the property about main / pre-main buffer size vs. hard limit.
 */
static int check_main_buffer_size(u32 is_pre_main,
		iav_reso_ex_t* main_buffer)
{
	u16 max_width, max_height;
	iav_reso_ex_t* orig = is_pre_main ? &G_cap_pre_main.size :
		&G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
	iav_source_buffer_state_ex_t state =
		G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].state;

	// error if main buffer is zero
	if (!main_buffer->width || !main_buffer->height) {
		iav_error("%s size %dx%d cannot be zero.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_buffer->width, main_buffer->height);
		return -1;
	}

	// error if not aligned
	if ((main_buffer->width & 0xF) || (main_buffer->height & 0x7)) {
		iav_error("%s width %d must be multiple of 16 and"
			" height %d must be multiple of 8.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_buffer->width, main_buffer->height);
		return -1;
	}

	if (is_pre_main) {
		// error if pre main size > limit
		if (is_multi_region_warp_mode()) {
			max_width = MAX_PRE_WIDTH_IN_WARP;
			max_height = MAX_PRE_HEIGHT_IN_WARP;
		} else {
			max_width = MAX_PRE_WIDTH_IN_HIGH_MP_WARP;
			max_height = MAX_PRE_HEIGHT_IN_HIGH_MP_WARP;
		}
		max_width = MIN(G_cap_pre_main.property.max.width, max_width);
		max_height = MIN(G_cap_pre_main.property.max.height, max_height);
		if ((main_buffer->width > max_width)
				|| (main_buffer->height > max_height)) {
			iav_error("Pre main buffer %dx%d is larger than limit %dx%d "
				"in [%s] mode.\n", main_buffer->width, main_buffer->height,
				max_width, max_height, G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}

	// error if iav is not idle or source buffer is busy
	if ((main_buffer->width != orig->width)
			|| (main_buffer->height != orig->height)) {
		if (!is_iav_state_idle()) {
			iav_error("Cannot change %s size while IAV is not in IDLE.\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]");
			return -1;
		}
		if (state == IAV_SOURCE_BUFFER_STATE_BUSY) {
			iav_error("Cannot change %s size while it's used by the encoder.\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]");
			return -1;
		}
	}

	return 0;
}

/*
 * Apply the default value for main / pre-main input window.
 * Check the property about (pre-)main buffer input vs. (hard limit, vin).
 */
static int check_main_buffer_input(u32 is_pre_main,
		iav_rect_ex_t* main_input,
		iav_rect_ex_t* vin)
{
	iav_rect_ex_t* orig;

	// default input window is VIN
	if (!main_input->width) {
		main_input->width = vin->width;
	}

	if (!main_input->height) {
		main_input->height = vin->height;
	}

	// error if main input window is not aligned
	if (main_input->y & 0x1) {
		iav_error("%s input window offset y %d is not multiple of 2.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->y);
		return -1;
	}
	if (main_input->height & 0x3) {
		iav_error("%s input window height %d is not multiple of 4.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->height);
		return -1;
	}

	if (main_input->width & 0x3) {
		iav_error("%s input window width %d is not multiple of 4.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->width);
		return -1;
	}
	if (main_input->x & 0x1) {
		iav_error("%s input window offset x %d is not multiple of 2.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->x);
		return -1;
	}

	// error if main input window > VIN
	if ((main_input->width + main_input->x > vin->width)
			|| (main_input->height + main_input->y > vin->height)) {
		iav_error("%s input window %dx%d + offset %dx%d "
			"cannot be larger than VIN %dx%d.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->width, main_input->height, main_input->x,
			main_input->y, vin->width, vin->height);
		return -1;
	}

	if (is_pre_main) {
		// error if iav is not idle when changing pre main window
		orig = &G_cap_pre_main.input;
		if ((main_input->width != orig->width)
				|| (main_input->height != orig->height)) {
			if (!is_iav_state_idle()) {
				iav_error("Cannot change pre-main input window "
					"while IAV is not in IDLE state.\n");
				return -1;
			}
		}
	}
	return 0;
}

/*
 *  Check the property about zoom factor between (pre-)main buffer size
 *  and input window.
 */
static int check_main_buffer_zoom(u32 is_pre_main,
	iav_reso_ex_t* main_buffer, iav_rect_ex_t* main_input)
{
	char msg[32];

	if (!main_input->width || !main_input->height) {
		iav_error("%s input window is invalid.\n",
				is_pre_main ? "Pre main buffer" : "Source buffer [0]");
		return -1;
	}

	// error if zoom factor exceeds the limit
	if (is_pre_main) {
		sprintf(msg, "%s", "Pre main buffer");
	} else {
		sprintf(msg, "%s", "Source buffer [0]");
	}
	if (check_zoom_property(IAV_ENCODE_SOURCE_MAIN_BUFFER,
		main_input->width, main_input->height,
		main_buffer->width, main_buffer->height, msg) < 0) {
		return -1;
	}

	if (is_warp_mode() || is_full_fps_full_perf_mode()) {
		// error if main input window height < main buffer height
		if (main_input->height < main_buffer->height) {
			iav_error("%s input window height %d cannot be smaller than "
				"its height %d in [%s] mode.\n",
				is_pre_main ? "Pre main buffer" : "Source buffer [0]",
				main_input->height, main_buffer->height,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}
	return 0;
}

/*
 * Check the property about rsub source buffer size vs. hard limit.
 */
static int check_sub_buffer_size(u32 buffer_id, iav_reso_ex_t* size)
{
	iav_reso_ex_t max = { 0 };
	iav_reso_ex_t* orig = &G_source_buffer[buffer_id].size;
	iav_source_buffer_state_ex_t state = G_source_buffer[buffer_id].state;
	iav_enc_mode_limit_t * limit = &G_modes_limit[get_enc_mode()];

	if (size->width && size->height) {
		// error if it is not aligned
		if ((size->width & 0xF) || (size->height & 0x7)) {
			iav_error("Source buffer %d [%dx%d], width must be multiple of "
				"16 and height must be multiple of 8.\n",
				buffer_id, size->width, size->height);
			return -1;
		}
		// error if size < min size
		if ((size->width < limit->main_width_min) ||
				(size->height < limit->main_height_min)) {
			iav_error("Source buffer %d [%dx%d] cannot be smaller than "
				"minimum size %dx%d in mode [%s].\n", buffer_id,
				size->width, size->height, limit->main_width_min,
				limit->main_height_min, limit->name);
			return -1;
		}
		// error if size > max size
		get_max_encode_buffer_size(buffer_id, &max);
		if ((size->width > max.width) || (size->height > max.height)) {
			iav_error("Source buffer %d [%dx%d] cannot be greater than "
				"system resource limit %dx%d.\n", buffer_id, size->width,
				size->height, max.width, max.height);
			return -1;
		}
		// error if the buffer in in use
		if ((size->width != orig->width) || (size->height != orig->height)) {
			if (state == IAV_SOURCE_BUFFER_STATE_BUSY) {
				iav_error("Cannot change source buffer %d size "
					"while it's used by the encoder.\n", buffer_id);
				return -1;
			}
		}
	}

	return 0;
}

/*
 * Apply the default value for sub source buffer's input window.
 * Check  Check the property about sub source buffer's input window vs.
 * (hard limit, main buffer size).
 */
static int check_sub_buffer_input(int buffer_id,
	iav_rect_ex_t* sub_input, iav_reso_ex_t* main_buffer)
{
	// default input window is main buffer
	if (!sub_input->width) {
		sub_input->width = main_buffer->width;
	}

	if (!sub_input->height) {
		sub_input->height = main_buffer->height;
	}

	//  error if it is aligned
	if ((sub_input->width & 0x1) || (sub_input->x & 0x1)) {
		iav_error("Source buffer [%d] input width %d, offset x %d must"
			" be even.\n", buffer_id, sub_input->width, sub_input->x);
		return -1;
	}
	if ((sub_input->height & 0x3) || (sub_input->y & 0x3)) {
		iav_error("Source buffer [%d] input height %d, offset y %d must be "
			"multiple of 4.\n", buffer_id, sub_input->height, sub_input->y);
		return -1;
	}
	// error if input window > main
	if ((sub_input->width + sub_input->x > main_buffer->width) ||
			(sub_input->height + sub_input->y > main_buffer->height)) {
		iav_error("Source buffer [%d] input %dx%d with offset %dx%d is "
			"out of main %dx%d!\n", buffer_id, sub_input->width,
			sub_input->height, sub_input->x, sub_input->y, main_buffer->width,
			main_buffer->height);
		return -1;
	}

	return 0;
}

/*
 * Check the property about zoom factor between sub buffer buffer size
 * and input window.
 */
static int check_sub_buffer_zoom(int buffer_id,
		u8 unwarp,
		iav_source_buffer_type_ex_t sub_type,
		iav_reso_ex_t* sub_buffer,
		iav_rect_ex_t* sub_input)
{
	char msg[32];
	u16 zoom_height;

	if (sub_type == IAV_SOURCE_BUFFER_TYPE_ENCODE
			&& sub_buffer->width && sub_buffer->height) {
		if (!sub_input->width || !sub_input->height) {
			iav_error("Source buffer [%d] input window is invalid.\n",
				buffer_id);
			return -1;
		}
		sprintf(msg, "Source buffer [%d]", buffer_id);
		if (check_zoom_property(buffer_id,
			sub_input->width, sub_input->height,
			sub_buffer->width, sub_buffer->height, msg) < 0) {
			return -1;
		}
	}

	if (unwarp) {
		if (is_warp_mode()) {
			if ((sub_buffer->width > sub_input->width)
					|| (sub_buffer->height > sub_input->height)) {
				iav_error("Source buffer [%d] size %dx%d upscaled from %dx%d. "
					"Up-scale is NOT supported when buffer is unwarped in "
					"[%s] mode.\n", buffer_id, sub_buffer->width,
					sub_buffer->height, sub_input->width, sub_input->height,
					G_modes_limit[G_dsp_enc_mode].name);
				return -1;
			}
		} else {
			iav_error("Source buffer [%d] unwarp is NOT supported in [%s] "
				"mode.\n", buffer_id,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}

	if (is_full_fps_full_perf_mode() && sub_buffer->width
			&& sub_buffer->height) {
		zoom_height = (sub_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW ?
				sub_input->height / 2 : sub_input->height);
		if (sub_buffer->height > zoom_height) {
			iav_error("Source buffer [%d] input height upscale [%d/2->%d] "
				"is NOT supported in [%s] mode.\n",
				buffer_id, sub_input->height, sub_buffer->height,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}
	return 0;
}

static inline int check_buf_pool_size(u16 buffer_id, iav_reso_ex_t * size)
{
	iav_reso_ex_t * max = NULL;
	iav_enc_mode_limit_t * limit = &G_modes_limit[get_enc_mode()];

	if (size->width && size->height) {
		// error if it is not aligned
		if ((size->width & 0xF) || (size->height & 0x7)) {
			iav_error("Buf pool %d [%dx%d], width must be multiple of "
				"16 and height must be multiple of 8.\n", buffer_id,
				size->width, size->height);
			return -1;
		}
		// error if size < min size
		if ((size->width < limit->main_width_min) ||
				(size->height < limit->main_height_min)) {
			iav_error("Buf pool %d [%dx%d] cannot be smaller than %dx%d "
				"in mode [%s].\n", buffer_id, size->width, size->height,
				limit->main_width_min, limit->main_height_min, limit->name);
			return -1;
		}
		// error if size > max size
		max = &G_source_buffer[buffer_id].property.max;
		if ((size->width > max->width) || (size->height > max->height)) {
			iav_error("Buf pool %d [%dx%d] must be smaller than %dx%d.\n",
				buffer_id, size->width, size->height, max->width, max->height);
			return -1;
		}
		max = &G_source_buffer[buffer_id].dram.buf_max_size;
		if ((size->width > max->width) || (size->height > max->height)) {
			iav_error("Buf pool %d [%dx%d] must be smaller than %dx%d.\n",
				buffer_id, size->width, size->height, max->width, max->height);
			return -1;
		}
	}
	return 0;
}

/*
 * Check system resource limit related to source buffer.
 */
static int check_system_resource_buffer_limit(int encode_mode,
	iav_system_resource_setup_ex_t* resource)
{
	iav_enc_mode_limit_t* limit = &G_modes_limit[encode_mode];
	iav_reso_ex_t * max, *prop_max;
	u16 main_max_w, main_max_h;
	int buffer_id, dram_id;

	// error if main max > limit
	buffer_id = IAV_ENCODE_SOURCE_MAIN_BUFFER;
	max = &resource->buffer_max_size[buffer_id];
	main_max_w = max->width;
	main_max_h = max->height;
	if ((main_max_w > limit->main_width_max) ||
			(main_max_h > limit->main_height_max)) {
		iav_error("Source buffer [%d] max size %dx%d cannot be larger than "
			"the limit %dx%d in [%s] mode.\n", buffer_id,
			main_max_w, main_max_h, limit->main_width_max,
			limit->main_height_max, limit->name);
		return -1;
	}

	for (buffer_id = 0; buffer_id < IAV_ENCODE_SOURCE_TOTAL_NUM; ++buffer_id) {
		max = &resource->buffer_max_size[buffer_id];
		prop_max = &G_source_buffer[buffer_id].property.max;
		// error if buffer max > property
		if (max->width > prop_max->width || max->height > prop_max->height) {
			iav_error("Source buffer [%d] max size %dx%d cannot be greater "
				"than %dx%d.\n", buffer_id, max->width, max->height,
				prop_max->width, prop_max->height);
			return -1;
		}
		// error if sub buffer max > main buffer max
		if ((resource->encode_mode != IAV_ENCODE_WARP_MODE) &&
			(resource->encode_mode != IAV_ENCODE_HIGH_MP_WARP_MODE) &&
			(buffer_id >= IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST) &&
			(buffer_id < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST) &&
			((max->width > main_max_w) || (max->height > main_max_h))) {
			iav_error("Source buffer [%d] max size %dx%d cannot be greater "
				"than main buffer max size %dx%d.\n", buffer_id,
				max->width, max->height, main_max_w, main_max_h);
			return -1;
		}
		// error if DRAM frame num > max
		if (is_valid_dram_buffer_id(buffer_id)) {
			dram_id = buffer_id - IAV_ENCODE_SOURCE_MAIN_DRAM;
			if (resource->max_dram_frame[dram_id] > IAV_MAX_ENC_DRAM_BUF_NUM) {
				iav_error("DRAM buffer [%d] max frame num %d cannot be larger "
					"than %d.\n", buffer_id, resource->max_dram_frame[dram_id],
					IAV_MAX_ENC_DRAM_BUF_NUM);
				return -1;
			}
		}
	}

	return 0;
}

/*
 * Check system resource limit related to encode mode.
 */
static inline int check_system_resource_encode_mode_limit(int encode_mode,
	iav_system_resource_setup_ex_t* resource)
{
	int i, max_warp_width;

	// check warp region width
	if ((encode_mode == DSP_MULTI_REGION_WARP_MODE) ||
		(encode_mode == DSP_HIGH_MP_WARP_MODE)) {
		max_warp_width = MAX_WIDTH_IN_WARP_REGION;
		if ((resource->max_warp_output_width > max_warp_width) ||
			(resource->max_warp_output_width
			> resource->buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width)) {
			iav_error("Max warp output width [%d] must be smaller than [%d] "
				"and source buffer [%d] max width %d in [%s] mode.\n",
				resource->max_warp_output_width, max_warp_width,
				IAV_ENCODE_SOURCE_MAIN_BUFFER,
				resource->buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width,
				G_modes_limit[encode_mode].name);
			return -1;
		}
		if (resource->max_warp_input_width > max_warp_width){
			iav_error("Max warp input width [%d] must be smaller than [%d] "
				"in [%s] mode.\n", resource->max_warp_input_width,
				max_warp_width, G_modes_limit[encode_mode].name);
			return -1;
		}
	}

	// check HDR exposure numbers
	if (encode_mode == DSP_HDR_FRAME_INTERLEAVED_MODE ||
			encode_mode == DSP_HDR_LINE_INTERLEAVED_MODE) {
		if (resource->exposure_num < MIN_HDR_EXPOSURES ||
				resource->exposure_num > MAX_HDR_EXPOSURES) {
			iav_error("HDR exposure number [%d] must be in the range [%d~%d] "
				"in [%s] mode.\n",
				resource->exposure_num, MIN_HDR_EXPOSURES,
				MAX_HDR_EXPOSURES, G_modes_limit[encode_mode].name);
			return -1;
		}
	} else {
		if (resource->exposure_num > MIN_HDR_EXPOSURES) {
			iav_printk("Exposure number %d must be 1 in [%s] mode.\n",
				resource->exposure_num, G_modes_limit[encode_mode].name);
			resource->exposure_num = 1;
		}
	}

	// check Multiple CFA VIN numbers
	if (encode_mode == DSP_MULTI_VIN_MODE) {
		if (resource->vin_num < MIN_CFA_VIN_NUM ||
				resource->vin_num > MAX_CFA_VIN_NUM) {
			iav_error("CFA VIN number [%d] must be in the range [%d~%d] in "
				"[%s] mode.\n", resource->vin_num, MIN_CFA_VIN_NUM,
				MAX_CFA_VIN_NUM, G_modes_limit[encode_mode].name);
			return -1;
		}
	} else {
		if (resource->vin_num > 1) {
			iav_printk("CFA VIN number %d must be 1 in [%s] mode.\n",
				resource->vin_num, G_modes_limit[encode_mode].name);
			resource->vin_num = 1;
		}
	}

	// check max GOP M resource limitation
	if (encode_mode == DSP_HIGH_MEGA_PIXEL_MODE ||
			encode_mode == DSP_HIGH_MP_FULL_PERF_MODE ||
			encode_mode == DSP_FULL_FPS_FULL_PERF_MODE ||
			encode_mode == DSP_MULTI_VIN_MODE) {
		if (resource->stream_max_GOP_M[0] > 1) {
			iav_error("Stream 0's max GOP M [%d] must be no greater than 1 in "
				"[%s] mode.\n", resource->stream_max_GOP_M[0],
				G_modes_limit[encode_mode].name);
			return -1;
		}
	} else {
		if (resource->stream_max_GOP_M[0] > MAX_B_FRAME_NUM) {
			iav_error("Stream 0's max GOP M [%d] must be no greater than %d in "
				"[%s] mode.\n", resource->stream_max_GOP_M[0],
				MAX_B_FRAME_NUM, G_modes_limit[encode_mode].name);
			return -1;
		}
	}
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (resource->debug_max_ref_P[i] > GOP_REF_P_MAX) {
			iav_error("Stream %d's max ref p can't be greater than %d "
				"in [%s] mode.\n", i, GOP_REF_P_MAX,
				G_modes_limit[encode_mode].name);
			return -1;
		}
	}
#endif
	if (encode_mode == DSP_FULL_FPS_FULL_PERF_MODE) {
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (resource->stream_2x_search_range[i]) {
				iav_error("Stream %d's 2x search range must be 0 "
					"in [%s] mode.\n", i, G_modes_limit[encode_mode].name);
				return -1;
			}
		}
	}

	return 0;
}

/*
 * Check general system resource limit.
 */
static int check_system_resource_general_limit(int encode_mode,
	iav_system_resource_setup_ex_t* resource)
{
	int i;
	iav_enc_mode_limit_t* limit = &G_modes_limit[encode_mode];
	int total_extra_buf_num = 0;

	// encode rotate possible
	if (resource->rotate_possible > limit->rotate_possible) {
		iav_error("Rotation is NOT supported in [%s] mode.\n", limit->name);
		return -1;
	}

	// raw capture
	if (resource->raw_capture_enable > limit->raw_cap_possible) {
		iav_error("Raw capture is NOT supported in [%s] mode.\n", limit->name);
		return -1;
	}

	// raw statistics capture possible
	if ((limit->raw_stat_cap_possible == 0) &&
			(resource->max_vin_stats_lines_top ||
			 resource->max_vin_stats_lines_bottom)) {
		iav_error("Setting max VIN statistics lines (top %d, bottom %d) to non "
			"zero is NOT supported in [%s] mode.\n",
			resource->max_vin_stats_lines_top,
			resource->max_vin_stats_lines_bottom, limit->name);
		return -1;
	}
	if ((resource->max_vin_stats_lines_top > MAX_RAW_STATS_LINES_TOP) ||
			(resource->max_vin_stats_lines_bottom > MAX_RAW_STATS_LINES_BOT)) {
		iav_error("Max VIN statistics lines (top %d, bottom %d) must be "
			"under the limit [%d, %d] in [%s] mode.\n",
			resource->max_vin_stats_lines_top,
			resource->max_vin_stats_lines_bottom, MAX_RAW_STATS_LINES_TOP,
			MAX_RAW_STATS_LINES_BOT, limit->name);
		return -1;
	}

	// H-warp bypass
	if (resource->hwarp_bypass_possible > limit->hwarp_bypass_possible) {
		iav_error("H-Warp Bypass is NOT supported in [%s] mode.\n", limit->name);
		return -1;
	}

	// max chroma noise shift
	if ((resource->max_chroma_noise_shift > CHROMA_NOISE_SHIFT_MAX) ||
		(resource->max_chroma_noise_shift < CHROMA_NOISE_SHIFT_MIN)) {
		iav_error("Max chroma noise shift [%u] must be in the range "
			"[%u, %u].\n", resource->max_chroma_noise_shift,
			CHROMA_NOISE_SHIFT_MIN, CHROMA_NOISE_SHIFT_MAX);
		return -1;
	}
	if (resource->max_chroma_noise_shift > limit->max_chroma_noise) {
		iav_error("Max chroma noise shift [%u] must be smaller than "
			"the max [%d].\n", resource->max_chroma_noise_shift,
			limit->max_chroma_noise);
		return -1;
	}

	// sharpen b
	if (resource->sharpen_b_enable > limit->sharpen_b_possible) {
		iav_error("Sharpen B is NOT supported in [%s] mode.\n", limit->name);
		return -1;
	}

	// mixer b
	if (resource->mixer_b_enable > limit->mixer_b_possible) {
		iav_error("Mixer B MUST be OFF in [%s] mode.\n", limit->name);
		return -1;
	}

	// vout_b_letter_box_enable
	if (resource->vout_b_letter_box_enable > limit->vout_b_letter_box_possible) {
		iav_error("VOUT B :etter Boxing MUST be disable in [%s] mode.\n", limit->name);
		return -1;
	}
	if (resource->mixer_b_enable && resource->vout_b_letter_box_enable) {
		iav_error("Mixer B MUST be OFF when Vout B Letter Box enable.\n");
		return -1;
	}

	// yuv input FPS enhanced
	if (resource->yuv_input_enhanced > limit->yuv_input_enhanced_possible) {
		iav_error("Not support enhance YUV input FPS feature in [%s] mode.\n",
			limit->name);
		return -1;
	}

	// Encode from RAW
	if (resource->enc_from_raw_enable > limit->enc_from_raw_possible) {
		iav_error("Encode from RAW is NOT supported in [%s] mode.\n",
			limit->name);
		return -1;
	}

	// max encode stream number
	if (resource->max_num_encode_streams > limit->max_streams_num) {
		iav_error("Max encoding stream number [%d] cannot be greater than %d "
			"in [%s] mode", resource->max_num_encode_streams,
			limit->max_streams_num, limit->name);
		return -1;
	}
	if (resource->max_num_encode_streams == 0) {
		iav_error("Max encoding stream number [%d] cannot be zero.\n",
			resource->max_num_encode_streams);
		return -1;
	}

	// VCA buffer
	if (resource->vca_buf_src_id) {
		if (!limit->vca_buffer_possible) {
			iav_error("Cannot enable VCA buffer in [%s] mode.\n", limit->name);
			return -1;
		}
		if (resource->vca_buf_src_id != IAV_ENCODE_SOURCE_SECOND_BUFFER) {
			iav_error("Cannot set VCA source id [%d] to others except "
				"2nd buffer.\n", resource->vca_buf_src_id);
			return -1;
		}
		if (resource->mixer_b_enable) {
			iav_error("Mixer B MUST be OFF when VCA buffer enabled in [%s] mode.\n",
				limit->name);
			return -1;
		}
	}

	// Extra DRAM buffer
	i = IAV_ENCODE_SOURCE_MAIN_BUFFER;
	if ((resource->extra_dram_buf[i] > IAV_EXTRA_BUF_FRAME_MAX) ||
		(resource->extra_dram_buf[i] < IAV_EXTRA_BUF_FRAME_MIN)) {
		iav_error("Main source buffer :Extra DRAM number [%d] must be in "
			"range of [%d, %d].\n", resource->extra_dram_buf[i],
			IAV_EXTRA_BUF_FRAME_MIN, IAV_EXTRA_BUF_FRAME_MAX);
		return -1;
	}
	total_extra_buf_num += resource->extra_dram_buf[i];
	for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; ++i) {
		if ((resource->extra_dram_buf[i] > IAV_EXTRA_BUF_FRAME_MAX) ||
			(resource->extra_dram_buf[i] < IAV_EXTRA_BUF_FRAME_MIN)) {
			iav_error("Source buffer [%d] :Extra DRAM number [%d] must be in "
				"range of [%d, %d].\n", i, resource->extra_dram_buf[i],
				IAV_EXTRA_BUF_FRAME_MIN, IAV_EXTRA_BUF_FRAME_MAX);
			return -1;
		}
		total_extra_buf_num += resource->extra_dram_buf[i];
		if (total_extra_buf_num > IAV_EXTRA_BUF_FRAME_TOTAL) {
			iav_error("Total extra DRAM number [%d] must be in range of [0, %d].\n",
				total_extra_buf_num, IAV_EXTRA_BUF_FRAME_TOTAL);
			return -1;
		}
	}

	if (resource->debug_chip_id < IAV_CHIP_ID_S2_UNKNOWN ||
			resource->debug_chip_id >=  IAV_CHIP_ID_S2_LAST) {
		iav_error("Incorrect Debug Chip ID [%d].\n",
				resource->debug_chip_id);
		return -1;
	}
	if (resource->vskip_before_encode < IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN) {
		iav_error("Vsync skip before encode [%d] cannot be smaller than [%d].\n",
			resource->vskip_before_encode, IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN);
		return -1;
	}

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		// B frame
		if (i > 0 && resource->stream_max_GOP_M[i] > 1) {
			iav_error("Stream %c cannot have B frames with M [%d] larger"
				" than 1.\n", 'A' + i, resource->stream_max_GOP_M[i]);
			return -1;
		}
		// check vertical search range 2X
		if ((resource->stream_2x_search_range[i] > 0) &&
				((resource->stream_max_size[i].width >
				  MAX_WIDTH_IN_2X_SEARCH_RANGE) ||
				 (resource->stream_max_size[i].height >
				  MAX_HEIGHT_IN_2X_SEARCH_RANGE))) {
			iav_error("Stream %c cannot have 2X search range because the max"
				" size %dx%d is larger than the limit %dx%d.\n",
				'A' + i, resource->stream_max_size[i].width,
				resource->stream_max_size[i].height,
				MAX_WIDTH_IN_2X_SEARCH_RANGE, MAX_HEIGHT_IN_2X_SEARCH_RANGE);
			return -1;
		}
	}

	return 0;
}

static int check_vcap_performance(struct amba_vin_src_capability *vin_info)
{
	int total_vcap_pps, vin_fps;
	iav_enc_mode_limit_t* limit = &G_modes_limit[G_dsp_enc_mode];
	vin_fps = DIV_ROUND(512000000, vin_info->frame_rate);
	total_vcap_pps = (vin_info->cap_cap_w) * (vin_info->cap_cap_h) * vin_fps;
	if (total_vcap_pps > limit->capture_pps_max) {
		iav_error("VIN %dx%d@%dfps is out of the max capture [%d] Pixel/s"
			" in [%s] mode. Please reduce sensor frame rate or resolution!\n",
			vin_info->cap_cap_w, vin_info->cap_cap_h,
			vin_fps, limit->capture_pps_max, limit->name);
		return -1;
	}
	return 0;
}

static inline int check_enc_dram_buf_frame(iav_enc_dram_buf_frame_ex_t * frame)
{
	u16 buf_id;
	iav_source_buffer_dram_ex_t * dram = NULL;
	iav_no_check();

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Invalid IAV state, must be in preview or encoding.\n");
		return -1;
	}
	buf_id = frame->buf_id;
	if (!is_enc_from_dram_enabled()) {
		iav_error("Encode from DRAM [%d] is not supported in mode %s!\n",
			buf_id, G_modes_limit[get_enc_mode()].name);
		return -1;
	}
	if (!is_valid_dram_buffer_id(buf_id)) {
		iav_error("Invalid buf id [%d] for encoding from DRAM!\n", buf_id);
		return -1;
	}
	if (!is_buf_type_enc(buf_id)) {
		iav_error("Buf pool [%d] is not in ENCODE type.\n", buf_id);
		return -1;
	}
	dram = &G_source_buffer[buf_id].dram;
	if (dram->buf_state == DSP_DRAM_BUFFER_POOL_INIT) {
		iav_error("Buf pool [%d] state [%d] is not ready to request frames!\n",
			buf_id, dram->buf_state);
		return -1;
	}
	if (!dram->max_frame_num) {
		iav_error("Buf pool [%d] is not allocated frames!\n", buf_id);
		return -1;
	}

	return 0;
}

static int check_enc_dram_buf_update(iav_enc_dram_buf_update_ex_t * update)
{
	u16 buf_id, frame_id;
	iav_reso_ex_t * size = NULL;
	iav_source_buffer_dram_pool_ex_t * pool = NULL;
	iav_no_check();

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Invalid IAV state, must be in preview or encoding.\n");
		return -1;
	}
	buf_id = update->buf_id;
	if (!is_enc_from_dram_enabled()) {
		iav_error("Encode from DRAM [%d] is not supported in mode %s!\n",
			buf_id, G_modes_limit[get_enc_mode()].name);
		return -1;
	}
	if (!is_valid_dram_buffer_id(buf_id)) {
		iav_error("Invalid buf id [%d] for encoding from DRAM!\n", buf_id);
		return -1;
	}
	frame_id = update->frame_id;
	if (frame_id >= IAV_MAX_ENC_DRAM_BUF_NUM) {
		iav_error("Invalid frame id %d, must be in the range of "
			"[0~%d].\n", frame_id, (IAV_MAX_ENC_DRAM_BUF_NUM - 1));
		return -1;
	}
	pool = &G_source_buffer[buf_id].dram.buf_pool;
	if (is_invalid_dsp_addr(pool->y[frame_id]) ||
		is_invalid_dsp_addr(pool->uv[frame_id])) {
		iav_error("Invalid YUV frame id %d. It's not requested yet!\n", frame_id);
		return -1;
	}
	if (is_invalid_dsp_addr(pool->me1[frame_id])) {
		iav_error("Invalid ME1 frame id %d. It's not requested yet!\n", frame_id);
		return -1;
	}
	size = &G_source_buffer[buf_id].dram.buf_max_size;
	if ((update->size.width > size->width) ||
		(update->size.height > size->height)) {
		iav_error("YUV data size %dx%d cannot be larger than max %dx%d.\n",
			update->size.width, update->size.height, size->width, size->height);
		return -1;
	}
	size = &G_source_buffer[buf_id].size;
	if ((update->size.width < size->width) ||
		(update->size.height < size->height)) {
		iav_error("YUV data size %dx%d cannot be smaller than buffer "
			"[%d] size %dx%d.\n", update->size.width, update->size.height,
			buf_id, size->width, size->height);
		return -1;
	}
	return 0;
}

/*
 * Check the property limited by VIN.
 */
static int cross_check_vin(iav_rect_ex_t* vin)
{
	iav_reso_ex_t* main_buffer;
	iav_rect_ex_t* main_input;
	struct amba_vin_src_capability *vin_info = get_vin_capability();
	int is_pre_main = is_warp_mode();

	main_buffer = is_pre_main ? &G_cap_pre_main.size :
		&G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
	main_input = is_pre_main ? &G_cap_pre_main.input :
		&G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].input;

	// check yuv_input_enhanced enabled
	if (G_iav_vcap.yuv_input_enhanced) {
		if (vin_info->input_format == AMBA_VIN_INPUT_FORMAT_RGB_RAW) {
			iav_error("Enhance yuv input FPS feature just support for "
				"YUV input in [%s] mode.\n",
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
		if ((main_input->width != vin->width) ||
			(main_input->height != vin->height) ||
			(main_buffer->width != vin->width) ||
			(main_buffer->height != vin->height)) {
			iav_error("%s size %dx%d must be same as VIN %dx%d "
				"with enhanced yuv input feature in [%s] mode\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
				main_buffer->width, main_buffer->height,
				vin->width, vin->height, G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}

	// vin error in some modes
	if (is_calibration_mode() || is_high_mp_mode()) {
		if ((vin->width > MAX_WIDTH_FOR_1080P_MAIN_IN_HIGH_MP)
				&& (main_buffer->width < MAX_WIDTH_IN_FULL_FPS)) {
			iav_error("%s width [%d] must be greater than %d"
				" when VIN width is [%d] in [%s] mode.\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
				main_buffer->width, MAX_WIDTH_IN_FULL_FPS, vin->width,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}

		if ((vin->width <= MAX_WIDTH_IN_FULL_FPS)
				&& (main_buffer->width > MAX_WIDTH_IN_FULL_FPS)) {
			iav_error("%s width [%d] must be smaller than %d"
				" when VIN width is [%d] in [%s] mode.\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
				main_buffer->width, MAX_WIDTH_IN_FULL_FPS, vin->width,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}

	// error if pre-main / main size > VIN
	if ((main_buffer->width > vin->width)
			|| (main_buffer->height > vin->height)) {
		iav_error("%s size %dx%d is bigger than VIN %dx%d in [%s] mode.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_buffer->width, main_buffer->height,
			vin->width, vin->height, G_modes_limit[G_dsp_enc_mode].name);
		return -1;
	}

	// error if pre-main /main window > VIN
	if ((main_input->width + main_input->x > vin->width)
			|| (main_input->height + main_input->y > vin->height)) {
		iav_error("%s  input window %dx%d + offset %dx%d "
			"cannot be larger than VIN %dx%d in [%s] mode.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			main_input->width, main_input->height, main_input->x,
			main_input->y, vin->width, vin->height,
			G_modes_limit[G_dsp_enc_mode].name);
		return -1;
	}

	if (check_main_buffer_zoom(is_pre_main, main_buffer, main_input) < 0) {
		return -1;
	}

	return 0;
}

/*
 * Check the property limited by VOUT.
 */
static int cross_check_vout(void)
{
	struct amba_iav_vout_info * vout_a = NULL, *vout_b = NULL;
	u16 main_h, vout_h;

	if (is_full_fps_full_perf_mode()) {
		main_h = G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.height;
		if (G_system_setup_info[G_dsp_enc_mode].vout_swap) {
			vout_a = G_iav_info.pvoutinfo[1];
			vout_b = G_iav_info.pvoutinfo[0];
		} else {
			vout_a = G_iav_info.pvoutinfo[0];
			vout_b = G_iav_info.pvoutinfo[1];
		}
		if (vout_b->enabled &&
				is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
			vout_h = vout_b->video_info.rotate ? vout_b->video_info.width
				: vout_b->video_info.height;
			if (vout_b->video_info.format == AMBA_VIDEO_FORMAT_INTERLACE) {
				vout_h = vout_h >> 1;
			}
			if (vout_h > main_h / 2) {
				iav_error("VOUT B height upscale [%d/2->%d] "
					"is NOT supported in [%s] mode.\n",  main_h, vout_h,
					G_modes_limit[G_dsp_enc_mode].name);
				return -1;
			}
		}
		if (vout_a->enabled &&
				is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
			vout_h = vout_a->video_info.rotate ? vout_a->video_info.width
				: vout_a->video_info.height;
			if (vout_a->video_info.format == AMBA_VIDEO_FORMAT_INTERLACE) {
				vout_h = vout_h >> 1;
			}
			if (vout_h > main_h / 2) {
				iav_error("VOUT A height upscale [%d/2->%d] "
					"is NOT supported in [%s] mode.\n",  main_h, vout_h,
					G_modes_limit[G_dsp_enc_mode].name);
				return -1;
			}
		}
	}
	return 0;
}

/*
 * Check the property limited by system resource.
 */
static int cross_check_resource(int buffer_id)
{
	iav_reso_ex_t max = { 0 };
	u32 vin_num, aligned_height;
	iav_reso_ex_t* buffer = &G_source_buffer[buffer_id].size;
	iav_enc_mode_limit_t *limit = &G_modes_limit[G_dsp_enc_mode];

	get_max_encode_buffer_size(buffer_id, &max);

	// error if buffer size > max size
	if (!G_source_buffer[buffer_id].enc_stop &&
			((buffer->width > max.width) || (buffer->height > max.height))) {
		iav_error("Source buffer [%d] size %dx%d cannot be greater than "
			"system resource limit %dx%d.\n", buffer_id, buffer->width,
			buffer->height, max.width, max.height);
		return -1;
	}

	if (!is_buf_type_off(buffer_id)) {
		// error if buffer max is 0x0 when it is non OFF type
		if (!max.width || !max.height) {
			iav_error("Souce buffer [%d] type cannot be PREVIEW OR ENCODE "
				"when max size is 0x0.\n", buffer_id);
			return -1;
		}
		// error if buffer max < limit min when buffer type is NOT off
		if ((max.width < limit->main_width_min) ||
				(max.height < limit->main_height_min)) {
			iav_error("Source buffer [%d] max size %dx%d cannot be smaller "
				"than the limit %dx%d in [%s] mode.\n", buffer_id,
				max.width, max.height, limit->main_width_min,
				limit->main_height_min, limit->name);
			return -1;
		}
		// error if sub buffer size * VIN num > buffer max size
		if (is_multi_vin_mode() && (IAV_ENCODE_SOURCE_MAIN_DRAM != buffer_id)) {
			vin_num = get_vin_num(G_dsp_enc_mode);
			aligned_height = ALIGN(buffer->height, 16);
			if ((max.height < vin_num * aligned_height)) {
				iav_error("Source buffer [%d] max height %d cannot be smaller "
					"than %dx%d for %d VIN number in [%s] mode\n",
					buffer_id, max.height, vin_num, aligned_height,
					vin_num, G_modes_limit[G_dsp_enc_mode].name);
			}
		}
	}

	return 0;
}

static int cross_check_in_warp_mode(u32 mode,
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource)
{
	u32 max_warp_width;
	iav_reso_ex_t * buf = NULL;
	u16 a_unwarp, c_unwarp;

	max_warp_width = resource->max_warped_region_input_width;
	buf = &G_cap_pre_main.size;
	if (max_warp_width > buf->width) {
		iav_error("Max warp input width [%d] cannot be larger than the "
			"pre main %d in [%s] mode.\n", max_warp_width, buf->width,
			G_modes_limit[mode].name);
		return -1;
	}
	max_warp_width = resource->max_warped_region_output_width;
	buf = &G_source_buffer[0].size;
	if (max_warp_width > buf->width) {
		iav_error("Max warp output width [%d] cannot be larger than the "
			"main %d in [%s] mode.\n", max_warp_width, buf->width,
			G_modes_limit[mode].name);
		return -1;
	}

	c_unwarp = G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER].unwarp;
	a_unwarp = G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER].unwarp;
	if ((!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER) && !c_unwarp) &&
		(!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER) && a_unwarp)) {
		iav_error("Cannot support warp 2nd buffer and unwarped 4th buffer together.\n");
		return -1;
	}

	/* Set default warp param with new buffer configuration */
	if (set_default_warp_param() < 0) {
		iav_error("Failed to set default warp param!\n");
		return -1;
	}

	return 0;
}

static int cross_check_in_hdr_line_mode(u32 mode,
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource)
{
	char *name = G_modes_limit[mode].name;

	if (!G_iav_vcap.sharpen_b_enable[mode]) {
		iav_error("Sharpen B must be enabled in [%s] mode.\n", name);
		return -1;
	}
	if (resource->hdr_num_exposures_minus_1) {
		if (is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
			iav_printk("Enter [%s] mode with optmized mode.\n", name);
		} else {
			iav_printk("Enter [%s] mode with unoptmized mode.\n", name);
		}
	}

	return 0;
}

static int cross_check_in_full_fps_full_perf_mode(u32 mode,
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource)
{
	struct amba_vin_src_capability * vin = get_vin_capability();
	u16 m_w, m_h, a_w, a_h, b_w, b_h;
	u32 vin_frame_rate;
	char *name = G_modes_limit[mode].name;

	m_w = resource->max_main_width;
	m_h = resource->max_main_height;
	b_w = resource->max_preview_B_width;
	b_h = resource->max_preview_B_height;
	a_w = resource->max_preview_A_width;
	a_h = resource->max_preview_A_height;
	vin_frame_rate = DIV_ROUND(512000000, vin->frame_rate);
	if (m_w >= 1920 && m_h >= 1080) {
		m_w = G_source_buffer[0].size.width;
		m_h = G_source_buffer[0].size.height;
		if ((vin_frame_rate > MAX_VIN_FPS_FOR_DUAL_1080P) &&
			(m_w >= 1920 || m_h >= 1080) &&
			G_iav_vcap.sharpen_b_enable[mode]) {
			iav_printk("It's suggested to TURN OFF sharpen B when VIN fps"
				" higher than %d to get more performance in [%s] mode.\n",
				MAX_VIN_FPS_FOR_DUAL_1080P, name);
		}
		if ((vin_frame_rate >= MAX_VIN_FPS_FOR_DUAL_1080P) &&
			(b_w >= 1920 || b_h >= 1080 || a_w >= 1920 || a_h >= 1080)) {
			iav_error("Cannot enable dual 1080p buffers when VIN %d fps "
				"higher than %d fps in [%s] mode, please use smaller 3rd "
				"or 4th buffer or lower VIN frame rate.\n",
				vin_frame_rate, MAX_VIN_FPS_FOR_DUAL_1080P, name);
			return -1;
		}
		if ((vin_frame_rate >= MAX_VIN_FPS_FOR_TRIPLE_1080P) &&
			(b_w >= 1920 || b_h >= 1080) && (a_w >= 1920 || a_h >= 1080)) {
			iav_error("Cannot enable triple 1080p buffers when VIN %d fps "
				"higher than %d fps in [%s] mode, please use smaller 3rd "
				"and 4th buffers or lower VIN frame rate.\n",
				vin_frame_rate, MAX_VIN_FPS_FOR_TRIPLE_1080P, name);
			return -1;
		}
	}

	if (check_hdr_pm_width() < 0) {
		return -1;
	}

	return 0;
}

static int cross_check_resource_in_mode(u32 mode)
{
	int rval = 0;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = &G_system_resource_setup[mode];

	switch (mode) {
	case DSP_MULTI_REGION_WARP_MODE:
	case DSP_HIGH_MP_WARP_MODE:
		rval = cross_check_in_warp_mode(mode, resource);
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		rval = cross_check_in_hdr_line_mode(mode, resource);
		break;
	case DSP_FULL_FPS_FULL_PERF_MODE:
		rval = cross_check_in_full_fps_full_perf_mode(mode, resource);
		break;
	case DSP_FULL_FRAMERATE_MODE:
		rval = check_hdr_pm_width();
		break;
	default:
		break;
	}

	return rval;
}

static int check_vcap_encode_mode(u16 vin_cap_width,
	u16 vin_cap_height)
{
	if (is_multi_vin_mode()) {
		if (vin_cap_width > MAX_WIDTH_IN_MULTI_VIN
				|| vin_cap_height > MAX_HEIGHT_IN_MULTI_VIN) {
			iav_error("VIN %dx%d must be no greater than the stitched limit "
				"%dx%d in [%s] mode.\n", vin_cap_width, vin_cap_height,
				MAX_WIDTH_IN_MULTI_VIN, MAX_HEIGHT_IN_MULTI_VIN,
				G_modes_limit[G_dsp_enc_mode].name);
			return -1;
		}
	}

	return 0;
}

static int iav_check_vin(iav_context_t *context)
{
	iav_no_check();

	if (!(G_iav_info.pvininfo->enabled & IAV_VIN_ENABLED_FOR_VIDEO)) {
		iav_error("vin not enabled %d\n", G_iav_info.pvininfo->enabled);
		return -1;
	}
	return 0;
}

static int iav_check_vout(iav_context_t *context)
{
	int vout_swap = G_system_setup_info[G_dsp_enc_mode].vout_swap;
	struct amba_iav_vout_info * vout_a = NULL, *vout_b = NULL;

	iav_no_check();

	if (vout_swap) {
		vout_a = G_iav_info.pvoutinfo[1];
		vout_b = G_iav_info.pvoutinfo[0];
	} else {
		vout_a = G_iav_info.pvoutinfo[0];
		vout_b = G_iav_info.pvoutinfo[1];
	}
#if 0	// Fixme: comment out the strictest check for VOUT and preview buffers
	if (!vout_a->enabled && is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		iav_error("VOUT Swap [%d], cannot set 4th buffer as preview for VOUT "
			"is disabled! Please set it to OFF.\n", vout_swap);
		return -1;
	}
	if (!vout_b->enabled && is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		iav_error("VOUT Swap [%d], cannot set 3rd buffer as preview for VOUT "
			"is disabled! Please set it to OFF.\n", vout_swap);
		return -1;
	}
#endif
	/* check mixer-b */
	if(G_modes_limit[G_dsp_enc_mode].mixer_b_possible <
		G_iav_vcap.vout_b_mixer_enable ) {
		iav_error("Mixer b must be OFF in [%s] mode.\n",
			G_modes_limit[G_dsp_enc_mode].name);
		return -1;
	}
	if (!vout_a->enabled && !vout_b->enabled) {
		iav_printk("VOUT are all disabled.\n");
	}

	/* check vout_swap with letter_box_enable */
	if (G_iav_vcap.vout_b_letter_boxing_enable && vout_swap) {
		iav_error("Can't enable VOUT B Letter box when VOUT swap!\n");
		return -1;
	}
	if (G_iav_vcap.vout_b_letter_boxing_enable &&
		!is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		iav_error("Enable VOUT B Letter box just when Preview B buffer "
			"is preview type!\n");
		return -1;
	}

	/* Check preview video height */
	if (is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		if (G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height ||
			G_iav_info.pvoutinfo[1]->active_mode.video_size.video_width) {
			if ((G_cap_pre_main.size.height >> 3) >=
				G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height) {
				iav_error("Video height must bigger than %d !\n",
					G_cap_pre_main.size.height >> 3);
				return -1;
			}
		}
	}

	return 0;
}

static int iav_check_vcap(iav_context_t * context)
{
	struct amba_vin_src_capability *vin_info = get_vin_capability();

	if (check_vcap_performance(vin_info) < 0)
		return -1;

	if (check_vcap_encode_mode(vin_info->cap_cap_w,
			vin_info->cap_cap_h) < 0) {
		return -1;
	}

	return 0;
}

/*
 * Apply the default input window for all the source buffers.
 */
static void update_default_input_window(iav_rect_ex_t* vin)
{
	int i;
	iav_reso_ex_t *default_sub_input;
	iav_source_buffer_ex_t* main_buffer, *buffer;

	main_buffer =
		&G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER];

	// default main input = vin
	buffer = &G_cap_pre_main;
	if (!buffer->input.width) {
		buffer->input.width = vin->width;
		buffer->input.x = 0;
	}
	if (!buffer->input.height) {
		buffer->input.height = vin->height;
		buffer->input.y = 0;
	}

	if (is_warp_mode()) {
		main_buffer->input.width = buffer->size.width;
		main_buffer->input.height = buffer->size.height;
		main_buffer->input.x = main_buffer->input.y = 0;
	} else {
		main_buffer->input = buffer->input;
	}

	// default sub buffer input = pre-main / main size
	for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
			i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; ++i) {
		buffer = &G_source_buffer[i];
		default_sub_input = ((is_warp_mode() && G_source_buffer[i].unwarp) ?
			&G_cap_pre_main.size : &main_buffer->size);
		if (!buffer->input.width) {
			buffer->input.width = default_sub_input->width;
			buffer->input.x = 0;
		}
		if (!buffer->input.height) {
			buffer->input.height = default_sub_input->height;
			buffer->input.y = 0;
		}
	}

	// default DRAM buffer input = DRAM max size
	for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
			i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
		buffer = &G_source_buffer[i];
		if (!buffer->input.width) {
			buffer->input.width = buffer->dram.buf_max_size.width;
			buffer->input.x = 0;
		}
		if (!buffer->input.height) {
			buffer->input.height = buffer->dram.buf_max_size.height;
			buffer->input.y = 0;
		}
	}
}

static int iav_cross_check_in_idle(void)
{
	int i;
	iav_rect_ex_t vin;

	get_vin_window(&vin);

	update_default_input_window(&vin);

	iav_no_check();

	if (cross_check_vin(&vin) < 0) {
		return -1;
	}

	if (cross_check_vout() < 0) {
		return -1;
	}

	for (i = IAV_ENCODE_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		if (cross_check_resource(i) < 0) {
			return -1;
		}
	}

	if (cross_check_resource_in_mode(G_dsp_enc_mode)) {
		return -1;
	}

	return 0;
}

static int iav_check_before_enter_preview(iav_context_t * context)
{
	if (!is_iav_state_idle())
		return -EPERM;

	if (!is_valid_enc_mode(G_dsp_enc_mode)) {
		iav_error("Invalid encode mode : [%d].\n", G_dsp_enc_mode);
		return -EINVAL;
	}

	if (iav_check_vin(context) < 0)
		return -EAGAIN;

	if (iav_check_vcap(context) < 0)
		return -EAGAIN;

	if (iav_check_vout(context) < 0)
		return -EAGAIN;

	if (iav_cross_check_in_idle() < 0)
		return -EAGAIN;

	if (iav_check_system_config(G_dsp_enc_mode, 0) < 0)
		return -EAGAIN;

	return 0;
}

static int iav_check_source_buffer_setup(iav_source_buffer_setup_ex_t* setup)
{
	iav_rect_ex_t vin;
	int i, main_id;
	u32 is_warp = is_warp_mode();
	iav_rect_ex_t* main_zoom_input;
	iav_reso_ex_t* main_zoom, *main_buffer, *default_sub_input;

	iav_no_check();

	get_vin_window(&vin);
	main_id = IAV_ENCODE_SOURCE_MAIN_BUFFER;
	main_buffer = &setup->size[main_id];

	if (is_warp) {
		main_zoom = &setup->pre_main;
		main_zoom_input = &setup->pre_main_input;
	} else {
		main_zoom = &setup->size[main_id];
		main_zoom_input = &setup->input[main_id];
	}

	if (check_source_buffer_type(main_id, setup->type[main_id]) < 0) {
		return -1;
	} else if (check_main_buffer_size(is_warp, main_zoom) < 0) {
		return -1;
	} else if (check_main_buffer_input(is_warp, main_zoom_input, &vin) < 0) {
		return -1;
	} else if (check_main_buffer_zoom(is_warp, main_zoom, main_zoom_input) < 0) {
		return -1;
	}

	if (is_warp) {
		// check main buffer size in warp mode
		if (check_main_buffer_size(0, main_buffer) < 0) {
			return -1;
		}
	}

	/* check sub source buffer */
	for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; ++i) {
		default_sub_input = (setup->unwarp[i] ? &setup->pre_main :
			&setup->size[main_id]);
		if (check_source_buffer_type(i, setup->type[i]) < 0) {
			return -1;
		} else if (check_sub_buffer_size(i, &setup->size[i]) < 0) {
			return -1;
		} else if (check_sub_buffer_input(i, &setup->input[i],
				default_sub_input) < 0) {
			return -1;
		} else if (check_sub_buffer_zoom(i, setup->unwarp[i], setup->type[i],
				&setup->size[i], &setup->input[i]) < 0) {
			return -1;
		}
	}

	/* check DRAM source buffer */
	for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
		i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
		if (check_source_buffer_type(i, setup->type[i]) < 0) {
			return -1;
		} else if (check_buf_pool_size(i, &setup->size[i]) < 0) {
			return -1;
		}
	}

	return 0;
}

static int iav_check_source_buffer_format(iav_source_buffer_format_ex_t* format)
{
	iav_source_buffer_ex_t* orig;
	iav_reso_ex_t* default_sub_input;

	iav_no_check();

	if (!is_sub_buf(format->source)) {
		iav_error("Invalid sub buffer id [%d].\n", format->source);
		return -1;
	}

	orig = &G_source_buffer[format->source];
	default_sub_input = orig->unwarp ? &G_cap_pre_main.size
		: &G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
	if (check_sub_buffer_size(format->source, &format->size) < 0) {
		return -1;
	} else if (check_sub_buffer_input(format->source,
			&format->input, default_sub_input) < 0) {
		return -1;
	} else if (check_sub_buffer_zoom(format->source,
			orig->unwarp, orig->type, &format->size, &format->input) < 0) {
		return -1;
	}
	return 0;
}

static int iav_check_digital_zoom_I(iav_digital_zoom_ex_t * dz)
{
	iav_rect_ex_t vin;
	iav_reso_ex_t* main_buffer =
		&G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;

	iav_no_check();

	if (!is_dptz_I_enabled()) {
		iav_error("DZ type I is NOT supported in [%s] mode.\n",
			G_modes_limit[G_dsp_enc_mode].name);
		return -1;
	}

	if (G_iav_vcap.yuv_input_enhanced) {
		iav_error("Cannot run DZ type I when enhance yuv input FPS "
			"feature enabled.\n");
		return -1;
	};

	if (is_video_freeze_enabled()) {
		iav_error("Cannot run DZ type I when video freeze is enabled.\n");
		return -1;
	}

	if (dz->source != IAV_ENCODE_SOURCE_MAIN_BUFFER) {
		iav_error("DZ type I is only available for main buffer."
			" Invalid source [%d].\n", dz->source);
		return -1;
	}

	get_vin_window(&vin);

	if (check_main_buffer_input(0, &dz->input, &vin) < 0) {
		return -1;
	} else if (check_main_buffer_zoom(0, main_buffer, &dz->input) < 0) {
		return -1;
	}
	return 0;
}

static int iav_check_digital_zoom_II(iav_digital_zoom_ex_t * dz)
{
	int buffer_id;
	iav_source_buffer_ex_t* buffer;
	iav_reso_ex_t* default_sub_input;

	iav_no_check();

	buffer_id = dz->source;

	if (!is_dptz_II_enabled()) {
		iav_error("DZ type II is NOT supported in [%s] mode.\n",
			G_modes_limit[G_dsp_enc_mode].name);
		return -1;
	} else if (is_video_freeze_enabled()) {
		iav_error("Cannot run DZ type II when video freeze is enabled!\n");
		return -1;
	} else if (!is_sub_buf(buffer_id)) {
		iav_error("Invalid sub source buffer id [%d] in DZ type II .\n",
			buffer_id);
		return -1;
	} else if (!is_buf_type_enc(buffer_id)) {
		iav_error("DZ type II source buffer [%d] must be in ENCODE type.\n",
			buffer_id);
		return -1;
	}
	buffer = &G_source_buffer[buffer_id];
	default_sub_input = buffer->unwarp ? &G_cap_pre_main.size
		: &G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
	if (check_sub_buffer_input(buffer_id, &dz->input, default_sub_input) < 0) {
		return -1;
	} else if (check_sub_buffer_zoom(buffer_id, buffer->unwarp, buffer->type,
			&buffer->size, &dz->input) < 0) {
		return -1;
	}

	return 0;
}

static int iav_check_system_resource_limit(iav_system_resource_setup_ex_t * resource)
{
	int encode_mode;

	iav_no_check();

	// check encode mode
	switch (resource->encode_mode) {
	case IAV_ENCODE_FULL_FRAMERATE_MODE:
		encode_mode = DSP_FULL_FRAMERATE_MODE;
		break;
	case IAV_ENCODE_WARP_MODE:
		encode_mode = DSP_MULTI_REGION_WARP_MODE;
		break;
	case IAV_ENCODE_HIGH_MEGA_MODE:
		encode_mode = DSP_HIGH_MEGA_PIXEL_MODE;
		break;
	case IAV_ENCODE_CALIBRATION_MODE:
		encode_mode = DSP_CALIBRATION_MODE;
		break;
	case IAV_ENCODE_HDR_FRAME_MODE:
		encode_mode = DSP_HDR_FRAME_INTERLEAVED_MODE;
		break;
	case IAV_ENCODE_HDR_LINE_MODE:
		encode_mode = DSP_HDR_LINE_INTERLEAVED_MODE;
		break;
	case IAV_ENCODE_HIGH_MP_FULL_PERF_MODE:
		encode_mode = DSP_HIGH_MP_FULL_PERF_MODE;
		break;
	case IAV_ENCODE_FULL_FPS_FULL_PERF_MODE:
		encode_mode = DSP_FULL_FPS_FULL_PERF_MODE;
		break;
	case IAV_ENCODE_MULTI_VIN_MODE:
		encode_mode = DSP_MULTI_VIN_MODE;
		break;
	case IAV_ENCODE_HISO_VIDEO_MODE:
		encode_mode = DSP_HISO_VIDEO_MODE;
		break;
	case IAV_ENCODE_HIGH_MP_WARP_MODE:
		encode_mode = DSP_HIGH_MP_WARP_MODE;
		break;
	default:
		iav_error("Invalid encode mode [%d].\n", resource->encode_mode);
		return -1;
		break;
	}
	resource->encode_mode = encode_mode;

	if (check_system_resource_general_limit(encode_mode, resource) < 0) {
		return -1;
	} else if (check_system_resource_buffer_limit(encode_mode, resource) < 0) {
		return -1;
	} else if (check_system_resource_encode_mode_limit(encode_mode,
			resource) < 0) {
		return -1;
	}

	return 0;
}

static int iav_check_system_setup_info(iav_system_setup_info_ex_t *info)
{
	iav_enc_mode_limit_t *limit = NULL;
	iav_no_check();

	// check coded_bits_interrupt_enable flag
	if (info->coded_bits_interrupt_enable > 1) {
		iav_error("Invalid value [%d] for coded_bits_interrupt_enable!\n",
			info->coded_bits_interrupt_enable);
		return -1;
	}
	limit = &G_modes_limit[G_dsp_enc_mode];
	if (info->vout_swap && !limit->vout_swap_possible) {
		iav_error("Cannot swap VOUT in mode [%s]!\n", limit->name);
		return -1;
	}
	if (!info->mctf_privacy_mask && !limit->hdr_pm_possible) {
		iav_error("Cannot enable HDR type privacy mask in mode [%s].\n",
			limit->name);
		return -1;
	}
	if (info->mctf_privacy_mask && !limit->mctf_pm_possible) {
		iav_error("Cannot enable MCTF type privacy mask in mode [%s]!\n",
			limit->name);
		return -1;
	}
	if (info->eis_delay_count > limit->max_eis_delay_count) {
		iav_error("Invalid value [%d] for eis_delay_count!\n",
			info->eis_delay_count);
		return -1;
	}
	return 0;
}

static int update_pre_main_buffer(iav_reso_ex_t* pre_main,
		iav_rect_ex_t* pre_main_input)
{
	iav_source_buffer_ex_t* buffer = &G_cap_pre_main;

	buffer->size.width = pre_main->width;
	buffer->size.height = pre_main->height;
	buffer->input.width = pre_main_input->width;
	buffer->input.height = pre_main_input->height;
	buffer->input.x = pre_main_input->x;
	buffer->input.y = pre_main_input->y;

	return 0;
}

static int update_source_buffer_format(int buffer_id,
		iav_reso_ex_t* size,
		iav_rect_ex_t* input)
{
	iav_source_buffer_ex_t* buffer = &G_source_buffer[buffer_id];

	if (size) {
		if (size->width == 0 || size->height == 0) {
			// set enc_stop flag when buffer size is 0 in encode type.
			buffer->enc_stop = 1;
		} else {
			buffer->enc_stop = 0;
			buffer->size.width = size->width;
			buffer->size.height = size->height;
		}
	}
	if (input) {
		buffer->input.width = input->width;
		buffer->input.height = input->height;
		buffer->input.x = input->x;
		buffer->input.y = input->y;
	}

	return 0;
}

static int update_source_buffer_setup(iav_source_buffer_setup_ex_t* setup)
{
	u32 i;

	if (is_warp_mode()) {
		update_pre_main_buffer(&setup->pre_main, &setup->pre_main_input);
	} else {
		update_pre_main_buffer(&setup->size[IAV_ENCODE_SOURCE_MAIN_BUFFER],
			&setup->input[IAV_ENCODE_SOURCE_MAIN_BUFFER]);
	}

	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		G_source_buffer[i].type = setup->type[i];
		G_source_buffer[i].unwarp = setup->unwarp[i];
		update_source_buffer_format(i, &setup->size[i], &setup->input[i]);
	}
	return 0;
}

static void update_system_resource_limit(iav_context_t* context,
	iav_system_resource_setup_ex_t* param)
{
	int i, j;
	u16 max_w, max_h;
	iav_source_buffer_dram_ex_t * dram = NULL;
	DSP_ENC_CFG * enc_cfg = NULL;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT * ext_cmd = NULL;

	G_dsp_enc_mode = param->encode_mode;
	resource = &G_system_resource_setup[G_dsp_enc_mode];

	/* Update mode related param */
	resource->mode_flags = param->encode_mode;
	resource->enc_rotation = param->rotate_possible;
	resource->raw_compression_disabled = param->raw_capture_enable;
	resource->hdr_num_exposures_minus_1 =
		calc_dsp_expo_num(G_dsp_enc_mode, param->exposure_num);
	resource->num_vin_minus_2 = calc_dsp_vin_num(G_dsp_enc_mode,
		param->vin_num);
	resource->max_warped_region_input_width = param->max_warp_input_width;
	resource->max_warped_region_output_width = param->max_warp_output_width;
	resource->max_chroma_filter_radius = param->max_chroma_noise_shift;
	resource->max_vin_stats_num_lines_top = param->max_vin_stats_lines_top;
	resource->max_vin_stats_num_lines_bot = param->max_vin_stats_lines_bottom;
	resource->extra_dram_buf = SET_EXTRA_BUF(param->extra_dram_buf[0]);
	resource->extra_buf_msb_ext = SET_EXTRA_BUF_MSB(param->extra_dram_buf[0]);
	resource->extra_dram_buf_prev_c = SET_EXTRA_BUF(param->extra_dram_buf[1]);
	resource->extra_buf_msb_ext_prev_c =
		SET_EXTRA_BUF_MSB(param->extra_dram_buf[1]);
	resource->extra_dram_buf_prev_b = SET_EXTRA_BUF(param->extra_dram_buf[2]);
	resource->extra_buf_msb_ext_prev_b =
		SET_EXTRA_BUF_MSB(param->extra_dram_buf[2]);
	resource->extra_dram_buf_prev_a = SET_EXTRA_BUF(param->extra_dram_buf[3]);
	resource->extra_buf_msb_ext_prev_a =
		SET_EXTRA_BUF_MSB(param->extra_dram_buf[3]);
	resource->h_warp_bypass = param->hwarp_bypass_possible;

	/* Update buffer config */
	resource->max_main_width =
		param->buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width;
	resource->max_main_height =
		param->buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height;
	resource->max_preview_C_width =
		param->buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width;
	resource->max_preview_C_height =
		param->buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height;
	resource->max_preview_B_width =
		param->buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width;
	resource->max_preview_B_height =
		param->buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height;
	resource->max_preview_A_width =
		param->buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width;
	resource->max_preview_A_height =
		param->buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height;
	G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER].disable_extra_2x =
		!param->extra_2x_zoom_enable;

	/* Update DRAM buffer config */
	for (i = IAV_ENCODE_SOURCE_DRAM_FIRST, j = 0;
		i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i, ++j) {
		dram = &G_source_buffer[i].dram;
		dram->max_frame_num = param->max_dram_frame[j];
		dram->buf_pitch = ALIGN(param->buffer_max_size[i].width, (PIXEL_IN_MB << 1));
		dram->buf_max_size.width= dram->buf_pitch;
		dram->buf_max_size.height = param->buffer_max_size[i].height;
	}

	/* Update stream config */
	if (likely(param->max_num_encode_streams < IPCAM_RECORD_MAX_NUM_ENC_ALL)) {
		resource->max_num_enc = param->max_num_encode_streams;
		resource->max_num_enc_msb = 0;
	} else {
		resource->max_num_enc = 0;
		resource->max_num_enc_msb = 1;
	}
	enc_cfg = resource->enc_cfg;
	for (i = 0; i < IPCAM_RECORD_MAX_NUM_ENC; ++i) {
		calc_roundup_size(param->stream_max_size[i].width,
			param->stream_max_size[i].height, IAV_ENCODE_H264, &max_w, &max_h);
		enc_cfg[i].max_enc_width = max_w;
		enc_cfg[i].max_enc_height = max_h;
		enc_cfg[i].max_GOP_M = param->stream_max_GOP_M[i];
		enc_cfg[i].vert_search_range_2x = param->stream_2x_search_range[i];
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
		enc_cfg[i].max_num_ref_p = param->debug_max_ref_P[i];
#endif
	}
	if (unlikely(param->max_num_encode_streams > IPCAM_RECORD_MAX_NUM_ENC)) {
		ext_cmd = (DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT *)G_max_enc_cfg_addr;
		enc_cfg = ext_cmd[G_dsp_enc_mode].enc_cfg;
		for (i = 0, j = IPCAM_RECORD_MAX_NUM_ENC;
			i < IPCAM_RECORD_MAX_NUM_ENC_EXT; ++i, ++j) {
			calc_roundup_size(param->stream_max_size[j].width,
				param->stream_max_size[j].height, IAV_ENCODE_H264, &max_w,
				&max_h);
			enc_cfg[i].max_enc_width = max_w;
			enc_cfg[i].max_enc_height = max_h;
			enc_cfg[i].max_GOP_M = param->stream_max_GOP_M[j];
			enc_cfg[i].vert_search_range_2x = param->stream_2x_search_range[j];
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
			enc_cfg[i].max_num_ref_p = param->debug_max_ref_P[i];
#endif
		}
	}

	/* Update other global variables */
	G_iav_vcap.sharpen_b_enable[G_dsp_enc_mode] = !!param->sharpen_b_enable;
	G_iav_vcap.enc_from_raw_enable[G_dsp_enc_mode] = !!param->enc_from_raw_enable;
	G_iav_vcap.vskip_before_encode[G_dsp_enc_mode] = param->vskip_before_encode;
	G_iav_vcap.mixer_b_enable = param->mixer_b_enable;
	G_iav_vcap.vout_b_letter_boxing_enable = param->vout_b_letter_box_enable;
	G_iav_vcap.yuv_input_enhanced = param->yuv_input_enhanced;

	/* Update VCA buffer (preview copied buffer) */
	G_iav_vcap.debug_chip_id = param->debug_chip_id;
	G_iav_obj.dsp_chip_id = (param->debug_chip_id != IAV_CHIP_ID_S2_UNKNOWN) ?
		 param->debug_chip_id : dsp_get_chip_id();
	G_iav_vcap.vca_src_id = param->vca_buf_src_id;
	G_iav_obj.system_resource = resource;
	G_iav_obj.system_setup_info = &G_system_setup_info[G_dsp_enc_mode];
	G_iav_info.high_mega_pixel_enable = is_high_mp_mode();
	G_iav_info.vin_num = get_vin_num(G_dsp_enc_mode);
	G_iav_info.hdr_mode = is_hdr_frame_interleaved_mode() ?
		IAV_HDR_FRM_INTERLEAVED : (is_hdr_line_interleaved_mode() ?
		IAV_HDR_LINE_INTERLEAVED : IAV_HDR_NONE);

	//map dsp save mode or not.
	G_dsp_partition.map_flag = param->map_dsp_partition;
}

static inline int update_cmd_read_delay(int delay)
{
	u32 cmd_read_delay;
	/* Fixme: Negative cmd read delay is for debug only. */
	delay = (delay >= MIN_CMD_READ_DELAY_IN_MS ?
		delay : MIN_CMD_READ_DELAY_IN_MS);
	if ((G_dsp_enc_mode == DSP_FULL_FPS_FULL_PERF_MODE) &&
		(delay > MIN_CMD_READ_DELAY_IN_MS)) {
		/* Cmd read delay cannot be longer than 4ms in mode 7.
		 * Otherwise, it might block VIN and cause VIN FPS drop to lower
		 * than 120 fps.
		 */
		iav_warning("Cmd read delay [%d] cannot be longer than [%d] ms in "
			"mode [Full FPS (FP)].\n", delay, MIN_CMD_READ_DELAY_IN_MS);
		delay = MIN_CMD_READ_DELAY_IN_MS;
	}
	if (delay >= 0) {
		cmd_read_delay = (u32) (delay * AUDIO_CLK_KHZ);
	} else {
		cmd_read_delay = (u32) (-delay * AUDIO_CLK_KHZ) | (1 << 31);
	}
	G_iav_vcap.cmd_read_delay = cmd_read_delay;
	return 0;
}

static void prepare_vin_capture_win(vin_cap_win_t *vin_cap_win)
{
	struct amba_vin_cap_win_info *vin_cap_info;
	struct amba_vin_src_capability *vin_info = get_vin_capability();

	vin_cap_info = &get_vin_capability()->vin_cap_info;
	vin_cap_win->s_ctrl_reg = vin_cap_info->s_ctrl_reg;
	vin_cap_info->s_inp_cfg_reg =
		((vin_cap_info->s_inp_cfg_reg & (~0x60)) | ((vin_info->bit_resolution & 0x6) << 4));
	vin_cap_win->s_inp_cfg_reg = vin_cap_info->s_inp_cfg_reg;
	vin_cap_win->s_v_width_reg = vin_cap_info->s_v_width_reg;
	vin_cap_win->s_h_width_reg = vin_cap_info->s_h_width_reg;
	vin_cap_win->s_h_offset_top_reg = vin_cap_info->s_h_offset_top_reg;
	vin_cap_win->s_h_offset_bot_reg = vin_cap_info->s_h_offset_bot_reg;
	vin_cap_win->s_v_reg = vin_cap_info->s_v_reg;
	vin_cap_win->s_h_reg = vin_cap_info->s_h_reg;
	vin_cap_win->s_min_v_reg = vin_cap_info->s_min_v_reg;
	vin_cap_win->s_min_h_reg = vin_cap_info->s_min_h_reg;
	vin_cap_win->s_trigger_0_start_reg = vin_cap_info->s_trigger_0_start_reg;
	vin_cap_win->s_trigger_0_end_reg = vin_cap_info->s_trigger_0_end_reg;
	vin_cap_win->s_trigger_1_start_reg = vin_cap_info->s_trigger_1_start_reg;
	vin_cap_win->s_trigger_1_end_reg = vin_cap_info->s_trigger_1_end_reg;
	vin_cap_win->s_vout_start_0_reg = vin_cap_info->s_vout_start_0_reg;
	vin_cap_win->s_vout_start_1_reg = vin_cap_info->s_vout_start_1_reg;
	vin_cap_win->s_cap_start_v_reg = vin_cap_info->s_cap_start_v_reg;
	vin_cap_win->s_cap_start_h_reg = vin_cap_info->s_cap_start_h_reg;
	vin_cap_win->s_cap_end_v_reg = vin_cap_info->s_cap_end_v_reg;
	vin_cap_win->s_cap_end_h_reg = vin_cap_info->s_cap_end_h_reg;
	vin_cap_win->s_blank_leng_h_reg = vin_cap_info->s_blank_leng_h_reg;
	vin_cap_win->vsync_timeout = 0xffff0000;
	vin_cap_win->hsync_timeout = 0xffff0000;
	vin_cap_win->mipi_cfg1_reg = vin_cap_info->mipi_cfg1_reg;
	vin_cap_win->mipi_cfg2_reg = vin_cap_info->mipi_cfg2_reg;
	vin_cap_win->mipi_bdphyctl_reg = vin_cap_info->mipi_bdphyctl_reg;
	vin_cap_win->mipi_sdphyctl_reg = vin_cap_info->mipi_sdphyctl_reg;

	//SLVDS control
	vin_cap_win->slvs_control = vin_cap_info->slvs_control;
	vin_cap_win->slvs_frame_line_w = vin_cap_info->s_timeout_h_low_reg;
	vin_cap_win->slvs_act_frame_line_w = vin_cap_info->s_timeout_v_low_reg;
	vin_cap_win->slvs_lane_mux_select[0] = vin_cap_info->slvs_lane_mux_select[0];
	vin_cap_win->slvs_lane_mux_select[1] = vin_cap_info->slvs_lane_mux_select[1];
	vin_cap_win->slvs_lane_mux_select[2] = vin_cap_info->slvs_lane_mux_select[2];
	vin_cap_win->slvs_debug = vin_cap_info->slvs_debug;
}

/******************************************
 *	DSP API functions
 ******************************************/

// 0x01000001
static void cmd_set_camera_record_mode(void)
{
	DSP_SET_OP_MODE_RECORD_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_DSP_SET_OPERATION_MODE;
	dsp_cmd.dsp_op_mode = DSP_OP_MODE_CAMERA_RECORD;

	dsp_cmd.mode_flags = 0x1;
	dsp_cmd.num_of_chans = 1;

	dsp_cmd.enc_chan_cfgs[0].enc_type_full_res = 2;
	dsp_cmd.enc_chan_cfgs[0].enc_type_pip = 0;
	dsp_cmd.enc_chan_cfgs[0].enc_frm_rate = 0;
	dsp_cmd.enc_chan_cfgs[0].enc_flags = 1; //0: no MCTF 1: MCTF sharpened 2: MCTF unsharpened
	dsp_cmd.enc_chan_cfgs[0].enc_width_full_res = 1920;
	dsp_cmd.enc_chan_cfgs[0].enc_height_full_res = 1080;
	dsp_cmd.enc_chan_cfgs[0].enc_width_pip = 320;
	dsp_cmd.enc_chan_cfgs[0].enc_height_pip = 240;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, dsp_op_mode);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_set_camera_playback_mode(void)
{
	DSP_SET_OP_MODE_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_DSP_SET_OPERATION_MODE;
	dsp_cmd.dsp_op_mode = DSP_OP_MODE_CAMERA_PLAYBACK;
	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, dsp_op_mode);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_set_ipcam_record_mode(void)
{
	u8 vout_swap;
	u16 max_w, max_h, max_pre_w, max_pre_h;
	iav_source_buffer_type_ex_t type;
	struct amba_video_info * video_info_a = NULL, *video_info_b;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD dsp_cmd, *resource = NULL;

	resource = &G_system_resource_setup[G_dsp_enc_mode];
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd = *resource;
	dsp_cmd.cmd_code = CMD_DSP_SET_OPERATION_MODE;
	dsp_cmd.dsp_op_mode = DSP_OP_MODE_IP_CAMERA_RECORD;

	dsp_cmd.preview_B_for_enc = is_buf_type_enc(IAV_ENCODE_SOURCE_THIRD_BUFFER);
	dsp_cmd.preview_A_for_enc = is_buf_type_enc(IAV_ENCODE_SOURCE_FOURTH_BUFFER);
	// Resize maximum of the main buffer
	calc_roundup_size(G_source_buffer[0].size.width, G_source_buffer[0].size.height,
		IAV_ENCODE_H264, &max_w, &max_h);
	if (is_warp_mode()) {
		dsp_cmd.max_warped_main_width = max_w;
		dsp_cmd.max_warped_main_height = max_h;
		dsp_cmd.max_warped_region_input_width =
			resource->max_warped_region_input_width;
		dsp_cmd.max_warped_region_output_width =
			resource->max_warped_region_output_width;
		calc_roundup_size(G_cap_pre_main.size.width,
			G_cap_pre_main.size.height, IAV_ENCODE_H264, &max_pre_w, &max_pre_h);
		dsp_cmd.max_main_width = max_pre_w;
		dsp_cmd.max_main_height = max_pre_h;
	} else {
		dsp_cmd.max_main_width = max_w;
		dsp_cmd.max_main_height = max_h;
	}

	// Resize maximum of the second buffer
	type = G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER].type;
	switch (type) {
	case IAV_SOURCE_BUFFER_TYPE_OFF:
		max_w = max_h = 0;
		break;
	case IAV_SOURCE_BUFFER_TYPE_ENCODE:
		calc_roundup_size(resource->max_preview_C_width,
			resource->max_preview_C_height, IAV_ENCODE_H264, &max_w, &max_h);
		break;
	default:
		iav_error("Invalid 2nd source buffer type [%d]!\n", type);
		return;
		break;
	}
	dsp_cmd.max_preview_C_width = max_w;
	dsp_cmd.max_preview_C_height = max_h;

	vout_swap = G_system_setup_info[G_dsp_enc_mode].vout_swap;
	if (vout_swap) {
		video_info_a = &G_iav_info.pvoutinfo[1]->video_info;
		video_info_b = &G_iav_info.pvoutinfo[0]->video_info;
	} else {
		video_info_a = &G_iav_info.pvoutinfo[0]->video_info;
		video_info_b = &G_iav_info.pvoutinfo[1]->video_info;
	}

	// Resize maximum of the third buffer
	type = G_source_buffer[IAV_ENCODE_SOURCE_THIRD_BUFFER].type;
	if (get_preview_size(resource->max_preview_B_width,
			resource->max_preview_B_height, type, video_info_b,
			&max_w, &max_h) < 0) {
		iav_error("Failed to get max size for 3rd buffer!\n");
		return;
	}
	dsp_cmd.max_preview_B_width = max_w;
	if (type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
		dsp_cmd.max_preview_B_height = max_h * get_vin_num(G_dsp_enc_mode);
		/* Vout B with letter boxing config when mixer B is off. */
		if (is_vout_b_letter_boxing_enabled()) {
			dsp_cmd.prev_B_extra_line_at_top =
				G_iav_info.pvoutinfo[1]->active_mode.video_offset.offset_y ?
				(G_iav_info.pvoutinfo[1]->active_mode.video_offset.offset_y + 1) : 0;
			dsp_cmd.prev_B_extra_line_at_bot =
				G_iav_info.pvoutinfo[1]->active_mode.video_size.vout_height -
				G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height -
				dsp_cmd.prev_B_extra_line_at_top;
		}
	} else {
		dsp_cmd.max_preview_B_height = max_h;
	}

	// Resize maximum of the fourth buffer
	type = G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER].type;
	if (get_preview_size(resource->max_preview_A_width,
			resource->max_preview_A_height, type, video_info_a,
			&max_w, &max_h) < 0) {
		iav_error("Failed to get max size for 4th buffer!\n");
		return;
	}
	dsp_cmd.max_preview_A_width = max_w;
	if (type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
		dsp_cmd.max_preview_A_height = max_h * get_vin_num(G_dsp_enc_mode);
	} else {
		dsp_cmd.max_preview_A_height = max_h;
	}

	// Resize maximum of the streams
	if (likely((resource->max_num_enc <= IPCAM_RECORD_MAX_NUM_ENC) &&
		!resource->max_num_enc_msb)) {
		dsp_cmd.set_op_mode_ext_daddr = 0;
	} else {
		clean_cache_aligned((u8 *)resource->set_op_mode_ext_daddr,
			DSP_ENC_CFG_EXT_SIZE);
		dsp_cmd.set_op_mode_ext_daddr = (u32)VIRT_TO_DSP(
			(u8 *)resource->set_op_mode_ext_daddr);
	}

	// Add 16 lines more to avoid roundup issue in dewarp mode.
	// It's to fix the 16 lines alignment for each warp region.
	if (is_warp_mode()) {
		dsp_cmd.max_main_height += PIXEL_IN_MB;
		if (dsp_cmd.max_preview_C_height > 0) {
			dsp_cmd.max_preview_C_height += PIXEL_IN_MB;
		}
		if (dsp_cmd.max_preview_A_height > 0) {
			dsp_cmd.max_preview_A_height += PIXEL_IN_MB;
		}
	}

	/* Increase two extra buffers when preview C copy is enabled. */
	if (G_iav_vcap.vca_src_id) {
		dsp_cmd.extra_dram_buf_prev_c += IAV_EXTRA_BUF_FOR_VCA_CP;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, dsp_op_mode);
	iav_dsp(dsp_cmd, mode_flags);
	iav_dsp(dsp_cmd, max_num_enc);
	iav_dsp(dsp_cmd, max_num_enc_msb);
	iav_dsp_hex(dsp_cmd, set_op_mode_ext_daddr);
	iav_dsp(dsp_cmd, preview_A_for_enc);
	iav_dsp(dsp_cmd, preview_B_for_enc);
	iav_dsp(dsp_cmd, enc_rotation);
	iav_dsp(dsp_cmd, hdr_num_exposures_minus_1);
	iav_dsp(dsp_cmd, raw_compression_disabled);
	iav_dsp(dsp_cmd, num_vin_minus_2);
	iav_dsp(dsp_cmd, extra_dram_buf);
	iav_dsp(dsp_cmd, extra_dram_buf_prev_c);
	iav_dsp(dsp_cmd, extra_dram_buf_prev_b);
	iav_dsp(dsp_cmd, extra_dram_buf_prev_a);
	iav_dsp(dsp_cmd, extra_buf_msb_ext);
	iav_dsp(dsp_cmd, extra_buf_msb_ext_prev_c);
	iav_dsp(dsp_cmd, extra_buf_msb_ext_prev_b);
	iav_dsp(dsp_cmd, extra_buf_msb_ext_prev_a);
	iav_dsp(dsp_cmd, h_warp_bypass);
	iav_dsp(dsp_cmd, max_main_width);
	iav_dsp(dsp_cmd, max_main_height);
	iav_dsp(dsp_cmd, max_preview_C_width);
	iav_dsp(dsp_cmd, max_preview_C_height);
	iav_dsp(dsp_cmd, max_preview_B_width);
	iav_dsp(dsp_cmd, max_preview_B_height);
	iav_dsp(dsp_cmd, prev_B_extra_line_at_top);
	iav_dsp(dsp_cmd, prev_B_extra_line_at_bot);
	iav_dsp(dsp_cmd, max_preview_A_width);
	iav_dsp(dsp_cmd, max_preview_A_height);
	iav_dsp(dsp_cmd, max_warped_main_width);
	iav_dsp(dsp_cmd, max_warped_main_height);
	iav_dsp(dsp_cmd, max_warped_region_input_width);
	iav_dsp(dsp_cmd, max_warped_region_output_width);
	iav_dsp(dsp_cmd, max_vin_stats_num_lines_top);
	iav_dsp(dsp_cmd, max_vin_stats_num_lines_bot);
	iav_dsp(dsp_cmd, max_chroma_filter_radius);
	iav_dsp(dsp_cmd, enc_cfg[0].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[0].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[0].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[0].vert_search_range_2x);
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	iav_dsp(dsp_cmd, enc_cfg[0].max_num_ref_p);
#endif
	iav_dsp(dsp_cmd, enc_cfg[1].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[1].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[1].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[1].vert_search_range_2x);
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	iav_dsp(dsp_cmd, enc_cfg[1].max_num_ref_p);
#endif
	iav_dsp(dsp_cmd, enc_cfg[2].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[2].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[2].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[2].vert_search_range_2x);
	iav_dsp(dsp_cmd, enc_cfg[3].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[3].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[3].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[3].vert_search_range_2x);
	iav_dsp(dsp_cmd, enc_cfg[4].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[4].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[4].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[4].vert_search_range_2x);
	iav_dsp(dsp_cmd, enc_cfg[5].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[5].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[5].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[5].vert_search_range_2x);
	iav_dsp(dsp_cmd, enc_cfg[6].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[6].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[6].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[6].vert_search_range_2x);
	iav_dsp(dsp_cmd, enc_cfg[7].max_enc_width);
	iav_dsp(dsp_cmd, enc_cfg[7].max_enc_height);
	iav_dsp(dsp_cmd, enc_cfg[7].max_GOP_M);
	iav_dsp(dsp_cmd, enc_cfg[7].vert_search_range_2x);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_set_operation_mode(DSP_OP_MODE mode)
{
	switch (mode) {
	case DSP_OP_MODE_CAMERA_RECORD:
		cmd_set_camera_record_mode();
		break;
	case DSP_OP_MODE_CAMERA_PLAYBACK:
		cmd_set_camera_playback_mode();
		break;
	case DSP_OP_MODE_IP_CAMERA_RECORD:
		cmd_set_ipcam_record_mode();
		break;
	default:
		iav_error("Unknown DSP operation mode %d.\n", mode);
		break;
	}
}

// 0x08000001
static void cmd_vcap_setup(void)
{
	#define	PTS_BASE			(90000)
	u8 vout_swap;
	u16 width, height, multi_frames;
	u32 vin_frame_rate;
	VCAP_SETUP_CMD dsp_cmd;
	iav_reso_ex_t max;
	iav_source_buffer_ex_t* buffer = NULL;
	struct iav_mem_block * mem = NULL;
	struct amba_video_info * video_info = NULL;
	struct amba_iav_vout_info * vout_info_a = NULL, *vout_info_b = NULL;
	struct amba_vin_src_capability *vin_info = get_vin_capability();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_SETUP;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = 0x11;	// not used
	dsp_cmd.vid_skip = 1;
	dsp_cmd.input_format = vin_info->input_format;

	dsp_cmd.keep_states = 0;
	dsp_cmd.interlaced_output = 0;		//false
	dsp_cmd.vidcap_w = vin_info->cap_cap_w;
	dsp_cmd.vidcap_h = vin_info->cap_cap_h;
	dsp_cmd.input_center_x = (vin_info->cap_cap_w << 15);
	dsp_cmd.input_center_y = (vin_info->cap_cap_h << 15);
	dsp_cmd.zoom_factor_x = 0;
	dsp_cmd.zoom_factor_y = 0;

	// Sensor related
	dsp_cmd.sensor_id = 0xFF;
	dsp_cmd.sensor_readout_mode = vin_info->sensor_readout_mode;

	// Capture pre main buffer and main buffer setup
	if (is_warp_mode()) {
		buffer = &G_cap_pre_main;
		dsp_cmd.vwarp_blk_h = 64;
	} else {
		buffer = &G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER];
		dsp_cmd.vwarp_blk_h = 32;
	}
	dsp_cmd.main_w = buffer->size.width;
	dsp_cmd.main_h = buffer->size.height;

	// Turbo cmd related
	dsp_cmd.CmdReadDly = G_iav_vcap.cmd_read_delay;

	//EIS delay count
	dsp_cmd.eis_delay_count = G_system_setup_info[G_dsp_enc_mode].eis_delay_count;

	// Noise reduction filter setup
	dsp_cmd.noise_filter_strength = 0x2;  //enable MCTF
	dsp_cmd.mctf_chan = 0x0;
	dsp_cmd.sharpen_b_chan = G_iav_vcap.sharpen_b_enable[G_dsp_enc_mode];
	dsp_cmd.cc_en = 0x0;
	dsp_cmd.cmpr_en = 0x0;	//turn off MCTF compression
	dsp_cmd.cmpr_dither = 0x0;
	dsp_cmd.mode = 0x0;

	dsp_cmd.image_stabilize_strength = 0; //default
	dsp_cmd.bit_resolution = vin_info->bit_resolution;
	dsp_cmd.bayer_pattern = vin_info->bayer_pattern;

//#ifdef BUILD_AMBARELLA_EIS
#if 0
	dsp_cmd.eis_update_addr = (G_iav_vcap.eis_update_addr == 0) ?
		0 : ambarella_virt_to_phys(G_iav_vcap.eis_update_addr);
#endif

	vout_swap = G_system_setup_info[G_dsp_enc_mode].vout_swap;
	if (vout_swap) {
		vout_info_a = G_iav_info.pvoutinfo[1];
		vout_info_b = G_iav_info.pvoutinfo[0];
	} else {
		vout_info_a = G_iav_info.pvoutinfo[0];
		vout_info_b = G_iav_info.pvoutinfo[1];
	}
	dsp_cmd.vout_swap = vout_swap;

	// Preview A
	video_info = &vout_info_a->video_info;
	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER];
	if (get_preview_size(buffer->size.width, buffer->size.height,
		buffer->type, video_info, &width, &height) < 0) {
		iav_error("Failed to get 4th source buffer size!\n");
		return;
	}
	dsp_cmd.preview_frame_rate_A = amba_iav_fps_to_fps(video_info->fps);
	if (is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		if (!vout_info_a->enabled) {
			dsp_cmd.preview_format_A = AMBA_DSP_VIDEO_FORMAT_NO_VIDEO;
			dsp_cmd.preview_A_src = 0xF;
			width = 0;
			height = 0;
		} else {
			dsp_cmd.preview_format_A = amba_iav_format_to_format(
				video_info->format);
			dsp_cmd.preview_A_src = DRAM_PREVIEW;
		}
	}
	dsp_cmd.preview_w_A = width;
	dsp_cmd.preview_h_A = height;
	dsp_cmd.preview_A_pipeline_pos = buffer->unwarp;

	// Preview B
	video_info = &vout_info_b->video_info;
	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_THIRD_BUFFER];
	if (get_preview_size(buffer->size.width, buffer->size.height,
		buffer->type, video_info, &width, &height) < 0) {
		iav_error("Failed to get 3rd source buffer size!\n");
		return;
	}
	dsp_cmd.preview_frame_rate_B = amba_iav_fps_to_fps(video_info->fps);
	if (is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		if (!vout_info_b->enabled) {
			dsp_cmd.preview_format_B = AMBA_DSP_VIDEO_FORMAT_NO_VIDEO;
			dsp_cmd.preview_B_src = 7;
			dsp_cmd.preview_B_stop = 1;
			width = 0;
			height = 0;
		} else {
			dsp_cmd.preview_format_B = amba_iav_format_to_format(
				video_info->format);
			dsp_cmd.preview_B_src = DRAM_PREVIEW;
		}
	}
	dsp_cmd.preview_w_B = width;

	/* Vout B with letter boxing config when mixer B is off. */
	if (is_vout_b_letter_boxing_enabled()) {
		dsp_cmd.preview_h_B =
			G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height;
	} else {
		dsp_cmd.preview_h_B = height;
	}

	dsp_cmd.preview_B_pipeline_pos = buffer->unwarp;

	// Preview C
	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER];
	get_max_encode_buffer_size(IAV_ENCODE_SOURCE_SECOND_BUFFER, &max);
	dsp_cmd.pip_w = is_buf_type_enc(IAV_ENCODE_SOURCE_SECOND_BUFFER) ?
		(buffer->enc_stop ? max.width : buffer->size.width) : 0;
	dsp_cmd.pip_h = is_buf_type_enc(IAV_ENCODE_SOURCE_SECOND_BUFFER) ?
		(buffer->enc_stop ? max.height : buffer->size.height) : 0;
	dsp_cmd.pip_pipeline_pos = buffer->unwarp;
	dsp_cmd.pip_frame_dec_rate = 1;

	// Others
	dsp_cmd.no_pipelineflush = 0;
	dsp_cmd.nbr_prevs_x = 1;  //obsolete
	dsp_cmd.nbr_prevs_y = 1;  //obsolete

	// IDSP output frame rate & PTS
	multi_frames = is_hdr_frame_interleaved_mode() ?
		get_expo_num(get_enc_mode()) : 1;
	vin_frame_rate = amba_iav_fps_format_to_vfr(vin_info->frame_rate,
		vin_info->video_format, multi_frames);
	dsp_cmd.vin_frame_rate = vin_frame_rate;
	dsp_cmd.idsp_out_frame_rate = vin_frame_rate;
	iav_printk("video frame rate %d, video format %d, vfr %d.\n",
		vin_info->frame_rate, vin_info->video_format, vin_frame_rate);
	dsp_cmd.vcap_cmd_msg_dec_rate = 1; // is VIN fps / 60
	dsp_cmd.vin_frame_rate_frac = 0;
	dsp_cmd.idsp_out_frame_rate_frac = 0;
	dsp_cmd.pts_delta = is_hdr_frame_interleaved_mode() ?
		((PTS_BASE / vin_frame_rate * multi_frames) << 16) : 0;

	dsp_cmd.video_delay_mode = 0; //unknown, keep 0 as default
	dsp_cmd.bbar_sz_mbs = 0; //unknown, keep 0 as default

	if (is_hiso_video_mode()) {
		iav_get_mem_block(IAV_MMAP_IMGPROC, &mem);
		dsp_cmd.hiLowISO_proc_cfg_ptr = PHYS_TO_DSP(mem->phys_start +
			IMG_KERNEL_OFFSET);
	}
	if (is_hdr_mode()) {
		dsp_cmd.hdr_data_read_protocol = vin_info->hdr_data_read_protocol;
	}
	dsp_cmd.hdr_based_pm = !is_mctf_pm_enabled();

	//yuv_input_fps_enhanced setup
	dsp_cmd.yuv_input_fps_enhanced = G_iav_vcap.yuv_input_enhanced;

	// Preview copy buffer for VCA
	dsp_cmd.preview_cp_id = G_iav_vcap.vca_src_id ?
		G_capture_source[G_iav_vcap.vca_src_id] + 1 : 0;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, vid_skip);
	iav_dsp(dsp_cmd, input_format);
	iav_dsp(dsp_cmd, sensor_id);
	iav_dsp(dsp_cmd, keep_states);
	iav_dsp(dsp_cmd, interlaced_output);
	iav_dsp(dsp_cmd, vidcap_w);
	iav_dsp(dsp_cmd, vidcap_h);

	iav_dsp(dsp_cmd, main_w);
	iav_dsp(dsp_cmd, main_h);
	iav_dsp(dsp_cmd, input_center_x);
	iav_dsp(dsp_cmd, input_center_y);
	iav_dsp(dsp_cmd, zoom_factor_x);
	iav_dsp(dsp_cmd, zoom_factor_y);
	iav_dsp(dsp_cmd, CmdReadDly);
	iav_dsp(dsp_cmd, eis_delay_count);
	iav_dsp(dsp_cmd, sensor_readout_mode);

	iav_dsp(dsp_cmd, noise_filter_strength);
	iav_dsp(dsp_cmd, mctf_chan);
	iav_dsp(dsp_cmd, sharpen_b_chan);

	iav_dsp(dsp_cmd, cc_en);
	iav_dsp(dsp_cmd, cmpr_en);
	iav_dsp(dsp_cmd, cmpr_dither);
	iav_dsp(dsp_cmd, mode);
	iav_dsp(dsp_cmd, image_stabilize_strength);
	iav_dsp(dsp_cmd, bit_resolution);
	iav_dsp(dsp_cmd, bayer_pattern);

	iav_dsp(dsp_cmd, no_pipelineflush);
	iav_dsp(dsp_cmd, preview_A_src);
	iav_dsp(dsp_cmd, preview_w_A);
	iav_dsp(dsp_cmd, preview_h_A);
	iav_dsp(dsp_cmd, preview_format_A);
	iav_dsp(dsp_cmd, preview_frame_rate_A);
	iav_dsp(dsp_cmd, preview_A_pipeline_pos);

	iav_dsp(dsp_cmd, preview_B_src);
	iav_dsp(dsp_cmd, preview_B_stop);
	iav_dsp(dsp_cmd, preview_w_B);
	iav_dsp(dsp_cmd, preview_h_B);
	iav_dsp(dsp_cmd, preview_format_B);
	iav_dsp(dsp_cmd, preview_frame_rate_B);
	iav_dsp(dsp_cmd, preview_B_pipeline_pos);

	iav_dsp(dsp_cmd, pip_w);
	iav_dsp(dsp_cmd, pip_h);
	iav_dsp(dsp_cmd, pip_frame_dec_rate);
	iav_dsp(dsp_cmd, pip_pipeline_pos);

	iav_dsp(dsp_cmd, nbr_prevs_x);
	iav_dsp(dsp_cmd, nbr_prevs_y);
	iav_dsp(dsp_cmd, vin_frame_rate);
	iav_dsp(dsp_cmd, idsp_out_frame_rate);
	iav_dsp(dsp_cmd, vcap_cmd_msg_dec_rate);
	iav_dsp(dsp_cmd, vin_frame_rate_frac);
	iav_dsp(dsp_cmd, idsp_out_frame_rate_frac);
	iav_dsp(dsp_cmd, pts_delta);

	iav_dsp(dsp_cmd, video_delay_mode);
	iav_dsp_hex(dsp_cmd, eis_update_addr);
	iav_dsp(dsp_cmd, bbar_sz_mbs);
	iav_dsp(dsp_cmd, vwarp_blk_h);
	iav_dsp(dsp_cmd, vout_swap);
	iav_dsp(dsp_cmd, hdr_data_read_protocol);
	iav_dsp(dsp_cmd, hdr_based_pm);
	iav_dsp(dsp_cmd, preview_cp_id);

	/* Just for test enc dummy latency */
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	dsp_cmd.enc_dummy_latency =
		G_system_setup_info[G_dsp_enc_mode].debug_enc_dummy_latency_count;
	iav_dsp(dsp_cmd, enc_dummy_latency);
#else
	iav_dsp(dsp_cmd, yuv_input_fps_enhanced);
#endif
	iav_dsp_hex(dsp_cmd, hiLowISO_proc_cfg_ptr);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

/* 0x08000003
 * buffer_id = 1: second buffer, 2: third buffer, 3: fourth buffer
 * This command must be issued when capture buffer is used for encoding.
 */
static void cmd_capture_buffer_setup(int buffer_id)
{
	VCAP_PREV_SETUP_CMD dsp_cmd;
	iav_source_buffer_ex_t * buffer = NULL;

	buffer = &G_source_buffer[buffer_id];

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_PREV_SETUP;
	dsp_cmd.preview_id = G_capture_source[buffer_id];
	dsp_cmd.preview_format = amba_iav_format_to_format(
		AMBA_VIDEO_FORMAT_PROGRESSIVE);
	dsp_cmd.preview_stop = buffer->enc_stop;

	if (unlikely(buffer->enc_stop)) {
		dsp_cmd.preview_w = 0;
		dsp_cmd.preview_h = 0;
	} else {
		dsp_cmd.preview_w = buffer->size.width;
		dsp_cmd.preview_h = buffer->size.height;
	}

	/* Fixme: Second buffer is not able to do up-sampling.
	if (unlikely(buffer->enc_stop &&
		(buffer_id == IAV_ENCODE_SOURCE_SECOND_BUFFER))) {
		dsp_cmd.preview_w = MIN(buffer->size.width,
			G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.width);
		dsp_cmd.preview_h = MIN(buffer->size.height,
			G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.height);
	}
	*/

	dsp_cmd.preview_src_w = buffer->input.width ? buffer->input.width
		: (buffer->unwarp ? G_cap_pre_main.size.width
			: G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.width);
	dsp_cmd.preview_src_h = buffer->input.height ? buffer->input.height
		: (buffer->unwarp ? G_cap_pre_main.size.height
			: G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.height);
	dsp_cmd.preview_src_x_offset = buffer->input.x;
	dsp_cmd.preview_src_y_offset = buffer->input.y;
	if (buffer_id == IAV_ENCODE_SOURCE_SECOND_BUFFER) {
		dsp_cmd.disable_extra_2x = G_source_buffer[buffer_id].disable_extra_2x;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, preview_id);
	iav_dsp(dsp_cmd, preview_format);
	iav_dsp(dsp_cmd, preview_w);
	iav_dsp(dsp_cmd, preview_h);
	iav_dsp(dsp_cmd, preview_frame_rate);
	iav_dsp(dsp_cmd, preview_src);
	iav_dsp(dsp_cmd, preview_stop);
	iav_dsp(dsp_cmd, preview_src_w);
	iav_dsp(dsp_cmd, preview_src_h);
	iav_dsp(dsp_cmd, preview_src_x_offset);
	iav_dsp(dsp_cmd, preview_src_y_offset);
	iav_dsp(dsp_cmd, disable_extra_2x);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x08000005
static void cmd_vcap_timer_mode(void *context,
		u8 interval)
{
	// scaler: 0 ~ 4, default 0
	// sleep time = 16 msec >> scaler, default fps is 60
#define TIMER_MODE_INTERVAL_DEFAULT	0	// as current VDSP period
#define TIMER_MODE_INTERVAL_8MS		1
#define TIMER_MODE_INTERVAL_4MS		2
#define SHOW_LAST_IMAGE				0
#define SHOW_DEFAULT_IMAGE			1
#define TERM_FRAME_WAIT				0
#define TERM_RIGHT_AWAY				1

	VCAP_TMR_MODE_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_TMR_MODE;
	dsp_cmd.timer_scaler = TIMER_MODE_INTERVAL_DEFAULT;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, timer_scaler);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x08000013
static void cmd_set_vcap_mctf_gmv(void)
{
	VCAP_MCTF_GMV_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_MCTF_GMV;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, gmv);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0800001D
void cmd_update_capture_params_ex(int flags)
{
	u32 dsp_vin_fps, vin_fps;
	VCAP_UPDATE_CAPTURE_PARAMETERS_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_UPDATE_CAPTURE_PARAMETERS;

	vin_fps = get_vin_capability()->frame_rate;

	if (flags & UPDATE_PREVIEW_FRAME_RATE_FLAG) {
		dsp_cmd.enable_flags |= UPDATE_PREVIEW_FRAME_RATE_FLAG;
		dsp_cmd.preview_id = 0;		// preview A only
		dsp_cmd.frame_rate_division_factor =
			G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER].preview_framerate_division_factor;
	}

	if (flags & UPDATE_CUSTOM_VIN_FPS_FLAG) {
		dsp_cmd.enable_flags |= UPDATE_CUSTOM_VIN_FPS_FLAG;
		if (calc_encode_frame_rate(vin_fps, 1, 1, &dsp_vin_fps) < 0) {
			iav_error("Failed to calculate fps from VIN 0x%x.\n", vin_fps);
			return;
		}
		dsp_cmd.custom_vin_frame_rate = dsp_vin_fps;
	}

	if (flags & UPDATE_FREEZE_ENABLED_FLAG) {
		dsp_cmd.enable_flags |= UPDATE_FREEZE_ENABLED_FLAG;
		dsp_cmd.freeze_en = G_iav_vcap.freeze_enable;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, enable_flags);
	if (flags & UPDATE_PREVIEW_FRAME_RATE_FLAG) {
		iav_dsp(dsp_cmd, preview_id);
		iav_dsp(dsp_cmd, frame_rate_division_factor);
	}
	if (flags & UPDATE_CUSTOM_VIN_FPS_FLAG) {
		iav_dsp(dsp_cmd, custom_vin_frame_rate);
	}
	if (flags & UPDATE_FREEZE_ENABLED_FLAG) {
		iav_dsp(dsp_cmd, freeze_en);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0F000004
static void cmd_enc_dram_create_buf_pool(int chroma_format,
	iav_source_buffer_dram_ex_t * dram)
{
	MEMM_CREATE_FRM_BUF_POOL_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_MEMM_CREATE_FRM_BUF_POOL;
	dsp_cmd.max_num_of_frm_bufs = dram->max_frame_num;
	dsp_cmd.buf_width = dram->buf_max_size.width;
	dsp_cmd.buf_height = dram->buf_max_size.height;
	dsp_cmd.luma_img_width = dram->buf_max_size.width;
	dsp_cmd.luma_img_height = dram->buf_max_size.height;
	dsp_cmd.chroma_format = chroma_format;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, max_num_of_frm_bufs);
	iav_dsp(dsp_cmd, buf_width);
	iav_dsp(dsp_cmd, buf_height);
	iav_dsp(dsp_cmd, luma_img_width);
	iav_dsp(dsp_cmd, luma_img_height);
	iav_dsp(dsp_cmd, chroma_format);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0F000006
static void cmd_enc_dram_req_buffer(u16 buf_pool_id)
{
	MEMM_REQ_FRM_BUF_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_MEMM_REQ_FRM_BUF;
	dsp_cmd.frm_buf_pool_id = buf_pool_id;
	dsp_cmd.pic_struct = 3;		/* It should be 3 to singal frame */

/*	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, frm_buf_pool_id);
	iav_dsp(dsp_cmd, pic_struct);
*/
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0F00000D
static void cmd_get_frm_buf_pool_info(u16 buf_type, u16 fbp_id)
{
	MEMM_GET_FRM_BUF_POOL_INFO_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_MEMM_GET_FRM_BUF_POOL_INFO;
	dsp_cmd.callback_id = (buf_type << 8) + fbp_id;
	dsp_cmd.frm_buf_pool_type = buf_type;
	dsp_cmd.fbp_id = fbp_id;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, callback_id);
	iav_dsp(dsp_cmd, frm_buf_pool_type);
	iav_dsp(dsp_cmd, fbp_id);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x08000022
static void cmd_enc_dram_handshake(u16 buf_id)
{
	iav_source_buffer_dram_ex_t * dram = &G_source_buffer[buf_id].dram;
	iav_source_buffer_dram_pool_ex_t * pool = &dram->buf_pool;
	VCAP_ENC_FRM_DRAM_HANDSHAKE_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_ENC_FRM_DRAM_HANDSHAKE;
	dsp_cmd.cap_buf_id = pool->yuv_frame_id[pool->update_index];
	dsp_cmd.me1_buf_id = pool->me1_frame_id[pool->update_index];
	dsp_cmd.pts = dram->frame_pts;
	dsp_cmd.enc_src = buf_id;

/*	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, cap_buf_id);
	iav_dsp(dsp_cmd, me1_buf_id);
	iav_dsp(dsp_cmd, pts);
	iav_dsp(dsp_cmd, enc_src);
*/
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x09001008
static void cmd_set_vin_config(vin_cap_win_t *vin_cap_win)
{
	#define VIN_CONFIG_SIZE		0x54

	set_vin_config_t dsp_cmd;
	struct amba_vin_src_capability *pvin_capability;
	struct amba_vin_cap_win_info *vin;
	static u8 *iav_vin_config = NULL;

	if (!iav_vin_config) {
		//Fixme: Potential Memory Leakage
		iav_vin_config = kmalloc(VIN_CONFIG_SIZE, GFP_KERNEL);
	}
	if (!iav_vin_config) {
		return;
	}

	memset(vin_cap_win, 0, sizeof(vin_cap_win_t));
	prepare_vin_capture_win(vin_cap_win);
	memset(iav_vin_config, 0, VIN_CONFIG_SIZE);
	memcpy(iav_vin_config, &vin_cap_win->s_ctrl_reg, VIN_CONFIG_SIZE);
	clean_cache_aligned(iav_vin_config, VIN_CONFIG_SIZE);

	pvin_capability = get_vin_capability();
	vin = &pvin_capability->vin_cap_info;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = SET_VIN_CONFIG;
	dsp_cmd.vin_width = vin->s_cap_end_h_reg - vin->s_cap_start_h_reg + 1;
	dsp_cmd.vin_height = vin->s_cap_end_v_reg - vin->s_cap_start_v_reg + 1;
	dsp_cmd.vin_config_dram_addr = VIRT_TO_DSP(iav_vin_config);
	dsp_cmd.config_data_size = VIN_CONFIG_SIZE;
	dsp_cmd.sensor_resolution = pvin_capability->bit_resolution;
	dsp_cmd.sensor_bayer_pattern = pvin_capability->bayer_pattern;
	dsp_cmd.vin_cap_start_x = vin->s_cap_start_h_reg - pvin_capability->cap_start_x;
	dsp_cmd.vin_cap_start_y = vin->s_cap_start_v_reg - pvin_capability->cap_start_y;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, vin_width);
	iav_dsp(dsp_cmd, vin_height);
	iav_dsp(dsp_cmd, vin_cap_start_x);
	iav_dsp(dsp_cmd, vin_cap_start_y);
	iav_dsp_hex(dsp_cmd, vin_config_dram_addr);
	iav_dsp_hex(dsp_cmd, config_data_size);
	iav_dsp(dsp_cmd, sensor_resolution);
	iav_dsp(dsp_cmd, sensor_bayer_pattern);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0900100A
static void cmd_set_vin_cap_win_ext(vin_cap_win_t *vin_cap_win)
{
	vin_cap_win_ext_t dsp_cmd;
	struct amba_vin_cap_win_info *vin_cap_info;

	vin_cap_info = &get_vin_capability()->vin_cap_info;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = SET_VIN_CAPTURE_WIN_EXT;
	dsp_cmd.enhance_mode = vin_cap_info->enhance_mode;
	dsp_cmd.syncmap_mode = vin_cap_info->syncmap_mode;

	dsp_cmd.s_slvs_lane_mux_sel_0 = vin_cap_win->slvs_lane_mux_select[0];
	dsp_cmd.s_slvs_lane_mux_sel_1 = vin_cap_win->slvs_lane_mux_select[1];
	dsp_cmd.s_slvs_lane_mux_sel_2 = vin_cap_win->slvs_lane_mux_select[2];
	dsp_cmd.s_slvs_lane_mux_sel_3 = vin_cap_win->slvs_lane_mux_select[3];

	dsp_cmd.s_slvs_line_width = vin_cap_win->slvs_frame_line_w;
	dsp_cmd.s_slvs_active_width = vin_cap_win->slvs_act_frame_line_w;

	dsp_cmd.s_slvs_ctrl_0 = vin_cap_win->slvs_control;
	dsp_cmd.s_slvs_ctrl_1 = vin_cap_info->s_slvs_ctrl_1;
	dsp_cmd.s_slvs_sav_vzero_map = vin_cap_info->s_slvs_sav_vzero_map;
	dsp_cmd.s_slvs_sav_vone_map = vin_cap_info->s_slvs_sav_vone_map;
	dsp_cmd.s_slvs_eav_vzero_map = vin_cap_info->s_slvs_eav_vzero_map;
	dsp_cmd.s_slvs_eav_vone_map = vin_cap_info->s_slvs_eav_vone_map;
	dsp_cmd.s_slvs_vsync_horizontal_start = vin_cap_info->s_slvs_vsync_horizontal_start;
	dsp_cmd.s_slvs_vsync_horizontal_end = vin_cap_info->s_slvs_vsync_horizontal_end;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, s_slvs_ctrl_1);
	iav_dsp(dsp_cmd, enhance_mode);
	iav_dsp(dsp_cmd, syncmap_mode);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x09001002
static void cmd_sensor_config(void)
{
	sensor_input_setup_t dsp_cmd;
	struct amba_vin_src_capability *vin_info = get_vin_capability();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = SENSOR_INPUT_SETUP;
	dsp_cmd.sensor_id = vin_info->sensor_id;
	dsp_cmd.field_format = 1;
	dsp_cmd.sensor_resolution = vin_info->bit_resolution;
	dsp_cmd.sensor_pattern = vin_info->bayer_pattern;
	dsp_cmd.first_line_field_0 = 0;
	dsp_cmd.first_line_field_1 = 0;
	dsp_cmd.first_line_field_2 = 0;
	dsp_cmd.first_line_field_3 = 0;
	dsp_cmd.first_line_field_4 = 0;
	dsp_cmd.first_line_field_5 = 0;
	dsp_cmd.first_line_field_6 = 0;
	dsp_cmd.first_line_field_7 = 0;
	dsp_cmd.sensor_readout_mode = vin_info->sensor_readout_mode;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, sensor_id);
	iav_dsp(dsp_cmd, field_format);
	iav_dsp(dsp_cmd, sensor_readout_mode);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

/* 0x09005008  -- Warp control command
 * Make dummy window equal to VIN window for EIS mode, it will take more memory
 * bandwidth due to the large dummy window in DZ.
 * Make dummy window equal to the input window of main source buffer to save
 * memory bandwidth in NON-EIS mode.
 */
void cmd_warp_control(iav_rect_ex_t* main_input)
{
	u32 crop_w, crop_h, crop_x, crop_y, vin_w, vin_h;
	iav_rect_ex_t vin;
	iav_reso_ex_t* buffer = NULL;
	set_warp_control_t dsp_cmd;
	struct amba_vin_src_capability * cap = get_vin_capability();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = SET_WARP_CONTROL;
	get_vin_window(&vin);

	/* Assumption for HDR: The VIN size is larger than the actual capture
	 * size only in multiple exposures HDR mode */
	if (is_hdr_line_interleaved_mode() &&
		get_expo_num(get_enc_mode()) > MIN_HDR_EXPOSURES) {
		crop_x = main_input->x + vin.x;
		crop_y = main_input->y + vin.y;
	} else {
		crop_x = main_input->x;
		crop_y = main_input->y;
	}
	crop_w = main_input->width;
	crop_h = main_input->height;

	buffer = &G_cap_pre_main.size;
	dsp_cmd.zoom_x = ((u32)buffer->width << 16) / crop_w;
	dsp_cmd.zoom_y = ((u32)buffer->height << 16) / crop_h;

	if (is_vin_cap_offset_enabled()) {
		vin_w = crop_w;
		vin_h = crop_h;
	} else {
		vin_w = vin.width;
		vin_h = vin.height;
	}
	dsp_cmd.x_center_offset = ((crop_x - ((u32)vin_w - crop_w) / 2) << 16);
	dsp_cmd.y_center_offset = ((crop_y - ((u32)vin_h - crop_h) / 2) << 16);
	dsp_cmd.dummy_window_x_left = crop_x;
	dsp_cmd.dummy_window_y_top = crop_y;
	dsp_cmd.dummy_window_width = crop_w;
	dsp_cmd.dummy_window_height = crop_h;

	/* The width can't exceed 1920, when imx172/imx226 using 72M clock */
	if (is_full_framerate_mode() && ((cap->sensor_id == SONYIMX172)
		|| (cap->sensor_id == SONYIMX226))) {
		crop_w = (crop_w > 1920) ? 1920 : crop_w;
	} else {
		crop_w = (crop_w > MAX_WIDTH_IN_FULL_FPS) ?
			MAX_WIDTH_IN_FULL_FPS : crop_w;
	}

	dsp_cmd.cfa_output_width = crop_w;
	dsp_cmd.cfa_output_height = crop_h;

	dsp_cmd.actual_left_top_x = 0;
	dsp_cmd.actual_left_top_y = 0;
	dsp_cmd.actual_right_bot_x = (crop_w << 16);
	dsp_cmd.actual_right_bot_y = (crop_h << 16);
	dsp_cmd.extra_sec2_vert_out_vid_mode = 0;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, zoom_x);
	iav_dsp_hex(dsp_cmd, zoom_y);
	iav_dsp_hex(dsp_cmd, x_center_offset);
	iav_dsp_hex(dsp_cmd, y_center_offset);
	iav_dsp(dsp_cmd, dummy_window_x_left);
	iav_dsp(dsp_cmd, dummy_window_y_top);
	iav_dsp(dsp_cmd, dummy_window_width);
	iav_dsp(dsp_cmd, dummy_window_height);
	iav_dsp(dsp_cmd, cfa_output_width);
	iav_dsp(dsp_cmd, cfa_output_height);
	iav_dsp(dsp_cmd, actual_left_top_x);
	iav_dsp(dsp_cmd, actual_left_top_y);
	iav_dsp(dsp_cmd, actual_right_bot_x);
	iav_dsp(dsp_cmd, actual_right_bot_y);
	iav_dsp(dsp_cmd, extra_sec2_vert_out_vid_mode);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_raw_encode_setup(void)
{
	raw_encode_video_setup_cmd_t dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = RAW_ENCODE_VIDEO_SETUP_CMD;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

/******************************************
 *	External API functions for IAV module
 ******************************************/

int iav_init(struct iav_global_info *g_info, void *dev)
{
	int rval;

	// This initialize order CANNOT be changed !
	if ((rval = iav_dsp_init()) < 0)
		return rval;

	if ((rval = iav_setup_irq(dev)) < 0)
		return rval;

	if ((rval = nl_init()) < 0)
		return rval;

	return 0;
}

/******************************************
 *	IAV API functions
 ******************************************/

static inline void wait_enc_dram(u16 buf_id, DSP_ENC_DRAM_MODE mode)
{
	iav_source_buffer_dram_ex_t * dram = &G_source_buffer[buf_id].dram;
//	iav_printk("Entering ENC DRAM mode [%d]...\n", mode);
	wait_event_interruptible_timeout(dram->wq, (dram->buf_state == mode), HZ);
//	iav_printk("[Done] Entering ENC DRAM mode [%d]...\n", mode);
}

static int sync_vin_crop(iav_context_t *context,
	iav_rect_ex_t* input, int for_all_sensor)
{
	struct amba_vin_cap_window window;
	struct amba_vin_cap_win_info * vin;
	struct amba_vin_src_capability * cap = get_vin_capability();
	u8 vin_cap_flag;

	vin_cap_flag = (is_vin_cap_offset_enabled() &&
		(for_all_sensor || (cap->sensor_id == SONYIMX172)
		|| (cap->sensor_id == SONYIMX226)));
	if (vin_cap_flag) {
		/* Fixme:
		 * This is temp solution for IMX172 in mode 6 to save DRAM bandwidth.
		 */
		cap = get_vin_capability();
		vin = &cap->vin_cap_info;
		/* When VIN capture offset is enabled, VIN crop window is same
		 * size as dummy window of DZ type I to save DRAM bandwidth.
		 */
		vin->s_cap_start_h_reg = cap->cap_start_x + input->x;
		vin->s_cap_start_v_reg = cap->cap_start_y + input->y;
		vin->s_cap_end_h_reg = vin->s_cap_start_h_reg + input->width - 1;
		vin->s_cap_end_v_reg = vin->s_cap_start_v_reg + input->height - 1;
		input->x = 0;
		input->y = 0;
		/* Update sensor crop window to VIN adapter */
		window.start_x = vin->s_cap_start_h_reg;
		window.start_y = vin->s_cap_start_v_reg;
		window.end_x = vin->s_cap_end_h_reg;
		window.end_y = vin->s_cap_end_v_reg;
		amba_vin_adapter_cmd(context->g_info->pvininfo->active_src_id,
			AMBA_VIN_ADAP_SET_CAPTURE_WINDOW, &window);
	}
	return vin_cap_flag;
}

static inline void set_dsp_id_map(u32 *id_map, IAV_DSP_PARTITION_TYPE partition)
{
	*id_map |= (1 << (partition_id_to_index(partition)));
}

static inline void clear_dsp_id_map(u32 *id_map, IAV_DSP_PARTITION_TYPE partition)
{
	*id_map &= ~(1 << partition_id_to_index(partition));
}

/*
 * This ISR handler will update the memory management of DSP.
 */
static void handle_enc_memm_msg(void *context,
	unsigned int cat, DSP_MSG *msg, int port)
{
	u16 req_index;
	iav_source_buffer_dram_pool_ex_t * buf_pool = NULL;
	iav_source_buffer_dram_ex_t * dram_buf = NULL;
	MEMM_CREATE_FRM_BUF_POOL_MSG * create_msg = NULL;
	MEMM_REQ_FRM_BUF_MSG * req_msg = NULL;
	MEMM_GET_FRM_BUF_ADDR_INFO_MSG * buf_addr_msg = NULL;
	int partition = 0;

	dram_buf = &G_source_buffer[IAV_ENCODE_SOURCE_MAIN_DRAM].dram;
	buf_pool = &dram_buf->buf_pool;
	switch (msg->msg_code) {
	case MSG_MEMM_CREATE_FRM_BUF_POOL:
		create_msg = (MEMM_CREATE_FRM_BUF_POOL_MSG *)msg;
		if (create_msg->error_code) {
			iav_error("EFM: Failed to allocate buffer pool!\n");
			return ;
		}
		if (create_msg->num_of_frm_bufs != dram_buf->max_frame_num) {
			iav_error("EFM: Failed to allocate [%d] frames for buffer pool,"
				" not equals to max [%d].\n", create_msg->num_of_frm_bufs,
				dram_buf->max_frame_num);
			return ;
		}
		if (!(dram_buf->buf_alloc & (1 << IAV_YUV_400_FORMAT))) {
			buf_pool->id[IAV_YUV_400_FORMAT] = create_msg->frm_buf_pool_id;
			dram_buf->buf_alloc |= (1 << IAV_YUV_400_FORMAT);
		} else if (!(dram_buf->buf_alloc & (1 << IAV_YUV_420_FORMAT))) {
			buf_pool->id[IAV_YUV_420_FORMAT] = create_msg->frm_buf_pool_id;
			dram_buf->buf_alloc |= (1 << IAV_YUV_420_FORMAT);
		} else {
			iav_error("EFM: Too many MSG for creating buffer pool!\n");
			return ;
		}
		if ((dram_buf->buf_alloc & (1 << IAV_YUV_400_FORMAT)) &&
			(dram_buf->buf_alloc & (1 << IAV_YUV_420_FORMAT))) {
			if (is_map_dsp_partition()) {
				dram_buf->buf_state = DSP_DRAM_BUFFER_POOL_CREATE_READY;
			} else {
				dram_buf->buf_state = DSP_DRAM_BUFFER_POOL_READY;
			}
			wake_up_interruptible(&dram_buf->wq);
		}
		iav_printk("Create FRM buf pool id : %d.\n", create_msg->frm_buf_pool_id);
		break;
	case MSG_MEMM_REQ_FRM_BUF:
		req_msg = (MEMM_REQ_FRM_BUF_MSG *)msg;
/*		iav_printk("MSG: [0x%x-0x%x], Err [%d], Frm [%d], P [%d], PID [%d], A [%d].\n",
			req_msg->luma_buf_base_addr, req_msg->chroma_buf_base_addr,
			req_msg->error_code, req_msg->frm_buf_id, req_msg->buf_pitch,
			req_msg->frm_buf_pool_id, req_msg->avail_frm_num);
*/		if (req_msg->error_code) {
			iav_printk("EFM: Failed to request frame [%d] from buffer pool."
				" Retry it!\n", req_msg->frm_buf_id);
			return ;
		}
		if (req_msg->buf_pitch != dram_buf->buf_pitch) {
			iav_error("EFM: Allocated buffer pool pitch [%d] is not equal to "
				"the config [%d]!\n", req_msg->buf_pitch, dram_buf->buf_pitch);
			return ;
		}
		req_index = buf_pool->req_index;
		if (req_msg->frm_buf_pool_id == buf_pool->id[IAV_YUV_420_FORMAT]) {
			if (!is_invalid_dsp_addr(req_msg->chroma_buf_base_addr) &&
				!is_invalid_dsp_addr(req_msg->luma_buf_base_addr)) {
				buf_pool->y[req_index] = req_msg->luma_buf_base_addr;
				buf_pool->uv[req_index] = req_msg->chroma_buf_base_addr;
				buf_pool->yuv_frame_id[req_index] = req_msg->frm_buf_id;
				buf_pool->frame_req_done |= (1 << IAV_YUV_420_FORMAT);
			}
		} else if (req_msg->frm_buf_pool_id == buf_pool->id[IAV_YUV_400_FORMAT]) {
			if (is_invalid_dsp_addr(req_msg->chroma_buf_base_addr) &&
				!is_invalid_dsp_addr(req_msg->luma_buf_base_addr)) {
				buf_pool->me1[req_index] = req_msg->luma_buf_base_addr;
				buf_pool->me1_frame_id[req_index] = req_msg->frm_buf_id;
				buf_pool->frame_req_done |= (1 << IAV_YUV_400_FORMAT);
			}
		} else {
			iav_error("EFM: Invalid buffer pool [%d] for requested frame!\n",
				req_msg->frm_buf_pool_id);
			return ;
		}
		if (buf_pool->y[req_index] && buf_pool->uv[req_index] && buf_pool->me1[req_index]) {
			dram_buf->buf_state = DSP_DRAM_BUFFER_POOL_READY;
			wake_up_interruptible(&dram_buf->wq);
			/* Print for debug
			iav_printk("Req FRM frame [%d]: YUV [%d: 0x%x - 0x%x], ME1 [%d: 0x%x].\n",
				req_index, buf_pool->yuv_frame_id[req_index], buf_pool->y[req_index],
				buf_pool->uv[req_index], buf_pool->me1_frame_id[req_index],
				buf_pool->me1[req_index]);
			//*/
		}
		break;
	case MSG_MEMM_GET_FRM_BUF_POOL_INFO:
		buf_addr_msg = (MEMM_GET_FRM_BUF_ADDR_INFO_MSG *)msg;
		switch (buf_addr_msg->callback_id >> 8) {
			case FRM_BUF_POOL_TYPE_EFM:
				if ((buf_addr_msg->callback_id & 0xFF) == buf_pool->id[IAV_YUV_400_FORMAT]) {
					iav_set_dsp_partition(partition_id_to_index(IAV_DSP_PARTITION_EFM_ME1),
						DSP_TO_PHYS(buf_addr_msg->start_base_daddr), buf_addr_msg->size);
					dram_buf->buf_alloc |= (1 << (IAV_YUV_400_FORMAT + 2));
					set_dsp_id_map(&G_dsp_partition.id_map, IAV_DSP_PARTITION_EFM_ME1);
				} else if ((buf_addr_msg->callback_id & 0xFF) == buf_pool->id[IAV_YUV_420_FORMAT]){
					iav_set_dsp_partition(partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV),
						DSP_TO_PHYS(buf_addr_msg->start_base_daddr), buf_addr_msg->size);
					dram_buf->buf_alloc |= (1 << (IAV_YUV_420_FORMAT + 2));
					set_dsp_id_map(&G_dsp_partition.id_map, IAV_DSP_PARTITION_EFM_YUV);
				}
				if ((dram_buf->buf_alloc & (1 << (IAV_YUV_400_FORMAT + 2))) &&
					(dram_buf->buf_alloc & (1 << (IAV_YUV_420_FORMAT + 2)))) {
					dram_buf->buf_state = DSP_DRAM_BUFFER_POOL_READY;
					wake_up_interruptible(&dram_buf->wq);
				}
				break;

			default:
				if ((buf_addr_msg->callback_id >> 8) == FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE) {
					partition = partition_id_to_index(IAV_DSP_PARTITION_POST_MAIN);
				} else {
					partition = buf_addr_msg->callback_id >> 8;
				}
				iav_set_dsp_partition(partition, DSP_TO_PHYS(buf_addr_msg->start_base_daddr),
					buf_addr_msg->size);
				G_dsp_partition.id_map |= (1 << partition);
				if (G_dsp_partition.id_map_user == G_dsp_partition.id_map) {
					wake_up_interruptible(&G_dsp_partition.wq);
				}
				break;
		}
		break;
	default:
		iav_printk("Unknown MSG, cat = %d, code =%d.\n", cat, msg->msg_code);
		break;
	}

	return ;
}

static inline void prepare_enc_dram_pool(u16 buf_id)
{
	iav_source_buffer_dram_ex_t * dram = &G_source_buffer[buf_id].dram;

	memset(&dram->buf_pool, 0, sizeof(dram->buf_pool));
	dram->buf_alloc = 0;
	if (dram->max_frame_num && is_buf_type_enc(buf_id)) {
		cmd_enc_dram_create_buf_pool(IAV_YUV_400_FORMAT, dram);
		cmd_enc_dram_create_buf_pool(IAV_YUV_420_FORMAT, dram);
		dram->buf_state = DSP_DRAM_BUFFER_POOL_INIT;
	} else {
		dram->buf_state = DSP_DRAM_BUFFER_POOL_CREATE_READY;
	}
	dram->valid_frame_num = 0;

	init_waitqueue_head(&dram->wq);
}

static int enter_vcap_timer_mode(iav_context_t *context)
{
	iav_printk("DSP is entering timer mode.\n");

	if (likely(!G_iav_obj.vsync_signal_lost)) {
		dsp_start_cmdblk(DSP_CMD_PORT_VCAP);
		cmd_vcap_timer_mode(context, TIMER_MODE_INTERVAL_4MS);
		dsp_end_cmdblk(DSP_CMD_PORT_VCAP);
		wait_vcap(VCAP_TIMER_MODE, "wait for TIMER_MODE");
		dsp_enable_vcap_cmd_port(0);
	} else {
		// use GEN port for vsync signal lost case
		dsp_enable_vcap_cmd_port(0);
		dsp_start_cmdblk(DSP_CMD_PORT_GEN);
		cmd_vcap_timer_mode(context, TIMER_MODE_INTERVAL_4MS);
		dsp_end_cmdblk(DSP_CMD_PORT_GEN);
		wait_vcap(VCAP_TIMER_MODE, "wait for TIMER_MODE");
	}

	iav_printk("dsp is already in timer mode \n");

	G_iav_obj.dsp_vcap_mode = -1; // vcap dead ? fixme

	return 0;
}

static void enter_encode_mode(void * context)
{
	cmd_set_operation_mode(DSP_OP_MODE_IP_CAMERA_RECORD);
}

static void enter_vcap_preview_mode(iav_context_t * context)
{
	int i;
	u32 vout0_src, vout1_src;
	iav_rect_ex_t dz_input;

	dz_input = G_cap_pre_main.input;
	if (is_multi_region_warp_mode()) {
		/* Run VIN crop for dewarp mode to make PM buffer be in VIN
		 * domain, before CFA scaler. */
		sync_vin_crop(context, &dz_input, 1);
	}
	iav_config_vin();

	/* Config VOUT */
	if (unlikely(G_system_setup_info[get_enc_mode()].vout_swap)) {
		vout0_src = is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER) ?
			VOUT_SRC_VCAP : VOUT_SRC_BACKGROUND;
		vout1_src = is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER) ?
			VOUT_SRC_VCAP : VOUT_SRC_BACKGROUND;
	} else {
		vout0_src = is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER) ?
			VOUT_SRC_VCAP : VOUT_SRC_BACKGROUND;
		vout1_src = is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER) ?
			VOUT_SRC_VCAP : VOUT_SRC_BACKGROUND;
	}
	iav_config_vout(context, -1, vout0_src | (vout1_src << 16));

	cmd_sensor_config();
	cmd_vcap_setup();

	/* warp control cmd must be called after VCAP setup cmd */
	cmd_warp_control(&dz_input);
	if (is_enc_from_raw_enabled()) {
		cmd_raw_encode_setup();
	}
	for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; ++i) {
		if (is_buf_type_enc(i)) {
			cmd_capture_buffer_setup(i);
		}
	}

	/* Set up encode interrupt handler */
	if (likely(is_interrupt_readout())) {
		dsp_set_enc_handler(enc_irq_handler, context);
	} else {
		dsp_set_enc_handler(NULL, context);
	}
}

static void enter_preview_state(iav_context_t *context)
{
	switch (G_iav_obj.op_mode) {
	case DSP_OP_MODE_IDLE:
		// boot to preview state
		iav_printk("### Enter preview from IDLE mode.\n");

		dsp_reset();
		// wait for DSP enter encode mode
		dsp_set_mode(DSP_OP_MODE_IP_CAMERA_RECORD, enter_encode_mode, context);
		dsp_start_cmdblk(DSP_CMD_PORT_GEN);
		enter_vcap_preview_mode(context);
		dsp_end_cmdblk(DSP_CMD_PORT_GEN);

		wait_vcap(VCAP_VIDEO_MODE, "wait for video capture mode");
		break;

	case DSP_OP_MODE_IP_CAMERA_RECORD:
		iav_printk("### Enter preview from IPCAM RECORD mode.\n");

		// wake up from timer mode
		dsp_reset_idsp();
		//if DSP is idle, set DSP to camera mode first
		dsp_set_mode(DSP_OP_MODE_IP_CAMERA_RECORD, enter_encode_mode, context);
		dsp_start_cmdblk(DSP_CMD_PORT_GEN);
		// wait for DSP enter encode mode
		enter_vcap_preview_mode(context);
		dsp_end_cmdblk(DSP_CMD_PORT_GEN);

		wait_vcap(VCAP_VIDEO_MODE, "wait for video capture mode");
		break;

	case DSP_OP_MODE_CAMERA_PLAYBACK:
		// switch decoding idle to preview
		iav_printk("### Enter previe from PLAYBACK mode.\n");
		iav_change_vout_src(context, VOUT_SRC_BACKGROUND);

		//if DSP is idle, set DSP to camera mode first
		dsp_set_mode(DSP_OP_MODE_IP_CAMERA_RECORD, enter_encode_mode, context);
		dsp_start_default_cmd();
		enter_vcap_preview_mode(context);
		dsp_end_default_cmd();

		dsp_reset();
		iav_change_vout_src(context, VOUT_SRC_VCAP);
		break;

	default:
		iav_error("!!! Enter preview from unexpected OPMODE [%d].\n",
			G_iav_obj.op_mode);
		BUG();
		break;
	}

	G_iav_obj.op_mode = DSP_OP_MODE_IP_CAMERA_RECORD;
}

static int set_default_param_after_preview(void)
{
	int i, buffer;

	if (is_warp_mode()) {
		/* Wait 2 frames to sync up the warp control and capture
		 * buffer setup commands.
		 */
		wait_vcap_msg_count(2);
	}

	G_dsp_partition.id_map = 0;
	G_dsp_partition.id_map_user = 0;
	iav_reset_dsp_partition();

	/* Allocate buffer pool for encode from DRAM feature */
	if (unlikely(is_enc_from_dram_enabled())) {
		/* Request DSP to create buffer pool */
		for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
			i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
			prepare_enc_dram_pool(i);
		}
		dsp_set_cat_msg_handler(handle_enc_memm_msg, CAT_MEMM, NULL);
		/* Wait for DSP return */
		for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
			i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
			if (is_map_dsp_partition()) {
				wait_enc_dram(i, DSP_DRAM_BUFFER_POOL_CREATE_READY);
			} else {
				wait_enc_dram(i, DSP_DRAM_BUFFER_POOL_READY);
			}
		}
		if (is_map_dsp_partition()) {
			for (i = IAV_ENCODE_SOURCE_DRAM_FIRST;
				i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i) {
				if (G_source_buffer[i].dram.buf_pool.id[0] && G_source_buffer[i].dram.buf_pool.id[1]) {
					cmd_get_frm_buf_pool_info(FRM_BUF_POOL_TYPE_EFM, G_source_buffer[i].dram.buf_pool.id[0]);
					cmd_get_frm_buf_pool_info(FRM_BUF_POOL_TYPE_EFM, G_source_buffer[i].dram.buf_pool.id[1]);
					wait_enc_dram(i, DSP_DRAM_BUFFER_POOL_READY);
				}
			}
		}
	} else {
		dsp_set_cat_msg_handler(NULL, CAT_MEMM, NULL);
	}

	/* Config default warp parameter for sub source buffers */
	if (is_warp_mode()) {
		buffer = 0;
		if (!is_buf_unwarped(IAV_ENCODE_SOURCE_SECOND_BUFFER) &&
				!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
			buffer |= IAV_2ND_BUFFER;
		}
		if (!is_buf_unwarped(IAV_ENCODE_SOURCE_FOURTH_BUFFER) &&
				!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
			buffer |= IAV_4TH_BUFFER;
		}
		if (buffer) {
			set_default_warp_dptz_param(buffer, IAV_BUF_WARP_SET);
		}
	}

	return 0;
}

/******************************************
 *
 *	IAV IOCTLs functions
 *
 ******************************************/

void iav_config_vin(void)
{
	vin_cap_win_t vin_cap_win;

	cmd_set_vin_config(&vin_cap_win);
	cmd_set_vin_cap_win_ext(&vin_cap_win);
}

int iav_get_preview_format(iav_context_t *context,
	iav_preview_format_t __user *arg)
{
	iav_preview_format_t format;
	calc_preview_param(context, &format);
	return copy_to_user(arg, &format, sizeof(format)) ? -EFAULT : 0;
}

static int unmap_dsp_partition(iav_context_t *context)
{
	int rval = 0;
	struct mm_struct *mm;
	int i = 0;

	mm = current->mm;
	if (mm != NULL) {
		for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
			if (context->dsp_partition[i].user_start) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
				down_write(&mm->mmap_sem);
				rval = do_munmap(mm, (unsigned long)context->dsp_partition[i].user_start,
					(size_t)(context->dsp_partition[i].user_end - context->dsp_partition[i].user_start));
				up_write(&mm->mmap_sem);
#else
				rval = vm_munmap((unsigned long)context->dsp_partition[i].user_start,
					(size_t)(context->dsp_partition[i].user_end - context->dsp_partition[i].user_start));
#endif

				if (rval == 0) {
					context->dsp_partition[i].user_start = NULL;
					context->dsp_partition[i].user_end = NULL;
				}
			}
		}
	}
	return rval;
}

int iav_enable_preview(iav_context_t *context)
{
	int rval = 0;

	if (is_iav_state_preview())
		return 0;

#ifdef CONFIG_IAV_CONTROL_AAA
	if (is_hiso_video_mode() && G_nl_image_config.nl_connected) {
		nl_image_prepare_aaa();
	}
#endif
	if ((rval = iav_check_before_enter_preview(context)) < 0)
		return rval;

	/* Clear flag before issuing any commands. */
	G_iav_obj.vcap_preview_done = 0;
	G_iav_obj.vout_b_update_done = 0;
	iav_bufcap_reset();

	enter_preview_state(context);
	dsp_enable_vcap_cmd_port(1);

	if (!is_hiso_video_mode()) {
		/* Wait for entering preview done */
		wait_event_interruptible(G_iav_obj.vcap_wq,
			(G_iav_obj.vcap_preview_done == 1));
	} else {
		G_iav_obj.vcap_preview_done = 1;
	}

	if (unlikely(G_iav_obj.vsync_error_again)) {
		iav_error("Vsync loss found in entering preview. ABORT rest cmds.\n");
		rval = -EAGAIN;
	} else {
		//setup GMV
		cmd_set_vcap_mctf_gmv();
		iav_printk("Preview enabled.\n");
		set_default_param_after_preview();
	}

	if (is_vout_b_letter_boxing_enabled()) {
		wait_event_interruptible(G_iav_obj.vout_b_update_wq,
			(G_iav_obj.vout_b_update_done == 1));
		prepare_vout_b_letter_box();
	}
	// Clear internal buffers
	reset_privacy_mask();
	set_iav_state(IAV_STATE_PREVIEW);

#ifdef CONFIG_IAV_CONTROL_AAA
	if (likely(!G_iav_obj.vsync_error_again)) {
		if (G_nl_image_config.nl_connected) {
			nl_image_start_aaa();
		}
	}
#endif

	//unmap all DSP partitions. Partition phys addr might change while re-entering Preview. So unmap it
	rval = unmap_dsp_partition(context);

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
	img_need_remap(1);
#endif

	return rval;
}

int iav_disable_preview(iav_context_t *context)
{
	int rval = 0;
	iav_printk("IAV is disabling preview from state [%d], entering IDLE...\n",
		get_iav_state());

	if (is_iav_state_idle()) {
		iav_printk("already in idle, do nothing \n");
		return 0;
	}

	if (!is_iav_state_preview()) {
		iav_error("not in preview mode, cannot enter timer mode.\n");
		return -EPERM;
	}

#ifdef CONFIG_IAV_CONTROL_AAA
	if (G_nl_image_config.nl_connected) {
		nl_image_stop_aaa();
	}
#endif

	//set IAV state to going to IDLE, and block all cmds that must run in PREVIEW/ENCODE
	set_iav_state(IAV_STATE_EXITING_PREVIEW);

	//iav_change_vout_src(context, VOUT_SRC_BACKGROUND);
	if ((rval = enter_vcap_timer_mode(context)) < 0) {
		iav_error("enter timer mode failed \n");
		return rval;
	}

	// Wait DSP enter IDLE mode
	dsp_set_mode(DSP_OP_MODE_IDLE, NULL, NULL);
	set_iav_state(IAV_STATE_IDLE);

	return 0;
}

int iav_get_state_info(iav_context_t *context,
	iav_state_info_t __user *arg)
{
	iav_state_info_t info;

	info.vout_irq_count = 0;
	info.vout1_irq_count = 0;
	info.vin_irq_count = 0;
	info.vdsp_irq_count = 0;
	info.dsp_op_mode = 0;
	info.dsp_encode_state = 0;
	info.dsp_encode_mode = 0;
	info.dsp_decode_state = 0;
	info.decode_state = 0;
	info.encode_timecode = 0;
	info.encode_pts = 0;

	info.state = get_iav_state();

	info.encoder_state = 0;
	info.decoder_state = 0;
	info.channel_state = 0;

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int iav_set_vin_capture_win(iav_context_t * context,
	struct iav_rect_ex_s __user * arg)
{
	iav_rect_ex_t cap_win;
	struct amba_vin_cap_win_info *vin_cap;
	u32 old_w, old_h;
	vin_cap_win_t vin_cap_win;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot change VIN Cap Win in non preview / encoding state!\n");
		return -EPERM;
	}
	if (!is_vin_cap_offset_enabled()) {
		iav_error("Cannot change VIN Cap Win in mode [%s].\n",
			G_modes_limit[G_dsp_enc_mode].name);
		return -EINVAL;
	}

	if (copy_from_user(&cap_win, arg, sizeof(cap_win)))
		return -EFAULT;

	vin_cap = &get_vin_capability()->vin_cap_info;
	old_w = vin_cap->s_cap_end_h_reg - vin_cap->s_cap_start_h_reg + 1;
	old_h = vin_cap->s_cap_end_v_reg - vin_cap->s_cap_start_v_reg + 1;

	if ((cap_win.width != old_w) || (cap_win.height != old_h)) {
		iav_error("Cannot change VIN cap win size %dx%d to %dx%d.\n",
			old_w, old_h, cap_win.width, cap_win.height);
		return -EINVAL;
	}

	vin_cap->s_cap_start_h_reg = cap_win.x;
	vin_cap->s_cap_start_v_reg = cap_win.y;
	vin_cap->s_cap_end_h_reg = cap_win.width + vin_cap->s_cap_start_h_reg - 1;
	vin_cap->s_cap_end_v_reg = cap_win.height + vin_cap->s_cap_start_v_reg - 1;

	cmd_set_vin_config(&vin_cap_win);

	return 0;
}

int iav_get_vin_capture_win(iav_context_t * context,
	struct iav_rect_ex_s __user * arg)
{
	iav_rect_ex_t cap_win;
	struct amba_vin_cap_win_info *vin_cap;

	if (is_iav_state_idle()) {
		iav_warning("VIN Cap Win may be incorrect when IAV in IDLE state!\n");
	}

	vin_cap = &get_vin_capability()->vin_cap_info;
	cap_win.x = vin_cap->s_cap_start_h_reg;
	cap_win.y = vin_cap->s_cap_start_v_reg;
	cap_win.width = vin_cap->s_cap_end_h_reg - vin_cap->s_cap_start_h_reg + 1;
	cap_win.height = vin_cap->s_cap_end_v_reg - vin_cap->s_cap_start_v_reg + 1;

	return copy_to_user(arg, &cap_win, sizeof(cap_win)) ? -EFAULT : 0;
}

int iav_set_system_setup_info(iav_context_t * context,
	iav_system_setup_info_ex_t * arg)
{
	iav_system_setup_info_ex_t info;

	if (!is_iav_state_idle()) {
		iav_error("Set system setup info must be called in IAV IDLE state!\n");
		return -EPERM;
	}

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (iav_check_system_setup_info(&info) < 0)
		return -EINVAL;

	G_iav_obj.dsp_noncached = info.dsp_noncached;
	G_system_setup_info[G_dsp_enc_mode] = info;
	update_cmd_read_delay(info.cmd_read_delay);

	return 0;
}

int iav_get_system_setup_info(iav_context_t * context,
	iav_system_setup_info_ex_t * arg)
{
	G_system_setup_info[G_dsp_enc_mode].dsp_noncached =
		G_iav_obj.dsp_noncached;
	return copy_to_user(arg, &G_system_setup_info[G_dsp_enc_mode],
		sizeof(iav_system_setup_info_ex_t)) ? -EFAULT : 0;
}

int iav_set_system_resource_limit_ex(iav_context_t *context,
	struct iav_system_resource_setup_ex_s __user *arg)
{
	iav_system_resource_setup_ex_t param;

	if (!is_iav_state_idle()) {
		iav_error("Set system resouce must be called in IAV IDLE state!\n");
		return -EPERM;
	}

	if (copy_from_user(&param, arg, sizeof(iav_system_resource_setup_ex_t)))
		return -EFAULT;

	if (iav_check_system_resource_limit(&param) < 0) {
		return -EINVAL;
	}
	update_system_resource_limit(context, &param);

	return 0;
}

int iav_get_system_resource_limit_ex(iav_context_t *context,
	struct iav_system_resource_setup_ex_s __user *arg)
{
	u32 total_size;
	int i, j, encode_mode;
	DSP_ENC_CFG * enc_cfg = NULL;
	iav_system_resource_setup_ex_t param;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;
	struct amba_video_info * video_info_a = NULL, * video_info_b = NULL;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if ((param.encode_mode != IAV_ENCODE_CURRENT_MODE) &&
			((param.encode_mode < IAV_ENCODE_MODE_FIRST) ||
			 (param.encode_mode >= IAV_ENCODE_MODE_LAST))) {
		iav_error("Invalid encode mode [%d].\n", param.encode_mode);
		return -EFAULT;
	}
	encode_mode = (param.encode_mode == IAV_ENCODE_CURRENT_MODE) ?
		G_dsp_enc_mode : param.encode_mode;

	resource = &G_system_resource_setup[encode_mode];
	param.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width
		= resource->max_main_width;
	param.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height =
		resource->max_main_height;
	param.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width =
		resource->max_preview_C_width;
	param.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height =
		resource->max_preview_C_height;
	param.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width =
		resource->max_preview_B_width;
	param.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height =
		resource->max_preview_B_height;
	param.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width =
		resource->max_preview_A_width;
	param.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height =
		resource->max_preview_A_height;
	param.max_warp_input_width = resource->max_warped_region_input_width;
	param.max_warp_output_width = resource->max_warped_region_output_width;
	param.extra_2x_zoom_enable =
		!G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER].disable_extra_2x;

	for (i = IAV_ENCODE_SOURCE_DRAM_FIRST, j = 0;
		i < IAV_ENCODE_SOURCE_DRAM_LAST; ++i, ++j) {
		param.buffer_max_size[i] = G_source_buffer[i].dram.buf_max_size;
		param.max_dram_frame[j] = G_source_buffer[i].dram.max_frame_num;
	}

	param.max_num_encode_streams = get_max_enc_num(encode_mode);
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		enc_cfg = get_enc_cfg(encode_mode, i);
		param.stream_max_size[i].width = enc_cfg->max_enc_width;
		param.stream_max_size[i].height = enc_cfg->max_enc_height;
		param.stream_2x_search_range[i] = enc_cfg->vert_search_range_2x;
		param.stream_max_GOP_M[i] = enc_cfg->max_GOP_M;
		param.stream_max_GOP_N[i] = 255;
		param.stream_max_advanced_quality_model[i] = 6;
	}
	param.max_num_cap_sources = 2;
	if (resource->preview_A_for_enc)
		param.max_num_cap_sources++;
	if (resource->preview_B_for_enc)
		param.max_num_cap_sources++;

	if (G_system_setup_info[G_dsp_enc_mode].vout_swap) {
		video_info_a = &G_iav_info.pvoutinfo[1]->video_info;
		video_info_b = &G_iav_info.pvoutinfo[0]->video_info;
	} else {
		video_info_a = &G_iav_info.pvoutinfo[0]->video_info;
		video_info_b = &G_iav_info.pvoutinfo[1]->video_info;
	}
	param.voutA.width = video_info_a->width;
	param.voutA.height = video_info_a->height;
	param.voutB.width = video_info_b->width;
	param.voutB.height = video_info_b->height;

	get_total_dram_size(&total_size);
	param.total_memory_size = total_size;
	param.sharpen_b_enable = G_iav_vcap.sharpen_b_enable[encode_mode];
	param.enc_from_raw_enable = G_iav_vcap.enc_from_raw_enable[encode_mode];
	param.raw_capture_enable = resource->raw_compression_disabled;
	param.max_vin_stats_lines_top = resource->max_vin_stats_num_lines_top;
	param.max_vin_stats_lines_bottom = resource->max_vin_stats_num_lines_bot;
	param.max_chroma_noise_shift = resource->max_chroma_filter_radius;
	param.extra_dram_buf[0] = Get_EXTRA_BUF_NUM(resource->extra_dram_buf,
		resource->extra_buf_msb_ext);
	param.extra_dram_buf[1] = Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_c,
		resource->extra_buf_msb_ext_prev_c);
	param.extra_dram_buf[2] = Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_b,
		resource->extra_buf_msb_ext_prev_b);
	param.extra_dram_buf[3] = Get_EXTRA_BUF_NUM(resource->extra_dram_buf_prev_a,
		resource->extra_buf_msb_ext_prev_a);
	param.hwarp_bypass_possible = resource->h_warp_bypass;
	param.mixer_b_enable = G_iav_vcap.mixer_b_enable;
	param.vca_buf_src_id = G_iav_vcap.vca_src_id;
	param.vout_b_letter_box_enable = G_iav_vcap.vout_b_letter_boxing_enable;
	param.yuv_input_enhanced = G_iav_vcap.yuv_input_enhanced;

	switch (encode_mode) {
	case DSP_FULL_FRAMERATE_MODE:
		encode_mode = IAV_ENCODE_FULL_FRAMERATE_MODE;
		break;
	case DSP_MULTI_REGION_WARP_MODE:
		encode_mode = IAV_ENCODE_WARP_MODE;
		break;
	case DSP_HIGH_MEGA_PIXEL_MODE:
		encode_mode = IAV_ENCODE_HIGH_MEGA_MODE;
		break;
	case DSP_CALIBRATION_MODE:
		encode_mode = IAV_ENCODE_CALIBRATION_MODE;
		break;
	case DSP_HDR_FRAME_INTERLEAVED_MODE:
		encode_mode = IAV_ENCODE_HDR_FRAME_MODE;
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		encode_mode = IAV_ENCODE_HDR_LINE_MODE;
		break;
	case DSP_HIGH_MP_FULL_PERF_MODE:
		encode_mode = IAV_ENCODE_HIGH_MP_FULL_PERF_MODE;
		break;
	case DSP_FULL_FPS_FULL_PERF_MODE:
		encode_mode = IAV_ENCODE_FULL_FPS_FULL_PERF_MODE;
		break;
	case DSP_MULTI_VIN_MODE:
		encode_mode = IAV_ENCODE_MULTI_VIN_MODE;
		break;

	case DSP_HISO_VIDEO_MODE:
		encode_mode = IAV_ENCODE_HISO_VIDEO_MODE;
		break;
	case DSP_HIGH_MP_WARP_MODE:
		encode_mode = IAV_ENCODE_HIGH_MP_WARP_MODE;
		break;
	default:
		encode_mode = IAV_ENCODE_FULL_FRAMERATE_MODE;
		break;
	}
	param.debug_chip_id = G_iav_vcap.debug_chip_id;
	param.encode_mode = encode_mode;
	param.rotate_possible = resource->enc_rotation;
	param.exposure_num = get_expo_num(encode_mode);
	param.vin_num = get_vin_num(encode_mode);
	param.vskip_before_encode = G_iav_vcap.vskip_before_encode[encode_mode];
	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_set_sharpen_filter_config_ex(iav_context_t *context,
	struct iav_sharpen_filter_cfg_s *arg)
{
	iav_error("This IOCTL is obsolete since SDK 2.5. Please use "
		"IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX to config "
		"sharpen_b.\n");

	return 0;
}

int iav_get_sharpen_filter_config_ex(iav_context_t *context,
	struct iav_sharpen_filter_cfg_s __user *arg)
{
	struct iav_sharpen_filter_cfg_s param;

	memset(&param, 0, sizeof(struct iav_sharpen_filter_cfg_s));

	param.sharpen_b_enable = G_iav_vcap.sharpen_b_enable[G_dsp_enc_mode];

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_get_source_buffer_info_ex(iav_context_t * context,
	struct iav_source_buffer_info_ex_s __user * arg)
{
	int i;
	iav_source_buffer_info_ex_t info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (get_single_buffer_num(info.id, &i) < 0) {
		return -EINVAL;
	}
	info.state = G_source_buffer[i].state;

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int iav_set_source_buffer_type_all_ex(iav_context_t * context,
	iav_source_buffer_type_all_ex_t __user * arg)
{
	iav_source_buffer_type_all_ex_t source_buffer_type_all;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;

	if (!is_iav_state_idle()) {
		iav_error("set_source_buffer_type_all must be called in IAV IDLE state!\n");
		return -EPERM;
	}

	if (copy_from_user(&source_buffer_type_all, arg, sizeof(iav_source_buffer_type_all_ex_t)))
		return -EFAULT;

	if ((source_buffer_type_all.main_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) ||
			(source_buffer_type_all.main_buffer_type == IAV_SOURCE_BUFFER_TYPE_OFF) ||
			(source_buffer_type_all.second_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW)) {
		iav_error("Invalid buffer type for main buffer [%d] and second buffer [%d].\n",
			source_buffer_type_all.main_buffer_type, source_buffer_type_all.second_buffer_type);
		return -EINVAL;
	}

	// update database
	G_source_buffer[0].type = source_buffer_type_all.main_buffer_type;
	G_source_buffer[1].type = source_buffer_type_all.second_buffer_type;
	G_source_buffer[2].type = source_buffer_type_all.third_buffer_type;
	G_source_buffer[3].type = source_buffer_type_all.fourth_buffer_type;

	resource = &G_system_resource_setup[G_dsp_enc_mode];
	resource->preview_B_for_enc =
		is_buf_type_enc(IAV_ENCODE_SOURCE_THIRD_BUFFER);
	resource->preview_A_for_enc =
		is_buf_type_enc(IAV_ENCODE_SOURCE_FOURTH_BUFFER);

	return 0;
}

int iav_get_source_buffer_type_all_ex(iav_context_t * context,
	iav_source_buffer_type_all_ex_t __user * arg)
{
	iav_source_buffer_type_all_ex_t buffer_type_all;

	buffer_type_all.main_buffer_type = G_source_buffer[0].type;
	buffer_type_all.second_buffer_type = G_source_buffer[1].type;
	buffer_type_all.third_buffer_type = G_source_buffer[2].type;
	buffer_type_all.fourth_buffer_type = G_source_buffer[3].type;

	return copy_to_user(arg, &buffer_type_all, sizeof(buffer_type_all)) ? -EFAULT : 0;
}

int iav_set_source_buffer_format_ex(iav_context_t * context,
	iav_source_buffer_format_ex_t __user * arg)
{
	iav_source_buffer_format_ex_t format;
	u32 is_buf_warp = 0;
	int buffer_id;
	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set sub source buffer format while IAV is not in "
				"PREVIEW or ENCODE.\n");
		return -EPERM;
	}

	if (iav_check_source_buffer_format(&format) < 0) {
		return -EINVAL;
	}
	buffer_id = format.source;
	update_source_buffer_format(buffer_id, &format.size, &format.input);
	if (is_buf_type_enc(buffer_id)) {
		is_buf_warp = is_warp_mode() && !is_buf_unwarped(buffer_id);
		cmd_capture_buffer_setup(buffer_id);
		if (is_buf_warp) {
			set_default_warp_dptz_param((1 << buffer_id), IAV_BUF_WARP_SET);
		}
	}

	return 0;
}

int iav_get_source_buffer_format_ex(iav_context_t * context,
	iav_source_buffer_format_ex_t __user * arg)
{
	iav_source_buffer_format_ex_t format;
	int buffer_id;

	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	buffer_id = format.source;
	if (!is_valid_buffer_id(buffer_id)) {
		iav_error("Invalid source buffer id %d.", buffer_id);
		return -EINVAL;
	}

	format.input = G_source_buffer[buffer_id].input;

	if (G_source_buffer[buffer_id].enc_stop) {
		format.size.width = 0;
		format.size.height = 0;
	} else {
		format.size = G_source_buffer[buffer_id].size;
	}

	return copy_to_user(arg, &format, sizeof(format)) ? -EFAULT : 0;
}

int iav_set_source_buffer_setup_ex(iav_context_t * context,
	struct iav_source_buffer_setup_ex_s __user * arg)
{
	iav_source_buffer_setup_ex_t setup;
	if (copy_from_user(&setup, arg, sizeof(setup)))
		return -EFAULT;

	if (!is_iav_state_idle()) {
		iav_error("Cannot set source buffer setup while IAV is not "
				"in IDLE.");
		return -EPERM;
	}

	if (iav_check_source_buffer_setup(&setup) < 0) {
		return -EINVAL;
	}
	update_source_buffer_setup(&setup);

	return 0;
}

int iav_get_source_buffer_setup_ex(iav_context_t * context,
	struct iav_source_buffer_setup_ex_s __user * arg)
{
	iav_source_buffer_setup_ex_t setup;
	iav_source_buffer_ex_t* buffer = NULL;
	u32 i;
	memset(&setup, 0, sizeof(setup));

	buffer = &G_cap_pre_main;
	setup.pre_main.width = buffer->size.width;
	setup.pre_main.height = buffer->size.height;
	setup.pre_main_input.width = buffer->input.width;
	setup.pre_main_input.height = buffer->input.height;
	setup.pre_main_input.x = buffer->input.x;
	setup.pre_main_input.y = buffer->input.y;

	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		buffer = &G_source_buffer[i];
		setup.unwarp[i] = buffer->unwarp;
		setup.type[i] = buffer->type;
		setup.input[i] = buffer->input;
		if (buffer->enc_stop) {
			setup.size[i].width = 0;
			setup.size[i].height = 0;
		} else {
			setup.size[i] = buffer->size;
		}
	}
	return copy_to_user(arg, &setup, sizeof(setup)) ? -EFAULT : 0;
}

int iav_set_source_buffer_format_all_ex(iav_context_t * context,
	iav_source_buffer_format_all_ex_t __user * arg)
{
	iav_source_buffer_format_all_ex_t format_all;
	iav_source_buffer_setup_ex_t setup;
	iav_reso_ex_t *origin_size = NULL;
	int i, is_buf_warp, specify[IAV_MAX_SOURCE_BUFFER_NUM];
	if (copy_from_user(&format_all, arg, sizeof(format_all)))
		return -EFAULT;

	memset(&setup, 0, sizeof(setup));

	setup.pre_main.width = format_all.pre_main_width;
	setup.pre_main.height = format_all.pre_main_height;
	setup.pre_main_input = format_all.pre_main_input;
	setup.size[0].width = format_all.main_width;
	setup.size[0].height = format_all.main_height;
	setup.unwarp[0] = G_source_buffer[0].unwarp;
	setup.type[0] = G_source_buffer[0].type;

	setup.size[1].width = format_all.second_width;
	setup.size[1].height = format_all.second_height;
	setup.input[1].width = format_all.second_input_width;
	setup.input[1].height = format_all.second_input_height;
	setup.unwarp[1] = format_all.second_unwarp;
	setup.type[1] = G_source_buffer[1].type;

	setup.size[2].width = format_all.third_width;
	setup.size[2].height = format_all.third_height;
	setup.input[2].width = format_all.third_input_width;
	setup.input[2].height = format_all.third_input_height;
	setup.unwarp[2] = format_all.third_unwarp;
	setup.type[2] = G_source_buffer[2].type;

	setup.size[3].width = format_all.fourth_width;
	setup.size[3].height = format_all.fourth_height;
	setup.input[3].width = format_all.fourth_input_width;
	setup.input[3].height = format_all.fourth_input_height;
	setup.unwarp[3] = format_all.fourth_unwarp;
	setup.type[3] = G_source_buffer[3].type;

	if (iav_check_source_buffer_setup(&setup) < 0) {
		return -EINVAL;
	}
	update_source_buffer_setup(&setup);

	/* For sub buffers, just do 0x8000003 when in preview or encoding,
	 * otherwise, only remember the value but do not send DSP cmd
	 */
	if (is_iav_state_prev_or_enc()) {
		memset(specify, 0, sizeof(specify));

		for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
			i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; ++i) {
			origin_size = &G_source_buffer[i].size;
			if ((!G_source_buffer[i].enc_stop &&
					((setup.size[i].width != origin_size->width) ||
					(setup.size[i].height != origin_size->height))) ||
				(G_source_buffer[i].enc_stop &&
					(setup.size[i].width || setup.size[i].height))) {
				specify[i] = 1;
			}

			if (specify[i] && is_buf_type_enc(i)) {
				is_buf_warp = is_warp_mode() && !is_buf_unwarped(i);
				cmd_capture_buffer_setup(i);
				if (is_buf_warp) {
					set_default_warp_dptz_param((1 << i), IAV_BUF_WARP_SET);
				}
			}
		}
	}

	return 0;
}

int iav_get_source_buffer_format_all_ex(iav_context_t * context,
	struct iav_source_buffer_format_all_ex_s __user * arg)
{
	iav_source_buffer_ex_t * buffer = NULL;
	iav_source_buffer_format_all_ex_t buffer_format_all;

	memset(&buffer_format_all, 0, sizeof(buffer_format_all));

	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER];
	buffer_format_all.main_width = buffer->size.width;
	buffer_format_all.main_height = buffer->size.height;

	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_SECOND_BUFFER];
	if (buffer->enc_stop) {
		buffer_format_all.second_width = 0;
		buffer_format_all.second_height = 0;
	} else {
		buffer_format_all.second_width = buffer->size.width;
		buffer_format_all.second_height = buffer->size.height;
	}
	buffer_format_all.second_input_width = buffer->input.width;
	buffer_format_all.second_input_height = buffer->input.height;
	buffer_format_all.second_unwarp = buffer->unwarp;

	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_THIRD_BUFFER];
	if (buffer->enc_stop) {
		buffer_format_all.third_width = 0;
		buffer_format_all.third_height = 0;
	} else {
		buffer_format_all.third_width = buffer->size.width;
		buffer_format_all.third_height = buffer->size.height;
	}
	buffer_format_all.third_input_width = buffer->input.width;
	buffer_format_all.third_input_height = buffer->input.height;
	buffer_format_all.third_unwarp = buffer->unwarp;

	buffer = &G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER];
	if (buffer->enc_stop) {
		buffer_format_all.fourth_width = 0;
		buffer_format_all.fourth_height = 0;
	} else {
		buffer_format_all.fourth_width = buffer->size.width;
		buffer_format_all.fourth_height = buffer->size.height;
	}
	buffer_format_all.fourth_input_width = buffer->input.width;
	buffer_format_all.fourth_input_height = buffer->input.height;
	buffer_format_all.fourth_unwarp = buffer->unwarp;

	buffer = &G_cap_pre_main;
	buffer_format_all.pre_main_width = buffer->size.width;
	buffer_format_all.pre_main_height = buffer->size.height;
	buffer_format_all.pre_main_input.width = buffer->input.width;
	buffer_format_all.pre_main_input.height = buffer->input.height;
	buffer_format_all.pre_main_input.x = buffer->input.x;
	buffer_format_all.pre_main_input.y = buffer->input.y;

	return copy_to_user(arg, &buffer_format_all, sizeof(buffer_format_all)) ?
		-EFAULT : 0;
}

int iav_set_digital_zoom_ex(iav_context_t * context,
	struct iav_digital_zoom_ex_s __user * arg)
{
	vin_cap_win_t vin_cap_win;
	iav_digital_zoom_ex_t dz;
	int flag_vin;

	if (copy_from_user(&dz, arg, sizeof(dz)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set DZ I in non preview/encoding state!\n");
		return -EPERM;
	}

	if (iav_check_digital_zoom_I(&dz) < 0) {
		return -EINVAL;
	}

	/*
	 * Save the dz parameter before deciding whether vin cropping occurs
	 * because vin cropping will modify the input window's offset.
	 */
	G_cap_pre_main.input = dz.input;
	G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].input = dz.input;

	flag_vin = sync_vin_crop(context, &dz.input, 0);
	if (flag_vin) {
		dsp_start_cmdblk(DSP_CMD_PORT_VCAP);
		cmd_set_vin_config(&vin_cap_win);
	}
	cmd_warp_control(&dz.input);
	if (flag_vin) {
		dsp_end_cmdblk(DSP_CMD_PORT_VCAP);
	}

	return 0;
}

int iav_get_digital_zoom_ex(iav_context_t * context,
	struct iav_digital_zoom_ex_s __user * arg)
{
	iav_digital_zoom_ex_t dz;
	dz.input = G_cap_pre_main.input;
	dz.source = IAV_ENCODE_SOURCE_MAIN_BUFFER;
	return copy_to_user(arg, &dz, sizeof(dz)) ? -EFAULT : 0;
}

int iav_set_2nd_digital_zoom_ex(iav_context_t * context,
	struct iav_digital_zoom_ex_s __user * arg)
{
	iav_digital_zoom_ex_t dz;

	if (copy_from_user(&dz, arg, sizeof(dz)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set DZ II in non preview/encoding state!\n");
		return -EPERM;
	}

	if (iav_check_digital_zoom_II(&dz) < 0) {
		return -EINVAL;
	}

	G_source_buffer[dz.source].input = dz.input;
	cmd_capture_buffer_setup(dz.source);

	return 0;
}

int iav_get_2nd_digital_zoom_ex(iav_context_t * context,
	struct iav_digital_zoom_ex_s __user * arg)
{
	iav_digital_zoom_ex_t dz;

	if (copy_from_user(&dz, arg, sizeof(dz)))
		return -EFAULT;

	if (!is_sub_buf(dz.source)) {
		iav_error("DZ II invalid sub source buffer id [%d]\n", dz.source);
		return -EINVAL;
	}

	dz.input = G_source_buffer[dz.source].input;
	return copy_to_user(arg, &dz, sizeof(dz)) ? -EFAULT : 0;
}

int iav_set_digital_zoom_privacy_mask_ex(iav_context_t * context,
	struct iav_digital_zoom_privacy_mask_ex_s __user * arg)
{
	iav_digital_zoom_privacy_mask_ex_t dz_pm;

	if (copy_from_user(&dz_pm, arg, sizeof(dz_pm)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot set digital zoom & privacy mask in non preview / "
			"encoding state!\n");
		return -EPERM;
	}

	if (!is_mctf_pm_enabled()) {
		iav_error("Cannot set digital zoom & privacy mask in [%s] mode.\n",
			G_modes_limit[G_dsp_enc_mode].name);
		return -EPERM;
	}

	// check DPTZ I
	if (iav_check_digital_zoom_I(&dz_pm.zoom) < 0) {
		return -EINVAL;
	}

	// check privacy mask
	if (check_privacy_mask_in_MB(context, &dz_pm.privacy_mask) < 0) {
		return -EINVAL;
	}

	// Update pre-main and main buffer ptz
	G_cap_pre_main.input =
		G_source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER].input =
		dz_pm.zoom.input;

	set_digital_zoom_privacy_mask(context, &dz_pm);

	return 0;
}

int iav_set_preview_buffer_format_all_ex(iav_context_t * context,
	iav_preview_buffer_format_all_ex_t __user * arg)
{
	return -1;	// deprecated
}

int iav_get_preview_buffer_format_all_ex(iav_context_t * context,
	iav_preview_buffer_format_all_ex_t __user * arg)
{
	iav_preview_buffer_format_all_ex_t preview;

	preview.main_preview_width = G_source_buffer[2].size.width;
	preview.main_preview_height = G_source_buffer[2].size.height;
	preview.second_preview_width = G_source_buffer[3].size.width;
	preview.second_preview_height = G_source_buffer[3].size.height;

	return copy_to_user(arg, &preview, sizeof(preview)) ? -EFAULT : 0;
}

int iav_set_preview_A_framerate_divisor_ex(iav_context_t * context,
	u8 __user arg)
{
#define	one_second_in_Q9		(512000000)
	u8 division_factor = arg;
	u32 vin_frametime = get_vin_capability()->frame_rate;

	if (vin_frametime * division_factor > one_second_in_Q9) {
		iav_error("Too large division factor [%d] for preview A!\n",
				division_factor);
		return -EINVAL;
	}
	if (!is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		iav_error("Fourth source buffer is NOT in preview state!\n");
		return -EPERM;
	}

	G_source_buffer[IAV_ENCODE_SOURCE_FOURTH_BUFFER].preview_framerate_division_factor =
		division_factor;
	cmd_update_capture_params_ex(UPDATE_PREVIEW_FRAME_RATE_FLAG);

	return 0;
}

int iav_query_encmode_cap_ex(iav_context_t * context,
	struct iav_encmode_cap_ex_s __user * arg)
{
	int enc_mode;
	iav_enc_mode_limit_t * limit = NULL;
	iav_encmode_cap_ex_t param;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if ((param.encode_mode != IAV_ENCODE_CURRENT_MODE) &&
		((param.encode_mode < IAV_ENCODE_MODE_FIRST) ||
		(param.encode_mode >= IAV_ENCODE_MODE_LAST))) {
		iav_error("Invalid encode mode [%d].\n", param.encode_mode);
		return -EFAULT;
	}
	enc_mode = (param.encode_mode == IAV_ENCODE_CURRENT_MODE) ?
		G_dsp_enc_mode : param.encode_mode;

	limit = &G_modes_limit[enc_mode];
	param.encode_mode = enc_mode;
	param.max_streams_num = limit->max_streams_num;
	param.max_chroma_noise_strength = (1 << (CHROMA_NOISE_BASE_SHIFT +
		limit->max_chroma_noise));
	param.max_encode_MB = limit->max_encode_MB;
	param.main_width_min = limit->main_width_min;
	param.main_height_min = limit->main_height_min;
	param.main_width_max = limit->main_width_max;
	param.main_height_max = limit->main_height_max;
	param.min_encode_width = limit->min_encode_width;
	param.min_encode_height = limit->min_encode_height;

	param.sharpen_b_possible = limit->sharpen_b_possible;
	param.rotate_possible = limit->rotate_possible;
	param.raw_cap_possible = limit->raw_cap_possible;
	param.raw_stat_cap_possible = limit->raw_stat_cap_possible;
	param.dptz_I_possible = limit->dptz_I_possible;
	param.dptz_II_possible = limit->dptz_II_possible;
	param.vin_cap_offset_possible = limit->vin_cap_offset_possible;
	param.hwarp_bypass_possible = limit->hwarp_bypass_possible;
	param.svc_t_possible = limit->svc_t_possible;
	param.enc_from_yuv_possible = limit->enc_from_yuv_possible;
	param.enc_from_raw_possible = limit->enc_from_raw_possible;
	param.vout_swap_possible = limit->vout_swap_possible;
	param.mctf_pm_possible = limit->mctf_pm_possible;
	param.hdr_pm_possible = limit->hdr_pm_possible;
	param.video_freeze_possible = limit->video_freeze_possible;
	param.mixer_b_possible = limit->mixer_b_possible;
	param.vca_buffer_possible = limit->vca_buffer_possible;
	param.vout_b_letter_box_possible = limit->vout_b_letter_box_possible;
	param.yuv_input_enhanced_possible = limit->yuv_input_enhanced_possible;
	param.map_dsp_partition = is_map_dsp_partition();

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_query_encbuf_cap_ex(iav_context_t * context,
	struct iav_encbuf_cap_ex_s __user * arg)
{
	iav_encbuf_cap_ex_t param;
	iav_source_buffer_property_ex_t *property = NULL;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (!is_valid_buffer_id(param.buf_id)) {
		iav_error("Invalid buffer id [%d].\n", param.buf_id);
		return -EINVAL;
	}

	property = &G_source_buffer[param.buf_id].property;

	param.max_width = property->max.width;
	param.max_height = property->max.height;
	param.max_zoom_in_factor = property->max_zoom_in_factor;
	param.max_zoom_out_factor = property->max_zoom_out_factor;

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_get_chip_id_ex(iav_context_t * context, int __user * arg)
{
	if (is_iav_state_init()) {
		iav_error("Cannot get chip ID before system booted!\n");
		return -EPERM;
	}
	if (unlikely(G_iav_obj.dsp_chip_id == IAV_CHIP_ID_S2_UNKNOWN)) {
		G_iav_obj.dsp_chip_id = dsp_get_chip_id();
	}
	return copy_to_user(arg, &G_iav_obj.dsp_chip_id, sizeof(u32)) ?
		-EFAULT : 0;
}

int iav_enc_dram_request_frame(iav_context_t * context,
	struct iav_enc_dram_buf_frame_ex_s __user *arg)
{
	u32 addr;
	u16 buf_id, frame_id, retry;
	iav_source_buffer_dram_ex_t * dram;
	iav_source_buffer_dram_pool_ex_t * pool;
	iav_enc_dram_buf_frame_ex_t frame;
	u32 phys_start = 0, size = 0;

	if (copy_from_user(&frame, arg, sizeof(frame)))
		return -EFAULT;

	if (check_enc_dram_buf_frame(&frame) < 0)
		return -EAGAIN;

	buf_id = frame.buf_id;
	dram = &G_source_buffer[buf_id].dram;
	dram->buf_state = DSP_DRAM_BUFFER_POOL_REQUEST;
	pool = &dram->buf_pool;

	retry = 1;
	while (1) {
		if (!(pool->frame_req_done & (1 << IAV_YUV_400_FORMAT))) {
			cmd_enc_dram_req_buffer(pool->id[IAV_YUV_400_FORMAT]);
		}
		if (!(pool->frame_req_done & (1 << IAV_YUV_420_FORMAT))) {
			cmd_enc_dram_req_buffer(pool->id[IAV_YUV_420_FORMAT]);
		}

		iav_unlock();
		wait_enc_dram(buf_id, DSP_DRAM_BUFFER_POOL_READY);
		iav_lock();

		/* Successful to request frames for YUV / ME1 both, quit. */
		if ((pool->frame_req_done & (1 << IAV_YUV_400_FORMAT)) &&
			(pool->frame_req_done & (1 << IAV_YUV_420_FORMAT))) {
			break;
		}

		/* Timeout to request frame from DSP */
		if (++retry > IAV_MAX_ENC_DRAM_BUF_NUM) {
			iav_error("Retry [%d] times fail. Timeout to request frame from"
				" buffer pool inside DSP.\n", IAV_MAX_ENC_DRAM_BUF_NUM);
			return -EAGAIN;
		}
	}

	/* Update requested frame info */
	frame.max_size = dram->buf_max_size;
	frame_id = pool->req_index;
	frame.frame_id = frame_id;
	frame.y_addr = 0;
	frame.uv_addr = 0;
	frame.me1_addr = 0;
	addr = pool->y[frame_id];
	if (likely(addr && (addr != 0xdeadbeef))) {
		iav_get_dsp_partition(partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV), &phys_start, &size);
		//frame.y_addr = dsp_dsp_to_user(context, addr);
		if (is_map_dsp_partition()) {
			frame.y_addr = DSP_TO_PHYS(addr) - phys_start + context->dsp_partition[partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV)].user_start;
		} else {
			frame.y_addr = dsp_dsp_to_user(context, addr);
		}
	} else {
		iav_error("Failed to request YUV data for encode!\n");
		return -EFAULT;
	}
	addr = pool->uv[frame_id];
	if (likely(addr && (addr != 0xdeadbeef))) {
		iav_get_dsp_partition(partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV), &phys_start, &size);
		//frame.uv_addr = dsp_dsp_to_user(context, addr);
		if (is_map_dsp_partition()) {
			frame.uv_addr = DSP_TO_PHYS(addr) - phys_start + context->dsp_partition[partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV)].user_start;
		} else {
			frame.uv_addr = dsp_dsp_to_user(context, addr);
		}
	} else {
		iav_error("Failed to request YUV data for encode!\n");
		return -EFAULT;
	}
	addr = pool->me1[frame_id];
	if (likely(addr && (addr != 0xdeadbeef))) {
		iav_get_dsp_partition(partition_id_to_index(IAV_DSP_PARTITION_EFM_ME1), &phys_start, &size);
		//frame.me1_addr = dsp_dsp_to_user(context, addr);
		if (is_map_dsp_partition()) {
			frame.me1_addr = DSP_TO_PHYS(addr) - phys_start + context->dsp_partition[partition_id_to_index(IAV_DSP_PARTITION_EFM_ME1)].user_start;
		} else {
			frame.me1_addr = dsp_dsp_to_user(context, addr);
		}
	} else {
		iav_error("Failed to request ME1 data for encode!\n");
		return -EFAULT;
	}
	pool->req_index = (pool->req_index + 1) % IAV_MAX_ENC_DRAM_BUF_NUM;

	return copy_to_user(arg, &frame, sizeof(frame)) ? -EFAULT : 0;
}

int iav_enc_dram_buf_update_frame(iav_context_t * context,
	struct iav_enc_dram_buf_update_ex_s __user *arg)
{
	u8 * virt_addr = NULL;
	u16 buf_id, frame_id;
	u32 frame_size, dsp_addr, me1_size;
	iav_source_buffer_dram_ex_t * dram;
	iav_source_buffer_dram_pool_ex_t * pool;
	iav_enc_dram_buf_update_ex_t update_frame;

	if (copy_from_user(&update_frame, arg, sizeof(update_frame)))
		return -EFAULT;

	if (check_enc_dram_buf_update(&update_frame) < 0)
		return -EAGAIN;

	buf_id = update_frame.buf_id;
	dram = &G_source_buffer[buf_id].dram;
	dram->frame_pts = update_frame.frame_pts;
	frame_size = dram->buf_pitch * G_source_buffer[buf_id].size.height;
	me1_size = frame_size >> 2;	/* ME1 pitch is same as YUV buffer pitch */
	pool = &dram->buf_pool;
	frame_id = update_frame.frame_id;
	pool->update_index = frame_id;

	if (!is_map_dsp_partition()) {
		if (!G_iav_obj.dsp_noncached) {
			/* Clean cache if DSP memory is cached. */
			dsp_addr = pool->y[frame_id];
			virt_addr = (u8*)DSP_TO_AMBVIRT(dsp_addr);
			clean_cache_aligned(virt_addr, frame_size);
			dsp_addr = pool->uv[frame_id];
			virt_addr = (u8*)DSP_TO_AMBVIRT(dsp_addr);
			clean_cache_aligned(virt_addr, (frame_size >> 1));
			dsp_addr = pool->me1[frame_id];
			virt_addr = (u8*)DSP_TO_AMBVIRT(dsp_addr);
			clean_cache_aligned(virt_addr, me1_size);
		}
	}

	pool->y[frame_id] = 0;
	pool->uv[frame_id] = 0;
	pool->me1[frame_id] = 0;
	pool->frame_req_done = 0;
	cmd_enc_dram_handshake(buf_id);
	dram->valid_frame_num++;

	return 0;
}

int iav_enc_dram_release_frame(iav_context_t * context,
	struct iav_enc_dram_buf_frame_ex_s __user *arg)
{
	return 0;
}

#ifdef CONFIG_GUARD_VSYNC_LOSS
int iav_vsync_guard_init(void)
{
	G_iav_obj.vsync_error_again = 0;
	G_iav_obj.vsync_error_handling = 0;

		/* code for guardian thread */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
	kernel_thread(iav_vsync_guard_task, NULL, 0);
#else
	kthread_run(iav_vsync_guard_task, NULL, "vsync_guard");
#endif

	return 0;
}

int iav_vsync_guard_task(void *arg)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
	daemonize("vsync_guard");
#endif
	while (1) {
		iav_lock();

		if (!G_iav_obj.vsync_error_again) {
			wait_vsync_loss_msg();
		}
		G_iav_obj.vsync_error_handling = 1;
		iav_printk("Start restoring vsync loss!\n");
		nl_restore_vsync_loss();
		iav_printk("End restoring vsync loss!\n");
		G_iav_obj.vsync_error_handling = 0;

		iav_unlock();
	}

	return 0;
}
#endif

static int check_video_proc_state(void)
{
	if (!is_iav_state_prev_or_enc()) {
		iav_error("Cannot get main preproc param when not in preview/encoding"
			" state\n");
		return -1;
	}
	return 0;
}

int get_digital_zoom_param(iav_digital_zoom_ex_t* dptz)
{
	switch (dptz->source) {
		case IAV_ENCODE_SOURCE_MAIN_BUFFER:
			dptz->input = G_cap_pre_main.input;
			dptz->source = IAV_ENCODE_SOURCE_MAIN_BUFFER;
			break;
		default:
			if (!is_sub_buf(dptz->source)) {
				iav_error("DPTZ II source %d is not sub buffer.\n",
				    dptz->source);
				return -1;
			}
			dptz->input = G_source_buffer[dptz->source].input;
			break;
	}
	return 0;
}

int cfg_digtal_zoom_param(iav_digital_zoom_ex_t* dptz)
{
	switch (dptz->source) {
		case IAV_ENCODE_SOURCE_MAIN_BUFFER:
			if (iav_check_digital_zoom_I(dptz) < 0) {
				return -1;
			}
			G_cap_pre_main.input = dptz->input;
			break;
		default:
			if (iav_check_digital_zoom_II(dptz) < 0) {
				return -1;
			}
			break;
	}

	G_source_buffer[dptz->source].input = dptz->input;
	return 0;
}

int iav_get_video_proc(iav_context_t * context, void __user * arg)
{
	iav_video_proc_t param;
	int rval = 0;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (check_video_proc_state() < 0)
		return -EPERM;

	switch (param.cid) {
		case IAV_DPTZ:
			rval = get_digital_zoom_param(&param.arg.dptz);
			break;
		case IAV_PM:
			rval = get_privacy_mask_param(&param.arg.pm);
			break;
		case IAV_CAWARP:
			rval = get_ca_warp_param(&param.arg.cawarp);
			break;
		case IAV_STREAM_PM:
			rval = get_stream_privacy_mask_param(&param.arg.stream_pm);
			break;
		case IAV_STREAM_OFFSET:
			rval = get_stream_offset_param(&param.arg.stream_offset);
			break;
		case IAV_STREAM_OVERLAY:
			rval = get_stream_overlay_param(&param.arg.stream_overlay);
			break;
		default:
			iav_error("Unknown cid: %d.\n", param.cid);
			return -EINVAL;
	}
	return (rval < 0) ? -EINVAL :
		(copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0);
}

int iav_cfg_video_proc(iav_context_t * context, void __user * arg)
{
	int rval = 0;
	iav_video_proc_t param;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (check_video_proc_state() < 0)
		return -EPERM;

	switch (param.cid) {
		case IAV_DPTZ:
			rval = cfg_digtal_zoom_param(&param.arg.dptz);
			break;
		case IAV_PM:
			rval = cfg_privacy_mask_param(context, &param.arg.pm);
			break;
		case IAV_CAWARP:
			rval = cfg_ca_warp_param(&param.arg.cawarp);
			break;
		case IAV_STREAM_PM:
			rval = cfg_stream_privacy_mask_param(context, &param.arg.stream_pm);
			break;
		case IAV_STREAM_OFFSET:
			rval = cfg_stream_offset_param(&param.arg.stream_offset);
			break;
		case IAV_STREAM_OVERLAY:
			rval = cfg_stream_overlay_param(context, &param.arg.stream_overlay);
			break;
		default:
			iav_error("Unknown cid: %d.\n", param.cid);
			rval = -1;
			break;
	}
	return (rval < 0) ? -EINVAL :  0;
}

int iav_apply_video_proc(iav_context_t * context, void __user * arg)
{
	u32 i;
	u8 flag_le = 0, flag_vin = 0;
	iav_rect_ex_t dz_input;
	vin_cap_win_t vin_cap_win;
	iav_apply_flag_t flags[IAV_VIDEO_PROC_NUM];

	if (copy_from_user(flags, arg, sizeof(flags)))
		return -EFAULT;

	if (check_video_proc_state() < 0)
		return -EPERM;

	if (flags[IAV_DPTZ].apply
	    && (flags[IAV_DPTZ].param & (1 << IAV_ENCODE_SOURCE_MAIN_BUFFER))) {
		//Fixme: Bind vin cropping if necessary.
		dz_input = G_cap_pre_main.input;
		flag_vin = sync_vin_crop(context, &dz_input, 0);
	}

	if (flags[IAV_PM].apply) {
		flag_le = sync_le_with_pm();
	}

	dsp_start_cmdblk(DSP_CMD_PORT_VCAP);

	//Fixme: Bind vin cropping if necessary.
	if (flag_vin) {
		cmd_set_vin_config(&vin_cap_win);
	}

	if (flags[IAV_DPTZ].apply) {
		if (flags[IAV_DPTZ].param & (1 << IAV_ENCODE_SOURCE_MAIN_BUFFER)) {
			cmd_warp_control(&dz_input);
		}
		for (i = IAV_ENCODE_SUB_SOURCE_BUFFER_FIRST;
		    i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST; i++) {
			if (is_buf_type_enc(i) && (flags[IAV_DPTZ].param & (1 << i))) {
				cmd_capture_buffer_setup(i);
			}
		}
	}
	if (flags[IAV_PM].apply) {
		cmd_set_privacy_mask(context);
	}
	if (flags[IAV_CAWARP].apply) {
		cmd_set_ca_warp();
	}

	if (flags[IAV_STREAM_PM].apply) {
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
			if (flags[IAV_STREAM_PM].param & (1 << i)) {
				cmd_update_encode_params_ex(context, i, UPDATE_STREAM_PM_FLAG);
			}
		}
	}

	if (flags[IAV_STREAM_OFFSET].apply) {
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
			if (flags[IAV_STREAM_OFFSET].param & (1 << i)) {
				cmd_update_encode_params_ex(context, i,
				    UPDATE_STREAM_OFFSET_FLAG);
			}
		}
	}

	if (flags[IAV_STREAM_OVERLAY].apply) {
		for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; i++) {
			if (flags[IAV_STREAM_OVERLAY].param & (1 << i)) {
				cmd_set_overlay_insert(context, i);
			}
		}
	}

	//Fixme: Bind local exposure if necessary.
	if (flag_le) {
		cmd_set_local_exposure();
	}

	dsp_end_cmdblk(DSP_CMD_PORT_VCAP);

	return 0;
}

int iav_get_vcap_proc(iav_context_t * context, void __user *arg)
{
	int rval = 0;
	iav_vcap_proc_t param;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	switch (param.vid) {
	case IAV_VCAP_FREEZE:
		param.arg.video_freeze = G_iav_vcap.freeze_enable;
		break;
	default:
		iav_error("Unknow vid: %d.\n", param.vid);
		return -EINVAL;
	}
	return (rval < 0) ? -EINVAL :
		(copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0);
}

int iav_cfg_vcap_proc(iav_context_t * context, void __user *arg)
{
	int rval = 0;
	u32 value, enc_mode;
	iav_vcap_proc_t param;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	enc_mode = get_enc_mode();
	switch (param.vid) {
	case IAV_VCAP_FREEZE:
		value = !!param.arg.video_freeze;
		if (value && !G_modes_limit[enc_mode].video_freeze_possible) {
			iav_error("'Video freeze' is forbidden in mode [%d].\n", enc_mode);
			return -EINVAL;
		}
		G_iav_vcap.freeze_enable = value;
		break;
	default:
		iav_error("Unknown vid: %d.\n", param.vid);
		rval = -1;
		break;
	}

	return (rval < 0) ? -EINVAL : 0;
}

int iav_apply_vcap_proc(iav_context_t * context, void __user *arg)
{
	u32 update_flag = 0;
	iav_apply_flag_t flags[IAV_VCAP_PROC_NUM];

	if (copy_from_user(flags, arg, sizeof(flags)))
		return -EFAULT;

	if (flags[IAV_VCAP_FREEZE].apply &&
		(G_iav_vcap.freeze_enable != G_iav_vcap.freeze_enable_saved)) {
		update_flag |= UPDATE_FREEZE_ENABLED_FLAG;
		G_iav_vcap.freeze_enable_saved = G_iav_vcap.freeze_enable;
	}

	if (update_flag) {
		cmd_update_capture_params_ex(update_flag);
	}

	return 0;
}

static void filter_dsp_map_id_map(u32 *id_map)
{
	struct amba_iav_vout_info * vout_info_a = NULL, *vout_info_b = NULL;
	int	enc_mode;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;

	enc_mode = get_enc_mode();
	resource = &G_system_resource_setup[enc_mode];

	if (!is_raw_capture_enabled() &&
		(!is_raw_stat_capture_enabled() ||
			(!resource->max_vin_stats_num_lines_top &&
			!resource->max_vin_stats_num_lines_bot))) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_RAW);
	} else {
		if (G_modes_limit[enc_mode].raw_cap_possible) {
			set_dsp_id_map(id_map, IAV_DSP_PARTITION_RAW);
		} else {
			clear_dsp_id_map(id_map, IAV_DSP_PARTITION_RAW);
		}
	}

	if (is_buf_type_off(IAV_ENCODE_SOURCE_MAIN_DRAM)) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_EFM_ME1);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_EFM_YUV);
	}

	if (is_buf_type_off(IAV_ENCODE_SOURCE_MAIN_BUFFER)) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_MAIN_CAPTURE);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_MAIN_CAPTURE_ME1);
	}

	if (is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_C);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_C_ME1);
	}

	if (is_buf_type_off(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_B);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_B_ME1);
	}

	if (is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_A);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_A_ME1);
	}

	if (is_buf_type_prev(IAV_ENCODE_SOURCE_MAIN_BUFFER)) {
		set_dsp_id_map(id_map, IAV_DSP_PARTITION_MAIN_CAPTURE);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_MAIN_CAPTURE_ME1);
	}

	if (is_buf_type_prev(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		set_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_C);
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_C_ME1);
	}

	if (G_system_setup_info[G_dsp_enc_mode].vout_swap) {
		vout_info_a = G_iav_info.pvoutinfo[1];
		vout_info_b = G_iav_info.pvoutinfo[0];
	} else {
		vout_info_a = G_iav_info.pvoutinfo[0];
		vout_info_b = G_iav_info.pvoutinfo[1];
	}

	if (is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		if (!vout_info_b->enabled) {
			clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_B);
		} else {
			set_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_B);
		}
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_B_ME1);
	}

	if (is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		if (!vout_info_a->enabled) {
			clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_A);
		} else {
			set_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_A);
		}
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_PREVIEW_A_ME1);
	}

	//alreradly query while entering preview
	if (G_dsp_partition.id_map & (1 << partition_id_to_index(IAV_DSP_PARTITION_EFM_ME1))) {
		set_dsp_id_map(id_map, IAV_DSP_PARTITION_EFM_ME1);
	}

	if (G_dsp_partition.id_map & (1 << partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV))) {
		set_dsp_id_map(id_map, IAV_DSP_PARTITION_EFM_YUV);
	}

	if (is_warp_mode()) {
		set_dsp_id_map(id_map, IAV_DSP_PARTITION_POST_MAIN);
	} else {
		clear_dsp_id_map(id_map, IAV_DSP_PARTITION_POST_MAIN);
	}
}
int iav_get_frm_buf_pool_info(u32 *id_map)
{
	int i = 0;
	u32 phys_start = 0, size = 0;
	init_waitqueue_head(&G_dsp_partition.wq);

	filter_dsp_map_id_map(id_map);

	for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
		if (*id_map & (1 << i)) {
			iav_get_dsp_partition(i, &phys_start, &size);
			// if we do not get the phys addr of this partition
			if (!phys_start || !size) {
				if ((i == partition_id_to_index(IAV_DSP_PARTITION_EFM_ME1) ||
					i == partition_id_to_index(IAV_DSP_PARTITION_EFM_YUV))
					&& (!G_source_buffer[IAV_ENCODE_SOURCE_MAIN_DRAM].dram.max_frame_num)) {
						iav_error("EFM buffer has not been configured!\n");
						return -EFAULT;
				} else {
					if (i == partition_id_to_index(IAV_DSP_PARTITION_POST_MAIN)) {
						cmd_get_frm_buf_pool_info(FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE, 0);
					} else {
						cmd_get_frm_buf_pool_info(i, 0);
					}
				}
			}
		}
	}

	G_dsp_partition.id_map_user = *id_map;

	if (G_dsp_partition.id_map != G_dsp_partition.id_map_user) {
		wait_event_interruptible(G_dsp_partition.wq,
			G_dsp_partition.id_map == G_dsp_partition.id_map_user);
	}

	for (i = 0; i < IAV_DSP_PARTITION_NUM; i++) {
		iav_get_dsp_partition(i, &phys_start, &size);
		if (*id_map & (1 << i)) {
			iav_get_dsp_partition(i, &phys_start, &size);
			if (!phys_start || !size) {
				iav_error("DSP partition %d has not been allocated by DSP yet\n", i);
				return -EFAULT;
			}
		}
	}

	return 0;
}

int iav_vout_cross_check(iav_context_t *context,
	struct amba_video_sink_mode *cfg)
{
	if (!is_iav_state_idle()) {
		iav_error("iav_configure_sink: harware is busy\n");
		return -1;
	}

	if (cfg->display_input == AMBA_VOUT_INPUT_FROM_MIXER) {
		G_iav_vcap.vout_b_mixer_enable = 1;
	} else {
		G_iav_vcap.vout_b_mixer_enable = 0;
	}

	if (cfg->video_offset.offset_y == 0) {
		return 0;
	}
	if (cfg->display_input == AMBA_VOUT_INPUT_FROM_SMEM) {
		if (((cfg->video_size.vout_height - cfg->video_size.video_height) >> 1) !=
			(cfg->video_offset.offset_y + 1)) {
			iav_error("Vout offset_y must be %d when Mixer b is OFF\n",
				(cfg->video_size.vout_height - cfg->video_size.video_height) >> 1);
			return -1;
		}
	}

	return 0;
}

int iav_vout_update_sink_cfg(iav_context_t *context,
	struct amba_video_sink_mode *cfg)
{
	if (cfg->display_input == AMBA_VOUT_INPUT_FROM_MIXER) {
		G_iav_vcap.mixer_b_enable = 1;
	} else {
		G_iav_vcap.mixer_b_enable = 0;
	}

	return 0;
}
