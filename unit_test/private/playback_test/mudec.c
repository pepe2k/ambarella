
/*
 * mudec.c
 *
 * History:
 *	2012/06/21 - [Zhi He] created file
 *
 * Description:
 *    This file is delicated for NVR(M UDEC) H264's test case
 * Copyright (C) 2012, Ambarella, Inc.
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
#include <dirent.h>
#include <sys/stat.h>

#include <pthread.h>
#include <semaphore.h>

#include <linux/fb.h>
#include <linux/input.h>

#include "types.h"
#include "ambas_common.h"
#include "ambas_vout.h"
#include "iav_drv.h"

#include "udecutil.h"

#ifndef KB
#define KB	(1*1024)
#endif

#ifndef MB
#define MB	(1*1024*1024)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)	(sizeof(_array)/sizeof(_array[0]))
#endif

static int vout_id[] = {0, 1};
static int vout_type[] = {AMBA_VOUT_SINK_TYPE_DIGITAL, AMBA_VOUT_SINK_TYPE_HDMI};
//static int vout_index = 1;

//keyboard input
#define KEY_INSTANCE	500*1000		//500ms

struct key_info {
	int	input_key;
	char	name[32];
};

#if 0
static int getch (void){
	int ch;
	struct termios oldt, newt;// get terminal input's attribute
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;	//set termios' local mode
	newt.c_lflag &= ~(ECHO|ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);	//read character from terminal input
	ch = getchar();	//recover terminal's attribute
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}
#endif

#if 0
struct key_info _AMBARELLA_INPUT_KEY_INFO[] = {
	{ KEY_BATTERY,	"KEY_BATTERY_COVER_DECTION"},
	{ KEY_UP,	"KEY_UP"},
	{ KEY_DOWN,	"KEY_DOWN"},
	{ KEY_LEFT,	"KEY_LEFT"},
	{ KEY_RIGHT,	"KEY_RIGHT"},
	{ KEY_OK,	"KEY_OK"},
	{ KEY_MENU,	"KEY_MENU"},
	{ KEY_5,	"KEY_LCD"},
	{ KEY_DELETE,	"KEY_DELETE"},
	{ KEY_A,	"KEY_RESERVED1"},
	{ KEY_0,	"KEY_RESERVED2"},
	{ KEY_9,	"BTN_VIDEO_CAPTURE"},
	{ KEY_8,	"KEY_RESERVED3"},
	{ KEY_PLAY,	"KEY_PLAYBACK"},
	{ KEY_RECORD,	"KEY_RECORD"},
	{ KEY_7,	"KEY_RESERVED4"},
	{ KEY_PHONE,	"ADC_KEY_TELE"},
	{ KEY_6,	"ADC_KEY_WIDE"},
	{ KEY_RESERVED,	"ADC_KEY_BASE"},

	{ KEY_1,	"Start/Stop"},
	{ KEY_2,	"Photo"},
	{ KEY_3,	"Display"},
	{ KEY_4,	"Self timer"},
	{ KEY_ZOOMIN,	"Zoom wide"},
	{ KEY_ZOOMOUT,	"Zoom tele"},
	{ KEY_PREVIOUS,	"Prev section"},
	{ KEY_NEXT,	"Next section"},
	{ KEY_BACK,	"Fast backward"},
	{ KEY_FORWARD,	"Fast forward"},
	{ KEY_STOP,	"Stop"},
	{ KEY_PLAYPAUSE,"Play/Pause"},
	{ KEY_SLOW,	"Slow forward"},
	{ KEY_MENU,	"Menu"},
	{ KEY_NEWS,	"Q-Menu"},

	{ KEY_ESC,	"GPIOKEY_ESC"},
	{ KEY_POWER,	"GPIOKEY_POWER"},

	{ KEY_N,	"ADC_KEY_N"},
	{ KEY_P,	"ADC_KEY_P"},
	{ KEY_C,	"ADC_KEY_C"},
	{ KEY_S,	"ADC_KEY_S"},
};
#endif

struct key_info _AMBARELLA_INPUT_KEY_INFO[] = {
	{ KEY_1,	"KEY_0"},
	{ KEY_1,	"KEY_1"},
	{ KEY_2,	"KEY_2"},
	{ KEY_3,	"KEY_3"},
	{ KEY_4,	"KEY_4"},

	{ KEY_N,	"KEY_N"},
	{ KEY_P,	"KEY_P"},
	{ KEY_C,	"KEY_C"},
	{ KEY_S,	"KEY_S"},
	{ KEY_Q,	"KEY_Q"},
};

#define KEY_ARRAY_SIZE	(sizeof(_AMBARELLA_INPUT_KEY_INFO) / sizeof((_AMBARELLA_INPUT_KEY_INFO)[0]))

struct keypadctrl {
	pthread_t thread;
	int running;
	int fd[4];
};

static struct keypadctrl _G_keypadctrl;

static int key_event_channel = 4;

#if 0
static struct input_event _prev_key =
	{
		.time.tv_sec=0,
		.time.tv_usec=0,
		.value = 0,
		.code = 0,
	};
#endif

//default is HDMI
static unsigned int mudec_request_voutmask = 2;
static unsigned int mudec_actual_voutmask = 2;
static unsigned int mudec_actual_vout_start_index = 1;

#define NUM_VOUT	ARRAY_SIZE(vout_id)

static int vout_width[NUM_VOUT] = {1280};
static int vout_height[NUM_VOUT] = {720};
static int vout_rotate[NUM_VOUT] = {1, 0};

static int max_vout_width;
static int max_vout_height;

//static int udec_id = 0;

int mdec_pindex = 0;

//#define DATA_PARSE_MIN_SIZE 128*1024
#define DATA_PARSE_MIN_SIZE 0
#define MAX_DUMP_FILE_NAME_LEN 256
static unsigned char _h264_eos[8] = {0x00, 0x00, 0x00, 0x01, 0x0A, 0x0, 0x0, 0x0};
static unsigned char _h264_delimiter[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x30};

#define HEADER_SIZE 1024

typedef struct _s_file_reader {
	FILE* fd;

	unsigned long read_buffer_size;
	unsigned long file_total_size;
	unsigned long file_remainning_size;
	unsigned long data_remainning_size_in_buffer;

	unsigned char* p_read_buffer_base;
	unsigned char* p_read_buffer_end;

	unsigned char* p_read_buffer_cur_start;
	unsigned char* p_read_buffer_cur_end;

	unsigned char b_alloc_read_buffer;
	unsigned char b_opened_file;
	unsigned char reserved1;
	unsigned char reserved2;
} _t_file_reader;

#define MAX_FILES	64

typedef char FileName[256];

//some debug options
static int test_decode_one_trunk = 0;
static int test_dump_total = 0;
static int test_dump_separate = 0;
static char test_dump_total_filename[MAX_DUMP_FILE_NAME_LEN] = "./tot_dump/es_%08d";
static char test_dump_separate_filename[MAX_DUMP_FILE_NAME_LEN] = "./sep_dump/es_%08d_%08d";
static int test_feed_background = 0;
static int enable_error_handling = 0;
static int feeding_sds = 1;
static int feeding_hds = 0;
static int use_udec_mode = 0;//ppmode = 2
static int no_upscaling = 0;
static int scan_dir = 0;
static int auto_vout = 0;
FileName dirname = {0};

static unsigned int capture_quality = 50;

//debug only
static int get_pts_from_dsp = 1;

FileName file_list[MAX_FILES];
static int current_file_index = 0;
static int total_file_number = 0;
static int shift_file_offset = 0;
static unsigned int file_codec[MAX_FILES];
static int file_video_width[MAX_FILES];
static int file_video_height[MAX_FILES];
static int file_framerate[MAX_FILES] = {0};
static int prevous_HDMI_Mode = 0;
static int file_prefer_HDMI_Mode[MAX_FILES] = {0};
static int file_prefer_HDMI_FPS[MAX_FILES] = {0};
static int is_hd[MAX_FILES];

FileName pre_sorted_file_list[MAX_FILES];
static unsigned int pre_sorted_file_codec[MAX_FILES];
static int pre_sorted_file_video_width[MAX_FILES];
static int pre_sorted_file_video_height[MAX_FILES];
static int pre_sorted_file_framerate[MAX_FILES] = {0};
static int pre_sorted_file_prefer_HDMI_Mode[MAX_FILES] = {0};
static int pre_sorted_file_prefer_HDMI_FPS[MAX_FILES] = {0};
static int pre_sorted_is_hd[MAX_FILES];
static int pre_sort_tag[MAX_FILES] = {0};
static int slot_has_content[MAX_FILES] = {0};

#define DBasicScore (1<<8)
static unsigned int performance_score[MAX_FILES] = {0};
static unsigned int tot_performance_score = 0;
static unsigned int system_max_performance_score = DBasicScore * 2;//i1 1080p30 x 2
static unsigned int cur_pb_speed[MAX_FILES] = {0};

static unsigned char first_show_full_screen = 0;
static unsigned char first_show_hd_stream = 0;
static unsigned char feed_padding_32bytes = 0;
static unsigned char specify_fps = 0;

static unsigned char mdec_loop = 1;
static unsigned char mdec_skip_feeding = 1;
static unsigned char mdec_running = 1;
static unsigned char udec_running = 1;

static unsigned int sleep_time = 20;

static unsigned char stop_background_udec = 0;
static unsigned char noncachable = 1;
static unsigned char pjpeg_size = 0;
static unsigned char enable_prefetch = 1;

static unsigned char last_speed = 0x01;
static unsigned char last_speed_frac = 0x00;
static unsigned char last_scan_mode = 0x0;
static unsigned char change_speed_with_fluch = 1;

static unsigned int refcache_size = 0;

//static int pic_width;
//static int pic_height;

//udec mode configuration
static int tiled_mode = 5;
static int ppmode = 2;	// 0: no pp; 1: decpp; 2: voutpp
static int deint = 0;

static int dec_types = 0x3F;
static int max_frm_num = 8;
static unsigned long bits_fifo_size = 16*MB;
static unsigned long rd_buffer_size = 16*MB;
static unsigned long ref_cache_size = 720*KB;
static unsigned long pjpeg_buf_size = 16*MB;

static void* prealloc_file_buffer = NULL;

static int npp = 5;
//static int interlaced_out = 0;
//static int packed_out = 0;	// 0: planar yuv; 1: packed yuyv

static int add_useq_upes_header = 0;
static int not_init_udec = 0;
static int exit_udec_in_middle = 0;

static unsigned int one_shot_size = 0;

static unsigned int preset_prefetch_count = 6;

enum {
	EPlayListMode_None = 0,
	EPlayListMode_ExitUDECMode = 1,
	EPlayListMode_ReSetupUDEC,
	EPlayListMode_StopUDEC,
	EPlayListMode_Seamless,
};

static unsigned char enable_one_shot = 0;
static unsigned char one_more_slice = 0;
static unsigned char multiple_slice = 0;
static unsigned char playlist_mode = EPlayListMode_None;

static unsigned int play_count = 0;

static int set_bg_color_YCbCr = 0;
static int bg_color_y = 0;
static int bg_color_cb = 0;
static int bg_color_cr = 0;

static unsigned int display_layout = 0x11;// 1 is default

static int input_sds = -1;
static int input_hds = -1;

static int render_2_udec[MAX_FILES] = {0};
static int render_2_window[MAX_FILES] = {0};

static udec_window_t *debug_p_windows = NULL;
static udec_render_t *debug_p_renders = NULL;
static int debug_sds, debug_hds;

static int framecount_mode = 0;
static int framecount_number = 10;
//static int sent_framecount_number = 0;

enum {
	e_playlist_next = 0x80,
	e_playlist_previous = 0x81,
	e_playlist_current = 0x82,
};

static unsigned char add_delimiter_at_each_slice = 0;
static unsigned char append_replace_delimiter_at_each_slice = 1;
//static unsigned char playlist_looping = 0;
static unsigned char user_press_quit = 0;

static unsigned int playlist_next_or_previous = e_playlist_next;

#define _no_log(format, args...)	(void)0
//debug log
#if 1
#define u_printf_binary _no_log
#define u_printf_binary_index _no_log
#else
#define u_printf_binary u_printf
#define u_printf_binary_index u_printf_index
#endif


#define M_MSG_KILL	0
#define M_MSG_ECHO	1
#define M_MSG_START	2
#define M_MSG_RESTART	3
#define M_MSG_PAUSE	4
#define M_MSG_RESUME	5
#define M_MSG_FLUSH	7
#define M_MSG_UPDATE_SPEED	8
#define M_MSG_PENDDING	9
#define M_MSG_EOS	10

//fb related test
static int need_update_clut_table = 1;

struct fb_cmap_user {
	unsigned int start;			/* First entry	*/
	unsigned int len;			/* Number of entries */
	unsigned short *y;		/* Red values	*/
	unsigned short *u;
	unsigned short *v;
	unsigned short *transp;		/* transparency, can be NULL */
};

static unsigned short clut_y_table[256] =
{
	  5,
	191,
	  0,
	191,
	  0,
	191,
	  0,
	192,
	128,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	204,
	242,
};


static unsigned short clut_u_table[256] =
{
	  4,
	  0,
	191,
	191,
	  0,
	  0,
	191,
	192,
	128,
	  0,
	255,
	255,
	  0,
	  0,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	102,
};


static unsigned short clut_v_table[256] =
{
	  3,
	  0,
	  0,
	  0,
	191,
	191,
	191,
	192,
	128,
	  0,
	  0,
	  0,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 34,
};

static unsigned short clut_blend_table[256] =
{
		0,  12, 24, 48, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255,
		12,  24, 48, 96, 128, 192, 224, 255
};

enum {
	PB_STRATEGY_ALL_FRAME = 0,
	PB_STRATEGY_REF_ONLY,
	PB_STRATEGY_I_ONLY,
	PB_STRATEGY_IDR_ONLY,//h264 only
};

enum {
	ACTION_NONE = 0,
	ACTION_QUIT,
	ACTION_START,
	ACTION_RESTART,
	ACTION_PAUSE,
	ACTION_RESUME,
	ACTION_FLUSH,
	ACTION_UPDATE_SPEED,
	ACTION_PENDING,
	ACTION_EOS,
};

enum {
	UDEC_TRICKPLAY_PAUSE = 0,
	UDEC_TRICKPLAY_RESUME = 1,
	UDEC_TRICKPLAY_STEP = 2,
};

typedef struct display_window_s {
	u8 win_id;
	u8 set_by_user;
	u8 reserved[2];

	u16 input_offset_x;
	u16 input_offset_y;
	u16 input_width;
	u16 input_height;

	u16 target_win_offset_x;
	u16 target_win_offset_y;
	u16 target_win_width;
	u16 target_win_height;
} display_window_t;

//refine display control
#define MAX_DISPLAY_LAYER 2
#define TOT_VOUT_NUMBER 2
static display_window_t display_window[TOT_VOUT_NUMBER][MAX_FILES];
static unsigned int current_display_layer = 0;
static unsigned int current_vout_start_index = 1;
static unsigned int display_layer_number[TOT_VOUT_NUMBER][MAX_DISPLAY_LAYER];

static struct timeval begin_playback_time = {0}, end_playback_time = {0};
static unsigned int total_feed_frame_count = 0;

//simple parse slice_type for h264
typedef struct {
	const u8 *buffer, *buffer_end;
	int index;
	int size_in_bits;
} GetbitsContext;

#define READ_LE_32(x)	\
	((((const u8*)(x))[3] << 24) |	\
	(((const u8*)(x))[2] << 16) |	\
	(((const u8*)(x))[1] <<  8) |	 \
	((const u8*)(x))[0])

#define READ_BE_32(x)	\
	((((const u8*)(x))[0] << 24) |	\
	(((const u8*)(x))[1] << 16) |	\
	(((const u8*)(x))[2] <<  8) |	 \
	((const u8*)(x))[3])

#define BIT_OPEN_READER(name, gb)	\
	unsigned int name##_index = (gb)->index;	\
	int name##_cache	= 0

#define BITS_CLOSE_READER(name, gb) (gb)->index = name##_index
#define BITS_UPDATE_CACHE_BE(name, gb) \
	name##_cache = READ_BE_32(((const u8 *)(gb)->buffer)+(name##_index>>3)) << (name##_index&0x07)
#define BITS_UPDATE_CACHE_LE(name, gb) \
	name##_cache = READ_LE_32(((const u8 *)(gb)->buffer)+(name##_index>>3)) >> (name##_index&0x07)

#define BITS_SKIP_CACHE(name, gb, num) name##_cache >>= (num)

#define BITS_SKIP_COUNTER(name, gb, num) name##_index += (num)

#define BITS_SKIP_BITS(name, gb, num) do {	\
	BITS_SKIP_CACHE(name, gb, num);	\
	BITS_SKIP_COUNTER(name, gb, num);	 \
} while (0)

#define BITS_LAST_SKIP_BITS(name, gb, num) BITS_SKIP_COUNTER(name, gb, num)
#define BITS_LAST_SKIP_CACHE(name, gb, num)

#define BITS_GET_CACHE(name, gb) ((u32)name##_cache)

