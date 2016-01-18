/*
 * test_qp_overlay.c
 *
 * History:
 *	2012/02/13 - [Jian Tang] created file
 *	2012/11/19 - [Qian Shen] ported to S2
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
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

#define NO_ARG		0
#define HAS_ARG		1

#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define MAX_ROI_AREA_NUM		(3)
#define MAX_LINE_WIDTH			(8)
#define MAX_LOOP_TIME			(1000)

#define OVERLAY_CLUT_NUM		(16)
#define OVERLAY_CLUT_SIZE		(1024)
#define OVERLAY_CLUT_OFFSET	(0)
#define OVERLAY_YUV_OFFSET		(OVERLAY_CLUT_NUM * OVERLAY_CLUT_SIZE)

typedef enum {
	LINE_COLOR_FIRST = 0,
	LINE_COLOR_BLANK = 0,
	LINE_COLOR_RED = 1,
	LINE_COLOR_BLUE,
	LINE_COLOR_BLACK,
	LINE_COLOR_WHITE,
	LINE_COLOR_GREEN,
	LINE_COLOR_YELLOW,
	LINE_COLOR_CYAN,
	LINE_COLOR_MAGENTA,
	LINE_COLOR_LAST = LINE_COLOR_MAGENTA,
	LINE_COLOR_MAX = 255,

	LINE_COLOR_FULL_TRANS = 0,
	LINE_COLOR_SEMI_TRANS = 128,
	LINE_COLOR_NON_TRANS = 255,

	SOLID_LINE_MULTIPLE = 2,
	DASH_LINE_MULTIPLE = 1,
	TOTAL_LINE_MULTIPLE = (SOLID_LINE_MULTIPLE + DASH_LINE_MULTIPLE),
} LINE_PARAMS;

typedef enum {
	ROI_BOX_DISABLE = 0,
	ROI_BOX_ENABLE,
} ROI_BOX_TYPE;

typedef enum {
	MB_OUT_ROI = 0,
	MB_IN_ROI = 1,
	QP_DELTA_OUT_ROI = 0,
	QP_DELTA_IN_ROI = -10,
	QP_DELTA_MAX = 51,
	QP_DELTA_MIN = -51,
} MB_TYPES;

typedef enum {
	DEFAULT_WIDTH = 256,
	DEFAULT_HEIGHT = 192,
	DEFAULT_OFFSET_X = 256,
	DEFAULT_OFFSET_Y = 256,
	DEFAULT_LINE_WIDTH = 4,
	DEFAULT_COLOR = LINE_COLOR_RED,
	DEFAULT_QP_DELTA = 10,
	DEFAULT_LOOP_TIME = 30,
} DEFAULT_OVERLAY_PARAMETERS;

#ifndef ROUND_UP
#define ROUND_UP(size, align)	(((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_ROI_AREA_NUM)) {	\
				printf("QP ROI area id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_STREAMID
#define VERIFY_STREAMID(x)		do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf("Stream id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do {						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

struct hint_s {
	const char *arg;
	const char *str;
};

typedef struct qp_region_s {
	int	width;
	int	height;
	int	resolution_flag;

	int	offset_x;
	int	offset_y;
	int	offset_flag;

	int	line_width;
	int	line_width_flag;

	int	color;
	int	color_flag;

	int	qp_delta_in;
	int	qp_delta_out;
	int	qp_flag;
} qp_region_t;

typedef struct stream_qp_region_s {
	qp_region_t region[MAX_ROI_AREA_NUM];
	int	qp_region_change_id;
	int	total_active_regions;
	int	active_region;
} stream_qp_region_t;

typedef struct line_clut_s {
	u8	v;
	u8	u;
	u8	y;
	u8	alpha;
} line_clut_t;

static line_clut_t line_clut[] = {
	{ .y = 128, .u = 128, .v = 128, .alpha = LINE_COLOR_FULL_TRANS },	/* blank */
	{ .y =  82, .u =  90, .v = 240, .alpha = LINE_COLOR_NON_TRANS },	/* red */
	{ .y =  41, .u = 240, .v = 110, .alpha = LINE_COLOR_NON_TRANS },	/* blue */
	{ .y =  12, .u = 128, .v = 128, .alpha = LINE_COLOR_NON_TRANS },	/* black */
	{ .y = 235, .u = 128, .v = 128, .alpha = LINE_COLOR_NON_TRANS },	/* white */
	{ .y = 145, .u =  54, .v =  34, .alpha = LINE_COLOR_NON_TRANS },	/* green */
	{ .y = 210, .u =  16, .v = 146, .alpha = LINE_COLOR_NON_TRANS },	/* yellow */
	{ .y = 170, .u = 166, .v =  16, .alpha = LINE_COLOR_NON_TRANS },	/* cyan */
	{ .y = 107, .u = 202, .v = 222, .alpha = LINE_COLOR_NON_TRANS },	/* magenta */
};

