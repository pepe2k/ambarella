/*
 * bsreader_s2.c
 *
 * History:
 *	2012/05/23 - [Jian Tang] created file
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include "basetypes.h"
#include "iav_drv.h"
#include "bsreader.h"
#include "fifo.h"

//#define BSRREADER_DEBUG

#ifdef BSRREADER_DEBUG
        #define DEBUG_PRINT(x...)   printf("BSREADER: " x)
#else
        #define DEBUG_PRINT(x...)    do{} while(0)
#endif

typedef enum {
	POLLING_MODE = 0,
	INTERRUPT_MODE = 1,
} BSREADER_PROTOCOL;

static version_t G_bsreader_version = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.mod_time = 0x20130924,
	.description = "Ambarella S2 Bsreader Library",
};

static bsreader_init_data_t G_bsreader_init_data;

static int G_read_protocol = POLLING_MODE;
static int G_ready = 0;
static pthread_mutex_t G_all_mutex;

static u8 *G_dsp_bsb_addr;
static int G_dsp_bsb_size;

static pthread_t G_main_thread_id = 0;

static int G_iav_fd = -1;

static int G_force_main_thread_quit = 0;

//local copy of current stream session for speical handling
static int G_stream_session_id[MAX_BS_READER_SOURCE_STREAM];

static fifo_t* myFIFO[MAX_BS_READER_SOURCE_STREAM];

/*****************************************************************************
 *
 *				private functions
 *
 *****************************************************************************/

static void inline lock_all(void)
{
	pthread_mutex_lock(&G_all_mutex);
}

static void inline unlock_all(void)
{
	pthread_mutex_unlock(&G_all_mutex);
}


//return 1 if session id changed,  return 0 if session id unchanged
static inline int is_stream_session_id_changed(bits_info_ex_t * bs_info)
{
	int stream = bs_info->stream_id;
	int session_id = bs_info->session_id;
	return (G_stream_session_id[stream] != session_id);
}

static inline void update_stream_session_id(bits_info_ex_t * bs_info)
{
	int stream = bs_info->stream_id;
	int session_id = bs_info->session_id;
	G_stream_session_id[stream] = session_id;
}

/* Blocking call to fetch one frame from BSB ,
 * return -1 to indicate unknown error
 * return 1 to indicate try again
 * this function just get frame information and does not
 * do memcpy
 */
static int fetch_one_frame(bsreader_frame_info_t * bs_info)
{
	bits_info_ex_t  bits_info;
	int i, stream_end_num, ret = 0;
	iav_encode_stream_info_ex_t info;
	static u32 dsp_h264_frame_num = 0;
	static u32 dsp_mjpeg_frame_num = 0;
	static u32 frame_num[MAX_BS_READER_SOURCE_STREAM];
	static u8 is_mjpeg_stream[MAX_BS_READER_SOURCE_STREAM];
	static u32 end_of_stream[MAX_BS_READER_SOURCE_STREAM];
	static int init_flag = 0;

	if (init_flag == 0) {
		for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
			frame_num[i] = 0;
			end_of_stream[i] = 1;
		}
		init_flag = 1;
	}

	for (i = 0, stream_end_num = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		info.id = (1 << i);
		if (ioctl(G_iav_fd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info) < 0) {
			perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
			ret = -1;
			goto error_catch;
		}
		if (info.state == IAV_STREAM_STATE_ENCODING) {
			end_of_stream[i] = 0;
		}
		stream_end_num += end_of_stream[i];
	}
	// There is no encoding stream, skip to next turn
	if (stream_end_num == MAX_BS_READER_SOURCE_STREAM) {
		dsp_h264_frame_num = 0;
		dsp_mjpeg_frame_num = 0;
		ret = -2;
		goto error_catch;
	}
	if (G_read_protocol == POLLING_MODE) {
		if (ioctl(G_iav_fd, IAV_IOC_READ_BITSTREAM_EX, &bits_info) < 0) {
			perror("IAV_IOC_READ_BITSTREAM_EX");
			ret = -1;
			goto error_catch;
		}
	} else {
		bits_info.stream_id = MAX_BS_READER_SOURCE_STREAM;
		if (ioctl(G_iav_fd, IAV_IOC_FETCH_FRAME_EX, &bits_info) < 0) {
			perror("IAV_IOC_FETCH_FRAME_EX");
			ret = -1;
			goto error_catch;
		}
	}

	memset(bs_info, 0, sizeof(*bs_info));	//clear zero
	bs_info->bs_info.stream_id= bits_info.stream_id;
	bs_info->bs_info.session_id = bits_info.session_id;
	bs_info->bs_info.frame_num = frame_num[bits_info.stream_id];
	bs_info->bs_info.pic_type = bits_info.pic_type;
	bs_info->bs_info.PTS = bits_info.PTS;
	bs_info->bs_info.mono_pts = bits_info.monotonic_pts;
	bs_info->bs_info.audio_pts = bits_info.stream_pts >> 32;
	bs_info->bs_info.stream_end = bits_info.stream_end;
	bs_info->bs_info.frame_index = bits_info.frame_index;
	bs_info->bs_info.jpeg_quality = bits_info.jpeg_quality;

	/* If it's a stream end null frame indicator, update the frame counter,
	 * but skip filling frame_addr and frame_size
	 */
	if (bits_info.stream_end) {
		end_of_stream[bits_info.stream_id] = 1;
		++stream_end_num;
		if (stream_end_num == MAX_BS_READER_SOURCE_STREAM) {
			dsp_h264_frame_num = 0;
			dsp_mjpeg_frame_num = 0;
		} else {
			if (is_mjpeg_stream[bits_info.stream_id]) {
				++dsp_mjpeg_frame_num;
			} else {
				++dsp_h264_frame_num;
			}
		}
		goto error_catch;
	}
	bs_info->frame_addr = (u8*)bits_info.start_addr;
	bs_info->frame_size = bits_info.pic_size;

	// Check BSB overflow
	if (bits_info.pic_type == JPEG_STREAM) {
		if ((bits_info.frame_num - dsp_mjpeg_frame_num) != 1) {
			printf(" MJPEG [%d] - [%d]  BSB overflow!\n",
				bits_info.frame_num, dsp_mjpeg_frame_num);
		}
		dsp_mjpeg_frame_num = bits_info.frame_num;
		is_mjpeg_stream[bits_info.stream_id] = 1;
	} else {
		if ((bits_info.frame_num - dsp_h264_frame_num) != 1) {
			printf(" H.264 [%d] - [%d] BSB overflow!\n",
				bits_info.frame_num, dsp_h264_frame_num);
		}
		dsp_h264_frame_num = bits_info.frame_num;
		is_mjpeg_stream[bits_info.stream_id] = 0;
	}

	frame_num[bits_info.stream_id] ++;

	//dump frame info
	DEBUG_PRINT(">>>>FETCH frm num %d, size %d, addr 0x%x, stream id %d\n",
		bs_info->bs_info.frame_num, bs_info->frame_size,
		(u32)bs_info->frame_addr, bs_info->bs_info.stream_id);

