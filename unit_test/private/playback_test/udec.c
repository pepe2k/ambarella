
/*
 * udec.c
 *
 * History:
 *	2010/12/13 - [Oliver Li] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>

#include "types.h"
#include "ambas_common.h"
#include "ambas_vout.h"
#include "iav_drv.h"

#ifndef KB
#define KB	(1*1024)
#endif

#ifndef MB
#define MB	(1*1024*1024)
#endif

#include "fbtest.c"

int vout_get_sink_id(int chan, int sink_type);
long long get_tick(void);

int Media_InitForFile(int fd, int fsize);
void Media_Reset(void);
int Media_EOF(void);
int Media_Release(void);

int Media_ThreadInit(void);
void Media_ThreadRun(void);
void Media_ThreadStop(void);
int Media_ThreadKill(void);

u32 Media_ReadFrame(u8 *fifo_start, u32 fifo_size, u8 *fifo_ptr);
u32 Media_AppendEOS(u8 *fifo_start, u32 fifo_size, u8 *fifo_ptr);

int ReadMediaEntries(void);
int GetMediaFileInfo(int file_index);
int GetMdecFileInfo(void);

int renderer_init(void);
int renderer_kill(void);
int renderer_echo(void);
int renderer_start(void);
int renderer_render(iav_frame_buffer_t *fb);

int verbose = 0;
int test_multi = 0;

int udec_id = 0;
int udec_type = UDEC_NONE;

int mdec_pindex = 0;

#define u_assert(expr) do { \
    if (!(expr)) { \
        u_printf("assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
    } \
} while (0)

#define u_printf_error(format,args...)  do { \
    u_printf("[Error] at %s:%d: ", __FILE__, __LINE__); \
    u_printf(format,##args); \
} while (0)

#define DATA_PARSE_MIN_SIZE 128*1024
#define MAX_DUMP_FILE_NAME_LEN 256
static unsigned char _h264_eos[8] = {0x00, 0x00, 0x00, 0x01, 0x0A, 0x0, 0x0, 0x0};

#define HEADER_SIZE 128

//some debug options
static int test_decode_one_trunk = 0;
static int test_dump_total = 0;
static int test_dump_separate = 0;
static char test_dump_total_filename[MAX_DUMP_FILE_NAME_LEN] = "./tot_dump/es_%d";
static char test_dump_separate_filename[MAX_DUMP_FILE_NAME_LEN] = "./sep_dump/es_%d_%d";
static int test_feed_background = 0;
static int feeding_sds = 1;
static int feeding_hds = 0;

//debug only
static int get_pts_from_dsp = 1;

#define _no_log(format, args...)	(void)0
//debug log
#if 1
#define u_printf_binary _no_log
#define u_printf_binary_index _no_log
#else
#define u_printf_binary u_printf
#define u_printf_binary_index u_printf_index
#endif


int u_printf(const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	long long ticks;

	ticks = get_tick();

	pthread_mutex_lock(&mutex);

	if (test_multi)
		printf("%lld [%d] ", ticks, udec_id);
	else
		printf("%lld  ", ticks);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int u_printf_index(unsigned index, const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	long long ticks;

	ticks = get_tick();

	pthread_mutex_lock(&mutex);

	printf("%lld [%d] ", ticks, index);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int v_printf(const char *fmt, ...)
{
	va_list args;
	int rval;
	if (verbose == 0)
		return 0;
	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);
	return rval;
}

#define Assert(_expr) \
	do { \
		if (!(_expr)) { \
			printf("Assertion failed: %s\n", #_expr); \
			printf("At line %d\n", __LINE__); \
			exit(-1); \
		} \
	} while (0)

char pathname[256] = "/tmp/mmcblk0p1";

#define MAX_FILES	8

typedef char FileName[256];
FileName file_list[MAX_FILES];
static int current_file_index = 0;
static unsigned int file_codec[MAX_FILES];
static int file_video_width[MAX_FILES];
static int file_video_height[MAX_FILES];
static int is_hd[MAX_FILES];
static int first_show_full_screen = 0;
static int first_show_hd_stream = 0;

static int mdec_seamless_switch = 0;
static int mdec_loop = 1;
static unsigned int mdec_num_of_udec[2] = {4, 1};
static unsigned int mdec_running = 1;

char current_bmp_filename[256];
FileName bmp_list[MAX_FILES];
int bmp_used = 0;
int file_count = 0;

int save = 0;
char save_filename[256];

int pic_width;
int pic_height;
int picf = 0;

int cmd_idle = 0;

int repeat_times = 1;
int loop_times = 1;
int circulate_times = 1;

int tiled_mode = 0;
int ppmode = 2;	// 0: no pp; 1: decpp; 2: voutpp
int deint = 0;

int dec_types = 0x3F;
int max_frm_num = 10;
int bits_fifo_size = 4*MB;
int ref_cache_size = 0;
int bits_fifo_prefill_size = 2*MB;
int bits_fifo_emptiness = 1*MB;
int pjpeg_buf_size = 4*MB;

int sync_vout = 1;
int nowait = 0;
int do_wait = 0;
int norend = 0;

int iav_deint = -1;
int iav_enable_irqp = 0;
int iav_enable_vm = 0;
int iav_enable_syncc = -1;
int iav_clear_log = 0;

int test_swdec = 0;
int test_mem = 0;
int test_map = 0;
int test_giveup = 0;
int test_takeover = 0;

int test_jpeg = 0;
const char *jpeg_filename;

int disp_bitmap = 0;
const char *bitmap_filename;

int wait_io = 0;

static int add_useq_upes_header = 1;

////////////////////////////////////////////////////////////////////////////////////

#define DECODE_MDEC
#include "decode_util.c"

////////////////////////////////////////////////////////////////////////////////////

typedef struct udec_obj_s {
	iav_udec_info_ex_t udec_info;

	int fd;
	u32 fsize;
	u32 fpos;

	int input_cmds;
	int input_frames;
	int output_frames;
	int input_counter;
	int eos_flag;

	u8 udec_id;
	u8 *fifo_ptr;
	u8 *fifo_end_ptr;
	u32 fifo_size;
	u32 room;
	int wrap_count;

	long long tick_before_read_file;
	long long tick_after_read_file;
	long long tick_before_start_decode;
	long long tick_after_first_outpic;
	long long tick_after_last_outpic;

	int total_input;
	int total_pics;
	u32 total_size;

	int num_waiters;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
} udec_obj_t;

udec_obj_t udec = {
	.num_waiters = 0,
	.cond = PTHREAD_COND_INITIALIZER,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};


int npp = 5;
int interlaced_out = 0;
int packed_out = 0;	// 0: planar yuv; 1: packed yuyv


#define MAX_DECODE_FRAMES	20
#define MAX_INPUT_FRAMES	24

//some utils code for USEQ/UPES Header
#define UDEC_SEQ_HEADER_LENGTH 20
#define UDEC_SEQ_HEADER_EX_LENGTH 24
#define UDEC_PES_HEADER_LENGTH 24

#define UDEC_SEQ_STARTCODE_H264 0x7D
#define UDEC_SEQ_STARTCODE_MPEG4 0xC4
#define UDEC_SEQ_STARTCODE_VC1WMV3 0x71

#define UDEC_PES_STARTCODE_H264 0x7B
#define UDEC_PES_STARTCODE_MPEG4 0xC5
#define UDEC_PES_STARTCODE_VC1WMV3 0x72

#define UDEC_PES_STARTCODE_MPEG12 0xE0

typedef struct bitswriter_t {
    unsigned char* p_start, *p_cur;
    unsigned int size;
    unsigned int left_bits;
    unsigned int left_bytes;
    unsigned int full;
} bitswriter_t;

static bitswriter_t* create_bitswriter(unsigned char* p_start, unsigned int size)
{
    bitswriter_t* p_writer = (bitswriter_t*) malloc(sizeof(bitswriter_t));

    //memset(p_start, 0x0, size);
    p_writer->p_start = p_start;
    p_writer->p_cur = p_start;
    p_writer->size = size;
    p_writer->left_bits = 8;
    p_writer->left_bytes = size;
    p_writer->full = 0;

    return p_writer;
}

static void delete_bitswriter(bitswriter_t* p_writer)
{
    free(p_writer);
}

static int write_bits(bitswriter_t* p_writer, unsigned int value, unsigned int bits)
{
    u_assert(!p_writer->full);
    u_assert(p_writer->size == (p_writer->p_cur - p_writer->p_start + p_writer->left_bytes));
    u_assert(p_writer->left_bits <= 8);

    if (p_writer->full) {
        u_assert(0);
        return -1;
    }

    while (bits > 0 && !p_writer->full) {
        if (bits <= p_writer->left_bits) {
            u_assert(p_writer->left_bits <= 8);
            *p_writer->p_cur |= (value << (32 - bits)) >> (32 - p_writer->left_bits);
            p_writer->left_bits -= bits;
            //value >>= bits;

            if (p_writer->left_bits == 0) {
                if (p_writer->left_bytes == 0) {
                    p_writer->full = 1;
                    return 0;
                }
                p_writer->left_bits = 8;
                p_writer->left_bytes --;
                p_writer->p_cur ++;
            }
            return 0;
        } else {
            u_assert(p_writer->left_bits <= 8);
            *p_writer->p_cur |= (value << (32 - bits)) >> (32 - p_writer->left_bits);
            value <<= 32 - bits + p_writer->left_bits;
            value >>= 32 - bits + p_writer->left_bits;
            bits -= p_writer->left_bits;

            if (p_writer->left_bytes == 0) {
                p_writer->full = 1;
                return 0;
            }
            p_writer->left_bits = 8;
            p_writer->left_bytes --;
            p_writer->p_cur ++;
        }

    }
    return -2;
}

static unsigned int fill_useq_header(unsigned char* p_header, unsigned int v_format, unsigned int time_scale, unsigned int frame_tick, unsigned int is_mp4s_flag, int vid_container_width, int vid_container_height)
{
    if (UDEC_MP12 == v_format) {
        return 0;
    }

    u_assert(p_header);
    u_assert(v_format >= UDEC_H264);
    u_assert(v_format <= UDEC_VC1);
    int ret = 0;

    bitswriter_t* p_writer = create_bitswriter(p_header, is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH);
    u_assert(p_writer);
    memset(p_header, 0x0, is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH);

    //start code prefix
    ret |= write_bits(p_writer, 0, 16);
    ret |= write_bits(p_writer, 1, 8);

    //start code
    if (v_format == UDEC_H264) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_H264, 8);
    } else if (v_format == UDEC_MP4H) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_MPEG4, 8);
    } else if (v_format == UDEC_VC1) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_VC1WMV3, 8);
    }

    //add Signature Code 0x2449504F "$IPO"
    ret |= write_bits(p_writer, 0x2449504F, 32);

    //video format
    ret |= write_bits(p_writer, v_format, 8);

    //Time Scale low
    ret |= write_bits(p_writer, (time_scale & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, time_scale & 0xff, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //Time Scale high
    ret |= write_bits(p_writer, (time_scale & 0xff000000) >> 24, 8);
    ret |= write_bits(p_writer, (time_scale &0x00ff0000) >> 16, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //num units tick low
    ret |= write_bits(p_writer, (frame_tick & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, frame_tick & 0xff, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //num units tick high
    ret |= write_bits(p_writer, (frame_tick & 0xff000000) >> 24, 8);
    ret |= write_bits(p_writer, (frame_tick &0x00ff0000)>> 16, 8);

    //resolution info flag(0:no res info, 1:has res info)
    ret |= write_bits(p_writer, (is_mp4s_flag &0x1), 1);

    if(is_mp4s_flag)
    {
        //pic width
        ret |= write_bits(p_writer, (vid_container_width & 0x7f00)>>8, 7);
        ret |= write_bits(p_writer, vid_container_width & 0xff, 8);

        //markbit
        ret |= write_bits(p_writer, 3, 2);

        //pic height
        ret |= write_bits(p_writer, (vid_container_height & 0x7f00)>>8, 7);
        ret |= write_bits(p_writer, vid_container_height & 0xff, 8);
    }

    //padding
    ret |= write_bits(p_writer, 0xffffffff, 20);

    u_assert(p_writer->left_bits == 0 || p_writer->left_bits == 8);
    u_assert(p_writer->left_bytes == 0);
    u_assert(!p_writer->full);
    u_assert(!ret);

    delete_bitswriter(p_writer);

    return is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH;
}

static void init_upes_header(unsigned char* p_header, unsigned int v_format)
{
    u_assert(p_header);
    u_assert(v_format >= UDEC_H264);
    u_assert(v_format <= UDEC_VC1);

    memset(p_header, 0x0, UDEC_PES_HEADER_LENGTH);

    //start code prefix
    p_header[2] = 0x1;

    //start code
    if (UDEC_H264 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_H264;
    } else if (UDEC_MP4H == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_MPEG4;
    } else if (UDEC_VC1 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_VC1WMV3;
    } else if(UDEC_MP12 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_MPEG12;
    } else {
        u_assert(0);
    }

    //add Signature Code 0x2449504F "$IPO"
    p_header[4] = 0x24;
    p_header[5] = 0x49;
    p_header[6] = 0x50;
    p_header[7] = 0x4F;
}

static unsigned int fill_upes_header(unsigned char* p_header, unsigned int pts_low, unsigned int pts_high, unsigned int au_datalen, unsigned int has_pts, unsigned int pts_disc)
{
    u_assert(p_header);

    int ret = 0;

    bitswriter_t* p_writer = create_bitswriter(p_header + 8, UDEC_PES_HEADER_LENGTH - 8);
    u_assert(p_writer);
    memset(p_header + 8, 0x0, UDEC_PES_HEADER_LENGTH - 8);

    ret |= write_bits(p_writer, has_pts, 1);

    if (has_pts) {
        //pts 0
        ret |= write_bits(p_writer, (pts_low & 0xff00)>>8, 8);
        ret |= write_bits(p_writer, pts_low & 0xff, 8);
        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 1
        ret |= write_bits(p_writer, (pts_low & 0xff000000) >> 24, 8);
        ret |= write_bits(p_writer, (pts_low & 0xff0000) >> 16, 8);
        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 2
        ret |= write_bits(p_writer, (pts_high & 0xff00)>>8, 8);
        ret |= write_bits(p_writer, pts_high & 0xff, 8);

        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 3
        ret |= write_bits(p_writer, (pts_high & 0xff000000) >> 24, 8);
        ret |= write_bits(p_writer, (pts_high & 0xff0000) >> 16, 8);

        //marker bit
        ret |= write_bits(p_writer, 1, 1);

        //pts_disc
        ret |= write_bits(p_writer, pts_disc, 1);

        //padding
        ret |= write_bits(p_writer, 0xffffffff, 27);
    }

    //au_data_length low
    //AM_PRINTF("au_datalen %d, 0x%x.\n", au_datalen, au_datalen);

    ret |= write_bits(p_writer, (au_datalen & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, (au_datalen & 0xff), 8);
    //marker bit
    ret |= write_bits(p_writer, 1, 1);
    //au_data_length high
    ret |= write_bits(p_writer, (au_datalen & 0x07000000) >> 24, 3);
    ret |= write_bits(p_writer, (au_datalen &0x00ff0000)>> 16, 8);
    //pading
    ret |= write_bits(p_writer, 0xf, 3);

    u_assert(p_writer->left_bits == 0 || p_writer->left_bits == 8);
    u_assert(!ret);

    ret = p_writer->size - p_writer->left_bytes + 8;

    delete_bitswriter(p_writer);
    return ret;

}

int ioctl_enter_idle(void)
{
	if (ioctl(fd_iav, IAV_IOC_ENTER_IDLE, 0) < 0) {
		perror("IAV_IOC_ENTER_IDLE");
		return -1;
	}
	u_printf("enter idle done\n");
	return 0;
}

void init_udec_mode_config(iav_udec_mode_config_t *udec_mode)
{
	memset(udec_mode, 0, sizeof(*udec_mode));

	udec_mode->postp_mode = ppmode;
	udec_mode->enable_deint = deint;

	udec_mode->pp_chroma_fmt_max = 2; // 4:2:2
	udec_mode->pp_max_frm_width = max_vout_width;	// vout_width;
	udec_mode->pp_max_frm_height = max_vout_height;	// vout_height;

	if (udec_type == UDEC_JPEG)
		udec_mode->pp_max_frm_num = 1;
	else
		udec_mode->pp_max_frm_num = npp;

	udec_mode->vout_mask = get_vout_mask();
}

int ioctl_enter_udec_mode(void)
{
	iav_udec_mode_config_t udec_mode;
	iav_udec_config_t udec_config;

	init_udec_mode_config(&udec_mode);

	udec_mode.num_udecs = 1;	// 1 udec instance
	udec_mode.udec_config = &udec_config;

	memset(&udec_config, 0, sizeof(udec_config));

	udec_config.tiled_mode = tiled_mode;
	udec_config.frm_chroma_fmt_max = 1;	// 4:2:0
	udec_config.dec_types = dec_types;
	udec_config.max_frm_num = max_frm_num; // MAX_DECODE_FRAMES - todo
	udec_config.max_frm_width = 1920;	// todo - read from file header
	udec_config.max_frm_height = 1088;	// todo - read from file header
	udec_config.max_fifo_size = bits_fifo_size;

	if (ioctl(fd_iav, IAV_IOC_ENTER_UDEC_MODE, &udec_mode) < 0) {
		perror("IAV_IOC_ENTER_UDEC_MODE");
		return -1;
	}

	u_printf("enter udec mode done\n");
	return 0;
}

int ioctl_init_udec(void)
{
	iav_udec_info_ex_t *info = &udec.udec_info;
	iav_udec_vout_config_t vout_config[NUM_VOUT];
	int i;

	memset(info, 0, sizeof(*info));
	memset(&vout_config, 0, sizeof(vout_config));

	u_printf("create udec %d, type %d\n", udec_id, udec_type);

	info->udec_id = udec_id;
	info->udec_type = udec_type;
	info->enable_pp = 1;
	info->enable_deint = deint;
	info->interlaced_out = interlaced_out;
	info->packed_out = packed_out;

	if (vout_index == NUM_VOUT) {
		info->vout_configs.num_vout = NUM_VOUT;
		info->vout_configs.vout_config = vout_config;

		for (i = 0; i < NUM_VOUT; i++) {
			vout_config[i].vout_id = vout_id[i];
			vout_config[i].target_win_offset_x = 0;
			vout_config[i].target_win_offset_y = 0;
			vout_config[i].target_win_width = vout_width[i];
			vout_config[i].target_win_height = vout_height[i];
			vout_config[i].zoom_factor_x = 1;
			vout_config[i].zoom_factor_y = 1;
		}

	} else {
		info->vout_configs.num_vout = 1;
		info->vout_configs.vout_config = vout_config;

		vout_config[0].vout_id = vout_id[vout_index];
		vout_config[0].target_win_offset_x = 0;
		vout_config[0].target_win_offset_y = 0;
		vout_config[0].target_win_width = vout_width[vout_index];
		vout_config[0].target_win_height = vout_height[vout_index];
		vout_config[0].zoom_factor_x = 1;
		vout_config[0].zoom_factor_y = 1;
	}

	info->vout_configs.first_pts_low = 0;
	info->vout_configs.first_pts_high = 0;

	info->vout_configs.input_center_x = pic_width / 2;
	info->vout_configs.input_center_y = pic_height / 2;

	info->bits_fifo_size = bits_fifo_size;
	info->ref_cache_size = ref_cache_size;

	switch (udec_type) {
	case UDEC_H264:
		info->u.h264.pjpeg_buf_size = pjpeg_buf_size;
		break;

	case UDEC_MP12:
	case UDEC_MP4H:
		info->u.mpeg.deblocking_flag = 0;
		break;

	case UDEC_JPEG:
		info->u.jpeg.still_bits_circular = 0;
		info->u.jpeg.still_max_decode_width = pic_width;
		info->u.jpeg.still_max_decode_height = pic_height;
		break;

	case UDEC_VC1:
		break;

	case UDEC_NONE:
		break;

	case UDEC_SW:
		break;

	default:
		u_printf("udec type %d not implemented\n", udec_type);
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_INIT_UDEC, info) < 0) {
		perror("IAV_IOC_INIT_UDEC");
		return -1;
	}

	return 0;
}

int ioctl_stop_udec(void)
{
	v_printf("stop udec...");
	if (ioctl(fd_iav, IAV_IOC_UDEC_STOP, udec.udec_id) < 0) {
		perror("IAV_IOC_UDEC_STOP");
		return -1;
	}
	v_printf("done\n");
	return 0;
}

int ioctl_release_udec(void)
{
	v_printf("destroy udec...");
	if (ioctl(fd_iav, IAV_IOC_RELEASE_UDEC, udec.udec_id) < 0) {
		perror("IAV_IOC_DESTROY_UDEC");
		return -1;
	}
	v_printf("done\n");
	return 0;
}

int ioctl_wait_decoder(iav_wait_decoder_t *wait)
{
	if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, wait) < 0) {
		perror("IAV_IOC_WAIT_DECODER");
		return -1;
	}
	return 0;
}

u8 *udec_next_fifo_ptr(u8 *ptr, u32 size)
{
	ptr += size;

	if (ptr < udec.fifo_end_ptr)
		return ptr;

	return udec.udec_info.bits_fifo_start + (ptr - udec.fifo_end_ptr);
}

int ioctl_udec_decode(void)
{
	iav_udec_decode_t dec;

	memset(&dec, 0, sizeof(dec));
	dec.udec_type = udec_type;
	dec.decoder_id = udec.udec_id;
	dec.num_pics = udec.total_pics;
	dec.u.fifo.start_addr = udec.fifo_ptr;
	dec.u.fifo.end_addr = udec_next_fifo_ptr(udec.fifo_ptr, udec.total_size);

	if (ioctl(fd_iav, IAV_IOC_UDEC_DECODE, &dec) < 0) {
		perror("IAV_IOC_UDEC_DECODE");
		return -1;
	}

	v_printf("udec_decode: num_pics=%d, fifo: 0x%x - 0x%x, size: %d (0x%x)\n",
		dec.num_pics, dec.u.fifo.start_addr, dec.u.fifo.end_addr,
		udec.total_size, udec.total_size);

	return 0;
}

int save_jpeg(iav_frame_buffer_t *frame)
{
	int fd;
	int size;

	u_printf("chroma_format: %d\n", frame->chroma_format);
	if (frame->chroma_format != 1 && frame->chroma_format != 2) {
		u_printf("Unkown chroma format\n");
		return -1;
	}

	u_printf("saving to %s...\n", save_filename);

	if ((fd = open(save_filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(save_filename);
		return -1;
	}

	// Y
	size = frame->buffer_pitch * frame->buffer_height;
	if (write(fd, frame->lu_buf_addr, size) != size) {
		perror("write");
		return -1;
	}

	// UV
	if (frame->chroma_format == 1)
		size /= 2;

	if (write(fd, frame->ch_buf_addr, size) != size) {
		perror("write");
		return -1;
	}

	close(fd);
	u_printf("JPEG saved to %s: %d*%d\n", save_filename, frame->buffer_pitch, frame->buffer_height);

	return 0;
}

int feed_bits(int pics_limit, u32 size_limit)
{
	u8 *ptr = udec.fifo_ptr;
	u32 frame_size;
	int num_pics;

	udec.total_pics = 0;
	udec.total_input = 0;
	udec.total_size = 0;

	while (!Media_EOF()) {
		iav_wait_decoder_t wait;

		if (do_wait) {
			u_printf("press enter to decode frame %d", udec.input_frames);
			if (getchar() == 'c')
				do_wait = 0;
		}

		memset(&wait, 0, sizeof(wait));
		wait.flags = IAV_WAIT_BITS_FIFO;
		wait.decoder_id = udec.udec_id;
		wait.emptiness.room = bits_fifo_emptiness + 16;	// 16 more for EOS
		wait.emptiness.start_addr = ptr;

		v_printf("wait bits fifo\n");
		if (ioctl_wait_decoder(&wait) < 0)
			return -1;

		if ((frame_size = Media_ReadFrame(udec.fifo_end_ptr - udec.fifo_size, udec.fifo_size, ptr)) == 0)
			break;

		v_printf("frame: %d bytes\n", frame_size);

		num_pics = (udec_type == UDEC_VC1 && ptr[3] == 0x0C) ? 0 : 1;

		udec.total_pics++;
		udec.total_input += num_pics;
		udec.total_size += frame_size;

		udec.fpos += frame_size;

		if (pics_limit != 0 && udec.total_pics >= pics_limit)
			break;

		if (size_limit != 0 && udec.total_size >= size_limit)
			break;

		ptr = udec_next_fifo_ptr(ptr, frame_size);
	}

	return 0;
}

int ioctl_wake_vout(void)
{
	iav_wait_decoder_t wait;
	iav_wake_vout_t wake_vout;

	u_printf("start wake_vout\n");
	while (1) {
		memset(&wait, 0, sizeof(wait));
		wait.flags = IAV_WAIT_VOUT_STATE;
		if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, &wait) < 0) {
			perror("IAV_IOC_WAIT_DECODER");
			return -1;
		}

		if (wait.flags != IAV_WAIT_VOUT_STATE) {
			u_printf("failed\n");
			return -1;
		}

		if (wait.vout_state == 	 IAV_VOUT_STATE_DORMANT)
			break;
	}
	u_printf("wake_vout done\n");

	memset(&wake_vout, 0, sizeof(wake_vout));
	wake_vout.decoder_id = udec.udec_id;

	if (ioctl(fd_iav, IAV_IOC_WAKE_VOUT, &wake_vout) < 0) {
		perror("IAV_IOC_WAKE_VOUT");
		return -1;
	}

	return 0;
}

void update_udec_obj(void)
{
	udec.fifo_ptr = udec_next_fifo_ptr(udec.fifo_ptr, udec.total_size);
	pthread_mutex_lock(&udec.mutex);
	udec.input_cmds += udec.total_input;
	udec.input_frames += udec.total_input;
	pthread_mutex_unlock(&udec.mutex);
}

int feed_bits_and_decode(int npic, int prepare)
{
	for (; npic > 0; npic--) {
		if (feed_bits(1, 0) < 0)
			return -1;

		if (udec_type == UDEC_JPEG)
			udec.tick_after_read_file = get_tick();

		if (udec.tick_before_start_decode == -1)
			udec.tick_before_start_decode = get_tick();

		if (ioctl_udec_decode() < 0)
			return -1;

		update_udec_obj();

		if (Media_EOF())
			break;

		if (prepare && udec.fpos >= bits_fifo_prefill_size)
			break;
	}
	return 0;
}

void reset_udec_obj(void)
{
	udec.tick_before_read_file = -1;
	udec.tick_before_start_decode = -1;
	udec.tick_after_first_outpic = -1;
	udec.tick_after_last_outpic = -1;

	udec.fpos = 0;
	udec.input_cmds = 0;
	udec.input_frames = 0;
	udec.output_frames = 0;
	udec.input_counter = 0;
	udec.eos_flag = 0;
}

int udec_decode_file(int findex, int last_file)
{
	int times = 1;
	int done = 0;

	reset_udec_obj();
	udec.tick_before_read_file = get_tick();

	// open the file
	if ((udec.fd = open_file(file_list[findex], &udec.fsize)) < 0)
		return -1;

	Media_InitForFile(udec.fd, udec.fsize);
	Media_Reset();

	if (!test_multi)
		Media_ThreadRun();

	// init udec
	if (ioctl_init_udec() < 0)
		return -1;

	udec.udec_id = udec.udec_info.udec_id;

	udec.fifo_ptr = udec.udec_info.bits_fifo_start;
	udec.fifo_size = udec.udec_info.bits_fifo_size;

	udec.fifo_end_ptr = udec.fifo_ptr + udec.fifo_size;
	udec.room = udec.fifo_size;

	u_printf("udec[%d]: bits_fifo_start = 0x%x, size = 0x%x\n",
		udec.udec_id, (u32)udec.fifo_ptr, udec.fifo_size);

	// open vout
	if (ppmode == 1 || ppmode == 2) {
		if (vout_index == NUM_VOUT) {
			int i;
			for (i = 0; i < NUM_VOUT; i++)
				if (ioctl_config_vout(vout_id[i], 0, 0, vout_width[i], vout_height[i]) < 0)
					return -1;
		} else {
			if (ioctl_config_vout(vout_id[vout_index], 0, 0, vout_width[vout_index], vout_height[vout_index]) < 0)
				return -1;
		}
	}

	// prepare
	u_printf("start feeding bits...\n");
	if (feed_bits_and_decode(MAX_INPUT_FRAMES, 1) < 0)
		return -1;
	u_printf("fed %d bytes\n", udec.fpos);

	if (ppmode != 3) {
		u_printf("start renderer\n");
		renderer_start();
	}

	while (!done) {
		// decoding
		while (!Media_EOF()) {

			if (wait_io) {
				pthread_mutex_lock(&udec.mutex);
				while (1) {
					if (udec.input_cmds < MAX_INPUT_FRAMES)
						break;
					if (udec.eos_flag) {
						u_printf("unexpected EOS frame: %d\n", udec.output_frames);
					}
					udec.num_waiters++;
					v_printf("wait outpic\n");
					pthread_cond_wait(&udec.cond, &udec.mutex);
				}
				pthread_mutex_unlock(&udec.mutex);
			}

			if (feed_bits_and_decode(1, 0) < 0)
				return -1;
		}

		// check: if we should goto the start of the file directly without sending EOS and waiting EOS
		if (times != circulate_times) {
			times++;
			u_printf("refill %s, round %d\n", file_list[findex], times);
			Media_Reset();
			//reset_udec_obj();
			//udec.tick_before_read_file = get_tick();
			continue;
		}

		// feed EOS
		u_printf("read file done\n");
		udec.total_size = Media_AppendEOS(udec.fifo_end_ptr - udec.fifo_size, udec.fifo_size, udec.fifo_ptr);
		if (udec.total_size > 0) {
			u_printf("=== total %d frames, send EOS ===\n", udec.input_frames);
			udec.total_pics = 1;
			if (ioctl_udec_decode() < 0)
				return -1;
			udec.fifo_ptr = udec_next_fifo_ptr(udec.fifo_ptr, udec.total_size);
		}

		// wait EOS
		u_printf("wait EOS\n");
		pthread_mutex_lock(&udec.mutex);
		while (1) {
			int total_frames;

			if (udec.eos_flag || (udec_type == UDEC_JPEG && udec.output_frames > 0)) {
				if (udec_type == UDEC_JPEG)
					u_printf("JPEG decoding done\n");
				else
					u_printf("=== EOS received ===\n");

				if (udec_type == UDEC_JPEG || ppmode == 1)
					total_frames = udec.output_frames;
				else
					total_frames = udec.input_frames;

				udec.tick_after_last_outpic = get_tick();
				u_printf("All frames rendered, input %d, output %d\n", udec.input_frames, udec.output_frames);
				u_printf("start_tick: %lld, %lld\n", udec.tick_before_read_file, udec.tick_after_first_outpic);

				u_printf("fps: %f, %f\n",
					(float)total_frames * 1000 / (udec.tick_after_last_outpic - udec.tick_before_read_file),
					(float)total_frames * 1000 / (udec.tick_after_last_outpic - udec.tick_after_first_outpic));

				if (udec_type == UDEC_JPEG) {
					u_printf("JPEG read: %lld ms\n", udec.tick_after_read_file - udec.tick_before_read_file);
					u_printf("JPEG decode: %lld ms\n", udec.tick_after_first_outpic - udec.tick_after_read_file);
				}

				if (times == repeat_times) {
					if (last_file && !nowait) {
						u_printf("press enter to exit...");
						getchar();
					}
					done = 1;
				} else {
					Media_Reset();
					times++;
					reset_udec_obj();
					udec.tick_before_read_file = get_tick();
				}

				break;
			}

			udec.num_waiters++;
			pthread_cond_wait(&udec.cond, &udec.mutex);
		}
		pthread_mutex_unlock(&udec.mutex);

		if (udec_type == UDEC_JPEG) {
		}
	}

	Media_Release();
	close(udec.fd);

	if (ioctl_stop_udec() < 0)
		return -1;

	if (ppmode != 3) {
		renderer_echo();
	}

	Media_ThreadStop();

	if (ioctl_release_udec() < 0)
		return -1;

	return 0;
}

int test_udec(void)
{
	int loops = 0;
	int i;

	u_printf("::: total %d files to decode for %d times\n", file_count, loop_times);
	u_printf("npp = %d, max_input = %d\n", npp, MAX_INPUT_FRAMES);

	u_printf("::: create renderer\n");
	if (renderer_init() < 0)
		return -1;

	u_printf("::: create reader\n");
	if (Media_ThreadInit() < 0)
		return -1;

	while (1) {
		if (loop_times != 1) {
			if (loop_times == 0)
				u_printf("Loop start: %d and forever.\n", loops + 1);
			else
				u_printf("Loop start: %d of %d\n", loops + 1, loop_times);
		}

		for (i = 0; i < file_count; i++) {
			u_printf("====================================\n");
			u_printf("::: [%d] decode file %d: %s\n", loops + 1, i, file_list[i]);
			u_printf("====================================\n");

			// get the file info
			if (GetMediaFileInfo(i) < 0)
				return -1;

			u_printf("current bmp: %s\n", current_bmp_filename);
			if (bmp_used || current_bmp_filename[0]) {
				const char *fname = bmp_list[i][0] ? bmp_list[i] : current_bmp_filename;
				u_printf("show bmp: %s\n", fname);
				show_bitmap(fname, 1);
			}

			if (udec_decode_file(i, i + 1 == file_count && loops + 1 == loop_times) < 0)
				return -1;

			if (bmp_used || current_bmp_filename[0]) {
				u_printf("clear screen\n");
				show_bitmap(NULL, 1);
			}
		}

		if (++loops == loop_times)
			break;
	}

	renderer_kill();

	return 0;
}

int map_dsp(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	u_printf("dsp_mem = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);
	return 0;
}

int ioctl_config_fb_pool(void)
{
	iav_fbp_config_t fbp_config;

	memset(&fbp_config, 0, sizeof(fbp_config));
	fbp_config.decoder_id = udec_id;
	fbp_config.chroma_format = 1;	// 420
	fbp_config.tiled_mode = 0;
	fbp_config.buf_width = pic_width;
	fbp_config.buf_height = pic_height;
	fbp_config.lu_width = pic_width;
	fbp_config.lu_height = pic_height;
	fbp_config.lu_row_offset = 0;
	fbp_config.lu_col_offset = 0;
	fbp_config.ch_row_offset = 0;
	fbp_config.ch_col_offset = 0;

	if (ioctl(fd_iav, IAV_IOC_CONFIG_FB_POOL, &fbp_config) < 0) {
		perror("IAV_IOC_CONFIG_FB_POOL");
		return -1;
	}

	u_printf("config fb pool done\n");
	return 0;
}

int start_swdec(void)
{
	iav_udec_decode_t info;
	memset(&info, 0, sizeof(info));

	info.udec_type = UDEC_SW;
	info.decoder_id = udec.udec_id;
	info.num_pics = 0;

	if (ioctl(fd_iav, IAV_IOC_UDEC_DECODE, &info) < 0) {
		perror("IAV_IOC_UDEC_DECODE");
		return -1;
	}

	return 0;
}

int do_test_swdec(void)
{
	u64 pts = 0;
	u8 y = 100;
	u8 u = 100;
	u8 v = 200;
	int uv;
	int frames = 0;
	u16 *ptr;
	int i, j;

	udec_type = UDEC_SW;
	bits_fifo_size = 0;
	pjpeg_buf_size = 0;

	pic_width = 640;
	pic_height = 480;

	if (map_dsp() < 0)
		return -1;

	if (ioctl_init_udec() < 0)
		return -1;

	if (ioctl_config_fb_pool() < 0)
		return -1;

	if (start_swdec() < 0)
		return -1;

	while (1) {
		iav_frame_buffer_t frame;

		memset(&frame, 0, sizeof(frame));
		frame.flags = 0;
		frame.decoder_id = udec.udec_id;
		frame.pic_struct = 3;

		if (ioctl(fd_iav, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
			perror("IAV_IOC_REQUEST_FRAME");
			return -1;
		}

		v_printf("fb_id: %d\n", frame.fb_id);

		// note: set IAV_FRAME_NO_RELEASE to frame.flags to prevent the buffer being released
		frame.flags = 0;
		frame.pts = (u32)pts;
		frame.pts_high = (u32)(pts >> 32);
		frame.interlaced = 0;
		frame.pic_width = pic_width;
		frame.pic_height = pic_height;

		pts += 3003;

		memset(frame.lu_buf_addr, y, frame.buffer_pitch * frame.pic_height);

		ptr = frame.ch_buf_addr;
		uv = (u << 8) | v;
		for (i = 0; i < frame.pic_height; i++)
			for (j = 0; j < frame.buffer_pitch; j+= 2)
				*ptr++ = uv;

		if (u >= 200)
			u = 100;
		else
			u++;
		if (v >= 200)
			v = 100;
		else
			v++;

		if (ioctl(fd_iav, IAV_IOC_POSTP_FRAME, &frame) < 0) {
			perror("IAV_IOC_POSTP_FRAME");
			return -1;
		}

		v_printf("postp_done\n");

		frames++;
		if ((frames % 30) == 0)
			u_printf("total frames: %d, last_pts: %d\n", frames, frame.pts);
	}

	return 0;
}

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"filename", HAS_ARG, 0, 'f'},
	{"save", HAS_ARG, 0, 's'},

	{"ppmode", HAS_ARG, 0, 'p'},
	{"tiled", NO_ARG, 0, 7},

	{"vout", HAS_ARG, 0, 15},

	{"repeat", HAS_ARG, 0, 'r'},
	{"loop", HAS_ARG, 0, 'l'},
	{"circulate", HAS_ARG, 0, 'c'},

	{"verbose", NO_ARG, 0, 'v'},

	{"nosync", NO_ARG, 0, 11},
	{"wait", NO_ARG, 0, 12},
	{"waitio", NO_ARG, 0, 19},
	{"nowait", NO_ARG, 0, 13},
	{"norend", NO_ARG, 0, 20},

	{"iavdeint", HAS_ARG, 0, 14},
	{"irqp", NO_ARG, 0, 9},
	{"vm", NO_ARG, 0, 23},
	{"syncc", HAS_ARG, 0, 24},
	{"clear-log", NO_ARG, 0, 25},

	{"npp", HAS_ARG, 0, 16},
	{"interlaced", NO_ARG, 0, 18},
	{"packed", NO_ARG, 0, 26},
	{"bmp", HAS_ARG, 0, 17},
	{"idle", NO_ARG, 0, 21},

	{"soft", NO_ARG, 0, 8},
	{"mem", NO_ARG, 0, 22},

	{"jpeg", HAS_ARG, 0, 27},
	{"bitmap", HAS_ARG, 0, 28},
	{"map", NO_ARG, 0, 29},

	{"giveup", NO_ARG, 0, 30},
	{"takeover", NO_ARG, 0, 31},

	{"multi", HAS_ARG, 0, 'm'},
	{"path", HAS_ARG, 0, 'P'},

	{0, 0, 0, 0}
};

static const char *short_options = "c:f:l:m:p:P:r:s:v";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename",	""	"specify filename"},
	{"filename",	"\t"	"specify filename to save"},

	{"0,1,2",	"\t"	"specify postp mode"},
	{"",		"\t\t"	"tiled_mode"},

	{"0,1,2",	"\t"	"set vout index"},

	{"times",	"\t"	"set repeat count. 0 means forever"},
	{"times",	"\t"	"set loop count. 0 means forever"},
	{"times",	"\t"	"circulate with no EOS. 0 means forever"},

	{"",		"\t\t"	"verbose print"},

	{"",		"\t\t"	"do not sync with vout irq"},
	{"",		"\t\t"	"decode frame by frame"},
	{"",		"\t\t"	"sync output with input"},
	{"",		"\t\t"	"do not wait when decoding ends"},
	{"",		"\t\t"	"do not render"},

	{"0,1",		"\t"	"disable/enable iav deinterlacing"},
	{"",		"\t\t"	"enable iav print in IRQ"},
	{"",		"\t\t\t""enable virtual memory"},
	{"0,1",		"\t"	"disable/enable sync counter"},
	{"",		"\t\t"	"clear dsp cmd/msg log"},

	{"pp_frm_num",	"\t"	"set max pp vout frame number"},
	{"",		"\t\t"	"output is interlaced"},
	{"",		"\t\t"	"output is packed"},
	{"bmp filename","\t"	"set bitmap for the video"},
	{"",		"\t\t"	"enter idle state"},

	{"",		"\t\t"	"test software decoder"},
	{"",		"\t\t"	"test memory"},

	{"filename",	"\t"	"specify filename"},
	{"filename",	"\t"	"specify filename"},

	{"",		"\t\t"	"test sync counter mapping"},

	{"",		"\t\t"	"give up DSP control"},
	{"",		"\t\t"	"take over DSP contro"},

	{"number",	"\t"	"test multi-window playback"},
	{"pathname",	"\t"	"set path for media.txt"},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		case 'p':
			ppmode = atoi(optarg);
			break;

		case 7:
			tiled_mode = 1;
			break;

		case 8:
			test_swdec = 1;
			break;

		case 9:
			iav_enable_irqp = 1;
			break;

		case 11:
			sync_vout = 0;
			break;

		case 12:
			do_wait = 1;
			break;

		case 13:
			nowait = 1;
			break;

		case 14:
			iav_deint = atoi(optarg);
			break;

		case 15:
			vout_index = atoi(optarg);
			if (vout_index < 0 || vout_index > NUM_VOUT) {
				// vout_index == 2: use 2 VOUTs
				u_printf("bad vout index %d\n", vout_index);
				return -1;
			}
			break;

		case 16:
			npp = atoi(optarg);
			break;

		case 17:
			if (file_count >= MAX_FILES) {
				u_printf("Waining: too many files (max %d)\n", MAX_FILES);
			} else {
				if (file_count > 0) {
					strcpy(bmp_list[file_count-1], optarg);
					bmp_used = 1;
				}
				//
			}
			break;

		case 18:
			interlaced_out = 1;
			break;

		case 19:
			wait_io = 1;
			break;

		case 20:
			norend = 1;
			break;

		case 21:
			cmd_idle = 1;
			break;

		case 22:
			test_mem = 1;
			break;

		case 23:
			iav_enable_vm = 1;
			break;

		case 24:
			iav_enable_syncc = atoi(optarg);
			break;

		case 25:
			iav_clear_log = 1;
			break;

		case 26:
			packed_out = 1;
			break;

		case 27:
			test_jpeg = 1;
			jpeg_filename = optarg;
			break;

		case 28:
			disp_bitmap = 1;
			bitmap_filename = optarg;
			break;

		case 29:
			test_map = 1;
			break;

		case 30:
			test_giveup = 1;
			break;

		case 31:
			test_takeover = 1;
			break;

		case 'r':
			repeat_times = atoi(optarg);
			break;

		case 's':
			save = 1;
			strcpy(save_filename, optarg);
			break;

		case 'l':
			loop_times = atoi(optarg);
			break;

		case 'c':
			circulate_times = atoi(optarg);
			break;

		case 'm':
			test_multi = atoi(optarg);
			break;

		case 'P':
			strcpy(pathname, optarg);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'f':
			if (file_count >= MAX_FILES) {
				u_printf("Waining: too many files (max %d)\n", MAX_FILES);
			} else {
				strcpy(file_list[file_count], optarg);
				file_count++;
			}
			break;

		default:
			u_printf("unknown option found: %d\n", ch);
			return -1;
		}
	}
	return 0;
}

void usage(void)
{
	int i;

	printf("udec usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

// for debug only
int set_iav_options(void)
{
	if (iav_enable_irqp) {
		ioctl(fd_iav, IAV_IOC_DECODE_DBG, 1);
	}

	if (iav_deint >= 0) {
		ioctl(fd_iav, IAV_IOC_DECODE_DBG, iav_deint ? 3 : 2);
	}

	if (iav_enable_vm) {
		ioctl(fd_iav, IAV_IOC_DECODE_DBG, 5);
	}

	if (iav_enable_syncc >= 0) {
		ioctl(fd_iav, IAV_IOC_DECODE_DBG, iav_enable_syncc ? 7 : 6);
	}

	if (iav_clear_log) {
		ioctl(fd_iav, IAV_IOC_DECODE_DBG, 8);
	}

	return 0;
}

void do_test_jpeg(void)
{
	iav_udec_mode_config_t mode_config;
	iav_udec_config_t udec_config;
	iav_udec_info_ex_t udec_info;
	iav_udec_decode_t udec_decode;
	iav_frame_buffer_t frame;
	int fd;
	u32 fsize;
	int counter = 0;

	while (1) {
		if (open_iav() < 0)
			return;

		ioctl(fd_iav, IAV_IOC_ENTER_IDLE, 0);

		memset(&mode_config, 0, sizeof(mode_config));
		mode_config.postp_mode = 0;
		mode_config.pp_chroma_fmt_max = 2;
		mode_config.pp_max_frm_width = 128;
		mode_config.pp_max_frm_height = 96;
		mode_config.pp_max_frm_num = 1;
		mode_config.num_udecs = 1;
		mode_config.udec_config = &udec_config;

		memset(&udec_config, 0, sizeof(udec_config));
		udec_config.frm_chroma_fmt_max = 1;
		udec_config.dec_types = dec_types;
		udec_config.max_frm_num = 1;
		udec_config.max_frm_width = 128;
		udec_config.max_frm_height = 96;
		udec_config.max_fifo_size = 4*1024*1024;

		if (ioctl(fd_iav, IAV_IOC_ENTER_UDEC_MODE, &mode_config) < 0) {
			perror("IAV_IOC_ENTER_UDEC_MODE");
			return;
		}

		memset(&udec_info, 0, sizeof(udec_info));
		udec_info.udec_id = 0;
		udec_info.udec_type = UDEC_JPEG;
		udec_info.bits_fifo_size = 4*1024*1024;
		udec_info.u.jpeg.still_bits_circular = 0;
		udec_info.u.jpeg.still_max_decode_width = 128;
		udec_info.u.jpeg.still_max_decode_height = 96;

		if (ioctl(fd_iav, IAV_IOC_INIT_UDEC, &udec_info) < 0) {
			perror("IAV_IOC_INIT_UDEC");
			return;
		}

		if ((fd = open_file(jpeg_filename, &fsize)) < 0) {
			perror("jpeg_filename");
			return;
		}

		if (read(fd, udec_info.bits_fifo_start, fsize) != fsize) {
			perror("read");
			return;
		}

		close(fd);

		memset(&udec_decode, 0, sizeof(udec_decode));
		udec_decode.udec_type = UDEC_JPEG;
		udec_decode.decoder_id = udec_info.udec_id;
		udec_decode.num_pics = 1;
		udec_decode.u.fifo.start_addr = udec_info.bits_fifo_start;
		udec_decode.u.fifo.end_addr = udec_info.bits_fifo_start + fsize;

		if (ioctl(fd_iav, IAV_IOC_UDEC_DECODE, &udec_decode) < 0) {
			perror("IAV_IOC_UDEC_DECODE");
			return;
		}

		memset(&frame, 0, sizeof(frame));
		frame.flags = 0;
		frame.decoder_id = udec_info.udec_id;

		if (ioctl(fd_iav, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
			perror("IAV_IOC_GET_DECODED_FRAME");
			return;
		}

		counter++;
		if ((counter % 10) == 0) {
			printf("JPEG decode done: y=0x%x, uv=0x%x\n", (u32)frame.lu_buf_addr, (u32)frame.ch_buf_addr);
			printf("width: %d, height: %d, pitch: %d\n", frame.buffer_width, frame.buffer_height, frame.buffer_pitch);
			printf("%d decode %s done\n", counter, jpeg_filename);
		}

		ioctl(fd_iav, IAV_IOC_RELEASE_UDEC, udec_info.udec_id);

		close(fd_iav);
	}
}

int do_test_map(void)
{
	iav_udec_map_t map;

	memset(&map, 0, sizeof(map));
	if (ioctl(fd_iav, IAV_IOC_UDEC_MAP, &map) < 0) {
		perror("IAV_IOC_UDEC_MAP");
		return -1;
	}

	printf("rv_sync_counter: 0x%x\n", (u32)map.rv_sync_counter);
	printf("rv_sync_counter = 0x%x\n", *map.rv_sync_counter);

	if (ioctl(fd_iav, IAV_IOC_UDEC_UNMAP, 0) < 0) {
		perror("IAV_IOC_UDEC_UNMAP");
		return -1;
	}

	return 0;
}

int do_test_giveup(void)
{
	if (ioctl(fd_iav, IAV_IOC_ENTER_IDLE, IAV_GOTO_IDLE_GIVEUP_DSP) < 0) {
		perror("IAV_GOTO_IDLE_GIVEUP_DSP");
		return -1;
	}

	return 0;
}

int do_test_takeover(void)
{
	if (ioctl(fd_iav, IAV_IOC_ENTER_IDLE, IAV_GOTO_IDLE_TAKEOVER_DSP) < 0) {
		perror("IAV_GOTO_IDLE_TAKEOVER_DSP");
		return -1;
	}

	return 0;
}

static void init_udec_configs(iav_udec_config_t *udec_config, int nr, u16 max_width, u16 max_height)
{
	int i;

	memset(udec_config, 0, sizeof(iav_udec_config_t) * nr);

	for (i = 0; i < nr; i++, udec_config++) {
		udec_config->tiled_mode = tiled_mode;
		udec_config->frm_chroma_fmt_max = 1;	// 4:2:0
		udec_config->dec_types = dec_types;
		udec_config->max_frm_num = max_frm_num; // MAX_DECODE_FRAMES - todo
		udec_config->max_frm_width = max_width;	// todo
		udec_config->max_frm_height = max_height;	// todo
		udec_config->max_fifo_size = bits_fifo_size;
	}
}

#if 0
void init_udec_windows_obsolete(iav_udec_window_t *window, int nr)
{
	int voutA_width = 0;
	int voutA_height = 0;
	int voutB_width = 0;
	int voutB_height = 0;
	u32 vout_mask = get_vout_mask();
	int rows;
	int cols;
	int rindex;
	int cindex;
	int i;

	if (vout_mask & 1) {
		voutA_width = vout_width[0];
		voutA_height = vout_height[0];
	}
	if (vout_mask & 2) {
		voutB_width = vout_width[1];
		voutB_height = vout_height[1];
	}

//	if (nr == 1) {
//		rows = 1;
//		cols = 1;
//	} else
	if (nr <= 4) {
		rows = 2;
		cols = 2;
	} else if (nr <= 9) {
		rows = 3;
		cols = 3;
	} else {
		rows = 4;
		cols = 4;
	}

	memset(window, 0, sizeof(iav_udec_window_t) * nr);

	rindex = 0;
	cindex = 0;
	for (i = 0; i < nr; i++, window++) {
		window->udec_id = i;
		window->input_src_type = 0;
		window->av_sync = 0;
		window->luma_gain = 128;

		window->first_pts_low = 0;
		window->first_pts_high = 0;
		window->input_offset_x = 0;
		window->input_offset_y = 0;
		window->input_width = 720;
		window->input_height = 480;

		window->voutA_rotation = 0;
		window->voutA_flip = 0;
		window->voutA_target_win_offset_x = cindex * voutA_width / cols;
		window->voutA_target_win_offset_y = rindex * voutA_height / rows;
		window->voutA_target_win_width = voutA_width / cols;
		window->voutA_target_win_height = voutA_height / rows;

		window->voutB_rotation = 0;
		window->voutB_flip = 0;
		window->voutB_target_win_offset_x = cindex * voutB_width / cols;
		window->voutB_target_win_offset_y = rindex * voutB_height / rows;
		window->voutB_target_win_width = voutB_width / cols;
		window->voutB_target_win_height = voutB_height / rows;

		if (++cindex == cols) {
			cindex = 0;
			rindex++;
		}
	}
}
#endif

static void init_udec_windows(udec_window_t *win, int start_index, int nr, int src_width, int src_height)
{
	//int voutA_width = 0;
	//int voutA_height = 0;
	int voutB_width = 0;
	int voutB_height = 0;

	int display_width = 0;
	int display_height = 0;

	u32 vout_mask = get_vout_mask();
	int rows;
	int cols;
	int rindex;
	int cindex;
	int i;

	udec_window_t *window = win + start_index;

	//LCD not used yet
#if 0
	if (vout_mask & 1) {
		voutA_width = vout_width[0];
		voutA_height = vout_height[0];
		u_printf("Error: M UDEC not support LCD now!!!!\n");
	}
#endif

	if (vout_mask & 2) {
		voutB_width = vout_width[1];
		voutB_height = vout_height[1];
	} else {
		u_printf("Error: M UDEC must have HDMI configured!!!!\n");
	}

	if (nr == 1) {
		rows = 1;
		cols = 1;
	} else if (nr <= 4) {
		rows = 2;
		cols = 2;
	} else if (nr <= 9) {
		rows = 3;
		cols = 3;
	} else {
		rows = 4;
		cols = 4;
	}

	memset(window, 0, sizeof(udec_window_t) * nr);

	rindex = 0;
	cindex = 0;

	display_width = voutB_width;
	display_height = voutB_height;

	if (!display_width || !display_height) {
		u_printf("why vout width %d, vout height %d are zero?\n");
		display_width = 1280;
		display_height = 720;
	}

	for (i = 0; i < nr; i++, window++) {
		window->win_config_id = i + start_index;

		window->input_offset_x = 0;
		window->input_offset_y = 0;
		window->input_width = src_width;
		window->input_height = src_height;

		window->target_win_offset_x = cindex * display_width / cols;
		window->target_win_offset_y = rindex * display_height / rows;
		window->target_win_width = display_width / cols;
		window->target_win_height = display_height / rows;

		if (++cindex == cols) {
			cindex = 0;
			rindex++;
		}
	}
}

static void init_udec_renders(udec_render_t *render, int nr)
{
	int i;
	memset(render, 0, sizeof(udec_render_t) * nr);

	for (i = 0; i < nr; i++, render++) {
		render->render_id = i;
		render->win_config_id = i;
		render->win_config_id_2nd = 0xff;//hard code
		render->udec_id = i;
	}
}

static void init_udec_renders_single(udec_render_t *render, u8 render_id, u8 win_id, u8 sec_win_id, u8 udec_id)
{
	memset(render, 0, sizeof(udec_render_t));

	render->render_id = render_id;
	render->win_config_id = win_id;
	render->win_config_id_2nd = sec_win_id;
	render->udec_id = udec_id;
}

int ioctl_enter_mdec_mode(void)
{
	//enter mdec mode config
	iav_mdec_mode_config_t mdec_mode;
	iav_udec_mode_config_t *udec_mode = &mdec_mode.super;
	iav_udec_config_t *udec_configs = NULL;
	udec_window_t *windows = NULL;
	udec_render_t *renders = NULL;

	u_printf("[flow]: before malloc some memory\n");
	test_multi = 4;//hard code to 4, fix me
	if ((udec_configs = malloc(test_multi * sizeof(iav_udec_config_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto ioctl_enter_mdec_mode_exit;
	}

	if ((windows = malloc(test_multi * sizeof(udec_window_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto ioctl_enter_mdec_mode_exit;
	}

	if ((renders = malloc(test_multi * sizeof(udec_render_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto ioctl_enter_mdec_mode_exit;
	}
	if (get_vout_info() < 0) {
		goto ioctl_enter_mdec_mode_exit;
	}

	init_udec_mode_config(udec_mode);
	udec_mode->num_udecs = test_multi;
	udec_mode->udec_config = udec_configs;

	mdec_mode.total_num_win_configs = test_multi;
	mdec_mode.windows_config = windows;
	mdec_mode.render_config = renders;

	mdec_mode.av_sync_enabled = 0;
	//mdec_mode.out_to_hdmi = 1;
	mdec_mode.audio_on_win_id = 0;

	u_printf("[flow]: before init_udec_configs\n");
	init_udec_configs(udec_configs, test_multi, 720, 480);

	u_printf("[flow]: before init_udec_windows\n");
	init_udec_windows(windows, 0, test_multi, 720, 480);

	u_printf("[flow]: before init_udec_windows\n");
	init_udec_renders(renders, test_multi);

	if (ioctl(fd_iav, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode) < 0) {
		perror("IAV_IOC_ENTER_MDEC_MODE");
		goto ioctl_enter_mdec_mode_exit;
	}

	free(udec_configs);
	free(renders);
	free(windows);

	u_printf("enter mdec mode done\n");
	return 0;

ioctl_enter_mdec_mode_exit:
	if (udec_configs) {
		free(udec_configs);
	}

	if (renders) {
		free(renders);
	}

	if (windows) {
		free(windows);
	}
	return -1;
}

int mdec_child(void)
{
	int loops = 0;

	if (open_iav() < 0)
		return -1;

	//u_printf("::: create reader\n");
	if (Media_ThreadInit() < 0)
		return -1;

	while (1) {
		if (loop_times != 1) {
			//if (loop_times == 0)
			//	u_printf("Loop start: %d and forever.\n", loops + 1);
			//else
			//	u_printf("Loop start: %d of %d\n", loops + 1, loop_times);
		}

		// get the file info
		if (GetMdecFileInfo() < 0)
			return -1;

		if (udec_decode_file(0, 1) < 0)
			return -1;

		if (++loops == loop_times)
			break;
	}

	u_printf("child %d exits\n", udec_id);

	return 0;
}

int do_test_multi(void)
{
	int i;

	dec_types = 0x17;	// no RV40, no hybrid MPEG4
	max_frm_num = 16;
	bits_fifo_size = 2*MB;
	ref_cache_size = 0;
	bits_fifo_prefill_size = 1*MB;
	bits_fifo_emptiness = 1*MB;
	pjpeg_buf_size = 4*MB;

	ppmode = 3;

	if (test_multi == 0 || test_multi > 16) {
		u_printf("bad number %d\n", test_multi);
		return -1;
	}

	if (ReadMediaEntries() < 0)
		return -1;

	if (ioctl_enter_mdec_mode() < 0)
		return -1;

	for (i = 0; i < test_multi; i++, mdec_pindex++) {
		udec_id = i;
		if (fork() == 0) {
			mdec_child();
			break;
		}
	}

	while (wait(NULL) != -1)
		;

	return 0;
}

static int init_mdec_params(int argc, char **argv)
{
	int i = 0;
	int ret = 0;
	int started_scan_filename = 0;

	//parse options
	for (i=2; i<argc; i++) {
		if (!strcmp("--mode", argv[i])) {
			ret = sscanf(argv[i+1], "%dto%d", &mdec_num_of_udec[0], &mdec_num_of_udec[1]);
			u_printf("[input argument] --mode: %dto%d.\n", mdec_num_of_udec[0], mdec_num_of_udec[1]);
			i ++;
		} else if (!strcmp("--loop", argv[i])) {
			mdec_loop = 1;
			u_printf("[input argument] --loop, enable loop feeding data.\n");
		} else if (!strcmp("--noloop", argv[i])) {
			mdec_loop = 0;
			u_printf("[input argument] --noloop, disable loop feeding data.\n");
		} else if (!strcmp("--nopts", argv[i])) {
			get_pts_from_dsp = 0;
			u_printf("[input argument] --nopts, disable get pts from driver.\n");
		} else if (!strcmp("--pts", argv[i])) {
			get_pts_from_dsp = 1;
			u_printf("[input argument] --pts, enable get pts from driver.\n");
		} else if (!strcmp("--dumpt", argv[i])) {
			test_dump_total = 1;
			u_printf("[input argument] --dumpt, dump to tatal file, default to ./tot_dump/.\n");
		} else if (!strcmp("--dumps", argv[i])) {
			test_dump_separate = 1;
			u_printf("[input argument] --dumps, dump to separate file, default to ./sep_dump/.\n");
		} else if (!strcmp("--oneshot", argv[i])) {
			test_decode_one_trunk = 1;
			u_printf("[input argument] --oneshot, only decode one data block(not greater than 8M).\n");
		} else if (!strcmp("--feedbackground", argv[i])) {
			test_feed_background = 1;
			u_printf("[input argument] --feedbackground, will feed all udec instance.\n");
		} else if (!strcmp("--usequpes", argv[i])) {
			add_useq_upes_header = 1;
			u_printf("[input argument] --usequpes, will add USEQ/UPES header.\n");
		} else if (!strcmp("--nousequpes", argv[i])) {
			add_useq_upes_header = 0;
			u_printf("[input argument] --nousequpes, will not add USEQ/UPES header.\n");
		} else if(!strcmp("--seamless", argv[i])) {
			mdec_seamless_switch = 1;
			u_printf("[input argument] --seamless, enable mdec seamless switch.\n");
		} else if(!strcmp("-f", argv[i])) {
			if (started_scan_filename) {
				current_file_index ++;
			}
			if (current_file_index >= MAX_FILES) {
				u_printf("max file number(current index %d, max value %d).\n", current_file_index, MAX_FILES);
				return (-1);
			}
			memcpy(&file_list[current_file_index][0], argv[i+1], sizeof(file_list[current_file_index]) - 1);
			file_list[current_file_index][sizeof(file_list[current_file_index]) - 1] = 0x0;
			//set default values
			file_codec[current_file_index] = UDEC_H264;
			file_video_width[current_file_index] = 720;
			file_video_height[current_file_index] = 480;
			is_hd[current_file_index] = 0;
			started_scan_filename = 1;
			i ++;
			u_printf("[input argument] -f[%d]: %s.\n", current_file_index, file_list[current_file_index]);
		} else if (!strcmp("-c", argv[i])) {
			if (!strcmp("h264", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_H264;
				u_printf("[input argument] -c[%d]: H264.\n", current_file_index);
			} else if (!strcmp("mpeg12", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_MP12;
				u_printf("[input argument] -c[%d]: MPEG12.\n", current_file_index);
			} else if (!strcmp("mpeg4", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_MP4H;
				u_printf("[input argument] -c[%d]: MPEG4.\n", current_file_index);
			} else if (!strcmp("vc1", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_VC1;
				u_printf("[input argument] -c[%d]: VC-1.\n", current_file_index);
			} else {
				u_printf("bad codec string %s, -c should followed by 'h264' 'mpeg12' 'mpeg4' or 'vc1' .\n", argv[i + 1]);
				return (-2);
			}
			i ++;
		} else if (!strcmp("-s", argv[i])) {
			ret = sscanf(argv[i+1], "%dx%d", &file_video_width[current_file_index], &file_video_height[current_file_index]);
			if (2 != ret) {
				u_printf_error("[input argument error], '-s' should be followed by video width x height.\n");
				return (-3);
			}
			u_printf("[input argument] -s[%d]: %dx%d.\n", current_file_index, file_video_width[current_file_index], file_video_height[current_file_index]);
			i ++;
			if ((720 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((1280 == file_video_width[current_file_index]) && (720 == file_video_height[current_file_index])) {
				//hd
				is_hd[current_file_index] = 1;
			} else if ((1920 == file_video_width[current_file_index]) && (1080 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else {
				u_printf("!!!NOT supported resolution, yet.\n");
			}
		} else {
			u_printf("NOT processed option(%s).\n", argv[i]);
		}
	}
	return 0;
}

static int do_test_mdec(int iav_fd);

int main(int argc, char **argv)
{
	int state;
	int ret = 0;

	if (argc < 2) {
		usage();
		return -1;
	}

	if (!strcmp(argv[1], "--mdec")) {
		if ((ret = init_mdec_params(argc, argv)) < 0) {
			u_printf_error("--mdec: params invalid.\n");
			return -2;
		}



		u_printf("[flow]: before open_iav().\n");
		// open the device
		if (open_iav() < 0) {
			u_printf_error("open iav fail.\n");
			return -1;
		}

		u_printf("[flow]: before do_test_mdec(%d).\n", fd_iav);
		do_test_mdec(fd_iav);

		u_printf("[flow]: close(fd_iav %d).\n", fd_iav);
		// close the device
		close(fd_iav);
		u_printf("[flow]: udec --mdec exit...\n");
		return 0;
	}

	// parse parameters of 'udec'
	if (init_param(argc, argv) < 0)
		return -1;

	if (disp_bitmap) {
		show_bitmap(bitmap_filename, 1);
		return 0;
	}

	if (test_jpeg) {
		do_test_jpeg();
		return 0;
	}

	// open the device
	if (open_iav() < 0)
		return -1;

	if (test_map) {
		do_test_map();
		return 0;
	}

	if (test_giveup) {
		do_test_giveup();
		return 0;
	}

	if (test_takeover) {
		do_test_takeover();
		return 0;
	}

	// test memory
	if (test_mem) {
		printf("test_mem not implemented\n");
		return -1;
	}

	// test multi-window playback
	if (test_multi) {
		do_test_multi();
		return 0;
	}

	// if specified --idle, enter IDLE state first
	if (cmd_idle && ioctl_enter_idle() < 0)
		return -1;

	if (get_vout_info() < 0)
		return -1;

	if (ReadMediaEntries() < 0)
		return -1;

	if (set_iav_options() < 0)
		return -1;

	// if we're in IDLE, enter udec mode
	// otherwise,
	//	(1) if in UDEC mode, do things under this mode;
	//	(2) if not in UDEC mode, will fail and exit
	ioctl(fd_iav, IAV_IOC_GET_STATE, &state);
	if (state == IAV_STATE_IDLE) {
		if (ioctl_enter_udec_mode() < 0)
			return -1;
	}

	// test software decoder
	if (test_swdec) {
		do_test_swdec();
		return 0;
	}

	// test hardware udec
	if (test_udec() < 0)
		return -1;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////

typedef struct msg_s {
	int	cmd;
	void	*ptr;
	u8 arg1, arg2, arg3, arg4;
} msg_t;

#define MAX_MSGS	8

typedef struct msg_queue_s {
	msg_t	msgs[MAX_MSGS];
	int	read_index;
	int	write_index;
	int	num_msgs;
	int	num_readers;
	int	num_writers;
	pthread_mutex_t mutex;
	pthread_cond_t cond_read;
	pthread_cond_t cond_write;
	sem_t	echo_event;
} msg_queue_t;

msg_queue_t renderer_queue;
pthread_t renderer_thread_id;

int msg_queue_init(msg_queue_t *q)
{
	q->read_index = 0;
	q->write_index = 0;
	q->num_msgs = 0;
	q->num_readers = 0;
	q->num_writers = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond_read, NULL);
	pthread_cond_init(&q->cond_write, NULL);
	sem_init(&q->echo_event, 0, 0);
	return 0;
}

void msg_queue_get(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs > 0) {
			*msg = q->msgs[q->read_index];

			if (++q->read_index == MAX_MSGS)
				q->read_index = 0;

			q->num_msgs--;

			if (q->num_writers > 0) {
				q->num_writers--;
				pthread_cond_signal(&q->cond_write);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_readers++;
		pthread_cond_wait(&q->cond_read, &q->mutex);
	}
}

int msg_queue_peek(msg_queue_t *q, msg_t *msg)
{
	int ret = 0;
	pthread_mutex_lock(&q->mutex);

	if (q->num_msgs > 0) {
		*msg = q->msgs[q->read_index];

		if (++q->read_index == MAX_MSGS)
			q->read_index = 0;

		q->num_msgs--;

		if (q->num_writers > 0) {
			q->num_writers--;
			pthread_cond_signal(&q->cond_write);
		}

		ret = 1;
	}

	pthread_mutex_unlock(&q->mutex);
	return ret;
}


void msg_queue_put(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs < MAX_MSGS) {
			q->msgs[q->write_index] = *msg;

			if (++q->write_index == MAX_MSGS)
				q->write_index = 0;

			q->num_msgs++;

			if (q->num_readers > 0) {
				q->num_readers--;
				pthread_cond_signal(&q->cond_read);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_writers++;
		pthread_cond_wait(&q->cond_write, &q->mutex);
	}
}

void msg_queue_ack(msg_queue_t *q)
{
	sem_post(&q->echo_event);
}

void msg_queue_wait(msg_queue_t *q)
{
	sem_wait(&q->echo_event);
}

//for thread safe, members in this struct cannot be used as inter-thread communication.
//it's better only the udec_instance thread access this struct after it spawn.
typedef struct udec_instance_param_s
{
	unsigned int	udec_index;
	int	iav_fd;
	unsigned int	udec_type;
	int	pic_width, pic_height;
	unsigned int	request_bsb_size;
	FILE*	file_fd[2];
	unsigned char	loop;
	unsigned char	wait_cmd_begin;
	unsigned char	wait_cmd_exit;

	iav_udec_vout_config_t* p_vout_config;
	unsigned int	num_vout;

	msg_queue_t	cmd_queue;

	//pts related
	unsigned long long cur_feeding_pts;
	unsigned long long last_display_pts;
	unsigned int frame_duration;

	unsigned int frame_tick;
	unsigned int time_scale;

	unsigned char useq_buffer[UDEC_SEQ_HEADER_EX_LENGTH + 4];// + 4 for safe
	unsigned char upes_buffer[UDEC_PES_HEADER_LENGTH + 4];// + 4 for safe

	unsigned int useq_header_len;

	unsigned char seq_header_sent;
	unsigned char last_pts_from_dsp_valid;
	unsigned char paused;
	unsigned char trickplay_mode;
} udec_instance_param_t;

msg_queue_t media_thread_queue;
pthread_t media_thread_id;

#define M_MSG_KILL	0
#define M_MSG_ECHO	1
#define M_MSG_START	2
#define M_MSG_RESTART	3
#define M_MSG_PAUSE	4
#define M_MSG_RESUME	5

void *MediaThread(void *arg)
{
	u_printf("media thread run\n");
	int sum = 0;

	while (1) {
		msg_t msg;
		msg_queue_get(&media_thread_queue, &msg);

		switch (msg.cmd) {
		case M_MSG_KILL:
			u_printf("reader killed\n");
			return NULL;

		case M_MSG_ECHO:
			u_printf("stop reader\n");
			msg_queue_ack(&media_thread_queue);
			break;

		case M_MSG_START: {
				u8 *ptr = msg.ptr;
				u_printf("reader start 0x%x, 0x%x\n", (u32)ptr, (u32)mfile_ptr_end);
				while (ptr < mfile_ptr_end) {
					sum += *ptr;
					ptr += 4095;
				}
				u_printf("reader done (%d)\n", sum);
			}
			break;

		default:
			u_printf("Unknown msg received by reader: %d\n", msg.cmd);
			break;
		}
	}

	return (void*)sum;
}

void put_reader_msg(int cmd)
{
	msg_t msg;
	msg.cmd = cmd;
	msg.ptr = 0;
	msg_queue_put(&media_thread_queue, &msg);
}

int Media_ThreadInit(void)
{
	int rc;
	msg_queue_init(&media_thread_queue);
	if ((rc = pthread_create(&media_thread_id, NULL, MediaThread, NULL)) != 0) {
		u_printf("Create thread failed: %d\n", rc);
		return -1;
	}
	return 0;
}

void Media_ThreadRun(void)
{
	msg_t msg;
	msg.cmd = M_MSG_START;
	msg.ptr = mfile_ptr;
	msg_queue_put(&media_thread_queue, &msg);
}

void Media_ThreadStop(void)
{
	put_reader_msg(M_MSG_ECHO);
	msg_queue_wait(&media_thread_queue);
}

int Media_ThreadKill(void)
{
	void *status;

	put_reader_msg(M_MSG_KILL);
	pthread_join(media_thread_id, &status);

	return 0;
}

#define R_MSG_KILL	0
#define R_MSG_ECHO	1
#define R_MSG_START	2

void wakeup_udec_waiters(void)
{
	while (udec.num_waiters > 0) {
		udec.num_waiters--;
		pthread_cond_signal(&udec.cond);
	}
}

void run_renderer_ppmode_1(void)
{
	int eos_reached = 0;
	//iav_udec_status_t eos_status = {0};
	iav_frame_buffer_t frame;

	while (1) {
		memset(&frame, 0, sizeof(frame));
		frame.flags = 0;
		frame.decoder_id = udec.udec_id;

		v_printf("wait OUTPIC\n");
		if (ioctl(fd_iav, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
			if (errno != EAGAIN)
				perror("IAV_IOC_WAIT_DECODER");
			break;
		}

		if (udec.tick_after_first_outpic == -1)
			udec.tick_after_first_outpic = get_tick();

		if (frame.fb_id == IAV_INVALID_FB_ID) {
			if (frame.eos_flag) {
				// carries EOS info
				u_printf("EOS received, pts = %d, %d\n", frame.pts, frame.pts_high);
				//eos_status.pts_low = frame.pts;
				//eos_status.pts_high = frame.pts_high;
				eos_reached = 1;
			}
		} else {
			// a valid frame received
			if (norend) {
				v_printf("release frame id %d\n", frame.fb_id);
				if (ioctl(fd_iav, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
					perror("IAV_IOC_RELEASE_FRAME");
					break;
				}
			} else {
				frame.flags = sync_vout ? IAV_FRAME_SYNC_VOUT : 0;
				v_printf("render frame id %d\n", frame.fb_id);
				if (ioctl(fd_iav, IAV_IOC_RENDER_FRAME, &frame) < 0) {
					perror("IAV_IOC_RENDER_FRAME");
					break;
				}
			}
		}

		if (udec_type == UDEC_JPEG && save)
			save_jpeg(&frame);

		pthread_mutex_lock(&udec.mutex);
		udec.eos_flag = eos_reached;
		if (frame.fb_id != IAV_INVALID_FB_ID)
			udec.output_frames++;
		udec.input_cmds -= (1 + picf);
		wakeup_udec_waiters();
		pthread_mutex_unlock(&udec.mutex);
	}
}

void run_renderer_ppmode_2(void)
{
	int eos_reached = 0;
	iav_udec_status_t eos_status = {0};
	iav_udec_status_t status;

	if (ioctl_wake_vout() < 0)
		return;

	while (1) {
		status.decoder_id = udec.udec_id;
		status.nowait = 0;

		if (ioctl(fd_iav, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
			if (errno != EAGAIN)
				perror("IAV_IOC_WAIT_UDEC_STATUS");
			u_printf("renderer_ppmode_2 exits\n");
			break;
		}

		if (status.eos_flag) {
			u_printf("EOS received, pts = %d, %d\n", status.pts_low, status.pts_high);
			eos_status = status;
			eos_reached = 1;
		}

		v_printf("eos: %d, clock: %d, pts: %d,%d\n",
			status.eos_flag, status.clock_counter, status.pts_low, status.pts_high);

		pthread_mutex_lock(&udec.mutex);
		udec.eos_flag = 0;
		if (eos_reached && status.eos_flag == 0 &&
			eos_status.pts_low == status.pts_low && eos_status.pts_high == status.pts_high) {
			udec.eos_flag = 1;
			u_printf("EOS reached: pts = %d, %d\n", status.pts_low, status.pts_high);
		}
		wakeup_udec_waiters();
		pthread_mutex_unlock(&udec.mutex);
	}
}

void *renderer_thread(void *arg)
{
	u_printf("renderer is running.\n");

	while (1) {
		msg_t msg;
		msg_queue_get(&renderer_queue, &msg);

		switch (msg.cmd) {
		case R_MSG_KILL:
			u_printf("renderer killed\n");
			return NULL;

		case R_MSG_ECHO:
			u_printf("stop renderer\n");
			msg_queue_ack(&renderer_queue);
			break;

		case R_MSG_START:
			u_printf("***  ppmode=%d  ***\n", ppmode);
			if (ppmode == 1 || udec_type == UDEC_JPEG)
				run_renderer_ppmode_1();
			else if (ppmode == 2)
				run_renderer_ppmode_2();
			break;

		default:
			u_printf("Unknown msg received by renderer: %d\n", msg.cmd);
			break;
		}
	}
}

int renderer_init(void)
{
	int rc;

	if (msg_queue_init(&renderer_queue) < 0)
		return -1;

	if ((rc = pthread_create(&renderer_thread_id, NULL, renderer_thread, NULL)) != 0) {
		u_printf("Create thread failed: %d\n", rc);
		return -1;
	}

	return 0;
}

void put_renderer_msg(int cmd)
{
	msg_t msg;
	msg.cmd = cmd;
	msg.ptr = 0;
	msg_queue_put(&renderer_queue, &msg);
}

int renderer_kill(void)
{
	void *status;

	put_renderer_msg(R_MSG_KILL);
	pthread_join(renderer_thread_id, &status);

	return 0;
}

int renderer_echo(void)
{
	put_renderer_msg(R_MSG_ECHO);
	msg_queue_wait(&renderer_queue);
	return 0;
}

int renderer_start(void)
{
	put_renderer_msg(R_MSG_START);
	return 0;
}

static int ioctl_init_udec_instance(iav_udec_info_ex_t* info, iav_udec_vout_config_t* vout_config, unsigned int num_vout, int udec_index, unsigned int request_bsb_size, int udec_type, int deint, int pic_width, int pic_height)
{
	u_printf("init udec %d, type %d\n", udec_index, udec_type);

	info->udec_id = udec_index;
	info->udec_type = udec_type;
	info->enable_pp = 1;
	info->enable_deint = deint;
	info->interlaced_out = 0;
	info->packed_out = 0;

	info->vout_configs.num_vout = num_vout;
	info->vout_configs.vout_config = vout_config;

	info->vout_configs.first_pts_low = 0;
	info->vout_configs.first_pts_high = 0;

	info->vout_configs.input_center_x = pic_width / 2;
	info->vout_configs.input_center_y = pic_height / 2;

	info->bits_fifo_size = request_bsb_size;
	info->ref_cache_size = 0;

	switch (udec_type) {
	case UDEC_H264:
		info->u.h264.pjpeg_buf_size = pjpeg_buf_size;
		break;

	case UDEC_MP12:
	case UDEC_MP4H:
		info->u.mpeg.deblocking_flag = 0;
		break;

	case UDEC_JPEG:
		info->u.jpeg.still_bits_circular = 0;
		info->u.jpeg.still_max_decode_width = pic_width;
		info->u.jpeg.still_max_decode_height = pic_height;
		break;

	case UDEC_VC1:
		break;

	case UDEC_NONE:
		break;

	case UDEC_SW:
		break;

	default:
		u_printf("udec type %d not implemented\n", udec_type);
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_INIT_UDEC, info) < 0) {
		perror("IAV_IOC_INIT_UDEC");
		return -1;
	}

	return 0;
}

static unsigned char* copy_to_bsb(unsigned char *p_bsb_cur, unsigned char *buffer, unsigned int size, unsigned char* p_bsb_start, unsigned char* p_bsb_end)
{
	//u_printf("copy to bsb %d, %x.\n", size, *buffer);
	if (p_bsb_cur + size <= p_bsb_end) {
		memcpy(p_bsb_cur, buffer, size);
		return p_bsb_cur + size;
	} else {
		//u_printf("-------wrap happened--------\n");
		int room = p_bsb_end - p_bsb_cur;
		unsigned char *ptr2;
		memcpy(p_bsb_cur, buffer, room);
		ptr2 = buffer + room;
		size -= room;
		memcpy(p_bsb_start, ptr2, size);
		return p_bsb_start + size;
	}
}

static unsigned int get_next_start_code(unsigned char* pstart, unsigned char* pend, unsigned int esType)
{
	unsigned int size = 0;
	unsigned char* pcur = pstart;
	unsigned int state = 0;

	int	parse_by_amba = 0;
	unsigned char	nal_type;

	//amba dsp defined wrapper
	unsigned char code1, code2;

	//spec defined
	unsigned char code1o0 = 0xff, code2o0 = 0xff;
	unsigned char code1o1 = 0xff, code2o1 = 0xff;
	unsigned int cnt = 2;//find start code cnt

	switch (esType) {
		case 1://UDEC_H264:
			code1 = 0x7D;
			code2 = 0x7B;
			break;
		case 2://UDEC_MP12:
			code1 = 0xB3;
			code2 = 0x00;
			code1o0 = 0xB3;
			code1o1 = 0xB8;
			code2o0 = 0x0;
			code2o1 = 0x0;
			break;
		case 3://UDEC_MP4H:
			//code1 = 0xB8;
			//code2 = 0xB7;
			code1 = 0xC4;
			code2 = 0xC5;
			code1o0 = 0xB6;
			code1o1 = 0xB6;
			code2o0 = 0xB6;
			code2o1 = 0xB6;
			break;
		case 5://UDEC_VC1:
			code1 = 0x71;
			code2 = 0x72;
			code1o0 = 0x0F;
			code1o1 = 0x0E;
			code2o0 = 0x0C;
			code2o1 = 0x0D;
			break;
		default:
			u_printf("not supported es type.\n");
			return pend - pstart;
	}

	while (pcur <= pend) {
		switch (state) {
			case 0:
				if (*pcur++ == 0x0)
					state = 1;
				break;
			case 1://0
				if (*pcur++ == 0x0)
					state = 2;
				else
					state = 0;
				break;
			case 2://0 0
				if (*pcur == 0x1)
					state = 3;
				else if (*pcur != 0x0)
					state = 0;
				pcur ++;
				break;
			case 3://0 0 1
				if (*pcur == code2) { //pic header
					parse_by_amba = 1;
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					cnt --;
					state = 0;
				} else if (*pcur == code1) { //seq header
					parse_by_amba = 1;
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					state = 0;
				} else if ((!parse_by_amba) && (UDEC_H264 == esType)) { //nal uint type
					nal_type = (*pcur) & 0x1F;
					if (nal_type >= 1 && nal_type <= 5) {
						if (--cnt == 0)
							return pcur - 3 - pstart;
						else
							state = 0;
					} else if (cnt == 1 && nal_type >= 6 && nal_type <= 9) {
						// left SPS and PPS be prefix
						return pcur - 3 - pstart;
					} else if (*pstart == 0x00) {
						state = 1;
					} else {
						state = 0;
					}
				} else if ((!parse_by_amba) && (*pcur == code2o0 || *pcur == code2o1)) { //original pic header
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					cnt --;
					state = 0;
				} else if ((!parse_by_amba) && (*pcur == code1o0 || *pcur == code1o1)) { //original seq header
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					state = 0;
				} else if (*pcur){
					state = 0;
				} else {
					state = 1;
				}
				pcur ++;
				break;
		}
	}
	u_printf("cannot find start code, return 0, bit-stream end?\n");
	//size = pend - pstart;
	return 0;
}

static unsigned int total_frame_count(unsigned char * start, unsigned char * end, unsigned int udec_type)
{
	unsigned int totsize = 0;
	unsigned int size = 0;
	unsigned int frames_cnt = 0;
	unsigned char* p_cur = start;

	while (p_cur < end) {
		//u_printf("while loop 1 p_cur %p, end %p.\n", p_cur, end);
		size = get_next_start_code(p_cur, end, udec_type);
		if (!size) {
			frames_cnt ++;
			break;
		}
		totsize += size;
		p_cur = start + totsize;
		frames_cnt ++;
	}
	return frames_cnt;
}

static int request_bits_fifo(int iav_fd, unsigned int udec_index, unsigned int size, unsigned char* p_bsb_cur)
{
	iav_wait_decoder_t wait;
	int ret;
	wait.emptiness.room = size + 256;//for safe
	wait.emptiness.start_addr = p_bsb_cur;

	wait.flags = IAV_WAIT_BITS_FIFO;
	wait.decoder_id = udec_index;

	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
		u_printf("[error]: IAV_IOC_WAIT_DECODER fail, ret %d.\n", ret);
		return ret;
	}

	return 0;
}

enum {
	ACTION_NONE = 0,
	ACTION_QUIT,
	ACTION_START,
	ACTION_RESTART,
	ACTION_PAUSE,
	ACTION_RESUME,
};

enum {
	UDEC_TRICKPLAY_PAUSE = 0,
	UDEC_TRICKPLAY_RESUME = 1,
	UDEC_TRICKPLAY_STEP = 2,
};

static int process_cmd(msg_queue_t* queue, msg_t* msg)
{
	int ret = ACTION_NONE;
	switch (msg->cmd) {
		case M_MSG_KILL:
			ret = ACTION_QUIT;
			break;
		case M_MSG_ECHO:
			msg_queue_ack(queue);
			break;
		case M_MSG_START:
			ret = ACTION_START;
			break;
		case M_MSG_RESTART:
			ret = ACTION_RESTART;
			break;
		case M_MSG_RESUME:
			ret = ACTION_RESUME;
			break;
		case M_MSG_PAUSE:
			ret = ACTION_PAUSE;
			break;
		default:
			u_printf_error("Bad msg.cmd %d.\n", msg->cmd);
			break;
	}
	return ret;
}

static void _write_data_ring(FILE* file, unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start, unsigned char* p_end)
{
	if (p_end > p_start) {
		fwrite(p_start, 1, p_end - p_start, file);
	} else {
		if (p_buf_end > p_start) {
			fwrite(p_start, 1, p_buf_end - p_start, file);
		}
		if (p_end > p_buf_start) {
			fwrite(p_buf_start, 1, p_end - p_buf_start, file);
		}
	}
}

static void dump_binary_new_file(char* filename, unsigned int udec_index, unsigned int file_index, unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start, unsigned char* p_end)
{
	char* file_name = (char*)malloc(strlen((char*)filename) + 32 + 32);
	if (!file_name) {
		u_printf("malloc fail.\n");
		return;
	}
	sprintf(file_name, "%s_%d_%d", filename, udec_index, file_index);
	FILE* file = fopen((char*)file_name, "wb");

	if (file) {
		_write_data_ring(file, p_buf_start, p_buf_end, p_start, p_end);
		fclose(file);
	} else {
		u_printf("open file fail, %s.\n", file_name);
	}
	free(file_name);
}

//obsolete
#if 0
static void udec_get_last_display_pts(int iav_fd, udec_instance_param_t* udec_param, iav_udec_status_t* status)
{
	int ret;
	if (!get_pts_from_dsp) {
		return;
	}

	ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, status);
	if (ret < 0) {
		perror("IAV_IOC_WAIT_UDEC_STATUS");
		u_printf_error("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
		if ((-EPERM) == ret) {
			//to do
		} else {
			//to do
		}
		return;
	}

	if (status->only_query_current_pts) {
		udec_param->last_pts_from_dsp_valid = 1;
		udec_param->last_display_pts = ((unsigned long long)status->pts_high << 32) | (unsigned long long)status->pts_low;
	} else {
		//restore query flag
		status->only_query_current_pts = 1;
	}
}
#endif

/*
static int query_udec_last_display_pts(int iav_fd, unsigned int udec_index, unsigned long long* pts)
{
	int ret;
	iav_udec_status_t status;

	memset(&status, 0x0, sizeof(status));

	status.decoder_id = udec_index;
	status.only_query_current_pts = 1;

	ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, &status);
	if (ret < 0) {
		perror("IAV_IOC_WAIT_UDEC_STATUS");
		u_printf_error("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
		return ret;
	}

	if (status.only_query_current_pts) {
		*pts = ((unsigned long long)status.pts_high << 32) | (unsigned long long)status.pts_low;
		return 0;
	}

	return (-1);
}
*/

