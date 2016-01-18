/*
 * iav_encode.h
 *
 * History:
 *	2011/11/11 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#ifndef __IAV_ENCODE_H__
#define __IAV_ENCODE_H__


/**********************************************************
 *
 * Internal enumerations and macro definition
 *
 *********************************************************/

#define	ENCODE_BITS_INFO_GET_STREAM_ID(stream_id)	(((stream_id) >> 6) & 0x3)

#define	SESSION_ID_INDEX_SIZE			(16)

typedef enum {
	ENCODE_INVALID_PTS = 0xc0c0c0c0c0c0c0c0LL,
	ENCODE_STOPPED_PTS = 0xffffffffffffffffLL,
	ENCODE_INVALID_ADDRESS = 0xffffffff,
	ENCODE_DURATION_FOREVRE = 0xFFFFFFFF,
	ENCODE_INVALID_LENGTH = ((1 << 24) - 1),

	Q_MATRIX_SIZE = (16 * IAV_QM_TYPE_NUM),
	TOTAL_Q_MATRIX_SIZE = (Q_MATRIX_SIZE * IAV_MAX_ENCODE_STREAMS_NUM *
		DEFAULT_TOGGLED_BUFFER_NUM),
} IAV_ENCODE_PARAMS;

typedef enum {
	// QP limit parameters
	H264_AQP_MAX = 4,
	H264_QP_MAX = 51,
	H264_QP_MIN = 0,
	H264_I_QP_REDUCE_MAX = 10,
	H264_I_QP_REDUCE_MIN = 1,
	H264_P_QP_REDUCE_MAX = 5,
	H264_P_QP_REDUCE_MIN = 1,
	H264_LOG_Q_NUM_MAX = 4,
	H264_MODE_BIAS_MIN = -4,
	H264_MODE_BIAS_MAX = 5,

	H264_USER_BIAS_MIN = 0,
	H264_USER_BIAS_MAX = 9,

	H264_WITHOUT_FRAME_DROP = 0,
	H264_WITH_FRAME_DROP = 6,

	H264_RC_LUT_NUM_MAX = 12,
} IAV_RATE_CONTROL_PARAMS;

typedef enum {
	IAV_QP_ROI_TYPE_BASIC = 0,
	IAV_QP_ROI_TYPE_ADV = 2,
} IAV_QP_ROI_TYPE;

typedef enum {
	BASELINE_PROFILE_IDC = 66,
	MAIN_PROFILE_IDC = 77,
	HIGH_PROFILE_IDC = 100,
} ENCODE_PROFILE_IDC;

typedef enum {
	LEVEL_IDC_11 = 11,
	LEVEL_IDC_12 = 12,
	LEVEL_IDC_13 = 13,
	LEVEL_IDC_20 = 20,
	LEVEL_IDC_21 = 21,
	LEVEL_IDC_22 = 22,
	LEVEL_IDC_30 = 30,
	LEVEL_IDC_31 = 31,
	LEVEL_IDC_32 = 32,
	LEVEL_IDC_40 = 40,
	LEVEL_IDC_41 = 41,
	LEVEL_IDC_42 = 42,
	LEVEL_IDC_50 = 50,
	LEVEL_IDC_51 = 51,
} ENCODE_LEVEL_IDC;

// refer to Annex A of H.264 specification
typedef enum {
	LEVEL_IDC_11_MB = 396,
	LEVEL_IDC_12_MB = 396,
	LEVEL_IDC_13_MB = 396,
	LEVEL_IDC_20_MB = 396,
	LEVEL_IDC_21_MB = 792,
	LEVEL_IDC_22_MB = 1620,
	LEVEL_IDC_30_MB = 1620,
	LEVEL_IDC_31_MB = 3600,
	LEVEL_IDC_32_MB = 5120,
	LEVEL_IDC_40_MB = 8192,
	LEVEL_IDC_41_MB = 8192,
	LEVEL_IDC_42_MB = 8704,
	LEVEL_IDC_50_MB = 22080,
	LEVEL_IDC_51_MB = 36864,
} ENCODE_LEVEL_IDC_MB;

// refer to Annex A of H.264 specification
typedef enum {
	LEVEL_IDC_11_MBPS = 3000,
	LEVEL_IDC_12_MBPS = 6000,
	LEVEL_IDC_13_MBPS = 11880,
	LEVEL_IDC_20_MBPS = 11880,
	LEVEL_IDC_21_MBPS = 19800,
	LEVEL_IDC_22_MBPS = 20250,
	LEVEL_IDC_30_MBPS = 40500,
	LEVEL_IDC_31_MBPS = 108000,
	LEVEL_IDC_32_MBPS = 216000,
	LEVEL_IDC_40_MBPS = 245760,
	LEVEL_IDC_41_MBPS = 245760,
	LEVEL_IDC_42_MBPS = 522240,
	LEVEL_IDC_50_MBPS = 589824,
	LEVEL_IDC_51_MBPS = 983040,
} ENCODE_LEVEL_IDC_MAX_MBPS;

