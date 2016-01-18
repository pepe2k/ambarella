
/*
 * test_stream.c
 * the program can read bitstream from BSB buffer and dump to different files
 * in different recording session, if there is no stream being recorded, then this
 * program will be in idle loop waiting and do not exit until explicitly closed.
 *
 * this program may stop encoding if the encoding was started to be "limited frames",
 * or stop all encoding when this program was forced to quit
 *
 * History:
 *	2009/12/31 - [Louis Sun] modify to use EX functions
 *	2012/01/11 - [Jian Tang] add different transfer method for streams
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

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

#include <signal.h>
#include <basetypes.h>
#include <pthread.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "datatx_lib.h"
#include "../encrypt/encrypt.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

//#define ENABLE_RT_SCHED

#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define BASE_PORT			(2000)
#define SVCT_PORT_OFFSET	(16)
#define MAX_SVCT_LAYERS	(4)
#define MAX_LENGTH			(256)

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


//debug options
#undef DEBUG_PRINT_FRAME_INFO

#define ACCURATE_FPS_CALC

#define GET_STREAMID(x)	((((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) ? -1: (x))

typedef struct transfer_method {
	trans_method_t method;
	int port;
	char filename[256];
} transfer_method;

#define	FILENAME_LENGTH		(32)
#define	MAX_YUV_FRAME_NUM	(120)
typedef struct efm_param {
	int width;
	int height;
	int size_flag;

	char yuv[FILENAME_LENGTH];
	int file_flag;

	int frame_num;
	int frame_num_flag;

	int frame_factor;
	u32 yuv_pts_distance;

	int source;
	int buf_id;

	int flag;
} efm_param;

// the device file handle
int fd_iav;

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;
struct encrypt_buff_s encrypt_buff;
#define ENCRYPT_BUFF_SIZE 4096

static int nofile_flag = 0;
static int frame_info_flag = 0;
static int show_pts_flag = 0;
static int check_pts_flag = 0;
static int file_size_flag = 0;
static int file_size_mega_byte = 100;
static int remove_time_string_flag = 0;
static int only_filename = 0;
static int split_svct_layer = 0;

static int fps_statistics_interval = 300;
static int print_interval = 30;

//create flags for testing
static int use_bs_info_queue_flag = 0;

//create files for writing
static int init_none, init_nfs, init_tcp, init_usb, init_stdout;
static transfer_method stream_transfer[MAX_ENCODE_STREAM_NUM];
static int default_transfer_method = TRANS_METHOD_NFS;
static int transfer_port = BASE_PORT;

// bitstream filename base
static char filename[256];
const char *default_filename;
const char *default_filename_nfs = "/mnt/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";

//total frames we need to capture
static int md5_idr_number = -1;

//verbose
static int verbose_mode = 0;

//print dot
static int print_dot_flag = 1;

//Encode from memory
typedef enum {
	EFM_SOURCE_FROM_BUF = 0,
	EFM_SOURCE_FROM_FILE = 1,
} EFM_SOURCE_TYPE;

static efm_param efm = {
	.frame_factor = 1,
};

static u32 efm_task_exit_flag = 0;
static pthread_t efm_task_id = 0;
static u32 user_mem_flag = 0;
u8 *user_mem_start_addr;
u8 *user_mem_ptr;
static int user_mem_size = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

enum numeric_short_options {
	TRANSFER_OPTIONS_BASE = 0,
	INFO_OPTIONS_BASE = 10,
	TEST_OPTIONS_BASE = 25,
	MISC_OPTIONS_BASE = 130,

	FILENAME = TRANSFER_OPTIONS_BASE,
	NOFILE_TRANSER,
	TCP_TRANSFER,
	USB_TRANSFER,
	STDOUT_TRANSFER,
	USER_TRANSFER,

	TOTAL_FRAMES = INFO_OPTIONS_BASE,
	TOTAL_SIZE,
	FILE_SIZE,
	SAVE_FRAME_INFO,
	SHOW_PTS_INFO,
	CHECK_PTS_INFO,

	DURATION_TEST = TEST_OPTIONS_BASE,
	REMOVE_TIME_STRING,
	USE_BS_INFO_QUEUE,
	MD5_TEST,
	ONLY_FILENAME,
	SPLIT_SVCT_LAYER,

	EFM_YUV_INPUT_FILE = MISC_OPTIONS_BASE,
	EFM_FRAME_SIZE,
	EFM_FRAME_NUM,
	EFM_FRAME_FACTOR,
	EFM_BUF_ID,
	EFM_SOURCE,

};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"tcp",		NO_ARG,		0,	't'},
	{"usb",		NO_ARG,		0,	'u'},
	{"stdout",	NO_ARG,		0,	'o'},
	{"nofile",		NO_ARG,		0,	NOFILE_TRANSER},
	{"frames",	HAS_ARG,	0,	TOTAL_FRAMES},
	{"size",		HAS_ARG,	0,	TOTAL_SIZE},
	{"file-size",	HAS_ARG,	0,	FILE_SIZE},
	{"frame-info",	NO_ARG,		0,	SAVE_FRAME_INFO},
	{"show-pts",	NO_ARG,		0,	SHOW_PTS_INFO},
	{"check-pts",	NO_ARG,		0,	CHECK_PTS_INFO},
	{"rm-time",	NO_ARG,		0,	REMOVE_TIME_STRING},
	{"only-filename",	NO_ARG,	0,	ONLY_FILENAME},
	{"split-svct",	NO_ARG,		0,	SPLIT_SVCT_LAYER},
	{"fps-intvl",	HAS_ARG,	0,	'i'},
	{"frame-intvl",	HAS_ARG,	0,	'n'},
	{"streamA",	NO_ARG,		0,	'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB",	NO_ARG,		0,	'B'},
	{"streamC",	NO_ARG,		0,	'C'},
	{"streamD",	NO_ARG,		0,	'D'},
	{"bs-info-queue",	NO_ARG,	0,	USE_BS_INFO_QUEUE},
	{"md5test",	HAS_ARG,	0,	MD5_TEST},
	{"verbose",	NO_ARG,		0,	'v'},
	{"encrypt",	NO_ARG,		0,	'e'},
	{"Key",		HAS_ARG,	0,	'K'},
	{"Key-length", HAS_ARG,	0,	'l'},
	{"disable-print-dot", NO_ARG,	0,	'd'},

	{"user-mem",	NO_ARG,		0,	USER_TRANSFER},
	{"efm-source",	HAS_ARG,	0,	EFM_SOURCE},
	{"efm-buf",	HAS_ARG,	0,	EFM_BUF_ID},
	{"efm-yuv-file",	HAS_ARG,	0,	EFM_YUV_INPUT_FILE},
	{"efm-fsize",	HAS_ARG,	0,	EFM_FRAME_SIZE},
	{"efm-fnum",	HAS_ARG,	0,	EFM_FRAME_NUM},
	{"efm-factor",	HAS_ARG,	0,	EFM_FRAME_FACTOR},
	{0, 0, 0, 0}
};

static const char *short_options = "ABCdDef:i:K:l:n:otuv";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename", "specify filename"},
	{"", "\t\tuse tcp (ps) to receive bitstream"},
	{"", "\t\tuse usb (us_pc2) to receive bitstream"},
	{"", "\t\treceive data from BSB buffer, and write to stdout"},
	{"", "\t\tjust receive data from BSB buffer, but do not write file"},
	{"frames", "\tspecify how many frames to encode"},
	{"size", "\tspecify how many bytes to encode"},
	{"size", "\tcreate new file to capture when size is over maximum (MB)"},
	{"", "\t\tgenerate frame info"},
	{"", "\t\tshow stream pts info"},
	{"", "\t\tcheck stream pts info"},
	{"", "\t\tremove time string from the file name"},
	{"", "\tonly use specifiy file name"},
	{"", "\t\tsplit SVCT layers into different streams"},
	{"", "\t\tset fps statistics interval"},
	{"", "\tset frame/field statistic interval"},
	{"", "\t\tstream A"},
	{"", "\t\tstream B"},
	{"", "\t\tstream C"},
	{"", "\t\tstream D"},
	{"", "\tUse BS info queue to fetch and release one frame"},
	{"", "\t\tmd5 checksum calculate"},
	{"", "\t\tprint more information"},
	{"encrypt", "\tencrypt the video stream\n\t\t\t\tThe ciphertext can be" \
		"decrypted by openssl.\n\t\t\t\tFor example:\n\t\t\t\t   openssl" \
		"enc -aes-128-ecb -d -nosalt -in ciphertest -out plaintext -K key"},
	{"Key", "\t\tkey in Hex is the next argument"},
	{"Key length", "key length,should be 128/192/256"},
	{"", "\tdisable print wait dot\n"},
	{"", "user space mem, it is used for customer"},
	{"0|1", "\tspecify source, 0 : from source buffer, 1: from yuv file"},
	{"0~3", "\tspecify yuv source buffer id (encode from yuv data directly)"},
	{"filename", "specify YUV input file for encoding from memory feature"},
	{"W x H", "\tspecify frame size for encoding from memory feature"},
	{"1~120", "\tspecify the total frame number from YUV file"},
	{"divisor", "specify frame factor for feed yuv buffer.(feed fps = vin fps / divisor)"},
};

void usage(void)
{
	int i;
	printf("test_stream usage:\n");
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

typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	u32 total_frames;	//count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;	//count how many bytes encoded
	int pic_type;		//picture type,  non changed during encoding, help to validate encoding state.
	u32 pts;
	u64 monotonic_pts;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start
	struct encrypt_s encrypt;
#ifdef ACCURATE_FPS_CALC
	int 	 total_frames2;	//for statistics only
	struct timeval time2;	//for statistics only
#endif

	int fd_svct[MAX_SVCT_LAYERS];	//file descriptor for svct streams
	int svct_layers;	//count how many svct layers
	u32 dsp_audio_pts;	// dsp pts from audio I2S
} stream_encoding_state_t;

static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static stream_encoding_state_t old_encoding_states[MAX_ENCODE_STREAM_NUM];  //old states for statistics only

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

int config_efm_params(void)
{
#define MAX_YUV_SOURCE_BUF_WIDTH	(3840)
#define MAX_YUV_SOURCE_BUF_HEIGHT	(2160)

	u32 	fps, fps_q9;
	iav_source_buffer_setup_ex_t	buffer_setup;
	iav_reso_ex_t *yuv_buf_size = NULL;

	if (efm.source == EFM_SOURCE_FROM_FILE) {
		if (efm.file_flag == 0 || efm.size_flag == 0) {
			printf("error: must specify the yuv file and size\n");
			return -1;
		}
	} else {
		if (efm.file_flag == 1 || efm.size_flag == 1) {
			printf("error: enc from source buf, no need specify the file and size\n");
			return -1;
		}
	}
	memset(&buffer_setup, 0, sizeof(buffer_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}

	if (buffer_setup.type[IAV_ENCODE_SOURCE_DRAM_FIRST] !=
		IAV_SOURCE_BUFFER_TYPE_ENCODE) {
		printf("yuv buf[%d] error, the buffer is not encode type\n",
			IAV_ENCODE_SOURCE_DRAM_FIRST);
		return -1;
	}

	yuv_buf_size = &buffer_setup.size[IAV_ENCODE_SOURCE_DRAM_FIRST];

	if (efm.source == EFM_SOURCE_FROM_FILE) {
		if ((yuv_buf_size->width < efm.width) || (yuv_buf_size->height < efm.height)) {
			printf("error:YUV buf[%d] size is smaller than the setting,"
				 "YUV buf:width[%d], height[%d], setting:width[%d], height[%d]\n",
				IAV_ENCODE_SOURCE_DRAM_FIRST,
				yuv_buf_size->width, yuv_buf_size->height, efm.width, efm.height);
			return -1;
		}
	} else {
		if (buffer_setup.type[efm.buf_id] != IAV_SOURCE_BUFFER_TYPE_ENCODE) {
			printf("efm-buf[%d] error, the buffer is not encode type\n", efm.buf_id);
			return -1;
		}

		if ((yuv_buf_size->width < buffer_setup.size[efm.buf_id].width)
			|| (yuv_buf_size->height < buffer_setup.size[efm.buf_id].height)) {
			printf("error:YUV buf[%d] size is smaller than the setting,"
				"YUV buf:width[%d], height[%d], Source buf[%d]:width[%d], height[%d]\n",
				IAV_ENCODE_SOURCE_DRAM_FIRST,
				yuv_buf_size->width, yuv_buf_size->height, efm.buf_id,
				buffer_setup.size[efm.buf_id].width, buffer_setup.size[efm.buf_id].height);
			return -1;
		}

		efm.width = buffer_setup.size[efm.buf_id].width;
		efm.height = buffer_setup.size[efm.buf_id].height;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &fps_q9) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}

	fps = DIV_ROUND(512000000, fps_q9);
	if (fps < 1) {
		printf("error:vin fps[%d] < 1\n ", fps);
		return -1;
	}

	if (efm.frame_factor > fps || (efm.frame_factor < 1)) {
		printf("error:the factor must be in 1~%d\n ", fps);
		return -1;
	}

	// both 90000 and 512000000 divide 10000.
	efm.yuv_pts_distance = (u64)fps_q9 * 9 / 51200 * efm.frame_factor;

	return 0;
}

int config_transfer(void)
{
	int i;
	transfer_method *trans;
	char str[][16] = { "NONE", "NFS", "TCP", "USB", "STDOUT"};

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		trans = &stream_transfer[i];
		trans->port = transfer_port + i * 2;
		if (strlen(trans->filename) > 0) {
			if (trans->method == TRANS_METHOD_UNDEFINED)
				trans->method = TRANS_METHOD_NFS;
		} else {
			if (trans->method == TRANS_METHOD_UNDEFINED) {
				trans->method = default_transfer_method;
			}
			switch (trans->method) {
			case TRANS_METHOD_NFS:
			case TRANS_METHOD_STDOUT:
				if (strlen(filename) == 0)
					default_filename = default_filename_nfs;
				else
					default_filename = filename;
				break;
			case TRANS_METHOD_TCP:
			case TRANS_METHOD_USB:
				if (strlen(filename) == 0)
					default_filename = default_filename_tcp;
				else
					default_filename = filename;
				break;
			default:
				default_filename = NULL;
				break;
			}
			if (default_filename != NULL)
				strcpy(trans->filename, default_filename);
		}
		printf("Stream %c %s: %s\n", 'A' + i, str[trans->method], trans->filename);
	}

	return 0;
}

static int init_param(int argc, char **argv)
{
	int i, ch;
	int width, height;
	int option_index = 0;
	int current_stream = -1;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		stream_transfer[i].method = TRANS_METHOD_UNDEFINED;
	}

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			break;
		case 'B':
			current_stream = 1;
			break;
		case 'C':
			current_stream = 2;
			break;
		case 'D':
			current_stream = 3;
			break;
		case 'f':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream < 0) {
				strcpy(filename, optarg);
			} else {
				strcpy(stream_transfer[current_stream].filename, optarg);
			}
			break;
		case 't':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_TCP;
			}
			default_transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_USB;
			}
			default_transfer_method = TRANS_METHOD_USB;
			break;
		case 'o':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_STDOUT;
			}
			default_transfer_method = TRANS_METHOD_STDOUT;
			break;
		case 'i':
			fps_statistics_interval = atoi(optarg);
			break;
		case 'n':
			print_interval = atoi(optarg);
			break;
		case NOFILE_TRANSER:
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_NONE;
			}
			default_transfer_method = TRANS_METHOD_NONE;
			break;
		case TOTAL_FRAMES:
			break;
		case TOTAL_SIZE:
			break;
		case FILE_SIZE:
			file_size_flag = 1;
			file_size_mega_byte = atoi(optarg);
			break;
		case SAVE_FRAME_INFO:
			frame_info_flag = 1;
			break;
		case SHOW_PTS_INFO:
			show_pts_flag = 1;
			break;
		case CHECK_PTS_INFO:
			check_pts_flag = 1;
			break;
		case REMOVE_TIME_STRING:
			remove_time_string_flag = 1;
			break;
		case ONLY_FILENAME:
			only_filename = 1;
			break;
		case SPLIT_SVCT_LAYER:
			split_svct_layer = 1;
			break;
		case USE_BS_INFO_QUEUE:
			use_bs_info_queue_flag = 1;
			break;
		case MD5_TEST:
			md5_idr_number = atoi(optarg);
			printf("md5_idr_number %d \n", md5_idr_number);
			break;

		case 'v':
			verbose_mode = 1;
			break;
		case 'e':
			current_stream = GET_STREAMID(current_stream);
			encoding_states[current_stream].encrypt.enable = 1;
			encoding_states[current_stream].encrypt.op = ALG_OP_ENCRYPT;
			break;
		case 'K':
			current_stream = GET_STREAMID(current_stream);
			strcpy((char *)encoding_states[current_stream].encrypt.key,key_atoh(optarg));
			break;
		case 'l':
			current_stream = GET_STREAMID(current_stream);
			encoding_states[current_stream].encrypt.key_len = atoi(optarg)/8;
			break;
		case 'd':
			print_dot_flag = 0;
			break;

		case EFM_YUV_INPUT_FILE:
			if (strlen(optarg) >= FILENAME_LENGTH) {
				printf("File name [%s] is too long. It cannot be longer than "
					"%d characters.\n", optarg, (FILENAME_LENGTH - 1));
				return -1;
			}
			strcpy(efm.yuv, optarg);
			printf("YUV input file [%s].\n", efm.yuv);
			efm.file_flag = 1;
			break;
		case EFM_FRAME_SIZE:
			if (get_arbitrary_resolution(optarg, &width, &height) < 0) {
				printf("Failed to get resolution from [%s].\n", optarg);
			}
			efm.width = width;
			efm.height = height;
			efm.size_flag = 1;
			break;
		case EFM_FRAME_NUM:
			i = atoi(optarg);
			if (i < 1 || i > MAX_YUV_FRAME_NUM) {
				printf("Total frame number [%d] must be in the range of [1~120].\n", i);
				return -1;
			}
			efm.frame_num = i;
			efm.frame_num_flag = 1;
			break;
		case EFM_BUF_ID:
			i = atoi(optarg);
			if ((i < IAV_ENCODE_SOURCE_BUFFER_FIRST) ||
				(i >= IAV_ENCODE_SUB_SOURCE_BUFFER_LAST)) {
				printf("Invalid yuv source buffer id : %d.\n", i);
				return -1;
			}
			efm.buf_id = i;
			break;
		case EFM_SOURCE:
			i = atoi(optarg);
			if (i > 1 || i < 0) {
				printf("The value must be 0 or 1\n");
				return -1;
			}
			efm.source = i;
			efm.flag = 1;
			break;
		case EFM_FRAME_FACTOR:
			i = atoi(optarg);
			if (i < 1) {
				printf("The value must be > 0\n");
				return -1;
			}
			efm.frame_factor = i;
			break;
		case USER_TRANSFER:
			user_mem_flag = 1;
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	if (efm.flag) {
		if (config_efm_params() < 0) {
			printf("config_efm_params error\n");
			return -1;
		}
	}

	if (config_transfer() < 0) {
		printf("config_transfer error\n");
		return -1;
	}

	return 0;
}

static int init_encoding_states(void)
{
	int i, j;
	//init all file hander and session id to invalid at start
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].fd = -1;
		encoding_states[i].fd_info = -1;
		encoding_states[i].session_id = -1;
		encoding_states[i].total_bytes = 0;
		encoding_states[i].total_frames = 0;
		encoding_states[i].pic_type = 0;
		encoding_states[i].pts = 0;
		encoding_states[i].svct_layers = 0;
		for (j = 0; j < MAX_SVCT_LAYERS; ++j) {
			encoding_states[i].fd_svct[j] = -1;
		}
	}
	return 0;
}

//return 0 if it's not new session,  return 1 if it is new session
static int is_new_session(bits_info_ex_t * bits_info)
{
	int stream_id = bits_info->stream_id;
	int new_session = 0 ;
	if  (bits_info ->session_id != encoding_states[stream_id].session_id) {
		//a new session
		new_session = 1;
	}
	if (file_size_flag) {
		if ((encoding_states[stream_id].total_bytes / 1024) > (file_size_mega_byte * 1024))
			new_session = 1;
	}

	return new_session;
}

#include <time.h>

static int get_time_string( char * time_str,  int len)
{
	time_t  t;
	struct tm * tmp;

	t= time(NULL);
	tmp = gmtime(&t);
	if (strftime(time_str, len, "%m%d%H%M%S", tmp) == 0) {
		printf("date string format error \n");
		return -1;
	}

	return 0;
}

#define VERSION	0x00000005
#define PTS_IN_ONE_SECOND		(90000)
static int write_frame_info_header(int stream_id)
{
	iav_h264_config_ex_t config;
	int version = VERSION;
	u32 size = sizeof(config);
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	config.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	if (amba_transfer_write(fd_info, &version, sizeof(int), method) < 0 ||
		amba_transfer_write(fd_info, &size, sizeof(u32), method) < 0 ||
		amba_transfer_write(fd_info, &config, sizeof(config), method) < 0) {
		perror("write_data(4)");
		return -1;
	}

	return 0;
}

static int write_frame_info(bits_info_ex_t * bits_info)
{
	typedef struct video_frame_s {
		u32     size;
		u32     pts;
		u32     pic_type;
		u32     reserved;
	} video_frame_t;
	video_frame_t frame_info;
	frame_info.pic_type = bits_info->pic_type;
	frame_info.pts = (u32)bits_info->stream_pts;
	frame_info.size = bits_info->pic_size;
	int stream_id = bits_info->stream_id;
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	if (amba_transfer_write(fd_info, &frame_info, sizeof(frame_info), method) < 0) {
		perror("write(5)");
		return -1;
	}
	return 0;
}

static int calc_svct_layers(int stream_id)
{
	iav_h264_config_ex_t h264;

	memset(&h264, 0, sizeof(h264));
	h264.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &h264);

	if (h264.gop_model > 1) {
		encoding_states[stream_id].svct_layers = h264.gop_model;
	}

	return 0;
}

//check session and update file handle for write when needed
static int check_session_file_handle(bits_info_ex_t * bits_info, int new_session)
{
	char write_file_name[MAX_LENGTH], filename[MAX_LENGTH];
	char time_str[MAX_LENGTH];
	char file_type[8];
	int i, is_h264;
	int stream_id = bits_info->stream_id;
	int method = stream_transfer[stream_id].method;
	int port = stream_transfer[stream_id].port;
	int encrypt_enable = encoding_states[stream_id].encrypt.enable;

   	if (new_session) {
		is_h264 = (bits_info->pic_type != JPEG_STREAM);
		sprintf(file_type, "%s", is_h264 ? "h264" : "mjpeg");

		//close old session if needed
		if (encoding_states[stream_id].fd > 0) {
			close(encoding_states[stream_id].fd);
			encoding_states[stream_id].fd = -1;
		}
		if (encrypt_enable) {
			if (encoding_states[stream_id].encrypt.fd_cipher > 0){
				close(encoding_states[stream_id].encrypt.fd_cipher);
				encoding_states[stream_id].encrypt.fd_cipher = -1;
			}
		}
		if (split_svct_layer) {
			for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
				if (encoding_states[stream_id].fd_svct[i] > 0) {
					close(encoding_states[stream_id].fd_svct[i]);
					encoding_states[stream_id].fd_svct[i] = -1;
				}
			}
			encoding_states[stream_id].svct_layers = 0;
		}

		if (remove_time_string_flag) {
			memset(time_str, 0, sizeof(time_str));
		} else {
			get_time_string(time_str, sizeof(time_str));
		}

		if (only_filename) {
			sprintf(write_file_name, "%s", stream_transfer[stream_id].filename);
		} else {
			sprintf(write_file_name, "%s_%c_%s_%x",
				stream_transfer[stream_id].filename, 'A' + stream_id,
				time_str, bits_info->session_id);
		}

		sprintf(filename, "%s.%s", write_file_name, file_type);
		if ((encoding_states[stream_id].fd =
				amba_transfer_open(filename, method, port)) < 0) {
			printf("create file for write failed %s \n", filename);
			return -1;
		} else {
			if (!nofile_flag) {
				printf("\nnew session file name [%s], fd [%d] \n", filename,
					encoding_states[stream_id].fd);
			}
		}

		if (encrypt_enable) {
			sprintf(filename, "%s.cipher.%s", write_file_name, file_type);
			// encrypted file use the different port from plain file.
			if ((encoding_states[stream_id].encrypt.fd_cipher =
				amba_transfer_open(filename, method, port + 1)) < 0) {
				printf("create encrypted file for write  failed %s.\n", filename);
				return -1;
			}
		}

		if (split_svct_layer && is_h264) {
			calc_svct_layers(stream_id);
			if (encoding_states[stream_id].svct_layers > 1) {
				for (i = 0; i < encoding_states[stream_id].svct_layers; ++i) {
					sprintf(filename, "%s.svct_%d.%s", write_file_name, i, file_type);
					encoding_states[stream_id].fd_svct[i] = amba_transfer_open(
						filename, method, (port + i + SVCT_PORT_OFFSET));
					if (encoding_states[stream_id].fd_svct[i] < 0) {
						printf("create file for write SVCT layers failed %s.\n", filename);
						return -1;
					}
				}
			}
		}

		if (frame_info_flag) {
			sprintf(filename, "%s.info", write_file_name);
			if ((encoding_states[stream_id].fd_info =
				amba_transfer_open(filename, method, port)) < 0) {
				printf("create file for frame info  failed %s \n", filename);
				return -1;
			}
			if (write_frame_info_header(stream_id) < 0) {
				printf("write h264 header info failed %s \n", filename);
				return -1;
			}
		}
	}

	return 0;
}

static int update_session_data(bits_info_ex_t * bits_info, int new_session)
{
	int stream_id = bits_info->stream_id;
	//update pic type, session id on new session
	if (new_session) {
		encoding_states[stream_id].pic_type = bits_info->pic_type;
		encoding_states[stream_id].session_id = bits_info->session_id;
		encoding_states[stream_id].total_bytes = 0;
		encoding_states[stream_id].total_frames = 0;
		old_encoding_states[stream_id] = encoding_states[stream_id];	//for statistics data only

#ifdef ACCURATE_FPS_CALC
		old_encoding_states[stream_id].total_frames2 = 0;
		old_encoding_states[stream_id].time2 = old_encoding_states[stream_id].capture_start_time;	//reset old counter
#endif
	}

	//update statistics on all frame
	encoding_states[stream_id].total_bytes += bits_info->pic_size;
	encoding_states[stream_id].total_frames++;
	encoding_states[stream_id].pts = (u32)bits_info->stream_pts;

	return 0;
}

static int write_encrypted_video_padding(bits_info_ex_t * bits_info)
{
	struct encrypt_s *enc_point = &encoding_states[bits_info->stream_id].encrypt;
	int i;
	for(i=0;i<(16-enc_point->buf_offset);i++){
		enc_point->enc_buf[enc_point->buf_offset + i] = (16-enc_point->buf_offset);
	}

	enc_point->padding = 1;
	oneshot_ecb_aes(enc_point->enc_buf,16,&encrypt_buff,enc_point);
	if(amba_transfer_write(enc_point->fd_cipher,
			(void *)encrypt_buff.cipher_addr,encrypt_buff.cipher_len,
			stream_transfer[bits_info->stream_id].method)<0){
		perror("Failed to write encrypt.bin file\n");
	}
	enc_point->enc_align=0;
	enc_point->buf_offset=0;
	for(i=0;i<32;i++)
		enc_point->key[i] = 0;
	enc_point->key_len=0;
	enc_point->op=0;
	enc_point->padding=0;

	return 0;
}

static int write_encrypted_file(int transfer_method, unsigned char *in, int in_len,
	struct encrypt_buff_s *encrypt_buff, struct encrypt_s *enc_point)
{
	int *align = &enc_point->enc_align;
	int fd_cipher = enc_point->fd_cipher;
	int ret=0;
	int finished=0;
	int len=0;

	if(*align != 0){
		oneshot_ecb_aes(in,in_len,encrypt_buff,enc_point);
		if(amba_transfer_write(fd_cipher, (void *)encrypt_buff->cipher_addr,
				encrypt_buff->cipher_len, transfer_method)<0){
			perror("Failed to write encrypt.bin file\n");
			ret = -1;
		}
		in_len = in_len - (16-*align);
		in = in + (16-*align);
	}
	do{
		len = (encrypt_buff->buff_size	> (in_len-finished)) ?
			(in_len-finished) : encrypt_buff->buff_size;
		*align = oneshot_ecb_aes(in+finished,len, encrypt_buff,enc_point);
		if(amba_transfer_write(fd_cipher, (void *)encrypt_buff->cipher_addr,
				encrypt_buff->cipher_len, transfer_method)<0){
			perror("Failed to write encrypt.bin file\n");
			ret = -1;
		}
		finished += len;
	} while (finished < in_len);

	return ret;

}

static int write_svct_file(int method, unsigned char *in, int len, int fd)
{
	if (amba_transfer_write(fd, in, len, method) < 0) {
		perror("Failed to write stream into SVCT file.\n");
		return -1;
	}
	return 0;
}

static int find_svct_layer(unsigned char *in, int in_len)
{
	const int header_magic_num = 0x00000001;
	unsigned int header_mn = 0;
	unsigned char nalu, layer = -1;
	int i = 0;

	do {
		header_mn = (in[i] << 24 | in[i+1] << 16 | in[i+2] << 8 | in[i+3]);
		if (header_mn == header_magic_num) {
			nalu = in[i+4] & 0x1F;
			if ((nalu == NT_IDR) || (nalu == NT_NON_IDR)) {
				layer = (in[i+4] >> 5) & 0x3;
				break;
			}
		}
	} while (++i < in_len);
	return layer;
}

static int write_svct_files(int transfer_method, int stream_id,
	unsigned char *in, int in_len)
{
	int layer = find_svct_layer(in, in_len);
	int rval = 0;

	switch (encoding_states[stream_id].svct_layers) {
	case 4:
		switch (layer) {
		case 3:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[3]);
			/* Fall through to write this frame into other layers */
		case 2:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[2]);
			/* Fall through to write this frame into other layers */
		case 1:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 0:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	case 3:
		switch (layer) {
		case 3:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[2]);
			/* Fall through to write this frame into other layers */
		case 2:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 0:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	case 2:
		switch (layer) {
		case 3:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 0:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	default:
		rval = -1;
		printf("Invalid SVCT layers. Cannot be larger than 4.\n");
		break;
	}

	if ((rval >= 0) && verbose_mode) {
		printf("Save SVCT layer [%d] into file.\n", layer);
	}

	return rval;
}

