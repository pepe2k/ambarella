/*
 * rtmp_video_audio.cpp
 *
 * History:
 *	2015/9/27 - [Shawn Xiao] create to use "H264 and AAC" to get encoded streams and use "librtmp" to send stream
 *              to cloud server.            
 *200
 
 * Copyright (C) 2015-2016, Misfit, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Misfit, Inc.
 *
 */

/******************************************************************************************************
*
*define part
*
*****************************************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <sys/signal.h>

//static pthread_mutex_t video_mutex;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rtmp_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  queue_cond = PTHREAD_COND_INITIALIZER;

typedef struct queue
{
	u32 data_type;
	u8 *data_addr;
	u32 data_size;
	struct queue *pre;
	struct queue *next;
}iav_queue;

enum data_type{
	AUDIO_HEADER,
	AUDIO_AAC,
	VIDEO_KEYFRAME = 0x05,
	VIDEO_FRAME,
	VIDEO_SPS,
	VIDEO_PPS,
};

#define QUEUE_LEN_MAX 8
static unsigned int queue_len = 0;
static iav_queue *q_head = NULL;
static iav_queue *q_tail = NULL;

//video
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "bsreader.h"
#include "datatx_lib.h"

#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define MAX_BUFFERED_FRAME_NUM 64

typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	int total_frames; // count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;  //count how many bytes encoded
	int pic_type;	  //picture type,  non changed during encoding, help to validate encoding state.
	u32 pts;
	u64 monotonic_pts;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start

	u32 dsp_audio_pts;	// dsp pts from audio I2S
#ifdef ACCURATE_FPS_CALC
	int total_frames2;	//for statistics only
	struct timeval time2;	//for statistics only
#else
	u32 reserved;
#endif

} stream_encoding_state_t;

//audio
#include "audio_encode.h" /* for PCM code */
#include "aac_audio_enc.h" /* for AAC code */
#include "formats.h"

#define DEFAULT_SAMPLE_FORMAT	SND_PCM_FORMAT_S16_LE // SND_PCM_FORMAT_A_LAW //SND_PCM_FORMAT_MU_LAW
#define DEFAULT_SAMPLE_RATE		48000
#define DEFAULT_SAMPLE_CHANNELS	2
#define DEFAULT_BITRATE     	128000

#define LLONG_MAX	9223372036854775807LL

static int audio_state = 1;
static snd_pcm_t *handle = NULL;
static size_t bits_per_sample = 0;
static size_t bits_per_frame = 0;
static snd_pcm_uframes_t chunk_size = 0;
static size_t chunk_bytes;
//rtmp
#include "rtmp.h"
#include "rtmp_sys.h"
#include "amf.h"
#include "sps_decode.h"
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

static int rtmp_state  = 1;
static RTMP* m_pRtmp = NULL;  
static int video_timeoffset = 0;
static int audio_timeoffset = 0;
int fd_iav = -1;

typedef struct _RTMPMetadata
{
	unsigned int    nWidth;
	unsigned int    nHeight;
	unsigned int    nFrameRate;
	unsigned int    nSpsLen;
	unsigned char   *Sps;
	unsigned int    nPpsLen;
	unsigned char   *Pps;
} RTMPMetadata,*LPRTMPMetadata;

RTMPMetadata meta_data;

/****************************************************************************************************
*
* program part
*
*****************************************************************************************************/
int map_bsb(void)
{
	iav_mmap_info_t info;
	//maps driver's bit stream buffer to the user space to read and solve the wraparound frame issue
	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}	
	printf("before addr = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);

	//maps driver's DSP memory to the user space for reading
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}
	printf("after addr = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);

	return 0;
}