static const char *short_opts = "a:c:f:l:o:q:r:t:vABCD";

static struct option long_opts[] = {
	{"stream-A",	NO_ARG, 0, 'A'},
	{"stream-B",	NO_ARG, 0, 'B'},
	{"stream-C",	NO_ARG, 0, 'C'},
	{"stream-D",	NO_ARG, 0, 'D'},
	{"area",		HAS_ARG, 0, 'a'},
	{"resolution",	HAS_ARG, 0, 'r'},
	{"offset",		HAS_ARG, 0, 'o'},
	{"line-width",	HAS_ARG, 0, 'l'},
	{"color",		HAS_ARG, 0, 'c'},
	{"qp-delta",	HAS_ARG, 0, 'q'},
	{"input-file",	HAS_ARG, 0, 'f'},
	{"loop-time",	HAS_ARG, 0, 't'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\tspecify QP ROI config for stream A"},
	{"", "\tspecify QP ROI config for stream B"},
	{"", "\tspecify QP ROI config for stream C"},
	{"", "\tspecify QP ROI config for stream D"},
	{"0~2", "\tspecify QP area number"},
	{"", "\tspecify area resolution, width and height must be the multiple of 16"},
	{"", "\tspecify area offset, x and y must be the multiple of 16"},
	{"1~8", "specify the border width"},
	{"1~8", "specify the border color, 1:red, 2:blue, 3:black, 4: white, 5: green, 6:yellow, 7: cyan, 8:magenta"},
	{"n", "specify QP delta for ROI"},
	{"file", "specify the parameter input file"},
	{"", "\tspecify the loop time of each area in seconds, default is 30s"},
	{"", "\tprint more messages"},
};

static int fd_iav = -1;
static int loop_time = DEFAULT_LOOP_TIME;
static int verbose = 0;
static int quit_flag = 0;

static int area = -1;
static int stream = -1;

static stream_qp_region_t qp_region[MAX_ENCODE_STREAM_NUM];

static u8 * qp_matrix_addr = NULL;
static u32 qp_matrix_size = 0;
static u32 stream_qp_matrix_size = 0;

// overlay parameters
static u8 * stream_clut_addr = NULL;
static u8 * stream_overlay_addr = NULL;
static u32 stream_overlay_size = 0;
static u32 area_overlay_size = 0;
static overlay_insert_ex_t overlay[MAX_ENCODE_STREAM_NUM];

// first and second values must be in format of "AxB" or "A/B"
static int get_two_int_of_multiple(const char *name, char delimiter,
	int order, int *first, int *second)
{
	char tmp_string[16];
	char * separator;
	int num1, num2;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator-name] = '\0';
	num1 = atoi(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	num2 = atoi(tmp_string);
	if ((num1 & ((1 << order) - 1)) || (num2 & ((1 << order) - 1))) {
		printf("Resolution or Offset must be the multiple of %d!\n", (1 << order));
		return -1;
	}

	*first = num1;
	*second = num2;

	return 0;
}

static void usage(void)
{
	u32 i;

	printf("test_qp_overlay usage:\n\n");
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
	printf("\n");
	printf("Examples:\n");
	printf("  test_qp_overlay -A -a 0 -r 128x128 -o 64x128 -a 1 -r 256x256 -o 512x128 -a 2 -r 128x128 -o 960x128 -t 15\n\n");
	printf("  test_qp_overlay -A -a 0 -r 128x128 -o 64x128 -q -15 -t 15\n\n");
}

static int init_param(int argc, char ** argv)
{
	int ch, value, first, second;
	int opt_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1) {
		switch (ch) {
		case 'A':
			stream = 0;
			break;
		case 'B':
			stream = 1;
			break;
		case 'C':
			stream = 2;
			break;
		case 'D':
			stream = 3;
			break;

		case 'a':
			VERIFY_STREAMID(stream);
			value = atoi(optarg);
			VERIFY_AREAID(value);
			area = value;
			break;
		case 'r':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			if (get_two_int_of_multiple(optarg, 'x', 4, &first, &second) < 0)
				return -1;
			qp_region[stream].region[area].width = first;
			qp_region[stream].region[area].height = second;
			qp_region[stream].region[area].resolution_flag = 1;
			qp_region[stream].qp_region_change_id |= (1 << area);
			break;
		case 'o':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			if (get_two_int_of_multiple(optarg, 'x', 4, &first, &second) < 0)
				return -1;
			qp_region[stream].region[area].offset_x = first;
			qp_region[stream].region[area].offset_y = second;
			qp_region[stream].region[area].offset_flag = 1;
			qp_region[stream].qp_region_change_id |= (1 << area);
			break;
		case 'l':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			value = atoi(optarg);
			if (value < 1 || value > MAX_LINE_WIDTH) {
				printf("Border line width must be in the range of '1~%d'.\n", MAX_LINE_WIDTH);
				return -1;
			}
			qp_region[stream].region[area].line_width = value;
			qp_region[stream].region[area].line_width_flag = 1;
			qp_region[stream].qp_region_change_id |= (1 << area);
			break;
		case 'c':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			value = atoi(optarg);
			if (value < LINE_COLOR_FIRST || value > LINE_COLOR_LAST) {
				printf("Border line color must be in the range of '%d~%d'.\n",
					LINE_COLOR_FIRST, LINE_COLOR_LAST);
				return -1;
			}
			qp_region[stream].region[area].color = value;
			qp_region[stream].region[area].color_flag = 1;
			qp_region[stream].qp_region_change_id |= (1 << area);
			break;
		case 'q':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			value = atoi(optarg);
			if ((value > QP_DELTA_MAX) || (value < QP_DELTA_MIN) ) {
				printf("QP delta [%d] must be in the range of (%d~%d).\n",
					value, QP_DELTA_MIN, QP_DELTA_MAX);
				return -1;
			}
			qp_region[stream].region[area].qp_delta_in = value;
			qp_region[stream].region[area].qp_delta_out = 0;
			qp_region[stream].region[area].qp_flag = 1;
			qp_region[stream].qp_region_change_id |= (1 << area);
			break;
		case 't':
			VERIFY_STREAMID(stream);
			VERIFY_AREAID(area);
			value = atoi(optarg);
			if (value <= 0 || value >= MAX_LOOP_TIME) {
				printf("Loop time must be in the range of '1~%d'.\n",
					MAX_LOOP_TIME);
				return -1;
			}
			loop_time = value;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found : %c\n", ch);
			break;
		}
	}