static int write_video_file(bits_info_ex_t * bits_info)
{
	static unsigned int whole_pic_size=0;
	u32 pic_size = bits_info->pic_size;
	int fd = encoding_states[bits_info->stream_id].fd;
	int stream_id = bits_info->stream_id;
	struct encrypt_s *enc_point = &encoding_states[bits_info->stream_id].encrypt;
	int encrypt_enable = enc_point->enable;
	//remove align
	whole_pic_size  += (pic_size & (~(1<<23)));

	if (pic_size>>23) {
		//end of frame
		pic_size = pic_size & (~(1<<23));
	 	 //padding some data to make whole picture to be 32 byte aligned
		pic_size += (((whole_pic_size + 31) & ~31)- whole_pic_size);
		//rewind whole pic size counter
		// printf("whole %d, pad %d \n", whole_pic_size, (((whole_pic_size + 31) & ~31)- whole_pic_size));
		 whole_pic_size = 0;
	}

	//if md5 check only write idr frame data
	if(md5_idr_number > 0){
		if(bits_info->frame_num > 90){ // do not count first three idr frame
			if (amba_transfer_write(fd, (void *)bits_info->start_addr, pic_size,
				stream_transfer[stream_id].method) < 0) {
				perror("Failed to write md5 idr streams into file!\n");
				return -1;
			}
			md5_idr_number = md5_idr_number -1;
		}
		return 0;
	}

	if (amba_transfer_write(fd, (void *)bits_info->start_addr, pic_size,
		stream_transfer[stream_id].method) < 0) {
		perror("Failed to write streams into file!\n");
		return -1;
	}
	if (encrypt_enable) {
		write_encrypted_file(stream_transfer[stream_id].method,
			(unsigned char *)bits_info->start_addr, pic_size, &encrypt_buff, enc_point);
	}
	if (split_svct_layer && (encoding_states[stream_id].svct_layers > 1)) {
		if (write_svct_files(stream_transfer[stream_id].method, stream_id,
			(unsigned char *)bits_info->start_addr, pic_size) < 0) {
			perror("Failed to split and write SVCT layers into files!\n");
			return -1;
		}
	}
	return 0;
}

