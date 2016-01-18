/*
 * iav_bufcap.c
 *
 * History:
 *	2012/02/06 - [Jian Tang] created file
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
#include "iav_api.h"
#include "utils.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "iav_capture.h"
#include "iav_bufcap.h"
#include "iav_pts.h"
#include "iav_encode.h"

#define	DEBUG_ENABLE		(0)
#define	RAW_PIXEL_WIDTH_IN_BYTE		(2)

typedef struct raw_cap_info_s {
	u32	addr;
	u16	width;
	u16	height;
	u16	pitch;
	u16	reserved[3];
	u64	raw_dsp_pts;
} raw_cap_info_t;

typedef struct source_buffer_info_s {
	u32	y_addr;
	u32	uv_addr;
	u32	pitch;
	u32	seqnum;
	u64	mono_pts;
	u64	dsp_pts;
} source_buffer_info_t;

typedef struct me1_buffer_info_s {
	u32	addr;
	u32	pitch;
	u32	seqnum;
	u32	reserved;
	u64	mono_pts;
	u64	dsp_pts;
} me1_buffer_info_t;

typedef struct iav_bufcap_info_s {
	u64	mono_pts;
	u64	sec2_out_mono_pts;
	u64	dsp_pts;
	u64	sec2_out_dsp_pts;
	u64	raw_dsp_pts;
	u8	frm_total_num;
	u8	write_index;
	u8	read_index;
	u8	raw_cap_data_valid;
	u8	buffer_data_valid;
	u8	me1_data_valid;
	u8	preview_b_tatal_num;
	u8	reserved;
	u32	seq_num;
	u32	first_preview_b_addr;

	raw_cap_info_t *raw_cap_info;
	iav_completion_t raw_cap_compl;

	source_buffer_info_t *buf_info[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_completion_t buffer_compl;

	me1_buffer_info_t *me1_info[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_completion_t me1_buffer_compl;

	source_buffer_info_t *vca_info;
} iav_bufcap_info_t;

static iav_bufcap_info_t G_iav_bufcap;

int iav_bufcap_init(void)
{
	u8 i, * addr = NULL;
	u32 raw_size, buf_size, me1_size, vca_size, total_size;

	memset(&G_iav_bufcap, 0, sizeof(iav_bufcap_info_t));

	raw_size = sizeof(raw_cap_info_t) * IAV_BUFCAP_MAX;
	buf_size = sizeof(source_buffer_info_t) * IAV_BUFCAP_MAX;
	me1_size = sizeof(me1_buffer_info_t) * IAV_BUFCAP_MAX;
	vca_size = sizeof(source_buffer_info_t) * IAV_BUFCAP_MAX;
	total_size = PAGE_ALIGN(raw_size + (buf_size + me1_size) *
		IAV_ENCODE_SOURCE_TOTAL_NUM + vca_size);

	addr = kzalloc(total_size, GFP_KERNEL);
	if (!addr) {
		iav_error("%s: out of memory for buffer capture.\n", __func__);
		return -ENOMEM;
	}

	G_iav_bufcap.raw_cap_info = (raw_cap_info_t *)addr;
	addr += raw_size;

	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		G_iav_bufcap.buf_info[i] = (source_buffer_info_t *)(addr +
			i * buf_size);
		G_iav_bufcap.me1_info[i] = (me1_buffer_info_t *)(addr +
			buf_size * IAV_ENCODE_SOURCE_TOTAL_NUM + i * me1_size);
	}
	G_iav_bufcap.vca_info = (source_buffer_info_t *)(addr +
		(buf_size + me1_size) * IAV_ENCODE_SOURCE_TOTAL_NUM);

	iav_init_compl(&G_iav_bufcap.raw_cap_compl);
	iav_init_compl(&G_iav_bufcap.buffer_compl);
	iav_init_compl(&G_iav_bufcap.me1_buffer_compl);

	return 0;
}

void iav_bufcap_reset(void)
{
	int i;

	memset(G_iav_bufcap.raw_cap_info, 0,
		sizeof(raw_cap_info_t) * IAV_BUFCAP_MAX);
	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		memset(G_iav_bufcap.buf_info[i], 0,
			sizeof(source_buffer_info_t) * IAV_BUFCAP_MAX);
		memset(G_iav_bufcap.me1_info[i], 0,
			sizeof(me1_buffer_info_t) * IAV_BUFCAP_MAX);
	}
	memset(G_iav_bufcap.vca_info, 0,
		sizeof(source_buffer_info_t) * IAV_BUFCAP_MAX);

	G_iav_bufcap.read_index = 0;
	G_iav_bufcap.write_index = 0;
	G_iav_bufcap.first_preview_b_addr = 0;
	G_iav_bufcap.preview_b_tatal_num = 0;
	G_iav_bufcap.frm_total_num = IAV_BUFCAP_MAX;
	G_iav_bufcap.raw_cap_data_valid = 0;
	G_iav_bufcap.buffer_data_valid = 0;
	G_iav_bufcap.me1_data_valid = 0;
}

static inline int check_before_dump_buffer(iav_context_t * context)
{
	if (!is_map_dsp_partition()) {
		if (context->dsp.user_start == NULL) {
			iav_error("DSP memory is NOT mapped!\n");
			return -1;
		}
	}

	if (is_iav_state_idle()) {
		iav_error("Cannot dump DSP buffers in IDLE state!\n");
		return -1;
	}
	return 0;
}

static u8 *get_dsp_user_addr(iav_context_t * context, u32 dsp_addr, int partition)
{
	u32 phys_start = 0, size = 0;
	iav_get_dsp_partition(partition, &phys_start, &size);
	return DSP_TO_PHYS(dsp_addr) - phys_start + context->dsp_partition[partition].user_start;
}

static inline void save_raw_cap_info(VCAP_STRM_REPORT * msg, u32 write_index)
{
	raw_cap_info_t * pinfo = NULL;
	if (is_invalid_dsp_addr(msg->raw_capture_dram_addr)) {
		return ;
	}
	pinfo = &G_iav_bufcap.raw_cap_info[write_index];
	pinfo->addr = msg->raw_capture_dram_addr;
	pinfo->pitch = msg->raw_cap_buf_pitch;
	pinfo->width = msg->raw_cap_buf_width / RAW_PIXEL_WIDTH_IN_BYTE;
	pinfo->height = msg->raw_cap_buf_height;

	G_iav_bufcap.raw_cap_info[write_index].raw_dsp_pts = G_iav_bufcap.raw_dsp_pts;
	G_iav_bufcap.raw_cap_data_valid = 1;

	notify_waiters(&G_iav_bufcap.raw_cap_compl);
}

inline void prepare_vout_b_letter_box(void)
{
	#define	YUV_BLACK_VALUE_Y	0x0
	#define	YUV_BLACK_VALUE_UV	0x80
	int top_extra_line;
	int vout_height, video_height;
	u8 *remap_addr = NULL;
	u8 *tmp_addr = NULL;
	int index;
	u32 phy_addr, remap_size;
	u32 pitch, vout_b_size;

	/* Vout B with letter boxing config when mixer B is off. */
	vout_height = G_iav_info.pvoutinfo[1]->active_mode.video_size.vout_height;
	video_height = G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height;
	top_extra_line =
		G_iav_info.pvoutinfo[1]->active_mode.video_offset.offset_y ?
		(G_iav_info.pvoutinfo[1]->active_mode.video_offset.offset_y + 1) : 0;
	pitch = G_iav_bufcap.buf_info[IAV_ENCODE_SOURCE_THIRD_BUFFER][0].pitch;
	vout_b_size = pitch * vout_height;
	phy_addr = DSP_TO_PHYS(G_iav_bufcap.first_preview_b_addr) -
		pitch * top_extra_line;
	remap_size = (vout_b_size << 1) * G_iav_bufcap.preview_b_tatal_num;
	remap_addr = (u8 *)ioremap_nocache(phy_addr, remap_size);
	for (index = 0, tmp_addr = remap_addr;
		index < G_iav_bufcap.preview_b_tatal_num;
		index++, tmp_addr += (vout_b_size << 1)) {
		memset(tmp_addr, YUV_BLACK_VALUE_Y, vout_b_size);
		memset(tmp_addr + vout_b_size, YUV_BLACK_VALUE_UV, vout_b_size);
	}
	iounmap(remap_addr);

	return;
}

