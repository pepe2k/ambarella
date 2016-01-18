/*
 * iav_warp.c
 *
 * History:
 *	2012/10/10 - [Jian Tang] Created file
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
#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_mem_perf.h"
#include "iav_pts.h"
#include "iav_encode.h"
#include "iav_capture.h"
#include "iav_warp.h"

#define	RID_MAP_ALL						(0xFFFFFFFF)
#define	YUV_WARP_TOGGLE_NUM				(5)
#define	TABLE_NUM_PER_AREA				(2)
#define	YUV_WARP_DEFAUL_AREA_NUM		(4)

typedef struct iav_warp_ar_ex_s {
	u32 num[MAX_NUM_WARP_AREAS];
	u32 den[MAX_NUM_WARP_AREAS];
} iav_warp_ar_ex_t;

typedef struct iav_warp_limit_ex_s {
	u8	max_warp_regions;
	u8	reserved[3];
} iav_warp_limit_ex_t;

static iav_warp_limit_ex_t G_warp_limit[IAV_CHIP_ID_S2_TOTAL_NUM] =
{
	[IAV_CHIP_ID_S2_33] = {
		.max_warp_regions = MIN_WARP_REGIONS_NUM,
	},
	[IAV_CHIP_ID_S2_55] = {
		.max_warp_regions = MIN_WARP_REGIONS_NUM,
	},
	[IAV_CHIP_ID_S2_66] = {
		.max_warp_regions = MAX_WARP_REGIONS_NUM,
	},
	[IAV_CHIP_ID_S2_88] = {
		.max_warp_regions = MAX_WARP_REGIONS_NUM,
	},
	[IAV_CHIP_ID_S2_99] = {
		.max_warp_regions = MAX_WARP_REGIONS_NUM,
	},
	[IAV_CHIP_ID_S2_22] = {
		.max_warp_regions = MEDIUM_WARP_REGIONS_NUM,
	},
};

static s8 G_user_rid_map[MAX_NUM_WARP_AREAS];
static u32 G_user_wp_num = 0;
static u32 G_user_wp_num_prev = 0;
static u8 * G_yuv_table_addr = NULL;
static int G_yuv_table_index;
static u32 G_yuv_table_size;
static iav_warp_ar_ex_t G_warp_ar[IAV_MAX_SOURCE_BUFFER_NUM];

static iav_warp_dptz_ex_t G_prev_a_regions;
static iav_warp_dptz_ex_t G_prev_c_regions;

iav_warp_control_ex_t G_warp_control;
static iav_warp_control_ex_t G_default_yuv_warp_cfg;


static inline u8* get_yuv_table_kernel_address(int index, int area_id)
{
	return (G_yuv_table_addr + TABLE_NUM_PER_AREA * (MAX_NUM_WARP_AREAS * index
		+ area_id) * G_yuv_table_size);
}

/******************************************
 *
 *	DSP command functions
 *
 ******************************************/