int send_video_h264_to_queue(bsreader_frame_info_t *frame_info)
{
	unsigned int nalhead_pos = 0;
	unsigned int naltail_pos = 0;
	int naltype = 0;
	int nalsize = 0;
	iav_queue *video_node = NULL;
		
	//printf("search for nal head\n");
	while(nalhead_pos <= frame_info->frame_size)
	{
		if( frame_info->frame_addr[nalhead_pos++] == 0x00 && 
			frame_info->frame_addr[nalhead_pos++] == 0x00 &&
			frame_info->frame_addr[nalhead_pos++] == 0x00 &&
			frame_info->frame_addr[nalhead_pos++] == 0x01 ) 
		{
			//printf("nalhead_pos = %d\n", nalhead_pos);
			break;
		}
		else 
		{
			continue;
		}
			//printf("nalhead_pos = %d\n", nalhead_pos);
	}

	//printf("search for nal tail\n");
	naltail_pos = nalhead_pos;
	while(naltail_pos < frame_info->frame_size)
	{	
		if( (frame_info->frame_addr[naltail_pos++] == 0x00) && 
			(frame_info->frame_addr[naltail_pos++] == 0x00) &&
			(frame_info->frame_addr[naltail_pos++] == 0x00) &&
			(frame_info->frame_addr[naltail_pos++] == 0x01) &&
			(naltail_pos <= frame_info->frame_size) ) //full freame
		{
			//printf("full nalhead_pos =%d naltail_pos = %d\n", nalhead_pos, naltail_pos);	
			nalsize = naltail_pos - nalhead_pos - 4;	
			goto NAL;
		}
			
		if(naltail_pos >= frame_info->frame_size)//end frame
		{
			//printf("part nalhead_pos =%d naltail_pos = %d\n", nalhead_pos, naltail_pos);
			nalsize = frame_info->frame_size - nalhead_pos;
			goto NAL;
		}
		else
		{
			continue;
		}
	NAL:
		naltype = frame_info->frame_addr[nalhead_pos] & 0x1f;
		if(naltype == 0x01 || naltype == 0x05 || naltype == 0x07 || naltype == 0x08)
		{
			if(naltype == 0x01)
			{
				naltype += 0x05;
			}
		}
		else
		{
			nalhead_pos = naltail_pos;
			continue;
		}
	
		do
		{
			video_node = (iav_queue *)malloc(sizeof(iav_queue));
			if(video_node == NULL)
			{
				printf("malloc video_node err\n");
				continue ;
			}
			memset(video_node, 0, sizeof(iav_queue));
	
			//fill node
			video_node->data_type = naltype;
			video_node->data_addr = (unsigned char *)malloc(nalsize);
			if(video_node->data_addr == NULL)
			{
				printf("malloc video err\n");
				free(video_node);
				continue ;
			}
			memset(video_node->data_addr, 0, nalsize);
			memcpy(video_node->data_addr, frame_info->frame_addr + nalhead_pos, nalsize);
			video_node->data_size = nalsize;
	
			//add to queue
			pthread_mutex_lock(&write_mutex);
			video_node->pre = q_head;
			video_node->next = q_head->next;
			q_head->next = video_node;
			video_node->next->pre = video_node;
			pthread_mutex_unlock(&write_mutex);
	
			//queue++
			pthread_mutex_lock(&queue_mutex);
			queue_len++;
			if(queue_len > 0)
			{
				pthread_cond_signal(&queue_cond);
			}
			pthread_mutex_unlock(&queue_mutex);	
			
			//for searching the next naltail_pos
			nalhead_pos = naltail_pos;
		
			break;
		}while(1);
	}
    return 0;
}

void* reading_video_thread_func(void *)
{
	int i;
	int total_size = 0;
	int ret = 0;
	int stream = 0;
	bsreader_frame_info_t frame_info;
	bsreader_init_data_t  init_data;
	
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("can not open /dev/iav");
		return NULL;
	}

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return NULL;
	}
	
	memset(&init_data, 0, sizeof(init_data));
	init_data.fd_iav = fd_iav;
	init_data.max_stream_num = MAX_ENCODE_STREAM_NUM;
	init_data.max_buffered_frame_num = MAX_BUFFERED_FRAME_NUM;
	
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		switch (i) {
		case 0:
			total_size = 8 << 20;
			break;
		case 1:
			total_size = 2 << 20;
			break;
		default:
			total_size = 1 << 20;
			break;
		}
		init_data.ring_buf_size[i] = total_size;
	}
	/* Enable cavlc encoding when configured.
	 * Enable this option will use more memory in bsreader.
	 */
	init_data.cavlc_possible = 1;

	if (bsreader_init(&init_data) < 0) {
		printf("bsreader init failed \n");
		return NULL;
	}

	if (bsreader_open() < 0) {
		printf("bsreader open failed \n");
		return NULL;
	}
	printf("video capture and send\n");
	memset(&frame_info, 0, sizeof(bsreader_frame_info_t));
	while(1)
	{
		ret = bsreader_get_one_frame(stream, &frame_info);
		if (ret < 0) {
			if (ret != -2) {
				printf("bs reader gets frame error\n");
				return NULL;
			}
			usleep(1*1000);
			continue;
		}

		printf("*****READ  [%d] frame_num %d, frame_size %d \n",
				stream, frame_info.bs_info.frame_num, frame_info.frame_size);
		
		pthread_mutex_lock(&queue_mutex);
		while(queue_len > QUEUE_LEN_MAX)
		{
			pthread_cond_wait(&queue_cond, &queue_mutex);
		}
		pthread_mutex_unlock(&queue_mutex);

		ret = send_video_h264_to_queue(&frame_info);						
	}
	
	return NULL;
}

