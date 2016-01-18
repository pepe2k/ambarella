/*******************************************************************************
 * pm_pixel.c
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
#include "iav_encode_drv.h"

#include "lib_vproc.h"
#include "priv.h"

#define BYTE_IN_BITS                 (8)
#define MB_IN_PIXELS                 (16)

#define OUTPUT_BITS                  (54)
#define INPUT_PIXELS                 (8)

typedef int (*update_buffer_method_t)(OP, u8*, iav_rect_ex_t*);

static update_buffer_method_t update_buffer_method = NULL;

static int G_pixel_inited;

//static pm_node_t* G_pixel_pm_list = NULL;
static int G_pixel_active_num = 0;
static int G_pixel_node_num = 0;

static u8* G_pixel_mask_addr = NULL;
static u8* G_pixel_unmask_addr = NULL;

static u8 G_pixel_clear_head[BYTE_IN_BITS];  // Clear bits from nth to 7th
static u8 G_pixel_clear_tail[BYTE_IN_BITS];  // clear bits from 0 to nth
static u8 G_pixel_unmask_head[BYTE_IN_BITS]; // UNMASK starts at nth bit
static u8 G_pixel_unmask_tail[BYTE_IN_BITS]; // UNMASK ends at nth bit

//static const u64 G_pixel_unmask_value   = 0x002082083c104107; // RAW: 0x4000
//
//static const u8 G_pixel_unmask_bytes[] = {
//		0x07, 0x41, 0x10, 0x3c, // 0x3c104107
//		0x08, 0x82, 0xe0, 0x41, // 0x41e08208
//		0x10, 0x04, 0x0f, 0x82, // 0x820f0410
//		0x20, 0x78, 0x10, 0x04, // 0x04107820
//		0xc1, 0x83, 0x20, 0x08, // 0x082083c1
//		0x1e, 0x04, 0x41, 0xf0, // 0xf041041e
//		0x20, 0x08, 0x82,       // 0x00820820
//};

static const u64 G_pixel_unmask_value = 0x003FFFFFF7FFFFFE; // 0x07FFFFFE

static const u8 G_pixel_unmask_bytes[] = {
	0xFE, 0xFF, 0xFF, 0xF7, // 0xF7FFFFFE
	0xFF, 0xFF, 0xBF, 0xFF, // 0xFFBFFFFF
	0xFF, 0xFF, 0xFD, 0xFF, // 0xFFFDFFFF
	0xFF, 0xEF, 0xFF, 0xFF, // 0xFFFFEFFF
	0x7F, 0xFF, 0xFF, 0xFF, // 0xFFFFFF7F
	0xFB, 0xFF, 0xFF, 0xDF, // 0xDFFFFFFB
	0xFF, 0xFF, 0xFF,       // 0x00FFFFFF
};

static inline int pixel_to_scaled_pm(const int x)
{
	return x * vproc.pm_info.pixel_width
	    / vproc.pm_record.buffer[0].domain_width;
}

static inline int scaled_pm_to_pixel(const int x)
{
	return x * vproc.pm_record.buffer[0].domain_width
	    / vproc.pm_info.pixel_width;
}

static inline int roundup_to_pm(const int x)
{
	return ROUND_UP(x, 8);
}

static inline int rounddown_to_pm(const int x)
{
	return ROUND_DOWN(x, 8);
}

static inline int rounddown_to_scaled_pm(const int x)
{
	return rounddown_to_pm(pixel_to_scaled_pm(x));
}

static inline int roundup_to_scaled_pm(const int x)
{
	return roundup_to_pm(pixel_to_scaled_pm(x));
}

static inline int get_next_buffer_id(void)
{
	return (vproc.pm_record.buffer[0].id >= PM_PIXEL_BUF_NUM - 1) ? 0 :
		(vproc.pm_record.buffer[0].id + 1);
}

inline int is_node_in_use(const pm_node_t* node)
{
	return !!(node->state & (1 << vproc.pm_record.buffer[0].id));
}

/*
 * Fill the prepared memory with unmask value.
 * It must be used when pixel privacy mask is disabled.
 */
