/*
 * iav_frame.c
 *
 * History:
 *	2012/05/10 - [Jian Tang] created file
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
#include "iav_pts.h"
#include "iav_mem.h"
#include "iav_encode.h"


typedef struct iav_frame_info_s {
	u32	unread_length;
	u32	valid_frames;
	u32	encoded_frames;
	u32	expired_frames;
	u32	corrupted_frames;
} iav_frame_info_t;

typedef struct iav_statis_info_s {
	u32	valid_frames;
	u32	expired_frames;
	u32	corrupted_frames;
} iav_statis_info_t;

typedef struct iav_audit_frame_info_s {
	iav_frame_info_t	stream[IAV_MAX_ENCODE_STREAMS_NUM];
	iav_frame_info_t	type[IAV_BITS_TYPE_NUM];
	int	emptiness;
} iav_audit_frame_info_t;

typedef struct iav_audit_statis_info_s {
	iav_statis_info_t	stream[IAV_MAX_ENCODE_STREAMS_NUM];
	iav_statis_info_t	total;
} iav_audit_statis_info_t;

typedef struct iav_frame_queue_s {
	struct list_head	stream[IAV_MAX_ENCODE_STREAMS_NUM];
	struct list_head	type[IAV_BITS_TYPE_NUM];
	struct list_head	valid;
} iav_frame_queue_t;

typedef struct iav_statis_queue_s {
	struct list_head	stream[IAV_MAX_ENCODE_STREAMS_NUM];
	struct list_head	valid;
} iav_statis_queue_t;


static iav_frame_queue_t		G_frame_queue;
static iav_statis_queue_t		G_statis_queue;
static iav_audit_frame_info_t	G_audit_frames;
static iav_audit_statis_info_t	G_audit_statis;

static u32 G_bsb_size = 0;

static struct iav_bits_desc *G_bits_desc = NULL;
static struct iav_stat_desc *G_statis_desc = NULL;


/**********************************************************
 *
 *	Internal helper functions
 *
 *********************************************************/

static inline int __is_invalid_stream(int stream)
{
	return ((stream < 0) || (stream >= IAV_MAX_ENCODE_STREAMS_NUM));
}

static inline int __is_frame_info_unused(dsp_extend_bits_info_t *fi)
{
	return (fi->state == FS_UNUSED);
}

static inline int __is_frame_info_queued(dsp_extend_bits_info_t *fi)
{
	return (fi->state == FS_QUEUED);
}

static inline int __is_frame_info_fetched(dsp_extend_bits_info_t *fi)
{
	return (fi->state == FS_FETCHED);
}

static inline int __is_frame_info_released(dsp_extend_bits_info_t *fi)
{
	return (fi->state == FS_RELEASED);
}

static inline void __set_frame_info_state(dsp_extend_bits_info_t * fi, u32 state)
{
	unsigned long flags;
	iav_irq_save(flags);
	fi->state = state;
	iav_irq_restore(flags);
}

static inline int __is_frame_bsb_full(IAV_BITS_DESC_TYPE type, int frame_size)
{
	return ((G_audit_frames.emptiness - frame_size) < 0) ? 1 : 0;
}

static inline int __is_frame_fq_empty(void)
{
	return ((G_audit_frames.type[IAV_BITS_MJPEG].valid_frames +
		G_audit_frames.type[IAV_BITS_H264].valid_frames) == 0);
}

static inline int __is_frame_fq_full(void)
{
	return ((G_audit_frames.type[IAV_BITS_MJPEG].valid_frames +
		G_audit_frames.type[IAV_BITS_H264].valid_frames) == NUM_BS_DESC);
}

static inline int __is_frame_tq_empty(IAV_BITS_DESC_TYPE type)
{
	return (G_audit_frames.type[type].valid_frames == 0);
}

static inline int __is_frame_tq_full(IAV_BITS_DESC_TYPE type)
{
	return __is_frame_fq_full();
}

static inline int __is_frame_sq_empty(int stream)
{
	return (G_audit_frames.stream[stream].valid_frames == 0);
}

static inline int __is_frame_sq_full(int stream)
{
	return (G_audit_frames.stream[stream].valid_frames == NUM_BS_DESC);
}

