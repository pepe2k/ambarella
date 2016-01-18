/*
 * iav_overlay.c
 *
 * History:
 *    2011/10/25 - [Jian Tang] created file
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
#include "amba_dsp.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"


#define	OVERLAY_CLUT_OFFSET	(0)
#define	OVERLAY_AREA_NUM		(MAX_NUM_OVERLAY_AREA)

#define	OVERLAY_CLUT_NUM		(OVERLAY_AREA_NUM * IAV_STREAM_MAX_NUM_IMPL)
#define	OVERLAY_CLUT_SIZE		(256*4)
#define	OVERLAY_DATA_OFFSET 	(OVERLAY_CLUT_SIZE* OVERLAY_CLUT_NUM)

#define	INVALID_ADDR			(0xFFFFFFFF)
#define MAX_NUM_OSD_AERA_NO_EXTENSION (3)

/*
 * The overlay memory layout:
 * It starts with OVERLAY_CLUT_NUM of CLUT, each CLUT is 1024 bytes, 256
 * entries. Data array should start after CLUT at any place. CLUT is put into
 * fixed location, so it's possible to just change CLUT to do palette
 * animation.
 *
 * 0 ~ 16KB for 16 CLUTs, each CLUT is 1KB :
 * 	|--CLUT--|--CLUT--|--CLUT--| ...  |--CLUT--|
 *
 * 16KB ~ 1MB for 4 streams, each has 3 areas :
 * 	|--- Data Array---|--- Data Array---|--- Data Array---|--- Data Array---|
 */


