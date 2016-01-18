/*
 * iav_encode.c
 *
 * History:
 *	2012/04/13 - [Jian Tang] created file
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include <linux/time.h>
#include <linux/random.h>

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
#include "iav_warp.h"
#include "iav_bufcap.h"
#include "iav_capture.h"
#include "iav_privmask.h"

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_imgproc.h"
#endif

#include "iav_encode_priv.c"

/******************************************
 *
 *	Interrupt handler and related functions
 *
 ******************************************/


inline u64 get_monotonic_pts(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	return ((u64)tv.tv_sec * 1000000L + tv.tv_usec);
}

/*
 * h264_pic_counter and mjpeg_pic_counter are the exact total frame numbers
 * encoded from DSP.
 * Advance them for last fake "stream end" frame, when stream is stopped.
 */
static inline int advanced_encoded_frame_counter(int stream)
{
	unsigned long flags;
	iav_irq_save(flags);
	if (__is_enc_h264(stream)) {
		++G_encode_obj.h264_pic_counter;
	} else if (__is_enc_mjpeg(stream)) {
		++G_encode_obj.mjpeg_pic_counter;
	} else {
		iav_error("Invalid encode type for stream [%d] !\n", stream);
		return -1;
	}
	iav_irq_restore(flags);
	iav_printk("Stream [%d] Advanced %s pic counter !\n", stream,
			__is_enc_h264(stream) ? "h264" : "mjpeg");
	return 0;
}

static inline void irq_update_iav_enc_state(void)
{
	int i;
	/* Update IAV global state:
	 * If there is still any streams encoding, set iav state back to
	 * ENCODING again */
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (!__is_stream_ready_for_encode(i)) {
			set_iav_state(IAV_STATE_ENCODING);
			break;
		}
	}
	if (i == IAV_MAX_ENCODE_STREAMS_NUM) {
		set_iav_state(IAV_STATE_PREVIEW);
	}
}

static inline void update_dsp_ext_bits_desc(dsp_bits_info_t * frame_index,
	dsp_extend_bits_info_t * new_frame_info)
{
	int stream = -1, bits_type;
	u32 audio_pts = 0;
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;
	iav_encode_format_ex_t *enc_format;

	stream = get_stream_num_from_type(frame_index->stream_type);
	bits_type = (frame_index->frameTy == JPEG_STREAM) ?
		IAV_BITS_MJPEG : IAV_BITS_H264;
	enc_format = &G_encode_stream[stream].format;
	audio_pts = frame_index->pts_64 >> 32;

	new_frame_info->dsp_pts = frame_index->pts_64;
	if (unlikely(is_end_frame(frame_index))) {
		spin_lock(&G_iav_obj.lock);
		if (__is_stream_in_starting(stream) &&
			((enc_format->duration < 3) && (enc_format->duration > 0))) {
			G_encode_stream[stream].fake_dsp_state = ENC_BUSY_STATE;
		}
		spin_unlock(&G_iav_obj.lock);
		new_frame_info->monotonic_pts = ENCODE_STOPPED_PTS;
	} else {
		if (is_valid_dram_buffer_id(enc_format->source)) {
			new_frame_info->monotonic_pts = audio_pts;
		} else if (pts_info->hwtimer_enabled == 0) {
			new_frame_info->monotonic_pts = get_monotonic_pts();
		} else {
			new_frame_info->monotonic_pts = get_hw_pts(audio_pts);
		}
		frame_index->pts_64 = ENCODE_INVALID_PTS;
		clean_cache_aligned((void *)frame_index, sizeof(dsp_bits_info_t));
	}

	new_frame_info->new_stream = stream;
	new_frame_info->new_bits_type = bits_type;
	new_frame_info->new_frame_size = is_end_frame(frame_index) ? 0 :
		ALIGN((u32)frame_index->length, 32);
	enqueue_frame(new_frame_info);

	/* Fixme: This is test function to add stress test inside ISR.
	 * It MUST be turned off in formal release code.
	iav_test_add_print_in_isr(stream, (u32)frame_index, (u32)pts_info, audio_pts);
	*/
}

static void update_statis_desc_read_ptr(void);
static void update_dsp_ext_statis_desc(dsp_enc_stat_info_t * statis_index)
{
	dsp_extend_enc_stat_info_t * new_statis = G_encode_obj.ext_statis_desc_write_ptr;
	new_statis->mono_pts = get_monotonic_pts();
	new_statis->new_stream = get_stream_num_from_type(statis_index->stream_type);
	new_statis->new_frame_num = statis_index->frmNo;
	enqueue_statis(new_statis, !is_invalid_statis_desc(statis_index));

	update_statis_desc_read_ptr();
	// update extend statis info write pointer
	if (++G_encode_obj.ext_statis_desc_write_ptr >=
			G_encode_obj.ext_statis_desc_end) {
		G_encode_obj.ext_statis_desc_write_ptr =
			G_encode_obj.ext_statis_desc_start;
	}
}

/* This function is called by interrupt based read out (CODE_VDSP_2_IRQ on S2)
 * to add timestamp on extend dsp bits info and also UP samephore so that
 * interrupt based read out protocol can work.
 * This function is also called to update frame counter for "NULL end" frame
 * packets when stream encoding stops
 */
void enc_irq_handler(void *context)
{
	int valid_frames;
	dsp_bits_info_t * read = NULL;
	dsp_extend_bits_info_t * frame_info = NULL;
	dsp_enc_stat_info_t * statis = NULL;

	/* If prefetch pointer points to an invalid frame, it means this ISR was
	 * triggered by a false interrupt, exit directly.
	 * Otherwise, prefetch pointer points to the valid frame will be read out
	 * immediately, update extend bits descriptor and counters, then
	 * advance the prefetch pointer for next round.
	 */
	do {
		spin_lock(&G_iav_obj.lock);
		read = G_encode_obj.bits_desc_prefetch;
		invalidate_d_cache((void *)read, sizeof(dsp_bits_info_t));
		if (is_invalid_bits_desc(read) ||
			(get_stream_num_from_type(read->stream_type) < 0)) {
			spin_unlock(&G_iav_obj.lock);
			break ;
		}
		++G_encode_obj.total_pic_counter;
		if (++G_encode_obj.bits_desc_prefetch >= G_encode_obj.bits_desc_end) {
			G_encode_obj.bits_desc_prefetch = G_encode_obj.bits_desc_start;
		}
		frame_info = G_encode_obj.ext_bits_desc_write_ptr;
		if (++G_encode_obj.ext_bits_desc_write_ptr >= G_encode_obj.ext_bits_desc_end) {
			G_encode_obj.ext_bits_desc_write_ptr = G_encode_obj.ext_bits_desc_start;
		}
		spin_unlock(&G_iav_obj.lock);

		/* Update extend bits descriptor */
		update_dsp_ext_bits_desc(read, frame_info);

		/* Update frame counter */
		wake_up_interruptible(&G_encode_obj.enc_wq);
	} while (0);

	do {
		statis = G_encode_obj.stat_desc_read_ptr;
		invalidate_d_cache((void *)statis, sizeof(dsp_enc_stat_info_t));
		if ((int)statis->frmNo < 0 || (int)statis->stream_type < 0) {
			break;
		}

		/* If frame or stream number is negative, the statitics info is not
		 * ready and IAV is not in "encoding" state. DSP writes the empty
		 * entry into the queue, while ARM should skip and wait for next frame.
		 */
		update_dsp_ext_statis_desc(statis);
		if (!is_invalid_statis_desc(statis)) {
			up(&G_encode_obj.sem_statistics);
		}

		/* "DOWN" unnecessary IRQ counter. Make it no larger than the total
		 * valid statistics inside the info queues.
		 */
		valid_frames = get_current_statis_number();
		while (G_encode_obj.sem_statistics.count > valid_frames) {
			down(&G_encode_obj.sem_statistics);
		}
	} while (0);

	return ;
}

static inline int get_one_encoded_frame(void)
{
	int rval = 0;
	rval = wait_event_interruptible(G_encode_obj.enc_wq,
		(get_current_frame_number() > 0));
	if (rval) {
		iav_error("Get one encoded frame interrupted!\n");
		return -1;
	} else {
		return 0;
	}
}

static inline int get_one_statistics_info(void)
{
	if (down_interruptible(&G_encode_obj.sem_statistics) != 0) {
		iav_printk("Get one statistics interrupted");
		return -1;
	} else {
		return 0;
	}
}

/* For interrupt based read out protocol, DSP will generate one more VOUT0
 * interrupt for "stream end" frame, which is a fake invalid frame to notice
 * ARM this session is stopped.
 * For multiple streams encoding, DSP will generate the last "stream end"
 * VOUT0 interrupts very closely, the time difference between two interrupts
 * may be in 30 us, which cannot be caught by ARM ISR.
 * Update encoded frame counter between Vsync and VOUT0 interrupt to avoid
 * miss any VOUT0 interrupt.
 */
static inline int sync_encoded_frame_counter(void)
{
	int i, delta;
	delta = (G_encode_obj.h264_pic_counter + G_encode_obj.mjpeg_pic_counter)
		- G_encode_obj.total_pic_counter;
	if (delta > 0) {
		spin_unlock(&G_iav_obj.lock);
		for (i = 0; i < delta; ++i) {
			enc_irq_handler(NULL);
		}
		spin_lock(&G_iav_obj.lock);
	}
	return 0;
}

static void notify_vcap_msg_waiters(iav_encode_obj_ex_t * enc)
{
	notify_all_waiters(&enc->vcap_msg_compl);
}

static void notify_vsync_loss_waiters(iav_encode_obj_ex_t * enc)
{
	notify_all_waiters(&enc->vsync_loss_compl);
}

static inline void irq_update_frame_count(ENCODER_STATUS_MSG *msg,
		int stream, u32 *total_encoded)
{
	u32 h264_pic_count = msg->total_bits_info_ctr_h264
		- G_encode_obj.total_bits_info_ctr_h264;
	u32 mjpeg_pic_count = msg->total_bits_info_ctr_mjpeg
		- G_encode_obj.total_bits_info_ctr_mjpeg;
	u32 tjpeg_pic_count = msg->total_bits_info_ctr_tjpeg
		- G_encode_obj.total_bits_info_ctr_tjpeg;

	G_encode_obj.total_bits_info_ctr_h264 = msg->total_bits_info_ctr_h264;
	G_encode_obj.total_bits_info_ctr_mjpeg = msg->total_bits_info_ctr_mjpeg;
	G_encode_obj.total_bits_info_ctr_tjpeg = msg->total_bits_info_ctr_tjpeg;
	if (h264_pic_count > 0 || mjpeg_pic_count > 0) {
		G_encode_obj.h264_pic_counter += h264_pic_count;
		G_encode_obj.mjpeg_pic_counter += mjpeg_pic_count;
//		iav_printk("%d=h %d m %d\n", stream, h264_pic_count, mjpeg_pic_count);
	}

	*total_encoded = mjpeg_pic_count + h264_pic_count + tjpeg_pic_count;
}

static int irq_update_stopping_stream(int stream)
{
	int wake_up = 0;
	iav_encode_format_ex_t *format = &G_encode_stream[stream].format;
	if (__is_stream_in_stopping(stream)) {
		wake_up = 1;
		__rel_source_buffer_ref(format->source, stream);
	} else if (__is_stream_in_encoding(stream)) {
		/* Encode duration is config as non-zero for stream */
		if (format->duration) {
			wake_up = 1;
			__set_enc_op(stream, IAV_ENCODE_OP_STOP);
			__rel_source_buffer_ref(format->source, stream);
		}
	}
	return wake_up;
}

/*
 * This ISR handler is called with the frequency of VOUT0. It will update the
 * status of each stream in codec, correct the "lost" encode interrupts, and
 * notify all waiters pending on the VDSP state in the transition mode (like
 * start / stop encode, enter / leave preview state).
 */
static void handle_vdsp_msg(void *context, unsigned int cat, DSP_MSG *msg, int port)
{
	int stream;
	int wake_up = 0;
	u32 encode_state, encoded_count;
	ENCODER_STATUS_MSG * encoder_status_msg = (ENCODER_STATUS_MSG*) msg;

	stream = get_stream_num_from_type(encoder_status_msg->stream_type);
	if (stream < 0) {
		iav_printk("abnormal stream_type %d\n",encoder_status_msg->stream_type);
		return ;
	}
	if (likely(G_encode_stream[stream].fake_dsp_state == ENC_IDLE_STATE)) {
		encode_state = encoder_status_msg->encode_state;
	} else {
		encode_state = ENC_BUSY_STATE;
	}

	spin_lock(&G_iav_obj.lock);
	irq_update_frame_count(encoder_status_msg, stream, &encoded_count);
	if (encode_state == ENC_BUSY_STATE) {
		if (__is_stream_in_starting(stream)) {
			wake_up = 1;
		}
		/* Fixme:
		 * Cortex has much faster speed to execute VENC ISR. No need to
		 * correct ISR. Add correction when error occurs. */
		if (is_interrupt_readout()) {
			sync_encoded_frame_counter();
		}
	} else if (encode_state == ENC_IDLE_STATE) {
		wake_up = irq_update_stopping_stream(stream);
	}
	__set_dsp_enc_state(stream, encode_state);
	if (unlikely(G_encode_stream[stream].fake_dsp_state == ENC_BUSY_STATE)) {
		G_encode_stream[stream].fake_dsp_state = ENC_IDLE_STATE;
	}
	irq_update_iav_enc_state();
	spin_unlock(&G_iav_obj.lock);

	if (wake_up) {
		wake_up_interruptible(&G_encode_stream[stream].venc_wq);
	}
	return ;
}

/*
 * This ISR handler is called with the frequency of VIN. It will update the
 * statistics data of 3A (CFA and RGB), raw buffer / YUV buffer / ME1 buffer
 * address, and notify all waiters pending on the statistics data.
 */
static void handle_vcap_msg(void *context, unsigned int cat, DSP_MSG *msg, int port)
{
	VCAP_STATUS_MSG * vcap_msg = NULL;
	VCAP_STRM_REPORT * str_rpt = NULL;
	VCAP_STATUS_EXT_MSG * vcap_ext_msg = NULL;
	VCAP_STRM_REPORT_EXT *str_rpt_ext = NULL;
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;
	iav_hwtimer_pts_info_t *hw_pts_info = &pts_info->hw_pts_info;

	u32 complete_flag = 0;
	static u32 last_addr = 0;
	static u32 curr_vcap_mode = VCAP_RESET_MODE;
	static VCAP_STRM_REPORT curr_str_rpt;

	vcap_msg = (VCAP_STATUS_MSG *)msg;
	if(vcap_msg->msg_code == MSG_VCAP_STATUS) {
		str_rpt = &vcap_msg->strm_reports;
		__set_dsp_vcap_mode(vcap_msg->vcap_setup_mode);

		/* Vsync loss handler:
		 * DSP reports "idsp_err_code" when VIN is not stable in preview state
		 * or invalid in entering preview state.
		 */
		if (vcap_msg->idsp_err_code == 1) {
			G_iav_obj.vsync_signal_lost = 1;
			if (!G_iav_obj.vsync_error_handling) {
				iav_printk("Vsync loss detected!\n");
				notify_vsync_loss_waiters(&G_encode_obj);
				return ;
			}
		} else {
			G_iav_obj.vsync_signal_lost = 0;
		}
		if (__is_dsp_vcap_mode(VCAP_VIDEO_MODE) &&
			curr_vcap_mode == VCAP_TIMER_MODE) {
			if (vcap_msg->idsp_err_code == 1) {
				iav_printk("Enable Vsync vsync_error_again!\n");
				G_iav_obj.vsync_error_again = 1;
			} else {
				G_iav_obj.vsync_error_again = 0;
			}
		}
		curr_vcap_mode = vcap_msg->vcap_setup_mode;

		/* Check if DSP enters preview done */
		if (!last_addr && str_rpt->main_pict_luma_addr) {
			complete_flag = 1;
		} else if (G_iav_obj.vsync_error_handling == 1) {
			complete_flag = 1;
		} else {
			complete_flag = 0;
		}

		last_addr = str_rpt->main_pict_luma_addr;
		if (complete_flag) {
			G_iav_obj.vcap_preview_done = 1;
			wake_up_interruptible(&G_iav_obj.vcap_wq);
		}

		amba_imgproc_msg(str_rpt, vcap_msg->vcap_setup_mode);
		// save stream report and use together with next message
		curr_str_rpt = *str_rpt;
	} else if (vcap_msg->msg_code == MSG_VCAP_STATUS_EXT) {
		vcap_ext_msg = (VCAP_STATUS_EXT_MSG *)msg;
		str_rpt_ext = &vcap_ext_msg->strm_report_ext;
		hw_pts_info->audio_tick = str_rpt_ext->vcap_audio_clk_counter;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
		if (str_rpt_ext->vcap_enc_cmd_updata_fail) {
			iav_error("vcap_enc_cmd_updata_fail \n");
		}
#endif
		get_hwtimer_output_ticks(&hw_pts_info->hwtimer_tick);
		if (__is_dsp_vcap_mode(VCAP_VIDEO_MODE) &&
			curr_str_rpt.main_pict_luma_addr) {
			save_dsp_buffers(&curr_str_rpt, str_rpt_ext);
		}
	} else {
		iav_printk("received vcap_msg->msg_code 0x%x \n", vcap_msg->msg_code);
	}

	notify_vcap_msg_waiters(&G_encode_obj);

	return ;
}

static inline int wait_vcap_msg(void)
{
	return iav_wait_compl_interruptible(&G_encode_obj.vcap_msg_compl);
}

void wait_vsync_loss_msg(void)
{
	iav_wait_compl_interruptible(&G_encode_obj.vsync_loss_compl);
}

void wait_vcap_msg_count(int count)
{
	int i;
	for (i = 0; i < count; ++i) {
		wait_vcap_msg();
	}
}

int wait_vcap(int mode, const char *desc)
{
	int rval = 0;
	unsigned long flags;
	struct amba_vin_src_capability * vin_info = get_vin_capability();

	//iav_printk("START: wait_vcap %s, to enter vcap state %d \n", desc, mode);
	while (1) {
		dsp_lock(flags);
		if (mode < 0) {
			break;
		}
		if (__is_dsp_vcap_mode(mode)) {
			break;
		}
		dsp_unlock(flags);
		if (vin_info->sensor_id == ALTERA_FPGAVIN) {
			amba_vin_source_cmd(
				G_iav_info.pvininfo->active_src_id,
				0,
				AMBA_VIN_SRC_RESET,
				NULL);
		}
		if (wait_vcap_msg() < 0) {
			rval = -EINTR;
			dsp_lock(flags);
			break;
		}
	}
	dsp_unlock(flags);
	// iav_printk("DONE: wait_vcap %s, to enter vcap state %d \n", desc, mode);
	return rval;
}

/******************************************
 *
 *	Initialize functions
 *
 ******************************************/

static void reset_encode_obj(void)
{
	int i;
	unsigned long flags;
	struct iav_mem_block *bsb_block = NULL;
	struct iav_bits_desc *bsb_desc = NULL;
	struct iav_stat_desc *stat_desc = NULL;

	iav_irq_save(flags);

	// Initialize counters
	G_encode_obj.total_bits_info_ctr_h264 = 0;
	G_encode_obj.total_bits_info_ctr_mjpeg = 0;
	G_encode_obj.total_bits_info_ctr_tjpeg = 0;
	G_encode_obj.total_pic_encoded_h264_mode = 0;
	G_encode_obj.total_pic_encoded_mjpeg_mode = 0;
	G_encode_obj.h264_pic_counter = 0;
	G_encode_obj.mjpeg_pic_counter = 0;
	G_encode_obj.total_pic_counter = 0;

	// Initialize bits info
	iav_get_mem_block(IAV_MMAP_BSB, &bsb_block);
	G_encode_obj.bits_fifo_next = bsb_block->phys_start;
	G_encode_obj.bits_fifo_fullness = 0;

	// Initialize bits info and extent descriptors
	iav_get_bits_desc(&bsb_desc);
	G_encode_obj.bits_desc_start = bsb_desc->start;
	G_encode_obj.bits_desc_end = bsb_desc->end;
	G_encode_obj.bits_desc_read_ptr = bsb_desc->start;
	G_encode_obj.bits_desc_prefetch = bsb_desc->start;
	G_encode_obj.total_desc = NUM_BS_DESC;
	G_encode_obj.curr_num_desc = 0;
	for (i = 0; i < NUM_BS_DESC; ++i) {
		set_invalid_bits_desc(&bsb_desc->start[i]);
	}
	G_encode_obj.ext_bits_desc_start = bsb_desc->ext_start;
	G_encode_obj.ext_bits_desc_end = bsb_desc->ext_end;
	G_encode_obj.ext_bits_desc_write_ptr = bsb_desc->ext_start;

	// Initialize H264 motion vector descriptors
	iav_get_stat_desc(&stat_desc);
	G_encode_obj.stat_desc_start = stat_desc->start;
	G_encode_obj.stat_desc_end = stat_desc->end;
	G_encode_obj.stat_desc_read_ptr = stat_desc->start;
	for (i = 0; i < NUM_STATIS_DESC; ++i) {
		set_invalid_statis_desc(&stat_desc->start[i]);
	}
	G_encode_obj.ext_statis_desc_start = stat_desc->ext_start;
	G_encode_obj.ext_statis_desc_end = stat_desc->ext_end;
	G_encode_obj.ext_statis_desc_write_ptr = stat_desc->ext_start;

	// Initialize session ID
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		G_stream_session_id[i][0] = 0;
	}

	iav_irq_restore(flags);

	return ;
}

static int start_encode_timer(void)
{
	if (is_polling_readout()) {
		// Init HR timer for BSB polling read out protocol
		if (iav_hrtimer_init(MAX_BSB_POLLING_PERIOD_MS,
					&G_bsb_polling_read) < 0) {
			iav_error("Failed to init HR timer for BSB polling read out!\n");
			return -EIO;
		} else {
			iav_hrtimer_start(&G_bsb_polling_read);
		}
		// Init HR timer for encode statistics polling read out protocol
		if (iav_hrtimer_init(MAX_STATIS_POLLING_PERIOD_MS,
					&G_statis_polling_read) < 0) {
			iav_error("Failed to init HR timer for statistics polling read out!\n");
			return -EIO;
		} else {
			iav_hrtimer_start(&G_statis_polling_read);
		}
	}
	return 0;
}

static int stop_encode_timer(void)
{
	if (is_polling_readout()) {
		iav_hrtimer_deinit(&G_bsb_polling_read);
		iav_hrtimer_deinit(&G_statis_polling_read);
	}
	return 0;
}


/************************************************
 *
 *	Assistant function for calculation
 *
 ***********************************************/

static void fill_h264_vui(iav_context_t *context, int stream,
		H264ENC_VUI_PAR * h264_vui_par)
{
	int buffer_id;
	iav_source_buffer_ex_t* main_buffer =
		&G_iav_obj.source_buffer[IAV_ENCODE_SOURCE_MAIN_BUFFER];
	iav_source_buffer_ex_t* buffer = NULL;
	iav_encode_format_ex_t* stream_format = NULL;

	if (h264_vui_par == NULL)
		return;

	h264_vui_par->vui_enable = 1;
	h264_vui_par->timing_info_present_flag = 1;
	h264_vui_par->fixed_frame_rate_flag = 1;
	h264_vui_par->pic_struct_present_flag = 1;
	h264_vui_par->nal_hrd_parameters_present_flag = 1;
	h264_vui_par->vcl_hrd_parameters_present_flag = 1;
	h264_vui_par->video_signal_type_present_flag = 1;
	h264_vui_par->video_full_range_flag = 1;
	h264_vui_par->video_format = 5;

	//color primaries
	{
		u32 color_primaries = 1;
		u32 transfer_characteristics = 1;
		u32 matrix_coefficients = 1;
		get_colour_primaries(stream, &color_primaries,
			&transfer_characteristics, &matrix_coefficients);
		h264_vui_par->colour_description_present_flag = 1;
		h264_vui_par->colour_primaries = color_primaries;
		h264_vui_par->transfer_characteristics = transfer_characteristics;
		h264_vui_par->matrix_coefficients = matrix_coefficients;
	}

	// add aspect ratio info in SPS
	{
		u8 aspect_ratio_idc = 0;
		u16 sar_width = 1;
		u16 sar_height = 1;
		stream_format = &G_encode_stream[stream].format;
		if (is_warp_mode()) {
			get_aspect_ratio_in_warp_mode(stream_format, &aspect_ratio_idc,
				&sar_width, &sar_height);
		} else {
			buffer_id = stream_format->source;
			buffer = &G_iav_obj.source_buffer[buffer_id];
			get_aspect_ratio(&buffer->size, &buffer->input, &aspect_ratio_idc,
				&sar_width, &sar_height);
			if (is_sub_buf(buffer_id)) {
				get_aspect_ratio(&main_buffer->size, &main_buffer->input,
					&aspect_ratio_idc, &sar_width, &sar_height);
			}
		}
		if (sar_width == sar_height) {
			sar_width = sar_height = 0;
		}
		h264_vui_par->aspect_ratio_info_present_flag = 1;
		h264_vui_par->aspect_ratio_idc = aspect_ratio_idc;
		if (stream_format->rotate_clockwise) {
			h264_vui_par->SAR_width = sar_height;
			h264_vui_par->SAR_height = sar_width;
		} else {
			h264_vui_par->SAR_width = sar_width;
			h264_vui_par->SAR_height = sar_height;
		}
	}

	return ;
}

static void init_jpeg_dqt(u8 *qtbl, int quality)
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

static void init_stream_jpeg_dqt(int stream, int quality)
{
	static int qtbl_idx[IAV_MAX_ENCODE_STREAMS_NUM] = {0};
	iav_jpeg_config_ex_t *config = &G_encode_config[stream].jpeg_encode_config;
	int buf_size, curr_idx;

	curr_idx = qtbl_idx[stream];
	buf_size = IAV_MAX_ENCODE_STREAMS_NUM * JPEG_QT_SIZE;
	config->jpeg_quant_matrix = G_quant_matrix_addr + curr_idx * buf_size +
		stream * JPEG_QT_SIZE;

	init_jpeg_dqt(config->jpeg_quant_matrix, quality);
	qtbl_idx[stream] = (curr_idx + 1) % DEFAULT_TOGGLED_BUFFER_NUM;
}

static void init_q_matrix(int stream, u8 * q)
{
	static int qm_idx[IAV_MAX_ENCODE_STREAMS_NUM] = {0};
	int curr_idx;
	u8 * addr = NULL;

	curr_idx = qm_idx[stream];
	addr = G_q_matrix_addr + (curr_idx * IAV_MAX_ENCODE_STREAMS_NUM +
		stream) * Q_MATRIX_SIZE;
	G_encode_config[stream].h264_encode_config.qmatrix_4x4_daddr = addr;
	memcpy(addr, q, Q_MATRIX_SIZE);
	qm_idx[stream] = (curr_idx + 1) % DEFAULT_TOGGLED_BUFFER_NUM;
}