// 0x0800001E
static void cmd_region_warp_control_ex(int active_id,
	int region_id, u32 num_of_used, iav_warp_vector_ex_t * area)
{
	u8* kernel_addr = NULL, *addr = NULL;
	iav_region_dptz_ex_t * dptz = NULL;
	VCAP_SET_REGION_WARP_CONTROL_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_SET_REGION_WARP_CONTROL;

	dsp_cmd.region_id = active_id;
	dsp_cmd.num_regions = num_of_used;
	dsp_cmd.input_win_w = area->input.width;
	dsp_cmd.input_win_h = area->input.height;
	dsp_cmd.input_win_offset_x = area->input.x;
	dsp_cmd.input_win_offset_y = area->input.y;
	dsp_cmd.output_win_w = area->output.width;
	dsp_cmd.output_win_h = area->output.height;
	dsp_cmd.output_win_offset_x = area->output.x;
	dsp_cmd.output_win_offset_y = area->output.y;

	if (area->dup) {
		/* Warp copy */
		dsp_cmd.rotate_flip_input = 0;
		dsp_cmd.warp_copy_enabled = 1;
	} else {
		/* Warp transform */
		dsp_cmd.rotate_flip_input = area->rotate_flip;
		dsp_cmd.warp_copy_enabled = 0;

		kernel_addr = get_yuv_table_kernel_address(G_yuv_table_index,
			region_id);
		if (area->hor_map.enable) {
			dsp_cmd.h_warp_enabled = area->hor_map.enable;
			dsp_cmd.h_warp_grid_w = area->hor_map.output_grid_col;
			dsp_cmd.h_warp_grid_h = area->hor_map.output_grid_row;
			dsp_cmd.h_warp_h_grid_spacing_exponent =
				area->hor_map.horizontal_spacing;
			dsp_cmd.h_warp_v_grid_spacing_exponent = area->hor_map.vertical_spacing;
			addr = kernel_addr;
			clean_cache_aligned(addr, G_yuv_table_size);
			dsp_cmd.h_warp_grid_addr = VIRT_TO_DSP(addr);
		} else {
			dsp_cmd.h_warp_bypass = is_hwarp_bypass_enabled() &&
				(area->input.width == area->output.width) &&
				(area->input.height <= area->output.height) &&
				(!area->rotate_flip);
		}

		if (area->ver_map.enable) {
			dsp_cmd.v_warp_enabled = area->ver_map.enable;
			dsp_cmd.v_warp_grid_w = area->ver_map.output_grid_col;
			dsp_cmd.v_warp_grid_h = area->ver_map.output_grid_row;
			dsp_cmd.v_warp_h_grid_spacing_exponent =
				area->ver_map.horizontal_spacing;
			dsp_cmd.v_warp_v_grid_spacing_exponent = area->ver_map.vertical_spacing;
			addr = kernel_addr + G_yuv_table_size;
			clean_cache_aligned(addr, G_yuv_table_size);
			dsp_cmd.v_warp_grid_addr = VIRT_TO_DSP(addr);
		}
	}

	if (!is_buf_unwarped(IAV_ENCODE_SOURCE_FOURTH_BUFFER) &&
			!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		dptz = &G_prev_a_regions.dptz[region_id];
		dsp_cmd.prev_a_input_offset_x = dptz->input.x;
		dsp_cmd.prev_a_input_offset_y = dptz->input.y;
		dsp_cmd.prev_a_input_w = dptz->input.width;
		dsp_cmd.prev_a_input_h = dptz->input.height;
		dsp_cmd.prev_a_output_offset_x = dptz->output.x;
		dsp_cmd.prev_a_output_offset_y = dptz->output.y;
		dsp_cmd.prev_a_output_w = dptz->output.width;
		dsp_cmd.prev_a_output_h = dptz->output.height;
	}

	if (!is_buf_unwarped(IAV_ENCODE_SOURCE_SECOND_BUFFER) &&
			!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		dptz = &G_prev_c_regions.dptz[region_id];
		dsp_cmd.prev_c_input_offset_x = dptz->input.x;
		dsp_cmd.prev_c_input_offset_y = dptz->input.y;
		dsp_cmd.prev_c_input_w = dptz->input.width;
		dsp_cmd.prev_c_input_h = dptz->input.height;
		dsp_cmd.prev_c_output_offset_x = dptz->output.x;
		dsp_cmd.prev_c_output_offset_y = dptz->output.y;
		dsp_cmd.prev_c_output_w = dptz->output.width;
		dsp_cmd.prev_c_output_h = dptz->output.height;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, region_id);
	iav_dsp(dsp_cmd, num_regions);
	iav_dsp(dsp_cmd, input_win_w);
	iav_dsp(dsp_cmd, input_win_h);
	iav_dsp(dsp_cmd, input_win_offset_x);
	iav_dsp(dsp_cmd, input_win_offset_y);
	iav_dsp(dsp_cmd, output_win_w);
	iav_dsp(dsp_cmd, output_win_h);
	iav_dsp(dsp_cmd, output_win_offset_x);
	iav_dsp(dsp_cmd, output_win_offset_y);
	iav_dsp(dsp_cmd, rotate_flip_input);
	iav_dsp(dsp_cmd, h_warp_bypass);
	iav_dsp(dsp_cmd, warp_copy_enabled);
	if (area->hor_map.enable) {
		iav_dsp(dsp_cmd, h_warp_enabled);
		iav_dsp(dsp_cmd, h_warp_grid_w);
		iav_dsp(dsp_cmd, h_warp_grid_h);
		iav_dsp(dsp_cmd, h_warp_h_grid_spacing_exponent);
		iav_dsp(dsp_cmd, h_warp_v_grid_spacing_exponent);
		iav_dsp_hex(dsp_cmd, h_warp_grid_addr);
	}
	if (area->ver_map.enable) {
		iav_dsp(dsp_cmd, v_warp_enabled);
		iav_dsp(dsp_cmd, v_warp_grid_w);
		iav_dsp(dsp_cmd, v_warp_grid_h);
		iav_dsp(dsp_cmd, v_warp_h_grid_spacing_exponent);
		iav_dsp(dsp_cmd, v_warp_v_grid_spacing_exponent);
		iav_dsp_hex(dsp_cmd, v_warp_grid_addr);
	}
	if (!is_buf_unwarped(IAV_ENCODE_SOURCE_FOURTH_BUFFER) &&
			!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		iav_dsp(dsp_cmd, prev_a_input_offset_x);
		iav_dsp(dsp_cmd, prev_a_input_offset_y);
		iav_dsp(dsp_cmd, prev_a_input_w);
		iav_dsp(dsp_cmd, prev_a_input_h);
		iav_dsp(dsp_cmd, prev_a_output_offset_x);
		iav_dsp(dsp_cmd, prev_a_output_offset_y);
		iav_dsp(dsp_cmd, prev_a_output_w);
		iav_dsp(dsp_cmd, prev_a_output_h);
	}
	if (!is_buf_unwarped(IAV_ENCODE_SOURCE_SECOND_BUFFER) &&
			!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		iav_dsp(dsp_cmd, prev_c_input_offset_x);
		iav_dsp(dsp_cmd, prev_c_input_offset_y);
		iav_dsp(dsp_cmd, prev_c_input_w);
		iav_dsp(dsp_cmd, prev_c_input_h);
		iav_dsp(dsp_cmd, prev_c_output_offset_x);
		iav_dsp(dsp_cmd, prev_c_output_offset_y);
		iav_dsp(dsp_cmd, prev_c_output_w);
		iav_dsp(dsp_cmd, prev_c_output_h);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

/******************************************
 *
 *	Internal helper functions
 *
 ******************************************/

static int init_yuv_warp_param(void)
{
	int i, buf;
	iav_warp_ar_ex_t * ar = NULL;

	memset(&G_prev_c_regions, 0, sizeof(G_prev_c_regions));
	memset(&G_prev_a_regions, 0, sizeof(G_prev_a_regions));
	memset(&G_warp_ar, 0, sizeof(G_warp_ar));
	memset(&G_warp_control, 0, sizeof(iav_warp_control_ex_t));
	G_user_wp_num = 0;
	G_user_wp_num_prev = 0;

	G_prev_c_regions.buffer_id = IAV_ENCODE_SOURCE_SECOND_BUFFER;
	G_prev_a_regions.buffer_id = IAV_ENCODE_SOURCE_FOURTH_BUFFER;
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		for (buf = IAV_ENCODE_SOURCE_BUFFER_FIRST;
				buf < IAV_ENCODE_SOURCE_BUFFER_LAST; ++buf) {
			ar = &G_warp_ar[buf];
			ar->num[i] = 1;
			ar->den[i] = 1;
		}
	}

	return 0;
}

static int prepare_default_warp_param(void)
{
	int i, area_num, max_input_w, max_output_w;
	u16 input_w, input_h, output_w, output_h;
	iav_warp_vector_ex_t * area = NULL;
	iav_reso_ex_t * main_size = &G_iav_obj.source_buffer[0].size;

	area_num = YUV_WARP_DEFAUL_AREA_NUM;
	do {
		input_w = G_cap_pre_main.size.width / area_num;
		output_w = main_size->width / area_num;
		max_input_w = G_iav_obj.system_resource->max_warped_region_input_width;
		max_output_w = G_iav_obj.system_resource->max_warped_region_output_width;
		if (input_w > max_input_w) {
			input_w = max_input_w;
			iav_printk("Default input region width %d is larger than limit %d.\n",
				input_w, max_input_w);
		}
		if (output_w > max_output_w) {
			output_w = max_output_w;
			iav_printk("Default output region width %d is larger than limit %d.\n",
				output_w, max_output_w);
		}
		if ((input_w >= MIN_REGION_WIDTH_IN_WARP) &&
				(output_w >= MIN_REGION_WIDTH_IN_WARP)) {
			/* Find proper area num for input width & output width, exit */
			break;
		}
		area_num >>= 1;
		if (area_num == 0) {
			iav_error("Too small pre main width [%d] or main width [%d].\n",
				G_cap_pre_main.size.width, main_size->width);
			return -1;
		}
	} while (1);

	input_h = G_cap_pre_main.size.height;
	output_h = main_size->height;
	memset(&G_default_yuv_warp_cfg, 0, sizeof(iav_warp_control_ex_t));
	for (i = 0; i < area_num; ++i) {
		area = &G_default_yuv_warp_cfg.area[i];
		area->enable = 1;
		area->input.width = input_w;
		area->input.height = input_h;
		area->input.x = i * input_w;
		area->output.width = output_w;
		area->output.height = output_h;
		area->output.x = i * output_w;
	}

	return 0;
}

static int copy_yuv_table_from_user(iav_warp_control_ex_t * param, int index)
{
	int i;
	u8* kernel_hor_addr, *kernel_ver_addr;
	iav_warp_vector_ex_t * area;

	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &param->area[i];
		if (area->enable) {
			kernel_hor_addr = get_yuv_table_kernel_address(index, i);
			kernel_ver_addr = kernel_hor_addr + G_yuv_table_size;
			if (area->hor_map.enable && area->hor_map.addr) {
				if (copy_from_user((s16 *) kernel_hor_addr,
				    (s16 *) area->hor_map.addr, G_yuv_table_size)) {
					iav_error("Failed to copy horizontal warp table from user!\n");
					return -1;
				}
			}
			if (area->ver_map.enable && area->ver_map.addr) {
				if (copy_from_user((s16 *) kernel_ver_addr,
				    (s16 *) area->ver_map.addr, G_yuv_table_size)) {
					iav_error("Failed to copy vertical warp table from user!\n");
					return -1;
				}
			}
		}
	}
	return 0;
}

