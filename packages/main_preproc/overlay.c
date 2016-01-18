/*******************************************************************************
 * overlay.c
 *
 * History:
 *  Mar 21, 2014 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#include "lib_vproc.h"
#include "priv.h"

#define CLUT_NUM         (16)
#define DATA_OFFSET (CLUT_NUM * OVERLAY_CLUT_BYTES)

static int G_overlay_inited;

static inline int get_next_data_id(const overlay_buffer_t* buf)
{
	return (1 + buf->data_id) % OVERLAY_BUF_NUM;
}

static inline int is_background(const int entry, const overlay_buffer_t* buf)
{
	return (buf->bg_entry == entry || buf->pm_entry == entry);
}

static int check_overlay(overlay_param_t* overlay)
{
	iav_encode_format_ex_t* stream;
	iav_rect_ex_t* rect;
	overlay_buffer_t* buf;

	if (!overlay) {
		ERROR("Overlay is NULL.\n");
		return -1;
	}
	if (overlay->stream_id < 0
	    || overlay->stream_id >= vproc.overlay_record.max_stream_num) {
		ERROR("Invalid stream overlay id [%d]. Only support %d streams.\n",
		    overlay->stream_id, vproc.overlay_record.max_stream_num);
		return -1;
	}
	if (overlay->area_id < 0 || overlay->area_id >= OVERLAY_MAX_AREA_NUM) {
		ERROR("Invalid area id [%d].\n", overlay->area_id);
		return -1;
	}
	if (overlay->enable) {
		if (ioctl_get_stream_format(overlay->stream_id) < 0) {
			return -1;
		}
		stream = &vproc.stream[overlay->stream_id];
		rect = &overlay->rect;
		buf =
		    &vproc.overlay_record.buffer[overlay->stream_id][overlay->area_id];
		if (!rect->width || !rect->height) {
			ERROR("invalid size %dx%d.\n", rect->width, rect->height);
			return -1;
		}
		if (overlay->pitch * rect->height > buf->bytes) {
			ERROR("too large overlay %dx%d > 0x%x.\n", overlay->pitch,
			    rect->height, buf->bytes);
			return -1;
		}
		if ((rect->x + rect->width > stream->encode_width)
		    || (rect->y + rect->height > stream->encode_height)) {
			ERROR("overlay (w %d, h %d, x %d, y %d) "
				"exceeds the stream size %dx%d.\n", rect->width, rect->height,
			    rect->x, rect->y, stream->encode_width, stream->encode_height);
			return -1;
		}
		if (!overlay->data_addr) {
			ERROR("overlay data is NULL.\n");
			return -1;
		}
		if (overlay->type == OSD_CLUT_8BIT && !overlay->clut_addr) {
			ERROR("clut is NULL.\n");
			return -1;
		}
	}

	return 0;
}

static int check_mbstream_overlay(overlay_param_t* overlay)
{
	if (check_overlay(overlay) < 0) {
		return -1;
	}

	if (overlay->enable) {
		if ((overlay->rect.x & 0xf) || (overlay->rect.y & 0xf)
			|| (overlay->rect.width &0xf) || (overlay->rect.height & 0xf)) {
			ERROR("Overlay (w %d, h %d, x %d, y %d) must be multiple of "
				"16.\n", overlay->rect.width, overlay->rect.height,
				overlay->rect.x, overlay->rect.y);
			return -1;
		}
	}
	return 0;
}

static void fill_new_overlay(overlay_param_t* overlay)
{
	overlay_insert_area_ex_t* area =
	    &vproc.overlay[overlay->stream_id]->area[overlay->area_id];
	overlay_buffer_t* buf =
	    &vproc.overlay_record.buffer[overlay->stream_id][overlay->area_id];
	int total_bytes = overlay->pitch * overlay->rect.height;
	int next_data_id = get_next_data_id(buf);

	area->clut_id = overlay->stream_id * OVERLAY_MAX_AREA_NUM
	    + overlay->area_id;
	if (overlay->type == OSD_CLUT_8BIT) {
		memcpy(buf->clut_addr, overlay->clut_addr, OVERLAY_CLUT_BYTES);
	}
	memcpy(buf->data_addr[next_data_id], overlay->data_addr, total_bytes);
	area->pitch = overlay->pitch;
	area->width = overlay->rect.width;
	area->height = overlay->rect.height;
	area->start_x = overlay->rect.x;
	area->start_y = overlay->rect.y;
	area->total_size = total_bytes;
	area->data = buf->data_addr[next_data_id];
	buf->data_id = next_data_id;
	buf->bg_entry = overlay->clut_bg_entry;
	buf->pm_entry = overlay->clut_pm_entry;
	buf->pm_overlapped = 0;

	DEBUG("fill data to buffer%d.\n", next_data_id);
}

static void fill_mbstream_clut(overlay_param_t* overlay)
{
	overlay_buffer_t* buf =
	    &vproc.overlay_record.buffer[overlay->stream_id][overlay->area_id];
	yuv_color_t* pm_clut = (yuv_color_t *) buf->clut_addr + buf->pm_entry;
	pm_clut->y = vproc.streampm[overlay->stream_id]->y;
	pm_clut->u = vproc.streampm[overlay->stream_id]->u;
	pm_clut->v = vproc.streampm[overlay->stream_id]->v;
	pm_clut->alpha = 255;
	DEBUG("stream %d pm clut (Y %d, U %d, V %d), alpha %d\n",
	    overlay->stream_id, overlay->area_id, pm_clut->y, pm_clut->u,
	    pm_clut->v, pm_clut->alpha);
}

static void prepare_mbstream_overlay(const int stream_id, const int area_id)
{
	int i, j;
	overlay_buffer_t* buf = &vproc.overlay_record.buffer[stream_id][area_id];
	overlay_insert_area_ex_t* area = &vproc.overlay[stream_id]->area[area_id];
	int next_buf_id = get_next_data_id(buf);
	u8* src = buf->data_addr[buf->data_id];
	u8* dst = buf->data_addr[next_buf_id];

	if (!buf->pm_overlapped) {
		// Copy unpolluted overlay to the next buffer
		memcpy(dst, src, area->total_size);
		DEBUG("stream%d area%d copy data to buffer %d.\n", stream_id,
			area_id, next_buf_id);
	} else {
		// Fill the unpolluted overlay to the next buffer one by one
		for (i = 0; i < area->height; i++) {
			for (j = 0; j < area->pitch; j++) {
				dst[j] = is_background(src[j], buf) ? buf->bg_entry : src[j];
			}
			src += area->pitch;
			dst += area->pitch;
		}
		DEBUG("stream%d area%d fill unpolluted data to buffer %d.\n",
		    stream_id, area_id, next_buf_id);
	}

	buf->data_id = next_buf_id;
	buf->pm_overlapped = 0;
}

static void resolve_mbstream_overlap(const int stream_id,
    const u32 overlap_areas, const iav_rect_ex_t* overlap_mb)
{
	int i, x, y;
	overlay_insert_area_ex_t* area;
	overlay_buffer_t* overlay_buf;
	iav_rect_ex_t overlap;
	u8* addr;

	DEBUG("resolve overlap in stream %d.\n", stream_id);
	for (i = 0; i < OVERLAY_MAX_AREA_NUM; i++) {
		if (!is_id_in_map(i, overlap_areas)) {
			continue;
		}
		DEBUG("area%d overlaps\n", i);
		TRACE_RECT("\t rect", overlap_mb[i]);

		// Remove overlap rect from pm
		erase_mbstream_pm(stream_id, &overlap_mb[i]);

		// Fill overlap background area as "pm"
		overlay_buf = &vproc.overlay_record.buffer[stream_id][i];
		area = &vproc.overlay[stream_id]->area[i];
		overlay_buf->pm_overlapped = 1;

		rectmb_to_rect(&overlap, &overlap_mb[i]);

		if ((IAV_ENCODE_HIGH_MP_FULL_PERF_MODE == vproc.resource.encode_mode)
			&& (PM_MB_STREAM == get_pm_type())) {
			flip_stream_pm_overlay(&overlap, stream_id);
		}
		addr = overlay_buf->data_addr[overlay_buf->data_id]
			+ (overlap.y - area->start_y) * area->pitch
			+ (overlap.x - area->start_x);
		for (y = 0; y < overlap.height; y++) {
			for (x = 0; x < overlap.width; x++) {
				if (is_background(addr[x], overlay_buf)) {
					// Mark as pm
					addr[x] = overlay_buf->pm_entry;
				}
			}
			addr += area->pitch;
		}

	}
}

void update_mbstream_buffer(const int stream_id, OP overlay_op)
{
	int i;
	overlay_insert_area_ex_t* area;
	iav_rect_ex_t pm_mb, pm, overlap_mb[OVERLAY_MAX_AREA_NUM];
	iav_rect_ex_t* overlay_mb;
	u32 overlap_areas;
	pm_node_t* pm_in_vin = vproc.pm_record.node_in_vin;

	if (overlay_op == OP_UPDATE) {
		// Prepare unpolluted overlay buffer
		for (i = 0; i < OVERLAY_MAX_AREA_NUM; i++) {
			area = &vproc.overlay[stream_id]->area[i];
			if (area->enable) {
				prepare_mbstream_overlay(stream_id, i);
			}
		}
	}

	prepare_mbstream_unmask(stream_id);

	for (pm_in_vin = vproc.pm_record.node_in_vin; pm_in_vin != NULL ;
	    pm_in_vin = pm_in_vin->next) {
		// Check if mask appears the stream
		if (rect_vin_to_stream(&pm, &pm_in_vin->rect, stream_id) < 0) {
			DEBUG("[Stream%d] not have Mask%d.\n", stream_id, pm_in_vin->id);
			continue;
		}
		// Draw mask to the pm buffer
		DEBUG("[Stream%d] have Mask%d.\n", stream_id, pm_in_vin->id);
		rect_to_rectmb(&pm_mb, &pm);
		draw_mbstream_pm(stream_id, &pm_mb);

		// Check if mask overlaps overlay
		for (i = 0, overlap_areas = 0; i < OVERLAY_MAX_AREA_NUM; i++) {
			area = &vproc.overlay[stream_id]->area[i];
			overlay_mb = &vproc.overlay_record.buffer[stream_id][i].rect_mb;

			if (!area->enable || !is_rect_overlap(&pm_mb, overlay_mb)) {
				DEBUG("[Stream%d] Mask%d not overlap Area%d.\n", stream_id,
				    pm_in_vin->id, i);
				continue;
			}

			overlap_areas |= (1 << i);
			DEBUG("[Stream%d] Mask%d overlaps Area%d. Overlap Areas 0x%x\n",
			    stream_id, pm_in_vin->id, i, overlap_areas);

			get_overlap_rect(&overlap_mb[i], &pm_mb, overlay_mb);
			TRACE_RECTP("overlay MB rect", overlay_mb);
			TRACE_RECT("overlap MB rect", overlap_mb[i]);
		}
		if (overlap_areas) {
			resolve_mbstream_overlap(stream_id, overlap_areas, overlap_mb);
		}
	}

}
int init_overlay(void)
{
	int buf_bytes, buf_num, buf_no, area_no;
	int i, j, k;
	overlay_buffer_t* buf;
	u8* clut_addr, *data_addr;

	if (unlikely(!G_overlay_inited)) {
		do {
			if (ioctl_map_overlay() < 0) {
				break;
			}
			clut_addr = vproc.overlay_mem.addr;
			data_addr = clut_addr + DATA_OFFSET;
			if (ioctl_get_system_resource() < 0) {
				break;
			}
			vproc.overlay_record.max_stream_num =
			    MIN(OVERLAY_MAX_STREAM_NUM, vproc.resource.max_num_encode_streams);
			buf_num = vproc.overlay_record.max_stream_num * OVERLAY_MAX_AREA_NUM
			    * OVERLAY_BUF_NUM;
			buf_bytes = (vproc.overlay_mem.length - DATA_OFFSET) / buf_num;
			DEBUG("buffer size 0x%x\n", buf_bytes);

			for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
				for (j = 0; j < OVERLAY_MAX_AREA_NUM; j++) {
					area_no = i * OVERLAY_MAX_AREA_NUM + j;
					buf = &vproc.overlay_record.buffer[i][j];
					buf->clut_addr = clut_addr + area_no * OVERLAY_CLUT_BYTES;
					buf->data_id = 0;
					buf->bytes = buf_bytes;
					for (k = 0; k < OVERLAY_BUF_NUM; k++) {
						buf_no = area_no * OVERLAY_BUF_NUM + k;
						buf->data_addr[k] = data_addr + buf_no * buf_bytes;
						TRACE("stream %d area %d buffer %d addr 0x%x\n", i, j,
						    k, buf->data_addr[k]);
					}
				}
			}

			G_overlay_inited = 1;
		} while (0);
	}
	return G_overlay_inited ? 0 : -1;
}

int deinit_overlay(void)
{
	if (G_overlay_inited) {
		G_overlay_inited = 0;
	}
	return G_overlay_inited ? -1 : 0;
}

int operate_overlay(overlay_param_t* param)
{
	overlay_insert_area_ex_t* area;

	if (init_overlay() < 0) {
		return -1;
	}

	if (check_overlay(param) < 0) {
		return -1;
	}

	area = &vproc.overlay[param->stream_id]->area[param->area_id];
	memset(area, 0, sizeof(*area));
	area->enable = param->enable;
	vproc.overlay[param->stream_id]->type = param->type;

	if (param->enable) {
		fill_new_overlay(param);
	}

	return ioctl_cfg_stream_overlay(1 << param->stream_id);
}

int operate_mbstream_overlaypm(const OP op, const int stream_id,
    overlay_param_t* param)
{
	int area_id;
	overlay_insert_area_ex_t* area;
	iav_stream_privacy_mask_t* pm;
	overlay_buffer_t* overlay_buf;
	pm_buffer_t* pm_buf;
	u32 streams = 1 << stream_id;

	if (init_dptz() < 0 || init_mbstream() < 0 || init_overlay() < 0) {
		return -1;
	}

	if (get_mbstream_format(1 << stream_id) < 0) {
		return -1;
	}

	if (op != OP_UPDATE) {
		if (check_mbstream_overlay(param) < 0) {
			return -1;
		}

		area_id = param->area_id;
		area = &vproc.overlay[stream_id]->area[area_id];
		memset(area, 0, sizeof(*area));
		area->enable = param->enable;
		vproc.overlay[param->stream_id]->type = param->type;

		if (param->enable) {
			overlay_buf =
			    &vproc.overlay_record.buffer[param->stream_id][param->area_id];
			if ((IAV_ENCODE_HIGH_MP_FULL_PERF_MODE == vproc.resource.encode_mode)
				&& (PM_MB_STREAM == get_pm_type())) {
				flip_stream_pm_overlay(&param->rect, param->stream_id);
			}
			fill_new_overlay(param);
			fill_mbstream_clut(param);
			if ((IAV_ENCODE_HIGH_MP_FULL_PERF_MODE == vproc.resource.encode_mode)
				&& (PM_MB_STREAM == get_pm_type())) {
				flip_stream_pm_overlay(&param->rect, param->stream_id);
			}
			rect_to_rectmb(&overlay_buf->rect_mb, &param->rect);
			TRACE_RECT("add overlay rect", param->rect);
			TRACE_RECT("\t\t=> MB", overlay_buf->rect_mb);
		}
	}

	update_mbstream_buffer(stream_id, op);
	pm = vproc.streampm[stream_id];
	pm_buf = &vproc.pm_record.buffer[stream_id];
	pm->enable = (pm->enable || vproc.pm_record.node_num > 0);
	pm->buffer_pitch = pm_buf->pitch;
	pm->buffer_height = pm_buf->height;
	pm->buffer_addr = pm_buf->addr[pm_buf->id];

	for (area_id = 0; area_id < OVERLAY_MAX_AREA_NUM; area_id++) {
		area = &vproc.overlay[stream_id]->area[area_id];
		overlay_buf = &vproc.overlay_record.buffer[stream_id][area_id];
		area->data = overlay_buf->data_addr[overlay_buf->data_id];
	}

	return
	    (ioctl_cfg_stream_pm(streams) || ioctl_cfg_stream_overlay(streams)) ?
	        -1 : 0;
}

int disable_overlay(void)
{
	int i, j;
	u32 streams = 0;
	for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
		for (j = 0; j < OVERLAY_MAX_AREA_NUM; j++) {
			vproc.overlay[i]->area[j].enable = 0;
		}
		streams |= (1 << i);
	}
	return ioctl_cfg_stream_overlay(streams);
}