static int search_nearest(u32 key, u32 arr[], int size, int order)
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	if (order == 0) {	//arr in ascending order
		while (1) {
			if (l == r)
				return l;
			if (key > arr[m]) {
				l = m + 1;
			} else if  (key < arr[m]) {
				r = m;
			} else {
				return m;
			}
			m = (l+r) / 2;
		}
	} else {			//arr in descending order
		while (1) {
			if (l == r)
				return l;
			if (key > arr[m]) {
				r = m;
			} else if  (key < arr[m]) {
				l = m + 1;
			} else {
				return m;
			}
			m = (l+r) / 2;
		}
	}
	return -1;
}

static int calc_target_qp(u32 min_bitrate, u32 resolution, u32 *qp)
{
	int bitrate_index, resolution_index;

	bitrate_index = search_nearest(min_bitrate, rc_br_table,
			ARRAY_SIZE(rc_br_table), 0);
	resolution_index = search_nearest(resolution, rc_reso_table,
			ARRAY_SIZE(rc_reso_table), 1);

	if ((bitrate_index < 0) || (resolution_index < 0)) {
		iav_error("Cannot find valid bitrate and resolution from RC LUT!\n");
		return -1;
	}

	*qp = rc_qp_for_vbr_lut[bitrate_index][resolution_index];
	return 0;
}

static u32 get_dsp_encode_bitrate(int stream)
{
	iav_stream_encode_config_ex_t * enc_config = &G_encode_config[stream];
	u32 ff_multi = enc_config->frame_rate_multiplication_factor;
	u32 ff_division = enc_config->frame_rate_division_factor;
	u32 full_bitrate = enc_config->h264_encode_config.average_bitrate;
	u32 bitrate = 0;

	if (ff_division) {
		bitrate =  full_bitrate * ff_multi / ff_division;
	}

	return bitrate;
}


/******************************************
 *
 *	DSP API functions
 *
 ******************************************/

// 0x02000001
static void cmd_h264_encode_setup(iav_context_t *context, int stream)
{
	s16 offset_y_shift;
	u16 width, height, multi_frames;
	u32 encoder_frame_rate;
	ENCODER_SETUP_CMD dsp_cmd;
	struct iav_mem_block *bsb_block;
	struct iav_bits_desc *bsb_desc;
	struct iav_stat_desc *stat_desc;
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_stream_encode_config_ex_t * config = &G_encode_config[stream];
	iav_h264_config_ex_t * h264_config = &G_encode_config[stream].h264_encode_config;
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_SETUP;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);
	dsp_cmd.profile_idc = get_profile_idc(stream);	// default main profile
	dsp_cmd.level_idc = get_level_idc(stream);		// default 4.0
	dsp_cmd.coding_type = 1; // 0: reserved  1: H.264 encoder

	/* (h.264 only)
	 *  0: disable scaling list.
	 *  1: use default scaling list.
	 *  X>=2: use customized scaling list No.(X-1)
	 */
	if ((HIGH_PROFILE_IDC == dsp_cmd.profile_idc) &&
			h264_config->qmatrix_4x4_enable) {
		dsp_cmd.scalelist_opt = 1;
		dsp_cmd.q_matrix4x4_daddr = VIRT_TO_DSP(h264_config->qmatrix_4x4_daddr);
	}
	dsp_cmd.force_intlc_tb_iframe = 0;
	dsp_cmd.enc_src = get_capture_buffer_id(format->source);
	dsp_cmd.chroma_format = h264_config->chroma_format;

	get_round_encode_format(stream, &width, &height, &offset_y_shift);
	dsp_cmd.encode_w_sz = width;
	dsp_cmd.encode_h_sz = height;
	dsp_cmd.encode_w_ofs = format->encode_x;
	/* Use negative offset to compensate the top & left cropping flag */
	dsp_cmd.encode_h_ofs = format->encode_y + offset_y_shift;
	if (vin_info->video_format == AMBA_VIDEO_FORMAT_INTERLACE)
		dsp_cmd.aff_mode = PAFF_ALL_FLD;
	else
		dsp_cmd.aff_mode = PAFF_ALL_FRM;
	dsp_cmd.M = h264_config->M;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	/* Add extra 4bits for long gop N, total 12bits for long gop N */
	if (h264_config->N > 0xfff) {
		h264_config->N = 0xfff;
	}
	dsp_cmd.N = h264_config->N & 0xff;
	dsp_cmd.N_div_256 = (h264_config->N >> 8) & 0xf;
	dsp_cmd.gop_structure = (h264_config->gop_model == IAV_GOP_LONGTERM_P1B0REF)
		? 5 : h264_config->gop_model;
	dsp_cmd.long_p_interval = h264_config->debug_long_p_interval;
	dsp_cmd.numRef_P = h264_config->numRef_P;
#else
	dsp_cmd.N = h264_config->N < 255 ? h264_config->N : 255;
	dsp_cmd.gop_structure = h264_config->gop_model;
#endif
	dsp_cmd.idr_interval = h264_config->idr_interval;
	dsp_cmd.use_cabac = (h264_config->profile != H264_BASELINE_PROFILE); //0:CAVLC	1: CABAC
	//dsp_cmd.quality_level = calc_quality_model(h264_config);
	dsp_cmd.quality_level = 0x81;
	dsp_cmd.average_bitrate = get_dsp_encode_bitrate(stream);
	dsp_cmd.vbr_cntl = calc_vbr_cntl(h264_config);
	dsp_cmd.vbr_setting = h264_config->bitrate_control;

	// CPB related parameters
	dsp_cmd.cpb_buf_idc = h264_config->cpb_buf_idc;
	dsp_cmd.en_panic_rc = h264_config->en_panic_rc;
	dsp_cmd.cpb_cmp_idc = h264_config->cpb_cmp_idc;
	dsp_cmd.fast_rc_idc = h264_config->fast_rc_idc;
	dsp_cmd.cpb_user_size = h264_config->cpb_user_size;

	iav_get_mem_block(IAV_MMAP_BSB2, &bsb_block);
	dsp_cmd.bits_fifo_base = PHYS_TO_DSP(bsb_block->phys_start);
	dsp_cmd.bits_fifo_limit = PHYS_TO_DSP(bsb_block->phys_end) - 1;
	iav_get_bits_desc(&bsb_desc);
	dsp_cmd.info_fifo_base = VIRT_TO_DSP(bsb_desc->start);
	dsp_cmd.info_fifo_limit = VIRT_TO_DSP(bsb_desc->end) - 1;

	// bits_partition FIFO, 0: DSP allocate its own
	dsp_cmd.num_mbrows_per_bitspart = 0;
	dsp_cmd.bits_partition_base = VIRT_TO_DSP(bsb_desc->bp_start);
	dsp_cmd.bits_partition_limit = VIRT_TO_DSP(bsb_desc->bp_end) - 1;

	dsp_cmd.audio_in_freq = AUDIO_IN_FREQ_48000HZ;
	multi_frames = is_hdr_frame_interleaved_mode() ?
		get_expo_num(get_enc_mode()) : 1;
	dsp_cmd.encode_frame_rate = amba_iav_fps_format_to_vfr(
		vin_info->frame_rate, vin_info->video_format,
		multi_frames);

	// Encoder Statistics descriptors, 0: DSP allocate its own stat FIFO
	iav_get_stat_desc(&stat_desc);
	dsp_cmd.stat_fifo_base =VIRT_TO_DSP(stat_desc->start);
	dsp_cmd.stat_fifo_limit = VIRT_TO_DSP(stat_desc->end) - 1;

	if (calc_encode_frame_rate(vin_info->frame_rate,
		config->frame_rate_multiplication_factor,
		config->frame_rate_division_factor, &encoder_frame_rate) < 0) {
		iav_error("Failed to calculate custom encode fps.\n");
		return ;
	}
	dsp_cmd.custom_encoder_frame_rate = encoder_frame_rate;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, profile_idc);
	iav_dsp(dsp_cmd, level_idc);
	iav_dsp(dsp_cmd, coding_type);
	iav_dsp(dsp_cmd, scalelist_opt);
	iav_dsp_hex(dsp_cmd, q_matrix4x4_daddr);
	iav_dsp(dsp_cmd, enc_src);
	iav_dsp(dsp_cmd, chroma_format);
	iav_dsp(dsp_cmd, encode_w_sz);
	iav_dsp(dsp_cmd, encode_h_sz);
	iav_dsp(dsp_cmd, encode_w_ofs);
	iav_dsp(dsp_cmd, encode_h_ofs);
	iav_dsp(dsp_cmd, aff_mode);
	iav_dsp(dsp_cmd, M);
	iav_dsp(dsp_cmd, N);
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	iav_dsp(dsp_cmd, N_div_256);
	iav_dsp(dsp_cmd, long_p_interval);
	iav_dsp(dsp_cmd, numRef_P);
#endif
	iav_dsp(dsp_cmd, idr_interval);
	iav_dsp(dsp_cmd, gop_structure);
	iav_dsp(dsp_cmd, use_cabac);
	iav_dsp_hex(dsp_cmd, quality_level);
	iav_dsp(dsp_cmd, average_bitrate);
	iav_dsp(dsp_cmd, vbr_cntl);
	iav_dsp(dsp_cmd, vbr_setting);
	iav_dsp_hex(dsp_cmd, cpb_buf_idc);
	iav_dsp_hex(dsp_cmd, en_panic_rc);
	iav_dsp_hex(dsp_cmd, cpb_cmp_idc);
	iav_dsp_hex(dsp_cmd, fast_rc_idc);
	iav_dsp_hex(dsp_cmd, cpb_user_size);
	iav_dsp_hex(dsp_cmd, bits_fifo_base);
	iav_dsp_hex(dsp_cmd, bits_fifo_limit);
	iav_dsp_hex(dsp_cmd, info_fifo_base);
	iav_dsp_hex(dsp_cmd, info_fifo_limit);
	iav_dsp(dsp_cmd, audio_in_freq);
	iav_dsp(dsp_cmd, encode_frame_rate);
	iav_dsp_hex(dsp_cmd, stat_fifo_base);
	iav_dsp_hex(dsp_cmd, stat_fifo_limit);
	iav_dsp(dsp_cmd, num_mbrows_per_bitspart);
	iav_dsp_hex(dsp_cmd, bits_partition_base);
	iav_dsp_hex(dsp_cmd, bits_partition_limit);
	iav_dsp(dsp_cmd, custom_encoder_frame_rate);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x02000002
static void cmd_h264_encode_start(iav_context_t * context, int stream)
{
	int stream_width, stream_height;
	int round_factor, round_width, round_height;
	int margin_w, margin_h;
	int interlaced_video = is_video_encode_interlaced();
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;
	iav_h264_config_ex_t * h264_config = &G_encode_config[stream].h264_encode_config;
	H264ENC_START_CMD dsp_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_START;
	dsp_cmd.stream_type = get_stream_type(stream);
	dsp_cmd.start_encode_frame_no = 0xffffffff;
	dsp_cmd.encode_duration = (format->duration == IAV_ENC_DURATION_FOREVER) ?
		ENCODE_DURATION_FOREVRE : format->duration;
	dsp_cmd.is_flush = 1;
	dsp_cmd.enable_slow_shutter = 0;
	dsp_cmd.res_rate_min = 40;
	dsp_cmd.alpha = h264_config->slice_alpha_c0_offset_div2;
	dsp_cmd.beta = h264_config->slice_beta_offset_div2;
	dsp_cmd.disable_deblocking_filter = h264_config->deblocking_filter_disable;
	dsp_cmd.max_upsampling_rate = 1;
	dsp_cmd.slow_shutter_upsampling_rate = 0;
	dsp_cmd.au_type = h264_config->au_type;
	fill_h264_vui(context, stream, &dsp_cmd.h264_vui_par);

	/* crop pictures when needed for height alignment
	 * hardware needs encoding height to be 16 aligned,
	 * for interlaced video, each field needs to be 16 aligned in height
	 */
	stream_width = G_encode_stream[stream].format.encode_width;
	stream_height = G_encode_stream[stream].format.encode_height;
	round_factor = interlaced_video ? 32 : 16;
	round_height = ALIGN(stream_height, round_factor);
	margin_h = round_height - stream_height;
	margin_h = interlaced_video ? (margin_h >> 2) : (margin_h >> 1);
	round_width = ALIGN(stream_width, PIXEL_IN_MB);
	margin_w = (round_width - stream_width) >> 1;
	if (format->negative_offset_disable) {
		if (format->rotate_clockwise) {
			dsp_cmd.frame_crop_bottom_offset = margin_w;
			if (format->hflip) {
				dsp_cmd.frame_crop_right_offset = margin_h;
			} else {
				dsp_cmd.frame_crop_left_offset = margin_h;
			}
		} else {
			dsp_cmd.frame_crop_right_offset = margin_w;
			if (format->vflip) {
				dsp_cmd.frame_crop_top_offset = margin_h;
			} else {
				dsp_cmd.frame_crop_bottom_offset = margin_h;
			}
		}
	} else {
		if (format->rotate_clockwise) {
			/* Use minus height offset to compensate left cropping flag, so use
			* right cropping flag all the time.
			*/
			dsp_cmd.frame_crop_right_offset = margin_h;
			dsp_cmd.frame_crop_bottom_offset = margin_w;
		} else {
			/* Use minus height offset to compensate top cropping flag, so
			* use bottom cropping flag all the time.
			*/
			dsp_cmd.frame_crop_right_offset = margin_w;
			dsp_cmd.frame_crop_bottom_offset = margin_h;
		}
	}
	dsp_cmd.frame_cropping_flag = 1;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, start_encode_frame_no);
	iav_dsp(dsp_cmd, encode_duration);
	iav_dsp(dsp_cmd, is_flush);
	iav_dsp(dsp_cmd, enable_slow_shutter);
	iav_dsp(dsp_cmd, res_rate_min);
	iav_dsp(dsp_cmd, alpha);
	iav_dsp(dsp_cmd, beta);
	iav_dsp(dsp_cmd, disable_deblocking_filter);
	iav_dsp(dsp_cmd, max_upsampling_rate);
	iav_dsp(dsp_cmd, slow_shutter_upsampling_rate);
	iav_dsp(dsp_cmd, frame_cropping_flag);
	iav_dsp(dsp_cmd, frame_crop_left_offset);
	iav_dsp(dsp_cmd, frame_crop_right_offset);
	iav_dsp(dsp_cmd, frame_crop_top_offset);
	iav_dsp(dsp_cmd, frame_crop_bottom_offset);
	iav_dsp(dsp_cmd, au_type);
	iav_dsp(dsp_cmd, h264_vui_par.aspect_ratio_info_present_flag);
	iav_dsp(dsp_cmd, h264_vui_par.aspect_ratio_idc);
	iav_dsp(dsp_cmd, h264_vui_par.SAR_width);
	iav_dsp(dsp_cmd, h264_vui_par.SAR_height);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x02000003
static void cmd_encode_stop(iav_context_t *context, int stream, int flag)
{
	ENCODER_STOP_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_H264ENC_STOP;
	dsp_cmd.stream_type = get_stream_type(stream);
	dsp_cmd.stop_method = flag;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, stop_method);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x02000004
static void cmd_jpeg_encode_setup(iav_context_t *context, int stream, int is_mjpeg)
{
	s16 offset_y_shift;
	u16 width, height;
	u32 encoder_frame_rate;
	JPEG_ENCODE_SETUP_CMD dsp_cmd;
	struct iav_mem_block *bsb_block;
	struct iav_bits_desc *bsb_desc;
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_stream_encode_config_ex_t * config = &G_encode_config[stream];
	iav_jpeg_config_ex_t * mjpeg = &config->jpeg_encode_config;
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_JPEG_SETUP;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);
	dsp_cmd.chroma_format = mjpeg->chroma_format;

	iav_get_mem_block(IAV_MMAP_BSB2, &bsb_block);
	dsp_cmd.bits_fifo_base = PHYS_TO_DSP(bsb_block->phys_start);
	dsp_cmd.bits_fifo_limit = PHYS_TO_DSP(bsb_block->phys_end) - 1;

	iav_get_bits_desc(&bsb_desc);
	dsp_cmd.info_fifo_base = VIRT_TO_DSP(bsb_desc->start);
	dsp_cmd.info_fifo_limit = VIRT_TO_DSP(bsb_desc->end) - 1;

	init_stream_jpeg_dqt(stream, mjpeg->quality);
	clean_cache_aligned(mjpeg->jpeg_quant_matrix, JPEG_QT_SIZE);
	dsp_cmd.jpeg_qlevel = mjpeg->quality;
	dsp_cmd.quant_matrix_addr = (u32)VIRT_TO_DSP(mjpeg->jpeg_quant_matrix);

	if (calc_encode_frame_rate(vin_info->frame_rate,
		config->frame_rate_multiplication_factor,
		config->frame_rate_division_factor, &encoder_frame_rate) < 0) {
		iav_error("Failed to calculate custom encode fps!\n");
		return ;
	}
	dsp_cmd.custom_encoder_frame_rate = encoder_frame_rate;

	dsp_cmd.is_mjpeg = !!is_mjpeg;
	if (!!is_mjpeg) {
		dsp_cmd.enc_src = get_capture_buffer_id(format->source);
		/*
		 * In High MP full perf mode, JPEG encode comes from MCTF which
		 * requires MB alignment. So use negative offset to avoid artifacts
		 * introduced by V-flip.
		 */
		get_round_encode_format(stream, &width, &height, &offset_y_shift);
		dsp_cmd.encode_w_sz = width;
		dsp_cmd.encode_h_sz = height;
		dsp_cmd.encode_w_ofs = format->encode_x;
		dsp_cmd.encode_h_ofs = format->encode_y + offset_y_shift;
		dsp_cmd.max_enc_loop = 0;
	}
	dsp_cmd.frame_rate_multiplication_factor = config->frame_rate_multiplication_factor;
	dsp_cmd.frame_rate_division_factor = config->frame_rate_division_factor;

	dsp_cmd.target_bpp = 0;
	dsp_cmd.tolerance = (15 << 8) / 100;		/* 15% with 0.8 fix format*/

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, chroma_format);
	iav_dsp_hex(dsp_cmd, bits_fifo_base);
	iav_dsp_hex(dsp_cmd, bits_fifo_limit);
	iav_dsp_hex(dsp_cmd, info_fifo_base);
	iav_dsp_hex(dsp_cmd, info_fifo_limit);
	iav_dsp(dsp_cmd, jpeg_qlevel);
	iav_dsp_hex(dsp_cmd, quant_matrix_addr);
	iav_dsp(dsp_cmd, is_mjpeg);
	iav_dsp(dsp_cmd, enc_src);
	iav_dsp(dsp_cmd, encode_w_sz);
	iav_dsp(dsp_cmd, encode_h_sz);
	iav_dsp(dsp_cmd, encode_w_ofs);
	iav_dsp(dsp_cmd, encode_h_ofs);
	iav_dsp(dsp_cmd, frame_rate_multiplication_factor);
	iav_dsp(dsp_cmd, frame_rate_division_factor);
	iav_dsp(dsp_cmd, custom_encoder_frame_rate);
	iav_dsp(dsp_cmd, target_bpp);
	iav_dsp(dsp_cmd, tolerance);
	iav_dsp(dsp_cmd, max_enc_loop);
	iav_dsp(dsp_cmd, rate_curve_points);
	iav_dsp_hex(dsp_cmd, rate_curve_addr);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0200000B