//log2
const u8 simple_log2_table[256]={
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

const u8 simple_golomb_vlc_len[512]={
19,17,15,15,13,13,13,13,11,11,11,11,11,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

const u8 simple_ue_golomb_vlc_code[512]={
32,32,32,32,32,32,32,32,31,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int simple_log2_c(unsigned int v)
{
	int n = 0;
	if (v & 0xffff0000) {
		v >>= 16;
		n += 16;
	}
	if (v & 0xff00) {
		v >>= 8;
		n += 8;
	}
	n += simple_log2_table[v];

	return n;
}

 /**
 * read unsigned exp golomb code.
 */
static inline int _get_ue_golomb(GetbitsContext *gb){
	unsigned int buf;
	int log;

	BIT_OPEN_READER(re, gb);
	BITS_UPDATE_CACHE_BE(re, gb);
	buf=BITS_GET_CACHE(re, gb);
	//u_printf("get_ue_golomb, 1, buf %08x\n", buf);
	if (buf >= (1<<27)) {
		buf >>= 32 - 9;
		//u_printf("get_ue_golomb, 1.1, buf %08x, vlc_len[buf] %d\n", buf, simple_golomb_vlc_len[buf]);
		BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
		BITS_CLOSE_READER(re, gb);
		//u_printf("result, index %08x, cache %08x, gd->index %08x.\n", re_index, re_cache, gb->index);
		return simple_ue_golomb_vlc_code[buf];
	} else {
		log= 2*simple_log2_c(buf) - 31;
		//u_printf("get_ue_golomb, 2.1, log %d, buf %d\n", log, buf);
		buf>>= log;
		//u_printf("get_ue_golomb, 2.2, buf %08x\n", buf);
		buf--;
		//u_printf("get_ue_golomb, 2.3, buf %08x\n", buf);
		BITS_LAST_SKIP_BITS(re, gb, 32 - log);
		BITS_CLOSE_READER(re, gb);
		//u_printf("result, index %08x, cache %08x, gd->index %08x.\n", re_index, re_cache, gb->index);
		return buf;
	}
}

 /**
 * read unsigned exp golomb code, constraint to a max of 31.
 * the return value is undefined if the stored value exceeds 31.
 */
static inline int _get_ue_golomb_31(GetbitsContext *gb){
	unsigned int buf;

	BIT_OPEN_READER(re, gb);
	//u_printf("open gb, gd->index %08x, re_index %08x, %02x %02x %02x %02x.\n", gb->index, re_index, gb->buffer[0], gb->buffer[1], gb->buffer[2], gb->buffer[3]);
	//u_printf("be32 %08x\n", READ_BE_32(((const u8 *)(gb)->buffer)+(re_index>>3)));
	//u_printf("r %08x\n", READ_BE_32(((const u8 *)(gb)->buffer)+(re_index>>3)) << (re_index&0x07));
	BITS_UPDATE_CACHE_BE(re, gb);
	buf=BITS_GET_CACHE(re, gb);
	//u_printf("get_ue_golomb_31, 1, buf %08x\n", buf);
	buf >>= 32 - 9;
	//u_printf("get_ue_golomb_31, 2, buf %08x\n", buf);
	BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
	BITS_CLOSE_READER(re, gb);
	//u_printf("result, index %08x, cache %08x, gd->index %08x.\n", re_index, re_cache, gb->index);

	return simple_ue_golomb_vlc_code[buf];
}

static unsigned char simple_get_slice_type_le(u8* pdata, u8* first_mb_in_slice)
{
	unsigned char slice_type = 0;
	GetbitsContext gb;

	gb.buffer = pdata;
	gb.buffer_end = pdata + 8;//hard code here
	gb.index = 0;
	gb.size_in_bits = 8*8;

//	u_printf("index 0x%x, size_in_bits %d\n", gb.index, gb.size_in_bits);
//	u_printf(" %02x %02x %02x %02x %02x %02x %02x %02x\n", *(pdata - 4), *(pdata - 3), *(pdata - 2), *(pdata - 1), *(pdata), *(pdata + 1), *(pdata + 2), *(pdata + 3));

	*first_mb_in_slice = _get_ue_golomb(&gb);// first_mb_in_slice
	slice_type = _get_ue_golomb_31(&gb);

//	u_printf("first_mb_in_slice %d, slice_type %d\n", first_mb_in_slice, slice_type);
	if (slice_type > 9) {
		u_printf_error("BAD slice_type %d, first_mb_in_slice %d\n", slice_type, *first_mb_in_slice);
		return 0;
	} else {
//		u_printf("slice_type %d\n", slice_type);
	}

	if (slice_type > 4) {
		slice_type -= 5;
	}

	return slice_type;// P B I SP SI
}

static unsigned int skip_slice_with_strategy(u8 slice_type, u8 nal_type, u8 strategy)
{
	if (PB_STRATEGY_ALL_FRAME == strategy) {
		return 0;
	}

	if (5 <= nal_type) {
		return 0;
	}

	if (PB_STRATEGY_IDR_ONLY == strategy) {
		return 1;
	}

	if (1 != nal_type) {
		u_printf_error("BAD nal_type %d, with partitioning?\n", nal_type);
		return 0;
	}

	if (PB_STRATEGY_I_ONLY == strategy){
		if (2 == slice_type) {
			return 0;
		}
		return 1;
	} else if (PB_STRATEGY_REF_ONLY == strategy) {
		if (2 == slice_type || 0 == slice_type) {
			return 0;
		}
		return 1;
	} else {
		u_printf_error("Internal ERROR: BAD strategy %d\n", strategy);
		return 0;
	}
}

#define Assert(_expr) \
	do { \
		if (!(_expr)) { \
			printf("Assertion failed: %s\n", #_expr); \
			printf("At line %d\n", __LINE__); \
			exit(-1); \
		} \
	} while (0)

//todo, hard code here, fix me
#define MAX_NUM_UDEC 16
#define MAX_NUM_UDEC_ORI 1

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

	if (is_mp4s_flag) {
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

static int vout_get_sink_id(int chan, int sink_type, int iav_fd)
{
	int					num;
	int					i;
	struct amba_vout_sink_info		sink_info;
	u32					sink_id = -1;

	num = 0;
	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return -1;
	}
	if (num < 1) {
		u_printf("Please load vout driver!\n");
		return -1;
	}

	for (i = num - 1; i >= 0; i--) {
		sink_info.id = i;
		if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return -1;
		}

		//u_printf("sink %d is %s\n", sink_info.id, sink_info.name);

		if ((sink_info.sink_type == sink_type) &&
			(sink_info.source_id == chan))
			sink_id = sink_info.id;
	}

	//u_printf("%s: %d %d, return %d\n", __func__, chan, sink_type, sink_id);

	return sink_id;
}

static int get_single_vout_info(int index, int *width, int *height, int iav_fd)
{
	struct amba_vout_sink_info info;

	memset(&info, 0, sizeof(info));
	info.id = vout_get_sink_id(vout_id[index], vout_type[index], iav_fd);
	if (info.id < 0) {
		return -1;
	}

	u_printf_debug("vout id: %d\n", info.id);

	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &info) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_INFO");
		return -1;
	}

	if ((1 == index) && (!prevous_HDMI_Mode)) {
		u_printf_debug("****init prevous_HDMI_Mode %d\n", prevous_HDMI_Mode, info.sink_mode.mode);
		prevous_HDMI_Mode = info.sink_mode.mode;
	}

	u_printf_debug("index %d, auto_vout %d, prevous_HDMI_Mode %x, current %x, shift_file_offset %d\n", index, auto_vout, prevous_HDMI_Mode, file_prefer_HDMI_Mode[shift_file_offset], shift_file_offset);

	if (auto_vout && (1 == index)) {
		if (prevous_HDMI_Mode != file_prefer_HDMI_Mode[shift_file_offset]) {
			info.sink_mode.mode = file_prefer_HDMI_Mode[shift_file_offset];
			info.sink_mode.frame_rate = file_prefer_HDMI_FPS[shift_file_offset];
			if (AMBA_VIDEO_MODE_720P == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 720p60\n");
				info.sink_mode.video_size.video_width = 1280;
				info.sink_mode.video_size.video_height = 720;
				info.sink_mode.video_size.vout_width = 1280;
				info.sink_mode.video_size.vout_height = 720;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else if (AMBA_VIDEO_MODE_1080P == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 1080p60\n");
				info.sink_mode.video_size.video_width = 1920;
				info.sink_mode.video_size.video_height = 1080;
				info.sink_mode.video_size.vout_width = 1920;
				info.sink_mode.video_size.vout_height = 1080;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else if (AMBA_VIDEO_MODE_2160P24 == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 2160p24\n");
				u_assert(info.sink_mode.frame_rate == AMBA_VIDEO_FPS_24);
				info.sink_mode.frame_rate = AMBA_VIDEO_FPS_24;
				info.sink_mode.video_size.video_width = 3840;
				info.sink_mode.video_size.video_height = 2160;
				info.sink_mode.video_size.vout_width = 3840;
				info.sink_mode.video_size.vout_height = 2160;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else if (AMBA_VIDEO_MODE_2160P30 == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 2160p30\n");
				u_assert(info.sink_mode.frame_rate == AMBA_VIDEO_FPS_30);
				info.sink_mode.frame_rate = AMBA_VIDEO_FPS_30;
				info.sink_mode.video_size.video_width = 3840;
				info.sink_mode.video_size.video_height = 2160;
				info.sink_mode.video_size.vout_width = 3840;
				info.sink_mode.video_size.vout_height = 2160;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else if (AMBA_VIDEO_MODE_2160P25 == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 2160p25\n");
				u_assert(info.sink_mode.frame_rate == AMBA_VIDEO_FPS_25);
				info.sink_mode.frame_rate = AMBA_VIDEO_FPS_25;
				info.sink_mode.video_size.video_width = 3840;
				info.sink_mode.video_size.video_height = 2160;
				info.sink_mode.video_size.vout_width = 3840;
				info.sink_mode.video_size.vout_height = 2160;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else if (AMBA_VIDEO_MODE_2160P24_SE == info.sink_mode.mode) {
				u_printf("[update vout setting]: to 2160p24se\n");
				u_assert(info.sink_mode.frame_rate == AMBA_VIDEO_FPS_24);
				info.sink_mode.frame_rate = AMBA_VIDEO_FPS_24;
				info.sink_mode.video_size.video_width = 4096;
				info.sink_mode.video_size.video_height = 2160;
				info.sink_mode.video_size.vout_width = 4096;
				info.sink_mode.video_size.vout_height = 2160;

				info.sink_mode.video_offset.offset_x = 0;
				info.sink_mode.video_offset.offset_y = 0;
			} else {
				u_printf_error("not support mode %d\n", info.sink_mode.mode);
				return (-1);
			}

			info.sink_mode.direct_to_dsp = 1;
			u_printf_debug("update vout, fps %d, video mode 0x%08x.\n", info.sink_mode.frame_rate, info.sink_mode.mode);
			if (ioctl(iav_fd, IAV_IOC_VOUT_CONFIGURE_SINK, &info.sink_mode) < 0) {
				perror("IAV_IOC_VOUT_CONFIGURE_SINK");
				u_printf_error("IAV_IOC_VOUT_CONFIGURE_SINK failed.\n");
				return -1;
			}

			prevous_HDMI_Mode = info.sink_mode.mode;
			u_assert(file_prefer_HDMI_Mode[shift_file_offset] == prevous_HDMI_Mode);
		}
	}

	*width = info.sink_mode.video_size.vout_width;
	*height = info.sink_mode.video_size.vout_height;
	vout_rotate[index] = info.sink_mode.video_rotate;

	/*u_printf("info.sink_mode.format: %d\n", info.sink_mode.format);
	if (info.sink_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) {
		vout_height *= 2;
	}*/

	u_printf_debug("vout size: %d * %d\n", *width, *height);
	if (*width == 0 || *height == 0) {
		//*width = 1280;
		//*height = 720;
		return -2;
	}

	return 0;
}

static int ioctl_enter_idle(int iav_fd)
{
	if (ioctl(iav_fd, IAV_IOC_ENTER_IDLE, 0) < 0) {
		perror("IAV_IOC_ENTER_IDLE");
		return -1;
	}
	//u_printf_debug("enter idle done\n");
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

	if (set_bg_color_YCbCr) {
		udec_mode->pp_background_Y = (u8)bg_color_y;
		udec_mode->pp_background_Cb = (u8)bg_color_cb;
		udec_mode->pp_background_Cr = (u8)bg_color_cr;
	} else {
		udec_mode->pp_background_Y = 0;
		udec_mode->pp_background_Cb = 0;
		udec_mode->pp_background_Cr = 0;
	}

	udec_mode->pp_max_frm_num = npp;

	udec_mode->vout_mask = mudec_actual_voutmask;
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

static void init_udec_windows(udec_window_t *win, int start_index, int nr, int src_width, int src_height)
{
	//int voutA_width = 0;
	//int voutA_height = 0;
	//int voutB_width = 0;
	//int voutB_height = 0;

	int display_width = 0;
	int display_height = 0;

	int rows;
	int cols;
	int rindex;
	int cindex;
	int i;
	u32 win_id;

	udec_window_t *window = win + start_index;

	//LCD not used yet
#if 0
	if (vout_mask & 1) {
		voutA_width = vout_width[0];
		voutA_height = vout_height[0];
		u_printf("Error: M UDEC not support LCD now!!!!\n");
	}

	if (vout_mask & 2) {
		voutB_width = vout_width[1];
		voutB_height = vout_height[1];
	} else {
		u_printf("Error: M UDEC must have HDMI configured!!!!\n");
	}
#endif

	if (nr == 1) {
		rows = 1;
		cols = 1;
	} else if (nr <= 4) {
		rows = 2;
		cols = 2;
	} else if (nr <= 6) {
		rows = 2;
		cols = 3;
	} else if (nr <= 9) {
		rows = 3;
		cols = 3;
	} else if (nr <= 12) {
		rows = 3;
		cols = 4;
	} else {
		rows = 4;
		cols = 4;
	}

	memset(window, 0, sizeof(udec_window_t) * nr);

	rindex = 0;
	cindex = 0;

	if (0 == vout_rotate[mudec_actual_vout_start_index]) {
		display_width = vout_width[mudec_actual_vout_start_index];
		display_height = vout_height[mudec_actual_vout_start_index];
	} else {
		display_width = vout_height[mudec_actual_vout_start_index];
		display_height = vout_width[mudec_actual_vout_start_index];
	}

	if (!display_width || !display_height) {
		u_printf("why vout width %d, vout height %d are zero?\n");
		display_width = 1280;
		display_height = 720;
	}
	u_printf("mudec vout width %d, vout height %d\n", display_width, display_height);

	for (i = 0; i < nr; i++, window++) {
		win_id = i + start_index;
		window->win_config_id = win_id;

		u_assert(win_id < MAX_FILES);
		if (win_id < MAX_FILES) {
			if (display_window[mudec_actual_vout_start_index][win_id].set_by_user) {
				u_printf("%d'th window set by user, off %d,%d, size %d,%d.\n", win_id, display_window[mudec_actual_vout_start_index][win_id].target_win_offset_x, display_window[mudec_actual_vout_start_index][win_id].target_win_offset_y, display_window[mudec_actual_vout_start_index][win_id].target_win_width, display_window[mudec_actual_vout_start_index][win_id].target_win_height);
				window->target_win_offset_x = display_window[mudec_actual_vout_start_index][win_id].target_win_offset_x;
				window->target_win_offset_y = display_window[mudec_actual_vout_start_index][win_id].target_win_offset_y;
				window->target_win_width = display_window[mudec_actual_vout_start_index][win_id].target_win_width;
				window->target_win_height = display_window[mudec_actual_vout_start_index][win_id].target_win_height;
			} else {
				window->target_win_offset_x = cindex * display_width / cols;
				window->target_win_offset_y = rindex * display_height / rows;
				window->target_win_width = display_width / cols;
				window->target_win_height = display_height / rows;
				display_window[mudec_actual_vout_start_index][win_id].win_id = win_id;
				display_window[mudec_actual_vout_start_index][win_id].target_win_offset_x = window->target_win_offset_x;
				display_window[mudec_actual_vout_start_index][win_id].target_win_offset_y = window->target_win_offset_y;
				display_window[mudec_actual_vout_start_index][win_id].target_win_width = window->target_win_width;
				display_window[mudec_actual_vout_start_index][win_id].target_win_height = window->target_win_height;
			}
		}

		window->input_offset_x = 0;
		window->input_offset_y = 0;
		window->input_width = src_width;
		window->input_height = src_height;

#if 0
		window->target_win_offset_x = cindex * display_width / cols;
		window->target_win_offset_y = rindex * display_height / rows;
		window->target_win_width = display_width / cols;
		window->target_win_height = display_height / rows;
#endif

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
		render->input_source_type = MUDEC_INPUT_SRC_UDEC;

		render_2_udec[render->render_id] = render->udec_id;
		render_2_window[render->render_id] = render->win_config_id;
	}
}

static void init_udec_renders_single(udec_render_t *render, u8 render_id, u8 win_id, u8 sec_win_id, u8 udec_id)
{
	memset(render, 0, sizeof(udec_render_t));

	render->render_id = render_id;
	render->win_config_id = win_id;
	render->win_config_id_2nd = sec_win_id;
	render->udec_id = udec_id;
	render->input_source_type = MUDEC_INPUT_SRC_UDEC;

	render_2_udec[render->render_id] = render->udec_id;
	render_2_window[render->render_id] = render->win_config_id;
}

static void _print_ut_options()
{
	u_printf("mudec options:\n");
	u_printf("\t'-f [filename]': '-f' specify input file name\n");
	u_printf("\t'-s [%%dx%%d]': '-s' specify input file's coded width and height, [width x height]\n");
	//u_printf("\t'-c h264': '-c' specify input file's video format, default is h264\n");
	//u_printf("\t'-d [%%d:%%d,%%d,%%d,%%d,%%d]': '-d' specify window's property, [win_id:offset_x,offset_y,size_x,size_y,vout]\n\n");

	u_printf("\t'--voutmask [mask]': '--voutmask' specify output device's mask, LCD's bit is 0x01, HDMI's bit is 0x02\n");
	u_printf("\t'--maxbuffernumber [number]': '--maxbuffernumber' specify each udec instance's dec buffer number, the minimum number is 6\n");
	u_printf("\t'--fps [fps]': '--fps' specify framerate\n");
	u_printf("\t'--help': print help\n\n");

	u_printf("\t'--enableerrorhandling': will enable UDEC error handling(default)\n");
	u_printf("\t'--disableerrorhandling': will disable UDEC error handling\n");
	u_printf("\t'--loop': will choose loop play mode(default)\n");
	u_printf("\t'--noloop': will choose non-loop play mode\n");
	u_printf("\t'--simpleplaylist', EPlayListMode_ExitUDECMode, enable playlist mode, exit UDEC mode, enter it again.\n");
	u_printf("\t'--playlist', EPlayListMode_ReSetupUDEC, enable playlist mode, STOP/EXIT UDEC instance, Re-SETUP it again.\n");
	u_printf("\t'--playlist1', EPlayListMode_StopUDEC, enable playlist mode, STOP UDEC instance, then Decode new bitstream\n");
	u_printf("\t'--playlist2', EPlayListMode_ReSetupUDEC, enable playlist mode, STOP/EXIT UDEC instance, Re-SETUP it again.\n");
	//u_printf("\t'--seamlessplaylist', EPlayListMode_Seamless, enable playlist mode, with seamless feature.\n");
	u_printf("\t'--cached': set buffer mapping as cached\n");
	u_printf("\t'--noncached': set buffer mapping as noncached\n");
	u_printf("\t'--layout': specify display layout: 1, rect layout; 2 bottom left highlight layout; 3, center V layout, 4: avg column layout, 5: avg row layout\n\n");
	u_printf("\t'--padding': will padding to align 32bytes(bit-stream buffer)\n");
	u_printf("\t'--nopadding': will not padding to align 32bytes(bit-stream buffer)\n");

	u_printf(" debug use only:\n");
	u_printf("\t'--debuglog': will enable debug log\n");
	u_printf("\t'--feedbackground': will always fill data, even the bit-stream is not displayed(not default)\n");
	u_printf("\t'--skipfeeding': will enable frame dropping for fast forward(IDR only, I Only, Ref Only, All frames)\n");
	u_printf("\t'--noskipfeeding': will disable frame dropping for fast forward, always feed all frames\n");
	u_printf("\t'--tilemode': use tilemode(DSP)\n");
	u_printf("\t'--usequpes': use DSP defined SEQ/PES header\n");
	u_printf("\t'--nousequpes': not use DSP defined SEQ/PES header\n");
	u_printf("\t'--pts': will generate PTS for each frame\n");
	u_printf("\t'--nopts': will not generate PTS\n");
	u_printf("\t'--fifosize [size]': set bit-stream buffer size(MBytes) for each udec instance\n");
	u_printf("\t'--pjpegsize [size]': set internal buffer(h264 cabac, MBytes) size for each udec instance\n");
	u_printf("\t'--rdsize [size]': set unit-test file reading buffer size(KBytes) for each udec instance\n");
	u_printf("\t'--prefetch [count]': prefetch %%d frames before decoding, for unit test\n");
	u_printf("\t'--refcachesize [size]': ref_cache_size (KBytes) for each udec instance\n");
	u_printf("\t'--dumpt': dump feeding data(whole file) for each udec instance\n");
	u_printf("\t'--dumps': dump feeding data(each frame per file) for each udec instance\n");
	u_printf("\t'--debug-noinitudec': will not invoke ioctl(init udec) when the bit-stream is not valid.\n");
	u_printf("\t'--debug-exitudec', will exit udec immediately if it has problem.\n");
	u_printf("\t'--bgcolor [Y,U,V]', set background color Y,U,V\n");
	u_printf("\t'--adddelimiter, will add additional delimiter at the end of each slice\n");
	u_printf("\t'--appenddelimiter, will add additional delimiter at the end of each slice, but the next slice will overwrite it\n");
	u_printf("\t'--notappenddelimiter, will not add/append additional delimiter at the end of each slice\n");
	u_printf("\t'--fileshot, will send total file to dsp in one decode cmd\n");
	u_printf("\t'--onemoreslice, will send decode cmd to dsp n-1 slice, when feeded n slice\n");
}

static void _print_ut_cmds()
{
	u_printf(" mudec cmds: press cmd + Enter\n");
	u_printf("\t'q': Quit unit test\n");
	u_printf("\t'h': Print cmd help\n");
	u_printf("\t's%%d': Step play for %%d udec instance\n");
	u_printf("\t' %%d': Pause/Resume for %%d udec instance\n");
	u_printf("\t'pa': Print all udec instance runtime infos\n");
	u_printf("\t'p%%d': Print %%d udec instance runtime infos\n\n");
	u_printf(" Extended cmds, start with 'c':\n");
	u_printf("\t'cs', switch category:\n");
	u_printf("\t\t'csa%%dto%%d', seamless switch, render_id, new_udec_id, then update renders\n");
	u_printf("\t\t'cs%%dto%%d', seamless switch, render_id, new_udec_id, not update renders\n");
	u_printf("\t'cr', update render categ:\n");
	u_printf("\t\t'crhd', update renders to display one window\n");
	u_printf("\t\t'crsd', update renders to display multi windows\n");
	u_printf("\t'cp', playback speed category:\n");
	u_printf("\t\t'cpa:%%d %%x.%%x', update udec to speed %%x.%%x, if speed greater than 2x, choose I Only automatically\n");
	u_printf("\t\t'cp:%%d %%x.%%x', update udec to speed %%x.%%x.\n");
	u_printf("\t\t'cpi:%%d %%x.%%x', update udec to speed %%x.%%x, always choose I Only feeding\n");
	u_printf("\t\t'cpr:%%d %%x.%%x', update udec to speed %%x.%%x, always choose Ref Only feeding\n");
	u_printf("\t\t'cpc:%%d', clear speed setting, 1x speed, All frame feeding\n");
	u_printf("\t\t'cpm:%%d %%d', update feeding rules only\n");
	u_printf("\t'cc', still capture category:\n");
	u_printf("\t\t'ccjpeg:%%d %%d', capture for udec %%d, file save index %%d.(capture coded/thumbnail/screennail)\n");
	u_printf("\t\t'ccjpegns:%%d %%d', capture for udec %%d, file save index %%d.(capture coded/thumbnail)\n");
	u_printf("\t\t'ccjpegnt:%%d %%d', capture for udec %%d, file save index %%d.(capture coded/screennail)\n");
	u_printf("\t\t'ccjpegntns:%%d %%d', capture for udec %%d, file save index %%d.(capture coded)\n");
	u_printf("\t'cz', zoom:\n");
	u_printf("\t\t'cz1:%%d,%%x,%%x', (zoom mode 1, zoom factor), render_id %%d, zoom factor x %%d, zoom factor y.\n");
	u_printf("\t\t'cz2:%%d,%%hd,%%hd,%%hd,%%hd', (zoom mode 2, input window) render_id %%d, input width %%d, height %%d, center x %%d, y %%d.\n");
	u_printf("\t\t'czi', continue zoom in\n");
	u_printf("\t\t'czo', continue zoom out\n");
	u_printf("\t'cd', update display:\n");
	u_printf("\t\t'cd%%d', change to display on requested vout(%%d), 0 means LCD, 1 means HDMI\n");
	u_printf("\t'cfb', fb related\n");
	u_printf("\t\t'cfb0clear', clear fb 0\n");
	u_printf("\t\t'cfb1clear', clear fb 1\n");
	u_printf("\t\t'cfb0set:%%d', set content in fb 0 to %%d\n");
	u_printf("\t\t'cfb1set:%%d', set content in fb 1 to %%d\n");

	u_printf("\t\t'cfb08clear', clear fb 0(8bit mode)\n");
	u_printf("\t\t'cfb18clear', clear fb 1(8bit mode)\n");
	u_printf("\t\t'cfb08set:%%d', set content in fb 0 to %%d(8bit mode)\n");
	u_printf("\t\t'cfb18set:%%d', set content in fb 1 to %%d(8bit mode)\n");

	u_printf("\t\t'cfb0clut', update clut table for fb 0\n");
	u_printf("\t\t'cfb1clut', update clut table for fb 1\n");

	u_printf("\t\t'cbmp0:', show bmp file on fb 0\n");
	u_printf("\t\t'cbmp1:', show bmp file on fb 1\n");

	u_printf("\t\t'cfb0rect:', show rect on fb 0\n");
	u_printf("\t\t'cfb1rect:', show rect on fb 1\n");
}

static void _print_udec_ut_cmds()
{
	u_printf(" mudec cmds: press cmd + Enter\n");
	u_printf("\t'q': Quit unit test\n");
	u_printf("\t'h': Print cmd help\n");
	u_printf("\t'n': switch to next file in playlist\n");
	u_printf("\t'cnext': switch to next file in playlist\n");
	u_printf("\t'cpre': switch to previous file in playlist\n");
}

static int init_mdec_params(int argc, char **argv)
{
	int i = 0;
	int ret = 0;
	int started_scan_filename = 0;
	u32 param0, param1, param2, param3, param4, param5;
	u64 performance_base = 1920*1080;

	if (argc < 2) {
		_print_ut_options();
		return 1;
	}

	//parse options
	for (i=1; i<argc; i++) {
		if (!strcmp("--voutmask", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if ((ret > 3) || (ret < 1)) {
					u_printf_error("[input argument error]: '--voutmask', ret(%d) should between 1 to 3.\n", ret);
				} else {
					mudec_request_voutmask = ret;
					u_printf("[input argument]: '--voutmask': (0x%x).\n", mudec_request_voutmask);
				}
			} else {
				u_printf_error("[input argument error]: '--voutmask', should follow with integer(voutmask), argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--udec", argv[i])) {
			use_udec_mode = 1;
			u_printf("[input argument] --udec, use udec mode (ppmode = 2).\n");
		} else if (!strcmp("--autovout", argv[i])) {
			auto_vout = 1;
			u_printf("[input argument] --auto_vout, will update vout according to file.\n");
		} else if (!strcmp("--noupscaling", argv[i])) {
			no_upscaling = 1;
			u_printf("[input argument] --noupscaling, no up-scaling.\n");
		} else if (!strcmp("--dir", argv[i])) {
			if ((i + 1) >= argc) {
				u_printf_error("'--dir' should be followed by a pathname\n");
				return -8;
			}
			if (started_scan_filename || current_file_index) {
				u_printf_error("'--dir' and '-f' can not use together\n");
				return -8;
			}
			snprintf(dirname, 255, "%s", argv[i + 1]);
			u_printf("[input argument] --dir, read dir: %s.\n", dirname);
			scan_dir = 1;
			playlist_mode = EPlayListMode_ExitUDECMode;
			i ++;
		} else if (!strcmp("--adddelimiter", argv[i])) {
			add_delimiter_at_each_slice = 1;
			u_printf("[input argument] --adddelimiter, will add additional delimiter at end of each slice (for debug purpuse).\n");
		} else if (!strcmp("--appenddelimiter", argv[i])) {
			append_replace_delimiter_at_each_slice = 1;
			u_printf("[input argument] --appenddelimiter, will add additional delimiter at end of each slice, the next slice will overwrite the delimiter (for debug purpuse).\n");
		} else if (!strcmp("--notappenddelimiter", argv[i])) {
			add_delimiter_at_each_slice = 0;
			append_replace_delimiter_at_each_slice = 0;
			u_printf("[input argument] --notappenddelimiter, will not add/append additional delimiter at end of each slice\n");
		} else if (!strcmp("--enableerrorhandling", argv[i])) {
			enable_error_handling = 1;
			u_printf("[input argument] --enableerrorhandling, enable error handling.\n");
		} else if (!strcmp("--disableerrorhandling", argv[i])) {
			enable_error_handling = 0;
			u_printf("[input argument] --disableerrorhandling, disable error handling.\n");
		} else if (!strcmp("--loop", argv[i])) {
			mdec_loop = 1;
			u_printf("[input argument] --loop, enable loop feeding data.\n");
		} else if (!strcmp("--noloop", argv[i])) {
			mdec_loop = 0;
			u_printf("[input argument] --noloop, disable loop feeding data.\n");
		} else if (!strcmp("--simpleplaylist", argv[i])) {
			playlist_mode = EPlayListMode_ExitUDECMode;
			u_printf("[input argument] --simpleplaylist, EPlayListMode_ExitUDECMode, enable playlist mode, exit UDEC mode, enter it again.\n");
		} else if (!strcmp("--debuglog", argv[i])) {
			enable_debug_log = 1;
			u_printf("[input argument] --debuglog, enable debug log.\n");
		} else if (!strcmp("--playlist2", argv[i])) {
			playlist_mode = EPlayListMode_ReSetupUDEC;
			u_printf("[input argument] --playlist2, EPlayListMode_ReSetupUDEC, enable playlist mode, STOP/EXIT UDEC instance, Re-SETUP it again.\n");
		} else if (!strcmp("--playlist1", argv[i])) {
			playlist_mode = EPlayListMode_StopUDEC;
			u_printf("[input argument] --playlist1, EPlayListMode_StopUDEC, enable playlist mode, STOP UDEC instance, then Decode new bitstream\n");
		} else if (!strcmp("--playlist", argv[i])) {
			playlist_mode = EPlayListMode_StopUDEC;
			u_printf("[input argument] --playlist, EPlayListMode_StopUDEC, enable playlist mode, STOP UDEC instance, then Decode new bitstream\n");
		} else if (!strcmp("--seamlessplaylist", argv[i])) {
			playlist_mode = EPlayListMode_Seamless;
			u_printf("[input argument] --seamlessplaylist, EPlayListMode_Seamless, enable playlist mode, with seamless feature.\n");
		} else if (!strcmp("--help", argv[i])) {
			_print_ut_options();
		} else if (!strcmp("--noskipfeeding", argv[i])) {
			mdec_skip_feeding = 0;
			u_printf("[input argument] --noskipfeeding, feed all data to dsp.\n");
		} else if (!strcmp("--skipfeeding", argv[i])) {
			mdec_skip_feeding = 1;
			u_printf("[input argument] --skipfeeding, not feed all data to dsp, skip P/B in IOnly mode, etc.\n");
		} else if (!strcmp("--padding", argv[i])) {
			feed_padding_32bytes = 1;
			u_printf("[input argument] --padding, padding each data trunk to 32bytes align\n");
		} else if (!strcmp("--nopadding", argv[i])) {
			feed_padding_32bytes = 0;
			u_printf("[input argument] --nopadding, not padding each data trunk to 32bytes align\n");
		} else if (!strcmp("--tilemode", argv[i])) {
			u_printf("[input argument] --tilemode.\n");
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%x", &ret))) {
				if ((0 == ret) || (4 == ret) || (5 == ret)) {
					tiled_mode = ret;
					u_printf("[input argument]: '--tilemode, (%x).\n", ret);
				} else {
					u_printf_error("[input argument error]: '--tilemode', not valid tilemode value.\n", ret);
					tiled_mode = 5;
				}
				i ++;
			} else {
				u_printf_error("[input argument warning]: '--tilemode', should follow with integer, argc %d, i %d.\n", argc, i);
			}
		} else if (!strcmp("--prefetch", argv[i])) {
			u_printf("[input argument] --prefetch.\n");
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if (0 < ret) {
					enable_prefetch = 1;
					preset_prefetch_count = ret;
					u_printf("[input argument]: '--prefetch, (%d).\n", preset_prefetch_count);
				} else if (0 == ret) {
					enable_prefetch = 0;
					preset_prefetch_count = 0;
					u_printf("[input argument]: '--prefetch 0, disable prefetch.\n");
				} else {
					u_printf_error("[input argument error]: '--prefetch', not valid prefetch param %d.\n", ret);
					enable_prefetch = 0;
				}
				i ++;
			} else {
				u_printf_error("[input argument warning]: '--prefetch', should follow with integer, argc %d, i %d.\n", argc, i);
			}
		} else if (!strcmp("--multipleslice", argv[i])) {
			int tmp_multiple_slice = 0;
			u_printf("[input argument] --multipleslice.\n");
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &tmp_multiple_slice))) {
				u_printf("[input argument]: '--multipleslice, (%d).\n", tmp_multiple_slice);
				multiple_slice = tmp_multiple_slice;
				i ++;
			} else {
				u_printf_error("[input argument warning]: '--multipleslice', should follow with integer, argc %d, i %d.\n", argc, i);
			}
		} else if (!strcmp("--framecount", argv[i])) {
			u_printf("[input argument] --framecount.\n");
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%x", &framecount_number))) {
				u_printf("[input argument]: '--framecount, (%x).\n", framecount_number);
				i ++;
				if (framecount_number) {
					framecount_mode = 1;
				}
			} else {
				u_printf_error("[input argument warning]: '--framecount', should follow with integer, argc %d, i %d.\n", argc, i);
			}
		} else if (!strcmp("--sleep", argv[i])) {
			u_printf("[input argument] --sleep.\n");
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &sleep_time))) {
				u_printf("[input argument]: '--sleep, (%d).\n", sleep_time);
				i ++;
			} else {
				u_printf_error("[input argument warning]: '--sleep', should follow with integer, argc %d, i %d.\n", argc, i);
			}
		} else if (!strcmp("--layout", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%x", &ret))) {
				display_layout = ret;
				u_printf("[input argument]: '--layout, (0x%08x).\n", ret);
			} else {
				u_printf_error("[input argument error]: '--layout', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--fps", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				specify_fps = ret;
				u_printf("[input argument]: '--fps, (%d).\n", specify_fps);
			} else {
				u_printf_error("[input argument error]: '--fps', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--noncached", argv[i])) {
			noncachable = 1;
			u_printf("[input argument] --noncached, set mapping noncached.\n");
		} else if (!strcmp("--cached", argv[i])) {
			noncachable = 0;
			u_printf("[input argument] --cached, set mapping cached.\n");
		} else if (!strcmp("--pjpegsize", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				pjpeg_size = ret;
				u_printf("[input argument]: '--pjpegsize, (%d MB).\n", pjpeg_size);
			} else {
				u_printf_error("[input argument error]: '--pjpegsize', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--refcachesize", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				refcache_size = ret;
				u_printf("[input argument]: '--refcachesize, (%d KB).\n", refcache_size);
				ref_cache_size = refcache_size * KB;
			} else {
				u_printf_error("[input argument error]: '--refcachesize', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--hds", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				input_hds = ret;
				u_printf("[input argument]: '--hds, number is (%d).\n", ret);
			} else {
				u_printf_error("[input argument error]: '--hds', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--stopbackground", argv[i])) {
			stop_background_udec = 1;
			u_printf("[input argument] --stopbackground, stop background udec instance.\n");
		} else if (!strcmp("--maxbuffernumber", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if (ret < 4 || ret  > 20) {
					u_printf_error("[input argument error]: '--maxbuffernumber', number(%d) should not less than 4, or greate than 20.\n", ret);
				} else {
					max_frm_num = ret;
					u_printf("[input argument]: '--maxbuffernumber', number is (%d).\n", ret);
				}
			} else {
				u_printf_error("[input argument error]: '--maxbuffernumber', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--fifosize", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if (ret < 1 || ret  > 64) {
					u_printf_error("[input argument error]: '--fifosize', fifosize(%d MB) should not less than 1MB, or greate than 12MB.\n", ret);
				} else {
					bits_fifo_size = ret * MB;
					u_printf("[input argument]: '--fifosize', bits fifo size is (%d MB).\n", ret);
				}
			} else {
				u_printf_error("[input argument error]: '--fifosize', should follow with integer.\n");
			}
			i ++;
		} else if (!strcmp("--rdsize", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if (ret < 1) {
					u_printf_error("[input argument error]: '--rdsize', fifosize(%d kb) should not less than 1kb.\n", ret);
				} else {
					rd_buffer_size = ret * KB;
					u_printf("[input argument]: '--rdsize', bits fifo size is (%d KB).\n", ret);
				}
			} else {
				u_printf_error("[input argument error]: '--rdsize', should follow with integer.\n");
			}
			i ++;
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
		} else if (!strcmp("--fileshot", argv[i])) {
			enable_one_shot = 1;
			u_printf("[input argument] --fileshot, send total file to dsp.\n");
		} else if (!strcmp("--onemoreslice", argv[i])) {
			one_more_slice = 1;
			u_printf("[input argument] --onemoreslice, will send one more slice data to dsp.\n");
		} else if (!strcmp("--feedbackground", argv[i])) {
			test_feed_background = 1;
			u_printf("[input argument] --feedbackground, will feed all udec instance.\n");
		} else if (!strcmp("--usequpes", argv[i])) {
			add_useq_upes_header = 1;
			u_printf("[input argument] --usequpes, will add USEQ/UPES header.\n");
		} else if (!strcmp("--nousequpes", argv[i])) {
			add_useq_upes_header = 0;
			u_printf("[input argument] --nousequpes, will not add USEQ/UPES header.\n");
		} else if (!strcmp("--debug-noinitudec", argv[i])) {
			not_init_udec = 1;
			u_printf("[input argument] --debug-noinitudec, will not invoke ioctl(init udec) when the bit-stream is not valid.\n");
		} else if (!strcmp("--debug-exitudec", argv[i])) {
			exit_udec_in_middle = 1;
			u_printf("[input argument] --debug-exitudec-in-midlle, will exit udec immediately if it has problem.\n");
		} else if (!strcmp("--bgcolor", argv[i])) {
			if (((i + 1) < argc) && (3 == sscanf(argv[i + 1], "%d,%d,%d", &bg_color_y, &bg_color_cb, &bg_color_cr))) {
				set_bg_color_YCbCr = 1;
				u_printf("[input argument]: '--bgcolor YCbCr (%d, %d, %d).\n", bg_color_y, bg_color_cb, bg_color_cr);
			} else {
				u_printf_error("[input argument error]: '--bgcolor', should follow with %%d,%%d,%%d\n");
			}
			i++;
		} else if(!strcmp("-f", argv[i])) {
			u_assert(!scan_dir);
			if (scan_dir) {
				u_printf_error("'-f' and '--dir' can not use together.\n");
				return (-9);
			}
			if (started_scan_filename) {
				current_file_index ++;
			}
			if (current_file_index >= MAX_FILES) {
				u_printf_error("max file number(current index %d, max value %d).\n", current_file_index, MAX_FILES);
				return (-1);
			}
			strncpy(&file_list[current_file_index][0], argv[i+1], sizeof(file_list[current_file_index]) - 1);
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
			i ++;
			if ((720 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((800 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((960 == file_video_width[current_file_index]) && (540 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((1280 == file_video_width[current_file_index]) && (720 == file_video_height[current_file_index])) {
				//hd
				is_hd[current_file_index] = 1;
			} else if ((1920 == file_video_width[current_file_index]) && (1080 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else if ((3840 == file_video_width[current_file_index]) && (2160 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else if ((4096 == file_video_width[current_file_index]) && (2160 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else if ((1600 == file_video_width[current_file_index]) && (1200 == file_video_height[current_file_index])) {
				//hd
				is_hd[current_file_index] = 1;
			} else if ((1280 == file_video_width[current_file_index]) && (1024 == file_video_height[current_file_index])) {
				//hd
				is_hd[current_file_index] = 1;
			} else if ((352 == file_video_width[current_file_index]) && (288 == file_video_height[current_file_index])) {
				is_hd[current_file_index] = 0;
			} else if ((640 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				is_hd[current_file_index] = 0;
			} else {
				u_printf("!!!NOT supported resolution %d x %d, yet.\n", file_video_width[current_file_index], file_video_height[current_file_index]);
			}
			performance_score[current_file_index] = (u32)((u64)file_video_width[current_file_index] * (u64)file_video_height[current_file_index] * DBasicScore / performance_base);
			tot_performance_score += performance_score[current_file_index];
			u_printf("[input argument] -s[%d]: %dx%d, score %u, tot_score %u.\n", current_file_index, file_video_width[current_file_index], file_video_height[current_file_index], performance_score[current_file_index], tot_performance_score);
		} else if (!strcmp("-d", argv[i])) {
			ret = sscanf(argv[i+1], "%d:%d,%d,%d,%d,%d", &param0, &param1, &param2, &param3, &param4, &param5);
			if (6 != ret) {
				u_printf_error("[input argument error], '-d' should be followed by 'win_id:off_x,off_y,size_x,size_y,vout'.\n");
				return (-3);
			}
			u_printf("[input argument] -d %d:%d,%d,%d,%d,%d.\n", param0, param1, param2, param3, param4, param5);
			if (param0 >= MAX_FILES) {
				u_printf_error("[input argument error], '-d' win_id(%d) should less than %d.\n", param0, MAX_FILES);
			} else {
				if (((param0 + param2) > vout_width[0]) || ((param1 + param3) > vout_height[0])) {
					u_printf_error("[input argument error], '-d' win argument invalid, off_x(%d) + size_x(%d) = %d, width %d, off_y(%d) + size_y(%d) = %d, height %d.\n", param0, param2, param0 + param2, vout_width[1], param1, param3, param1 + param3, vout_height[1]);
				}
			}
			if (param5 >= TOT_VOUT_NUMBER) {
				u_printf_error("[input argument error], '-d' vout %d exceed valid range.\n", param5);
				break;
			}
			display_window[param5][param0].set_by_user = 1;
			display_window[param5][param0].win_id = param0;
			display_window[param5][param0].target_win_offset_x = param1;
			display_window[param5][param0].target_win_offset_y = param2;
			display_window[param5][param0].target_win_width = param3;
			display_window[param5][param0].target_win_height = param4;
			i ++;
		} else if (!strcmp("-h", argv[i])) {
			_print_ut_options();
		} else {
			u_printf("NOT processed option(%s).\n", argv[i]);
		}
	}
	return 0;
}

static int do_test_mdec(int iav_fd);

static int open_iav(void)
{
	int iav_fd = -1;
	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	return iav_fd;
}

////////////////////////////////////////////////////////////////////////////////////

int enable_debug_log = 0;

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

//msg_queue_t renderer_queue;
//pthread_t renderer_thread_id;

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

	msg_queue_t	renderer_cmd_queue;

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

	unsigned char current_playback_strategy;
	unsigned char tobe_playback_strategy;
	unsigned char reserved4[2];

	//change speed with flush
	unsigned char tobe_playback_strategy_flushed;
	unsigned char tobe_playback_direction_flushed;
	unsigned char tobe_playback_scan_mode_flushed;
	unsigned char reserved5;

	unsigned short tobe_playback_speed_flushed;
	unsigned short tobe_playback_speed_frac_flushed;
} udec_instance_param_t;

static int ioctl_init_udec_instance(iav_udec_info_ex_t* info, int iav_fd, iav_udec_vout_config_t* vout_config, unsigned int num_vout, int udec_index, unsigned int request_bsb_size, int udec_type, int deint, int error_handling, int pic_width, int pic_height)
{
	u_printf_debug("init udec %d, type %d\n", udec_index, udec_type);
	u_assert(UDEC_H264 == udec_type);

	info->udec_id = udec_index;
	info->udec_type = udec_type;
	info->noncachable_buffer = noncachable;
	info->enable_pp = 1;
	info->enable_deint = deint;
	info->enable_err_handle = error_handling;
	info->interlaced_out = 0;
	info->packed_out = 0;

	info->vout_configs.num_vout = num_vout;
	info->vout_configs.vout_config = vout_config;

	info->vout_configs.first_pts_low = 0;
	info->vout_configs.first_pts_high = 0;

	info->vout_configs.input_center_x = pic_width / 2;
	info->vout_configs.input_center_y = pic_height / 2;

	info->bits_fifo_size = request_bsb_size;
	info->ref_cache_size = ref_cache_size;

	switch (udec_type) {
	case UDEC_H264:
		if (!pjpeg_size) {
			info->u.h264.pjpeg_buf_size = pjpeg_buf_size;
		} else {
			info->u.h264.pjpeg_buf_size = pjpeg_size * MB;
		}
		u_printf_debug("[configuration]: init udec %d, h264, pjpeg_buf_size %d, ref_cache_size %d\n", udec_index, info->u.h264.pjpeg_buf_size, info->ref_cache_size);
		break;

	default:
		u_printf_error("udec type %d not implemented\n", udec_type);
		return -1;
	}

	if (ioctl(iav_fd, IAV_IOC_INIT_UDEC, info) < 0) {
		perror("IAV_IOC_INIT_UDEC");
		return -1;
	}

	u_printf_debug("[configuration]: init udec %d, type %d done, noncachable %d, info->bits_fifo_size %d, info->ref_cache_size %d\n", udec_index, udec_type, info->noncachable_buffer, info->bits_fifo_size, info->ref_cache_size);

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

#if 0
static unsigned char* padding_to_32bytes(unsigned char *p_bsb_cur)
{
	unsigned char* p_padding_end = p_bsb_cur;

	if (0 == ((u32)p_padding_end & 0x0000001f)) {
		u_printf("no padding: 0x%08x\n", p_bsb_cur);
		return p_bsb_cur;
	} else {
		p_padding_end = (unsigned char*) (((u32)p_padding_end + 31) & 0xffffffe0);
		u_printf("padding [start 0x%08x, 0x%08x)\n", p_bsb_cur, p_padding_end);
		memset(p_bsb_cur, 0x0, (u32)p_padding_end - (u32)p_bsb_cur);
		return p_padding_end;
	}
}

static unsigned int get_next_start_code(unsigned char* pstart, unsigned char* pend, unsigned int esType, u8* p_nal_type, u8* p_slice_type)
{
	unsigned int size = 0;
	unsigned char* pcur = pstart;
	unsigned int state = 0;

	int	parse_by_amba = 0;
	unsigned char	nal_type;
	unsigned char	first_mb_in_slice;

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

	while (pcur < pend) {
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
					//*p_nal_type = nal_type;
					if (nal_type >= 1 && nal_type <= 5) {
						if (cnt == 1) {
							simple_get_slice_type_le(pcur + 1, &first_mb_in_slice);

							if (!first_mb_in_slice) {
								if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
									return pcur - 4 - pstart;
								} else {
									return pcur - 3 - pstart;
								}
							} else {
								state = 0;
							}
						}
						else {
							//u_printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", *(pcur - 4), *(pcur - 3), *(pcur - 2), *(pcur - 1), *(pcur), *(pcur + 1), *(pcur + 2), *(pcur + 3));
							*p_slice_type = simple_get_slice_type_le(pcur + 1, &first_mb_in_slice);
							*p_nal_type = nal_type;
							state = 0;
							cnt --;
						}
					} else if (cnt == 1 && nal_type >= 6 && nal_type <= 9) {
						// left SPS and PPS be prefix
						if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
							return pcur - 4 - pstart;
						} else {
							return pcur - 3 - pstart;
						}
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
	//u_printf("not find start code, file end, or need read more data from file\n");
	//size = pend - pstart;
	return 0;
}
#endif
#if 0
static unsigned int total_frame_count(unsigned char * start, unsigned char * end, unsigned int udec_type)
{
	unsigned int totsize = 0;
	unsigned int size = 0;
	unsigned int frames_cnt = 0;
	unsigned char* p_cur = start;

	u8 t1, t2;

	while (p_cur < end) {
		//u_printf("while loop 1 p_cur %p, end %p.\n", p_cur, end);
		size = get_next_start_code(p_cur, end, udec_type, &t1, & t2);
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
#endif

static int request_bits_fifo(int iav_fd, unsigned int udec_index, unsigned int size, unsigned char* p_bsb_cur)
{
	iav_wait_decoder_t wait;
	int ret;
	wait.emptiness.room = size + 256;//for safe
	wait.emptiness.start_addr = p_bsb_cur;

	wait.flags = IAV_WAIT_BITS_FIFO;
	wait.decoder_id = udec_index;

	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
		if (enable_debug_log) {
			u_printf("[error]: IAV_IOC_WAIT_DECODER fail, ret %d.\n", ret);
			perror("IAV_IOC_WAIT_DECODER");
		}
		return ret;
	}

	return 0;
}

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
		case M_MSG_FLUSH:
			ret = ACTION_FLUSH;
			break;
		case M_MSG_UPDATE_SPEED:
			ret = ACTION_UPDATE_SPEED;
			break;
		case M_MSG_PENDDING:
			ret = ACTION_PENDING;
			break;
		case M_MSG_EOS:
			ret = ACTION_EOS;
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
	//for debug purpose
	fflush(file);
}

static void dump_binary_new_file(char* filename, unsigned int udec_index, unsigned int file_index, unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start, unsigned char* p_end)
{
	char* file_name = (char*)malloc(strlen(filename) + 32 + 32);
	if (!file_name) {
		u_printf("malloc fail.\n");
		return;
	}
	sprintf(file_name, filename, udec_index, file_index);
	FILE* file = fopen(file_name, "wb");

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
			u_printf("[warnning]: udec(%d) is already paused/resumed(%d).\n", udec_param->udec_index, trickplay_mode);
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

static int ioctl_udec_stop(int iav_fd, unsigned int udec_index, unsigned int stop_flag)
{
	int ret;
	unsigned int stop_code = (stop_flag << 24) | udec_index;

	u_printf_debug("[flow cmd]: before IAV_IOC_UDEC_STOP, udec(%d), stop_flag(%d).\n", udec_index, stop_flag);
	ret = ioctl(iav_fd, IAV_IOC_UDEC_STOP, stop_code);
	if (ret < 0) {
		if (enable_debug_log) {
			perror("IAV_IOC_UDEC_STOP");
			u_printf_error("!!!!!IAV_IOC_UDEC_STOP error, ret %d.\n", ret);
		}
		return ret;
	}
	u_printf_debug("[flow cmd]: IAV_IOC_UDEC_STOP done, udec(%d), stop_flag(%d).\n", udec_index, stop_flag);

	return 0;
}

static void udec_pause_resume(udec_instance_param_t* udec_param)
{
	int ret;

	if (UDEC_TRICKPLAY_PAUSE == udec_param->trickplay_mode) {
		//resume
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_RESUME);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_RESUME;
		}
	} else if (UDEC_TRICKPLAY_RESUME == udec_param->trickplay_mode) {
		//pause
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_PAUSE);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_PAUSE;
		}
	} else if (UDEC_TRICKPLAY_STEP == udec_param->trickplay_mode) {
		//resume
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_RESUME);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_RESUME;
		}
	} else {
		u_printf_error("BAD trickplay mode for udec(%d). trickplay_mode %d.\n", udec_param->udec_index, udec_param->trickplay_mode);
	}
}

static void udec_step(udec_instance_param_t* udec_param)
{
	int ret;
	ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_STEP);
	if (!ret) {
		udec_param->trickplay_mode = UDEC_TRICKPLAY_STEP;
	}
}

static int _get_udec_state(int iavFd, int udec_id, u32* udec_state, u32* vout_state, u32* error_code)
{
	int ret = 0;
	iav_udec_state_t state;
	memset(&state, 0, sizeof(state));
	state.decoder_id = udec_id;
	state.flags = 0;

	ret = ioctl(iavFd, IAV_IOC_GET_UDEC_STATE, &state);
	u_assert(ret == 0);
	if (ret) {
		perror("IAV_IOC_GET_UDEC_STATE");
		u_printf_error("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
	}

	if (udec_state) {
		*udec_state = state.udec_state;
	}
	if (vout_state) {
		*vout_state = state.vout_state;
	}
	if (error_code) {
		*error_code = state.error_code;
	}

	if (IAV_UDEC_STATE_ERROR == state.udec_state) {
		u_printf("[error handling]: Fatal error comes, error code %08x, udec state %d, vout state %d\n", state.error_code, state.udec_state, state.vout_state);
	}

	return state.udec_state;
}

static int ioctl_playback_speed(int iav_fd, int decoder_id, unsigned int speed, unsigned int speed_frac, unsigned int scan_mode, unsigned int direction)
{
	int ret;
	iav_udec_pb_speed_t speed_t;
	memset(&speed_t, 0x0, sizeof(speed_t));

	speed_t.speed = ((speed & 0xff) << 8) | (speed_frac & 0xff);
	speed_t.decoder_id = decoder_id;
	speed_t.scan_mode = scan_mode;
	speed_t.direction = direction;

	u_printf("[cmd flow]: before IAV_IOC_UDEC_PB_SPEED, decoder_id %d, speed 0x%04x, scan_mode %d, direction %d.\n", speed_t.decoder_id, speed_t.speed, speed_t.scan_mode, speed_t.direction);
	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_PB_SPEED, &speed_t)) < 0) {
		perror("IAV_IOC_UDEC_PB_SPEED");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_UDEC_PB_SPEED.\n");

	return 0;
}

static void send_wake_vout_signal(udec_instance_param_t* udec_param)
{
	msg_t msg;
	if (udec_param) {
		msg.cmd = M_MSG_START;
		msg.ptr = NULL;
		msg_queue_put(&udec_param->renderer_cmd_queue, &msg);
	} else {
		u_printf_error("NULL param in send_wake_vout_signal!!\n");
	}
}

static void __append_delimiter(unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start)
{
	unsigned int write_len = 0;

	if (p_buf_end > (p_start + sizeof(_h264_delimiter))) {
		memcpy(p_start, _h264_delimiter, sizeof(_h264_delimiter));
	} else {
		write_len = p_buf_end - p_start;
		memcpy(p_start, _h264_delimiter, write_len);
		memcpy(p_buf_start, _h264_delimiter + write_len, sizeof(_h264_delimiter) - write_len);
	}
}

static u8* find_start_code_264(u8* p_start, u8* p_end, int key_frame)
{
	u8* p_tmp = p_start;
	unsigned int state = 0;
	unsigned int nal_type = 0;

	while (p_tmp < p_end) {
		switch (state) {
			case 0:
			case 1:
				if (p_tmp[0]) {
					state = 0;
				} else {
					state ++;
				}
				break;
			case 2:
				if (0x1 == p_tmp[0]) {
					state = 3;
				} else if (0x0 != p_tmp[0]) {
					state = 0;
				}
				break;
			case 3:
				nal_type = p_tmp[0]&0x1f;
				u_printf("naltype %d\n", nal_type);
				if (key_frame) {
					if (5 == nal_type) {
						return (p_tmp - 3);
					}
				} else {
					return (p_tmp - 3);
				}
				state = 0;
				break;
			default:
				u_printf_error("must not comes here %d\n", state);
				break;
		}
		p_tmp ++;
	}

	u_printf_error("can not find!!!\n");
	return NULL;
}

static u32 calculate_slice_count(u8* p_start, u8* p_end)
{
	u8* p_tmp = p_start;
	unsigned int state = 0;
	unsigned int nal_type = 0;
	u32 count = 0;

	while (p_tmp < p_end) {
		switch (state) {
			case 0:
			case 1:
				if (p_tmp[0]) {
					state = 0;
				} else {
					state ++;
				}
				break;
			case 2:
				if (0x1 == p_tmp[0]) {
					state = 3;
				} else if (0x0 != p_tmp[0]) {
					state = 0;
				}
				break;
			case 3:
				nal_type = p_tmp[0]&0x1f;
				u_printf("naltype %d\n", nal_type);

				if (5 == nal_type) {
					count ++;
				} else if (nal_type < 5) {
					count ++;
				} else if (7 == nal_type) {
					//sps
				} else if (8 == nal_type) {
					//pps
				} else {
					//others
				}
				state = 0;
				break;
			default:
				u_printf_error("must not comes here %d\n", state);
				break;
		}
		p_tmp ++;
	}

	return count;
}

static int init_file_reader(_t_file_reader* p_reader, FILE* file_fd, unsigned long read_buffer_size)
{
	u_assert(p_reader);
	u_assert(file_fd);

	if (!p_reader || !file_fd) {
		u_printf_error("NULL p_reader(%p) or NULL file_fd(%p)\n", p_reader, file_fd);
		return (-1);
	}

	u_assert(read_buffer_size);
	if (!read_buffer_size) {
		read_buffer_size = 8*MB;
	}

	if (!prealloc_file_buffer) {
		u_assert(!p_reader->b_alloc_read_buffer);
		if (!p_reader->b_alloc_read_buffer) {
			u_assert(!p_reader->p_read_buffer_base);
			p_reader->p_read_buffer_base = malloc(read_buffer_size);
			u_printf("malloc1(%d)\n", read_buffer_size);
			if (p_reader->p_read_buffer_base) {
				p_reader->read_buffer_size = read_buffer_size;
				p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
				p_reader->b_alloc_read_buffer = 1;
			} else {
				u_printf_error("no memory!\n");
				return (-2);
			}
			p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
			p_reader->data_remainning_size_in_buffer = 0;
			u_printf_debug("[init_file_reader]: alloc memory size %lu, addr %p\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base);
		} else if (p_reader->read_buffer_size < read_buffer_size) {
			u_assert(p_reader->p_read_buffer_base);
			if (p_reader->p_read_buffer_base) {
				free(p_reader->p_read_buffer_base);
				p_reader->p_read_buffer_base = NULL;
			}

			p_reader->p_read_buffer_base = malloc(read_buffer_size);
			u_printf("malloc2(%d)\n", read_buffer_size);
			if (p_reader->p_read_buffer_base) {
				p_reader->read_buffer_size = read_buffer_size;
				p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
				p_reader->b_alloc_read_buffer = 1;
			} else {
				u_printf_error("no memory!\n");
				return (-2);
			}
			p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
			p_reader->data_remainning_size_in_buffer = 0;
			u_printf_debug("[init_file_reader]: re-alloc memory size %lu, addr %p\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base);
		} else {
			p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
			p_reader->data_remainning_size_in_buffer = 0;
			u_printf_debug("[init_file_reader]: re-use read buffer %lu, addr %p, request size %lu\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base, read_buffer_size);
		}
	} else {
		p_reader->b_alloc_read_buffer = 0;
		p_reader->read_buffer_size = rd_buffer_size;
		p_reader->p_read_buffer_base = prealloc_file_buffer;
		p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
		p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
		p_reader->data_remainning_size_in_buffer = 0;
	}

	p_reader->fd = file_fd;
	p_reader->b_opened_file = 0;

	fseek(p_reader->fd, 0L, SEEK_END);
	p_reader->file_total_size = ftell(p_reader->fd);
	p_reader->file_remainning_size = p_reader->file_total_size;

	fseek(p_reader->fd, 0L, SEEK_SET);

	return 0;
}

static void deinit_file_reader(_t_file_reader* p_reader)
{
	u_assert(p_reader);

	if (!p_reader) {
		u_printf_error("NULL p_reader(%p)\n", p_reader);
		return;
	}

	if (p_reader->b_alloc_read_buffer) {
		if (p_reader->p_read_buffer_base) {
			free(p_reader->p_read_buffer_base);
			p_reader->b_alloc_read_buffer = 0;
		} else {
			u_printf_error("NULL p_reader->p_read_buffer_base\n");
		}
	}

	return;
}

static void reset_file_reader(_t_file_reader* p_reader)
{
	p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
	p_reader->data_remainning_size_in_buffer = 0;
	p_reader->file_remainning_size = p_reader->file_total_size;

	fseek(p_reader->fd, 0L, SEEK_SET);
}

static int read_file_data_trunk(_t_file_reader* p_reader)
{
	unsigned long size = 0;
	u_assert(p_reader);
	//u_assert(p_reader->b_alloc_read_buffer);
	u_assert(p_reader->p_read_buffer_cur_start);
	u_assert(p_reader->p_read_buffer_cur_end);
	u_assert(p_reader->file_remainning_size);

	u_assert((unsigned long)(p_reader->data_remainning_size_in_buffer + p_reader->p_read_buffer_cur_start) == (unsigned long)(p_reader->p_read_buffer_cur_end));
	u_assert((unsigned long)(p_reader->p_read_buffer_cur_start) >= (unsigned long)(p_reader->p_read_buffer_base));
	u_assert((unsigned long)(p_reader->p_read_buffer_cur_start) < (unsigned long)(p_reader->p_read_buffer_end));
	u_assert((unsigned long)(p_reader->p_read_buffer_cur_end) >= (unsigned long)(p_reader->p_read_buffer_base));
	u_assert((unsigned long)(p_reader->p_read_buffer_cur_end) <= (unsigned long)(p_reader->p_read_buffer_end));

	size = (unsigned long)(p_reader->p_read_buffer_end - p_reader->p_read_buffer_cur_end);

	if (p_reader->file_remainning_size <= size) {
		size = p_reader->file_remainning_size;
		fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
		p_reader->file_remainning_size -= size;
		u_assert(0 == p_reader->file_remainning_size);
		p_reader->data_remainning_size_in_buffer += size;
		p_reader->p_read_buffer_cur_end += size;
		return 1;//file read done

	} else {

		memmove(p_reader->p_read_buffer_base, p_reader->p_read_buffer_cur_start, p_reader->data_remainning_size_in_buffer);
		p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_base;
		p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_cur_start + p_reader->data_remainning_size_in_buffer;
		size = p_reader->p_read_buffer_end - p_reader->p_read_buffer_cur_end;

		if (p_reader->file_remainning_size <= size) {
			size = p_reader->file_remainning_size;
			fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
			p_reader->file_remainning_size -= size;
			u_assert(0 == p_reader->file_remainning_size);
			p_reader->data_remainning_size_in_buffer += size;
			p_reader->p_read_buffer_cur_end += size;
			return 1;//file read done
		}

		fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
		p_reader->file_remainning_size -= size;
		u_assert(0 != p_reader->file_remainning_size);
		p_reader->data_remainning_size_in_buffer += size;
		p_reader->p_read_buffer_cur_end += size;
	}

	return 0;
}

static unsigned int get_next_frame(unsigned char* pstart, unsigned char* pend, u8* p_nal_type, u8* p_slice_type, u8* need_more_data)
{
	unsigned char* pcur = pstart;
	unsigned int state = 0;
	unsigned int is_header = 1;

	unsigned char	nal_type;
	unsigned char	first_mb_in_slice = 0;

	*need_more_data = 0;
	*p_slice_type = 0;
	*p_nal_type = 0;

	while (pcur < pend) {
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
				if ((*pcur) ==0x0A) {
					//eos
					if (is_header) {
						u_printf("eos comes, pcur + 1 - pstart is %d\n", pcur + 1 - pstart);
						*p_nal_type = 0x0A;
						return pcur + 1 - pstart;
					} else {
						if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
							u_printf("before eos in bit-stream, pcur - 4 - pstart is %d\n", pcur - 4 - pstart);
							return pcur - 4 - pstart;
						} else {
							u_printf("before eos in bit-stream, pcur - 3 - pstart is %d\n", pcur - 3 - pstart);
							return pcur - 3 - pstart;
						}
					}
				} else if ((*pcur)) { //nal uint type
					nal_type = (*pcur) & 0x1F;
					//*p_nal_type = nal_type;
					if (nal_type >= 1 && nal_type <= 5) {
						if (!is_header) {
							if (pcur + 16 < pend) {
								//u_printf("1 %p: %02x %02x %02x %02x, %02x %02x %02x %02x\n", pcur, *(pcur -3), *(pcur -2), *(pcur -1), *(pcur), *(pcur + 1), *(pcur + 2), *(pcur + 3), *(pcur + 4));
								simple_get_slice_type_le(pcur + 1, &first_mb_in_slice);

								if (!first_mb_in_slice) {
									if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
										return pcur - 4 - pstart;
									} else {
										return pcur - 3 - pstart;
									}
								} else {
									state = 0;
								}
							} else {
								*need_more_data = 1;
								return 0;
							}
						} else {
							if (pcur + 16 < pend) {
								//u_printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", *(pcur - 4), *(pcur - 3), *(pcur - 2), *(pcur - 1), *(pcur), *(pcur + 1), *(pcur + 2), *(pcur + 3));
								*p_slice_type = simple_get_slice_type_le(pcur + 1, &first_mb_in_slice);
								*p_nal_type = nal_type;
								state = 0;
								is_header = 0;
							} else {
								u_printf_error("must not comes here!\n");
								*need_more_data = 1;
								return 0;
							}
						}
					} else if (nal_type >= 6 && nal_type <= 9) {
						if (!is_header) {
							if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
								return pcur - 4 - pstart;
							} else {
								return pcur - 3 - pstart;
							}
						}
						state = 0;
					} else {
						u_printf_error("why comes here? nal_type %d\n", nal_type);
					}
				} else {
					state = 1;
				}
				pcur ++;
				break;

			default:
				u_printf_error("must not comes here! state %d\n", state);
				state = 0;
				break;
		}
	}

	*need_more_data = 1;
	return 0;
}

static int udec_instance_decode_es_file_264(unsigned int udec_index, int iav_fd, FILE* file_fd, unsigned int udec_type, unsigned char* p_bsb_start, unsigned char* p_bsb_end, unsigned char* p_bsb_cur, msg_queue_t *cmd_queue, udec_instance_param_t* udec_param)
{
	int ret = 0;
	int sendsize = 0;
	//int file_read_finished = 0;
	//playlist_looping = 0;
	unsigned int pes_header_len = 0;

	FILE* p_dump_file = NULL;
	char dump_file_name[MAX_DUMP_FILE_NAME_LEN + 64];
	unsigned int dump_separate_file_index = 0;

	unsigned char *p_frame_start;

	unsigned char nal_type, slice_type, need_more_data, wait_next_key_frame = 0;
	unsigned char udec_stopped = 0;
	unsigned char send_wake_vout = 0;
	unsigned char file_eos = 0;

	unsigned int target_prefetch_count = preset_prefetch_count;
	unsigned int current_prefetch_count = 0;
	unsigned int in_prefetching = enable_prefetch;
	unsigned char* p_prefetching = NULL;

	msg_t msg;

	iav_udec_decode_t dec;

	_t_file_reader file_reader;

	if (!file_fd) {
		u_printf("NULL input file.\n");
		ret = -1;
		goto udec_instance_decode_es_file_264_exit;
	}

	memset(&file_reader, 0x0, sizeof(file_reader));
	ret = init_file_reader(&file_reader, file_fd, rd_buffer_size);
	if (ret < 0) {
		u_printf_error("init_file_reader fail, ret %d\n", ret);
		goto udec_instance_decode_es_file_264_exit;
	}

	if (test_dump_total) {
		snprintf(dump_file_name, MAX_DUMP_FILE_NAME_LEN + 60, test_dump_total_filename, udec_index);
		p_dump_file = fopen(dump_file_name, "wb");
	}

repeat_feeding:
	reset_file_reader(&file_reader);

	read_file_data_trunk(&file_reader);

	while (1) {

		u_printf_flow("[loop flow]: before process cmd\n");

		while (msg_queue_peek(cmd_queue, &msg)) {

			ret = process_cmd(cmd_queue, &msg);

			if (ACTION_QUIT == ret) {
				u_printf("recieve quit cmd, return.\n");
				ret = -5;
				goto udec_instance_decode_es_file_264_exit;
			} else if (ACTION_RESUME  == ret) {
				udec_param->paused = 0;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
			} else if (ACTION_PAUSE  == ret) {
				udec_param->paused = 1;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

				//wait resume, ugly code..
				while (udec_param->paused) {
					u_printf("thread %d paused at 1...\n", udec_index);
					msg_queue_get(cmd_queue, &msg);
					ret = process_cmd(cmd_queue, &msg);
					if (ACTION_QUIT == ret) {
						u_printf("recieve quit cmd, return.\n");
						ret = -5;
						goto udec_instance_decode_es_file_264_exit;
					} else if (ACTION_RESUME == ret) {
						u_printf("thread %d resumed\n", udec_index);
						udec_param->paused = 0;
						//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
						break;
					} else if (ACTION_PAUSE == ret) {
						//udec_param->paused = 1;
					} else {
						u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
					}
				}

			} else if (ACTION_FLUSH  == ret) {
				;
			} else if (ACTION_UPDATE_SPEED == ret) {
				u_printf("[cmd flow(%d)]: update speed, before UDEC_STOP(1).\n", udec_index);
				ioctl_udec_stop(iav_fd, udec_index, 1);
				u_printf("[cmd flow(%d)]: update speed, before UDEC_SPEED(speed = %hx.%hx, scan mode %d, direction %d).\n", udec_index, udec_param->tobe_playback_speed_flushed, udec_param->tobe_playback_speed_frac_flushed, udec_param->tobe_playback_scan_mode_flushed, udec_param->tobe_playback_direction_flushed);
				ioctl_playback_speed(iav_fd, udec_index, udec_param->tobe_playback_speed_flushed, udec_param->tobe_playback_speed_frac_flushed, udec_param->tobe_playback_scan_mode_flushed, udec_param->tobe_playback_direction_flushed);
				udec_param->tobe_playback_strategy =udec_param->tobe_playback_strategy_flushed;
				u_printf("[cmd flow(%d)]: update speed, cmd done, clear iav_status.\n", udec_index);
				ioctl_udec_stop(iav_fd, udec_index, 0xff);
				u_printf("[cmd flow(%d)]: clear related variables.\n", udec_index);

				p_frame_start = p_bsb_start;
				p_bsb_cur = p_bsb_start;
				wait_next_key_frame = 1;
				udec_stopped = 1;
			} else {
				u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
			}

		}

		u_printf_flow("[loop flow]: after process cmd\n");

		sendsize = get_next_frame(file_reader.p_read_buffer_cur_start, file_reader.p_read_buffer_cur_end, &nal_type, &slice_type, &need_more_data);

		if (!sendsize && need_more_data) {
			if (!file_reader.file_remainning_size) {
				u_printf("[loop flow]: file end, feed last frame\n");
				sendsize = file_reader.p_read_buffer_cur_end - file_reader.p_read_buffer_cur_start;
				file_eos = 1;
			} else {
				read_file_data_trunk(&file_reader);
				continue;
			}
		}

		if (mdec_loop) {
			if (0x0A == nal_type) {
				u_assert(5 == sendsize);
				u_assert(5 == file_reader.data_remainning_size_in_buffer);
				file_reader.p_read_buffer_cur_start += sendsize;
				file_reader.data_remainning_size_in_buffer -= sendsize;
				u_assert(file_reader.p_read_buffer_cur_start == file_reader.p_read_buffer_cur_end);
				play_count ++;
				u_printf("[loop flow]: skip eos at the end of file, play_count %d\n", play_count);
				goto repeat_feeding;
			}
		} else {
			if (0x0A == nal_type) {
				u_assert(5 == sendsize);
				if (5 == file_reader.data_remainning_size_in_buffer) {
					file_eos = 1;
				} else {
					u_printf_error("eos in middle of the file, file_reader.data_remainning_size_in_buffer %d?\n", file_reader.data_remainning_size_in_buffer);
				}
			}
		}

		//check if need send
		if ((udec_param->current_playback_strategy != udec_param->tobe_playback_strategy) && (((5 == nal_type) /*|| (2 == slice_type)*/) || (udec_param->current_playback_strategy < udec_param->tobe_playback_strategy))) {
			u_printf("[playback strategy update]: nal_type %d, ori %d, new %d\n", nal_type, udec_param->current_playback_strategy, udec_param->tobe_playback_strategy);
			udec_param->current_playback_strategy = udec_param->tobe_playback_strategy;
		}

		if (wait_next_key_frame && (5 > nal_type)) {
			u_printf("[error_handling]: (udec_id %d) skip frame(nal_type %d, slice_type %d) till IDR\n", udec_index, nal_type, slice_type);
		} else if (mdec_skip_feeding && skip_slice_with_strategy(slice_type, nal_type, udec_param->current_playback_strategy)) {
			u_printf("[debug], skip slice\n");
			if (!wait_next_key_frame) {
				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}
		} else {

			wait_next_key_frame = 0;
			p_frame_start = p_bsb_cur;
			//wrap case
			if (p_frame_start == p_bsb_end) {
				p_frame_start = p_bsb_start;
			}

			u_printf_flow("[loop flow]: before request_bits_fifo, sendsize %d\n", sendsize);
			if (!udec_stopped) {
				ret = request_bits_fifo(iav_fd, udec_index, sendsize + HEADER_SIZE, p_frame_start);
			} else {
				ret = 0;
			}
			u_printf_flow("[loop flow]: after request_bits_fifo\n");

			if (ret < 0) {
				//check if there's udec error
				if (IAV_UDEC_STATE_ERROR == _get_udec_state(iav_fd, udec_index, NULL, NULL, NULL)) {
					//do error handling
					u_printf("[error_handling]: (udec_id %d) stop udec\n", udec_index);
					ioctl_udec_stop(iav_fd, udec_index, 0);
					//reset bsb
					u_printf("[error_handling]: (udec_id %d) stop udec done, reset bsb\n", udec_index);
					p_frame_start = p_bsb_start;
					p_bsb_cur = p_bsb_start;
					wait_next_key_frame = 1;
					udec_stopped = 1;
				} else {
					if (enable_debug_log) {
						u_printf_error("[ioctl error]: (udec_id %d) request_bits_fifo(IAV_IOC_WAIT_DECODER(IAV_WAIT_BITS_FIFO)) error, return %d, exit loop\n", udec_index, ret);
					}
					goto udec_instance_decode_es_file_264_exit;
				}
			} else {

				//feed USEQ/UPES header
				if (add_useq_upes_header && (0x0A != nal_type)) {
					if (!udec_param->seq_header_sent) {
						p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
						udec_param->seq_header_sent = 1;
					}
					pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, sendsize, 0, 0);
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

					udec_param->cur_feeding_pts += udec_param->frame_duration;
				}

				p_bsb_cur = copy_to_bsb(p_bsb_cur, file_reader.p_read_buffer_cur_start, sendsize, p_bsb_start, p_bsb_end);

				file_reader.p_read_buffer_cur_start += sendsize;
				file_reader.data_remainning_size_in_buffer -= sendsize;

				if (test_dump_separate) {
					//u_printf("[debug], dump_separate_file_index %d\n", dump_separate_file_index);
					dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
				}

				if (add_delimiter_at_each_slice) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, _h264_delimiter, sizeof(_h264_delimiter), p_bsb_start, p_bsb_end);
				} else if (append_replace_delimiter_at_each_slice) {
					__append_delimiter(p_bsb_start, p_bsb_end, p_bsb_cur);
				}

				if (test_dump_total && p_dump_file) {
					_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
				}

				if (file_eos) {
					u_assert(!file_reader.data_remainning_size_in_buffer);
					u_assert(file_reader.p_read_buffer_cur_start == file_reader.p_read_buffer_cur_end);
					u_assert(!file_reader.file_remainning_size);
					if (!mdec_loop) {
						if (0x0A != nal_type) {
							u_printf_debug("[feeding flow]: append eos to bsb, nal_type %d\n", nal_type);
							p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);
						} else {
							u_printf_debug("[feeding flow]: bit-stream file already have eos, nal_type %d\n", nal_type);
						}
					}
				}

				memset(&dec, 0, sizeof(dec));
				dec.udec_type = udec_type;
				dec.decoder_id = udec_index;
				dec.u.fifo.start_addr = p_frame_start;
				dec.u.fifo.end_addr = p_bsb_cur;
				dec.num_pics = 1;

				total_feed_frame_count ++;
				if ((framecount_mode == 1) && (total_feed_frame_count > framecount_number)) {
					u_printf("framecount mode done, framecount %d, total_feed_frame_count %d\n", framecount_number, total_feed_frame_count);
					sleep(sleep_time);
					udec_running = 0;
					user_press_quit = 1;
					ret = 0;
					goto udec_instance_decode_es_file_264_exit;
				}
				if (in_prefetching && (target_prefetch_count > current_prefetch_count)) {
					if (!p_prefetching) {
						p_prefetching = p_frame_start;
						u_assert(0 == current_prefetch_count);
						u_printf_debug("[prebuffering start], target_prefetch_count %d, p_prefetching %p\n", target_prefetch_count, p_prefetching);
					}

					current_prefetch_count ++;
					u_printf_debug("[prebuffering]: current_prefetch_count %d, target_prefetch_count %d\n", current_prefetch_count, target_prefetch_count);
					if (target_prefetch_count == current_prefetch_count) {
						u_printf_debug("[prebuffering done]\n");
						in_prefetching = 0;
						dec.u.fifo.start_addr = p_prefetching;
						dec.num_pics = current_prefetch_count;
					}
				}
				if (!in_prefetching) {
					u_printf_flow("[loop flow]: before IAV_IOC_UDEC_DECODE\n");
					if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
						//check if there's udec error
						if (IAV_UDEC_STATE_ERROR == _get_udec_state(iav_fd, udec_index, NULL, NULL, NULL)) {
							//do error handling
							u_printf("[error_handling]: (udec_id %d) stop udec\n", udec_index);
							ioctl_udec_stop(iav_fd, udec_index, 0);
							//reset bsb
							u_printf("[error_handling]: (udec_id %d) stop udec done, reset bsb\n", udec_index);
							p_frame_start = p_bsb_start;
							p_bsb_cur = p_bsb_start;
							wait_next_key_frame = 1;
							udec_stopped = 1;
						} else {
							u_printf_error("[ioctl error]: (udec_id %d) IAV_IOC_UDEC_DECODE ret %d, exit loop\n", udec_index, ret);
							ret = -11;
							goto udec_instance_decode_es_file_264_exit;
						}
					} else {
						if (!send_wake_vout) {
							send_wake_vout = 1;
							u_printf_debug("[flow]: before singal render to wait dormant state 1\n");
							send_wake_vout_signal(udec_param);
							u_printf_debug("[flow]: after singal render to wait dormant state 1\n");
						}
						udec_stopped = 0;
					}

					u_printf_flow("[loop flow]: after IAV_IOC_UDEC_DECODE\n");
				}

				if (file_eos) {
					u_printf_debug("eos here, mdec_loop %d, playlist_mode %d\n", mdec_loop, playlist_mode);
					file_eos = 0;
					if (mdec_loop) {
						if (EPlayListMode_ExitUDECMode != playlist_mode) {
							play_count ++;
							u_printf_debug("[feeding flow]: rewind to file header, play_count %d\n", play_count);
							goto repeat_feeding;
						} else {
							u_printf_debug("[feeding flow]: playlist mode %d\n", playlist_mode);
							break;
						}
					} else {
						u_printf_debug("[feeding flow]: exit feeding loop\n");
						break;
					}
				}
			}

		}

	}

udec_instance_decode_es_file_264_exit:

	u_printf_debug("[feeding flow]: udec_instance_decode_es_file_264 end, before fclose(p_dump_file)\n");

	if (p_dump_file) {
		fclose(p_dump_file);
	}

	u_printf_debug("[feeding flow]: udec_instance_decode_es_file_264 end, before deinit_file_reader()\n");

	deinit_file_reader(&file_reader);

	u_printf_debug("[feeding flow]: udec_instance_decode_es_file_264 end\n");

	if (EPlayListMode_ExitUDECMode == playlist_mode) {
		//udec_running = 0;
	}

	return ret;
}

static int udec_instance_decode_es_file_264_playlist(unsigned int udec_index, int iav_fd, unsigned int udec_type, unsigned char* p_bsb_start, unsigned char* p_bsb_end, unsigned char* p_bsb_cur, msg_queue_t *cmd_queue, udec_instance_param_t* udec_param)
{
	int ret = 0;
	int sendsize = 0;
	//int file_read_finished = 0;

	unsigned int pes_header_len = 0;
	int cur_file_index = 0;
	int tot_file_number = current_file_index + 1;
	FILE* file_fd = NULL;

	unsigned char *p_frame_start;

	unsigned char nal_type, slice_type, need_more_data, wait_next_key_frame = 0;
	unsigned char udec_stopped = 0;
	unsigned char send_wake_vout = 0;
	unsigned char file_eos = 0;

	unsigned char re_init_udec = 0;
	unsigned char first_loop = 1;
	unsigned char re_stop_udec = 0;

	msg_t msg;

	iav_udec_decode_t dec;

	_t_file_reader file_reader;
	iav_udec_info_ex_t info;

	u_assert(tot_file_number);

	if (EPlayListMode_ReSetupUDEC == playlist_mode) {
		re_init_udec = 1;
		re_stop_udec = 1;
	} else if (EPlayListMode_StopUDEC == playlist_mode) {
		re_init_udec = 0;
		re_stop_udec = 1;
	} else if (EPlayListMode_Seamless == playlist_mode) {
		re_init_udec = 0;
		re_stop_udec = 0;
	} else {
		u_printf_error("bad playlist_mode %d\n", playlist_mode);
		re_init_udec = 0;
		re_stop_udec = 0;
	}

playlist_start:

	if (!first_loop) {

		if (re_stop_udec) {
			//pending in renderer thread
			msg.cmd = M_MSG_PENDDING;
			msg.ptr = NULL;
			u_printf("[flow]: pending in renderer thread.\n", udec_param->udec_index);
			msg_queue_put(&udec_param->renderer_cmd_queue, &msg);

			//stop udec if needed
			u_printf("[flow]: before ioctl_udec_stop(%d, 1).\n", udec_param->udec_index);
			ioctl_udec_stop(udec_param->iav_fd, udec_param->udec_index, 1);

			u_printf("[flow]: before ioctl_udec_stop(%d, 0xff).\n", udec_param->udec_index);
			ioctl_udec_stop(iav_fd, udec_index, 0xff);

			p_frame_start = p_bsb_start;
			p_bsb_cur = p_bsb_start;
			wait_next_key_frame = 1;
			udec_stopped = 0;
			send_wake_vout = 0;
			file_eos = 0;
		}

		if (re_init_udec) {
			//release udec
			u_printf("[flow]: before IAV_IOC_RELEASE_UDEC(%d).\n", udec_param->udec_index);
			if (ioctl(udec_param->iav_fd, IAV_IOC_RELEASE_UDEC, udec_param->udec_index) < 0) {
				perror("IAV_IOC_RELEASE_UDEC");
				u_printf_error("release udec instance(%d) fail.\n");
			}

			u_printf("[flow]: udec instance(%d) before re-init.\n", udec_param->udec_index);

			//init udec instance
			memset(&info, 0x0, sizeof(info));
			ret = ioctl_init_udec_instance(&info, udec_param->iav_fd, udec_param->p_vout_config, udec_param->num_vout, udec_param->udec_index, udec_param->request_bsb_size, udec_param->udec_type, 0, enable_error_handling, file_video_width[cur_file_index], file_video_height[cur_file_index]);
			if (ret < 0) {
				u_printf("[error]: ioctl_init_udec_instance fail.\n");
				goto udec_instance_decode_es_file_264_playlist_exit;
			}
		}

		if (re_stop_udec) {
			msg.cmd = M_MSG_RESTART;
			msg.ptr = NULL;
			u_printf("[flow]: restart in renderer thread.\n", udec_param->udec_index);
			msg_queue_put(&udec_param->renderer_cmd_queue, &msg);
		}
	} else {
		first_loop = 0;
	}

	udec_param->seq_header_sent = 0;
	wait_next_key_frame = 1;

	file_fd = fopen(file_list[cur_file_index], "rb");

	if (!file_fd) {
		u_printf("NULL input file.\n");
		ret = 9;
		goto udec_instance_decode_es_file_264_playlist_exit;
	}

	memset(&file_reader, 0x0, sizeof(file_reader));
	ret = init_file_reader(&file_reader, file_fd, rd_buffer_size);
	if (ret < 0) {
		u_printf_error("init_file_reader fail, ret %d\n", ret);
		goto udec_instance_decode_es_file_264_playlist_exit;
	}

	reset_file_reader(&file_reader);

	read_file_data_trunk(&file_reader);

	while (1) {

		u_printf_flow("[loop flow]: before process cmd\n");

		while (msg_queue_peek(cmd_queue, &msg)) {

			ret = process_cmd(cmd_queue, &msg);

			if (ACTION_QUIT == ret) {
				u_printf("recieve quit cmd, return.\n");
				ret = -5;
				goto udec_instance_decode_es_file_264_playlist_exit;
			} else if (ACTION_RESUME  == ret) {
				udec_param->paused = 0;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
			} else if (ACTION_PAUSE  == ret) {
				udec_param->paused = 1;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

				//wait resume, ugly code..
				while (udec_param->paused) {
					u_printf("thread %d paused at 1...\n", udec_index);
					msg_queue_get(cmd_queue, &msg);
					ret = process_cmd(cmd_queue, &msg);
					if (ACTION_QUIT == ret) {
						u_printf("recieve quit cmd, return.\n");
						ret = -5;
						goto udec_instance_decode_es_file_264_playlist_exit;
					} else if (ACTION_RESUME == ret) {
						u_printf("thread %d resumed\n", udec_index);
						udec_param->paused = 0;
						//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
						break;
					} else if (ACTION_PAUSE == ret) {
						//udec_param->paused = 1;
					} else {
						u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
					}
				}

			} else if (ACTION_FLUSH  == ret) {
				;
			} else if (ACTION_UPDATE_SPEED == ret) {
				u_printf("[cmd flow(%d)]: update speed, before UDEC_STOP(1).\n", udec_index);
				ioctl_udec_stop(iav_fd, udec_index, 1);
				u_printf("[cmd flow(%d)]: update speed, before UDEC_SPEED(speed = %hx.%hx, scan mode %d, direction %d).\n", udec_index, udec_param->tobe_playback_speed_flushed, udec_param->tobe_playback_speed_frac_flushed, udec_param->tobe_playback_scan_mode_flushed, udec_param->tobe_playback_direction_flushed);
				ioctl_playback_speed(iav_fd, udec_index, udec_param->tobe_playback_speed_flushed, udec_param->tobe_playback_speed_frac_flushed, udec_param->tobe_playback_scan_mode_flushed, udec_param->tobe_playback_direction_flushed);
				udec_param->tobe_playback_strategy =udec_param->tobe_playback_strategy_flushed;
				u_printf("[cmd flow(%d)]: update speed, cmd done, clear iav_status.\n", udec_index);
				ioctl_udec_stop(iav_fd, udec_index, 0xff);
				u_printf("[cmd flow(%d)]: clear related variables.\n", udec_index);

				p_frame_start = p_bsb_start;
				p_bsb_cur = p_bsb_start;
				wait_next_key_frame = 1;
				udec_stopped = 1;
			} else {
				u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
			}

		}

		u_printf_flow("[loop flow]: after process cmd\n");

		sendsize = get_next_frame(file_reader.p_read_buffer_cur_start, file_reader.p_read_buffer_cur_end, &nal_type, &slice_type, &need_more_data);

		if (!sendsize && need_more_data) {
			if (!file_reader.file_remainning_size) {
				u_printf_flow("[loop flow]: file end, last frame\n");
				sendsize = file_reader.p_read_buffer_cur_end - file_reader.p_read_buffer_cur_start;
				file_eos = 1;
			} else {
				read_file_data_trunk(&file_reader);
				continue;
			}
		}

		//check if need send
		if ((udec_param->current_playback_strategy != udec_param->tobe_playback_strategy) && (((5 == nal_type) /*|| (2 == slice_type)*/) || (udec_param->current_playback_strategy < udec_param->tobe_playback_strategy))) {
			u_printf("[playback strategy update]: nal_type %d, ori %d, new %d\n", nal_type, udec_param->current_playback_strategy, udec_param->tobe_playback_strategy);
			udec_param->current_playback_strategy = udec_param->tobe_playback_strategy;
		}

		if (wait_next_key_frame && (5 > nal_type)) {
			u_printf("[error_handling]: (udec_id %d) skip frame(nal_type %d, slice_type %d) till IDR\n", udec_index, nal_type, slice_type);
		} else if (mdec_skip_feeding && skip_slice_with_strategy(slice_type, nal_type, udec_param->current_playback_strategy)) {
			u_printf("[debug], skip slice\n");
			if (!wait_next_key_frame) {
				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}
		} else {

			wait_next_key_frame = 0;
			p_frame_start = p_bsb_cur;
			//wrap case
			if (p_frame_start == p_bsb_end) {
				p_frame_start = p_bsb_start;
			}

			u_printf_flow("[loop flow]: before request_bits_fifo, sendsize %d\n", sendsize);
			if (!udec_stopped) {
				ret = request_bits_fifo(iav_fd, udec_index, sendsize + HEADER_SIZE, p_frame_start);
			} else {
				ret = 0;
			}
			u_printf_flow("[loop flow]: after request_bits_fifo\n");

			if (ret < 0) {
				//check if there's udec error
				if (IAV_UDEC_STATE_ERROR == _get_udec_state(iav_fd, udec_index, NULL, NULL, NULL)) {
					//do error handling
					u_printf("[error_handling]: (udec_id %d) stop udec\n", udec_index);
					ioctl_udec_stop(iav_fd, udec_index, 0);
					//reset bsb
					u_printf("[error_handling]: (udec_id %d) stop udec done, reset bsb\n", udec_index);
					p_frame_start = p_bsb_start;
					p_bsb_cur = p_bsb_start;
					wait_next_key_frame = 1;
					udec_stopped = 1;
				} else {
					u_printf_error("[ioctl error]: (udec_id %d) request_bits_fifo(IAV_IOC_WAIT_DECODER(IAV_WAIT_BITS_FIFO)) error, return %d, exit loop\n", udec_index, ret);

					ret = -10;
					goto udec_instance_decode_es_file_264_playlist_exit;
				}
			} else {

				//feed USEQ/UPES header
				if (add_useq_upes_header) {
					if (!udec_param->seq_header_sent) {
						p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
						udec_param->seq_header_sent = 1;
					}
					pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, sendsize, 0, 0);
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

					udec_param->cur_feeding_pts += udec_param->frame_duration;
				}

				p_bsb_cur = copy_to_bsb(p_bsb_cur, file_reader.p_read_buffer_cur_start, sendsize, p_bsb_start, p_bsb_end);

				file_reader.p_read_buffer_cur_start += sendsize;
				file_reader.data_remainning_size_in_buffer -= sendsize;

				if (add_delimiter_at_each_slice) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, _h264_delimiter, sizeof(_h264_delimiter), p_bsb_start, p_bsb_end);
				} else if (append_replace_delimiter_at_each_slice) {
					__append_delimiter(p_bsb_start, p_bsb_end, p_bsb_cur);
				}

				if (file_eos) {
					u_assert(!file_reader.data_remainning_size_in_buffer);
					u_assert(file_reader.p_read_buffer_cur_start == file_reader.p_read_buffer_cur_end);
					u_assert(!file_reader.file_remainning_size);
					if (!mdec_loop) {
						u_printf("[feeding flow]: append eos to bsb\n");
						p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);
					}
				}

				memset(&dec, 0, sizeof(dec));
				dec.udec_type = udec_type;
				dec.decoder_id = udec_index;
				dec.u.fifo.start_addr = p_frame_start;
				dec.u.fifo.end_addr = p_bsb_cur;
				dec.num_pics = 1;

				u_printf_flow("[loop flow]: before IAV_IOC_UDEC_DECODE\n");
				total_feed_frame_count ++;
				if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
					//check if there's udec error
					if (IAV_UDEC_STATE_ERROR == _get_udec_state(iav_fd, udec_index, NULL, NULL, NULL)) {
						//do error handling
						u_printf("[error_handling]: (udec_id %d) stop udec\n", udec_index);
						ioctl_udec_stop(iav_fd, udec_index, 0);
						//reset bsb
						u_printf("[error_handling]: (udec_id %d) stop udec done, reset bsb\n", udec_index);
						p_frame_start = p_bsb_start;
						p_bsb_cur = p_bsb_start;
						wait_next_key_frame = 1;
						udec_stopped = 1;
					} else {
						u_printf_error("[ioctl error]: (udec_id %d) IAV_IOC_UDEC_DECODE ret %d, exit loop\n", udec_index, ret);
						ret = -11;
						goto udec_instance_decode_es_file_264_playlist_exit;
					}
				} else {
					if (!send_wake_vout) {
						send_wake_vout = 1;
						u_printf("[flow]: before singal render to wait dormant state 1\n");
						send_wake_vout_signal(udec_param);
						u_printf("[flow]: after singal render to wait dormant state 1\n");
					}
					udec_stopped = 0;
				}

				u_printf_flow("[loop flow]: after IAV_IOC_UDEC_DECODE\n");

				if (file_eos) {
					file_eos = 0;

					deinit_file_reader(&file_reader);
					if (file_fd) {
						fclose(file_fd);
						file_fd = NULL;
					}

					cur_file_index ++;
					if (cur_file_index >= tot_file_number) {
						if (!mdec_loop) {
							u_printf("[feeding flow]: exit playlist, [--noloop]\n");
							break;
						}
						cur_file_index = 0; //cur_file_index % tot_file_number;
					}
					u_printf("[feeding flow]: rewind to file header cur file index %d, tot_file_number %d\n", cur_file_index, tot_file_number);
					goto playlist_start;
				}
			}

		}

	}

	u_printf("[feeding flow]: udec_instance_decode_es_file_264_playlist end, before deinit_file_reader()\n");

udec_instance_decode_es_file_264_playlist_exit:

	deinit_file_reader(&file_reader);
	if (file_fd) {
		fclose(file_fd);
		file_fd = NULL;
	}

	u_printf("[feeding flow]: udec_instance_decode_es_file_264_playlist end\n");
	return ret;
}