	VERIFY_STREAMID(stream);
	VERIFY_AREAID(area);

	return 0;
}

static int map_qp_overlay(void)
{
	static int mapped = 0;
	iav_mmap_info_t overlay_info, qp_info;

	if (mapped)
		return 0;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_OVERLAY, &overlay_info);
	stream_clut_addr = overlay_info.addr + OVERLAY_CLUT_OFFSET;
	stream_overlay_addr = overlay_info.addr + OVERLAY_YUV_OFFSET;
	stream_overlay_size = (overlay_info.length - OVERLAY_YUV_OFFSET)
	        / MAX_ENCODE_STREAM_NUM;
	area_overlay_size = stream_overlay_size / MAX_ROI_AREA_NUM;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_QP_ROI_MATRIX_EX, &qp_info);
	qp_matrix_addr = qp_info.addr;
	qp_matrix_size = qp_info.length;
	stream_qp_matrix_size = qp_info.length / MAX_ENCODE_STREAM_NUM;

	if (verbose) {
		printf("overlay: start = %p, total size = 0x%x (bytes)\n",
		       overlay_info.addr, overlay_info.length);
		printf("QP matrix: start = %p, total size = 0x%x (bytes)\n",
			qp_info.addr, qp_info.length);
	}

	mapped = 1;

	return 0;
}