static snd_pcm_uframes_t snd_pcm_set_params(snd_pcm_t *handle, snd_pcm_stream_t stream)
{
	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_uframes_t buffer_size = 0;	
	unsigned int period_time = 0;
	unsigned int buffer_time = 0;
	int err;
	
	snd_pcm_hw_params_alloca(&params);

	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("Broken configuration for this PCM: no configurations available");
		return err;
	}

	err = snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("Access type not available");
		return err;
	}

	err = snd_pcm_hw_params_set_format(handle, params, DEFAULT_SAMPLE_FORMAT);
	if (err < 0) {
		printf("Sample format non available");
		return err;
	}

	err = snd_pcm_hw_params_set_channels(handle, params, DEFAULT_SAMPLE_CHANNELS);
	if (err < 0) {
		printf("Channels count non available");
		return err;
	}

	err = snd_pcm_hw_params_set_rate(handle, params, DEFAULT_SAMPLE_RATE, 0);
	if (err < 0) {
		printf("Rate non available");
		return err;
	}

	err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
	assert(err >= 0);
	
	if (buffer_time > 500000)
		buffer_time = 500000;

	period_time = buffer_time / 4;

	err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, 0);
	assert(err >= 0);

	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, 0);
	assert(err >= 0);

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to install hw params:");
		exit(EXIT_FAILURE);
	}

	printf("before chunk_size = %ld\n", chunk_size);
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	printf("after chunk_size = %ld\n", chunk_size);

	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		printf("Can't use period equal to buffer size (%lu == %lu)",
			chunk_size, buffer_size);
		exit(EXIT_FAILURE);
	}

	bits_per_sample = snd_pcm_format_physical_width(DEFAULT_SAMPLE_FORMAT);
	bits_per_frame = bits_per_sample * DEFAULT_SAMPLE_CHANNELS;
	chunk_bytes = chunk_size * bits_per_frame / 8;
	//printf("bits_per_sample = %d, bits_per_frame = %d\n", bits_per_sample, bits_per_frame);
	//printf("chunk_size = %ld, chunk_bytes = %ld\n", chunk_size, chunk_bytes);

	return 0;
}

int send_audio_header_to_queue(void)
{
	int index;
	iav_queue *tmp_node;
	unsigned int aac_header_len = 2;
	unsigned char *aac_header_buf = NULL;
	static u32 audio_sample_rates[16] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350};
	
	for(index = 0; index < 16; index++)
	{
		if(audio_sample_rates[index] == DEFAULT_SAMPLE_RATE)
		{
			break;
		}
	
		if(index == 16)
		{
			printf("error sample rate!\n");
			return 1;	
        }
	}
	
	aac_header_buf = (unsigned char *)malloc(aac_header_len);
	if(aac_header_buf == NULL)
	{
		printf("malloc aac_header_buf err\n");
		return 1;
	}

	tmp_node = (iav_queue *)malloc(sizeof(iav_queue));
	if(tmp_node == NULL)
	{
		printf("malloc tmp_node err\n");
		return 1;
	}

	//need to check the correct of calculation.
	aac_header_buf[0] = 0x02 << 3 | index >> 1;
	aac_header_buf[1] = (index & 0x01) << 7 | DEFAULT_SAMPLE_CHANNELS << 3;
	
	//fill node
	tmp_node->data_type = 0x00;
	tmp_node->data_addr = aac_header_buf;
	tmp_node->data_size = aac_header_len;
	
	//add to queue
	pthread_mutex_lock(&write_mutex);
	tmp_node->pre = q_head;
	tmp_node->next = q_head->next;
	q_head->next = tmp_node;
	tmp_node->next->pre = tmp_node;	
	pthread_mutex_unlock(&write_mutex);	

	q_tail = tmp_node;
	
	pthread_mutex_lock(&queue_mutex);
	queue_len++;
	printf("queue_len header = %d\n",queue_len);
	if(queue_len > 0)
	{
		pthread_cond_signal(&queue_cond);
	}
	pthread_mutex_unlock(&queue_mutex);	
	
	return 0;
}