static int udec_instance_decode_es_test_file_oneshot(unsigned int udec_index, int iav_fd, FILE* file_fd, unsigned int udec_type, unsigned char* p_bsb_start, unsigned char* p_bsb_end, unsigned char* p_bsb_cur, msg_queue_t *cmd_queue, udec_instance_param_t* udec_param, unsigned int one_shot_size)
{
	int ret = 0;

	unsigned char *p_frame_end;
	iav_udec_decode_t dec;

	//read es file
	unsigned int bsb_size = p_bsb_end - p_bsb_start;
	unsigned int file_size = 0;
	unsigned int tot_read_size = 0;
	unsigned int request_send_size = 0;

	unsigned int send_whole_file = 0;
	unsigned int tot_frame_count = 0;

	FILE* pFile = file_fd;

	if (!pFile) {
		u_printf_error("NULL input file.\n");
		return (9);
	}

	fseek(pFile, 0L, SEEK_END);
	file_size = ftell(pFile);

	u_printf("[one shot test]: file size 0x%x, bsb_size 0x%x, one_shot_size 0x%x\n", file_size, bsb_size, one_shot_size);

	if (file_size <= bsb_size) {
		if (0 == one_shot_size || one_shot_size > file_size) {
			send_whole_file = 1;
			request_send_size = file_size;
		} else {
			send_whole_file = 0;
			request_send_size = one_shot_size;
		}
		tot_read_size = file_size;
	} else {
		send_whole_file = 0;
		if ((0 == one_shot_size) || (one_shot_size >= (bsb_size + 1024*128))) {
			request_send_size = bsb_size/2;
		} else {
			request_send_size = one_shot_size;
		}
		tot_read_size = bsb_size;
	}

	fseek(pFile, 0L, SEEK_SET);

	ret = request_bits_fifo(iav_fd, udec_index, bsb_size - 2048, p_bsb_start);

	fread(p_bsb_start, 1, tot_read_size, pFile);

	if (!send_whole_file) {
		p_frame_end = find_start_code_264(p_bsb_start + request_send_size, p_bsb_start + tot_read_size, 0);
		if (!p_frame_end) {
			u_printf_error("cannot find boundary, request_send_size 0x%x, tot_read_size 0x%x.\n", request_send_size, tot_read_size);
			return (-11);
		}

	} else {
		p_frame_end = p_bsb_start + tot_read_size;
	}

	tot_frame_count = calculate_slice_count(p_bsb_start, p_frame_end);

	if (p_frame_end + 6 < p_bsb_end) {
		memcpy(p_frame_end, _h264_eos, 5);
		p_frame_end += 5;
	}

	memset(&dec, 0, sizeof(dec));
	dec.udec_type = udec_type;
	dec.decoder_id = udec_index;
	dec.u.fifo.start_addr = p_bsb_start;
	dec.u.fifo.end_addr = p_frame_end;
	dec.num_pics = tot_frame_count;

	u_printf("[one shot test]: decode %d frames, bsb [%p %p], p_frame_end %p, data size 0x%x\n", tot_frame_count, p_bsb_start, p_bsb_end, p_frame_end - p_bsb_start);
	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
		u_printf_error("[ioctl error]: (udec_id %d) IAV_IOC_UDEC_DECODE ret %d, exit loop\n", udec_index, ret);
		return (-12);
	}

	u_printf("[one shot test]: before singal render to wait dormant state 1\n");
	send_wake_vout_signal(udec_param);
	u_printf("[one shot test]: after singal render to wait dormant state 1\n");

	return 0;
}