static void cmd_jpeg_encode_start(iav_context_t * context, int stream)
{
	MJPEG_CAPTURE_CMD dsp_cmd;
	u32 duration = G_encode_stream[stream].format.duration;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_MJPEG_CAPTURE;
	dsp_cmd.stream_type = get_stream_type(stream);
	dsp_cmd.start_encode_frame_no = 0xFFFFFFFF;
	dsp_cmd.encode_duration = (duration == IAV_ENC_DURATION_FOREVER) ?
		ENCODE_DURATION_FOREVRE : duration;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp_hex(dsp_cmd, bits_fifo_next);
	iav_dsp_hex(dsp_cmd, info_fifo_next);
	iav_dsp_hex(dsp_cmd, start_encode_frame_no);
	iav_dsp_hex(dsp_cmd, encode_duration);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0200000C
static void cmd_encode_rotate(iav_context_t *context, int stream)
{
	ENCODER_ROTATE_CMD dsp_cmd;
	iav_encode_format_ex_t * format = NULL;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_ROTATE;
	dsp_cmd.stream_type = get_stream_type(stream);

	format = &G_encode_stream[stream].format;
	dsp_cmd.hflip = format->hflip;
	dsp_cmd.vflip = format->vflip;
	dsp_cmd.rotate = format->rotate_clockwise;

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp(dsp_cmd, hflip);
	iav_dsp(dsp_cmd, vflip);
	iav_dsp(dsp_cmd, rotate);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

// 0x0200000F
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
void cmd_update_encode_params_ex(iav_context_t *context, int stream, int flags)
{
	int i, enable;
	u8 * addr = NULL;
	s16 offset_y_shift = 0;
	u16 width = 0, height = 0;
	u32 dsp_encoder_fps;
	ENCODER_UPDATE_ENC_PARAMETERS_CMD dsp_cmd;
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;
	iav_stream_encode_config_ex_t *config = &G_encode_config[stream];
	iav_stream_privacy_mask_t* pm = &G_encode_stream[stream].pm;
	iav_h264_config_ex_t *h264_config = &config->h264_encode_config;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_UPDATE_ENC_PARAMETERS;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);

	if (flags & UPDATE_FRAME_FACTOR_FLAG) {
		if (calc_encode_frame_rate(get_vin_capability()->frame_rate,
				config->frame_rate_multiplication_factor,
				config->frame_rate_division_factor, &dsp_encoder_fps) < 0) {
			iav_error("Failed to calculate custom encode fps.\n");
			return ;
		}
		dsp_cmd.custom_encoder_frame_rate = dsp_encoder_fps;
		dsp_cmd.multiplication_factor = config->frame_rate_multiplication_factor;
		dsp_cmd.division_factor = config->frame_rate_division_factor;
		dsp_cmd.enable_flags |= UPDATE_FRAME_FACTOR_FLAG;
	}
	if (flags & UPDATE_STREAM_BITRATE_FLAG) {
		dsp_cmd.average_bitrate = get_dsp_encode_bitrate(stream);
		dsp_cmd.enable_flags |= UPDATE_STREAM_BITRATE_FLAG;
	}
	if (flags & UPDATE_STREAM_OFFSET_FLAG) {
		get_round_encode_format(stream, &width, &height, &offset_y_shift);
		dsp_cmd.encode_h_ofs = format->encode_y + offset_y_shift;
		dsp_cmd.encode_w_ofs = format->encode_x;
		dsp_cmd.enable_flags |= UPDATE_STREAM_OFFSET_FLAG;
	}
	if (flags & UPDATE_GOP_PARAM_FLAG) {
		dsp_cmd.N = h264_config->N & 0xff;
		dsp_cmd.N_div_256 = (h264_config->N >> 8) & 0xf;
		dsp_cmd.idr_interval = h264_config->idr_interval;
		dsp_cmd.enable_flags |= UPDATE_GOP_PARAM_FLAG;
	}
	if (flags & UPDATE_SCBR_PARAM_FLAG) {
		dsp_cmd.max_qp_i = h264_config->qp_max_on_I;
		dsp_cmd.max_qp_p = h264_config->qp_max_on_P;
		dsp_cmd.max_qp_b = h264_config->qp_max_on_B;
		dsp_cmd.max_qp_c = h264_config->qp_max_on_C;
		dsp_cmd.max_qp_d = h264_config->qp_max_on_D;

		dsp_cmd.min_qp_i = h264_config->qp_min_on_I;
		dsp_cmd.min_qp_p = h264_config->qp_min_on_P;
		dsp_cmd.min_qp_b = h264_config->qp_min_on_B;
		dsp_cmd.min_qp_c = h264_config->qp_min_on_C;
		dsp_cmd.min_qp_d = h264_config->qp_min_on_D;
		dsp_cmd.aqp = h264_config->adapt_qp;
		dsp_cmd.i_qp_reduce = h264_config->i_qp_reduce;
		dsp_cmd.p_qp_reduce = h264_config->p_qp_reduce;
		dsp_cmd.b_qp_reduce = h264_config->b_qp_reduce;
		dsp_cmd.c_qp_reduce = h264_config->c_qp_reduce;
		dsp_cmd.skip_flags = h264_config->skip_flag;
		dsp_cmd.enable_flags |= UPDATE_SCBR_PARAM_FLAG;
	}
	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		enable = h264_config->qp_roi_enable;
		if (enable) {
			addr = G_qp_matrix_current_daddr;
			// clean cache on QP matrix buffer data
			clean_cache_aligned(addr, STREAM_QP_MATRIX_MEM_SIZE);
			dsp_cmd.roi.roi_daddr = AMBVIRT_TO_DSP(addr);
			if (h264_config->qp_roi_type == QP_ROI_TYPE_ADV) {
				dsp_cmd.user_aqp = IAV_QP_ROI_TYPE_ADV;
			} else {
				dsp_cmd.user_aqp = IAV_QP_ROI_TYPE_BASIC;
			}
		} else {
			dsp_cmd.roi.roi_daddr = 0;
		}
		for (i = 0; i < NUM_PIC_TYPES; ++i) {
			dsp_cmd.roi.roi_delta[i][0] = h264_config->qp_delta[i][0];
			dsp_cmd.roi.roi_delta[i][1] = h264_config->qp_delta[i][1];
			dsp_cmd.roi.roi_delta[i][2] = h264_config->qp_delta[i][2];
			dsp_cmd.roi.roi_delta[i][3] = h264_config->qp_delta[i][3];
		}
		dsp_cmd.user1_intra_bias = h264_config->user1_intra_bias;
		dsp_cmd.user1_direct_bias = h264_config->user1_direct_bias;
		dsp_cmd.user2_intra_bias = h264_config->user2_intra_bias;
		dsp_cmd.user2_direct_bias = h264_config->user2_direct_bias;
		dsp_cmd.user3_intra_bias = h264_config->user3_intra_bias;
		dsp_cmd.user3_direct_bias = h264_config->user3_direct_bias;

		dsp_cmd.enable_flags |= UPDATE_QP_ROI_MATRIX_FLAG;
		/* need update for usr1/2/3 bias*/
	}
	if (flags & UPDATE_FORCE_IDR_FLAG) {
		dsp_cmd.on_demand_IDR = 1;
		dsp_cmd.enable_flags |= UPDATE_FORCE_IDR_FLAG;
	}
	if (flags & UPDATE_INTRA_BIAS_FLAG) {
		dsp_cmd.P_IntraBiasAdd = h264_config->intrabias_P;
		dsp_cmd.B_IntraBiasAdd = h264_config->intrabias_B;
		dsp_cmd.enable_flags |= UPDATE_INTRA_BIAS_FLAG;
	}
	if (flags & UPDATE_P_SKIP_FLAG) {
		dsp_cmd.nonSkipCandidate_bias = h264_config->nonSkipCandidate_bias;
		dsp_cmd.skipCandidate_threshold = h264_config->skipCandidate_threshold;
		dsp_cmd.enable_flags |= UPDATE_P_SKIP_FLAG;
	}
	if (flags & UPDATE_QUANT_MATRIX_FLAG) {
		init_stream_jpeg_dqt(stream, config->jpeg_encode_config.quality);
		addr = config->jpeg_encode_config.jpeg_quant_matrix;
		clean_cache_aligned(addr, JPEG_QT_SIZE);
		dsp_cmd.jpeg_qlevel = config->jpeg_encode_config.quality;
		dsp_cmd.quant_matrix_addr = (u32)VIRT_TO_DSP(addr);
		dsp_cmd.enable_flags |= UPDATE_QUANT_MATRIX_FLAG;
	}
	if (flags & UPDATE_MONOCHROME_FLAG) {
		if (__is_enc_h264(stream)) {
			dsp_cmd.is_monochrome = h264_config->chroma_format;
		} else if (__is_enc_mjpeg(stream)) {
			if (config->jpeg_encode_config.chroma_format ==
					IAV_JPEG_CHROMA_FORMAT_MONO) {
				dsp_cmd.is_monochrome = 1;
			} else {
				dsp_cmd.is_monochrome = 0;
			}
		}
		dsp_cmd.enable_flags |= UPDATE_MONOCHROME_FLAG;
	}
	if (is_high_mp_full_perf_mode() && (flags & UPDATE_STREAM_PM_FLAG)) {
		if (pm->enable) {
			addr = (u8*)pm_user_to_phy_addr(context, pm->buffer_addr);
			dsp_cmd.mctf_privacy_mask_daddr = PHYS_TO_DSP(addr);
		}
		dsp_cmd.mctf_privacy_mask_dpitch = pm->buffer_pitch;
		dsp_cmd.mctf_privacy_mask_Y = pm->y;
		dsp_cmd.mctf_privacy_mask_U = pm->u;
		dsp_cmd.mctf_privacy_mask_V = pm->v;
		dsp_cmd.enable_flags |= UPDATE_STREAM_PM_FLAG;
	}
	if (flags & UPDATE_PANIC_PARAM_FLAG) {
		dsp_cmd.panic_num = h264_config->panic_num;
		dsp_cmd.panic_den = h264_config->panic_den;
		dsp_cmd.enable_flags |= UPDATE_PANIC_PARAM_FLAG;
	}
	if (flags & UPDATE_FRAME_DROP_FLAG) {
		dsp_cmd.frame_drop = config->frame_drop;
		dsp_cmd.enable_flags |= UPDATE_FRAME_DROP_FLAG;
	}
	if (flags & UPDATE_CPB_RESET_FLAG) {
		dsp_cmd.cpb_underflow_num = h264_config->cpb_underflow_num;
		dsp_cmd.cpb_underflow_den = h264_config->cpb_underflow_den;
		dsp_cmd.enable_flags |= UPDATE_CPB_RESET_FLAG;
	}
	if (flags & UPDATE_ZMV_THRESHOLD_FLAG) {
		dsp_cmd.zmv_threshold = h264_config->zmv_threshold;
		dsp_cmd.enable_flags |= UPDATE_ZMV_THRESHOLD_FLAG;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp_hex(dsp_cmd, enable_flags);
	if (flags & UPDATE_FRAME_FACTOR_FLAG) {
		iav_dsp(dsp_cmd, multiplication_factor);
		iav_dsp(dsp_cmd, division_factor);
		iav_dsp(dsp_cmd, custom_encoder_frame_rate);
	}
	if (flags & UPDATE_FORCE_IDR_FLAG) {
		iav_dsp(dsp_cmd, on_demand_IDR);
	}
	if (flags & UPDATE_STREAM_BITRATE_FLAG) {
		iav_dsp(dsp_cmd, average_bitrate);
	}
	if (flags & UPDATE_STREAM_OFFSET_FLAG) {
		iav_dsp(dsp_cmd, encode_w_ofs);
		iav_dsp(dsp_cmd, encode_h_ofs);
	}
	if (flags & UPDATE_GOP_PARAM_FLAG) {
		iav_dsp(dsp_cmd, N);
		iav_dsp(dsp_cmd, N_div_256);
		iav_dsp(dsp_cmd, idr_interval);
	}
	if (flags & UPDATE_SCBR_PARAM_FLAG) {
		iav_dsp(dsp_cmd, max_qp_i);
		iav_dsp(dsp_cmd, min_qp_i);
		iav_dsp(dsp_cmd, max_qp_p);
		iav_dsp(dsp_cmd, min_qp_p);
		iav_dsp(dsp_cmd, max_qp_b);
		iav_dsp(dsp_cmd, min_qp_b);
		iav_dsp(dsp_cmd, max_qp_c);
		iav_dsp(dsp_cmd, min_qp_c);
		iav_dsp(dsp_cmd, max_qp_d);
		iav_dsp(dsp_cmd, min_qp_d);
		iav_dsp(dsp_cmd, aqp);
		iav_dsp(dsp_cmd, i_qp_reduce);
		iav_dsp(dsp_cmd, p_qp_reduce);
		iav_dsp(dsp_cmd, b_qp_reduce);
		iav_dsp(dsp_cmd, c_qp_reduce);
		iav_dsp(dsp_cmd, skip_flags);
	}
	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		iav_dsp(dsp_cmd, user_aqp);
		iav_dsp_hex(dsp_cmd, roi.roi_daddr);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][3]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][3]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][3]);
		iav_dsp(dsp_cmd, user1_intra_bias);
		iav_dsp(dsp_cmd, user1_direct_bias);
		iav_dsp(dsp_cmd, user2_intra_bias);
		iav_dsp(dsp_cmd, user2_direct_bias);
		iav_dsp(dsp_cmd, user3_intra_bias);
		iav_dsp(dsp_cmd, user3_direct_bias);
	}
	if (flags & UPDATE_INTRA_BIAS_FLAG) {
		iav_dsp(dsp_cmd, P_IntraBiasAdd);
		iav_dsp(dsp_cmd, B_IntraBiasAdd);
	}
	if (flags & UPDATE_P_SKIP_FLAG) {
		iav_dsp(dsp_cmd, nonSkipCandidate_bias);
		iav_dsp(dsp_cmd, skipCandidate_threshold);
	}
	if (flags & UPDATE_QUANT_MATRIX_FLAG) {
		iav_dsp(dsp_cmd, jpeg_qlevel);
		iav_dsp_hex(dsp_cmd, quant_matrix_addr);
	}
	if (flags & UPDATE_MONOCHROME_FLAG) {
		iav_dsp(dsp_cmd, is_monochrome);
	}
	if (is_high_mp_full_perf_mode() && (flags & UPDATE_STREAM_PM_FLAG)) {
		iav_dsp_hex(dsp_cmd, mctf_privacy_mask_daddr);
		iav_dsp(dsp_cmd, mctf_privacy_mask_dpitch);
		iav_dsp(dsp_cmd, mctf_privacy_mask_Y);
		iav_dsp(dsp_cmd, mctf_privacy_mask_U);
		iav_dsp(dsp_cmd, mctf_privacy_mask_V);
	}
	if (flags & UPDATE_PANIC_PARAM_FLAG) {
		iav_dsp(dsp_cmd, panic_num);
		iav_dsp(dsp_cmd, panic_den);
	}
	if (flags & UPDATE_FRAME_DROP_FLAG) {
		iav_dsp(dsp_cmd, frame_drop);
	}
	if (flags & UPDATE_CPB_RESET_FLAG) {
		iav_dsp(dsp_cmd, cpb_underflow_num);
		iav_dsp(dsp_cmd, cpb_underflow_den);
	}
	if (flags & UPDATE_ZMV_THRESHOLD_FLAG) {
		iav_dsp(dsp_cmd, zmv_threshold);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}

#else

void cmd_update_encode_params_ex(iav_context_t *context, int stream, int flags)
{
	int i, enable;
	u8 * addr = NULL;
	s16 offset_y_shift = 0;
	u16 width = 0, height = 0;
	u32 dsp_encoder_fps;
	ENCODER_UPDATE_ENC_PARAMETERS_CMD dsp_cmd;
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;
	iav_stream_encode_config_ex_t *config = &G_encode_config[stream];
	iav_stream_privacy_mask_t* pm = &G_encode_stream[stream].pm;
	iav_h264_config_ex_t *h264_config = &config->h264_encode_config;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_UPDATE_ENC_PARAMETERS;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);

	if (flags & UPDATE_FRAME_FACTOR_FLAG) {
		if (calc_encode_frame_rate(get_vin_capability()->frame_rate,
				config->frame_rate_multiplication_factor,
				config->frame_rate_division_factor, &dsp_encoder_fps) < 0) {
			iav_error("Failed to calculate custom encode fps.\n");
			return ;
		}
		dsp_cmd.custom_encoder_frame_rate = dsp_encoder_fps;
		dsp_cmd.multiplication_factor = config->frame_rate_multiplication_factor;
		dsp_cmd.division_factor = config->frame_rate_division_factor;
		dsp_cmd.enable_flags |= UPDATE_FRAME_FACTOR_FLAG;
	}
	if (flags & UPDATE_STREAM_BITRATE_FLAG) {
		dsp_cmd.average_bitrate = get_dsp_encode_bitrate(stream);
		dsp_cmd.enable_flags |= UPDATE_STREAM_BITRATE_FLAG;
	}
	if (flags & UPDATE_STREAM_OFFSET_FLAG) {
		get_round_encode_format(stream, &width, &height, &offset_y_shift);
		dsp_cmd.encode_h_ofs = format->encode_y + offset_y_shift;
		dsp_cmd.encode_w_ofs = format->encode_x;
		dsp_cmd.enable_flags |= UPDATE_STREAM_OFFSET_FLAG;
	}
	if (flags & UPDATE_GOP_PARAM_FLAG) {
		dsp_cmd.N = h264_config->N & 0xff;
		dsp_cmd.idr_interval = h264_config->idr_interval;
		dsp_cmd.enable_flags |= UPDATE_GOP_PARAM_FLAG;
	}
	if (flags & UPDATE_SCBR_PARAM_FLAG) {
		dsp_cmd.max_qp_i = h264_config->qp_max_on_I;
		dsp_cmd.max_qp_p = h264_config->qp_max_on_P;
		dsp_cmd.max_qp_b = h264_config->qp_max_on_B;
		dsp_cmd.max_qp_c = h264_config->qp_max_on_C;
		dsp_cmd.max_qp_d = h264_config->qp_max_on_D;
		dsp_cmd.max_qp_q = h264_config->qp_max_on_Q;
		dsp_cmd.min_qp_i = h264_config->qp_min_on_I;
		dsp_cmd.min_qp_p = h264_config->qp_min_on_P;
		dsp_cmd.min_qp_b = h264_config->qp_min_on_B;
		dsp_cmd.min_qp_c = h264_config->qp_min_on_C;
		dsp_cmd.min_qp_d = h264_config->qp_min_on_D;
		dsp_cmd.min_qp_q = h264_config->qp_min_on_Q;
		dsp_cmd.aqp = h264_config->adapt_qp;
		dsp_cmd.i_qp_reduce = h264_config->i_qp_reduce;
		dsp_cmd.p_qp_reduce = h264_config->p_qp_reduce;
		dsp_cmd.b_qp_reduce = h264_config->b_qp_reduce;
		dsp_cmd.c_qp_reduce = h264_config->c_qp_reduce;
		dsp_cmd.q_qp_reduce = h264_config->q_qp_reduce;
		dsp_cmd.skip_flags = h264_config->skip_flag;
		dsp_cmd.log_q_num_per_gop_minus_1 = h264_config->log_q_num_minus_1;
		dsp_cmd.enable_flags |= UPDATE_SCBR_PARAM_FLAG;
	}
	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		enable = h264_config->qp_roi_enable;
		if (enable) {
			addr = G_qp_matrix_current_daddr;
			// clean cache on QP matrix buffer data
			clean_cache_aligned(addr, STREAM_QP_MATRIX_MEM_SIZE);
			dsp_cmd.roi.roi_daddr = AMBVIRT_TO_DSP(addr);
			if (h264_config->qp_roi_type == QP_ROI_TYPE_ADV) {
				dsp_cmd.user_aqp = IAV_QP_ROI_TYPE_ADV;
#ifndef  DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
				addr = G_qp_matrix_current_daddr +
					STREAM_QP_MATRIX_MEM_SIZE *
					QP_FRAME_P / QP_FRAME_TYPE_NUM;
				dsp_cmd.roi_daddr_p_pic = AMBVIRT_TO_DSP(addr);
				addr = G_qp_matrix_current_daddr +
					STREAM_QP_MATRIX_MEM_SIZE *
					QP_FRAME_B / QP_FRAME_TYPE_NUM;
				dsp_cmd.roi_daddr_b_pic = AMBVIRT_TO_DSP(addr);
#endif
			} else {
				dsp_cmd.user_aqp = IAV_QP_ROI_TYPE_BASIC;
#ifndef  DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
				dsp_cmd.roi_daddr_p_pic = dsp_cmd.roi.roi_daddr;
				dsp_cmd.roi_daddr_b_pic = dsp_cmd.roi.roi_daddr;
#endif
			}
		} else {
			dsp_cmd.roi.roi_daddr = 0;
		}
		for (i = 0; i < NUM_PIC_TYPES; ++i) {
			dsp_cmd.roi.roi_delta[i][0] = h264_config->qp_delta[i][0];
			dsp_cmd.roi.roi_delta[i][1] = h264_config->qp_delta[i][1];
			dsp_cmd.roi.roi_delta[i][2] = h264_config->qp_delta[i][2];
			dsp_cmd.roi.roi_delta[i][3] = h264_config->qp_delta[i][3];
		}
		dsp_cmd.enable_flags |= UPDATE_QP_ROI_MATRIX_FLAG;
	}
	if (flags & UPDATE_FORCE_IDR_FLAG) {
		dsp_cmd.on_demand_IDR = 1;
		dsp_cmd.enable_flags |= UPDATE_FORCE_IDR_FLAG;
	}
	if (flags & UPDATE_INTRA_BIAS_FLAG) {
		dsp_cmd.P_IntraBiasAdd = h264_config->intrabias_P;
		dsp_cmd.B_IntraBiasAdd = h264_config->intrabias_B;
		dsp_cmd.enable_flags |= UPDATE_INTRA_BIAS_FLAG;
	}
	if (flags & UPDATE_P_SKIP_FLAG) {
		dsp_cmd.nonSkipCandidate_bias = h264_config->nonSkipCandidate_bias;
		dsp_cmd.skipCandidate_threshold = h264_config->skipCandidate_threshold;
		dsp_cmd.enable_flags |= UPDATE_P_SKIP_FLAG;
	}
	if (flags & UPDATE_QUANT_MATRIX_FLAG) {
		init_stream_jpeg_dqt(stream, config->jpeg_encode_config.quality);
		addr = config->jpeg_encode_config.jpeg_quant_matrix;
		clean_cache_aligned(addr, JPEG_QT_SIZE);
		dsp_cmd.jpeg_qlevel = config->jpeg_encode_config.quality;
		dsp_cmd.quant_matrix_addr = (u32)VIRT_TO_DSP(addr);
		dsp_cmd.enable_flags |= UPDATE_QUANT_MATRIX_FLAG;
	}
	if (flags & UPDATE_MONOCHROME_FLAG) {
		if (__is_enc_h264(stream)) {
			dsp_cmd.is_monochrome = h264_config->chroma_format;
		} else if (__is_enc_mjpeg(stream)) {
			if (config->jpeg_encode_config.chroma_format ==
					IAV_JPEG_CHROMA_FORMAT_MONO) {
				dsp_cmd.is_monochrome = 1;
			} else {
				dsp_cmd.is_monochrome = 0;
			}
		}
		dsp_cmd.enable_flags |= UPDATE_MONOCHROME_FLAG;
	}
	if (is_high_mp_full_perf_mode() && (flags & UPDATE_STREAM_PM_FLAG)) {
		if (pm->enable) {
			addr = (u8*)pm_user_to_phy_addr(context, pm->buffer_addr);
			dsp_cmd.mctf_privacy_mask_daddr = PHYS_TO_DSP(addr);
		}
		dsp_cmd.mctf_privacy_mask_dpitch = pm->buffer_pitch;
		dsp_cmd.mctf_privacy_mask_Y = pm->y;
		dsp_cmd.mctf_privacy_mask_U = pm->u;
		dsp_cmd.mctf_privacy_mask_V = pm->v;
		dsp_cmd.enable_flags |= UPDATE_STREAM_PM_FLAG;
	}
	if (flags & UPDATE_PANIC_PARAM_FLAG) {
		dsp_cmd.panic_num = h264_config->panic_num;
		dsp_cmd.panic_den = h264_config->panic_den;
		dsp_cmd.enable_flags |= UPDATE_PANIC_PARAM_FLAG;
	}
	if (flags & UPDATE_FRAME_DROP_FLAG) {
		dsp_cmd.frame_drop = config->frame_drop;
		dsp_cmd.enable_flags |= UPDATE_FRAME_DROP_FLAG;
	}
	if (flags & UPDATE_CPB_RESET_FLAG) {
		dsp_cmd.cpb_underflow_num = h264_config->cpb_underflow_num;
		dsp_cmd.cpb_underflow_den = h264_config->cpb_underflow_den;
		dsp_cmd.enable_flags |= UPDATE_CPB_RESET_FLAG;
	}
	if (flags & UPDATE_ZMV_THRESHOLD_FLAG) {
		dsp_cmd.zmv_threshold = h264_config->zmv_threshold;
		dsp_cmd.enable_flags |= UPDATE_ZMV_THRESHOLD_FLAG;
	}
	if (flags & UPDATE_MODE_BIAS_FLAG) {
		dsp_cmd.modeBias_I4Add = h264_config->mode_bias_I4Add;
		dsp_cmd.modeBias_I16Add = h264_config->mode_bias_I16Add;
		dsp_cmd.modeBias_Inter8Add = h264_config->mode_bias_Inter8Add;
		dsp_cmd.modeBias_Inter16Add = h264_config->mode_bias_Inter16Add;
		dsp_cmd.modeBias_DirectAdd = h264_config->mode_bias_DirectAdd;
		dsp_cmd.enable_flags |= UPDATE_MODE_BIAS_FLAG;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp_hex(dsp_cmd, enable_flags);
	if (flags & UPDATE_FRAME_FACTOR_FLAG) {
		iav_dsp(dsp_cmd, multiplication_factor);
		iav_dsp(dsp_cmd, division_factor);
		iav_dsp(dsp_cmd, custom_encoder_frame_rate);
	}
	if (flags & UPDATE_FORCE_IDR_FLAG) {
		iav_dsp(dsp_cmd, on_demand_IDR);
	}
	if (flags & UPDATE_STREAM_BITRATE_FLAG) {
		iav_dsp(dsp_cmd, average_bitrate);
	}
	if (flags & UPDATE_STREAM_OFFSET_FLAG) {
		iav_dsp(dsp_cmd, encode_w_ofs);
		iav_dsp(dsp_cmd, encode_h_ofs);
	}
	if (flags & UPDATE_GOP_PARAM_FLAG) {
		iav_dsp(dsp_cmd, N);
		iav_dsp(dsp_cmd, idr_interval);
	}
	if (flags & UPDATE_SCBR_PARAM_FLAG) {
		iav_dsp(dsp_cmd, max_qp_i);
		iav_dsp(dsp_cmd, min_qp_i);
		iav_dsp(dsp_cmd, max_qp_p);
		iav_dsp(dsp_cmd, min_qp_p);
		iav_dsp(dsp_cmd, max_qp_b);
		iav_dsp(dsp_cmd, min_qp_b);
		iav_dsp(dsp_cmd, max_qp_c);
		iav_dsp(dsp_cmd, min_qp_c);
		iav_dsp(dsp_cmd, max_qp_d);
		iav_dsp(dsp_cmd, min_qp_d);
		iav_dsp(dsp_cmd, max_qp_q);
		iav_dsp(dsp_cmd, min_qp_q);
		iav_dsp(dsp_cmd, aqp);
		iav_dsp(dsp_cmd, i_qp_reduce);
		iav_dsp(dsp_cmd, p_qp_reduce);
		iav_dsp(dsp_cmd, b_qp_reduce);
		iav_dsp(dsp_cmd, c_qp_reduce);
		iav_dsp(dsp_cmd, q_qp_reduce);
		iav_dsp(dsp_cmd, skip_flags);
		iav_dsp(dsp_cmd, log_q_num_per_gop_minus_1);
	}
	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		iav_dsp(dsp_cmd, user_aqp);
		iav_dsp_hex(dsp_cmd, roi.roi_daddr);
#ifndef  DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
		iav_dsp_hex(dsp_cmd, roi_daddr_p_pic);
		iav_dsp_hex(dsp_cmd, roi_daddr_b_pic);
#endif
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[0][3]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[1][3]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][0]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][1]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][2]);
		iav_dsp_s32(dsp_cmd, roi.roi_delta[2][3]);
	}
	if (flags & UPDATE_INTRA_BIAS_FLAG) {
		iav_dsp(dsp_cmd, P_IntraBiasAdd);
		iav_dsp(dsp_cmd, B_IntraBiasAdd);
	}
	if (flags & UPDATE_P_SKIP_FLAG) {
		iav_dsp(dsp_cmd, nonSkipCandidate_bias);
		iav_dsp(dsp_cmd, skipCandidate_threshold);
	}
	if (flags & UPDATE_QUANT_MATRIX_FLAG) {
		iav_dsp(dsp_cmd, jpeg_qlevel);
		iav_dsp_hex(dsp_cmd, quant_matrix_addr);
	}
	if (flags & UPDATE_MONOCHROME_FLAG) {
		iav_dsp(dsp_cmd, is_monochrome);
	}
	if (is_high_mp_full_perf_mode() && (flags & UPDATE_STREAM_PM_FLAG)) {
		iav_dsp_hex(dsp_cmd, mctf_privacy_mask_daddr);
		iav_dsp(dsp_cmd, mctf_privacy_mask_dpitch);
		iav_dsp(dsp_cmd, mctf_privacy_mask_Y);
		iav_dsp(dsp_cmd, mctf_privacy_mask_U);
		iav_dsp(dsp_cmd, mctf_privacy_mask_V);
	}
	if (flags & UPDATE_PANIC_PARAM_FLAG) {
		iav_dsp(dsp_cmd, panic_num);
		iav_dsp(dsp_cmd, panic_den);
	}
	if (flags & UPDATE_FRAME_DROP_FLAG) {
		iav_dsp(dsp_cmd, frame_drop);
	}
	if (flags & UPDATE_CPB_RESET_FLAG) {
		iav_dsp(dsp_cmd, cpb_underflow_num);
		iav_dsp(dsp_cmd, cpb_underflow_den);
	}
	if (flags & UPDATE_ZMV_THRESHOLD_FLAG) {
		iav_dsp(dsp_cmd, zmv_threshold);
	}
	if (flags & UPDATE_MODE_BIAS_FLAG) {
		iav_dsp(dsp_cmd, modeBias_I4Add);
		iav_dsp(dsp_cmd, modeBias_I16Add);
		iav_dsp(dsp_cmd, modeBias_Inter8Add);
		iav_dsp(dsp_cmd, modeBias_Inter16Add);
		iav_dsp(dsp_cmd, modeBias_DirectAdd);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}
#endif

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
/******************************************
 *
 *	encode with sync frame
 *
 ******************************************/
// 0x0200000F
static int G_frame_sync_index = 1;
static u8 * G_frame_sync_map = NULL;

static inline u8* get_dummy_enc_kernel_address(int index, int stream_id)
{
	u8 *  addr = NULL;
	addr = (G_frame_sync_map + (IAV_MAX_ENCODE_STREAMS_NUM *
		G_frame_sync_index + stream_id) * DSP_CMD_SIZE);

	return addr;
}

static int update_sync_frame_enc_params_ex(iav_context_t *context,
	int stream, int flags)
{
	int i, enable;
	u8 * addr = NULL;

	iav_stream_encode_config_ex_t *config = &G_encode_config[stream];
	iav_h264_config_ex_t *h264_config = &config->h264_encode_config;

	ENCODER_UPDATE_ENC_PARAMETERS_CMD *sync_dsp_cmd =
		(ENCODER_UPDATE_ENC_PARAMETERS_CMD *)
		get_dummy_enc_kernel_address(G_frame_sync_index, stream);

	sync_dsp_cmd->cmd_code = CMD_H264ENC_UPDATE_ENC_PARAMETERS;
	sync_dsp_cmd->channel_id = 0;
	sync_dsp_cmd->stream_type = get_stream_type(stream);

	if (flags & UPDATE_GOP_PARAM_FLAG) {
		sync_dsp_cmd->N = h264_config->N & 0xff;
		sync_dsp_cmd->N_div_256 = (h264_config->N >> 8) & 0xf;
		sync_dsp_cmd->idr_interval = h264_config->idr_interval;
		sync_dsp_cmd->enable_flags |= UPDATE_GOP_PARAM_FLAG;
	}

	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		enable = h264_config->qp_roi_enable;
		if (enable) {
			addr = G_qp_matrix_current_daddr;
			// clean cache on QP matrix buffer data
			clean_cache_aligned(addr, STREAM_QP_MATRIX_MEM_SIZE);
			sync_dsp_cmd->roi.roi_daddr = AMBVIRT_TO_DSP(addr);
			if (h264_config->qp_roi_type == QP_ROI_TYPE_ADV) {
				sync_dsp_cmd->user_aqp = IAV_QP_ROI_TYPE_ADV;
			} else {
				sync_dsp_cmd->user_aqp = IAV_QP_ROI_TYPE_BASIC;
				for (i = 0; i < NUM_PIC_TYPES; ++i) {
					sync_dsp_cmd->roi.roi_delta[i][0] = h264_config->qp_delta[i][0];
					sync_dsp_cmd->roi.roi_delta[i][1] = h264_config->qp_delta[i][1];
					sync_dsp_cmd->roi.roi_delta[i][2] = h264_config->qp_delta[i][2];
					sync_dsp_cmd->roi.roi_delta[i][3] = h264_config->qp_delta[i][3];
				}
			}
		} else {
			sync_dsp_cmd->roi.roi_daddr = 0;
		}
		sync_dsp_cmd->user1_intra_bias = h264_config->user1_intra_bias;
		sync_dsp_cmd->user1_direct_bias = h264_config->user1_direct_bias;
		sync_dsp_cmd->user2_intra_bias = h264_config->user2_intra_bias;
		sync_dsp_cmd->user2_direct_bias = h264_config->user2_direct_bias;
		sync_dsp_cmd->user3_intra_bias = h264_config->user3_intra_bias;
		sync_dsp_cmd->user3_direct_bias = h264_config->user3_direct_bias;

		sync_dsp_cmd->enable_flags |= UPDATE_QP_ROI_MATRIX_FLAG;
	}
	if (flags & UPDATE_FORCE_IDR_FLAG) {
		sync_dsp_cmd->on_demand_IDR = 1;
		sync_dsp_cmd->enable_flags |= UPDATE_FORCE_IDR_FLAG;
	}
	if (flags & UPDATE_INTRA_BIAS_FLAG) {
		sync_dsp_cmd->P_IntraBiasAdd = h264_config->intrabias_P;
		sync_dsp_cmd->B_IntraBiasAdd = h264_config->intrabias_B;
		sync_dsp_cmd->enable_flags |= UPDATE_INTRA_BIAS_FLAG;
	}
	if (flags & UPDATE_P_SKIP_FLAG) {
		sync_dsp_cmd->nonSkipCandidate_bias = h264_config->nonSkipCandidate_bias;
		sync_dsp_cmd->skipCandidate_threshold = h264_config->skipCandidate_threshold;
		sync_dsp_cmd->enable_flags |= UPDATE_P_SKIP_FLAG;
	}

	if (flags & UPDATE_ZMV_THRESHOLD_FLAG) {
		sync_dsp_cmd->zmv_threshold = h264_config->zmv_threshold;
		sync_dsp_cmd->enable_flags |= UPDATE_ZMV_THRESHOLD_FLAG;
	}

	if (flags & UPDATE_QP_ROI_MATRIX_FLAG) {
		iav_printk("Set:Qp matrix Phys addr of stream[%d] : daddr:0x%x\n",
			stream,  AMBVIRT_TO_DSP(G_qp_matrix_current_daddr));
	}

	if (flags & UPDATE_FRAME_DROP_FLAG) {
		sync_dsp_cmd->frame_drop = config->frame_drop;
		sync_dsp_cmd->enable_flags |= UPDATE_FRAME_DROP_FLAG;
	}
	return 0;
}