static void fill_pixel_unmask_mem(void)
{
	u32 remain, unmask;
	const u8* src;
	u8* dst;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	// Fill one line
	src = G_pixel_unmask_bytes;
	dst = G_pixel_unmask_addr;
	unmask = sizeof(G_pixel_unmask_bytes);
	remain = buf->pitch - unmask;
	memcpy(dst, src, unmask);

	src = G_pixel_unmask_addr;
	dst = G_pixel_unmask_addr + unmask;
	while (remain >= unmask) {
		memcpy(dst, src, unmask);
		remain -= unmask;
		dst += unmask;
		unmask += unmask;
	}
	if (remain) {
		memcpy(dst, src, remain);
	}
	// Repeat to the remain lines
	unmask = 1;
	remain = buf->height - unmask;
	dst = G_pixel_unmask_addr + unmask * buf->pitch;
	while (remain >= unmask) {
		memcpy(dst, G_pixel_unmask_addr, unmask * buf->pitch);
		remain -= unmask;
		dst += unmask * buf->pitch;
		unmask += unmask;
	}
	if (remain) {
		memcpy(dst, G_pixel_unmask_addr, remain * buf->pitch);
	}
}

/*
 * Fill the prepared memory with mask value.
 * It must be used when pixel privacy mask is disabled.
 */
static void fill_pixel_mask_mem(void)
{
	memset(G_pixel_mask_addr, 0, vproc.pm_record.buffer[0].bytes);
}

static void locate_mem(const u32 x, u32* first_round_byte, u32* bits_in_byte)
{
	u32 x_group = x / INPUT_PIXELS;
	u32 total_bits = x_group * OUTPUT_BITS;
	u32 round_bits = ROUND_DOWN(total_bits, BYTE_IN_BITS);
	*first_round_byte = round_bits / BYTE_IN_BITS;
	*bits_in_byte = total_bits - round_bits;
	TRACE("round byte = %d, start bits = %d\n", *first_round_byte,
		*bits_in_byte);
}

inline static int is_node_unused(const pm_node_t* node)
{
	return node->state == 0;
}

inline static void set_node_in_use(pm_node_t* node, const int buffer_id)
{
	node->state |= (0x1 << buffer_id);
}

inline static void clear_node_in_use(pm_node_t* node, const int buffer_id)
{
	node->state &= ~(0x1 << buffer_id);
}

inline static int is_node_in_buffer(pm_node_t* node, const int buffer_id)
{
	return !!(node->state & (1 << buffer_id));
}

inline static void rect_input_to_scaled(iav_rect_ex_t* scaled_rect,
    const iav_rect_ex_t* rect_in_input)
{
	scaled_rect->x = rounddown_to_scaled_pm(rect_in_input->x);
	scaled_rect->width = roundup_to_scaled_pm(
		rect_in_input->x + rect_in_input->width) - scaled_rect->x;
	scaled_rect->y = rect_in_input->y;
	scaled_rect->height = rect_in_input->height;
}

inline static void rect_scaled_to_input(iav_rect_ex_t* rect_in_input,
    const iav_rect_ex_t* scaled_rect)
{
	rect_in_input->x = scaled_pm_to_pixel(scaled_rect->x);
	rect_in_input->width = scaled_pm_to_pixel(scaled_rect->width);
	rect_in_input->y = scaled_rect->y;
	rect_in_input->height = scaled_rect->height;
}

static void display_all_nodes(void)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	int i = 0, j = 0, k = 0;
	char buf[32 * 2 + 1] = { 0 };
	DEBUG(" Total node number = %d \n", G_pixel_node_num);
	while (node) {
		memset(buf, 0, sizeof(buf));
		for (j = 0, k = 0; j < sizeof(buf) - 1; ++j) {
			if ((node->state & (0x1 << j))) {
				buf[k++] = '0' + j;
				buf[k++] = ',';
			}
		}
		DEBUG("\t[%d] mask%d (w %d, h %d, x %d, y %d), state = %d, buffers = %s\n",
			i++, node->id, node->rect.width, node->rect.height, node->rect.x,
			node->rect.y, node->state, buf);
		node = node->next;
	}
}

