/*******************************************************************************
 * pm_mb.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"

#include "lib_vproc.h"
#include "priv.h"

#define MCTF_MAX_STRENGTH          (0xf)
#define MB_PM_UNMASK               (0x8)

static int G_mbmain_inited = 0;
static int G_mbstream_inited = 0;

static inline int get_next_buffer_id(const pm_buffer_t* buf)
{
	return (buf->id >= PM_MB_BUF_NUM - 1) ? 0 : (buf->id + 1);
}

static inline int get_cur_buffer_id(pm_buffer_t* buf)
{
	if (unlikely(buf->id < 0)) {
		buf->id = 0;
	}
	return buf->id;
}

static int get_mb_bytes(const int pitch, const int mb_height)
{
	return pitch * (mb_height + 1); // one row reserved from MCTF
}

static void fill_buffer_pm_unmask(const pm_buffer_t* buf, const int buf_id)
{
	int remain, unmask;
	u8* src, *dst;
	iav_mctf_filter_strength_t* pm =
	    (iav_mctf_filter_strength_t *) buf->addr[buf_id];

	DEBUG("fill buffer %d unmask.\n", buf_id);

	// one MB
	pm->u |= MB_PM_UNMASK;
	pm->v |= MB_PM_UNMASK;
	pm->y |= MB_PM_UNMASK;
	pm->privacy_mask = 0;

	unmask = sizeof(iav_mctf_filter_strength_t);
	remain = buf->bytes - unmask;

	src = (u8 *) pm;
	dst = src + unmask;

	// copy the unmask bytes to the remain memory
	// 1,1,2,4,8,16,32,...
	while (remain >= unmask) {
		memcpy(dst, src, unmask);
		remain -= unmask;
		dst += unmask;
		unmask += unmask;
	}
	if (remain) {
		memcpy(dst, src, remain);
	}
}

static void clear_mbmain_pm(const int buf_id)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];
	iav_mctf_filter_strength_t* pm =
	    (iav_mctf_filter_strength_t *) buf->addr[buf_id];
	int pm_num = buf->bytes / sizeof(iav_mctf_filter_strength_t);
	int i;
	for (i = 0; i < pm_num; ++i) {
		pm[i].privacy_mask = 0;
	}
	DEBUG("clear buffer %d\n", buf_id);
}

static void copy_mbmain_pm(const int src_buf_id, const int dst_buf_id)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];
	if (src_buf_id < 0) {
		return;
	}
	memcpy(buf->addr[dst_buf_id], buf->addr[src_buf_id], buf->bytes);
	DEBUG("copy buffer %d to buffer %d\n", src_buf_id, dst_buf_id);
}

static void display_pm_nodes(void)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;

	DEBUG("\tTotal mask number = %d\n", vproc.pm_record.node_num);
	while (node) {
		DEBUG("\t   mask%d (x %d, y %d, w %d, h %d)\n", node->id, node->rect.x,
			node->rect.y, node->rect.width, node->rect.height);
		node = node->next;
	}
}

static void fill_buffer_pm(const pm_buffer_t* buf, const int buf_id,
    const iav_rect_ex_t* rect_mb, const int is_mask)
{
	int i, j;
	iav_mctf_filter_strength_t* p;
	u8* line_addr = buf->addr[buf_id] + rect_mb->y * buf->pitch
	    + rect_mb->x * sizeof(iav_mctf_filter_strength_t);

	for (i = 0; i < rect_mb->height; i++) {
		p = (iav_mctf_filter_strength_t*) line_addr;
		for (j = 0; j < rect_mb->width; j++, p++) {
			p->privacy_mask = (is_mask ? 0x1 : 0x0);
		}
		line_addr += buf->pitch;
	}
}

static void fill_buffer_mctf(const pm_buffer_t* buf, const int buf_id,
    const u8 strength, const iav_rect_ex_t* rect_mb)
{
	int i, j;
	u8* line_head = buf->addr[buf_id] + rect_mb->y * buf->pitch
	    + rect_mb->x * sizeof(iav_mctf_filter_strength_t);
	iav_mctf_filter_strength_t* pm = NULL;

	for (i = 0; i < rect_mb->height; i++) {
		pm = (iav_mctf_filter_strength_t *) line_head;
		for (j = 0; j < rect_mb->width; j++, pm++) {
			pm->y = pm->u = pm->v = strength;
		}
		line_head += buf->pitch;
	}
}

static void draw_mbmain_pm(const int buf_id, pm_node_t* node)
{
	iav_rect_ex_t pm_mb, pm_in_main;

	if (rect_vin_to_main(&pm_in_main, &node->rect) == 0) {
		rect_to_rectmb(&pm_mb, &pm_in_main);
		fill_buffer_pm(&vproc.pm_record.buffer[0], buf_id, &pm_mb, 1);
	}
	clear_pm_node_from_redraw(node);
}

static void erase_mbmain_pm(const int buf_id, pm_node_t* node)
{
	pm_node_t* cur = vproc.pm_record.node_in_vin;
	iav_rect_ex_t pm_mb, pm_in_main;

	if (rect_vin_to_main(&pm_in_main, &node->rect) == 0) {
		rect_to_rectmb(&pm_mb, &pm_in_main);
		fill_buffer_pm(&vproc.pm_record.buffer[0], buf_id, &pm_mb, 0);
	}

	// Mark the masks overlapped with the removed one.
	while (cur) {
		if ((cur != node) && is_rect_overlap(&cur->rect, &node->rect)) {
			set_pm_node_to_redraw(node);
			DEBUG("mark mask%d to be redraw\n", node->id);
		}
		cur = cur->next;
	}
}

static int check_mb_add(const int id, const iav_rect_ex_t* rect)
{
	iav_reso_ex_t* mainbuf = &vproc.srcbuf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
	pm_node_t* node = vproc.pm_record.node_in_vin;

	if (!rect) {
		ERROR("Privacy mask rect is NULL.\n");
		return -1;
	}

	if (!rect->width || !rect->height) {
		ERROR("Privacy mask rect %ux%u cannot be zero.\n", rect->width,
		    rect->height);
		return -1;
	}

	if ((rect->x + rect->width > mainbuf->width)
	    || (rect->y + rect->height > mainbuf->height)) {
		ERROR("Privacy mask rect %ux%u offset %ux%u is out of main buffer"
			" %ux%u.\n", rect->width, rect->height, rect->x, rect->y,
		    mainbuf->width, mainbuf->height);
		return -1;
	}

	while (node) {
		if (id == node->id) {
			ERROR("privacy mask id [%d] existed.\n", id);
			return -1;
		}
		node = node->next;
	}
	return 0;
}

static int check_mb_remove(const int id)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	while (node) {
		if (id == node->id) {
			return 0;
		}
		node = node->next;
	}
	ERROR("privacy mask [%d] does not exist.\n", id);
	return -1;
}

static int check_mb_mctf(const u32 strength, const iav_rect_ex_t* rect_in_main)
{
	iav_reso_ex_t* mainbuf = &vproc.srcbuf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;

	if (strength < 0 || strength > MCTF_MAX_STRENGTH) {
		ERROR("MCTF strength [%d] must be in the range [0, %d]\n", strength,
		    MCTF_MAX_STRENGTH);
		return -1;
	}

	if (!rect_in_main) {
		ERROR("MCTF rect is NULL.\n");
		return -1;
	}

	if (!rect_in_main->width || !rect_in_main->height) {
		ERROR("MCTF rect %ux%u cannot be zero.\n", rect_in_main->width,
		    rect_in_main->height);
		return -1;
	}
	if ((rect_in_main->x + rect_in_main->width > mainbuf->width)
	    || (rect_in_main->y + rect_in_main->height > mainbuf->height)) {
		ERROR("MCTF rect %ux%u offset %ux%u is out of main buffer %ux%u.\n",
		    rect_in_main->width, rect_in_main->height, rect_in_main->x,
		    rect_in_main->y, mainbuf->width, mainbuf->height);
		return -1;
	}

	return 0;
}

int get_mbstream_format(u32 streams)
{
	int i;
	pm_buffer_t* buf;

	for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; i++) {
		if (is_id_in_map(i, streams)) {
			if (ioctl_get_stream_format(i) < 0) {
				return -1;
			}
			buf = &vproc.pm_record.buffer[i];
			buf->domain_width = vproc.stream[i].encode_width;
			buf->domain_height = vproc.stream[i].encode_height;
			buf->width = roundup_to_mb(buf->domain_width);
			buf->height = roundup_to_mb(buf->domain_height);
			buf->pitch = ROUND_UP(buf->width * sizeof(u32), 32);
		}
	}
	return 0;
}

void prepare_mbstream_unmask(const int stream_id)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer[stream_id];
	int next_buf_id = get_next_buffer_id(buf);
	fill_buffer_pm_unmask(buf, next_buf_id);
	buf->id = next_buf_id;
	DEBUG("prepare clean pm buffer %d\n", next_buf_id);
}

void draw_mbstream_pm(const int stream_id, const iav_rect_ex_t* rect_mb)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer[stream_id];
	DEBUG("draw pm rect to Stream%d\n", stream_id);
	TRACE_RECTP("\tMB", rect_mb);
	fill_buffer_pm(buf, buf->id, rect_mb, 1);
}

void erase_mbstream_pm(const int stream_id, const iav_rect_ex_t* rect_mb)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer[stream_id];
	DEBUG("erase pm rect for Stream%d\n", stream_id);
	TRACE_RECTP("\tMB", rect_mb);
	fill_buffer_pm(buf, buf->id, rect_mb, 0);
}

int init_mbmain(void)
{
	int i;
	u32 total_size;
	iav_reso_ex_t* mainbuf;
	pm_buffer_t* buf;

	if (unlikely(!G_mbmain_inited)) {
		buf = &vproc.pm_record.buffer[0];
		do {
			if (ioctl_map_pm() < 0) {
				break;
			}
			if (ioctl_get_srcbuf_format(IAV_ENCODE_SOURCE_MAIN_BUFFER) < 0) {
				break;
			}
			mainbuf = &vproc.srcbuf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;
			buf->domain_width = mainbuf->width;
			buf->domain_height = mainbuf->height;
			buf->pitch = vproc.pm_info.buffer_pitch;
			buf->width = roundup_to_mb(buf->domain_width);
			buf->height = roundup_to_mb(buf->domain_height);
			buf->id = -1;
			// Reserve one row for MCTF
			buf->bytes = get_mb_bytes(buf->pitch, buf->height);
			total_size = buf->bytes * PM_MB_BUF_NUM;

			DEBUG("domain %dx%d, buffer pitch %d, width %d, height %d, "
				"size 0x%x\n", buf->domain_width, buf->domain_height,
			    buf->pitch, buf->width, buf->height, buf->bytes);

			if (unlikely(total_size > vproc.pm_mem.length)) {
				ERROR("Memory size [0x%x] for mb main privacy mask "
					"must be no less than [0x%x].\n", vproc.pm_mem.length,
				    total_size);
				break;
			}

			for (i = 0; i < PM_MB_BUF_NUM; ++i) {
				buf->addr[i] = vproc.pm_mem.addr + i * buf->bytes;
				fill_buffer_pm_unmask(buf, i);
				DEBUG("buffer %d addr 0x%x\n", i, (u32 ) buf->addr[i]);
			}
			vproc.pm->y = DEFAULT_COLOR_Y;
			vproc.pm->u = DEFAULT_COLOR_U;
			vproc.pm->v = DEFAULT_COLOR_V;
			G_mbmain_inited = 1;

		} while (0);
	}
	return G_mbmain_inited ? 0 : -1;
}

int deinit_mbmain(void)
{
	pm_node_t* node, *next;

	if (G_mbmain_inited) {
		node = vproc.pm_record.node_in_vin;
		DEBUG("free total %d masks\n", vproc.pm_record.node_num);
		while (node) {
			DEBUG("free mask%d\n", node->id);
			next = node->next;
			free(node);
			node = next;
		}
		vproc.pm_record.node_in_vin = NULL;
		vproc.pm_record.node_num = 0;
		G_mbmain_inited = 0;
		DEBUG("done\n");
	}
	return G_mbmain_inited ? -1 : 0;
}

int operate_mbmain_pm(const OP op, const int mask_id,
    iav_rect_ex_t* rect_in_main)
{
	pm_node_t* cur, *prev;
	int cur_buf_id, buf_id;
	iav_rect_ex_t rect_in_vin;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	if (init_dptz() < 0 || init_mbmain() < 0)
		return -1;

	switch (op) {
		case OP_ADD:
			if (check_mb_add(mask_id, rect_in_main) < 0)
				return -1;
			break;
		case OP_REMOVE:
			if (check_mb_remove(mask_id) < 0)
				return -1;
			break;
		default:
			break;
	}

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;
	cur_buf_id = get_cur_buffer_id(buf);
	buf_id = get_next_buffer_id(buf);

	if (op == OP_UPDATE) {
		clear_mbmain_pm(buf_id);
	} else if (cur_buf_id >= 0) {
		copy_mbmain_pm(cur_buf_id, buf_id);
	}

	while (cur) {
		if ((op == OP_REMOVE) && (cur->id == mask_id)) {
			DEBUG("remove mask%d\n", cur->id);
			erase_mbmain_pm(buf_id, cur);
			if (prev == NULL ) {
				// cur is head
				vproc.pm_record.node_in_vin = cur->next;
				free(cur);
				cur = vproc.pm_record.node_in_vin;
			} else {
				prev->next = cur->next;
				free(cur);
				cur = prev->next;
			}
			DEBUG("remove done\n");
			--vproc.pm_record.node_num;
		} else {
			if (op == OP_UPDATE) {
				draw_mbmain_pm(buf_id, cur);
			}
			prev = cur;
			cur = prev->next;
		}
	}

	cur = vproc.pm_record.node_in_vin;
	while (cur) {
		if (is_pm_node_to_redraw(cur)) {
			draw_mbmain_pm(buf_id, cur);
			DEBUG("redraw done: mask%d\n", cur->id);
		}
		cur = cur->next;
	}
	display_pm_nodes();

	if (op == OP_ADD) {
		TRACE("try to add new mask%d\n", mask_id);
		rect_main_to_vin(&rect_in_vin, rect_in_main);
		cur = create_pm_node(mask_id, &rect_in_vin);
		if (prev == NULL ) {
			vproc.pm_record.node_in_vin = cur;
		} else {
			prev->next = cur;
		}
		++vproc.pm_record.node_num;
		draw_mbmain_pm(buf_id, cur);
		DEBUG("append the new mask%d\n", cur->id);
	}
	display_pm_nodes();

	buf->id = buf_id;

	vproc.pm->enable = 1;
	vproc.pm->buffer_addr = buf->addr[buf_id];
	vproc.pm->buffer_pitch = buf->pitch;
	vproc.pm->buffer_height = buf->height;

	return ioctl_cfg_pm();
}

int operate_mbmain_mctf(mctf_param_t* mctf)
{
	iav_rect_ex_t rect_mb;
	int buf_id;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	if (init_dptz() < 0 || init_mbmain() < 0)
		return -1;

	if (check_mb_mctf(mctf->strength, &mctf->rect) < 0)
		return -1;

	buf_id = get_cur_buffer_id(buf);
	rect_to_rectmb(&rect_mb, &mctf->rect);
	fill_buffer_mctf(buf, buf_id, mctf->strength, &rect_mb);

	vproc.pm->enable = 1;
	vproc.pm->buffer_addr = buf->addr[buf_id];
	vproc.pm->buffer_pitch = buf->pitch;
	vproc.pm->buffer_height = buf->height;

	return ioctl_cfg_pm();
}

int operate_mbmain_color(yuv_color_t* color)
{
	if (init_mbmain() < 0)
		return -1;

	if (ioctl_get_pm() < 0)
		return -1;

	vproc.pm->y = color->y;
	vproc.pm->u = color->u;
	vproc.pm->v = color->v;

	return ioctl_cfg_pm();
}

int init_mbstream(void)
{
	int i, j;
	u32 total_bytes = 0;
	iav_reso_ex_t* stream_max;
	pm_buffer_t* buf;

	if (unlikely(!G_mbstream_inited)) {
		do {
			if (ioctl_map_pm() < 0) {
				break;
			}
			if (ioctl_get_system_resource() < 0) {
				break;
			}
			vproc.overlay_record.max_stream_num = MIN(
				vproc.resource.max_num_encode_streams,
				IAV_STREAM_MAX_NUM_IMPL);
			vproc.pm_record.node_num = 0;

			for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
				stream_max = &vproc.resource.stream_max_size[i];
				buf = &vproc.pm_record.buffer[i];
				buf->id = -1;
				buf->bytes = get_mb_bytes(
					ROUND_UP(roundup_to_mb(stream_max->width) * sizeof(u32), 32),
				    roundup_to_mb(stream_max->height));
				DEBUG("buffer bytes 0x%x\n", buf->bytes);
				for (j = 0; j < PM_MB_BUF_NUM; j++) {
					buf->addr[j] = vproc.pm_mem.addr + total_bytes;
					total_bytes += buf->bytes;
					if (total_bytes > vproc.pm_mem.length) {
						ERROR("Memory size [0x%x] for mb stream privacy mask "
							"is insufficient. Please reduce stream max "
							"size or increase privacy mask memory in BSB.\n",
						    vproc.pm_mem.length);
						return -1;
					}

					fill_buffer_pm_unmask(buf, j);
					TRACE("Stream %d buffer %d addr 0x%x\n", i, j,
					    buf->addr[j]);
				}
				vproc.streampm[i]->y = DEFAULT_COLOR_Y;
				vproc.streampm[i]->u = DEFAULT_COLOR_U;
				vproc.streampm[i]->v = DEFAULT_COLOR_V;
			}
			DEBUG("stream num %d\n", vproc.overlay_record.max_stream_num);
			G_mbstream_inited = 1;

		} while (0);
	}
	return G_mbstream_inited ? 0 : -1;
}

int deinit_mbstream(void)
{
	pm_node_t* node, *next;

	if (G_mbstream_inited) {
		node = vproc.pm_record.node_in_vin;
		DEBUG("free total %d masks\n", vproc.pm_record.node_num);
		while (node) {
			DEBUG("free mask [%d]\n", node->id);
			next = node->next;
			free(node);
			node = next;
		}
		vproc.pm_record.node_in_vin = NULL;
		vproc.pm_record.node_num = 0;
		G_mbstream_inited = 0;
		DEBUG("done\n");
	}
	return G_mbstream_inited ? -1 : 0;
}

int operate_mbstream_pmoverlay(const OP op, const int mask_id,
	iav_rect_ex_t* rect_in_main)
{
	pm_node_t* cur, *prev;
	iav_rect_ex_t rect_in_vin;
	int i, j;
	u32 streams = 0;
	pm_buffer_t* pm_buf;
	overlay_insert_area_ex_t* area;
	iav_stream_privacy_mask_t* pm;
	overlay_buffer_t* overlay_buf;

	if (init_dptz() < 0 || init_encofs() < 0 || init_overlay() < 0
	    || init_mbstream() < 0) {
		return -1;
	}

	if (get_mbstream_format((1 << vproc.overlay_record.max_stream_num) - 1) < 0) {
		return -1;
	}

	switch (op) {
		case OP_ADD:
			if (check_mb_add(mask_id, rect_in_main) < 0) {
				return -1;
			}
			DEBUG("add mask%d to vin list\n", mask_id);
			rect_main_to_vin(&rect_in_vin, rect_in_main);
			if (unlikely((prev = create_pm_node(mask_id, &rect_in_vin))
				== NULL)) {
				ERROR("failed to add mask%d.\n", mask_id);
				return -1;

			}
			cur = vproc.pm_record.node_in_vin;
			prev->next = cur;
			vproc.pm_record.node_in_vin = prev;
			++vproc.pm_record.node_num;
			break;

		case OP_REMOVE:
			if (check_mb_remove(mask_id) < 0) {
				return -1;
			}
			cur = vproc.pm_record.node_in_vin;
			prev = NULL;
			while (cur) {
				if (cur->id != mask_id) {
					prev = cur;
					cur = prev->next;
					continue;
				}
				DEBUG("remove mask%d from vin list\n", cur->id);
				if (prev == NULL ) {
					// cur is head
					vproc.pm_record.node_in_vin = cur->next;
					free(cur);
					cur = vproc.pm_record.node_in_vin;
				} else {
					prev->next = cur->next;
					free(cur);
					cur = prev->next;
				}
				--vproc.pm_record.node_num;
				break;
			}
			break;

		default:
			break;
	}

	display_pm_nodes();

	for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
		update_mbstream_buffer(i, OP_UPDATE);
		pm = vproc.streampm[i];
		pm_buf = &vproc.pm_record.buffer[i];
		pm->enable = 1;
		pm->buffer_pitch = pm_buf->pitch;
		pm->buffer_height = pm_buf->height;
		pm->buffer_addr = pm_buf->addr[pm_buf->id];
		for (j = 0; j < OVERLAY_MAX_AREA_NUM; j++) {
			area = &vproc.overlay[i]->area[j];
			overlay_buf = &vproc.overlay_record.buffer[i][j];
			area->data = overlay_buf->data_addr[overlay_buf->data_id];
		}
		streams |= (1 << i);
	}

	return (ioctl_cfg_stream_pm(streams) || ioctl_cfg_stream_overlay(streams))
		? -1 : 0;
}

int operate_mbstream_color(yuv_color_t* color)
{
	int i, j;
	u32 streams = 0;
	overlay_buffer_t* overlay_buffer;
	yuv_color_t* pm_clut;

	if (init_mbstream() < 0)
		return -1;

	for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
		if (ioctl_get_stream_pm(i) < 0) {
			return -1;
		}

		vproc.streampm[i]->y = color->y;
		vproc.streampm[i]->u = color->u;
		vproc.streampm[i]->v = color->v;
		streams |= (1 << i);

		for (j = 0; j < OVERLAY_MAX_AREA_NUM; j++) {
			overlay_buffer = &vproc.overlay_record.buffer[i][j];
			pm_clut = (yuv_color_t*) overlay_buffer->clut_addr
			    +  overlay_buffer->pm_entry;
			pm_clut->y = color->y;
			pm_clut->u = color->u;
			pm_clut->v = color->v;
		}
	}

	return ioctl_cfg_stream_pm(streams);
}


int disable_pm(void)
{
	vproc.pm->enable = 0;
	return ioctl_cfg_pm();
}

int disable_stream_pm(void)
{
	int i;
	u32 streams = 0;
	for (i = 0; i < vproc.overlay_record.max_stream_num; i++) {
		vproc.streampm[i]->enable = 0;
		streams |= (1 << i);
	}
	return ioctl_cfg_stream_pm(streams);
}