static inline void __update_bsb_emptyness(u32 bits_type, u32 * empty)
{
	#define	BSB_GAP_LENGTH		(32)
	static u32 first_addr = 0, last_addr = 0;
	u32 first_index, last_index, addr, emptiness;
	struct iav_bits_desc * frame_addr = NULL;
	frame_addr = G_bits_desc;
	if (likely(!__is_frame_tq_empty(bits_type))) {
		first_index = list_entry(G_frame_queue.valid.next,
			dsp_extend_bits_info_t, valid) - frame_addr->ext_start;
		addr = frame_addr->start[first_index].start_addr;
		if (addr != ~0) {
			first_addr = addr;
		}
		last_index = list_entry(G_frame_queue.valid.prev,
			dsp_extend_bits_info_t, valid) - frame_addr->ext_start;
		addr = frame_addr->start[last_index].start_addr;
		if (addr != ~0) {
			last_addr = addr;
		}
		emptiness = (first_addr + G_bsb_size - BSB_GAP_LENGTH - last_addr -
			frame_addr->ext_start[last_index].old_frame_size) % G_bsb_size;
	} else {
		emptiness = G_bsb_size;
	}
	*empty = emptiness;
}

static inline void __frame_enq(dsp_extend_bits_info_t * new_frame)
{
	unsigned long flags;
	u32 stream = new_frame->new_stream;
	IAV_BITS_DESC_TYPE type = new_frame->new_bits_type;

	if (__is_invalid_stream(stream)) {
		iav_error("Invalid stream ID [%d]!\n", stream);
		return ;
	}

	iav_irq_save(flags);

	// Step 1: Save frame info first
	new_frame->old_frame_size = new_frame->new_frame_size;
	new_frame->old_bits_type = new_frame->new_bits_type;
	new_frame->old_stream = new_frame->new_stream;

	// Step 2: Add frame info into Frame Q, Type Q, and Stream Q
	list_add_tail(&new_frame->valid, &G_frame_queue.valid);
	list_add_tail(&new_frame->type, &G_frame_queue.type[type]);
	list_add_tail(&new_frame->frame, &G_frame_queue.stream[stream]);
	++G_audit_frames.type[type].valid_frames;
	++G_audit_frames.type[type].encoded_frames;
	G_audit_frames.type[type].unread_length += new_frame->new_frame_size;
	++G_audit_frames.stream[stream].valid_frames;
	++G_audit_frames.stream[stream].encoded_frames;
	G_audit_frames.stream[stream].unread_length += new_frame->new_frame_size;

	// Step 3: Update BSB emptyness and frame state
	__update_bsb_emptyness(type, &G_audit_frames.emptiness);
	new_frame->state = FS_QUEUED;

	iav_irq_restore(flags);
}

static inline void __frame_deq(IAV_FRAME_QUEUE q, u32 arg)
{
	unsigned long flags;
	u32 stream;
	IAV_BITS_DESC_TYPE type;
	dsp_extend_bits_info_t * old_frame = NULL;

	iav_irq_save(flags);

	do {
		switch (q) {
		case FQ_FQ:
			/* DeQ frame from the header of FQ */
			old_frame = list_first_entry(&G_frame_queue.valid,
				dsp_extend_bits_info_t, valid);
			break;
		case FQ_SQ:
			/* DeQ frame from the header of SQ */
			old_frame = list_first_entry(&G_frame_queue.stream[arg],
				dsp_extend_bits_info_t, frame);
			break;
		case FQ_FRM:
		default:
			/* DeQ frame with "old_frame" pointer */
			old_frame = (dsp_extend_bits_info_t *)arg;
			break;
		}
		stream = old_frame->old_stream;
		type = old_frame->old_bits_type;

		if (unlikely(__is_frame_fq_empty() || __is_frame_tq_empty(type) ||
			__is_frame_sq_empty(stream))) {
			iav_error("FQ/TQ/SQ is already empty!\n");
			break;
		}

		if (old_frame->valid.next && old_frame->valid.prev) {
			/* Step 1: Delete frame info from Frame Q, Type Q, and Stream Q */
			list_del(&old_frame->valid);
			list_del(&old_frame->type);
			list_del(&old_frame->frame);
			--G_audit_frames.type[type].valid_frames;
			--G_audit_frames.stream[stream].valid_frames;
			G_audit_frames.type[type].unread_length -= old_frame->old_frame_size;
			G_audit_frames.stream[stream].unread_length -= old_frame->old_frame_size;

			/* Step 2: Update BSB emptyness and frame state */
			switch (old_frame->state) {
			case FS_QUEUED:
				++G_audit_frames.type[type].expired_frames;
				++G_audit_frames.stream[stream].expired_frames;
				break;
			case FS_FETCHED:
				++G_audit_frames.type[type].corrupted_frames;
				++G_audit_frames.stream[stream].corrupted_frames;
				break;
			case FS_RELEASED:
				/* Do nothing */
				break;
			case FS_UNUSED:
			default :
				iav_error("ERROR: Invalid frame state [%d]!\n", old_frame->state);
				break;
			}
			__update_bsb_emptyness(type, &G_audit_frames.emptiness);
			old_frame->state = FS_UNUSED;

			/* Step 3: Reset deleted frame info */
			old_frame->valid.next = old_frame->valid.prev = NULL;
			old_frame->type.next = old_frame->type.prev = NULL;
			old_frame->frame.next = old_frame->frame.prev = NULL;
			old_frame->old_frame_size = 0;
			old_frame->old_bits_type = IAV_BITS_TYPE_INVALID;
			old_frame->old_stream = IAV_MAX_ENCODE_STREAMS_NUM;
		}
	} while (0);

	iav_irq_restore(flags);
}

