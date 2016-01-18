/*
 * iav_decode.c
 *
 * History:
 *	2008/1/25 - [Oliver Li] created file
 *	2012/11/15 - [Zhi He] modify for s2
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

#include "iav_drv.h"
#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "amba_dsp.h"
#include "iav_common.h"
#include "utils.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "iav_priv.h"
#include "iav_api.h"
#include "iav_mem.h"
#include "iav_decode.h"

#define MAX_UDEC_MSG_LOG_SIZE	(128*KB)
#define MAX_DECODED_FRAMES	20

#define MIN_FRAME_WIDTH		16
#define MIN_FRAME_HEIGHT	16

#define MAX_FRAME_WIDTH		1920
#define MAX_FRAME_HEIGHT	1088

#define VALID_WIDTH(_w)		((_w) >= MIN_FRAME_WIDTH && (_w) <= MAX_FRAME_WIDTH)
#define VALID_HEIGHT(_h)	((_h) >= MIN_FRAME_HEIGHT && (_h) <= MAX_FRAME_HEIGHT)

#define WIDTH_ALIGN		32
#define HEIGHT_ALIGN		16

#define W_ALIGNED(_w)		(((_w) + (WIDTH_ALIGN - 1)) & ~(WIDTH_ALIGN - 1))
#define H_ALIGNED(_h)		(((_h) + (HEIGHT_ALIGN - 1)) & ~(HEIGHT_ALIGN - 1))

#define MAX_NUM_DECODER	16
#define MAX_NUM_WINDOWS		16

//use UDEC capture cmd, which is obsolete now, use POSTP capture cmd now
//#define CONFIG_IAV_USE_OLD_UDEC_CAPTURE

//#define CONFIG_IAV_ENABLE_512K_WITH_CONSTRAINED_FEATURE

#define MAX_DEC_CAPTURED_ENC_SIZE	(2*1024*1024)
#define MAX_DEC_THUMBNNAIL_CAPTURED_ENC_SIZE	(512*1024)

//dsp related
#define DMAX_WINDOW_CONFIG_PER_CMD 6
#define DMAX_RENDER_CONFIG_PER_CMD 6
#define DMAX_SWITCH_CONFIG_PER_CMD 16

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#if 0
#define UDEC_DEBUG_ASSERT(expr) (void)0
#define UDEC_LOG_ERROR(format, args...) (void)0
#define UDEC_LOG_ALWAYS(format, args...) (void)0
#else
#define UDEC_DEBUG_ASSERT(expr) do { \
    if (!(expr)) { \
        DRV_PRINT("assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
    } \
} while (0)

#define UDEC_LOG_ALWAYS(format, args...) do { \
    DRV_PRINT(format,##args); \
} while (0)

#define UDEC_LOG_ERROR(format, args...) do { \
    DRV_PRINT("Error at %s:%d: \n", __FILE__, __LINE__); \
    DRV_PRINT(format,##args); \
} while (0)
#endif

//#define ENABLE_MULTI_WINDOW_UDEC

//this will add more print in driver, should disable it in normal case
//#define VERBOSE_LOG
//#define DEBUG_DUMP_DEWARP_TABLE

// frame buffer state
enum {
	FBS_INVALID,
	FBS_REQUESTED,
	FBS_OUTPIC,
	FBS_VOUT,
};

typedef struct fb_pool_s {
	int	index;
	int	num_fb;
	atomic_t fb_state[MAX_DECODED_FRAMES];
	iav_frame_buffer_t fb[MAX_DECODED_FRAMES];
} fb_pool_t;

typedef struct fb_queue_s {
	int	read_index;
	int	write_index;
	int	num_fb;
	int	num_waiters;
	struct semaphore sem;
	u8	fb_index[MAX_DECODED_FRAMES];
} fb_queue_t;

typedef struct udec_mmap_s {
	u32	phys_start;
	u32	phys_end;
	u32	virt_start;
	u32	virt_end;
	u8	*user_start;
	u8	*user_end;
	u32	size;

	unsigned long unmap_start;
	unsigned long unmap_size;
} udec_mmap_t;

#define UDEC_STATUS_EOS		(1 << 0)
#define UDEC_STATUS_DISP	(1 << 1)

//same with dsp document
#define DSP_DEC_CODEC_BIT_H264 (0x1)
#define DSP_DEC_CODEC_BIT_MPEG12 (0x2)
#define DSP_DEC_CODEC_BIT_MPEG4H (0x4)
#define DSP_DEC_CODEC_BIT_MPEG4HY (0x8)
#define DSP_DEC_CODEC_BIT_VC1 (0x10)
#define DSP_DEC_CODEC_BIT_RV40HY (0x20)
#define DSP_DEC_CODEC_BIT_JPEG (0x40)
#define DSP_DEC_CODEC_BIT_SW (0x80)

typedef struct udec_s {

	u8	udec_id;
	u8	udec_type;
	u8	noncachable_buffer;
	u8	udec_stopped;	// set by iav_stop_udec()

	u8	udec_inuse;
	u8	udec_status;
	u8	pts_comes;
	u8	reserved;

	u32	eos_pts_low;
	u32	eos_pts_high;

	UDEC_DISP_STATUS_MSG disp_status;

	atomic_t udec_state;	// UDEC_STAT_IDLE, ...
	atomic_t vout_state;	// VOUT_STATE_IDLE, ...
	int	prev_vout_state;

	iav_udec_config_t udec_config;		// used when entering udec mode
	iav_udec_info_ex_t udec_info;		// used when setup udec instance
	iav_udec_vout_config_t *vout_config[IAV_NUM_VOUT]; // VOUTs used by this udec; point to iav_decode_obj_t->vout_config

	u32	error_code;
	u32	error_pic_pts_low;
	u32	error_pic_pts_high;

	u16	bits_fifo_id;
	u16	mv_fifo_id;

	u32	bits_fifo_base;
	u32	mv_fifo_base;

	u32	num_decoded_frames;
	u32	curr_frame;

	u16	space_id;
	u16	fbp_id;

	udec_mmap_t bits_fifo;
	udec_mmap_t mv_fifo;
	udec_mmap_t jpeg_y;
	udec_mmap_t jpeg_uv;

	u32	bits_fifo_ptr;
	u32	dsp_bits_fifo_next_phys;
	u32	dsp_mv_fifo_next_phys;
	u32	dsp_bits_fifo_last_write_phys;//for debug use

	int	num_waiters;
	struct semaphore sem;

	u32	frames_received;
	u32	frames_released;

	fb_queue_t	outpic_queue;
	fb_pool_t	udec_fb_pool;

	u8	abort_flag;
	u32	code_timeout_pc;
	u32	code_timeout_reason;
	u32	mdxf_timeout_pc;
	u32	mdxf_timeout_reason;

	u32 vdsp_ori_seq_number;
	u32 vdsp_new_seq_number;

	//debug only
	//u32 decode_cmd_count;
	u32 decode_frame_count;
} udec_t;

typedef struct vout_fb_s {
	u8	decoder_id;
	u16	fb_index;
	u8	valid;
	u8	render;
	u8	first_field_rendered;
} vout_fb_t;

typedef struct udec_vout_info_s {
	u8	vout_inuse;
	u8	rendered;
	u8	intl_state;
	u8	irq_entered;
	unsigned long tick;
	unsigned long	irq_counter;
	vout_fb_t	A;
	vout_fb_t	B;
	vout_fb_t	C;
	vout_fb_t	D;
	vout_fb_t	E;
	int	frame_not_rendered;
	int	num_waiters;
	struct semaphore sem;
} udec_vout_info_t;

//initialized value
#define INVALID_SWITCH_STATUS 0x7F7FFFFF
#define INITIAL_SWITCH_STATUS 0x7F7F7FFF

typedef struct iav_decode_obj_s {
	u32	dsp_decode_pts;
	u32	decode_last_pts;
	u32	decode_start_pts;

	u32	dsp_bits_fifo_next_phys;
	u32	dsp_decoded_pic_number;
	int	eos_flag;

	u16	pic_width;
	u16	pic_height;
	u16	fb_width;
	u16	fb_height;

	u8	*udec_msg_base;
	int	udec_msg_bytes_total;
	u8	*udec_msg_addr;
	int	udec_msg_bytes_left;

	udec_vout_info_t vout_info[IAV_NUM_VOUT];
	udec_t udec[MAX_NUM_DECODER];
	u32	udec_max_number;

	iav_udec_vout_config_t vout_config[IAV_NUM_VOUT];
	u8	vout_owner[IAV_NUM_VOUT];	// which udec is using this vout
	u8	vout_config_valid[IAV_NUM_VOUT];// is vout_config valid

	u8	enable_udec_irq_print;
	u8	disable_deintl;
	u8	enable_vm;
	u8	reserved;

	int	num_waiters;
	struct semaphore decode_sem;

	iav_udec_deint_config_t udec_deint_config;
	u8	deint_config_valid;

	iav_mdec_mode_config_t mdec_mode_config;
	udec_window_t windows[MAX_NUM_WINDOWS];
	udec_render_t renders[MAX_NUM_WINDOWS];

	//feature constrains related
	u32	input_codec_mask;
	iav_udec_mode_feature_constrain_t input_feature_constrains;
	u8	dsp_only_use_512k_smem;
	u8	always_disable_l2_cache;

	u8	max_mdec_window_number;
	u8	dewarp_feature_enabled;

	//last switch msg
	u32	last_switch_status[MAX_NUM_WINDOWS];

	//last capture msg
	u8	last_capture_dec_id;
	u8	last_capture_dec_status;
	u8	last_capture_msg_seqnum;
	u8	processed_last_capture_msg_seqnum;

	u32	main_coded_size;
	u32	thumb_coded_size;
	u32	screen_coded_size;

	//dewarp table
	s16*	dewarp_table;
	u32		dewarp_table_len;

	//dewarp params
	u32		horz_warp_enable;
	u32		warp_horizontal_table_address;

	u8		grid_array_width;
	u8		grid_array_height;
	u8		horz_grid_spacing_exponent;
	u8		vert_grid_spacing_exponent;

	//decoder capture
	u8	capture_context_setup;//debug check flag
	u8	capture_mem_allocated;
	u8	reserved11[2];
	iav_udec_capture_t	dec_capture;

	u8*	coded_capture_bsb_virtual;
	u8*	thumbnail_capture_bsb_virtual;
	u8*	screennail_capture_bsb_virtual;
	u8*	coded_capture_qt_virtual;
	u8*	thumbnail_capture_qt_virtual;
	u8*	screennail_capture_qt_virtual;

	u8*	coded_capture_bsb_dsp;
	u8*	thumbnail_capture_bsb_dsp;
	u8*	screennail_capture_bsb_dsp;
	u8*	coded_capture_qt_dsp;
	u8*	thumbnail_capture_qt_dsp;
	u8*	screennail_capture_qt_dsp;

	udec_mmap_t		coded_capture_bsb_mmap;
	udec_mmap_t		thumbnail_capture_bsb_mmap;
	udec_mmap_t		screennail_capture_bsb_mmap;
} iav_decode_obj_t;

static iav_decode_obj_t *G_decode_obj = NULL;
unsigned int g_decode_cmd_cnt[MAX_NUM_DECODER];
unsigned int g_decode_frame_cnt[MAX_NUM_DECODER];
unsigned int g_udec_debug_on = 0;

static struct {
	int	action;		// 0: no action; 1: send postp
	int	enable_video;
	u32	udec_id;	//
	u32	vout_id;
	u32	other_vout_id;
} g_vout_action;

static int decode_vout_preproc(iav_context_t *context, unsigned int cmd, unsigned long arg);
static void stop_udec(iav_context_t *context, udec_t *udec, int stop_flag);
static void release_udec(iav_context_t *context, udec_t *udec, int stop_flag);
static void __render_fb(int vout_id, int decoder_id, int fb_index, int valid);
static void flush_vout(iav_context_t *context, u32 vout_id);
static void clean_vout(iav_context_t *context, u32 vout_id);

static int dsp_udec_type(int udec_type)
{
	switch (udec_type) {
	case UDEC_NONE: return UDEC_TYPE_NULL;
	case UDEC_H264: return UDEC_TYPE_H264;
	case UDEC_MP12: return UDEC_TYPE_MP12;
	case UDEC_MP4H: return UDEC_TYPE_MP4H;
	case UDEC_MP4S: return UDEC_TYPE_MP4S;
	case UDEC_VC1:  return UDEC_TYPE_VC1;
	case UDEC_RV40: return UDEC_TYPE_RV40;
	case UDEC_JPEG: return UDEC_TYPE_STILL;
	case UDEC_SW: return UDEC_TYPE_SW;
	default: return UDEC_TYPE_NULL;
	}
}

static u8 __valid_tilemode_value(u8 value)
{
	if ((0 == value) || (4 == value) || (5 == value)) {
		return value;
	} else {
		UDEC_LOG_ERROR("not valid tilemode value %d\n", value);
	}
	return 5;
}

static inline udec_t *__udec(int udec_id)
{
	return G_decode_obj->udec + udec_id;
}

static inline iav_udec_info_ex_t *__udec_info(int udec_id)
{
	return &G_decode_obj->udec[udec_id].udec_info;
}

static inline udec_vout_info_t *__vout_info(int vout_id)
{
	return G_decode_obj->vout_info + vout_id;
}

static inline iav_udec_vout_config_t *__vout_config(int vout_id)
{
	return G_decode_obj->vout_config + vout_id;
}

static inline int __vout_owner(int vout_id)
{
	return G_decode_obj->vout_owner[vout_id];
}

static inline void __set_vout_owner(int vout_id, int udec_id)
{
	G_decode_obj->vout_owner[vout_id] = udec_id;
}

static inline iav_udec_config_t *__udec_config(int udec_id)
{
	return &G_decode_obj->udec[udec_id].udec_config;
}

static inline iav_udec_mode_config_t *__udec_mode_config(void)
{
	return &G_decode_obj->mdec_mode_config.super;
}

static inline int __get_udec_state(udec_t *udec)
{
	return atomic_read(&udec->udec_state);
}

static inline void __set_udec_state(udec_t *udec, int state)
{
	atomic_set(&udec->udec_state, state);
}

static inline int __get_vout_state(udec_t *udec)
{
	return atomic_read(&udec->vout_state);
}

static inline void __set_vout_state(udec_t *udec, int state)
{
	atomic_set(&udec->vout_state, state);
}

static inline void __check_feature_constrains(u8 vout_mask)
{
	UDEC_LOG_ALWAYS("check feature constrains: enable %d, codec mask 0x%x.\n", G_decode_obj->input_feature_constrains.set_constrains_enable, G_decode_obj->input_codec_mask);
	if (G_decode_obj->input_feature_constrains.set_constrains_enable) {
		if ((G_decode_obj->input_feature_constrains.h264_no_fmo) \
			&& (DSP_DEC_CODEC_BIT_H264 == G_decode_obj->input_codec_mask) \
			&& (1 == vout_mask)) {
				UDEC_LOG_ALWAYS("check feature constrains: h264 only, h264 no fmo, LCD only's case, try 512 k smem.\n");
				G_decode_obj->dsp_only_use_512k_smem = 1;
		} else if ((G_decode_obj->input_feature_constrains.h264_no_fmo) \
			&& (DSP_DEC_CODEC_BIT_H264 == G_decode_obj->input_codec_mask) \
			&& (2 == vout_mask)) {
				UDEC_LOG_ALWAYS("check feature constrains: h264 only, h264 no fmo, HDMI only's case, try 512 k smem.\n");
				G_decode_obj->dsp_only_use_512k_smem = 1;
		} else if ((DSP_DEC_CODEC_BIT_MPEG12 == G_decode_obj->input_codec_mask) \
			&& (1 == vout_mask)) {
				UDEC_LOG_ALWAYS("check feature constrains: mpeg12 only, LCD only's case, try 512 k smem.\n");
				G_decode_obj->dsp_only_use_512k_smem = 1;
		} else if ((DSP_DEC_CODEC_BIT_MPEG4H == G_decode_obj->input_codec_mask) \
			&& (1 == vout_mask)) {
				UDEC_LOG_ALWAYS("check feature constrains: mpeg4 H only, LCD only's case, try 512 k smem.\n");
				G_decode_obj->dsp_only_use_512k_smem = 1;
		} else if ((G_decode_obj->input_feature_constrains.h264_no_fmo) \
			&& (!(G_decode_obj->input_codec_mask & (~(0x1|0x2|0x4)))) \
			&& (1 == vout_mask)) {
				UDEC_LOG_ALWAYS("check feature constrains: h264/mpeg12/mpeg4 only, h264 no fmo, LCD only's case, try 512 k smem.\n");
				G_decode_obj->dsp_only_use_512k_smem = 1;
		}
	}
	UDEC_LOG_ALWAYS("check feature constrains: result, use 512k smem %d.\n", G_decode_obj->dsp_only_use_512k_smem);
}

#ifdef ENABLE_MULTI_WINDOW_UDEC
#define JPEG_QT_SIZE	128
//dec capture related
static void __init_jpeg_dqt(u8 *qtbl, int quality)
{
	static const u8 std_jpeg_qt[128] = {
		0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
		0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
		0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
		0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
		0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
		0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
		0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
		0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
		0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
		0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
	};

	int i, scale, temp;

	scale = (quality < 50) ? (5000 / quality) : (200 - quality * 2);

	for (i = 0; i < 128; i++) {
		temp = ((long) std_jpeg_qt[i] * scale + 50L) / 100L;
		/* limit the values to the valid range */
		if (temp <= 0L) temp = 1L;
		else if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
		qtbl[i] = temp;
	}

}

static u8* __alloc_memory(u32 size)
{
	u8 * addr = NULL;

	if ((addr = (u8*)kmalloc(size, GFP_KERNEL)) == NULL) {
		LOG_ERROR("Not enough memory to allocate memory %d!\n", size);
	}

	return addr;
}
#endif

static inline int is_udec_mode(void)
{
	return G_iav_info.state == IAV_STATE_DECODING && G_iav_obj.dec_type == DEC_TYPE_UDEC;
}

static int check_udec_mode(void)
{
	if (!is_udec_mode()) {
		iav_dbg_printk("not in udec mode\n");
		return -EPERM;
	}
	return 0;
}

static inline int get_udec_vout_id(udec_t *udec)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(G_decode_obj->vout_config); i++) {
		if (__vout_owner(i) == udec->udec_id)
			return i;
	}
	return -1;
}

static inline udec_t *get_udec(iav_context_t *context, u32 udec_id)
{
	if (udec_id >= G_decode_obj->udec_max_number || (context->udec_flags & (1 << udec_id)) == 0) {
		UDEC_LOG_ALWAYS("no such udec: %d\n", udec_id);
		return NULL;
	}
	return __udec(udec_id);
}

static inline fb_pool_t *get_fb_pool(udec_t *udec)
{
	return &udec->udec_fb_pool;
}

static inline void __udec_notify_waiters(udec_t *udec)
{
	while (udec->num_waiters > 0) {
		udec->num_waiters--;
		up(&udec->sem);
	}
}

static void udec_notify_waiters(udec_t *udec)
{
	unsigned long flags;
	dsp_lock(flags);
	__udec_notify_waiters(udec);
	dsp_unlock(flags);
}

static inline iav_frame_buffer_t *__fb_pool_get(fb_pool_t *fb_pool, int fb_index)
{
	return fb_pool->fb + fb_index;
}

static inline int __get_fb_state(fb_pool_t *fb_pool, int fb_index)
{
	return atomic_read(fb_pool->fb_state + fb_index);
}

static inline void __set_fb_state(fb_pool_t *fb_pool, int fb_index, int state)
{
	atomic_set(fb_pool->fb_state + fb_index, state);
}

static int fb_pool_alloc(fb_pool_t *fb_pool, int state)
{
	int i = fb_pool->index;
	while (1) {
		if (__get_fb_state(fb_pool, i) == FBS_INVALID) {
			if (fb_pool->num_fb >= MAX_DECODED_FRAMES)
				BUG();

			__set_fb_state(fb_pool, i, state);
			fb_pool->num_fb++;

			if ((fb_pool->index = i + 1) == MAX_DECODED_FRAMES)
				fb_pool->index = 0;

			break;
		}

		if (++i == MAX_DECODED_FRAMES)
			i = 0;

		if (i == fb_pool->index)
			BUG();
	}
	return i;
}

static int fb_pool_get_index(fb_pool_t *fb_pool, int fb_id)
{
	int i;
	for (i = 0; i < MAX_DECODED_FRAMES; i++) {
		if (__get_fb_state(fb_pool, i) != FBS_INVALID && fb_pool->fb[i].real_fb_id == fb_id)
			return i;
	}
	return -1;
}

static void __fb_pool_remove(fb_pool_t *fb_pool, int fb_index)
{
	unsigned long flags;

	dsp_lock(flags);

	if (__get_fb_state(fb_pool, fb_index) == FBS_INVALID) {
		iav_dbg_printk("fb_index: %d\n", fb_index);
		BUG();
	}

	__set_fb_state(fb_pool, fb_index, FBS_INVALID);
	fb_pool->num_fb--;

	dsp_unlock(flags);
}

static inline void cmd_release_fb(int udec_id, u16 real_fb_id, int flags)
{
	if ((flags & IAV_FRAME_NO_RELEASE) == 0) {
		dsp_mm_release_fb(real_fb_id);
		__udec(udec_id)->frames_released++;
	}
}

static void __fb_pool_release_all(fb_pool_t *fb_pool)
{
	int i;

	if (fb_pool->num_fb == 0)
		return;

	for (i = 0; i < MAX_DECODED_FRAMES; i++) {
		if (__get_fb_state(fb_pool, i) != FBS_INVALID) {
			iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, i);
			u16 flags = pframe->flags;
			u16 real_fb_id = pframe->real_fb_id;
			u8 decoder_id = pframe->decoder_id;

			__set_fb_state(fb_pool, i, FBS_INVALID);
			cmd_release_fb(decoder_id, real_fb_id, flags);
		}
	}

	fb_pool->num_fb = 0;
}

static inline void fb_queue_init(fb_queue_t *queue)
{
	sema_init(&queue->sem, 0);
}

static inline void __fb_queue_reset(fb_queue_t *queue)
{
	queue->num_fb = 0;
	queue->write_index = 0;
	queue->read_index = 0;
}

static inline void __fb_queue_push(fb_queue_t *queue, int fb_index)
{
	if (queue->num_fb >= MAX_DECODED_FRAMES) {
		iav_dbg_printk("queue is full\n");
	} else {
		queue->num_fb++;
	}

	queue->fb_index[queue->write_index] = fb_index;
	if (++queue->write_index == MAX_DECODED_FRAMES)
		queue->write_index = 0;
}

static inline int __fb_queue_pop(fb_queue_t *queue)
{
	int fb_index = queue->fb_index[queue->read_index];
	if (++queue->read_index == MAX_DECODED_FRAMES)
		queue->read_index = 0;
	queue->num_fb--;
	return fb_index;
}

static void __release_fb(u8 decoder_id, u16 fb_index)
{
	udec_t *udec = __udec(decoder_id);
	fb_pool_t *fb_pool = get_fb_pool(udec);
	iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb_index);
	u16 flags = pframe->flags;
	u16 real_fb_id = pframe->real_fb_id;
	pframe->flags &= ~IAV_FRAME_NO_RELEASE;
	__fb_pool_remove(fb_pool, fb_index);
	cmd_release_fb(decoder_id, real_fb_id, flags);
}

static udec_t *get_udec_by_msg(u32 udec_id, u32 msg_code)
{
	udec_t *udec;

	if (udec_id >= G_decode_obj->udec_max_number) {
		UDEC_LOG_ERROR("unknown udec msg, decoder_id=%d\n", udec_id);
		return NULL;
	}

	udec = __udec(udec_id);
	if (udec->udec_inuse == 0) {
		//UDEC_LOG_ERROR("udec[%d] not used, but received msg 0x%x\n", udec_id, msg_code);
		return NULL;
	}

	return udec;
}

static inline int get_error_level(int error_code)
{
	return (error_code >> 16) & 0xFF;
}

static u8 *map_udec_fifo(iav_context_t *context, const char *name,
	u32 base, u32 size, udec_mmap_t *mmap, int noncached)
{
	u8 *user_start;

	iav_dbg_printk("%s start: 0x%x, size %u\n", name, base, size);
	user_start = iav_create_mmap(context, base, ambarella_phys_to_virt(base), size, PROT_READ | PROT_WRITE, noncached, NULL);
	if (user_start == NULL) {
		iav_dbg_printk("mmap failed\n");
		return NULL;
	}
	iav_dbg_printk("user start: 0x%x, size = 0x%x\n", (u32)user_start, size);

	mmap->phys_start = base;
	mmap->phys_end = base + size;
	mmap->virt_start = ambarella_phys_to_virt(base);
	mmap->virt_end = ambarella_phys_to_virt(base) + size;
	mmap->user_start = user_start;
	mmap->user_end = user_start + size;
	mmap->size = size;

	mmap->unmap_start = (unsigned long)user_start - G_iav_info.mmap_page_offset;
	mmap->unmap_size = G_iav_info.mmap_size;

	return user_start;
}

#ifdef ENABLE_MULTI_WINDOW_UDEC
static u8 *map_udec_playback_capture(iav_context_t *context, const char *name,
	u32 base, u32 size, udec_mmap_t *mmap)
{
	u8 *user_start;

	iav_dbg_printk("%s start: 0x%x\n", name, base);
	user_start = iav_create_mmap(context, base, phys_to_virt(base), size, PROT_READ | PROT_WRITE, 0, NULL);
	if (user_start == NULL) {
		iav_dbg_printk("mmap failed\n");
		return NULL;
	}
	iav_dbg_printk("user start: 0x%x, size = 0x%x\n", (u32)user_start, size);

	mmap->phys_start = base;
	mmap->phys_end = base + size;
	mmap->user_start = user_start;
	mmap->user_end = user_start + size;
	mmap->size = size;

	mmap->unmap_start = (unsigned long)user_start - G_iav_info.mmap_page_offset;
	mmap->unmap_size = G_iav_info.mmap_size;

	return user_start;
}
#endif

static void destroy_mmap(iav_context_t *context, udec_mmap_t *mmap)
{
	if (mmap->unmap_start) {
		iav_destroy_mmap(context, mmap->unmap_start, mmap->unmap_size);
		mmap->unmap_start = 0;
		mmap->user_start = 0;
	}
}