static int ioctl_wake_vout(int iav_fd, int udec_id)
{
	iav_wake_vout_t wake_vout;
	memset(&wake_vout, 0, sizeof(wake_vout));
	wake_vout.decoder_id = udec_id;

	u_printf_debug("[flow renderer]: before IAV_IOC_WAKE_VOUT, udec_id %d\n", udec_id);
	if (ioctl(iav_fd, IAV_IOC_WAKE_VOUT, &wake_vout) < 0) {
		perror("IAV_IOC_WAKE_VOUT");
		u_printf_error("[flow renderer]: IAV_IOC_WAKE_VOUT fail, udec_id %d\n", udec_id);
		return -1;
	}
	u_printf_debug("[flow renderer]: IAV_IOC_WAKE_VOUT done, udec_id %d\n", udec_id);

	return 0;
}

enum {
	ERendererState_idle = 0,
	ERendererState_wait_vout_dormant,
	ERendererState_wake_vout,
	ERendererState_wait_eos,
	ERendererState_pending,
	ERendererState_error,
};

static int vout_wait_dormant(int iav_fd, unsigned int udec_id)
{
	int ret;
	iav_udec_state_t state;
	iav_wait_decoder_t wait;

	memset(&state, 0, sizeof(state));
	state.decoder_id = udec_id;
	state.flags = 0;

	ret = ioctl(iav_fd, IAV_IOC_GET_UDEC_STATE, &state);
	if (ret) {
		perror("IAV_IOC_GET_UDEC_STATE");
		u_printf_error("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
		return (-7);
	}

	if ((IAV_UDEC_STATE_RUN != state.udec_state) && (IAV_UDEC_STATE_READY != state.udec_state)) {
		u_printf_error("[renderer error]: udec state is (%u), not in run or ready state!!!\n", state.udec_state);
		return (-1);
	}

	if (IAV_VOUT_STATE_PRE_RUN != state.vout_state) {
		if (IAV_VOUT_STATE_DORMANT == state.vout_state) {
			u_printf_debug("[flow renderer]: udec state %u, vout state %d, vout in dormant now\n", state.udec_state, state.vout_state);
			return 0;
		} else if (IAV_VOUT_STATE_RUN == state.vout_state) {
			u_printf_debug("[flow renderer]: udec state %u, vout state %d, vout in run now\n", state.udec_state, state.vout_state);
			return 1;
		}
		u_printf_error("[renderer error]: vout state is (%u), not in pre_run state!!!\n", state.vout_state);
	}

	memset(&wait, 0x0, sizeof(wait));
	wait.decoder_id = udec_id;
	wait.flags = IAV_WAIT_VOUT_STATE | IAV_WAIT_VDSP_INTERRUPT;
	ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODER, &wait);

	if (0 > ret) {
		perror("IAV_IOC_WAIT_DECODER");
		u_printf_error("[renderer error]: IAV_IOC_WAIT_DECODER(%d) fail ret %d\n", udec_id, ret);
		return (-2);
	}

	if (IAV_WAIT_UDEC_ERROR == wait.flags) {
		u_printf_error("[renderer error]: UDEC error?!!\n");
		return (-5);
	} else if (IAV_WAIT_VDSP_INTERRUPT == wait.flags) {
		return (-4);
	} else if (IAV_WAIT_VOUT_STATE == wait.flags) {
		u_printf_debug("[flow renderer]: vout state changed\n");
		memset(&state, 0, sizeof(state));
		state.decoder_id = udec_id;
		state.flags = 0;
		ret = ioctl(iav_fd, IAV_IOC_GET_UDEC_STATE, &state);
		if (ret) {
			perror("IAV_IOC_GET_UDEC_STATE");
			u_printf_error("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
			return (-6);
		}
		if (IAV_VOUT_STATE_DORMANT == state.vout_state) {
			u_printf_debug("[flow renderer]: udec state %u, vout state %d, vout in dormant now\n", state.udec_state, state.vout_state);
			return 0;
		} else if (IAV_VOUT_STATE_RUN == state.vout_state) {
			u_printf_debug("[flow renderer]: udec state %u, vout state %d, vout in run now\n", state.udec_state, state.vout_state);
			return 1;
		}
		u_printf_debug("[flow renderer]: vout state is (%u), not in dormant or run!!!\n", state.vout_state);
	} else {
		u_printf_error("[renderer error]: why comes here, BAD flag 0x%x\n", wait.flags);
	}
	return (-8);
}

static int vout_wait_eos_ppmode2(int iav_fd, unsigned int udec_id, unsigned int* last_pts_flag_comes, unsigned int* last_pts_high, unsigned int* last_pts_low)
{
	int ret = 0;
	iav_udec_status_t status;

	//wait udec status
	memset(&status, 0, sizeof(status));
	status.decoder_id = udec_id;
	status.nowait = 0;

	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, &status)) < 0) {
		perror("IAV_IOC_WAIT_UDEC_STATUS");
		u_printf_error("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
		return (-1);
	}

	if (IAV_VOUT_STATE_RUN != status.vout_state) {
		return (4);
	}

	if (*last_pts_flag_comes) {
		if ((status.pts_high == (*last_pts_high)) && (status.pts_low == (*last_pts_low))) {
			u_printf_debug("[flow renderer]: wait last pts done!, high %u, low %u.\n", status.pts_high, status.pts_low);
			return 0;
		} else {
			*last_pts_high = status.pts_high;
			*last_pts_low = status.pts_low;
			return 2;
		}
	} else if (status.eos_flag) {
		*last_pts_flag_comes = 1;
		*last_pts_high = status.pts_high;
		*last_pts_low = status.pts_low;
		u_printf_debug("[flow renderer]: last pts flag comes, high %u, low %u.\n", status.pts_high, status.pts_low);
	} else {
		*last_pts_high = status.pts_high;
		*last_pts_low = status.pts_low;
	}
	return 1;
}

static void* udec_renderer_ppmode2(void* param)
{
	int renderer_state = ERendererState_idle;
	int running = 1;
	udec_instance_param_t* udec_param = (udec_instance_param_t*)param;
	msg_queue_t* msg_queue = NULL;
	msg_t msg;
	int ret = 0;
	int timeout_count = 30;
	unsigned int last_pts_comes = 0;
	unsigned int last_pts_high = 0, last_pts_low = 0;
	unsigned int last_pts_high_reserved = 0, last_pts_low_reserved = 0;
	unsigned int from_pasued = 2;

	u_printf_debug("[flow renderer]: start thread\n");

	if (!param) {
		u_printf_error("NULL pointer in udec_renderer_ppmode2!!\n");
		return NULL;
	}
	msg_queue = &udec_param->renderer_cmd_queue;

	while (running) {
		//u_printf("[flow renderer state]: renderer_state %d\n", renderer_state);
		switch (renderer_state) {

			case ERendererState_idle:
				msg_queue_get(msg_queue, &msg);
				if (M_MSG_START == msg.cmd) {
					renderer_state = ERendererState_wait_vout_dormant;
					u_printf_debug("[flow (renderer)]: start wait vout dormant state\n");
				} else if (M_MSG_PENDDING == msg.cmd) {
					renderer_state = ERendererState_pending;
					u_printf_debug("[flow (renderer)]: pending from idle!\n");
				} else if (M_MSG_KILL == msg.cmd) {
					running = 0;
					u_printf_debug("[flow (renderer)]: exit in ERendererState_idle\n");
				}
				break;

			case ERendererState_wait_vout_dormant:
				//process msg
				while (msg_queue_peek(msg_queue, &msg)) {
					if (M_MSG_KILL == msg.cmd) {
						running = 0;
						u_printf_debug("[flow (renderer)]: exit in ERendererState_wait_vout_dormant\n");
						break;
					} else if (M_MSG_PENDDING == msg.cmd) {
						renderer_state = ERendererState_pending;
						u_printf_debug("[flow (renderer)]: pending from wait_dormant!\n");
					} else {
						u_printf_error("[flow (renderer)]: not processed cmd %d\n", msg.cmd);
					}
				}
				if ((!running) || (renderer_state != ERendererState_wait_vout_dormant)) {
					u_printf_debug("[flow (renderer)]: exit in ERendererState_wait_vout_dormant\n");
					break;
				}

				u_printf_debug("[flow renderer state]: before vout_wait_dormant\n");
				ret = vout_wait_dormant(udec_param->iav_fd, udec_param->udec_index);
				u_printf_debug("[flow renderer state]: after vout_wait_dormant\n");
				if (0 == ret) {
					renderer_state = ERendererState_wait_eos;
					u_printf("[flow (renderer)]: wait vout dormant state done\n");
					ioctl_wake_vout(udec_param->iav_fd, udec_param->udec_index);
					gettimeofday(&begin_playback_time, NULL);
					u_printf_debug("[playback time]: begin playback, time: %d s, %d us\n", begin_playback_time.tv_sec, begin_playback_time.tv_usec);
				} else if (0 > ret) {
					usleep(20000);
					u_printf_debug("[flow renderer state]: after sleep, vout_wait_dormant\n");
				} else {
					renderer_state = ERendererState_wait_eos;
					u_printf_debug("[flow (renderer)]: vout already in run state??\n");
				}
				break;

			case ERendererState_wait_eos:
				//process msg
				while (msg_queue_peek(msg_queue, &msg)) {
					if (M_MSG_KILL == msg.cmd) {
						running = 0;
						u_printf_debug("[flow (renderer)]: exit in ERendererState_wait_eos\n");
						break;
					} else if (M_MSG_PENDDING == msg.cmd) {
						renderer_state = ERendererState_pending;
						u_printf_debug("[flow (renderer)]: pending from wait_eos!\n");
					} else {
						u_printf_error("[flow (renderer)]: not processed cmd %d\n", msg.cmd);
					}
				}
				if ((!running) || (renderer_state != ERendererState_wait_eos)) {
					u_printf_debug("[flow (renderer)]: exit in ERendererState_wait_eos\n");
					break;
				}

				ret = vout_wait_eos_ppmode2(udec_param->iav_fd, udec_param->udec_index, &last_pts_comes, &last_pts_high, &last_pts_low);
				if (0 > ret) {
					renderer_state = ERendererState_pending;
					gettimeofday(&end_playback_time, NULL);
					u_printf("[playback time]: end playback time: %d s, %d us\n", end_playback_time.tv_sec, end_playback_time.tv_usec);
					if (begin_playback_time.tv_usec > end_playback_time.tv_usec) {
						u_printf("[playback time]: total playback time %d s, %d us\n", end_playback_time.tv_sec - begin_playback_time.tv_sec - 1, 1000000 + end_playback_time.tv_usec - begin_playback_time.tv_usec);
						u_printf("[playback time]: total frame count %d, estimated fps %f\n", total_feed_frame_count + 1, (float)(total_feed_frame_count + 1)/((float)(end_playback_time.tv_sec - begin_playback_time.tv_sec - 1) + (float)(1000000 + end_playback_time.tv_usec - begin_playback_time.tv_usec)/1000000));
					} else {
						u_printf("[playback time]: total playback time %d s, %d us\n", end_playback_time.tv_sec - begin_playback_time.tv_sec, end_playback_time.tv_usec - begin_playback_time.tv_usec);
						u_printf("[playback time]: total frame count %d, estimated fps %f\n", total_feed_frame_count + 1, (float)(total_feed_frame_count + 1)/((float)(end_playback_time.tv_sec - begin_playback_time.tv_sec) + (float)(end_playback_time.tv_usec - begin_playback_time.tv_usec)/1000000));
					}
					u_printf("[flow (renderer)]: eos comes, playback finished\n");
				} else if (1 == ret) {
					if (!last_pts_comes) {
						//u_printf("[flow renderer]: wait last pts flag, current high %u, low %u.\n", last_pts_high, last_pts_low);
						if ((2 == from_pasued) && (0 != last_pts_low) && (last_pts_high == last_pts_high_reserved) && (last_pts_low == last_pts_low_reserved)) {
							//if (timeout_count > 0) {
								//u_printf("[flow renderer]: wait last pts repeat, count %d, high %u, low %u.\n", timeout_count, last_pts_high, last_pts_low);
								//usleep(100000);
								//timeout_count --;
							//} else {
							//if (!udec_param->paused) {
								u_printf("[flow renderer]: playback end, pts high %u, low %u.\n", last_pts_high, last_pts_low);
								running = 0;
								msg.cmd = M_MSG_EOS;
								msg.ptr = NULL;
								u_printf_debug("[flow]: send eos to udec thread.\n", udec_param->udec_index);
								msg_queue_put(&udec_param->cmd_queue, &msg);
								u_printf_debug("[flow]: send eos to udec thread done.\n", udec_param->udec_index);
							//}
							//}
						} else {
							usleep(100000);
						}
						if (from_pasued < 2) {
							from_pasued ++;
						}
						last_pts_high_reserved = last_pts_high;
						last_pts_low_reserved = last_pts_low;
					} else {
						//comes
						//u_printf("[flow renderer]: last pts flag comes, high %u, low %u.\n", last_pts_high, last_pts_low);
					}
					break;
				} else if (2 == ret) {
					if (timeout_count > 0) {
						u_printf_debug("[flow renderer]: wait last pts, count %d, high %u, low %u.\n", timeout_count, last_pts_high, last_pts_low);
						usleep(100000);
						timeout_count --;
					} else {
						u_printf_debug("[flow renderer]: timeout, high %u, low %u.\n", last_pts_high, last_pts_low);
						running = 0;
						msg.cmd = M_MSG_EOS;
						msg.ptr = NULL;
						u_printf_debug("[flow]: send eos to udec thread.\n", udec_param->udec_index);
						msg_queue_put(&udec_param->cmd_queue, &msg);
						u_printf_debug("[flow]: send eos to udec thread done.\n", udec_param->udec_index);
					}
				} else if (0 == ret) {
					u_printf("[flow renderer]: last pts flag comes, high %u, low %u.\n", last_pts_high, last_pts_low);
					running = 0;
					msg.cmd = M_MSG_EOS;
					msg.ptr = NULL;
					u_printf_debug("[flow]: send eos to udec thread.\n", udec_param->udec_index);
					msg_queue_put(&udec_param->cmd_queue, &msg);
					u_printf_debug("[flow]: send eos to udec thread done.\n", udec_param->udec_index);
				} else if (4 == ret) {
					//u_printf("[flow]: not in run state, paused state?.\n");
					from_pasued = 0;
				} else {
					u_printf_error("bad ret %d\n", ret);
					running = 0;
					msg.cmd = M_MSG_EOS;
					msg.ptr = NULL;
					u_printf_debug("[flow]: send eos to udec thread.\n", udec_param->udec_index);
					msg_queue_put(&udec_param->cmd_queue, &msg);
					u_printf_debug("[flow]: send eos to udec thread done.\n", udec_param->udec_index);
				}
				break;

			case ERendererState_pending:
			case ERendererState_error:
				msg_queue_get(msg_queue, &msg);
				if (M_MSG_KILL == msg.cmd) {
					running = 0;
					u_printf_debug("[flow (renderer)]: exit in ERendererState_idle\n");
					msg.cmd = M_MSG_EOS;
					msg.ptr = NULL;
					u_printf_error("[flow]: send eos to udec thread.\n", udec_param->udec_index);
					msg_queue_put(&udec_param->cmd_queue, &msg);
					u_printf_error("[flow]: send eos to udec thread done.\n", udec_param->udec_index);
				} else if (M_MSG_RESTART== msg.cmd) {
					if (ERendererState_pending == renderer_state) {
						u_printf_debug("[flow (renderer)]: restart...\n");
						renderer_state = ERendererState_idle;
					}
				}
				break;

			default:
				u_printf_error("[renderer error]: BAD state %d\n", renderer_state);
				renderer_state = ERendererState_error;
				break;

		}
	}

	u_printf_debug("[flow renderer]: thread exit\n");

	return NULL;
}