error_catch:
	return ret;
}


/* dispatch frame , with ring buffer full handling  */
static int dispatch_and_copy_one_frame(bsreader_frame_info_t * bs_info)
{
	int ret;
	bits_info_ex_t bits_info;
	DEBUG_PRINT("dispatch_and_copy_one_frame \n");
	if (bs_info->bs_info.stream_end == 1) {
		bs_info->frame_size = 0;
	}
	ret = fifo_write_one_packet(myFIFO[bs_info->bs_info.stream_id],
		(u8 *)&bs_info->bs_info, bs_info->frame_addr, bs_info->frame_size);

	if (G_read_protocol == INTERRUPT_MODE) {
		bits_info.stream_id = bs_info->bs_info.stream_id;
		bits_info.frame_index = bs_info->bs_info.frame_index;
		if (ioctl(G_iav_fd, IAV_IOC_RELEASE_FRAME_EX, &bits_info) < 0) {
			perror("IAV_IOC_RELEASE_FRAME_EX");
			ret = -1;
		}
	}

	return ret;
}


static int set_realtime_schedule(pthread_t thread_id)
{
	struct sched_param param;
	int policy = SCHED_RR;
	int priority = 90;
	if (!thread_id)
		return -1;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;
	if (pthread_setschedparam(thread_id, policy, &param) < 0)
		perror("pthread_setschedparam");
	pthread_getschedparam(thread_id, &policy, &param);
	if (param.sched_priority != priority)
		return -1;
	return 0;
}


/* This thread main func will be a loop to fetch frames from BSB and fill
 * into max to four stream ring buffers
 */
static void * bsreader_dispatcher_thread_func(void * arg)
{
	bsreader_frame_info_t frame_info;
	int retv = 0;
	DEBUG_PRINT("->enter bsreader dispatcher main loop\n");

	while (!G_force_main_thread_quit) {
		if ((retv = fetch_one_frame(&frame_info)) < 0) {
			if ((retv == -1) && (errno != EAGAIN)) {
				perror("fetch one frame failed");
			}
			usleep(10*1000);
			continue;
		}

		//dispatch frame (including ring buffer full handling)
		if (dispatch_and_copy_one_frame(&frame_info) < 0) {
			printf("dispatch frame failed \n");
			continue;
		}
	}
	DEBUG_PRINT("->quit bsreader dispatcher main loop\n");

	return 0;
}


/* create a main thread reading data and four threads for four buffer */
static int create_working_thread(void)
{
	pthread_t tid;
	int ret;
	G_force_main_thread_quit = 0;

	ret = pthread_create(&tid, NULL, bsreader_dispatcher_thread_func, NULL);
	if (ret != 0) {
		perror("main thread create failed");
		return -1;
	}
	G_main_thread_id = tid;
	if (set_realtime_schedule(G_main_thread_id) < 0) {
		printf("set realtime schedule error \n");
		return -1;
	}
	return 0;
}

