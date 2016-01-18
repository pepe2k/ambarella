/********************************************************************
 * test_statistics.c
 *
 * History:
 *	2012/08/09 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/

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
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"


#define NO_ARG		0
#define HAS_ARG		1

#define MAX_MVDUMP_DIVISION_FACTOR		(30)

#define MAX_NUM_ENCODE_STREAMS	(8)

typedef enum {
	DUMP_NONE = 0,
	DUMP_MV = 1,
	DUMP_QP_HIST = 2,
	DUMP_TOTAL_NUM,

	DUMP_FIRST = DUMP_MV,
	DUMP_LAST = DUMP_TOTAL_NUM,
} DUMP_DATA_TYPE;

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef	ROUND_UP
#define ROUND_UP(x, n)    ( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#define VERIFY_STREAMID(x)	do {		\
			if ((x < 0) || (x >= MAX_NUM_ENCODE_STREAMS)) {	\
				printf("Stream id [%d] is wrong!\n", x);	\
				return -1;	\
			}	\
		} while (0)


struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_opts = "Ab:Bc:Cd:DEFGHm:v";

static struct option long_opts[] = {
	{"stream-A",	NO_ARG, 0, 'A'},
	{"stream-B",	NO_ARG, 0, 'B'},
	{"stream-C",	NO_ARG, 0, 'C'},
	{"stream-D",	NO_ARG, 0, 'D'},
	{"stream-E",	NO_ARG, 0, 'E'},
	{"stream-F",	NO_ARG, 0, 'F'},
	{"stream-G",	NO_ARG, 0, 'G'},
	{"stream-H",	NO_ARG, 0, 'H'},
	{"non-block",	HAS_ARG, 0, 'b'},
	{"count",		HAS_ARG, 0, 'c'},
	{"dump",		HAS_ARG, 0, 'd'},
	{"mv-div",	HAS_ARG, 0, 'm'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\tSet configuration for stream A"},
	{"", "\tSet configuration for stream B"},
	{"", "\tSet configuration for stream C"},
	{"", "\tSet configuration for stream D"},
	{"", "\tSet configuration for stream E"},
	{"", "\tSet configuration for stream F"},
	{"", "\tSet configuration for stream G"},
	{"", "\tSet configuration for stream H"},
	{"0|1", "Select the read method, 0 is block call, 1 is non-block call. Default is block call."},
	{"1~60", "The frames to display the statistics. Default is one frame."},
	{"1~2", "\tDump data. 1: Dump MV statistics. 2: Dump QP histogram statistics."},
	{"1~30", "Set the frequency of motion vectors dumped in DRAM, default is 3\n"},

	{"", "\tprint more messages"},
};


static int fd_iav = -1;
static int current_stream = -1;
static int non_block_read = 0;
static int frame_count = 1;
static int dump_data_type = DUMP_NONE;
static int mvdump_division_factor = 3;
static int verbose = 0;

static void examples_usage(const char *name)
{
	printf("\nExamples:\n");
	printf("  Dump MV statistics data per 5 frames:\n");
	printf("    %s -A -d 1 -m 5\n\n", name);
	printf("  Dump QP histogram data:\n");
	printf("    %s -A -d 2", name);
}

static void usage(void)
{
	u32 i;
	const char * itself = "test_statistics";

	printf("%s usage:\n\n", itself);
	for (i = 0; i < sizeof(long_opts) / sizeof(long_opts[0]) - 1; ++i) {
		if (isalpha(long_opts[i].val))
			printf("-%c ", long_opts[i].val);
		else
			printf("   ");
		printf("--%s", long_opts[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	examples_usage(itself);

	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index)) != -1) {
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
		case 'b':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Read flag [%d] must be [0|1].\n", value);
				return -1;
			}
			non_block_read = value;
			break;
		case 'c':
			value = atoi(optarg);
			if (value < 1 || value > 60) {
				printf("Frame count [%d] must be [1~60].\n", value);
				return -1;
			}
			frame_count = value;
			break;
		case 'd':
			value = atoi(optarg);
			if (value < DUMP_FIRST || value >= DUMP_LAST) {
				printf("Dump data type %d must be in the range of [%d~%d].\n",
					value, DUMP_FIRST, (DUMP_LAST - 1));
				return -1;
			}
			dump_data_type = value;
			break;
		case 'm':
			VERIFY_STREAMID(current_stream);
			value = atoi(optarg);
			if (value < 1 || value > MAX_MVDUMP_DIVISION_FACTOR) {
				printf("Please set the division factor [%d] from the range of [1~%d]\n",
					value, MAX_MVDUMP_DIVISION_FACTOR);
				return -1;
			}
			mvdump_division_factor = value;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}

	if (dump_data_type == DUMP_NONE) {
		printf("Please select dump data type from [%d~%d].\n",
			DUMP_FIRST, (DUMP_LAST - 1));
		return -1;
	}

	return 0;
}


static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_MV_EX, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_MAP_QP_HIST_EX, &info) < 0) {
		perror("IAV_IOC_MAP_QP_HIST_EX");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}

static int display_mv_statistics(iav_enc_statis_info_ex_t * stat_info)
{
	iav_motion_vector_ex_t * mv_dump = NULL;
	iav_encode_format_ex_t encode_format;
	u32 stream_pitch, stream_height, unit_sz = 0;
	int i, j;

	encode_format.id = (1 << current_stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format);
	stream_pitch = ROUND_UP(ROUND_UP(encode_format.encode_width, 16) / 16 *
		sizeof(iav_motion_vector_ex_t), 32);
	if (stream_pitch != stat_info->mvdump_pitch) {
		printf("Incorrect MV dump pitch [%d] for stream [%d] of pitch [%d].\n",
			stat_info->mvdump_pitch, current_stream, stream_pitch);
		return -1;
	}
	stream_height = ROUND_UP(encode_format.encode_height, 16) / 16;
	unit_sz = stream_pitch * stream_height;
	if (unit_sz != stat_info->mvdump_unit_size) {
		printf("Incorrect MV dump unit size [%d] for stream [%d] of size [%d].\n",
			stat_info->mvdump_unit_size, current_stream, unit_sz);
		return -1;
	}
	stream_pitch /= sizeof(iav_motion_vector_ex_t);
	printf("Motion vector for stream [%d], frame [%d], mono_pts [%llu], pitch [%d],"
		" height [%d].\n", current_stream, stat_info->frame_num,
		stat_info->mono_pts, stream_pitch, stream_height);
	mv_dump = (iav_motion_vector_ex_t *)stat_info->mv_start_addr;
	printf("Motion Vector X Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].x);
		}
		printf("\n");
	}
	printf("Motion Vector Y Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].y);
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

static int display_qp_histogram(iav_enc_statis_info_ex_t * info, int count)
{
	const int entry_num = 4;
	int i, j, index;
	int mb_sum, qp_sum, mb_entry;
	iav_qp_hist_ex_t * hist = NULL;

	hist = (iav_qp_hist_ex_t *)info->qp_hist_addr;
	if (!hist) {
		printf("Invalid QP histogram address for stream [%d].\n", info->stream);
		return -1;
	}

	printf("==[%2d]== Stream [%d], Frame NO. [%d], Type [%d], PTS [%llu].\n",
		count, info->stream, info->frame_num, info->frame_type, info->mono_pts);
	printf("====== QP : MB\n");

	mb_sum = qp_sum = 0;
	for (i = 0; i < IAV_QP_HIST_BIN_MAX_NUM / entry_num; ++i) {
		mb_entry = 0;
		printf(" [Set %d] ", i);
		for (j = 0; j < entry_num; ++j) {
			index = i * entry_num + j;
			printf("%2d:%-4d  ", hist->qp[index], hist->mb[index]);
			mb_entry += hist->mb[index];
			mb_sum += hist->mb[index];
			qp_sum += hist->qp[index] * hist->mb[index];
		}
		printf("[MBs: %d].\n", mb_entry);
	}
	printf("\n====== Total MB : %d.", mb_sum);
	printf("\n====== Average QP : %d.\n\n", qp_sum / mb_sum);

	return 0;
}

static int get_mv_statistics(int non_block, int frames)
{
	int failed = 0;
	iav_enc_statis_config_ex_t stat_config;
	iav_enc_statis_info_ex_t stat_info;

	memset(&stat_config, 0, sizeof(stat_config));
	stat_config.id = (1 << current_stream);
	stat_config.enable_mv_dump = 1;
	stat_config.mvdump_division_factor = mvdump_division_factor;
	if (ioctl(fd_iav, IAV_IOC_SET_STATIS_CONFIG_EX, &stat_config) < 0) {
		perror("IAV_IOC_SET_STATIS_CONFIG_EX");
		failed = 1;
	}
	do {
		if (failed) {
			break;
		}
		memset(&stat_info, 0, sizeof(stat_info));
		stat_info.stream = current_stream;
		stat_info.data_type = IAV_MV_DUMP_FLAG;
		stat_info.flag = non_block;
		if (ioctl(fd_iav, IAV_IOC_FETCH_STATIS_INFO_EX, &stat_info) < 0) {
			perror("IAV_IOC_FETCH_STATIS_INFO_EX");
			break;
		}
		if (display_mv_statistics(&stat_info) < 0) {
			printf("Failed to display motion vector!\n");
			break;
		}
		AM_IOCTL(fd_iav, IAV_IOC_RELEASE_STATIS_INFO_EX, &stat_info);
	} while (0);

	stat_config.enable_mv_dump = 0;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STATIS_CONFIG_EX, &stat_config);

	return 0;
}

static int get_qp_histogram(int non_block, int frames)
{
	iav_enc_statis_config_ex_t stat_config;
	iav_enc_statis_info_ex_t stat_info;

	memset(&stat_config, 0, sizeof(stat_config));
	stat_config.id = (1 << current_stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_STATIS_CONFIG_EX, &stat_config);
	stat_config.enable_qp_hist = 1;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STATIS_CONFIG_EX, &stat_config);

	/* Wait for statistics data ready. */
	usleep(100000);

	memset(&stat_info, 0, sizeof(stat_info));
	stat_info.stream = current_stream;
	stat_info.data_type = IAV_QP_HIST_DUMP_FLAG;
	stat_info.flag = non_block;
	while (frames > 0) {
		AM_IOCTL(fd_iav, IAV_IOC_FETCH_STATIS_INFO_EX, &stat_info);
		if (display_qp_histogram(&stat_info, frames) < 0) {
			printf("Failed to display QP histogram!\n");
			return -1;
		}
		AM_IOCTL(fd_iav, IAV_IOC_RELEASE_STATIS_INFO_EX, &stat_info);
		--frames;
	}

	stat_config.enable_qp_hist = 0;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STATIS_CONFIG_EX, &stat_config);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0) {
		printf("Failed to map buffer!\n");
		return -1;
	}

	switch (dump_data_type) {
	case DUMP_MV:
		if (get_mv_statistics(non_block_read, frame_count) < 0) {
			return -1;
		}
		break;
	case DUMP_QP_HIST:
		if (get_qp_histogram(non_block_read, frame_count) < 0) {
			return -1;
		}
		break;
	default:
		printf("Invalid dump data type [%d].\n", dump_data_type);
		break;
	}

	close(fd_iav);
	fd_iav = -1;

	return 0;
}