static int __need_into_play_es_file_264_playlist(unsigned char play_list_mode)
{
	if ((EPlayListMode_ExitUDECMode == play_list_mode) || (EPlayListMode_None == play_list_mode)) {
		return 0;
	}

	return 1;
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

	if ((not_init_udec) && (!udec_param->file_fd[0])) {
		u_printf_error("[flow, error]: not valid file name for udec(%d), return\n", udec_param->udec_index);
		return NULL;
	}

	//init udec instance
	memset(&info, 0x0, sizeof(info));
	ret = ioctl_init_udec_instance(&info, udec_param->iav_fd, udec_param->p_vout_config, udec_param->num_vout, udec_param->udec_index, udec_param->request_bsb_size, udec_param->udec_type, 0, enable_error_handling, udec_param->pic_width, udec_param->pic_height);
	if (ret < 0) {
		u_printf("[error]: ioctl_init_udec_instance fail.\n");
		return NULL;
	}

	u_printf_debug("[flow (%d)]: type(%d) begin loop.\n", udec_param->udec_index, udec_param->udec_type);
	u_printf_debug("	[params %d]: iav_fd(%d), udec_type(%d), bsb_start(%p), loop(%d), file fd0(%p), fd1(%p).\n", udec_param->udec_index, udec_param->iav_fd, udec_param->udec_type, info.bits_fifo_start, udec_param->loop, udec_param->file_fd[0], udec_param->file_fd[1]);
	u_printf_debug("	[params %d]: wait_cmd_begin(%d), wait_cmd_exit(%d).\n", udec_param->udec_index, udec_param->wait_cmd_begin, udec_param->wait_cmd_exit);

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

	if ((exit_udec_in_middle) && (!udec_param->file_fd[0])) {
		exit_flag = 1;
		u_printf_error("[flow, error]: not valid file name for udec(%d), invoke UDEC_EXIT immediately\n", udec_param->udec_index);
	} else {
		u_printf_debug("[flow (%d)]: udec instance index, before udec_instance_decode_es_file, playlist_mode %d.\n", udec_param->udec_index, playlist_mode);
		if (enable_one_shot) {
			ret = udec_instance_decode_es_test_file_oneshot(udec_param->udec_index, udec_param->iav_fd, udec_param->file_fd[0], udec_param->udec_type, info.bits_fifo_start, info.bits_fifo_start + udec_param->request_bsb_size, info.bits_fifo_start, &udec_param->cmd_queue, udec_param, one_shot_size);
		} else if (__need_into_play_es_file_264_playlist(playlist_mode)) {
			ret = udec_instance_decode_es_file_264_playlist(udec_param->udec_index, udec_param->iav_fd, udec_param->udec_type, info.bits_fifo_start, info.bits_fifo_start + udec_param->request_bsb_size, info.bits_fifo_start, &udec_param->cmd_queue, udec_param);
		} else {
			ret = udec_instance_decode_es_file_264(udec_param->udec_index, udec_param->iav_fd, udec_param->file_fd[0], udec_param->udec_type, info.bits_fifo_start, info.bits_fifo_start + udec_param->request_bsb_size, info.bits_fifo_start, &udec_param->cmd_queue, udec_param);
		}
		if (ret < 0) {
			//u_printf("udec_instance(%d)_decode_es_file ret %d, exit thread.\n", udec_param->udec_index, ret);
			exit_flag = 1;
		}
		u_printf_debug("[flow (%d)]: udec instance index, after udec_instance_decode_es_file, ret %d, exit_flag %d.\n", udec_param->udec_index, ret, exit_flag);
	}
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
			} else if (ACTION_EOS == ret) {
				break;
			} else {
				u_printf_error("un-processed cmd %d, ret %d.\n", msg.cmd, ret);
			}
		}
	}

	//stop udec if needed
	u_printf_debug("[flow]: before ioctl_udec_stop(%d, 1).\n", udec_param->udec_index);
	ioctl_udec_stop(udec_param->iav_fd, udec_param->udec_index, 1);

	//release udec
	u_printf_debug("[flow]: before IAV_IOC_RELEASE_UDEC(%d).\n", udec_param->udec_index);
	if (ioctl(udec_param->iav_fd, IAV_IOC_RELEASE_UDEC, udec_param->udec_index) < 0) {
		perror("IAV_IOC_RELEASE_UDEC");
		u_printf_error("release udec instance(%d) fail.\n");
	}

	u_printf_debug("[flow]: udec instance(%d) exit loop.\n", udec_param->udec_index);

	udec_running = 0;
	mdec_running = 0;
	return NULL;
}

static void sig_stop(int a)
{
	mdec_running = 0;
	user_press_quit = 1;
}

static void udec_sig_stop(int a)
{
	udec_running = 0;
	user_press_quit = 1;
}

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
	u_printf("     bsb info: phys start addr 0x%08x, total size %u(0x%08x), free space 0x%08x:\n", state.bits_fifo_phys_start, state.bits_fifo_total_size, state.bits_fifo_total_size, state.bits_fifo_free_size);

	u_printf("     dsp read pointer from msg: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.dsp_current_read_bitsfifo_addr_phys, state.dsp_current_read_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.dsp_current_read_bitsfifo_addr);
	u_printf("     last arm write pointer: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.arm_last_write_bitsfifo_addr_phys, state.arm_last_write_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.arm_last_write_bitsfifo_addr);

	//u_printf("     tot send decode cmd %d, tot frame count %d.\n", state.tot_decode_cmd_cnt, state.tot_decode_frame_cnt);

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
	u_assert(number_of_render < 12);
	if (number_of_render < 12) {
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

static int ioctl_zoom_mode_1(int iav_fd, int render_id, u32 zoom_factor_x, u32 zoom_factor_y)
{
	int ret;
	iav_udec_zoom_t zoom;
	memset(&zoom, 0x0, sizeof(zoom));

	zoom.render_id = render_id;
	zoom.zoom_factor_x = zoom_factor_x;
	zoom.zoom_factor_y = zoom_factor_y;

	u_printf("[cmd flow]: before IAV_IOC_UDEC_ZOOM(mode 1), render_id %d, zoom factor x 0x%08x, zoom factor y 0x%08x.\n", zoom.render_id, zoom.zoom_factor_x, zoom.zoom_factor_y);
	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_ZOOM, &zoom)) < 0) {
		perror("IAV_IOC_UDEC_ZOOM");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_UDEC_ZOOM.\n");

	return 0;
}

static int ioctl_zoom_mode_2(int iav_fd, int render_id, u16 input_win_width, u16 input_win_height, u16 input_win_center_x, u16 input_win_center_y)
{
	int ret;
	iav_udec_zoom_t zoom;
	memset(&zoom, 0x0, sizeof(zoom));

	zoom.render_id = render_id;
	zoom.input_center_x = input_win_center_x;
	zoom.input_center_y = input_win_center_y;
	zoom.input_width = input_win_width;
	zoom.input_height = input_win_height;

	u_printf("[cmd flow]: before IAV_IOC_UDEC_ZOOM(mode 2), render_id %d, input width %d, height %d, center x %d, center y %d.\n", zoom.render_id, zoom.input_width, zoom.input_height, zoom.input_center_x, zoom.input_center_y);
	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_ZOOM, &zoom)) < 0) {
		perror("IAV_IOC_UDEC_ZOOM");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_UDEC_ZOOM.\n");

	return 0;
}

static void set_playback_mode(udec_instance_param_t* params, int decoder_id, int mode, int udec_number)
{
	if ((decoder_id < 0) || (decoder_id >= udec_number)) {
		u_printf_error("BAD decoder_id %d, udec_number %d\n", decoder_id, udec_number);
		return;
	}

	if ((PB_STRATEGY_ALL_FRAME > mode) || (PB_STRATEGY_IDR_ONLY < mode)) {
		u_printf_error("BAD playback mode %d\n", mode);
		return;
	}

	params[decoder_id].tobe_playback_strategy = mode;
	return;
}

static int _get_display_window_size(int decoder_id, u16* width, u16* height)
{
	int i;
	u_assert(debug_p_windows);
	u_assert(debug_p_renders);
	u_assert(decoder_id < (debug_sds + debug_hds));
	if (decoder_id >= (debug_sds + debug_hds)) {
		u_printf_error("BAD decoder id %d\n", decoder_id);
		return (-2);
	}

	for (i = 0; i < (debug_sds + debug_hds); i ++) {
		if (debug_p_renders[i].udec_id == decoder_id) {
			*width = debug_p_windows[debug_p_renders[i].win_config_id].target_win_width;
			*height = debug_p_windows[debug_p_renders[i].win_config_id].target_win_height;
			return 0;
		}
	}

	u_printf_error("can not find display window for decoder id %d\n", decoder_id);
	return (-3);
}

static int _get_default_capture_size(int decoder_id, iav_udec_capture_t* capture)
{
	u16 width, height;

	if (!capture) {
		u_printf_error("NULL capture pointer here\n");
		return (-1);
	}

	if (IAV_ALL_UDEC_TAG != decoder_id) {
		u_assert(decoder_id < (debug_sds + debug_hds));
		if (decoder_id >= (debug_sds + debug_hds)) {
			u_printf_error("BAD decoder_id\n");
			return (-2);
		}

		if (0 > _get_display_window_size(decoder_id, &width, &height)) {
			u_printf_error("get display window size fail\n");
			return (-4);
		}

		capture->capture[CAPTURE_CODED].target_pic_width = file_video_width[decoder_id];
		capture->capture[CAPTURE_CODED].target_pic_height = file_video_height[decoder_id];

		capture->capture[CAPTURE_THUMBNAIL].target_pic_width = 320;
		capture->capture[CAPTURE_THUMBNAIL].target_pic_height = 240;

		capture->capture[CAPTURE_SCREENNAIL].target_pic_width = width;
		capture->capture[CAPTURE_SCREENNAIL].target_pic_height = height;
	} else {
		capture->capture[CAPTURE_CODED].target_pic_width = 0;
		capture->capture[CAPTURE_CODED].target_pic_height = 0;

		capture->capture[CAPTURE_THUMBNAIL].target_pic_width = 320;
		capture->capture[CAPTURE_THUMBNAIL].target_pic_height = 240;

		capture->capture[CAPTURE_SCREENNAIL].target_pic_width = vout_width[current_vout_start_index];
		capture->capture[CAPTURE_SCREENNAIL].target_pic_height = vout_height[current_vout_start_index];
	}

	return 0;
}

static int do_playback_capture(int iav_fd, int decoder_id, unsigned int file_index)
{
	int ret;
	unsigned int i = 0;
	iav_udec_capture_t capture;
	u16 cap_max_width = 0, cap_max_height = 0;

	FILE* pfile = NULL;
	char dump_file_name[320] = {0};

	memset(&capture, 0x0, sizeof(capture));

	capture.dec_id = decoder_id;
	if (IAV_ALL_UDEC_TAG != decoder_id) {
		capture.capture_coded = 1;
	} else {
		capture.capture_coded = 0;
	}
	capture.capture_thumbnail = 1;
	capture.capture_screennail = 1;

	for (i = 0; i < CAPTURE_TOT_NUM; i ++) {
		capture.capture[i].quality = 50;
	}

	if (0 > _get_default_capture_size(decoder_id, &capture)) {
		u_printf_error("_get_default_capture_size fail\n");
		return (-4);
	}

#if 0
	if (IAV_ALL_UDEC_TAG != decoder_id) {
		capture.capture[CAPTURE_CODED].target_pic_width = file_video_width[decoder_id];
		capture.capture[CAPTURE_CODED].target_pic_height = file_video_height[decoder_id];
	} else {
		capture.capture[CAPTURE_CODED].target_pic_width = vout_width[current_vout_start_index];
		capture.capture[CAPTURE_CODED].target_pic_height = vout_height[current_vout_start_index];
	}

	capture.capture[CAPTURE_THUMBNAIL].target_pic_width = 320;//hard code here
	capture.capture[CAPTURE_THUMBNAIL].target_pic_height = 240;

	if (cap_max_width < capture.capture[CAPTURE_THUMBNAIL].target_pic_width) {
		cap_max_width = capture.capture[CAPTURE_THUMBNAIL].target_pic_width;
	}
	if (cap_max_height < capture.capture[CAPTURE_THUMBNAIL].target_pic_height) {
		cap_max_height = capture.capture[CAPTURE_THUMBNAIL].target_pic_height;
	}

	capture.capture[CAPTURE_SCREENNAIL].target_pic_width = vout_width[current_vout_start_index];
	capture.capture[CAPTURE_SCREENNAIL].target_pic_height = vout_height[current_vout_start_index];

	if (cap_max_width < capture.capture[CAPTURE_SCREENNAIL].target_pic_width) {
		cap_max_width = capture.capture[CAPTURE_SCREENNAIL].target_pic_width;
	}
	if (cap_max_height < capture.capture[CAPTURE_SCREENNAIL].target_pic_height) {
		cap_max_height = capture.capture[CAPTURE_SCREENNAIL].target_pic_height;
	}

	if (capture.capture[CAPTURE_CODED].target_pic_width <= cap_max_width) {
		capture.capture[CAPTURE_CODED].target_pic_width = 1920;//cap_max_width;
	}
	if (capture.capture[CAPTURE_CODED].target_pic_height <= cap_max_height) {
		capture.capture[CAPTURE_CODED].target_pic_height = 1080;//cap_max_height;
	}
#endif

	u_assert(capture.capture[CAPTURE_CODED].target_pic_width <= 1920);
	u_assert(capture.capture[CAPTURE_CODED].target_pic_height <= 1080);//or 1088?

	if ((capture.capture[CAPTURE_CODED].target_pic_width == capture.capture[CAPTURE_SCREENNAIL].target_pic_width) && (capture.capture[CAPTURE_CODED].target_pic_height == capture.capture[CAPTURE_SCREENNAIL].target_pic_height)) {
		capture.capture[CAPTURE_CODED].target_pic_width = capture.capture[CAPTURE_CODED].target_pic_width/2;
		capture.capture[CAPTURE_CODED].target_pic_height = capture.capture[CAPTURE_CODED].target_pic_height/2;
		u_printf("[cmd flow]: warning, screennail's size same as coded size.\n");
	}

	u_printf("[cmd flow]: before IAV_IOC_UDEC_CAPTURE, decoder_id %02x, coded %dx%d, thumbnail %dx%d, screennail %dx%d, max %dx%d.\n", decoder_id, \
		capture.capture[CAPTURE_CODED].target_pic_width, capture.capture[CAPTURE_CODED].target_pic_height, \
		capture.capture[CAPTURE_THUMBNAIL].target_pic_width, capture.capture[CAPTURE_THUMBNAIL].target_pic_height, \
		capture.capture[CAPTURE_SCREENNAIL].target_pic_width, capture.capture[CAPTURE_SCREENNAIL].target_pic_height, \
		cap_max_width, cap_max_height \
		);

	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_CAPTURE, &capture)) < 0) {
		perror("IAV_IOC_UDEC_CAPTURE");
		return -1;
	}

	u_printf("[cmd flow]: after IAV_IOC_UDEC_CAPTURE.\n");

	//save bit-stream
	sprintf(dump_file_name, "coded_%02x_%02x.jpeg", decoder_id, file_index);
	pfile = fopen(dump_file_name, "wb");
	if (pfile) {
		u_assert(capture.capture[CAPTURE_CODED].buffer_base);
		u_assert(capture.capture[CAPTURE_CODED].buffer_limit);
		u_assert(capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit);
		u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_CODED].buffer_base, capture.capture[CAPTURE_CODED].buffer_limit, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base);
		if (capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit) {
			fwrite((void*)capture.capture[CAPTURE_CODED].buffer_base, 1, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base, pfile);
		}
		fclose(pfile);
	}

	sprintf(dump_file_name, "thumb_%02x_%02x.jpeg", decoder_id, file_index);
	pfile = fopen(dump_file_name, "wb");
	if (pfile) {
		u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_base);
		u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
		u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
		u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_THUMBNAIL].buffer_base, capture.capture[CAPTURE_THUMBNAIL].buffer_limit, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base);
		if (capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit) {
			fwrite((void*)capture.capture[CAPTURE_THUMBNAIL].buffer_base, 1, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base, pfile);
		}
		fclose(pfile);
	}

	sprintf(dump_file_name, "screen_%02x_%02x.jpeg", decoder_id, file_index);
	pfile = fopen(dump_file_name, "wb");
	if (pfile) {
		u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_base);
		u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
		u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
		u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_SCREENNAIL].buffer_base, capture.capture[CAPTURE_SCREENNAIL].buffer_limit, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base);
		if (capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit) {
			fwrite((void*)capture.capture[CAPTURE_SCREENNAIL].buffer_base, 1, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base, pfile);
		}
		fclose(pfile);
	}

	return 0;
}

static int do_playback_capture_cycle(int iav_fd, int decoder_id, unsigned int cap_interval)
{
	unsigned int file_index = 0;
	char input_buffer[128] = {0};
	int fd = open("/dev/tty", O_RDONLY|O_NONBLOCK);
	if (fd<0) {
		perror("open /dev/tty");
		return -1;
	}
	do {
		file_index++;
		do_playback_capture(iav_fd, decoder_id, file_index);
		u_printf("capture %u done.\n", file_index);
		usleep(cap_interval*1000);
		if (read(fd, input_buffer, sizeof(input_buffer)) < 0)
			continue;
		else
			break;
	} while(1);
	if (fd) {
		close(fd);
		fd=0;
	}

	return 0;
}

static int do_playback_capture_detailed(int iav_fd, int decoder_id, unsigned int file_index, unsigned int cap_coded, unsigned int cap_thumbnail, unsigned int cap_screennail)
{
	int ret;
	unsigned int i = 0;
	iav_udec_capture_t capture;
	u16 cap_max_width = 0, cap_max_height = 0;

	FILE* pfile = NULL;
	char dump_file_name[320] = {0};

	memset(&capture, 0x0, sizeof(capture));

	capture.dec_id = decoder_id;
	capture.capture_coded = cap_coded;
	capture.capture_thumbnail = cap_thumbnail;
	capture.capture_screennail = cap_screennail;

	if (IAV_ALL_UDEC_TAG != decoder_id) {
		capture.capture_coded = 1;
	} else {
		capture.capture_coded = 0;
	}

	for (i = 0; i < CAPTURE_TOT_NUM; i ++) {
		capture.capture[i].quality = 50;
	}

	if (0 > _get_default_capture_size(decoder_id, &capture)) {
		u_printf_error("_get_default_capture_size fail\n");
		return (-4);
	}

#if 0
	if (IAV_ALL_UDEC_TAG != decoder_id) {
		capture.capture[CAPTURE_CODED].target_pic_width = file_video_width[decoder_id];
		capture.capture[CAPTURE_CODED].target_pic_height = file_video_height[decoder_id];
	} else {
		capture.capture[CAPTURE_CODED].target_pic_width = vout_width[current_vout_start_index];
		capture.capture[CAPTURE_CODED].target_pic_height = vout_height[current_vout_start_index];
	}

	capture.capture[CAPTURE_THUMBNAIL].target_pic_width = 320;//hard code here
	capture.capture[CAPTURE_THUMBNAIL].target_pic_height = 240;

	if (cap_max_width < capture.capture[CAPTURE_THUMBNAIL].target_pic_width) {
		cap_max_width = capture.capture[CAPTURE_THUMBNAIL].target_pic_width;
	}
	if (cap_max_height < capture.capture[CAPTURE_THUMBNAIL].target_pic_height) {
		cap_max_height = capture.capture[CAPTURE_THUMBNAIL].target_pic_height;
	}

	capture.capture[CAPTURE_SCREENNAIL].target_pic_width = vout_width[current_vout_start_index];
	capture.capture[CAPTURE_SCREENNAIL].target_pic_height = vout_height[current_vout_start_index];

	if (cap_max_width < capture.capture[CAPTURE_SCREENNAIL].target_pic_width) {
		cap_max_width = capture.capture[CAPTURE_SCREENNAIL].target_pic_width;
	}
	if (cap_max_height < capture.capture[CAPTURE_SCREENNAIL].target_pic_height) {
		cap_max_height = capture.capture[CAPTURE_SCREENNAIL].target_pic_height;
	}

	if (capture.capture[CAPTURE_CODED].target_pic_width <= cap_max_width) {
		capture.capture[CAPTURE_CODED].target_pic_width = 1920;//cap_max_width;
	}
	if (capture.capture[CAPTURE_CODED].target_pic_height <= cap_max_height) {
		capture.capture[CAPTURE_CODED].target_pic_height = 1080;//cap_max_height;
	}
#endif

	u_assert(capture.capture[CAPTURE_CODED].target_pic_width <= 1920);
	u_assert(capture.capture[CAPTURE_CODED].target_pic_height <= 1080);//or 1088?

	u_printf("[cmd flow]: before IAV_IOC_UDEC_CAPTURE, decoder_id %02x, coded %dx%d, thumbnail %dx%d, screennail %dx%d, max %dx%d, cap_coded %d, cap_thumbnail %d, cap_screennail %d.\n", decoder_id, \
		capture.capture[CAPTURE_CODED].target_pic_width, capture.capture[CAPTURE_CODED].target_pic_height, \
		capture.capture[CAPTURE_THUMBNAIL].target_pic_width, capture.capture[CAPTURE_THUMBNAIL].target_pic_height, \
		capture.capture[CAPTURE_SCREENNAIL].target_pic_width, capture.capture[CAPTURE_SCREENNAIL].target_pic_height, \
		cap_max_width, cap_max_height, \
		capture.capture_coded, capture.capture_thumbnail, capture.capture_screennail);

	if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_CAPTURE, &capture)) < 0) {
		perror("IAV_IOC_UDEC_CAPTURE");
		return -1;
	}

	u_printf("[cmd flow]: after IAV_IOC_UDEC_CAPTURE.\n");

	//save bit-stream
	if (cap_coded) {
		sprintf(dump_file_name, "coded_%02x_%02x.jpeg", decoder_id, file_index);
		pfile = fopen(dump_file_name, "wb");
		if (pfile) {
			u_assert(capture.capture[CAPTURE_CODED].buffer_base);
			u_assert(capture.capture[CAPTURE_CODED].buffer_limit);
			u_assert(capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit);
			u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_CODED].buffer_base, capture.capture[CAPTURE_CODED].buffer_limit, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base);
			if (capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit) {
				fwrite((void*)capture.capture[CAPTURE_CODED].buffer_base, 1, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base, pfile);
			}
			fclose(pfile);
		}
	}

	if (cap_thumbnail) {
		sprintf(dump_file_name, "thumb_%02x_%02x.jpeg", decoder_id, file_index);
		pfile = fopen(dump_file_name, "wb");
		if (pfile) {
			u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_base);
			u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
			u_assert(capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
			u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_THUMBNAIL].buffer_base, capture.capture[CAPTURE_THUMBNAIL].buffer_limit, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base);
			if (capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit) {
				fwrite((void*)capture.capture[CAPTURE_THUMBNAIL].buffer_base, 1, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base, pfile);
			}
			fclose(pfile);
		}
	}

	if (cap_screennail) {
		sprintf(dump_file_name, "screen_%02x_%02x.jpeg", decoder_id, file_index);
		pfile = fopen(dump_file_name, "wb");
		if (pfile) {
			u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_base);
			u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
			u_assert(capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
			u_printf("[pb capture]: coded buffer start %p, end %p, size %d\n", capture.capture[CAPTURE_SCREENNAIL].buffer_base, capture.capture[CAPTURE_SCREENNAIL].buffer_limit, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base);
			if (capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit) {
				fwrite((void*)capture.capture[CAPTURE_SCREENNAIL].buffer_base, 1, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base, pfile);
			}
			fclose(pfile);
		}
	}

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

static u32 __get_current_max_pb_speed(u32 index, u32 tot_dec_num, u16* max, u16* max_frac)
{
	u32 i = 0;
	u32 remaining_performance = system_max_performance_score;
	u32 detailed_speed;

	for (i = 0; i < tot_dec_num; i++) {
		if (i != index) {
			remaining_performance -= performance_score[i] * cur_pb_speed[i];
		}
	}

	u_assert(remaining_performance <= system_max_performance_score);
	if (remaining_performance <= system_max_performance_score) {
		u_assert(remaining_performance >= performance_score[index]);
		detailed_speed = (remaining_performance * 0x10000)/performance_score[index];
		u_printf("get max speed, remaining score %u, system max %u, index %u\n", remaining_performance, system_max_performance_score, performance_score[index]);
		if (max) {
			*max = detailed_speed >> 16;
		}
		if (max_frac) {
			*max_frac= detailed_speed & 0xffff;
		}

		if (remaining_performance > performance_score[index]) {
			return (remaining_performance + 2)/performance_score[index];
		}
		return 1;
	}
	if (max) {
		*max = 0x01;
	}
	if (max_frac) {
		*max_frac= 0x00;
	}
	return 1;
}

static void __rect_layout(int vout_index, int cols, int rows, int start_index, int end_index, int display_width, int display_height)
{
	int cindex, rindex;
	u32 win_id = start_index;
	int rect_w, rect_h;

	if (!display_width || !display_height || !rows || !cols ||!end_index) {
		u_printf_error("zero input!!\n");
		return;
	}

	rect_w = display_width/cols;
	rect_h = display_height/rows;

	for (rindex = 0; rindex < rows; rindex++) {
		for (cindex = 0; cindex < cols; cindex++) {
			display_window[vout_index][win_id].win_id = win_id;
			display_window[vout_index][win_id].set_by_user = 1;
			display_window[vout_index][win_id].target_win_offset_x = cindex * rect_w;
			display_window[vout_index][win_id].target_win_offset_y = rindex * rect_h;
			display_window[vout_index][win_id].target_win_width = rect_w;
			display_window[vout_index][win_id].target_win_height = rect_h;
			win_id ++;

			if (win_id >= end_index) {
				break;
			}
		}
	}

	return;
}

static void __highlight_bottomleft_layout_non_highlight_rects(int vout_index, int cols, int rows, int start_index, int end_index, int display_width, int display_height, int highlight_w, int highlight_h)
{
	int cindex, rindex;
	u32 win_id = start_index;
	int rect_w, rect_h;
	int rect_w_1, rect_h_1;

	if (!display_width || !display_height ||!end_index) {
		u_printf_error("zero input!!\n");
		return;
	}

	if ((highlight_w >= display_width) && (highlight_h >= display_height)) {
		u_printf_error("bad highlight w %d,h %d!!\n", highlight_w, highlight_h);
		highlight_w = display_width*3/4;
		highlight_h = display_height*3/4;
	}

	rect_w_1 = display_width - highlight_w;
	rect_h_1 = display_height - highlight_h;

	if (cols) {
		rect_w = highlight_w/cols;
	} else {
		rect_w = highlight_w;
	}
	if (rows) {
		rect_h = highlight_h/rows;
	} else {
		rect_h = highlight_h;
	}

	//non-highlight window, top row
	if (cols) {
		for (cindex = 0; cindex < cols; cindex++) {
			display_window[vout_index][win_id].win_id = win_id;
			display_window[vout_index][win_id].set_by_user = 1;
			display_window[vout_index][win_id].target_win_offset_x = cindex * rect_w;
			display_window[vout_index][win_id].target_win_offset_y = 0;
			display_window[vout_index][win_id].target_win_width = rect_w;
			display_window[vout_index][win_id].target_win_height = rect_h_1;
			win_id ++;

			if (win_id >= end_index) {
				break;
			}
		}
	}

	//non-highligh window, top right conner
	display_window[vout_index][win_id].win_id = win_id;
	display_window[vout_index][win_id].set_by_user = 1;
	display_window[vout_index][win_id].target_win_offset_x = highlight_w;
	display_window[vout_index][win_id].target_win_offset_y = 0;
	display_window[vout_index][win_id].target_win_width = rect_w_1;
	display_window[vout_index][win_id].target_win_height = rect_h_1;
	win_id ++;
	if (win_id >= end_index) {
		u_printf_error("config exit here, wind_id %d, end_index %d\n", win_id, end_index);
		return;
	}

	//non-highlight window, right col
	if (rows) {
		for (rindex = 0; rindex < rows; rindex++) {
			display_window[vout_index][win_id].win_id = win_id;
			display_window[vout_index][win_id].set_by_user = 1;
			display_window[vout_index][win_id].target_win_offset_x = highlight_w;
			display_window[vout_index][win_id].target_win_offset_y = rindex * rect_h + rect_h_1;
			display_window[vout_index][win_id].target_win_width = rect_w_1;
			display_window[vout_index][win_id].target_win_height = rect_h;
			win_id ++;

			if (win_id >= end_index) {
				break;
			}
		}
	}

	return;
}

static void __highlight_bottomleft_layout_highlight_rect(int vout_index, int start_index, int display_width, int display_height, int highlight_w, int highlight_h)
{
	u32 win_id = start_index;

	if (!display_width || !display_height || !highlight_w || !highlight_h) {
		u_printf_error("zero input!!\n");
		return;
	}

	if ((highlight_w >= display_width) && (highlight_h >= display_height)) {
		u_printf_error("bad highlight w %d,h %d!!\n", highlight_w, highlight_h);
		highlight_w = display_width*3/4;
		highlight_h = display_height*3/4;
	}

	//highlight window
	display_window[vout_index][win_id].win_id = win_id;
	display_window[vout_index][win_id].set_by_user = 1;
	display_window[vout_index][win_id].target_win_offset_x = 0;
	display_window[vout_index][win_id].target_win_offset_y = display_height - highlight_h;
	display_window[vout_index][win_id].target_win_width = highlight_w;
	display_window[vout_index][win_id].target_win_height = highlight_h;

	return;
}

//window number = rows + cols + cols
static void __center_v_layout(int vout_index, int cols, int rows, int start_index, int end_index, int display_width, int display_height, int center_w, int center_h)
{
	int cindex = 0, rindex = 0;
	u32 win_id = start_index;
	int offset_x = 0;

	int rect_w, rect_h;

	if (!display_width || !display_height || !rows || !cols) {
		u_printf_error("zero input!!\n");
		return;
	}

	rect_h = display_height/rows;

	if ((center_w >= display_width) && (center_h >= display_height)) {
		u_printf_error("bad highlight w %d,h %d!!\n", center_w, center_h);
		center_w = display_width/2;
		center_h = display_height;
	}

	//center column
	offset_x = (display_width - center_w)/2;
	for (rindex = 0; rindex < rows; rindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = offset_x;
		display_window[vout_index][win_id].target_win_offset_y = rindex * rect_h;
		display_window[vout_index][win_id].target_win_width = center_w;
		display_window[vout_index][win_id].target_win_height = rect_h;
		win_id ++;

		if (win_id >= end_index) {
			u_printf_error("center column exceed?!!\n");
			return;
		}
	}

	//left part
	rect_w = 	(display_width - center_w)/cols/2;
	for (cindex = 0; cindex < cols; cindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = cindex * rect_w;
		display_window[vout_index][win_id].target_win_offset_y = 0;
		display_window[vout_index][win_id].target_win_width = rect_w;
		display_window[vout_index][win_id].target_win_height = display_height;
		win_id ++;

		if (win_id >= end_index) {
			u_printf_error("center column + left part exceed?!!\n");
			return;
		}
	}

	//right part
	offset_x += center_w;
	for (cindex = 0; cindex < cols; cindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = cindex * rect_w + offset_x;
		display_window[vout_index][win_id].target_win_offset_y = 0;
		display_window[vout_index][win_id].target_win_width = rect_w;
		display_window[vout_index][win_id].target_win_height = display_height;
		win_id ++;

		if (win_id >= end_index) {
			return;
		}
	}

	return;
}

static void __avg_column_layout(int vout_index, int cols, int start_index, int end_index, int display_width, int display_height)
{
	int cindex;
	u32 win_id = start_index;
	int rect_w, rect_h;

	if (!display_width || !display_height || !cols) {
		u_printf_error("zero input!!\n");
		return;
	}

	rect_w = display_width/cols;
	rect_h = display_height;

	for (cindex = 0; cindex < cols; cindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = cindex * rect_w;
		display_window[vout_index][win_id].target_win_offset_y = 0;
		display_window[vout_index][win_id].target_win_width = rect_w;
		display_window[vout_index][win_id].target_win_height = rect_h;
		win_id ++;

		if (win_id >= end_index) {
			break;
		}
	}

	return;
}

static void __avg_row_layout(int vout_index, int rows, int start_index, int end_index, int display_width, int display_height)
{
	int rindex;
	u32 win_id = start_index;
	int rect_w, rect_h;

	if (!display_width || !display_height || !rows) {
		u_printf_error("zero input!!\n");
		return;
	}

	rect_w = display_width;
	rect_h = display_height/rows;

	for (rindex = 0; rindex < rows; rindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = 0;
		display_window[vout_index][win_id].target_win_offset_y = rindex*rect_h;
		display_window[vout_index][win_id].target_win_width = rect_w;
		display_window[vout_index][win_id].target_win_height = rect_h;
		win_id ++;

		if (win_id >= end_index) {
			break;
		}
	}

	return;
}

// 1 HD + multiple SD
static void __telepresence_layout(int vout_index, int sd_cnt, int hd_cnt, int display_width, int display_height)
{
#define SD_H_GAP 40
#define SD_V_GAP 40

	int rindex = 0;
	int win_id = 0;
	int rect_w, rect_h;

	if (!display_width || !display_height || (!sd_cnt && !hd_cnt)) {
		u_printf_error("__telepresence_layout: zero input!!\n");
		return;
	}

	// config fullscreen first
	display_window[vout_index][win_id].win_id = win_id;
	display_window[vout_index][win_id].set_by_user = 1;
	display_window[vout_index][win_id].target_win_offset_x = 0;
	display_window[vout_index][win_id].target_win_offset_y = 0;
	display_window[vout_index][win_id].target_win_width = display_width;
	display_window[vout_index][win_id].target_win_height = display_height;
	win_id++;

	// config average
	int left_stream_cnt = sd_cnt + hd_cnt - 1;
	rect_w = (display_width - (left_stream_cnt + 1)*SD_H_GAP)/left_stream_cnt;
	rect_h = (display_height - (left_stream_cnt + 1)*SD_V_GAP)/left_stream_cnt;
	rect_w = (rect_w>>2)<<2;

	for (rindex = 0; rindex < left_stream_cnt; rindex++) {
		display_window[vout_index][win_id].win_id = win_id;
		display_window[vout_index][win_id].set_by_user = 1;
		display_window[vout_index][win_id].target_win_offset_x = (rect_w+SD_H_GAP)*rindex + SD_H_GAP;
		display_window[vout_index][win_id].target_win_offset_y = (rect_h+SD_V_GAP)*(left_stream_cnt-1) + SD_V_GAP;
		display_window[vout_index][win_id].target_win_width = rect_w;
		display_window[vout_index][win_id].target_win_height = rect_h;
		win_id++;
	}
}