static int ioctl_udec_trickplay(int iav_fd, unsigned int udec_index, unsigned int trickplay_mode)
{
	iav_udec_trickplay_t trickplay;
	int ret;

	trickplay.decoder_id = udec_index;
	trickplay.mode = trickplay_mode;
	u_printf("[flow cmd]: before IAV_IOC_UDEC_TRICKPLAY, udec(%d), trickplay_mode(%d).\n", udec_index, trickplay_mode);
	ret = ioctl(iav_fd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
	if (ret < 0) {
		perror("IAV_IOC_UDEC_TRICKPLAY");
		u_printf_error("!!!!!IAV_IOC_UDEC_TRICKPLAY error, ret %d.\n", ret);
		if ((-EPERM) == ret) {
			//to do
		} else {
			//to do
		}
		return ret;
	}
	u_printf("[flow cmd]: IAV_IOC_UDEC_TRICKPLAY done, udec(%d), trickplay_mode(%d).\n", udec_index, trickplay_mode);
	return 0;
}

static void udec_trickplay(udec_instance_param_t* udec_param, unsigned int trickplay_mode)
{
	int ret;

	if ((UDEC_TRICKPLAY_PAUSE == trickplay_mode) || (UDEC_TRICKPLAY_RESUME == trickplay_mode)) {
		if (udec_param->trickplay_mode == trickplay_mode) {
			u_printf_error("[warnning]: udec(%d) is already paused/resumed(%d).\n", udec_param->udec_index, trickplay_mode);
		} else {
			ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, trickplay_mode);
			if (!ret) {
				udec_param->trickplay_mode = trickplay_mode;
			}
		}
	} else if (UDEC_TRICKPLAY_STEP == trickplay_mode) {
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, trickplay_mode);
		if (!ret) {
			udec_param->trickplay_mode = trickplay_mode;
		}
	} else {
		u_printf_error("BAD trickplay cmd for udec(%d). trickplay_mode %d.\n", udec_param->udec_index, trickplay_mode);
	}

}