static int cmd_apply_frame_sync_cmd(u32 pts)
{
	u8 * addr = NULL;
	int i = 0;
	CMD_VCAP_UPDATE_ENC_PARAMETERS_CMD dsp_cmd;
	ENCODER_UPDATE_ENC_PARAMETERS_CMD * cmd_data = NULL;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_UPDATEA_ENC_PARAMETERS;
	dsp_cmd.target_pts = pts;

	addr = get_dummy_enc_kernel_address(G_frame_sync_index, 0);
	dsp_cmd.cmd_daddr = AMBVIRT_TO_DSP(addr);

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i ++, addr += DSP_CMD_SIZE) {
		cmd_data = (ENCODER_UPDATE_ENC_PARAMETERS_CMD *) addr;
		if (cmd_data->cmd_code) {
			dsp_cmd.cmd_daddr_enable_flag |= 1 << i;
		}
	}

	if (!dsp_cmd.cmd_daddr_enable_flag) {
		iav_printk("No frame need sync\n");
		return -1;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp_hex(dsp_cmd, target_pts);
	iav_dsp_hex(dsp_cmd, cmd_daddr);
	iav_dsp_hex(dsp_cmd, cmd_daddr_enable_flag);
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}
#else
static int update_sync_frame_enc_params_ex(iav_context_t *context,
	int stream, int flags)
{
	iav_error("%s is not supported yet.\n", __func__);
	return 0;
}
#endif

// 0x2000010
static void cmd_enable_encoder_statistics(iav_context_t * context,
		int stream, int flags)
{
	ENCODER_ENABLE_ENC_STATS_CMD dsp_cmd;
	iav_stream_statis_config_ex_t * statis = &G_encode_stream[stream].statis;
	struct iav_mem_block *block;
	u32 unit_size, mvb_size;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_ENABLE_ENC_STATS;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = get_stream_type(stream);

	if (flags & ENABLE_MVDUMP_FLAG) {
		iav_get_mem_block(IAV_MMAP_MV, &block);
		unit_size = statis->mvdump_unit_size;
		// make MV buffer total size be an integer multiple of unit size
		mvb_size = block->size / unit_size * unit_size;
		dsp_cmd.mvdump_division_factor = statis->mvdump_division_factor;
		dsp_cmd.mvdump_dpitch = statis->mvdump_pitch;
		dsp_cmd.mvdump_fifo_unit_sz = unit_size;
		dsp_cmd.mvdump_fifo_base = PHYS_TO_DSP(block->phys_start);
		dsp_cmd.mvdump_fifo_limit = PHYS_TO_DSP(block->phys_start) +
			mvb_size - 1;
		dsp_cmd.enable_flags |= ENABLE_MVDUMP_FLAG;
	}
	if (flags & ENABLE_QP_HIST_DUMP_FLAG) {
		iav_get_mem_block(IAV_MMAP_QP_HIST, &block);
		dsp_cmd.qp_hist_fifo_base = PHYS_TO_DSP(block->phys_start) +
			stream * NUM_SD_PER_STREAM * sizeof(iav_qp_hist_ex_t);
		dsp_cmd.qp_hist_fifo_limit = dsp_cmd.qp_hist_fifo_base +
			NUM_SD_PER_STREAM * sizeof(iav_qp_hist_ex_t) - 1;
		dsp_cmd.qp_hist_fifo_unit_sz = sizeof(iav_qp_hist_ex_t);
		dsp_cmd.enable_flags |= ENABLE_QP_HIST_DUMP_FLAG;
	}

	iav_dsp_hex(dsp_cmd, cmd_code);
	iav_dsp(dsp_cmd, channel_id);
	iav_dsp_hex(dsp_cmd, stream_type);
	iav_dsp_hex(dsp_cmd, enable_flags);
	if (flags & ENABLE_MVDUMP_FLAG) {
		iav_dsp(dsp_cmd, mvdump_division_factor);
		iav_dsp(dsp_cmd, mvdump_dpitch);
		iav_dsp(dsp_cmd, mvdump_fifo_unit_sz);
		iav_dsp_hex(dsp_cmd, mvdump_fifo_base);
		iav_dsp_hex(dsp_cmd, mvdump_fifo_limit);
	}
	if (flags & ENABLE_QP_HIST_DUMP_FLAG) {
		iav_dsp_hex(dsp_cmd, qp_hist_fifo_base);
		iav_dsp_hex(dsp_cmd, qp_hist_fifo_limit);
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}


/******************************************
 *
 *	Internal IAV Helper functions
 *
 ******************************************/

static int map_addr_dsp_to_user(iav_context_t * context, u32 mmap, u32 * paddr)
{
	u32 addr, start_addr;
	struct iav_addr_map * map = NULL;
	struct iav_mem_block * block = NULL;

	addr = DSP_TO_PHYS(*paddr);
	switch (mmap) {
	case IAV_MMAP_BSB:
	case IAV_MMAP_BSB2:
		map = &context->bsb;
		break;
	case IAV_MMAP_MV:
		map = &context->mv;
		break;
	case IAV_MMAP_QP_HIST:
		map = &context->qp_hist;
		break;
	default:
		iav_error("Invalid mmap type %d.\n", mmap);
		return -EINVAL;
		break;
	}
	iav_get_mem_block(mmap, &block);

	start_addr = addr - block->phys_start + (u32)map->user_start;
	if ((u8 *)start_addr < map->user_start ||
			(u8 *)start_addr >= map->user_end) {
		iav_error("DSP = 0x%x, addr = 0x%08x, phys [0x%08x, 0x%08x], "
			"user [0x%08x, 0x%08x].\n", *paddr, addr, block->phys_start,
			block->phys_end, (u32)map->user_start, (u32)map->user_end);
		return -EIO;
	}
	*paddr = start_addr;
	return 0;
}

static int get_qp_limit(int stream, iav_change_qp_limit_ex_t *qp_limit)
{
	iav_h264_config_ex_t * h264 = &G_encode_config[stream].h264_encode_config;
	qp_limit->qp_max_on_I = h264->qp_max_on_I;
	qp_limit->qp_max_on_P = h264->qp_max_on_P;
	qp_limit->qp_max_on_B = h264->qp_max_on_B;
	qp_limit->qp_max_on_C = h264->qp_max_on_C;
	qp_limit->qp_max_on_Q = h264->qp_max_on_Q;
	qp_limit->qp_min_on_I = h264->qp_min_on_I;
	qp_limit->qp_min_on_P = h264->qp_min_on_P;
	qp_limit->qp_min_on_B = h264->qp_min_on_B;
	qp_limit->qp_min_on_C = h264->qp_min_on_C;
	qp_limit->qp_min_on_Q = h264->qp_min_on_Q;
	qp_limit->adapt_qp = h264->adapt_qp;
	qp_limit->i_qp_reduce = h264->i_qp_reduce;
	qp_limit->p_qp_reduce = h264->p_qp_reduce;
	qp_limit->b_qp_reduce = h264->b_qp_reduce;
	qp_limit->q_qp_reduce = h264->q_qp_reduce;
	qp_limit->skip_flag = h264->skip_flag;
	qp_limit->log_q_num_minus_1 = h264->log_q_num_minus_1;
	qp_limit->id = (1 << stream);

	return 0;
}

static int check_encode_stream_state(iav_stream_id_t stream_id_to_encode)
{
	int i, encoding_counter, max_num;
	u32 chip_id;

	if (__is_invalid_stream_id(stream_id_to_encode))
		return -EINVAL;

	encoding_counter = 0;
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id_to_encode & (1 << i)) {
			if (__is_enc_none(i)) {
				iav_error("Encode type is NOT set for stream %d.\n", i);
				return -EINVAL;
			}
			if (!__is_stream_ready_for_encode(i)) {
				iav_error("Stream %d is not ready or already encoding.\n", i);
				return -EAGAIN;
			}
			/* Fixme: 3rd buffer is not available in dewarp mode */
			if (is_warp_mode() &&
				(G_encode_stream[i].format.source == IAV_ENCODE_SOURCE_THIRD_BUFFER) &&
				is_buf_type_enc(IAV_ENCODE_SOURCE_THIRD_BUFFER)) {
				iav_error("3rd buffer is NOT available in multi dewarp mode. "
					"Please change encode source for stream [%d].\n", i);
				return -EINVAL;
			}
			++encoding_counter;
		} else {
			if (__is_stream_in_encoding(i) || __is_stream_in_transition(i)) {
				++encoding_counter;
			}
		}
	}
	max_num = get_max_enc_num(get_enc_mode());
	if (encoding_counter > max_num) {
		iav_error("Total encode stream num [%d] is more than max"
			" supported num [%d].\n", encoding_counter, max_num);
		return -EINVAL;
	}

	if (unlikely(G_iav_obj.dsp_chip_id == IAV_CHIP_ID_S2_UNKNOWN)) {
		G_iav_obj.dsp_chip_id = dsp_get_chip_id();
	}
	chip_id = G_iav_obj.dsp_chip_id;
	if (encoding_counter > G_system_load[chip_id].max_enc_num) {
		iav_error("Total encode stream num [%d] is more than max"
			" supported num [%d] for S2 chip [%d].\n", encoding_counter,
			G_system_load[chip_id].max_enc_num, chip_id);
		return -EINVAL;
	}

	return 0;
}

static int check_encode_resource_limit(iav_stream_id_t encode_stream_id,
		u32 vin_fps)
{
	int i;
	u32 chip_id, system_load_limit, system_load, load;
	u32 system_bitrate, bitrate;
	u32 vin_frame_rate = DIV_ROUND(512000000, vin_fps);
	iav_encode_format_ex_t * format = NULL;
	iav_enc_mode_limit_t * mode_limit = NULL;

	if (unlikely(G_iav_obj.dsp_chip_id == IAV_CHIP_ID_S2_UNKNOWN)) {
		G_iav_obj.dsp_chip_id = dsp_get_chip_id();
	}
	chip_id = G_iav_obj.dsp_chip_id;
	if (chip_id >= IAV_CHIP_ID_S2_LAST) {
		iav_error("Invalid S2 chip ID [%d].\n", chip_id);
		chip_id = IAV_CHIP_ID_S2_99;
	}
	system_load = system_bitrate = 0;
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (__is_stream_in_starting(i) || __is_stream_in_stopping(i) ||
			__is_stream_in_encoding(i) || (encode_stream_id & (1 << i))) {
			format = &G_encode_stream[i].format;
			load = ALIGN(format->encode_width, 16) / 16 *
				ALIGN(format->encode_height, 16) / 16 *
				vin_frame_rate *
				G_encode_config[i].frame_rate_multiplication_factor /
				G_encode_config[i].frame_rate_division_factor;
			system_load += load;
			if (__is_enc_h264(i)) {
				bitrate = G_encode_config[i].h264_encode_config.average_bitrate *
					G_encode_config[i].frame_rate_multiplication_factor /
					G_encode_config[i].frame_rate_division_factor;
				system_bitrate += bitrate;
			}
		}
	}

	iav_printk("Check encode resource limit : VIN [%d], system load %d.\n",
		vin_fps, system_load);

	system_load_limit = G_system_load[chip_id].system_load;
	if (system_load > system_load_limit) {
		iav_error("Total system load %d exceed maximum %d (%s).\n",
			system_load, system_load_limit, G_system_load[chip_id].desc);
		return -1;
	}
	mode_limit = &G_modes_limit[get_enc_mode()];
	system_load_limit = mode_limit->max_encode_MB;
	if (system_load > system_load_limit) {
		iav_error("Total system load %d exceed maximum %d in mode [%s].\n",
			system_load, system_load_limit, mode_limit->name);
		return -1;
	}

	if (system_bitrate > IAV_MAX_ENCODE_BITRATE) {
		iav_error("Total system bitrate [%d] exceed maximum [%d].\n",
			system_bitrate, IAV_MAX_ENCODE_BITRATE);
		return -1;
	}

	return 0;
}

static int check_buffer_config_limit(iav_stream_id_t stream_id)
{
	int i, buffer_id;
	iav_source_buffer_ex_t* source_buffer = G_iav_obj.source_buffer;
	DSP_SET_OP_MODE_IPCAM_RECORD_CMD * resource = G_iav_obj.system_resource;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			buffer_id = G_encode_stream[i].format.source;

			if (!is_buf_type_enc(buffer_id)) {
				iav_error("Please config the source buffer [%d] type for "
					"encoding first!\n", buffer_id);
				return -1;
			}
			if (source_buffer[buffer_id].enc_stop) {
				iav_error("Please config the source buffer [%d] to be "
					"non-zero first!\n", buffer_id);
				return -1;
			}
			switch (buffer_id) {
			case IAV_ENCODE_SOURCE_SECOND_BUFFER:
				if ((resource->max_preview_C_width == 0) ||
						(resource->max_preview_C_height == 0)) {
					iav_error("Source buffer [%d] is disabled, cannot start "
						"encoding from it!\n", buffer_id);
					return -1;
				}
				break;
			case IAV_ENCODE_SOURCE_THIRD_BUFFER:
				if ((resource->max_preview_B_width == 0) ||
						(resource->max_preview_B_height == 0)) {
					iav_error("Source buffer [%d] is disabled, cannot start "
						"encoding from it!\n", buffer_id);
					return -1;
				}
				break;
			case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
				if ((resource->max_preview_A_width == 0) ||
						(resource->max_preview_A_height == 0)) {
					iav_error("Source buffer [%d] is disabled, cannot start "
						"encoding from it!\n", buffer_id);
					return -1;
				}
				break;
			case IAV_ENCODE_SOURCE_MAIN_DRAM:
				if (__is_enc_h264(i) &&
					(source_buffer[buffer_id].dram.valid_frame_num < IAV_MIN_ENC_DRAM_FRAME_NUM) ) {
						iav_warning("DRAM buffer [%d] doesn't have enough frames, "
							"start encode may be blocked until feeding enough"
							" frames.\n", buffer_id);
				}
				break;
			}
		}
	}

	return 0;
}

static int check_encode_offset(iav_encode_format_ex_t *format)
{
	u32 buf_w, buf_h;
	iav_reso_ex_t* buffer = NULL;

	buffer = &G_iav_obj.source_buffer[format->source].size;
	if ((format->encode_x & 0x1) || (format->encode_y & 0x3)) {
		iav_error("Stream offset x %d must be even, y %d must be multiple of 4.\n",
			format->encode_x, format->encode_y);
		return -1;
	}

	buf_w = buffer->width;
	buf_h = buffer->height * get_vin_num(get_enc_mode());
	if (((format->encode_width + format->encode_x) > buf_w) ||
			((format->encode_height + format->encode_y) > buf_h)) {
		iav_error("Stream size %dx%d with offset %dx%d is out of "
				"source buffer [%d] %dx%d.\n",
				format->encode_width, format->encode_height,
				format->encode_x, format->encode_y, format->source,
				buf_w, buf_h);
		return -1;
	}
	return 0;
}

static int check_stream_offset(iav_stream_offset_t* offset, int stream_id)
{
	u32 buf_w, buf_h;
	iav_encode_format_ex_t* format = &G_iav_obj.stream[stream_id].format;
	iav_reso_ex_t* buffer = &G_iav_obj.source_buffer[format->source].size;
	if ((offset->x & 0x1) || (offset->y & 0x3)) {
		iav_error("Stream offset x %d must be even, y %d must be multiple"
			" of 4.\n", offset->x, offset->y);
		return -1;
	}
	buf_w = buffer->width;
	buf_h = buffer->height * get_vin_num(get_enc_mode());
	if (((format->encode_width + offset->x) > buf_w) ||
		((format->encode_height + offset->y) > buf_h)) {
		iav_error("Stream size %dx%d with offset %dx%d is out of source buffer"
			" [%d] %dx%d.\n", format->encode_width, format->encode_height,
			offset->x, offset->y, format->source, buf_w, buf_h);
		return -1;
	}
	return 0;
}

static int check_h264_config(int stream, iav_h264_config_ex_t *config)
{
#define LONG_TERM_P_INTERVAL_MAX		0x3f
	DSP_ENC_CFG *enc_cfg = NULL;
	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}

	enc_cfg = get_enc_cfg(get_enc_mode(), stream);

	if ((config->bitrate_control < IAV_BRC_MODE_FIRST) ||
			(config->bitrate_control >= IAV_BRC_MODE_LAST)) {
		iav_error("Invalid rate control mode [%d].\n", config->bitrate_control);
		return -1;
	}
	if((config->slice_alpha_c0_offset_div2 < SLICE_ALPHA_VALUE_MIN) ||
			(config->slice_alpha_c0_offset_div2 > SLICE_ALPHA_VALUE_MAX)) {
		iav_error("Invalid slice alpha param [%d], not in the range [%d~%d].\n",
			config->slice_alpha_c0_offset_div2, SLICE_ALPHA_VALUE_MIN,
			SLICE_ALPHA_VALUE_MAX);
		return -1;
	}
	if((config->slice_beta_offset_div2 < SLICE_BETA_VALUE_MIN) ||
			(config->slice_beta_offset_div2 > SLICE_BETA_VALUE_MAX)) {
		iav_error("Invalid slice beta param [%d], not in the range [%d~%d].\n",
			config->slice_beta_offset_div2, SLICE_BETA_VALUE_MIN,
			SLICE_BETA_VALUE_MAX);
		return -1;
	}
	if((config->deblocking_filter_disable != 0) && (config->deblocking_filter_disable != 1)) {
		iav_error("Invalid deblocking disable param [%d], must be 0 | 1.\n",
			config->deblocking_filter_disable);
		return -1;
	}
	if (config->entropy_codec > 1) {
		iav_error("Invalid entropy codec type [%d].\n", config->entropy_codec);
		return -1;
	}
	if (config->entropy_codec && config->high_profile) {
		iav_error("'CAVLC' and 'high-profile(CABAC) cannot be set together!\n");
		return -1;
	}
	if (config->profile < H264_PROFILE_FIRST ||
			config->profile >= H264_PROFILE_LAST) {
		iav_error("Invalid profile param [%d], not in the range [%d~%d].\n",
			config->profile, H264_PROFILE_FIRST, H264_PROFILE_LAST);
		return -1;
	}
	if ((config->profile != H264_HIGH_PROFILE) && config->qmatrix_4x4_enable) {
		iav_error("Cannot enable Q matrix 4x4 when H264 is not in high profile.\n");
		return -1;
	}
	if (!config->panic_num || config->panic_num > 255 ||
		!config->panic_den || config->panic_den > 255) {
		iav_error("Invalid Panic param [%d/%d], not in the range [1~255].\n",
			config->panic_num, config->panic_den);
		return -1;
	}
	if (!config->cpb_underflow_num || !config->cpb_underflow_den) {
		iav_error("CPB underflow ratio [%d/%d] cannot be 0.\n",
			config->cpb_underflow_num, config->cpb_underflow_den);
		return -1;
	}
	if (!config->M || !config->N) {
		iav_error("Invalid M [%d], N [%d]. M and N must be non-zero value!\n",
			config->M, config->N);
		return -1;
	}
	if (config->N % config->M != 0) {
		iav_error("Invalid M [%d] / N [%d]. N must be multiple of M!\n",
			config->M, config->N);
		return -1;
	}
	if (config->M > MAX_B_FRAME_NUM) {
		iav_error("Invalid M [%d], must be no larger than %d.\n",
			config->M, MAX_B_FRAME_NUM);
		return -1;
	}
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if (config->N > 0xfff) {
		iav_error("Invalid N [%d], must be smaller than 4096.\n", config->N);
		return -1;
	}
	if (config->gop_model >= IAV_GOP_SVCT_FIRST &&
		config->gop_model < IAV_GOP_SVCT_LAST) {
		if (!is_svc_t_enabled()) {
			iav_error("Invalid gop num [%d], must be 0 | 1.\n",
				config->gop_model);
			return -1;
		}
		if (config->M != 1) {
			iav_error("Invalid M [%d] with gop num[%d], must be 1.\n",
				config->M, config->gop_model);
			return -1;
		}

		if ((config->N & ((1<<(config->gop_model - 1)) - 1)) != 0) {
			iav_error("Invalid N [%d], gop [%d]. N must be multiple of 2^(gop - 1)!\n",
				config->N, config->gop_model);
			return -1;
		}

		if (config->numRef_P > enc_cfg->max_num_ref_p) {
			iav_error("Invalid numRef_P [%d] in gop [%d], can't greater than %d.\n",
			config->numRef_P, config->gop_model, enc_cfg->max_num_ref_p);
			return -1;
		}
	} else if (config->gop_model == IAV_GOP_LONGTERM_P1B0REF) {
		/* 6bits for debug_long_p_interval */
		if (config->debug_long_p_interval > LONG_TERM_P_INTERVAL_MAX) {
			iav_error("Invalid debug_long_p_interval [%d],"
				" the value must be [0 ~ %d] in gop [%d]!\n",
				config->debug_long_p_interval, LONG_TERM_P_INTERVAL_MAX,
				config->gop_model);
			return -1;
		}
		if (config->M != 1) {
			iav_error("Invalid M [%d] with gop num[%d], must be 1.\n",
				config->M, config->gop_model);
			return -1;
		}
		if (config->debug_long_p_interval >= config->N) {
			iav_error("Invalid long-p-interval[%d], N[%d]: "
				"long-p-interval must be smaller than N!\n",
				config->debug_long_p_interval, config->N);
			return -1;
		}

		if (config->numRef_P > enc_cfg->max_num_ref_p) {
			iav_error("Invalid numRef_P [%d], can't greater than %d.\n",
				config->numRef_P, enc_cfg->max_num_ref_p);
			return -1;
		}
	}else if (config->gop_model >= IAV_GOP_SVCT_LAST) {
		iav_error("Invalid gop num [%d], must be 0~4 in enc-mode 0 "
			"or 0|1 in other enc-mode.\n", config->gop_model);
		return -1;
	}
#else
	if (config->N > 255) {
		iav_error("Invalid N [%d], must be smaller than 256.\n", config->N);
		return -1;
	}
	if (config->gop_model >= IAV_GOP_SVCT_FIRST &&
		config->gop_model < IAV_GOP_SVCT_LAST) {
		if (!is_svc_t_enabled()) {
			iav_error("Invalid gop num [%d], must be 0 | 1.\n",
				config->gop_model);
			return -1;
		}
		if (config->M != 1) {
			iav_error("Invalid M [%d] with gop num[%d], must be 1.\n",
				config->M, config->gop_model);
			return -1;
		}

		if ((config->N & ((1<<(config->gop_model - 1)) - 1)) != 0) {
			iav_error("Invalid N [%d], gop [%d]. N must be multiple of 2^(gop - 1)!\n",
				config->N, config->gop_model);
			return -1;
		}
	} else if (config->gop_model >= IAV_GOP_SVCT_LAST) {
		iav_error("Invalid gop num [%d], must be 0~4 in enc-mode 0 "
			"or 0|1 in other enc-mode.\n", config->gop_model);
		return -1;
	}
#endif

	if ((config->chroma_format != IAV_H264_CHROMA_FORMAT_YUV420) &&
			(config->chroma_format != IAV_H264_CHROMA_FORMAT_MONO)) {
		iav_error("Only support YUV420 and Monochrome H264!\n");
		return -1;
	}

	if (config->M > enc_cfg->max_GOP_M) {
		iav_error("Invalid M %d is larger than %d. Please increase the stream "
			"max M by IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX\n",
			config->M, enc_cfg->max_GOP_M);
		return -1;
	}

	return 0;
}

static int check_stream_config_limit(iav_stream_id_t stream_id)
{
	int i;
	u16 enc_w, enc_h;
	u16 rotate_flag;
	u16 enc_mode = get_enc_mode();
	iav_h264_config_ex_t * config = NULL;
	iav_encode_format_ex_t * format = NULL;
	iav_enc_mode_limit_t * limit = &G_modes_limit[enc_mode];
	DSP_ENC_CFG * enc_cfg = NULL;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			format = &G_encode_stream[i].format;
			rotate_flag = format->rotate_clockwise;

			// check rotate limit
			if (!is_rotate_possible_enabled() && rotate_flag) {
				iav_error("Cannot rotate stream [%d] when rotate_possible"
					" is disabled!\n", i);
				return -1;
			}
			if (rotate_flag) {
				enc_w = format->encode_height;
				enc_h = format->encode_width;
			} else {
				enc_w = format->encode_width;
				enc_h = format->encode_height;
			}

			// check stream size lower limit
			if ((enc_w < limit->min_encode_width) ||
				(enc_h < limit->min_encode_height)) {
				iav_error("Stream size %dx%d cannot be smaller than %dx%d.\n",
					enc_w, enc_h, limit->min_encode_width, limit->min_encode_height);
				return -1;
			}

			// check stream offset with source buffer size
			if (check_encode_offset(format) < 0) {
				return -1;
			}

			// check max stream size in system resource limit
			enc_cfg = get_enc_cfg(enc_mode, i);
			if ((enc_w > enc_cfg->max_enc_width) ||
				(enc_h > enc_cfg->max_enc_height)) {
				iav_error("Stream [%d] encoding size %dx%d is bigger than max "
					"size %dx%d. Please increase the stream max size by "
					"IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX\n", i, enc_w, enc_h,
					enc_cfg->max_enc_width, enc_cfg->max_enc_height);
				return -1;
			}

			// check max GOP M in system resource limit
			if (__is_enc_h264(i)) {
				config = &G_encode_config[i].h264_encode_config;
				if (check_h264_config(i, config) < 0) {
					iav_error("Failed check_h264_config \n");
					return -1;
				}
			} else {
				if (enc_w & 0xF) {
					iav_error("MJPEG encode width %d (rotate : %d) must be multiple of 16.\n",
						enc_w, rotate_flag);
					return -1;
				}
			}
		}
	}

	return 0;
}