static int copy_yuv_table_to_user(iav_warp_control_ex_t * param)
{
	int i;
	u32 user_addr;
	u8* kernel_hor_addr, *kernel_ver_addr;
	iav_warp_vector_ex_t* area;

	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &G_warp_control.area[i];
		if (unlikely(area->dup)) {
			/* Skip when the region is "Warp Copy" type */
			continue;
		}
		kernel_hor_addr = get_yuv_table_kernel_address(G_yuv_table_index, i);
		kernel_ver_addr = kernel_hor_addr + G_yuv_table_size;
		user_addr = param->area[i].hor_map.addr;
		if (user_addr && area->hor_map.enable) {
			if (copy_to_user((void *) user_addr, (void *) kernel_hor_addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy horizontal warp table to user!\n");
				return -1;
			}
		}
		user_addr = param->area[i].ver_map.addr;
		if (user_addr && area->ver_map.enable) {
			if (copy_to_user((void *) user_addr, (void *) kernel_ver_addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy vertical warp table to user!\n");
				return -1;
			}
		}
	}
	return 0;
}

static void copy_warp_map_param(iav_warp_map_ex_t* dst, iav_warp_map_ex_t* src)
{
	dst->enable = src->enable;
	dst->output_grid_col = src->output_grid_col;
	dst->output_grid_row = src->output_grid_row;
	dst->horizontal_spacing = src->horizontal_spacing;
	dst->vertical_spacing = src->vertical_spacing;
}

static inline int check_multi_warp_control_state(void)
{
	if (!is_warp_mode()) {
		iav_error("Please enter 'Warp mode' first!\n");
		return -1;
	}
	return 0;
}

static inline int check_multi_warp_system_config(u32 active_regions, u32 addr)
{
	if (unlikely(G_iav_obj.dsp_chip_id == IAV_CHIP_ID_S2_UNKNOWN)) {
		G_iav_obj.dsp_chip_id = dsp_get_chip_id();
	}
	if (active_regions > G_warp_limit[G_iav_obj.dsp_chip_id].max_warp_regions) {
		iav_error("Too many warp regions [%d].\n", active_regions);
		return -1;
	}
	if (iav_check_system_config(get_enc_mode(), addr) < 0) {
		return -1;
	}
	return 0;
}

static int check_warp_transform(u32 rid, iav_warp_vector_ex_t * area,
	u16 max_in_w, u16 max_out_w, u16 out_w, u16 out_h)
{
	char msg[32];

	iav_no_check();

	if ((area->input.width & 0x3) || (area->output.width & 0x3)) {
		iav_error("Warp region [%d] input / output width [%d/%d] must be "
			"multiple of 4.\n", rid, area->input.width, area->output.width);
		return -1;
	}
	if ((area->input.height & 0x1) || (area->output.height & 0x1)) {
		iav_error("Warp region [%d] input / output height [%d/%d] must be "
			"multiple of 2.\n", rid, area->input.height, area->output.height);
		return -1;
	}
	if ((area->input.width > max_in_w) || (area->output.width > max_out_w)) {
		iav_error("Warp region [%d] input /output width [%d/%d] cannot be "
			"larger than system limit [%d/%d].\n", rid, area->input.width,
			area->output.width, max_in_w, max_out_w);
		return -1;
	}
	if (area->output.width < MIN_REGION_WIDTH_IN_WARP) {
		iav_error("Warp region [%d] output width [%d] cannot be smaller "
			"than system limit [%d].\n", rid, area->output.width,
			MIN_REGION_WIDTH_IN_WARP);
		return -1;
	}
	if ((area->output.x + area->output.width > out_w) ||
			(area->output.y + area->output.height > out_h)) {
		iav_error("Warp region [%d] output %dx%d with offset %dx%d exceed "
			"main source buffer %dx%d.\n", rid, area->output.width,
			area->output.height, area->output.x, area->output.y,
			out_w, out_h);
		return -1;
	}
	if (!area->input.width || !area->input.height ||
			!area->output.width || !area->output.height) {
		iav_error("Active region [%d] input [%dx%d] output [%dx%d] cannot "
			"be 0!\n", rid, area->input.width, area->input.height,
			area->output.width, area->output.height);
		return -1;
	}
	if (area->hor_map.enable && !area->hor_map.addr) {
		iav_error("Warp region [%d] horizontal address cannot be empty if "
			"enabled.\n", rid);
		return -1;
	}
	if (area->ver_map.enable && !area->ver_map.addr) {
		iav_error("Warp region [%d] vertical address cannot be empty if "
			"enabled.\n", rid);
		return -1;
	}
	if (area->ver_map.enable &&
			(area->ver_map.horizontal_spacing == GRID_SPACING_PIXEL_16)) {
		iav_error("Active region [%d] vertical warp vector horizontal spacing"
			" must be larger than 16 pixel!\n", rid);
		return -1;
	}

	/* Check the zoom factor between warp input and output.
	 * Preview B is used to do scaling.
	 */
	sprintf(msg, "Warp region [%d]", rid);
	if (check_zoom_property(IAV_ENCODE_SOURCE_THIRD_BUFFER,
			area->input.width, area->input.height,
			area->output.width, area->output.height, msg) < 0) {
		return -1;
	}

	return 0;
}

static int check_multi_warp_param(iav_warp_control_ex_t * warp)
{
	int i, active_regions;
	u16 output_w, output_h, max_input_w, max_output_w;
	iav_region_dptz_ex_t * dptz = NULL;
	iav_warp_vector_ex_t * area = NULL;
	iav_source_buffer_ex_t * src_buf = G_iav_obj.source_buffer;

	iav_no_check();

	output_w = src_buf[0].size.width;
	output_h = src_buf[0].size.height;
	max_input_w = G_iav_obj.system_resource->max_warped_region_input_width;
	max_output_w = G_iav_obj.system_resource->max_warped_region_output_width;

	if (warp->keep_dptz[IAV_ENCODE_SOURCE_MAIN_BUFFER] ||
		warp->keep_dptz[IAV_ENCODE_SOURCE_THIRD_BUFFER]) {
		iav_error("Warp keep DPTZ is not supported for main / 3rd buffer!\n");
		return -1;
	}

	for (i = 0, active_regions = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &warp->area[i];
		if (area->enable == 0) {
			/* This region is NOT actived, skip and read next. */
			continue;
		}
		if (check_warp_transform(i, area, max_input_w, max_output_w,
				output_w, output_h) < 0) {
			return -1;
		}
		if (warp->keep_dptz[IAV_ENCODE_SOURCE_FOURTH_BUFFER]) {
			dptz = &G_prev_a_regions.dptz[i];
			if ((dptz->input.width + dptz->input.x > area->output.width) ||
					(dptz->input.height + dptz->input.y > area->output.height)) {
				iav_error("Keep DPTZ - Reg [%d]: 4th buffer input [%dx%d] with"
					" offset %dx%d is out out area [%dx%d] in main buffer.\n",
					i, dptz->input.width, dptz->input.height, dptz->input.x,
					dptz->input.y,
					area->output.width, area->output.height);
				return -1;
			}
		}
		if (warp->keep_dptz[IAV_ENCODE_SOURCE_SECOND_BUFFER]) {
			dptz = &G_prev_c_regions.dptz[i];
			if ((dptz->input.width + dptz->input.x > area->output.width) ||
					(dptz->input.height + dptz->input.y > area->output.height)) {
				iav_error("Keep DPTZ - Reg [%d]: 2nd buffer input [%dx%d] with"
					" offset %dx%d is out out area [%dx%d] in main buffer.\n",
					i, dptz->input.width, dptz->input.height, dptz->input.x,
					dptz->input.y,
					area->output.width, area->output.height);
				return -1;
			}
		}
		++active_regions;
	}

	if (check_multi_warp_system_config(active_regions, (u32)warp) < 0) {
		return -1;
	}

	return 0;
}

static int check_warp_region_dptz_param(iav_warp_dptz_ex_t *warp_dptz)
{
	int i;
	char msg[32];
	u16 output_win_w, output_win_h, buffer_id;
	iav_warp_vector_ex_t * area = NULL;
	iav_region_dptz_ex_t * dptz = NULL;
	iav_source_buffer_ex_t * src_buf = G_iav_obj.source_buffer;

	iav_no_check();

	/* Check buffer id is valid */
	buffer_id = warp_dptz->buffer_id;
	if ((buffer_id == IAV_ENCODE_SOURCE_SECOND_BUFFER) ||
			(buffer_id == IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		output_win_w = src_buf[buffer_id].size.width;
		output_win_h = src_buf[buffer_id].size.height;
	} else {
		iav_error("Invalid buffer id [%d] for warp region DPTZ! "
			"Only support for 2nd and 4th source buffer!\n", buffer_id);
		return -1;
	}
	if (is_buf_unwarped(buffer_id)) {
		iav_error("Buffer [%d] comes from unwarp image! NO DPTZ support!\n",
			buffer_id);
		return -1;
	}

	/* Check region id is valid */
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &G_warp_control.area[i];
		if (likely(area->enable)) {
			dptz = &warp_dptz->dptz[i];
			/* Check if the input and output window is valid */
			if ((dptz->output.width & 0x3) || (dptz->output.x & 0x3)) {
				iav_error("Region [%d] output width [%d] and offset "
					"x [%d] must be multiple of 4.\n", i, dptz->output.width,
					dptz->output.x);
				return -1;
			}
			if ((dptz->output.height & 0x1) || (dptz->output.y & 0x1)) {
				iav_error("Region [%d] output height [%d] and "
					"offset y [%d] must be even.\n", i, dptz->output.height,
					dptz->output.y);
				return -1;
			}
			if ((dptz->input.width > area->output.width) ||
					(dptz->input.height > area->output.height) ||
					(dptz->input.x + dptz->input.width > area->output.width) ||
					(dptz->input.y + dptz->input.height > area->output.height)) {
				iav_error("Region [%d] input %dx%d with offset %dx%d exceeds the "
					"capture window %dx%d with offset %dx%d in main buffer.\n",
					i, dptz->input.width, dptz->input.height,
					dptz->input.x, dptz->input.y, area->output.width,
					area->output.height, area->output.x, area->output.y);
				return -1;
			}
			if ((dptz->output.width > output_win_w) ||
					(dptz->output.height > output_win_h) ||
					(dptz->output.width + dptz->output.x > output_win_w) ||
					(dptz->output.height + dptz->output.y > output_win_h)) {
				iav_error("Region [%d] output %dx%d with offset %dx%d exceeds"
					" the buffer [%d] size %dx%d.\n", i,
					dptz->output.width, dptz->output.height, dptz->output.x,
					dptz->output.y, buffer_id, output_win_w, output_win_h);
				return -1;
			}
			if (dptz->output.width && dptz->output.height) {
				sprintf(msg, "Buffer %d Region %d", buffer_id, i);
				if (check_zoom_property(buffer_id,
						dptz->input.width, dptz->input.height,
						dptz->output.width, dptz->output.height, msg) < 0) {
					iav_error("Failed to set warp DPTZ param!\n");
					return -1;
				}
			}
		}
	}

	return 0;
}

static int check_warp_proc_apply(u32 rid_map)
{
	int i;
	iav_region_dptz_ex_t * dptz = NULL;
	iav_warp_vector_ex_t * area = NULL;
	iav_warp_control_ex_t * warp = &G_warp_control;

	iav_no_check();

	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		if (!(rid_map & (1 << i))) {
			continue;
		}
		area = &warp->area[i];
		if (warp->keep_dptz[IAV_ENCODE_SOURCE_FOURTH_BUFFER]) {
			dptz = &G_prev_a_regions.dptz[i];
			if ((dptz->input.width + dptz->input.x > area->output.width) ||
					(dptz->input.height + dptz->input.y > area->output.height)) {
				iav_error("Keep DPTZ - Reg [%d]: 4th buffer input [%dx%d] with"
					" offset %dx%d is out out area [%dx%d] in main buffer.\n",
					i, dptz->input.width, dptz->input.height, dptz->input.x,
					dptz->input.y,
					area->output.width, area->output.height);
				return -1;
			}
		}
		if (warp->keep_dptz[IAV_ENCODE_SOURCE_SECOND_BUFFER]) {
			dptz = &G_prev_c_regions.dptz[i];
			if ((dptz->input.width + dptz->input.x > area->output.width) ||
					(dptz->input.height + dptz->input.y > area->output.height)) {
				iav_error("Keep DPTZ - Reg [%d]: 2nd buffer input [%dx%d] with"
					" offset %dx%d is out out area [%dx%d] in main buffer.\n",
					i, dptz->input.width, dptz->input.height, dptz->input.x,
					dptz->input.y,
					area->output.width, area->output.height);
				return -1;
			}
		}
	}

	if (check_multi_warp_system_config(G_user_wp_num,
			(u32)&G_warp_control) < 0)
		return -1;

	return 0;
}