// refer to Annex A of H.264 specification
typedef enum {
	BR_Kbps = 1000,
	LEVEL_IDC_11_BR = 192 * BR_Kbps,
	LEVEL_IDC_12_BR = 384 * BR_Kbps,
	LEVEL_IDC_13_BR = 768 * BR_Kbps,
	LEVEL_IDC_20_BR = 2000 * BR_Kbps,
	LEVEL_IDC_21_BR = 4000 * BR_Kbps,
	LEVEL_IDC_22_BR = 4000 * BR_Kbps,
	LEVEL_IDC_30_BR = 10000 * BR_Kbps,
	LEVEL_IDC_31_BR = 14000 * BR_Kbps,
	LEVEL_IDC_32_BR = 20000 * BR_Kbps,
	LEVEL_IDC_40_BR = 20000 * BR_Kbps,
	LEVEL_IDC_41_BR = 50000 * BR_Kbps,
	LEVEL_IDC_42_BR = 50000 * BR_Kbps,
	LEVEL_IDC_50_BR = 135000 * BR_Kbps,
	LEVEL_IDC_51_BR = 240000 * BR_Kbps,
} ENCODE_LEVEL_IDC_MAX_BR;

typedef enum {
	ENCODE_ASPECT_RATIO_UNSPECIFIED = 0,
	ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL = 1,
	ENCODE_ASPECT_RATIO_PAL_4_3 = 2,
	ENCODE_ASPECT_RATIO_NTSC_4_3 = 3,
	ENCODE_ASPECT_RATIO_CUSTOM = 255,
} ENCODE_ASPECT_RATIO_TYPE;

typedef enum {
	UPDATE_FRAME_FACTOR_FLAG = (1 << 0),
	UPDATE_FORCE_IDR_FLAG = (1 << 1),
	UPDATE_STREAM_BITRATE_FLAG = (1 << 2),
	UPDATE_STREAM_OFFSET_FLAG = (1 << 3),
	UPDATE_GOP_PARAM_FLAG = (1 << 4),
	UPDATE_SCBR_PARAM_FLAG = (1 << 5),
	UPDATE_QP_ROI_MATRIX_FLAG = (1 << 6),
	UPDATE_INTRA_BIAS_FLAG = (1 << 7),
	UPDATE_P_SKIP_FLAG = (1 << 8),
	UPDATE_QUANT_MATRIX_FLAG = (1 << 9),
	UPDATE_MONOCHROME_FLAG = (1 << 10),
	UPDATE_STREAM_PM_FLAG = (1 << 11),
	UPDATE_PANIC_PARAM_FLAG = (1 << 12),
	UPDATE_FRAME_DROP_FLAG = (1 << 13),
	UPDATE_CPB_RESET_FLAG = (1 << 14),
	UPDATE_ZMV_THRESHOLD_FLAG = (1 << 15),
	UPDATE_MODE_BIAS_FLAG = (1 << 16),
} UPDATE_ENCODE_PARAM_FLAG;

typedef enum {
	SCAN_FORMAT_BIT_SHIFT = 31,
	DENOMINATOR_BIT_SHIFT = 30,
} ENCODE_FRAME_RATE_BIT_SHIFT;

typedef enum {
	ENABLE_MVDUMP_FLAG = (1 << 0),
	ENABLE_QP_HIST_DUMP_FLAG = (1 << 1),
} ENCODE_STATISTICS_ENABLE_FLAG;

typedef enum {
	FS_UNUSED = 0,
	FS_QUEUED = 1,
	FS_FETCHED = 2,
	FS_RELEASED = 3,
	FS_INVALID = 4,
	FS_TOTAL_NUM,
	FS_FIRST = FS_UNUSED,
	FS_LAST = FS_TOTAL_NUM,
} IAV_FRAME_STATE;

typedef enum {
	FQ_FRM = 0,		/* Single frame */
	FQ_FQ = 1,		/* Frame Queue, including MJPEG and H264 */
	FQ_SQ = 2,		/* Stream Queue for streams separately */
} IAV_FRAME_QUEUE;

typedef enum {
	AUDIO_IN_FREQ_32000HZ = 0,
	AUDIO_IN_FREQ_44100HZ = 1,
	AUDIO_IN_FREQ_48000HZ = 2,
	AUDIO_IN_FREQ_TOTAL_NUM,
	AUDIO_IN_FREQ_FIRST = AUDIO_IN_FREQ_32000HZ,
	AUDIO_IN_FREQ_LAST = AUDIO_IN_FREQ_TOTAL_NUM,
} IAV_AUDIO_IN_FREQ;