#ifdef ENABLE_MULTI_WINDOW_UDEC
static int __alloc_dec_capture_mem(void)
{
	UDEC_DEBUG_ASSERT(!G_decode_obj->capture_mem_allocated);
	if (G_decode_obj->capture_mem_allocated) {
		return 0;
	}

	//alloc memory
	UDEC_DEBUG_ASSERT(!G_decode_obj->coded_capture_bsb_virtual);
	if (!G_decode_obj->coded_capture_bsb_virtual) {
		G_decode_obj->coded_capture_bsb_virtual = __alloc_memory(MAX_DEC_CAPTURED_ENC_SIZE);
	}
	if (!G_decode_obj->coded_capture_bsb_virtual) {
		UDEC_LOG_ERROR("NO memory for coded_capture_bsb_virtual\n");
		return -1;
	}

	UDEC_DEBUG_ASSERT(!G_decode_obj->thumbnail_capture_bsb_virtual);
	if (!G_decode_obj->thumbnail_capture_bsb_virtual) {
		G_decode_obj->thumbnail_capture_bsb_virtual = __alloc_memory(MAX_DEC_THUMBNNAIL_CAPTURED_ENC_SIZE);
	}
	if (!G_decode_obj->thumbnail_capture_bsb_virtual) {
		UDEC_LOG_ERROR("NO memory for thumbnail_capture_bsb_virtual\n");
		return -1;
	}

	UDEC_DEBUG_ASSERT(!G_decode_obj->screennail_capture_bsb_virtual);
	if (!G_decode_obj->screennail_capture_bsb_virtual) {
		G_decode_obj->screennail_capture_bsb_virtual = __alloc_memory(MAX_DEC_CAPTURED_ENC_SIZE);
	}
	if (!G_decode_obj->screennail_capture_bsb_virtual) {
		UDEC_LOG_ERROR("NO memory for screennail_capture_bsb_virtual\n");
		return -1;
	}

	UDEC_DEBUG_ASSERT(!G_decode_obj->coded_capture_qt_virtual);
	if (!G_decode_obj->coded_capture_qt_virtual) {
		G_decode_obj->coded_capture_qt_virtual = __alloc_memory(JPEG_QT_SIZE);
	}
	if (!G_decode_obj->coded_capture_qt_virtual) {
		UDEC_LOG_ERROR("NO memory for coded_capture_qt_virtual\n");
		return -1;
	}

	UDEC_DEBUG_ASSERT(!G_decode_obj->thumbnail_capture_qt_virtual);
	if (!G_decode_obj->thumbnail_capture_qt_virtual) {
		G_decode_obj->thumbnail_capture_qt_virtual = __alloc_memory(JPEG_QT_SIZE);
	}
	if (!G_decode_obj->thumbnail_capture_qt_virtual) {
		UDEC_LOG_ERROR("NO memory for thumbnail_capture_qt_virtual\n");
		return -1;
	}

	UDEC_DEBUG_ASSERT(!G_decode_obj->screennail_capture_qt_virtual);
	if (!G_decode_obj->screennail_capture_qt_virtual) {
		G_decode_obj->screennail_capture_qt_virtual = __alloc_memory(JPEG_QT_SIZE);
	}
	if (!G_decode_obj->screennail_capture_qt_virtual) {
		UDEC_LOG_ERROR("NO memory for screennail_capture_qt_virtual\n");
		return -1;
	}

	G_decode_obj->capture_mem_allocated = 1;
	return 0;
}

static void __free_dec_capture_mem(void)
{
	//release memory
	UDEC_DEBUG_ASSERT(G_decode_obj->coded_capture_bsb_virtual);
	if (G_decode_obj->coded_capture_bsb_virtual) {
		kfree(G_decode_obj->coded_capture_bsb_virtual);
		G_decode_obj->coded_capture_bsb_virtual = NULL;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->thumbnail_capture_bsb_virtual);
	if (G_decode_obj->thumbnail_capture_bsb_virtual) {
		kfree(G_decode_obj->thumbnail_capture_bsb_virtual);
		G_decode_obj->thumbnail_capture_bsb_virtual = NULL;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->screennail_capture_bsb_virtual);
	if (G_decode_obj->screennail_capture_bsb_virtual) {
		kfree(G_decode_obj->screennail_capture_bsb_virtual);
		G_decode_obj->screennail_capture_bsb_virtual = NULL;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->coded_capture_qt_virtual);
	if (G_decode_obj->coded_capture_qt_virtual) {
		kfree(G_decode_obj->coded_capture_qt_virtual);
		G_decode_obj->coded_capture_qt_virtual = NULL;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->thumbnail_capture_qt_virtual);
	if (G_decode_obj->thumbnail_capture_qt_virtual) {
		kfree(G_decode_obj->thumbnail_capture_qt_virtual);
		G_decode_obj->thumbnail_capture_qt_virtual = NULL;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->screennail_capture_qt_virtual);
	if (G_decode_obj->screennail_capture_qt_virtual) {
		kfree(G_decode_obj->screennail_capture_qt_virtual);
		G_decode_obj->screennail_capture_qt_virtual = NULL;
	}

	G_decode_obj->capture_mem_allocated = 0;
}
#endif

static void handle_udec_msg(void *context, unsigned cat, DSP_MSG *msg, int port)
{
	//UDEC_LOG_ALWAYS("handle_udec_msg comes\n");
	switch (msg->msg_code) {
	case MSG_UDEC_STATUS: {

			UDEC_STATUS_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);

			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL) {
				//UDEC_LOG_ERROR("NULL udec here, decoder_id %d\n", udec_msg->decoder_id);
				break;
			}

			udec->vdsp_new_seq_number ++;
			__set_udec_state(udec, udec_msg->decoder_state);
			__set_vout_state(udec, udec_msg->vout_state);

			iav_dbg_printk("MSG_UDEC_STATUS[%d] udec_state %d, vout_state %d\n", udec_msg->decoder_id, udec_msg->decoder_state, udec_msg->vout_state);

			udec->space_id = udec_msg->dram_sp_id;
			udec->fbp_id = udec_msg->frame_fbp_id;

			udec->bits_fifo_id = udec_msg->bs_fifo_cpb_id;
			udec->mv_fifo_id = udec_msg->mv_fifo_cpb_id;
			udec->bits_fifo_base = udec_msg->bs_fifo_base;
			udec->mv_fifo_base = udec_msg->mv_fifo_base;
			udec->num_decoded_frames = udec_msg->total_decoded_pic_num;
			udec->curr_frame = udec_msg->cur_decoded_pic_num;

			if (get_error_level(udec_msg->error_code) >= get_error_level(udec->error_code)) {
				udec->error_code = udec_msg->error_code;
				udec->error_pic_pts_low = udec_msg->last_pic_pts_low;
				udec->error_pic_pts_high = udec_msg->last_pic_pts_high;
			}
			//UDEC_LOG_ALWAYS("MSG_UDEC_STATUS comes, udec_msg->decoder_state %d\n", udec_msg->decoder_state);
			__udec_notify_waiters(udec);
		}
		break;

	case MSG_UDEC_OUTPIC: {

			UDEC_OUTPIC_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);

			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec) {

				fb_pool_t *fb_pool = get_fb_pool(udec);
				iav_frame_buffer_t *pframe;
				int fb_index;

				udec->vdsp_new_seq_number ++;
				if (udec->udec_type != UDEC_RV40) {
					udec->frames_received++;
					if (__get_udec_state(udec) != UDEC_STAT_RUN)
						iav_dbg_printk("OUTPIC received in state %d !!!\n", __get_udec_state(udec));

					fb_index = fb_pool_alloc(fb_pool, FBS_OUTPIC);
					pframe = __fb_pool_get(fb_pool, fb_index);

					pframe->flags = (udec_msg->decoder_type == UDEC_TYPE_STILL) ? IAV_FRAME_NO_RELEASE : 0;

					pframe->fb_format = udec_msg->tiled_fmt;
					pframe->chroma_format = udec_msg->chroma_fmt;

					pframe->fb_id = udec_msg->frm_buf_id;
					pframe->real_fb_id = udec_msg->frm_buf_id;
					pframe->pic_width = pframe->buffer_width = udec_msg->pic_width;
					pframe->pic_height = pframe->buffer_height = udec_msg->pic_height;
					pframe->buffer_pitch = udec_msg->buf_pitch;
					pframe->decoder_id = udec_msg->decoder_id;
					pframe->eos_flag = 0;

					pframe->pts = udec_msg->pts_low;
					pframe->pts_high = udec_msg->pts_high;

					pframe->top_or_frm_word = udec_msg->pic_info_word_top_or_frm;
					pframe->bot_word = udec_msg->pic_info_word_bot;

					pframe->lu_buf_addr = (void*)udec_msg->luma_buf_base_addr;
					pframe->ch_buf_addr = (void*)udec_msg->chroma_buf_base_addr;
				} else {
					fb_index = fb_pool_get_index(fb_pool, udec_msg->frm_buf_id);
					if (fb_index < 0)
						BUG();
				}

				__fb_queue_push(&udec->outpic_queue, fb_index);

				__udec_notify_waiters(udec);
			}
		}
		break;

	case MSG_UDEC_FIFO_STATUS: {

			UDEC_FIFO_STATUS_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);

			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL)
				break;

			udec->vdsp_new_seq_number ++;
			udec->dsp_bits_fifo_next_phys = DSP_TO_PHYS(udec_msg->bits_fifo_cur_pos);
			udec->dsp_mv_fifo_next_phys = DSP_TO_PHYS(udec_msg->mv_fifo_cur_pos);

#ifdef VERBOSE_LOG
			//add print if bit-fifo is under flow
			if (udec->dsp_bits_fifo_next_phys < udec->dsp_bits_fifo_last_write_phys) {
				if (udec->dsp_bits_fifo_last_write_phys < (udec->dsp_bits_fifo_next_phys + 8*1024)) {
					UDEC_LOG_ALWAYS("warning: udec[%d]'s bits-fifo has less than 8*1024 bytes, %d.\n", \
						udec_msg->decoder_id, udec->dsp_bits_fifo_last_write_phys - udec->dsp_bits_fifo_next_phys);
				}
			}
#endif

			__udec_notify_waiters(udec);
		}
		break;

	case MSG_UDEC_DISP_STATUS: {
			UDEC_DISP_STATUS_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL)
				break;

			udec->vdsp_new_seq_number ++;
			if (!(udec->udec_status & UDEC_STATUS_DISP) ||
					udec->disp_status.latest_pts_low != udec_msg->latest_pts_low ||
					udec->disp_status.latest_pts_high != udec_msg->latest_pts_high) {

				//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);

				udec->disp_status = *udec_msg;
				udec->udec_status |= UDEC_STATUS_DISP;
				udec->pts_comes = 1;
				__udec_notify_waiters(udec);
			}
		}
		break;

	case MSG_UDEC_EOS_STATUS: {
			UDEC_EOS_STATUS_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);

			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL)
				break;

			udec->vdsp_new_seq_number ++;
			udec->eos_pts_low = udec_msg->last_pts_low;
			udec->eos_pts_high = udec_msg->last_pts_high;
			udec->udec_status |= UDEC_STATUS_EOS;

			__udec_notify_waiters(udec);
		}
		break;

#ifdef ENABLE_MULTI_WINDOW_UDEC
	case MSG_UDEC_STILL_CAP: {
			UDEC_STILL_CAP_MSG* udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);
			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL)
				break;

			G_decode_obj->last_capture_dec_id = udec_msg->decoder_id;
			G_decode_obj->last_capture_dec_status = udec_msg->status;
			G_decode_obj->last_capture_msg_seqnum ++;
			G_decode_obj->main_coded_size = udec_msg->main_coded_size;
			G_decode_obj->thumb_coded_size = udec_msg->thumb_coded_size;
			G_decode_obj->screen_coded_size = udec_msg->screen_coded_size;

			__udec_notify_waiters(udec);
		}
		break;
#endif

	case MSG_UDEC_ABORT_STATUS: {
			UDEC_ABORT_STATUS_MSG *udec_msg = (void*)msg;
			udec_t *udec;

			//dsp_save_msg(udec_msg, sizeof(*udec_msg), port);
			udec = get_udec_by_msg(udec_msg->decoder_id, msg->msg_code);
			if (udec == NULL)
				break;

			udec->vdsp_new_seq_number ++;
			udec->abort_flag = 1;
			udec->code_timeout_pc = udec_msg->code_timeout_pc;
			udec->code_timeout_reason = udec_msg->code_timeout_reason;
			udec->mdxf_timeout_pc = udec_msg->mdxf_timeout_pc;
			udec->mdxf_timeout_reason = udec_msg->mdxf_timeout_reason;

			__udec_notify_waiters(udec);
		}
		break;

	default:
		UDEC_LOG_ERROR("unknown msg, cat = %d, code = 0x%x\n", cat, msg->msg_code);
		break;
	}

	while (G_decode_obj->num_waiters > 0) {
		G_decode_obj->num_waiters--;
		up(&G_decode_obj->decode_sem);
	}
}

static void handle_postp_msg(void *context, unsigned cat, DSP_MSG *msg, int port)
{
	//UDEC_LOG_ALWAYS("handle_postp_msg comes\n");
	switch (msg->msg_code) {
#ifdef ENABLE_MULTI_WINDOW_UDEC
	case MSG_POSTPROC_MW_SWITCH_STATUS: {
			POSTPROC_MW_SWITCH_STATUS_MSG *postp_msg = (void*)msg;

			//dsp_save_msg(postp_msg, sizeof(*postp_msg), port);
			if (postp_msg->render_id < MAX_NUM_WINDOWS) {
				UDEC_DEBUG_ASSERT(INITIAL_SWITCH_STATUS == G_decode_obj->last_switch_status[postp_msg->render_id]);
				G_decode_obj->last_switch_status[postp_msg->render_id] = postp_msg->switch_status;
			} else {
				UDEC_LOG_ERROR("BAD render id %d, in switch status msg.\n", postp_msg->render_id);
			}
		}
		break;

	case MSG_POSTPROC_MW_SCREEN_SHOT_STATUS: {
			POSTPROC_MW_SCREEN_SHOT_STATUS_MSG* postp_msg = (void*)msg;

			//dsp_save_msg(postp_msg, sizeof(*postp_msg), port);

			G_decode_obj->last_capture_dec_status = postp_msg->status;
			G_decode_obj->last_capture_msg_seqnum ++;
			G_decode_obj->main_coded_size = postp_msg->main_coded_size;
			G_decode_obj->thumb_coded_size = postp_msg->thumb_coded_size;
			G_decode_obj->screen_coded_size = postp_msg->screen_coded_size;
		}
		break;
#endif
	default:
		UDEC_LOG_ERROR("unknown msg, cat = %d, code = 0x%x\n", cat, msg->msg_code);
		break;
	}

	while (G_decode_obj->num_waiters > 0) {
		G_decode_obj->num_waiters--;
		up(&G_decode_obj->decode_sem);
	}
}

// runs in vout irq
static void __iav_decode_vout_irq(unsigned vout_id)
{
	udec_vout_info_t *vout_info;
	vout_default_info_t *default_info;
	vout_fb_t *fb;
	u32 state;

	if (vout_id >= IAV_NUM_VOUT)
		return;

	vout_info = __vout_info(vout_id);
	vout_info->irq_entered = 1;
	vout_info->irq_counter++;

	if (vout_info->vout_inuse == 0)
		return;

	default_info = iav_vout_default_info(vout_id);

	if (G_decode_obj->disable_deintl == 0 && G_iav_info.pvoutinfo[vout_id]->active_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) {

		state = default_info->fld_polarity;

		if (vout_info->intl_state != state) {
			vout_info->intl_state = state;
			//iav_dbg_printk("state = 0x%x\n", state);
		} else {
			//iav_dbg_printk("not changed\n");
		}

		if (state == 0)
			return;
	}

	if (vout_info->rendered) {

		BUG_ON(vout_info->A.valid);

		vout_info->A = vout_info->B;
		vout_info->B = vout_info->C;
		vout_info->C = vout_info->D;
		vout_info->D.valid = 0;
		vout_info->rendered = 0;

		fb = &vout_info->A;
		if (fb->valid) {
			__udec_notify_waiters(__udec(fb->decoder_id));
		}

		fb = &vout_info->B;
		if (fb->valid) {
			__udec_notify_waiters(__udec(fb->decoder_id));
		}
	}

	fb = &vout_info->E;
	if (fb->render) {
		vout_info->D = *fb;

		if (fb->valid == 0) {
			default_info->y_addr = 0;
			default_info->uv_addr = 0;
			default_info->pitch = 0;
			default_info->repeat_field = 0;
		} else {
			udec_t *udec = __udec(fb->decoder_id);
			fb_pool_t *fb_pool = get_fb_pool(udec);
			iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb->fb_index);

			default_info->y_addr = (u32)pframe->lu_buf_addr;
			default_info->uv_addr = (u32)pframe->ch_buf_addr;
			default_info->pitch = pframe->buffer_pitch;
			default_info->repeat_field = 0;
		}

		clean_d_cache(default_info, sizeof(*default_info));

		fb->valid = 0;
		fb->render = 0;

		vout_info->rendered = 1;
	}

	while (vout_info->num_waiters) {
		vout_info->num_waiters--;
		up(&vout_info->sem);
	}
}

static inline void init_udec_mmap(udec_mmap_t *mmap)
{
	mmap->phys_start = 0;
	mmap->user_start = 0;
	mmap->size = 0;
}

static void udec_internal_init(int i)
{
	udec_t *udec = __udec(i);

	udec->udec_id = i;
	udec->num_waiters = 0;
	udec->error_code = 0;
	udec->vdsp_ori_seq_number = 0;
	udec->vdsp_new_seq_number = 0;

	__set_udec_state(udec, UDEC_STAT_INVALID);
	__set_vout_state(udec, IAV_VOUT_STATE_INVALID);
	udec->prev_vout_state = -1;

	init_udec_mmap(&udec->bits_fifo);
	init_udec_mmap(&udec->mv_fifo);
	init_udec_mmap(&udec->jpeg_y);
	init_udec_mmap(&udec->jpeg_uv);

	fb_queue_init(&udec->outpic_queue);

	sema_init(&udec->sem, 0);
}

static void vout_info_init(int i)
{
	udec_vout_info_t *vout_info = __vout_info(i);
	sema_init(&vout_info->sem, 0);
}

int iav_decode_init(void *dev)
{
	int i;

	if ((G_decode_obj = kzalloc(sizeof(iav_decode_obj_t), GFP_KERNEL)) == NULL)
		return -ENOMEM;

	if (G_iav_debug_info) {
		G_iav_debug_info->kernel_start = (u32)PAGE_OFFSET;
		G_iav_debug_info->phys_offset = (u32)PHYS_OFFSET;
		G_iav_debug_info->decode_obj_addr = (u32)virt_to_phys((void*)G_decode_obj);
		G_iav_debug_info->dsp_obj_addr = (u32)virt_to_phys((void*)dsp_get_obj_addr());
	}

	sema_init(&G_decode_obj->decode_sem, 0);

	G_decode_obj->disable_deintl = 0;
	G_decode_obj->dewarp_table = NULL;
	G_decode_obj->dewarp_table_len = 0;

	for (i = 0; i < MAX_NUM_DECODER; i++)
		udec_internal_init(i);

	for (i = 0; i < IAV_NUM_VOUT; i++)
		vout_info_init(i);

	//iav_init_vout_irq(dev);

#ifdef ENABLE_MULTI_WINDOW_UDEC
	__alloc_dec_capture_mem();
#endif

	return 0;
}

void iav_decode_deinit(void)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	__free_dec_capture_mem();
#endif
}

static int __wait_udec_state(udec_t *udec, int state, int state2, const char *desc)
{
	int _state;

	iav_dbg_printk("wait udec[%d] state '%s'\n", udec->udec_id, desc);
	while (1) {
		unsigned long flags;

		dsp_lock(flags);

		_state = __get_udec_state(udec);
		if (_state == state || _state == state2) {
			dsp_unlock(flags);
			break;
		}

		udec->num_waiters++;
		dsp_unlock(flags);

		down(&udec->sem);
	}
	iav_dbg_printk("wait '%s' done\n", desc);

	return _state;
}

static void __wait_udec_vout_state(udec_t *udec, int vout_state, int udec_keep_state, const char *desc)
{
	int _vout_state;
	int _udec_keep_state;

	iav_dbg_printk("wait (udec[%d] always in state %d) vout target state[%d], '%s'\n", udec->udec_id, udec_keep_state, vout_state, desc);
	while (1) {
		unsigned long flags;

		dsp_lock(flags);

		_vout_state = __get_vout_state(udec);
		_udec_keep_state = __get_udec_state(udec);
		if (_vout_state == vout_state) {
			dsp_unlock(flags);
			break;
		}

		if (_udec_keep_state != udec_keep_state) {
			dsp_unlock(flags);
			LOG_ERROR("udec state changes(%d), not expected(%d).\n", udec_keep_state, _udec_keep_state);
			break;
		}

		udec->num_waiters++;
		dsp_unlock(flags);

		down(&udec->sem);
	}

	iav_dbg_printk("wait '%s' done\n", desc);
}

#define wait_udec_state(_udec, _state) \
	__wait_udec_state(_udec, _state, -1, #_state)

static int wait_decode_msg(iav_context_t *context)
{
	mutex_unlock(context->mutex);
	if (down_interruptible(&G_decode_obj->decode_sem)) {
		unsigned long flags;
		dsp_lock(flags);
		G_decode_obj->num_waiters--;
		dsp_unlock(flags);
		mutex_lock(context->mutex);
		return -EINTR;
	}
	mutex_lock(context->mutex);
	return 0;
}

static int wait_udec_msg(iav_context_t *context, udec_t *udec)
{
	mutex_unlock(context->mutex);
	if (down_interruptible(&udec->sem)) {
		unsigned long flags;
		dsp_lock(flags);
		udec->num_waiters--;
		dsp_unlock(flags);
		mutex_lock(context->mutex);
		return -EINTR;
	}
	mutex_lock(context->mutex);
	return 0;
}

static u32 bsb_user_to_dsp(iav_context_t *context, u8 *addr)
{
	struct iav_addr_map *bsb = &context->bsb;
	struct iav_mem_block *bsb_block = NULL;
	u32 rval;

	if (addr < bsb->user_start || addr >= bsb->user_end) {
		iav_dbg_printk("bad address, addr = 0x%p, start = 0x%p, end = 0x%p\n",
			addr, bsb->user_start, bsb->user_end);
		return -1;
	}

	iav_get_mem_block(IAV_MMAP_BSB, &bsb_block);
	if (bsb->flags & IAV_MAP2) {
		if (addr > bsb->user_start + bsb_block->size) {
			rval = (u32)((addr - bsb->user_start - bsb_block->size)
				+ bsb_block->phys_start);
		} else {
			rval = (u32)(addr - bsb->user_start + bsb_block->phys_start);
		}
	} else {
		rval = (u32)(addr - bsb->user_start + bsb_block->phys_start);
	}

	return rval;
}

static u32 udec_user_to_dsp(udec_mmap_t *mmap, u8 *addr)
{
	u32 result;

	//tmp fix code
	if (addr == mmap->user_end) {
		UDEC_LOG_ERROR("please ensure the start address's range [start, end), "
			"can not same with end! (addr %p, start %p, end %p)\n",
			addr, mmap->user_start, mmap->user_end);
		addr = mmap->user_start;
	}

	if (addr < mmap->user_start || addr >= mmap->user_end) {
		UDEC_LOG_ERROR("bad address, addr = 0x%p, start = 0x%p, end = 0x%p\n",
			addr, mmap->user_start, mmap->user_end);
		return -1;
	}
	result = (u32)(addr - mmap->user_start) + mmap->phys_start;
	if (result == mmap->phys_end)
		result = mmap->phys_start;
	return result;
}

static u32 udec_user_to_dsp_end(udec_mmap_t *mmap, u8 *addr)
{
	//tmp fix code
	if (addr == mmap->user_start) {
		UDEC_LOG_ERROR("please ensure the end address's range (start, end], "
			"can not same with start! (addr %p, start %p, end %p)\n",
			addr, mmap->user_start, mmap->user_end);
		addr = mmap->user_end;
	}

	if (addr <= mmap->user_start || addr > mmap->user_end) {
		UDEC_LOG_ERROR("bad end address, addr = 0x%p, start = 0x%p, end = 0x%p\n",
			addr, mmap->user_start, mmap->user_end);
		return -1;
	}
	return (u32)(addr - mmap->user_start) + mmap->phys_start;
}

static inline u8 *bsb_phys_to_user(iav_context_t *context, u32 addr)
{
	struct iav_mem_block *bsb = NULL;

	iav_get_mem_block(IAV_MMAP_BSB, &bsb);
	return (addr - bsb->phys_start) + context->bsb.user_start;
}

// dsp address to user address
static inline void *dsp_to_user(iav_context_t *context, u32 addr)
{
	struct iav_mem_block *dsp = NULL;
	addr = DSP_TO_PHYS(addr);
	iav_get_mem_block(IAV_MMAP_DSP, &dsp);
	return (void*)(addr - dsp->phys_start + context->dsp.user_start);
}

static int get_udec_bits_range(udec_mmap_t *mmap,
	u8 *u_start_addr, u8 *u_end_addr, u32 *start_addr, u32 *end_addr)
{
	if (mmap->user_start == NULL) {
		iav_dbg_printk("not mapped\n");
		return -EFAULT;
	}

	if (u_start_addr == mmap->user_end) {
		UDEC_LOG_ERROR("Please take care about start pointer, it's range "
			"would be [start, end), can not be same with bsb end pointer\n");
		u_start_addr = mmap->user_start;
	}

	if (u_end_addr == mmap->user_start) {
		UDEC_LOG_ERROR("Please take care about end pointer, it's range "
			"would be (start, end], can not be same with bsb start pointer\n");
		u_end_addr = mmap->user_end;
	}

	*start_addr = udec_user_to_dsp(mmap, u_start_addr);
	if ((int)*start_addr == -1)
		return -EFAULT;

	*end_addr = udec_user_to_dsp_end(mmap, u_end_addr);
	if ((int)*end_addr == -1)
		return -EFAULT;

	return 0;
}

#if 0
#define CACHE_LINE_SIZE	32
static inline void clean_cache_aligned(u8 *start, unsigned long size)
{
	unsigned long offset = (unsigned long)start & (CACHE_LINE_SIZE - 1);
	start -= offset;
	size += offset;
	clean_d_cache(start, size);
}
#endif

static void clean_fifo_cache(udec_mmap_t *mmap, u32 start_addr, u32 end_addr)
{
	if (end_addr > start_addr) {
		clean_cache_aligned((u8 *)ambarella_phys_to_virt(start_addr),
			end_addr - start_addr);
	} else {
		u32 size = mmap->phys_end - start_addr;
		if (size > 0)
			clean_cache_aligned((u8 *)ambarella_phys_to_virt(start_addr), size);

		size = end_addr - mmap->phys_start;
		if (size > 0)
			clean_cache_aligned((u8 *)ambarella_phys_to_virt(mmap->phys_start), size);
	}
}

static void clean_fb_cache(iav_context_t *context, udec_t *udec, int fb_index)
{
	fb_pool_t *fb_pool = get_fb_pool(udec);
	iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb_index);
	u32 size;

	if (context->dsp.user_start == NULL) {
		iav_dbg_printk("DSP memory not mapped!\n");
		return;
	}

	size = pframe->buffer_pitch * pframe->buffer_height;

	clean_cache_aligned(dsp_to_user(context, (u32)pframe->lu_buf_addr), size);
	clean_cache_aligned(dsp_to_user(context, (u32)pframe->ch_buf_addr), size / 2);
}