static int prepare_roi_param(int stream, int area)
{
	qp_region_t * qp_area = &qp_region[stream].region[area];

	if (!qp_area->resolution_flag) {
		qp_area->width = DEFAULT_WIDTH;
		qp_area->height = DEFAULT_HEIGHT;
	}
	if (!qp_area->offset_flag) {
		qp_area->offset_x = DEFAULT_OFFSET_X;
		qp_area->offset_y = DEFAULT_OFFSET_Y;
	}
	if (!qp_area->line_width_flag) {
		qp_area->line_width = DEFAULT_LINE_WIDTH;
	}
	if (!qp_area->color_flag) {
		qp_area->color = DEFAULT_COLOR;
	}
	if (!qp_area->qp_flag) {
		qp_area->qp_delta_in = QP_DELTA_IN_ROI;
		qp_area->qp_delta_out = QP_DELTA_OUT_ROI;
	}
	return 0;
}

static int fill_overlay_clut(int stream, int area)
{
	int i;

	line_clut_t *clut_addr = (line_clut_t *)(stream_clut_addr +
		OVERLAY_CLUT_SIZE * (MAX_ROI_AREA_NUM * stream + area));
	for (i = LINE_COLOR_FIRST; i < LINE_COLOR_LAST; ++i) {
		clut_addr[i*2] = line_clut[i];
		clut_addr[i*2+1] = line_clut[i];
		clut_addr[i*2+1].alpha = LINE_COLOR_SEMI_TRANS;
	}
	return 0;
}

static int fill_overlay_param(int stream, int area)
{
	u8 * addr = stream_overlay_addr + stream * stream_overlay_size +
			area * area_overlay_size;
	overlay_insert_area_ex_t *overlay_area = &overlay[stream].area[area];
	qp_region_t * qp_area = &qp_region[stream].region[area];
	iav_encode_format_ex_t format;
	int start_x, start_y;

	overlay_area->enable = 1;
	overlay_area->width = qp_area->width;
	overlay_area->height = qp_area->height;
	overlay_area->pitch = ROUND_UP(overlay_area->width, 16);
	overlay_area->total_size = (overlay_area->width) * (overlay_area->height);

	if (overlay_area->total_size > area_overlay_size) {
		printf("Too large box!\n");
		return -1;
	}

	overlay_area->clut_id = MAX_ROI_AREA_NUM * stream + area;
	overlay_area->data = addr;
	memset(addr, LINE_COLOR_BLANK, overlay_area->total_size);

	// calculate the offset for hflip and vflip
	memset(&format, 0, sizeof(format));
	format.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
	if (format.hflip) {
		start_x = format.encode_width - qp_area->width -
			qp_area->offset_x;
	} else {
		start_x = qp_area->offset_x;
	}
	overlay_area->start_x = start_x;
	if (format.vflip) {
		start_y = format.encode_height - qp_area->height -
			qp_area->offset_y;
	} else {
		start_y = qp_area->offset_y;
	}
	overlay_area->start_y = start_y;

	return 0;
}