static int check_start_encode_state(iav_context_t * context,
	iav_stream_id_t stream_id)
{
	int retv = 0;
	iav_no_check();

	// check IAV state
	if (!is_iav_state_prev_or_enc()) {
		iav_error("Can't start encode, not in preview state, please enable preview first\n");
		return -EPERM;
	}

	if ((retv = check_encode_stream_state(stream_id)) < 0) {
		return retv;
	}

	return 0;
}

static int check_before_start_encode(iav_context_t * context,
	iav_stream_id_t stream_id)
{
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_no_check();

	// check total encode performance limit
	if (check_encode_resource_limit(stream_id, vin_info->frame_rate) < 0) {
		iav_error("Cannot start encode, system resource is not enough.\n");
		return -EINVAL;
	}

	// check buffer config limit
	if (check_buffer_config_limit(stream_id) < 0) {
		iav_error("Cannot start encode, invalid buffer configuration!\n");
		return -EINVAL;
	}

	// check stream config limit
	if (check_stream_config_limit(stream_id) < 0) {
		iav_error("Cannot start encode, invalid stream configuration!\n");
		return -EINVAL;
	}

	return 0;
}

static int check_stop_encode_state(iav_stream_id_t * total_stream_id)
{
	int i, stream_id;

	iav_no_check();

	stream_id = *total_stream_id;
	if (__is_invalid_stream_id(stream_id))
		return -EFAULT;

	if (!is_iav_state_encoding()) {
		iav_error("Can't stop encode, not in encoding state!\n");
		return -EPERM;
	}

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			if (!__is_stream_in_encoding(i)) {
				iav_printk("stream %d is already stopped!\n", i);
				stream_id &= (~(1 << i));
				continue;
			}
		}
	}
	*total_stream_id = stream_id;

	return 0;
}

static int check_framerate_factor_ex(int stream,
		iav_change_framerate_factor_ex_t * framerate)
{
#define	Q9_BASE_1FPS	(512000)
	u32 real_fps = get_vin_capability()->frame_rate;

	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
	if (framerate->ratio_numerator > framerate->ratio_denominator) {
		iav_error("Can't change frame rate by %u/%u, faster than VIN frame rate.\n",
				framerate->ratio_numerator, framerate->ratio_denominator);
		return -1;
	}
	if ((framerate->ratio_numerator > 255) ||
			(framerate->ratio_numerator == 0) ||
			(framerate->ratio_denominator > 255) ||
			(framerate->ratio_denominator == 0)) {
		iav_error("Stream [%d]: can't change frame rate by %u/%u.\n", stream,
				framerate->ratio_numerator, framerate->ratio_denominator);
		return -1;
	}
	real_fps = real_fps * framerate->ratio_denominator / framerate->ratio_numerator;
	if (real_fps > Q9_BASE_1FPS * 1002) {
		/* Add 0.1% margin for the frame rate calculation */
		iav_error("Stream [%d]: can't change fps less than 1.\n", stream);
		return -1;
	}

	return 0;
}

static inline int check_sync_in_batch_cmd(u32 flag)
{
	/* Fixme: stream 0 is out of sync when sharpen B is off in mode 7 */
	if (unlikely(is_full_fps_full_perf_mode() &&
			!is_sharpen_b_enabled() && flag)) {
		iav_error("Stream [0] is out of sync when sharpen B is "
			"off in mode [Full FPS (FP)].\n");
		return -1;
	}
	return 0;
}

static int check_encode_format(int stream, iav_encode_format_ex_t * format)
{
	int rotate_flag;
	u16 type, width, height, source;

	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
	source = format->source;
	type = format->encode_type;
	width = format->encode_width;
	height = format->encode_height;
	rotate_flag = format->rotate_clockwise;
	if (type == IAV_ENCODE_MJPEG) {
		if (rotate_flag && ((width & 0x7) || (height & 0xF))) {
			iav_error("MJPEG [%d] with rotation, height [%d] must be multiple"
					" of 16 and width [%d] must be multiple of 8.\n",
					stream, height, width);
			return -1;
		} else if (!rotate_flag && ((width & 0xF) || (height & 0x7))) {
			iav_error("MJPEG [%d] width [%d] must be multiple of 16 and height"
					" [%d] must be multiple of 8.\n", stream, width, height);
			return -1;
		}
	} else if (type == IAV_ENCODE_H264) {
		if ((width & 0xF) || (height & 0x7)) {
			iav_error("H264 stream [%d], width [%d] must be multiple of 16 and"
					" height [%d] must be multiple of 8.\n", stream, width, height);
			return -1;
		}
	} else {
		iav_error("Invalid encode type [%d] for stream [%d]!\n", type, stream);
		return -1;
	}
	if (is_valid_dram_buffer_id(source)) {
		if (!is_enc_from_dram_enabled()) {
			iav_error("Cannot encode stream [%d] from DRAM (%d). It's not "
				"supported in mode [%s]!\n", stream, source,
				G_modes_limit[get_enc_mode()].name);
			return -1;
		}
		if (!G_iav_obj.source_buffer[source].dram.max_frame_num) {
			iav_error("DRAM buffer pool is not allocated yet. Please set"
				" \"max_dram_frame\" to non-zero in system resource.\n");
			return -1;
		}
	}
	if (!is_rotate_possible_enabled() && rotate_flag) {
		iav_error("Cannot rotate stream [%d] when rotate_possible"
			" is disabled!\n", stream);
		return -1;
	}
	if ((format->duration != IAV_ENC_DURATION_FOREVER) &&
		((format->duration < IAV_ENC_DURATION_MIN) ||
		(format->duration > IAV_ENC_DURATION_MAX))) {
		iav_error("Invalid encode duration [%d], must be in the range of "
			"[0, %d~%d].\n", format->duration, IAV_ENC_DURATION_MIN,
			IAV_ENC_DURATION_MAX);
		return -1;
	}
	return 0;
}

static int check_jpeg_config(int stream, iav_jpeg_config_ex_t *config)
{
	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
	if ((config->chroma_format != IAV_JPEG_CHROMA_FORMAT_YUV420) &&
			(config->chroma_format != IAV_JPEG_CHROMA_FORMAT_MONO)) {
		iav_error("Only support YUV420 and Monochrome MJPEG!\n");
		return -1;
	}
	if ((config->quality > 100) || (config->quality == 0)) {
		iav_error("MJPEG quality must be in the range of 1~100!\n");
		return -1;
	}
	return 0;
}

static int check_chroma_format(int stream, u8 chroma_format)
{
	if (__is_enc_h264(stream)) {
		if (chroma_format != IAV_H264_CHROMA_FORMAT_MONO &&
				chroma_format != IAV_H264_CHROMA_FORMAT_YUV420) {
			iav_error("Unsupported chroma format [%d] for H264.\n",
					chroma_format);
			return -1;
		}
	} else if (__is_enc_mjpeg(stream)) {
		if (chroma_format != IAV_JPEG_CHROMA_FORMAT_MONO &&
				chroma_format != IAV_JPEG_CHROMA_FORMAT_YUV420) {
			iav_error("Unsupported chroma format [%d] for MJPEG.\n",
					chroma_format);
			return -1;
		}
	} else {
		iav_error("Unsupported encode type!\n");
		return -1;
	}
	return 0;
}

static int check_frame_drop(int stream, u16 frame_drop)
{
	if (frame_drop > MAX_FRAME_DROP_COUNT) {
		iav_error("frame_drop [%d] can not be greater than [%d]!\n", frame_drop,
			MAX_FRAME_DROP_COUNT);
		return -1;
	}
	return 0;
}

static inline int check_qp_limit(int stream, iav_change_qp_limit_ex_t *qp_limit)
{
	iav_h264_config_ex_t * h264 = NULL;

	h264 = &G_encode_config[stream].h264_encode_config;

	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if((h264->qp_max_on_Q != qp_limit->qp_max_on_Q) ||
		(h264->qp_min_on_Q != qp_limit->qp_min_on_Q) ||
		(h264->q_qp_reduce != qp_limit->q_qp_reduce) ||
		(h264->log_q_num_minus_1 != qp_limit->log_q_num_minus_1)) {
		iav_error("It doesn't support setting in debug mode\n");
		return -1;
	}
#endif
	if ((qp_limit->qp_max_on_I > H264_QP_MAX) ||
			(qp_limit->qp_min_on_I > qp_limit->qp_max_on_I) ||
			(qp_limit->qp_max_on_P > H264_QP_MAX) ||
			(qp_limit->qp_min_on_P > qp_limit->qp_max_on_P) ||
			(qp_limit->qp_max_on_B > H264_QP_MAX) ||
			(qp_limit->qp_min_on_B > qp_limit->qp_max_on_B) ||
			(qp_limit->qp_max_on_C > H264_QP_MAX) ||
			(qp_limit->qp_min_on_C > qp_limit->qp_max_on_C) ||
			(qp_limit->qp_max_on_Q > H264_QP_MAX) ||
			(qp_limit->qp_min_on_Q > qp_limit->qp_max_on_Q) ||
			(qp_limit->adapt_qp > H264_AQP_MAX) ||
			(qp_limit->i_qp_reduce < H264_I_QP_REDUCE_MIN) ||
			(qp_limit->i_qp_reduce > H264_I_QP_REDUCE_MAX) ||
			(qp_limit->q_qp_reduce < H264_I_QP_REDUCE_MIN) ||
			(qp_limit->q_qp_reduce > H264_I_QP_REDUCE_MAX) ||
			(qp_limit->p_qp_reduce < H264_P_QP_REDUCE_MIN) ||
			(qp_limit->p_qp_reduce > H264_P_QP_REDUCE_MAX) ||
			(qp_limit->b_qp_reduce < H264_P_QP_REDUCE_MIN) ||
			(qp_limit->b_qp_reduce > H264_P_QP_REDUCE_MAX) ||
			(qp_limit->log_q_num_minus_1 > H264_LOG_Q_NUM_MAX)) {
		iav_error("Invalid QP limit, out of range!\n");
		return -1;
	}

	return 0;
}

static int check_qp_matrix_param(int stream, iav_qp_roi_matrix_ex_t * qp_matrix)
{
	int i, mb_width, mb_height, total_size;
	u32 roi_data;
	u32 *start_addr = NULL;

	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}

	start_addr = (u32 *)(G_qp_matrix_current_daddr);
	mb_width = ALIGN(G_encode_stream[stream].format.encode_width, 16) / 16;
	mb_height = ALIGN(G_encode_stream[stream].format.encode_height, 16) /16;
	total_size = ALIGN(mb_width, 8) * mb_height;

	switch (qp_matrix->type) {
	case QP_ROI_TYPE_ADV:
		for (i = 0; i < total_size; ++i) {
			/* bit8~15 is for adnvance QP ROI type	*/
			roi_data = (start_addr[i] >> 8) & 0xff;
			if (roi_data < QP_MATRIX_VALUE_MIN_ADV ||
				roi_data > QP_MATRIX_VALUE_MAX_ADV) {
				iav_error("Invalid QP matrix value [%d] of element [%d] on "
					"stream [%d] with qp roi advance type.\n", start_addr[i], i, stream);
				return -1;
			}
		}
		break;
	case QP_ROI_TYPE_BASIC:
		for (i = 0; i < total_size; ++i) {
			/* bit0~1 is for base QP ROI type	*/
			roi_data = start_addr[i] & 0x3;
			if (roi_data < QP_MATRIX_VALUE_MIN ||
				roi_data > QP_MATRIX_VALUE_MAX) {
				iav_error("Invalid QP matrix value [%d] of element [%d] on "
					"stream [%d].\n", start_addr[i], i, stream);
				return -1;
			}
		}
		break;
	default:
		iav_error("Don't support QP roi type:%d\n", qp_matrix->type);
		return -1;
	}
	return 0;
}

static inline int check_statistics_config(int stream,
		iav_enc_statis_config_ex_t * statis_config)
{
	static u32 total_mvdump_streams = 0;
	u8 mv_div_factor = 0;

	if (!is_supported_stream(stream)) {
		return -1;
	}
	if (statis_config->enable_mv_dump) {
		if ((total_mvdump_streams != (1 << stream)) &&
				(total_mvdump_streams != 0)) {
			iav_error("CANNOT do MV dump for stream [%d], only allow ONE!"
					" Please disable other stream first [ID: %d].\n",
					stream, total_mvdump_streams);
			return -1;
		}
		mv_div_factor = statis_config->mvdump_division_factor;
		if (mv_div_factor == 0 || mv_div_factor > MAX_MVDUMP_DIV_FACTOR) {
			iav_error("MV dump division factor %d cannot be 0 or larger than %d.\n",
					mv_div_factor, MAX_MVDUMP_DIV_FACTOR);
			return -1;
		}
		total_mvdump_streams |= (1 << stream);
	} else {
		total_mvdump_streams &= (~(1 << stream));
	}
	if (statis_config->enable_qp_hist && !__is_enc_h264(stream)) {
		iav_error("CANNOT dump QP histogram for stream [%d]. It must be H264"
			" stream.\n", stream);
		return -1;
	}
	return 0;
}

static inline int check_h264_rt_enc_param(int stream,
	iav_h264_enc_param_ex_t *enc_param)
{
	iav_no_check();

	if (!is_supported_stream(stream)) {
		return -1;
	}
	if (enc_param->intrabias_P > INTRABIAS_MAX ||
			enc_param->intrabias_P < INTRABIAS_MIN) {
		iav_error("P frame intrabias [%d] is out of range [%d~%d].\n",
				enc_param->intrabias_P, INTRABIAS_MIN, INTRABIAS_MAX);
		return -1;
	}
	if (enc_param->intrabias_B > INTRABIAS_MAX ||
			enc_param->intrabias_B < INTRABIAS_MIN) {
		iav_error("B frame intrabias [%d] is out of range [%d~%d].\n",
				enc_param->intrabias_B, INTRABIAS_MIN, INTRABIAS_MAX);
		return -1;
	}
	if (enc_param->nonSkipCandidate_bias > P_SKIP_MAX ||
			enc_param->nonSkipCandidate_bias < P_SKIP_MIN) {
		iav_error("Non skip bias [%d] is out of range [%d~%d].\n",
				enc_param->nonSkipCandidate_bias, P_SKIP_MIN, P_SKIP_MAX);
		return -1;
	}
	if (enc_param->skipCandidate_threshold > P_SKIP_MAX ||
			enc_param->skipCandidate_threshold < P_SKIP_MIN) {
		iav_error("P Skip threshold [%d] is out of range [%d~%d].\n",
			enc_param->skipCandidate_threshold, P_SKIP_MIN, P_SKIP_MAX);
		return -1;
	}
	if (!enc_param->cpb_underflow_num || !enc_param->cpb_underflow_den) {
		iav_error("CPB underflow ratio [%d/%d] cannot be 0.\n",
			enc_param->cpb_underflow_num, enc_param->cpb_underflow_den);
		return -1;
	}
	if (enc_param->zmv_threshold > ZMV_TH_MAX ||
			enc_param->zmv_threshold < ZMV_TH_MIN) {
		iav_error("ZMV threshold [%d] is out of range [%d~%d].\n",
			enc_param->zmv_threshold, ZMV_TH_MIN, ZMV_TH_MAX);
		return -1;
	}
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if ((enc_param->user1_intra_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user1_intra_bias > H264_USER_BIAS_MAX) ||
		(enc_param->user1_direct_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user1_direct_bias > H264_USER_BIAS_MAX) ||
		(enc_param->user2_intra_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user2_intra_bias > H264_USER_BIAS_MAX) ||
		(enc_param->user2_direct_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user2_direct_bias > H264_USER_BIAS_MAX) ||
		(enc_param->user3_intra_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user3_intra_bias > H264_USER_BIAS_MAX) ||
		(enc_param->user3_direct_bias < H264_USER_BIAS_MIN) ||
		(enc_param->user3_direct_bias > H264_USER_BIAS_MAX)) {
		iav_error("User1 bias [intra: %d, direct: %d], must be in [%d,%d].\n",
			enc_param->user1_intra_bias, enc_param->user1_direct_bias,
			H264_USER_BIAS_MIN, H264_USER_BIAS_MAX);
		iav_error("User2 bias [intra: %d, direct: %d], must be in [%d,%d].\n",
			enc_param->user2_intra_bias, enc_param->user2_direct_bias,
			H264_USER_BIAS_MIN, H264_USER_BIAS_MAX);
		iav_error("User3 bias [intra: %d, direct: %d], must be in [%d,%d].\n",
			enc_param->user3_intra_bias, enc_param->user3_direct_bias,
			H264_USER_BIAS_MIN, H264_USER_BIAS_MAX);
		return -1;
	}
#else
	if ((enc_param->mode_bias_I4Add < H264_MODE_BIAS_MIN) ||
		(enc_param->mode_bias_I4Add > H264_MODE_BIAS_MAX) ||
		(enc_param->mode_bias_I16Add < H264_MODE_BIAS_MIN) ||
		(enc_param->mode_bias_I16Add > H264_MODE_BIAS_MAX) ||
		(enc_param->mode_bias_Inter8Add < H264_MODE_BIAS_MIN) ||
		(enc_param->mode_bias_Inter8Add > H264_MODE_BIAS_MAX) ||
		(enc_param->mode_bias_Inter16Add < H264_MODE_BIAS_MIN) ||
		(enc_param->mode_bias_Inter16Add > H264_MODE_BIAS_MAX) ||
		(enc_param->mode_bias_DirectAdd < H264_MODE_BIAS_MIN) ||
		(enc_param->mode_bias_DirectAdd > H264_MODE_BIAS_MAX)) {
		iav_error("Mode bias I add [ 4: %d, 16: %d].\n",
			enc_param->mode_bias_I4Add, enc_param->mode_bias_I16Add);
		return -1;
	}
#endif
	return 0;
}

static inline void clean_previous_bits_desc(dsp_bits_info_t * current_entry)
{
	dsp_bits_info_t * previous_index = NULL;

	//advance to next descriptor, so clear previous descriptor
	if (current_entry > G_encode_obj.bits_desc_start) {
		previous_index = current_entry - 1;
	} else {
		previous_index = G_encode_obj.bits_desc_end - 1;	// bits_desc_end is exclusive
	}
	//update PTS of previous descriptor to be invalid
	set_invalid_bits_desc(previous_index);
}

static inline void clean_prev_statis_desc(dsp_enc_stat_info_t * current_entry)
{
	dsp_enc_stat_info_t * prev = NULL;
	if (current_entry > G_encode_obj.stat_desc_start) {
		prev = current_entry - 1;
	} else {
		prev = G_encode_obj.stat_desc_end - 1;
	}
	set_invalid_statis_desc(prev);
}

static inline void update_bits_desc_read_ptr(void)
{
	if (++G_encode_obj.bits_desc_read_ptr == G_encode_obj.bits_desc_end) {
		G_encode_obj.bits_desc_read_ptr = G_encode_obj.bits_desc_start;
	}
}

static inline void update_statis_desc_read_ptr(void)
{
	if (++G_encode_obj.stat_desc_read_ptr == G_encode_obj.stat_desc_end)
		G_encode_obj.stat_desc_read_ptr = G_encode_obj.stat_desc_start;
}

static inline int get_pts_from_dsp_ex_bits_info(iav_bits_info_slot_ex_t * encode_slot)
{
	int ext_info_index;
	dsp_extend_bits_info_t * ext_bits_info = NULL;
	if (!encode_slot) {
		iav_error("Null encode slot!\n");
		return -1;
	}

	ext_info_index = encode_slot->read_index - G_encode_obj.bits_desc_start;
	ext_bits_info = G_encode_obj.ext_bits_desc_start + ext_info_index;
	if (ext_bits_info >= G_encode_obj.ext_bits_desc_end) {
		iav_error("Invalid extend info index!\n");
		BUG();
	}

	encode_slot->mono_pts = ext_bits_info->monotonic_pts;
	encode_slot->dsp_pts = ext_bits_info->dsp_pts;
	return 0;
}

/*
 * Use polling mode based method to read out frames
 */
static int read_encode_fifo_by_polling(iav_context_t *context,
		iav_bits_info_slot_ex_t *encode_slot)
{
	int poll_counter = 0;
	dsp_bits_info_t * read_index = NULL;

	while (1) {
		if (++poll_counter == MAX_BSB_POLLING_TIMES) {
			iav_error("No new frames, exit polling bit info in [%d] times!\n",
					MAX_BSB_POLLING_TIMES);
			return -EFAULT;
		}
		fetch_frame_from_queue(encode_slot);
		if (!encode_slot->read_index) {
			// no new entry exist, sleep a tick, actual time depends on CPU HZ
			iav_unlock();
			// msleep(1);
			iav_hrtimer_wait_next(&G_bsb_polling_read);
			iav_lock();
			continue;
		}
		// it's a valid entry from DSP anyway, so clean previous entry
		clean_previous_bits_desc(read_index);
		break;
	} // end while (1) looping protocol

	return 0;
}

/* Uses CODE_VDSP_2_IRQ to indicate one frame is encoded and DMA to BSB buffer
 * already. The benefit of this design is to cancel polling and high resolution
 * timer to reduce frame read out jitter.
 */
static int read_encode_fifo_by_interrupt(iav_context_t * context,
		iav_bits_info_slot_ex_t * encode_slot)
{
	iav_unlock();
	if (get_one_encoded_frame() < 0) {
		iav_lock();
		iav_error("Error to read out fifo by interrupt!\n");
		return -EIO;
	}
	iav_lock();

	fetch_frame_from_queue(encode_slot);
	if (!encode_slot->read_index) {
		iav_error("Got frame encode INTR, while data is not ready.\n");
		return -EAGAIN;
	}

	// it's valid entry from DSP anyway, so clean previous entry
	clean_previous_bits_desc(encode_slot->read_index);
	if (get_pts_from_dsp_ex_bits_info(encode_slot) < 0) {
		iav_error("Invalid monotomic PTS!\n");
		return -EINVAL;
	}
	return 0;
}

static int read_encode_fifo_from_queue_by_interrupt(iav_context_t * context,
		int stream, iav_bits_info_slot_ex_t * encode_slot)
{
	iav_unlock();
	if (get_one_encoded_frame() < 0) {
		iav_lock();
		iav_printk("Error to read out fifo by interrupt!\n");
		return -EIO;
	}
	iav_lock();

	if (stream == IAV_MAX_ENCODE_STREAMS_NUM) {
		fetch_frame_from_queue(encode_slot);
	} else {
		fetch_frame_from_stream_queue(stream, encode_slot);
	}
	if (encode_slot->read_index == NULL) {
		iav_error("Got frame encode INTR but data not ready, try again!\n");
		return -EAGAIN;
	}

	// it's valid entry from DSP, clean previous entry
	clean_previous_bits_desc(encode_slot->read_index);
	return 0;
}

static int update_encode_info_ex(iav_context_t * context,
	bits_info_ex_t * pd, iav_bits_info_slot_ex_t * encode_slot)
{
	dsp_bits_info_t * dsp_pd = NULL;
	u32 align_size, stream;
	u32 bits_fifo_type = IAV_BITS_MJPEG;
	int retv = 0;

	memset(pd, 0, sizeof(bits_info_ex_t));
	// copy one descriptor
	dsp_pd = encode_slot->read_index;
	bits_fifo_type = encode_slot->bits_type;

	// convert picture format
	pd->frame_num = dsp_pd->frmNo;
	pd->frame_index = encode_slot->frame_index;
	pd->start_addr = dsp_pd->start_addr;

	pd->pic_type = dsp_pd->frameTy;
	pd->level_idc = dsp_pd->levelIDC;
	pd->ref_idc = dsp_pd->refIDC;
	pd->pic_struct = dsp_pd->picStruct;
	pd->pic_size = dsp_pd->length;
	pd->channel_id = 0;
	pd->PTS = (u32)encode_slot->dsp_pts;
	pd->stream_pts = encode_slot->dsp_pts;
	pd->monotonic_pts = encode_slot->mono_pts;

	stream = get_stream_num_from_type(dsp_pd->stream_type);
	// validation
	if (__is_invalid_stream(stream)) {
		iav_error("stream id is out of range : %d.\n", stream);
		return -EIO;
	}
	pd->stream_id = stream;
	pd->session_id = G_stream_session_id[stream][0];
	pd->pic_width = G_encode_stream[stream].format.encode_width;
	pd->pic_height = G_encode_stream[stream].format.encode_height;

	if (bits_fifo_type == IAV_BITS_MJPEG) {
		pd->jpeg_quality = dsp_pd->jpeg_qlevel;
	}

#ifdef CONFIG_AMBARELLA_IAV_SUPPORT_BITS_INFO_PRINT
	DRV_PRINT("Stream [0x%x], frmNo is %d, monotonic pts is %llu, start_addr is 0x%08x,"
			" pic_type is %d, pic_size is %d, JPG Q %d.\n",
			dsp_pd->stream_type, dsp_pd->frmNo, pd->monotonic_pts,
			dsp_pd->start_addr, dsp_pd->frameTy, dsp_pd->length, dsp_pd->jpeg_qlevel);
#endif

	// exit when received the stream end null frame
	if (is_end_frame(dsp_pd)) {
		iav_printk("Sent STREAM END null frame for stream %c.\n",
				'A' + pd->stream_id);
		pd ->stream_end = 1;
		spin_lock_irq(&G_iav_obj.lock);
		irq_update_stopping_stream(pd->stream_id);
		spin_unlock_irq(&G_iav_obj.lock);
		return 0;
	}

	// picture real size
	align_size = pd->pic_size & (~(1 << 23));
	if (align_size == ENCODE_INVALID_LENGTH) {
		iav_error("Wrong pic size!\n");
		return -EIO;
	} else {
		align_size = (align_size + 31) & ~31;

		// address translation and cache sync.
		retv = map_addr_dsp_to_user(context, IAV_MMAP_BSB2, &pd->start_addr);
		if (retv < 0) {
			iav_error("bits descriptor: [0x%x]\n"
					"frame_num=%d\n"
					"mono_pts=%llu\n"
					"start_addr=0x%x\n"
					"pic_type=%d\n"
					"level_idc=%d\n"
					"ref_idc=%d\n"
					"pic_struct=%d\n"
					"pic_size=%d\n", (u32)dsp_pd,
					pd->frame_num, pd->monotonic_pts, pd->start_addr,
					pd->pic_type, pd->level_idc, pd->ref_idc,
					pd->pic_struct, pd->pic_size);
			return retv;
		}
	}
	return 0;
}