static int udec_instance_decode_es_file(unsigned int udec_index, int iav_fd, FILE* file_fd, unsigned int udec_type, unsigned char* p_bsb_start, unsigned char* p_bsb_end, unsigned char* p_bsb_cur, msg_queue_t *cmd_queue, udec_instance_param_t* udec_param)
{
	int ret = 0;

	FILE* p_dump_file = NULL;
	char dump_file_name[MAX_DUMP_FILE_NAME_LEN + 64];
	unsigned int dump_separate_file_index = 0;

	unsigned char *p_frame_start;
	iav_udec_decode_t dec;

	//read es file
	unsigned char* p_es = NULL, *p_es_end;
	unsigned char* p_cur = NULL;
	unsigned int size = 0;
	unsigned int totsize = 0;
	unsigned int bytes_left_in_file = 0;
	unsigned int sendsize = 0;
	unsigned int pes_header_len = 0;

	unsigned int mem_size;
	FILE* pFile = file_fd;
	msg_t msg;

	//iav_udec_status_t status;

	if (!pFile) {
		u_printf("NULL input file.\n");
		return (-9);
	}

	if (test_dump_total) {
		snprintf(dump_file_name, MAX_DUMP_FILE_NAME_LEN + 60, test_dump_total_filename, udec_index);
		p_dump_file = fopen(dump_file_name, "wb");
	}

	fseek(pFile, 0L, SEEK_END);
	totsize = ftell(pFile);
	u_printf_index(udec_index, "	file total size %d.\n", totsize);

	if (totsize > 8*1024*1024) {
		mem_size = 8*1024*1024;
	} else {
		mem_size = totsize;
	}

	u_assert(!p_es);
	p_es = (unsigned char*)malloc(mem_size);

	if (!p_es) {
		u_printf_index(udec_index, "[error]: cannot alloc buffer.\n");
		if (p_dump_file) {
			fclose(p_dump_file);
		}
		return (-1);
	}

repeat_feeding:

	//memset(&status, 0x0, sizeof(status));
	//status.decoder_id = udec_index;
	//status.only_query_current_pts = 1;

	bytes_left_in_file = totsize;
	fseek(pFile, 0L, SEEK_SET);

	u_assert(p_es);

	if (!p_es) {
		u_printf_index(udec_index, "[error]: cannot alloc buffer.\n");
		if (p_dump_file) {
			fclose(p_dump_file);
		}
		return (-1);
	}

	size = mem_size;
	fread(p_es, 1, size, pFile);
	bytes_left_in_file -= size;

	//send data
	p_es_end = p_es + size;
	p_cur = p_es;

	while (1) {

		while (size > (DATA_PARSE_MIN_SIZE)) {

			while (msg_queue_peek(cmd_queue, &msg)) {
				ret = process_cmd(cmd_queue, &msg);

				if (ACTION_QUIT == ret) {
					u_printf("recieve quit cmd, return.\n");
					if (p_dump_file) {
						fclose(p_dump_file);
					}
					free(p_es);
					return (-5);
				} else if (ACTION_RESUME  == ret) {
					udec_param->paused = 0;
					udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
				} else if (ACTION_PAUSE  == ret) {
					udec_param->paused = 1;
					udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

					//wait resume, ugly code..
					while (udec_param->paused) {
						u_printf("thread %d paused at 1...\n", udec_index);
						msg_queue_get(cmd_queue, &msg);
						ret = process_cmd(cmd_queue, &msg);
						if (ACTION_QUIT == ret) {
							u_printf("recieve quit cmd, return.\n");
							if (p_dump_file) {
								fclose(p_dump_file);
							}
							free(p_es);
							return (-5);
						} else if (ACTION_RESUME == ret) {
							u_printf("thread %d resumed\n", udec_index);
							udec_param->paused = 0;
							udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
							break;
						} else if (ACTION_PAUSE == ret) {
							//udec_param->paused = 1;
						} else {
							u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
						}
					}

				} else {
					u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
				}

			}

			//u_printf("**send start size %d.\n", size);
			//send_frames = 0;
			//sendsize = next_es_packet_size(p_cur, p_es_end, &send_frames, udec_type);

			sendsize = get_next_start_code(p_cur, p_es_end, udec_type);
			u_printf_binary_index(udec_index, " left size %d, send size %d.\n", size, sendsize);
			if (!sendsize) {
				break;
			}

			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, sendsize + HEADER_SIZE, p_bsb_cur);

			p_frame_start = p_bsb_cur;

			//feed USEQ/UPES header
			if (add_useq_upes_header) {
				if (!udec_param->seq_header_sent) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
					udec_param->seq_header_sent = 1;
				}
				pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, sendsize, 1, 0);
				p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}

			p_bsb_cur = copy_to_bsb(p_bsb_cur, p_cur, sendsize, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = 1;

			//u_printf("decoding size %d.\n", sendsize);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			u_printf_binary_index(udec_index, "a) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			u_printf_binary_index(udec_index, "(a %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_cur[sendsize], p_cur[sendsize + 1], p_cur[sendsize + 2], p_cur[sendsize + 3], p_cur[sendsize + 4]);
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[0error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}

			p_cur += sendsize;
			size -= sendsize;

			//u_printf("**send end size %d.\n", size);
		}

		if (mdec_loop) {
			//u_printf("[flow %d]: loop, return to beginning.\n", udec_index);
			goto repeat_feeding;
		}

		if ((test_decode_one_trunk) || (!size && !bytes_left_in_file)) {
			if (test_decode_one_trunk) {
				u_printf_index(udec_index, " one shot done.\n");
			}
			//fill done
			u_printf_index(udec_index, " fill es done, 1.\n");
			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, 8, p_bsb_cur);

			p_frame_start = p_bsb_cur;
			p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = 0;

			//u_printf("decoding size %d.\n", sendsize);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			u_printf_binary_index(udec_index, "e) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[1error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}
			break;
		}

		while (msg_queue_peek(cmd_queue, &msg)) {
			ret = process_cmd(cmd_queue, &msg);

			if (ACTION_QUIT == ret) {
				u_printf("recieve quit cmd, return.\n");
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				free(p_es);
				return (-5);
			} else if (ACTION_RESUME  == ret) {
				udec_param->paused = 0;
				udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
			} else if (ACTION_PAUSE  == ret) {
				udec_param->paused = 1;
				udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

				//wait resume, ugly code..
				while (udec_param->paused) {
					u_printf("thread %d paused at 2...\n", udec_index);
					msg_queue_get(cmd_queue, &msg);
					ret = process_cmd(cmd_queue, &msg);
					if (ACTION_QUIT == ret) {
						u_printf("recieve quit cmd, return.\n");
						if (p_dump_file) {
							fclose(p_dump_file);
						}
						free(p_es);
						return (-5);
					} else if (ACTION_RESUME == ret) {
						u_printf("thread %d resumed\n", udec_index);
						udec_param->paused = 0;
						udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
						break;
					} else if (ACTION_PAUSE == ret) {
						//udec_param->paused = 1;
					} else {
						u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
					}
				}

			} else {
				u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
			}

		}

		//copy left bytes
		if (size) {
			memcpy(p_es, p_cur, size);
		}
		p_cur = p_es + size;

		if (bytes_left_in_file) {
			if ((mem_size - size) >= bytes_left_in_file) {
				fread(p_cur, 1, bytes_left_in_file, pFile);
				size += bytes_left_in_file;
				bytes_left_in_file = 0;
				p_cur = p_es;

				if (size <= (DATA_PARSE_MIN_SIZE)) {
					//last
					//udec_get_last_display_pts(iav_fd, udec_param, &status);
					ret = request_bits_fifo(iav_fd, udec_index, size + HEADER_SIZE, p_bsb_cur);
					p_frame_start = p_bsb_cur;

					//feed USEQ/UPES header
					if (add_useq_upes_header) {
						if (!udec_param->seq_header_sent) {
							p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
							udec_param->seq_header_sent = 1;
						}
						pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, size, 1, 0);
						p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

						udec_param->cur_feeding_pts += udec_param->frame_duration;
					}

					p_bsb_cur = copy_to_bsb(p_bsb_cur, p_es, size, p_bsb_start, p_bsb_end);

					//add eos
					p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

					if (test_dump_separate) {
						dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
					}

					if (test_dump_total && p_dump_file) {
						_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
					}

					memset(&dec, 0, sizeof(dec));
					dec.udec_type = udec_type;
					dec.decoder_id = udec_index;
					dec.u.fifo.start_addr = p_frame_start;
					dec.u.fifo.end_addr = p_bsb_cur;
					dec.num_pics = total_frame_count(p_es, p_es + size, udec_type);

					u_printf_binary_index(udec_index, " fill es done, 2, last size %d, frame count %d.\n", size, dec.num_pics);
					u_printf_binary_index(udec_index, "m) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
					//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
					if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
						u_printf_index(udec_index, "[3error]: ----IAV_IOC_UDEC_DECODE----");
						free(p_es);
						if (p_dump_file) {
							fclose(p_dump_file);
						}
						return (-2);
					}
					break;//done
				}
			} else {
				fread(p_cur, 1, mem_size - size, pFile);
				bytes_left_in_file -= (mem_size - size);
				size = mem_size;
				p_cur = p_es;
			}
		} else {
			//last
			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, size + HEADER_SIZE, p_bsb_cur);
			p_frame_start = p_bsb_cur;

			//feed USEQ/UPES header
			if (add_useq_upes_header) {
				if (!udec_param->seq_header_sent) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
					udec_param->seq_header_sent = 1;
				}
				pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, size, 1, 0);
				p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}

			p_bsb_cur = copy_to_bsb(p_bsb_cur, p_es, size, p_bsb_start, p_bsb_end);

			//add eos
			p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = total_frame_count(p_es, p_es + size, udec_type);

			u_printf_binary_index(udec_index, " fill es done, 3, last size %d, frame count %d, [index %d].\n", size, dec.num_pics);
			u_printf_binary_index(udec_index, "c) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[3error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}
			break;//done
		}
	}

	u_printf_index(udec_index, "[flow]: send es data done.\n");
	free(p_es);
	if (p_dump_file) {
		fclose(p_dump_file);
	}
	return 0;
}

