/********************************************************************
 * test_warp.c
 *
 * History:
 *	2012/02/09 - [Jian Tang] created file
 *	2012/03/23 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
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

#include "mw_struct.h"
#include "mw_api.h"
#include "test_warp.h"


#define NO_ARG		0
#define HAS_ARG		1
#define WARP_OPTIONS_BASE		0
#define MAX_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)

enum numeric_short_options {
	SHOW_WARP_INFO = WARP_OPTIONS_BASE,
	DPTZ_INPUT_SIZE,
	DPTZ_INPUT_OFFSET,
	DPTZ_OUTPUT_SIZE,
	DPTZ_OUTPUT_OFFSET,
	STREAM_ID,
	STREAM_OFFSET,
};

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef VERIFY_STREAMID
#define VERIFY_STREAMID(x)		do {		\
			if (((x) < 0) || ((x) >= MAX_STREAM_NUM)) {	\
				printf("Stream id wrong %d.\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_WARP_DPTZ_BUFFER_ID
#define VERIFY_WARP_DPTZ_BUFFER_ID(x)		do {		\
			if ((x) != IAV_ENCODE_SOURCE_SECOND_BUFFER &&	\
				(x) != IAV_ENCODE_SOURCE_FOURTH_BUFFER) {	\
				printf("Warp DPTZ area buffer id [%d] is wrong! Please select from [1|3].\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_NUM_WARP_AREAS)) {	\
				printf("Warp area id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_GRID_TYPE
#define VERIFY_GRID_TYPE(x)		do {		\
			if (((x) < MAP_GRID_TYPE_FIRST) || ((x) >= MAP_GRID_TYPE_LAST)) {	\
				printf("Wrong grid type [%d]!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_opts = "a:b:cf:F:g:G:i:k:l:m:o:p:P:r:R:s:S:vt:";

static struct option long_opts[] = {
	{"method",	HAS_ARG, 0, 'm'},
	{"area",		HAS_ARG, 0, 'a'},
	{"input",		HAS_ARG, 0, 'i'},
	{"output",	HAS_ARG, 0, 'o'},
	{"in-offset",	HAS_ARG, 0, 's'},
	{"out-offset",	HAS_ARG, 0, 'S'},
	{"src-rid",	HAS_ARG, 0, 'R'},
	{"rotate",		HAS_ARG, 0, 'r'},
	{"hor-grid",	HAS_ARG, 0, 'g'},
	{"hor-file",	HAS_ARG, 0, 'f'},
	{"ver-grid",	HAS_ARG, 0, 'G'},
	{"ver-file",	HAS_ARG, 0, 'F'},
	{"hor-space",	HAS_ARG, 0, 'p'},
	{"ver-space",	HAS_ARG, 0, 'P'},
	{"load",		HAS_ARG, 0, 'l'},
	{"clear",		NO_ARG, 0, 'c'},

	{"buffer",		HAS_ARG, 0, 'b'},
	{"din",	HAS_ARG, 0, DPTZ_INPUT_SIZE},
	{"din-offset",	HAS_ARG, 0, DPTZ_INPUT_OFFSET},
	{"dout",	HAS_ARG, 0, DPTZ_OUTPUT_SIZE},
	{"dout-offset",	HAS_ARG, 0, DPTZ_OUTPUT_OFFSET},
	{"keep-dptz",		HAS_ARG, 0, 'k'},
	{"show-info",	NO_ARG, 0, SHOW_WARP_INFO},
	{"verbose",	NO_ARG, 0, 'v'},
	{"auto-test",	HAS_ARG, 0, 't'},

	{"stm-id",	HAS_ARG, 0, STREAM_ID},
	{"stm-offset",	HAS_ARG, 0, STREAM_OFFSET},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0|1", "\tFisheye warp method. 0: Default warp control API. 1: New warp process API."},
	{"0~7", "\t\tspecify warp area number"},
	{"", "\t\tspecify input size, default is main source buffer size"},
	{"", "\t\tspecify output size, default is main source buffer size"},
	{"", "\t\tspecify input offset, default is 0x0"},
	{"", "\t\tspecify output offset, default is 0x0"},
	{"0~7", "\tspecify input source region id for warp copy regions"},
	{"", "\t\tspecify rotate option, 0: non-rotate, 1: 90, 2: 180, 3: 270\n"},

	{"", "\t\tspecify horizontal map grid size, the maximum size is 32x48"},
	{"file", "\tspecify horizontal map file, must have 32x48 parameters\n"},

	{"", "\t\tspecify vertical map grid size, the maximum size is 32x48"},
	{"file", "\tspecify vertical map file, must have 32x48 parameters\n"},

	{"16|32|64|128", "specify horizontal grid spacing in pixels, for both horizontal / vertical map"},
	{"16|32|64|128", "specify vertical grid spacing in pixels, for both horizontal / vertical map\n"},

	{"cfg-file", "\tload warp configuration from cfg file"},
	{"", "\t\tclear all warp effect\n"},

	{"1|3", "\tset buffer id for the DPTZ area, 1 for 2nd buffer, 3 for 4th buffer"},
	{"w x h", "\tspecify DPTZ area input size, default is the same area of main buffer"},
	{"X x Y", "\tspecify DPTZ area input offset, it's in the same area dimension of main buffer"},
	{"w x h", "\tspecify DPTZ area output size, default is the same ratio in 2nd / 4th buffer"},
	{"X x Y", "specify DPTZ area output offset, it's in the 2nd / 4th buffer dimension"},
	{"0|1", "\tflag to keep previous DPTZ layout on 2nd / 4th buffer. Default (0) is to let 2nd / 4th buffer have same layout as main buffer"},
	{"", "\t\tshow warp DPTZ area info"},
	{"", "\t\tprint more messages"},
	{"1~256", "\tauto test, it's the total steps from full size to 1/8 size\n"},
	{"0~7", "\tspecify the stream id to change the offset"},
	{"X x Y", "\tspecify the stream offset"},
};


static int fd_iav = -1;
static char cfg_file_name[FILENAME_LENGTH];
static int load_cfg_file_flag = 0;
static int clear_flag = 0;
static int keep_dptz_flag = 0;
static int keep_dptz[IAV_ENCODE_SOURCE_TOTAL_NUM] = { 0 };
static int show_info_flag = 0;
static int warp_method = 0;
static int verbose = 0;
static int auto_test_flag = 0;
static int auto_test_step = 1;
static char irq_proc_name[256] = "/proc/ambarella/vin0_vsync";
static int irq_fd = -1;

static int current_stream = -1;
static iav_stream_offset_t stm_offset[MAX_STREAM_NUM];
static int stm_offset_changed_id = 0;

static int warp_region_changed_id = 0;
static int warp_region_dptz_changed_id = 0;

// first and second values must be in format of "AxB"
static int get_two_int(const char *name, int *first, int *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator-name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atoi(tmp_string);

	return 0;
}

static int get_multi_s16_args(char *optarg, s16 *argarr, int argcnt)
{
	int i;
	char *delim = ", \n\t";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; ++i) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt) {
		printf("It's expected to have [%d] params, only get [%d].\n",
			argcnt, i);
		return -1;
	}
	return 0;
}

static int get_warp_table_from_file(const char * file, s16 * table_addr)
{
	#define	MAX_FILE_LENGTH	(512 * MAX_GRID_HEIGHT)
	int fd, retv = 0;
	char line[MAX_FILE_LENGTH];

	if ((fd = open(file, O_RDONLY, 0)) < 0) {
		printf("### Cannot open file [%s].\n", file);
		return -1;
	}

	do {
		memset(line, 0, sizeof(line));
		read(fd, line, MAX_FILE_LENGTH);
		if (get_multi_s16_args(line, table_addr, MAX_WARP_TABLE_SIZE) < 0) {
			printf("!!! Failed to get warp table from file [%s]!\n", file);
			retv = -1;
			break;
		}
		printf(">>> Succeed to load warp table from file [%s].\n", file);
	} while (0);

	if (fd > 0)
		close(fd);
	return retv;
}

static int get_rotate_option(const int rotate)
{
	int rotate_option = 0;
	switch (rotate) {
	case 0:
		rotate_option = NO_ROTATE_FLIP;
		break;
	case 1:
		rotate_option = CW_ROTATE_90;
		break;
	case 2:
		rotate_option = CW_ROTATE_180;
		break;
	case 3:
		rotate_option = CW_ROTATE_270;
		break;
	default:
		printf("NOT supported rotate flag [%d]. Use 'no rotate' option!\n",
			rotate);
		rotate_option = NO_ROTATE_FLIP;
		break;
	}
	return rotate_option;
}

static int get_grid_spacing(const int spacing)
{
	int grid_space = GRID_SPACING_PIXEL_64;
	switch (spacing) {
	case 16:
		grid_space = GRID_SPACING_PIXEL_16;
		break;
	case 32:
		grid_space = GRID_SPACING_PIXEL_32;
		break;
	case 64:
		grid_space = GRID_SPACING_PIXEL_64;
		break;
	case 128:
		grid_space = GRID_SPACING_PIXEL_128;
		break;
	default:
		printf("NOT supported spacing [%d]. Use 'spacing of 64 pixels'!\n",
			spacing);
		grid_space = GRID_SPACING_PIXEL_64;
		break;
	}
	return grid_space;
}

static void usage(void)
{
	u32 i;
	char itself[16] = "test_warp";

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
	printf("\n");
	printf("Examples:\n");
	printf("  Use warp to do zoom and multi regions:\n");
	printf("    %s -a 0 -i 960x540 -s 0x0 -o 1920x1080 -S 0x0\n\n", itself);
	printf("  Load warp configuration from config file:\n");
	printf("    %s -l ./warp.cfg\n\n", itself);
	printf("  Show warp area DPTZ info:\n");
	printf("    %s --show-info\n\n", itself);
	printf("  Change post-main layout after show info:\n");
	printf("    %s -a 3 -S 1280x720\n\n", itself);
	printf("  Set warp area DPTZ in 4th buffer:\n");
	printf("    %s -b 3 -a 0 --din 720x480 --din-offset 0x0 --dout 480x480 --dout-offset 200x0\n\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch, value, first, second;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index)) != -1) {
		switch (ch) {
		case 'm':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Invalid value [%d] for warp method [0|1].\n", value);
				return -1;
			}
			warp_method = value;
			break;
		case 'a':
			value = atoi(optarg);
			VERIFY_AREAID(value);
			current_area = value;
			break;
		case 'i':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region[current_area].input_width = first;
			warp_region[current_area].input_height = second;
			warp_region[current_area].input_size_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'o':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region[current_area].output_width = first;
			warp_region[current_area].output_height = second;
			warp_region[current_area].output_size_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 's':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region[current_area].input_offset_x = first;
			warp_region[current_area].input_offset_y = second;
			warp_region[current_area].input_offset_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'S':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region[current_area].output_offset_x = first;
			warp_region[current_area].output_offset_y = second;
			warp_region[current_area].output_offset_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'R':
			VERIFY_AREAID(current_area);
			value = atoi(optarg);
			VERIFY_AREAID(value);
			warp_region[current_area].input_src_rid = value;
			warp_region[current_area].input_src_rid_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'r':
			VERIFY_AREAID(current_area);
			value = atoi(optarg);
			warp_region[current_area].rotate = get_rotate_option(value);
			warp_region[current_area].rotate_flag = 1;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'g':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			if ((first > MAX_GRID_WIDTH) || (second > MAX_GRID_HEIGHT)) {
				printf("Invalid grid size %dx%d, it's larger than 32x48.\n",
					first, second);
				return -1;
			}
			warp_region[current_area].hor_grid_col = first;
			warp_region[current_area].hor_grid_row = second;
			warp_region[current_area].hor_grid_size_flag = 1;
			grid_type = MAP_GRID_HORIZONTAL;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'f':
			VERIFY_AREAID(current_area);
			value = strlen(optarg);
			if (value >= FILENAME_LENGTH) {
				printf("Filename [%s] is too long [%d], please keep it less than %d.\n",
					optarg, value, FILENAME_LENGTH);
				return -1;
			}
			strncpy(warp_region[current_area].hor_file, optarg, value);
			warp_region[current_area].hor_file_flag = 1;
			break;
		case 'G':
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			if ((first > MAX_GRID_WIDTH) || (second > MAX_GRID_HEIGHT)) {
				printf("Invalid grid size %dx%d, it's larger than 32x48.\n",
					first, second);
				return -1;
			}
			warp_region[current_area].ver_grid_col = first;
			warp_region[current_area].ver_grid_row = second;
			warp_region[current_area].ver_grid_size_flag = 1;
			grid_type = MAP_GRID_VERTICAL;
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'F':
			VERIFY_AREAID(current_area);
			value = strlen(optarg);
			if (value >= FILENAME_LENGTH) {
				printf("Filename [%s] is too long [%d], please keep it less than %d.\n",
					optarg, value, FILENAME_LENGTH);
				return -1;
			}
			strncpy(warp_region[current_area].ver_file, optarg, value);
			warp_region[current_area].ver_file_flag = 1;
			break;
		case 'p':
			VERIFY_AREAID(current_area);
			VERIFY_GRID_TYPE(grid_type);
			value = atoi(optarg);
			if (grid_type == MAP_GRID_HORIZONTAL) {
				warp_region[current_area].hor_hor_grid_spacing = value;
				warp_region[current_area].hor_hor_grid_flag = 1;
			} else {
				warp_region[current_area].ver_hor_grid_spacing = value;
				warp_region[current_area].ver_hor_grid_flag = 1;
			}
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'P':
			VERIFY_AREAID(current_area);
			value = atoi(optarg);
			if (grid_type == MAP_GRID_HORIZONTAL) {
				warp_region[current_area].hor_ver_grid_spacing = value;
				warp_region[current_area].hor_ver_grid_flag = 1;
			} else {
				warp_region[current_area].ver_ver_grid_spacing = value;
				warp_region[current_area].ver_ver_grid_flag = 1;
			}
			warp_region_changed_id |= (1 << current_area);
			break;
		case 'l':
			sprintf(cfg_file_name, "%s", optarg);
			load_cfg_file_flag = 1;
			break;
		case 'c':
			clear_flag = 1;
			break;
		case 'b':
			value = atoi(optarg);
			VERIFY_WARP_DPTZ_BUFFER_ID(value);
			current_buffer = value;
			break;
		case DPTZ_INPUT_SIZE:
			VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region_dptz[current_area].input_w = first;
			warp_region_dptz[current_area].input_h = second;
			warp_region_dptz[current_area].input_size_flag = 1;
			warp_region_dptz_changed_id |= (1 << current_area);
			break;
		case DPTZ_INPUT_OFFSET:
			VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region_dptz[current_area].input_x = first;
			warp_region_dptz[current_area].input_y = second;
			warp_region_dptz[current_area].input_offset_flag = 1;
			warp_region_dptz_changed_id |= (1 << current_area);
			break;
		case DPTZ_OUTPUT_SIZE:
			VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region_dptz[current_area].output_w = first;
			warp_region_dptz[current_area].output_h = second;
			warp_region_dptz[current_area].output_size_flag = 1;
			warp_region_dptz_changed_id |= (1 << current_area);
			break;
		case DPTZ_OUTPUT_OFFSET:
			VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
			VERIFY_AREAID(current_area);
			if (get_two_int(optarg, &first, &second, 'x') < 0)
				return -1;
			warp_region_dptz[current_area].output_x = first;
			warp_region_dptz[current_area].output_y = second;
			warp_region_dptz[current_area].output_offset_flag = 1;
			warp_region_dptz_changed_id |= (1 << current_area);
			break;
		case 'k':
			VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
			first = atoi(optarg);
			if (first < 0 || first > 1) {
				printf("Invalid value [%d] for 'keep-flag' option [0|1].\n", first);
				return -1;
			}
			keep_dptz[current_buffer] = first;
			keep_dptz_flag = 1;
			break;
		case SHOW_WARP_INFO:
			show_info_flag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			auto_test_flag = 1;
			value = atoi(optarg);
			if (value < 1 || value > 256) {
				printf("Invalid value [%d] for 'auto-test' option [1~256].\n", value);
				return -1;
			}
			auto_test_step = value;
			break;
		case STREAM_ID:
			value = atoi(optarg);
			VERIFY_STREAMID(value);
			current_stream = value;
			break;
		case STREAM_OFFSET:
			VERIFY_STREAMID(current_stream);
			if (get_two_int(optarg, &first, &second, 'x') < 0) {
				return -1;
			}
			stm_offset[current_stream].id = (1 << current_stream);
			stm_offset[current_stream].x = first;
			stm_offset[current_stream].y = second;
			stm_offset_changed_id |= (1 << current_stream);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

static int load_config_file(void)
{
	#define	FILE_CONTENT_SIZE		(10240)
	char string[FILE_CONTENT_SIZE];
	int cfg_fd = 0;

	if ((cfg_fd = open(cfg_file_name, O_RDONLY, 0)) < 0) {
		printf("Cannot open config file [%s].\n", cfg_file_name);
		return -1;
	}
	read(cfg_fd, string, FILE_CONTENT_SIZE);
	close(cfg_fd);
	if (mw_config_params(WarpMap, (char *)string) < 0) {
		printf("Failed config warp params!\n");
		return -1;
	}
	printf(">>> Succeed to load config file [%s]!\n", cfg_file_name);

	return 0;
}

static int update_area_transform(int area_num,
	iav_warp_vector_ex_t *area, int *act)
{
	warp_region_t * region = &warp_region[area_num];
	if (region->input_size_flag) {
		area->input.width = region->input_width;
		area->input.height = region->input_height;
		*act = 1;
	}

	if (region->input_offset_flag) {
		area->input.x = region->input_offset_x;
		area->input.y = region->input_offset_y;
		*act = 1;
	}

	if (region->output_size_flag) {
		area->output.width = region->output_width;
		area->output.height = region->output_height;
		*act = 1;
	}

	if (region->rotate_flag) {
		area->rotate_flip = region->rotate;
		*act = 1;
	}

	if (region->hor_grid_size_flag) {
		area->hor_map.enable = 1;
		area->hor_map.output_grid_col = region->hor_grid_col;
		area->hor_map.output_grid_row = region->hor_grid_row;
		if (get_warp_table_from_file(region->hor_file,
			wt_hor[area_num]) < 0) {
			printf("Invalid horizontal warp file [%s].\n", region->hor_file);
			return -1;
		}
	}
	if (region->hor_hor_grid_flag) {
		area->hor_map.horizontal_spacing =
			get_grid_spacing(region->hor_hor_grid_spacing);
	} else {
		// use default 64 pixels
		area->hor_map.horizontal_spacing = GRID_SPACING_PIXEL_64;
	}
	if (region->hor_ver_grid_flag) {
		area->hor_map.vertical_spacing =
			get_grid_spacing(region->hor_ver_grid_spacing);
	} else {
		// use default 32 pixels
		area->hor_map.vertical_spacing = GRID_SPACING_PIXEL_32;
	}

	if (region->ver_grid_size_flag) {
		area->ver_map.enable = 1;
		area->ver_map.output_grid_col = region->ver_grid_col;
		area->ver_map.output_grid_row = region->ver_grid_row;
		if (get_warp_table_from_file(region->ver_file,
			wt_ver[area_num]) < 0) {
			printf("Invalid vertical warp file [%s].\n", region->ver_file);
			return -1;
		}
	}
	if (region->ver_hor_grid_flag) {
		area->ver_map.horizontal_spacing =
			get_grid_spacing(region->ver_hor_grid_spacing);
	} else {
		// use default 64 pixels
		area->ver_map.horizontal_spacing = GRID_SPACING_PIXEL_64;
	}
	if (region->ver_ver_grid_flag) {
		area->ver_map.vertical_spacing =
			get_grid_spacing(region->ver_ver_grid_spacing);
	} else {
		// use default 32 pixels
		area->ver_map.vertical_spacing = GRID_SPACING_PIXEL_32;
	}

	return 0;
}

static int update_area_param(int area_num,
	iav_warp_vector_ex_t * area, int * act)
{
	int active = 0;
	warp_region_t * region = &warp_region[area_num];

	if (region->output_offset_flag) {
		area->output.x = region->output_offset_x;
		area->output.y = region->output_offset_y;
		active = 1;
	}

	if (area->dup) {
		if (region->input_src_rid_flag) {
			area->src_rid = region->input_src_rid;
			active = 1;
		}
	} else {
		update_area_transform(area_num, area, &active);
	}

	if (active) {
		area->enable = 1;
	}
	if (act) {
		*act = active;
	}

	if (verbose) {
		printf("Update RID [%d], enable [%d], active [%d].\n",
			area_num, area->enable, active);
	}

	return 0;
}

static int check_area_resolution(iav_source_buffer_setup_ex_t *setup,
	iav_warp_vector_ex_t * area)
{
	iav_reso_ex_t size;

	size = area->dup ? setup->size[0] : setup->pre_main;
	if ((area->input.x + area->input.width > size.width) ||
		(area->input.y + area->input.height > size.height)) {
		printf("input size %dx%d + offset %dx%d is out of main %dx%d.\n",
			area->input.width, area->input.height, area->input.x,
			area->input.y, size.width, size.height);
		return -1;
	}

	size = setup->size[0];
	if ((area->output.x + area->output.width > size.width) ||
		(area->output.y + area->output.height > size.height)) {
		printf("output size %dx%d + offset %dx%d is out of main %dx%d.\n",
			area->output.width, area->output.height, area->output.x,
			area->output.y, setup->size[0].width, setup->size[0].height);
		return -1;
	}

	return 0;
}

static int show_warp_info(void)
{
	int i;
	iav_warp_map_ex_t * map = NULL;
	iav_warp_vector_ex_t * area = NULL;
	iav_region_dptz_ex_t * dptz = NULL;
	iav_warp_control_ex_t warp;
	iav_warp_dptz_ex_t prev_c, prev_a;

	memset(&warp, 0, sizeof(warp));
	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CONTROL_EX, &warp);
	memset(&prev_a, 0, sizeof(prev_a));
	prev_a.buffer_id = IAV_ENCODE_SOURCE_FOURTH_BUFFER;
	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_AREA_DPTZ_EX, &prev_a);
	memset(&prev_c, 0, sizeof(prev_c));
	prev_c.buffer_id = IAV_ENCODE_SOURCE_SECOND_BUFFER;
	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_AREA_DPTZ_EX, &prev_c);

	printf("============ Warp control parameters ============\n");

	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		area = &warp.area[i];
		printf("\n+++++++ Area %d  - Enable [%d]", i, area->enable);
		if (area->dup) {
			printf(" Copy [SRC region :%d] +++++++\n", area->src_rid);
		} else {
			printf(" Transform +++++++\n");
		}
		printf("  Input:  %4dx%4d, offset %4dx%4d\n",
			area->input.width, area->input.height, area->input.x, area->input.y);
		printf("  Output: %4dx%4d, offset %4dx%4d\n",
			area->output.width, area->output.height, area->output.x, area->output.y);
		printf("  Rotate / flip = %d\n", area->rotate_flip);
		map = &area->hor_map;
		printf("  Horizontal warp vector map: enable = %d\n", map->enable);
		printf("    output grid = %2dx%2d\n",
			map->output_grid_col, map->output_grid_row);
		printf("    grid spacing = %3d [H] / %3d [V]\n",
			(1 << (4 + map->horizontal_spacing)),
			(1 << (4 + map->vertical_spacing)));
		map = &area->ver_map;
		printf("  Vertical warp vector map: enable = %d\n", map->enable);
		printf("    output grid = %2dx%2d\n",
			map->output_grid_col, map->output_grid_row);
		printf("    grid spacing = %3d [H] / %3d [V]\n",
			(1 << (4 + map->horizontal_spacing)),
			(1 << (4 + map->vertical_spacing)));

		dptz = &prev_a.dptz[i];
		printf("  Prev A keep DPTZ: %d.\n", warp.keep_dptz[prev_a.buffer_id]);
		printf("  Prev A input:  %4dx%4d, offset %4dx%4d\n",
			dptz->input.width, dptz->input.height, dptz->input.x, dptz->input.y);
		printf("  Prev A output: %4dx%4d, offset %4dx%4d\n",
			dptz->output.width, dptz->output.height, dptz->output.x, dptz->output.y);

		dptz = &prev_c.dptz[i];
		printf("  Prev C keep DPTZ: %d.\n", warp.keep_dptz[prev_c.buffer_id]);
		printf("  Prev C input:  %4dx%4d, offset %4dx%4d\n",
			dptz->input.width, dptz->input.height, dptz->input.x, dptz->input.y);
		printf("  Prev C output: %4dx%4d, offset %4dx%4d\n",
			dptz->output.width, dptz->output.height, dptz->output.x, dptz->output.y);
	}

	return 0;
}

static int set_region_warp_control(void)
{
	int i;
	iav_warp_control_ex_t region_warp;
	iav_warp_vector_ex_t * warp_area = NULL;
	iav_source_buffer_setup_ex_t setup;

	memset(&region_warp, 0, sizeof(region_warp));
	if (!clear_flag) {
		for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
			region_warp.area[i].hor_map.addr = (u32)wt_hor[i];
			region_warp.area[i].ver_map.addr = (u32)wt_ver[i];
		}
		AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CONTROL_EX, &region_warp);
		memset(&setup, 0, sizeof(setup));
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup);
		for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
			warp_area = &region_warp.area[i];
			if (update_area_param(i, warp_area, NULL) < 0) {
				printf("### Failed to update warp parameters !\n");
				return -1;
			}
			if (check_area_resolution(&setup, warp_area) < 0) {
				printf("Invalid warp parameters of warp area [%d]!\n", i);
				return -1;
			}
		}
	}
	if (keep_dptz_flag) {
		region_warp.keep_dptz[1] = keep_dptz[1];
		region_warp.keep_dptz[3] = keep_dptz[3];
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_WARP_CONTROL_EX, &region_warp);

	return 0;
}

static int set_region_warp_proc(void)
{
	iav_warp_proc_t wp;
	iav_warp_vector_ex_t *wt = &wp.arg.transform, *area = NULL;
	iav_warp_copy_t *wc = &wp.arg.copy;
	iav_stream_offset_t *so = &wp.arg.offset;
	iav_warp_control_ex_t region_warp;
	iav_source_buffer_setup_ex_t setup;
	iav_apply_flag_t flags[IAV_WARP_PROC_NUM];
	int i, active;

	memset(&region_warp, 0, sizeof(region_warp));
	if (!clear_flag) {
		for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
			region_warp.area[i].hor_map.addr = (u32)wt_hor[i];
			region_warp.area[i].ver_map.addr = (u32)wt_ver[i];
		}
		AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CONTROL_EX, &region_warp);
		memset(&setup, 0, sizeof(setup));
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup);

		for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
			area = &region_warp.area[i];
			if (update_area_param(i, area, &active) < 0) {
				printf("### Failed to update warp parameters!\n");
				return -1;
			}
			if (check_area_resolution(&setup, area) < 0) {
				printf("Invalid warp parameters of warp area [%d].\n", i);
				return -1;
			}
			if (active) {
				memset(&wp, 0, sizeof(wp));
				wp.rid = i;
				if (area->dup) {
					wp.wid = IAV_WARP_COPY;
					wc->enable = area->enable;
					wc->src_rid = area->src_rid;
					wc->dst_x = area->output.x;
					wc->dst_y = area->output.y;
					flags[IAV_WARP_COPY].apply = 1;
					flags[IAV_WARP_COPY].param |= (1 << i);
				} else {
					wp.wid = IAV_WARP_TRANSFORM;
					*wt = *area;
					flags[IAV_WARP_TRANSFORM].apply = 1;
					flags[IAV_WARP_TRANSFORM].param |= (1 << i);
				}
				AM_IOCTL(fd_iav, IAV_IOC_CFG_WARP_PROC, &wp);
			}
		}
		for (i = 0; i < MAX_STREAM_NUM; ++i) {
			if (stm_offset_changed_id & (1 << i)) {
				memset(&wp, 0, sizeof(wp));
				wp.wid = IAV_WARP_OFFSET;
				*so = stm_offset[i];
				AM_IOCTL(fd_iav, IAV_IOC_CFG_WARP_PROC, &wp);
			}
		}
		if (stm_offset_changed_id) {
			flags[IAV_WARP_OFFSET].apply = 1;
			flags[IAV_WARP_OFFSET].param = stm_offset_changed_id;
		}
		AM_IOCTL(fd_iav, IAV_IOC_APPLY_WARP_PROC, flags);
	}
	return 0;
}

static int set_region_warp(void)
{
	int retv = 0;
	switch (warp_method) {
	case FISHEYE_WARP_CONTROL:
		retv = set_region_warp_control();
		break;
	case FISHEYE_WARP_PROC:
		retv = set_region_warp_proc();
		break;
	default:
		printf("INVALID warp method [%d].\n", warp_method);
		retv = -1;
		break;
	}
	return retv;
}

static int set_warp_region_dptz(int changed_id)
{
	int area;
	iav_warp_dptz_ex_t area_dptz;
	iav_region_dptz_ex_t * dptz = NULL;

	memset(&area_dptz, 0, sizeof(area_dptz));
	area_dptz.buffer_id = current_buffer;
	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_AREA_DPTZ_EX, &area_dptz);

	for (area = 0; area < MAX_NUM_WARP_AREAS; ++area) {
		if (changed_id & (1 << area)) {
			dptz = &area_dptz.dptz[area];
			printf("Before = [B: %d, A: %d]: Input [%dx%d : %dx%d], Output"
				" [%dx%d : %dx%d].\n",
				current_buffer, area, dptz->input.width, dptz->input.height,
				dptz->input.x, dptz->input.y, dptz->output.width,
				dptz->output.height, dptz->output.x, dptz->output.y);
			if (warp_region_dptz[area].input_size_flag) {
				dptz->input.width = warp_region_dptz[area].input_w;
				dptz->input.height = warp_region_dptz[area].input_h;
			}
			if (warp_region_dptz[area].input_offset_flag) {
				dptz->input.x = warp_region_dptz[area].input_x;
				dptz->input.y = warp_region_dptz[area].input_y;
			}
			if (warp_region_dptz[area].output_size_flag) {
				dptz->output.width = warp_region_dptz[area].output_w;
				dptz->output.height = warp_region_dptz[area].output_h;
			}
			if (warp_region_dptz[area].output_offset_flag) {
				dptz->output.x = warp_region_dptz[area].output_x;
				dptz->output.y = warp_region_dptz[area].output_y;
			}
			printf("After = [B: %d, A: %d]: Input [%dx%d : %dx%d], Output"
				" [%dx%d : %dx%d].\n",
				current_buffer, area, dptz->input.width, dptz->input.height,
				dptz->input.x, dptz->input.y, dptz->output.width,
				dptz->output.height, dptz->output.x, dptz->output.y);
		}
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_WARP_AREA_DPTZ_EX, &area_dptz);

	return 0;
}

static inline void soft_vsync(void)
{
	char vin_int_array[32];

	vin_int_array[8] = 0;
	read(irq_fd, vin_int_array, 8);
	lseek(irq_fd, 0 , 0);
}

static int auto_test()
{
	iav_source_buffer_setup_ex_t setup;
	iav_warp_control_ex_t region_warp;
	iav_warp_vector_ex_t * warp_area = NULL;
	int direct_w = 0, direct_h = 0;
	memset(&region_warp, 0, sizeof(region_warp));
	warp_area = &region_warp.area[0];
	int min_input_height = 0, min_input_width = 0;

	irq_fd = open(irq_proc_name, O_RDONLY);
	if (irq_fd < 0) {
		printf("\nCan't open %s.\n", irq_proc_name);
		return -1;
	}

	memset(&setup, 0, sizeof(setup));
	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup);

	warp_region[0].input_width = setup.pre_main.width;
	warp_region[0].input_height = setup.pre_main.height;
	warp_region[0].output_width = setup.pre_main.width;
	warp_region[0].output_height= setup.pre_main.height;
	warp_region[0].input_size_flag = 1;
	warp_region[0].output_size_flag = 1;

	min_input_height = warp_region[0].input_height >> 3;
	direct_h = (warp_region[0].input_height - min_input_height) /auto_test_step;
	min_input_width = warp_region[0].input_width >> 3;
	direct_w = (warp_region[0].input_width - min_input_width) / auto_test_step;
	if (direct_h < 1)
		direct_h = 1;
	if (direct_w < 1)
		direct_w = 1;

	if (update_area_param(0, warp_area, 0) < 0) {
		printf("### Failed to update warp parameters !\n");
		return -1;
	}

	while (auto_test_flag) {
		soft_vsync();
		if ((warp_area->input.height >= warp_region[0].input_height) ||
			(warp_area->input.width >= warp_region[0].input_width)) {
			direct_w = -abs(direct_w);
			direct_h = -abs(direct_h);
		}
		if ((warp_area->input.height <= min_input_height) ||
			(warp_area->input.width <= min_input_width)) {
			direct_w = abs(direct_w);
			direct_h = abs(direct_h);
		}
		warp_area->input.width += direct_w;
		warp_area->input.height += direct_h;
		warp_area->input.x = (warp_region[0].input_width -warp_area->input.width) >> 1;
		warp_area->input.y = (warp_region[0].input_height -warp_area->input.height) >> 1;

		AM_IOCTL(fd_iav, IAV_IOC_SET_WARP_CONTROL_EX, &region_warp);
	}

	if (irq_fd >= 0) {
		if (close(irq_fd) < 0) {
			printf("close %s fail\n", irq_proc_name);
			return -1;
		}
		irq_fd = -1;
	}
	return 0;
}

static void sigstop(int a)
{
	auto_test_flag = 0;
	if (irq_fd >= 0) {
		if (close(irq_fd) < 0) {
			printf("close %s fail\n", irq_proc_name);
			return;
		}
		irq_fd = -1;
	}
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
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (load_cfg_file_flag) {
		if (load_config_file() < 0)
			return -1;
	}

	if (warp_region_changed_id || clear_flag) {
		if (set_region_warp() < 0)
			return -1;
	}

	if (warp_region_dptz_changed_id) {
		if (set_warp_region_dptz(warp_region_dptz_changed_id) < 0)
			return -1;
	}

	if (show_info_flag) {
		show_warp_info();
	}
	if (auto_test_flag) {
		auto_test();
	}
	return 0;
}