static int prepare_overlay_data(void)
{
	int i, area;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		qp_region[i].total_active_regions = 0;
		overlay[i].id = (1 << i);
		if (qp_region[i].qp_region_change_id == 0) {
			overlay[i].enable = ROI_BOX_DISABLE;
			continue;
		}
		overlay[i].enable = ROI_BOX_ENABLE;

		for (area = 0; area < MAX_ROI_AREA_NUM; ++area) {
			if (qp_region[i].qp_region_change_id & (1 << area)) {
				prepare_roi_param(i, area);
				fill_overlay_clut(i, area);
				if (fill_overlay_param(i, area) < 0)
					return -1;
				++qp_region[i].total_active_regions;
			}
		}
		qp_region[i].active_region = 0;
	}
	return 0;
}

static int draw_dash_border(u8 * addr, int width, int height,
	int line_width, int border_color)
{
	int x, y, fill_color;
	int solid_length = SOLID_LINE_MULTIPLE * line_width;
	int total_length = TOTAL_LINE_MULTIPLE * line_width;
	// draw top / bottom border
	for (y = 0; y < height; ++y) {
		if (y >= line_width && y < (height - line_width))
			continue;
		for (x = 0; x < width; ++x) {
			if ((x % total_length) < solid_length) {
				fill_color = border_color;
			} else {
				fill_color = LINE_COLOR_BLANK;
			}
			addr[y * width + x] = fill_color;
		}
	}

	// draw left / right border
	for (y = line_width; y < height - line_width; ++y) {
		if ((y % total_length) < solid_length) {
			fill_color = border_color;
		} else {
			fill_color = LINE_COLOR_BLANK;
		}
		for (x = 0; x < line_width; ++x) {
			addr[y * width + x] = fill_color;
		}
		for (x = width - line_width; x < width; ++x) {
			addr[y * width + x] = fill_color;
		}
	}
	return 0;
}

static int draw_solid_border(u8 * addr, int width, int height,
	int line_width, int border_color)
{
	int x, y;
	// draw top / bottom border
	for (y = 0; y < height; ++y) {
		if (y >= line_width && y < (height - line_width))
			continue;
		for (x = 0; x < width; ++x) {
			addr[y * width + x] = border_color;
		}
	}

	// draw left / right border
	for (y = line_width; y < height - line_width; ++y) {
		for (x = 0; x < line_width; ++x) {
			addr[y * width + x] = border_color;
		}
		for (x = width - line_width; x < width; ++x) {
			addr[y * width + x] = border_color;
		}
	}
	return 0;
}

static int draw_box_border(u8 *addr, qp_region_t * qp_area, int inactive)
{
	int border_color = qp_area->color * 2 + inactive;
	int width = qp_area->width;
	int height = qp_area->height;
	int line_width = qp_area->line_width;

	if (inactive) {
		draw_dash_border(addr, width, height, line_width, border_color);
	} else {
		draw_solid_border(addr, width, height, line_width, border_color);
	}

	return 0;
}

