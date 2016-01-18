
/*
 * iav_priv.h
 *
 * History:
 *	2011/07/21 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#ifndef __IAV_PRIV_H__
#define __IAV_PRIV_H__


#define JPEG_QT_SIZE	128
enum {
	IAV_CORTEX = 0,
	IAV_ARM11 = 1,
};

extern struct iav_global_info G_iav_info;
extern struct iav_obj G_iav_obj;

extern struct amba_iav_vout_info G_vout0info;
extern struct amba_iav_vout_info G_vout1info;
extern struct amba_iav_vin_info G_vininfo;
extern struct iav_debug_info * G_iav_debug_info;
extern u32 G_iav_debug_flag;

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#define 	SET_EXTRA_BUF(value) (((value) > 0) ? ((value) & 0x7) : (value))

#define	SET_EXTRA_BUF_MSB(value) (((value) > 0) ? ((value) >> 3) : 0)

#define	Get_EXTRA_BUF_NUM(value1, value2)	(((value1) < 0) ?  \
	(value1) : (((value2) << 3) | (value1)))

#ifndef CONFIG_AMBARELLA_IAV_DEBUG
#define iav_dsp(cmd, arg)
#define iav_dsp_s32(cmd, arg)
#define iav_dsp_hex(cmd,arg)
#define iav_no_check()		do {} while (0)
#else
#define iav_dsp(cmd, arg)		do {		\
	if (G_iav_debug_flag & IAV_DEBUG_PRINT_ENABLE) {	\
		DRV_PRINT(KERN_DEBUG "%s: "#arg" = %d\n", __func__, (u32)cmd.arg);	\
	}	\
} while (0)

#define iav_dsp_s32(cmd, arg)	do {		\
	if (G_iav_debug_flag & IAV_DEBUG_PRINT_ENABLE) {	\
		DRV_PRINT(KERN_DEBUG "%s: "#arg" = %d\n", __func__, (s32)cmd.arg);	\
	}	\
} while (0)

#define iav_dsp_hex(cmd,arg) 	do {		\
	if (G_iav_debug_flag & IAV_DEBUG_PRINT_ENABLE) {	\
		DRV_PRINT(KERN_DEBUG "%s: "#arg" = 0x%x\n", __func__, (u32)cmd.arg);	\
	}	\
} while (0)

#ifdef iav_printk
#undef iav_printk
#define iav_printk(S...)		do {		\
	if (G_iav_debug_flag & IAV_DEBUG_PRINT_ENABLE) {		\
		__iav_printk(S);	\
	}		\
} while (0)
#endif

#define iav_no_check()		do {		\
	if (G_iav_debug_flag & IAV_DEBUG_CHECK_DISABLE) {	\
		__iav_printk("%s: NO CHECK!\n", __func__);		\
		return 0;	\
	}	\
} while (0)

#endif	// end of "CONFIG_AMBARELLA_IAV_DEBUG"

#ifndef IAV_ASSERT
#define IAV_ASSERT(x)	BUG_ON(!(x))
#endif

#ifdef ENABLE_RT_COUNTER
#define READ_COUNTER(timer)	timer = *(u32 *)(TIMER_BASE + 0x10)
#else
#define READ_COUNTER(timer)
#endif	// end of "ENABLE_RT_COUNTER"


typedef enum {
	MAIN_BUFFER_ID = 0,
	PREVIEW_A_BUFFER_ID = 1,
	PREVIEW_B_BUFFER_ID = 2,
	PREVIEW_C_BUFFER_ID = 3,
	DRAM_BUFFER_ID = 4,
	PREVIEW_BUFFER_TOTAL_NUM,
} CAPTURE_BUFFER_ID;

typedef enum {
	DECODE_STOPPED,
	DECODE_H264,
	DECODE_RAW,
	DECODE_JPEG,
	DECODE_MULTI,
} iav_decode_state_t;

typedef enum {
	IAV_ENCODE_OP_STOP = 0,
	IAV_ENCODE_OP_START,
} iav_encode_op_ex_t;

typedef struct iav_stream_encode_config_ex_s {
	iav_h264_config_ex_t	h264_encode_config;
	iav_jpeg_config_ex_t	jpeg_encode_config;
	u8 frame_rate_division_factor;
	u8 frame_rate_multiplication_factor;
	u16 frame_drop : 9;
	u8 keep_fps_in_ss : 1;
	u16 reserved : 6;
} iav_stream_encode_config_ex_t;

typedef struct iav_stream_statis_config_ex_s {
	u32	enable_flags;
	u8	mvdump_division_factor;
	u8	reserved[3];
	u32	mvdump_pitch;
	u32	mvdump_unit_size;
} iav_stream_statis_config_ex_t;

/* This structure defines source buffer hard limitation in DSP HW */
typedef struct iav_source_buffer_property_ex_s {
	iav_reso_ex_t max;
	u16	max_zoom_in_factor;
	u16	max_zoom_out_factor;
} iav_source_buffer_property_ex_t;