/* I/O error handler */
static void xrun(snd_pcm_stream_t stream)
{
	snd_pcm_status_t *status;
	int res;

	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		printf("status error: %s\n", snd_strerror(res));
		return ;
	}

	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		struct timeval now, diff, tstamp;
		gettimeofday(&now, 0);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
		fprintf(stderr, "%s!!! (at least %.3f ms long)\n",
			stream == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",
			diff.tv_sec * 1000 + diff.tv_usec / 1000.0);

		if ((res = snd_pcm_prepare(handle))<0) {
			printf("xrun: prepare error: %s\n", snd_strerror(res));
		}
		return;		/* ok, data should be accepted again */
	} 
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
		printf("draining!!!\n");
		if (stream == SND_PCM_STREAM_CAPTURE) {
			fprintf(stderr, "capture stream format change? attempting recover...\n");
			if ((res = snd_pcm_prepare(handle))<0) {
				printf("xrun(DRAINING): prepare error: %s", snd_strerror(res));
			}
		}
	}

	return ;
}

static size_t pcm_read(snd_pcm_uframes_t chunk_size, u_char *data)
{
	ssize_t r;
	size_t result = 0;
	size_t count = chunk_size;

	while (count > 0) 
	{
		r = snd_pcm_readi(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 1000);
		} else if (r == -EPIPE) {
			xrun(SND_PCM_STREAM_CAPTURE);
		} else if (r < 0) {
			printf("read pcm error: %s", snd_strerror(r));
			
			pthread_mutex_lock(&audio_mutex);
			audio_state = 0;
			pthread_mutex_unlock(&audio_mutex);
			break;
		}

		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}

	return result;
}

int send_audio_data_to_queue(unsigned char * aac_data_buf,int aac_data_len)
{
	iav_queue *tmp_node = NULL;
	
	tmp_node = (iav_queue *)malloc(sizeof(iav_queue));
	if(tmp_node == NULL)
	{
		printf("malloc tmp_node err\n");
		return 1;
	}
	memset(tmp_node, 0, sizeof(iav_queue));
	
	//fill node
	tmp_node->data_type = 0x01;
	tmp_node->data_addr = (unsigned char *)malloc(aac_data_len);
	if(aac_data_buf == NULL)
	{
		printf("malloc aac_header_buf err\n");
		return 1;
	}
	memset(tmp_node->data_addr, 0, aac_data_len);
	memcpy(tmp_node->data_addr, aac_data_buf, aac_data_len);
	tmp_node->data_size = aac_data_len;
	//add to queue
	pthread_mutex_lock(&write_mutex);
	tmp_node->pre = q_head;
	tmp_node->next = q_head->next;
	q_head->next = tmp_node;
	tmp_node->next->pre = tmp_node;
	pthread_mutex_unlock(&write_mutex);
    //queue++
	pthread_mutex_lock(&queue_mutex);
	queue_len++;
	printf("queue_len data = %d\n",queue_len);
	if(queue_len == 8)
	{
		pthread_cond_signal(&queue_cond);
	}
	pthread_mutex_unlock(&queue_mutex);	

	return 0;
}