static void display_active_nodes(void)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	int i = 0;
	iav_rect_ex_t rect_in_input;
	DEBUG("Total mask number = %d\n", G_pixel_active_num);
	while (node) {
		if (is_node_in_use(node)) {
			rect_scaled_to_input(&rect_in_input, &node->rect);
			DEBUG("\t[%d] mask%d (w %d, h %d, x %d, y %d)\n", i++, node->id,
			    rect_in_input.width, rect_in_input.height, rect_in_input.x,
			    rect_in_input.y);
		}
		node = node->next;
	}
}

static int fill_mem(u8* src, u8* dst, iav_rect_ex_t* rect)
{
	iav_gdma_copy_ex_t gdma;
	int i = 0;
	u32 offset = rect->x + rect->y * vproc.pm_record.buffer[0].pitch;
	memset(&gdma, 0, sizeof(gdma));
	gdma.dst_pitch = vproc.pm_record.buffer[0].pitch;
	gdma.src_pitch = vproc.pm_record.buffer[0].pitch;
	gdma.width = rect->width;
	gdma.height = rect->height;
	gdma.src_mmap_type = IAV_MMAP_PRIVACYMASK;
	gdma.dst_mmap_type = IAV_MMAP_PRIVACYMASK;
	gdma.usr_src_addr = (u32) (src + offset);
	gdma.usr_dst_addr = (u32) (dst + offset);
	TRACE("GDMA copy : pitch %d, x %d, y %d, width %d, height %d => size = "
		"0x%x\n", gdma.src_pitch, rect->x, rect->y, gdma.width, gdma.height,
		gdma.width * gdma.height);
	TRACE("GDMA copy : src_buf = 0x%x, dst_buf = 0x%x, offset = 0x%x, "
		"usr_src_addr = 0x%x, usr_dst_addr = 0x%x\n", (u32 )src, (u32 )dst,
		offset, gdma.usr_src_addr, gdma.usr_dst_addr);

	//If address is not aligned to 4 bytes, use memcpy instead of gdma copy.
	if ((gdma.usr_src_addr & 0x3) || (gdma.usr_dst_addr & 0x3)) {
		for (i = 0; i < gdma.height; i++) {
			memcpy((u8 *)gdma.usr_dst_addr, (u8 *)gdma.usr_src_addr, gdma.width);
			gdma.usr_src_addr += gdma.src_pitch;
			gdma.usr_dst_addr += gdma.dst_pitch;
		}
		TRACE("usr_dst_addr[0x%x] usr_dst_addr[0x%x] not align to 4 bytes, \
			use memcpy instead!\n", gdma.usr_src_addr, gdma.usr_dst_addr);
		return 0;
	} else {
		return ioctl_gdma_copy(&gdma);
	}
}

static int update_pixel_buffer(OP op, u8* dst, iav_rect_ex_t* rect)
{
	u32 start_bits = 0, end_bits;
	u32 start_byte = 0, end_byte = 0, head_byte = 0, tail_byte = 0;
	u8* src = NULL, *addr = NULL;
	int i;
	iav_rect_ex_t mem_rect = { 0 };

	locate_mem(rect->x, &head_byte, &start_bits);
	locate_mem(rect->x + rect->width, &tail_byte, &end_bits);

	TRACE("head byte %d, tail byte %d, start bits %d, end bits %d\n",
		head_byte, tail_byte, start_bits, end_bits);

	addr = dst + rect->y * vproc.pm_record.buffer[0].pitch; // Address the head and tail byte.
	// handle the head and tail byte.
	for (i = 0; i < rect->height;
			++i, addr += vproc.pm_record.buffer[0].pitch) {
		// clear the head / tail bits
		TRACE("before clear: head  0x%2x    tail 0x%2x\n", *(addr + head_byte),
			*(addr + tail_byte));
		*(addr + head_byte) &= G_pixel_clear_head[start_bits];
		*(addr + tail_byte) &= G_pixel_clear_tail[end_bits];
		TRACE("clear       : head  0x%2x    tail 0x%2x\n",
			G_pixel_clear_head[start_bits], G_pixel_clear_tail[end_bits]);
		TRACE("after clear : head  0x%2x    tail 0x%2x\n", *(addr + head_byte),
			*(addr + tail_byte));
		if (op == OP_REMOVE) {
			// set unmask value
			*(addr + head_byte) |= G_pixel_unmask_head[start_bits];
			*(addr + tail_byte) |= G_pixel_unmask_tail[end_bits];
			TRACE("unmask      : head  0x%2x    tail 0x%2x\n",
				G_pixel_unmask_head[start_bits], G_pixel_unmask_tail[end_bits]);
			TRACE("after unmask: head  0x%2x    tail 0x%2x\n",
				*(addr + head_byte), *(addr + tail_byte));
		}
	}

	// Copy the memory between the head and tail byte.
	start_byte = head_byte + 1;         // skip the head byte
	end_byte = tail_byte;
	src = (op == OP_ADD) ? G_pixel_mask_addr : G_pixel_unmask_addr;

	mem_rect.x = start_byte;
	mem_rect.width = (end_byte > start_byte ? end_byte - start_byte : 0);
	mem_rect.y = rect->y;
	mem_rect.height = rect->height;

	if (fill_mem(src, dst, &mem_rect) < 0) {
		return -1;
	}

	return 0;
}

