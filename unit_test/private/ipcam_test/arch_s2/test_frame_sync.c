/*
 * test_encode.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2010/12/31 - [Louis Sun] create this file base on test2.c
 *	2011/10/31 - [Jian Tang] modified this file.
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

#include <basetypes.h>
#include <config.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			} 						\
		} while (0)
#endif

#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)

// the device file handle
static int fd_iav = -1;

#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while (0)

// h.264 config
typedef struct h264_gop_param_s {
	int h264_N;
	int h264_N_changed;
	int h264_idr_interval;
	int h264_idr_interval_changed;
} h264_gop_param_t;

typedef struct h264_enc_param_s {
	u16	intrabias_p;
	u16	intrabias_p_changed;

	u16	intrabias_b;
	u16	intrabias_b_changed;

	u16	bias_p_skip;
	u16	bias_p_skip_changed;

	u16	cpb_underflow_num;
	u16	cpb_underflow_den;
	u16	cpb_underflow_changed;

	u8	zmv_threshold;
	u8	zmv_threshold_changed;

	s8	user1_intra_bias;
	u8	user1_intra_bias_changed;
	s8	user1_direct_bias;
	u8	user1_direct_bias_changed;

	s8	user2_intra_bias;
	u8	user2_intra_bias_changed;
	s8	user2_direct_bias;
	u8	user2_direct_bias_changed;

	s8	user3_intra_bias;
	u8	user3_intra_bias_changed;
	s8	user3_direct_bias;
	u8	user3_direct_bias_changed;
} h264_enc_param_t;

typedef struct stream_qproi_param_t {
	int	type;
	int	type_changed;

	int	level;
	int	level_changed;

	u8	zmv_force_ref0;
	u8	zmv_force_ref0_changed;

	u8	zmv_force_ref1;
	u8	zmv_force_ref1_changed;

	int	stream_width;
	int	stream_height;
	int	qp_roi_start_x;
	int	qp_roi_start_y;
	int	qp_roi_end_x;
	int	qp_roi_end_y;
} stream_qproi_param_t;

typedef struct iav_matrix_data_s {
	u8 basic_qp_data;
	u8 adv_qp_data;
	u8 zmv_force_ref0 : 1;
	u8 zmv_force_ref1 : 1;
	u8 reserved1 : 6;
	u8 reserved0;
}  iav_matrix_data_t;

//stream and source buffer identifier
static int current_stream = -1; 	// -1 is a invalid stream, for initialize data only

//encoding settings
static iav_stream_id_t force_idr_id = 0;

static h264_gop_param_t h264_gop_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t h264_gop_param_changed_map = 0;

static h264_enc_param_t h264_enc_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t h264_enc_param_changed_map = 0;

static iav_frame_drop_info_ex_t frame_drop_info[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t frame_drop_info_changed_map;

static stream_qproi_param_t stream_qproi_matrix[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t stream_qproi_matrix_changed_map = 0;

static iav_stream_id_t show_params_map = 0;
static int sleep_time = 1;

#define	NO_ARG		0
#define	HAS_ARG		1

enum numeric_short_options {
	SPECIFY_FORCE_IDR = 0,
	SPECIFY_GOP_IDR,
	SPECIFY_FRAME_DROP,

	SPECIFY_INTRABIAS_P,
	SPECIFY_INTRABIAS_B,
	SPECIFY_PM_TYPE,
	SPECIFY_P_SKIP_BIAS,
	SPECIFY_VSKIP_BEFORE_ENCODE,
	SPECIFY_CPB_UNDERFLOW_RATIO,
	SPECIFY_ZMV_THRESHOLD,
	SPECIFY_MODE_BIAS_I4,
	SPECIFY_MODE_BIAS_I16,

	SPECIFY_QP_ROI_TYPE,
	SPECIFY_QP_ROI_LEVEL,

	SPECIFY_ZMV_REF0,
	SPECIFY_ZMV_REF1,

	SPECIFY_USER1_INTRA_BIAS,
	SPECIFY_USER1_DIRECT_BIAS,
	SPECIFY_USER2_INTRA_BIAS,
	SPECIFY_USER2_DIRECT_BIAS,
	SPECIFY_USER3_INTRA_BIAS,
	SPECIFY_USER3_DIRECT_BIAS,

	SHOW_PARAMETERS,

};

static struct option long_options[] = {
	{"stream",	HAS_ARG,	0,	'S' },   // -Sx means stream no. X
	{"stream_A",	NO_ARG,		0,	'A' },   // -A xxxxx    means all following configs will be applied to stream A
	{"stream_B",	NO_ARG,		0,	'B' },
	{"stream_C",	NO_ARG,		0,	'C' },
	{"stream_D",	NO_ARG,		0,	'D' },
	{"stream_E",	NO_ARG,		0,	'E' },
	{"stream_F",	NO_ARG,		0,	'F' },
	{"stream_G",	NO_ARG,		0,	'G' },
	{"stream_H",	NO_ARG,		0,	'H' },

	{"force-idr",	NO_ARG,		0,	SPECIFY_FORCE_IDR },

	// 264 params
	{"intrabias-p",	HAS_ARG,	0,	SPECIFY_INTRABIAS_P },
	{"intrabias-b",	HAS_ARG,	0,	SPECIFY_INTRABIAS_B },
	{"p-skip-bias",	HAS_ARG,	0,	SPECIFY_P_SKIP_BIAS },
	{"cpb-ratio",		HAS_ARG,	0,	SPECIFY_CPB_UNDERFLOW_RATIO},
	{"zmv-threshold",	HAS_ARG,	0,	SPECIFY_ZMV_THRESHOLD},
	{"mode-bias-I4",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_I4},
	{"mode-bias-I16",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_I16},

	//h264 gop encode configurations
	{"N",		HAS_ARG,	0,	'N'},
	{"idr",		HAS_ARG,	0,	SPECIFY_GOP_IDR},

	//qp roi setting
	{"qproi-type",	HAS_ARG,	0,	 SPECIFY_QP_ROI_TYPE},
	{"qproi-level",	HAS_ARG,	0,	 SPECIFY_QP_ROI_LEVEL},

	//zmv ref setting
	{"zmv-force-ref0",	HAS_ARG,	0,	 SPECIFY_ZMV_REF0},
	{"zmv-force-ref1",	HAS_ARG,	0,	 SPECIFY_ZMV_REF1},

	//user bias setting
	{"user1-intra",	HAS_ARG,	0,	 SPECIFY_USER1_INTRA_BIAS},
	{"user1-direct",	HAS_ARG,	0,	 SPECIFY_USER1_DIRECT_BIAS},
	{"user2-intra",	HAS_ARG,	0,	 SPECIFY_USER2_INTRA_BIAS},
	{"user2-direct",	HAS_ARG,	0,	 SPECIFY_USER2_DIRECT_BIAS},
	{"user3-intra",	HAS_ARG,	0,	 SPECIFY_USER3_INTRA_BIAS},
	{"user3-direct",	HAS_ARG,	0,	 SPECIFY_USER3_DIRECT_BIAS},

	// frame drop setting
	{"frame-drop",		HAS_ARG,	0,	SPECIFY_FRAME_DROP},

	{"show-param",	NO_ARG, 0,	SHOW_PARAMETERS},
	{"sleep",	HAS_ARG,		0,	's' },

	{0, 0, 0, 0}
};

static const char *short_options = "ABCDEFGHN:s:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0~15", "\tspecify stream ID\n"},
	{"", "\t\tconfig for stream A"},
	{"", "\t\tconfig for stream B"},
	{"", "\t\tconfig for stream C"},
	{"", "\t\tconfig for stream D"},
	{"", "\t\tconfig for stream E"},
	{"", "\t\tconfig for stream F"},
	{"", "\t\tconfig for stream G"},
	{"", "\t\tconfig for stream H\n"},

	//immediate action, configure encode stream on the fly
	{"", "\t\tforce IDR at once for current stream"},
	{"1~4000", "set intrabias for current stream P frame"},
	{"1~4000", "set intrabias for current stream B frame"},
	{"1~4000", "set bias for P-skip of current stream, larger value means higher possibility to encode frame as P skip"},
	{"1~255/1~255", "set CPB buffer underflow ratio"},
	{"0~255", "set zmv threshold for current stream, value 0 means disable this"},
	{"-4~5", "set mode bias of I4, default is 0."},
	{"-4~5", "set mode bias of I16, default is 0.\n"},

	//h264 gop encode configurations
	{"1~255", "\t\tH.264 GOP parameter N, must be multiple of M, can be changed during encoding"},
	{"1~128", "\thow many GOP's, an IDR picture should happen, can be changed during encoding"},
	{"0|1", "\tsetting qp roi type, 0:base type, 1: adv type, Just support adv type now, default 1"},
	{"-51~51", "\tsetting qp roi level"},

	{"0|1", "\tforce zmv on ref0"},
	{"0|1", "\tforce zmv on ref1"},

	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},
	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},
	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},
	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},
	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},
	{"0~9", "\t setting bias strength,0:no bias, 9: the strongest."},

	{"0~300", "\thow many frame drops"},

	{"", "show parameters. \n"},
	{"", "sleep time, unit:ms\n"},

};

//first second value must in format "x~y" if delimiter is '~'
static int get_two_unsigned_int(const char *name, u32 *first, u32 *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

void usage(void)
{
	int i;

	printf("test_encode usage:\n");
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

static inline int check_intrabias(u32 intrabias)
{
	if (intrabias > INTRABIAS_MAX || intrabias < INTRABIAS_MIN) {
		printf("Invalid value:must be %d~%d\n", INTRABIAS_MIN, INTRABIAS_MAX);
		return -1;
	}

	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int value;
	u32 min_value, max_value;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

		// handle all other options
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
			case 'E':
				current_stream = 4;
				break;
			case 'F':
				current_stream = 5;
				break;
			case 'G':
				current_stream = 6;
				break;
			case 'H':
				current_stream = 7;
				break;

			case 'S':
				min_value = atoi(optarg);
				VERIFY_STREAMID(min_value);
				current_stream = min_value;
				break;

			case SPECIFY_FORCE_IDR:
				VERIFY_STREAMID(current_stream);
				//force idr
				force_idr_id |= (1 << current_stream);
				break;


			case SPECIFY_INTRABIAS_P:
				VERIFY_STREAMID(current_stream);
				//change intrabias_p on the fly
				h264_enc_param[current_stream].intrabias_p = atoi(optarg);
				h264_enc_param[current_stream].intrabias_p_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_INTRABIAS_B:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].intrabias_b = atoi(optarg);
				h264_enc_param[current_stream].intrabias_b_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_P_SKIP_BIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].bias_p_skip = atoi(optarg);
				h264_enc_param[current_stream].bias_p_skip_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_CPB_UNDERFLOW_RATIO:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '/') < 0) {
					return -1;
				}
				h264_enc_param[current_stream].cpb_underflow_num = min_value;
				h264_enc_param[current_stream].cpb_underflow_den = max_value;
				h264_enc_param[current_stream].cpb_underflow_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_ZMV_THRESHOLD:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (min_value > ZMV_TH_MAX || min_value < ZMV_TH_MIN) {
					printf("Invalid zmv threshold value [%d], please choose from [%d~%d].\n",
						min_value, ZMV_TH_MIN, ZMV_TH_MAX);
					return -1;
				}
				h264_enc_param[current_stream].zmv_threshold = min_value;
				h264_enc_param[current_stream].zmv_threshold_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case 'N':
				VERIFY_STREAMID(current_stream);
				h264_gop_param[current_stream].h264_N = atoi(optarg);
				h264_gop_param[current_stream].h264_N_changed = 1;
				h264_gop_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_GOP_IDR:
				//idr
				VERIFY_STREAMID(current_stream);
				h264_gop_param[current_stream].h264_idr_interval = atoi(optarg);
				h264_gop_param[current_stream].h264_idr_interval_changed = 1;
				h264_gop_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_ROI_TYPE:
				VERIFY_STREAMID(current_stream);
				stream_qproi_matrix[current_stream].type = atoi(optarg);
				stream_qproi_matrix[current_stream].type_changed= 1;
				stream_qproi_matrix_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_ROI_LEVEL:
				VERIFY_STREAMID(current_stream);
				stream_qproi_matrix[current_stream].level = atoi(optarg);
				stream_qproi_matrix[current_stream].level_changed= 1;
				stream_qproi_matrix_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_ZMV_REF0:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > 1 || value < 0) {
					printf("Invalid zmv force ref value [%d], please choose [0 | 1].\n",
						value);
					return -1;
				}
				stream_qproi_matrix[current_stream].zmv_force_ref0 = value;
				stream_qproi_matrix[current_stream].zmv_force_ref0_changed = 1;
				stream_qproi_matrix_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_ZMV_REF1:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > 1 || value < 0) {
					printf("Invalid zmv force ref value [%d], please choose [0 | 1].\n",
						value);
					return -1;
				}
				stream_qproi_matrix[current_stream].zmv_force_ref1 = value;
				stream_qproi_matrix[current_stream].zmv_force_ref1_changed = 1;
				stream_qproi_matrix_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER1_INTRA_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user1_intra_bias = value;
				h264_enc_param[current_stream].user1_intra_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER1_DIRECT_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user1_direct_bias = value;
				h264_enc_param[current_stream].user1_direct_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER2_INTRA_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user2_intra_bias = value;
				h264_enc_param[current_stream].user2_intra_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER2_DIRECT_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user2_direct_bias = value;
				h264_enc_param[current_stream].user2_direct_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER3_INTRA_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user3_intra_bias = value;
				h264_enc_param[current_stream].user3_intra_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_USER3_DIRECT_BIAS:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > USER_BIAS_VALUE_MAX || value < USER_BIAS_VALUE_MIN) {
					printf("Invalid user bias value [%d], please choose from [%d~%d].\n",
						value, USER_BIAS_VALUE_MIN, USER_BIAS_VALUE_MAX);
					return -1;
				}
				h264_enc_param[current_stream].user3_direct_bias = value;
				h264_enc_param[current_stream].user3_direct_bias_changed = 1;
				h264_enc_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_FRAME_DROP:
				VERIFY_STREAMID(current_stream);
				value = atoi(optarg);
				if (value > 300) {
					printf("Invalid frame drop num [%d], must be [0~300]\n", value);
					return -1;
				}
				frame_drop_info[current_stream].drop_frames_num = value;
				frame_drop_info_changed_map |= (1 << current_stream);
				break;

			case SHOW_PARAMETERS:
				VERIFY_STREAMID(current_stream);
				show_params_map |= (1 << current_stream);
				break;

			case 's':
				sleep_time = atoi(optarg);
				break;

			default:
				printf("unknown option found: %c\n", ch);
				return -1;
			}
	}

	return 0;
}

static int init_default_value(void)
{
	memset(stream_qproi_matrix, 0, sizeof(stream_qproi_param_t) *
		MAX_ENCODE_STREAM_NUM);

	return 0;
}

static int cfg_sync_frame_gop_param(int stream)
{
	iav_stream_cfg_t sync_frame;
	h264_gop_param_t * param = &h264_gop_param[stream];
	iav_change_gop_ex_t *gop_param = &sync_frame.arg.h_gop;

	memset(&sync_frame, 0, sizeof(iav_stream_cfg_t));

	gop_param->id = 1 << stream;

	sync_frame.cid = IAV_H264CFG_GOP;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->h264_N_changed)
		gop_param->N = param->h264_N;

	if (param->h264_idr_interval_changed)
		gop_param->idr_interval = param->h264_idr_interval;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_enc_param(int stream)
{
	iav_stream_cfg_t sync_frame;
	h264_enc_param_t * param = &h264_enc_param[stream];
	iav_h264_enc_param_ex_t *h264_param = &sync_frame.arg.h_enc_param;

	memset(&sync_frame, 0, sizeof(iav_stream_cfg_t));

	h264_param->id = 1 << stream;

	sync_frame.cid = IAV_H264CFG_ENC_PARAM;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->intrabias_p_changed)
		h264_param->intrabias_P = param->intrabias_p;
	if (param->intrabias_b_changed)
		h264_param->intrabias_B = param->intrabias_b;
	if (param->bias_p_skip_changed) {
		h264_param->nonSkipCandidate_bias = param->bias_p_skip;
		h264_param->skipCandidate_threshold = param->bias_p_skip;
	}
	if (param->cpb_underflow_changed) {
		h264_param->cpb_underflow_num = param->cpb_underflow_num;
		h264_param->cpb_underflow_den= param->cpb_underflow_den;
	}
	if (param->zmv_threshold_changed)
		h264_param->zmv_threshold = param->zmv_threshold;

	if (param->user1_intra_bias_changed)
		h264_param->user1_intra_bias = param->user1_intra_bias;
	if (param->user1_direct_bias_changed)
		h264_param->user1_direct_bias = param->user1_direct_bias;
	if (param->user2_intra_bias_changed)
		h264_param->user2_intra_bias = param->user2_intra_bias;
	if (param->user2_direct_bias_changed)
		h264_param->user2_direct_bias = param->user2_direct_bias;
	if (param->user3_intra_bias_changed)
		h264_param->user3_intra_bias = param->user3_intra_bias;
	if (param->user3_direct_bias_changed)
		h264_param->user3_direct_bias = param->user3_direct_bias;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_qproi_param(int stream)
{
	iav_stream_cfg_t sync_frame;
	iav_qp_roi_matrix_ex_t *h264_qp_param = &sync_frame.arg.h_qp_roi;
	iav_matrix_data_t *qproi_daddr = NULL;
	u32 size;

	int i, j;
	u32 buf_width, buf_pitch, buf_height, start_x, start_y, end_x, end_y;

	sync_frame.cid = IAV_H264CFG_QP_ROI;
	h264_qp_param->id = (1 << stream);

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_qproi_matrix[stream].stream_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_qproi_matrix[stream].stream_height, 16) / 16;
	size = buf_pitch * buf_height;

	h264_qp_param->size = size * sizeof(iav_matrix_data_t);

	h264_qp_param->daddr = (u8 *)malloc(h264_qp_param->size);
	if (h264_qp_param->daddr == NULL) {
		perror("The point is NULL\n");
		return -1;
	}
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	/*	get the last qp roi setting	*/
	qproi_daddr = (iav_matrix_data_t *)(h264_qp_param->daddr);

	/* clear buf for qp roi */
	if (stream_qproi_matrix[stream].type) {
		for (i = 0; i < size; i++) {
			qproi_daddr[i].adv_qp_data = QP_MATRIX_ADV_DEFAULT_VALUE;
		}
	} else {
		for (i = 0; i < size; i++) {
			qproi_daddr[i].basic_qp_data = 0;
		}
	}

	/* Just test with a fix area (left top)	*/
	start_x = 0;
	start_y = 0;
	end_x = buf_width / 2 + start_x;
	end_y =  buf_height / 2 + start_y;

	if (stream_qproi_matrix[stream].level_changed) {
		if (stream_qproi_matrix[stream].type) {
			for (i = start_y; i < end_y && i < buf_height; i++) {
				for (j = start_x; j < end_x && j < buf_width; j++) {
					/* bit8~15 is used for advance qp roi type */
					qproi_daddr[i * buf_pitch + j].adv_qp_data =
						stream_qproi_matrix[stream].level +
						QP_MATRIX_ADV_DEFAULT_VALUE;
				}
			}
		} else {
			for (i = start_y; i < end_y && i < buf_height; i++) {
				for (j = start_x; j < end_x && j < buf_width; j++) {
					qproi_daddr[i * buf_pitch + j].basic_qp_data =
						stream_qproi_matrix[stream].level;
				}
			}
		}
	}

	/* Just test for ref0, ref1 with a fix area (right half)	*/
	start_x = ROUND_UP(buf_pitch >> 1, 8);
	start_y = 0;
	end_x = buf_pitch;
	end_y =  buf_height;

	if (stream_qproi_matrix[stream].zmv_force_ref0_changed ||
		stream_qproi_matrix[stream].zmv_force_ref1_changed) {
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_pitch; j++) {
				if (stream_qproi_matrix[stream].zmv_force_ref0_changed) {
					qproi_daddr[i * buf_pitch + j].zmv_force_ref0 =
						stream_qproi_matrix[stream].zmv_force_ref0;
				}
				if (stream_qproi_matrix[stream].zmv_force_ref1_changed) {
					qproi_daddr[i * buf_pitch + j].zmv_force_ref1 =
						stream_qproi_matrix[stream].zmv_force_ref1;
				}
			}
		}
	}

	/* update new addr */
	sync_frame.cid = IAV_H264CFG_QP_ROI;
	h264_qp_param->enable = 1;
	h264_qp_param->type = stream_qproi_matrix[stream].type;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	if (h264_qp_param->daddr > 0) {
		free(h264_qp_param->daddr);
		h264_qp_param->daddr = NULL;
	}

	return 0;
}