static u8 * yuv_addr = NULL;
static u8 * yuv_end = NULL;
static u8 * frame_start = NULL;

#define test_me1		0

static int prepare_efm_files(void)
{
	int yuv_fd = -1;
	u32 frame_size, total_size;

	if (efm.size_flag == 0) {
		printf("Need frame resolution for EFM!\n");
		return -1;
	}
	if (!efm.frame_num_flag) {
		efm.frame_num = 1;
	}
	frame_size = efm.width * efm.height;
	total_size = frame_size * 3 / 2 * efm.frame_num;

	// read YV12 planar format
	yuv_addr = (u8*)malloc(total_size);
	if (!yuv_addr) {
		printf("Failed to allocate buffers.");
		goto ERROR_EXIT;
	}
	if ((yuv_fd = open(efm.yuv, O_RDONLY)) < 0) {
		printf("Failed to open YUV file [%s].\n", efm.yuv);
		goto ERROR_EXIT;
	}
	read(yuv_fd, yuv_addr, total_size);
	if (yuv_fd >= 0) {
		close(yuv_fd);
		yuv_fd = -1;
	}
	frame_start = yuv_addr;
	yuv_end = yuv_addr + total_size;

	return 0;

ERROR_EXIT:
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	if (yuv_fd >= 0) {
		close(yuv_fd);
		yuv_fd = -1;
	}
	return -1;
}