static void __different_display_layout(int vout_index, u32 layout_input, u32 number_input, int vout_w, int vout_h)
{
	int w, h;
	u32 i;
	u32 current_win_id = 0;
	int num_rects = 1;
	u8 layout[4] = {0};
	u8 number[4] = {0};


	layout[0] = layout_input & 0xf;
	layout[1] = (layout_input >> 4) & 0xf;

	number[0] = number_input & 0xff;
	number[1] = (number_input >> 8) & 0xff;


	for (i = 0; i< 2; i++) {
		u_printf("[pre-config]: set pre-layout %d, i %d, number %d, vout_w %d, vout_h %d\n", layout[i], i, number[i], vout_w, vout_h);
		if ((!number[i])) {
			continue;
		}

		if (1 == number[i]) {
			__rect_layout(vout_index, 1, 1, current_win_id, current_win_id, vout_w, vout_h);
			current_win_id += 1;
			continue;
		}

		switch (layout[i]) {
			case 0:
				//u_printf("not set pre-layout\n");
				//break;

			//general rect-m x n layout
			case 1:
				if (5 > number[i]) {
					// 2x2 mode
					w = 2;
					h = 2;
				} else if (7 > number[i]) {
					// 3x2 mode
					w = 3;
					h = 2;
				} else if (10 >  number[i]) {
					// 3x3 mode
					w = 3;
					h = 3;
				} else if (13 >  number[i]) {
					// 4x3 mode
					w = 4;
					h = 3;
				} else if (17 >  number[i]) {
					// 4x4 mode
					w = 4;
					h = 4;
				} else {
					u_printf_error("not supported,  number[i] %d\n", number[i]);
					break;
				}
				u_printf("[display layout]: rect layout, number[i] %d, w %d, h %d, start win id %d\n", number[i], w, h, current_win_id);
				__rect_layout(vout_index, w, h, current_win_id, current_win_id + number[i], vout_w, vout_h);
				current_win_id += number[i];
				display_layer_number[vout_index][i] = number[i];
				break;

			case 2:
				if (number[i] <= 2) {
					w = 0;
					h = 0;
				} else {
					num_rects = number[i] - 1 - 1;
					w = (num_rects + 1)/2;
					h = num_rects - w;
				}
				u_printf("[display layout]: bottomleft layout, number[i] %d, num_rects %d, w %d, h %d, start win id %d\n", number[i], num_rects, w, h, current_win_id);
				__highlight_bottomleft_layout_non_highlight_rects(vout_index, w, h, current_win_id, current_win_id + number[i] - 1, vout_w, vout_h,  vout_w*3/4, vout_h*3/4);
				__highlight_bottomleft_layout_highlight_rect(vout_index, current_win_id + number[i] - 1, vout_w, vout_h,  vout_w*3/4, vout_h*3/4);
				current_win_id += number[i];
				display_layer_number[vout_index][i] = number[i];
				break;

			case 3:
				if (number[i] >= 2) {
					h = 2;
					w = (number[i] - h + 1)/2;
					u_printf("[display layout]: center v layout, number[i] %d, w %d, h %d\n", number[i], w, h);
					__center_v_layout(vout_index, w, h, current_win_id, current_win_id + number[i], vout_w, vout_h, vout_w/2, vout_h);
					current_win_id += number[i];
				} else if (1 == number[i]) {
					__rect_layout(vout_index, 1, 1, current_win_id, current_win_id, vout_w, vout_h);
					current_win_id += 1;
				} else {
					u_printf_error("BAD number[i] %d\n", number[i]);
					break;
				}
				display_layer_number[vout_index][i] = number[i];
				break;

			case 4:
				u_printf("[display layout]: avg column layout, number[i] %d, current_win_id %d\n", number[i], current_win_id);
				__avg_column_layout(vout_index, number[i], current_win_id, current_win_id + number[i], vout_w, vout_h);
				current_win_id += number[i];
				display_layer_number[vout_index][i] = number[i];
				break;

			case 5:
				u_printf("[display layout]: avg row layout, number[i] %d, win id %d\n", number[i], current_win_id);
				__avg_row_layout(vout_index, number[i], current_win_id, current_win_id + number[i], vout_w, vout_h);
				current_win_id += number[i];
				display_layer_number[vout_index][i] = number[i];
				break;
			case 6:
				u_printf("[display layout]: telepresence layout, sd %d, hd %d, win id %d\n", number[0], number[1], current_win_id);
				__telepresence_layout(vout_index, number[0], number[1], vout_w, vout_h);
				break;
			default:
				u_printf("not-support layout number %d\n", layout);
				break;
		}
 	}
	return;
}

static int __zoom_params_valid_mode1(int render_id, u32 zoom_factor_x, u32 zoom_factor_y)
{
	u32 max_zoom_factor_x = 0, max_zoom_factor_y = 0;

	u_assert(file_video_width[render_2_udec[render_id]]);
	max_zoom_factor_x = (((u64)(display_window[current_vout_start_index][render_2_window[render_id]].target_win_width))*0x10000)/file_video_width[render_2_udec[render_id]];
	max_zoom_factor_y = (((u64)(display_window[current_vout_start_index][render_2_window[render_id]].target_win_height))*0x10000)/file_video_height[render_2_udec[render_id]];

	if ((zoom_factor_x > max_zoom_factor_x) || (zoom_factor_y > max_zoom_factor_y)) {
		return 0;
	}

	return 1;
}

static int __zoom_params_valid_mode2(int render_id, u16 input_w, u16 input_h, u16 center_x, u16 center_y)
{
	if ((center_x + (input_w + 1)/2 > file_video_width[render_2_udec[render_id]]) || (center_y + (input_h + 1)/2 > file_video_height[render_2_udec[render_id]])) {
		return 0;
	}

	return 1;
}

static void __update_windows(u32 new_vout_index, udec_window_t *windows, u32 total_window_num, u32 start_winid)
{
	u32 i = 0;
	for (i = 0; i < total_window_num; i++) {
		windows->win_config_id = start_winid + i;
		windows->target_win_offset_x = display_window[new_vout_index][i].target_win_offset_x;
		windows->target_win_offset_y = display_window[new_vout_index][i].target_win_offset_y;
		windows->target_win_width = display_window[new_vout_index][i].target_win_width;
		windows->target_win_height = display_window[new_vout_index][i].target_win_height;
		windows ++;
	}
}

static void __update_mudec_display(int iav_fd, u32 new_vout_index, udec_window_t *windows, udec_render_t *renders, u32 total_window_num, u32 total_render_number)
{
	int ret;
	iav_postp_update_mw_display_t display;

	memset(&display, 0x0, sizeof(display));

	//display.out_to_hdmi = (new_vout_index == 1)? 1:0;

	display.max_num_windows = total_window_num + 1;
	display.total_num_win_configs = total_window_num;
	display.total_num_render_configs = total_render_number;

	if (0 == vout_rotate[new_vout_index]) {
		display.video_win_width = vout_width[new_vout_index];
		display.video_win_height = vout_height[new_vout_index];
	} else {
		display.video_win_width = vout_height[new_vout_index];
		display.video_win_height = vout_width[new_vout_index];
	}

	display.windows_config = windows;
	display.render_config = renders;

	__update_windows(new_vout_index, windows, total_window_num, 0);

	u_printf("[cmd flow], before IAV_IOC_POSTP_UPDATE_MW_DISPLAY\n");
	ret = ioctl(iav_fd, IAV_IOC_POSTP_UPDATE_MW_DISPLAY, &display);
	if (ret < 0) {
		perror("IAV_IOC_POSTP_UPDATE_MW_DISPLAY");
		u_printf_error("IAV_IOC_POSTP_UPDATE_MW_DISPLAY fail, ret %d\n", ret);
	}
	u_printf("[cmd flow], IAV_IOC_POSTP_UPDATE_MW_DISPLAY done\n");

	return;
}

static void __exchange_window_circularly(udec_render_t* p_render, u8 tot_number, u8 exchange_seed)
{
	u8 first_win_id = 0, i = 0, target = 0, wrapped = 0;
	u8 first_win_id_2rd = 0xff;

	exchange_seed = exchange_seed % tot_number;
	//debug assert;
	u_assert(tot_number < 8);

	first_win_id = p_render->win_config_id;
	first_win_id_2rd = p_render->win_config_id_2nd;
	target = exchange_seed;

	for (i = 0; i < tot_number; i ++, target ++) {
		if (target >= tot_number) {
			u_assert(!wrapped);
			target = 0;
			wrapped = 1;
			p_render[i].win_config_id = first_win_id;
			p_render[i].win_config_id_2nd = first_win_id_2rd;
		} else {
			p_render[i].win_config_id = p_render[target].win_config_id;
			p_render[i].win_config_id_2nd = p_render[target].win_config_id_2nd;
		}
	}
}

static void __exchange_udec_circularly(udec_render_t* p_render, u8 tot_number, u8 exchange_seed)
{
	u8 first_udec_id = 0, i = 0, target = 0, wrapped = 0;

	exchange_seed = exchange_seed % tot_number;
	//debug assert;
	u_assert(tot_number < 8);

	first_udec_id = p_render->udec_id;
	target = exchange_seed;

	for (i = 0; i < tot_number; i ++, target ++) {
		if (target >= tot_number) {
			u_assert(!wrapped);
			target = 0;
			wrapped = 1;
			p_render[i].udec_id = first_udec_id;
		} else {
			p_render[i].udec_id = p_render[target].udec_id;
		}
	}
}

enum {
	EOPFB_Clear = 0,
	EOPFB_Set,
	EOPFB_VerticalStrip,
	EOPFB_HorizontalStrip,
	EOPFB_DisplayBitmap,
	EOPFB_Rect,
};

static void __op_update_framebuffer_clut(int fd, const char* dev)
{
	int need_close_fd = 0;
	struct fb_cmap_user	cmap;
	u_printf("Set color map ...\n");

	if (fd < 0) {
		fd = open(dev, O_RDWR);
		if (fd >= 0) {
			u_printf("[fb flow]: open %s, ret %d\n", dev, fd);
		} else {
			perror(dev);
			u_printf_error("open %s fail, ret %d\n", dev, fd);
			return;
		}
		need_close_fd = 1;
	}

	cmap.start = 0;
	cmap.len = 256;
	cmap.y = clut_y_table;
	cmap.u = clut_u_table;
	cmap.v = clut_v_table;
	cmap.transp = clut_blend_table;
	if (ioctl(fd, FBIOPUTCMAP, &cmap) < 0) {
		u_printf_error("Unable to put cmap!\n");
	}

	if (need_close_fd) {
		close(fd);
	}
}

static void __op_framebuffer_0(const char* dev, int op, const char* filename, int param1, int param2, int param3, int param4)
{
	int fd;
	struct fb_var_screeninfo var;
	int fb_dev_pitch;
	char *fb_dev_base;
	int fb_dev_size;

	if (!dev) {
		u_printf_error("NULL dev\n");
		return;
	}

	fd = open(dev, O_RDWR);
	if (fd >= 0) {
		u_printf("[fb flow]: open %s, ret %d\n", dev, fd);
	} else {
		perror(dev);
		u_printf_error("open %s fail, ret %d\n", dev, fd);
		return;
	}

	if (need_update_clut_table) {
		__op_update_framebuffer_clut(fd, NULL);
		need_update_clut_table = 0;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("FBIOGET_VSCREENINFO");
		u_printf_error("FBIOGET_VSCREENINFO fail\n", dev);
		goto __op_framebuffer_0_error2;
	}

	u_printf("[fb flow]: xres: %d, yres: %d, bpp: %d\n", var.xres, var.yres, var.bits_per_pixel);

	fb_dev_pitch = var.xres * (var.bits_per_pixel >> 3);
	fb_dev_size = fb_dev_pitch * var.yres;
	fb_dev_base = mmap(NULL, fb_dev_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb_dev_base == MAP_FAILED) {
		perror("mmap");
		u_printf_error("mmap fail\n", dev);
		goto __op_framebuffer_0_error2;
	}

	u_printf("[fb flow]: op %d, before memory copy, param1 %d.\n", op, param1);
	switch (op) {
		case EOPFB_Clear:
			memset(fb_dev_base, 0, fb_dev_size);
			break;
		case EOPFB_Set:
			memset(fb_dev_base, (u8)param1, fb_dev_size);
			break;
		default:
			u_printf_error("not supported op %d\n", op);
			break;
	}
	u_printf("[fb flow]: op %d, after memory copy, start display\n", op);

	if (ioctl(fd, FBIOPAN_DISPLAY, &var) < 0) {
		perror("FBIOPAN_DISPLAY");
		u_printf_error("[fb flow]: display fail\n");
		goto __op_framebuffer_0_error1;
	}

	u_printf("[fb flow]: after display\n");

__op_framebuffer_0_error1:

	if (munmap(fb_dev_base, fb_dev_size) < 0) {
		perror("munmap");
		u_printf_error("[fb flow]: munmap fail\n");
	}

	u_printf("[fb flow]: after munmap\n");

__op_framebuffer_0_error2:
	close(fd);

	u_printf("[fb flow]: after close %d\n", fd);
}

static void __op_framebuffer_1(const char* dev, int op, const char* filename, int param1, int param2, int param3, int param4, int param5, bmp_t* bmp)
{
	int				fbfd = 0;
	struct fb_var_screeninfo	vinfo;
	struct fb_fix_screeninfo	finfo;
	long int	screensize = 0;
	char		*fbp = 0;
	int		x = 0, y = 0;
	int		x_start = 0, y_start = 0;
	int		width = 0, height = 0;
	int		color = 0;
	long int	location = 0;
	int ret;

	unsigned int row = 8, col = 8;
	unsigned int r_0 = 0, b_0 = 0, g_0 = 0;
	unsigned int r_r = 7, b_r = 17, g_r = 29;
	unsigned int r_c = 211, b_c = 157, g_c = 97;

	unsigned int ii, jj;
	unsigned int r, g, b;
	unsigned short value;
	unsigned short* pv;
	unsigned short* pv1;
	unsigned int count = 0;

	if (!dev) {
		u_printf_error("NULL dev\n");
		return;
	}

	fbfd = open(dev, O_RDWR);
	if (fbfd >= 0) {
		u_printf("[fb flow]: open %s, ret %d\n", dev, fbfd);
	} else {
		perror(dev);
		u_printf_error("open %s fail, ret %d\n", dev, fbfd);
		return;
	}

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		u_printf_error("Error reading fixed information.\n");
		goto __op_framebuffer_1_error2;
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		u_printf_error("Error reading variable information.\n");
		goto __op_framebuffer_1_error2;
	} else {
		u_printf("[fb flow]: %dx%d, %dbpp,  finfo.line_length %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,  finfo.line_length);
	}

	// Figure out the size of the screen in bytes
	screensize = vinfo.yres * finfo.line_length;

	// Map the device to memory
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (!fbp) {
		u_printf_error("Error: failed to map framebuffer device to memory.\n");
		goto __op_framebuffer_1_error2;
	} else {
		u_printf("The framebuffer device was mapped to memory successfully.\n");
	}

	switch (op) {
		case EOPFB_Set:
			color	= param1;

			if (param2 >= 0) {
				x_start	= param2;
			} else {
				x_start	= 0;
			}

			if (param3 >= 0) {
				y_start	= param3;
			} else {
				y_start = 0;
			}

			if (param4 > 0) {
				width	= param4;
			} else {
				width	= vinfo.xres;
			}

			if (param5 > 0) {
				height	= param5;
			} else {
				height	= vinfo.yres;
			}

			u_printf("[fb flow]: op %d, before memory copy, color 0x%08x, x_start %d, y_start %d, width %d, height %d.\n", color, x_start, y_start, width, height);

			// Figure out where in memory to put the pixel
			for (y = y_start; y < (y_start + height); y++) {
				for (x = x_start; x < (x_start + width); x++) {
					location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
				       (y + vinfo.yoffset) * finfo.line_length;
					if (vinfo.bits_per_pixel == 8) {
						*((unsigned char *)(fbp + location)) = color;
					}
					if (vinfo.bits_per_pixel == 16) {
						*((unsigned short *)(fbp + location)) = color;
					}
					if (vinfo.bits_per_pixel == 32) {
						*((unsigned int *)(fbp + location)) = color;
					}
				}
			}
			break;

		case EOPFB_Clear:
			memset(fbp, 0x0, screensize);
			break;

		case EOPFB_Rect:
			for (jj = 0; jj < row; jj ++) {
				pv = (unsigned short*) (fbp + ((finfo.line_length * jj * vinfo.yres) / row));
				for (ii = 0; ii < col; ii ++) {
					pv1 =(unsigned short*) ((unsigned char*)pv + ((ii * vinfo.xres * 2) /col));
					r = (r_0 + r_r * jj + r_c * ii) & 0xff;
					g = (g_0 + g_r * jj + g_c * ii) & 0xff;
					b = (b_0 + b_r * jj + b_c * ii) & 0xff;
					value = ((r>>3) << 11) | ((g>>2) <<5) | (b &0x1f);

					count = vinfo.xres / col;
					while (count) {
						*pv1 = value;
						pv1 ++;
						count --;
					}
				}
				count = (vinfo.yres / row) - 1;
				pv1 = pv + (finfo.line_length/2);
				while (count) {
					memcpy(pv1, pv, finfo.line_length);
					pv1 += (finfo.line_length/2);
					count --;
				}
			}
			break;

		case EOPFB_DisplayBitmap:
			u_assert(bmp);
			if (vinfo.bits_per_pixel == 16) {
				ret = bmf_file_to_display_buffer(bmp, vinfo.xres, vinfo.yres, e_buffer_format_rgb565);
			} else if (vinfo.bits_per_pixel == 8) {
				ret = bmf_file_to_display_buffer(bmp, vinfo.xres, vinfo.yres, e_buffer_format_clut8);
			} else if (vinfo.bits_per_pixel == 32) {
				ret = bmf_file_to_display_buffer(bmp, vinfo.xres, vinfo.yres, e_buffer_format_rgba32);
			} else {
				u_printf_error("bad bits_per_pixel %d\n", vinfo.bits_per_pixel);
				break;
			}
			if (!ret) {
				memcpy(fbp, bmp->display_buf, bmp->display_buf_size);
			} else {
				u_printf_error("bmf_file_to_display_buffer fail, ret %d\n", ret);
			}
			break;

		default:
			u_printf_error("NOT supported op %d\n", op);
			break;
	}

	u_printf("[fb flow]: op %d, after memory copy, start display\n", op);

	if (ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo) < 0) {
		perror("FBIOPAN_DISPLAY");
		u_printf_error("[fb flow]: display fail\n");
		goto __op_framebuffer_1_error1;
	}

	u_printf("[fb flow]: after display\n");

__op_framebuffer_1_error1:

	if (munmap(fbp, screensize) < 0) {
		perror("munmap");
		u_printf_error("[fb flow]: munmap fail\n");
	}

	u_printf("[fb flow]: after munmap\n");

__op_framebuffer_1_error2:
	close(fbfd);

	u_printf("[fb flow]: after close %d\n", fbfd);

}

static int is_h264_file(char* filename)
{
	char* pchr = NULL;
	if (!filename) {
		u_printf_error("NULL filename\n");
		return 0;
	}

	pchr = strrchr(filename, '.');
	if (!pchr) {
		return 0;
	}
	if (!strcmp(pchr, ".h264")) {
		return 1;
	}
	if (!strcmp(pchr, ".264")) {
		return 1;
	}

	return 0;
}

static int get_framerate_from_filename(char* filename)
{
	char* pchr = NULL;
	if (!filename) {
		u_printf_error("NULL filename\n");
		return 0;
	}

	pchr = strrchr(filename, '.');
	if (!pchr) {
		return 0;
	}
	if ((pchr[-1] == '0') && (pchr[-2] == '3')) {
		return 30;
	}
	if ((pchr[-1] == '4') && (pchr[-2] == '2')) {
		return 24;
	}
	if ((pchr[-1] == '5') && (pchr[-2] == '2')) {
		return 25;
	}

	return 0;
}

static int get_tag_from_filename(char* filename)
{
	char* pchr = NULL;
	int tag = 0;
	if (!filename) {
		u_printf_error("NULL filename\n");
		return (16*1024);
	}

	pchr = strchr(filename, '-');
	if (!pchr) {
		return (16*1024);
	}

	*pchr = 0x0;
	sscanf(filename, "%d", &tag);
	*pchr = '-';

	//u_printf("[!!] find tag %d, filename %s\n", tag, filename);

	return tag;
}

static int find_first_file(int number_of_files)
{
	int i = 0;
	int min_file_tag = 1024*16;
	int file_index = -1;

	u_assert(number_of_files == (current_file_index + 1));
	while (i < number_of_files) {
		if (slot_has_content[i] && (pre_sort_tag[i] < min_file_tag)) {
			file_index = i;
			min_file_tag = pre_sort_tag[i];
		}
		i ++;
	}

	return file_index;
}

static void mudec_sort_files(int number_of_files)
{
	int current_index = 0;
	int finded_index = 0;

	while (current_index < number_of_files) {
		finded_index = find_first_file(number_of_files);
		if (0 > finded_index) {
			u_assert((current_index + 1) == number_of_files);
			break;
		}
		//u_printf("[!!]sorted index %d, find_index %d, filename %s, number_of_files %d\n", current_index, finded_index, pre_sorted_file_list[finded_index], number_of_files);
		slot_has_content[finded_index] = 0;

		memcpy(&file_list[current_index][0], &pre_sorted_file_list[finded_index][0], 256);

		file_codec[current_index] = pre_sorted_file_codec[finded_index];
		file_video_width[current_index] = pre_sorted_file_video_width[finded_index];
		file_video_height[current_index] = pre_sorted_file_video_height[finded_index];
		file_framerate[current_index] = pre_sorted_file_framerate[finded_index];
		file_prefer_HDMI_Mode[current_index] = pre_sorted_file_prefer_HDMI_Mode[finded_index];
		file_prefer_HDMI_FPS[current_index] = pre_sorted_file_prefer_HDMI_FPS[finded_index];
		is_hd[current_index] = pre_sorted_is_hd[finded_index];

		current_index ++;
	}

	return;
}

static void mudec_scandir_pre_sorted(char *path) {
	DIR *dir;
	int framerate_in_name = 0;
	char fullpath[1024],currfile[1024];
	char only_file_name[256];
	struct dirent *s_dir;
	struct stat file_stat;

	strcpy(fullpath,path);
	dir=opendir(fullpath);

	u_assert(!current_file_index);
	while ((s_dir=readdir(dir))!=NULL) {
		if ((strcmp(s_dir->d_name, ".")==0)||(strcmp(s_dir->d_name, "..")==0)) {
			continue;
		}
		sprintf(currfile,"%s/%s", fullpath, s_dir->d_name);
		stat(currfile, &file_stat);
		if (S_ISDIR(file_stat.st_mode)) {
			mudec_scandir_pre_sorted(currfile);
		} else {
			if (!is_h264_file(currfile)) {
				continue;
			}

			if (current_file_index >= MAX_FILES) {
				u_printf_error("max file number(current index %d, max value %d).\n", current_file_index, MAX_FILES);
				closedir(dir);
				return;
			}
			strncpy(&pre_sorted_file_list[current_file_index][0], currfile, sizeof(pre_sorted_file_list[current_file_index]) - 1);
			pre_sorted_file_list[current_file_index][sizeof(pre_sorted_file_list[current_file_index]) - 1] = 0x0;
			//set default values
			pre_sorted_file_codec[current_file_index] = UDEC_H264;
			pre_sorted_file_video_width[current_file_index] = 3840;
			pre_sorted_file_video_height[current_file_index] = 2160;
			pre_sorted_file_framerate[current_file_index] = 30;
			pre_sorted_is_hd[current_file_index] = 1;
			slot_has_content[current_file_index] = 1;

			snprintf(only_file_name, 256, "%s", s_dir->d_name);
			pre_sort_tag[current_file_index] = get_tag_from_filename(only_file_name);

			framerate_in_name = get_framerate_from_filename(currfile);
			if (24 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p24, %s\n", current_file_index, currfile);
				pre_sorted_file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P24;
				pre_sorted_file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_24;
			} else if (30 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p30, %s\n", current_file_index, currfile);
				pre_sorted_file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P30;
				pre_sorted_file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_30;
			} else if (25 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p25, %s\n", current_file_index, currfile);
				pre_sorted_file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P25;
				pre_sorted_file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_25;
			} else {
				if (1 || (current_file_index & 0x1)) {
					u_printf("current_file_index %d, default 1080p60, %s\n", current_file_index, currfile);
					pre_sorted_file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_1080P;
				} else {
					u_printf("current_file_index %d, 720p, %s\n", current_file_index, currfile);
					pre_sorted_file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_720P;
				}
				pre_sorted_file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_AUTO;
			}

			current_file_index ++;
		}
	}
	closedir(dir);

	if (current_file_index) {
		current_file_index --;
	}
}

static void mudec_scandir(char *path) {
	DIR *dir;
	int framerate_in_name = 0;
	char fullpath[1024],currfile[1024];
	struct dirent *s_dir;
	struct stat file_stat;

	strcpy(fullpath,path);
	dir=opendir(fullpath);

	u_assert(!current_file_index);
	while ((s_dir=readdir(dir))!=NULL) {
		if ((strcmp(s_dir->d_name, ".")==0)||(strcmp(s_dir->d_name, "..")==0)) {
			continue;
		}
		sprintf(currfile,"%s/%s", fullpath, s_dir->d_name);
		stat(currfile, &file_stat);
		if (S_ISDIR(file_stat.st_mode)) {
			mudec_scandir(currfile);
		} else {
			if (!is_h264_file(currfile)) {
				continue;
			}

			if (current_file_index >= MAX_FILES) {
				u_printf_error("max file number(current index %d, max value %d).\n", current_file_index, MAX_FILES);
				closedir(dir);
				return;
			}
			strncpy(&file_list[current_file_index][0], currfile, sizeof(file_list[current_file_index]) - 1);
			file_list[current_file_index][sizeof(file_list[current_file_index]) - 1] = 0x0;
			//set default values
			file_codec[current_file_index] = UDEC_H264;
			file_video_width[current_file_index] = 3840;
			file_video_height[current_file_index] = 2160;
			file_framerate[current_file_index] = 30;
			is_hd[current_file_index] = 1;

			framerate_in_name = get_framerate_from_filename(currfile);
			if (24 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p24, %s\n", current_file_index, currfile);
				file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P24;
				file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_24;
			} else if (30 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p30, %s\n", current_file_index, currfile);
				file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P30;
				file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_30;
			} else if (25 == framerate_in_name) {
				u_printf("current_file_index %d, 2160p25, %s\n", current_file_index, currfile);
				file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_2160P25;
				file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_25;
			} else {
				if (1 || (current_file_index & 0x1)) {
					u_printf("current_file_index %d, default 1080p60, %s\n", current_file_index, currfile);
					file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_1080P;
				} else {
					u_printf("current_file_index %d, 720p, %s\n", current_file_index, currfile);
					file_prefer_HDMI_Mode[current_file_index] = AMBA_VIDEO_MODE_720P;
				}
				file_prefer_HDMI_FPS[current_file_index] = AMBA_VIDEO_FPS_AUTO;
			}

			current_file_index ++;
		}
	}
	closedir(dir);

	if (current_file_index) {
		current_file_index --;
	}
}