static int cfg_sync_frame_force_idr(int stream)
{
	iav_stream_cfg_t sync_frame;
	memset(&sync_frame, 0, sizeof(iav_stream_cfg_t));

	sync_frame.cid = IAV_H264CFG_FORCE_IDR;
	sync_frame.arg.h_force_idr_sid = stream;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_drop(int stream)
{
	iav_stream_cfg_t sync_frame;
	sync_frame.cid = IAV_STMCFG_FRAME_DROP;
	sync_frame.arg.frame_drop.id = (1 << stream);
	sync_frame.arg.frame_drop.drop_frames_num =
		frame_drop_info[stream].drop_frames_num;
	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int do_vca_on_yuv(iav_yuv_buffer_info_ex_t *info)
{
	iav_buf_cap_t cap;
	iav_yuv_cap_t *yuv;
	int i;
#define TEST_PATTERN	256

	if (info == NULL) {
		printf("The point is NULL\n");
		return -1;
	}
	memset(&cap, 0, sizeof(cap));
	cap.flag = 1;
	if (ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap)) {
		return -1;
	}
	yuv = &cap.yuv[info->source];
	info->y_addr = yuv->y_addr;
	info->uv_addr = yuv->uv_addr;
	info->width = yuv->width;
	info->height = yuv->height;
	info->pitch = yuv->pitch;
	info->seqnum = yuv->seqnum;
	info->mono_pts = yuv->mono_pts;
	info->dsp_pts = yuv->dsp_pts;
	info->format = yuv->format;

	/*	To do,  run VCA algorithm here	*/
	for (i = 0; i < TEST_PATTERN; i++) {
		memset(info->y_addr + (16 + i) * info->pitch, 0x0, TEST_PATTERN);
	}
	usleep(1000 * sleep_time);

	return 0;
}

static int apply_sync_frame(u32 dsp_pts)
{
	AM_IOCTL(fd_iav, IAV_IOC_APPLY_FRAME_SYNC_PROC, dsp_pts);
	return 0;
}

static int do_iav_sync_frame(void)
{
	int i;
	iav_state_info_t info;
	iav_encode_stream_info_ex_t stream_info;
	iav_encode_format_ex_t format;
	iav_yuv_buffer_info_ex_t yuv_info;
	u32 dsp_pts;
	int apply_flag = 0;

	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);
	if (info.state != IAV_STATE_ENCODING) {
		return 0;
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		format.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);

		if ((format.encode_type != IAV_ENCODE_H264) &&
			(stream_info.state != IAV_STREAM_STATE_ENCODING)) {
			continue;
		}

		/* dsp_pts from yuv is the same in one interrrupt */
		if (!apply_flag) {
			yuv_info.source = format.source;
			if (do_vca_on_yuv(&yuv_info) < 0) {
				perror("do_vca_on_yuv");
				return -1;
			}
		}

		if (stream_qproi_matrix_changed_map & stream_info.id) {
			stream_qproi_matrix[i].stream_width = format.encode_width;
			stream_qproi_matrix[i].stream_height = format.encode_height;
			if (!stream_qproi_matrix[i].type_changed)
				stream_qproi_matrix[i].type = 1;
			if (!stream_qproi_matrix[i].level_changed)
				stream_qproi_matrix[i].level = 0;
			if (cfg_sync_frame_qproi_param(i) < 0) {
				return -1;
			}
			apply_flag = 1;
		}
		if (h264_enc_param_changed_map & stream_info.id) {
			if (cfg_sync_frame_enc_param(i) < 0) {
				return -1;
			}
			apply_flag = 1;

		}
		if (h264_gop_param_changed_map & stream_info.id) {
			if (cfg_sync_frame_gop_param(i) < 0) {
				return -1;
			}
			apply_flag = 1;
		}
		if (force_idr_id & stream_info.id) {
			if (cfg_sync_frame_force_idr(i) < 0) {
				return -1;
			}
			apply_flag = 1;
		}
		if (frame_drop_info_changed_map & stream_info.id) {
			if (cfg_sync_frame_drop(i) < 0) {
				return -1;
			}
			apply_flag = 1;
		}
	}

	if (apply_flag) {
		dsp_pts = (u32)(yuv_info.dsp_pts & 0xffffffff);
		if (apply_sync_frame(dsp_pts) < 0) {
			perror("apply_sync_frame\n");
			return -1;
		}
	}
	return 0;
}