static int read_statis_info_by_polling(iav_context_t * context,
		iav_statis_info_slot_ex_t * statis_slot)
{
	dsp_enc_stat_info_t * statis_info = NULL;
	int poll_counter = 0;
	static int frame_num = -1;

	while (1) {
		if (++poll_counter == MAX_STATIS_POLLING_TIMES) {
			iav_error("No new statistics data, exit polling statistics FIFO!\n");
			return -EAGAIN;
		}
		statis_info = G_encode_obj.stat_desc_read_ptr;
		invalidate_d_cache((void*)statis_info, sizeof(dsp_enc_stat_info_t));
		if (frame_num != statis_info->frmNo) {
			frame_num = statis_info->frmNo;
			update_statis_desc_read_ptr();
		}

		if (is_invalid_statis_desc(statis_info)) {
			iav_unlock();
			iav_hrtimer_wait_next(&G_statis_polling_read);
			iav_lock();
			continue;
		} else {
			clean_prev_statis_desc(statis_info);
			break;
		}
	}
	statis_slot->read_index = statis_info;
	statis_slot->mono_pts = get_monotonic_pts();
	return 0;
}

static int read_statis_info_by_interrupt(iav_context_t * context,
		int stream, iav_statis_info_slot_ex_t * statis_slot)
{
	if (!(statis_slot->flag & IAV_IOCTL_NONBLOCK)) {
		iav_unlock();
		if (get_one_statistics_info() < 0) {
			iav_lock();
			iav_error("Error to read out statistics by INT!\n");
			return -EIO;
		}
		iav_lock();
	}

	fetch_statistics_from_queue(stream, statis_slot);
	if (statis_slot->read_index == NULL) {
		iav_error("Got statistics INTR but data not ready, try again!\n");
		return -EAGAIN;
	}

	clean_prev_statis_desc(statis_slot->read_index);
	return 0;
}

static int update_statistics_info_ex(iav_context_t * context,
	iav_enc_statis_info_ex_t * pd, iav_statis_info_slot_ex_t * statis_slot)
{
	iav_stream_statis_config_ex_t * stream_statis = NULL;
	dsp_enc_stat_info_t * dsp_pd = statis_slot->read_index;
	int retv = 0, stream;
	u8 * virt_addr = NULL;

	stream = get_stream_num_from_type(dsp_pd->stream_type);
	pd->stream = stream;
	pd->statis_index = statis_slot->statis_index;
	pd->frame_num = dsp_pd->frmNo;
	pd->stream_pts = dsp_pd->pts_64;
	pd->mono_pts = statis_slot->mono_pts;
	stream_statis =&G_encode_stream[stream].statis;
	pd->mvdump_pitch = stream_statis->mvdump_pitch;
	pd->mvdump_unit_size = stream_statis->mvdump_unit_size;

	pd->mv_start_addr = 0;
	if (pd->data_type & IAV_MV_DUMP_FLAG) {
		// address translation and cache sync
		virt_addr = DSP_TO_VIRT(dsp_pd->mv_start_addr);
		pd->mv_start_addr = dsp_pd->mv_start_addr;
		retv = map_addr_dsp_to_user(context, IAV_MMAP_MV, &pd->mv_start_addr);
		if (retv < 0) {
			iav_error("Statistics descriptor:\n"
					"[%d] stream = %d, frameNO = %d, mono_pts = %llu, "
					"mv_start_addr = 0x%x\n", pd->statis_index, pd->stream,
					pd->frame_num, pd->mono_pts, pd->mv_start_addr);
			return retv;
		}
		invalidate_d_cache(virt_addr, pd->mvdump_unit_size);
	}

	pd->qp_hist_addr = 0;
	if (pd->data_type & IAV_QP_HIST_DUMP_FLAG) {
		// address translation and cache sync
		virt_addr = DSP_TO_VIRT(dsp_pd->qp_hist_daddr);
		pd->qp_hist_addr = dsp_pd->qp_hist_daddr;
		retv = map_addr_dsp_to_user(context,
			IAV_MMAP_QP_HIST, &pd->qp_hist_addr);
		if (retv < 0) {
			iav_error("Statistics descriptor:\n"
				"[%d] stream = %d, frmNo = %d, mono_pts = %llu, "
				"qp_hist_addr = 0x%x\n", pd->statis_index, pd->stream,
				pd->frame_num, pd->mono_pts, pd->qp_hist_addr);
			return retv;
		}
		invalidate_d_cache(virt_addr, sizeof(iav_qp_hist_ex_t));
		pd->frame_type = ((iav_qp_hist_ex_t *)virt_addr)->pic_type;
	}

	return 0;
}

static int create_session_id(int stream)
{
	u32 random_data;
	get_random_bytes(&random_data, sizeof(random_data));
	G_stream_session_id[stream][0] = random_data;
	return 0;
}

static int update_statistics_config(int stream,
		iav_enc_statis_config_ex_t * config, u32 * enable_flags)
{
	u32 flags = 0, pitch;
	iav_encode_format_ex_t * format = &G_encode_stream[stream].format;
	iav_stream_statis_config_ex_t * statis = &G_encode_stream[stream].statis;

	if (config->enable_mv_dump) {
		flags |= ENABLE_MVDUMP_FLAG;
		pitch = get_buffer_pitch_in_MB(get_MB_num(format->encode_width));
		statis->mvdump_pitch = pitch;
		statis->mvdump_unit_size = pitch * get_MB_num(format->encode_height);
		statis->mvdump_division_factor = config->mvdump_division_factor;
		statis->enable_flags |= ENABLE_MVDUMP_FLAG;
	} else {
		statis->enable_flags &= (~ENABLE_MVDUMP_FLAG);
	}
	if (config->enable_qp_hist) {
		flags |= ENABLE_QP_HIST_DUMP_FLAG;
		statis->enable_flags |= ENABLE_QP_HIST_DUMP_FLAG;
	} else {
		statis->enable_flags &= (~ENABLE_QP_HIST_DUMP_FLAG);
	}
	*enable_flags = flags;
	return 0;
}

static int set_h264_rt_enc_param(iav_context_t * context,
	iav_h264_enc_param_ex_t *arg, int sync_frame_flag)
{
	int i;
	u32 flags = 0;
	iav_h264_config_ex_t * h264 = NULL;

	if (get_single_stream_num(arg->id, &i) < 0) {
		return -EINVAL;
	}
	if (check_h264_rt_enc_param(i, arg) < 0) {
		return -EINVAL;
	}
	h264 = &G_encode_config[i].h264_encode_config;
	if ((arg->intrabias_P != h264->intrabias_P) ||
			(arg->intrabias_B != h264->intrabias_B)) {
		h264->intrabias_P = arg->intrabias_P;
		h264->intrabias_B = arg->intrabias_B;
		flags |= UPDATE_INTRA_BIAS_FLAG;
	}
	if ((arg->nonSkipCandidate_bias != h264->nonSkipCandidate_bias) ||
			(arg->skipCandidate_threshold != h264->skipCandidate_threshold)) {
		h264->nonSkipCandidate_bias = arg->nonSkipCandidate_bias;
		h264->skipCandidate_threshold = arg->skipCandidate_threshold;
		flags |= UPDATE_P_SKIP_FLAG;
	}
	if ((arg->cpb_underflow_num != h264->cpb_underflow_num) ||
		(arg->cpb_underflow_den != h264->cpb_underflow_den)) {
		h264->cpb_underflow_num = arg->cpb_underflow_num;
		h264->cpb_underflow_den = arg->cpb_underflow_den;
		flags |= UPDATE_CPB_RESET_FLAG;
	}
	if (arg->zmv_threshold != h264->zmv_threshold) {
		h264->zmv_threshold = arg->zmv_threshold;
		flags |= UPDATE_ZMV_THRESHOLD_FLAG;
	}

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if ((arg->user1_intra_bias != h264->user1_intra_bias) ||
		(arg->user1_direct_bias != h264->user1_direct_bias) ||
		(arg->user2_intra_bias != h264->user2_intra_bias) ||
		(arg->user2_direct_bias != h264->user2_direct_bias) ||
		(arg->user3_intra_bias != h264->user3_intra_bias)||
		(arg->user3_direct_bias != h264->user3_direct_bias)) {
		h264->user1_intra_bias = arg->user1_intra_bias;
		h264->user1_direct_bias = arg->user1_direct_bias;
		h264->user2_intra_bias = arg->user2_intra_bias;
		h264->user2_direct_bias = arg->user2_direct_bias;
		h264->user3_intra_bias = arg->user3_intra_bias;
		h264->user3_direct_bias = arg->user3_direct_bias;

		flags |= UPDATE_QP_ROI_MATRIX_FLAG;
	}

#else
	if ((arg->mode_bias_I4Add != h264->mode_bias_I4Add) ||
		(arg->mode_bias_I16Add != h264->mode_bias_I16Add) ||
		(arg->mode_bias_Inter8Add != h264->mode_bias_Inter8Add) ||
		(arg->mode_bias_Inter16Add != h264->mode_bias_Inter16Add) ||
		(arg->mode_bias_DirectAdd != h264->mode_bias_DirectAdd)) {
		h264->mode_bias_I4Add = arg->mode_bias_I4Add;
		h264->mode_bias_I16Add = arg->mode_bias_I16Add;
		h264->mode_bias_Inter8Add = arg->mode_bias_Inter8Add;
		h264->mode_bias_Inter16Add = arg->mode_bias_Inter16Add;
		h264->mode_bias_DirectAdd = arg->mode_bias_DirectAdd;

		flags |= UPDATE_MODE_BIAS_FLAG;
	}
#endif
	if (__is_stream_in_h264_encoding(i) && flags) {
		if (sync_frame_flag) {
			update_sync_frame_enc_params_ex(context, i, flags);
		} else {
			cmd_update_encode_params_ex(context, i, flags);
		}
	}

	return 0;
}

static int get_h264_rt_enc_param(iav_h264_enc_param_ex_t *arg)
{
	int i;
	iav_h264_config_ex_t * h264 = NULL;

	if (get_single_stream_num(arg->id, &i) < 0) {
		return -EINVAL;
	}
	h264 = &G_encode_config[i].h264_encode_config;
	arg->intrabias_P = h264->intrabias_P;
	arg->intrabias_B = h264->intrabias_B;
	arg->nonSkipCandidate_bias = h264->nonSkipCandidate_bias;
	arg->skipCandidate_threshold = h264->skipCandidate_threshold;
	arg->cpb_underflow_num = h264->cpb_underflow_num;
	arg->cpb_underflow_den = h264->cpb_underflow_den;
	arg->zmv_threshold = h264->zmv_threshold;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	arg->user1_intra_bias = h264->user1_intra_bias;
	arg->user1_direct_bias = h264->user1_direct_bias;
	arg->user2_intra_bias = h264->user2_intra_bias;
	arg->user2_direct_bias = h264->user2_direct_bias;
	arg->user3_intra_bias = h264->user3_intra_bias;
	arg->user3_direct_bias = h264->user3_direct_bias;
#else
	arg->mode_bias_I4Add = h264->mode_bias_I4Add;
	arg->mode_bias_I16Add = h264->mode_bias_I16Add;
	arg->mode_bias_Inter8Add = h264->mode_bias_Inter8Add;
	arg->mode_bias_Inter16Add = h264->mode_bias_Inter16Add;
	arg->mode_bias_DirectAdd = h264->mode_bias_DirectAdd;
#endif
	return 0;
}

/******************************************
 *
 *	IAV IOCTLs functions
 *
 ******************************************/

//ipcam ucode, dual stream support
static int iav_start_encode_ex(iav_context_t *context, iav_stream_id_t stream_id)
{
	iav_encode_format_ex_t * format = NULL;
	int i, encoding_counter, vcap_count;
	u32 update_flags, flush_stream_map;
	int encode_mode = 0;
	int ret = 0;

	if ((ret = check_start_encode_state(context, stream_id)) < 0) {
		return ret;
	}

	/* Wait Vsync before start encoding for source buffer resolution changed
	*/
	encode_mode = get_enc_mode();
	vcap_count = G_iav_vcap.vskip_before_encode[encode_mode];
	wait_vcap_msg_count(vcap_count);
	iav_printk("wait %d Vsync before start encoding for source buffer resolution changed.\n",
		vcap_count);

	if ((ret = check_before_start_encode(context, stream_id)) < 0) {
		return ret;
	}

	/* Start a new travel from previe state, init the encode obj.
	*/
	if (is_iav_state_preview()) {
		reset_encode_obj();
		start_encode_timer();
		flush_stream_map = (1 << IAV_MAX_ENCODE_STREAMS_NUM) - 1;
		iav_printk("Init encoder from preview. Flush out all streams.\n");
	} else {
		flush_stream_map = stream_id;
	}

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (flush_stream_map & (1 << i)) {
			flush_all_frames(i);
			flush_all_statis(i);
		}
	}

	/* Guarantee the encode setup, rotate and encode start commands are
	 * issued in the same Vsync for all streams to have streams have the
	 * identical start frame.
	 */
	dsp_start_enc_batch_cmd();
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			if (create_session_id(i) < 0) {
				iav_error("Create session id for stream %d failed.\n", i);
				return -EFAULT;
			}
			if (__is_enc_h264(i)) {
				cmd_h264_encode_setup(context, i);
				update_flags = UPDATE_INTRA_BIAS_FLAG
					| UPDATE_SCBR_PARAM_FLAG
					| UPDATE_FRAME_FACTOR_FLAG
					| UPDATE_STREAM_BITRATE_FLAG
					| UPDATE_PANIC_PARAM_FLAG
					| UPDATE_ZMV_THRESHOLD_FLAG
					| UPDATE_MODE_BIAS_FLAG;
				cmd_update_encode_params_ex(context, i, update_flags);
			} else {
				cmd_jpeg_encode_setup(context, i, 1);
			}
			cmd_encode_rotate(context, i);
			__set_enc_op(i, IAV_ENCODE_OP_START);
		}
	}
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			format = &G_encode_stream[i].format;
			if (__is_enc_h264(i)) {
				cmd_h264_encode_start(context, i);
			} else {
				cmd_jpeg_encode_start(context, i);
			}
			spin_lock_irq(&G_iav_obj.lock);
			__inc_source_buffer_ref(format->source, i);
			spin_unlock_irq(&G_iav_obj.lock);
		}
	}
	dsp_end_enc_batch_cmd();

	// wait for multiple to start
	for (i = 0, encoding_counter = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_id & (1 << i)) {
			if (__is_stream_in_starting(i)) {
				iav_unlock();
				format = &G_encode_stream[i].format;
				wait_event_interruptible(G_encode_stream[i].venc_wq,
					(G_iav_obj.dsp_encode_state_ex[i] == ENC_BUSY_STATE));
				iav_lock();
			}
			if (__is_stream_in_encoding(i)) {
				++encoding_counter;
			}
		}
	}

	// update IAV global state
	G_encode_obj.encoding_stream_num += encoding_counter;

	return 0;
}

int iav_stop_encode_ex(iav_context_t *context, iav_stream_id_t stream_id)
{
	int i, stopped_counter;
	iav_stream_id_t multi_id;
	int ret = 0;

	if ((i = check_stop_encode_state(&stream_id)) < 0) {
		return i;
	}

	// stop multiple encoding streams at same VSYNC interrupt
	dsp_start_enc_batch_cmd();
	for (i = 0, multi_id = stream_id; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if ((stream_id & (1 << i)) && __is_stream_in_encoding(i)) {
			cmd_encode_stop(context, i, H264_STOP_IMMEDIATELY);

			spin_lock_irq(&G_iav_obj.lock);
			__set_enc_op(i, IAV_ENCODE_OP_STOP);
			spin_unlock_irq(&G_iav_obj.lock);
		} else {
			multi_id &= ~(1 << i);
		}
	}
	dsp_end_enc_batch_cmd();

	// wait for multiple to stop
	stopped_counter = 0;
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (multi_id & (1 << i)) {
			if (__is_stream_in_stopping(i)) {
				iav_unlock();
				ret = wait_event_interruptible_timeout(G_encode_stream[i].venc_wq,
					(G_iav_obj.dsp_encode_state_ex[i] == ENC_IDLE_STATE),
					TIMEOUT_JIFFIES);
				iav_lock();
				if (ret <= 0) {
					iav_printk("wait_event_interruptible_timeout: stream[%d]\n", i);
				}
			}
			if (__is_stream_ready_for_encode(i)) {
				++stopped_counter;
			}
			if (G_encode_stream[i].format.source == IAV_ENCODE_SOURCE_MAIN_DRAM) {
				G_iav_obj.source_buffer[IAV_ENCODE_SOURCE_MAIN_DRAM].dram.valid_frame_num = 0;
			}
		}
	}

	G_encode_obj.encoding_stream_num -= stopped_counter;
	if (G_encode_obj.encoding_stream_num < 0) {
		iav_error("Assertion: Incorrect state!!!\n");
		return -EFAULT;
	}

	if (is_iav_state_preview()) {
		stop_encode_timer();
	}

	return 0;
}

static int iav_read_encode_fifo_ex(iav_context_t *context,
		bits_info_ex_t __user *arg)
{
	int retv = 0;
	bits_info_ex_t bits_info;
	iav_bits_info_slot_ex_t encode_slot;

	if (!__is_bsb_mapped(context)) {
		iav_error("Failed to map BSB, cannot read bit stream!\n");
		return -EFAULT;
	}

	memset(&encode_slot, 0, sizeof(encode_slot));
	if (likely(is_interrupt_readout())) {
		retv = read_encode_fifo_by_interrupt(context, &encode_slot);
	} else {
		retv = read_encode_fifo_by_polling(context, &encode_slot);
	}
	if (retv < 0) {
		return retv;
	}

	retv = update_encode_info_ex(context, &bits_info, &encode_slot);
	if (retv < 0)
		return retv;

	update_bits_desc_read_ptr();

	release_frame_to_queue(bits_info.frame_index);

	return copy_to_user(arg, &bits_info, sizeof(bits_info_ex_t)) ? -EFAULT : 0;
}

static int iav_fetch_encoded_frame_ex(iav_context_t * context,
		bits_info_ex_t __user * arg)
{
	int retv = 0;
	bits_info_ex_t bits_info;
	iav_bits_info_slot_ex_t encode_slot;

	if (!__is_bsb_mapped(context)) {
		iav_error("Failed to map BSB, cannot read bit stream !\n");
		return -EFAULT;
	}
	if (copy_from_user(&bits_info, arg, sizeof(bits_info))) {
		return -EFAULT;
	}

	memset(&encode_slot, 0, sizeof(encode_slot));
	if (likely(is_interrupt_readout())) {
		retv = read_encode_fifo_from_queue_by_interrupt(context,
			bits_info.stream_id, &encode_slot);
	} else {
		iav_error("This IOCTL needs to be called with interrupt read out mode!\n");
		return -EFAULT;
	}
	if (retv < 0)
		return retv;

	retv = update_encode_info_ex(context, &bits_info, &encode_slot);
	if (retv < 0)
		return retv;

	update_bits_desc_read_ptr();

	return copy_to_user(arg, &bits_info, sizeof(bits_info_ex_t)) ? -EFAULT : 0;
}

static int iav_release_encoded_frame_ex(iav_context_t * context,
		bits_info_ex_t __user * arg)
{
	int frame_index;
	bits_info_ex_t bits_info;

	if (is_polling_readout())
		return 0;

	if (copy_from_user(&bits_info, arg, sizeof(bits_info)))
		return -EFAULT;

	frame_index = bits_info.frame_index;
	if (is_invalid_frame_index(frame_index)) {
		iav_error("Invalid frame index [%d] to release, it's not in the range of [0~%d].\n",
				frame_index, NUM_BS_DESC);
		return -EINVAL;
	}
	release_frame_to_queue(frame_index);
	return 0;
}

static int iav_get_encode_stream_info_ex(iav_context_t *context,
		struct iav_encode_stream_info_ex_s __user *arg)
{
	iav_encode_stream_info_ex_t stream_info;
	int i;

	if (copy_from_user(&stream_info, arg, sizeof(*arg)))
		return -EFAULT;

	if (get_single_stream_num(stream_info.id, &i) < 0) {
		return -EINVAL;
	}
	stream_info.state = __get_stream_encode_state(i);

	return copy_to_user(arg, &stream_info, sizeof(stream_info)) ? -EFAULT : 0;
}

static int iav_get_bsb_fullness(iav_context_t *context, iav_bsb_fullness_t __user *arg)
{
	struct iav_mem_block *bsb;
	iav_bsb_fullness_t fullness;

	iav_get_mem_block(IAV_MMAP_BSB, &bsb);

	fullness.total_size = bsb->size;
	fullness.fullness = G_encode_obj.bits_fifo_fullness;

	fullness.total_pictures = G_encode_obj.total_desc;
	fullness.num_pictures = G_encode_obj.curr_num_desc;
	iav_printk("This function is NOT supported yet!\n");

	if (copy_to_user(arg, &fullness, sizeof(fullness)))
		return -EFAULT;

	return 0;
}

//set encode format to stream id, do not update stream state
static int iav_set_encode_format_ex(iav_context_t *context,
		struct iav_encode_format_ex_s __user * arg)
{
	iav_encode_format_ex_t encode_format, * format = NULL;
	int i;

	if (copy_from_user(&encode_format, arg, sizeof(encode_format)))
		return -EFAULT;

	if (get_single_stream_num(encode_format.id, &i) < 0) {
		return -EINVAL;
	}

	if (check_encode_format(i, &encode_format) < 0) {
		iav_error("Invalid encode format of stream [%d]!\n", i);
		return -EINVAL;
	}

	//if unchanged, then do nothing
	format = &G_encode_stream[i].format;
	if (!is_encode_format_changed(format, &encode_format))
		return 0;

	if (__is_stream_in_encoding(i)) {
		if (is_encode_offset_changed_only(format, &encode_format)) {
			if (check_encode_offset(&encode_format) < 0) {
				return -EINVAL;
			}
			G_encode_stream[i].format = encode_format;
			cmd_update_encode_params_ex(context, i, UPDATE_STREAM_OFFSET_FLAG);
		} else {
			iav_error("CANNOT modify stream %d format except"
					" offset when in encoding.\n", i);
			return -EINVAL;
		}
	} else {
		G_encode_stream[i].format = encode_format;
	}

	return 0;
}

//get encode format by stream id
static int iav_get_encode_format_ex(iav_context_t *context,
		struct iav_encode_format_ex_s __user *arg)
{
	iav_encode_format_ex_t format;
	int i;

	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	if (get_single_stream_num(format.id, &i) < 0) {
		return -EINVAL;
	}
	format = G_encode_stream[i].format;

	return  copy_to_user(arg, &format, sizeof(format)) ? -EFAULT : 0;
}

static int iav_set_jpeg_config_ex(iav_context_t * context,
		iav_jpeg_config_ex_t __user * arg)
{
	iav_jpeg_config_ex_t config;
	int i;

	if (copy_from_user(&config, arg, sizeof(config)))
		return -EFAULT;

	if (get_single_stream_num(config.id, &i) < 0) {
		return -EINVAL;
	}

	if (check_jpeg_config(i, &config) < 0) {
		iav_error("Invalid MJPEG config for stream [%d].\n", i);
		return -EINVAL;
	}

	G_encode_config[i].jpeg_encode_config.chroma_format = config.chroma_format;
	G_encode_config[i].jpeg_encode_config.quality = config.quality;

	if (__is_stream_in_encoding(i)) {
		cmd_update_encode_params_ex(context, i, UPDATE_QUANT_MATRIX_FLAG);
	}

	return 0;
}

static int iav_get_jpeg_config_ex(iav_context_t * context,
		iav_jpeg_config_ex_t __user * arg)
{
	iav_jpeg_config_ex_t config;
	int i;

	if (copy_from_user(&config, arg, sizeof(config)))
		return -EFAULT;

	if (get_single_stream_num(config.id, &i) < 0) {
		return -EINVAL;
	}
	config = G_encode_config[i].jpeg_encode_config;

	return copy_to_user(arg, &config, sizeof(config)) ? -EFAULT : 0;
}

static int iav_set_h264_config_ex(iav_context_t *context,
		iav_h264_config_ex_t __user *arg)
{
	iav_h264_config_ex_t config;
	u8 *addr = NULL;
	int i;

	if (copy_from_user(&config, arg, sizeof(config)))
		return -EFAULT;

	if (get_single_stream_num(config.id, &i) < 0) {
		return -EINVAL;
	}

	// basic check
	//	if (calc_quality_model(&config) < 0)
	//		return -EINVAL;

	if (check_h264_config(i, &config) < 0) {
		iav_error("Invalid H.264 config for stream [%d].\n", i);
		return -EINVAL;
	}
	addr = G_encode_config[i].h264_encode_config.qmatrix_4x4_daddr;
	G_encode_config[i].h264_encode_config = config;
	G_encode_config[i].h264_encode_config.qmatrix_4x4_daddr = addr;

	return 0;
}

static int iav_get_h264_config_ex(iav_context_t *context,
		iav_h264_config_ex_t __user *arg)
{
	iav_h264_config_ex_t config;
	int i;

	if (copy_from_user(&config, arg, sizeof(config)))
		return -EFAULT;

	if (get_single_stream_num(config.id, &i) < 0) {
		return -EINVAL;
	}
	config = G_encode_config[i].h264_encode_config;

	// get pic info for playback
	get_pic_info_in_h264(i, &config);

	return copy_to_user(arg, &config, sizeof(config)) ? -EFAULT : 0;
}

static int iav_set_framerate_factor_ex(iav_context_t * context,
		iav_change_framerate_factor_ex_t __user * arg)
{
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_change_framerate_factor_ex_t frame_factor, fr_pre;
	u32 flags;
	int i;

	if (copy_from_user(&frame_factor, arg, sizeof(frame_factor)))
		return -EFAULT;

	if (get_single_stream_num(frame_factor.id, &i) < 0) {
		return -EINVAL;
	}
	if (check_framerate_factor_ex(i, &frame_factor) < 0) {
		return -EINVAL;
	}
	fr_pre.ratio_numerator = G_encode_config[i].frame_rate_multiplication_factor;
	fr_pre.ratio_denominator = G_encode_config[i].frame_rate_division_factor;
	fr_pre.keep_fps_in_ss = G_encode_config[i].keep_fps_in_ss;
	G_encode_config[i].frame_rate_multiplication_factor =
		frame_factor.ratio_numerator;
	G_encode_config[i].frame_rate_division_factor =
		frame_factor.ratio_denominator;
	G_encode_config[i].keep_fps_in_ss = frame_factor.keep_fps_in_ss;

	if (check_encode_resource_limit(0, vin_info->frame_rate) < 0) {
		iav_error("Cannot change frame factor, system resource not enough!\n");
		G_encode_config[i].frame_rate_multiplication_factor =
			fr_pre.ratio_numerator;
		G_encode_config[i].frame_rate_division_factor =
			fr_pre.ratio_denominator;
		G_encode_config[i].keep_fps_in_ss = fr_pre.keep_fps_in_ss;
		return -EINVAL;
	}
	if ((fr_pre.ratio_numerator != frame_factor.ratio_numerator) ||
		(fr_pre.ratio_denominator != frame_factor.ratio_denominator)) {
		if (is_iav_state_prev_or_enc()) {
			flags = UPDATE_FRAME_FACTOR_FLAG;
			if (__is_enc_h264(i)) {
				flags |= UPDATE_STREAM_BITRATE_FLAG;
			}
			cmd_update_encode_params_ex(context, i, flags);
		}
	}

	return 0;
}