static void draw_mask_to_buffer(const int buf_id, pm_node_t* node)
{
	update_buffer_method(OP_ADD, vproc.pm_record.buffer[0].addr[buf_id],
		&node->rect);
	set_node_in_use(node, buf_id);
	clear_pm_node_from_redraw(node);
}

static void erase_mask_from_buffer(const int buf_id, pm_node_t* node)
{
	pm_node_t* cur = vproc.pm_record.node_in_vin;

	update_buffer_method(OP_REMOVE, vproc.pm_record.buffer[0].addr[buf_id],
	    &node->rect);
	clear_node_in_use(node, buf_id);

	// Mark the masks overlapped with the removed one.
	while (cur) {
		if (is_node_in_buffer(cur, buf_id)
		    && is_rect_overlap(&cur->rect, &node->rect)) {
			set_pm_node_to_redraw(cur);
			DEBUG("redraw marked: mask%d\n", node->id);
		}
		cur = cur->next;
	}
}

int init_pixel(void)
{
	int i, total_size;
	int resi_bits = OUTPUT_BITS % BYTE_IN_BITS;
	int round_bytes = OUTPUT_BITS / BYTE_IN_BITS;
	u64 shift_unmask;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	if (unlikely(!G_pixel_inited)) {
		do {
			if (ioctl_map_pm() < 0) {
				break;
			}
			if (vproc.pm_info.domain == IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT) {
				if (ioctl_get_srcbuf_setup() < 0) {
					break;
				}
				buf->domain_width = vproc.srcbuf_setup.pre_main_input.width;
				buf->domain_height = vproc.srcbuf_setup.pre_main_input.height;
			} else {
				if (ioctl_get_vin_info() < 0) {
					break;
				}
				buf->domain_width = vproc.vin.width;
				buf->domain_height = vproc.vin.height;
			}
			buf->pitch = vproc.pm_info.buffer_pitch;
			buf->width = roundup_to_pm(vproc.pm_info.pixel_width) * OUTPUT_BITS
			    / BYTE_IN_BITS;
			buf->height = buf->domain_height;
			buf->id = 0;
			buf->bytes = buf->pitch * buf->height;

			DEBUG("domain %dx%d, buffer pitch %d, width %d, height %d, "
				"size 0x%x\n", buf->domain_width, buf->domain_height,
			    buf->pitch, buf->width, buf->height, buf->bytes);

			total_size = buf->bytes * (PM_PIXEL_BUF_NUM + 2); // 2 for mask/unmask

			if (unlikely(total_size > vproc.pm_mem.length)) {
				ERROR("Memory size [0x%x] for pixel privacy mask must "
					"be no less than [0x%x].\n", vproc.pm_mem.length,
				    total_size);
				break;
			}

			for (i = 0; i < BYTE_IN_BITS; ++i) {
				G_pixel_clear_head[i] = (0xff >> (BYTE_IN_BITS - i));
				G_pixel_clear_tail[i] = (0xff << i);
				G_pixel_unmask_head[i] = (*(u8*) &G_pixel_unmask_value << i);
				shift_unmask = (
				    (i < resi_bits) ?
				        (G_pixel_unmask_value >> (resi_bits - i)) :
				        (G_pixel_unmask_value << (i - resi_bits)));
				G_pixel_unmask_tail[i] = *((u8*) &shift_unmask + round_bytes);

				TRACE("clear head %d = 0x%x\n", i, G_pixel_clear_head[i]);
				TRACE("clear tail %d = 0x%x\n", i, G_pixel_clear_tail[i]);
				TRACE("unmask head %d = 0x%x\n", i, G_pixel_unmask_head[i]);
				TRACE("unmask tail %d = 0x%x\n", i, G_pixel_unmask_tail[i]);
			}

			G_pixel_unmask_addr = vproc.pm_mem.addr;
			G_pixel_mask_addr = G_pixel_unmask_addr + buf->bytes;

			DEBUG("unmask addr 0x%x, mask addr 0x%x,\n",
			    (u32 )G_pixel_unmask_addr, (u32 )G_pixel_mask_addr);

			fill_pixel_unmask_mem();
			fill_pixel_mask_mem();

			for (i = 0; i < PM_PIXEL_BUF_NUM; ++i) {
				buf->addr[i] = G_pixel_mask_addr + (i + 1) * buf->bytes;
				memcpy(buf->addr[i], G_pixel_unmask_addr, buf->bytes);
				DEBUG("buffer_%d addr 0x%x\n", i, (u32 )buf->addr[i]);
			}
			update_buffer_method = update_pixel_buffer;
			G_pixel_inited = 1;

		} while (0);
	}

	return G_pixel_inited ? 0 : -1;
}

