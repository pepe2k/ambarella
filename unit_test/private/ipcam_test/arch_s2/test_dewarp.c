/********************************************************************
 * test_dewarp.c
 *
 * History:
 *	2012/02/09 - [Jian Tang] created file
 *	2012/03/23 - [Jian Tang] modified file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"

#include "lib_dewarp_header.h"
#include "lib_dewarp.h"

#define NO_ARG          0
#define HAS_ARG         1
#define MODE_STRING_LENGTH		(64)
#define FILENAME_LENGTH			(256)
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")

#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if ((x < 0) || (x >= MAX_NUM_WARP_AREAS)) {	\
				printf("Warp area id [%d] is wrong!\n", x);	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_GRID_TYPE
#define VERIFY_GRID_TYPE(x)		do {		\
			if ((x < MAP_GRID_TYPE_FIRST) || (x >= MAP_GRID_TYPE_LAST)) {	\
				printf("Wrong grid type [%d]!\n", x);	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_WARP_DPTZ_BUFFER_ID
#define VERIFY_WARP_DPTZ_BUFFER_ID(x)		do {		\
			if ((x) != IAV_ENCODE_SOURCE_SECOND_BUFFER &&	\
				(x) != IAV_ENCODE_SOURCE_FOURTH_BUFFER) {	\
				printf("Keep DPTZ area buffer id [%d] is wrong! Please select from [1|3].\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

enum {
	MOUNT_WALL = 0,
	MOUNT_CEILING,
	MOUNT_DESKTOP,
};

enum {
	FISHEYE_WARP_CONTROL = 0,
	FISHEYE_WARP_PROC = 1,
};

enum {
	WARP_NO_TRANSFORM = 0,
	WARP_NORMAL = 1,
	WARP_PANOR = 2,
	WARP_SUBREGION = 3,
	WARP_COPY = 4,
	WARP_MODE_TOTAL_NUM,

	MAX_FISHEYE_REGION_NUM = MAX_NUM_WARP_AREAS,
};

typedef struct point_mapping_s {
	fpoint_t region;
	fpoint_t fisheye;
	pantilt_angle_t angle;
} point_mapping_t;

typedef struct warp_copy_s {
	int	src;
	int	src_flag;
	u16	dst_x;
	u16	dst_y;
	int	off_flag;
} warp_copy_t;

static int fd_iav = -1;
static int current_buffer = -1;
static int update_flag = 0;
static int clear_flag = 0;
static int verbose = 0;
static int log_level = LOG_INFO;
static int current_region = -1;
static int file_flag = 0;
static int keep_dptz_flag = 0;
static int keep_dptz[IAV_ENCODE_SOURCE_TOTAL_NUM] = { 0 };
static char file[FILENAME_LENGTH] = "vector";

static int fisheye_warp_method = 0;

static fisheye_config_t fisheye_config_param = { 0 };
static int fisheye_config_flag = 0;

static dewarp_init_t dewarp_init_param;
static int fisheye_mount;

static s16 ver_map_addr[MAX_NUM_WARP_AREAS][MAX_WARP_TABLE_SIZE];
static s16 hor_map_addr[MAX_NUM_WARP_AREAS][MAX_WARP_TABLE_SIZE];

static int fisheye_mode[MAX_FISHEYE_REGION_NUM];
static int fisheye_mode_flag = 0;

static warp_vector_t fisheye_vector[MAX_NUM_WARP_AREAS];
static warp_region_t fisheye_region[MAX_FISHEYE_REGION_NUM];

// No Transform
rect_in_main_t notrans[MAX_FISHEYE_REGION_NUM];

// [Ceiling / Desktop] Panorama
static degree_t hor_panor_range[MAX_FISHEYE_REGION_NUM];

// [Ceiling / Desktop] Normal / Panorama
static degree_t edge_angle[MAX_FISHEYE_REGION_NUM];
static int orient[MAX_FISHEYE_REGION_NUM];

// [Wall / Ceiling / Desktop] Subregion
static pantilt_angle_t pantilt[MAX_FISHEYE_REGION_NUM];
static int pantilt_flag[MAX_FISHEYE_REGION_NUM];
static fpoint_t roi[MAX_FISHEYE_REGION_NUM];
static point_mapping_t point_mapping_output[MAX_FISHEYE_REGION_NUM];
static int point_mapping_output_flag[MAX_FISHEYE_REGION_NUM];
static point_mapping_t point_mapping_fisheye[MAX_FISHEYE_REGION_NUM];
static int point_mapping_fisheye_flag[MAX_FISHEYE_REGION_NUM];

// Warp Copy
static warp_copy_t warp_copy[MAX_FISHEYE_REGION_NUM];

iav_source_buffer_setup_ex_t buffer_setup;

struct hint_s {
	const char *arg;
	const char *str;
};

enum numeric_short_options {
	SPECIFY_NOTRANS_OFFSET = 10,
	SPECIFY_NOTRANS_SIZE,
	SPECIFY_LUT_EFL,
	SPECIFY_LUT_FILE,
	SPECIFY_MAX_WALLNOR_AREAS,
	SPECIFY_MAX_WALLPANOR_AREAS,
	SPECIFY_MAX_CEILNOR_AREAS,
	SPECIFY_MAX_CEILSUB_AREAS,
	SPECIFY_MAX_CEILPANOR_AREAS,
	SPECIFY_MAX_DESKNOR_AREAS,
	SPECIFY_MAX_DESKSUB_AREAS,
	SPECIFY_MAX_DESKPANOR_AREAS,
	SPECIFY_POINT_MAPPING_OUTPUT,
	SPECIFY_POINT_MAPPING_FISHEYE,
	SPECIFY_WARP_COPY_SOURCE,
	SPECIFY_WARP_COPY_OFFSET,
	SPECIFY_WARP_METHOD,
	SPECIFY_WARP_UPDATE,
};

static const char *short_opts = "M:R:F:L:C:a:m:s:o:z:h:p:t:r:G:NSWEcuf:b:k:vl:";

static struct option long_opts[] = {
	{ "mount", HAS_ARG, 0, 'M' },
	{ "max-radius", HAS_ARG, 0, 'R' },
	{ "max-fov", HAS_ARG, 0, 'F' },
	{ "lens", HAS_ARG, 0, 'L' },
	{ "center", HAS_ARG, 0, 'C' },
	{ "area", HAS_ARG, 0, 'a' },
	{ "mode", HAS_ARG, 0, 'm' },
	{ "output-size", HAS_ARG, 0, 's' },
	{ "output-offset", HAS_ARG, 0, 'o' },
	{ "zoom", HAS_ARG, 0, 'z' },
	{ "hor-range", HAS_ARG, 0, 'h' },
	{ "pan", HAS_ARG, 0, 'p' },
	{ "tilt", HAS_ARG, 0, 't' },
	{ "roi", HAS_ARG, 0, 'r' },
	{ "no", HAS_ARG, 0, SPECIFY_NOTRANS_OFFSET },
	{ "ns", HAS_ARG, 0, SPECIFY_NOTRANS_SIZE },
	{ "wc-src", HAS_ARG, 0, SPECIFY_WARP_COPY_SOURCE },
	{ "wc-offset", HAS_ARG, 0, SPECIFY_WARP_COPY_OFFSET },
	{ "output-point", HAS_ARG, 0, SPECIFY_POINT_MAPPING_OUTPUT },
	{ "fish-point", HAS_ARG, 0, SPECIFY_POINT_MAPPING_FISHEYE },
	{ "edge-angle", HAS_ARG, 0, 'G' },
	{ "north", NO_ARG, 0, 'N' },
	{ "south", NO_ARG, 0, 'S' },
	{ "west", NO_ARG, 0, 'W' },
	{ "east", NO_ARG, 0, 'E' },
	{ "clear", NO_ARG, 0, 'c' },
	{ "lut-efl", HAS_ARG, 0, SPECIFY_LUT_EFL },
	{ "lut-file", HAS_ARG, 0, SPECIFY_LUT_FILE },
	{ "max-wallnor-areas", HAS_ARG, 0, SPECIFY_MAX_WALLNOR_AREAS },
	{ "max-wallpanor-areas", HAS_ARG, 0, SPECIFY_MAX_WALLPANOR_AREAS },
	{ "max-ceilnor-areas", HAS_ARG, 0, SPECIFY_MAX_CEILNOR_AREAS },
	{ "max-ceilpanor-areas", HAS_ARG, 0, SPECIFY_MAX_CEILPANOR_AREAS },
	{ "max-ceilsub-areas", HAS_ARG, 0, SPECIFY_MAX_CEILSUB_AREAS },
	{ "max-desknor-areas", HAS_ARG, 0, SPECIFY_MAX_DESKNOR_AREAS },
	{ "max-deskpanor-areas", HAS_ARG, 0, SPECIFY_MAX_DESKPANOR_AREAS },
	{ "max-desksub-areas", HAS_ARG, 0, SPECIFY_MAX_DESKSUB_AREAS },
	{ "wm", HAS_ARG, 0, SPECIFY_WARP_METHOD },
	{ "update", NO_ARG, 0, 'u' },
	{ "file", HAS_ARG, 0, 'f' },
	{ "buffer", HAS_ARG, 0, 'b' },
	{ "keep-dptz", HAS_ARG, 0, 'k' },
	{ "verbose", NO_ARG, 0, 'v' },
	{ "level", HAS_ARG, 0, 'l' },
	{ 0, 0, 0, 0 }
};

static const struct hint_s hint[] =
{
	{ "0~2", "\tLens mount. 0 (default): wall, 1: ceiling, 2: desktop" },
	{ "", "\t\tFull FOV circle radius (pixel) in pre main buffer" },
	{ "0~360", "\tLens full FOV in degree" },
	{ "0|1|2", "\tLens projection mode. 0: equidistant (Linear scaled, r = f * theta), 1: Stereographic (conform, r = 2 * f * tan(theta/2), 2: Look up table for r and theta" },
	{ "axb", "\tLens circle center in pre main." },
	{ "0~7", "\t\tFisheye correction region number" },
	{ "0~4", "\t\t0: No transform, 1: Normal, 2: Panorama, 3: Subregion, 4: Warp copy" },
	{ "axb", "\tOutput size in main source buffer" },
	{ "axb", "Output offset to the main buffer, default is 0x0" },
	{ "a/b", "\t\tZoom factor. a<b: zoom out (Wall/Ceiling Normal, Wall Panorama), a>b: zoom in (for all mode expect no transform" },

	{ "0~180", "\t(Wall/Ceiling/Desktop panorama)Panorama horizontal angle." },
	{ "-180~180", "\t(Wall/Ceiling/Desktop Subregion)Pan angle. -180~180 for ceiling/desktop mount, -90~90 for wall mount." },
	{ "-90~90", "\t(Wall/Ceiling/Desktop Subregion)Tilt angle. -90~90 for wall mount, -90~0 for ceiling mount, 0~90 for desktop." },
	{ "axb", "\t\t(Wall/Ceiling/Desktop Subregion) ROI center offset to the circle center. Negative is left/top and positive is right/bottom" },
	{ "axb", "\t\t(No Transform) ROI offset in pre main buffer." },
	{ "axb", "\t\t(No Transform) ROI size in pre main buffer." },
	{ "0~7", "\tSource region number for warp copy" },
	{ "axb", "\tOffset in main buffer for warp copy" },

	{ "axb", "\tGet the axis in fisheye domain converted from the output region.  axb is the axis to the output left top. Negative is left/top and positive is right/bottom" },
	{ "axb", "\tGet the axis in the output region converted from the fisheye domain.  axb is the axis to the fisheye center. Negative is left/top and positive is right/bottom" },

	{ "80~90", "\t(Ceiling Normal / Ceiling Panorama) Edge angle." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) North." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) South." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) West." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) East." },

	{ "", "\t\tClear all warp effect" },
	{ ">0", "\tEffective focal length in pixel for the lens using look up table for r and theta." },
	{ "file", "\tLook up table file." },
	{ "1~4", "Max warp area number for wall normal mode. Default is 4." },
	{ "1~2", "Max warp area number for wall panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling normal mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling sub region mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop normal mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop sub region mode. Default is 2." },
	{ "0|1", "\t\tFisheye warp method. 0: Default warp control API, 1: New warp process API." },
	{ "", "\t\tUpdate selected warp regions, which are already configured." },

	{ "file", "\tFile prefix to save vector maps." },
	{ "1|3", "\tSpecify the sub source buffer id, 1 for 2nd buffer, 3 for 4th buffer."},
	{ "0|1", "\tFlag to keep previous DPTZ layout on 2nd / 4th buffer. Default (0) is to let 2nd / 4th buffer have same layout as main buffer." },
	{ "", "\t\tPrint area info" },
	{ "0|1|2", "\tLog level. 0: error, 1: info (default), 2: debug" },
};

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
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atoi(tmp_string);

	return 0;
}

// first and second values must be in format of "AxB"
static int get_two_float(const char *name, float *first, float *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atof(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atof(tmp_string);

	return 0;
}

static void usage_examples(char * program_self)
{
	printf("\n");
	printf("Example:\n");
	BOLD_PRINT0("  Len: Focusafe 1.25mm, CS mount, 1/3\", FOV 185 degree\n");
	BOLD_PRINT0("  Sensor: IMX172\n");
	BOLD_PRINT0("  Prepare Work:\n");
	BOLD_PRINT0("    test_encode -i 4000x3000 -V480p --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x2048 --bmaxsize 2048x2048 -J --btype off -K --btype prev --bsize 480p --vout-swap 1 -w 2048 -W 2048 -P --bsize 2048x2048 --bins 2280x2280 --bino 880x360 -A --smaxsize 2048x2048\n");
	BOLD_PRINT0("    test_idsp -a\n");

	BOLD_PRINT0("  Len: Sunex PN DSL 215, FOV 185 degree, image circle 4.7mm\n");
	BOLD_PRINT0("  Sensor: IMX178\n");
	BOLD_PRINT0("  Prepare Work:\n");
	BOLD_PRINT0("    test_encode -i 3072x2048 -V480p --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x2048 --bmaxsize 2048x2048 -J --btype off -K --btype prev --bsize 480p --vout-swap 1 -w 1968 -W 2048 -P --bsize 1968x1968 --bins 1968x1968 --bino 556x40 -A --smaxsize 2048x2048\n");
	BOLD_PRINT0("    test_idsp -a\n");

	printf("\n");
	printf("0. Clear warping effect\n");
	printf("    %s -c\n\n", program_self);
	printf("1.[Wall Normal]  Correct both of horizontal/vertical lines\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x2048 -m %d\n\n", program_self, MOUNT_WALL, WARP_NORMAL);
	printf("2.[Unwarp + Wall Sub Region + Wall panorama]\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 --ns 2048x2048 -m%d -a1 -r -400x0 -s1024x1024 -o1024x0 -z15/10 -m%d -a2 -s2048x1024 -o0x1024 -h 180 -m%d\n\n",
	    program_self, MOUNT_WALL, WARP_NO_TRANSFORM, WARP_SUBREGION,
	    WARP_PANOR);
	printf("3.[Unwarp + Wall Sub Region + Wall panorama] Pan/Tilt the sub region and keep previous PTZ in other source buffers\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 --ns 2048x2048 -m%d -a1 -r -200x0 -s1024x1024 -o1024x0 -z15/10 -m%d -a2 -s2048x1024 -o0x1024 -h 180 -m%d -b 1 -k 1 -b 3 -k 1\n\n",
	    program_self, MOUNT_WALL, WARP_NO_TRANSFORM, WARP_SUBREGION,
	    WARP_PANOR);
	printf("4.[Ceiling Normal North and South]  Correct the lines in the north or south.\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x1024 -N -m%d -a1 -s2048x1024 -o 0x1024 -S -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_NORMAL, WARP_NORMAL);
	printf("5.[Ceiling West and East] Correct the lines in the west or east.\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x1024 -W -m%d -a1 -s2048x1024 -o 0x1024 -E -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_NORMAL, WARP_NORMAL);
	printf("6.[Ceiling Panorama]\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 -N -h 90 -z3/2 -m%d -a1 -s1024x1024 -o 1024x0 -E -h 90 -z3/2 -m%d -a2 -s1024x1024 -o 0x1024 -S -h 90 -z3/2 -m%d -a3 -s1024x1024 -o 1024x1024 -W -h 90 -z3/2 -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_PANOR, WARP_PANOR,
	    WARP_PANOR, WARP_PANOR);
	printf("7.[Ceiling Surround] Correct north/south/west/east four regions\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 -t -40 -m%d -a1 -s1024x1024 -o1024x0 -t -40 -p 90 -m%d -a2 -s1024x1024 -o0x1024 -t -40 -p 180 -m%d -a3 -s1024x1024 -o1024x1024 -t -40 -p -90 -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_SUBREGION, WARP_SUBREGION,
	    WARP_SUBREGION, WARP_SUBREGION);
	printf("8.[Desktop Normal North and South]  Correct the lines in the north or south.\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x1024 -N -m%d -a1 -s2048x1024 -o 0x1024 -S -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_NORMAL, WARP_NORMAL);
	printf("9.[Desktop West and East] Correct the lines in the west or east.\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x1024 -W -m%d -a1 -s2048x1024 -o 0x1024 -E -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_NORMAL, WARP_NORMAL);
	printf("10.[Desktop Panorama]\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 -W -h 90 -z3/2 -m%d -a1 -s1024x1024 -o 1024x0 -S -h 90 -z3/2 -m%d -a2 -s1024x1024 -o 0x1024 -E -h 90 -z3/2 -m%d -a3 -s1024x1024 -o 1024x1024 -N -h 90 -z3/2 -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_PANOR, WARP_PANOR,
	    WARP_PANOR, WARP_PANOR);
	printf("11.[Desktop Surround] Correct north/south/west/east four regions\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s1024x1024 -t 40 -m%d -a1 -s1024x1024 -o1024x0 -t 40 -p 90 -m%d -a2 -s1024x1024 -o0x1024 -t 40 -p 180 -m%d -a3 -s1024x1024 -o1024x1024 -t 40 -p -90 -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_SUBREGION, WARP_SUBREGION,
	    WARP_SUBREGION, WARP_SUBREGION);
	printf("12. Point Mapping between fisheye and output region in wall normal mode.\n");
	printf("    %s -M %d -F 185 -R 1024 -a0 -s2048x2048 -m %d --output-point 100x200 --fish-point -519x-463\n\n", program_self, MOUNT_WALL, WARP_NORMAL);
}

static void usage(void)
{
	u32 i;
	char * program_self = "test_dewarp";
	printf("\n%s usage:\n\n", program_self);
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
	usage_examples(program_self);
}

static int get_multi_arg(char *input_str, int *argarr, int *argcnt, int maxcnt)
{
	int i = 0;
	char *delim = ",:-\n\t";
	char *ptr;
	*argcnt = 0;

	iav_source_buffer_setup_ex_t buffer_setup;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = (int)(atof(ptr) * buffer_setup.pre_main.width);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = (int)(atof(ptr) * buffer_setup.pre_main.width);
		if (i >= maxcnt) {
			break;
		}
	}

	*argcnt = i;
	return 0;
}

static int get_lut_data_from_file(const char* file)
{
	FILE* fp;
	char line[1024];
	int num = dewarp_init_param.max_fov / 2 + 1;
	int argcnt, left = num;
	int* param;

	dewarp_init_param.lut_radius = malloc(num * sizeof(int));

	if (!dewarp_init_param.lut_radius) {
		printf("Failed to malloc %d.\n", num);
		return -1;
	}
	param = dewarp_init_param.lut_radius;
	if ((fp = fopen(file, "r")) == NULL) {
		printf("Failed to open file [%s].\n", file);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL && left > 0) {
		get_multi_arg(line, param, &argcnt, left);
		param += argcnt;
		left -= argcnt;
	}
	fclose(fp);

	if (left > 0) {
		printf("Need %d elements in file [%s].\n", num, file);
		return -1;
	}

	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, value, first, second, i;
	float first_f, second_f;
	int option_index = 0;
	char lut_file[FILENAME_LENGTH];

	opterr = 0;
	for (i = 0; i < MAX_FISHEYE_REGION_NUM; ++i) {
		fisheye_mode[i] = -1;
		warp_copy[i].src = -1;
		warp_copy[i].src_flag = 0;
		edge_angle[i] = 90;
	}

	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index))
			!= -1) {
		switch (ch) {
			case 'M':
				value = atoi(optarg);
				fisheye_mount = value;
				break;
			case 'F':
				value = atoi(optarg);
				dewarp_init_param.max_fov = value;
				break;
			case 'L':
				value = atoi(optarg);
				dewarp_init_param.projection_mode = value;
				break;
			case 'R':
				dewarp_init_param.max_radius = atoi(optarg);
				break;
			case 'C':
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dewarp_init_param.lens_center_in_max_input.x = first;
				dewarp_init_param.lens_center_in_max_input.y = second;
				break;
			case 'a':
				value = atoi(optarg);
				VERIFY_AREAID(value);
				current_region = value;
				break;
			case 's':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				fisheye_region[current_region].output.width = first;
				fisheye_region[current_region].output.height = second;
				break;
			case 'o':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				fisheye_region[current_region].output.upper_left.x = first;
				fisheye_region[current_region].output.upper_left.y = second;
				break;
			case 'm':
				VERIFY_AREAID(current_region);
				value = atoi(optarg);
				if (value < 0 || value >= WARP_MODE_TOTAL_NUM) {
					printf("Invalid mode [%d] for region [%d]. It must be in [0~%d].\n",
						value, current_region, (WARP_MODE_TOTAL_NUM - 1));
				}
				fisheye_mode[current_region] = value;
				fisheye_mode_flag |= (1 << current_region);
				break;
			case 'z':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, '/') < 0)
					return -1;
				fisheye_region[current_region].zoom.denom = second;
				fisheye_region[current_region].zoom.num = first;
				break;
			case 'N':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_NORTH;
				break;
			case 'S':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_SOUTH;
				break;
			case 'W':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_WEST;
				break;
			case 'E':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_EAST;
				break;
			case 'f':
				VERIFY_AREAID(current_region);
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] is too long [%d] (>%d).\n", optarg,
					    value, FILENAME_LENGTH);
					return -1;
				}
				file_flag = 1;
				strncpy(file, optarg, value);
				break;
			case 'G':
				VERIFY_AREAID(current_region);
				edge_angle[current_region] = (degree_t) atof(optarg);
				break;
			case 'h':
				VERIFY_AREAID(current_region);
				hor_panor_range[current_region] = (degree_t) atof(optarg);
				break;
			case 'p':
				VERIFY_AREAID(current_region);
				pantilt[current_region].pan = (degree_t) atof(optarg);
				pantilt_flag[current_region] = 1;
				break;
			case 't':
				VERIFY_AREAID(current_region);
				pantilt[current_region].tilt = (degree_t) atof(optarg);
				pantilt_flag[current_region] = 1;
				break;
			case 'r':
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				roi[current_region].x = first_f;
				roi[current_region].y = second_f;
				break;
			case SPECIFY_NOTRANS_OFFSET:
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans[current_region].upper_left.x = first;
				notrans[current_region].upper_left.y = second;
				break;
			case SPECIFY_NOTRANS_SIZE:
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans[current_region].width = first;
				notrans[current_region].height = second;
				break;
			case SPECIFY_WARP_COPY_SOURCE:
				VERIFY_AREAID(current_region);
				if (fisheye_mode[current_region] != WARP_COPY) {
					printf("Region [%d] is not in [Warp Copy] mode.\n", current_region);
					return -1;
				}
				value = atoi(optarg);
				if (value < 0 || value >= MAX_FISHEYE_REGION_NUM) {
					printf("Warp copy region source [%d] is out of range [0~%d].\n",
						value, (MAX_FISHEYE_REGION_NUM - 1));
					return -1;
				}
				warp_copy[current_region].src = value;
				warp_copy[current_region].src_flag = 1;
				break;
			case SPECIFY_WARP_COPY_OFFSET:
				VERIFY_AREAID(current_region);
				if (fisheye_mode[current_region] != WARP_COPY) {
					printf("Region [%d] is not in [Warp Copy] mode.\n", current_region);
					return -1;
				}
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				warp_copy[current_region].dst_x = first;
				warp_copy[current_region].dst_y = second;
				warp_copy[current_region].off_flag = 1;
				break;

			case SPECIFY_POINT_MAPPING_OUTPUT:
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				point_mapping_output[current_region].region.x = first_f;
				point_mapping_output[current_region].region.y = second_f;
				point_mapping_output_flag[current_region] = 1;
				break;

			case SPECIFY_POINT_MAPPING_FISHEYE:
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				point_mapping_fisheye[current_region].fisheye.x = first_f;
				point_mapping_fisheye[current_region].fisheye.y = second_f;
				point_mapping_fisheye_flag[current_region] = 1;
				break;

			case SPECIFY_LUT_EFL:
				dewarp_init_param.lut_focal_length = atoi(optarg);
				break;
			case SPECIFY_LUT_FILE:
				snprintf(lut_file, sizeof(lut_file), "%s", optarg);
				break;
			case SPECIFY_MAX_WALLNOR_AREAS:
				fisheye_config_param.wall_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_WALLPANOR_AREAS:
				fisheye_config_param.wall_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILNOR_AREAS:
				fisheye_config_param.ceiling_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILPANOR_AREAS:
				fisheye_config_param.ceiling_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILSUB_AREAS:
				fisheye_config_param.ceiling_sub_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKNOR_AREAS:
				fisheye_config_param.desktop_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKPANOR_AREAS:
				fisheye_config_param.desktop_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKSUB_AREAS:
				fisheye_config_param.desktop_sub_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_WARP_METHOD:
				value = atoi(optarg);
				if (value < 0 || value > 1) {
					printf("Invalid value [%d] for fisheye warp method [0|1].\n",
						value);
					return -1;
				}
				fisheye_warp_method = value;
				break;
			case 'c':
				clear_flag = 1;
				break;
			case 'u':
				update_flag = 1;
				break;
			case 'b':
				value = atoi(optarg);
				VERIFY_WARP_DPTZ_BUFFER_ID(value);
				current_buffer = value;
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
			case 'v':
				verbose = 1;
				break;
			case 'l':
				log_level = atoi(optarg);
				break;
			default:
				printf("unknown option found: %d\n", ch);
				return -1;
				break;
		}
	}

	if (dewarp_init_param.projection_mode == PROJECTION_LOOKUPTABLE) {
		if (strlen(lut_file) <= 0) {
			printf("No look up table file!\n");
			return -1;
		}
		if (get_lut_data_from_file(lut_file) < 0) {
			printf("Failed to load lens degree-radius table from file [%s].\n",
			    lut_file);
			return -1;
		}
	}

	for (i = 0; i < MAX_FISHEYE_REGION_NUM; ++i) {
		if ((fisheye_mode[i] == WARP_COPY) && (warp_copy[i].src_flag == 0)) {
			printf("Region [%d] is set to [Warp Copy] mode with "
				"no source region!\n", i);
			return -1;
		}
	}

	return 0;
}

static void save_warp_table_to_file(iav_warp_vector_ex_t *p_area, int area_id)
{
	FILE *fp;
	int i, j;

	char fullname[FILENAME_LENGTH];

	snprintf(fullname, sizeof(fullname), "%s_hor_%d", file, area_id);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	for (i = 0; i < MAX_GRID_HEIGHT; i++) {
		for (j = 0; j < MAX_GRID_WIDTH; j++) {
			if (!p_area->hor_map.addr) {
				fprintf(fp, "0\t");
			} else {
				fprintf(fp, "%d\t",
			    *((s16 *) p_area->hor_map.addr + i * MAX_GRID_WIDTH + j));
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	fp = NULL;
	printf("Save hor table to [%s].\n", fullname);

	snprintf(fullname, sizeof(fullname), "%s_ver_%d", file, area_id);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	for (i = 0; i < MAX_GRID_HEIGHT; i++) {
		for (j = 0; j < MAX_GRID_WIDTH; j++) {
			if (!p_area->ver_map.addr) {
				fprintf(fp, "0\t");
			} else {
				fprintf(fp, "%d\t",
			    *((s16 *) p_area->ver_map.addr + i * MAX_GRID_WIDTH + j));
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	printf("Save ver table to [%s].\n", fullname);
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
			//		printf("NOT supported spacing [%d]. Use 'spacing of 64 pixels'!\n",
//			spacing);
			grid_space = GRID_SPACING_PIXEL_64;
			break;
	}
	return grid_space;
}

static void update_warp_area(iav_warp_vector_ex_t *p_area, int area_id)
{
	warp_vector_t *p_vector = &fisheye_vector[area_id];

	p_area->enable = 1;
	p_area->input.width = p_vector->input.width;
	p_area->input.height = p_vector->input.height;
	p_area->input.x = p_vector->input.upper_left.x;
	p_area->input.y = p_vector->input.upper_left.y;
	p_area->output.width = p_vector->output.width;
	p_area->output.height = p_vector->output.height;
	p_area->output.x = p_vector->output.upper_left.x;
	p_area->output.y = p_vector->output.upper_left.y;
	p_area->rotate_flip = p_vector->rotate_flip;

	p_area->hor_map.enable = (p_vector->hor_map.rows > 0
	    && p_vector->hor_map.cols > 0);
	p_area->hor_map.horizontal_spacing =
	    get_grid_spacing(p_vector->hor_map.grid_width);
	p_area->hor_map.vertical_spacing =
	    get_grid_spacing(p_vector->hor_map.grid_height);
	p_area->hor_map.output_grid_row = p_vector->hor_map.rows;
	p_area->hor_map.output_grid_col = p_vector->hor_map.cols;
	p_area->hor_map.addr = (u32) p_vector->hor_map.addr;

	p_area->ver_map.enable = (p_vector->ver_map.rows > 0
	    && p_vector->ver_map.cols > 0);
	p_area->ver_map.horizontal_spacing =
	    get_grid_spacing(p_vector->ver_map.grid_width);
	p_area->ver_map.vertical_spacing =
	    get_grid_spacing(p_vector->ver_map.grid_height);
	p_area->ver_map.output_grid_row = p_vector->ver_map.rows;
	p_area->ver_map.output_grid_col = p_vector->ver_map.cols;
	p_area->ver_map.addr = (u32) p_vector->ver_map.addr;

	if (verbose) {
		printf("\t==Area[%d]==\n", area_id);
		printf("\tInput size %dx%d, offset %dx%d\n", p_area->input.width,
		    p_area->input.height, p_area->input.x, p_area->input.y);
		printf("\tOutput size %dx%d, offset %dx%d\n", p_area->output.width,
		    p_area->output.height,
		    p_area->output.x, p_area->output.y);
		printf("\tRotate/Flip %d\n", p_area->rotate_flip);
		printf("\tHor Table %dx%d, grid %dx%d (%dx%d)\n",
		    p_area->hor_map.output_grid_col, p_area->hor_map.output_grid_row,
		    p_area->hor_map.horizontal_spacing,
		    p_area->hor_map.vertical_spacing,
		    p_vector->hor_map.grid_width, p_vector->hor_map.grid_height);
		printf("\tVer Table %dx%d, grid %dx%d (%dx%d)\n",
		    p_area->ver_map.output_grid_col, p_area->ver_map.output_grid_row,
		    p_area->ver_map.horizontal_spacing,
		    p_area->ver_map.vertical_spacing,
		    p_vector->ver_map.grid_width, p_vector->ver_map.grid_height);
	}
}

static int check_warp_area(iav_warp_vector_ex_t * p_area, int area_id)
{
	u32 new_width, new_height;
	if (!p_area->input.height || !p_area->input.width ||
		!p_area->output.width || !p_area->output.height) {
		printf("Area_%d: input size %dx%d, output size %dx%d cannot be 0\n",
		    area_id, p_area->input.width, p_area->input.height,
		    p_area->output.width, p_area->output.height);
		return -1;
	}

	if ((p_area->input.x + p_area->input.width > buffer_setup.pre_main.width)
		|| (p_area->input.y + p_area->input.height > buffer_setup.pre_main.height)) {
		printf("Area_%d: input size %dx%d + offset %dx%d is out of pre main %dx%d.\n",
			area_id, p_area->input.width, p_area->input.height,
			p_area->input.x, p_area->input.y,
			buffer_setup.pre_main.width, buffer_setup.pre_main.height);
		return -1;
	}

	if ((p_area->output.x + p_area->output.width > buffer_setup.size[0].width)
		|| (p_area->output.y + p_area->output.height > buffer_setup.size[0].height)) {
		new_width = (p_area->output.x + p_area->output.width
		        > buffer_setup.size[0].width) ?
		        (buffer_setup.size[0].width - p_area->output.x) :
		        p_area->output.width;
		new_height = (p_area->output.y + p_area->output.height
		        > buffer_setup.size[0].height) ?
		        (buffer_setup.size[0].height - p_area->output.y) :
		        p_area->output.height;
		printf("Area_%d: output size %dx%d + offset %dx%d is out of main "
			"%dx%d. Reset to size [%dx%d].\n",
			area_id, p_area->output.width, p_area->output.height,
			p_area->output.x, p_area->output.y,
			buffer_setup.size[0].width, buffer_setup.size[0].height,
			new_width, new_height);
		p_area->output.width = new_width;
		p_area->output.height = new_height;
	}

	return 0;
}

static int create_wall_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Normal");
		if ((*area_num = fisheye_wall_normal(&fisheye_region[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_normal_to_fisheye(&fisheye_region[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_normal(&fisheye_region[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Panorama");
		if ((*area_num = fisheye_wall_panorama(&fisheye_region[region_id],
				hor_panor_range[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_panorama_to_fisheye(&fisheye_region[region_id],
					hor_panor_range[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_panorama(&fisheye_region[region_id],
					hor_panor_range[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_wall_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_wall_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_ceiling_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Normal");
		if ((*area_num = fisheye_ceiling_normal(&fisheye_region[region_id],
				edge_angle[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_normal_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_normal(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Panorama");
		if ((*area_num = fisheye_ceiling_panorama(&fisheye_region[region_id],
				edge_angle[region_id],
				hor_panor_range[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_panorama_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_panorama(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_ceiling_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_ceiling_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_desktop_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Normal");
		if ((*area_num = fisheye_desktop_normal(&fisheye_region[region_id],
				edge_angle[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_normal_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id], &point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_normal(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Panorama");
		if ((*area_num = fisheye_desktop_panorama(&fisheye_region[region_id],
				edge_angle[region_id], hor_panor_range[region_id],
				orient[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_panorama_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_panorama(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_desktop_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_desktop_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_fisheye_warp_vector(int region_id, int area_id)
{
	int area_num = 0;
	char mode_str[MODE_STRING_LENGTH];
	struct timespec lasttime, curtime;

	if (verbose) {
		clock_gettime(CLOCK_REALTIME, &lasttime);
	}

	switch (fisheye_mode[region_id]) {
	case WARP_NO_TRANSFORM:
		snprintf(mode_str, MODE_STRING_LENGTH, "No Transform");
		if ((area_num = fisheye_no_transform(&fisheye_region[region_id],
				&notrans[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
		break;

	case WARP_COPY:
		/* Use one region to duplicate the source warp region */
		area_num = 1;
		break;

	default:
		switch (fisheye_mount) {
		case MOUNT_WALL:
			if (create_wall_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		case MOUNT_CEILING:
			if (create_ceiling_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		case MOUNT_DESKTOP:
			if (create_desktop_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		default:
			break;
		}
		break;
	}

	if ((verbose || point_mapping_output_flag[region_id] ||
		point_mapping_fisheye_flag[region_id]) && area_num > 0) {
		printf("\nFisheye Region [%d]: %s uses %d warp area(s).\n", region_id,
		    mode_str, area_num);
		if (point_mapping_output_flag[region_id]) {
			printf("Point in region (%f, %f) => Point in fisheye (%f, %f)",
			    point_mapping_output[region_id].region.x,
			    point_mapping_output[region_id].region.y,
			    point_mapping_output[region_id].fisheye.x,
			    point_mapping_output[region_id].fisheye.y);
			if (fisheye_mode[region_id] == WARP_SUBREGION) {
				printf(", pan %f, tilt %f",
					point_mapping_output[region_id].angle.pan,
					point_mapping_output[region_id].angle.tilt);
			}
			printf("\n");
		}
		if (point_mapping_fisheye_flag[region_id]) {
			printf("Point in fisheye (%f, %f) => Point in region (%f, %f)\n",
			    point_mapping_fisheye[region_id].fisheye.x,
			    point_mapping_fisheye[region_id].fisheye.y,
			    point_mapping_fisheye[region_id].region.x,
			    point_mapping_fisheye[region_id].region.y);
		}
		if (verbose) {
			clock_gettime(CLOCK_REALTIME, &curtime);
			printf("Elapsed Time (ms): [%05ld]\n",
			    (curtime.tv_sec - lasttime.tv_sec) * 1000
			        + (curtime.tv_nsec - lasttime.tv_nsec) / 1000000);
			if (fisheye_mode[region_id] == WARP_SUBREGION) {
				if (pantilt_flag[region_id]) {
					printf("pan %f, tilt %f => ROI (%f, %f)\n",
					    pantilt[region_id].pan, pantilt[region_id].tilt,
					    roi[region_id].x, roi[region_id].y);
				} else {
					printf("ROI (%f, %f) => pan %f, tilt %f\n",
					    roi[region_id].x, roi[region_id].y,
					    pantilt[region_id].pan, pantilt[region_id].tilt);
				}
			}
		}
	}

	return area_num;
}

static int set_fisheye_warp_control(void)
{
	iav_warp_control_ex_t warp_control;
	iav_warp_vector_ex_t * p_area = NULL;
	int i, fy_id, area_num = 0, used_area_num = 0;

	memset(&warp_control, 0, sizeof(iav_warp_control_ex_t));
	if (!clear_flag) {
		for (fy_id = 0, used_area_num = 0;
			fy_id < MAX_FISHEYE_REGION_NUM; fy_id++) {
			if ((area_num = create_fisheye_warp_vector(fy_id,
				used_area_num)) < 0)
				return -1;
			for (i = used_area_num; i < used_area_num + area_num; ++i) {
				p_area = &warp_control.area[i];
				update_warp_area(p_area, i);
				if (check_warp_area(p_area, i) < 0)
					return -1;
				if (file_flag) {
					save_warp_table_to_file(p_area, i);
				}
			}
			used_area_num += area_num;
		}
	}
	if (keep_dptz_flag) {
		warp_control.keep_dptz[1] = keep_dptz[1];
		warp_control.keep_dptz[3] = keep_dptz[3];
	}
	if (ioctl(fd_iav, IAV_IOC_SET_WARP_CONTROL_EX, &warp_control) < 0) {
		perror("IAV_IOC_SET_WARP_CONTROL_EX");
		return -1;
	}
	return 0;
}

static int set_fisheye_warp_proc(void)
{
	iav_warp_proc_t wp;
	iav_warp_vector_ex_t * wt = &wp.arg.transform;
	iav_warp_copy_t * wc = &wp.arg.copy;
	iav_apply_flag_t flags[IAV_WARP_PROC_NUM];
	int i, rid, area_num = 0, used_areas = 0, target_rid;
	u8 wc_rid_map[MAX_FISHEYE_REGION_NUM];

	if (!clear_flag) {
		memset(flags, 0, sizeof(flags));
		memset(wc_rid_map, 0, sizeof(wc_rid_map));
		for (rid = 0, used_areas = 0; rid < MAX_FISHEYE_REGION_NUM; ++rid) {
			if (!(fisheye_mode_flag & (1 << rid))) {
				continue;
			}
			memset(&wp, 0, sizeof(wp));

			/* For first time setup all regions, used_areas is possible bigger
			 * than rid due to the multiple areas for one region, so use
			 * "used_areas" for the region id.
			 * For later run time update for some regions, rid is usually
			 * no smaller than "used_areas", so use rid for the region.
			 */
			target_rid = (rid > used_areas) ? rid : used_areas;
			if ((area_num = create_fisheye_warp_vector(rid, used_areas)) < 0)
				return -1;
			/* "wc_rid_map" is used to save the actual region id from
			 * the user region id for warp copy regions */
			wc_rid_map[rid] = used_areas;
			for (i = used_areas; i < used_areas + area_num; ++i) {
				if (WARP_COPY == fisheye_mode[rid]) {
					wp.wid = IAV_WARP_TRANSFORM;
					wp.rid = warp_copy[rid].src;
					if (ioctl(fd_iav, IAV_IOC_GET_WARP_PROC, &wp) < 0) {
						perror("IAV_IOC_GET_WARP_PROC");
						return -1;
					}
					if (!wt->enable || wt->dup) {
						printf("Region [%d] warp copy source [%d] is "
							"not ready.\n", rid, wp.rid);
						return -1;
					}
					memset(&wp, 0, sizeof(wp));
					wc->enable = 1;
					wc->src_rid = wc_rid_map[warp_copy[rid].src];
					if (warp_copy[rid].off_flag) {
						wc->dst_x = warp_copy[rid].dst_x;
						wc->dst_y = warp_copy[rid].dst_y;
					}
					wp.wid = IAV_WARP_COPY;
				} else {
					update_warp_area(wt, i);
					if (check_warp_area(wt, i) < 0)
						return -1;
					if (file_flag) {
						save_warp_table_to_file(wt, i);
					}
					wp.wid = IAV_WARP_TRANSFORM;
				}
				flags[wp.wid].apply = 1;

				/* 1: Update parameters for the selected warp region;
				 * 0: Update parameter for all regions.
				 */
				flags[wp.wid].param |= update_flag ? (1 << target_rid) :
					(1 << i);
				wp.rid = update_flag ? target_rid : i;
				if (ioctl(fd_iav, IAV_IOC_CFG_WARP_PROC, &wp) < 0) {
					perror("IAV_IOC_CFG_WARP_PROC");
					return -1;
				}
			}
			used_areas += area_num;
		}
	}

	if (ioctl(fd_iav, IAV_IOC_APPLY_WARP_PROC, flags) < 0) {
		perror("IAV_IOC_APPLY_WARP_PROC");
		return -1;
	}

	return 0;
}

static int set_fisheye_warp(void)
{
	int rval = 0;
	switch (fisheye_warp_method) {
	case FISHEYE_WARP_CONTROL:
		rval = set_fisheye_warp_control();
		break;
	case FISHEYE_WARP_PROC:
		rval = set_fisheye_warp_proc();
		break;
	default:
		printf("INVALID fisheye warp method [%d].\n", fisheye_warp_method);
		rval = -1;
		break;
	}
	return rval;
}

static int init_fisheye_warp(void)
{
	int fy_id, j, cur_area_id, need_area_num;
	version_t version;
	fisheye_config_t config;
	iav_system_resource_setup_ex_t sys_res;
	memset(&sys_res, 0, sizeof(sys_res));
	sys_res.encode_mode = IAV_ENCODE_CURRENT_MODE;

	if (!clear_flag) {
		if (dewarp_get_version(&version) < 0)
			return -1;
		if (verbose) {
			printf("\nDewarp Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
			    version.description, version.major, version.minor,
			    version.patch, version.mod_time);
		}
		if (set_log(log_level, "stderr") < 0)
			return -1;

		if (fisheye_get_config(&config) < 0)
			return -1;

		if (fisheye_config_flag) {
			if (fisheye_config_param.wall_normal_max_area_num) {
				config.wall_normal_max_area_num =
				    fisheye_config_param.wall_normal_max_area_num;
			}
			if (fisheye_config_param.wall_panor_max_area_num) {
				config.wall_panor_max_area_num =
				    fisheye_config_param.wall_panor_max_area_num;
			}
			if (fisheye_config_param.ceiling_normal_max_area_num) {
				config.ceiling_normal_max_area_num =
				    fisheye_config_param.ceiling_normal_max_area_num;
			}
			if (fisheye_config_param.ceiling_panor_max_area_num) {
				config.ceiling_panor_max_area_num =
				    fisheye_config_param.ceiling_panor_max_area_num;
			}
			if (fisheye_config_param.ceiling_sub_max_area_num) {
				config.ceiling_sub_max_area_num =
				    fisheye_config_param.ceiling_sub_max_area_num;
			}
			if (fisheye_config_param.desktop_normal_max_area_num) {
				config.desktop_normal_max_area_num =
				    fisheye_config_param.desktop_normal_max_area_num;
			}
			if (fisheye_config_param.desktop_panor_max_area_num) {
				config.desktop_panor_max_area_num =
				    fisheye_config_param.desktop_panor_max_area_num;
			}
			if (fisheye_config_param.desktop_sub_max_area_num) {
				config.desktop_sub_max_area_num =
				    fisheye_config_param.desktop_sub_max_area_num;
			}
		}

		if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &sys_res) < 0) {
			perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
			return -1;
		}

		config.max_warp_input_width = sys_res.max_warp_input_width;
		config.max_warp_output_width = sys_res.max_warp_output_width;

		if (fisheye_set_config(&config) < 0)
			return -1;

		for (fy_id = 0, cur_area_id = 0;
			fy_id < MAX_FISHEYE_REGION_NUM; fy_id++) {
			need_area_num = 0;
			switch (fisheye_mount) {
			case MOUNT_WALL:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.wall_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.wall_panor_max_area_num;
					break;
				case WARP_SUBREGION:
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			case MOUNT_CEILING:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.ceiling_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.ceiling_panor_max_area_num;
					break;
				case WARP_SUBREGION:
					need_area_num = config.ceiling_sub_max_area_num;
					break;
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			case MOUNT_DESKTOP:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.desktop_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.desktop_panor_max_area_num;
					break;
				case WARP_SUBREGION:
					need_area_num = config.desktop_sub_max_area_num;
					break;
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			default:
				printf("Unknown mount %d.\n", fisheye_mount);
				return -1;
				break;
			}

			if (cur_area_id + need_area_num > MAX_NUM_WARP_AREAS) {
				printf("Need %d areas (> max %d)\n", cur_area_id
				    + need_area_num, MAX_NUM_WARP_AREAS);
				return -1;
			}
			for (j = 0; j < need_area_num; j++) {
				fisheye_vector[cur_area_id + j].hor_map.addr =
				    &hor_map_addr[cur_area_id + j][0];
				fisheye_vector[cur_area_id + j].ver_map.addr =
				    &ver_map_addr[cur_area_id + j][0];
			}
			cur_area_id += need_area_num;
		}

		if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX,
			&buffer_setup) < 0) {
			perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
			return -1;
		}
		dewarp_init_param.max_input_width = buffer_setup.pre_main.width;
		dewarp_init_param.max_input_height = buffer_setup.pre_main.height;
		if (!dewarp_init_param.lens_center_in_max_input.x
		    || !dewarp_init_param.lens_center_in_max_input.y) {
			dewarp_init_param.lens_center_in_max_input.x =
				buffer_setup.pre_main.width / 2;
			dewarp_init_param.lens_center_in_max_input.y =
				buffer_setup.pre_main.height / 2;
		}

		if (dewarp_init(&dewarp_init_param) < 0)
			return -1;
	}

	return 0;
}

static int deinit_fisheye_warp(void)
{
	dewarp_deinit();
	if (dewarp_init_param.lut_radius) {
		free(dewarp_init_param.lut_radius);
		dewarp_init_param.lut_radius = NULL;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;

	if (argc < 2) {
		usage();
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (init_fisheye_warp() < 0) {
		ret = -1;
	} else if (set_fisheye_warp() < 0) {
		ret = -1;
	}
	if (deinit_fisheye_warp() < 0) {
		ret = -1;
	}
	return ret;
}