static int iav_sync_framerate_factor_ex(iav_context_t * context,
		iav_sync_framerate_factor_ex_t __user * arg)
{
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_sync_framerate_factor_ex_t fr_sync, frame_factor_pre;
	u32 flags;
	int i, stream_id[IAV_MAX_ENCODE_STREAMS_NUM], stream_num;

	if (copy_from_user(&fr_sync, arg, sizeof(fr_sync)))
		return -EFAULT;

	for (i = 0, stream_num = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (fr_sync.enable[i]) {
			if (get_single_stream_num(fr_sync.framefactor[i].id,
					&stream_id[i]) < 0)
				return -EINVAL;
			if (check_framerate_factor_ex(stream_id[i],
					&fr_sync.framefactor[i]) < 0)
				return -EINVAL;

			frame_factor_pre.framefactor[i].ratio_numerator =
				G_encode_config[i].frame_rate_multiplication_factor;
			frame_factor_pre.framefactor[i].ratio_denominator =
				G_encode_config[i].frame_rate_division_factor;
			G_encode_config[i].frame_rate_multiplication_factor =
				fr_sync.framefactor[i].ratio_numerator;
			G_encode_config[i].frame_rate_division_factor =
				fr_sync.framefactor[i].ratio_denominator;

			if ((frame_factor_pre.framefactor[i].ratio_numerator ==
					fr_sync.framefactor[i].ratio_numerator) &&
				(frame_factor_pre.framefactor[i].ratio_denominator ==
					fr_sync.framefactor[i].ratio_denominator)) {
				fr_sync.enable[i] = 0;
			}
			++stream_num;
		}
	}

	if (check_encode_resource_limit(0, vin_info->frame_rate) < 0) {
		iav_error("Cannot sync frame factor, system resource not enough!\n");

		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (fr_sync.enable[i]) {
				G_encode_config[i].frame_rate_multiplication_factor =
					frame_factor_pre.framefactor[i].ratio_numerator;
				G_encode_config[i].frame_rate_division_factor =
					frame_factor_pre.framefactor[i].ratio_denominator;
			}
		}
		return -EINVAL;
	}

	if (is_iav_state_prev_or_enc()) {
		/* Issue batch cmd to change all streams frame rate at the same time */
		check_sync_in_batch_cmd(fr_sync.enable[0]);
		dsp_start_enc_batch_cmd();
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (fr_sync.enable[i]) {
				flags = UPDATE_FRAME_FACTOR_FLAG;
				if (__is_enc_h264(i)) {
					flags |= UPDATE_STREAM_BITRATE_FLAG;
				}
				cmd_update_encode_params_ex(context, stream_id[i], flags);
			}
		}
		dsp_end_enc_batch_cmd();
	}

	return 0;
}

static int iav_get_framerate_factor_ex(iav_context_t * context,
		iav_change_framerate_factor_ex_t __user * arg)
{
	int i;
	iav_change_framerate_factor_ex_t frame_factor;

	if (copy_from_user(&frame_factor, arg, sizeof(frame_factor)))
		return -EFAULT;

	if (get_single_stream_num(frame_factor.id, &i) < 0) {
		return -EINVAL;
	}
	frame_factor.ratio_numerator = G_encode_config[i].frame_rate_multiplication_factor;
	frame_factor.ratio_denominator = G_encode_config[i].frame_rate_division_factor;
	frame_factor.keep_fps_in_ss = G_encode_config[i].keep_fps_in_ss;

	return copy_to_user(arg, &frame_factor, sizeof(frame_factor)) ? -EFAULT : 0;
}

static int iav_set_bitrate_ex(iav_context_t *context, iav_bitrate_info_ex_t __user *arg)
{
	struct amba_vin_src_capability * vin_info = get_vin_capability();
	iav_h264_config_ex_t * h264 = NULL;
	iav_bitrate_info_ex_t bitrate_info;
	u32 target_qp, resolution, previous_br, flags;
	int i;

	if (copy_from_user(&bitrate_info, arg, sizeof(bitrate_info)))
		return -EFAULT;

	if (get_single_stream_num(bitrate_info.id, &i) < 0) {
		return -EINVAL;
	}
	resolution = G_encode_stream[i].format.encode_width *
		G_encode_stream[i].format.encode_height;
	h264 = &G_encode_config[i].h264_encode_config;
	previous_br = h264->average_bitrate;

	switch (bitrate_info.rate_control_mode) {
		case IAV_CBR:
			h264->bitrate_control = IAV_BRC_SCBR;
			h264->average_bitrate = bitrate_info.cbr_avg_bitrate;
			h264->qp_min_on_I = 1;
			h264->qp_max_on_I = H264_QP_MAX;
			h264->qp_min_on_P = 1;
			h264->qp_max_on_P = H264_QP_MAX;
			h264->qp_min_on_B = 1;
			h264->qp_max_on_B = H264_QP_MAX;
			h264->skip_flag = H264_WITHOUT_FRAME_DROP;
			break;
		case IAV_CBR_QUALITY_KEEPING:
			if (calc_target_qp(bitrate_info.cbr_avg_bitrate,
						resolution, &target_qp) < 0) {
				return -EINVAL;
			}
			h264->bitrate_control = IAV_BRC_SCBR;
			h264->average_bitrate = bitrate_info.cbr_avg_bitrate;
			h264->qp_min_on_I = 1;
			h264->qp_max_on_I = target_qp * 6 / 5;
			h264->qp_min_on_P = 1;
			h264->qp_max_on_P = target_qp * 6 / 5;
			h264->qp_min_on_B = 1;
			h264->qp_max_on_B = target_qp * 6 / 5;
			h264->skip_flag = H264_WITH_FRAME_DROP;	// enable frame dropping
			break;
		case IAV_VBR:
			if (calc_target_qp(bitrate_info.vbr_min_bitrate,
						resolution, &target_qp) < 0) {
				return -EINVAL;
			}
			h264->bitrate_control = IAV_BRC_SCBR;
			h264->average_bitrate = bitrate_info.vbr_max_bitrate;
			h264->qp_min_on_I = target_qp;
			h264->qp_max_on_I = H264_QP_MAX;
			h264->qp_min_on_P = target_qp;
			h264->qp_max_on_P = H264_QP_MAX;
			h264->qp_min_on_B = target_qp;
			h264->qp_max_on_B = H264_QP_MAX;
			h264->skip_flag = H264_WITHOUT_FRAME_DROP;
			break;
		case IAV_VBR_QUALITY_KEEPING:
			if (calc_target_qp(bitrate_info.vbr_min_bitrate,
						resolution, &target_qp) < 0) {
				return -EINVAL;
			}
			h264->bitrate_control = IAV_BRC_SCBR;
			h264->average_bitrate = bitrate_info.vbr_max_bitrate;
			h264->qp_min_on_I = target_qp;
			h264->qp_max_on_I = target_qp;
			h264->qp_min_on_P = target_qp;
			h264->qp_max_on_P = target_qp;
			h264->qp_min_on_B = target_qp;
			h264->qp_max_on_B = target_qp;
			h264->skip_flag = H264_WITH_FRAME_DROP;	// enable frame dropping
			break;
		case IAV_CBR2:
			h264->bitrate_control = IAV_BRC_CBR;
			h264->average_bitrate = bitrate_info.cbr_avg_bitrate;
			break;

		default:
			iav_error("Unknown rate control mode [%d] !\n", bitrate_info.rate_control_mode);
			return -EINVAL;
			break;
	}

	// stream is in encoding state, changing bitrate on the fly
	if (__is_stream_in_h264_encoding(i)) {
		if (check_encode_resource_limit(0, vin_info->frame_rate) < 0) {
			iav_error("Cannot change bitrate, system resource is not enough!\n");
			h264->average_bitrate = previous_br;
			return -EINVAL;
		}
		flags = UPDATE_STREAM_BITRATE_FLAG | UPDATE_SCBR_PARAM_FLAG;
		cmd_update_encode_params_ex(context, i, flags);
	}
	G_rate_control_info[i] = bitrate_info;

	return 0;
}

static int iav_get_bitrate_ex(iav_context_t *context, iav_bitrate_info_ex_t __user *arg)
{
	int i;
	iav_bitrate_info_ex_t bitrate_info;

	if (copy_from_user(&bitrate_info, arg, sizeof(bitrate_info)))
		return -EFAULT;

	if (get_single_stream_num(bitrate_info.id, &i) < 0) {
		return -EINVAL;
	}
	bitrate_info = G_rate_control_info[i];

	return copy_to_user(arg, &bitrate_info, sizeof(bitrate_info)) ? -EFAULT : 0;
}

static int iav_set_h264_enc_param_ex(iav_context_t * context,
		iav_h264_enc_param_ex_t __user * arg)
{
	int rval = 0;
	iav_h264_enc_param_ex_t enc_param;

	if (copy_from_user(&enc_param, arg, sizeof(enc_param)))
		return -EFAULT;

	rval = set_h264_rt_enc_param(context, &enc_param, 0);

	return 0;
}

static int iav_get_h264_enc_param_ex(iav_context_t * context,
		iav_h264_enc_param_ex_t __user * arg)
{
	int rval = 0;
	iav_h264_enc_param_ex_t enc_param;

	if (copy_from_user(&enc_param, arg, sizeof(enc_param)))
		return -EFAULT;

	rval = get_h264_rt_enc_param(&enc_param);
	if (rval < 0)
		return rval;

	return copy_to_user(arg, &enc_param, sizeof(enc_param)) ? -EFAULT : 0;
}

static int set_h264_gop(iav_context_t *context,
		iav_change_gop_ex_t *change_gop, int sync_frame_flag)
{
	int i;
	iav_h264_config_ex_t config;
	if (get_single_stream_num(change_gop->id, &i) < 0) {
		return -EINVAL;
	}
	config = G_encode_config[i].h264_encode_config;

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	/* Add extra 4bits for long gop N, total 12bits for long gop N */
	config.N = (change_gop->N > 0xfff) ? 0xfff : change_gop->N;
#else
	config.N = (change_gop->N > 255) ? 255 : change_gop->N;
#endif

	config.idr_interval = (change_gop->idr_interval > 255) ?
		255 : change_gop->idr_interval;
	if (check_h264_config(i, &config) < 0) {
		iav_error("Invalid H264 config for stream [%d].\n", i);
		return -EINVAL;
	}
	G_encode_config[i].h264_encode_config = config;
	if (__is_stream_in_h264_encoding(i)) {
		if (sync_frame_flag) {
			update_sync_frame_enc_params_ex(context, i, UPDATE_GOP_PARAM_FLAG);
		} else {
			cmd_update_encode_params_ex(context, i, UPDATE_GOP_PARAM_FLAG);
		}
	}

	return 0;
}

static int iav_change_gop_ex(iav_context_t *context,
		iav_change_gop_ex_t __user *arg)
{
	iav_change_gop_ex_t change_gop;

	if (copy_from_user(&change_gop, arg, sizeof(change_gop)))
		return -EFAULT;

	if (set_h264_gop(context, &change_gop, 0) < 0) {
		return -EINVAL;
	}

	return 0;
}

static int iav_set_chroma_format_ex(iav_context_t * context,
		iav_chroma_format_info_ex_t __user * arg)
{
	iav_chroma_format_info_ex_t chroma;
	int i;

	if (copy_from_user(&chroma, arg, sizeof(chroma)))
		return -EFAULT;

	if (get_single_stream_num(chroma.id, &i) < 0) {
		return -EINVAL;
	}

	if (check_chroma_format(i, chroma.chroma_format) < 0) {
		iav_error("Invalid chroma format param!\n");
		return -EINVAL;
	}
	if (__is_enc_h264(i)) {
		G_encode_config[i].h264_encode_config.chroma_format =
			chroma.chroma_format;
	} else if (__is_enc_mjpeg(i)) {
		G_encode_config[i].jpeg_encode_config.chroma_format =
			chroma.chroma_format;
	}

	if (__is_stream_in_encoding(i)) {
		cmd_update_encode_params_ex(context, i, UPDATE_MONOCHROME_FLAG);
	}

	return 0;
}

static int iav_s_frame_drop(iav_context_t * context,
		iav_frame_drop_info_ex_t * arg, int sync_frame_flag)
{
	int i;

	if (get_single_stream_num(arg->id, &i) < 0) {
		return -EINVAL;
	}

	if (check_frame_drop(i, arg->drop_frames_num) < 0) {
		iav_error("Invalid frame drop format param!\n");
		return -EINVAL;
	}

	G_encode_config[i].frame_drop = arg->drop_frames_num;

	if (__is_stream_in_encoding(i)) {
		if (sync_frame_flag) {
			update_sync_frame_enc_params_ex(context, i, UPDATE_FRAME_DROP_FLAG);
		} else {
			cmd_update_encode_params_ex(context, i, UPDATE_FRAME_DROP_FLAG);
		}
	}

	return 0;
}

static int iav_set_qp_limit_ex(iav_context_t * context,
		iav_change_qp_limit_ex_t __user * arg)
{
	iav_h264_config_ex_t * h264 = NULL;
	iav_change_qp_limit_ex_t qp_limit;
	int i;

	if (copy_from_user(&qp_limit, arg, sizeof(qp_limit)))
		return -EFAULT;

	if (get_single_stream_num(qp_limit.id, &i) < 0) {
		return -EINVAL;
	}
	if (check_qp_limit(i, &qp_limit) < 0) {
		return -EINVAL;
	}

	h264 = &G_encode_config[i].h264_encode_config;
	h264->qp_min_on_I = qp_limit.qp_min_on_I;
	h264->qp_max_on_I = qp_limit.qp_max_on_I;
	h264->qp_min_on_P = qp_limit.qp_min_on_P;
	h264->qp_max_on_P = qp_limit.qp_max_on_P;
	h264->qp_min_on_B = qp_limit.qp_min_on_B;
	h264->qp_max_on_B = qp_limit.qp_max_on_B;
	h264->qp_min_on_C = qp_limit.qp_min_on_C;
	h264->qp_max_on_C = qp_limit.qp_max_on_C;
	h264->adapt_qp = qp_limit.adapt_qp;
	h264->i_qp_reduce = qp_limit.i_qp_reduce;
	h264->p_qp_reduce = qp_limit.p_qp_reduce;
	h264->b_qp_reduce = qp_limit.b_qp_reduce;
	h264->skip_flag = qp_limit.skip_flag;

#ifndef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	h264->qp_min_on_Q = qp_limit.qp_min_on_Q;
	h264->qp_max_on_Q = qp_limit.qp_max_on_Q;
	h264->q_qp_reduce = qp_limit.q_qp_reduce;
	h264->log_q_num_minus_1 = qp_limit.log_q_num_minus_1;
#endif

	if (__is_stream_in_h264_encoding(i)) {
		cmd_update_encode_params_ex(context, i, UPDATE_SCBR_PARAM_FLAG);
	}

	return 0;
}

static int iav_get_qp_limit_ex(iav_context_t * context,
		iav_change_qp_limit_ex_t __user * arg)
{
	int i;
	iav_change_qp_limit_ex_t qp_limit;

	if (copy_from_user(&qp_limit, arg, sizeof(qp_limit)))
		return -EFAULT;

	if (get_single_stream_num(qp_limit.id, &i) < 0) {
		return -EINVAL;
	}

	get_qp_limit(i, &qp_limit);

	return copy_to_user(arg, &qp_limit, sizeof(qp_limit)) ? -EFAULT : 0;
}

static int set_qp_roi_matrix(iav_context_t * context,
		iav_qp_roi_matrix_ex_t * qp_matrix,  int sync_frame_flag)
{
	iav_h264_config_ex_t * h264 = NULL;
	int stream, i;

	if (get_single_stream_num(qp_matrix->id, &stream) < 0) {
		return -EINVAL;
	}

	// check user space address mapping
	if (!sync_frame_flag) {
		if (context->qp_matrix.user_start == NULL) {
			iav_error("QP ROI matrix is NOT mapped!\n");
			return -EAGAIN;
		}
	}

	// check QP matrix parameter
	if (check_qp_matrix_param(stream, qp_matrix) < 0) {
		iav_error("QP ROI matrix parameters error!\n");
		return -EINVAL;
	}

	h264 = &G_encode_config[stream].h264_encode_config;
	h264->qp_roi_enable = qp_matrix->enable;
	h264->qp_roi_type = qp_matrix->type;

	for (i = 0; i < NUM_PIC_TYPES; ++i) {
		h264->qp_delta[i][0] = 0;		// Leave user class 0 as 0 all the time
		h264->qp_delta[i][1] = qp_matrix->delta[i][1];
		h264->qp_delta[i][2] = qp_matrix->delta[i][2];
		h264->qp_delta[i][3] = qp_matrix->delta[i][3];
	}

	// now udpate QP matrix
	if (__is_stream_in_h264_encoding(stream)) {
		if (sync_frame_flag) {
			update_sync_frame_enc_params_ex(context, stream,
				UPDATE_QP_ROI_MATRIX_FLAG);
		} else {
			cmd_update_encode_params_ex(context,
				stream, UPDATE_QP_ROI_MATRIX_FLAG);
		}
	}

	return 0;
}

static int iav_set_qp_roi_matrix_ex(iav_context_t * context,
		iav_qp_roi_matrix_ex_t __user * arg)
{
	int stream;
	iav_qp_roi_matrix_ex_t qp_matrix;

	if (copy_from_user(&qp_matrix, arg, sizeof(qp_matrix)))
		return -EFAULT;

	/* fix me, the old struct for the old test app has no daddr  */
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	qp_matrix.daddr = NULL;
#else
	qp_matrix.daddr[QP_FRAME_I] = NULL;
#endif

	if (get_single_stream_num(qp_matrix.id, &stream) < 0) {
		return -EINVAL;
	}

	// check user space address mapping
	if (context->qp_matrix.user_start == NULL) {
		iav_error("QP ROI matrix is NOT mapped!\n");
		return -EAGAIN;
	}

	G_qp_matrix_current_daddr = G_qp_matrix_map +
		stream * STREAM_QP_MATRIX_MEM_SIZE;

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if (qp_matrix.daddr != NULL) {
		if (copy_from_user(G_qp_matrix_current_daddr, qp_matrix.daddr,
			STREAM_QP_MATRIX_MEM_SIZE))
			return -EFAULT;
	}
#else
	if (qp_matrix.daddr[QP_FRAME_I] != NULL) {
		if (copy_from_user(G_qp_matrix_current_daddr, qp_matrix.daddr[QP_FRAME_I],
			STREAM_QP_MATRIX_MEM_SIZE))
			return -EFAULT;
	}
#endif

	if (set_qp_roi_matrix(context, &qp_matrix, 0) < 0) {
		return -EINVAL;
	}

	return 0;
}

static int get_qp_roi_matrix(iav_context_t * context,
	iav_qp_roi_matrix_ex_t * qp_matrix, int sync_frame_flag)
{
	int stream, i;
	iav_h264_config_ex_t * config = NULL;

	if (get_single_stream_num(qp_matrix->id, &stream) < 0) {
		return -EINVAL;
	}

	// check user space address mapping
	if (!sync_frame_flag) {
		if (context->qp_matrix.user_start == NULL) {
			iav_error("QP ROI matrix is NOT mapped!\n");
			return -EAGAIN;
		}
	}

	config = &G_encode_config[stream].h264_encode_config;
	qp_matrix->enable = config->qp_roi_enable;
	qp_matrix->type = config->qp_roi_type;

	for (i = 0; i < NUM_PIC_TYPES; ++i) {
		qp_matrix->delta[i][0] = config->qp_delta[i][0];
		qp_matrix->delta[i][1] = config->qp_delta[i][1];
		qp_matrix->delta[i][2] = config->qp_delta[i][2];
		qp_matrix->delta[i][3] = config->qp_delta[i][3];
	}

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	if(sync_frame_flag) {
		if ((G_frame_sync_index - 1) < 0) {
			G_qp_matrix_current_daddr = G_qp_matrix_map +
				stream * STREAM_QP_MATRIX_MEM_SIZE +
				(DEFAULT_TOGGLED_ENC_CMD_NUM - 1) *
				STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
		} else {
			G_qp_matrix_current_daddr = G_qp_matrix_map +
				stream * STREAM_QP_MATRIX_MEM_SIZE +
				(G_frame_sync_index - 1) *
				STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
		}
	} else {
		G_qp_matrix_current_daddr = G_qp_matrix_map +
			stream * STREAM_QP_MATRIX_MEM_SIZE;
	}
	iav_printk("Get:Qp matrix Phys addr of stream[%d] : daddr:0x%x\n",
		stream,  AMBVIRT_TO_DSP(G_qp_matrix_current_daddr));
#else
	G_qp_matrix_current_daddr = G_qp_matrix_map +
		stream * STREAM_QP_MATRIX_MEM_SIZE;
#endif

	return 0;
}

static int iav_get_qp_roi_matrix_ex(iav_context_t * context,
		iav_qp_roi_matrix_ex_t __user * arg)
{
	iav_qp_roi_matrix_ex_t qp_matrix;

	if (copy_from_user(&qp_matrix, arg, sizeof(qp_matrix)))
		return -EFAULT;

	/* fix me, the old struct for the old test app has no daddr  */
	if (get_qp_roi_matrix(context, &qp_matrix, 0) < 0) {
		return -EINVAL;
	}
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	qp_matrix.daddr = NULL;

	if (qp_matrix.daddr != NULL) {
		if (copy_to_user(qp_matrix.daddr, G_qp_matrix_current_daddr,
			STREAM_QP_MATRIX_MEM_SIZE))
			return -EFAULT;
	}
#else
	if (qp_matrix.daddr[QP_FRAME_I] != NULL) {
		if (copy_to_user(qp_matrix.daddr[QP_FRAME_I], G_qp_matrix_current_daddr,
			STREAM_QP_MATRIX_MEM_SIZE))
			return -EFAULT;
	}

#endif
	return copy_to_user(arg, &qp_matrix, sizeof(qp_matrix)) ? -EFAULT : 0;
}


static int iav_force_idr_ex(iav_context_t * context, iav_stream_id_t stream_id)
{
	u32 flags;
	int i;

	check_sync_in_batch_cmd(stream_id & (1 << 0));
	dsp_start_enc_batch_cmd();
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if ((stream_id & (1 << i)) && __is_stream_in_encoding(i)) {
			flags = UPDATE_FORCE_IDR_FLAG;
			cmd_update_encode_params_ex(context, i, flags);
		}
	}
	dsp_end_enc_batch_cmd();

	return 0;
}

static int iav_set_statistics_config_ex(iav_context_t * context,
		iav_enc_statis_config_ex_t __user * arg)
{
	int i;
	u32 flags = 0;
	iav_enc_statis_config_ex_t statis_config;

	if (copy_from_user(&statis_config, arg, sizeof(statis_config)))
		return -EFAULT;

	if (get_single_stream_num(statis_config.id, &i) < 0) {
		return -EINVAL;
	}
	if (check_statistics_config(i, &statis_config) < 0) {
		return -EINVAL;
	}
	update_statistics_config(i, &statis_config, &flags);

	if (__is_stream_in_encoding(i)) {
		cmd_enable_encoder_statistics(context, i, flags);
	}

	return 0;
}

static int iav_get_statistics_config_ex(iav_context_t * context,
		iav_enc_statis_config_ex_t __user * arg)
{
	int i;
	iav_enc_statis_config_ex_t statis_config;
	iav_stream_statis_config_ex_t * statis = NULL;

	if (copy_from_user(&statis_config, arg, sizeof(statis_config)))
		return -EFAULT;

	if (get_single_stream_num(statis_config.id, &i) < 0) {
		return -EINVAL;
	}
	statis = &G_encode_stream[i].statis;
	if (statis->enable_flags & ENABLE_MVDUMP_FLAG) {
		statis_config.enable_mv_dump = 1;
		statis_config.mvdump_division_factor = statis->mvdump_division_factor;
	}
	if (statis->enable_flags & ENABLE_QP_HIST_DUMP_FLAG) {
		statis_config.enable_qp_hist = 1;
	}

	return copy_to_user(arg, &statis_config, sizeof(statis_config)) ? -EFAULT : 0;
}

static int iav_fetch_statistics_info_ex(iav_context_t * context,
	iav_enc_statis_info_ex_t __user * arg)
{
	int retv = 0, stream;
	iav_enc_statis_info_ex_t statis_info;
	iav_statis_info_slot_ex_t statis_slot;

	if (copy_from_user(&statis_info, arg, sizeof(statis_info))) {
		return -EFAULT;
	}
	if (statis_info.data_type & IAV_MV_DUMP_FLAG) {
		if (!__is_mv_mapped(context)) {
			iav_error("Failed to map MV, cannot read enocde statistics info!\n");
			return -EFAULT;
		}
	}
	if (statis_info.data_type & IAV_QP_HIST_DUMP_FLAG) {
		if (!__is_qp_hist_mapped(context)) {
			iav_error("Failed to map QP histogram, cannot read enocde statistics info!\n");
			return -EFAULT;
		}
	}
	stream = statis_info.stream;
	if (__is_invalid_stream(stream)) {
		iav_error("Invalid stream [%d].\n", stream);
		return -EINVAL;
	}
	if (!__is_stream_in_h264_encoding(stream)) {
		iav_error("Stream [%d] is not in encoding state!\n", stream);
		return -EINVAL;
	}

	memset(&statis_slot, 0, sizeof(statis_slot));
	statis_slot.flag = statis_info.flag;
	if (likely(is_interrupt_readout())) {
		retv = read_statis_info_by_interrupt(context, stream, &statis_slot);
	} else {
		retv = read_statis_info_by_polling(context, &statis_slot);
	}
	if (retv < 0) {
		return retv;
	}

	retv = update_statistics_info_ex(context, &statis_info, &statis_slot);
	if (retv < 0)
		return retv;

	return copy_to_user(arg, &statis_info, sizeof(statis_info)) ? -EFAULT : 0;
}

static int iav_release_statistics_info_ex(iav_context_t * context,
		iav_enc_statis_info_ex_t __user * arg)
{
	int statis_index;
	iav_enc_statis_info_ex_t statis_info;

	if (is_polling_readout())
		return 0;

	if (copy_from_user(&statis_info, arg, sizeof(statis_info)))
		return -EFAULT;

	statis_index = statis_info.statis_index;
	if (is_invalid_statis_index(statis_index)) {
		iav_error("Invalid statistics index [%d] to release, it's not in the "
			"range of [0~%d].\n", statis_index, NUM_STATIS_DESC);
		return -EINVAL;
	}
	release_statistics_to_queue(statis_index);
	return 0;
}