static void invalidate_fb_cache(iav_context_t *context, udec_t *udec, int fb_index)
{
	fb_pool_t *fb_pool = get_fb_pool(udec);
	iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb_index);
	u32 size;

	if (context->dsp.user_start == NULL) {
		iav_dbg_printk("DSP memory not mapped!\n");
		return;
	}

	size = pframe->buffer_pitch * pframe->buffer_height;

	invalidate_d_cache(dsp_to_user(context, (u32)pframe->lu_buf_addr), size);
	invalidate_d_cache(dsp_to_user(context, (u32)pframe->ch_buf_addr), size / 2);
}

// IAV_STATE_DECODING -> IAV_STATE_IDLE
int iav_leave_decode_mode(iav_context_t *context)
{
	if (!is_mode_flag_set(context, IAV_MODE_UDEC))
		return -EPERM;

	// check state
	if (G_iav_info.state != IAV_STATE_DECODING) {
		iav_dbg_printk("leave_decode_mode: bad state %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (G_iav_obj.dec_type == DEC_TYPE_UDEC) {
		int i;
		for (i = 0; i < G_decode_obj->udec_max_number; i++) {
			udec_t *udec = __udec(i);
			if (udec->udec_inuse) {
				release_udec(context, udec, 0);
				//iav_dbg_printk("leave_decode_mode: udec[%d] is in use\n", i);
				//return -EBUSY;
			}
		}

		dsp_set_mode(DSP_OP_MODE_IDLE, NULL, NULL);
	} else {
		// todo
	}

	iav_vout_set_preproc(NULL);

	G_iav_info.state = IAV_STATE_IDLE;
	G_iav_obj.dec_type = DEC_TYPE_NONE;
	G_iav_obj.dec_state = DEC_STATE_IDLE;

	clear_mode_flag(context, IAV_MODE_UDEC);

	return 0;
}

static void cmd_set_udec_mode(iav_udec_mode_config_t *mode)
{
	DSP_SET_OP_MODE_UDEC_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_DSP_SET_OPERATION_MODE;
	dsp_cmd.dsp_op_mode = DSP_OP_MODE_UDEC;

	dsp_cmd.mode_flags = mode->postp_mode;

    // config default video color
	dsp_cmd.pp_background_Y = mode->pp_background_Y;
	dsp_cmd.pp_background_Cb = mode->pp_background_Cb;
	dsp_cmd.pp_background_Cr = mode->pp_background_Cr;

	if (mode->enable_deint)
		dsp_cmd.mode_flags |= UDEC_MODE_DEINT_ENABLED;

	if (1 == G_decode_obj->dewarp_feature_enabled) {
		dsp_cmd.mode_flags |= UDEC_MODE_WARP_ENABLED;
	}

//	if (mode->enable_error_mode)
//		dsp_cmd.mode_flags |= UDEC_MODE_ERROR_ENABLED;

	dsp_cmd.num_of_instances = mode->num_udecs;
	dsp_cmd.pp_chroma_fmt_max = mode->pp_chroma_fmt_max;
	dsp_cmd.pp_max_frm_width = mode->pp_max_frm_width;
	dsp_cmd.pp_max_frm_height = mode->pp_max_frm_height;
	dsp_cmd.pp_max_frm_num = mode->pp_max_frm_num;

	if (G_decode_obj->dsp_only_use_512k_smem) {
		//fix me, only LCD/HDMI's case
		if (0x3 == mode->vout_mask) {
			dsp_cmd.dual_vout = 1;
		}
	} else {
		//if not use L2, always set this field to 1
		dsp_cmd.dual_vout = 1;
	}
	UDEC_LOG_ALWAYS("dual vout is set to %d, voutmask 0x%x, dsp_cmd.mode_flags 0x%x.\n", dsp_cmd.dual_vout, mode->vout_mask, dsp_cmd.mode_flags);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_udec_init(int id, iav_udec_config_t *udec_config)
{
	UDEC_INIT_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_INIT;
	dsp_cmd.decoder_id = id;
	dsp_cmd.dsp_dram_sp_id = dsp_get_mode_space_id();

	dsp_cmd.tiled_mode = __valid_tilemode_value(udec_config->tiled_mode);
	dsp_cmd.frm_chroma_fmt_max = udec_config->frm_chroma_fmt_max;

	dsp_cmd.dec_types = udec_config->dec_types;
	dsp_cmd.max_frm_num = udec_config->max_frm_num;

	dsp_cmd.max_frm_width = W_ALIGNED(udec_config->max_frm_width);
	dsp_cmd.max_frm_height = H_ALIGNED(udec_config->max_frm_height);

	dsp_cmd.max_fifo_size = udec_config->max_fifo_size;

	dsp_cmd.no_fmo = G_decode_obj->input_feature_constrains.h264_no_fmo;
	UDEC_LOG_ALWAYS("CMD_UDEC_INIT no fmo %d.\n", dsp_cmd.no_fmo);

    	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_decpp_config(iav_udec_info_ex_t *info)
{
	iav_udec_vout_config_t *vout_config = NULL;
	int i;

	DECPP_CONFIG_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	for (i = 0; i < IAV_NUM_VOUT; i++) {
		if (__vout_owner(i) == info->udec_id) {
			vout_config = __vout_config(i);
			break;
		}
	}

	dsp_cmd.cmd_code = CMD_DECPP_CONFIG;

	dsp_cmd.dec_id = info->udec_id;
	dsp_cmd.dec_type = dsp_udec_type(info->udec_type);

	dsp_cmd.interlaced_out = info->interlaced_out;
	dsp_cmd.packed_out = info->packed_out;
	dsp_cmd.out_chroma_format = info->out_chroma_format;
	// dsp_cmd.use_deint
	// dsp_cmd.half_deint_frame_rate

	dsp_cmd.input_center_x = info->vout_configs.input_center_x;
	dsp_cmd.input_center_y = info->vout_configs.input_center_y;

	dsp_cmd.out_win_offset_x = vout_config->target_win_offset_x;
	dsp_cmd.out_win_offset_y = vout_config->target_win_offset_y;
	dsp_cmd.out_win_width = vout_config->target_win_width;
	dsp_cmd.out_win_height = vout_config->target_win_height;

	dsp_cmd.out_zoom_factor_x = vout_config->zoom_factor_x;
	dsp_cmd.out_zoom_factor_y = vout_config->zoom_factor_y;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_udec_postproc(int udec_id, int udec_type)
{
	iav_udec_info_ex_t *udec_info = __udec_info(udec_id);
	iav_udec_display_t *vout_display = &udec_info->vout_configs;
	iav_udec_vout_config_t *vout0_config = NULL;
	iav_udec_vout_config_t *vout1_config = NULL;
	int i;

	POSTPROC_UDEC_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	for (i = 0; i < IAV_NUM_VOUT; i++) {
		iav_udec_vout_config_t *vout_config = __vout_config(i);
		if (__vout_owner(i) == udec_id) {
			if (vout_config->vout_id == 0) {
				vout0_config = vout_config;
				G_decode_obj->vout_config_valid[0] = 1;
			} else if (vout_config->vout_id == 1) {
				vout1_config = vout_config;
				G_decode_obj->vout_config_valid[1] = 1;
			}
		}
	}

	dsp_cmd.cmd_code = CMD_POSTPROC_UDEC;

	dsp_cmd.decode_cat_id = CAT_UDEC;
	dsp_cmd.decode_id = udec_id;
	dsp_cmd.decode_type = udec_type;

	dsp_cmd.fir_pts_low = vout_display->first_pts_low;
	dsp_cmd.fir_pts_high = vout_display->first_pts_high;

	dsp_cmd.input_center_x = vout_display->input_center_x;
	dsp_cmd.input_center_y = vout_display->input_center_y;

	if (vout0_config) {
		dsp_cmd.voutA_enable = !vout0_config->disable;
		dsp_cmd.vout0_flip = vout0_config->flip;
		dsp_cmd.vout0_rotate = vout0_config->rotate;
		if (vout0_config->rotate) {
			dsp_cmd.voutA_target_win_offset_x = vout0_config->target_win_offset_y;
			dsp_cmd.voutA_target_win_offset_y = vout0_config->target_win_offset_x;
			dsp_cmd.voutA_target_win_width = vout0_config->target_win_height;
			dsp_cmd.voutA_target_win_height = vout0_config->target_win_width;
		} else {
			dsp_cmd.voutA_target_win_offset_x = vout0_config->target_win_offset_x;
			dsp_cmd.voutA_target_win_offset_y = vout0_config->target_win_offset_y;
			dsp_cmd.voutA_target_win_width = vout0_config->target_win_width;
			dsp_cmd.voutA_target_win_height = vout0_config->target_win_height;
		}
		dsp_cmd.vout0_win_offset_x = vout0_config->win_offset_x;
		dsp_cmd.vout0_win_offset_y = vout0_config->win_offset_y;
		dsp_cmd.vout0_win_width = vout0_config->win_width;
		dsp_cmd.vout0_win_height = vout0_config->win_height;
		dsp_cmd.voutA_zoom_factor_x = vout0_config->zoom_factor_x;
		dsp_cmd.voutA_zoom_factor_y = vout0_config->zoom_factor_y;
	}

	if (vout1_config) {
		dsp_cmd.voutB_enable = !vout1_config->disable;
		dsp_cmd.vout1_flip = vout1_config->flip;
		dsp_cmd.vout1_rotate = vout1_config->rotate;
		if (vout1_config->rotate) {
			dsp_cmd.voutB_target_win_offset_x = vout1_config->target_win_offset_y;
			dsp_cmd.voutB_target_win_offset_y = vout1_config->target_win_offset_x;
			dsp_cmd.voutB_target_win_width = vout1_config->target_win_height;
			dsp_cmd.voutB_target_win_height = vout1_config->target_win_width;
		} else {
			dsp_cmd.voutB_target_win_offset_x = vout1_config->target_win_offset_x;
			dsp_cmd.voutB_target_win_offset_y = vout1_config->target_win_offset_y;
			dsp_cmd.voutB_target_win_width = vout1_config->target_win_width;
			dsp_cmd.voutB_target_win_height = vout1_config->target_win_height;
		}
		dsp_cmd.vout1_win_offset_x = vout1_config->win_offset_x;
		dsp_cmd.vout1_win_offset_y = vout1_config->win_offset_y;
		dsp_cmd.vout1_win_width = vout1_config->win_width;
		dsp_cmd.vout1_win_height = vout1_config->win_height;
		dsp_cmd.voutB_zoom_factor_x = vout1_config->zoom_factor_x;
		dsp_cmd.voutB_zoom_factor_y = vout1_config->zoom_factor_y;
	}

	if (G_decode_obj->dewarp_feature_enabled) {
		if (G_decode_obj->horz_warp_enable) {
			dsp_cmd.horz_warp_enable = 1;
			dsp_cmd.warp_horizontal_table_address = (u32)VIRT_TO_DSP(G_decode_obj->dewarp_table);
			dsp_cmd.grid_array_width = G_decode_obj->grid_array_width;
			dsp_cmd.grid_array_height = G_decode_obj->grid_array_height;
			dsp_cmd.horz_grid_spacing_exponent = G_decode_obj->horz_grid_spacing_exponent;
			dsp_cmd.vert_grid_spacing_exponent = G_decode_obj->vert_grid_spacing_exponent;
		}
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

//s2 do not support this cmd?
#if 0
	/* Inform vout the original aspect ratio of decoding video */
	if (vout_display->input_center_x * 3 == vout_display->input_center_y * 4) {
		iav_update_vout_video_src_ar(AMBA_VIDEO_RATIO_4_3);
	}
	if (vout_display->input_center_x * 9 == vout_display->input_center_y * 16) {
		iav_update_vout_video_src_ar(AMBA_VIDEO_RATIO_16_9);
	}
#endif
}

static int _get_video_dimention(int vout_id, u32* width, u32* height)
{
	iav_udec_vout_config_t *vout_config = NULL;
	int i;

	for (i = 0; i < IAV_NUM_VOUT; i++) {
		vout_config = __vout_config(i);
		if (vout_config->vout_id == vout_id) {
			if (vout_config->rotate) {
				*width = vout_config->target_win_height;
				*height = vout_config->target_win_width;
				return 0;
			} else {
				*width = vout_config->target_win_width;
				*height = vout_config->target_win_height;
				return 0;
			}
		}
	}
	return -1;
}

static void _debug_print_warp_table_vectors_hex(s16*p_table, u32 width, u32 height)
{
	u32 i, j;
	s16*p = p_table;

	if (!p_table) {
		UDEC_LOG_ERROR("NULL dewarp table pointer\n");
		return;
	} else {
		UDEC_LOG_ALWAYS("print dewarp table pointer %p, w %d, h %d\n", p_table, width, height);
	}

	//table value
	for (j = 0; j < (height + 1); j++) {
		UDEC_LOG_ALWAYS("%d line:\t\t", j);
		for (i = 0; i < (width + 1); i++) {
			UDEC_LOG_ALWAYS("%4.4hx ", *p++);
		}
		UDEC_LOG_ALWAYS("\n");
		//p_table += 1;
	}

	UDEC_LOG_ALWAYS("\ntable without diff:\n");
	//with diff
	p = p_table;
	for (j = 0; j < (height + 1); j++) {
		UDEC_LOG_ALWAYS("%d line:\t\t", j);
		for (i = 0; i < (width + 1); i++) {
			UDEC_LOG_ALWAYS("%4.4hx ", (*p++) + (i * 32 * 16));
		}
		UDEC_LOG_ALWAYS("\n");
		//p_table += 1;
	}
}

static int _interpret_dewarp_params(iav_udec_vout_dewarp_config_t* dewarp_config)
{
#define base_grid_spacing_exponent 4
#define fixed_point_fractional 4
#define default_exponent 1

	u32 grid_width, grid_height;
	u32 grid_size;
	u32 top_left_x = 0, top_left_y = 0;
	u32 bottom_left_x = 0, bottom_left_y = 0;
	u32 top_right_x = 0, top_right_y = 0;
	u32 bottom_right_x = 0, bottom_right_y = 0;
	u32 i, j;
	s16* p_table;
	s16 ori_value, new_value, ori_step;

	u32 left_x = 0, right_x = 0;
	u32 left_y = 0, right_y = 0;

	u32 vout_width, vout_height;

	if (_get_video_dimention(0, &vout_width, &vout_height) < 0) {
		UDEC_LOG_ERROR("get video 0 dimention fail\n");
		return -1;
	}

	UDEC_LOG_ALWAYS("_interpret_dewarp_params start, vout_width %d, vout_height %d.\n", vout_width, vout_height);

	if (!dewarp_config->horz_warp_enable) {
		G_decode_obj->horz_warp_enable = 0;

		G_decode_obj->grid_array_width = 0;
		G_decode_obj->grid_array_height= 0;
		G_decode_obj->horz_grid_spacing_exponent = 0;
		G_decode_obj->vert_grid_spacing_exponent = 0;
		UDEC_LOG_ALWAYS("disable dewarp settings.\n");
		return 1;
	} else if (!dewarp_config->use_customized_dewarp_params) {
		//use default params
		grid_size = 1 << (base_grid_spacing_exponent + default_exponent);
		grid_width = (vout_width + grid_size - 1)/grid_size;
		grid_height = (vout_height + grid_size - 1)/grid_size;

		//store params
		G_decode_obj->horz_warp_enable = 1;
		G_decode_obj->horz_grid_spacing_exponent = default_exponent;
		G_decode_obj->vert_grid_spacing_exponent = default_exponent;
		G_decode_obj->grid_array_width = grid_width;
		G_decode_obj->grid_array_height = grid_height;

		ori_step = grid_size;

		UDEC_LOG_ALWAYS("use default dewarp params grid_size %d, grid_width %d, grid_height %d.\n", grid_size, grid_width, grid_height);
	} else {
		//debug check
		UDEC_DEBUG_ASSERT(dewarp_config->horz_grid_spacing_exponent < 6);
		UDEC_DEBUG_ASSERT(dewarp_config->vert_grid_spacing_exponent < 6);

		grid_size = 1 << (base_grid_spacing_exponent + dewarp_config->horz_grid_spacing_exponent);
		grid_width = (vout_width + grid_size -1)/grid_size;

		ori_step = grid_size;

		grid_size = 1 << (base_grid_spacing_exponent + dewarp_config->vert_grid_spacing_exponent);
		grid_height = (vout_height + grid_size -1)/grid_size;

		//store params
		G_decode_obj->horz_warp_enable = 1;
		G_decode_obj->horz_grid_spacing_exponent = dewarp_config->horz_grid_spacing_exponent;
		G_decode_obj->vert_grid_spacing_exponent = dewarp_config->vert_grid_spacing_exponent;
		G_decode_obj->grid_array_width = grid_width;
		G_decode_obj->grid_array_height = grid_height;

		UDEC_LOG_ALWAYS("use costomized dewarp params horz_exp %d, vert_exp %d, grid_width %d, grid_height %d.\n", dewarp_config->horz_grid_spacing_exponent, dewarp_config->vert_grid_spacing_exponent, grid_width, grid_height);
	}

	i = (grid_width + 1) * (grid_height + 1) * sizeof(s16);//only x vector
	UDEC_LOG_ALWAYS("i %d, G_decode_obj->dewarp_table %p, dewarp_table_len %d.\n", i, G_decode_obj->dewarp_table, G_decode_obj->dewarp_table_len);
	if (!G_decode_obj->dewarp_table) {
		UDEC_LOG_ALWAYS("before kmalloc 1.\n");
		G_decode_obj->dewarp_table = kmalloc(i, GFP_KERNEL);
		if (G_decode_obj->dewarp_table) {
			G_decode_obj->dewarp_table_len = i;
		} else {
			UDEC_LOG_ERROR("NO memory %d.\n", i);
		}
	} else if (G_decode_obj->dewarp_table_len < i) {
		UDEC_LOG_ALWAYS("before kfee 2.\n");
		kfree(G_decode_obj->dewarp_table);
		G_decode_obj->dewarp_table_len = 0;
		UDEC_LOG_ALWAYS("before kmalloc 2.\n");
		G_decode_obj->dewarp_table = kmalloc(i, GFP_KERNEL);
		if (G_decode_obj->dewarp_table) {
			G_decode_obj->dewarp_table_len = i;
		} else {
			UDEC_LOG_ERROR("NO memory %d.\n", i);
		}
	}

	UDEC_LOG_ALWAYS("after i %d, G_decode_obj->dewarp_table %p, dewarp_table_len %d.\n", i, G_decode_obj->dewarp_table, G_decode_obj->dewarp_table_len);
	UDEC_DEBUG_ASSERT(G_decode_obj->dewarp_table);
	UDEC_DEBUG_ASSERT(G_decode_obj->dewarp_table_len >= i);

	UDEC_LOG_ALWAYS("dewarp_config->warp_table_type %d\n", dewarp_config->warp_table_type);
	if (IAV_DEWARP_PARAMS_CUSTOMIZED_TABLE == dewarp_config->warp_table_type) {
		UDEC_LOG_ERROR("must not comes here.\n");
		if (dewarp_config->warp_horizontal_table_address) {
			if (copy_from_user(G_decode_obj->dewarp_table, dewarp_config->warp_horizontal_table_address, i)) {
				UDEC_LOG_ERROR("copy_from_user error?\n");
				return -EFAULT;
			}
		} else {
			UDEC_LOG_ERROR("NULL warp_horizontal_table_address.\n");
			return -EFAULT;
		}
		return 0;
	} else if (IAV_DEWARP_PARAMS_TOP_BOTTOM_WIDTH == dewarp_config->warp_table_type) {
		//UDEC_DEBUG_ASSERT(vout_display->warp_param0 <= vout_width);
		//UDEC_DEBUG_ASSERT(vout_display->warp_param1 <= vout_width);
		if ((dewarp_config->warp_param0 > vout_width) || (dewarp_config->warp_param1 > vout_width)) {
			UDEC_LOG_ERROR("BAD params %d, %d, vout width %d.\n", dewarp_config->warp_param0, dewarp_config->warp_param1, vout_width);
			return (-2);
		}
		UDEC_LOG_ALWAYS("dewarp_config->warp_param0 %d, dewarp_config->warp_param1 %d\n", dewarp_config->warp_param0, dewarp_config->warp_param1);

		top_left_x = (vout_width - dewarp_config->warp_param0)/2;
		top_left_y = 0;

		top_right_x = (vout_width + dewarp_config->warp_param0)/2 - 1;
		top_right_y = 0;

		bottom_left_x = (vout_width - dewarp_config->warp_param1)/2;
		bottom_left_y = vout_height - 1;

		bottom_right_x = (vout_width + dewarp_config->warp_param1)/2 - 1;
		bottom_right_y = vout_height - 1;

	} else if (IAV_DEWARP_PARAMS_FOUR_POINT == dewarp_config->warp_table_type) {
		if ((dewarp_config->warp_param0 > vout_width) ||\
			(dewarp_config->warp_param2 > vout_width) ||\
			(dewarp_config->warp_param4 > vout_width) ||\
			(dewarp_config->warp_param6 > vout_width) ||\
			(dewarp_config->warp_param1 > vout_height) ||\
			(dewarp_config->warp_param3 > vout_height) ||\
			(dewarp_config->warp_param5 > vout_height) ||\
			(dewarp_config->warp_param7 > vout_height)) {
			UDEC_LOG_ERROR("BAD params, exceed vout width %d, vout height %d.\n", vout_width, vout_height);
			return (-2);
		}

		UDEC_DEBUG_ASSERT(top_left_x < top_right_x);
		UDEC_DEBUG_ASSERT(top_left_y < bottom_left_y);
		UDEC_DEBUG_ASSERT(top_right_y < bottom_right_y);
		UDEC_DEBUG_ASSERT(bottom_left_x < bottom_right_x);

		if ((top_left_x < top_right_x) && (top_left_y < bottom_left_y) && (top_right_y < bottom_right_y) && (bottom_left_x < bottom_right_x)) {
			top_left_x = dewarp_config->warp_param0;
			top_left_y = dewarp_config->warp_param1;

			top_right_x = dewarp_config->warp_param2;
			top_right_y = dewarp_config->warp_param3;

			bottom_left_x = dewarp_config->warp_param4;
			bottom_left_y = dewarp_config->warp_param5;

			bottom_right_x = dewarp_config->warp_param6;
			bottom_right_y = dewarp_config->warp_param7;
		} else {
			UDEC_LOG_ERROR("BAD params, wrong position for four points.\n");
			return (-2);
		}
	} else {
		UDEC_LOG_ERROR("BAD warp table type %d.\n", dewarp_config->warp_table_type);
		return (-1);
	}

	UDEC_LOG_ALWAYS("four points result:\n");
	UDEC_LOG_ALWAYS("top_left_x %d, top_left_y %d.\n", top_left_x, top_left_y);
	UDEC_LOG_ALWAYS("top_right_x %d, top_right_y %d.\n", top_right_x, top_right_y);
	UDEC_LOG_ALWAYS("bottom_left_x %d, bottom_left_y %d.\n", bottom_left_x, bottom_left_y);
	UDEC_LOG_ALWAYS("bottom_right_x %d, bottom_right_y %d.\n", bottom_right_x, bottom_right_y);

	p_table = G_decode_obj->dewarp_table;
	ori_step = ori_step << fixed_point_fractional;

	//grid_height ++;
	//grid_width ++;
	//construct table
	for (j = 0; j <= grid_height; j++) {
		left_x = ((top_left_x * (grid_height - j) + bottom_left_x * (j)) << fixed_point_fractional) / grid_height;
		//UDEC_LOG_ALWAYS("median left x result %d.\n", ((top_left_x * (grid_height - j) + bottom_left_x * (j)) << fixed_point_fractional));
		//left_y = ((top_left_y * (grid_height - j) + bottom_left_y * (j)) << fixed_point_fractional) / grid_height;
		//UDEC_LOG_ALWAYS("median left y result %d.\n", ((top_left_y * (grid_height - j) + bottom_left_y * (j)) << fixed_point_fractional));
		right_x = ((top_right_x * (grid_height - j) + bottom_right_x * (j)) << fixed_point_fractional) / grid_height;
		//UDEC_LOG_ALWAYS("median right x result %d.\n", ((top_right_x * (grid_height - j) + bottom_right_x * (j)) << fixed_point_fractional));
		//right_y = ((top_right_y * (grid_height - j) + bottom_right_y * (j)) << fixed_point_fractional) / grid_height;
		//UDEC_LOG_ALWAYS("median right y result %d.\n", ((top_right_y * (grid_height - j) + bottom_right_y * (j)) << fixed_point_fractional));

		UDEC_LOG_ALWAYS("left x %d, y %d:\n", left_x, left_y);
		UDEC_LOG_ALWAYS("right x %d, y %d:\n", right_x, right_y);
		if (left_x == right_x) {
			UDEC_LOG_ERROR("left_x == right_x, should not comes here.\n");
			p_table += grid_width + 1;
			continue;
		}

		ori_value = 0;
		for (i = 0; i <= grid_width; i++) {
			//*p_table++ = (left_x * (grid_width - i) + right_x * (i)) / grid_width;
			//UDEC_LOG_ALWAYS("median x result %d.\n", (left_x * (grid_width - i) + right_x * (i)));
			//*p_table++ = (left_y * (grid_width - i) + right_y * (i)) / grid_width;
			//UDEC_LOG_ALWAYS("median y result %d.\n", (left_y * (grid_width - i) + right_y * (i)));

//			if (ori_value  < (s16)left_x) {
//				new_value = 0 - ori_step;
				//UDEC_LOG_ALWAYS("j %d, i %d, out of range 1, new_value %d, ori_value %d, diff 0x%x.\n", j, i, new_value, ori_value, new_value - ori_value);
//			} else if (ori_value > (s16)right_x) {
//				new_value = (grid_width + 1) * ori_step;
				//UDEC_LOG_ALWAYS("j %d, i %d, out of range 2, new_value %d, ori_value %d, diff 0x%x.\n", j, i, new_value, ori_value, new_value - ori_value);
//			} else {
				new_value = ((s32)(ori_value - left_x)) * (s32)grid_width * (s32)ori_step /(s32)(right_x - left_x);
//			}
			*p_table++ = new_value - ori_value;
			ori_value += ori_step;
		}
	}

	//clean cache here
	clean_cache_aligned((u8*)G_decode_obj->dewarp_table, (unsigned long)G_decode_obj->dewarp_table_len);

	return 0;
}

// update vout config for the udec (iav_udec_display_t.udec_id)
int iav_update_vout_config(iav_context_t *context, iav_udec_display_t __user *arg)
{
	iav_udec_display_t vout_display;
	iav_udec_vout_config_t *vout_config;
	iav_udec_info_ex_t *udec_info;
	int udec_id;
	int i;
	int ret;
#ifdef DEBUG_DUMP_DEWARP_TABLE
	struct file *f;
#endif

	if (check_udec_mode())
		return -EPERM;

	if (copy_from_user(&vout_display, arg, sizeof(vout_display)))
		return -EFAULT;

	udec_id = vout_display.udec_id;
	if (udec_id >= G_decode_obj->udec_max_number) {
		iav_dbg_printk("bad udec_id %d\n", vout_display.udec_id);
		return -EINVAL;
	}

	// is our udec?
	if ((context->udec_flags & (1 << udec_id)) == 0) {
		iav_dbg_printk("not my udec: %d\n", udec_id);
		return -EINVAL;
	}

	if (vout_display.num_vout > IAV_NUM_VOUT) {
		iav_dbg_printk("too many vout: %d\n", vout_display.num_vout);
		return -EINVAL;
	}

	// save vout configs
	for (i = 0; i < vout_display.num_vout; i++) {
		u32 vout_id;

		if (get_user(vout_id, (u8 __user*)&vout_display.vout_config[i].vout_id))
			return -EINVAL;

		if (vout_id >= IAV_NUM_VOUT) {
			iav_dbg_printk("bad vout id: %d\n", vout_id);
			return -EINVAL;
		}

		// vout owner
		if (__vout_owner(vout_id) != udec_id) {
			iav_dbg_printk("not my vout: %d\n", vout_id);
			return -EINVAL;
		}

		vout_config = __vout_config(vout_id);

		if (copy_from_user(vout_config, vout_display.vout_config + i, sizeof(*vout_config)))
			return -EFAULT;
	}

	//if need update dewarp
	if (vout_display.dewarp_params_updated) {
		UDEC_LOG_ALWAYS("update to new dewarp settings 1.\n");
		ret = _interpret_dewarp_params(&vout_display.dewarp);
		if (ret < 0) {
			UDEC_LOG_ERROR("BAD params.\n");
			return -EINVAL;
		}

#ifdef DEBUG_DUMP_DEWARP_TABLE
		f = filp_open("/tmp/mmcblk1p1/dewarp_bin", O_WRONLY | O_CREAT | O_TRUNC, 0);
		if (f) {
			mm_segment_t oldfs = get_fs();
			set_fs(KERNEL_DS);
			ssize_t ret= f->f_op->write(f, G_decode_obj->dewarp_table, (G_decode_obj->grid_array_width + 1)*(G_decode_obj->grid_array_height + 1) * sizeof(s16), &f->f_pos);
			set_fs(oldfs);
			filp_close(f, NULL);
		}
#endif
		if (!ret) {
			_debug_print_warp_table_vectors_hex(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
			//_debug_print_warp_table_vectors_float(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
		}
	}

	// save display config
	udec_info = __udec_info(udec_id);
	udec_info->vout_configs = vout_display;

	cmd_udec_postproc(udec_id, __udec(udec_id)->udec_type);

	return 0;
}

// update vout config for the udec (iav_udec_display_t.udec_id)
int iav_update_vout_dewarp_config(iav_context_t *context, iav_udec_vout_dewarp_config_t __user *arg)
{
	iav_udec_vout_dewarp_config_t vout_dewarp;
	int udec_id;
	int ret;
#ifdef DEBUG_DUMP_DEWARP_TABLE
	struct file *f;
#endif

	if (check_udec_mode())
		return -EPERM;

	if (copy_from_user(&vout_dewarp, arg, sizeof(vout_dewarp)))
		return -EFAULT;

	if (!G_decode_obj->dewarp_feature_enabled) {
		UDEC_LOG_ERROR("dewarp feature is not enabled.\n");
		return -EPERM;
	}

	udec_id = vout_dewarp.udec_id;
	if (udec_id >= G_decode_obj->udec_max_number) {
		UDEC_LOG_ERROR("bad udec_id %d\n", vout_dewarp.udec_id);
		return -EINVAL;
	}

	// is our udec?
	if ((context->udec_flags & (1 << udec_id)) == 0) {
		UDEC_LOG_ERROR("not my udec: %d\n", udec_id);
		return -EINVAL;
	}

	//save

	//update dewarp settings
	UDEC_LOG_ALWAYS("update to new dewarp settings 0.\n");
	ret = _interpret_dewarp_params(&vout_dewarp);

	if (ret < 0) {
		UDEC_LOG_ERROR("BAD params.\n");
		return -EINVAL;
	}

#ifdef DEBUG_DUMP_DEWARP_TABLE
	f = filp_open("/tmp/mmcblk1p1/dewarp_bin", O_WRONLY | O_CREAT | O_TRUNC, 0);
	if (f) {
		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		ssize_t ret= f->f_op->write(f, G_decode_obj->dewarp_table, (G_decode_obj->grid_array_width + 1)*(G_decode_obj->grid_array_height + 1) * sizeof(s16), &f->f_pos);
		set_fs(oldfs);
		filp_close(f, NULL);
	}
#endif

	if (!ret) {
		_debug_print_warp_table_vectors_hex(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
		//_debug_print_warp_table_vectors_float(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
	}

	cmd_udec_postproc(udec_id, __udec(udec_id)->udec_type);

	return 0;
}

#ifdef ENABLE_MULTI_WINDOW_UDEC
static int __setup_capture_context(iav_context_t *context)
{
	UDEC_DEBUG_ASSERT(!G_decode_obj->capture_context_setup);
	if (G_decode_obj->capture_context_setup) {
		return 0;
	}

	UDEC_DEBUG_ASSERT(G_decode_obj->capture_mem_allocated);
	if (!G_decode_obj->capture_mem_allocated) {
		if (__alloc_dec_capture_mem() < 0) {
			return -1;
		}
	}
	G_decode_obj->capture_context_setup = 1;

	//mmap
	G_decode_obj->dec_capture.capture[CAPTURE_CODED].buffer_base = (u32)map_udec_playback_capture(context, "dec_coded_cap_bsb", (u32)(virt_to_phys(G_decode_obj->coded_capture_bsb_virtual)), MAX_DEC_CAPTURED_ENC_SIZE, &G_decode_obj->coded_capture_bsb_mmap);
	G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].buffer_base = (u32)map_udec_playback_capture(context, "dec_thumb_cap_bsb", (u32)(virt_to_phys(G_decode_obj->thumbnail_capture_bsb_virtual)), MAX_DEC_THUMBNNAIL_CAPTURED_ENC_SIZE, &G_decode_obj->thumbnail_capture_bsb_mmap);
	G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].buffer_base = (u32)map_udec_playback_capture(context, "dec_screen_cap_bsb", (u32)(virt_to_phys(G_decode_obj->screennail_capture_bsb_virtual)), MAX_DEC_CAPTURED_ENC_SIZE, &G_decode_obj->screennail_capture_bsb_mmap);

	G_decode_obj->coded_capture_bsb_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->coded_capture_bsb_virtual);
	G_decode_obj->thumbnail_capture_bsb_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->thumbnail_capture_bsb_virtual);
	G_decode_obj->screennail_capture_bsb_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->screennail_capture_bsb_virtual);
	G_decode_obj->coded_capture_qt_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->coded_capture_qt_virtual);
	G_decode_obj->thumbnail_capture_qt_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->thumbnail_capture_qt_virtual);
	G_decode_obj->screennail_capture_qt_dsp = (u8*)VIRT_TO_DSP(G_decode_obj->screennail_capture_qt_virtual);

	//init qt table, default quality is 50
	G_decode_obj->dec_capture.capture[CAPTURE_CODED].quality = 50;
	__init_jpeg_dqt(G_decode_obj->coded_capture_qt_virtual, 50);
	clean_cache_aligned(G_decode_obj->coded_capture_qt_virtual, JPEG_QT_SIZE);

	G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].quality = 50;
	__init_jpeg_dqt(G_decode_obj->thumbnail_capture_qt_virtual, 50);
	clean_cache_aligned(G_decode_obj->thumbnail_capture_qt_virtual, JPEG_QT_SIZE);

	G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].quality = 50;
	__init_jpeg_dqt(G_decode_obj->screennail_capture_qt_virtual, 50);
	clean_cache_aligned(G_decode_obj->screennail_capture_qt_virtual, JPEG_QT_SIZE);

	UDEC_LOG_ALWAYS("[important flow]: __setup_capture_context(%p)!\n", context);

	return 0;
}
#endif