static void capture_and_send_data(void)
{
	unsigned long long rest;
	size_t i = 0;
	size_t c = 0;
	int ret;
	uint8_t *tmp = NULL;
	int len = 4096;
	s32 pcm_buf[2*1024];

	int aac_data_len = 0;	
	u_char buf_work[106000];
	u_char buf_out[(6144/8)*2+100];
	au_aacenc_config_t au_aacenc_config;
	
	u_char *buf_in;
	buf_in = (u_char *)malloc(chunk_bytes);
	if (buf_in == NULL) {
		printf("not enough memory");
		exit(EXIT_FAILURE);
	}

	/* aacenc set params */
	au_aacenc_config.sample_freq = DEFAULT_SAMPLE_RATE;
	au_aacenc_config.Src_numCh = DEFAULT_SAMPLE_CHANNELS;
	au_aacenc_config.Out_numCh = DEFAULT_SAMPLE_CHANNELS;
	au_aacenc_config.bitRate = DEFAULT_BITRATE;
	au_aacenc_config.quantizerQuality = 2;
	au_aacenc_config.tns = 1;
	au_aacenc_config.ffType = 't';
	au_aacenc_config.enc_mode = AACPLAIN;
	au_aacenc_config.codec_lib_mem_adr = (u32*)buf_work; 
	au_aacenc_config.calibre_proc_on = 0; 
	au_aacenc_config.calibre_apply = 0; 
	
	/* aacenc setup */
	aacenc_setup(&au_aacenc_config); 
	
	/* aacenc open*/
	aacenc_open(&au_aacenc_config);
	
	printf("send aac header\n");
	ret = send_audio_header_to_queue();
	if(ret)
	{
		printf("can not send_audio_header_to_queue\n");
		return ;
	}

	rest = LLONG_MAX;
	
	/* capture */
	while (rest > 0) 
	{
		c = (rest <= chunk_bytes) ? (size_t)rest : chunk_bytes;
		/* read pcm data for ALSA */
		if (pcm_read(chunk_size, buf_in) != chunk_size)
			break;
		i = 0;
		tmp = buf_in;
		while(i < c)
		{
			i += 4096;
			if(i < c)
			{
				if(len == 4096)
				{
					memcpy(pcm_buf, tmp, len);
					tmp += len;
					//printf("len = %d\n", len);
				}
				else
				{
					memcpy((char *)pcm_buf+len, tmp, 4096-len);
					i = 4096-len;
					len = 4096;
					tmp += i;
					//printf("i+ = %d\n", i);
				}
			}
			else
			{
				len = c -(i-4096);
				memcpy(pcm_buf, tmp, len);
				printf("last len = %d\n", len);
				break;
			}

			au_aacenc_config.enc_rptr = pcm_buf; 
			au_aacenc_config.enc_wptr = buf_out;

			/* Encode PCM to AAC*/
			aacenc_encode(&au_aacenc_config);
		
			au_aacenc_config.frameCounter++;	
			aac_data_len = (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3;
			printf("aac_data_len = %d\n", aac_data_len);
	
			/* send AACdata to server */
			ret = 1;
			while(ret)
			{
				pthread_mutex_lock(&queue_mutex);
				while(queue_len > QUEUE_LEN_MAX)
				{
					pthread_cond_wait(&queue_cond, &queue_mutex);
				}
				pthread_mutex_unlock(&queue_mutex);

				ret	= send_audio_data_to_queue(buf_out, aac_data_len);
				if(ret)
				{
					printf("can not send_audio_data_to_queue\n");
				}
			}
		}		
		rest -= c;
	}
	snd_pcm_close(handle);
	free(buf_in);
}

void* reading_audio_thread_func(void *)
{
	int err = 0;
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

	err = snd_pcm_open(&handle, "default", stream, 0);
	if (err < 0) {
		printf("audio open error: %s", snd_strerror(err));
		return NULL;
	}

	snd_pcm_set_params(handle, stream);
	sleep(5);
	printf("audio capture and send\n");
	capture_and_send_data();

	return NULL;
}

int RTMP_iav_connect(const char* url)
{
	m_pRtmp = RTMP_Alloc(); 
	RTMP_Init(m_pRtmp);

	if(RTMP_SetupURL(m_pRtmp, (char*)url) == 0)
	{
		RTMP_Free(m_pRtmp);
		return false;
	}

	RTMP_EnableWrite(m_pRtmp);

	if(RTMP_Connect(m_pRtmp, NULL) == 0)
	{
		RTMP_Free(m_pRtmp);
		return false;
	}

	if(RTMP_ConnectStream(m_pRtmp,0) == 0)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		return false;
	}

	return true;
}