static int do_test_mdec(int iav_fd)
{
#define DZOOM_FACTOR_STEP 0x2000
#define DINPUT_WIN_STEP 20
	unsigned int i;
	pthread_t thread_id[MAX_NUM_UDEC];
	udec_instance_param_t params[MAX_NUM_UDEC];
	iav_udec_vout_config_t vout_cfg[NUM_VOUT];
	int vout_start_index = -1;
	int num_of_vout = 0;
	msg_t msg;
	//unsigned int tot_number_udec;
	int foreground_stream_cnt, background_stream_cnt;
	int total_num;

	char buffer_old[128] = {0};
	char buffer[128];
	char* p_buffer = buffer;
	int flag_stdin = 0;

	int cur_display = 0;// 0: 4xwindow, 1: 1xwindow

	//zoom related
	u32 last_zoom_mode = 0;
	u32 zoom_factor_x = 0x10000, zoom_factor_y = 0x10000;
	u16 input_w, input_h, center_x, center_y;

	signal(SIGINT, sig_stop);
	signal(SIGQUIT, sig_stop);
	signal(SIGTERM, sig_stop);

	dec_types = 0x17;	// no RV40, no hybrid MPEG4
	ppmode = 3;

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

	memset(&mdec_mode, 0x0, sizeof(mdec_mode));

	total_num = current_file_index + 1;
	_get_stream_number(&foreground_stream_cnt, &background_stream_cnt, total_num);

	if ((input_sds  >= 0) || (input_hds >= 0)) {
		u_printf("input_sds %d, input_hds %d, from files foreground_stream_cnt %d, background_stream_cnt %d\n", input_sds, input_hds, foreground_stream_cnt, background_stream_cnt);
		if ((foreground_stream_cnt + background_stream_cnt) >= (input_sds + input_hds)) {
			foreground_stream_cnt = input_sds;
			background_stream_cnt = input_hds;
		} else {
			u_printf_error("not valid input sds %d, hds %d\n", input_sds, input_hds);
		}
	}

	//ugly code here, to do
	if ((background_stream_cnt > 1) && (0 == foreground_stream_cnt)) {
		//change background_stream_cnt to foreground_stream_cnt
		u_printf("[info]: play multiple background_stream_cnt\n");
		if (background_stream_cnt > 4) {
			foreground_stream_cnt = 4;
			background_stream_cnt = background_stream_cnt - 4;
		} else {
			foreground_stream_cnt = background_stream_cnt;
			background_stream_cnt = 0;
		}
	}

	if ((1 == total_num) || (0 == foreground_stream_cnt)) {
		if (!first_show_full_screen) {
			first_show_full_screen = 1;
			u_printf("[change settings]: first use full screen show one stream playback.\n");
		}
	}

	if (0 == foreground_stream_cnt) {
		if (!first_show_hd_stream) {
			first_show_hd_stream = 1;
			u_printf("[change settings]: first show hd stream.\n");
		}
	}

	u_printf("[flow]: before malloc some memory, current_file_index %d, total_num %d, foreground_stream_cnt %d, background_stream_cnt %d, first_show_full_screen %d, first_show_hd_stream %d\n", current_file_index, total_num, foreground_stream_cnt, background_stream_cnt, first_show_full_screen, first_show_hd_stream);

/*
	if (foreground_stream_cnt > 4 && 0 == background_stream_cnt) {
		u_printf("[debug]: make 5'th stream as hd, for debug.\n");
		background_stream_cnt = 1;
		foreground_stream_cnt = 4;
	}
*/

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
		if (get_single_vout_info(i, vout_width + i, vout_height + i, iav_fd) < 0) {
			u_printf_debug("[vout info query fail]: vout(%d) fail\n", i);
			vout_width[i] = 0;
			vout_height[i] = 0;
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
		u_printf("[vout info query]: vout(%d), width %d, height %d, rotate %d.\n", i, *(vout_width + i), *(vout_height + i), vout_rotate[i]);
	}
	u_printf("[vout info query]: max_vout_width %d, max_vout_height %d, vout_start_index %d, num_of_vout %d.\n", max_vout_width, max_vout_height, vout_start_index, num_of_vout);

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

	//process request vout mask
	if (1 == mudec_request_voutmask) {
		//LCD's case
		if (!vout_width[0] || !vout_height[0]) {
			u_printf_error("[vout info error]: LCD device is not avaiable!!!\n");
			goto do_test_mdec_exit;
		}
		vout_start_index = 0;
		num_of_vout = 1;
		mudec_actual_voutmask = 1;
		mudec_actual_vout_start_index = 0;
	} else if (2 == mudec_request_voutmask) {
		//HDMI's case
		if (!vout_width[1] || !vout_height[1]) {
			u_printf_error("[vout info error]: HDMI device is not avaiable!!!\n");
			goto do_test_mdec_exit;
		}
		vout_start_index = 1;
		num_of_vout = 1;
		mudec_actual_voutmask = 2;
		mudec_actual_vout_start_index = 1;
	} else if (3 == mudec_request_voutmask) {
		//LCD + HDMI's case
		if (!vout_width[0] || !vout_height[0]) {
			u_printf_error("[vout info error]: LCD device is not avaiable!!!\n");
			goto do_test_mdec_exit;
		}
		if (!vout_width[1] || !vout_height[1]) {
			u_printf_error("[vout info error]: HDMI device is not avaiable!!!\n");
			goto do_test_mdec_exit;
		}
		vout_start_index = 0;
		num_of_vout = 2;
		mudec_actual_voutmask = 3;
		mudec_actual_vout_start_index = 0;
	} else {
		u_printf_error("BAD vout mask 0x%x\n", mudec_request_voutmask);
		goto do_test_mdec_exit;
	}

	u_printf("[flow]: before init_udec_mode_config\n");
	init_udec_mode_config(udec_mode);
	udec_mode->num_udecs = total_num;
	udec_mode->udec_config = udec_configs;

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.windows_config = windows;
	//mdec_mode.render_config = renders;

	mdec_mode.av_sync_enabled = 0;
	if (2 == mudec_actual_voutmask) {
		//mdec_mode.out_to_hdmi = 1;
	} else if (1 == mudec_actual_voutmask) {
		//mdec_mode.out_to_hdmi = 0;
	} else if (3 == mudec_actual_voutmask) {
		//is it correct?
		//mdec_mode.out_to_hdmi = 1;
	}
	mdec_mode.audio_on_win_id = 0;

	mdec_mode.enable_buffering_ctrl = 0;
	mdec_mode.pre_buffer_len = max_frm_num - 4;

	//fill display size
	if (0 == vout_rotate[mudec_actual_vout_start_index]) {
		mdec_mode.video_win_width = vout_width[mudec_actual_vout_start_index];
		mdec_mode.video_win_height = vout_height[mudec_actual_vout_start_index];
	} else {
		mdec_mode.video_win_width = vout_height[mudec_actual_vout_start_index];
		mdec_mode.video_win_height = vout_width[mudec_actual_vout_start_index];
	}

	//preset display layout
	//if (display_layout) {
		__different_display_layout(mudec_actual_vout_start_index, display_layout, (foreground_stream_cnt & 0xff) | ((background_stream_cnt & 0xff) << 8), mdec_mode.video_win_width, mdec_mode.video_win_height);

		//calculate other vout display parameters
		if (1) {
			u32 iii;
			u16 iii_width, iii_height;
			for (iii = 0; iii < 2; iii ++) {
				if (mudec_actual_vout_start_index != iii) {
					if (0 == vout_rotate[iii]) {
						iii_width = vout_width[iii];
						iii_height = vout_height[iii];
					} else {
						iii_width = vout_height[iii];
						iii_height = vout_width[iii];
					}
					__different_display_layout(iii, display_layout, (foreground_stream_cnt & 0xff) | ((background_stream_cnt & 0xff) << 8), iii_width, iii_height);
				}
			}
		}
	//}

	if (6 == display_layout) {
		foreground_stream_cnt = foreground_stream_cnt + background_stream_cnt;
		background_stream_cnt = 0;
	}

	if (foreground_stream_cnt) {
		u_printf("[flow]: before init_udec_configs, first layer udecs (number %d)\n", foreground_stream_cnt);
		for (i = 0; i < foreground_stream_cnt; i ++) {
			init_udec_configs(udec_configs + i, 1, file_video_width[i], file_video_height[i]);
		}
	}

	if (background_stream_cnt) {
		u_printf("[flow]: before init_udec_configs, second layer udecs (number %d)\n", background_stream_cnt);
		for (i = foreground_stream_cnt; i < foreground_stream_cnt + background_stream_cnt; i ++) {
			init_udec_configs(udec_configs + i, 1, file_video_width[i], file_video_height[i]);
		}
	}

	if (foreground_stream_cnt) {
		u_printf("[flow]: before init_udec_windows, first layer udecs (number %d)\n", foreground_stream_cnt);
		for (i = 0; i < foreground_stream_cnt; i ++) {
			init_udec_windows(windows, i, 1, file_video_width[i], file_video_height[i]);
		}
	}

	if (background_stream_cnt) {
		u_printf("[flow]: before init_udec_windows, second layer (number %d)\n", background_stream_cnt);
		for (i = foreground_stream_cnt; i < foreground_stream_cnt + background_stream_cnt; i ++) {
			init_udec_windows(windows, i, 1, file_video_width[i], file_video_height[i]);
		}
	}

	if (foreground_stream_cnt) {
		u_printf("[flow]: before init_udec_renders, first layer (number %d)\n", foreground_stream_cnt);
		init_udec_renders(renders, foreground_stream_cnt);
	}

	if (background_stream_cnt) {
		//for hd
		u_printf("[flow]: before init_udec_renders, second layer, (number %d)\n", background_stream_cnt);
		tmp_renders = renders + foreground_stream_cnt;
		for (i = 0; i < background_stream_cnt; i++, tmp_renders ++) {
			init_udec_renders_single(tmp_renders, i, i + foreground_stream_cnt, 0xff, i + foreground_stream_cnt);
		}
	}

	display_layer_number[0][0] = display_layer_number[1][0] = foreground_stream_cnt;
	display_layer_number[0][1] = display_layer_number[1][1] = background_stream_cnt;

	//config renders, which will display
	#if 1
	if (!first_show_hd_stream) {
		mdec_mode.total_num_render_configs = foreground_stream_cnt;
		mdec_mode.render_config = renders;

		if (foreground_stream_cnt > 1) {
			cur_display = 0;
		} else if (1 ==  foreground_stream_cnt){
			u_printf("[flow]: show sd stream, but full screen.\n");
			cur_display = 1;
		} else {
			u_printf_error("[error]: invalid settings, foreground_stream_cnt(%d) <= 0?\n", foreground_stream_cnt);
			goto do_test_mdec_exit;
		}
		current_display_layer = 0;
	} else {
		mdec_mode.total_num_render_configs = display_layer_number[mudec_actual_vout_start_index][1];
		tmp_renders = renders + foreground_stream_cnt;
		mdec_mode.render_config = tmp_renders;

		cur_display = 1;
		current_display_layer = 1;
	}
	#else
		mdec_mode.total_num_render_configs = foreground_stream_cnt+background_stream_cnt;
		mdec_mode.render_config = renders;
	#endif

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.max_num_windows = total_num + 1;

	u_printf("[flow]: before IAV_IOC_ENTER_MDEC_MODE, num render configs %d, num win configs %d, max num windows %d\n", mdec_mode.total_num_render_configs, mdec_mode.total_num_win_configs, mdec_mode.max_num_windows);
	if (ioctl(iav_fd, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode) < 0) {
		perror("IAV_IOC_ENTER_MDEC_MODE");
		u_printf_error(" enter mdec mode fail\n");
		goto do_test_mdec_exit;
	}

	u_printf("[flow]: enter mdec mode done\n");

	i = 0;
	//each instance's parameters, sd
	for (i = 0; i < foreground_stream_cnt; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = bits_fifo_size;
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
		params[i].p_vout_config = &vout_cfg[mudec_actual_vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");

		params[i].file_fd[1] = NULL;
	}

	//each instance's parameters, sd
	for (; i < foreground_stream_cnt + background_stream_cnt; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = bits_fifo_size;
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
		params[i].p_vout_config = &vout_cfg[mudec_actual_vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");

		params[i].file_fd[1] = NULL;
	}

	current_vout_start_index = mudec_actual_vout_start_index;

	//each instance's USEQ, UPES header
	for (i = 0; i < foreground_stream_cnt + background_stream_cnt; i++) {
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

		params[i].current_playback_strategy = PB_STRATEGY_ALL_FRAME;
		params[i].tobe_playback_strategy = PB_STRATEGY_ALL_FRAME;
	}

	debug_p_windows = windows;
	debug_p_renders = renders;
	debug_sds = foreground_stream_cnt;
	debug_hds = background_stream_cnt;

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

	unsigned int input_flag;

	//main cmd loop
	while (mdec_running) {
		//add sleep to avoid affecting the performance
		usleep(100000);
		memset(buffer, 0x0, sizeof(buffer));
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
				user_press_quit = 1;

				//resume all udecs in this case
				for (i = 0; i < foreground_stream_cnt + background_stream_cnt; i ++) {
					udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
				}
				break;

			case 'h':	// help
				u_printf("Print help\n");
				_print_ut_cmds();
				break;

			case 's':	//step
				if (1 != sscanf(p_buffer, "s%d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type 's\%d' and enter, to do step mode for the udec(\%d).\n");
					break;
				}

				u_assert(switch_new_udec_id >= 0);
				u_assert(switch_new_udec_id < (background_stream_cnt + foreground_stream_cnt));
				if ((switch_new_udec_id < 0) || (switch_new_udec_id >= (background_stream_cnt + foreground_stream_cnt))) {
					u_printf_error("BAD input params, udec_id %d, out of range.\n", switch_new_udec_id);
					break;
				}

				udec_step(params + switch_new_udec_id);
				break;

			case ' ':
				if (1 != sscanf(p_buffer, " %d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type ' \%d' and enter, to do pause/resume the udec(\%d).\n");
					break;
				}

				u_assert(switch_new_udec_id >= 0);
				u_assert(switch_new_udec_id < (background_stream_cnt + foreground_stream_cnt));
				if ((switch_new_udec_id < 0) || (switch_new_udec_id >= (background_stream_cnt + foreground_stream_cnt))) {
					u_printf_error("BAD input params, udec_id %d, out of range.\n", switch_new_udec_id);
					break;
				}

				udec_pause_resume(params + switch_new_udec_id);
				break;

			case 'z':
				if ('0' == p_buffer[1]) {
					//stop(0)
					input_flag = 0x0;
				} else if ('1' == p_buffer[1]) {
					//stop(1)
					input_flag = 0x1;
				} else if ('f' == p_buffer[1]) {
					//stop(0xff)
					input_flag = 0xff;
				} else {
					u_printf("not support cmd %s.\n", p_buffer);
					u_printf("   type 'z0\%d' to send stop(0) to udec(\%d).\n");
					u_printf("   type 'z1\%d' to send stop(1) to udec(\%d).\n");
					u_printf("   type 'zf\%d' to send stop(0xff) to udec(\%d).\n");
					break;
				}

				if (1 != sscanf(p_buffer + 2, "%d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type 'z%c\%d' and enter, to send stop(%x) for udec(\%d).\n", p_buffer[1], input_flag);
					break;
				}
				ioctl_udec_stop(iav_fd, switch_new_udec_id, input_flag);
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
					u_assert(switch_new_udec_id < (background_stream_cnt + foreground_stream_cnt));
					if ((switch_render_id >= 4) || (switch_new_udec_id >= (background_stream_cnt + foreground_stream_cnt))) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if ((switch_render_id < 0) || (switch_new_udec_id < 0)) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if (renders[switch_render_id].udec_id != switch_new_udec_id) {
						if (!test_feed_background) {
							adjust_start_pts_ex(params, switch_new_udec_id, renders[switch_render_id].udec_id, foreground_stream_cnt + background_stream_cnt);
						}

						if (!test_feed_background) {
							//resume feeding
							//u_printf("[flow]: resume feeding for thread %d.\n", switch_new_udec_id);
							resume_pause_feeding(params, switch_new_udec_id, switch_new_udec_id, foreground_stream_cnt + background_stream_cnt, 0);

							//resume UDEC if needed
							udec_trickplay(params + switch_new_udec_id, UDEC_TRICKPLAY_RESUME);
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
								resume_pause_feeding(params, renders[switch_render_id].udec_id, renders[switch_render_id].udec_id, foreground_stream_cnt + background_stream_cnt, 1);
								udec_trickplay(params + renders[switch_render_id].udec_id, UDEC_TRICKPLAY_PAUSE);
							}

						//}
						renders[switch_render_id].udec_id = switch_new_udec_id;

						//update display if needed
						if (switch_auto_update_display) {
							if ((0 == cur_display) && (switch_new_udec_id >= foreground_stream_cnt)) {
								u_printf("[flow]: auto update display to 1 x window.\n");
								//switch to one window display
								for (i = foreground_stream_cnt; i < foreground_stream_cnt + background_stream_cnt; i ++) {
									renders[i].render_id = i - foreground_stream_cnt;
									renders[i].win_config_id = i;
									renders[i].win_config_id_2nd = 0xff;//hard code here
									renders[i].udec_id = i;

									render_2_udec[renders[i].render_id] = renders[i].udec_id;
									render_2_window[renders[i].render_id] = renders[i].win_config_id;

								}
								//#endif
								ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
								cur_display = 1;
								current_display_layer = 1;
								if (!test_feed_background) {
									//pause foreground_stream_cnt
									resume_pause_feeding(params, 0, foreground_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 1);
								}

							} else if ((1 == cur_display) && (switch_new_udec_id < foreground_stream_cnt)) {
								u_printf("[flow]: auto update display to 4 x window.\n");

								if (!test_feed_background) {
									//resume if needed
									resume_pause_feeding(params, 0, foreground_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 0);

									for (i = 0; i < foreground_stream_cnt; i ++) {
										//resume UDEC if needed
										udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
									}
								}

								ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
								cur_display = 0;
								current_display_layer = 0;
								if (!test_feed_background) {
									//pause background_stream_cnt
									resume_pause_feeding(params, foreground_stream_cnt, foreground_stream_cnt + background_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 1);
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
						for (i = 0; i < foreground_stream_cnt; i ++) {
							renders[i].render_id = i;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;
						}
						#endif
						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, 0, foreground_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 0);

							for (i = 0; i < foreground_stream_cnt; i ++) {
								//resume UDEC if needed
								udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
							}
						}
						ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
						if (!test_feed_background) {
							//pause background_stream_cnt
							resume_pause_feeding(params, foreground_stream_cnt, foreground_stream_cnt + background_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 1);
						}
						current_display_layer = 0;
						cur_display = 0;
					} else if ('h' == p_buffer[2] && 'd' == p_buffer[3]) {
						// restore display 1x hd's case
						//#if 0
						for (i = foreground_stream_cnt; i < foreground_stream_cnt + background_stream_cnt; i ++) {
							renders[i].render_id = i - foreground_stream_cnt;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;

							render_2_udec[renders[i].render_id] = renders[i].udec_id;
							render_2_window[renders[i].render_id] = renders[i].win_config_id;
						}
						//#endif

						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, foreground_stream_cnt, foreground_stream_cnt + background_stream_cnt -1, foreground_stream_cnt + background_stream_cnt, 0);

							//resume UDEC if needed
							udec_trickplay(params + foreground_stream_cnt, UDEC_TRICKPLAY_RESUME);
						}

						ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
						if (!test_feed_background) {
							//pause foreground_stream_cnt
							resume_pause_feeding(params, 0, foreground_stream_cnt - 1, foreground_stream_cnt + background_stream_cnt, 1);
						}
						current_display_layer = 1;
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

							render_2_udec[renders[i].render_id] = renders[i].udec_id;
							render_2_window[renders[i].render_id] = renders[i].win_config_id;

							i ++;
						}
						ioctl_render_cmd(iav_fd, i, i, renders);
					} else {
						u_printf("\t[not supported cmd]: for update render for NVR, please use 'crsd' to show 4xsd, 'crhd' to show 1xhd.'\n");
						u_printf("\t                              or use 'cr render_number:win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id...'\n");
					}
				} else if ('p' == p_buffer[1]) {
					unsigned int speed, speed_frac;
					unsigned int dec_id = 0;
					unsigned int max_speed = 1;
					unsigned short max_speed_hex = 0, max_speed_hex_frac = 0;

					if ('a' == p_buffer[2]) {
						//auto mode, if speed > max possiable speed, use I ONLY mode
						if (3 == sscanf(p_buffer, "cpa:%d %x.%x", &dec_id, &speed, &speed_frac)) {
							u_printf("specify playback speed %x.%x\n", speed, speed_frac);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								max_speed = __get_current_max_pb_speed(dec_id, foreground_stream_cnt + background_stream_cnt, &max_speed_hex, &max_speed_hex_frac);
								u_printf("current max speed %u, %04hx.%04hx\n", max_speed, max_speed_hex, max_speed_hex_frac);
								if (speed > max_speed) {
									u_printf("speed(%d) > %d, use I only mode\n", speed, max_speed);

									last_scan_mode = 0x02;
								} else {
									last_scan_mode = 0x00;
								}
								last_speed = speed;
								last_speed_frac = speed_frac;


								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									if (speed > max_speed) {
										params[dec_id].tobe_playback_strategy_flushed = PB_STRATEGY_I_ONLY;
									} else {
										params[dec_id].tobe_playback_strategy_flushed = PB_STRATEGY_ALL_FRAME;
									}
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
									if (speed > max_speed) {
										set_playback_mode(params, dec_id, PB_STRATEGY_I_ONLY, foreground_stream_cnt + background_stream_cnt);
									} else {
										set_playback_mode(params, dec_id, PB_STRATEGY_ALL_FRAME, foreground_stream_cnt + background_stream_cnt);
									}
								}

								if (last_speed_frac < 0x8000) {
									cur_pb_speed[dec_id] = last_speed;
								} else {
									cur_pb_speed[dec_id] = last_speed + 1;
								}
							}
						} else {
							u_printf("you should type 'cpa:udec_id speed.speed_frac' and enter to specify playback speed, if speed >=4x, will choose I only mode\n");
						}
					} else if ('i' == p_buffer[2]) {
						//I only mode
						if (3 == sscanf(p_buffer, "cpi:%d %x.%x", &dec_id, &speed, &speed_frac)) {
							u_printf("specify playback speed %x.%x, feed I only\n", speed, speed_frac);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								last_scan_mode = 0x02;
								last_speed = speed;
								last_speed_frac = speed_frac;

								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									params[dec_id].tobe_playback_strategy_flushed = PB_STRATEGY_I_ONLY;
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
									set_playback_mode(params, dec_id, PB_STRATEGY_I_ONLY, foreground_stream_cnt + background_stream_cnt);
								}

								if (last_speed < 8) {
									cur_pb_speed[dec_id] = 1;
								} else {
									cur_pb_speed[dec_id] = (last_speed + 1) / 8;
								}
							}
						} else {
							u_printf("you should type 'cpi:udec_id speed.speed_frac' and enter to specify playback speed, feed I only\n");
						}
					} else if ('r' == p_buffer[2]) {
						//Ref only mode
						if (3 == sscanf(p_buffer, "cpr:%d %x.%x", &dec_id, &speed, &speed_frac)) {
							u_printf("specify playback speed %x.%x, feed Ref only\n", speed, speed_frac);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								last_scan_mode = 0x01;
								last_speed = speed;
								last_speed_frac = speed_frac;

								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									params[dec_id].tobe_playback_strategy_flushed = PB_STRATEGY_REF_ONLY;
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
									set_playback_mode(params, dec_id, PB_STRATEGY_REF_ONLY, foreground_stream_cnt + background_stream_cnt);
								}

							}
						} else {
							u_printf("you should type 'cpr:udec_id speed.speed_frac' and enter to specify playback speed, feed Ref only\n");
						}
					} else if (':' == p_buffer[2]) {
						//only set playback speed
						if (3 == sscanf(p_buffer, "cp:%d %x.%x", &dec_id, &speed, &speed_frac)) {
							u_printf("specify playback speed %x.%x\n", speed, speed_frac);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								last_speed = speed;
								last_speed_frac = speed_frac;

								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									params[dec_id].tobe_playback_strategy_flushed = params[dec_id].current_playback_strategy;
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
								}

								if (last_speed_frac < 0x8000) {
									cur_pb_speed[dec_id] = last_speed;
								} else {
									cur_pb_speed[dec_id] = last_speed + 1;
								}
							}
						} else {
							u_printf("you should type 'cp:udec_id speed.speed_frac' and enter to specify playback speed, only issue speed cmd to DSP\n");
						}
					} else if (('c' == p_buffer[2])) {
						//clear speed setting
						if (1 == sscanf(p_buffer, "cpc:%d", &dec_id)) {
							u_printf("clear playback speed setting, for udec %d\n", dec_id);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								last_scan_mode = 0x00;
								last_speed = 0x01;
								last_speed_frac = 0x00;
								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									params[dec_id].tobe_playback_strategy_flushed = PB_STRATEGY_ALL_FRAME;
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									set_playback_mode(params, dec_id, PB_STRATEGY_ALL_FRAME, foreground_stream_cnt + background_stream_cnt);
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
								}
								cur_pb_speed[dec_id] = 1;
							}
						} else {
							u_printf("you should type 'cpc:udec_id' and enter to clear playback speed setting, reset to 1x, all frame mode\n");
						}
					} else if (('m' == p_buffer[2])) {
						//only change pb-strategy
						if (1 == sscanf(p_buffer, "cpm:%d strategy %d", &dec_id, &speed)) {
							u_printf("set playback mode %d, for udec %d\n", speed, dec_id);
							if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
								u_printf_error("BAD dec_id %d\n", dec_id);
							} else {
								if (PB_STRATEGY_ALL_FRAME == speed) {
									last_scan_mode = 0x00;
								} else if (PB_STRATEGY_REF_ONLY == speed) {
									last_scan_mode = 0x01;
								} else if (PB_STRATEGY_I_ONLY == speed) {
									last_scan_mode = 0x02;
								} else {
									u_printf_error("NOT supported strategy %d\n", speed);
									break;
								}

								if (change_speed_with_fluch) {
									params[dec_id].tobe_playback_direction_flushed = 0;
									params[dec_id].tobe_playback_strategy_flushed = params[dec_id].current_playback_strategy;
									params[dec_id].tobe_playback_speed_flushed = last_speed;
									params[dec_id].tobe_playback_speed_frac_flushed = last_speed_frac;
									params[dec_id].tobe_playback_scan_mode_flushed = last_scan_mode;

									msg_t msg;
									msg.cmd = M_MSG_UPDATE_SPEED;
									msg.ptr = NULL;
									msg_queue_put(&params[dec_id].cmd_queue, &msg);
								} else {
									set_playback_mode(params, dec_id, PB_STRATEGY_ALL_FRAME, foreground_stream_cnt + background_stream_cnt);
									ioctl_playback_speed(iav_fd, dec_id, last_speed, last_speed_frac, last_scan_mode, 0);
								}

							}
						} else {
							u_printf("you should type 'cpm:udec_id strategy' and enter to set playback mode(0: all frames, 1: ref only, 2: I only, 3: IDR only)\n");
						}
					} else {
						u_printf("you should type 'cpa:udec_id speed.speed_frac' and enter to specify playback speed\n");
						u_printf("\tor type 'cpi:udec_id speed.speed_frac' and enter to specify playback speed, I only mode\n");
						u_printf("\tor type 'cpr:udec_id speed.speed_frac' and enter to specify playback speed, feed Ref only\n");
						u_printf("\tor type 'cp:udec_id speed.speed_frac' and enter to specify playback speed\n");
						u_printf("\tor type 'cpc:udec_id' and enter to clear playback speed setting\n");
						u_printf("\tor type 'cpm:udec_id strategy' and enter to set playback mode(0: all frames, 1: ref only, 2: I only, 3: IDR only)\n");
					}

				} else if ('c' == p_buffer[1]) {
					unsigned int cap_index = 0;
					unsigned int dec_id = 0;
					if (2 == sscanf(p_buffer, "ccjpeg:%x %x", &dec_id, &cap_index)) {
						u_printf("capture udec_id %x, file index %x\n", dec_id, cap_index);
						if ((IAV_ALL_UDEC_TAG != dec_id) && (dec_id >= foreground_stream_cnt + background_stream_cnt)) {
							u_printf_error("BAD dec_id %x\n", dec_id);
						} else {
							do_playback_capture(iav_fd, dec_id, cap_index);
							u_printf("capture udec_id %x, file index %d done.\n", dec_id, cap_index);
						}
					} else if (2 == sscanf(p_buffer, "ccjpegns:%x %x", &dec_id, &cap_index)) {
						u_printf("capture udec_id %x, file index %x\n", dec_id, cap_index);
						if ((IAV_ALL_UDEC_TAG != dec_id) && (dec_id >= foreground_stream_cnt + background_stream_cnt)) {
							u_printf_error("BAD dec_id %x\n", dec_id);
						} else {
							do_playback_capture_detailed(iav_fd, dec_id, cap_index, 1, 1, 0);
							u_printf("capture(no screennail) udec_id %x, file index %d done.\n", dec_id, cap_index);
						}
					} else if (2 == sscanf(p_buffer, "ccjpegnt:%x %x", &dec_id, &cap_index)) {
						u_printf("capture udec_id %x, file index %x\n", dec_id, cap_index);
						if ((IAV_ALL_UDEC_TAG != dec_id) && (dec_id >= foreground_stream_cnt + background_stream_cnt)) {
							u_printf_error("BAD dec_id %x\n", dec_id);
						} else {
							do_playback_capture_detailed(iav_fd, dec_id, cap_index, 1, 0, 1);
							u_printf("capture(no thumbnail) udec_id %x, file index %d done.\n", dec_id, cap_index);
						}
					} else if (2 == sscanf(p_buffer, "ccjpegntns:%x %x", &dec_id, &cap_index)) {
						u_printf("capture udec_id %x, file index %x\n", dec_id, cap_index);
						if (dec_id >= foreground_stream_cnt + background_stream_cnt) {
							u_printf_error("BAD dec_id %x\n", dec_id);
						} else {
							do_playback_capture_detailed(iav_fd, dec_id, cap_index, 1, 0, 0);
							u_printf("capture(no thumbnail, no screennail) udec_id %x, file index %x done.\n", dec_id, cap_index);
						}
					} else if (1 == sscanf(p_buffer, "ccjpegquality:%d", &cap_index)) {
						u_printf("set capture quality %d\n", cap_index);
						capture_quality = cap_index;
					} else {
						u_printf("you should type 'ccjpeg:udec_id file_index' and enter to do playback capture(coded + thumbnail + screennail)\n");
						u_printf("   or type 'ccjpegns:udec_id file_index' and enter to do playback capture(coded + thumbnail)\n");
						u_printf("   or type 'ccjpegnt:udec_id file_index' and enter to do playback capture(coded + screennail)\n");
						u_printf("   or type 'ccjpegntns:udec_id file_index' and enter to do playback capture(coded)\n");
						u_printf("   or type 'ccjpegquality:quality' and enter to specify jpeg quality\n");
					}
				} else if ('z' == p_buffer[1]) {
					int render_id;
					if ('1' == p_buffer[2]) {
						//mode 1, specify zoom factor
						if (3 == sscanf(p_buffer + 4, "%d,%x,%x", &render_id, &zoom_factor_x, &zoom_factor_y)) {
							if (__zoom_params_valid_mode1(render_id, zoom_factor_x, zoom_factor_y)) {
								ioctl_zoom_mode_1(iav_fd, render_id, zoom_factor_x, zoom_factor_y);
								last_zoom_mode = 1;
							} else {
								u_printf("zoom parameters excced expect range(mode 1)\n");
							}
						} else {
							u_printf("you should type 'cz1:render_id,zoomfactorx,zoomfactory', zoomfactor use hex format\n");
						}
					} else if ('2' == p_buffer[2]) {
						//mode 2, specify input rect
						if (5 == sscanf(p_buffer + 4, "%d,%hd,%hd,%hd,%hd", &render_id, &input_w, &input_h, &center_x, &center_y)) {
							if (__zoom_params_valid_mode2(render_id, input_w, input_h, center_x, center_y)) {
								ioctl_zoom_mode_2(iav_fd, render_id, input_w, input_h, center_x, center_y);
								last_zoom_mode = 2;
							} else {
								u_printf("zoom parameters excced expect range(mode 2)\n");
							}
						} else {
							u_printf("you should type 'cz2:render_id,input_with,input_height,center_x,center_y', use decimal format\n");
						}
					} else if ('i' == p_buffer[2]) {
						//repeat last mode
						if (1 == last_zoom_mode) {
							//check zoom factor
							if ((zoom_factor_x > DZOOM_FACTOR_STEP) && (zoom_factor_y > DZOOM_FACTOR_STEP)) {
								zoom_factor_x -= DZOOM_FACTOR_STEP;
								zoom_factor_y -= DZOOM_FACTOR_STEP;
								ioctl_zoom_mode_1(iav_fd, render_id, zoom_factor_x, zoom_factor_y);
							} else {
								u_printf("cannot zoom in now, current zoom factor %x,%x\n", zoom_factor_x, zoom_factor_y);
							}
						} else if (2 == last_zoom_mode) {
							//check inout window
							if ((input_w > DINPUT_WIN_STEP) && (input_h > DINPUT_WIN_STEP)) {
								input_w -= DINPUT_WIN_STEP;
								input_h -= DINPUT_WIN_STEP;
								ioctl_zoom_mode_2(iav_fd, render_id, input_w, input_h, center_x, center_y);
							} else {
								u_printf("cannot zoom in now, input width %d, height %d\n", input_w, input_h);
							}
						}
					} else if ('o' == p_buffer[2]) {
						//mode 2, specify input rect
						if (1 == last_zoom_mode) {
							//check zoom factor
							if (__zoom_params_valid_mode1(render_id, zoom_factor_x + DZOOM_FACTOR_STEP, zoom_factor_y + DZOOM_FACTOR_STEP)) {
								zoom_factor_x += DZOOM_FACTOR_STEP;
								zoom_factor_y += DZOOM_FACTOR_STEP;
								ioctl_zoom_mode_1(iav_fd, render_id, zoom_factor_x, zoom_factor_y);
							} else {
								u_printf("cannot zoom out now, current zoom factor %x,%x\n", zoom_factor_x, zoom_factor_y);
							}
						} else if (2 == last_zoom_mode) {
							//check inout window
							if (__zoom_params_valid_mode2(render_id, input_w + DINPUT_WIN_STEP, input_h + DINPUT_WIN_STEP, center_x, center_y)) {
								input_w += DINPUT_WIN_STEP;
								input_h += DINPUT_WIN_STEP;
								ioctl_zoom_mode_2(iav_fd, render_id, input_w, input_h, center_x, center_y);
							} else {
								u_printf("cannot zoom out now, current input win width %d, height %d\n", input_w, input_h);
							}
						}
					} else {
						u_printf("you should type 'cz1:render_id,zoomfactorx,zoomfactory', zoomfactor use hex format\n");
						u_printf("or type 'cz2:render_id,input_with,input_height,center_x,center_y', use decimal format\n");
						u_printf("or type 'czi', zoom in with current zoom mode\n");
						u_printf("or type 'czo', zoom out with current zoom mode\n");
					}
				} else if ('d' == p_buffer[1]) {
					//display update
					u32 request_vout = 0;
					if (1 == sscanf(p_buffer, "cd%d", &request_vout)) {
						if (request_vout == current_vout_start_index) {
							u_printf("already display on vout %d\n", request_vout);
							break;
						} else if (request_vout >= TOT_VOUT_NUMBER) {
							u_printf_error("request vout index %d exceed valid range(%d)\n", request_vout, TOT_VOUT_NUMBER);
							break;
						} else {
							if (0 == current_display_layer) {
								__update_mudec_display(iav_fd, request_vout, windows, renders, total_num, foreground_stream_cnt);
							} else if (1 == current_display_layer) {
								__update_mudec_display(iav_fd, request_vout, windows, renders + foreground_stream_cnt, total_num, background_stream_cnt);
							}
							current_vout_start_index = request_vout;
						}
					} else {
						u_printf("it's better to specify request vout index: press 'cd%%d', to specify vout index(0 means LCD, 1 means HDMI)\n");
						if (0 == current_vout_start_index) {
							request_vout = 1;
						} else {
							request_vout = 0;
						}
						if (0 == current_display_layer) {
							__update_mudec_display(iav_fd, request_vout, windows, renders, total_num, foreground_stream_cnt);
						} else if (1 == current_display_layer) {
							__update_mudec_display(iav_fd, request_vout, windows, renders + foreground_stream_cnt, total_num, background_stream_cnt);
						}
						current_vout_start_index = request_vout;
					}
				} else if ('y' == p_buffer[1]) {
					unsigned int dec_id = 0;
					unsigned int cap_interval = 0;
					if (2 == sscanf(p_buffer, "cycjpeg:%u %u", &dec_id, &cap_interval)) {
						u_printf("start capture udec_id %u cycle with interval %u MS.\n", dec_id, cap_interval);
						do_playback_capture_cycle(iav_fd, dec_id, cap_interval);
						u_printf("stop capture udec_id %u cycle.\n", dec_id);
					} else if (1 == sscanf(p_buffer, "cycjpeg:%u", &dec_id)) {
						u_printf("start capture udec_id %u cycle with interval 1000 MS.\n", dec_id);
						do_playback_capture_cycle(iav_fd, dec_id, 1000);
						u_printf("stop capture udec_id %u cycle.\n", dec_id);
					}
				} else if ('x' == p_buffer[1]) {
					//exchange display
					int exchange = 0;
					if ('w' == p_buffer[2]) {
						if (1 == sscanf(p_buffer, "cxw:%d", &exchange)) {
							u_printf("[update display %%d]: exchange window %d, current layer %d, layer 0: %d, layer 1: %d\n", exchange, current_display_layer, foreground_stream_cnt, background_stream_cnt);
							if (0 == current_display_layer) {
								__exchange_window_circularly(renders, (u8)foreground_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
							} else {
								__exchange_window_circularly(renders + foreground_stream_cnt, (u8)background_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
							}
						} else {
							exchange = 1;
							u_printf("[update display]: exchange window, current layer %d, layer 0: %d, layer 1: %d\n", current_display_layer, foreground_stream_cnt, background_stream_cnt);
							if (0 == current_display_layer) {
								__exchange_window_circularly(renders, (u8)foreground_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
							} else {
								__exchange_window_circularly(renders + foreground_stream_cnt, (u8)background_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
							}
						}
					} else if ('u' == p_buffer[2]) {
						if (1 == sscanf(p_buffer, "cxu:%d", &exchange)) {
							u_printf("[update display %%d]: exchange udec %d, current layer %d, layer 0: %d, layer 1: %d\n", exchange, current_display_layer, foreground_stream_cnt, background_stream_cnt);
							if (0 == current_display_layer) {
								__exchange_udec_circularly(renders, (u8)foreground_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
							} else {
								__exchange_udec_circularly(renders + foreground_stream_cnt, (u8)background_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
							}
						} else {
							exchange = 1;
							u_printf("[update display]: exchange udec, current layer %d, layer 0: %d, layer 1: %d\n", current_display_layer, foreground_stream_cnt, background_stream_cnt);
							if (0 == current_display_layer) {
								__exchange_udec_circularly(renders, (u8)foreground_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, foreground_stream_cnt, foreground_stream_cnt, renders);
							} else {
								__exchange_udec_circularly(renders + foreground_stream_cnt, (u8)background_stream_cnt, exchange);
								ioctl_render_cmd(iav_fd, background_stream_cnt, background_stream_cnt, renders + foreground_stream_cnt);
							}
						}
					}
				} else if ('f' == p_buffer[1]) {
					//fb related
					int param1 = 0;
					if ('b' == p_buffer[2]) {
						if (!strncmp(p_buffer, "cfb0clear", 9)) {
							__op_framebuffer_1("/dev/fb0", EOPFB_Clear, NULL, 0, -1, -1, 0, 0, NULL);
						} else if  (!strncmp(p_buffer, "cfb1clear", 9)) {
							__op_framebuffer_1("/dev/fb1", EOPFB_Clear, NULL, 0, -1, -1, 0, 0, NULL);
						} else if (1 == sscanf(p_buffer, "cfb0set:%x", &param1)) {
							__op_framebuffer_1("/dev/fb0", EOPFB_Set, NULL, param1, -1, -1, 0, 0, NULL);
						} else if (1 == sscanf(p_buffer, "cfb1set:%x", &param1)) {
							__op_framebuffer_1("/dev/fb1", EOPFB_Set, NULL, param1, -1, -1, 0, 0, NULL);
						} else if (!strncmp(p_buffer, "cfb08clear", 10)) {
							__op_framebuffer_0("/dev/fb0", EOPFB_Clear, NULL, 0, 0, 0, 0);
						} else if  (!strncmp(p_buffer, "cfb18clear", 10)) {
							__op_framebuffer_0("/dev/fb1", EOPFB_Clear, NULL, 0, 0, 0, 0);
						} else if (1 == sscanf(p_buffer, "cfb08set:%x", &param1)) {
							__op_framebuffer_0("/dev/fb0", EOPFB_Set, NULL, param1, 0, 0, 0);
						} else if (1 == sscanf(p_buffer, "cfb18set:%x", &param1)) {
							__op_framebuffer_0("/dev/fb1", EOPFB_Set, NULL, param1, 0, 0, 0);
						} else if (!strncmp(p_buffer, "cfb0clut", 8)) {
							__op_update_framebuffer_clut(-1, "/dev/fb0");
						} else if (!strncmp(p_buffer, "cfb1clut", 8)) {
							__op_update_framebuffer_clut(-1, "/dev/fb1");
						} else {
							//to do
						}
					}
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
	u_printf_debug("[flow]: before enter idle\n");
	ioctl_enter_idle(iav_fd);
	u_printf_debug("[flow]: enter idle done\n");

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
	user_press_quit = 1;
	return -1;
}

static int do_test_udec(int iav_fd)
{
	unsigned int i;
	pthread_t thread_id[MAX_NUM_UDEC_ORI];
	pthread_t renderer_thread_id[MAX_NUM_UDEC_ORI];
	udec_instance_param_t params[MAX_NUM_UDEC_ORI];
	iav_udec_vout_config_t vout_cfg[NUM_VOUT];

	int vout_start_index = -1;
	int num_of_vout = 0;
	msg_t msg;

	char buffer_old[128] = {0};
	char buffer[128];
	char* p_buffer = buffer;

	int total_num = MAX_NUM_UDEC_ORI;//hard code here

	void* pv;
	int ret = 0;

	signal(SIGINT, udec_sig_stop);
	signal(SIGQUIT, udec_sig_stop);
	signal(SIGTERM, udec_sig_stop);

	dec_types = 0x17;	// no RV40, no hybrid MPEG4
	ppmode = 2;

	//enter mdec mode config
	iav_udec_mode_config_t udec_mode_content;
	iav_udec_mode_config_t *udec_mode = &udec_mode_content;
	iav_udec_config_t *udec_configs;

	memset(&udec_mode_content, 0x0, sizeof(udec_mode_content));

	u_printf_debug("[flow]: before malloc some memory, current_file_index %d, total_num %d\n", current_file_index, total_num);

	if ((udec_configs = malloc(total_num * sizeof(iav_udec_config_t))) == NULL) {
		u_printf_error(" no memory\n");
		goto do_test_udec_exit;
	}

	u_printf_debug("[flow]: before get vout info\n");
	//get all vout info
	for (i = 0; i < NUM_VOUT; i++) {
		if (get_single_vout_info(i, vout_width + i, vout_height + i, iav_fd) < 0) {
			u_printf_debug("[vout info query fail]: vout(%d) fail\n", i);
			vout_width[i] = 0;
			vout_height[i] = 0;
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
		u_printf_debug("[vout info query]: vout(%d), width %d, height %d, rotate %d.\n", i, *(vout_width + i), *(vout_height + i), vout_rotate[i]);
	}
	u_printf_debug("[vout info query]: max_vout_width %d, max_vout_height %d, vout_start_index %d, num_of_vout %d.\n", max_vout_width, max_vout_height, vout_start_index, num_of_vout);

	for (i = vout_start_index; i < (num_of_vout + vout_start_index); i++) {
		vout_cfg[i].disable = 0;
		vout_cfg[i].udec_id = 0;//hard code, fix me
		vout_cfg[i].flip = 0;
		vout_cfg[i].rotate = 0;
		vout_cfg[i].vout_id = i;
		if ((!no_upscaling) || ((vout_width[i] <= file_video_width[0]) || (vout_height[i] <= file_video_height[0]))) {
			u_printf_debug("do full screen vout_width[i] %d, vout_height[i] %d, file_video_width[0] %d, file_video_height[0] %d\n", vout_width[i], vout_height[i], file_video_width[0], file_video_height[0]);
			vout_cfg[i].win_offset_x = vout_cfg[i].target_win_offset_x = 0;
			vout_cfg[i].win_offset_y = vout_cfg[i].target_win_offset_y = 0;
			vout_cfg[i].win_width = vout_cfg[i].target_win_width = vout_width[i];
			vout_cfg[i].win_height = vout_cfg[i].target_win_height = vout_height[i];
			vout_cfg[i].zoom_factor_x = vout_cfg[i].zoom_factor_y = 1;
		} else {
			u_printf_debug("do not-upscaling vout_width[i] %d, vout_height[i] %d, file_video_width[0] %d, file_video_height[0] %d\n", vout_width[i], vout_height[i], file_video_width[0], file_video_height[0]);
			vout_cfg[i].win_offset_x = vout_cfg[i].target_win_offset_x = (vout_width[i] - file_video_width[0]) / 2;
			vout_cfg[i].win_offset_y = vout_cfg[i].target_win_offset_y = (vout_height[i] - file_video_height[0]) / 2;

			vout_cfg[i].win_width = vout_cfg[i].target_win_width = file_video_width[0];
			vout_cfg[i].win_height = vout_cfg[i].target_win_height = file_video_height[0];
			u_printf_debug("do not-upscaling vout_cfg[i].win_width %d, vout_cfg[i].win_height  %d, vout_cfg[i].win_offset_x %d, vout_cfg[i].win_offset_y %d\n", vout_cfg[i].win_width, vout_cfg[i].win_height, vout_cfg[i].win_offset_x, vout_cfg[i].win_offset_y);
			vout_cfg[i].zoom_factor_x = vout_cfg[i].zoom_factor_y = 0;
		}
	}

	//process request vout mask
	if (1 == mudec_request_voutmask) {
		//LCD's case
		if (!vout_width[0] || !vout_height[0]) {
			u_printf_error("[vout info error]: LCD device is not avaiable!!!\n");
			goto do_test_udec_exit;
		}
		vout_start_index = 0;
		num_of_vout = 1;
		mudec_actual_voutmask = 1;
		mudec_actual_vout_start_index = 0;
	} else if (2 == mudec_request_voutmask) {
		//HDMI's case
		if (!vout_width[1] || !vout_height[1]) {
			u_printf_error("[vout info error]: HDMI device is not avaiable!!!\n");
			goto do_test_udec_exit;
		}
		vout_start_index = 1;
		num_of_vout = 1;
		mudec_actual_voutmask = 2;
		mudec_actual_vout_start_index = 1;
	} else if (3 == mudec_request_voutmask) {
		//LCD + HDMI's case
		if (!vout_width[0] || !vout_height[0]) {
			u_printf_error("[vout info error]: LCD device is not avaiable!!!\n");
			goto do_test_udec_exit;
		}
		if (!vout_width[1] || !vout_height[1]) {
			u_printf_error("[vout info error]: HDMI device is not avaiable!!!\n");
			goto do_test_udec_exit;
		}
		vout_start_index = 0;
		num_of_vout = 2;
		mudec_actual_voutmask = 3;
		mudec_actual_vout_start_index = 0;
	} else {
		u_printf_error("BAD vout mask 0x%x\n", mudec_request_voutmask);
		goto do_test_udec_exit;
	}

	u_printf_debug("[flow]: before init_udec_mode_config\n");
	init_udec_mode_config(udec_mode);
	udec_mode->num_udecs = total_num;
	udec_mode->udec_config = udec_configs;

	//udec info
	for (i = 0; i < total_num; i ++) {
		init_udec_configs(udec_configs + i, 1, file_video_width[i], file_video_height[i]);
	}

	u_printf_debug("[flow]: before IAV_IOC_ENTER_UDEC_MODE\n");
	if (ioctl(iav_fd, IAV_IOC_ENTER_UDEC_MODE, udec_mode) < 0) {
		perror("IAV_IOC_ENTER_UDEC_MODE");
		u_printf_error(" enter udec mode fail\n");
		goto do_test_udec_exit;
	}

	u_printf_debug("[flow]: enter udec mode done, total_file_number %d, shift_file_offset %d\n", total_file_number, shift_file_offset);

	if (shift_file_offset) {
		int tmp_file_index = 0;
		//total_file_number = current_file_index + 1;

		u_assert(total_file_number);

		i = 0;
		//each instance's parameters
		for (i = 0; i < total_num; i++) {

			tmp_file_index = (i + shift_file_offset);
			if (tmp_file_index >= total_file_number) {
				tmp_file_index -= total_file_number;
			}

			u_assert(tmp_file_index >= 0);
			u_assert(tmp_file_index < total_file_number);

			if (!i) {
				u_printf("[playback url]: choose file %d, %s\n", tmp_file_index, file_list[tmp_file_index]);
			}

			params[i].udec_index = i;
			params[i].iav_fd = iav_fd;
			params[i].loop = 1;
			params[i].request_bsb_size = bits_fifo_size;
			params[i].udec_type = file_codec[tmp_file_index];
			params[i].pic_width = file_video_width[tmp_file_index];
			params[i].pic_height= file_video_height[tmp_file_index];
			msg_queue_init(&params[i].cmd_queue);
			msg_queue_init(&params[i].renderer_cmd_queue);

			params[i].wait_cmd_begin = 0;

			params[i].wait_cmd_exit = 1;

			params[i].num_vout = num_of_vout;
			params[i].p_vout_config = &vout_cfg[mudec_actual_vout_start_index];
			params[i].file_fd[0] = fopen(file_list[tmp_file_index], "rb");
			params[i].file_fd[1] = NULL;
		}
	} else {
		i = 0;
		//each instance's parameters
		for (i = 0; i < total_num; i++) {
			params[i].udec_index = i;
			params[i].iav_fd = iav_fd;
			params[i].loop = 1;
			params[i].request_bsb_size = bits_fifo_size;
			params[i].udec_type = file_codec[i];
			params[i].pic_width = file_video_width[i];
			params[i].pic_height= file_video_height[i];
			msg_queue_init(&params[i].cmd_queue);
			msg_queue_init(&params[i].renderer_cmd_queue);

			if (!i) {
				u_printf("[playback url]: choose file %d, %s\n", i, file_list[i]);
			}

			params[i].wait_cmd_begin = 0;

			params[i].wait_cmd_exit = 1;

			params[i].num_vout = num_of_vout;
			params[i].p_vout_config = &vout_cfg[mudec_actual_vout_start_index];
			params[i].file_fd[0] = fopen(file_list[i], "rb");
			params[i].file_fd[1] = NULL;
		}
	}

	current_vout_start_index = mudec_actual_vout_start_index;

	//each instance's USEQ, UPES header
	for (i = 0; i < total_num; i++) {

		if (!specify_fps) {
			params[i].cur_feeding_pts = 0;

			//hard code to 29.97 fps
			params[i].frame_tick = 3003;
			params[i].time_scale = 90000;

			params[i].frame_duration = ((u64)90000)*((u64)params[i].frame_tick)/params[i].time_scale;
			u_assert(3003 == params[i].frame_duration);//debug assertion
		} else {
			params[i].cur_feeding_pts = 0;

			params[i].frame_tick = 90000/specify_fps;
			params[i].time_scale = 90000;

			params[i].frame_duration = ((u64)90000)*((u64)params[i].frame_tick)/params[i].time_scale;
			add_useq_upes_header = 1;
		}
		u_printf_debug("[fps]: params[i].frame_tick %d, params[i].time_scale %d, params[i].frame_duration %d, specify_fps %d.\n", params[i].frame_tick, params[i].time_scale, params[i].frame_duration, specify_fps);

		//hard code to h264
		//init USEQ/UPES header
		params[i].useq_header_len = fill_useq_header(params[i].useq_buffer, UDEC_H264, params[i].time_scale, params[i].frame_tick, 0, 0, 0);
		u_assert(UDEC_SEQ_HEADER_LENGTH == params[i].useq_header_len);

		init_upes_header(params[i].upes_buffer, UDEC_H264);

		params[i].seq_header_sent = 0;
		params[i].last_pts_from_dsp_valid = 0;
		params[i].paused = 0;
		params[i].trickplay_mode = UDEC_TRICKPLAY_RESUME;

		params[i].current_playback_strategy = PB_STRATEGY_ALL_FRAME;
		params[i].tobe_playback_strategy = PB_STRATEGY_ALL_FRAME;
	}


	//spawn all decoding threads
	for (i = 0; i < total_num; i++) {
		pthread_create(&thread_id[i], NULL, udec_instance, &params[i]);
	}

	//spawn all renderer threads
	for (i = 0; i < total_num; i++) {
		pthread_create(&renderer_thread_id[i], NULL, udec_renderer_ppmode2, &params[i]);
	}

	memset(buffer, 0x0, sizeof(buffer));
	memset(buffer_old, 0x0, sizeof(buffer_old));

	unsigned int input_udec_id = 0;
	unsigned int input_flag;
	bmp_t bmp;
	memset(&bmp, 0, sizeof(bmp_t));

	if (1) {
		int flag_stdin = 0;
		flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
		if(fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
			u_printf("[error]: stdin_fileno set error.\n");
		}
		//char cmd_char = 'x';
		while (udec_running) {
			//cmd_char = 'x';
			//ret = scanf("%c", &cmd_char);
			//if (!ret || ('x' == cmd_char)) {
			//	usleep(100000);
			//	continue;
			//}
			usleep(100000);
			memset(buffer, 0x0, sizeof(buffer));
			if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
				continue;

			//u_printf_debug("[debug 1]: %s\n", buffer);

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
			//u_printf_debug("[debug 2]: %s\n", p_buffer);
			if ('n' == p_buffer[0]) {
				u_printf("Next, 'n'\n");
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = e_playlist_next;
			} else if ('p' == p_buffer[0]) {
				u_printf("Previous, 'p'\n");
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = e_playlist_previous;
			} else if ('c' == p_buffer[0]) {
				u_printf("Current, 'c'\n");
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = e_playlist_current;
			} else if ('s' == p_buffer[0]) {
				u_printf("Pause/Resume, 's'\n");
				udec_pause_resume(params);
			} else if ('q' == p_buffer[0]) {
				u_printf("Quit, 'q'\n");
				udec_running = 0;
				user_press_quit = 1;
			} else {
				int file_number = 0;
				if (1 == sscanf(p_buffer, "%d", &file_number)) {
					if ((file_number > 0) && (file_number < 32)) {
						u_printf("jump to file %d\n", file_number);
						udec_running = 0;
						user_press_quit = 0;
						playlist_next_or_previous = file_number;
					} else {
						u_printf_error("file number %d exceed 32\n", file_number);
					}
				} else {
					//u_printf_debug("[debug 3]: not processed %s\n", p_buffer);
				}
			}
#if 0
			else if ('1' == cmd_char) {
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = 0;
			} else if ('2' == cmd_char) {
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = 1;
			} else if ('3' == cmd_char) {
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = 2;
			} else if ('4' == cmd_char) {
				udec_running = 0;
				user_press_quit = 0;
				playlist_next_or_previous = 3;
			} else {
				//u_printf("why comes here? %c.\n", cmd_char);
			}
#endif
		}

		if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
			u_printf("[error]: stdin_fileno set error");
		}

	} else if (0) {

		struct	keypadctrl *keypad = (struct keypadctrl *) &_G_keypadctrl;
		struct input_event key_input;

		//struct input_event event;
		fd_set	rfds;
		int	rval= -1;
		int	i = 0;
		char	str[64];
		//u32	tmp_time;
		for (i = 0; i < key_event_channel; i++) {
			sprintf(str, "/dev/input/event%d", i);
			keypad->fd[i] = open(str, O_NONBLOCK);
			if (keypad->fd[i] < 0) {
				perror("open");
				u_printf_error("open %s error\n", str);
				continue;
				//goto close_keyboard;
			}
		}

		FD_ZERO(&rfds);
		for (i = 0; i < key_event_channel; i++) {
			if (keypad->fd[i] > 0) {
				FD_SET(keypad->fd[i], &rfds);
			}
		}

		while (udec_running) {

			rval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
			if (rval < 0) {
				u_printf_error("select fail\n");
				goto do_test_udec_exit;
			}

			for (i = 0; i < key_event_channel; i++) {
				if (FD_ISSET(keypad->fd[i], &rfds)) {
					rval = read(keypad->fd[i], &key_input, sizeof(key_input));
					if (rval < 0) {
						u_printf_error("read keypad->fd[%d] error",i);
						ret = -1;
						goto do_test_udec_exit;
					}

					if (key_input.type != EV_KEY) {
						continue;
					}

					for (i = 0; i < KEY_ARRAY_SIZE; i++) {
						if (key_input.code == _AMBARELLA_INPUT_KEY_INFO[i].input_key) {
							if (KEY_N == key_input.code) {
								u_printf("Next, 'KEY_N'\n");
								udec_running = 0;
								user_press_quit = 0;
								playlist_next_or_previous = e_playlist_next;
							} else if (KEY_P == key_input.code) {
								u_printf("Previous, 'KEY_P'\n");
								udec_running = 0;
								user_press_quit = 0;
								playlist_next_or_previous = e_playlist_previous;
							} else if (KEY_C == key_input.code) {
								u_printf("Current, 'KEY_C'\n");
								udec_running = 0;
								user_press_quit = 0;
								playlist_next_or_previous = e_playlist_current;
							} else if (KEY_S == key_input.code) {
								u_printf("Pause/Resume, 'KEY_S'\n");
							} else if (KEY_Q == key_input.code) {
								u_printf("Quit, 'KEY_Q'\n");
								udec_running = 0;
								user_press_quit = 1;
							} else if (KEY_1 == key_input.code) {

							} else if (KEY_2 == key_input.code) {

							} else if (KEY_3 == key_input.code) {

							} else if (KEY_4 == key_input.code) {

							} else {
								u_printf_error("why comes here?\n");
							}
						}
					}

					if (i == KEY_ARRAY_SIZE) {
						u_printf_error(" can't find the key map for key_input.code %d\n", key_input.code);
						continue;
					}

				}
			}
		}

//close_keyboard:

		for (i = 0; i < key_event_channel; i++) {
			if (keypad->fd[i] > 0) {
				close(keypad->fd[i]);
				keypad->fd[i] = -1;
			}
		}

		//user_press_quit = 1;

	} else {
		int flag_stdin = 0;
		flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
		if(fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
			u_printf("[error]: stdin_fileno set error.\n");
		}
		//main cmd loop
		while (udec_running) {
			//add sleep to avoid affecting the performance
			usleep(100000);
			memset(buffer, 0x0, sizeof(buffer));
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
					udec_running = 0;
					user_press_quit = 1;
					break;

				case 'n':	// next
					u_printf("Next\n");
					udec_running = 0;
					user_press_quit = 0;
					playlist_next_or_previous = e_playlist_next;
					break;

				case 'h':	// help
					u_printf("Print help\n");
					_print_udec_ut_cmds();
					break;

				case 's':	//step
					if (1 != sscanf(p_buffer, "s%d", &input_udec_id)) {
						u_printf("BAD cmd format, you should type 's%%d' and enter, to do step mode for the udec(%%d).\n");
						break;
					}

					u_assert(input_udec_id >= 0);
					u_assert(input_udec_id < total_num);
					if ((input_udec_id < 0) || (input_udec_id >= total_num)) {
						u_printf_error("BAD input params, udec_id %d, out of range.\n", input_udec_id);
						break;
					}

					udec_step(params + input_udec_id);
					break;

				case ' ':
					if (1 != sscanf(p_buffer, " %d", &input_udec_id)) {
						u_printf("BAD cmd format, you should type ' %%d' and enter, to do pause/resume the udec(%%d).\n");
						break;
					}

					u_assert(input_udec_id >= 0);
					u_assert(input_udec_id < total_num);
					if ((input_udec_id < 0) || (input_udec_id >= total_num)) {
						u_printf_error("BAD input params, udec_id %d, out of range.\n", input_udec_id);
						break;
					}

					udec_pause_resume(params + input_udec_id);
					break;

				case 'z':
					if ('0' == p_buffer[1]) {
						//stop(0)
						input_flag = 0x0;
					} else if ('1' == p_buffer[1]) {
						//stop(1)
						input_flag = 0x1;
					} else if ('f' == p_buffer[1]) {
						//stop(0xff)
						input_flag = 0xff;
					} else {
						u_printf("not support cmd %s.\n", p_buffer);
						u_printf("   type 'z0\%d' to send stop(0) to udec(\%d).\n");
						u_printf("   type 'z1\%d' to send stop(1) to udec(\%d).\n");
						u_printf("   type 'zf\%d' to send stop(0xff) to udec(\%d).\n");
						break;
					}

					if (1 != sscanf(p_buffer + 2, "%d", &input_udec_id)) {
						u_printf("BAD cmd format, you should type 'z%c\%d' and enter, to send stop(%x) for udec(\%d).\n", p_buffer[1], input_flag);
						break;
					}
					ioctl_udec_stop(iav_fd, input_udec_id, input_flag);
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
					if ('f' == p_buffer[1]) {
						//fb related
						int param1 = 0;
						if ('b' == p_buffer[2]) {
							if (!strncmp(p_buffer, "cfb0clear", 9)) {
								__op_framebuffer_1("/dev/fb0", EOPFB_Clear, NULL, 0, -1, -1, 0, 0, NULL);
							} else if  (!strncmp(p_buffer, "cfb1clear", 9)) {
								__op_framebuffer_1("/dev/fb1", EOPFB_Clear, NULL, 0, -1, -1, 0, 0, NULL);
							} else if (1 == sscanf(p_buffer, "cfb0set:%x", &param1)) {
								__op_framebuffer_1("/dev/fb0", EOPFB_Set, NULL, param1, -1, -1, 0, 0, NULL);
							} else if (1 == sscanf(p_buffer, "cfb1set:%x", &param1)) {
								__op_framebuffer_1("/dev/fb1", EOPFB_Set, NULL, param1, -1, -1, 0, 0, NULL);
							} else if (!strncmp(p_buffer, "cfb08clear", 10)) {
								__op_framebuffer_0("/dev/fb0", EOPFB_Clear, NULL, 0, 0, 0, 0);
							} else if  (!strncmp(p_buffer, "cfb18clear", 10)) {
								__op_framebuffer_0("/dev/fb1", EOPFB_Clear, NULL, 0, 0, 0, 0);
							} else if (1 == sscanf(p_buffer, "cfb08set:%x", &param1)) {
								__op_framebuffer_0("/dev/fb0", EOPFB_Set, NULL, param1, 0, 0, 0);
							} else if (1 == sscanf(p_buffer, "cfb18set:%x", &param1)) {
								__op_framebuffer_0("/dev/fb1", EOPFB_Set, NULL, param1, 0, 0, 0);
							} else if (!strncmp(p_buffer, "cfb0clut", 8)) {
								__op_update_framebuffer_clut(-1, "/dev/fb0");
							} else if (!strncmp(p_buffer, "cfb1clut", 8)) {
								__op_update_framebuffer_clut(-1, "/dev/fb1");
							} else if  (!strncmp(p_buffer, "cfb0rect", 8)) {
								__op_framebuffer_1("/dev/fb0", EOPFB_Rect, NULL, 0, -1, -1, 0, 0, NULL);
							} else if  (!strncmp(p_buffer, "cfb1rect", 8)) {
								__op_framebuffer_1("/dev/fb1", EOPFB_Rect, NULL, 0, -1, -1, 0, 0, NULL);
							} else {
								//to do
							}
						}
					} else if ('b' == p_buffer[1]) {
						//bmp file
						if (!strncmp(p_buffer, "cbmp0:", 6)) {
							set_end_of_string(p_buffer, '\n', 126);
							u_printf("try load bmp file to fb 0: %s\n", p_buffer + 6);
							if (0 == (ret = open_bmp_file(p_buffer + 6, &bmp))) {
								__op_framebuffer_1("/dev/fb0", EOPFB_DisplayBitmap, NULL, 0, -1, -1, 0, 0, &bmp);
							} else {
								u_printf("[error]: open bmf file(%s) fail, ret %d\n", p_buffer + 6, ret);
							}
						} else if (!strncmp(p_buffer, "cbmp1:", 6)) {
							set_end_of_string(p_buffer, '\n', 126);
							u_printf("try load bmp file to fb 1: %s\n", p_buffer + 6);
							if (0 == (ret = open_bmp_file(p_buffer + 6, &bmp))) {
								__op_framebuffer_1("/dev/fb1", EOPFB_DisplayBitmap, NULL, 0, -1, -1, 0, 0, &bmp);
							} else {
								u_printf("[error]: open bmf file(%s) fail, ret %d\n", p_buffer + 6, ret);
							}
						}
					} else if ('n' == p_buffer[1]) {
						//next
						if (!strncmp(p_buffer, "cnext", 5)) {
							u_printf("cnext, play next file\n");
							udec_running = 0;
							user_press_quit = 0;
							playlist_next_or_previous = e_playlist_next;
						}
					} else if ('p' == p_buffer[1]) {
						if (!strncmp(p_buffer, "cpre", 4)) {
							u_printf("cpre, play previous file\n");
							udec_running = 0;
							user_press_quit = 0;
							playlist_next_or_previous = e_playlist_previous;
						}
					}
					break;

				default:
					break;
			}

		}

		if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
			u_printf("[error]: stdin_fileno set error");
		}
	}

	u_printf_debug("[flow]: break cmd loop\n");

	//exit each threads
	msg.cmd = M_MSG_KILL;
	msg.ptr = NULL;
	for (i = 0; i < total_num; i++) {
		params[i].wait_cmd_exit = 0;
		msg_queue_put(&params[i].cmd_queue, &msg);
		msg_queue_put(&params[i].renderer_cmd_queue, &msg);

		//stop if needed
		u_printf_debug("[flow]: send stop cmd to stop current playback, start\n");
		ioctl_udec_stop(iav_fd, i, 1);
		u_printf_debug("[flow]: send stop cmd to stop current playback, end\n");
	}

	for (i = 0; i < total_num; i++) {
		u_printf_debug("[flow]: wait udec_instance_thread(%d) exit...\n", i);
		ret = pthread_join(thread_id[i], &pv);
		u_printf_debug("[flow]: wait udec_instance_thread(%d) exit done(ret %d).\n", i, ret);
	}

	for (i = 0; i < total_num; i++) {
		u_printf_debug("[flow]: wait renderer_thread(%d) exit...\n", i);
		ret = pthread_join(renderer_thread_id[i], &pv);
		u_printf_debug("[flow]: wait renderer_thread(%d) exit done(ret %d).\n", i, ret);
	}

	release_bmp_file(&bmp);

	//exit UDEC mode
	u_printf_debug("[flow]: before enter idle\n");
	ioctl_enter_idle(iav_fd);
	u_printf_debug("[flow]: enter idle done\n");

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

	return ret;

do_test_udec_exit:
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
	if (udec_configs) {
		free(udec_configs);
	}
	user_press_quit = 1;
	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int iav_fd = -1;

	u_printf("[flow]: mudec start...\n");

	//init some variables
	memset(&display_window, 0x0, sizeof(display_window));
	memset(&display_layer_number, 0x0, sizeof(display_layer_number));

	if ((ret = init_mdec_params(argc, argv)) < 0) {
		u_printf_error("mudec: params invalid.\n");
		return -2;
	} else if (ret > 0) {
		return 1;
	}

	if (scan_dir) {
		u_printf_debug("[flow]: before scandir(%s).\n", dirname);
		if (0) {
			mudec_scandir(dirname);
		} else {
			mudec_scandir_pre_sorted(dirname);
			mudec_sort_files(current_file_index + 1);
		}
		u_printf_debug("[flow]: after scandir().\n");
	}

	total_file_number = current_file_index + 1;
	u_assert(total_file_number > 0);

	//u_printf("shift_file_offset %d, total_file_number %d\n", shift_file_offset, total_file_number);

	u_printf_debug("[flow]: before open_iav().\n");

	// open the device
	if ((iav_fd = open_iav()) < 0) {
		u_printf_error("open_iav fail.\n");
		user_press_quit = 1;
		return -1;
	}

	if (use_udec_mode && rd_buffer_size) {
		prealloc_file_buffer = malloc(rd_buffer_size);
		u_assert(prealloc_file_buffer);
	}

replayfile:

	if (!use_udec_mode) {
		u_printf_debug("[flow]: before do_test_mdec(%d).\n", iav_fd);
		do_test_mdec(iav_fd);
	} else {
		u_printf_debug("[flow]: before do_test_udec(%d), total_file_number %d.\n", iav_fd, total_file_number);
		do_test_udec(iav_fd);
	}

	if (!user_press_quit) {

		if ((EPlayListMode_ExitUDECMode == playlist_mode) && (mdec_loop)) {
			if (e_playlist_next == playlist_next_or_previous) {
				shift_file_offset = (shift_file_offset + 1) % (total_file_number);
			} else if (e_playlist_previous == playlist_next_or_previous) {
				shift_file_offset = (shift_file_offset + total_file_number - 1) % total_file_number;
			} else if (e_playlist_current == playlist_next_or_previous) {
			} else {
				shift_file_offset = (playlist_next_or_previous + total_file_number) % total_file_number;
				playlist_next_or_previous = e_playlist_next;
			}
			if (shift_file_offset >= total_file_number) {
				u_printf_error("shift_file_offset %d, total_file_number %d\n", shift_file_offset, total_file_number);
				shift_file_offset = 0;
			}
			u_printf_debug("shift_file_offset %d, total_file_number %d\n", shift_file_offset, total_file_number);

			udec_running = 1;
			goto replayfile;
		} else {
#if 0
			if (e_playlist_next == playlist_next_or_previous) {
				shift_file_offset = (shift_file_offset + 1) % total_file_number;
			} else if (e_playlist_previous == playlist_next_or_previous) {
				shift_file_offset = (shift_file_offset + total_file_number - 1) % total_file_number;
			} else if (e_playlist_current == playlist_next_or_previous) {
			} else {
				shift_file_offset = (playlist_next_or_previous + total_file_number) % total_file_number;
			}
			if (shift_file_offset >= total_file_number) {
				u_printf_error("shift_file_offset %d, total_file_number %d\n", shift_file_offset, total_file_number);
				shift_file_offset = 0;
			}
			u_printf_debug("shift_file_offset %d, total_file_number %d\n", shift_file_offset, total_file_number);

			udec_running = 1;
			goto replayfile;
#endif
		}
	}

	u_printf_debug("[flow]: close(iav_fd %d).\n", iav_fd);
	// close the device
	close(iav_fd);

	if (prealloc_file_buffer) {
		free(prealloc_file_buffer);
		prealloc_file_buffer = NULL;
	}

	u_printf("[flow]: mudec exit...\n");
	return 0;
}