static void __destroy_capture_context(iav_context_t *context)
{
	UDEC_DEBUG_ASSERT(G_decode_obj->capture_context_setup);
	destroy_mmap(context, &G_decode_obj->coded_capture_bsb_mmap);
	destroy_mmap(context, &G_decode_obj->thumbnail_capture_bsb_mmap);
	destroy_mmap(context, &G_decode_obj->screennail_capture_bsb_mmap);
	UDEC_LOG_ALWAYS("[important flow]: __destroy_capture_context(%p)!\n", context);
	G_decode_obj->capture_context_setup = 0;
}

#ifdef ENABLE_MULTI_WINDOW_UDEC
static void cmd_udec_zoom(iav_udec_zoom_t *zoom)
{
	POSTPROC_MW_PLAYBACK_ZOOM_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTPROC_MW_PLAYBACK_ZOOM;
	dsp_cmd.render_id = zoom->render_id;
	dsp_cmd.input_center_x = zoom->input_center_x;
	dsp_cmd.input_center_y = zoom->input_center_y;
	dsp_cmd.zoom_factor_x = zoom->zoom_factor_x;
	dsp_cmd.zoom_factor_y = zoom->zoom_factor_y;
	dsp_cmd.input_width = zoom->input_width;
	dsp_cmd.input_height = zoom->input_height;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

#ifdef CONFIG_IAV_USE_OLD_UDEC_CAPTURE
static void cmd_udec_capture(iav_udec_capture_t *capture)
{
	UDEC_STILL_CAP_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_STILL_CAP;
	dsp_cmd.decoder_id = capture->dec_id;

	UDEC_DEBUG_ASSERT(capture->capture_coded);
	if (capture->capture_coded) {
		dsp_cmd.coded_pic_base = (u32)G_decode_obj->coded_capture_bsb_dsp;
		dsp_cmd.coded_pic_limit = (u32)G_decode_obj->coded_capture_bsb_dsp + MAX_DEC_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.coded_pic_base = 0xffffffff;
		dsp_cmd.coded_pic_limit = 0xffffffff;
	}

	if (capture->capture_thumbnail) {
		dsp_cmd.thumbnail_pic_base = (u32)G_decode_obj->thumbnail_capture_bsb_dsp;
		dsp_cmd.thumbnail_pic_limit = (u32)G_decode_obj->thumbnail_capture_bsb_dsp + MAX_DEC_THUMBNNAIL_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.thumbnail_pic_base = 0xffffffff;
		dsp_cmd.thumbnail_pic_limit = 0xffffffff;
	}

	dsp_cmd.thumbnail_width = capture->capture[CAPTURE_THUMBNAIL].target_pic_width;
	dsp_cmd.thumbnail_height = capture->capture[CAPTURE_THUMBNAIL].target_pic_height;

	dsp_cmd.thumbnail_letterbox_strip_width = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_width;
	dsp_cmd.thumbnail_letterbox_strip_height = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_height;
	dsp_cmd.thumbnail_letterbox_strip_y = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_y;
	dsp_cmd.thumbnail_letterbox_strip_cb = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_cb;
	dsp_cmd.thumbnail_letterbox_strip_cr = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_cr;

	dsp_cmd.quant_matrix_addr = (u32)G_decode_obj->coded_capture_qt_dsp;

	dsp_cmd.target_pic_width = capture->capture[CAPTURE_CODED].target_pic_width;
	dsp_cmd.target_pic_height = capture->capture[CAPTURE_CODED].target_pic_height;

	dsp_cmd.pic_structure = 0;//correct here?

	if (capture->capture_screennail) {
		dsp_cmd.screennail_pic_base = (u32)G_decode_obj->screennail_capture_bsb_dsp;
		dsp_cmd.screennail_pic_limit = (u32)G_decode_obj->screennail_capture_bsb_dsp + MAX_DEC_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.screennail_pic_base = 0xffffffff;
		dsp_cmd.screennail_pic_limit = 0xffffffff;
	}

	dsp_cmd.screennail_width = capture->capture[CAPTURE_SCREENNAIL].target_pic_width;
	dsp_cmd.screennail_height = capture->capture[CAPTURE_SCREENNAIL].target_pic_height;

	dsp_cmd.screennail_letterbox_strip_width = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_width;
	dsp_cmd.screennail_letterbox_strip_height = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_height;
	dsp_cmd.screennail_letterbox_strip_y = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_y;
	dsp_cmd.screennail_letterbox_strip_cb = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_cb;
	dsp_cmd.screennail_letterbox_strip_cr = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_cr;

	dsp_cmd.quant_matrix_addr_thumbnail = (u32)G_decode_obj->thumbnail_capture_qt_dsp;
	dsp_cmd.quant_matrix_addr_screennail = (u32)G_decode_obj->screennail_capture_qt_dsp;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}
#endif

static void cmd_postp_capture(iav_udec_capture_t *capture)
{
	POSTPROC_MW_SCREEN_SHOT_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTPROC_MW_SCREEN_SHOT;
	dsp_cmd.udec_id = capture->dec_id;

	UDEC_DEBUG_ASSERT(capture->capture_coded);
	if (capture->capture_coded) {
		dsp_cmd.coded_pic_base = (u32)G_decode_obj->coded_capture_bsb_dsp;
		dsp_cmd.coded_pic_limit = (u32)G_decode_obj->coded_capture_bsb_dsp + MAX_DEC_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.coded_pic_base = 0xffffffff;
		dsp_cmd.coded_pic_limit = 0xffffffff;
	}

	if (capture->capture_thumbnail) {
		dsp_cmd.thumbnail_pic_base = (u32)G_decode_obj->thumbnail_capture_bsb_dsp;
		dsp_cmd.thumbnail_pic_limit = (u32)G_decode_obj->thumbnail_capture_bsb_dsp + MAX_DEC_THUMBNNAIL_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.thumbnail_pic_base = 0xffffffff;
		dsp_cmd.thumbnail_pic_limit = 0xffffffff;
	}

	dsp_cmd.thumbnail_width = capture->capture[CAPTURE_THUMBNAIL].target_pic_width;
	dsp_cmd.thumbnail_height = capture->capture[CAPTURE_THUMBNAIL].target_pic_height;

	dsp_cmd.thumbnail_letterbox_strip_width = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_width;
	dsp_cmd.thumbnail_letterbox_strip_height = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_height;
	dsp_cmd.thumbnail_letterbox_strip_y = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_y;
	dsp_cmd.thumbnail_letterbox_strip_cb = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_cb;
	dsp_cmd.thumbnail_letterbox_strip_cr = capture->capture[CAPTURE_THUMBNAIL].letterbox_strip_cr;

	dsp_cmd.quant_matrix_addr = (u32)G_decode_obj->coded_capture_qt_dsp;

	dsp_cmd.target_pic_width = capture->capture[CAPTURE_CODED].target_pic_width;
	dsp_cmd.target_pic_height = capture->capture[CAPTURE_CODED].target_pic_height;

	if (capture->capture_screennail) {
		dsp_cmd.screennail_pic_base = (u32)G_decode_obj->screennail_capture_bsb_dsp;
		dsp_cmd.screennail_pic_limit = (u32)G_decode_obj->screennail_capture_bsb_dsp + MAX_DEC_CAPTURED_ENC_SIZE;
	} else {
		dsp_cmd.screennail_pic_base = 0xffffffff;
		dsp_cmd.screennail_pic_limit = 0xffffffff;
	}

	dsp_cmd.screennail_width = capture->capture[CAPTURE_SCREENNAIL].target_pic_width;
	dsp_cmd.screennail_height = capture->capture[CAPTURE_SCREENNAIL].target_pic_height;

	dsp_cmd.screennail_letterbox_strip_width = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_width;
	dsp_cmd.screennail_letterbox_strip_height = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_height;
	dsp_cmd.screennail_letterbox_strip_y = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_y;
	dsp_cmd.screennail_letterbox_strip_cb = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_cb;
	dsp_cmd.screennail_letterbox_strip_cr = capture->capture[CAPTURE_SCREENNAIL].letterbox_strip_cr;

	dsp_cmd.quant_matrix_addr_thumbnail = (u32)G_decode_obj->thumbnail_capture_qt_dsp;
	dsp_cmd.quant_matrix_addr_screennail = (u32)G_decode_obj->screennail_capture_qt_dsp;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}
#endif

// update vout config for the udec (iav_udec_display_t.udec_id)
int iav_udec_capture(iav_context_t *context, iav_udec_capture_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_udec_capture_t capture;
	u32 flags;
	u32 size = 0;

	if (check_udec_mode()) {
		UDEC_LOG_ERROR("not in UDEC mode\n");
		return -EPERM;
	}

	if (copy_from_user(&capture, arg, sizeof(capture))) {
		UDEC_LOG_ERROR("copy_from_user fail\n");
		return -EFAULT;
	}

	if (!G_decode_obj->capture_context_setup) {

		if (__setup_capture_context(context) < 0) {
			UDEC_LOG_ERROR("__setup_capture_context() fail\n");
			return -EPERM;
		}

		UDEC_DEBUG_ASSERT(!is_mode_flag_set(context, IAV_MODE_UDEC_CAPTURE));
		set_mode_flag(context, IAV_MODE_UDEC_CAPTURE);
	}

	//flag check
	if (!is_mode_flag_set(context, IAV_MODE_UDEC_CAPTURE)) {
		UDEC_LOG_ERROR("someone is using UDEC capture now, cannot open another iav_fd to do it\n");
		return -EPERM;
	}

	//safe check
	if (!G_decode_obj->capture_mem_allocated) {
		UDEC_LOG_ERROR("Fatal error, why the memory is not alloc yet?\n");
		return -EPERM;
	}

	//update dequant table, if needed
	if (capture.capture[CAPTURE_CODED].quality && (G_decode_obj->dec_capture.capture[CAPTURE_CODED].quality != capture.capture[CAPTURE_CODED].quality)) {
		__init_jpeg_dqt(G_decode_obj->coded_capture_qt_virtual, capture.capture[CAPTURE_CODED].quality);
		clean_cache_aligned(G_decode_obj->coded_capture_qt_virtual, JPEG_QT_SIZE);
		G_decode_obj->dec_capture.capture[CAPTURE_CODED].quality = capture.capture[CAPTURE_CODED].quality;
	}

	if (capture.capture[CAPTURE_THUMBNAIL].quality && (G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].quality != capture.capture[CAPTURE_THUMBNAIL].quality)) {
		__init_jpeg_dqt(G_decode_obj->thumbnail_capture_qt_virtual, capture.capture[CAPTURE_THUMBNAIL].quality);
		clean_cache_aligned(G_decode_obj->thumbnail_capture_qt_virtual, JPEG_QT_SIZE);
		G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].quality = capture.capture[CAPTURE_THUMBNAIL].quality;
	}

	if (capture.capture[CAPTURE_SCREENNAIL].quality && (G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].quality != capture.capture[CAPTURE_SCREENNAIL].quality)) {
		__init_jpeg_dqt(G_decode_obj->screennail_capture_qt_virtual, capture.capture[CAPTURE_SCREENNAIL].quality);
		clean_cache_aligned(G_decode_obj->screennail_capture_qt_virtual, JPEG_QT_SIZE);
		G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].quality = capture.capture[CAPTURE_SCREENNAIL].quality;
	}

	//parameters check
	if ((capture.capture[CAPTURE_CODED].target_pic_width <= capture.capture[CAPTURE_SCREENNAIL].target_pic_width) ||\
		(capture.capture[CAPTURE_CODED].target_pic_height <= capture.capture[CAPTURE_SCREENNAIL].target_pic_height) ||\
		(capture.capture[CAPTURE_CODED].target_pic_width <= capture.capture[CAPTURE_THUMBNAIL].target_pic_width) ||\
		(capture.capture[CAPTURE_CODED].target_pic_height <= capture.capture[CAPTURE_THUMBNAIL].target_pic_height)) {
		UDEC_LOG_ERROR("target size should greater than screennail/thumbnail size!!\n");
		return -EPERM;
	}

	//issue cmd
#ifdef CONFIG_IAV_USE_OLD_UDEC_CAPTURE
	cmd_udec_capture(&capture);
#else
	cmd_postp_capture(&capture);
#endif

	//fill base
	capture.capture[CAPTURE_CODED].buffer_base = G_decode_obj->dec_capture.capture[CAPTURE_CODED].buffer_base;
	capture.capture[CAPTURE_THUMBNAIL].buffer_base = G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].buffer_base;
	capture.capture[CAPTURE_SCREENNAIL].buffer_base = G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].buffer_base;

	//wait related msg
	while (1) {

		dsp_lock(flags);

		if (G_decode_obj->processed_last_capture_msg_seqnum!= G_decode_obj->last_capture_msg_seqnum) {
			G_decode_obj->processed_last_capture_msg_seqnum = G_decode_obj->last_capture_msg_seqnum;
			//fill base limit
			capture.capture[CAPTURE_CODED].buffer_limit = G_decode_obj->dec_capture.capture[CAPTURE_CODED].buffer_base + G_decode_obj->main_coded_size;
			capture.capture[CAPTURE_THUMBNAIL].buffer_limit = G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].buffer_base + G_decode_obj->thumb_coded_size;
			capture.capture[CAPTURE_SCREENNAIL].buffer_limit = G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].buffer_base + G_decode_obj->screen_coded_size;
			dsp_unlock(flags);
			break;
		}

		G_decode_obj->num_waiters++;
		dsp_unlock(flags);

		if (wait_decode_msg(context) < 0)
			return -EINTR;
	}

	//invalidate cache
	if (capture.capture_coded) {
		size = (G_decode_obj->main_coded_size + 16) & (~(16 - 1));
		invalidate_d_cache((void*)G_decode_obj->dec_capture.capture[CAPTURE_CODED].buffer_base, size);
	}

	if (capture.capture_thumbnail) {
		size = (G_decode_obj->thumb_coded_size + 16) & (~(16 - 1));
		invalidate_d_cache((void*)G_decode_obj->dec_capture.capture[CAPTURE_THUMBNAIL].buffer_base, size);
	}

	if (capture.capture_screennail) {
		size = (G_decode_obj->screen_coded_size + 16) & (~(16 - 1));
		invalidate_d_cache((void*)G_decode_obj->dec_capture.capture[CAPTURE_SCREENNAIL].buffer_base, size);
	}

	if (copy_to_user(arg, &capture, sizeof(capture)))
		return -EFAULT;

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

int iav_release_udec_capture(iav_context_t *context, u32 flag)
{
	if (check_udec_mode()) {
		UDEC_LOG_ERROR("not in UDEC mode\n");
		return -EPERM;
	}

	if (!G_decode_obj->capture_context_setup) {
		UDEC_LOG_ERROR("udec_capture is not setup? but someone invoke release...\n");
		return -EPERM;
	}

	//flag check
	if (!is_mode_flag_set(context, IAV_MODE_UDEC_CAPTURE)) {
		UDEC_LOG_ERROR("this context is not owner, cannot release\n");
		return -EPERM;
	}

	__destroy_capture_context(context);
	clear_mode_flag(context, IAV_MODE_UDEC_CAPTURE);
	return 0;
}

int iav_udec_zoom(iav_context_t *context, iav_udec_zoom_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_udec_zoom_t zoom;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&zoom, arg, sizeof(zoom)))
		return -EFAULT;

	cmd_udec_zoom(&zoom);
	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static void fill_udec_base_setup_cmd(iav_context_t *context, iav_udec_info_ex_t *info, UDEC_SETUP_BASE_CMD *dsp_cmd)
{
	dsp_cmd->cmd_code = CMD_UDEC_SETUP;

	dsp_cmd->decoder_id = info->udec_id;
	dsp_cmd->decoder_type = dsp_udec_type(info->udec_type);

	if (dsp_cmd->decoder_type != UDEC_TYPE_STILL) {
		dsp_cmd->enable_pp = info->enable_pp;	// todo
		dsp_cmd->enable_deint = info->enable_deint;
		dsp_cmd->enable_err_handle = info->enable_err_handle;
	}

	dsp_cmd->bits_fifo_size = info->bits_fifo_size;
	dsp_cmd->ref_cache_size = info->ref_cache_size;

	dsp_cmd->concealment_mode = info->concealment_mode;
	dsp_cmd->concealment_ref_frm_buf_id = 0;

	if (info->other_flags & IAV_UDEC_VALIDATION_ONLY)
		dsp_cmd->validation_only = 1;

	if (info->other_flags & IAV_UDEC_FORCE_DECODE)
		dsp_cmd->force_decode = 1;
}

static void cmd_udec_setup(iav_context_t *context, iav_udec_info_ex_t *info, u32 *mv_fifo_size)
{
	*mv_fifo_size = 0;

	switch (info->udec_type) {
	case UDEC_NONE:
		BUG();
		break;

	case UDEC_H264: {
			UDEC_SETUP_H264_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);
			dsp_cmd.pjpeg_buf_size = info->u.h264.pjpeg_buf_size;

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_MP12:
	case UDEC_MP4H: {
			UDEC_SETUP_MPEG24_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);
			dsp_cmd.deblocking_flag = info->u.mpeg.deblocking_flag;
			dsp_cmd.pquant_mode = info->u.mpeg.pquant_mode;
			memcpy(dsp_cmd.pquant_table, info->u.mpeg.pquant_table, 32);
			dsp_cmd.is_avi_flag = info->u.mpeg.is_avi_flag;

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_MP4S: {
			UDEC_SETUP_MPEG4S_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);

			dsp_cmd.mv_fifo_size = info->u.mp4s.mv_fifo_size;
			dsp_cmd.deblocking_flag = info->u.mp4s.deblocking_flag;
			dsp_cmd.pquant_mode = info->u.mp4s.pquant_mode;
			memcpy(dsp_cmd.pquant_table, info->u.mp4s.pquant_table, 32);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));

			*mv_fifo_size = info->u.mp4s.mv_fifo_size;
		}
		break;

	case UDEC_VC1: {
			UDEC_SETUP_VC1_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_RV40: {
			UDEC_SETUP_RV40_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));

			*mv_fifo_size = info->u.rv40.mv_fifo_size;
		}
		break;

	case UDEC_JPEG: {
			UDEC_SETUP_SDEC_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);

			dsp_cmd.still_bits_circular = info->u.jpeg.still_bits_circular;
			dsp_cmd.still_max_decode_width = info->u.jpeg.still_max_decode_width;
			dsp_cmd.still_max_decode_height = info->u.jpeg.still_max_decode_height;

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_SW: {
			UDEC_SETUP_SW_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			fill_udec_base_setup_cmd(context, info, &dsp_cmd.base_cmd);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	default:
		break;
	}
}