static inline void __print_frame_audit(iav_audit_frame_info_t * audit,
	int reset, int print_flag)
{
	const int print_interval = 3000;
	static int counter = 0;
	int i;
	if (reset) {
		counter = 0;
		return ;
	}
	if (++counter == print_interval) {
		counter = 0;
		if (print_flag) {
			iav_printk("==== Frame Queue Audit ===[%d]=\n",
				sizeof(dsp_extend_bits_info_t));
			for (i = 0; i < IAV_BITS_TYPE_NUM; ++i) {
				iav_printk(" ======== BITS type [%d] ========\n", i);
				iav_printk("     BSB emptyness = %d\n", audit->emptiness);
				iav_printk("     Unread length = %d\n", audit->type[i].unread_length);
				iav_printk("     Total encoded = %d\n", audit->type[i].encoded_frames);
				iav_printk("       Total valid = %d\n", audit->type[i].valid_frames);
				iav_printk("   Total corrupted = %d\n", audit->type[i].corrupted_frames);
				iav_printk("     Total expired = %d\n", audit->type[i].expired_frames);
			}
			for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
				iav_printk(" ++++++++ stream [%d] ++++++++\n", i);
				iav_printk(" Unread size = %d\n", audit->stream[i].unread_length);
				iav_printk("     Encoded = %d\n", audit->stream[i].encoded_frames);
				iav_printk("       Valid = %d\n", audit->stream[i].valid_frames);
				iav_printk("   Corrupted = %d\n", audit->stream[i].corrupted_frames);
				iav_printk("     Expired = %d\n", audit->stream[i].expired_frames);
			}
		}
	}
}

static inline int __is_statis_info_unused(dsp_extend_enc_stat_info_t * esi)
{
	return (esi->state == FS_UNUSED);
}

static inline int __is_statis_info_queued(dsp_extend_enc_stat_info_t * esi)
{
	return (esi->state == FS_QUEUED);
}

static inline int __is_statis_info_fetched(dsp_extend_enc_stat_info_t * esi)
{
	return (esi->state == FS_FETCHED);
}

static inline int __is_statis_info_released(dsp_extend_enc_stat_info_t * esi)
{
	return (esi->state == FS_RELEASED);
}

static inline void __set_statis_info_state(dsp_extend_enc_stat_info_t *esi, u32 state)
{
	unsigned long flags;
	iav_irq_save(flags);
	esi->state = state;
	iav_irq_restore(flags);
}

static inline int __is_statis_fq_empty(void)
{
	return (G_audit_statis.total.valid_frames == 0);
}

static inline int __is_statis_fq_full(void)
{
	return (G_audit_statis.total.valid_frames == NUM_STATIS_DESC);
}

static inline int __is_statis_sq_empty(int stream)
{
	return (G_audit_statis.stream[stream].valid_frames == 0);
}

static inline int __is_statis_sq_full(int stream)
{
	return (G_audit_statis.stream[stream].valid_frames == NUM_SD_PER_STREAM);
}