static overlay_insert_ex_t G_overlay_insert[IAV_MAX_ENCODE_STREAMS_NUM] =
{
	{
		.id = IAV_MAIN_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_2ND_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_3RD_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_4TH_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_5TH_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_6TH_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_7TH_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
	{
		.id = IAV_8TH_STREAM,
		.enable = 0,
		.type = IAV_OSD_CLUT_8BIT,
		.insert_always = 0,
	},
};

static u32 get_overlay_data_phy_addr(iav_context_t *context, u8 *addr)
{
	struct iav_mem_block *overlay;
	if (addr < context->overlay.user_start ||
		addr >= context->overlay.user_end) {
		iav_printk("overlay addr not in range %p\n", addr);
		return 0;
	}
	iav_get_mem_block(IAV_MMAP_OVERLAY, &overlay);
	return (u32)(addr - context->overlay.user_start + overlay->phys_start);
}

static u32 get_overlay_data_kernel_addr(iav_context_t *context, u8 * addr)
{
	struct iav_mem_block *overlay;
	if (addr < context->overlay.user_start ||
		addr >= context->overlay.user_end) {
		iav_printk("Overlay addr not in range %p\n", addr);
		return 0;
	}
	iav_get_mem_block(IAV_MMAP_OVERLAY, &overlay);
	return (u32)(addr - context->overlay.user_start + overlay->kernel_start);
}

static u32 get_overlay_clut_phy_addr(u32 clut_id)
{
	struct iav_mem_block *overlay;
	if (clut_id >= OVERLAY_CLUT_NUM) {
		iav_printk("wrong clut id \n");
		return INVALID_ADDR;
	}
	iav_get_mem_block(IAV_MMAP_OVERLAY, &overlay);
	return (overlay->phys_start + clut_id * OVERLAY_CLUT_SIZE);
}

static u32 get_overlay_clut_kernel_addr(u32 clut_id)
{
	struct iav_mem_block *overlay;
	if (clut_id >= OVERLAY_CLUT_NUM) {
		iav_printk("wrong clut id \n");
		return INVALID_ADDR;
	}
	iav_get_mem_block(IAV_MMAP_OVERLAY, &overlay);
	return ((u32)overlay->kernel_start + clut_id * OVERLAY_CLUT_SIZE);
}

static int check_overlay_insert_area(iav_context_t * context,
	overlay_insert_ex_t *info, int stream)
{
	int i, width, height;
	overlay_insert_area_ex_t *area;

	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
	if (check_full_fps_full_perf_cap("Overlay") < 0) {
		return -1;
	}

	width = G_iav_obj.stream[stream].format.encode_width;
	height = G_iav_obj.stream[stream].format.encode_height;
	for (i = 0; i < OVERLAY_AREA_NUM; ++i) {
		area = &info->area[i];
		if (area->enable) {
			if (area->clut_id >= OVERLAY_CLUT_NUM) {
				iav_error("overlay area clut id [%d] must be no larger than %d.\n",
					area->clut_id, (OVERLAY_CLUT_NUM - 1));
				return -1;
			}

			if (!area->pitch || !area->width || !area->height) {
				iav_error("overlay area pitch/width/height cannot be zero.\n");
				return -1;
			}

			if (area->pitch < area->width) {
				iav_error("overlay area pitch must be not smaller than width.\n");
				return -1;
			}
			if (area->start_x & 0x1) {
				iav_error("overlay area start x must be multiple of 2.\n");
				return -1;
			}
			if (area->start_y & 0x3) {
				iav_error("overlay area start y must be multiple of 4.\n");
				return -1;
			}
			if (area->width & 0x1) {
				iav_error("overlay area width must be multiple of 2.\n");
				return -1;
			}
			if (area->height & 0x3) {
				iav_error("overlay area height must be multiple of 4.\n");
				return -1;
			}
			if (area->width > OVERLAY_AREA_MAX_WIDTH) {
				iav_error("overlay area width [%d] must be no greater than %d.\n",
						area->width, OVERLAY_AREA_MAX_WIDTH);
				return -1;
			}
			if ((area->start_x + area->width) > width) {
				iav_error("overlay area width [%d] with offset [%d] is out of stream width [%d].\n",
						area->width, area->start_x, width);
				return -1;
			}
			if ((area->start_y + area->height) > height) {
				iav_error("overlay area height [%d] with offset [%d] is out of stream height [%d].\n",
						area->height, area->start_y, height);
				return -1;
			}
			if (area->data < context->overlay.user_start ||
				area->data >= context->overlay.user_end) {
				iav_error("Overlay addr [%p] not in the range!\n", area->data);
				return -1;
			}
			if (get_overlay_clut_phy_addr(area->clut_id) == INVALID_ADDR) {
				iav_error("Invalid CLUT address [0x%X].\n", INVALID_ADDR);
				return -1;
			}
			if (get_overlay_data_phy_addr(context, area->data) == INVALID_ADDR) {
				iav_error("Invalid Data address [0x%X].\n", (u32)area->data);
				return -1;
			}
		}
	}

	return 0;
}

void cmd_set_overlay_insert(iav_context_t * context, int stream)
{
	ENCODER_OSD_INSERT_CMD dsp_cmd;
	overlay_insert_area_ex_t area;
	int i, active_areas, area_index = 0;
	u32 addr;
	u16 width, height;
	s16 offset_y_shift;
	overlay_insert_ex_t* info = &G_overlay_insert[stream];

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_H264ENC_OSD_INSERT;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);

	if (info->enable != 0) {
		if (G_iav_obj.system_setup_info->voutA_osd_blend_enable == 1) {
			dsp_cmd.vout_id = 0;
		} else if (G_iav_obj.system_setup_info->voutB_osd_blend_enable == 1) {
			dsp_cmd.vout_id = 1;
		} else {
			iav_error("osd_blend disabled in system_info_setup!\n");
			return;
		}

		switch (info->type) {
			case IAV_OSD_AYUV_16BIT:
				dsp_cmd.osd_enable = OSD_TYPE_DIRECT_16BIT;
				dsp_cmd.osd_mode = OSD_MODE_AYUV;
				break;
			case IAV_OSD_ARGB_16BIT:
				dsp_cmd.osd_enable = OSD_TYPE_DIRECT_16BIT;
				dsp_cmd.osd_mode = OSD_MODE_ARGB;
				break;
			default:
				dsp_cmd.osd_enable = OSD_TYPE_MAP_CLUT_8BIT;
				dsp_cmd.osd_mode = OSD_MODE_CLUT;
				break;
		}

		dsp_cmd.osd_csc_param_dram_address = 0;
		dsp_cmd.osd_insert_always = info->insert_always;

		get_round_encode_format(stream, &width, &height, &offset_y_shift);

		for (i = 0, active_areas = 0; i < OVERLAY_AREA_NUM; ++i) {
			area = info->area[i];
			if (area.enable) {
				switch (info->type) {
					case IAV_OSD_AYUV_16BIT:
					case IAV_OSD_ARGB_16BIT:
						if (active_areas < MAX_NUM_OSD_AERA_NO_EXTENSION) {
							dsp_cmd.osd_clut_dram_address[active_areas] = 0;
						} else {
							area_index = active_areas - MAX_NUM_OSD_AERA_NO_EXTENSION;
							dsp_cmd.osd_clut_dram_address_ex[area_index] = 0;
						}
						break;
					default:
						addr = PHYS_TO_DSP(get_overlay_clut_phy_addr(area.clut_id));
						if (active_areas < MAX_NUM_OSD_AERA_NO_EXTENSION) {
							dsp_cmd.osd_clut_dram_address[active_areas] = addr;
						} else {
							area_index = active_areas - MAX_NUM_OSD_AERA_NO_EXTENSION;
							dsp_cmd.osd_clut_dram_address_ex[area_index] = addr;
						}
						break;
				}
				addr = PHYS_TO_DSP(get_overlay_data_phy_addr(context, area.data));
				if (active_areas < MAX_NUM_OSD_AERA_NO_EXTENSION) {
					dsp_cmd.osd_buf_dram_address[active_areas] = addr;
					dsp_cmd.osd_buf_pitch[active_areas] = area.pitch;
					dsp_cmd.osd_win_offset_x[active_areas] = area.start_x;
					dsp_cmd.osd_win_offset_y[active_areas] = area.start_y - offset_y_shift;
					dsp_cmd.osd_win_w[active_areas] = area.width;
					dsp_cmd.osd_win_h[active_areas] = area.height;
				} else {
					dsp_cmd.osd_buf_dram_address_ex[area_index] = addr;
					dsp_cmd.osd_buf_pitch_ex[area_index] = area.pitch;
					dsp_cmd.osd_win_offset_x_ex[area_index] = area.start_x;
					dsp_cmd.osd_win_offset_y_ex[area_index] = area.start_y - offset_y_shift;
					dsp_cmd.osd_win_w_ex[area_index] = area.width;
					dsp_cmd.osd_win_h_ex[area_index] = area.height;
					dsp_cmd.osd_enable_ex = 1;
				}

				++active_areas;
				// clean cache on data buffer
				clean_cache_aligned((void *)get_overlay_data_kernel_addr(
					context, area.data), area.pitch * area.height);
				// clean cache on clut buffer
				clean_cache_aligned((void *)get_overlay_clut_kernel_addr(
					area.clut_id), OVERLAY_CLUT_SIZE);

/*				iav_printk("Overlay area %d, clut 0x%08X, clut id %d, data 0x%08X.\n",
					i, dsp_cmd.osd_clut_dram_address[active_areas],
					area.clut_id, dsp_cmd.osd_buf_dram_address[active_areas]);*/
			}
		}

		if (active_areas <= MAX_NUM_OSD_AERA_NO_EXTENSION) {
			dsp_cmd.osd_num_regions = active_areas;
		} else {
			dsp_cmd.osd_num_regions = MAX_NUM_OSD_AERA_NO_EXTENSION;
			dsp_cmd.osd_num_regions_ex = active_areas - MAX_NUM_OSD_AERA_NO_EXTENSION;
		}
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, osd_enable);
	iav_dsp(dsp_cmd, osd_insert_always);
	iav_dsp(dsp_cmd, osd_mode);
	iav_dsp(dsp_cmd, osd_num_regions);
	iav_dsp(dsp_cmd, osd_enable_ex);
	iav_dsp(dsp_cmd, osd_num_regions_ex);
	iav_dsp(dsp_cmd, vout_id);

#if 0
	iav_dsp(dsp_cmd, osd_mode);
	iav_dsp_hex(dsp_cmd, osd_csc_param_dram_address);
	iav_dsp_hex(dsp_cmd, osd_clut_dram_address[0]);
	iav_dsp_hex(dsp_cmd, osd_buf_dram_address[0]);
	iav_dsp(dsp_cmd, osd_buf_pitch[0]);
	iav_dsp(dsp_cmd, osd_win_offset_x[0]);
	iav_dsp(dsp_cmd, osd_win_offset_y[0]);
	iav_dsp(dsp_cmd, osd_win_w[0]);
	iav_dsp(dsp_cmd, osd_win_h[0]);
	iav_dsp_hex(dsp_cmd, osd_clut_dram_address[1]);
	iav_dsp_hex(dsp_cmd, osd_buf_dram_address[1]);
	iav_dsp(dsp_cmd, osd_buf_pitch[1]);
	iav_dsp(dsp_cmd, osd_win_offset_x[1]);
	iav_dsp(dsp_cmd, osd_win_offset_y[1]);
	iav_dsp(dsp_cmd, osd_win_w[1]);
	iav_dsp(dsp_cmd, osd_win_h[1]);
	iav_dsp_hex(dsp_cmd, osd_clut_dram_address[2]);
	iav_dsp_hex(dsp_cmd, osd_buf_dram_address[2]);
	iav_dsp(dsp_cmd, osd_buf_pitch[2]);
	iav_dsp(dsp_cmd, osd_win_offset_x[2]);
	iav_dsp(dsp_cmd, osd_win_offset_y[2]);
	iav_dsp(dsp_cmd, osd_win_w[2]);
	iav_dsp(dsp_cmd, osd_win_h[2]);
#endif

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

int iav_overlay_insert_ex(iav_context_t *context,
	struct overlay_insert_ex_s __user *arg)
{
	overlay_insert_ex_t info;
	int stream = -1;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (!is_iav_state_prev_or_enc()) {
		iav_error("Invalid IAV state, must be in preview or encoding.\n");
		return -EPERM;
	}

	//check user space addr mapping.
	if (context->overlay.user_start == NULL) {
		iav_error("overlay/alphamask not mapped\n");
		return -EAGAIN;
	}

	if (get_single_stream_num(info.id, &stream) < 0) {
		return -EINVAL;
	}
	// fix me before encode size setup
//	if (check_encode_format_limit(info.id) < 0) {
//		iav_error("Please set encode format before overlay configuration.\n");
//		return -EINVAL;
//	}

	//check overlay insert area
	if (check_overlay_insert_area(context, &info, stream) < 0) {
		iav_error("Overlay insert area format error!\n");
		return -EINVAL;
	}

	// fix me for encode size setup
//	if (encode_size_setup_ex(context, stream) < 0) {
//		iav_error("Failed to set encode format ex.\n");
//		return -EINVAL;
//	}

	G_overlay_insert[stream] = info;
	cmd_set_overlay_insert(context, stream);

	return 0;
}

int iav_get_overlay_insert_ex(iav_context_t * context,
	struct overlay_insert_ex_s __user * arg)
{
	int stream;
	overlay_insert_ex_t info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (get_single_stream_num(info.id, &stream) < 0) {
		iav_error("Cannot get with multi stream id \n");
		return -EINVAL;
	}
	info = G_overlay_insert[stream];

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int get_stream_overlay_param(overlay_insert_ex_t* param)
{
	int stream_id;
	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	*param = G_overlay_insert[stream_id];
	return 0;
}

int cfg_stream_overlay_param(iav_context_t* context,
	overlay_insert_ex_t* param)
{
	int stream_id;

	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	if (check_overlay_insert_area(context, param, stream_id) < 0) {
		return -1;
	}
	G_overlay_insert[stream_id] = *param;
	return 0;
}