static void* udec_instance(void* param)
{
	int ret;
	//int cur_index = 0;
	iav_udec_info_ex_t info;
	//int num_vout = 2;
	msg_t msg;
	int exit_flag = 0;

	udec_instance_param_t* udec_param = (udec_instance_param_t*)param;
	if (!param) {
		u_printf("NULL params in udec_instance.\n");
		return NULL;
	}

	//init udec instance
	memset(&info, 0x0, sizeof(info));
	ret = ioctl_init_udec_instance(&info, udec_param->p_vout_config, udec_param->num_vout, udec_param->udec_index, udec_param->request_bsb_size, udec_param->udec_type, 0, udec_param->pic_width, udec_param->pic_height);
	if (ret < 0) {
		u_printf("[error]: ioctl_init_udec_instance fail.\n");
		return NULL;
	}

	u_printf("[flow (%d)]: type(%d) begin loop.\n", udec_param->udec_index, udec_param->udec_type);
	u_printf("	[params %d]: iav_fd(%d), udec_type(%d), bsb_start(%p), loop(%d), file fd0(%p), fd1(%p).\n", udec_param->udec_index, udec_param->iav_fd, udec_param->udec_type, info.bits_fifo_start, udec_param->loop, udec_param->file_fd[0], udec_param->file_fd[1]);
	u_printf("	[params %d]: wait_cmd_begin(%d), wait_cmd_exit(%d).\n", udec_param->udec_index, udec_param->wait_cmd_begin, udec_param->wait_cmd_exit);

udec_instance_loop_begin:
	//wait begin if needed
	while (udec_param->wait_cmd_begin) {
		msg_queue_get(&udec_param->cmd_queue, &msg);
		ret = process_cmd(&udec_param->cmd_queue, &msg);
		if (ACTION_QUIT == ret) {
			exit_flag = 1;
			goto udec_instance_loop_end;
		} else if (ACTION_START == ret) {
			break;
		} else if (ACTION_RESUME == ret) {
			break;
		} else if (ACTION_PAUSE == ret) {
			//
		} else {
			u_printf_error("un-processed cmd %d, ret %d.\n", msg.cmd, ret);
		}
	}

	u_printf("[flow (%d)]: udec instance index, before udec_instance_decode_es_file.\n", udec_param->udec_index);
	ret = udec_instance_decode_es_file(udec_param->udec_index, udec_param->iav_fd, udec_param->file_fd[0], udec_param->udec_type, info.bits_fifo_start, info.bits_fifo_start + udec_param->request_bsb_size, info.bits_fifo_start, &udec_param->cmd_queue, udec_param);
	if (ret < 0) {
		//u_printf("udec_instance(%d)_decode_es_file ret %d, exit thread.\n", udec_param->udec_index, ret);
		exit_flag = 1;
	}
	u_printf("[flow (%d)]: udec instance index, after udec_instance_decode_es_file, ret %d.\n", udec_param->udec_index, ret);

udec_instance_loop_end:
	if (!exit_flag) {
		while (udec_param->wait_cmd_exit) {
			msg_queue_get(&udec_param->cmd_queue, &msg);
			ret = process_cmd(&udec_param->cmd_queue, &msg);
			if (ACTION_QUIT == ret) {
				break;
			} else if ((ACTION_RESTART == ret) || (ACTION_RESUME == ret)) {
				u_printf("[flow]: resume cmd comes\n");
				udec_param->wait_cmd_begin = 0;
				goto udec_instance_loop_begin;
			} else if (ACTION_RESUME == ret) {
				break;
			} else if (ACTION_PAUSE == ret) {
				//
			} else {
				u_printf_error("un-processed cmd %d, ret %d.\n", msg.cmd, ret);
			}
		}
	}

	//stop udec
	u_printf("[flow]: before IAV_IOC_UDEC_STOP(%d).\n", udec_param->udec_index);
	if (ioctl(udec_param->iav_fd, IAV_IOC_UDEC_STOP, udec_param->udec_index) < 0) {
		perror("IAV_IOC_UDEC_STOP");
		u_printf_error("stop udec instance(%d) fail.\n");
	}

	//release udec
	u_printf("[flow]: before IAV_IOC_RELEASE_UDEC(%d).\n", udec_param->udec_index);
	if (ioctl(udec_param->iav_fd, IAV_IOC_RELEASE_UDEC, udec_param->udec_index) < 0) {
		perror("IAV_IOC_RELEASE_UDEC");
		u_printf_error("release udec instance(%d) fail.\n");
	}

	u_printf("[flow]: udec instance(%d) exit loop.\n", udec_param->udec_index);

	return NULL;
}