static inline void __statis_enq(dsp_extend_enc_stat_info_t * new_statis, int valid)
{
	unsigned long flags;
	u32 stream = new_statis->new_stream;

	if (unlikely(__is_invalid_stream(stream))) {
		iav_error("Invalid stream ID [%d]!\n", stream);
		return ;
	}

	iav_irq_save(flags);

	// Step 1: Save statistics info first
	new_statis->old_frame_num = new_statis->new_frame_num;
	new_statis->old_stream = new_statis->new_stream;

	// Step 2: Add statis info into Statis Q and Stream Q
	do {
		list_add_tail(&new_statis->valid, &G_statis_queue.valid);
		++G_audit_statis.total.valid_frames;
		if (valid == 0)
			break ;
		list_add_tail(&new_statis->frame, &G_statis_queue.stream[stream]);
		++G_audit_statis.stream[stream].valid_frames;
	} while (0);
	new_statis->state = FS_QUEUED;

	iav_irq_restore(flags);
}

static inline void __statis_deq(IAV_FRAME_QUEUE q, u32 arg)
{
	unsigned long flags;
	u32 stream;
	dsp_extend_enc_stat_info_t * old_statis = NULL;
	dsp_extend_enc_stat_info_t * first_statis = NULL;

	iav_irq_save(flags);

	do {
		switch (q) {
		case FQ_FQ:
			/* DeQ frame from the header of FQ */
			old_statis = list_first_entry(&G_statis_queue.valid,
				dsp_extend_enc_stat_info_t, valid);
			break;
		case FQ_SQ:
			/* DeQ frame from the header of SQ */
			old_statis =list_first_entry(&G_statis_queue.stream[arg],
				dsp_extend_enc_stat_info_t, frame);
			break;
		case FQ_FRM:
		default:
			/* DeQ frame with "old_statis" pointer */
			old_statis = (dsp_extend_enc_stat_info_t *)arg;
			break;
		}

		if (unlikely(__is_statis_fq_empty())) {
			break;
		}

		if (old_statis->valid.next && old_statis->valid.prev) {
			/* Step 1: Delete statis info from Statis Q and Stream Q */
			do {
				list_del(&old_statis->valid);
				--G_audit_statis.total.valid_frames;

				stream = old_statis->old_stream;
				if (__is_statis_sq_empty(stream))
					break ;
				first_statis = list_first_entry(&G_statis_queue.stream[stream],
					dsp_extend_enc_stat_info_t, frame);
				if (old_statis != first_statis) {
					break ;
				}

				list_del(&old_statis->frame);
				--G_audit_statis.stream[stream].valid_frames;
			} while (0);

			/* Step 2: Update audit data and statistics state */
			switch (old_statis->state) {
			case FS_QUEUED:
				++G_audit_statis.stream[stream].expired_frames;
				++G_audit_statis.total.expired_frames;
				break;
			case FS_FETCHED:
				++G_audit_statis.stream[stream].corrupted_frames;
				++G_audit_statis.total.corrupted_frames;
				break;
			case FS_RELEASED:
				/* Do nothing */
				break;
			case FS_UNUSED:
			default:
				iav_error("ERROR: Invalid frame state [%d]!\n", old_statis->state);
				break;
			}
			old_statis->state = FS_UNUSED;

			/* Step 3: Reset deleted statistics info */
			old_statis->valid.next = old_statis->valid.prev = NULL;
			old_statis->frame.next = old_statis->frame.prev = NULL;
			old_statis->old_frame_num = -1;
			old_statis->old_stream = IAV_MAX_ENCODE_STREAMS_NUM;
		}
	} while (0);

	iav_irq_restore(flags);
}

static inline void __print_statis_audit(iav_audit_statis_info_t * audit,
	int reset, int print_flag)
{
	const int print_interval = 3000;
	static int counter = 0;
	int i;
	if (reset) {
		counter = 0;
		return ;
	}
	if (++counter == print_interval) {
		counter = 0;
		if (print_flag) {
			iav_printk("==== Statistics Queue Audit === [%d] =\n",
				sizeof(dsp_extend_enc_stat_info_t));
			iav_printk("     Valid = %d\n", audit->total.valid_frames);
			iav_printk(" Corrupted = %d\n", audit->total.corrupted_frames);
			iav_printk("   Expired = %d\n", audit->total.expired_frames);
			for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
				iav_printk("++++++++ stream [%d] ++++++++\n", i);
				iav_printk("     Valid = %d\n", audit->stream[i].valid_frames);
				iav_printk(" Corrupted = %d\n", audit->stream[i].corrupted_frames);
				iav_printk("   Expired = %d\n", audit->stream[i].expired_frames);
			}
		}
	}
}