static void cmd_set_audio_clk(void)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	struct ambarella_i2s_interface  i2s_config;
	DSP_AUDIO_CLK_FREQUENCY_CMD dsp_cmd;

	i2s_config = get_audio_i2s_interface();
	switch (i2s_config.mclk) {
	default:
	case AudioCodec_18_432M: dsp_cmd.audio_clk_freq = 18432000; break;
	case AudioCodec_16_9344M: dsp_cmd.audio_clk_freq = 16934400; break;
	case AudioCodec_12_288M: dsp_cmd.audio_clk_freq = 12288000; break;
	case AudioCodec_11_2896M: dsp_cmd.audio_clk_freq = 11289600; break;
	case AudioCodec_9_216M: dsp_cmd.audio_clk_freq = 9216000; break;
	case AudioCodec_8_4672M: dsp_cmd.audio_clk_freq = 8467200; break;
	case AudioCodec_8_192M: dsp_cmd.audio_clk_freq = 8192000; break;
	case AudioCodec_6_144: dsp_cmd.audio_clk_freq = 6144000; break;
	case AudioCodec_5_6448M: dsp_cmd.audio_clk_freq = 5644800; break;
	case AudioCodec_4_608M: dsp_cmd.audio_clk_freq = 4608000; break;
	case AudioCodec_4_2336M: dsp_cmd.audio_clk_freq = 4233600; break;
	case AudioCodec_4_096M: dsp_cmd.audio_clk_freq = 4096000; break;
	case AudioCodec_3_072M: dsp_cmd.audio_clk_freq = 3072000; break;
	case AudioCodec_2_8224M: dsp_cmd.audio_clk_freq = 2822400; break;
	case AudioCodec_2_048M: dsp_cmd.audio_clk_freq = 2048000; break;
	}

	iav_dbg_printk("set audio clk to %d\n", dsp_cmd.audio_clk_freq);

	dsp_cmd.cmd_code = CMD_DSP_AUDIO_CLK_FREQUENCY;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
#endif
}