void init_queue_head(void)
{
	q_head = (iav_queue *)malloc(sizeof(iav_queue));
	
	q_head->data_type = 0;
	q_head->data_addr = NULL;
	q_head->data_size = 0;
	q_head->pre = q_head;
	q_head->next = q_head;
	
	q_tail = q_head;

	return ;
}

int rtmp_sendaac_header(iav_queue *tmp)
{
    int ret = 1;
	int i = 0;
	RTMPPacket * packet = NULL;
    unsigned char * body = NULL;

	//the length of AAC_audio_frame_flag is 2. 
    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + tmp->data_size + 2);
	if(packet == NULL)
	{
		printf("rtmp_sendaac_spec malloc error\n");
		return ret;
	}
    memset(packet, 0, RTMP_HEAD_SIZE + tmp->data_size + 2);
 
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
 
    /*AF 00 + AAC spec data*/
    body[i++] = 0xAF;
    body[i++] = 0x00;
    memcpy(&body[i], tmp->data_addr, tmp->data_size);
 
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = tmp->data_size + i;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;//audio header's timestamp is 0.
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;//large for audio header : type is important
    packet->m_nInfoField2 = m_pRtmp->m_stream_id;
 
    /*调用发送接口*/
    ret = RTMP_SendPacket(m_pRtmp, packet, TRUE);
	free(packet);
	
    return !ret;
}

int rtmp_sendaac_data(iav_queue *tmp)
{
	int ret = 1;
 	if (tmp->data_size > 0) 
	{	
		int i = 0;
        unsigned char *body;
		RTMPPacket * packet;
		
		//the length of AAC_audio_frame_flag is 2. 
        packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + tmp->data_size + 2);
	    if(packet == NULL)
		{
			printf("rtmp_sendaac_data malloc error\n");
			return ret;
		}
        memset(packet, 0, RTMP_HEAD_SIZE + tmp->data_size + 2);
 
        packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
        body = (unsigned char *)packet->m_body;
 
        /*AF 01 + AAC RAW data*/
        body[i++] = 0xAF;
       	body[i++] = 0x01;
    	tmp->data_addr += 7;//7 datas are not important for transfering of audio data.
    	tmp->data_size -= 7;
        memcpy(&body[i], tmp->data_addr, tmp->data_size);
			
		//timestamp
		audio_timeoffset++;//need to calculate

	    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    	packet->m_nBodySize = tmp->data_size + i;
      	packet->m_nChannel = 0x04;
        packet->m_nTimeStamp = audio_timeoffset;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM; //medium for audio data : type is important
        packet->m_nInfoField2 = m_pRtmp->m_stream_id;
 
        /*send packet*/
        ret = RTMP_SendPacket(m_pRtmp,packet,TRUE);
        free(packet);
	}
	
	return !ret;
}

int send_video_sps_sps(unsigned char *pps, int pps_len, unsigned char *sps, int sps_len)
{
	int i = 0;
	int ret = 1;
	RTMPPacket * packet = NULL;
	unsigned char * body = NULL;
	
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 1024);
	if(packet == NULL)
	{
		printf("send_video_sps_sps malloc error\n");
		return ret;
	}
	memset(packet, 0, RTMP_HEAD_SIZE + 1024);
	
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;

	body[i++] = 0x17;//I frame
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], sps, sps_len);
	i += sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i], pps, pps_len);
	i += pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;

	ret = RTMP_SendPacket(m_pRtmp, packet, TRUE);
	free(packet);    
	
	return ret;
}