static int set_stream_qp_matrix(int stream, int area)
{
	u32 * addr = NULL;
	iav_qp_roi_matrix_ex_t matrix;
	iav_encode_format_ex_t format;
	qp_region_t *qp_area = &qp_region[stream].region[area];
	u32 width, buf_pitch, height, total_size, total_bytes;
	int i, j, start_x_mb, end_x_mb, start_y_mb, end_y_mb;
	int k;
	memset(&matrix, 0, sizeof(matrix));
	matrix.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_QP_ROI_MATRIX_EX, &matrix);
	matrix.enable = 1;
	for (k = 0; k < QP_FRAME_TYPE_NUM; k++) {
		matrix.delta[k][MB_IN_ROI] = qp_area->qp_delta_in;
		matrix.delta[k][MB_OUT_ROI] = qp_area->qp_delta_out;
	}

	memset(&format, 0, sizeof(format));
	format.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
	if (format.encode_type == IAV_ENCODE_MJPEG) {
		printf("=== Only H.264 support ROI encoding!\n");
		return 0;
	}
	width = ROUND_UP(format.encode_width, 16) / 16;
	buf_pitch = ROUND_UP(width, 8);
	height = ROUND_UP(format.encode_height, 16) / 16;
	total_size = buf_pitch * height;	// (((width * 4) * height) + 31) & (~31);
	total_bytes = total_size * sizeof(u32);

	if (total_bytes > stream_qp_matrix_size) {
		printf("Too large box.");
		return -1;
	}
	addr = (u32 *)(qp_matrix_addr + stream * stream_qp_matrix_size);

	memset(addr, 0, total_bytes);

	start_x_mb = qp_area->offset_x / 16;
	end_x_mb = start_x_mb + qp_area->width / 16;
	start_y_mb = qp_area->offset_y / 16;
	end_y_mb = start_y_mb + qp_area->height / 16;
	for (j = start_y_mb; j < end_y_mb; ++j) {
		for (i = start_x_mb; i < end_x_mb; ++i) {
			addr[j * buf_pitch + i] = MB_IN_ROI;
		}
	}

	printf("set stream %c qp matrix: width (pitch) %d (%d), height %d, "
			"total %d (%d bytes).\n", 'A' + (stream - 0), width, buf_pitch,
			height, total_size, total_bytes);

	if (verbose) {
		for (j = 0; j < height; ++j) {
			printf("\n");
			for (i = 0; i < width; ++i) {
				printf("%d ", addr[j * buf_pitch + i]);
			}
		}
		printf("\n\n");
	}
	AM_IOCTL(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &matrix);

	return 0;
}

static int set_roi_qp_overlay(void)
{
	int stream, area, inactive;
	stream_qp_region_t * regions = NULL;

	for (stream = 0; stream < MAX_ENCODE_STREAM_NUM; ++stream) {
		regions = &qp_region[stream];
		for (area = 0; area < regions->total_active_regions; ++area) {
			inactive = 1;
			if (area == regions->active_region) {
				if (set_stream_qp_matrix(stream, area))
					return -1;
				inactive = 0;
			}
			draw_box_border(overlay[stream].area[area].data,
				&regions->region[area], inactive);
			if (verbose) {
				printf("== [Done] Draw QP overlay box of stream %d area %d.\n",
					stream, area);
			}
		}
		regions->active_region++;
		if (regions->active_region == regions->total_active_regions)
			regions->active_region = 0;
		if (regions->total_active_regions) {
			AM_IOCTL(fd_iav, IAV_IOC_OVERLAY_INSERT_EX, &overlay[stream]);
			printf("== Stream [%d] wait [%d] seconds for entering ROI [%d]!\n",
				stream, loop_time, regions->active_region);
		}
	}

	return 0;
}

static int clear_overlay_data(void)
{
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (overlay[i].enable == ROI_BOX_ENABLE) {
			overlay[i].enable = ROI_BOX_DISABLE;
			overlay[i].id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_OVERLAY_INSERT_EX, &overlay[i]);
		}
	}
	printf("\n Exit program !\n");
	return 0;
}

static int do_roi_loop(void)
{
	if (prepare_overlay_data() < 0)
		return -1;

	while (!quit_flag) {
		if (set_roi_qp_overlay() < 0)
			return -1;
		sleep(loop_time);
	}

	clear_overlay_data();
	return 0;
}

static void sigstop()
{
	quit_flag = 1;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

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

	if (map_qp_overlay() < 0)
		return -1;

	do_roi_loop();

	close(fd_iav);
	return 0;
}