static void sig_stop(int a)
{
	mdec_running = 0;
}

//todo, hard code here, fix me
#define MAX_NUM_UDEC 5

static void _get_stream_number(int* sds, int* hds, int total_cnt)
{
	int i = 0;
	int sd = 0, hd = 0;
	*sds = 0;
	*hds = 0;
	for (i = 0; i < total_cnt; i++) {
		if (is_hd[i]) {
			hd ++;
		} else {
			sd ++;
		}
	}

	u_printf("found hd stream number %d, sd stream number %d.\n", hd, sd);
	*sds = sd;
	*hds = hd;
}

static void _print_udec_info(int iav_fd, u8 udec_index)
{
	int ret;
	iav_udec_state_t state;

	memset(&state, 0, sizeof(state));
	state.decoder_id = udec_index;
	state.flags = IAV_UDEC_STATE_DSP_READ_POINTER | IAV_UDEC_STATE_BTIS_FIFO_ROOM | IAV_UDEC_STATE_ARM_WRITE_POINTER;

	ret = ioctl(iav_fd, IAV_IOC_GET_UDEC_STATE, &state);
	if (ret) {
		perror("IAV_IOC_GET_UDEC_STATE");
		u_printf("IAV_IOC_GET_UDEC_STATE %d fail.\n", udec_index);
		return;
	}

	u_printf("print udec(%d) info:\n", udec_index);
	u_printf("     udec_state(%d), vout_state(%d), error_code(0x%08x):\n", state.udec_state, state.vout_state, state.error_code);
	u_printf("     bsb info: phys start addr 0x%08x, total size %u(0x%08x), free space %u:\n", state.bits_fifo_phys_start, state.bits_fifo_total_size, state.bits_fifo_total_size, state.bits_fifo_free_size);

	u_printf("     dsp read pointer from msg: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.dsp_current_read_bitsfifo_addr_phys, state.dsp_current_read_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.dsp_current_read_bitsfifo_addr);
	u_printf("     last arm write pointer: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.arm_last_write_bitsfifo_addr_phys, state.arm_last_write_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.arm_last_write_bitsfifo_addr);

	u_printf("     tot send decode cmd %d, tot frame count %d.\n", state.tot_decode_cmd_cnt, state.tot_decode_frame_cnt);

	return;
}

static int ioctl_stream_switch_cmd(int iav_fd, int render_id, int new_udec_index)
{
	int ret;
	iav_postp_stream_switch_t stream_switch;
	memset(&stream_switch, 0x0, sizeof(stream_switch));

	stream_switch.num_config = 1;
	stream_switch.switch_config[0].render_id = render_id;
	stream_switch.switch_config[0].new_udec_id = new_udec_index;

	u_printf("[cmd flow]: before IAV_IOC_POSTP_STREAM_SWITCH, render_id %d, new_udec_index %d.\n", render_id, new_udec_index);
	if ((ret = ioctl(iav_fd, IAV_IOC_POSTP_STREAM_SWITCH, &stream_switch)) < 0) {
		perror("IAV_IOC_POSTP_STREAM_SWITCH");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_POSTP_STREAM_SWITCH.\n");

	return 0;
}

static int ioctl_wait_stream_switch_cmd(int iav_fd, int render_id)
{
	int ret;
	iav_wait_stream_switch_msg_t wait_stream_switch;
	memset(&wait_stream_switch, 0x0, sizeof(wait_stream_switch));

	wait_stream_switch.render_id = render_id;
	wait_stream_switch.wait_flags = 0;//blocked

	u_printf("[cmd flow]: before IAV_IOC_WAIT_STREAM_SWITCH_MSG, render_id %d.\n", render_id);
	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_STREAM_SWITCH_MSG, &wait_stream_switch)) < 0) {
		perror("IAV_IOC_WAIT_STREAM_SWITCH_MSG");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_WAIT_STREAM_SWITCH_MSG, status %d.\n", wait_stream_switch.switch_status);

	return 0;
}

static int ioctl_render_cmd(int iav_fd, int number_of_render, int number_of_window, udec_render_t* p_render)
{
	int ret;
	iav_postp_render_config_t render;

	if (number_of_render <= 0 || number_of_window<= 0) {
		u_printf_error("internal error, inavlid number_of_render %d, number_of_window %d.\n", number_of_render, number_of_window);
		return -2;
	}

	memset(&render, 0x0, sizeof(render));

	render.total_num_windows_to_render = number_of_window;
	render.num_configs = number_of_render;
	u_assert(number_of_render < 6);
	if (number_of_render < 6) {
		for (ret = 0; ret < number_of_render; ret ++) {
			render.configs[ret] = p_render[ret];
			u_printf("    [render %d]: win %d, win_2rd %d, udec_id %d.\n", render.configs[ret].render_id, render.configs[ret].win_config_id, render.configs[ret].win_config_id_2nd, render.configs[ret].udec_id);
		}
	}

	u_printf("[cmd flow]: before IAV_IOC_POSTP_RENDER_CONFIG.\n");
	if ((ret = ioctl(iav_fd, IAV_IOC_POSTP_RENDER_CONFIG, &render)) < 0) {
		perror("IAV_IOC_POSTP_RENDER_CONFIG");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_POSTP_RENDER_CONFIG.\n");

	return 0;
}

static void resume_pause_feeding(udec_instance_param_t* params, int thread_start_index, int thread_end_index, int total_feeding_thread_number, int pause)
{
	int i = 0;
	msg_t msg;

	if ((thread_start_index >= total_feeding_thread_number) || (thread_start_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_end_index >= total_feeding_thread_number) || (thread_end_index < 0)) {
		u_assert(0);
		return;
	}

	if (pause) {
		msg.cmd = M_MSG_PAUSE;
	} else {
		msg.cmd = M_MSG_RESUME;
	}
	msg.ptr = NULL;

	for (i = thread_start_index; i <= thread_end_index; i++) {
		u_printf("[cmd]: pause/resume(%d) udec(%d), msg.cmd %d.\n", pause, i, msg.cmd);
		msg_queue_put(&params[i].cmd_queue, &msg);
	}
}

//simulate real time streaming's case, adjust pts
#if 0
static void adjust_start_pts(udec_instance_param_t* params, int thread_from_index, int thread_to_index, int total_feeding_thread_number)
{
	unsigned long long to_pts;
	if ((thread_from_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_to_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}

	if (!params[thread_to_index].last_pts_from_dsp_valid) {
		u_printf("[warnning]: udec(%d)'s pts from dsp is not avaiable, use feeding pts instead.\n", thread_to_index);
		to_pts = params[thread_to_index].cur_feeding_pts;
	} else {
		to_pts = params[thread_to_index].last_display_pts;
	}
	u_printf("try to adjust pts from %llu to %llu, align %d to %d.\n", params[thread_from_index].cur_feeding_pts, to_pts, thread_from_index, thread_to_index);
	params[thread_from_index].cur_feeding_pts = to_pts;
}
#endif

/*
static void adjust_start_pts_ex(udec_instance_param_t* params, int thread_from_index, int thread_to_index, int total_feeding_thread_number)
{
	int ret;
	unsigned long long to_pts;
	if ((thread_from_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_to_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}

	ret = query_udec_last_display_pts(params->iav_fd, thread_to_index, &to_pts);
	if (0 != ret) {
		u_printf("[warnning]: there's no pts from dsp, this stream does not start feeding? use feeding pts instead.\n");
		to_pts = params[thread_to_index].cur_feeding_pts;
	}
	u_printf("try to adjust pts from %llu to %llu, align %d to %d.\n", params[thread_from_index].cur_feeding_pts, to_pts, thread_from_index, thread_to_index);
	params[thread_from_index].cur_feeding_pts = to_pts;
}
*/

static int do_test_mdec(int iav_fd)
{
	unsigned int i;
	pthread_t thread_id[MAX_NUM_UDEC];
	udec_instance_param_t params[MAX_NUM_UDEC];
	iav_udec_vout_config_t vout_cfg[NUM_VOUT];
	int vout_start_index = -1;
	int num_of_vout = 0;
	msg_t msg;
	//unsigned int tot_number_udec;
	int sds, hds;
	int total_num;

	char buffer_old[128] = {0};
	char buffer[128];
	char* p_buffer = buffer;
	int flag_stdin = 0;

	int cur_display = 0;// 0: 4xwindow, 1: 1xwindow

	signal(SIGINT, sig_stop);
	signal(SIGQUIT, sig_stop);
	signal(SIGTERM, sig_stop);

	dec_types = 0x17;	// no RV40, no hybrid MPEG4
	max_frm_num = 16;
	bits_fifo_size = 2*MB;
	ref_cache_size = 0;
	bits_fifo_prefill_size = 1*MB;
	bits_fifo_emptiness = 1*MB;
	pjpeg_buf_size = 4*MB;

	ppmode = 3;
	//tot_number_udec = mdec_num_of_udec[0] + mdec_num_of_udec[1];

	//if (tot_number_udec == 0 || tot_number_udec > MAX_NUM_UDEC) {
	//	u_printf_error(" bad number %d\n", tot_number_udec);
	//	return -1;
	//}

	//enter mdec mode config
	iav_mdec_mode_config_t mdec_mode;
	iav_udec_mode_config_t *udec_mode = &mdec_mode.super;
	iav_udec_config_t *udec_configs = NULL;
	udec_window_t *windows = NULL;
	udec_render_t *renders = NULL, *tmp_renders = NULL;

	total_num = current_file_index + 1;
	_get_stream_number(&sds, &hds, total_num);

	if ((1 == total_num) || (0 == sds)) {
		if (!first_show_full_screen) {
			first_show_full_screen = 1;
			u_printf("[change settings]: first use full screen show one stream playback.\n");
		}
	}

	if (0 == sds) {
		if (!first_show_hd_stream) {
			first_show_hd_stream = 1;
			u_printf("[change settings]: first show hd stream.\n");
		}
	}

	u_printf("[flow]: before malloc some memory, current_file_index %d, total_num, sds %d, hds %d, first_show_full_screen %d, first_show_hd_stream %d\n", current_file_index, total_num, sds, hds, first_show_full_screen, first_show_hd_stream);

	if (sds > 4 && 0 == hds) {
		u_printf("[debug]: make 5'th stream as hd, for debug.\n");
		hds = 1;
		sds = 4;
	}

	if ((udec_configs = malloc(total_num * sizeof(iav_udec_config_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto do_test_mdec_exit;
	}

	if ((windows = malloc(total_num * sizeof(udec_window_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto do_test_mdec_exit;
	}

	if ((renders = malloc(total_num * sizeof(udec_render_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto do_test_mdec_exit;
	}

	u_printf("[flow]: before get vout info\n");
	//get all vout info
	for (i = 0; i < NUM_VOUT; i++) {
		if (get_single_vout_info(i, vout_width + i, vout_height + i) < 0) {
			u_printf_error(" get vout(%d) info fail\n", i);
			continue;
		}
		if (max_vout_width < vout_width[i])
			max_vout_width = vout_width[i];
		if (max_vout_height < vout_height[i])
			max_vout_height = vout_height[i];
		if (vout_start_index < 0) {
			vout_start_index = i;
		}
		num_of_vout ++;
	}

	for (i = vout_start_index; i < (num_of_vout + vout_start_index); i++) {
		vout_cfg[i].disable = 0;
		vout_cfg[i].udec_id = 0;//hard code, fix me
		vout_cfg[i].flip = 0;
		vout_cfg[i].rotate = 0;
		vout_cfg[i].vout_id = i;
		vout_cfg[i].win_offset_x = vout_cfg[i].target_win_offset_x = 0;
		vout_cfg[i].win_offset_y = vout_cfg[i].target_win_offset_y = 0;
		vout_cfg[i].win_width = vout_cfg[i].target_win_width = vout_width[i];
		vout_cfg[i].win_height = vout_cfg[i].target_win_height = vout_height[i];
		vout_cfg[i].zoom_factor_x = vout_cfg[i].zoom_factor_y = 1;
	}

	u_printf("[flow]: before init_udec_mode_config\n");
	init_udec_mode_config(udec_mode);
	udec_mode->num_udecs = total_num;
	udec_mode->udec_config = udec_configs;

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.windows_config = windows;
	//mdec_mode.render_config = renders;

	mdec_mode.av_sync_enabled = 0;
	//mdec_mode.out_to_hdmi = 1;
	mdec_mode.audio_on_win_id = 0;

	if (sds) {
		u_printf("[flow]: before init_udec_configs, sd\n");
		init_udec_configs(udec_configs, sds, file_video_width[0], file_video_height[0]);
	}

	if (hds) {
		u_printf("[flow]: before init_udec_configs, hd\n");
		init_udec_configs(udec_configs + sds, hds, file_video_width[sds], file_video_height[sds]);
	}

	if (sds) {
		u_printf("[flow]: before init_udec_windows, sd\n");
		init_udec_windows(windows, 0, sds, file_video_width[0], file_video_height[0]);
	}

	if (hds) {
		u_printf("[flow]: before init_udec_windows, hd\n");
		init_udec_windows(windows, sds, hds, file_video_width[sds + 0], file_video_height[sds + 0]);
	}

	if (sds) {
		u_printf("[flow]: before init_udec_renders\n");
		init_udec_renders(renders, sds);
	}

	if (hds) {
		//for hd
		tmp_renders = renders + sds;
		for (i = 0; i < hds; i++, tmp_renders ++) {
			u_printf("[flow]: before init_udec_renders_single for hd\n");
			init_udec_renders_single(tmp_renders, i + sds, 0xff, 0xff, i + sds);
		}
	}

	//config renders, which will display
	if (!first_show_hd_stream) {
		mdec_mode.total_num_render_configs = sds;
		mdec_mode.render_config = renders;
	} else {
		//first hd stream, hard code here

		tmp_renders = renders + sds;
		init_udec_renders_single(tmp_renders, 0, 0 + sds, 0xff, 0 + sds);

		mdec_mode.total_num_render_configs = 1;
		mdec_mode.render_config = tmp_renders;
	}

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.max_num_windows = total_num + 1;

	u_printf("[flow]: before IAV_IOC_ENTER_MDEC_MODE, num render configs %d, num win configs %d, max num windows %d\n", mdec_mode.total_num_render_configs, mdec_mode.total_num_win_configs, mdec_mode.max_num_windows);
	if (ioctl(fd_iav, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode) < 0) {
		perror("IAV_IOC_ENTER_MDEC_MODE");
		u_printf_error(" enter mdec mode fail\n");
		goto do_test_mdec_exit;
	}

	u_printf("[flow]: enter mdec mode done\n");

	i = 0;
	//each instance's parameters, sd
	for (i = 0; i < sds; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = 2*1024*1024;
		params[i].udec_type = file_codec[i];
		params[i].pic_width = file_video_width[i];
		params[i].pic_height= file_video_height[i];
		msg_queue_init(&params[i].cmd_queue);

		if (!test_feed_background) {
			if (!first_show_hd_stream) {
				params[i].wait_cmd_begin = 0;
			} else {
				params[i].wait_cmd_begin = 1;
			}
		} else {
			params[i].wait_cmd_begin = 0;
		}

		//debug use
		if (params[i].wait_cmd_begin) {
			feeding_sds = 0;
		} else {
			feeding_sds = 1;
		}

		params[i].wait_cmd_exit = 1;

		params[i].num_vout = num_of_vout;
		params[i].p_vout_config = &vout_cfg[vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");
		params[i].file_fd[1] = NULL;
	}

	//each instance's parameters, sd
	for (; i < sds + hds; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = 2*1024*1024;
		params[i].udec_type = file_codec[i];
		params[i].pic_width = file_video_width[i];
		params[i].pic_height= file_video_height[i];
		msg_queue_init(&params[i].cmd_queue);

		if (!test_feed_background) {
			if (first_show_hd_stream) {
				params[i].wait_cmd_begin = 0;
			} else {
				params[i].wait_cmd_begin = 1;
			}
		} else {
			params[i].wait_cmd_begin = 0;
		}

		//debug use
		if (params[i].wait_cmd_begin) {
			feeding_hds = 0;
		} else {
			feeding_hds = 1;
		}

		params[i].wait_cmd_exit = 1;

		params[i].num_vout = num_of_vout;
		params[i].p_vout_config = &vout_cfg[vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");
		params[i].file_fd[1] = NULL;
	}

	//each instance's USEQ, UPES header
	for (i = 0; i < sds + hds; i++) {
		params[i].cur_feeding_pts = 0;

		//hard code to 29.97 fps
		params[i].frame_tick = 3003;
		params[i].time_scale = 90000;

		params[i].frame_duration = ((u64)90000)*((u64)params[i].frame_tick)/params[i].time_scale;
		u_assert(3003 == params[i].frame_duration);//debug assertion

		//hard code to h264
		//init USEQ/UPES header
		params[i].useq_header_len = fill_useq_header(params[i].useq_buffer, UDEC_H264, params[i].time_scale, params[i].frame_tick, 0, 0, 0);
		u_assert(UDEC_SEQ_HEADER_LENGTH == params[i].useq_header_len);

		init_upes_header(params[i].upes_buffer, UDEC_H264);

		params[i].seq_header_sent = 0;
		params[i].last_pts_from_dsp_valid = 0;
		params[i].paused = 0;
		params[i].trickplay_mode = UDEC_TRICKPLAY_RESUME;
	}


	//spawn all threads
	for (i = 0; i < total_num; i++) {
		pthread_create(&thread_id[i], NULL, udec_instance, &params[i]);
	}

	flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
	if(fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
		u_printf("[error]: stdin_fileno set error.\n");
	}

	memset(buffer, 0x0, sizeof(buffer));
	memset(buffer_old, 0x0, sizeof(buffer_old));

	int switch_render_id;
	int switch_new_udec_id;
	int switch_auto_update_display;

	int input_total_renders;
//	int input_render_id;
	int input_win_id;
	int input_win2_id;
	int input_udec_id;
	char* p_input_tmp;

	//main cmd loop
	while (mdec_running) {
		//add sleep to avoid affecting the performance
		usleep(100000);
		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
			continue;

		if (buffer[0] == '\n') {
			p_buffer = buffer_old;
			u_printf("repeat last cmd:\n\t\t%s\n", buffer_old);
		} else if (buffer[0] == 'l') {
			u_printf("show last cmd:\n\t\t%s\n", buffer_old);
			continue;
		} else {
			p_buffer = buffer;

			//record last cmd
			strncpy(buffer_old, buffer, sizeof(buffer_old) -1);
			buffer_old[sizeof(buffer_old) -1] = 0x0;
		}

		switch (p_buffer[0]) {
			case 'q':	// exit
				u_printf("Quit\n");
				mdec_running = 0;
				break;

			case 's':	//switch mode0 and mode1
				//todo
				break;

			case 'p':
				if ('a' == p_buffer[1]) {
					//print all udec instance
					for (i = 0; i < total_num; i ++) {
						_print_udec_info(iav_fd, (u8)i);
					}
				} else if (1 == sscanf(p_buffer, "p%d", &i)) {
					//print specified udec instance
					if (i >=0 && i < total_num) {
						_print_udec_info(iav_fd, (u8)i);
					} else {
						u_printf("BAD udec index %d.\n", i);
					}
				} else {
					u_printf("not support cmd %s.\n", p_buffer);
					u_printf("   type 'pa' to print all udec instances.\n");
					u_printf("   type 'p\%d' to print specified udec instance.\n");
				}
				break;

			case 'c':
				if ('s' == p_buffer[1]) {

					switch_auto_update_display = 0;
					if ('a' == p_buffer[2]) {
						switch_auto_update_display = 1;
						if (2 != sscanf(p_buffer, "csa%dto%d", &switch_render_id, &switch_new_udec_id)) {
							u_printf("BAD cmd format, you should type 'csa\%dto\%d' and enter, to do stream switch render_id, new_udec_id, (auto update display after switch is finished).\n");
							break;
						}
					} else {
						if (2 != sscanf(p_buffer, "cs%dto%d", &switch_render_id, &switch_new_udec_id)) {
							u_printf("BAD cmd format, you should type 'cs\%dto\%d' and enter, to do stream switch render_id, new_udec_id, (do not auto update display).\n");
							break;
						}
					}

					//stream switch
					u_assert(switch_render_id < 4);
					u_assert(switch_new_udec_id < (hds + sds));
					if ((switch_render_id >= 4) || (switch_new_udec_id >= (hds + sds))) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if ((switch_render_id < 0) || (switch_new_udec_id < 0)) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if (renders[switch_render_id].udec_id != switch_new_udec_id) {
						if (0) {
							//adjust_start_pts_ex(params, switch_new_udec_id, renders[switch_render_id].udec_id, sds + hds);
						}

						if (!test_feed_background) {
							//resume feeding
							//u_printf("[flow]: resume feeding for thread %d.\n", switch_new_udec_id);
							resume_pause_feeding(params, switch_new_udec_id, switch_new_udec_id, sds + hds, 0);
						}

						//send switch cmd
						ioctl_stream_switch_cmd(iav_fd, switch_render_id, switch_new_udec_id);

						//p_input_tmp = strchr(p_buffer, ':');
						//if (p_input_tmp && ('w' == p_input_tmp[1])) {
							u_printf("[flow]: request wait switch done.\n");
							ioctl_wait_stream_switch_cmd(iav_fd, switch_render_id);

							if (!test_feed_background) {
								//pause feeding
								//u_printf("[flow]: pause feeding for thread %d.\n", renders[switch_render_id].udec_id);
								resume_pause_feeding(params, renders[switch_render_id].udec_id, renders[switch_render_id].udec_id, sds + hds, 1);
							}

						//}
						renders[switch_render_id].udec_id = switch_new_udec_id;

						//update display if needed
						if (switch_auto_update_display) {
							if ((0 == cur_display) && (switch_new_udec_id >= sds)) {
								u_printf("[flow]: auto update display to 1 x window.\n");
								//switch to one window display
								for (i = sds; i < sds + hds; i ++) {
									renders[i].render_id = i - sds;
									renders[i].win_config_id = i;
									renders[i].win_config_id_2nd = 0xff;//hard code here
									renders[i].udec_id = i;
								}
								//#endif
								ioctl_render_cmd(iav_fd, hds, hds, renders + sds);
								cur_display = 1;

								if (!test_feed_background) {
									//pause sds
									resume_pause_feeding(params, 0, sds - 1, sds + hds, 1);
								}

							} else if ((1 == cur_display) && (switch_new_udec_id < sds)) {
								u_printf("[flow]: auto update display to 4 x window.\n");

								if (!test_feed_background) {
									//resume if needed
									resume_pause_feeding(params, 0, sds - 1, sds + hds, 0);
								}

								ioctl_render_cmd(iav_fd, sds, sds, renders);
								cur_display = 0;
								if (!test_feed_background) {
									//pause hds
									resume_pause_feeding(params, sds, sds + hds - 1, sds + hds, 1);
								}
							}
						}
					} else {
						u_printf("udec_id not changed, do not send switch cmd\n");
					}

				} else if ('w' == p_buffer[1]) {
					//wait
					if (1 == sscanf(p_buffer, "cw%d", &switch_render_id)) {
						ioctl_wait_stream_switch_cmd(iav_fd, switch_render_id);
					}
				} else if ('r' == p_buffer[1]) {
					//render, total number of  renders first
					if ('s' == p_buffer[2] && 'd' == p_buffer[3]) {
						// restore to display 4x sd's case
						#if 0
						for (i = 0; i < sds; i ++) {
							renders[i].render_id = i;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;
						}
						#endif
						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, 0, sds - 1, sds + hds, 0);
						}
						ioctl_render_cmd(iav_fd, sds, sds, renders);
						if (!test_feed_background) {
							//pause hds
							resume_pause_feeding(params, sds, sds + hds - 1, sds + hds, 1);
						}

						cur_display = 0;
					} else if ('h' == p_buffer[2] && 'd' == p_buffer[3]) {
						// restore display 1x hd's case
						//#if 0
						for (i = sds; i < sds + hds; i ++) {
							renders[i].render_id = i - sds;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;
						}
						//#endif

						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, sds, sds + hds -1, sds + hds, 0);
						}

						ioctl_render_cmd(iav_fd, hds, hds, renders + sds);
						if (!test_feed_background) {
							//pause sds
							resume_pause_feeding(params, 0, sds - 1, sds + hds, 1);
						}

						cur_display = 1;
					} else if (1 == sscanf(p_buffer, "cr %d:", &input_total_renders)) {
						u_printf("[ipnut cmd]: update render, total render count %d.\n", input_total_renders);
						p_input_tmp = strchr(p_buffer, ':');
						u_assert(p_input_tmp);
						u_assert(input_total_renders < 6);
						if (input_total_renders >= 6) {
							u_printf("[input cmd]: bad parameters, number of renders should less than 6.\n");
							break;
						}
						i = 0;
						while ((i < input_total_renders) && p_input_tmp) {
							sscanf(p_input_tmp, ":%x %x %x", &input_win_id, &input_win2_id, &input_udec_id);
							u_printf("[ipnut cmd]: render id %d, win id %d, win2 id %d, udec_id %d.\n", i, input_win_id, input_win2_id, input_udec_id);
							p_input_tmp = strchr(p_input_tmp + 1, ':');

							//update render_t
							renders[i].render_id = i;
							renders[i].win_config_id = input_win_id;
							renders[i].win_config_id_2nd = input_win2_id;
							renders[i].udec_id = input_udec_id;
							i ++;
						}
						ioctl_render_cmd(iav_fd, i, i, renders);
					} else {
						u_printf("\t[not supported cmd]: for update render for NVR, please use 'crsd' to show 4xsd, 'crhd' to show 1xhd.'\n");
						u_printf("\t                              or use 'cr render_number:win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id...'\n");
					}
				} else {
					//render
				}

				break;

			default:
				break;
		}

	}

	if(fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
		u_printf("[error]: stdin_fileno set error");
	}

	//exit each threads
	msg.cmd = M_MSG_KILL;
	msg.ptr = NULL;
	for (i = 0; i < total_num; i++) {
		msg_queue_put(&params[i].cmd_queue, &msg);
	}

	void* pv;
	int ret = 0;
	for (i = 0; i < total_num; i++) {
		u_printf("[flow]: wait udec_instance_thread(%d) exit...\n", i);
		ret = pthread_join(thread_id[i], &pv);
		u_printf("[flow]: wait udec_instance_thread(%d) exit done(ret %d).\n", i, ret);
	}

	//exit UDEC mode
	u_printf("[flow]: before enter idle\n");
	ioctl_enter_idle();
	u_printf("[flow]: enter idle done\n");

	//close opened file
	for (i = 0; i < total_num; i++) {
		if (params[i].file_fd[0]) {
			fclose(params[i].file_fd[0]);
			params[i].file_fd[0] = NULL;
		}

		if (params[i].file_fd[1]) {
			fclose(params[i].file_fd[1]);
			params[i].file_fd[1] = NULL;
		}
	}

	free(udec_configs);
	free(windows);
	free(renders);
	return 0;

do_test_mdec_exit:

	if (udec_configs) {
		free(udec_configs);
	}

	if (windows) {
		free(windows);
	}

	if (renders) {
		free(renders);
	}

	return -1;
}