#if test_me1
static int write_me1_file(u8 * data, u16 w, u16 h)
{
	int me1_test_fd = -1;
	char name[32];
	u8 * uv_80 = NULL;
	u32 size = w * h;
	static int init_flag = 0;

	if (init_flag == 2) {
		return 0;
	}

	sprintf(name, "/tmp/me1_%dx%d.yuv", w, h);

	if ((me1_test_fd = open(name, O_CREAT | O_RDWR)) < 0) {
		printf("Failed to open file [%s].\n", name);
		return -1;
	}
	write(me1_test_fd, data, size);

	size >>= 1;
	if ((uv_80 = (u8*)malloc(size)) == NULL) {
		printf("Failed to malloc UV data for ME1 buffer!\n");
		close(me1_test_fd);
		me1_test_fd = -1;
		return -1;
	}
	memset(uv_80, 0x80, size);
	write(me1_test_fd, uv_80, size);

	free(uv_80);
	close(me1_test_fd);
	me1_test_fd = -1;

	init_flag++;
	printf("Save the ME1 data into file [%s].\n", name);

	return 0;
}
#endif

static int map_user_mem()
{
	iav_mmap_info_t user_info;
	if (ioctl(fd_iav, IAV_IOC_MAP_USER, &user_info) < 0) {
		printf("map user mem failed \n");
		return -1;
	}
	user_mem_start_addr = user_info.addr;
	user_mem_size = user_info.length;

	return 0;
}
int gdma_cpoy(iav_gdma_copy_ex_t *pGdma)
{
	//copy to new position
	if (ioctl(fd_iav, IAV_IOC_GDMA_COPY_EX, pGdma) < 0) {
		printf("iav gdma copy failed \n");
		return -1;
	}
	return 0;
}