static void cmd_deint_init(iav_udec_deint_config_t *deint_config)
{
	DEINT_INIT_CMD dsp_cmd;
	dsp_cmd.cmd_code = CMD_DEINT_INIT;
	dsp_cmd.init_tff = deint_config->init_tff;
	dsp_cmd.deint_lu_en = deint_config->deint_lu_en;
	dsp_cmd.deint_ch_en = deint_config->deint_ch_en;
	dsp_cmd.osd_en = 0;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_deint_config(iav_udec_deint_config_t *deint_config)
{
	DEINT_CONF_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_DEINT_CONF;
	dsp_cmd.deint_mode = deint_config->deint_mode;
	dsp_cmd.deint_spatial_shift = deint_config->deint_spatial_shift;
	dsp_cmd.deint_lowpass_shift = deint_config->deint_lowpass_shift;
	dsp_cmd.deint_lowpass_center_weight = deint_config->deint_lowpass_center_weight;
	dsp_cmd.deint_lowpass_hor_weight = deint_config->deint_lowpass_hor_weight;
	dsp_cmd.deint_lowpass_ver_weight = deint_config->deint_lowpass_ver_weight;
	dsp_cmd.deint_gradient_bias = deint_config->deint_gradient_bias;
	dsp_cmd.deint_predict_bias = deint_config->deint_predict_bias;
	dsp_cmd.deint_candidate_bias = deint_config->deint_candidate_bias;
	dsp_cmd.deint_spatial_score_bias = deint_config->deint_spatial_score_bias;
	dsp_cmd.deint_temporal_score_bias = deint_config->deint_temporal_score_bias;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void init_udec_mode(void *context)
{
	iav_udec_mode_config_t *udec_mode = context;
	int i;

	cmd_set_udec_mode(udec_mode);

	cmd_set_audio_clk();

	if (G_decode_obj->deint_config_valid) {
		iav_udec_deint_config_t *deint_config = &G_decode_obj->udec_deint_config;
		cmd_deint_init(deint_config);
		cmd_deint_config(deint_config);
	}

	for (i = 0; i < udec_mode->num_udecs; i++) {
		cmd_udec_init(i, __udec_config(i));
	}
}

static void cmd_udec_stop(int udec_id, int stop_flag)
{
	UDEC_STOP_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_STOP;
	dsp_cmd.decoder_id = udec_id;
	dsp_cmd.stop_flag = !!stop_flag;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_udec_exit(int udec_id, int stop_flag)
{
	UDEC_EXIT_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_EXIT;
	dsp_cmd.decoder_id = udec_id;
	dsp_cmd.stop_flag = !!stop_flag;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

static inline void reset_udec(udec_t *udec)
{
	udec->bits_fifo_ptr = udec->bits_fifo_base;
	udec->dsp_bits_fifo_next_phys = udec->bits_fifo_base;
	udec->dsp_mv_fifo_next_phys = udec->mv_fifo_base;
	udec->error_code = 0;
	udec->abort_flag = 0;
	udec->pts_comes = 0;
	udec->code_timeout_pc = -1;
	udec->code_timeout_reason = -1;
	udec->mdxf_timeout_pc = -1;
	udec->mdxf_timeout_reason = -1;
}

static void stop_udec(iav_context_t *context, udec_t *udec, int stop_flag)
{
	int state = __get_udec_state(udec);

	if (state == UDEC_STAT_RUN || state == UDEC_STAT_ERROR) {
		cmd_udec_stop(udec->udec_id, stop_flag);
		wait_udec_state(udec, UDEC_STAT_READY);
	}

	if (udec->udec_stopped == 0) {
		udec->udec_stopped = 1;
		udec->udec_status = 0;
		reset_udec(udec);
		udec_notify_waiters(udec);
		dsp_notify_waiters();
	}
}

static void release_udec(iav_context_t *context, udec_t *udec, int stop_flag)
{
	int i;

	// stop udec
	stop_udec(context, udec, stop_flag);

	// release vout
	for (i = 0; i < ARRAY_SIZE(context->vout_flag); i++) {
		if (context->vout_flag[i]) {
			iav_dbg_printk("=== release vout %d ===\n", i);
			flush_vout(context, i);
			context->vout_flag[i] = 0;
			__vout_info(i)->vout_inuse = 0;
		}
	}

	// exit udec
	if (__get_udec_state(udec) == UDEC_STAT_READY) {
		iav_dbg_printk("=== exit udec ===\n");
		cmd_udec_exit(udec->udec_id, stop_flag);
		wait_udec_state(udec, UDEC_STAT_IDLE);
	}

	// release fbp
	iav_dbg_printk("=== release fbp ===\n");
	__fb_pool_release_all(&udec->udec_fb_pool);

	// memory mappings
	iav_dbg_printk("=== destroy mmaps ===\n");
	destroy_mmap(context, &udec->bits_fifo);
	destroy_mmap(context, &udec->mv_fifo);
	destroy_mmap(context, &udec->jpeg_y);
	destroy_mmap(context, &udec->jpeg_uv);

	udec->udec_inuse = 0;
	context->udec_flags &= ~(1 << udec->udec_id);

	if (!context->udec_flags) {
		g_udec_debug_on = 0;
	}
}

static int config_udecs(iav_udec_mode_config_t *udec_mode)
{
	int i;

	//udec_mode->enable_deint

	if (udec_mode->pp_chroma_fmt_max >= 3) {
		iav_dbg_printk("bad pp_chroma_fmt_max: %d\n", udec_mode->pp_chroma_fmt_max);
		return -EINVAL;
	}

	//udec_mode->pp_max_frm_width;
	//udec_mode->pp_max_frm_height;

	if (udec_mode->pp_max_frm_num > MAX_DECODED_FRAMES) {
		iav_dbg_printk("bad pp_max_frm_num: %d\n", udec_mode->pp_max_frm_num);
		return -EINVAL;
	}

	if (udec_mode->num_udecs == 0 || udec_mode->num_udecs > G_decode_obj->udec_max_number) {
		iav_dbg_printk("bad num_udecs: %d\n", udec_mode->num_udecs);
		return -EINVAL;
	}

	for (i = 0; i < udec_mode->num_udecs; i++) {
		iav_udec_config_t *udec_config;

		udec_config = __udec_config(i);
		if (copy_from_user(udec_config, udec_mode->udec_config + i, sizeof(*udec_config)))
			return -EFAULT;

		//udec_config->tiled_mode

		if (udec_config->frm_chroma_fmt_max > 1) {
			iav_dbg_printk("bad frm_chroma_fmt_max: %d\n", udec_config->frm_chroma_fmt_max);
			return -EINVAL;
		}

		G_decode_obj->input_codec_mask = udec_config->dec_types;

		//udec_config->dec_types

		if (udec_config->max_frm_num > MAX_DECODED_FRAMES) {
			iav_dbg_printk("bad max_frm_num: %d\n", udec_config->max_frm_num);
			return -EINVAL;
		}

		if ((udec_config->max_frm_width | udec_config->max_frm_height) == 0) {
			iav_dbg_printk("max_frm_width: %d, max_frm_height: %d\n",
				udec_config->max_frm_width, udec_config->max_frm_height);
			return -EINVAL;
		}

		udec_config->max_fifo_size = udec_config->max_fifo_size;
	}

	return 0;
}

static void do_enter_udec_mode(iav_context_t *context, iav_udec_mode_config_t *udec_mode)
{
	// install msg handler
	dsp_set_cat_msg_handler(handle_udec_msg, CAT_UDEC, NULL);
	dsp_set_cat_msg_handler(handle_postp_msg, CAT_POSTP, NULL);

	// vout irq handler
	iav_vout_set_irq_handler(__iav_decode_vout_irq);

	// enter UDEC mode
	dsp_set_mode(DSP_OP_MODE_UDEC, init_udec_mode, udec_mode);

	iav_vout_set_preproc(decode_vout_preproc);

	G_decode_obj->enable_vm = G_enable_vm;
	G_iav_info.state = IAV_STATE_DECODING;
	G_iav_obj.dec_type = DEC_TYPE_UDEC;

	set_mode_flag(context, IAV_MODE_UDEC);
}

static void clear_vout_owner(void)
{
	int i;
	for (i = 0; i < IAV_NUM_VOUT; i++) {
		__set_vout_owner(i, -1);
		G_decode_obj->vout_config_valid[i] = 0;
	}
}

int iav_enter_udec_mode(iav_context_t *context, iav_udec_mode_config_t __user *arg)
{
	iav_udec_mode_config_t *udec_mode;
	int rval;

	// only in IDLE state
	if (G_iav_info.state != IAV_STATE_IDLE || !G_iav_info.dsp_booted) {
		UDEC_LOG_ERROR("bad state: %d, booted: %d\n", G_iav_info.state, G_iav_info.dsp_booted);
		return -EPERM;
	}

	// vout owner
	clear_vout_owner();

	udec_mode = __udec_mode_config();
	if (copy_from_user(udec_mode, arg, sizeof(*udec_mode)))
		return -EFAULT;

	// verify parameters

	if (udec_mode->postp_mode > 2) {
		iav_dbg_printk("bad postp_mode: %d\n", udec_mode->postp_mode);
		return -EINVAL;
	}

	G_decode_obj->udec_max_number = MAX_NUM_DECODER;

	if ((rval = config_udecs(udec_mode)) < 0)
		return rval;

	// deinterlacer
	G_decode_obj->deint_config_valid = 0;
	if (udec_mode->deint_config) {
		iav_udec_deint_config_t *deint_config = &G_decode_obj->udec_deint_config;
		if (copy_from_user(deint_config, udec_mode->deint_config, sizeof(*deint_config)))
			return -EFAULT;
		// todo - check
		G_decode_obj->deint_config_valid = 1;
	}

	// allocate memory for this mode
	if ((rval = dsp_alloc_mode_vms(80*MB)) < 0)
		return rval;

	context->need_issue_reset_hdmi = 1;
	if (udec_mode->postp_mode == 2) {
		iav_dbg_printk("vout_mask: 0x%x\n", udec_mode->vout_mask);
		iav_change_vout_src_ex(context, udec_mode->vout_mask, VOUT_SRC_UDEC);
	}

	G_decode_obj->input_feature_constrains = udec_mode->feature_constrains;

//wait debug 512k udec mode
#ifdef CONFIG_IAV_ENABLE_512K_WITH_CONSTRAINED_FEATURE
	__check_feature_constrains(udec_mode->vout_mask);

	if (G_decode_obj->input_feature_constrains.set_constrains_enable) {
		if (G_decode_obj->input_feature_constrains.always_disable_l2_cache) {
			G_decode_obj->always_disable_l2_cache = 1;
		} else {
			G_decode_obj->always_disable_l2_cache = 0;
		}
	}
	iav_dbg_printk("G_decode_obj->always_disable_l2_cache: %d.\n", G_decode_obj->always_disable_l2_cache);

	if (G_decode_obj->dsp_only_use_512k_smem) {
		dsp_smem_strategy(DSP_OP_MODE_UDEC, 1, G_decode_obj->always_disable_l2_cache);
	} else {
		dsp_smem_strategy(DSP_OP_MODE_UDEC, 0, G_decode_obj->always_disable_l2_cache);
	}
#endif

	//dewarp parameters
	G_decode_obj->dewarp_feature_enabled = 0;
	if (udec_mode->enable_horizontal_dewarp) {
		UDEC_DEBUG_ASSERT(2 == udec_mode->postp_mode);
		if (2 == udec_mode->postp_mode) {
			if (1 != udec_mode->vout_mask) {
				UDEC_LOG_ERROR("dewarp only supported in LCD only, disable dewarp here.\n");
			} else {
				UDEC_LOG_ALWAYS("dewarp enabled in LCD only's case, voutmask 0x%x.\n", udec_mode->vout_mask);
				G_decode_obj->dewarp_feature_enabled = 1;
			}
		} else {
			UDEC_LOG_ERROR("dewarp only supported in ppmode(%d) = 2.\n", udec_mode->postp_mode);
		}
	} else {
		G_decode_obj->dewarp_feature_enabled = 0;
	}
	G_decode_obj->horz_warp_enable = 0;

	do_enter_udec_mode(context, udec_mode);

	return 0;
}

static int save_vout_configs(int udec_id, iav_udec_display_t *vout_display, u8 vout_ids[])
{
	int i;

	if (vout_display->num_vout > IAV_NUM_VOUT) {
		iav_dbg_printk("too many vout: %d\n", vout_display->num_vout);
		return -EINVAL;
	}

	for (i = 0; i < vout_display->num_vout; i++) {
		iav_udec_vout_config_t *vout_config;
		udec_vout_info_t *vout_info;
		u32 vout_id;

		if (get_user(vout_id, (u8 __user*)&vout_display->vout_config[i].vout_id))
			return -EINVAL;

		if (vout_id >= IAV_NUM_VOUT) {
			iav_dbg_printk("bad vout id: %d\n", vout_id);
			return -EINVAL;
		}

		vout_info = __vout_info(vout_id);
		if (vout_info->vout_inuse && __udec_mode_config()->postp_mode != 3) {
			iav_dbg_printk("vout %d is already in use\n", vout_id);
			return -EBUSY;
		}

		vout_config = __vout_config(vout_id);
		if (copy_from_user(vout_config, vout_display->vout_config + i, sizeof(*vout_config)))
			return -EFAULT;

		vout_config->vout_id = vout_id;

		vout_ids[i] = vout_id;
	}

	return 0;
}

int iav_init_udec(iav_context_t *context, iav_udec_info_ex_t __user *arg)
{
	iav_udec_mode_config_t *udec_mode;
	iav_udec_info_ex_t *udec_info;
	u8 vout_ids[IAV_NUM_VOUT] = {0};
	udec_t *udec;
	u32 udec_id;
	u32 mv_fifo_size = 0;
	u8 *user_start;
	int rval;
	int i;
	u32 bit_stream_addr_phycs;
#ifdef DEBUG_DUMP_DEWARP_TABLE
	struct file *f;
#endif

	// check if we're in UDEC mode
	if (check_udec_mode() < 0)
		return -EPERM;

	if (get_user(udec_id, (u8 __user *)&arg->udec_id))
		return -EFAULT;

	udec_mode = __udec_mode_config();

	if (udec_id >= udec_mode->num_udecs) {
		iav_dbg_printk("bad udec_id %d\n", udec_id);
		return -EINVAL;
	}

	// save udec info
	udec_info = __udec_info(udec_id);
	if (copy_from_user(udec_info, arg, sizeof(*udec_info)))
		return -EFAULT;

	udec = __udec(udec_id);
	if (udec->udec_inuse) {
		iav_dbg_printk("udec %d is in use\n", udec_id);
		return -EBUSY;
	}

	if (udec_info->udec_type == UDEC_NONE || udec_info->udec_type >= UDEC_LAST) {
		iav_dbg_printk("bad udec_type: %d\n", udec_info->udec_type);
		return -EINVAL;
	}

	if (udec_info->concealment_mode != 0 && udec_info->concealment_mode != 1) {
		iav_dbg_printk("concealment_mode %d is not supported\n", udec_info->concealment_mode);
		return -EINVAL;
	}

	// udec_info->udec_type - todo
	// udec_info->enable_pp - todo
	// udec_info->enable_deint - todo
	// udec_info->interlaced_out - todo

	if ((rval = save_vout_configs(udec_id, &udec_info->vout_configs, vout_ids)) < 0)
		return rval;

	if (udec_info->vout_configs.dewarp_params_updated) {
		UDEC_LOG_ALWAYS("update to new dewarp settings 2.\n");
		rval = _interpret_dewarp_params(&udec_info->vout_configs.dewarp);
		if (rval < 0) {
			UDEC_LOG_ERROR("BAD params.\n");
			return -EINVAL;
		}

#ifdef DEBUG_DUMP_DEWARP_TABLE
		f = filp_open("/tmp/mmcblk1p1/dewarp_bin", O_WRONLY | O_CREAT | O_TRUNC, 0);
		if (f) {
			mm_segment_t oldfs = get_fs();
			set_fs(KERNEL_DS);
			ssize_t ret= f->f_op->write(f, G_decode_obj->dewarp_table, (G_decode_obj->grid_array_width + 1)*(G_decode_obj->grid_array_height + 1) * sizeof(s16), &f->f_pos);
			set_fs(oldfs);
			filp_close(f, NULL);
		}
#endif
		if (!rval) {
			_debug_print_warp_table_vectors_hex(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
			//_debug_print_warp_table_vectors_float(G_decode_obj->dewarp_table, G_decode_obj->grid_array_width, G_decode_obj->grid_array_height);
		}
	}

	for (i = 0; i < udec_info->vout_configs.num_vout; i++) {
		udec_vout_info_t *vout_info;
		unsigned long flags;
		u32 vout_id;

		vout_id = vout_ids[i];
		vout_info = __vout_info(vout_id);

		dsp_lock(flags);
		vout_info->vout_inuse = 1;
		vout_info->rendered = 0;
		vout_info->irq_entered = 0;
		vout_info->frame_not_rendered = 0;
		dsp_unlock(flags);

		__set_vout_owner(vout_id, udec_id);
		context->vout_flag[vout_id] = 1;
	}

	udec->udec_id = udec_id;
	udec->udec_type = udec_info->udec_type;
	udec->noncachable_buffer = udec_info->noncachable_buffer;
	udec->udec_stopped = 0;
	udec->udec_status = 0;
	udec->pts_comes = 0;
	g_decode_cmd_cnt[udec_id] = 0;
	udec->decode_frame_count = 0;

	udec->udec_inuse = 1;
	context->udec_flags |= (1 << udec_id);

	if (udec_mode->postp_mode == 1) {
		if (udec_info->udec_type != UDEC_JPEG && udec_info->vout_configs.num_vout > 0) {
			cmd_decpp_config(udec_info);
		}
	}

	cmd_udec_setup(context, udec_info, &mv_fifo_size);

	if (udec_mode->postp_mode == 2) {
		if (udec_info->udec_type != UDEC_JPEG && udec_info->vout_configs.num_vout > 0) {
			cmd_udec_postproc(udec_id, udec->udec_type);
		}
	}

	wait_udec_state(udec, UDEC_STAT_READY);

	bit_stream_addr_phycs = DSP_TO_PHYS(udec->bits_fifo_base);
	UDEC_LOG_ALWAYS("[BSB], udec->bits_fifo_base 0x%08x, bit_stream_addr_phycs 0x%08x\n", udec->bits_fifo_base, bit_stream_addr_phycs);

	// bits fifo
	udec_info->bits_fifo_start = NULL;
	if (udec_info->bits_fifo_size > 0) {
		udec_info->bits_fifo_size = udec_info->bits_fifo_size;
		user_start = map_udec_fifo(context, "bits fifo",
			bit_stream_addr_phycs, udec_info->bits_fifo_size, &udec->bits_fifo, udec_info->noncachable_buffer);
		if (user_start == NULL) {
			rval = -ENOMEM;
			goto Error;
		}
		udec_info->bits_fifo_start = user_start;
	}

	// mv fifo
	udec_info->mv_fifo_start = NULL;
	if (mv_fifo_size > 0) {
		user_start = map_udec_fifo(context, "mv fifo",
			udec->mv_fifo_base, mv_fifo_size, &udec->mv_fifo, udec->noncachable_buffer);
		if (user_start == NULL) {
			rval = -ENOMEM;
			goto Error;
		}
		udec_info->mv_fifo_start = user_start;
	}

	reset_udec(udec);

	__fb_queue_reset(&udec->outpic_queue);

	if (copy_to_user(arg, udec_info, sizeof(*udec_info))) {
		rval = -EFAULT;
		goto Error;
	}

	if (udec_mode->postp_mode == 1) {
		iav_change_vout_src(context, VOUT_SRC_DEFAULT_IMG);
	}

	iav_dbg_printk("init udec %d done\n", udec->udec_id);
	g_udec_debug_on = 1;
	UDEC_LOG_ALWAYS("[important flow]: iav_init_udec(context %p, id %d, udec->noncachable_buffer %d)!\n", context, udec->udec_id, udec->noncachable_buffer);
	return 0;

Error:
	release_udec(context, udec, 0);
	return rval;
}

int iav_release_udec(iav_context_t *context, u32 udec_id)
{
	u32 stop_flag;
	udec_t *udec;

	stop_flag = udec_id >> 24;
	udec_id &= 0xFF;

	if ((udec = get_udec(context, udec_id)) == NULL)
		return -EINVAL;

	release_udec(context, udec, stop_flag);
	UDEC_LOG_ALWAYS("[important flow]: iav_release_udec(context %p, id %d)!\n", context, udec_id);
	return 0;
}

static u16 get_real_fb_id(udec_t *udec, u16 fb_index)
{
	fb_pool_t *fb_pool;

	if (fb_index >= MAX_DECODED_FRAMES) {
		iav_dbg_printk("bad fb_index: %d\n", fb_index);
		return IAV_INVALID_FB_ID;
	}

	fb_pool = get_fb_pool(udec);

	if (__get_fb_state(fb_pool, fb_index) == FBS_INVALID) {
		iav_dbg_printk("invalid fb, fb_index: %d\n", fb_index);
		return IAV_INVALID_FB_ID;
	}

	return __fb_pool_get(fb_pool, fb_index)->real_fb_id;
}

static int cmd_udec_decode(iav_context_t *context, udec_t *udec, iav_udec_decode_t *info)
{
	switch (udec->udec_type) {
	case UDEC_NONE:
		return -EPERM;

	case UDEC_H264:
	case UDEC_MP12:
	case UDEC_MP4H:
	case UDEC_VC1:
	case UDEC_JPEG: {
			udec_mmap_t *mmap;
			u8 *u_start_addr;
			u8 *u_end_addr;
			u32 start_addr;
			u32 end_addr;

			UDEC_DEC_FIFO_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			dsp_cmd.base_cmd.cmd_code = CMD_UDEC_DECODE;
			dsp_cmd.base_cmd.decoder_id = info->decoder_id;
			dsp_cmd.base_cmd.decoder_type = dsp_udec_type(info->udec_type);
			dsp_cmd.base_cmd.num_of_pics = info->num_pics;

			mmap = &udec->bits_fifo;
			u_start_addr = info->u.fifo.start_addr;
			u_end_addr = info->u.fifo.end_addr;
			if (get_udec_bits_range(mmap,
					u_start_addr, u_end_addr,
					&start_addr, &end_addr) < 0) {
				UDEC_LOG_ERROR("BAD address, get_udec_bits_range check fail: u_start_addr 0x%x, u_end_addr 0x%x\n", (u32)u_start_addr, (u32)u_end_addr);
				return -EFAULT;
			}

			dsp_cmd.bits_fifo_start = PHYS_TO_DSP(start_addr);
			dsp_cmd.bits_fifo_end = PHYS_TO_DSP(end_addr) - 1;

			udec->dsp_bits_fifo_last_write_phys = PHYS_TO_DSP(end_addr);//for debug

			if (dsp_cmd.bits_fifo_start != udec->bits_fifo_ptr) {
				if (dsp_cmd.bits_fifo_start != (u32)udec->bits_fifo.phys_start) {
					UDEC_LOG_ERROR("bad fifo.start_addr: 0x%x\n", (u32)info->u.fifo.start_addr);
					return -EINVAL;
				}
			}
			udec->bits_fifo_ptr = dsp_cmd.bits_fifo_end + 1;

			if (!udec->noncachable_buffer) {
				clean_fifo_cache(mmap, start_addr, end_addr);
			}

			udec->decode_frame_count += dsp_cmd.base_cmd.num_of_pics;

			if (udec->udec_type == UDEC_JPEG) {
				iav_dbg_printk("udec_decode: 0x%x - 0x%x\n", start_addr, end_addr);
				dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
			} else {
				//dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
				dsp_issue_udec_dec_cmd(&dsp_cmd, sizeof(dsp_cmd));
			}

			//iav_dbg_printk("[BSB]: decoding, usr addr [%p, %p), dsp addr [%p, %p], diff 0x%x, udec->dsp_bits_fifo_last_write_phys %p\n", u_start_addr, u_end_addr, dsp_cmd.bits_fifo_start, dsp_cmd.bits_fifo_end, u_end_addr - u_start_addr, udec->dsp_bits_fifo_last_write_phys);
		}
		break;

	case UDEC_MP4S: {
			UDEC_DEC_MP4S_CMD dsp_cmd;
			udec_mmap_t *mmap;
			u8 *u_start_addr;
			u8 *u_end_addr;
			u32 start_addr;
			u32 end_addr;

			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			dsp_cmd.base_cmd.cmd_code = CMD_UDEC_DECODE;
			dsp_cmd.base_cmd.decoder_id = info->decoder_id;
			dsp_cmd.base_cmd.decoder_type = dsp_udec_type(info->udec_type);
			dsp_cmd.base_cmd.num_of_pics = info->num_pics;

			// bits fifo
			mmap = &udec->bits_fifo;
			u_start_addr = info->u.mp4s2.vop_coef_start_addr;
			u_end_addr = info->u.mp4s2.vop_coef_end_addr;
			if (get_udec_bits_range(mmap,
					u_start_addr, u_end_addr,
					&start_addr, &end_addr) < 0)
				return -EFAULT;

			dsp_cmd.vop_coef_start_addr = PHYS_TO_DSP(start_addr);
			dsp_cmd.vop_coef_end_addr = PHYS_TO_DSP(end_addr);

			udec->dsp_bits_fifo_last_write_phys = dsp_cmd.vop_coef_end_addr;//for debug

			clean_fifo_cache(mmap, start_addr, end_addr);

			// mv fifo
			mmap = &udec->mv_fifo;
			u_start_addr = info->u.mp4s2.vop_mv_start_addr;
			u_end_addr = info->u.mp4s2.vop_mv_end_addr;
			if (get_udec_bits_range(mmap,
					u_start_addr, u_end_addr,
					&start_addr, &end_addr) < 0)
				return -EFAULT;

			dsp_cmd.vop_mv_start_addr = PHYS_TO_DSP(start_addr);
			dsp_cmd.vop_mv_end_addr = PHYS_TO_DSP(end_addr);

			clean_fifo_cache(mmap, start_addr, end_addr);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_RV40: {
			udec_mmap_t *mmap;
			u8 *u_start_addr;
			u8 *u_end_addr;
			u32 start_addr;
			u32 end_addr;

			UDEC_DEC_RV40_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			dsp_cmd.base_cmd.cmd_code = CMD_UDEC_DECODE;
			dsp_cmd.base_cmd.decoder_id = info->decoder_id;
			dsp_cmd.base_cmd.decoder_type = dsp_udec_type(info->udec_type);
			dsp_cmd.base_cmd.num_of_pics = info->num_pics;

			mmap = &udec->bits_fifo;
			u_start_addr = info->u.rv40.residual_fifo_start;
			u_end_addr = info->u.rv40.residual_fifo_end;
			if (get_udec_bits_range(mmap,
					u_start_addr, u_end_addr,
					&start_addr, &end_addr) < 0)
				return -EFAULT;

			dsp_cmd.residual_fifo_start = PHYS_TO_DSP(start_addr);
			dsp_cmd.residual_fifo_end = PHYS_TO_DSP(end_addr) - 1;

			udec->dsp_bits_fifo_last_write_phys = PHYS_TO_DSP(end_addr);//for debug

			clean_fifo_cache(mmap, start_addr, end_addr);

			mmap = &udec->mv_fifo;
			u_start_addr = info->u.rv40.mv_fifo_start;
			u_end_addr = info->u.rv40.mv_fifo_end;
			if (get_udec_bits_range(mmap,
					u_start_addr, u_end_addr,
					&start_addr, &end_addr) < 0)
				return -EFAULT;

			dsp_cmd.mv_fifo_start = PHYS_TO_DSP(start_addr);
			dsp_cmd.mv_fifo_end = PHYS_TO_DSP(end_addr) - 1;

			clean_fifo_cache(mmap, start_addr, end_addr);

			if ((dsp_cmd.fwd_ref_buf_id = get_real_fb_id(udec, info->u.rv40.fwd_ref_fb_id)) == IAV_INVALID_FB_ID)
				return -EINVAL;

			if (info->u.rv40.clean_fwd_ref_fb)
				clean_fb_cache(context, udec, info->u.rv40.fwd_ref_fb_id);

			// backward
			if ((dsp_cmd.bwd_ref_buf_id = get_real_fb_id(udec, info->u.rv40.bwd_ref_fb_id)) == IAV_INVALID_FB_ID)
				return -EINVAL;

			if (info->u.rv40.clean_bwd_ref_fb)
				clean_fb_cache(context, udec, info->u.rv40.bwd_ref_fb_id);

			// target
			if ((dsp_cmd.target_buf_id = get_real_fb_id(udec, info->u.rv40.target_fb_id)) == IAV_INVALID_FB_ID)
				return -EINVAL;

			dsp_cmd.pic_width = info->u.rv40.pic_width;
			dsp_cmd.pic_height = info->u.rv40.pic_height;

			dsp_cmd.pic_coding_type = info->u.rv40.pic_coding_type;
//			dsp_cmd.tiled_mode = __valid_tilemode_value(info->u.rv40.tiled_mode);

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	case UDEC_SW: {
			UDEC_DEC_SW_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));

			dsp_cmd.base_cmd.cmd_code = CMD_UDEC_DECODE;
			dsp_cmd.base_cmd.decoder_type = UDEC_TYPE_SW;
			dsp_cmd.base_cmd.num_of_pics = 0;

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
		}
		break;

	default:
		break;
	}

	return 0;
}

int iav_udec_decode(iav_context_t *context, iav_udec_decode_t __user *arg)
{
	iav_udec_decode_t info;
	udec_t *udec;
	int prev_state;
	int rval;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if ((udec = get_udec(context, info.decoder_id)) == NULL)
		return -EINVAL;

	prev_state = __get_udec_state(udec);

	if (prev_state != UDEC_STAT_READY && prev_state != UDEC_STAT_RUN) {
		iav_dbg_printk("iav_udec_decode: bad state %d\n", prev_state);
		return -EPERM;
	}

	if (udec->udec_stopped) {
		iav_dbg_printk("iav_udec_decode: already stopped\n");
		return -EPERM;
	}

	// do decoding
	if ((rval = cmd_udec_decode(context, udec, &info)) < 0)
		return rval;

	if (prev_state == UDEC_STAT_READY) {
		int state = __wait_udec_state(udec, UDEC_STAT_RUN, UDEC_STAT_ERROR, "UDEC_STAT_RUN");
		if (state == UDEC_STAT_ERROR) {
			iav_dbg_printk("iav_udec_decode: UDEC ERROR\n");
			return -EPERM;
		}
		udec->udec_stopped = 0;
	}

	return 0;
}

// high byte: stop flag
int iav_stop_udec(iav_context_t *context, u32 udec_id)
{
	u32 stop_flag;
	udec_t *udec;

	stop_flag = udec_id >> 24;
	udec_id &= 0xFF;

	if ((udec = get_udec(context, udec_id)) == NULL)
		return -EINVAL;

	if (stop_flag == 0xFF) {
		if (__get_udec_state(udec) == UDEC_STAT_READY && udec->udec_stopped) {
			udec->udec_stopped = 0;
			return 0;
		}
		return -EPERM;
	} else {
		stop_udec(context, udec, stop_flag);
		return 0;
	}
}

static inline void __release_vout_fb(vout_fb_t *fb)
{
	if (fb->valid) {
		__release_fb(fb->decoder_id, fb->fb_index);
	}
}

static void flush_vout(iav_context_t *context, u32 vout_id)
{
	udec_vout_info_t *vout_info = __vout_info(vout_id);

	__render_fb(vout_id, 0, 0, 0);
	while (1) {
		unsigned long flags;
		vout_fb_t A, B, C, D;
		int need_wait;

		dsp_lock(flags);

		A = vout_info->A;
		B = vout_info->B;

		vout_info->A.valid = 0;
		vout_info->B.valid = 0;

		C = vout_info->C;
		D = vout_info->D;

		need_wait = C.valid + D.valid != 0;
		if (need_wait)
			vout_info->num_waiters++;

		dsp_unlock(flags);

		__release_vout_fb(&A);
		__release_vout_fb(&B);

		if (need_wait == 0)
			break;

		down(&vout_info->sem);
	}
}

static void clean_vout(iav_context_t *context, u32 vout_id)
{
	udec_vout_info_t *vout_info = __vout_info(vout_id);
	unsigned long flags;
	vout_fb_t A, B;

	dsp_lock(flags);

	A = vout_info->A;
	B = vout_info->B;

	vout_info->A.valid = 0;
	vout_info->B.valid = 0;

	dsp_unlock(flags);

	__release_vout_fb(&A);
	__release_vout_fb(&B);
}

int iav_config_fb_pool(iav_context_t *context, iav_fbp_config_t __user *arg)
{
	udec_t *udec;
	iav_fbp_config_t fbp_config;
	MEMM_CONFIG_FRM_BUF_POOL_CMD dsp_cmd;
	MEMM_CONFIG_FRM_BUF_POOL_MSG dsp_msg;

	if (copy_from_user(&fbp_config, arg, sizeof(fbp_config)))
		return -EFAULT;

	udec = get_udec(context, fbp_config.decoder_id);
	if (udec == NULL)
		return -EINVAL;

	if (__get_udec_state(udec) != UDEC_STAT_READY) {
		iav_dbg_printk("udec[%d] is not ready (%d)\n", udec->udec_id, __get_udec_state(udec));
		return -EPERM;
	}

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_MEMM_CONFIG_FRM_BUF_POOL;
	dsp_cmd.frm_buf_pool_id = udec->fbp_id;
	dsp_cmd.chroma_format = fbp_config.chroma_format;
	dsp_cmd.tile_mode = __valid_tilemode_value(fbp_config.tiled_mode);

	dsp_cmd.buf_width = fbp_config.buf_width;
	dsp_cmd.buf_height = fbp_config.buf_height;

	dsp_cmd.luma_img_width = fbp_config.lu_width;
	dsp_cmd.luma_img_height = fbp_config.lu_height;

	dsp_cmd.luma_img_row_offset = fbp_config.lu_row_offset;
	dsp_cmd.luma_img_col_offset = fbp_config.lu_col_offset;
	dsp_cmd.chroma_img_row_offset = fbp_config.ch_row_offset;
	dsp_cmd.chroma_img_col_offset = fbp_config.ch_col_offset;

	dsp_mm_config_frm_buf_pool(&dsp_cmd, &dsp_msg);

	fbp_config.num_frm_bufs = dsp_msg.num_of_frm_bufs;

	if (copy_to_user(arg, &fbp_config, sizeof(fbp_config)))
		return -EFAULT;

	return 0;
}

int iav_udec_update_fb_pool_config(iav_context_t *context, iav_fbp_config_t __user *arg)
{
	udec_t *udec;
	iav_fbp_config_t fbp_config;
	MEMM_UPDATE_FRM_BUF_POOL_CONFG_CMD dsp_cmd;
	MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSG dsp_msg;

	if (copy_from_user(&fbp_config, arg, sizeof(fbp_config)))
		return -EFAULT;

	udec = get_udec(context, fbp_config.decoder_id);
	if (udec == NULL)
		return -EINVAL;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_MEMM_UPDATE_FRM_BUF_POOL_CONFG;
	dsp_cmd.frm_buf_pool_id = udec->fbp_id;

	dsp_cmd.buf_width = fbp_config.buf_width;
	dsp_cmd.buf_height = fbp_config.buf_height;
	dsp_cmd.luma_img_width = fbp_config.lu_width;
	dsp_cmd.luma_img_height = fbp_config.lu_height;

	dsp_cmd.luma_img_row_offset = fbp_config.lu_row_offset;
	dsp_cmd.luma_img_col_offset = fbp_config.lu_col_offset;
	dsp_cmd.chroma_img_row_offset = fbp_config.ch_row_offset;
	dsp_cmd.chroma_img_col_offset = fbp_config.ch_col_offset;

	dsp_cmd.update_img_info_only = 1;

	dsp_cmd.chroma_format = fbp_config.chroma_format;
	dsp_cmd.tile_mode = __valid_tilemode_value(fbp_config.tiled_mode);

	dsp_mm_update_frm_buf_pool_config(&dsp_cmd, &dsp_msg);

	if (dsp_msg.error_code != 0)
		return -EINVAL;

	return 0;
}

int iav_decode_dbg(iav_context_t *context, u32 arg)
{
#ifdef CONFIG_IAV_DBG
	switch (arg) {
	case 0:
		G_decode_obj->enable_udec_irq_print = 0;
		break;

	case 1:
		G_decode_obj->enable_udec_irq_print = 1;
		break;

	case 2:
		iav_dbg_printk("enable IAV deinterlacing\n");
		G_decode_obj->disable_deintl = 0;
		break;

	case 3:
		iav_dbg_printk("disable IAV deinterlacing\n");
		G_decode_obj->disable_deintl = 1;
		break;

	case 4:
#ifdef CONFIG_DSP_USE_VIRTUAL_MEMORY
		iav_dbg_printk("disable virtual memory\n");
		G_enable_vm = 0;
#else
		iav_dbg_printk("virtual memory not configured!!\n");
#endif
		break;

	case 5:
#ifdef CONFIG_DSP_USE_VIRTUAL_MEMORY
		iav_dbg_printk("enable virtual memory\n");
		G_enable_vm = 1;
#else
		iav_dbg_printk("virtual memory not configured!!\n");
#endif
		break;

	case 6:
		iav_dbg_printk("disable sync counter\n");
		G_use_syncc = 0;
		break;

	case 7:
		iav_dbg_printk("enable sync counter\n");
		G_use_syncc = 1;
		break;

	case 8:
		iav_dbg_printk("clear cmd msg log\n");
		dsp_clear_cmd_msg_log();
		break;

	default: {
			iav_debug_t __user *__debug = (void*)arg;
			iav_debug_t debug;

			iav_dbg_printk("G_iav_debug_info = 0x%x - 0x%x\n",
				(int)G_iav_debug_info,
				(int)virt_to_phys(G_iav_debug_info));

			if (copy_from_user(&debug, __debug, sizeof(debug)))
				return -EFAULT;

			switch (debug.cmd) {
			case 1:
				debug.value = (int)virt_to_phys(G_iav_debug_info);
				if (copy_to_user(__debug, &debug, sizeof(debug)))
					return -EFAULT;
				return 0;

			default:
				break;
			}
		}
		return -EINVAL;
	}

	return 0;

#else
	return -EPERM;
#endif
}

static int check_udec_state(udec_t *udec)
{
	if (check_udec_mode() < 0)
		return -EAGAIN;

	switch (__get_udec_state(udec)) {
	case UDEC_STAT_INVALID:
	case UDEC_STAT_INIT:
	case UDEC_STAT_IDLE:
		return -EAGAIN;

	case UDEC_STAT_READY:
		if (udec->udec_stopped) {
			iav_dbg_printk("wait_udec: udec is stopped\n");
			return -EAGAIN;
		}
		break;

	case UDEC_STAT_ERROR:
		return -EPERM;

	default:
		break;
	}

	return 0;
}

static int request_frame(iav_context_t *context, iav_frame_buffer_t *frame)
{
	udec_t *udec;
	fb_pool_t *fb_pool;
	iav_frame_buffer_t *pframe;
	int fb_index;
	unsigned long flags;
	MEMM_REQ_FRM_BUF_CMD dsp_cmd;
	MEMM_REQ_FRM_BUF_MSG dsp_msg;

	if ((udec = get_udec(context, frame->decoder_id)) == NULL)
		return -EINVAL;

	dsp_cmd.cmd_code = CMD_MEMM_REQ_FRM_BUF;
	dsp_cmd.frm_buf_pool_id = udec->fbp_id;
	dsp_cmd.pic_struct = frame->pic_struct;

	while (1) {
		int rval;

		if ((rval = check_udec_state(udec)) < 0)
			return rval;

		dsp_mm_request_fb(&dsp_cmd, &dsp_msg);
		if (dsp_msg.error_code == 0) {
			//iav_dbg_printk("\nrequest_fb returns, fb_id=%d\n", dsp_msg.frm_buf_id);
			break;
		}

		if (frame->flags & IAV_FRAME_NO_WAIT) {
			return -EAGAIN;
		}

		if (dsp_wait_irq_interruptible())
			return -EINTR;
	}

	fb_pool = &udec->udec_fb_pool;

	dsp_lock(flags);
	fb_index = fb_pool_alloc(fb_pool, FBS_REQUESTED);
	pframe = __fb_pool_get(fb_pool, fb_index);
	dsp_unlock(flags);

	pframe->flags = 0;
	pframe->decoder_id = frame->decoder_id;
	pframe->fb_id = dsp_msg.frm_buf_id;
	pframe->real_fb_id = dsp_msg.frm_buf_id;
	pframe->lu_buf_addr = (void*)dsp_msg.luma_buf_base_addr;
	pframe->ch_buf_addr = (void*)dsp_msg.chroma_buf_base_addr;
	pframe->buffer_pitch = dsp_msg.buf_pitch;

	*frame = *pframe;

	frame->fb_id = fb_index;
	frame->lu_buf_addr = dsp_to_user(context, (u32)frame->lu_buf_addr);
	frame->ch_buf_addr = dsp_to_user(context, (u32)frame->ch_buf_addr);

	return 0;
}

int iav_request_frame(iav_context_t *context, iav_frame_buffer_t __user *arg)
{
	iav_frame_buffer_t frame;
	int rval;

	if (copy_from_user(&frame, arg, sizeof(frame)))
		return -EFAULT;

	if (context->dsp.user_start == NULL) {
		iav_dbg_printk("DSP memory not mapped\n");
		return -EPERM;
	}

	if ((rval = request_frame(context, &frame)) < 0)
		return rval;

	if (copy_to_user(arg, &frame, sizeof(frame)))
		return -EFAULT;

	return 0;
}

void iav_decode_release(iav_context_t *context)
{
	int i;

	if (context->udec_flags) {
		for (i = 0; i < G_decode_obj->udec_max_number; i++) {
			if (context->udec_flags & (1 << i)) {
				iav_dbg_printk("=== release udec[%d] ===\n", i);
				release_udec(context, __udec(i), 0);
			}
		}
	}

	if (is_mode_flag_set(context, IAV_MODE_UDEC)) {
		iav_leave_decode_mode(context);
	}

	iav_udec_unmap(context);

	if (G_decode_obj->dewarp_table) {
		UDEC_LOG_ALWAYS("kfree dewarp table, %p\n", G_decode_obj->dewarp_table);
		kfree(G_decode_obj->dewarp_table);
	}
	G_decode_obj->dewarp_table = NULL;
	G_decode_obj->dewarp_table_len = 0;

	if (is_mode_flag_set(context, IAV_MODE_UDEC_CAPTURE)) {
		UDEC_DEBUG_ASSERT(G_decode_obj->capture_context_setup);
		if (G_decode_obj->capture_context_setup) {
			__destroy_capture_context(context);
		}
		clear_mode_flag(context, IAV_MODE_UDEC_CAPTURE);
	}

}

static int __wait_decoder(iav_context_t *context, iav_wait_decoder_t *wait)
{
	u32 start_addr_phys = 0;
	u32 room = 0;
	unsigned long flags;
	struct iav_mem_block *bsb = NULL;

	// h.264 flags
	if (wait->flags & IAV_WAIT_BSB) {
		start_addr_phys = bsb_user_to_dsp(context, wait->emptiness.start_addr);
		if ((int)start_addr_phys == -1) {
			iav_dbg_printk("bad start_addr\n");
			return -EINVAL;
		}
		start_addr_phys = DSP_TO_PHYS(start_addr_phys);
	}

	while (1) {
		if (G_iav_obj.dec_state == DEC_STATE_IDLE)
			return -EAGAIN;

		dsp_lock(flags);

		// check EOS
		if (wait->flags & IAV_WAIT_EOS) {
			if (G_decode_obj->eos_flag) {
				wait->flags = IAV_WAIT_EOS;
				break;
			}
		}

		// check bsb emptiness
		if (wait->flags & IAV_WAIT_BSB) {
			if (G_decode_obj->dsp_bits_fifo_next_phys > start_addr_phys) {
				room = G_decode_obj->dsp_bits_fifo_next_phys - start_addr_phys;
			} else {
				iav_get_mem_block(IAV_MMAP_BSB, &bsb);
				room = bsb->size - (start_addr_phys - G_decode_obj->dsp_bits_fifo_next_phys);
			}

			if (room >= wait->emptiness.room) {
				wait->emptiness.room = room;
				wait->emptiness.end_addr = bsb_phys_to_user(context, G_decode_obj->dsp_bits_fifo_next_phys);
				wait->flags = IAV_WAIT_BSB;
				break;
			}
		}

		G_decode_obj->num_waiters++;
		dsp_unlock(flags);

		if (wait_decode_msg(context) < 0)
			return -EINTR;
	}

	dsp_unlock(flags);
	return 0;
}

static int __wait_udec(iav_context_t *context, iav_wait_decoder_t *wait)
{
	u32 start_addr_phys = 0;
	u32 mv_start_addr_phys = 0;
	udec_t *udec;
	unsigned long flags;
	u32 room = 0;

	if ((udec = get_udec(context, wait->decoder_id)) == NULL) {
		UDEC_LOG_ERROR("get_udec fail, dec_id %d\n", wait->decoder_id);
		return -EINVAL;
	}

	if (wait->flags & IAV_WAIT_BITS_FIFO) {
		start_addr_phys = udec_user_to_dsp(&udec->bits_fifo, wait->emptiness.start_addr);
		if ((int)start_addr_phys == -1) {
			UDEC_LOG_ERROR("bad start_addr\n");
			return -EPERM;
		}
		start_addr_phys = DSP_TO_PHYS(start_addr_phys);
	}

	if (wait->flags & IAV_WAIT_MV_FIFO) {
		mv_start_addr_phys = udec_user_to_dsp(&udec->mv_fifo, wait->mv_emptiness.start_addr);
		if ((int)mv_start_addr_phys == -1) {
			UDEC_LOG_ERROR("bad start_addr\n");
			return -EPERM;
		}
		mv_start_addr_phys = DSP_TO_PHYS(mv_start_addr_phys);
	}

	if (wait->flags & IAV_WAIT_VDSP_INTERRUPT) {
		udec->vdsp_ori_seq_number = udec->vdsp_new_seq_number;
	}

	while (1) {
		int rval;

		if ((rval = check_udec_state(udec)) < 0)
			return rval;

		dsp_lock(flags);

		// check OUTPIC
		if (wait->flags & IAV_WAIT_OUTPIC) {
			if (udec->outpic_queue.num_fb > 0 || (udec->udec_status & UDEC_STATUS_EOS)) {
				wait->flags = IAV_WAIT_OUTPIC;
				break;
			}
		}

		// check vout state
		if (wait->flags & IAV_WAIT_VOUT_STATE) {
			int vout_state = __get_vout_state(udec);
			if (vout_state != udec->prev_vout_state) {
				udec->prev_vout_state = vout_state;
				wait->flags = IAV_WAIT_VOUT_STATE;
				wait->vout_state = vout_state;
				break;
			}
		}

		// check EOS - obsolete
		if (wait->flags & IAV_WAIT_UDEC_EOS) {
			if (udec->udec_status & UDEC_STATUS_EOS) {
				wait->flags = IAV_WAIT_UDEC_EOS;
				break;
			}
		}

		// check udec bits fifo
		if (wait->flags & IAV_WAIT_BITS_FIFO) {

			if (udec->dsp_bits_fifo_next_phys > start_addr_phys)
				room = udec->dsp_bits_fifo_next_phys - start_addr_phys;
			else
				room = udec->bits_fifo.size - (start_addr_phys - udec->dsp_bits_fifo_next_phys);

			if (room >= wait->emptiness.room) {
				wait->emptiness.room = room;
				wait->emptiness.end_addr = udec->dsp_bits_fifo_next_phys -
					udec->bits_fifo.phys_start + udec->bits_fifo.user_start;
				wait->flags = IAV_WAIT_BITS_FIFO;
				break;
			}
		}

		// check udec mv fifo
		if (wait->flags & IAV_WAIT_MV_FIFO) {

			if (udec->dsp_mv_fifo_next_phys > mv_start_addr_phys)
				room = udec->dsp_mv_fifo_next_phys - mv_start_addr_phys;
			else
				room = udec->mv_fifo.size - (mv_start_addr_phys - udec->dsp_mv_fifo_next_phys);

//			if (0 == wait->decoder_id) {
//				printk("udec 0, wait BSB, room %u, request room %u, udec last addr %x, start addr %x.\n", room, wait->emptiness.room, udec->dsp_bits_fifo_next_phys, start_addr_phys);
//			}

			if (room >= wait->mv_emptiness.room) {
				wait->mv_emptiness.room = room;
				wait->mv_emptiness.end_addr = udec->dsp_mv_fifo_next_phys -
					udec->mv_fifo.phys_start + udec->mv_fifo.user_start;
				wait->flags = IAV_WAIT_MV_FIFO;

//				if (0 == wait->decoder_id) {
//					printk("udec 0, wait BSB done.\n");
//				}

				break;
			}
		}

		// check for error
		if (wait->flags & IAV_WAIT_UDEC_ERROR) {
			if (udec->error_code != 0) {
				wait->flags = IAV_WAIT_UDEC_ERROR;
				break;
			}
		}

		// check for new comes interrupt
		if (wait->flags & IAV_WAIT_VDSP_INTERRUPT) {
			if (udec->vdsp_new_seq_number!= udec->vdsp_ori_seq_number) {
				udec->vdsp_ori_seq_number = udec->vdsp_new_seq_number;
				wait->flags = IAV_WAIT_VDSP_INTERRUPT;
				break;
			}
		}

		udec->num_waiters++;
		dsp_unlock(flags);

		if (wait_udec_msg(context, udec) < 0)
			return -EINTR;
	}

	dsp_unlock(flags);
	return 0;
}

int iav_wait_udec_status(iav_context_t *context, iav_udec_status_t __user *arg)
{
	iav_udec_status_t status;
	udec_t *udec;
	unsigned long flags;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&status, arg, sizeof(status)))
		return -EFAULT;

	if ((udec = get_udec(context, status.decoder_id)) == NULL)
		return -EINVAL;

	while (1) {
		int rval;

		if ((rval = check_udec_state(udec)) < 0)
			return rval;

		dsp_lock(flags);
		status.udec_state  = __get_udec_state(udec);
		status.vout_state  = __get_vout_state(udec);
		if (status.only_query_current_pts) {
			if (udec->pts_comes) {
				status.eos_flag = 0;
				status.clock_counter = udec->disp_status.latest_clock_counter;
				status.pts_low = udec->disp_status.latest_pts_low;
				status.pts_high = udec->disp_status.latest_pts_high;
			} else {
				status.only_query_current_pts = 0;
				status.pts_low = udec->disp_status.latest_pts_low;
				status.pts_high = udec->disp_status.latest_pts_high;
			}
			status.pts_valid = udec->pts_comes;
			dsp_unlock(flags);
			break;
		}

		if (udec->udec_status) {
			if (udec->udec_status & UDEC_STATUS_EOS) {
				status.eos_flag = 1;
				status.clock_counter = -1;
				status.pts_low = udec->eos_pts_low;
				status.pts_high = udec->eos_pts_high;
				udec->udec_status &= ~UDEC_STATUS_EOS;
				dsp_unlock(flags);
				break;
			}
			if (udec->udec_status & UDEC_STATUS_DISP) {
				status.eos_flag = 0;
				status.clock_counter = udec->disp_status.latest_clock_counter;
				status.pts_low = udec->disp_status.latest_pts_low;
				status.pts_high = udec->disp_status.latest_pts_high;
				status.pts_valid = udec->pts_comes;
				udec->udec_status &= ~UDEC_STATUS_DISP;
				dsp_unlock(flags);
				break;
			}
		}

		if (status.nowait) {
			dsp_unlock(flags);
			return -EAGAIN;
		}

		udec->num_waiters++;
		dsp_unlock(flags);

		if (wait_udec_msg(context, udec) < 0)
			return -EINTR;
	}

	if (copy_to_user(arg, &status, sizeof(status)))
		return -EFAULT;

	return 0;
}

int iav_get_udec_state(iav_context_t *context, iav_udec_state_t __user *arg)
{
	iav_udec_state_t state;
	udec_t *udec;
	unsigned long flags;

	if (copy_from_user(&state, arg, sizeof(state)))
		return -EFAULT;

	if ((udec = get_udec(context, state.decoder_id)) == NULL)
		return -EINVAL;

	dsp_lock(flags);
	state.udec_state = __get_udec_state(udec);
	state.vout_state = __get_vout_state(udec);
	state.error_code = udec->error_code;
	state.error_pic_pts_low = udec->error_pic_pts_low;
	state.error_pic_pts_high = udec->error_pic_pts_high;
	if (state.flags & IAV_UDEC_STATE_CLEAR_ERROR) {
		udec->error_code = 0;
	}
	if (state.flags & IAV_UDEC_STATE_DSP_READ_POINTER) {
		state.dsp_current_read_bitsfifo_addr = udec->dsp_bits_fifo_next_phys -
			udec->bits_fifo.phys_start + udec->bits_fifo.user_start;
		state.dsp_current_read_mvfifo_addr = udec->dsp_mv_fifo_next_phys -
			udec->mv_fifo.phys_start + udec->mv_fifo.user_start;

		state.dsp_current_read_bitsfifo_addr_phys = (u8*)udec->dsp_bits_fifo_next_phys;
	}

	if (state.flags & IAV_UDEC_STATE_ARM_WRITE_POINTER) {
		state.arm_last_write_bitsfifo_addr = udec->dsp_bits_fifo_last_write_phys -
			udec->bits_fifo.phys_start + udec->bits_fifo.user_start;

		state.arm_last_write_bitsfifo_addr_phys = (u8*)udec->dsp_bits_fifo_last_write_phys;
	}

	if (state.flags & IAV_UDEC_STATE_BTIS_FIFO_ROOM) {
		u32 dsp_last_read = udec->dsp_bits_fifo_next_phys;
		u32 arm_last_write = udec->dsp_bits_fifo_last_write_phys;

		state.bits_fifo_total_size = udec->bits_fifo.size;
		if (dsp_last_read > arm_last_write) {
			state.bits_fifo_free_size = dsp_last_read - arm_last_write;
		} else {
			state.bits_fifo_free_size = state.bits_fifo_total_size - (arm_last_write - dsp_last_read);
		}

		state.bits_fifo_phys_start = (u32)udec->bits_fifo.phys_start;
		state.tot_decode_cmd_cnt = g_decode_cmd_cnt[udec->udec_id];
		state.tot_decode_frame_cnt = udec->decode_frame_count;
	}
	dsp_unlock(flags);

	if (copy_to_user(arg, &state, sizeof(state)))
		return -EFAULT;

	return 0;
}

static void cmd_set_audio_clk_offset(iav_audio_clk_offset_t *clk_offset)
{
	POSTP_SET_AUDIO_CLK_OFFSET_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTP_SET_AUDIO_CLK_OFFSET;
	dsp_cmd.decoder_id = clk_offset->decoder_id;
	dsp_cmd.audio_clk_offset = clk_offset->audio_clk_offset;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

int iav_set_audio_clk_offset(iav_context_t *context, iav_audio_clk_offset_t __user *arg)
{
	iav_audio_clk_offset_t clk_offset;
	udec_t *udec;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&clk_offset, arg, sizeof(clk_offset)))
		return -EFAULT;

	if ((udec = get_udec(context, clk_offset.decoder_id)) == NULL)
		return -EINVAL;

	if (__get_udec_state(udec) != UDEC_STAT_RUN) {
		iav_dbg_printk("udec %d state is not UDEC_STAT_RUN\n", udec->udec_id);
		return -EPERM;
	}

	cmd_set_audio_clk_offset(&clk_offset);

	return 0;
}

static void cmd_wake_vout(iav_wake_vout_t *wake_vout)
{
	POSTP_WAKE_VOUT_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTP_WAKE_VOUT;
	dsp_cmd.decoder_id = wake_vout->decoder_id;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

int iav_wake_vout(iav_context_t *context, iav_wake_vout_t __user*arg)
{
	iav_wake_vout_t wake_vout;
	udec_t *udec;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&wake_vout, arg, sizeof(wake_vout)))
		return -EFAULT;

	if ((udec = get_udec(context, wake_vout.decoder_id)) == NULL)
		return -EINVAL;

	if (__get_udec_state(udec) != UDEC_STAT_RUN) {
		iav_dbg_printk("udec %d state is not UDEC_STAT_RUN\n", udec->udec_id);
		return -EPERM;
	}

	if (__get_vout_state(udec) != IAV_VOUT_STATE_DORMANT) {
		iav_dbg_printk("vout state is %d\n", __get_vout_state(udec));
		return -EPERM;
	}

	cmd_wake_vout(&wake_vout);
	__wait_udec_vout_state(udec, IAV_VOUT_STATE_RUN, UDEC_STAT_RUN, "wait vout waken");

	if (__get_vout_state(udec) != IAV_VOUT_STATE_RUN) {
		LOG_ERROR("wake vout fail, vout state is %d, udec state %d.\n", __get_vout_state(udec), __get_udec_state(udec));
		return -EPERM;
	}
	return 0;
}

static void cmd_udec_trickplay(iav_udec_trickplay_t *trickplay)
{
	UDEC_TRICKPLAY_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_TRICKPLAY;
	dsp_cmd.decoder_id = trickplay->decoder_id;
	dsp_cmd.mode = trickplay->mode;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	//dsp_issue_cmd_sync(&dsp_cmd, sizeof(dsp_cmd));
}

int iav_udec_trickplay(iav_context_t *context, iav_udec_trickplay_t __user *arg)
{
	iav_udec_trickplay_t trickplay;
	udec_t *udec;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&trickplay, arg, sizeof(trickplay)))
		return -EFAULT;

	if ((udec = get_udec(context, trickplay.decoder_id)) == NULL)
		return -EINVAL;

	switch (trickplay.mode) {
	case 0:	// PAUSE
		if (__get_vout_state(udec) != IAV_VOUT_STATE_RUN) {
			iav_dbg_printk("Pause: not in run state\n");
			return -EPERM;
		}
		break;

	case 1:	// RESUME
		if (__get_vout_state(udec) != IAV_VOUT_STATE_PAUSE) {
			iav_dbg_printk("Resume: not in pause state\n");
			return -EPERM;
		}
		break;

	case 2:	// STEP
		break;

	default:
		iav_dbg_printk("Unsupported trick mode: %d\n", trickplay.mode);
		return -EINVAL;
	}

	cmd_udec_trickplay(&trickplay);

	return 0;
}