static int cancel_working_thread (void)
{
	int ret;
	if (!G_main_thread_id)
		return 0;

	G_force_main_thread_quit = 1;
	ret = pthread_join(G_main_thread_id, NULL);
	if (ret < 0) {
		perror("pthread_cancel bsreader main");
		return -1;
	}
	G_main_thread_id = 0;
	return 0;
}

static int init_iav(void)
{
	iav_mmap_t mmap;
	iav_state_info_t info;
	iav_system_setup_info_ex_t system_setup;

	if (G_iav_fd != -1) {
		printf("iav already initialized \n");
		return -1;
	}

	if (G_bsreader_init_data.fd_iav < 0) {
		printf("iav handler invalid %d for bsreader\n", G_bsreader_init_data.fd_iav);
		return -1;
	}
	G_iav_fd = G_bsreader_init_data.fd_iav;

	if (ioctl(G_iav_fd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	//check iav state, must start bsreader before start encoding to ensure no frame missing
	if ((info.state != IAV_STATE_INIT) &&
		(info.state != IAV_STATE_IDLE) &&
		(info.state != IAV_STATE_PREVIEW)) {
		printf("iav state must be in either idle or preview before open bsreader\n");
		return -1;
	}

	//check iav mmap
	if (ioctl(G_iav_fd, IAV_IOC_GET_MMAP_INFO, &mmap) < 0) {
		return -1;
	}
	G_dsp_bsb_addr = mmap.bsb_addr;
	G_dsp_bsb_size = mmap.bsb_size;
	if ((!G_dsp_bsb_addr) || (!G_dsp_bsb_size)) {
		printf("please call IAV_IOC_MAP_BSB2 before open bsreader\n");
		return -1;
	}

	if (ioctl(G_iav_fd, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &system_setup) < 0) {
		perror("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
		return -1;
	}
	G_read_protocol = system_setup.coded_bits_interrupt_enable;

	return 0;
}


/*****************************************************************************
 *
 *				export functions
 *
 *****************************************************************************/
/* set init parameters */
int bsreader_init(bsreader_init_data_t * bsinit)
{
	/* data check*/
	if (bsinit == NULL)
		return -1;

	if ((bsinit->max_stream_num > MAX_BS_READER_SOURCE_STREAM) ||
		(bsinit->max_stream_num  == 0))
		return -1;

	if (G_ready) {
		printf("bsreader not closed, cannot reinit\n");
		return -1;
	}

	G_bsreader_init_data = *bsinit;
	if (G_bsreader_init_data.max_buffered_frame_num == 0)
		G_bsreader_init_data.max_buffered_frame_num = 8;
	//init mux
	pthread_mutex_init(&G_all_mutex, NULL);
	G_ready = 0;
	return 0;
}


/* allocate memory and do data initialization, threads initialization,
 * also call reset write/read pointers
 */
int bsreader_open(void)
{
	int ret = 0, i = 0;
	u32 max_entry_num;

	lock_all();

	if (G_ready) {
		printf("already opened\n");
		return -1;
	}

	/* Init iav and test iav state, it's expected iav should not be
	 * in encoding, before open bsreader
	 */
	if (init_iav() < 0) {
		ret = -3;
		goto error_catch;
	}
	max_entry_num = G_bsreader_init_data.max_buffered_frame_num;

	for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		myFIFO[i] = fifo_create(G_bsreader_init_data.ring_buf_size[i],
			sizeof(bs_info_t), max_entry_num);
	}

	// Create thread after all other initialization is done
	if (create_working_thread() < 0) {
		printf("create working thread error \n");
		ret = -1;
		goto error_catch;
	}
	G_ready = 1;

	unlock_all();
	return 0;

error_catch:
	cancel_working_thread();
	unlock_all();
	DEBUG_PRINT("bsreader opened.\n");
	return ret;
}


/* free all resources, close all threads */
int bsreader_close(void)
{
	int ret = 0, i = 0;

	lock_all();
	//TODO, possible release
	if (!G_ready) {
		printf("has not opened, cannot close\n");
		return -1;
	}

	if (cancel_working_thread() < 0) {
		perror("pthread_cancel bsreader main");
		return -1;
	} else {
		DEBUG_PRINT("main working thread canceled \n");
	}

	for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		fifo_close(myFIFO[i]);
	}
	G_ready = 0;
	unlock_all();

	pthread_mutex_destroy(&G_all_mutex);

	return ret;
}


int bsreader_reset(int stream)
{
	if (!G_ready) {
		return -1;
	}
//	printf("bsreader_reset\n");
	fifo_flush(myFIFO[stream]);

	return 0;
}


int bsreader_get_one_frame(int stream, bsreader_frame_info_t * info)
{
	if (!G_ready) {
		return -1;
	}

	return fifo_read_one_packet(myFIFO[stream], (u8 *)&info->bs_info,
		&info->frame_addr, &info->frame_size);
}

int bsreader_get_version(version_t* version)
{
	memcpy(version, &G_bsreader_version, sizeof(version_t));
	return 0;
}