static int feed_buf_from_srcbuf(iav_enc_dram_buf_frame_ex_t *buf)
{
	iav_yuv_cap_t *yuv;
	iav_me1_cap_t *me1;
	iav_gdma_copy_ex_t gdma_param;
	iav_buf_cap_t cap;
	iav_system_setup_info_ex_t info;
	int mask_row,mask_col,mask_width,mask_height;

	if (buf == NULL) {
		perror("The point is null\n");
		return -1;
	}
	memset(&cap, 0, sizeof(cap));
	cap.flag = 1;
	if (ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap)) {
		if (errno == EINTR) {
			return -1;	/* back to for() */
		} else {
			perror("IAV_IOC_READ_BUF_CAP_EX");
			return -1;
		}
	}
	yuv = &cap.yuv[efm.buf_id];
	me1 = &cap.me1[efm.buf_id];

	if ((yuv->y_addr == NULL) || (yuv->uv_addr == NULL)) {
		printf("YUV buffer [%d] address is NULL! Skip to next!\n", efm.buf_id);
		return -1;;
	}
	if (me1->addr == NULL) {
		printf("ME1 buffer [%d] address is NULL! Skip to next!\n", efm.buf_id);
		return -1;;
	}

	// DSP memory is used for "non-cahced" GDMA copy
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &info) < 0) {
		perror("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
		return -1;
	}
	if(user_mem_flag) {
		user_mem_ptr = user_mem_start_addr;
		gdma_param.src_non_cached = info.dsp_noncached;
		gdma_param.dst_non_cached = info.dsp_noncached;

		gdma_param.usr_src_addr = (u32)(yuv->y_addr);
		gdma_param.usr_dst_addr = (u32)user_mem_ptr;
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_USER;

		gdma_param.src_pitch = yuv->pitch;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		mask_width = mask_height = 100;
		mask_width = mask_width;
		mask_height = mask_height;
		for(mask_row = 0; mask_row < mask_width; mask_row++) {
			for(mask_col = 0; mask_col < mask_height; mask_col++) {
				*(user_mem_ptr + mask_row*(yuv->width) + mask_col) = 0;
			}
		}

		user_mem_ptr += yuv->width * yuv->height;
		gdma_param.usr_src_addr = (u32)(yuv->uv_addr);
		gdma_param.usr_dst_addr = (u32)(user_mem_ptr);
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_USER;

		gdma_param.src_pitch = yuv->pitch;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height >> 1;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		user_mem_ptr += yuv->width * (yuv->height >> 1);
		gdma_param.usr_src_addr = (u32)(me1->addr);
		gdma_param.usr_dst_addr = (u32)(user_mem_ptr);
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_USER;

		gdma_param.src_pitch = me1->pitch;
		gdma_param.dst_pitch = me1->width;
		gdma_param.width = me1->width;
		gdma_param.height = me1->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		user_mem_ptr = user_mem_start_addr;
		gdma_param.src_non_cached = info.dsp_noncached;
		gdma_param.dst_non_cached = info.dsp_noncached;

		gdma_param.usr_src_addr = (u32)(user_mem_ptr);
		gdma_param.usr_dst_addr = (u32)(buf->y_addr);
		gdma_param.src_mmap_type = IAV_MMAP_USER;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = buf->max_size.width;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		user_mem_ptr += yuv->width * yuv->height;
		gdma_param.usr_src_addr = (u32)(user_mem_ptr);
		gdma_param.usr_dst_addr = (u32)(buf->uv_addr);
		gdma_param.src_mmap_type = IAV_MMAP_USER;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = buf->max_size.width;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height >> 1;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		user_mem_ptr += yuv->width * (yuv->height >> 1);
		gdma_param.usr_src_addr = (u32)(user_mem_ptr);
		gdma_param.usr_dst_addr = (u32)(buf->me1_addr);
		gdma_param.src_mmap_type = IAV_MMAP_USER;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = me1->width;
		gdma_param.dst_pitch = me1->width;
		gdma_param.width = me1->width;
		gdma_param.height = me1->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}
	}else {
		gdma_param.src_non_cached = info.dsp_noncached;
		gdma_param.dst_non_cached = info.dsp_noncached;

		gdma_param.usr_src_addr = (u32)(yuv->y_addr);
		gdma_param.usr_dst_addr = (u32)(buf->y_addr);
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = yuv->pitch;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		gdma_param.usr_src_addr = (u32)(yuv->uv_addr);
		gdma_param.usr_dst_addr = (u32)(buf->uv_addr);
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = yuv->pitch;
		gdma_param.dst_pitch = buf->max_size.width;
		gdma_param.width = yuv->width;
		gdma_param.height = yuv->height >> 1;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

		gdma_param.usr_src_addr = (u32)(me1->addr);
		gdma_param.usr_dst_addr = (u32)(buf->me1_addr);
		gdma_param.src_mmap_type = IAV_MMAP_DSP;
		gdma_param.dst_mmap_type = IAV_MMAP_DSP;

		gdma_param.src_pitch = me1->pitch;
		gdma_param.dst_pitch = me1->width;
		gdma_param.width = me1->width;
		gdma_param.height = me1->height;

		if (gdma_cpoy(&gdma_param) < 0) {
			printf("gdma_cpoy error\n");
			return -1;
		}

	}
	return 0;
}