typedef struct iav_source_buffer_dram_pool_ex_s {
	u16	id[2];	/* Buffer pool id is returned in the creation MEMM buffer pool message. */
	u16	req_index;	/* Request index in the array of frame buffer id, buffer address */
	u16	update_index;	/* Handshake index in the array of frame buffer id */

	/* frame_buffer_id, buffer addresses are returned in the request of
	 * MEMM frame buffer message. */
	u16	yuv_frame_id[IAV_MAX_ENC_DRAM_BUF_NUM];
	u16	me1_frame_id[IAV_MAX_ENC_DRAM_BUF_NUM];
	u32	y[IAV_MAX_ENC_DRAM_BUF_NUM];
	u32	uv[IAV_MAX_ENC_DRAM_BUF_NUM];
	u32	me1[IAV_MAX_ENC_DRAM_BUF_NUM];
	u32	frame_req_done;		/* bit 0 for ME1, bit 1 for YUV */
} iav_source_buffer_dram_pool_ex_t;

/* This structure defines source buffer DRAM related parameters */
typedef struct iav_source_buffer_dram_ex_s {
	u8	max_frame_num;
	u8	buf_alloc;			/* bit 0 for YUV400 (ME1), bit 1 for YUV420, bit 2 for ME1 addr query, bit 3 for YUV addr query*/
	u16	buf_pitch;
	iav_reso_ex_t buf_max_size;
	u32	buf_state;
	u32	frame_pts;
	u32	valid_frame_num;
	iav_source_buffer_dram_pool_ex_t buf_pool;
	wait_queue_head_t	wq;
} iav_source_buffer_dram_ex_t;

typedef struct iav_encode_stream_ex_s {
	STREAM_TP					stream_type;	// stream type for DSP
	iav_encode_op_ex_t			op;
	iav_encode_format_ex_t		format;
	iav_stream_statis_config_ex_t	statis;
	iav_stream_privacy_mask_t	pm;
	wait_queue_head_t	venc_wq;
	encode_state_t 	fake_dsp_state;  /* Venc interrupt send fake state to Vdsp interrupt */
} iav_encode_stream_ex_t;

typedef struct iav_source_buffer_ex_s {
	iav_buffer_id_t						id;
	iav_source_buffer_type_ex_t			type;
	iav_source_buffer_state_ex_t		state;
	iav_reso_ex_t						size;
	iav_rect_ex_t						input;
	iav_source_buffer_property_ex_t		property;
	iav_source_buffer_dram_ex_t		dram;		// only for encode from DRAM
	u8	preview_framerate_division_factor;		// only for preview
	u8	enc_stop;								// for encode
	u16	unwarp : 1;								// for dewarp pipeline
	u16	disable_extra_2x : 1;						// for preview C only
	u16	reserved : 14;
	u32	ref_count;
} iav_source_buffer_ex_t;

typedef struct iav_dsp_partition_s {
	u32	id_map;
	u32	id_map_user;
	u32	map_flag;
	wait_queue_head_t	wq;
} iav_dsp_partition_t;

struct iav_debug_info
{
	u32			global_info_addr;
	u32			kernel_start;
	u32			phys_offset;

	u32			bs_desc_base;
	u32			bs_desc_size;

	u32			bsb_mem_addr;
	u32			bsb_mem_size;

	u32			dsp_mem_addr;
	u32			dsp_mem_size;

	u32			decode_obj_addr;
	u32			dsp_obj_addr;
};

typedef enum {
	DEC_STATE_IDLE,
	DEC_STATE_STOPPED,
	DEC_STATE_RUNNING,
} DECODE_STATE;

typedef enum {
	DEC_TYPE_NONE,
	DEC_TYPE_H264,
	DEC_TYPE_RAW,
	DEC_TYPE_JPEG,
	DEC_TYPE_MULTI,
	DEC_TYPE_UDEC,
} DECODE_TYPE;

struct iav_obj
{
	int	op_mode;		// DSP_OP_MODE		//removed and don't know if this is useful

	int	dsp_encode_mode;	// encode_mode_t
	int	dsp_encode_state;	// encode_state_t
	int	dsp_encode_state2;	// encode_state_t
	int	dsp_encode_state_ex[IAV_MAX_ENCODE_STREAMS_NUM];
	int	dsp_vcap_mode;
	int	dsp_decode_state;	// decode_state_t
	int	decode_state;		// iav_decode_state_t