int deinit_pixel(void)
{
	pm_node_t* node, *next;

	if (G_pixel_inited) {
		node = vproc.pm_record.node_in_vin;
		DEBUG("free total %d masks\n", G_pixel_node_num);
		while (node) {
			DEBUG("free mask [%d]\n", node->id);
			next = node->next;
			free(node);
			node = next;
		}
		G_pixel_active_num = 0;
		G_pixel_node_num = 0;
		vproc.pm_record.node_in_vin = NULL;
		update_buffer_method = NULL;
		G_pixel_inited = 0;
		DEBUG("done\n");
	}
	return G_pixel_inited ? -1 : 0;
}

static int check_pixel_add(const int id, const iav_rect_ex_t* rect)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	if (!rect) {
		ERROR("Privacy mask rect is NULL.\n");
		return -1;
	}

	if (!rect->width || !rect->height) {
		ERROR("Privacy mask rect %ux%u cannot be zero.\n", rect->width,
		    rect->height);
		return -1;
	}

	if ((rect->x + rect->width > buf->domain_width)
	    || (rect->y + rect->height > buf->domain_height)) {
		ERROR("Privacy mask rect %ux%u offset %ux%u is out of picture"
			" %ux%u.\n", rect->width, rect->height, rect->x, rect->y,
			buf->domain_width, buf->domain_height);
		return -1;
	}

	if ((rect->x & 0x7) || (rect->width & 0x7)) {
		ERROR("pixel privacy mask x [%d] and width [%d] must be multiple of "
			"8.", rect->x, rect->width);
		return -1;
	}

	while (node) {
		if (is_node_in_use(node)) {
			if (id == node->id) {
				ERROR("privacy mask id [%d] existed.\n", id);
				return -1;
			}
		}
		node = node->next;
	}

	return 0;
}

static int check_pixel_remove(const int id)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	while (node) {
		if (is_node_in_use(node) && (id == node->id)) {
			return 0;
		}
		node = node->next;
	}
	ERROR("privacy mask [%d] does not exist.\n", id);
	return -1;
}