static inline void save_source_buffer_info(VCAP_STRM_REPORT * msg, u32 index)
{
	u32 seq_num = G_iav_bufcap.seq_num;
	source_buffer_info_t *yuv_buffer = NULL;

	yuv_buffer = &G_iav_bufcap.buf_info[IAV_ENCODE_SOURCE_MAIN_BUFFER][index];
	yuv_buffer->y_addr = msg->main_pict_luma_addr;
	yuv_buffer->uv_addr = msg->main_pict_chroma_addr;
	yuv_buffer->pitch = msg->main_pict_dram_pitch;
	yuv_buffer->seqnum = seq_num;

	yuv_buffer = &G_iav_bufcap.buf_info[IAV_ENCODE_SOURCE_SECOND_BUFFER][index];
	yuv_buffer->y_addr = msg->PIP_luma_addr;
	yuv_buffer->uv_addr = msg->PIP_chroma_addr;
	yuv_buffer->pitch = msg->pip_dram_pitch;
	yuv_buffer->seqnum = seq_num;

	yuv_buffer = &G_iav_bufcap.buf_info[IAV_ENCODE_SOURCE_THIRD_BUFFER][index];
	yuv_buffer->y_addr = msg->prev_B_luma_addr;
	yuv_buffer->uv_addr = msg->prev_B_chroma_addr;
	yuv_buffer->pitch = msg->prev_B_dram_pitch;
	yuv_buffer->seqnum = seq_num;

	if (!G_iav_obj.vout_b_update_done) {
		if (G_iav_bufcap.first_preview_b_addr == 0) {
			G_iav_bufcap.first_preview_b_addr = yuv_buffer->y_addr;
		} else if ((G_iav_bufcap.first_preview_b_addr == yuv_buffer->y_addr) &&
			(index > 3)) {
			G_iav_bufcap.preview_b_tatal_num = index;
			G_iav_obj.vout_b_update_done = 1;
			wake_up_interruptible(&G_iav_obj.vcap_wq);
		}
	}

	yuv_buffer = &G_iav_bufcap.buf_info[IAV_ENCODE_SOURCE_FOURTH_BUFFER][index];
	yuv_buffer->y_addr = msg->prev_A_luma_addr;
	yuv_buffer->uv_addr = msg->prev_A_chroma_addr;
	yuv_buffer->pitch = msg->prev_A_dram_pitch;
	yuv_buffer->seqnum = seq_num;

#if DEBUG_ENABLE
	yuv_buffer = &G_iav_bufcap.buf_info[1][index];
	iav_printk("SAVE PREVIEW C [%d] : y_addr 0x%08x, uv_addr 0x%08x, pitch %d.\n",
			index, yuv_buffer->y_addr, yuv_buffer->uv_addr, yuv_buffer->pitch);
	yuv_buffer = &G_iav_bufcap.buf_info[2][index];
	iav_printk("SAVE PREVIEW B : y_addr 0x%08x, uv_addr 0x%08x, pitch %d.\n",
			yuv_buffer->y_addr, yuv_buffer->uv_addr, yuv_buffer->pitch);
	yuv_buffer = &G_iav_bufcap.buf_info[3][index];
	iav_printk("SAVE PREVIEW A : y_addr 0x%08x, uv_addr 0x%08x, pitch %d.\n",
			yuv_buffer->y_addr, yuv_buffer->uv_addr, yuv_buffer->pitch);
#endif
}