int rtmp_sendh264_header(iav_queue* tmp)
{
	int ret = 1;
	if(tmp->data_type == VIDEO_SPS)
	{
		memset(&meta_data, 0, sizeof(RTMPMetadata));
		meta_data.nSpsLen = tmp->data_size;
		meta_data.Sps=NULL;
 		meta_data.Sps=(unsigned char*)malloc(tmp->data_size);
	    if(meta_data.Sps == NULL)
		{
			printf("meta_data.Sps malloc error\n");
			return ret;
		}
        memcpy(meta_data.Sps, tmp->data_addr, tmp->data_size);
	}
	else if(tmp->data_type == VIDEO_PPS)
	{
		meta_data.nPpsLen = tmp->data_size;
		meta_data.Pps=NULL;
		meta_data.Pps=(unsigned char*)malloc(tmp->data_size);
	    if(meta_data.Pps == NULL)
		{
			printf("meta_data.Pps malloc error\n");
			return ret;
		}
		memcpy(meta_data.Pps, tmp->data_addr, tmp->data_size);
		
		#if 1
		ret = send_video_sps_sps(meta_data.Sps, meta_data.nSpsLen, meta_data.Pps, meta_data.nPpsLen);
		#else
		int width = 0;
		int height = 0;
		int fps = 0;
		h264_decode_sps(meta_data.Sps, metaData.nSpsLen, width, height, fps);
		meta_data.nWidth = width;
		meta_data.nHeight = height;
		if(fps)
			meta_data.nFrameRate = fps;
		else
			meta_data.nFrameRate = 25;
		printf("metaData.nWidth=%d metaData.nHeight=%d metaData.nFrameRate=%d\n", 
		meta_data.nWidth, meta_data.nHeight, meta_data.nFrameRate);

		if(send_video_sps_sps(meta_data.Sps, meta_data.nSpsLen, meta_data.Pps, meta_data.nPpsLen) <= 0)
		{
			printf("send sps and pps error!\n");
			return 1;
		}
		#endif
		
		free(meta_data.Sps);
		free(meta_data.Pps);
	}
	else
	{
		printf("video header's type is wrong!\n");
		return ret;
	}

	return !ret;
}

int rtmp_sendh264_data(iav_queue* tmp)
{
	int ret = 1;

	if(tmp->data_size > 0)
	{
		int i = 0; 
		unsigned char *body;  
	  	RTMPPacket * packet;
		
		//the length of H264_video_frame_flag is 9. 
	    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + tmp->data_size + 9);
	    if(packet == NULL)
		{
			printf("rtmp_sendh264_data malloc error\n");
			return ret;
		}
		memset(packet, 0, RTMP_HEAD_SIZE + tmp->data_size + 9);	
		packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
        body = (unsigned char *)packet->m_body;

		if(tmp->data_type == VIDEO_KEYFRAME)
		{  
			body[i++] = 0x17;// 1:Iframe  7:AVC  
			//printf("SendKeyFrame\n");
		}
		else
		{  
			body[i++] = 0x27;// 2:Pframe  7:AVC   	
			//printf("SendFrame\n");
		}  

		// AVC NALU  
		body[i++] = 0x01; 
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		
		// NALU size   
		body[i++] = (tmp->data_size >> 24) & 0xff;  
		body[i++] = (tmp->data_size >> 16) & 0xff;  
		body[i++] = (tmp->data_size >>8) & 0xff;  
		body[i++] = (tmp->data_size) & 0xff;
	
		// NALU data   
		memcpy(&body[i], tmp->data_addr, tmp->data_size);  
		
		video_timeoffset += 40 ;//need to calculate, and I freame : what is the timeoffset ?
		//video_timeoffset += 1000/meta_data.nFrameRate

		packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    	packet->m_nBodySize = tmp->data_size + i;
      	packet->m_nChannel = 0x04;
        packet->m_nTimeStamp = video_timeoffset;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM; //medium for audio data : type is important
        packet->m_nInfoField2 = m_pRtmp->m_stream_id;
 
        /*send packet*/
        ret = RTMP_SendPacket(m_pRtmp, packet, TRUE);

		free(packet);
	}
	
	return !ret;
}