/**********************************************************
 *
 *	external helper functions for other modules
 *
 *********************************************************/

inline u32 get_current_frame_number(void)
{
	return (G_audit_frames.type[IAV_BITS_MJPEG].valid_frames +
		G_audit_frames.type[IAV_BITS_H264].valid_frames);
}

inline int frame_info_queue_init(void)
{
	int i;
	struct iav_mem_block *bsb;
	INIT_LIST_HEAD(&G_frame_queue.valid);
	for (i = 0; i < IAV_BITS_TYPE_NUM; ++i) {
		INIT_LIST_HEAD(&G_frame_queue.type[i]);
		memset(&G_audit_frames.type[i], 0, sizeof(iav_frame_info_t));
	}
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		INIT_LIST_HEAD(&G_frame_queue.stream[i]);
		memset(&G_audit_frames.stream[i], 0, sizeof(iav_frame_info_t));
	}

	iav_get_bits_desc(&G_bits_desc);

	iav_get_mem_block(IAV_MMAP_BSB2, &bsb);
	G_bsb_size = bsb->size;
	G_audit_frames.emptiness = bsb->size;

	return 0;
}

int enqueue_frame(dsp_extend_bits_info_t * new_frame)
{
	IAV_BITS_DESC_TYPE bits_type = new_frame->new_bits_type;

	/* If BSB or bits info buffer is full, discard the oldest frames directly,
	 * and update the counter of corrupted frame and expired frame.
	 */
	while (__is_frame_fq_full() ||
		__is_frame_bsb_full(bits_type, new_frame->new_frame_size)) {
		if (likely(!__is_frame_fq_empty())) {
			__frame_deq(FQ_FQ, 0);
		} else {
			iav_error("BSB empty [%d], stream [%d], frame size [%d], TQ [%d].\n",
				G_audit_frames.emptiness,
				new_frame->new_stream, new_frame->new_frame_size,
				G_audit_frames.type[bits_type].valid_frames);
			iav_error("ERROR: TQ[%d] is empty but BSB buffer is full!\n", bits_type);
			return -1;
		}
	}

	/* If new frame is already in the queue, dequeue it first. */
	if (!__is_frame_info_unused(new_frame)) {
		__frame_deq(FQ_FRM, (u32)new_frame);
	}

	__frame_enq(new_frame);
	__print_frame_audit(&G_audit_frames, 0, 0);

	return 0;
}

int dequeue_frame(dsp_extend_bits_info_t * old_frame)
{
	if (__is_frame_info_fetched(old_frame)) {
		__frame_deq(FQ_FRM, (u32)old_frame);
	}
	return 0;
}

int flush_all_frames(int stream)
{
	while (!__is_frame_sq_empty(stream)) {
		__frame_deq(FQ_SQ, stream);
	}
	__print_frame_audit(&G_audit_frames, 1, 0);

	return 0;
}

static int fill_frame_info(dsp_extend_bits_info_t * fetch_frame,
	iav_bits_info_slot_ex_t * encode_slot)
{
	int index;

	if (fetch_frame >= G_bits_desc->ext_start &&
		fetch_frame < G_bits_desc->ext_end) {
		index = fetch_frame - G_bits_desc->ext_start;
		__set_frame_info_state(fetch_frame, FS_FETCHED);
		encode_slot->frame_index = index;
		encode_slot->read_index = G_bits_desc->start + index;
		encode_slot->bits_type = fetch_frame->old_bits_type;
		encode_slot->mono_pts = fetch_frame->monotonic_pts;
		encode_slot->dsp_pts = fetch_frame->dsp_pts;
	} else {
		encode_slot->frame_index = -1;
		encode_slot->read_index = NULL;
	}
	return 0;
}

int fetch_frame_from_stream_queue(int stream,
	iav_bits_info_slot_ex_t * encode_slot)
{
	dsp_extend_bits_info_t * fetch_frame = NULL;

	if (!__is_frame_sq_empty(stream)) {
		fetch_frame = list_first_entry(&G_frame_queue.stream[stream],
			dsp_extend_bits_info_t, frame);
	}
	fill_frame_info(fetch_frame, encode_slot);
	return 0;
}