static inline void save_me1_buffer_info(VCAP_STRM_REPORT_EXT * msg, u32 index)
{
	u32 seq_num = G_iav_bufcap.seq_num;
	me1_buffer_info_t * me1 = NULL;

	me1 = &G_iav_bufcap.me1_info[IAV_ENCODE_SOURCE_MAIN_BUFFER][index];
	me1->addr = msg->main_pict_me1_addr;
	me1->pitch = msg->main_pict_me1_pitch;
	me1->seqnum = seq_num;

	if (is_buf_type_enc(IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
		me1 = &G_iav_bufcap.me1_info[IAV_ENCODE_SOURCE_SECOND_BUFFER][index];
		me1->addr = msg->prev_C_me1_addr;
		me1->pitch = msg->prev_C_me1_pitch;
		me1->seqnum = seq_num;
	}

	if (is_buf_type_enc(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
		me1 = &G_iav_bufcap.me1_info[IAV_ENCODE_SOURCE_THIRD_BUFFER][index];
		me1->addr = msg->prev_B_me1_addr;
		me1->pitch = msg->prev_B_me1_pitch;
		me1->seqnum = seq_num;
	}

	if (is_buf_type_enc(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
		me1 = &G_iav_bufcap.me1_info[IAV_ENCODE_SOURCE_FOURTH_BUFFER][index];
		me1->addr = msg->prev_A_me1_addr;
		me1->pitch = msg->prev_A_me1_pitch;
		me1->seqnum = seq_num;
	}
}

static inline void save_vca_buffer_info(VCAP_STRM_REPORT_EXT * msg, u32 index)
{
	u8 vca_src_id = G_iav_vcap.vca_src_id;
	source_buffer_info_t * vca = &G_iav_bufcap.vca_info[index];

	if (vca_src_id && !is_buf_type_off(vca_src_id)) {
		vca->y_addr = msg->prev_cp_luma_addr;
		vca->uv_addr = msg->prev_cp_chroma_addr;
		vca->pitch = G_iav_bufcap.buf_info[vca_src_id][index].pitch;
		vca->seqnum = G_iav_bufcap.seq_num;
	}
}

static inline void update_pts_and_readers(u32 index)
{
	int i;
	u64 mono_pts1 = G_iav_bufcap.sec2_out_mono_pts;
	u64 mono_pts2 = G_iav_bufcap.mono_pts;
	u64 dsp_pts1 = G_iav_bufcap.sec2_out_dsp_pts;
	u64 dsp_pts2 = G_iav_bufcap.dsp_pts;
	source_buffer_info_t * yuv = NULL;
	me1_buffer_info_t * me1 = NULL;

	for (i = IAV_ENCODE_SOURCE_BUFFER_FIRST;
			i < IAV_ENCODE_SOURCE_BUFFER_LAST; ++i) {
		yuv = &G_iav_bufcap.buf_info[i][index];
		yuv->mono_pts = G_iav_obj.source_buffer[i].unwarp ? mono_pts1 : mono_pts2;
		yuv->dsp_pts = G_iav_obj.source_buffer[i].unwarp ? dsp_pts1 : dsp_pts2;
		me1 = &G_iav_bufcap.me1_info[i][index];
		me1->mono_pts = yuv->mono_pts;
		me1->dsp_pts = yuv->dsp_pts;
	}
	G_iav_bufcap.vca_info[index].mono_pts =
		G_iav_bufcap.buf_info[G_iav_vcap.vca_src_id][index].mono_pts;
	G_iav_bufcap.vca_info[index].dsp_pts =
		G_iav_bufcap.buf_info[G_iav_vcap.vca_src_id][index].dsp_pts;
	G_iav_bufcap.buffer_data_valid = 1;
	notify_waiters(&G_iav_bufcap.buffer_compl);

	G_iav_bufcap.me1_data_valid = 1;
	notify_waiters(&G_iav_bufcap.me1_buffer_compl);

#if DEBUG_ENABLE
	iav_printk("mono_pts: sec2 [%llu], mctf [%llu] dsp_pts: sec2 [%llu], mctf [%llu].\n",
			mono_pts1, mono_pts2, dsp_pts1, dsp_pts2);
	iav_printk("VCA buffer Y [0x%x], UV [0x%x], pitch [%d], mono_pts [%llu] dsp_pts [%llu].\n",
		G_iav_bufcap.vca_info[index].y_addr,
		G_iav_bufcap.vca_info[index].uv_addr,
		G_iav_bufcap.vca_info[index].pitch,
		G_iav_bufcap.vca_info[index].mono_pts,
		G_iav_bufcap.vca_info[index].dsp_pts);
#endif
}

static void read_yuv_info(iav_context_t * context, u32 src,
	source_buffer_info_t * buf_info, iav_yuv_cap_t * yuv)
{
	u32 w, h;
	int partition = partition_id_to_index(IAV_DSP_PARTITION_MAIN_CAPTURE);
	IAV_YUV_DATA_FORMAT format = IAV_YUV_420_FORMAT;

	if (is_warp_mode()) {
		partition = partition_id_to_index(IAV_DSP_PARTITION_POST_MAIN);
	}
	w = G_iav_obj.source_buffer[src].size.width;
	h = G_iav_obj.source_buffer[src].size.height;
	switch (src) {
	case IAV_ENCODE_SOURCE_SECOND_BUFFER:
		if (buf_info->pitch > ALIGN(MAX_WIDTH_FOR_2ND, 32)) {
			buf_info->pitch = 0;
		}
		partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_C);
		break;
	case IAV_ENCODE_SOURCE_THIRD_BUFFER:
		if (is_buf_type_prev(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
			if (G_iav_obj.system_setup_info->vout_swap) {
				w = G_iav_info.pvoutinfo[0]->active_mode.video_size.video_width;
				h = G_iav_info.pvoutinfo[0]->active_mode.video_size.video_height;
			} else {
				w = G_iav_info.pvoutinfo[1]->active_mode.video_size.video_width;
				h = G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height;
			}
			format = IAV_YUV_422_FORMAT;
		}
		if (buf_info->pitch > ALIGN(MAX_WIDTH_FOR_3RD, 32)) {
			buf_info->pitch = 0;
		}
		partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_B);
		break;
	case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
		if (is_buf_type_prev(IAV_ENCODE_SOURCE_FOURTH_BUFFER)) {
			if (G_iav_obj.system_setup_info->vout_swap) {
				w = G_iav_info.pvoutinfo[1]->active_mode.video_size.video_width;
				h = G_iav_info.pvoutinfo[1]->active_mode.video_size.video_height;
			} else {
				w = G_iav_info.pvoutinfo[0]->active_mode.video_size.video_width;
				h = G_iav_info.pvoutinfo[0]->active_mode.video_size.video_height;
			}
			format = IAV_YUV_422_FORMAT;
		}
		if (buf_info->pitch > ALIGN(MAX_WIDTH_FOR_4TH, 32)) {
			buf_info->pitch = 0;
		}
		partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_A);
		break;
	case IAV_ENCODE_SOURCE_MAIN_VCA:
		w = G_iav_obj.source_buffer[G_iav_vcap.vca_src_id].size.width;
		h = G_iav_obj.source_buffer[G_iav_vcap.vca_src_id].size.height;
		partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_C);
		break;
	default:
		break;
	}

	if (is_invalid_dsp_addr(buf_info->y_addr) ||
			is_invalid_dsp_addr(buf_info->uv_addr)) {
		buf_info->y_addr = 0;
		buf_info->uv_addr = 0;
	}

	if (unlikely(is_invalid_dsp_addr(buf_info->y_addr))) {
		yuv->y_addr = NULL;
	} else {
		if (is_map_dsp_partition()) {
			if (context->dsp_partition[partition].user_start) {
				yuv->y_addr = get_dsp_user_addr(context, buf_info->y_addr, partition);
			} else {
				yuv->y_addr = NULL;
			}
		} else {
			yuv->y_addr = dsp_dsp_to_user(context, buf_info->y_addr);
		}
	}
	if (unlikely(is_invalid_dsp_addr(buf_info->uv_addr))) {
		yuv->uv_addr = NULL;
	} else {
		if (is_map_dsp_partition()) {
			if (context->dsp_partition[partition].user_start) {
				yuv->uv_addr = get_dsp_user_addr(context, buf_info->uv_addr, partition);
			} else {
				yuv->uv_addr = NULL;
			}
		} else {
			yuv->uv_addr = dsp_dsp_to_user(context, buf_info->uv_addr);
		}
	}
	yuv->width = w;
	yuv->height = h;
	yuv->pitch = buf_info->pitch;
	yuv->seqnum = buf_info->seqnum;
	yuv->mono_pts = buf_info->mono_pts;
	yuv->dsp_pts = buf_info->dsp_pts;
	yuv->format = format;

#if DEBUG_ENABLE
	iav_printk("y_addr: 0x%08x, mono_pts: %d, dsp_pts: %d, format: %d, pitch %d, size %dx%d.\n",
		buf_info->y_addr, buf_info->mono_pts, buf_info->dsp_pts, format, buf_info->pitch, w, h);
#endif

}