static inline int update_region_aspect_ratio(int buffer_id,
	u16 region_id, iav_rect_ex_t * input, iav_rect_ex_t * output)
{
	int gcd, num, den;
	iav_warp_ar_ex_t * ar = NULL;
	if (!input->width || !input->height) {
		num = 1;
		den = 1;
	} else {
		num = input->width;
		den = input->height;
	}
	if (output->width && output->height) {
		num *= output->height;
		den *= output->width;
	}
	gcd = get_gcd(num, den);
	ar = &G_warp_ar[buffer_id];
	ar->num[region_id] = num / gcd;
	ar->den[region_id] = den / gcd;
	return 0;
}

static int prepare_default_warp_dptz_param(iav_buffer_id_t update_buffer_id,
	u32 flag)
{
	int i, warped_buffer_id;
	u32 main_w, main_h;
	u16 buf_a_w, buf_a_h, buf_c_w, buf_c_h;
	iav_warp_control_ex_t* warp = NULL;
	iav_region_dptz_ex_t * dptz = NULL;
	iav_warp_vector_ex_t * area = NULL;
	struct amba_video_info * video_info = NULL;
	iav_reso_ex_t * buffer = NULL;
	iav_source_buffer_ex_t * src_buf = G_iav_obj.source_buffer;

	/* VOUT swap is enabled by default in dewarp mode */
	video_info = &G_iav_info.pvoutinfo[1]->video_info;
	buffer = &src_buf[IAV_ENCODE_SOURCE_FOURTH_BUFFER].size;

	if (is_buf_type_enc(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		if (src_buf[IAV_ENCODE_SOURCE_FOURTH_BUFFER].enc_stop) {
			buf_a_w = 0;
			buf_a_h = 0;
		} else {
			buf_a_w = buffer->width;
			buf_a_h = buffer->height;
		}
	} else if (is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		if (video_info->rotate) {
			buf_a_w = video_info->height;
			buf_a_h = video_info->width;
		} else {
			buf_a_w = video_info->width;
			buf_a_h = video_info->height;
		}
	} else {
		buf_a_w = 0;
		buf_a_h = 0;
	}

	buffer = &src_buf[IAV_ENCODE_SOURCE_SECOND_BUFFER].size;
	if (is_buf_type_enc(IAV_ENCODE_SOURCE_SECOND_BUFFER) &&
		!src_buf[IAV_ENCODE_SOURCE_SECOND_BUFFER].enc_stop) {
		buf_c_w = buffer->width;
		buf_c_h = buffer->height;
	} else {
		buf_c_w = 0;
		buf_c_h = 0;
	}

	warped_buffer_id = update_buffer_id;

	/* AR for unwarped buffer:
	 * No transform from dewarp engine. Only the scale factor affect the AR.
	 */
	main_w = G_cap_pre_main.size.width;
	main_h = G_cap_pre_main.size.height;
	if (is_buf_unwarped(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		warped_buffer_id &= ~IAV_4TH_BUFFER;
		dptz = &G_prev_a_regions.dptz[0];
		dptz->input.width = main_w;
		dptz->input.height = main_h;
		dptz->input.x = dptz->input.y = 0;
		dptz->output.width = buf_a_w;
		dptz->output.height = buf_a_h;
		dptz->output.x = dptz->output.y = 0;
		if (buf_a_w && buf_a_h) {
			update_region_aspect_ratio(IAV_ENCODE_SOURCE_FOURTH_BUFFER,
				0, &dptz->input, &dptz->output);
		}
	}
	if (is_buf_unwarped(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		warped_buffer_id &= ~IAV_2ND_BUFFER;
		dptz = &G_prev_c_regions.dptz[0];
		dptz->input.width = main_w;
		dptz->input.height = main_h;
		dptz->input.x = dptz->input.y = 0;
		dptz->output.width = buf_c_w;
		dptz->output.height = buf_c_h;
		dptz->output.x = dptz->output.y = 0;
		if (buf_c_w && buf_c_h) {
			update_region_aspect_ratio(IAV_ENCODE_SOURCE_SECOND_BUFFER,
				0, &dptz->input, &dptz->output);
		}
	}

	/* Re-calculate the AR for warped buffer only when "keep_dptz" flag
	 * is disabled.
	 * The dewarp transform always output AR of 1:1 in main source buffer.
	 * The scale factor between main buffer and 2nd / 4th buffer affect the AR.
	 */
	main_w = src_buf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.width;
	main_h = src_buf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size.height;
	warp = G_user_wp_num ? &G_warp_control : &G_default_yuv_warp_cfg;
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &warp->area[i];

		if (area->enable == 0) {
			continue;
		}
		if ((warped_buffer_id & IAV_4TH_BUFFER) &&
			!warp->keep_dptz[IAV_ENCODE_SOURCE_FOURTH_BUFFER]) {
			dptz = &G_prev_a_regions.dptz[i];
			switch (flag) {
			case IAV_BUF_WARP_CLEAR:
				dptz->output.width = 0;
				dptz->output.height = 0;
				dptz->output.x = 0;
				dptz->output.y = 0;
				break;
			case IAV_BUF_WARP_SET:
				dptz->input.width = area->output.width;
				dptz->input.height = area->output.height;
				dptz->input.x = dptz->input.y = 0;
				dptz->output.x = area->output.x * buf_a_w / main_w;
				if (dptz->output.x > buf_a_w) {
					dptz->output.x = buf_a_w;
				}
				dptz->output.width = area->output.width * buf_a_w / main_w;
				if (dptz->output.x + dptz->output.width > buf_a_w) {
					dptz->output.width = buf_a_w - dptz->output.x;
				}
				dptz->output.y = area->output.y * buf_a_h / main_h;
				if (dptz->output.y > buf_a_h) {
					dptz->output.y = buf_a_h;
				}
				dptz->output.height = area->output.height * buf_a_h / main_h;
				if (dptz->output.y + dptz->output.height > buf_a_h) {
					dptz->output.height = buf_a_h - dptz->output.y;
				}
				if ((dptz->output.width == 0) ||(dptz->output.height == 0)) {
					memset(&dptz->input, 0, sizeof(iav_rect_ex_t));
					memset(&dptz->output, 0, sizeof(iav_rect_ex_t));
				} else {
					update_region_aspect_ratio(
						IAV_ENCODE_SOURCE_FOURTH_BUFFER,
						i, &dptz->input, &dptz->output);
				}
				break;
			default:
				break;
			}
		}

		if ((warped_buffer_id & IAV_2ND_BUFFER) &&
			!warp->keep_dptz[IAV_ENCODE_SOURCE_SECOND_BUFFER]) {
			dptz = &G_prev_c_regions.dptz[i];
			switch (flag) {
			case IAV_BUF_WARP_CLEAR:
				dptz->output.width = 0;
				dptz->output.height = 0;
				dptz->output.x = 0;
				dptz->output.y = 0;
				break;
			case IAV_BUF_WARP_SET:
				dptz->input.width = area->output.width;
				dptz->input.height = area->output.height;
				dptz->input.x = dptz->input.y = 0;
				dptz->output.x = area->output.x * buf_c_w / main_w;
				if (dptz->output.x > buf_c_w) {
					dptz->output.x = buf_c_w;
				}
				dptz->output.width = area->output.width * buf_c_w / main_w;
				if (dptz->output.x + dptz->output.width > buf_c_w) {
					dptz->output.width = buf_c_w - dptz->output.x;
				}
				dptz->output.y = area->output.y * buf_c_h / main_h;
				if (dptz->output.y > buf_c_h) {
					dptz->output.y = buf_c_h;
				}
				dptz->output.height = area->output.height * buf_c_h / main_h;
				if (dptz->output.y + dptz->output.height > buf_c_h) {
					dptz->output.height = buf_c_h - dptz->output.y;
				}
				if ((dptz->output.width == 0) || (dptz->output.height == 0)) {
					memset(&dptz->input, 0, sizeof(iav_rect_ex_t));
					memset(&dptz->output, 0, sizeof(iav_rect_ex_t));
				} else {
					update_region_aspect_ratio(
						IAV_ENCODE_SOURCE_SECOND_BUFFER,
						i, &dptz->input, &dptz->output);
				}
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

static int update_warp_region_dptz_param(iav_warp_dptz_ex_t * warp_dptz)
{
	int i;
	u16 buffer_id;
	iav_warp_vector_ex_t * warp = NULL;
	iav_warp_dptz_ex_t * dst = NULL;
	iav_region_dptz_ex_t * dptz = NULL;

	buffer_id = warp_dptz->buffer_id;
	switch (buffer_id) {
	case IAV_ENCODE_SOURCE_SECOND_BUFFER:
		dst = &G_prev_c_regions;
		break;
	case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
		dst = &G_prev_a_regions;
		break;
	default:
		iav_error("Invalid buffer id [%d] for warp region DPTZ!\n", buffer_id);
		return -1;
	}

	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		warp = &G_warp_control.area[i];
		if (warp->enable) {
			dptz = &dst->dptz[i];
			*dptz = warp_dptz->dptz[i];
			/* Warp region DPTZ affect the AR of this region. */
			update_region_aspect_ratio(buffer_id, i,
				&dptz->input, &dptz->output);
		}
	}

	return 0;
}

static inline int issue_warp_control_cmd(u32 rid_map)
{
	int i, active_rid, active_num, block_cmd_flag;
	iav_warp_vector_ex_t * area = NULL;
	iav_warp_control_ex_t * warp = NULL;

	if (G_user_wp_num == 0) {
		warp = &G_default_yuv_warp_cfg;
		for (i = 0, active_num = 0; i < MAX_NUM_WARP_AREAS; ++i) {
			area = &warp->area[i];
			active_num += area->enable;
		}
	} else {
		warp = &G_warp_control;
		active_num = G_user_wp_num;
	}

	if (rid_map == RID_MAP_ALL) {
		memset(G_user_rid_map, -1, sizeof(G_user_rid_map));
	}

	/* Guarantee the multiple region warp control (0x8000001E)
	 * are issued in the same VSYNC
	 */
	block_cmd_flag = (active_num > 1) && (rid_map & (rid_map - 1));
	if (block_cmd_flag) {
		dsp_start_cmdblk(DSP_CMD_PORT_VCAP);
	}
	active_rid = 0;
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		if (rid_map & (1 << i)) {
			area = &warp->area[i];
			/* Issue warp transform regions first */
			if (area->enable && !area->dup) {
				if (G_user_rid_map[i] < 0) {
					G_user_rid_map[i] = active_rid;
					++active_rid;
				}
				cmd_region_warp_control_ex(G_user_rid_map[i],
					i, active_num, area);
			}
		}
	}
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		if (rid_map & (1 << i)) {
			area = &warp->area[i];
			/* Issue warp copy regions later */
			if (area->enable && area->dup) {
				if (G_user_rid_map[i] < 0) {
					G_user_rid_map[i] = active_rid;
					++active_rid;
				}
				cmd_region_warp_control_ex(G_user_rid_map[i],
					i, active_num, area);
			}
		}
	}
	if (block_cmd_flag) {
		dsp_end_cmdblk(DSP_CMD_PORT_VCAP);
	}

	G_user_wp_num_prev = G_user_wp_num ? active_num : 0;

	return 0;
}

static void update_user_warp_param(int user_regions,
	iav_warp_control_ex_t* user_param)
{
	if (user_regions) {
		G_warp_control = *user_param;
		G_user_wp_num = user_regions;
	} else {
		memset(&G_warp_control, 0, sizeof(iav_warp_control_ex_t));
		G_user_wp_num = 0;
	}
}

static inline int get_warp_transform_param(u32 rid, iav_warp_vector_ex_t *area)
{
	iav_warp_vector_ex_t * src = &G_warp_control.area[rid];
	u8 *ker_hor_addr, *ker_ver_addr;
	u32 user_addr;

	area->enable = src->enable;
	area->rotate_flip = src->rotate_flip;
	area->dup = src->dup;
	area->src_rid = src->src_rid;
	area->input = src->input;
	area->output = src->output;

	if (likely(!src->dup)) {
		copy_warp_map_param(&area->hor_map, &src->hor_map);
		copy_warp_map_param(&area->ver_map, &src->ver_map);

		ker_hor_addr = get_yuv_table_kernel_address(G_yuv_table_index, rid);
		user_addr = area->hor_map.addr;
		if (user_addr && src->hor_map.enable) {
			if (copy_to_user((void *)user_addr, (void *)ker_hor_addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy horizontal warp table to user!\n");
				return -1;
			}
		}
		ker_ver_addr = ker_hor_addr + G_yuv_table_size;
		user_addr = area->ver_map.addr;
		if (user_addr && src->ver_map.enable) {
			if (copy_to_user((void *)user_addr, (void *)ker_ver_addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy vertical warp table to user!\n");
				return -1;
			}
		}
	}

	return 0;
}

static int get_warp_copy_param(u32 rid, iav_warp_copy_t *dup)
{
	iav_warp_vector_ex_t * src = &G_warp_control.area[rid];
	dup->src_rid = src->src_rid;
	dup->dst_x = src->output.x;
	dup->dst_y = src->output.y;
	return 0;
}

static int get_warp_sub_dptz_param(u32 rid, iav_warp_sub_dptz_t *sub)
{
	switch (sub->sub_buf) {
	case IAV_ENCODE_SOURCE_SECOND_BUFFER:
		sub->dptz = G_prev_c_regions.dptz[rid];
		break;
	case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
		sub->dptz = G_prev_a_regions.dptz[rid];
		break;
	default:
		iav_error("Invalid buffer id [%d] in warp sub DPTZ.\n",
			sub->sub_buf);
		return -1;
		break;
	}
	return 0;
}

static int cfg_warp_transform_param(u32 rid, iav_warp_vector_ex_t * area)
{
	int next_index;
	u8 *ker_hor, *ker_ver;
	u16 out_w, out_h, max_in_w, max_out_w;

	if (unlikely(area->dup)) {
		iav_error("Cannot enable [dup] for warp transform op.\n");
		return -EFAULT;
	}

	if (likely(area->enable)) {
		out_w = G_iav_obj.source_buffer[0].size.width;
		out_h = G_iav_obj.source_buffer[0].size.height;
		max_in_w = G_iav_obj.system_resource->max_warped_region_input_width;
		max_out_w = G_iav_obj.system_resource->max_warped_region_output_width;
		if (check_warp_transform(rid, area, max_in_w, max_out_w,
				out_w, out_h) < 0) {
			return -EINVAL;
		}
		next_index = (G_yuv_table_index + 1) % YUV_WARP_TOGGLE_NUM;
		ker_hor = get_yuv_table_kernel_address(next_index, rid);
		ker_ver = ker_hor + G_yuv_table_size;
		if (area->hor_map.enable && area->hor_map.addr) {
			if (copy_from_user((s16 *)ker_hor, (s16 *)area->hor_map.addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy horizontal warp table from user");
				return -EINVAL;
			}
		}
		if (area->ver_map.enable && area->ver_map.addr) {
			if (copy_from_user((s16 *)ker_ver, (s16 *)area->ver_map.addr,
				G_yuv_table_size)) {
				iav_error("Failed to copy horizontal warp table from user");
				return -EINVAL;
			}
		}
		area->dup = 0;
		area->src_rid = 0;
		if (!G_warp_control.area[rid].enable) {
			BUG_ON(G_user_wp_num == MAX_NUM_WARP_AREAS);
			++G_user_wp_num;
		}
		G_warp_control.area[rid] = *area;
	} else {
		if (G_warp_control.area[rid].enable) {
			BUG_ON(G_user_wp_num == 0);
			--G_user_wp_num;
		}
		memset(&G_warp_control.area[rid], 0, sizeof(iav_warp_vector_ex_t));
	}
	return 0;
}

static int cfg_warp_copy_param(u32 rid, iav_warp_copy_t *dup)
{
	iav_warp_vector_ex_t * src = NULL, *dst = NULL;
	u16 out_w, out_h;

	dst = &G_warp_control.area[rid];
	if (likely(dup->enable)) {
		if (dup->src_rid >= MAX_NUM_WARP_AREAS) {
			iav_error("Invalid source region ID [%d] for warp copy.\n",
				dup->src_rid);
			return -EFAULT;
		}
		src = &G_warp_control.area[dup->src_rid];
		out_w = G_iav_obj.source_buffer[0].size.width;
		out_h = G_iav_obj.source_buffer[0].size.height;
		if (!src->enable || src->dup) {
			iav_error("Source region [%d] is not config for warp transform.\n",
				dup->src_rid);
			return -EINVAL;
		}
		if ((src->output.width + dup->dst_x > out_w) ||
				(src->output.height + dup->dst_y > out_h)) {
			iav_error("Warp copy region [%d] output %dx%d with offset %dx%d "
				"exceed main source buffer %dx%d.\n", rid, src->output.width,
				src->output.height, dup->dst_x, dup->dst_y, out_w, out_h);
			return -EINVAL;
		}

		dst->dup = 1;
		dst->src_rid = dup->src_rid;
		dst->input = src->output;
		dst->output.width = src->output.width;
		dst->output.height = src->output.height;
		dst->output.x = dup->dst_x;
		dst->output.y = dup->dst_y;
		memset(&dst->hor_map, 0, sizeof(iav_warp_map_ex_t));
		memset(&dst->ver_map, 0, sizeof(iav_warp_map_ex_t));

		if (!dst->enable) {
			BUG_ON(G_user_wp_num == MAX_NUM_WARP_AREAS);
			++G_user_wp_num;
			dst->enable = 1;
		}
	} else {
		if (dst->enable) {
			BUG_ON(G_user_wp_num == 0);
			--G_user_wp_num;
		}
		memset(dst, 0, sizeof(iav_warp_vector_ex_t));
	}

	return 0;
}

static int cfg_warp_sub_dptz_param(u32 rid, iav_warp_sub_dptz_t *sub)
{
	iav_error("WARP sub DPTZ is not implemented yet.\n");
	return 0;
}

/******************************************
 *
 *	External functions
 *
 ******************************************/

int mem_alloc_yuv_warp(u8 ** ptr, int *alloc_size)
{
	u32 total_size;

	G_yuv_table_addr = NULL;
	G_yuv_table_size = MAX_WARP_TABLE_SIZE * sizeof(s16);
	total_size = ALIGN(YUV_WARP_TOGGLE_NUM * MAX_NUM_WARP_AREAS *
		TABLE_NUM_PER_AREA * G_yuv_table_size, 4 * KB);

	if ((G_yuv_table_addr = kzalloc(total_size, GFP_KERNEL)) == NULL ) {
		iav_error("Not enough memory to allocate WARP table!\n");
		return -1;
	}
	*ptr = G_yuv_table_addr;
	*alloc_size = total_size;
	return 0;
}

int set_default_warp_dptz_param(iav_buffer_id_t update_buffer_id,
	IAV_BUF_WARP_ACTION_FLAG flag)
{
	prepare_default_warp_dptz_param(update_buffer_id, flag);
	issue_warp_control_cmd(RID_MAP_ALL);

	return 0;
}

int set_default_warp_param(void)
{
	update_user_warp_param(0, NULL);
	if (prepare_default_warp_param() < 0) {
		return -1;
	}
	if (check_multi_warp_param(&G_default_yuv_warp_cfg) < 0) {
		return -1;
	}
	prepare_default_warp_dptz_param((IAV_2ND_BUFFER | IAV_4TH_BUFFER),
		IAV_BUF_WARP_SET);

	return 0;
}

int get_aspect_ratio_in_warp_mode(iav_encode_format_ex_t* format,
	u8 * aspect_ratio_idc, u16 * sar_width, u16 * sar_height)
{
	int i, buffer_id, find_region;
	int num, den, gcd;
	iav_warp_vector_ex_t * area = NULL;
	iav_region_dptz_ex_t * dptz = NULL;

	buffer_id = format->source;
	for (i = 0, find_region = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		switch (buffer_id) {
		case IAV_ENCODE_SOURCE_MAIN_BUFFER:
			area = &G_warp_control.area[i];
			if ((format->encode_x >= area->output.x &&
					format->encode_x < area->output.x + area->output.width) &&
				(format->encode_y >= area->output.y &&
					format->encode_y < area->output.y + area->output.height)) {
				find_region = 1;
			}
			break;
		case IAV_ENCODE_SOURCE_SECOND_BUFFER:
			dptz = &G_prev_c_regions.dptz[i];
			if ((format->encode_x >= dptz->output.x &&
					format->encode_x < dptz->output.x + dptz->output.width) &&
				(format->encode_y >= dptz->output.y &&
					format->encode_y < dptz->output.y + dptz->output.height)) {
				find_region = 1;
			}
			break;
		case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
			dptz = &G_prev_a_regions.dptz[i];
			if ((format->encode_x >= dptz->output.x &&
					format->encode_x < dptz->output.x + dptz->output.width) &&
				(format->encode_y >= dptz->output.y &&
					format->encode_y < dptz->output.y + dptz->output.height)) {
				find_region = 1;
			}
			break;
		default:
			break;
		}
		if (find_region) {
			break;
		}
	}
	if (find_region) {
		num = G_warp_ar[buffer_id].num[i];
		den = G_warp_ar[buffer_id].den[i];
	} else {
		/* Stream start point is not in any warp region */
		num = 1;
		den = 1;
	}

	num *= *sar_width;
	den *= *sar_height;

	if (num == den) {
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL;
		*sar_width = 1;
		*sar_height = 1;
	} else {
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_CUSTOM;
		gcd = get_gcd(num, den);
		*sar_width = num / gcd;
		*sar_height = den / gcd;
	}
	return 0;
}

int iav_set_warp_control_ex(iav_context_t * context,
	struct iav_warp_control_ex_s __user * arg)
{
	iav_warp_control_ex_t user_param;
	u32 buffer_id, i;
	int next_index, user_areas = 0;

	if (check_multi_warp_control_state())
		return -EPERM;

	if (copy_from_user(&user_param, arg, sizeof(iav_warp_control_ex_t)))
		return -EFAULT;

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		user_areas += (user_param.area[i].enable != 0);
	}

	if (!user_areas) {
		if (set_default_warp_param() < 0) {
			return -EINVAL;
		}
	} else {
		if (check_multi_warp_param(&user_param) < 0) {
			return -EINVAL;
		}

		// Update user warp param
		next_index = (1 + G_yuv_table_index) % YUV_WARP_TOGGLE_NUM;
		if (copy_yuv_table_from_user(&user_param, next_index) < 0)
			return -EFAULT;
		G_yuv_table_index = next_index;
		update_user_warp_param(user_areas, &user_param);
	}

	// Reset warp dptz param
	buffer_id = 0;
	if (!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		buffer_id |= IAV_2ND_BUFFER;
	}
	if (!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		buffer_id |= IAV_4TH_BUFFER;
	}
	prepare_default_warp_dptz_param(buffer_id, IAV_BUF_WARP_SET);

	issue_warp_control_cmd(RID_MAP_ALL);

	return 0;
}

int iav_get_warp_control_ex(iav_context_t * context,
	struct iav_warp_control_ex_s __user * arg)
{
	int i;
	iav_warp_control_ex_t user_param;
	iav_warp_vector_ex_t *area, *user_area;

	memset(&user_param, 0, sizeof(user_param));

	if (copy_from_user(&user_param, arg, sizeof(iav_warp_control_ex_t)))
		return -EFAULT;

	if (copy_yuv_table_to_user(&user_param) < 0) {
		return -EFAULT;
	}
	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; i++) {
		user_param.keep_dptz[i] = G_warp_control.keep_dptz[i];
	}
	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		user_area = &user_param.area[i];
		area = &G_warp_control.area[i];
		user_area->enable = area->enable;
		user_area->rotate_flip = area->rotate_flip;
		user_area->dup = area->dup;
		user_area->src_rid = area->src_rid;
		user_area->input = area->input;
		user_area->output = area->output;
		copy_warp_map_param(&user_area->hor_map, &area->hor_map);
		copy_warp_map_param(&user_area->ver_map, &area->ver_map);
	}

	return copy_to_user(arg, &user_param, sizeof(user_param)) ? -EFAULT : 0;
}

int iav_set_warp_region_dptz_ex(iav_context_t * context,
	struct iav_warp_dptz_ex_s __user * arg)
{
	iav_warp_dptz_ex_t warp_dptz;

	if (check_multi_warp_control_state())
		return -EPERM;

	if (copy_from_user(&warp_dptz, arg, sizeof(warp_dptz)))
		return -EFAULT;

	if (check_warp_region_dptz_param(&warp_dptz) < 0) {
		iav_error("Invalid warp region DPTZ parameters!\n");
		return -EINVAL;
	}
	if (update_warp_region_dptz_param(&warp_dptz) < 0) {
		iav_error("Failed to update warp region DPTZ parameter!\n");
		return -EINVAL;
	}

	issue_warp_control_cmd(RID_MAP_ALL);

	return 0;
}

int iav_get_warp_region_dptz_ex(iav_context_t * context,
	struct iav_warp_dptz_ex_s __user * arg)
{
	iav_warp_dptz_ex_t param;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (G_user_wp_num) {
		switch (param.buffer_id) {
		case IAV_ENCODE_SOURCE_SECOND_BUFFER:
			param = G_prev_c_regions;
			break;
		case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
			param = G_prev_a_regions;
			break;
		default:
			iav_error("Invalid buffer id [%d] in warp region DPTZ!\n",
				param.buffer_id);
			return -EINVAL;
			break;
		}
	} else {
		memset(&param.dptz, 0, sizeof(iav_region_dptz_ex_t));
	}

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_get_warp_proc(iav_context_t * context, void __user * arg)
{
	iav_warp_proc_t param;
	int rval = 0;

	if (check_multi_warp_control_state())
		return -EPERM;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (param.rid >= MAX_NUM_WARP_AREAS) {
		iav_error("Invalid region ID [%d].\n", param.rid);
		return -EINVAL;
	}

	if (likely(G_user_wp_num)) {
		switch (param.wid) {
		case IAV_WARP_TRANSFORM:
			rval = get_warp_transform_param(param.rid, &param.arg.transform);
			break;
		case IAV_WARP_COPY:
			rval = get_warp_copy_param(param.rid, &param.arg.copy);
			break;
		case IAV_WARP_SUB_DPTZ:
			rval  = get_warp_sub_dptz_param(param.rid, &param.arg.sub_dptz);
			break;
		case IAV_WARP_OFFSET:
			rval = get_stream_offset_param(&param.arg.offset);
			break;
		default:
			iav_error("Unknown wid: %d.\n", param.wid);
			rval = -EINVAL;
			break;
		}
	} else {
		memset(&param.arg, 0, sizeof(param.arg));
	}
	if (rval < 0) {
		return -EINVAL;
	}

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

int iav_cfg_warp_proc(iav_context_t * context, void __user * arg)
{
	iav_warp_proc_t param;
	int rval = 0;

	if (check_multi_warp_control_state())
		return -EPERM;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (param.rid >= MAX_NUM_WARP_AREAS) {
		iav_error("Invalid region ID [%d].\n", param.rid);
		return -EINVAL;
	}

	switch (param.wid) {
	case IAV_WARP_TRANSFORM:
		rval = cfg_warp_transform_param(param.rid, &param.arg.transform);
		break;
	case IAV_WARP_COPY:
		rval = cfg_warp_copy_param(param.rid, &param.arg.copy);
		break;
	case IAV_WARP_SUB_DPTZ:
		rval = cfg_warp_sub_dptz_param(param.rid, &param.arg.sub_dptz);
		break;
	case IAV_WARP_OFFSET:
		rval = cfg_stream_offset_param(&param.arg.offset);
		break;
	default:
		iav_error("Unknown wid: %d.\n", param.wid);
		rval = -EINVAL;
		break;
	}

	return rval;
}

int iav_apply_warp_proc(iav_context_t * context, void __user * arg)
{
	int rval = 0, i;
	u32 rid_map, buf_map;
	iav_apply_flag_t flags[IAV_WARP_PROC_NUM];

	if (check_multi_warp_control_state())
		return -EPERM;

	if (copy_from_user(flags, arg, sizeof(flags)))
		return -EFAULT;

	for (i = IAV_WARP_PROC_FIRST, rid_map = 0; i < IAV_WARP_PROC_LAST; ++i) {
		if (flags[i].apply && flags[i].param) {
			rid_map |= flags[i].param;
		}
	}
	if ((G_user_wp_num != G_user_wp_num_prev) && rid_map) {
		/* Total warp region num is increased / decreased so it needs to
		 * re-send commands for all warp regions.
		 */
		rid_map = RID_MAP_ALL;
	}
	if (check_warp_proc_apply(rid_map) < 0) {
		return -EINVAL;
	}

	G_yuv_table_index = (G_yuv_table_index + 1) % YUV_WARP_TOGGLE_NUM;
	buf_map = 0;
	if (!is_buf_type_off(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		buf_map |= IAV_2ND_BUFFER;
	}
	if (!is_buf_type_off(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		buf_map |= IAV_4TH_BUFFER;
	}
	prepare_default_warp_dptz_param(buf_map, IAV_BUF_WARP_SET);

	issue_warp_control_cmd(rid_map);

	if (flags[IAV_WARP_OFFSET].apply) {
		/* Wait to sync up the offset and warp param */
		wait_vcap_msg_count(IAV_WAIT_VSYNC_SYNC_OFFSET_WARP);

		/* Fixme: Need to issue batch command if the offset is issued for
		 * multiple streams. */
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (flags[IAV_WARP_OFFSET].param & (1 << i)) {
				cmd_update_encode_params_ex(context, i,
					UPDATE_STREAM_OFFSET_FLAG);
			}
		}
	}

	return rval;
}


/*
 * Chromatic Aberration Warp
 */
#define CA_WARP_TOGGLE_NUM  5
static u8* G_ca_table_addr;
static iav_ca_warp_t G_ca_warp_cfg;
static u32 G_ca_table_max_size;
static int G_ca_table_index;

static u8* get_ca_table_kernel_address(int index)
{
	return (G_ca_table_addr  + TABLE_NUM_PER_AREA * index * G_ca_table_max_size);
}

static int get_ca_warp_max_grid_col(void)
{
	switch (get_enc_mode()) {
		case DSP_HIGH_MEGA_PIXEL_MODE:
		case DSP_HIGH_MP_FULL_PERF_MODE:
		case DSP_HIGH_MP_WARP_MODE:
			return 2 * MAX_GRID_WIDTH;
		default:
			return MAX_GRID_WIDTH;
	}
}

static int get_ca_warp_table_size(int col, int row)
{
	return row * col * sizeof(s16);
}

static void init_ca_warp_param(void)
{
	G_ca_table_index = 0;
	G_ca_warp_cfg.hor_map.addr = (u32) get_ca_table_kernel_address(
	    G_ca_table_index);
	G_ca_warp_cfg.ver_map.addr = G_ca_warp_cfg.hor_map.addr
		+ G_ca_table_max_size;
	memset(G_ca_table_addr, 0, G_ca_table_max_size * CA_WARP_TOGGLE_NUM);
}

static int check_ca_warp_param(iav_ca_warp_t* cfg)
{
	iav_warp_map_ex_t* map;
	int max_grid_col = get_ca_warp_max_grid_col();

	map = &cfg->hor_map;
	if (map->enable) {
		if (!map->addr) {
			iav_error("CA warp horizontal address cannot be NULL if enabled.\n");
			return -1;
		}
		if (!map->output_grid_col || !map->output_grid_row) {
			iav_error("CA warp horizontal grid row [%d] and column [%d] "
				"cannot be zero if enabled.\n",
			    map->output_grid_row, map->output_grid_col);
			return -1;
		}
		if (map->output_grid_col > max_grid_col
			|| map->output_grid_row > MAX_GRID_HEIGHT) {
			iav_error("CA warp horizontal grid row [%d] and column [%d] "
				"cannot be greater than %d and %d.\n",
			    map->output_grid_row, map->output_grid_col,
			    MAX_GRID_HEIGHT, max_grid_col);
			return -1;
		}
	}

	map = &cfg->ver_map;
	if (map->enable) {
		if (!map->addr) {
			iav_error("CA warp vertical address cannot be NULL if enabled.\n");
			return -1;
		}
		if (!map->output_grid_col || !map->output_grid_row) {
			iav_error("CA warp vertical grid row [%d] and column [%d] "
				"cannot be zero if enabled.\n",
			    map->output_grid_row, map->output_grid_col);
			return -1;
		}
		if (map->output_grid_col > max_grid_col
			|| map->output_grid_row > MAX_GRID_HEIGHT) {
			iav_error("CA warp vertical grid row [%d] and column [%d] "
				"cannot be greater than %d and %d.\n",
			    map->output_grid_row, map->output_grid_col,
			    MAX_GRID_HEIGHT, max_grid_col);
			return -1;
		}
	}
	return 0;
}

void cmd_set_ca_warp(void)
{
	u8 * kernel_addr, *addr = NULL;
	iav_warp_map_ex_t* map;

	set_chromatic_aberration_warp_control_t dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CHROMATIC_ABERRATION_WARP_CONTROL;
	dsp_cmd.red_scale_factor = G_ca_warp_cfg.red_scale_factor;
	dsp_cmd.blue_scale_factor = G_ca_warp_cfg.blue_scale_factor;

	map = &G_ca_warp_cfg.hor_map;
	kernel_addr = get_ca_table_kernel_address(G_ca_table_index);
	if (map->enable) {
		dsp_cmd.horz_warp_enable = 1;
		dsp_cmd.horz_pass_grid_array_width = map->output_grid_col - 1; // Tile number
		dsp_cmd.horz_pass_grid_array_height = map->output_grid_row - 1;
		dsp_cmd.horz_pass_horz_grid_spacing_exponent = map->horizontal_spacing;
		dsp_cmd.horz_pass_vert_grid_spacing_exponent = map->vertical_spacing;

		addr = kernel_addr;
		clean_cache_aligned((u8*) addr,
		    get_ca_warp_table_size(map->output_grid_col, map->output_grid_row));
		dsp_cmd.warp_horizontal_table_address = VIRT_TO_DSP(addr);
	}

	map = &G_ca_warp_cfg.ver_map;
	if (map->enable) {
		dsp_cmd.vert_warp_enable = 1;
		dsp_cmd.vert_pass_grid_array_width = map->output_grid_col - 1;
		dsp_cmd.vert_pass_grid_array_height = map->output_grid_row - 1;
		dsp_cmd.vert_pass_horz_grid_spacing_exponent = map->horizontal_spacing;
		dsp_cmd.vert_pass_vert_grid_spacing_exponent = map->vertical_spacing;

		addr = kernel_addr + G_ca_table_max_size;
		clean_cache_aligned((u8*) addr,
		    get_ca_warp_table_size(map->output_grid_col, map->output_grid_row));
		dsp_cmd.warp_vertical_table_address = VIRT_TO_DSP(addr);
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, red_scale_factor);
	iav_dsp(dsp_cmd, blue_scale_factor);
	iav_dsp(dsp_cmd, horz_warp_enable);
	if (dsp_cmd.horz_warp_enable) {
		iav_dsp(dsp_cmd, horz_pass_grid_array_width);
		iav_dsp(dsp_cmd, horz_pass_grid_array_height);
		iav_dsp(dsp_cmd, horz_pass_horz_grid_spacing_exponent);
		iav_dsp(dsp_cmd, horz_pass_vert_grid_spacing_exponent);
		iav_dsp_hex(dsp_cmd, warp_horizontal_table_address);
	}
	iav_dsp(dsp_cmd, vert_warp_enable);
	if (dsp_cmd.vert_warp_enable) {
		iav_dsp(dsp_cmd, vert_pass_grid_array_width);
		iav_dsp(dsp_cmd, vert_pass_grid_array_height);
		iav_dsp(dsp_cmd, vert_pass_horz_grid_spacing_exponent);
		iav_dsp(dsp_cmd, vert_pass_vert_grid_spacing_exponent);
		iav_dsp_hex(dsp_cmd, warp_vertical_table_address);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

int mem_alloc_ca_warp(u8 ** ptr, int *alloc_size)
{
	u32 total_size;
	G_ca_table_addr = NULL;
	G_ca_table_max_size = CA_TABLE_MAX_SIZE * sizeof(s16) * 2; // 2 tables for stitching

	total_size = ALIGN(
		CA_WARP_TOGGLE_NUM * G_ca_table_max_size * TABLE_NUM_PER_AREA, 4 * KB);

	if ((G_ca_table_addr = kzalloc(total_size, GFP_KERNEL)) == NULL ) {
		iav_error("Not enough memory to allocate CA table!\n");
		return -1;
	}

	*ptr = G_ca_table_addr;
	*alloc_size = total_size;
	return 0;
}

int get_ca_warp_param(iav_ca_warp_t* cfg)
{
	u8* hor_kernel_addr, *ver_kernel_addr;
	iav_warp_map_ex_t* map;
	hor_kernel_addr = get_ca_table_kernel_address(G_ca_table_index);
	ver_kernel_addr = hor_kernel_addr + G_ca_table_max_size;

	map = &G_ca_warp_cfg.hor_map;
	if (map->enable && cfg->hor_map.addr) {
		if (copy_to_user((void *) cfg->hor_map.addr, (void *) hor_kernel_addr,
			get_ca_warp_table_size(map->output_grid_col, map->output_grid_row))) {
			iav_error("Failed to copy chroma aberration horizontal table to"
				" user!\n");
			return -1;
		}
	}
	map = &G_ca_warp_cfg.ver_map;
	if (map->enable && cfg->ver_map.addr) {
		if (copy_to_user((void *) cfg->ver_map.addr, (void *) ver_kernel_addr,
			get_ca_warp_table_size(map->output_grid_col,map->output_grid_row))) {
			iav_error("Failed to copy chroma aberration vertical table to"
			    " user!\n");
			return -1;
		}
	}
	cfg->red_scale_factor = G_ca_warp_cfg.red_scale_factor;
	cfg->blue_scale_factor = G_ca_warp_cfg.blue_scale_factor;
	copy_warp_map_param(&cfg->hor_map, &G_ca_warp_cfg.hor_map);
	copy_warp_map_param(&cfg->ver_map, &G_ca_warp_cfg.ver_map);

	return 0;
}

int cfg_ca_warp_param(iav_ca_warp_t* param)
{
	int next_index;
	u8* next_hor_addr, *next_ver_addr;
	iav_warp_map_ex_t* map;
	if (check_ca_warp_param(param) < 0) {
		return -1;
	}
	next_index = (G_ca_table_index + 1) % CA_WARP_TOGGLE_NUM;
	next_hor_addr = get_ca_table_kernel_address(next_index);
	next_ver_addr = next_hor_addr + G_ca_table_max_size;

	map = &param->hor_map;
	if (map->enable) {
		if (copy_from_user((void *)next_hor_addr, (void *)map->addr,
			get_ca_warp_table_size(map->output_grid_col, map->output_grid_row))) {
			iav_error("Failed to copy CA warp horizontal table from user!\n");
			return -1;
		}
	}

	map = &param->ver_map;
	if (map->enable) {
		if (copy_from_user((void *)next_ver_addr, (void *)map->addr,
			get_ca_warp_table_size(map->output_grid_col, map->output_grid_row))) {
			iav_error("Failed to copy CA warp vertical table from user!\n");
			return -1;
		}
	}
	G_ca_warp_cfg = *param;
	G_ca_table_index = next_index;

	return 0;
}

int iav_warp_init(void)
{
	init_yuv_warp_param();
	init_ca_warp_param();
	return 0;
}