int fetch_frame_from_queue(iav_bits_info_slot_ex_t * encode_slot)
{
	dsp_extend_bits_info_t * fetch_frame = NULL;

	if (!__is_frame_fq_empty()) {
		fetch_frame = list_first_entry(&G_frame_queue.valid,
			dsp_extend_bits_info_t, valid);
	}
	fill_frame_info(fetch_frame, encode_slot);
	return 0;
}

int release_frame_to_queue(int frame_index)
{
	dsp_extend_bits_info_t * release_frame = NULL;

	release_frame = G_bits_desc->ext_start + frame_index;
	switch (release_frame->state) {
	case FS_QUEUED:
		/*
		 * This frame was already expired and updated with new content.
		 * No need to release.
		 */
		break;
	case FS_FETCHED:
		__set_frame_info_state(release_frame, FS_RELEASED);
		__frame_deq(FQ_FRM, (u32)release_frame);
		break;
	default :
		iav_error("Invalid frame state [%d] to release.\n", release_frame->state);
		break;
	}
	return 0;
}

inline u32 get_current_statis_number(void)
{
	int i, valid_frames;
	for (i = 0, valid_frames = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		valid_frames += G_audit_statis.stream[i].valid_frames;
	}
	return valid_frames;
}

inline int statis_info_queue_init(void)
{
	int i;
	INIT_LIST_HEAD(&G_statis_queue.valid);
	memset(&G_audit_statis.total, 0, sizeof(iav_statis_info_t));
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		INIT_LIST_HEAD(&G_statis_queue.stream[i]);
		memset(&G_audit_statis.stream[i], 0, sizeof(iav_statis_info_t));
	}

	iav_get_stat_desc(&G_statis_desc);

	return 0;
}

int enqueue_statis(dsp_extend_enc_stat_info_t * new_statis, int valid)
{
	while (__is_statis_sq_full(new_statis->new_stream)) {
		__statis_deq(FQ_SQ, new_statis->new_stream);
	}

	/* If new statistics info is already in the Q, deQ it first. */
	if (!__is_statis_info_unused(new_statis)) {
		__statis_deq(FQ_FRM, (u32)new_statis);
	}

	__statis_enq(new_statis, valid);
	__print_statis_audit(&G_audit_statis, 0, 0);

	return 0;
}

int dequeue_statis(dsp_extend_enc_stat_info_t * old_statis)
{
	if (__is_statis_info_fetched(old_statis)) {
		__statis_deq(FQ_FRM, (u32)old_statis);
	}
	return 0;
}

int flush_all_statis(int stream)
{
	while (!__is_statis_sq_empty(stream)) {
		__statis_deq(FQ_SQ, stream);
	}
	__print_statis_audit(&G_audit_statis, 1, 0);

	return 0;
}

static int fill_statis_info(dsp_extend_enc_stat_info_t * fetch_statis,
	iav_statis_info_slot_ex_t * statis_slot)
{
	int index;

	if (fetch_statis >= G_statis_desc->ext_start &&
		fetch_statis < G_statis_desc->ext_end) {
		index = fetch_statis - G_statis_desc->ext_start;
		__set_statis_info_state(fetch_statis, FS_FETCHED);
		statis_slot->statis_index = index;
		statis_slot->read_index = G_statis_desc->start + index;
		statis_slot->mono_pts = fetch_statis->mono_pts;
	} else {
		statis_slot->statis_index = -1;
		statis_slot->read_index = NULL;
	}
	return 0;
}

int fetch_statistics_from_queue(int stream, iav_statis_info_slot_ex_t * statis_slot)
{
	dsp_extend_enc_stat_info_t * fetch_statis = NULL;

	if (!__is_statis_sq_empty(stream)) {
		fetch_statis = list_first_entry(&G_statis_queue.stream[stream],
			dsp_extend_enc_stat_info_t, frame);
	}
	fill_statis_info(fetch_statis, statis_slot);
	return 0;
}

int release_statistics_to_queue(int statis_index)
{
	dsp_extend_enc_stat_info_t * release_statis = NULL;

	release_statis = G_statis_desc->ext_start + statis_index;
	switch (release_statis->state) {
	case FS_QUEUED:
		/*
		 * This statistics data was already expired and updated with new
		 * content. No need to release.
		 */
		break;
	case FS_FETCHED:
		__set_statis_info_state(release_statis, FS_RELEASED);
		__statis_deq(FQ_FRM, (u32)release_statis);
		break;
	default :
		iav_error("Invalid statistics state [%d] to release!\n", release_statis->state);
		break;
	}
	return 0;
}