static int feed_buf_from_file(iav_enc_dram_buf_frame_ex_t *buf)
{
	#define ROW_MAX	(4)
	#define COL_MAX		(4)
	u16 yuv_pitch;
	u16 i, j, row, col;
	u32 frame_size, me1_data;
	u8 * src = NULL, * dst = NULL;

	// Feed YUV data
	frame_size = efm.width * efm.height * 3 / 2;
	yuv_pitch = buf->max_size.width;
	for (i = 0; i < (efm.height / ROW_MAX); ++i) {
		// Read Y data
		src = frame_start + i * ROW_MAX * efm.width;
		dst = buf->y_addr + i * ROW_MAX * yuv_pitch;
		for (row = 0; row < ROW_MAX; ++row, src += efm.width, dst += yuv_pitch) {
			memcpy(dst, src, efm.width);
		}

		// Read UV data
		src = frame_start + (efm.height + i * ROW_MAX / 2) * efm.width;
		dst = buf->uv_addr + i * ROW_MAX / 2 * yuv_pitch;
		for (row = 0; row < ROW_MAX / 2; ++row, src += efm.width, dst += yuv_pitch) {
			memcpy(dst, src, efm.width);
		}

		// Read ME1 data
		src = frame_start + i * ROW_MAX * efm.width;
		dst = buf->me1_addr + i * yuv_pitch;
		for (col = 0; col < (efm.width / COL_MAX); ++col) {
			for (row = 0, j = col * COL_MAX, me1_data = 0;
					row < efm.width * ROW_MAX; row += efm.width) {
				me1_data += (src[row + j] + src[row + j + 1] +
					src[row + j + 2] + src[row + j + 3]);
			}
			dst[col] = me1_data >> 4;
		}
	}
#if test_me1
		write_me1_file(buf->y_addr, yuv_pitch, efm.height);
		write_me1_file(buf->me1_addr, yuv_pitch, efm.height / ROW_MAX);
#endif

	// Update frame start address
	if (frame_start + frame_size < yuv_end) {
		frame_start += frame_size;
	} else {
		frame_start = yuv_addr;
	}

	return 0;
}

static int feed_efm_data(void)
{
	static u32 pts = 0;
	iav_enc_dram_buf_frame_ex_t buf_frame;
	iav_enc_dram_buf_update_ex_t buf_update;
	struct timeval curr_time, curr_time_end;

	memset(&buf_frame, 0, sizeof(buf_frame));
	buf_frame.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;

	if (verbose_mode) {
		gettimeofday(&curr_time, NULL);
	}

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX, &buf_frame);

	if (verbose_mode) {
		gettimeofday(&curr_time_end, NULL);
		printf("------time diff:%ld \n", curr_time_end.tv_usec - curr_time.tv_usec
			+ (curr_time_end.tv_sec - curr_time.tv_sec)* 1000000);
		curr_time = curr_time_end;
	}

	if (efm.source == EFM_SOURCE_FROM_FILE) {
		if (feed_buf_from_file(&buf_frame) < 0) {
			printf("feed_buf_from_file error\n");
			return -1;
		}
	} else {
		if (feed_buf_from_srcbuf(&buf_frame) < 0) {
			printf("feed_buf_from_srcbuf error\n");
			return -1;
		}
	}

	if (verbose_mode) {
		gettimeofday(&curr_time_end, NULL);
		printf("++++++time diff:%ld \n",  curr_time_end.tv_usec - curr_time.tv_usec
			+ (curr_time_end.tv_sec - curr_time.tv_sec)* 1000000);
	}

	// Update ME1 & YUV data
	memset(&buf_update, 0, sizeof(buf_update));
	buf_update.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
	buf_update.frame_id = buf_frame.frame_id;
	buf_update.frame_pts = pts;
	buf_update.size.width = efm.width;
	buf_update.size.height = efm.height;

	pts += efm.yuv_pts_distance;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX, &buf_update);

	return 0;
}