static void cmd_udec_pb_speed(iav_udec_pb_speed_t *speed)
{
	UDEC_PLAYBACK_SPEED_CMD_t dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_UDEC_PLAYBACK_SPEED;
	dsp_cmd.speed = speed->speed;
	dsp_cmd.decoder_id = speed->decoder_id;
	//dsp_cmd.scan_mode = speed->scan_mode;
	dsp_cmd.direction = speed->direction;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

int iav_udec_pb_speed(iav_context_t *context, iav_udec_pb_speed_t __user *arg)
{
	iav_udec_pb_speed_t speed;
	udec_t *udec;

	if (check_udec_mode() < 0)
		return -EPERM;

	if (copy_from_user(&speed, arg, sizeof(speed)))
		return -EFAULT;

	if ((udec = get_udec(context, speed.decoder_id)) == NULL)
		return -EINVAL;

	//debug infos
	UDEC_LOG_ALWAYS("udec pb speed(udec state %d, vout state %d): speed 0x%04x, decoder_id %d, scan_mode %d, direction %d\n", __get_udec_state(udec), __get_vout_state(udec), speed.speed, speed.decoder_id, speed.scan_mode, speed.direction);

	cmd_udec_pb_speed(&speed);

	return 0;
}

#define SYNC_COUNTER_ADDR	0xe8107c38

int iav_udec_map(iav_context_t *context, iav_udec_map_t __user *arg)
{
	iav_udec_map_t map;

	if (context->udec_mapping) {
		iav_dbg_printk("already mapped\n");
		return -EPERM;
	}

	context->udec_mapping = iav_create_mmap(context,
		SYNC_COUNTER_ADDR & PAGE_MASK, ambarella_phys_to_virt(SYNC_COUNTER_ADDR & PAGE_MASK), PAGE_SIZE, PROT_READ, 1, NULL);
	if (context->udec_mapping == NULL) {
		iav_dbg_printk("iav_udec_map failed\n");
		return -ENOMEM;
	}

	map.rv_sync_counter = (u32*)((u8*)context->udec_mapping + (SYNC_COUNTER_ADDR & ~PAGE_MASK));

	if (copy_to_user(arg, &map, sizeof(map)))
		return -EFAULT;

	return 0;
}

int iav_udec_unmap(iav_context_t *context)
{
	if (context->udec_mapping) {
		iav_destroy_mmap(context, (unsigned long)context->udec_mapping, PAGE_SIZE);
		context->udec_mapping = NULL;
	}
	return 0;
}

static int wait_decoder(iav_context_t *context, iav_wait_decoder_t *wait)
{
	if (wait->flags & (IAV_WAIT_BSB | IAV_WAIT_FRAME | IAV_WAIT_EOS | IAV_WAIT_BUFFER))
		return __wait_decoder(context, wait);

	if (wait->flags & (IAV_WAIT_BITS_FIFO | IAV_WAIT_MV_FIFO | IAV_WAIT_OUTPIC | IAV_WAIT_UDEC_EOS | IAV_WAIT_UDEC_ERROR | IAV_WAIT_VOUT_STATE | IAV_WAIT_VDSP_INTERRUPT))
		return __wait_udec(context, wait);

	UDEC_LOG_ERROR("Invalid flag 0x%x\n", wait->flags);
	return -EINVAL;
}

int iav_get_decoded_frame(iav_context_t *context, iav_frame_buffer_t __user *arg)
{
	iav_frame_buffer_t frame;
	udec_t *udec;
	unsigned long flags;
	int frame_flags;
	int fb_index;
	int i;

	if (copy_from_user(&frame, arg, sizeof(frame)))
		return -EFAULT;

	frame_flags = frame.flags;

	if ((udec = get_udec(context, frame.decoder_id)) == NULL)
		return -EINVAL;

	while (1) {
		fb_queue_t *fb_queue;
		int rval;

		// check if state is changed
		if ((rval = check_udec_state(udec)) < 0)
			return rval;

		// release frame buffers in VOUT
		for (i = 0; i < IAV_NUM_VOUT; i++)
			if (context->vout_flag[i])
				clean_vout(context, i);

		dsp_lock(flags);

		// check OUTPUT queue
		fb_queue = &udec->outpic_queue;
		if (fb_queue->num_fb > 0) {
			fb_pool_t *fb_pool;

			fb_index = __fb_queue_pop(fb_queue);
			fb_pool = get_fb_pool(udec);
			frame = *__fb_pool_get(fb_pool, fb_index);

			dsp_unlock(flags);
			break;
		}

		// check EOS flag
		if (udec->udec_status & UDEC_STATUS_EOS) {
			frame.fb_id = IAV_INVALID_FB_ID;
			frame.eos_flag = 1;
			frame.pts = udec->eos_pts_low;
			frame.pts_high = udec->eos_pts_high;
			udec->udec_status &= ~UDEC_STATUS_EOS;

			dsp_unlock(flags);
			goto out;
		}

		if (frame_flags & IAV_FRAME_NO_WAIT) {
			dsp_unlock(flags);
			return -EAGAIN;
		}

		udec->num_waiters++;
		dsp_unlock(flags);

		if (wait_udec_msg(context, udec) < 0)
			return -EINTR;
	}

	if (frame.fb_id == IAV_INVALID_FB_ID) {
		goto out;
	}

	// mangle
	frame.fb_id = fb_index;

	if (udec->udec_type == UDEC_JPEG) {

		iav_dbg_printk("got jpeg frame buffer\n");

		if (frame.chroma_format != 1 && frame.chroma_format != 2) {
			// todo
			frame.lu_buf_addr = NULL;
			frame.ch_buf_addr = NULL;
		} else {
			int size;

			destroy_mmap(context, &udec->jpeg_y);
			destroy_mmap(context, &udec->jpeg_uv);

			size = frame.buffer_pitch * frame.buffer_height;
			frame.lu_buf_addr = map_udec_fifo(context, "jpeg y",
				(u32)frame.lu_buf_addr, size, &udec->jpeg_y, udec->noncachable_buffer);

			if (frame.chroma_format == 1)
				size /= 2;
			frame.ch_buf_addr = map_udec_fifo(context, "jpeg uv",
				(u32)frame.ch_buf_addr, size, &udec->jpeg_uv, udec->noncachable_buffer);
		}
	} else {
		if (context->dsp.user_start) {

			if ((frame_flags & IAV_FRAME_PADDR) == 0) {
				frame.lu_buf_addr = dsp_to_user(context, (u32)frame.lu_buf_addr);
				frame.ch_buf_addr = dsp_to_user(context, (u32)frame.ch_buf_addr);
			}

			if (frame_flags & IAV_FRAME_NEED_SYNC) {
				invalidate_fb_cache(context, udec, frame.fb_id);
			}

		} else {
			frame.lu_buf_addr = NULL;
			frame.ch_buf_addr = NULL;
		}
	}

out:
	if (copy_to_user(arg, &frame, sizeof(frame)))
		return -EFAULT;

	return 0;
}

static void __render_fb(int vout_id, int decoder_id, int fb_index, int valid)
{
	udec_vout_info_t *vout_info = __vout_info(vout_id);
	unsigned long flags;
	vout_fb_t A;
	vout_fb_t B;
	vout_fb_t E;

	dsp_lock(flags);

	A = vout_info->A;
	vout_info->A.valid = 0;

	B = vout_info->B;
	vout_info->B.valid = 0;

	E = vout_info->E;

	vout_info->E.decoder_id = decoder_id;
	vout_info->E.fb_index = fb_index;
	vout_info->E.valid = valid;
	vout_info->E.render = 1;
	vout_info->E.first_field_rendered = 0;

	dsp_unlock(flags);

	if (E.valid) {
		vout_info->frame_not_rendered++;
		//iav_dbg_printk("not rendered %d\n", vout_info->frame_not_rendered++);
	}

	__release_vout_fb(&A);
	__release_vout_fb(&B);
	__release_vout_fb(&E);
}

// release_only:
//	0 - render and then release
//	1 - release only
static int render_or_release_frame(iav_context_t *context, iav_frame_buffer_t *frame, int release_only)
{
	udec_t *udec;
	fb_pool_t *fb_pool;
	int fb_index;
	int fb_state;

	if ((udec = get_udec(context, frame->decoder_id)) == NULL)
		return -EINVAL;

	fb_index = frame->fb_id;
	if (fb_index >= MAX_DECODED_FRAMES) {
		iav_dbg_printk("bad fb_index: %d\n", fb_index);
		return -EINVAL;
	}

	fb_pool = get_fb_pool(udec);
	fb_state = __get_fb_state(fb_pool, fb_index);

	if (release_only) {
		if (udec->udec_type == UDEC_JPEG) {
			iav_dbg_printk("release_frame for jpeg is not permitted\n");
			return -EPERM;
		}

		if (fb_state != FBS_REQUESTED && fb_state != FBS_OUTPIC) {
			if (fb_state == FBS_INVALID) {
				iav_dbg_printk("release_frame: bad fb (fb_index = %d)\n", fb_index);
				return -EINVAL;
			}
			if (fb_state == FBS_VOUT) {
				iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb_index);
				if (pframe->flags & IAV_FRAME_NO_RELEASE)
					pframe->flags &= ~IAV_FRAME_NO_RELEASE;
				else {
					iav_dbg_printk("release_frame: bad fb (fb_index = %d)\n", fb_index);
					return -EPERM;
				}
				return 0;
			}
		}

		__release_fb(frame->decoder_id, fb_index);

	} else {
		int vout_id;

		if (fb_state != FBS_REQUESTED && fb_state != FBS_OUTPIC) {
			iav_dbg_printk("render_frame: bad fb (fb_index = %d)\n", fb_index);
			return -EINVAL;
		}

		vout_id = get_udec_vout_id(udec);
		if (vout_id < 0) {
			iav_dbg_printk("no vout for udec %d\n", udec->udec_id);
			return -EPERM;
		}

		if (frame->flags & IAV_FRAME_NEED_SYNC)
			clean_fb_cache(context, udec, fb_index);

		if (frame->flags & IAV_FRAME_SYNC_VOUT) {
			udec_vout_info_t *vout_info = __vout_info(vout_id);
			while (1) {
				unsigned long flags;
				dsp_lock(flags);
				if (vout_info->E.valid == 0) {
					dsp_unlock(flags);
					break;
				}
				vout_info->num_waiters++;
				dsp_unlock(flags);
				down(&vout_info->sem);
			}
		}

		__set_fb_state(fb_pool, fb_index, FBS_VOUT);

		if (frame->flags & IAV_FRAME_NO_RELEASE) {
			iav_frame_buffer_t *pframe = __fb_pool_get(fb_pool, fb_index);
			pframe->flags |= IAV_FRAME_NO_RELEASE;
		}

		__render_fb(vout_id, frame->decoder_id, fb_index, 1);
	}

	return 0;
}

static void cmd_postp_inpic(udec_t *udec, iav_frame_buffer_t *pframe, iav_frame_buffer_t *frame)
{
	POSTP_INPIC_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTP_INPIC;
	dsp_cmd.decoder_id = udec->udec_id;
	dsp_cmd.progressive = !frame->interlaced;
	dsp_cmd.frm_buf_id = pframe->real_fb_id;
	dsp_cmd.pts_low = frame->pts;
	dsp_cmd.pts_high = frame->pts_high;

	dsp_issue_cmd_sync_ex(&dsp_cmd, sizeof(dsp_cmd));
}

static int postp_frame(iav_context_t *context, iav_frame_buffer_t *frame)
{
	iav_frame_buffer_t *pframe;
	udec_t *udec;
	fb_pool_t *fb_pool;
	int fb_index;
	int fb_state;

	if ((udec = get_udec(context, frame->decoder_id)) == NULL)
		return -EINVAL;

	if (udec->udec_type != UDEC_SW && udec->udec_type != UDEC_RV40) {
		iav_dbg_printk("bad udec type: %d\n", udec->udec_type);
		return -EPERM;
	}

	if (__get_udec_state(udec) != UDEC_STAT_RUN) {
		iav_dbg_printk("bad udec[%d] state: %d\n", udec->udec_id, __get_udec_state(udec));
		return -EPERM;
	}

	fb_index = frame->fb_id;
	if (fb_index >= MAX_DECODED_FRAMES) {
		iav_dbg_printk("bad fb_index: %d\n", fb_index);
		return -EINVAL;
	}

	fb_pool = get_fb_pool(udec);
	fb_state = __get_fb_state(fb_pool, fb_index);

	if (fb_state != FBS_REQUESTED && fb_state != FBS_OUTPIC) {
		iav_dbg_printk("postp_frame: bad fb (fb_index = %d)\n", fb_index);
		return -EINVAL;
	}

	if (frame->flags & IAV_FRAME_NEED_SYNC)
		clean_fb_cache(context, udec, fb_index);

	dsp_start_sync_cmd();

	if (frame->flags & IAV_FRAME_NO_RELEASE)
		dsp_mm_register_fb(fb_pool->fb[fb_index].real_fb_id);

	pframe = fb_pool->fb + fb_index;
	cmd_postp_inpic(udec, pframe, frame);

	if ((frame->flags & IAV_FRAME_NO_RELEASE) == 0)
		__fb_pool_remove(fb_pool, fb_index);

	dsp_end_sync_cmd();

	return 0;
}

int iav_render_frame(iav_context_t *context, iav_frame_buffer_t __user * arg, int release_only)
{
	iav_frame_buffer_t frame;
	int rval;

	if (copy_from_user(&frame, arg, sizeof(frame)))
		return -EFAULT;

	if ((rval = render_or_release_frame(context, &frame, release_only)) < 0)
		return rval;

	return 0;
}

int iav_postp_frame(iav_context_t *context, iav_frame_buffer_t __user *arg)
{
	iav_frame_buffer_t frame;
	int rval;

	if (copy_from_user(&frame, arg, sizeof(frame)))
		return -EFAULT;

	if ((rval = postp_frame(context, &frame)) < 0)
		return rval;

	return 0;
}

int iav_decode_wait(iav_context_t *context, iav_wait_decoder_t __user *arg)
{
	iav_wait_decoder_t wait;
	int rval;

	if (G_iav_info.state != IAV_STATE_DECODING)
		return -EINVAL;

	if (copy_from_user(&wait, arg, sizeof(wait)))
		return -EFAULT;

	if ((rval = wait_decoder(context, &wait)) < 0)
		return rval;

	return copy_to_user(arg, &wait, sizeof(wait)) ? -EFAULT : 0;
}

static int prepare_postp_param_for_enable_vout(vout_config_sink_t *config_sink)
{
	u32 vout_id = config_sink->vout_id;
	u32 udec_id;
	int i;

	g_vout_action.action = 0;
	g_vout_action.enable_video = config_sink->pcfg->video_en;

	if (vout_id >= IAV_NUM_VOUT)
		return 0;

	for (i = 0; i < IAV_NUM_VOUT; i++) {
		// find another vout (and the decoder using it)...
		if (G_decode_obj->vout_config_valid[i] && i != vout_id)
			break;
	}
	if (i >= IAV_NUM_VOUT) {
		iav_dbg_printk("Never sent postp\n");
		return 0;
	}

	if ((udec_id = __vout_owner(i)) >= G_decode_obj->udec_max_number) {
		iav_dbg_printk("No decoder?\n");
		return 0;
	}

	// now we have a udec and another vout being used...

	g_vout_action.udec_id = udec_id;
	g_vout_action.vout_id = vout_id;
	g_vout_action.other_vout_id = i;
	g_vout_action.action = 1;

	return 0;
}

static int decode_vout_preproc(iav_context_t *context, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case VOUT_HALT_BEGIN: {
			struct iav_vout_enable_video_s vout_enable_video;
			iav_udec_vout_config_t *vout_config;
			u32 vout_id = arg;
			u32 udec_id;
			udec_t *udec;
			int rval;

			if (vout_id >= IAV_NUM_VOUT) {
				iav_dbg_printk("bad vout_id %d\n", vout_id);
				return -EINVAL;
			}

			if ((udec_id = __vout_owner(vout_id)) >= G_decode_obj->udec_max_number)
				return 0;

			udec = __udec(udec_id);

			vout_enable_video.vout_id = vout_id;
			vout_enable_video.video_en = 0;
			if ((rval = __iav_enable_vout_video(context, &vout_enable_video)) < 0)
				return rval;

			vout_config = __vout_config(vout_id);
			vout_config->disable = 1;

			cmd_udec_postproc(udec_id, udec->udec_type);
			iav_dbg_printk("halt vout\n");

			return 0;
		}
		break;

	case VOUT_CONFIG_SINK_BEGIN: {
			// user is trying to configure vout
			// it needs a POSTP cmd.
			// It is really ugly here.

			vout_config_sink_t *config_sink = (void*)arg;

			if (prepare_postp_param_for_enable_vout(config_sink) < 0)
				return -EAGAIN;		// let it fail

			if (g_vout_action.action == 1) {
				dsp_start_cmdblk(DSP_CMD_PORT_GEN);
			}

			return 0;
		}
		break;

	case VOUT_CONFIG_SINK_END:
		if (g_vout_action.action == 1) {
			vout_config_sink_t *config_sink = (void*)arg;
			u32 udec_id = g_vout_action.udec_id;
			//u32 old_udec_id;
			//iav_udec_vout_config_t old_vout_config;
			iav_udec_vout_config_t *vout_config;

			if (g_vout_action.enable_video) {
				// save
				//old_udec_id = __vout_owner(g_vout_action.vout_id);
				__set_vout_owner(g_vout_action.vout_id, udec_id);

				vout_config = __vout_config(g_vout_action.vout_id);
				//old_vout_config = *vout_config;

				// now, we're ready
				vout_config->vout_id = g_vout_action.vout_id;
				vout_config->disable = 0;	// enable it
				vout_config->flip = 0;		// no flip
				vout_config->rotate = 0;	// no rotation

				vout_config->target_win_offset_x = 0;
				vout_config->target_win_offset_y = 0;
				vout_config->target_win_width = config_sink->pcfg->video_size.vout_width;
				vout_config->target_win_height = config_sink->pcfg->video_size.vout_height;

				vout_config->win_offset_x = 0;
				vout_config->win_offset_y = 0;
				vout_config->win_width = config_sink->pcfg->video_size.vout_width;
				vout_config->win_height = config_sink->pcfg->video_size.vout_height;

				vout_config->zoom_factor_x = 1;
				vout_config->zoom_factor_y = 1;

				// do it !
				cmd_udec_postproc(udec_id, __udec(udec_id)->udec_type);
				iav_set_vout_src(context, g_vout_action.vout_id, VOUT_SRC_UDEC);
				//iav_change_vout_src_ex(context, (1 << g_vout_action.vout_id), VOUT_SRC_UDEC);

				// restore
				///*vout_config = old_vout_config;
				///__set_vout_owner(g_vout_action.vout_id, old_udec_id);
			}

			dsp_end_cmdblk(DSP_CMD_PORT_GEN);

			g_vout_action.action   = 0;
		}
		break;

	default:
		break;
	}

	return 0;
}