static int iav_update_vin_framerate(iav_context_t * context)
{
	iav_pts_info_t *pts_info = &G_encode_obj.pts_info;
	// When hw timer is enabled, do not update vin fps on the fly, keep dsp PTS stable
	if (pts_info->hwtimer_enabled == 0) {
		cmd_update_capture_params_ex(UPDATE_CUSTOM_VIN_FPS_FLAG);
	}

	return 0;
}

static int iav_s_h264_q_matrix(iav_context_t * context, iav_h264_q_matrix_ex_t * arg)
{
	int i;

	if (get_single_stream_num(arg->id, &i) < 0) {
		return -EINVAL;
	}
	init_q_matrix(i, &arg->qm_4x4[0][0]);
	return 0;
}

static int iav_s_h264_panic(iav_context_t *context, iav_panic_control_param_ex_t *arg)
{
	int i;
	iav_h264_config_ex_t *h264 = NULL;

	if (get_single_stream_num(arg->id, &i) < 0) {
		return -EINVAL;
	}
	h264 = &G_encode_config[i].h264_encode_config;
	if ((!arg->panic_num || !arg->panic_den) ||
		(arg->panic_num > 255 || arg->panic_den > 255)) {
		iav_error("Invalid panic control param [%d/%d].\n",
			arg->panic_num, arg->panic_den);
		return -EINVAL;
	}
	h264->panic_num = arg->panic_num;
	h264->panic_den = arg->panic_den;
	if (__is_stream_in_h264_encoding(i)) {
		cmd_update_encode_params_ex(context, i, UPDATE_PANIC_PARAM_FLAG);
	}

	return 0;
}

static int iav_set_stream_cfg_ex(iav_context_t * context, iav_stream_cfg_t __user * arg)
{
	int rval = 0;
	iav_stream_cfg_t cfg;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_H264CFG_ENC_PARAM:
		rval = set_h264_rt_enc_param(context, &cfg.arg.h_enc_param, 0);
		break;
	case IAV_H264CFG_Q_MATRIX:
		rval = iav_s_h264_q_matrix(context, &cfg.arg.h_qm);
		break;
	case IAV_H264CFG_PANIC:
		rval = iav_s_h264_panic(context, &cfg.arg.h_panic);
		break;
	case IAV_STMCFG_FRAME_DROP:
		rval = iav_s_frame_drop(context, &cfg.arg.frame_drop, 0);
		break;
	default:
		iav_error("Config ID [%d] is not supported yet.\n", cfg.cid);
		break;
	}

	return rval;
}

static int iav_get_stream_cfg_ex(iav_context_t * context, iav_stream_cfg_t __user * arg)
{
	int i;
	iav_stream_cfg_t cfg;
	iav_h264_config_ex_t * h264 = NULL;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_H264CFG_ENC_PARAM:
		if (get_h264_rt_enc_param(&cfg.arg.h_enc_param) < 0) {
			return -EINVAL;
		}
		break;
	case IAV_H264CFG_Q_MATRIX:
		if (get_single_stream_num(cfg.arg.h_qm.id, &i) < 0) {
			return -EINVAL;
		}
		h264 = &G_encode_config[i].h264_encode_config;
		memcpy((u8 *)cfg.arg.h_qm.qm_4x4, h264->qmatrix_4x4_daddr,
			Q_MATRIX_SIZE);
		break;
	case IAV_H264CFG_PANIC:
		if (get_single_stream_num(cfg.arg.h_panic.id, &i) < 0) {
			return -EINVAL;
		}
		h264 = &G_encode_config[i].h264_encode_config;
		cfg.arg.h_panic.panic_num = h264->panic_num;
		cfg.arg.h_panic.panic_den = h264->panic_den;
		break;
	case IAV_STMCFG_FRAME_DROP:
		if (get_single_stream_num(cfg.arg.frame_drop.id, &i) < 0) {
			return -EINVAL;
		}
		cfg.arg.frame_drop.drop_frames_num = G_encode_config[i].frame_drop;
		break;
	default:
		iav_error("Config ID [%d] is not supported yet.\n", cfg.cid);
		break;
	}

	return copy_to_user(arg, &cfg, sizeof(cfg)) ? -EFAULT : 0;
}


#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
/******************************************
 *
 *	encode with sync frame
 *
 ******************************************/

static int get_h264_gop(iav_change_gop_ex_t *arg)
{
	int stream;
	iav_h264_config_ex_t * h264 = NULL;

	if (get_single_stream_num(arg->id, &stream) < 0) {
		return -EINVAL;
	}
	h264 = &G_encode_config[stream].h264_encode_config;

	arg->N = h264->N;
	arg->idr_interval = h264->idr_interval;

	return 0;
}

static int set_h264_frame_sync_force_idr(iav_context_t * context,
	int stream)
{
	if (__is_stream_in_h264_encoding(stream)) {
		update_sync_frame_enc_params_ex(context, stream,
			UPDATE_FORCE_IDR_FLAG);
	}
	return 0;
}

static int check_qp_roi_frame_sync(iav_qp_roi_matrix_ex_t * qp_matrix)
{
	int stream;

	if (get_single_stream_num(qp_matrix->id, &stream) < 0) {
		return -1;
	}
	if (qp_matrix->daddr == NULL) {
		iav_error("stream[%c] qp_roi.daddr is NULL \n", 'A' + stream);
		return -1;
	}
	if (qp_matrix->size > STREAM_QP_MATRIX_MEM_SIZE) {
		iav_error("stream[%c] qp_roi size[%d] is larger than %d \n",
			'A' + stream, qp_matrix->size, STREAM_QP_MATRIX_MEM_SIZE);
		return -1;
	}

	return 0;
}

static int iav_cfg_frame_sync_proc(iav_context_t * context,
	iav_stream_cfg_t __user * arg)
{
	int rval = 0;
	iav_stream_cfg_t cfg;
	int stream;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_H264CFG_ENC_PARAM:
		rval = set_h264_rt_enc_param(context,
			&cfg.arg.h_enc_param, 1);
		break;
	case IAV_H264CFG_QP_ROI:
		if (get_single_stream_num(cfg.arg.h_qp_roi.id, &stream) < 0) {
			return -EINVAL;
		}
		if (check_qp_roi_frame_sync(&cfg.arg.h_qp_roi) < 0) {
			return -EINVAL;
		}
		G_qp_matrix_current_daddr = G_qp_matrix_map +
			stream * STREAM_QP_MATRIX_MEM_SIZE +
			G_frame_sync_index * STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
		if (copy_from_user(G_qp_matrix_current_daddr, cfg.arg.h_qp_roi.daddr,
			cfg.arg.h_qp_roi.size))
			return -EFAULT;
		rval = set_qp_roi_matrix(context, &cfg.arg.h_qp_roi, 1);
		break;
	case IAV_H264CFG_FORCE_IDR:
		rval = set_h264_frame_sync_force_idr(context, cfg.arg.h_force_idr_sid);
		break;
	case IAV_H264CFG_GOP:
		rval = set_h264_gop(context, &cfg.arg.h_gop, 1);
		break;
	case IAV_STMCFG_FRAME_DROP:
		rval = iav_s_frame_drop(context, &cfg.arg.frame_drop, 1);
		break;
	default:
		iav_error("Config ID [%d] is not supported yet.\n", cfg.cid);
		break;
	}

	return rval;
}

static int iav_get_frame_sync_proc(iav_context_t * context,
	iav_stream_cfg_t __user * arg)
{
	int rval = 0;
	iav_stream_cfg_t cfg;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_H264CFG_ENC_PARAM:
		rval = get_h264_rt_enc_param(&cfg.arg.h_enc_param);
		break;
	case IAV_H264CFG_QP_ROI:
		if ((rval = check_qp_roi_frame_sync(&cfg.arg.h_qp_roi)) < 0) {
			break;
		}
		if ((rval = get_qp_roi_matrix(context, &cfg.arg.h_qp_roi, 1)) < 0) {
			break;
		}
		if (copy_to_user(cfg.arg.h_qp_roi.daddr, G_qp_matrix_current_daddr,
			cfg.arg.h_qp_roi.size)) {
			return -EFAULT;
		}
		break;
	case IAV_H264CFG_GOP:
		rval = get_h264_gop(&cfg.arg.h_gop);
		break;
	default:
		iav_error("Config ID [%d] is not supported yet.\n", cfg.cid);
		break;
	}

	return copy_to_user(arg, &cfg, sizeof(cfg)) ? -EFAULT : 0;
}

static int iav_apply_frame_sync_proc(iav_context_t *context, u32 arg)
{
	u8 * addr = NULL;

	if (cmd_apply_frame_sync_cmd(arg) < 0) {
		iav_error("cmd_apply_frame_sync_cmd\n");
		return 0;
	}

	G_frame_sync_index = (G_frame_sync_index + 1) %
		DEFAULT_TOGGLED_ENC_CMD_NUM;
	addr = get_dummy_enc_kernel_address(G_frame_sync_index, 0);
	memset(addr, 0, DSP_CMD_SIZE * IAV_MAX_ENCODE_STREAMS_NUM);
	clean_cache_aligned(addr, IAV_MAX_ENCODE_STREAMS_NUM *
		DSP_CMD_SIZE);

	addr = G_qp_matrix_map +
		G_frame_sync_index * STREAM_QP_MATRIX_MEM_TOTAL_SIZE;
	memset(addr, 0, STREAM_QP_MATRIX_MEM_TOTAL_SIZE);

	return 0;
}

#else
static int iav_cfg_frame_sync_proc(iav_context_t * context,
	iav_stream_cfg_t __user * arg)
{
	iav_error("%s is not supported yet.\n", __func__);
	return -EINVAL;
}

static int iav_get_frame_sync_proc(iav_context_t * context,
	iav_stream_cfg_t __user * arg)
{
	iav_error("%s is not supported yet.\n", __func__);
	return -EINVAL;
}

static int iav_apply_frame_sync_proc(iav_context_t *context, u32 arg)
{
	iav_error("%s is not supported yet.\n", __func__);
	return -EINVAL;
}

#endif
/******************************************
 *
 *	External IAV functions
 *
 ******************************************/
int get_stream_offset_param(iav_stream_offset_t* param)
{
	int stream_id;
	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	param->x = G_encode_stream[stream_id].format.encode_x;
	param->y = G_encode_stream[stream_id].format.encode_y;
	return 0;
}

int cfg_stream_offset_param(iav_stream_offset_t* param)
{
	int stream_id;
	if (get_single_stream_num(param->id, &stream_id) < 0) {
		return -1;
	}
	if (check_stream_offset(param, stream_id) < 0) {
		return -1;
	}
	G_encode_stream[stream_id].format.encode_x = param->x;
	G_encode_stream[stream_id].format.encode_y = param->y;
	return 0;
}

int mem_alloc_jpeg_qt_matrix(u8 ** ptr, int * alloc_size)
{
	int i, total_size;
	u8 * addr = NULL;

	total_size = IAV_MAX_ENCODE_STREAMS_NUM * JPEG_QT_SIZE *
		DEFAULT_TOGGLED_BUFFER_NUM;
	if ((addr = kzalloc(total_size, GFP_KERNEL)) == NULL) {
		iav_error("Not enough memory to allocate JPEG QT matrix!\n");
		return -1;
	}
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		G_encode_config[i].jpeg_encode_config.jpeg_quant_matrix = addr +
			i * JPEG_QT_SIZE;
	}
	*ptr = addr;
	*alloc_size = total_size;
	G_quant_matrix_addr = addr;

	return 0;
}

int mem_alloc_q_matrix(u8 * * ptr, int * alloc_size)
{
	int i, total_size;
	u8 * addr = NULL;

	total_size = TOTAL_Q_MATRIX_SIZE;
	if ((addr = kzalloc(total_size, GFP_KERNEL)) == NULL) {
		iav_error("Not enough memory to allocate Q matrix for scaling list.\n");
		return -1;
	}
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
	}
	*ptr = addr;
	*alloc_size = total_size;
	G_q_matrix_addr = addr;

	return 0;
}

//only run iav_encode_init once
int iav_encode_init(void)
{
	int rval;
	struct iav_mem_block *qp_matrix = NULL;

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	struct iav_mem_block *cmd_sync = NULL;
#endif

	if ((rval = iav_warp_init()) < 0)
		return rval;

	// init encode
	prepare_encode_init();
	reset_encode_obj();
	hw_pts_init();
	iav_init_compl(&G_encode_obj.enc_msg_compl);
	iav_init_compl(&G_encode_obj.vcap_msg_compl);
	iav_init_compl(&G_encode_obj.vsync_loss_compl);
	G_iav_obj.stream = &G_encode_stream[0];

	iav_get_mem_block(IAV_MMAP_QP_MATRIX, &qp_matrix);
	G_qp_matrix_map = qp_matrix->kernel_start;

#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	iav_get_mem_block(IAV_MMAP_CMD_SYNC, &cmd_sync);
	G_frame_sync_map = cmd_sync->kernel_start;
#endif
	// Initialize frame counter semaphore
	init_waitqueue_head(&G_encode_obj.enc_wq);
	sema_init(&G_encode_obj.sem_statistics, 0);

	// Initialize frame management
	memset(G_encode_obj.ext_bits_desc_start, 0,
		sizeof(dsp_extend_bits_info_t) * NUM_BS_DESC);
	frame_info_queue_init();
	memset(G_encode_obj.ext_statis_desc_start, 0,
		sizeof(dsp_extend_enc_stat_info_t) * NUM_STATIS_DESC);
	statis_info_queue_init();

	// set msg handler
	dsp_set_cat_msg_handler(handle_vdsp_msg, CAT_H264_ENC, NULL);
	dsp_set_cat_msg_handler(handle_vcap_msg, CAT_VCAP, NULL);

#ifdef CONFIG_GUARD_VSYNC_LOSS
	iav_vsync_guard_init();
#endif

	return 0;
}

void iav_encode_exit(void)
{
	// Todo
	hw_pts_deinit();
}

int __iav_encode_ioctl(iav_context_t *context, unsigned int cmd, unsigned long arg)
{
	int retv;

	switch (cmd) {
		case IAV_IOC_GET_PREVIEW_FORMAT:
			retv = iav_get_preview_format(context,
					(iav_preview_format_t __user *)arg);
			break;
		case IAV_IOC_ENABLE_PREVIEW:
			retv = iav_enable_preview(context);
			break;
		case IAV_IOC_DISABLE_PREVIEW:
			retv = iav_disable_preview(context);
			break;

		case IAV_IOC_GET_BSB_FULLNESS:
			retv = iav_get_bsb_fullness(context, (iav_bsb_fullness_t __user *)arg);
			break;

		case IAV_IOC_READ_BITSTREAM_EX:
			retv = iav_read_encode_fifo_ex(context, (bits_info_ex_t __user *)arg);
			break;

		case IAV_IOC_FETCH_FRAME_EX:
			retv = iav_fetch_encoded_frame_ex(context, (bits_info_ex_t __user *)arg);
			break;
		case IAV_IOC_RELEASE_FRAME_EX:
			retv = iav_release_encoded_frame_ex(context, (bits_info_ex_t __user *)arg);
			break;

		case IAV_IOC_START_ENCODE_EX:
			retv = iav_start_encode_ex(context, (iav_stream_id_t)arg);
			break;

		case IAV_IOC_STOP_ENCODE_EX:
			retv = iav_stop_encode_ex(context, (iav_stream_id_t)arg);
			break;

		case IAV_IOC_SET_JPEG_CONFIG_EX:
			retv = iav_set_jpeg_config_ex(context,
					(struct iav_jpeg_config_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_JPEG_CONFIG_EX:
			retv = iav_get_jpeg_config_ex(context,
					(struct iav_jpeg_config_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_H264_CONFIG_EX:
			retv = iav_set_h264_config_ex(context,
					(struct iav_h264_config_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_H264_CONFIG_EX:
			retv = iav_get_h264_config_ex(context,
					(struct iav_h264_config_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_BITRATE_EX:
			retv = iav_set_bitrate_ex(context,
					(struct iav_bitrate_info_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_BITRATE_EX:
			retv = iav_get_bitrate_ex(context,
					(struct iav_bitrate_info_ex_s __user *)arg);
			break;

		case IAV_IOC_FORCE_IDR_EX:
			retv = iav_force_idr_ex(context, (iav_stream_id_t)arg);
			break;

		case IAV_IOC_CHANGE_GOP_EX:
			retv = iav_change_gop_ex(context,
					(struct iav_change_gop_ex_s __user *)arg);
			break;

		case IAV_IOC_CHANGE_QP_LIMIT_EX:
			retv = iav_set_qp_limit_ex(context,
					(iav_change_qp_limit_ex_t __user *)arg);
			break;
		case IAV_IOC_GET_QP_LIMIT_EX:
			retv = iav_get_qp_limit_ex(context,
					(iav_change_qp_limit_ex_t __user *)arg);
			break;

		case IAV_IOC_SET_ENCODE_FORMAT_EX:
			retv = iav_set_encode_format_ex(context,
					(struct iav_encode_format_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_ENCODE_FORMAT_EX:
			retv = iav_get_encode_format_ex(context,
					(struct iav_encode_format_ex_s __user *)arg);
			break;

		case IAV_IOC_GET_ENCODE_STREAM_INFO_EX:
			retv = iav_get_encode_stream_info_ex(context,
					(struct iav_encode_stream_info_ex_s __user *)arg);
			break;

		case IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX:
			retv = iav_set_framerate_factor_ex(context,
					(struct iav_change_framerate_factor_ex_s __user *)arg);
			break;
		case IAV_IOC_SYNC_FRAMERATE_FACTOR_EX:
			retv = iav_sync_framerate_factor_ex(context,
					(struct iav_sync_framerate_factor_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_FRAMERATE_FACTOR_EX:
			retv = iav_get_framerate_factor_ex(context,
					(struct iav_change_framerate_factor_ex_s __user *)arg);
			break;

		case IAV_IOC_GET_H264_ENC_PARAM_EX:
			retv = iav_get_h264_enc_param_ex(context,
					(struct iav_h264_enc_param_ex_s __user *)arg);
			break;
		case IAV_IOC_SET_H264_ENC_PARAM_EX:
			retv = iav_set_h264_enc_param_ex(context,
					(struct iav_h264_enc_param_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_CHROMA_FORMAT_EX:
			retv = iav_set_chroma_format_ex(context,
					(struct iav_chroma_format_info_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_SHARPEN_FILTER_CONFIG_EX:
			retv = iav_set_sharpen_filter_config_ex(context,
					(struct iav_sharpen_filter_cfg_s __user *)arg);
			break;
		case IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX:
			retv = iav_get_sharpen_filter_config_ex(context,
					(struct iav_sharpen_filter_cfg_s __user *)arg);
			break;

		case IAV_IOC_GET_SOURCE_BUFFER_INFO_EX:
			retv = iav_get_source_buffer_info_ex(context,
					(struct iav_source_buffer_info_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX:
			retv = iav_set_source_buffer_type_all_ex(context,
					(struct iav_source_buffer_type_all_ex_s __user *) arg);
			break;

		case IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX:
			retv = iav_get_source_buffer_type_all_ex(context,
					(struct iav_source_buffer_type_all_ex_s __user *) arg);
			break;

		case IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX:
			retv = iav_set_source_buffer_format_all_ex(context,
					(struct iav_source_buffer_format_all_ex_s __user *)arg);
			break;

		case IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX:
			retv = iav_get_source_buffer_format_all_ex(context,
					(struct iav_source_buffer_format_all_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_SOURCE_BUFFER_SETUP_EX:
			retv = iav_set_source_buffer_setup_ex(context,
					(struct iav_source_buffer_setup_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX:
			retv = iav_get_source_buffer_setup_ex(context,
					(struct iav_source_buffer_setup_ex_s __user *)arg);
			break;
		case IAV_IOC_SET_SOURCE_BUFFER_FORMAT_EX:
			retv = iav_set_source_buffer_format_ex(context,
					(struct iav_source_buffer_format_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX:
			retv = iav_get_source_buffer_format_ex(context,
					(struct iav_source_buffer_format_ex_s __user *)arg);
			break;
		case IAV_IOC_SET_DIGITAL_ZOOM_EX:
			retv = iav_set_digital_zoom_ex(context,
					(struct iav_digital_zoom_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_DIGITAL_ZOOM_EX:
			retv = iav_get_digital_zoom_ex(context,
					(struct iav_digital_zoom_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX:
			retv = iav_set_2nd_digital_zoom_ex(context,
					(struct iav_digital_zoom_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX:
			retv = iav_get_2nd_digital_zoom_ex(context,
					(struct iav_digital_zoom_ex_s __user *)arg);
			break;

		case IAV_IOC_READ_YUV_BUFFER_INFO_EX:
			retv = iav_read_yuv_buffer_info_ex(context,
					(struct iav_yuv_buffer_info_ex_s __user *)arg);
			break;
		case IAV_IOC_READ_ME1_BUFFER_INFO_EX:
			retv = iav_read_me1_buffer_info(context,
					(struct iav_me1_buffer_info_ex_s __user *)arg);
			break;
		case IAV_IOC_READ_BUF_CAP_EX:
			retv = iav_bufcap_read(context,
					(struct iav_buf_cap_s __user *) arg);
			break;

		case IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX:
			retv = iav_set_digital_zoom_privacy_mask_ex(context,
					(struct iav_digital_zoom_privacy_mask_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_PREVIEW_BUFFER_FORMAT_ALL_EX:
			retv = iav_set_preview_buffer_format_all_ex(context,
					(struct iav_preview_buffer_format_all_ex_s __user *)arg);
			break;

		case IAV_IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX:
			retv = iav_get_preview_buffer_format_all_ex(context,
					(struct iav_preview_buffer_format_all_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_PREV_A_FRAMERATE_DIV_EX:
			retv = iav_set_preview_A_framerate_divisor_ex(context, (u8 __user)arg);
			break;

		case IAV_IOC_SET_WARP_CONTROL_EX:
			retv = iav_set_warp_control_ex(context,
					(struct iav_warp_control_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_WARP_CONTROL_EX:
			retv = iav_get_warp_control_ex(context,
					(struct iav_warp_control_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_QP_ROI_MATRIX_EX:
			retv = iav_set_qp_roi_matrix_ex(context,
					(iav_qp_roi_matrix_ex_t __user *)arg);
			break;

		case IAV_IOC_GET_QP_ROI_MATRIX_EX:
			retv = iav_get_qp_roi_matrix_ex(context,
					(iav_qp_roi_matrix_ex_t __user *)arg);
			break;

		case IAV_IOC_SET_STATIS_CONFIG_EX:
			retv = iav_set_statistics_config_ex(context,
					(struct iav_enc_statis_config_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_STATIS_CONFIG_EX:
			retv = iav_get_statistics_config_ex(context,
					(struct iav_enc_statis_config_ex_s __user *)arg);
			break;

		case IAV_IOC_FETCH_STATIS_INFO_EX:
			retv = iav_fetch_statistics_info_ex(context,
					(struct iav_enc_statis_info_ex_s __user *)arg);
			break;
		case IAV_IOC_RELEASE_STATIS_INFO_EX:
			retv = iav_release_statistics_info_ex(context,
					(struct iav_enc_statis_info_ex_s __user *)arg);
			break;

		case IAV_IOC_SET_WARP_AREA_DPTZ_EX:
			retv = iav_set_warp_region_dptz_ex(context,
					(struct iav_warp_dptz_ex_s __user *)arg);
			break;
		case IAV_IOC_GET_WARP_AREA_DPTZ_EX:
			retv = iav_get_warp_region_dptz_ex(context,
					(struct iav_warp_dptz_ex_s __user *)arg);
			break;

		case IAV_IOC_UPDATE_VIN_FRAMERATE_EX:
			retv = iav_update_vin_framerate(context);
			break;

		case IAV_IOC_QUERY_ENCMODE_CAP_EX:
			retv = iav_query_encmode_cap_ex(context,
				(struct iav_encmode_cap_ex_s __user *)arg);
			break;
		case IAV_IOC_QUERY_ENCBUF_CAP_EX:
			retv = iav_query_encbuf_cap_ex(context,
				(struct iav_encbuf_cap_ex_s __user *)arg);
			break;

		case IAV_IOC_START_VIN_FPS_STAT:
			retv = iav_start_vin_fps_stat(context, (u8)arg);
			break;
		case IAV_IOC_GET_VIN_FPS_STAT:
			retv = iav_get_vin_fps_stat(context,
				(struct amba_fps_report_s __user *)arg);
			break;

		case IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX:
			retv = iav_enc_dram_request_frame(context,
				(struct iav_enc_dram_buf_frame_ex_s __user *)arg);
			break;
		case IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX:
			retv = iav_enc_dram_buf_update_frame(context,
				(struct iav_enc_dram_buf_update_ex_s __user *)arg);
			break;
		case IAV_IOC_ENC_DRAM_RELEASE_FRAME_EX:
			retv = iav_enc_dram_release_frame(context,
				(struct iav_enc_dram_buf_frame_ex_s __user *)arg);
			break;

		case IAV_IOC_GET_VIDEO_PROC:
			retv = iav_get_video_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_CFG_VIDEO_PROC:
			retv = iav_cfg_video_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_APPLY_VIDEO_PROC:
			retv = iav_apply_video_proc(context, (void __user *)arg);
			break;

		case IAV_IOC_SET_STREAM_CFG_EX:
			retv = iav_set_stream_cfg_ex(context,
				(struct iav_stream_cfg_s __user *)arg);
			break;
		case IAV_IOC_GET_STREAM_CFG_EX:
			retv = iav_get_stream_cfg_ex(context,
				(struct iav_stream_cfg_s __user *)arg);
			break;

		case IAV_IOC_GET_WARP_PROC:
			retv = iav_get_warp_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_CFG_WARP_PROC:
			retv = iav_cfg_warp_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_APPLY_WARP_PROC:
			retv = iav_apply_warp_proc(context, (void __user *)arg);
			break;

		case IAV_IOC_GET_VCAP_PROC:
			retv = iav_get_vcap_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_CFG_VCAP_PROC:
			retv = iav_cfg_vcap_proc(context, (void __user *)arg);
			break;
		case IAV_IOC_APPLY_VCAP_PROC:
			retv = iav_apply_vcap_proc(context, (void __user *)arg);
			break;

		case IAV_IOC_CFG_FRAME_SYNC_PROC:
			retv = iav_cfg_frame_sync_proc(context,
				(struct iav_stream_cfg_s __user *)arg);
			break;
		case IAV_IOC_APPLY_FRAME_SYNC_PROC:
			retv = iav_apply_frame_sync_proc(context, (u32)arg);
			break;
		case IAV_IOC_GET_FRAME_SYNC_PROC:
			retv = iav_get_frame_sync_proc(context,
				(struct iav_stream_cfg_s __user *)arg);
			break;
		default:
			iav_error("unknown cmd 0x%x\n", cmd);
			retv = -ENOIOCTLCMD;
			break;
	}

	return retv;
}

void iav_encode_open(iav_context_t * context)
{
	// Todo
	return ;
}

void iav_encode_release(iav_context_t * context)
{
	// Todo
	return ;
}