static void read_me1_info(iav_context_t * context, u32 src,
	me1_buffer_info_t * me1_info, iav_me1_cap_t * me1)
{
	iav_reso_ex_t * buf = NULL;
	int partition = partition_id_to_index(IAV_DSP_PARTITION_MAIN_CAPTURE_ME1);

	switch (src) {
		case IAV_ENCODE_SOURCE_SECOND_BUFFER:
			partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_C_ME1);
			break;
		case IAV_ENCODE_SOURCE_THIRD_BUFFER:
			partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_B_ME1);
			break;
		case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
			partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_A_ME1);
			break;
		case IAV_ENCODE_SOURCE_MAIN_VCA:
			partition = partition_id_to_index(IAV_DSP_PARTITION_PREVIEW_C_ME1);
			break;
		default:
			break;
	}

	if (unlikely(is_invalid_dsp_addr(me1_info->addr))) {
		me1->addr = NULL;
	} else {
		if (is_map_dsp_partition()) {
			if (context->dsp_partition[partition].user_start) {
				me1->addr = get_dsp_user_addr(context, me1_info->addr, partition);
			} else {
				me1->addr = NULL;
			}
		} else {
			me1->addr = dsp_dsp_to_user(context, me1_info->addr);
		}
	}
	buf = &G_iav_obj.source_buffer[src].size;
	me1->width = buf->width >> 2;
	me1->height = buf->height >> 2;
	me1->pitch = me1_info->pitch;
	me1->seqnum = me1_info->seqnum;
	me1->mono_pts = me1_info->mono_pts;
	me1->dsp_pts = me1_info->dsp_pts;