/*********************************************************
 *
 * Internal structure definition
 *
 *********************************************************/

typedef struct {
	#define	DESC_LENGTH		(32)
	u32	system_load;
	char	desc[DESC_LENGTH];
	u8	max_enc_num;
	u8	reserved[3];
} iav_system_load_t;

typedef struct iav_bits_info_slot_ex_s {
	dsp_bits_info_t		* read_index;
	IAV_BITS_DESC_TYPE	bits_type;
	u16		frame_index;
	u16		reserved[3];
	u64		mono_pts;
	u64		dsp_pts;
} iav_bits_info_slot_ex_t;

typedef struct iav_statis_info_slot_ex_s {
	dsp_enc_stat_info_t	* read_index;
	u16		statis_index;
	u16		flag;
	u64		mono_pts;
} iav_statis_info_slot_ex_t;

typedef struct iav_encode_obj_ex_s {
	u32	total_bits_info_ctr_h264;
	u32	total_bits_info_ctr_mjpeg;
	u32	total_bits_info_ctr_tjpeg;
	u32	total_pic_encoded_h264_mode;
	u32	total_pic_encoded_mjpeg_mode;

	u32	h264_pic_counter;
	u32	mjpeg_pic_counter;
	u32	total_pic_counter;

	dsp_bits_info_t	*bits_desc_start;
	dsp_bits_info_t	*bits_desc_end;
	dsp_bits_info_t	*bits_desc_read_ptr;
	dsp_bits_info_t	*bits_desc_prefetch;

	iav_pts_info_t			pts_info;

	// dsp based interrupt will use this PTR to move, to put PTS into extend bits info
	dsp_extend_bits_info_t	*ext_bits_desc_start;
	dsp_extend_bits_info_t	*ext_bits_desc_end;
	dsp_extend_bits_info_t	*ext_bits_desc_write_ptr;

	dsp_enc_stat_info_t * stat_desc_start;
	dsp_enc_stat_info_t * stat_desc_end;
	dsp_enc_stat_info_t * stat_desc_read_ptr;

	dsp_extend_enc_stat_info_t	*ext_statis_desc_start;
	dsp_extend_enc_stat_info_t	*ext_statis_desc_end;
	dsp_extend_enc_stat_info_t	*ext_statis_desc_write_ptr;

	u32	bits_fifo_next;
	u32	bits_fifo_fullness;

	u32	total_desc;
	u32	curr_num_desc;

	int	encoding_stream_num;

	iav_encode_stream_state_ex_t encode_state[IAV_MAX_ENCODE_STREAMS_NUM];

	// Used for sync in wait encode
	iav_completion_t	enc_msg_compl;
	iav_completion_t	vcap_msg_compl;

	iav_completion_t	vsync_loss_compl;

	// Used for sync in encode interrupt
	wait_queue_head_t		enc_wq;
	struct semaphore	sem_statistics;
} iav_encode_obj_ex_t;


/**********************************************************
 *
 * Export external structures and variables for other module
 *
 *********************************************************/
extern iav_encode_obj_ex_t G_encode_obj;
extern iav_encode_stream_ex_t G_encode_stream[IAV_MAX_ENCODE_STREAMS_NUM];


/**********************************************************
 *
 * Export external APIs for other module
 *
 *********************************************************/
int iav_encode_init(void);
void iav_encode_exit(void);
int __iav_encode_ioctl(iav_context_t * context, unsigned int cmd, unsigned long arg);

void iav_encode_open(iav_context_t * context);
void iav_encode_release(iav_context_t * context);

int iav_stop_encode_ex(iav_context_t *context, iav_stream_id_t stream_id);

int get_gcd(int a, int b);
u32 get_current_frame_number(void);
int frame_info_queue_init(void);
int enqueue_frame(dsp_extend_bits_info_t * new_frame);
int dequeue_frame(dsp_extend_bits_info_t * old_frame);
int flush_all_frames(int stream);
int fetch_frame_from_stream_queue(int stream, iav_bits_info_slot_ex_t * encode_slot);
int fetch_frame_from_queue(iav_bits_info_slot_ex_t * encode_slot);
int release_frame_to_queue(int frame_index);

u32 get_current_statis_number(void);
int statis_info_queue_init(void);
int enqueue_statis(dsp_extend_enc_stat_info_t * new_statis, int valid);
int dequeue_statis(dsp_extend_enc_stat_info_t * old_statis);
int flush_all_statis(int stream);
int fetch_statistics_from_queue(int stream, iav_statis_info_slot_ex_t * statis_slot);
int release_statistics_to_queue(int statis_index);

#endif	// __IAV_ENCODE_H__