static int write_stream(int *total_frames, u64 *total_bytes)
{
	int new_session; //0:  old session  1: new session
	int print_frame = 1;
	u32 time_interval_us;
#ifdef ACCURATE_FPS_CALC
	u32 time_interval_us2;
#endif
	int stream_id;
	struct timeval pre_time, curr_time;
	int pre_frames ,curr_frames;
	u64 pre_bytes, curr_bytes;
	u32 pre_pts, curr_pts, curr_vin_fps;
	bits_info_ex_t  bits_info;
	char stream_name[128];
	static int init_flag = 0;
	static int end_of_stream[MAX_ENCODE_STREAM_NUM];
	iav_encode_stream_info_ex_t stream_info;
	int i, stream_end_num;
	int encrypt_enable;
	u32 dsp_audio_pts;
	int dsp_audio_diff;
	int mono_diff;
	int pts_diff_normal,frame_drop;
	iav_change_framerate_factor_ex_t frame_factor;

	if (init_flag == 0) {
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
			end_of_stream[i] = 1;
		}
		init_flag = 1;
	}

	for (i = 0, stream_end_num = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		if (stream_info.state == IAV_STREAM_STATE_ENCODING) {
			end_of_stream[i] = 0;
		}
		stream_end_num += end_of_stream[i];
	}

	// There is no encoding stream, skip to next turn
	if (stream_end_num == MAX_ENCODE_STREAM_NUM)
		return -1;

	if (use_bs_info_queue_flag) {
		bits_info.stream_id = MAX_ENCODE_STREAM_NUM;
		if (ioctl(fd_iav, IAV_IOC_FETCH_FRAME_EX, &bits_info) < 0) {
			if (errno != EAGAIN)
				perror("IAV_IOC_FETCH_FRAME_EX");
			return -1;
		}
	} else {
		if (ioctl(fd_iav, IAV_IOC_READ_BITSTREAM_EX, &bits_info) < 0) {
			if (errno != EAGAIN)
				perror("IAV_IOC_READ_BITSTREAM_EX");
			return -1;
		}
	}

	//update current frame encoding time
	stream_id = bits_info.stream_id;
	gettimeofday(&encoding_states[stream_id].capture_start_time, NULL);

	if (verbose_mode) {
		dsp_audio_pts = (bits_info.stream_pts >> 32);
		dsp_audio_diff = dsp_audio_pts - old_encoding_states[stream_id].dsp_audio_pts;
		mono_diff = bits_info.monotonic_pts - old_encoding_states[stream_id].monotonic_pts;
		printf("type=%d, frmNo=%d, PTS=%d, size=%d, addr=0x%x, strm_id=%d,"
			" sesn_id=%u, dsp_audio_pts=%u, dsp_audio_diff=%d, monotonic_pts=%llu,"
			" mono_diff=%d, resolution=%dx%d\n",
			bits_info.pic_type, bits_info.frame_num, bits_info.PTS, bits_info.pic_size,
			bits_info.start_addr, bits_info.stream_id, bits_info.session_id,
			dsp_audio_pts, dsp_audio_diff, bits_info.monotonic_pts, mono_diff,
			bits_info.pic_width, bits_info.pic_height);
		old_encoding_states[stream_id].monotonic_pts = bits_info.monotonic_pts;
		// high 32 bit from stream PTS is dsp i2s pts
		old_encoding_states[stream_id].dsp_audio_pts = bits_info.stream_pts >> 32;
	}

	//check if it's a stream end null frame indicator
	if (bits_info.stream_end) {
		end_of_stream[stream_id] = 1;
		//printf("close file of stream %d at end, session id %d \n", stream_id, bits_info.session_id);
		if (encoding_states[stream_id].fd > 0) {
			amba_transfer_close(encoding_states[stream_id].fd,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd = -1;
		}
		if (encoding_states[stream_id].fd_info > 0) {
			amba_transfer_close(encoding_states[stream_id].fd_info,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd_info = -1;
		}
		encrypt_enable = encoding_states[stream_id].encrypt.enable;
		if (encrypt_enable) {
			if (encoding_states[stream_id].encrypt.fd_cipher > 0) {
				write_encrypted_video_padding(&bits_info);
				amba_transfer_close(encoding_states[stream_id].encrypt.fd_cipher,
					stream_transfer[stream_id].method);
				encoding_states[stream_id].encrypt.fd_cipher = -1;
			}
		}
		if (split_svct_layer) {
			for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
				if (encoding_states[stream_id].fd_svct[i] > 0) {
					amba_transfer_close(encoding_states[stream_id].fd_svct[i],
						stream_transfer[stream_id].method);
					encoding_states[stream_id].fd_svct[i] = -1;
				}
			}
			if (encoding_states[stream_id].svct_layers > 0) {
				encoding_states[stream_id].svct_layers = 0;
			}
		}

		goto write_stream_exit;
	}

	//check if it's new record session, since file name and recording control are based on session,
	//session id and change are important data
	new_session = is_new_session(&bits_info);
	//update session data
	if (update_session_data(&bits_info, new_session) < 0) {
		printf("update session data failed \n");
		return -2;
	}

	//check and update session file handle
	if (check_session_file_handle(&bits_info, new_session) < 0) {
		printf("check session file handle failed \n");
		return -3;
	}

	if (frame_info_flag) {
		if (write_frame_info(&bits_info) < 0) {
			printf("write video frame info failed for stream %d, session id = %d.\n",
				stream_id, bits_info.session_id);
			return -5;
		}
	}

	//write file if file is still opened
	if (write_video_file(&bits_info) < 0) {
		printf("write video file failed for stream %d, session id = %d \n",
			stream_id, bits_info.session_id);
		return -4;
	}

	//update global statistics
	if (total_frames)
		*total_frames = (*total_frames) + 1;
	if (total_bytes)
		*total_bytes = (*total_bytes) + bits_info.pic_size;

	//print statistics
	pre_time = old_encoding_states[stream_id].capture_start_time;
	curr_time = encoding_states[stream_id].capture_start_time;
	pre_frames = old_encoding_states[stream_id].total_frames;
	curr_frames = encoding_states[stream_id].total_frames;
	pre_bytes = old_encoding_states[stream_id].total_bytes;
	curr_bytes = encoding_states[stream_id].total_bytes;
	pre_pts = old_encoding_states[stream_id].pts;
	curr_pts = encoding_states[stream_id].pts;
	if (show_pts_flag) {
		AM_IOCTL(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &curr_vin_fps);
		frame_factor.id = (1 << stream_id);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAMERATE_FACTOR_EX, &frame_factor);
		sprintf(stream_name, "stream %c", 'A' + stream_id);
		pts_diff_normal = 90000 / ((int)(512000000.0 / curr_vin_fps *
			frame_factor.ratio_numerator / frame_factor.ratio_denominator * 1000) / 1000.0);
		frame_drop = (curr_pts - pre_pts) / pts_diff_normal - 1;
		if (frame_drop > 0) {
			printf("%s:============ %d frames lost!  ===========\n", stream_name, frame_drop);
		}
		printf("%s: \tVIN: [%d], PTS: %d, diff: %d, frames NO: %d, size: %d, JPG Q: %d\n",
			stream_name, curr_vin_fps, curr_pts, (curr_pts - pre_pts),
			curr_frames, bits_info.pic_size, bits_info.jpeg_quality);
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
	}
#if 0		// to be done
	if (check_pts_flag) {
		iav_h264_config_ex_t config;
		int den, pts_delta, current_fps;
		u32 custom_encoder_frame_rate;
		config.id = (1 << stream_id);
		if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config) < 0) {
			perror("IAV_IOC_GET_H264_CONFIG_EX");
			return -1;
		}
		custom_encoder_frame_rate = config.custom_encoder_frame_rate;
		den = ((custom_encoder_frame_rate & (1 << 30)) >> 30) ? 1001 : 1000;
		current_fps = (custom_encoder_frame_rate & ~(3 << 30));
		pts_delta = PTS_IN_ONE_SECOND * den / current_fps;
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
		if ((curr_pts != 0) && (pre_pts != 0) && (curr_pts - pre_pts) % pts_delta != 0) {
			printf("Check PTS Error: VIN FPS = %4.2f, PTS delta = %d, Curr PTS = %d, Pre PTS = %d\n",
				(current_fps * 1.0 / den), pts_delta, curr_pts, pre_pts);
			exit(-1);
		}
	}