	DECODE_TYPE	dec_type;
	DECODE_STATE	dec_state;

	u8	udec_type;
	u8	reserved1[3];

	u32	dsp_timecode;
	u32	dsp_encode_pts;
	u32	dsp_decode_pts;

	// encode related structures
	iav_encode_stream_ex_t	*stream;
	iav_source_buffer_ex_t		*source_buffer;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD	*system_resource;
	iav_system_setup_info_ex_t	*system_setup_info;

	u32	dsp_chip_id;

	spinlock_t	lock;
	wait_queue_head_t		vcap_wq;
	wait_queue_head_t		vout_b_update_wq;

	struct notifier_block	system_event;

	struct dsp_vm_space_s	mode_vms;

	//VIN VSYNC error handling, 1 means doing error handling
	u32	vsync_error_handling	: 1;
	// VIN VSYNC error again, 1 means vsync error during vsync loss handling
	u32	vsync_error_again 		: 1;
	// VIN VSYNC signal lost info, 1 means vsync signal alreadylost
	u32	vsync_signal_lost		: 1;
	u32	vcap_preview_done		: 1;
	u32	dsp_noncached			: 1;
	u32	vout_b_update_done		: 1;
	u32	reserved2				: 26;
};

typedef struct iav_completion_s {
	int waiters;
	struct completion compl;
} iav_completion_t;

typedef struct iav_hrtimer_s {
	struct hrtimer	hr_timer;
	struct semaphore	sem_ready;
	int	enable;
	int	period_in_ms;	//polling period in ms UNIT
	int	method;		//not used now
	u32	wait_count;
} iav_hrtimer_t;

#define DEFINE_IAV_TIMER(param)	struct iav_hrtimer_s param

static inline struct amba_vin_src_capability * get_vin_capability(void)
{
	return &(G_iav_info.pvininfo->capability);
}


/* iav_api.c */
unsigned long __iav_spin_lock(void);
#define iav_irq_save(_flags)	\
	do { _flags = __iav_spin_lock(); } while (0)
void iav_irq_restore(unsigned long flags);
void notify_waiters(iav_completion_t * compl);
void notify_all_waiters(iav_completion_t * compl);
void iav_init_compl(iav_completion_t * compl);
int iav_wait_compl_interruptible(iav_completion_t * compl);
void set_iav_state(IAV_STATE state);
int get_iav_state(void);
int is_iav_state_init(void);
int is_iav_state_idle(void);
int is_iav_state_preview(void);
int is_iav_state_encoding(void);
int is_iav_state_prev_or_enc(void);
int is_valid_buffer_id(int buffer);
int is_valid_dram_buffer_id(int buffer);
int is_valid_vca_buffer_id(int buffer);
int is_invalid_dsp_addr(u32 addr);
int is_buf_type_off(int buffer);
int is_buf_type_enc(int buffer);
int is_buf_type_prev(int buffer);
int is_buf_unwarped(int buffer);
int is_sub_buf(int buffer);
int get_enc_mode(void);
int is_full_framerate_mode(void);
int is_multi_region_warp_mode(void);
int is_high_mega_pixel_mode(void);
int is_calibration_mode(void);
int is_hdr_frame_interleaved_mode(void);
int is_hdr_line_interleaved_mode(void);
int is_high_mp_full_perf_mode(void);
int is_full_fps_full_perf_mode(void);
int is_multi_vin_mode(void);
int is_hiso_video_mode(void);
int is_hdr_mode(void);
int is_full_performance_mode(void);
int is_warp_mode(void);
int is_sharpen_b_enabled(void);
int is_rotate_possible_enabled(void);
int is_polling_readout(void);
int is_interrupt_readout(void);
int is_raw_capture_enabled(void);
int is_raw_stat_capture_enabled(void);
int is_dptz_I_enabled(void);
int is_dptz_II_enabled(void);
int is_vin_cap_offset_enabled(void);
int is_mctf_pm_enabled(void);
int is_hwarp_bypass_enabled(void);
int is_svc_t_enabled(void);
int is_enc_from_dram_enabled(void);
int is_video_freeze_enabled(void);
int is_map_dsp_partition(void);
int is_vout_b_letter_boxing_enabled(void);
u8 * dsp_dsp_to_user(iav_context_t * context, u32 addr);
void cmd_warp_control(iav_rect_ex_t* main_input);
void cmd_update_capture_params_ex(int flags);
u32 get_expo_num(int encode_mode);
u32 get_vin_num(int encode_mode);
int get_vin_window(iav_rect_ex_t* vin);
int get_preview_size(u16 buf_enc_w, u16 buf_enc_h, iav_source_buffer_type_ex_t type, struct amba_video_info * video_info, u16 * width, u16 * height);
int check_zoom_property(int buffer_id, u16 input_width, u16 input_height, u16 output_width, u16 output_height, char * str);
int check_full_fps_full_perf_cap(char *str);