#if DEBUG_ENABLE
	iav_printk("ME1 Src [%d], Addr: 0x%08x, mono_pts: %d, dsp_pts: %d, pitch %d, size %dx%d.\n",
		src, (u32)me1_info->addr, me1_info->mono_pts, me1_info->dsp_pts, me1_info->pitch,
		me1->width, me1->height);
#endif

}

int iav_read_raw_info_ex(iav_context_t * context, iav_raw_info_t __user * arg)
{
	u32 addr = 0;
	int enc_mode;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = NULL;
	raw_cap_info_t * raw_cap_info = NULL;
	iav_raw_info_t raw_info;
	unsigned long flags;

	if (check_before_dump_buffer(context) < 0) {
		return -EACCES;
	}
	if (copy_from_user(&raw_info, arg, sizeof(raw_info)))
		return -EFAULT;

	enc_mode = get_enc_mode();
	resource = &G_system_resource_setup[enc_mode];
	if (!is_raw_capture_enabled() &&
		(!is_raw_stat_capture_enabled() ||
			(!resource->max_vin_stats_num_lines_top &&
			!resource->max_vin_stats_num_lines_bot))) {
		iav_error("Mode [%s] DO NOT support capture raw nor raw statistics line.\n",
			G_modes_limit[enc_mode].name);
		if (G_modes_limit[enc_mode].raw_cap_possible) {
			iav_error("Please enable raw capture first!\n");
		}
		return -EPERM;
	}

	while (1) {
		if (G_iav_bufcap.raw_cap_data_valid)
			break;

		if (raw_info.flag & IAV_IOCTL_NONBLOCK)
			break;

		if (iav_wait_compl_interruptible(&G_iav_bufcap.raw_cap_compl)) {
			return -EINTR;
		}
	}

	// now fill data
	raw_cap_info = &G_iav_bufcap.raw_cap_info[G_iav_bufcap.read_index];
	addr = raw_cap_info->addr;
	memset(&raw_info, 0, sizeof(raw_info));
	if (!is_invalid_dsp_addr(addr)) {
		if (is_map_dsp_partition()) {
			if (context->dsp_partition[partition_id_to_index(IAV_DSP_PARTITION_RAW)].user_start) {
				raw_info.raw_addr = get_dsp_user_addr(context, addr,
					partition_id_to_index(IAV_DSP_PARTITION_RAW));
			} else {
				raw_info.raw_addr = NULL;
			}
		} else {
			raw_info.raw_addr = dsp_dsp_to_user(context, addr);
		}
		raw_info.pitch = raw_cap_info->pitch;
		raw_info.width = raw_cap_info->width;
		raw_info.height = raw_cap_info->height;
		raw_info.raw_dsp_pts = raw_cap_info->raw_dsp_pts;
	} else {
		raw_info.raw_addr = 0;
		raw_info.pitch = 0;
		raw_info.width = 0;
		raw_info.height = 0;
	}

	if (!is_map_dsp_partition()) {
		if (!G_iav_obj.dsp_noncached && raw_info.raw_addr) {
			invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(raw_cap_info->addr),
				raw_info.width * raw_info.height * RAW_PIXEL_WIDTH_IN_BYTE);
		}
	}

	// clear data valid flag
	iav_irq_save(flags);
	G_iav_bufcap.raw_cap_data_valid = 0;
	iav_irq_restore(flags);

	iav_printk("[Done] Dump raw data from [0x%08x], resolution %dx%d.\n",
			(u32)raw_info.raw_addr, raw_info.width, raw_info.height);
	return (copy_to_user(arg, &raw_info, sizeof(raw_info))) ? -EFAULT : 0;
}