#ifdef ENABLE_MULTI_WINDOW_UDEC

static void cmd_udec_postp_global_config(void)
{
	POSTPROC_MW_GLOBAL_CONFIG_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTPROC_MW_GLOBAL_CONFIG;
	dsp_cmd.total_num_win_configs = G_decode_obj->mdec_mode_config.total_num_win_configs;
	dsp_cmd.out_to_hdmi = G_decode_obj->mdec_mode_config.out_to_hdmi;
	dsp_cmd.av_sync_enabled = G_decode_obj->mdec_mode_config.av_sync_enabled;
	dsp_cmd.audio_on_win_id = G_decode_obj->mdec_mode_config.audio_on_win_id;
	dsp_cmd.pre_buffer_len = G_decode_obj->mdec_mode_config.pre_buffer_len;
	dsp_cmd.enable_buffering_ctrl = G_decode_obj->mdec_mode_config.enable_buffering_ctrl;
	dsp_cmd.video_win_width = G_decode_obj->mdec_mode_config.video_win_width;
	dsp_cmd.video_win_height = G_decode_obj->mdec_mode_config.video_win_height;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_udec_postp_render_config(udec_render_t *render, u8 tot_num, u8 num_configs)
{
	u32 i;
	u32 remaining_number = num_configs;
	POSTPROC_MW_RENDER_CONFIG_CMD dsp_cmd;

	if (remaining_number > DMAX_RENDER_CONFIG_PER_CMD) {

		while (remaining_number) {

			memset(&dsp_cmd, 0, sizeof(dsp_cmd));
			dsp_cmd.cmd_code = CMD_POSTPROC_MW_RENDER_CONFIG;
			dsp_cmd.total_num_win_to_render = tot_num;
			dsp_cmd.num_config = num_configs;

			if (remaining_number > DMAX_RENDER_CONFIG_PER_CMD) {
				remaining_number -= DMAX_RENDER_CONFIG_PER_CMD;
				dsp_cmd.num_config = DMAX_RENDER_CONFIG_PER_CMD;
			} else {
				dsp_cmd.num_config = remaining_number;
				remaining_number = 0;
			}

			for (i= 0; i < dsp_cmd.num_config; i++, render ++) {
				dsp_cmd.configs[i].render_id = render->render_id;
				dsp_cmd.configs[i].win_config_id = render->win_config_id;
				dsp_cmd.configs[i].win_config_id_2nd = render->win_config_id_2nd;
				dsp_cmd.configs[i].stream_id = render->udec_id;
				//dsp_cmd.configs[i].first_pts_low = render->first_pts_low;
				//dsp_cmd.configs[i].first_pts_high = render->first_pts_high;
				dsp_cmd.configs[i].input_source_type = render->input_source_type;
			}
			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
		}
	} else {
		memset(&dsp_cmd, 0, sizeof(dsp_cmd));

		dsp_cmd.cmd_code = CMD_POSTPROC_MW_RENDER_CONFIG;
		dsp_cmd.total_num_win_to_render = tot_num;
		dsp_cmd.num_config = num_configs;

		for (i= 0; i < dsp_cmd.num_config; i++, render ++) {
			dsp_cmd.configs[i].render_id = render->render_id;
			dsp_cmd.configs[i].win_config_id = render->win_config_id;
			dsp_cmd.configs[i].win_config_id_2nd = render->win_config_id_2nd;
			dsp_cmd.configs[i].stream_id = render->udec_id;
			//dsp_cmd.configs[i].first_pts_low = render->first_pts_low;
			//dsp_cmd.configs[i].first_pts_high = render->first_pts_high;
			dsp_cmd.configs[i].input_source_type = render->input_source_type;
		}

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

}

static void cmd_udec_postp_window_config(udec_window_t *window, u32 number)
{
	u32 i;
	u32 remaining_number = number;
	POSTPROC_MW_WIN_CONFIG_CMD dsp_cmd;

	if (remaining_number > DMAX_WINDOW_CONFIG_PER_CMD) {

		while (remaining_number) {

			memset(&dsp_cmd, 0, sizeof(dsp_cmd));
			dsp_cmd.cmd_code = CMD_POSTPROC_MW_WIN_CONFIG;

			if (remaining_number > DMAX_WINDOW_CONFIG_PER_CMD) {
				remaining_number -= DMAX_WINDOW_CONFIG_PER_CMD;
				dsp_cmd.num_config = DMAX_WINDOW_CONFIG_PER_CMD;
			} else {
				dsp_cmd.num_config = remaining_number;
				remaining_number = 0;
			}

			for (i= 0; i < dsp_cmd.num_config; i++, window ++) {
				dsp_cmd.configs[i].win_config_id = window->win_config_id;
				dsp_cmd.configs[i].input_offset_x = window->input_offset_x;
				dsp_cmd.configs[i].input_offset_y = window->input_offset_y;
				dsp_cmd.configs[i].input_width = window->input_width;
				dsp_cmd.configs[i].input_height = window->input_height;
				// Patch: make target_win_offset_x even temporally
				// Todo:  remove patch after ucode fixes it
				dsp_cmd.configs[i].target_win_offset_x = window->target_win_offset_x&(~(u16)0x1);
				dsp_cmd.configs[i].target_win_offset_y = window->target_win_offset_y;
				dsp_cmd.configs[i].target_win_width = window->target_win_width;
				dsp_cmd.configs[i].target_win_height = window->target_win_height;
			}

			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
		}
	} else {
		memset(&dsp_cmd, 0, sizeof(dsp_cmd));
		dsp_cmd.cmd_code = CMD_POSTPROC_MW_WIN_CONFIG;
		dsp_cmd.num_config = number;

		for (i= 0; i < number; i++, window ++) {
			dsp_cmd.configs[i].win_config_id = window->win_config_id;
			dsp_cmd.configs[i].input_offset_x = window->input_offset_x;
			dsp_cmd.configs[i].input_offset_y = window->input_offset_y;
			dsp_cmd.configs[i].input_width = window->input_width;
			dsp_cmd.configs[i].input_height = window->input_height;
			dsp_cmd.configs[i].target_win_offset_x = window->target_win_offset_x&(~(u16)0x1);
			dsp_cmd.configs[i].target_win_offset_y = window->target_win_offset_y;
			dsp_cmd.configs[i].target_win_width = window->target_win_width;
			dsp_cmd.configs[i].target_win_height = window->target_win_height;
		}

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}
}

static void cmd_udec_stream_switch(iav_postp_stream_switch_t *s_switch)
{
	u32 i;
	unsigned long flags;
	SwitchConfig_t* pconfig;
	POSTPROC_MW_STREAM_SWITCH_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_POSTPROC_MW_STREAM_SWITCH;
	//dsp_cmd.num_config = s_switch->num_config;

	//save check
	UDEC_DEBUG_ASSERT(dsp_cmd.num_config <= DMAX_SWITCH_CONFIG_PER_CMD);
	if (dsp_cmd.num_config > DMAX_SWITCH_CONFIG_PER_CMD) {
		dsp_cmd.num_config = DMAX_SWITCH_CONFIG_PER_CMD;
	}

	pconfig = dsp_cmd.configs;
	for (i= 0; i < s_switch->num_config; i++) {
		if ((s_switch->switch_config[i].render_id < MAX_NUM_WINDOWS) && (s_switch->switch_config[i].new_udec_id < MAX_NUM_WINDOWS)) {
			dsp_cmd.configs[i].render_id = s_switch->switch_config[i].render_id;
			dsp_cmd.configs[i].new_stream_id = s_switch->switch_config[i].new_udec_id;
			dsp_cmd.configs[i].seamless = s_switch->switch_config[i].seamless;
			dsp_lock(flags);
			G_decode_obj->last_switch_status[dsp_cmd.configs[i].render_id] = INITIAL_SWITCH_STATUS;
			dsp_unlock(flags);
			pconfig++;
			dsp_cmd.num_config ++ ;
		} else {
			UDEC_LOG_ERROR("BAD render id %d, new udec id %d.\n", dsp_cmd.configs[i].render_id, s_switch->switch_config[i].new_udec_id);
		}
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

static void cmd_udec_postp_buffering_control(iav_postp_buffering_control_t* control)
{
	POSTPROC_MW_BUFFERING_CONTROL_CMD cmd;
	memset(&cmd, 0, sizeof(cmd));

	cmd.cmd_code = CMD_POSTPROC_MW_BUFFERING_CONTROL;
	cmd.stream_id = control->stream_id;
	cmd.control_direction = control->control_direction;
	cmd.frame_time = control->frame_time;

	dsp_issue_cmd(&cmd, sizeof(cmd));
}

static int save_windows_config(udec_window_t __user* windows_config, u32 number)
{
	u32 i;
	udec_window_t* window = G_decode_obj->windows;

	UDEC_DEBUG_ASSERT(number < MAX_NUM_WINDOWS);
	for (i = 0; i < number; i++, window++) {
		if (copy_from_user(window, windows_config + i, sizeof(udec_window_t)))
			return -EFAULT;
	}

	return 0;
}

static int save_renders_config(udec_render_t __user* render_config, u32 number)
{
	u32 i;
	udec_render_t* render = G_decode_obj->renders;

	UDEC_DEBUG_ASSERT(number < MAX_NUM_WINDOWS);
	for (i = 0; i < number; i++, render++) {
		if (copy_from_user(render, render_config + i, sizeof(udec_render_t)))
			return -EFAULT;
	}

	return 0;
}

static void _reset_mudec_variables(void)
{
	u32 i;

	//reset last switch status
	for (i=0; i< MAX_NUM_WINDOWS; i++) {
		G_decode_obj->last_switch_status[i] = INVALID_SWITCH_STATUS;
	}
}
#endif

static int iav_enter_mdec_mode(iav_context_t *context, iav_mdec_mode_config_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_mdec_mode_config_t *mdec_mode;
	iav_udec_mode_config_t *udec_mode;
	int rval;

	// only in IDLE state
	if (G_iav_info.state != IAV_STATE_IDLE || !G_iav_info.dsp_booted) {
		UDEC_LOG_ERROR("bad state: %d, booted: %d\n", G_iav_info.state, G_iav_info.dsp_booted);
		return -EPERM;
	}

	// vout owner
	clear_vout_owner();

	mdec_mode = &G_decode_obj->mdec_mode_config;
	if (copy_from_user(mdec_mode, arg, sizeof(*mdec_mode)))
		return -EFAULT;

	udec_mode = &mdec_mode->super;

#if 0
	UDEC_DEBUG_ASSERT(2 == udec_mode->vout_mask);
	if (2 != udec_mode->vout_mask) {
		UDEC_LOG_ERROR("mdec only support HDMI, please check code, vout_mask 0x%x\n", udec_mode->vout_mask);
		return -EPERM;
	}
#endif
	// verify parameters

	_reset_mudec_variables();

	//debug check
	if (!mdec_mode->total_num_render_configs) {
		UDEC_LOG_ERROR("please use new api, fill mdec_mode->total_num_render_configs.\n");
		UDEC_DEBUG_ASSERT(mdec_mode->total_num_win_configs);

		mdec_mode->max_num_windows = mdec_mode->total_num_win_configs + 1;
		mdec_mode->total_num_render_configs = mdec_mode->total_num_win_configs;
	}

	//current constrain
	//if (mdec_mode->total_num_render_configs > 4) {
	//	UDEC_LOG_ERROR("mdec_mode->total_num_render_configs %d > 4, not as expected with current ucode.\n", mdec_mode->total_num_render_configs);
	//	mdec_mode->total_num_render_configs = 4;
	//}

	udec_mode->postp_mode = 3;	// multi-window playback

	// setup VOUT src
	iav_dbg_printk("vout_mask: 0x%x\n", udec_mode->vout_mask);
	context->need_issue_reset_hdmi = 0;
	iav_change_vout_src_ex(context, udec_mode->vout_mask, VOUT_SRC_UDEC);
	G_decode_obj->udec_max_number = IAV_NUM_MDEC;

	if ((rval = config_udecs(udec_mode)) < 0)
		return rval;

//	if ((rval = config_windows(mdec_mode)) < 0)
//		return rval;

	//udec_mode->vout_mask

	//save vout settings
	save_windows_config(mdec_mode->windows_config, mdec_mode->total_num_win_configs);
	save_renders_config(mdec_mode->render_config, mdec_mode->total_num_render_configs);

	do_enter_udec_mode(context, udec_mode);

	UDEC_LOG_ALWAYS("[m udec flow]: do_enter_udec_mode done.\n");

	//send display related cmd
	cmd_udec_postp_global_config();
	cmd_udec_postp_window_config(G_decode_obj->windows, mdec_mode->total_num_win_configs);
	cmd_udec_postp_render_config(G_decode_obj->renders, mdec_mode->total_num_render_configs, mdec_mode->total_num_render_configs);//hard code here

	UDEC_LOG_ALWAYS("[m udec flow]: send multi-window postp cmd done.\n");

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_postp_window_config(iav_context_t *context, iav_postp_window_config_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_postp_window_config_t window;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&window, arg, sizeof(window)))
		return -EFAULT;

	cmd_udec_postp_window_config(window.configs, window.num_configs);

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_postp_render_config(iav_context_t *context, iav_postp_render_config_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_postp_render_config_t render;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&render, arg, sizeof(render)))
		return -EFAULT;

	cmd_udec_postp_render_config(render.configs, render.total_num_windows_to_render, render.num_configs);

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_postp_stream_switch(iav_context_t *context, iav_postp_stream_switch_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_postp_stream_switch_t s_switch;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&s_switch, arg, sizeof(s_switch)))
		return -EFAULT;

	cmd_udec_stream_switch(&s_switch);

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_wait_stream_switch_msg(iav_context_t *context, iav_wait_stream_switch_msg_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_wait_stream_switch_msg_t status;
	u32 render_id;
	unsigned long flags;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&status, arg, sizeof(status))) {
		UDEC_LOG_ERROR("copy_from_user error?\n");
		return -EFAULT;
	}

	render_id = status.render_id;
	if (render_id >= MAX_NUM_WINDOWS) {
		UDEC_LOG_ERROR("BAD render id %d\n", render_id);
		return -EINVAL;
	}

	while (1) {
		//if (G_iav_obj.dec_state == DEC_STATE_IDLE)
		//	return -EAGAIN;

		dsp_lock(flags);

		if (INITIAL_SWITCH_STATUS != G_decode_obj->last_switch_status[render_id]) {
			if (INVALID_SWITCH_STATUS == G_decode_obj->last_switch_status[render_id]) {
				UDEC_LOG_ERROR("This render id %d do not have sent switch cmd?\n", render_id);
				dsp_unlock(flags);
				return -EINVAL;
			}
			status.switch_status = G_decode_obj->last_switch_status[render_id];
			dsp_unlock(flags);
			break;
		}

		G_decode_obj->num_waiters++;
		dsp_unlock(flags);

		if (wait_decode_msg(context) < 0)
			return -EINTR;
	}

	return copy_to_user(arg, &status, sizeof(status)) ? -EFAULT : 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_postp_buffering_control(iav_context_t *context, iav_postp_buffering_control_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_postp_buffering_control_t control;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&control, arg, sizeof(control))) {
		UDEC_LOG_ERROR("copy_from_user error?\n");
		return -EFAULT;
	}

	cmd_udec_postp_buffering_control(&control);
	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

static int iav_postp_update_mw_display(iav_context_t *context, iav_postp_update_mw_display_t __user *arg)
{
#ifdef ENABLE_MULTI_WINDOW_UDEC
	iav_postp_update_mw_display_t mw_display;

	if (G_iav_info.state != IAV_STATE_DECODING) {
		UDEC_LOG_ERROR("bad state: %d\n", G_iav_info.state);
		return -EPERM;
	}

	if (3 != G_decode_obj->mdec_mode_config.super.postp_mode) {
		UDEC_LOG_ERROR("postp mode(%d) is not 3, this ioctl is not permitted (not in MUDEC mode)\n", G_decode_obj->mdec_mode_config.super.postp_mode);
		return -EPERM;
	}

	if (copy_from_user(&mw_display, arg, sizeof(mw_display))) {
		UDEC_LOG_ERROR("copy_from_user error?\n");
		return -EFAULT;
	}

	if (mw_display.out_to_hdmi == G_decode_obj->mdec_mode_config.out_to_hdmi) {
		UDEC_LOG_ERROR("vout device is not changed(ori %d, new %d), should not invoke this api\n", mw_display.out_to_hdmi, G_decode_obj->mdec_mode_config.out_to_hdmi);
		return -EPERM;
	}

	UDEC_LOG_ALWAYS("[m udec flow]: before update mw display, ori (vout on hdmi %d, max window %d, window number %d, render number %d).\n", G_decode_obj->mdec_mode_config.out_to_hdmi, G_decode_obj->mdec_mode_config.max_num_windows, G_decode_obj->mdec_mode_config.total_num_win_configs, G_decode_obj->mdec_mode_config.total_num_render_configs);

	if (mw_display.total_num_win_configs) {
		if (mw_display.windows_config) {
			save_windows_config(mw_display.windows_config, mw_display.total_num_win_configs);
			G_decode_obj->mdec_mode_config.total_num_win_configs = mw_display.total_num_win_configs;
		} else {
			UDEC_LOG_ERROR("NULL iav_postp_update_mw_display_t->windows_config\n");
			return -EINVAL;
		}
	}

	if (mw_display.total_num_render_configs) {
		if (mw_display.render_config) {
			save_renders_config(mw_display.render_config, mw_display.total_num_render_configs);
			G_decode_obj->mdec_mode_config.total_num_render_configs = mw_display.total_num_render_configs;
		} else {
			UDEC_LOG_ERROR("NULL iav_postp_update_mw_display_t->render_config\n");
			return -EINVAL;
		}
	}

	//save display settings
	if (mw_display.max_num_windows) {
		G_decode_obj->mdec_mode_config.max_num_windows = mw_display.max_num_windows;
	}
	G_decode_obj->mdec_mode_config.out_to_hdmi = mw_display.out_to_hdmi;
	G_decode_obj->mdec_mode_config.video_win_width = mw_display.video_win_width;
	G_decode_obj->mdec_mode_config.video_win_height = mw_display.video_win_height;

	UDEC_LOG_ALWAYS("[m udec flow]: before update mw display, new (vout on hdmi %d, max window %d, window number %d, render number %d), send cmd to dsp.\n", G_decode_obj->mdec_mode_config.out_to_hdmi, G_decode_obj->mdec_mode_config.max_num_windows, G_decode_obj->mdec_mode_config.total_num_win_configs, G_decode_obj->mdec_mode_config.total_num_render_configs);

	dsp_start_cmdblk(DSP_CMD_PORT_GEN);
	//send display related cmd
	cmd_udec_postp_global_config();
	if (mw_display.total_num_win_configs) {
		cmd_udec_postp_window_config(G_decode_obj->windows, mw_display.total_num_win_configs);
	}
	if (mw_display.total_num_render_configs) {
		cmd_udec_postp_render_config(G_decode_obj->renders, G_decode_obj->mdec_mode_config.total_num_render_configs, G_decode_obj->mdec_mode_config.total_num_render_configs);//hard code here
	}
	dsp_end_cmdblk(DSP_CMD_PORT_GEN);

	UDEC_LOG_ALWAYS("[m udec flow]: send multi-window postp cmd done.\n");

	return 0;
#else
	UDEC_LOG_ERROR("ENABLE_MULTI_WINDOW_UDEC is not enabled\n");
	return -EPERM;
#endif
}

int __iav_decode_ioctl(iav_context_t *context, unsigned int cmd, unsigned long arg)
{
	int rval;

	switch (_IOC_NR(cmd)) {
	case IOC_START_DECODE:
		rval = -EPERM;
		break;

	case IOC_STOP_DECODE:
		rval = -EPERM;
		break;

	case IOC_DECODE_H264:
		rval = -EPERM;
		break;

	case IOC_DECODE_JPEG:
		rval = -EPERM;
		break;

	case IOC_DECODE_MULTI:
		rval = -EPERM;
		break;

	case IOC_DECODE_PAUSE:
		rval = -EPERM;
		break;

	case IOC_DECODE_RESUME:
		rval = -EPERM;
		break;

	case IOC_DECODE_STEP:
		rval = -EPERM;
		break;

	case IOC_TRICK_PLAY:
		rval = -EPERM;
		break;

	case IOC_GET_DECODE_INFO:
		rval = -EPERM;
		break;

	case IOC_WAIT_BSB:
		rval = -EPERM;
		break;

	case IOC_WAIT_EOS:
		rval = -EPERM;
		break;

	case IOC_CONFIG_DECODER:
		rval = -EPERM;
		break;

	case IOC_CONFIG_DISPLAY:
		rval = -EPERM;
		break;

	case IOC_ENTER_UDEC_MODE:
		rval = iav_enter_udec_mode(context, (iav_udec_mode_config_t __user *)arg);
		break;

	case IOC_CONFIG_FB_POOL:
		rval = iav_config_fb_pool(context, (iav_fbp_config_t __user *)arg);
		break;

	case IOC_REQUEST_FRAME:
		rval = iav_request_frame(context, (iav_frame_buffer_t __user *)arg);
		break;

	case IOC_GET_DECODED_FRAME:
		rval = iav_get_decoded_frame(context, (iav_frame_buffer_t __user *)arg);
		break;

	case IOC_RENDER_FRAME:
		rval = iav_render_frame(context, (iav_frame_buffer_t __user *)arg, 0);
		break;

	case IOC_RELEASE_FRAME:
		rval = iav_render_frame(context, (iav_frame_buffer_t __user *)arg, 1);
		break;

	case IOC_POSTP_FRAME:
		rval = iav_postp_frame(context, (iav_frame_buffer_t __user *)arg);
		break;

	case IOC_WAIT_DECODER:
		rval = iav_decode_wait(context, (iav_wait_decoder_t __user *)arg);
		break;

	case IOC_DECODE_FLUSH:
		rval = -EPERM;
		break;

	case IOC_INIT_UDEC:
		rval = iav_init_udec(context, (iav_udec_info_ex_t __user *)arg);
		break;

	case IOC_RELEASE_UDEC:
		rval = iav_release_udec(context, (u32)arg);
		break;

	case IOC_UDEC_DECODE:
		rval = iav_udec_decode(context, (iav_udec_decode_t __user *)arg);
		break;

	case IOC_UDEC_STOP:
		rval = iav_stop_udec(context, (u32)arg);
		break;

	case IOC_WAIT_UDEC_STATUS:
		rval = iav_wait_udec_status(context, (iav_udec_status_t __user *)arg);
		break;

	case IOC_GET_UDEC_STATE:
		rval = iav_get_udec_state(context, (iav_udec_state_t __user *)arg);
		break;

	case IOC_SET_AUDIO_CLK_OFFSET:
		rval = iav_set_audio_clk_offset(context, (iav_audio_clk_offset_t __user *)arg);
		break;

	case IOC_WAKE_VOUT:
		rval = iav_wake_vout(context, (iav_wake_vout_t __user*)arg);
		break;

	case IOC_UPDATE_VOUT_CONFIG:
		rval = iav_update_vout_config(context, (iav_udec_display_t __user*)arg);
		break;

	case IOC_UDEC_TRICKPLAY:
		rval = iav_udec_trickplay(context, (iav_udec_trickplay_t __user*)arg);
		break;

	case IOC_UDEC_PB_SPEED:
		rval = iav_udec_pb_speed(context, (iav_udec_pb_speed_t __user*)arg);
		break;

	case IOC_UDEC_MAP:
		rval = iav_udec_map(context, (iav_udec_map_t __user*)arg);
		break;

	case IOC_UDEC_UNMAP:
		rval = iav_udec_unmap(context);
		break;

	case IOC_UDEC_UPDATE_FB_POOL_CONFIG:
		rval = iav_udec_update_fb_pool_config(context, (iav_fbp_config_t __user*)arg);
		break;

	case IOC_ENTER_MDEC_MODE:
		rval = iav_enter_mdec_mode(context, (iav_mdec_mode_config_t __user*)arg);
		break;

//	case IOC_ENTER_MDEC_MODE_EX:
//		rval = iav_enter_mdec_mode_ex(context, (iav_mdec_mode_config_t __user*)arg);
//		break;

	case IOC_POSTP_WINDOW_CONFIG:
		rval = iav_postp_window_config(context, (iav_postp_window_config_t __user*)arg);
		break;

	case IOC_POSTP_RENDER_CONFIG:
		rval = iav_postp_render_config(context, (iav_postp_render_config_t __user*)arg);
		break;

	case IOC_POSTP_STREAM_SWITCH:
		rval = iav_postp_stream_switch(context, (iav_postp_stream_switch_t __user*)arg);
		break;

	case IOC_WAIT_STREAM_SWITCH_MSG:
		rval = iav_wait_stream_switch_msg(context, (iav_wait_stream_switch_msg_t __user*)arg);
		break;

	case IOC_POSTP_BUFFERING_CONTROL:
		rval = iav_postp_buffering_control(context, (iav_postp_buffering_control_t __user*)arg);
		break;

	case IOC_UPDATE_VOUT_DEWARP_CONFIG:
		rval = iav_update_vout_dewarp_config(context, (iav_udec_vout_dewarp_config_t __user*)arg);
		break;

	case IOC_UDEC_CAPTURE:
		rval = iav_udec_capture(context, (iav_udec_capture_t __user*)arg);
		break;

	case IOC_RELEASE_UDEC_CAPTURE:
		rval = iav_release_udec_capture(context, (u32)arg);
		break;

	case IOC_UDEC_ZOOM:
		rval = iav_udec_zoom(context, (iav_udec_zoom_t __user*)arg);
		break;

	case IOC_POSTP_UPDATE_MW_DISPLAY:
		rval = iav_postp_update_mw_display(context, (iav_postp_update_mw_display_t __user*)arg);
		break;

	case IOC_DECODE_DBG:
		rval = iav_decode_dbg(context, (u32)arg);
		break;

	default:
		LOG_ERROR("unknown cmd 0x%x.\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