int reading_queue_and_send_to_server(iav_queue *q_tail)
{
	int ret = 1;
	iav_queue *tmp = NULL;
	printf("read and send\n");
	while(ret)
	{
		pthread_mutex_lock(&queue_mutex);
		while(queue_len <= 0)
		{
			printf("wait begin\n");
			pthread_cond_wait(&queue_cond, &queue_mutex);
			printf("wait end\n");
		}
			printf("no wait\n");
		
		pthread_mutex_unlock(&queue_mutex);
		//if(q_tail != NULL)
		{
			tmp = q_tail;
			//delete a node
			q_tail = q_tail->pre;
			q_tail->next = NULL;
			pthread_mutex_lock(&queue_mutex);
			printf("queue_len send = %d\n",queue_len);
			queue_len--;
			pthread_mutex_unlock(&queue_mutex);

			while(ret)
			{	
				switch(tmp->data_type)
				{
					case AUDIO_HEADER:
						ret = rtmp_sendaac_header(tmp);
						break;
					case AUDIO_AAC:
						ret = rtmp_sendaac_data(tmp);
						break;
					case VIDEO_SPS:
					case VIDEO_PPS:
						ret = rtmp_sendh264_header(tmp);
						break;
					case VIDEO_FRAME:
					case VIDEO_KEYFRAME:
						ret = rtmp_sendh264_data(tmp);
						break;
					default:
						printf("the type of rtmp_send_date if wrong!\n");
						break;
				}
				                    
				if(ret)
				{
					pthread_mutex_lock(&rtmp_mutex);
					rtmp_state = 0;
					pthread_mutex_unlock(&rtmp_mutex);
					printf("rtmp sending error!\n");
				}
			}
			printf("ret again =%d\n", ret);
			ret = 1;
			free(tmp->data_addr);//maybe it is wrong.
			free(tmp);
			printf("ret ag =%d\n", ret);
		}
	}
	return 0;
}

void RTMP_iav_close(void)
{
	if(m_pRtmp)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
	return ;
}

void* seading_data_thread_func(void *)
{
	int ret;
	
	init_queue_head();
	printf("rtmp connect begin\n");
	do 
	{
		ret = RTMP_iav_connect("rtmp://52.91.246.203/record/safe1");
		if(!ret)
		{
			pthread_mutex_lock(&rtmp_mutex);
			rtmp_state = 0;
			pthread_mutex_unlock(&rtmp_mutex);
			printf("rtmp connect error!\n");
		}
	}while(!ret);
	printf("rtmp connect end\n");
	
	reading_queue_and_send_to_server(q_tail);
	
	RTMP_iav_close();
	
	return NULL;
}

//return 1 is IAV is in encoding,  else, return 0
int is_video_encoding(void)
{
	iav_state_info_t info;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		exit(1);
	}
	return (info.state == IAV_STATE_ENCODING);
}

int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stderr, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else {
		fprintf(stderr, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "\r");
		dot_count = 0;
	}

	fflush(stderr);
	return 0;
}

/****************************************
*
* main function ：main pthread for checking 
* other pthread's running status.
*
* including 3 pthreads for rtmp_audio_video
*
****************************************/
int main(int argc, char **argv)
{
	int ret = 0;
	pthread_t video_pid;
	pthread_t audio_pid;
	pthread_t rtmp_pid;
	
	//pthread_mutex_init(&queue_mutex, NULL);
	//pthread_mutex_init(&write_mutex, NULL);  
	//pthread_cond_init(&cond, NULL); 

	//first create the queue for storing data and prepare to send data.
	ret = pthread_create(&rtmp_pid, NULL, seading_data_thread_func, NULL);
	if (ret != 0) {
		perror("video thread create failed");
		return -1;
	}
	
	//audio	before video to sure that playing is right.usually audio timestamp is smaller than video timestamp
	ret = pthread_create(&audio_pid, NULL, reading_audio_thread_func, NULL);
	if (ret != 0) {
		perror("video thread create failed");
		return -1;
	}

	//video
	/*
	ret = pthread_create(&video_pid, NULL, reading_video_thread_func, NULL);
	if (ret != 0) {
		perror("video thread create failed");
		return -1;
	}
	*/
	
	//main pthread
	{
		printf("main pthread\n");
	};

	//sleep for a relatively long time, and check the running state.
	while(1)
	{
		//video
		/*
		if (!is_video_encoding()) 
		{
			usleep(100 * 1000);
			printf("video encoding stop !\n");
			//show_waiting();
		} 
		*/
		//audio
		pthread_mutex_lock(&audio_mutex);
		if (audio_state == 0) 
		{
			usleep(100 * 1000);
			printf("audio encoding error !\n");
			audio_state = 1;
			//show_waiting();
		} 
		pthread_mutex_unlock(&audio_mutex);		
		//rtmp
		pthread_mutex_lock(&rtmp_mutex);
		if (audio_state == 0) 
		{
			usleep(100 * 1000);
			printf("audio encoding error !\n");
			audio_state = 1;
			//show_waiting();
		} 
		pthread_mutex_unlock(&rtmp_mutex);

		usleep(100 * 1000);
	}
	return 0;
}