static int show_iav_sync_frame(void)
{
	int i;
	iav_stream_cfg_t sync_frame;
	u32 buf_width, buf_pitch, buf_height;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		sync_frame.arg.h_enc_param.id = ((1 << i) & show_params_map);
		if (sync_frame.arg.h_enc_param.id == 0)
			continue;

		sync_frame.cid = IAV_H264CFG_ENC_PARAM;
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("===IAV_H264CFG_ENC_PARAM stream[%c]===\n", 'A' + i);
		printf("\t intrabias_P:%d\n", sync_frame.arg.h_enc_param.intrabias_P);
		printf("\t intrabias_B:%d\n", sync_frame.arg.h_enc_param.intrabias_B);
		printf("\t nonSkipCandidate_bias:%d\n", sync_frame.arg.h_enc_param.nonSkipCandidate_bias);
		printf("\t skipCandidate_threshold:%d\n", sync_frame.arg.h_enc_param.skipCandidate_threshold);
		printf("\t cpb_underflow_num:%d\n", sync_frame.arg.h_enc_param.cpb_underflow_num);
		printf("\t cpb_underflow_den:%d\n", sync_frame.arg.h_enc_param.cpb_underflow_den);
		printf("\t zmv_threshold:%d\n", sync_frame.arg.h_enc_param.zmv_threshold);
		printf("\t mode_bias_I4Add:%d\n", sync_frame.arg.h_enc_param.mode_bias_I4Add);
		printf("\t mode_bias_I16Add:%d\n", sync_frame.arg.h_enc_param.mode_bias_I16Add);

		sync_frame.cid = IAV_H264CFG_QP_ROI;
		sync_frame.arg.h_qp_roi.id = ((1 << i) & show_params_map);

		buf_width = ROUND_UP(stream_qproi_matrix[i].stream_width, 16) / 16;
		buf_pitch = ROUND_UP(buf_width, 8);
		buf_height = ROUND_UP(stream_qproi_matrix[i].stream_height, 16) / 16;

		sync_frame.arg.h_qp_roi.size = buf_pitch * buf_height * sizeof(iav_matrix_data_t);
		sync_frame.arg.h_qp_roi.daddr = malloc(sync_frame.arg.h_qp_roi.size);
		if (sync_frame.arg.h_qp_roi.daddr == NULL) {
			perror("malloc\n");
			return -1;
		}
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t qp roi: enable:%d\n", sync_frame.arg.h_qp_roi.enable);
		printf("\t qp roi: type:%d\n", sync_frame.arg.h_qp_roi.type);

		if (sync_frame.arg.h_qp_roi.daddr) {
			free(sync_frame.arg.h_qp_roi.daddr);
			sync_frame.arg.h_qp_roi.daddr  = NULL;
		}

		sync_frame.cid = IAV_H264CFG_GOP;
		sync_frame.arg.h_gop.id = ((1 << i) & show_params_map);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t Gop: N:%d\n", sync_frame.arg.h_gop.N);
		printf("\t Gop: idr_interval:%d\n", sync_frame.arg.h_gop.idr_interval);

	}
	return 0;
}

static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	/*	for yuv cap	*/
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}

int main(int argc, char **argv)
{
	int do_iav_sync_frame_flag = 0;

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_default_value() < 0)
		return -1;

	if (init_param(argc, argv) < 0)
		return -1;

	if (map_buffer() < 0) {
		return -1;
	}

	//real time change flag
	if (force_idr_id || h264_enc_param_changed_map
		|| h264_gop_param_changed_map || stream_qproi_matrix_changed_map
		|| frame_drop_info_changed_map) {
		do_iav_sync_frame_flag = 1;
	}

/********************************************************
 *  execution base on flag
 *******************************************************/

	//real time change encoding parameter
	if (do_iav_sync_frame_flag) {
		if (do_iav_sync_frame() < 0)
			return -1;
	}

	if (show_params_map) {
		if (show_iav_sync_frame() < 0)
			return -1;
	}

	if (fd_iav >= 0)
		close(fd_iav);

	return 0;
}