int iav_read_yuv_buffer_info_ex(iav_context_t * context,
		iav_yuv_buffer_info_ex_t __user *arg)
{
	u32 buf_size;
	u32 source, read_index;
	iav_yuv_cap_t yuv;
	source_buffer_info_t * buf_info;
	iav_yuv_buffer_info_ex_t info;
	unsigned long flags;

	if (check_before_dump_buffer(context) < 0) {
		return -EACCES;
	}
	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	source = info.source;
	if (!is_valid_buffer_id(source) &&
		(source != IAV_ENCODE_SOURCE_MAIN_VCA)) {
		iav_error("Invalid buffer ID [%d] to get YUV data.\n", source);
		return -EFAULT;
	}

	while (1) {
		if (G_iav_bufcap.buffer_data_valid)
			break;

		if (info.flag & IAV_IOCTL_NONBLOCK)
			break;

		if (iav_wait_compl_interruptible(&G_iav_bufcap.buffer_compl)) {
			return -EINTR;
		}
	}

	// now fill the data
	read_index = G_iav_bufcap.read_index;
	if (is_valid_buffer_id(source)) {
		buf_info = &G_iav_bufcap.buf_info[source][read_index];
	} else {
		buf_info = &G_iav_bufcap.vca_info[read_index];
	}
	read_yuv_info(context, source, buf_info, &yuv);
	info.y_addr = yuv.y_addr;
	info.uv_addr = yuv.uv_addr;
	info.width = yuv.width;
	info.height = yuv.height;
	info.pitch = yuv.pitch;
	info.seqnum = yuv.seqnum;
	info.mono_pts = yuv.mono_pts;
	info.dsp_pts = yuv.dsp_pts;
	info.format = yuv.format;

	if (!is_map_dsp_partition()) {
		buf_size = info.pitch * info.height;
		if (!G_iav_obj.dsp_noncached && info.y_addr) {
			invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(buf_info->y_addr),
				buf_size);
		}

		if (yuv.format == IAV_YUV_420_FORMAT) {
			buf_size >>= 1;
		}
		if (!G_iav_obj.dsp_noncached && info.uv_addr) {
			invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(buf_info->uv_addr),
				buf_size);
		}
	}
	// clear data valid flag
	iav_irq_save(flags);
	G_iav_bufcap.buffer_data_valid = 0;
	iav_irq_restore(flags);

	return (copy_to_user(arg, &info, sizeof(info))) ? -EFAULT : 0;
}