static int operate_pixel(const OP op, const int id, iav_rect_ex_t* rect)
{
	int buf_id, is_in_use, is_in_buffer;
	pm_node_t* cur, *prev;
	iav_rect_ex_t scaled_rect;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	switch (op) {
	case OP_ADD:
		if (check_pixel_add(id, rect) < 0)
			return -1;
		break;
	case OP_REMOVE:
		if (check_pixel_remove(id) < 0)
			return -1;
		break;
	default:
		break;
	}

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;
	buf_id = get_next_buffer_id();
	is_in_use = 0;
	is_in_buffer = 0;

	while (cur) {
		is_in_use = is_node_in_use(cur);
		is_in_buffer = is_node_in_buffer(cur, buf_id);
		if (is_in_use != is_in_buffer) {
			DEBUG("mismatch detected : mask%d, state = %d, %s in use, "
				"%s in buffer\n", cur->id, cur->state, is_in_use ? "" : "not",
				is_in_buffer ? "" : "not");
			// do nothing if cur id == remove id
			if (is_in_buffer || cur->id != id) {
				if (is_in_use) {
					DEBUG("mismatch action: add mask%d\n", cur->id);
					draw_mask_to_buffer(buf_id, cur);
				} else {
					DEBUG("mismatch action: remove mask%d\n", cur->id);
					erase_mask_from_buffer(buf_id, cur);
				}
				DEBUG("mismatch resolved : mask%d, state = %d\n", cur->id,
					cur->state);
			} else if (cur->id == id) {
				erase_mask_from_buffer(buf_id, cur);
				--G_pixel_active_num;
			}
		} else if (cur->id == id) {
			DEBUG("remove found : mask%d, state = %d\n", cur->id, cur->state);
			erase_mask_from_buffer(buf_id, cur);
			erase_mask_from_buffer(buf->id, cur);
			DEBUG("remove done : mask%d, state = %d\n", cur->id, cur->state);
			--G_pixel_active_num;
		}

		if (is_node_unused(cur)) {
			DEBUG("delete unused mask%d\n", cur->id);
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
			--G_pixel_node_num;
		} else {
			prev = cur;
			cur = prev->next;
		}
	}
	// Redraw nodes that are marked.
	cur = vproc.pm_record.node_in_vin;
	while (cur) {
		if (is_pm_node_to_redraw(cur)) {
			draw_mask_to_buffer(buf_id, cur);
			DEBUG("redraw done: mask%d\n", cur->id);
		}
		cur = cur->next;
	}

	display_all_nodes();

	if (op == OP_ADD) {
		rect_input_to_scaled(&scaled_rect, rect);
		cur = create_pm_node(id, &scaled_rect);
		cur->next = vproc.pm_record.node_in_vin;
		vproc.pm_record.node_in_vin = cur;
		++G_pixel_node_num;
		++G_pixel_active_num;
		draw_mask_to_buffer(buf_id, cur);
		DEBUG("append the new mask : id = %d, state = %d\n", cur->id,
			cur->state);
	}
	buf->id = buf_id;

	display_all_nodes();
	display_active_nodes();

	vproc.pm->enable = 1;
	vproc.pm->buffer_addr = buf->addr[buf->id];
	vproc.pm->buffer_pitch = buf->pitch;
	vproc.pm->buffer_height = buf->height;

	return ioctl_cfg_pm();
}

int operate_pixel_pm(const OP op, const int id, iav_rect_ex_t* rect)
{
	if (init_pixel() < 0)
		return -1;
	return operate_pixel(op, id, rect);
}

static const u16 G_pixelraw_unmask_value = 0x4000;
static const u16 G_pixelraw_mask_value = 0x0;
static int G_pixelraw_inited = 0;

static void fill_pixelraw_unmask_mem(void)
{
	u32 remain, unmask;
	const u8* src;
	u8* dst;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	// Fill one line
	src = (u8*) &G_pixelraw_unmask_value;
	dst = G_pixel_unmask_addr;
	unmask = sizeof(G_pixelraw_unmask_value);
	remain = buf->pitch - unmask;
	memcpy(dst, src, unmask);

	src = G_pixel_unmask_addr;
	dst = G_pixel_unmask_addr + unmask;
	while (remain >= unmask) {
		memcpy(dst, src, unmask);
		remain -= unmask;
		dst += unmask;
		unmask += unmask;
	}
	if (remain) {
		memcpy(dst, src, remain);
	}
	// Repeat to the remain lines
	unmask = 1;
	remain = buf->height - unmask;
	dst = G_pixel_unmask_addr + unmask * buf->pitch;
	while (remain >= unmask) {
		memcpy(dst, G_pixel_unmask_addr, unmask * buf->pitch);
		remain -= unmask;
		dst += unmask * buf->pitch;
		unmask += unmask;
	}
	if (remain) {
		memcpy(dst, G_pixel_unmask_addr, remain * buf->pitch);
	}
}

/*
 * Fill the prepared memory with mask value.
 * It must be used when pixel privacy mask is disabled.
 */