#endif
	if ((curr_frames % print_interval == 0) && (print_frame)) {
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time.tv_usec;

		sprintf(stream_name, "stream %c",  'A'+ stream_id);
		printf("%s:\t%4d %s, %2d fps, %18lld\tbytes, %5d kbps\n", stream_name,
			curr_frames, nofile_flag ? "discard" : "frames",
			DIV_ROUND((curr_frames - pre_frames) * 1000000, time_interval_us), curr_bytes,
			pre_time.tv_sec ? (int)((curr_bytes - pre_bytes) * 8000000LL /time_interval_us /1024) : 0);
		//backup time and states
		old_encoding_states[stream_id].session_id = encoding_states[stream_id].session_id;
		old_encoding_states[stream_id].fd = encoding_states[stream_id].fd;
		old_encoding_states[stream_id].total_frames = encoding_states[stream_id].total_frames;
		old_encoding_states[stream_id].total_bytes = encoding_states[stream_id].total_bytes;
		old_encoding_states[stream_id].pic_type = encoding_states[stream_id].pic_type;
		old_encoding_states[stream_id].capture_start_time = encoding_states[stream_id].capture_start_time;
	}
	#ifdef ACCURATE_FPS_CALC
	{
		const int fps_statistics_interval = 900;
		int pre_frames2;
		struct timeval pre_time2;
		pre_frames2 = old_encoding_states[stream_id].total_frames2;
		pre_time2 = old_encoding_states[stream_id].time2;
		if ((curr_frames % fps_statistics_interval ==0) &&(print_frame)) {
			time_interval_us2 = (curr_time.tv_sec - pre_time2.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time2.tv_usec;
			double fps = (curr_frames - pre_frames2)* 1000000.0/(double)time_interval_us2;
			BOLD_PRINT("AVG FPS = %4.2f\n",fps);
			old_encoding_states[stream_id].total_frames2 = encoding_states[stream_id].total_frames;
			old_encoding_states[stream_id].time2 = encoding_states[stream_id].capture_start_time;
		}
	}
	#endif

write_stream_exit:
	if (use_bs_info_queue_flag) {
		if (ioctl(fd_iav, IAV_IOC_RELEASE_FRAME_EX, &bits_info) < 0) {
			perror("IAV_IOC_RELEASE_FRAME_EX");
			return -1;
		}
	}

	return 0;
}

int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stderr, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else{
		fprintf(stderr, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "\r");
		dot_count = 0;
	}

	fflush(stderr);
	return 0;
}

void efm_loop_task(void *arg)
{
	char *vsync_proc = "/proc/ambarella/vin0_vsync";
	int vin_tick = -1;
	char vin_array[8];

	vin_tick = open(vsync_proc, O_RDONLY);
	if (vin_tick < 0) {
		printf("Cannot open [%s].\n", vsync_proc);
		return ;
	}

	if(user_mem_flag) {
		if(map_user_mem() < 0)
			return;
	}
	if (efm.source == EFM_SOURCE_FROM_FILE) {
		prepare_efm_files();
	}

	while (!efm_task_exit_flag) {
		read(vin_tick, vin_array, 8);
		feed_efm_data();
	}

	if (vin_tick >= 0) {
		close(vin_tick);
		vin_tick = -1;
	}

	return ;
}

static int create_efm_task(void)
{
	if (!efm.flag) {
		return 0;
	}

	efm_task_exit_flag = 0;
	if (efm_task_id == 0) {
		if (pthread_create(&efm_task_id, NULL, (void *)efm_loop_task,
			NULL) != 0) {
			printf("!! Fail to create thread <EFM task>\n");
		}
		printf("\n== Create thread <EFM task> successful ==\n");
	}

	printf("=== Setting EFM buf done ===\n");
	if (efm.source == EFM_SOURCE_FROM_FILE) {
		printf("  EFM source: file[%s], resolution[%dx%d], frame nums[%d]\n",
			efm.yuv, efm.width, efm.height, efm.frame_num);
	} else {
		printf("  EFM source: source buffer[%d], resolution[%dx%d]\n",
			efm.buf_id, efm.width, efm.height);
	}

	printf("  EFM buffer: factor[1/%d], pts[%d]\n", efm.frame_factor, efm.yuv_pts_distance);

	return 0;
}

static int destroy_efm_task(void)
{
	if (!efm.flag) {
		return 0;
	}
	efm_task_exit_flag = 1;
	if (efm_task_id != 0) {
		if (pthread_join(efm_task_id, NULL) != 0) {
			printf("!! Fail to destroy (join) thread <EFM task>\n");
		}
		printf("\n== Destroy (join) thread <EFM task> successful ==\n");
	}
	efm_task_exit_flag = 0;
	efm_task_id = 0;

	return 0;
}

int capture_encoded_video()
{
	int rval;
	//open file handles to write to
	int total_frames;
	u64 total_bytes;
	total_frames = 0;
	total_bytes =  0;

#ifdef ENABLE_RT_SCHED
	{
	    struct sched_param param;
	    param.sched_priority = 99;
	    if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
	        perror("sched_setscheduler");
	}
#endif

	create_efm_task();

	while (1) {
		if ((rval = write_stream(&total_frames, &total_bytes)) < 0) {
			if (rval == -1) {
				usleep(100 * 1000);
				if (print_dot_flag) {
					show_waiting();
				}
			} else {
				printf("write_stream err code %d \n", rval);
			}
			continue;
		}

		if(md5_idr_number == 0){
			md5_idr_number = -1;
			break;
		}
	}

	printf("stop encoded stream capture\n");

	printf("total_frames = %d\n", total_frames);
	printf("total_bytes = %lld\n", total_bytes);

	return 0;
}

static int init_transfer(void)
{
	int i, do_init, rtn = 0;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		do_init = 0;
		switch (stream_transfer[i].method) {
		case TRANS_METHOD_NONE:
			if (init_none == 0)
				do_init = init_none = 1;
			break;
		case TRANS_METHOD_NFS:
			if (init_nfs == 0)
				do_init = init_nfs = 1;
			break;
		case TRANS_METHOD_TCP:
			if (init_tcp == 0)
				do_init = init_tcp = 1;
			break;
		case TRANS_METHOD_USB:
			if (init_usb == 0)
				do_init = init_usb = 1;
			break;
		case TRANS_METHOD_STDOUT:
			if (init_stdout == 0)
				do_init = init_stdout = 1;
			break;
		default:
			return -1;
		}
		if (do_init)
			rtn = amba_transfer_init(stream_transfer[i].method);
		if (rtn < 0)
			return -1;
	}
	return 0;
}

static int deinit_transfer(void)
{
	if (init_none > 0)
		init_none = amba_transfer_deinit(TRANS_METHOD_NONE);
	if (init_nfs > 0)
		init_nfs = amba_transfer_deinit(TRANS_METHOD_NFS);
	if (init_tcp > 0)
		init_tcp = amba_transfer_deinit(TRANS_METHOD_TCP);
	if (init_usb)
		init_usb = amba_transfer_deinit(TRANS_METHOD_USB);
	if (init_stdout)
		init_stdout = amba_transfer_deinit(TRANS_METHOD_STDOUT);
	if (init_none < 0 || init_nfs < 0 || init_tcp < 0 || init_usb < 0 || init_stdout < 0)
		return -1;
	return 0;
}

static void sigstop()
{
	destroy_efm_task();
	deinit_transfer();
	if (encrypt_buff.cipher_addr) {
		free(encrypt_buff.cipher_addr);
		encrypt_buff.cipher_addr = NULL;
	}

	/* Free up the YUV data reading from file */
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}

	exit(1);
}


int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	/* Map for EFM feature */
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	encrypt_buff.cipher_addr = (u8 *)malloc(ENCRYPT_BUFF_SIZE);
	if (encrypt_buff.cipher_addr == NULL) {
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!malloc cipher_addr filed\n");
	}
	encrypt_buff.buff_size = ENCRYPT_BUFF_SIZE;

	return 0;
}



int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	init_encoding_states();

	if (init_transfer() < 0) {
		return -1;
	}

	if (capture_encoded_video() < 0) {
		printf("capture encoded video failed \n");
		return -1;
	}

	destroy_efm_task();
	if (deinit_transfer() < 0) {
		return -1;
	}

	close(fd_iav);
	free(encrypt_buff.cipher_addr);
	return 0;
}