int iav_read_me1_buffer_info(iav_context_t *context,
		iav_me1_buffer_info_ex_t __user * arg)
{
	u32 source, read_index;
	iav_me1_cap_t me1;
	me1_buffer_info_t * me1_info;
	iav_me1_buffer_info_ex_t info;
	unsigned long flags;

	if (check_before_dump_buffer(context) < 0) {
		return -EACCES;
	}
	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	source = info.source;
	if (!is_valid_buffer_id(source)) {
		iav_error("Invalid source buffer ID [%d].\n", source);
		return -EFAULT;
	}
	if (!is_buf_type_enc(source)) {
		iav_error("Cannot fetch ME1 data if buffer [%d] is not in "
			"encode state.\n", source);
		return -EPERM;
	}

	while (1) {
		if (G_iav_bufcap.me1_data_valid)
			break;

		if (info.flag & IAV_IOCTL_NONBLOCK)
			break;

		if (iav_wait_compl_interruptible(&G_iav_bufcap.me1_buffer_compl)) {
			return -EINTR;
		}
	}

	//now fill data
	read_index = G_iav_bufcap.read_index;
	me1_info = &G_iav_bufcap.me1_info[source][read_index];
	read_me1_info(context, source, me1_info, &me1);
	info.addr = me1.addr;
	info.width = me1.width;
	info.height = me1.height;
	info.pitch  = me1.pitch;
	info.seqnum = me1.seqnum;
	info.mono_pts = me1.mono_pts;
	info.dsp_pts = me1.dsp_pts;

	if (!is_map_dsp_partition()) {
		//ME1 buffer width* height must be already 32 aligned
		if (!G_iav_obj.dsp_noncached && info.addr) {
			invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(me1_info->addr),
				info.pitch * info.height);
		}
	}
	// clear data valid flag
	iav_irq_save(flags);
	G_iav_bufcap.me1_data_valid = 0;
	iav_irq_restore(flags);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}