static void fill_pixelraw_mask_mem(void)
{
	memset(G_pixel_mask_addr, 0, vproc.pm_record.buffer[0].bytes);
}

static int update_pixelraw_buffer(OP op, u8* dst, iav_rect_ex_t* rect)
{
	u8* src = NULL;
	iav_rect_ex_t mem_rect = { 0 };

	if (op == OP_ADD) {
		src = G_pixel_mask_addr;
	} else if (op == OP_REMOVE) {
		src = G_pixel_unmask_addr;
	}

	mem_rect.x = rect->x * sizeof(G_pixelraw_mask_value);
	mem_rect.y = rect->y;
	mem_rect.width = rect->width * sizeof(G_pixelraw_mask_value);
	mem_rect.height = rect->height;

	if (fill_mem(src, dst, &mem_rect) < 0) {
		return -1;
	}
	return 0;
}

int init_pixelraw(void)
{
	int i;
	u32 total_size;
	pm_buffer_t* buf = &vproc.pm_record.buffer[0];

	if (unlikely(!G_pixelraw_inited)) {
		do {
			if (ioctl_map_pm() < 0) {
				break;
			}
			if (vproc.pm_info.domain == IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT) {
				if (ioctl_get_srcbuf_setup() < 0) {
					break;
				}
				buf->domain_width = vproc.srcbuf_setup.pre_main_input.width;
				buf->domain_height = vproc.srcbuf_setup.pre_main_input.height;
			} else {
				if (ioctl_get_vin_info() < 0) {
					break;
				}
				buf->domain_width = vproc.vin.width;
				buf->domain_height = vproc.vin.height;
			}
			buf->pitch = vproc.pm_info.buffer_pitch;
			buf->width = roundup_to_pm(vproc.pm_info.pixel_width);
			buf->height = buf->domain_height;
			buf->id = 0;
			buf->bytes = buf->pitch * buf->height;

			DEBUG("domain %dx%d, buffer pitch %d, width %d, height %d, "
				"size 0x%x\n", buf->domain_width, buf->domain_height,
			    buf->pitch, buf->width, buf->height, buf->bytes);

			total_size = buf->bytes * (PM_PIXEL_BUF_NUM + 2); // 2 for mask/unmask

			if (unlikely(total_size > vproc.pm_mem.length)) {
				ERROR("Memory size [0x%x] for pixel privacy mask must "
					"be no less than [0x%x].\n", vproc.pm_mem.length,
				    total_size);
				break;
			}
			G_pixel_unmask_addr = vproc.pm_mem.addr;
			G_pixel_mask_addr = G_pixel_unmask_addr + buf->bytes;

			DEBUG("unmask addr 0x%x, mask addr 0x%x,\n",
			    (u32 )G_pixel_unmask_addr, (u32 )G_pixel_mask_addr);

			fill_pixelraw_unmask_mem();
			fill_pixelraw_mask_mem();

			for (i = 0; i < PM_PIXEL_BUF_NUM; ++i) {
				buf->addr[i] = G_pixel_mask_addr + (i + 1) * buf->bytes;
				memcpy(buf->addr[i], G_pixel_unmask_addr, buf->bytes);
				DEBUG("buffer_%d addr 0x%x\n", i, (u32 )buf->addr[i]);
			}
			update_buffer_method = update_pixelraw_buffer;
			G_pixelraw_inited = 1;
		} while (0);
	}
	return G_pixelraw_inited ? 0 : -1;
}

int deinit_pixelraw(void)
{
	pm_node_t* node, *next;

	if (G_pixelraw_inited) {
		node = vproc.pm_record.node_in_vin;
		DEBUG("free total %d masks\n", G_pixel_node_num);
		while (node) {
			DEBUG("free mask [%d]\n", node->id);
			next = node->next;
			free(node);
			node = next;
		}
		G_pixel_active_num = 0;
		G_pixel_node_num = 0;
		vproc.pm_record.node_in_vin = NULL;
		update_buffer_method = NULL;
		G_pixel_inited = 0;
		DEBUG("done\n");
	}
	return G_pixelraw_inited ? -1 : 0;
}

int operate_pixelraw_pm(const OP op, const int id, iav_rect_ex_t* rect)
{
	if (init_pixelraw() < 0)
		return -1;
	return operate_pixel(op, id, rect);
}