u32 get_max_enc_num(int encode_mode);
DSP_ENC_CFG * get_enc_cfg(int encode_mode, int stream);

int iav_vsync_guard_init(void);
int iav_vsync_guard_task(void * arg);


/* iav_encode.c */
u64 get_monotonic_pts(void);
int get_stream_type(int stream);
int get_single_stream_num(iav_stream_id_t stream_id, int * stream_num);
int is_supported_stream(int stream);
int is_end_frame(dsp_bits_info_t * bits_info);
void enc_irq_handler(void * context);
void wait_vcap_msg_count(int count);
void set_all_encode_state(iav_encode_stream_state_ex_t stream_state);
void calc_roundup_size(u16 width_in, u16 height_in, u32 enc_type, u16 *width_out, u16 *height_out);
int calc_encode_frame_rate(u32 vin_frame_rate, u32 multiplication_factor, u32 division_factor, u32 *dsp_frame_rate);
void get_round_encode_format(int stream, u16* round_width, u16* round_height, s16* offset_y_shift);

/* iav_decode.c */
void parse_decode_msg(void *msg);


/* iav_timer.c */
int iav_hrtimer_init(int ms, iav_hrtimer_t *hrtimer);
int iav_hrtimer_deinit(iav_hrtimer_t *hrtimer);
int iav_hrtimer_start(iav_hrtimer_t *hrtimer);
int iav_hrtimer_wait_next(iav_hrtimer_t *hrtimer);


/* iav_vout.c */
enum {
	VOUT_HALT_BEGIN,	// arg: vout_id
	VOUT_HALT_END,		// arg: vout_id
	VOUT_CONFIG_SINK_BEGIN,	// arg: struct amba_video_sink_mode
	VOUT_CONFIG_SINK_END,	// arg: struct amba_video_sink_mode
};

typedef struct vout_config_sink_s {
	int	vout_id;
	struct amba_video_sink_mode *pcfg;
} vout_config_sink_t;

typedef int (*iav_vout_preproc)(iav_context_t *context, unsigned int cmd, unsigned long arg);

void iav_vout_set_preproc(iav_vout_preproc preproc);

int iav_vout_init(void);
void iav_config_vout(iav_context_t *context, int vout_mask, VOUT_SRC vout_src);
void iav_change_vout_src(iav_context_t *context, VOUT_SRC vout_src);
void iav_config_vout_osd(iav_context_t *context);

int __iav_enable_vout_video(iav_context_t *context, struct iav_vout_enable_video_s *vout_enable_video);
void iav_set_vout_src(iav_context_t *context, unsigned vout_id, VOUT_SRC vout_src);

//for decoding
typedef struct vout_default_info_s {
	u32	y_addr;
	u32	uv_addr;
	u16	pitch;
	u8	repeat_field;
	u8	fld_polarity;
} vout_default_info_t;

typedef int (*iav_vout_preproc)(iav_context_t *context, unsigned int cmd, unsigned long arg);

void iav_vout_set_preproc(iav_vout_preproc preproc);
void iav_vout_set_irq_handler(void (*)(unsigned));

/* iav_pts.c */
int hw_pts_init(void);
int hw_pts_deinit(void);
u64 get_hw_pts(u32 dsp_pts);
int get_hwtimer_output_ticks(u64 *out_tick);
int get_hwtimer_output_freq(u32 *out_freq);

//vout
void iav_change_vout_src_ex(iav_context_t *context, int vout_mask, VOUT_SRC vout_src);
int iav_update_vout_video_src_ar(u8 ar);
vout_default_info_t *iav_vout_default_info(unsigned vout_id);
int iav_init_vout_irq(void *dev);

/* iav_encode.c */
void cmd_update_encode_params_ex(iav_context_t *context, int stream, int flags);
int get_stream_offset_param(iav_stream_offset_t* param);
int cfg_stream_offset_param(iav_stream_offset_t* param);

/* iav_overlay */
void cmd_set_overlay_insert(iav_context_t * context, int stream);
int get_stream_overlay_param(overlay_insert_ex_t* param);
int cfg_stream_overlay_param(iav_context_t* context, overlay_insert_ex_t* param);
#endif	// __IAV_PRIV_H__