int iav_bufcap_read(iav_context_t * context, iav_buf_cap_t __user *arg)
{
	u32 i, rd, size;
	source_buffer_info_t * buf_info = NULL;
	me1_buffer_info_t * me1_info = NULL;
	iav_yuv_cap_t * yuv = NULL;
	iav_me1_cap_t * me1 = NULL;
	iav_buf_cap_t cap;
	unsigned long flags;

	if (check_before_dump_buffer(context) < 0) {
		return -EACCES;
	}
	if (copy_from_user(&cap, arg, sizeof(cap)))
		return -EFAULT;

	while (1) {
		if (G_iav_bufcap.buffer_data_valid)
			break;

		if (cap.flag & IAV_IOCTL_NONBLOCK)
			break;

		if (iav_wait_compl_interruptible(&G_iav_bufcap.buffer_compl)) {
			return -EINTR;
		}
	}

	/* fill the data */
	rd = G_iav_bufcap.read_index;
	buf_info = &G_iav_bufcap.vca_info[rd];
	yuv = &cap.vca;
	read_yuv_info(context, IAV_ENCODE_SOURCE_MAIN_VCA, buf_info, yuv);
	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
		buf_info = &G_iav_bufcap.buf_info[i][rd];
		yuv = &cap.yuv[i];
		read_yuv_info(context, i, buf_info, yuv);

		me1_info = &G_iav_bufcap.me1_info[i][rd];
		me1 = &cap.me1[i];
		read_me1_info(context, i, me1_info, me1);
	}

	if (!is_map_dsp_partition()) {
		if (!G_iav_obj.dsp_noncached) {
			buf_info = &G_iav_bufcap.vca_info[rd];
			yuv = &cap.vca;
			size = yuv->pitch * (yuv->height);
			if (yuv->y_addr) {
				invalidate_d_cache((u8*)DSP_TO_AMBVIRT(buf_info->y_addr), size);
			}
			if (yuv->format == IAV_YUV_420_FORMAT) {
				size >>= 1;
			}
			if (yuv->uv_addr) {
				invalidate_d_cache((u8*)DSP_TO_AMBVIRT(buf_info->uv_addr), size);
			}
			for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++i) {
				buf_info = &G_iav_bufcap.buf_info[i][rd];
				yuv = &cap.yuv[i];
				size = yuv->pitch * (yuv->height);
				if (yuv->y_addr) {
					invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(buf_info->y_addr), size);
				}
				if (yuv->format == IAV_YUV_420_FORMAT) {
					size >>= 1;
				}
				if (yuv->uv_addr) {
					invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(buf_info->uv_addr), size);
				}

				me1_info = &G_iav_bufcap.me1_info[i][rd];
				me1 = &cap.me1[i];
				if (me1->addr) {
					invalidate_d_cache((u8 *)DSP_TO_AMBVIRT(me1_info->addr),
						(me1->pitch) * (me1->height));
				}
			}
		}
	}

	iav_irq_save(flags);
	G_iav_bufcap.buffer_data_valid = 0;
	G_iav_bufcap.me1_data_valid = 0;
	iav_irq_restore(flags);

	return copy_to_user(arg, &cap, sizeof(cap)) ? -EFAULT : 0;
}


void save_dsp_buffers(VCAP_STRM_REPORT * msg, VCAP_STRM_REPORT_EXT * msg_ext)
{
	u16 index = G_iav_bufcap.write_index;
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;

	G_iav_bufcap.dsp_pts = msg_ext->hw_pts;
	G_iav_bufcap.raw_dsp_pts = msg_ext->raw_hw_pts;
	G_iav_bufcap.sec2_out_dsp_pts = msg_ext->sec2_out_hw_pts;
	if (pts_info->hwtimer_enabled) {
		G_iav_bufcap.mono_pts = get_hw_pts(msg_ext->hw_pts);
		if (is_warp_mode()) {
			G_iav_bufcap.sec2_out_mono_pts = get_hw_pts(msg_ext->sec2_out_hw_pts);
		}
	} else {
		G_iav_bufcap.mono_pts = get_monotonic_pts();
		if (is_warp_mode()) {
			G_iav_bufcap.sec2_out_mono_pts = get_monotonic_pts();
		}
	}

	if (is_raw_capture_enabled() || is_raw_stat_capture_enabled()) {
		save_raw_cap_info(msg, index);
	}
	save_source_buffer_info(msg, index);
	save_me1_buffer_info(msg_ext, index);
	save_vca_buffer_info(msg_ext, index);
	update_pts_and_readers(index);

	++G_iav_bufcap.seq_num;

	// always read out the latest frame
	G_iav_bufcap.read_index = index;

	if ((index + 1) == G_iav_bufcap.frm_total_num) {
		G_iav_bufcap.write_index = 0;
	} else {
		G_iav_bufcap.write_index = index + 1;
	}
}

