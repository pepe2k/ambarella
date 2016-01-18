/*
 * test_fdet.c
 *
 * History:
 *	2012/07/10 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <getopt.h>

#include "basetypes.h"
#include "ambas_fdet.h"
#include "ambas_event.h"
#include "iav_drv.h"
#include "iav_encode_drv.h"

#define FDET_DEV			"/dev/fdet"
#define FB_DEV				"/dev/fb0"
#define IAV_DEV				"/dev/iav"

#define FDET_CLASSIFIER_BINARY_PATH	"/usr/local/bin/vca_data/fdet/classifiers/default.cls"
#define FDET_INPUT_FILE_PATH		"/usr/local/bin/vca_data/fdet/samples/1.y"

#define FACE_LINE_WIDTH			0
#define FACE_LINE_PIXEL_FS		0xf800
#define FACE_LINE_PIXEL_TS		0x001f

typedef enum {
	FDET_INPUT_TYPE_Y,
	FDET_INPUT_TYPE_YV12,
	FDET_INPUT_TYPE_VIN,
} fdet_input_type_t;

typedef struct {
	u32	width;
	u32	height;
} fdet_input_size_t;

char				g_cls_path[64] = FDET_CLASSIFIER_BINARY_PATH;
int				g_video_mode = 0;
fdet_input_type_t		g_input_type = FDET_INPUT_TYPE_Y;
fdet_input_size_t		g_input_size = {320, 240};
char				g_input_file[64] = FDET_INPUT_FILE_PATH;
u32				g_policy = 0;
unsigned char			g_bitmasks[3] = {0};
unsigned int			g_min_hitcounts[8] = {0};

int				g_fdet_fd;
int				g_fb_fd;
int				g_iav_fd;

u16				*g_fb_mem;
int				g_fb_size;
struct fb_var_screeninfo	g_fb_vinfo;
struct fb_fix_screeninfo	g_fb_finfo;

struct fdet_configuration	g_fdet_cfg;
char				*g_orig_buf;

struct fdet_face		g_faces[32];

FILE				*g_file;
u32				g_frames;

iav_mmap_info_t			g_iav_info;
struct iav_yuv_buffer_info_ex_s	g_iav_buf;

u32				g_seconds;

struct option long_options[] = {
	{"classifier",	1,	0,	'c'},
	{"video",	0,	0,	'v'},
	{"input_type",	1,	0,	't'},
	{"input_size",	1,	0,	's'},
	{"input_file",	1,	0,	'f'},
	{"policy",	1,	0,	'p'},
	{"om0",		1,	0,	'A'},	// om: Orientation Bitmask
	{"om1",		1,	0,	'B'},
	{"om2",		1,	0,	'C'},
	{"mh",		1,	0,	'D'},	// mh: minimum hitcounts
	{"mh0",		1,	0,	'E'},
	{"mh1",		1,	0,	'F'},
	{"mh2",		1,	0,	'G'},
	{"mh3",		1,	0,	'H'},
	{"mh4",		1,	0,	'I'},
	{"mh5",		1,	0,	'J'},
	{"mh6",		1,	0,	'K'},
	{"mh7",		1,	0,	'L'},
	{0,		0,	0,	 0 },
};

const char *short_options = "c:vt:s:f:p:A:B:C:D:E:F:G:H:I:J:K:L:";

void usage(void)
{
	printf("Usage:    test_fdet [-c classifier_path] [-v] [-t y|yv12|vin] [-s widthxheight] [-f file_path] [-p policy] [-A om0] [-B om1] [-C om2]\n");
	printf("          policy: bit0 - wait for entire fs to complete\n");
	printf("          policy: bit1 - do not perform ts\n");
	printf("          policy: bit2 - debug fdet\n");
	printf("          policy: bit3 - measure fs and ts time\n");
	printf("Example:  test_fdet -t yv12 -s 320x240 -f /sdcard/input.yv12.qvga -p 4 -A ff\n");
}

int init_param(int argc, char **argv)
{
	int	c;
	u32	d;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'c':
			strcpy(g_cls_path, optarg);;
			break;

		case 'v':
			g_video_mode = 1;
			break;

		case 't':
			if (!strcmp(optarg, "y")) {
				g_input_type = FDET_INPUT_TYPE_Y;
			}
			if (!strcmp(optarg, "yv12")) {
				g_input_type = FDET_INPUT_TYPE_YV12;
			}
			if (!strcmp(optarg, "vin")) {
				g_input_type = FDET_INPUT_TYPE_VIN;
			}
			break;

		case 's':
			sscanf(optarg, "%ux%u", &g_input_size.width, &g_input_size.height);
			break;

		case 'f':
			strcpy(g_input_file, optarg);
			break;

		case 'p':
			sscanf(optarg, "%x", &g_policy);
			break;

		case 'A':
			sscanf(optarg, "%x", &d);
			g_bitmasks[0] = d;
			break;

		case 'B':
			sscanf(optarg, "%x", &d);
			g_bitmasks[1] = d;
			break;

		case 'C':
			sscanf(optarg, "%x", &d);
			g_bitmasks[2] = d;
			break;

		case 'D':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[0] = d;
			g_min_hitcounts[1] = d;
			g_min_hitcounts[2] = d;
			g_min_hitcounts[3] = d;
			g_min_hitcounts[4] = d;
			g_min_hitcounts[5] = d;
			g_min_hitcounts[6] = d;
			g_min_hitcounts[7] = d;
			break;

		case 'E':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[0] = d;
			break;

		case 'F':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[1] = d;
			break;

		case 'G':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[2] = d;
			break;

		case 'H':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[3] = d;
			break;

		case 'I':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[4] = d;
			break;

		case 'J':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[5] = d;
			break;

		case 'K':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[6] = d;
			break;

		case 'L':
			sscanf(optarg, "%x", &d);
			g_min_hitcounts[7] = d;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			usage();
			return -1;

		}
	}

	return 0;
}

int init_fdet(void)
{
	int			ret = 0, size;
	FILE			*fp;
	char			*cls;

	g_fdet_fd = open(FDET_DEV, O_RDWR);
	if (g_fdet_fd < 0) {
		perror("Unable to open fdet device ");
		ret = -1;
		goto init_fdet_exit;
	}

	fp = fopen(g_cls_path, "rb");
	if (!fp) {
		perror("Unable to open classifier binary file ");
		ret = -1;
		goto init_fdet_exit;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	ret = ioctl(g_fdet_fd, FDET_IOCTL_SET_MMAP_TYPE, FDET_MMAP_TYPE_CLASSIFIER_BINARY);
	if (ret < 0) {
		perror("Unable to set mmap type to classifier binary ");
		ret = -1;
		goto init_fdet_exit;
	}

	cls = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fdet_fd, 0);
	if (!cls) {
		perror("Unable to mmap ");
		ret = -1;
		goto init_fdet_exit;
	}

	ret = fread(cls, 1, size, fp);
	if (ret != size) {
		perror("Error reading classifier binary file ");
		ret = -1;
		goto init_fdet_exit;
	}

	fclose(fp);
	munmap(cls, size);

	ret = ioctl(g_fdet_fd, FDET_IOCTL_SET_MMAP_TYPE, FDET_MMAP_TYPE_ORIG_BUFFER);
	if (ret < 0) {
		perror("Unable to set mmap type to orig buffer ");
		ret = -1;
		goto init_fdet_exit;
	}

	size = g_input_size.width * g_input_size.height;
	g_orig_buf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fdet_fd, 0);
	if (!g_orig_buf) {
		perror("Unable to mmap ");
		ret = -1;
		goto init_fdet_exit;
	}

	memset(&g_fdet_cfg, 0, sizeof(g_fdet_cfg));
	g_fdet_cfg.input_width	= g_input_size.width;
	g_fdet_cfg.input_height	= g_input_size.height;
	g_fdet_cfg.input_pitch	= g_input_size.width;
	g_fdet_cfg.input_mode	= FDET_MODE_STILL;
	g_fdet_cfg.policy	= g_policy;
	memcpy(g_fdet_cfg.om, g_bitmasks, sizeof(g_bitmasks));
	memcpy(g_fdet_cfg.min_hitcounts, g_min_hitcounts, sizeof(g_min_hitcounts));

init_fdet_exit:
	if (ret && g_fdet_fd >= 0) {
		close(g_fdet_fd);
	}
	return ret;
}

int init_fb(void)
{
	int			ret = 0;

	g_fb_fd = open(FB_DEV, O_RDWR);
	if (g_fb_fd < 0) {
		perror("Error opening fb ");
		ret = -1;
		goto init_fb_exit;
	}

	ret = ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &g_fb_finfo);
        if (ret < 0) {
		perror("Error reading fb fixed information ");
		ret = -1;
		goto init_fb_exit;
        }

        ret = ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &g_fb_vinfo);
        if (ret < 0) {
		perror("Error reading fb variable information ");
		ret = -1;
		goto init_fb_exit;
        }

	if (g_input_size.width > g_fb_vinfo.xres || g_input_size.height > g_fb_vinfo.yres) {
		printf("Size Error!\n");
		ret = -1;
		goto init_fb_exit;
	}

	g_fb_size = g_fb_vinfo.yres * g_fb_finfo.line_length;
        g_fb_mem = (unsigned short *)mmap(0, g_fb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                           g_fb_fd, 0);
        if (!g_fb_mem) {
		perror("Error mmaping fb ");
		ret = -1;
		goto init_fb_exit;
        }

	ret = ioctl(g_fb_fd, FBIOBLANK, FB_BLANK_NORMAL);
	if (ret < 0) {
		perror("Error blanking fb ");
		ret = -1;
		goto init_fb_exit;
	}

init_fb_exit:
	if (ret && g_fb_fd >= 0) {
		close(g_fb_fd);
	}
	return ret;
}

int init_iav(void)
{
	int	ret = 0;

	g_iav_fd = open(IAV_DEV, O_RDWR);
	if (!g_iav_fd) {
		perror("Error open iav ");
		ret = -1;
		goto init_iav_exit;
	}

	ret = ioctl(g_iav_fd, IAV_IOC_MAP_DSP, &g_iav_info);
	if (ret < 0) {
		perror("IAV_IOC_MAP_DSP");
		ret = -1;
		goto init_iav_exit;
	}

	memset(&g_iav_buf, 0, sizeof(g_iav_buf));
	g_iav_buf.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;
	ret = ioctl(g_iav_fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &g_iav_buf);
	if (ret < 0) {
		perror("Unable to get iav info ");
		ret = -1;
		goto init_iav_exit;
	}

	g_frames		= 1;

	g_fdet_cfg.input_source	= 1;
	g_fdet_cfg.input_pitch	= g_iav_buf.pitch;
	g_fdet_cfg.input_offset	= g_iav_buf.y_addr - g_iav_info.addr;

init_iav_exit:
	if (ret && g_iav_fd >= 0) {
		close(g_iav_fd);
	}
	return ret;
}

int init_sequence(void)
{
	int		ret = 0, size;

	g_file = fopen(g_input_file, "rb");
	if (!g_file) {
		perror("Unable to open input file ");
		ret = -1;
		goto init_sequence_exit;
	}

	fseek(g_file, 0, SEEK_END);
	size = ftell(g_file);
	fseek(g_file, 0, SEEK_SET);

	switch (g_input_type) {
	case FDET_INPUT_TYPE_Y:
		g_frames = size / (g_input_size.width * g_input_size.height);
		break;

	case FDET_INPUT_TYPE_YV12:
		g_frames = 2 * size / (3 * g_input_size.width * g_input_size.height);
		break;

	default:
		g_frames = 1;
		break;
	}

init_sequence_exit:
	return ret;
}

int load_y(void)
{
	int			ret, size;

	size = g_input_size.width * g_input_size.height;

	ret = fread(g_orig_buf, 1, size, g_file);
	if (ret != size) {
		ret = -1;
		goto load_y_exit;
	}

load_y_exit:
	return ret;
}

int load_yv12(void)
{
	int			ret, size;

	size = g_input_size.width * g_input_size.height;

	ret = fread(g_orig_buf, 1, size, g_file);
	if (ret != size) {
		ret = -1;
		goto load_yv12_exit;
	}
	fseek(g_file, size / 2, SEEK_CUR);

load_yv12_exit:
	return ret;
}

int load_vin(void)
{
	int			ret;

	g_iav_buf.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;
	ret = ioctl(g_iav_fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &g_iav_buf);
	if (ret < 0) {
		perror("Unable to get iav info ");
		ret = -1;
		goto load_vin_exit;
	}

	g_fdet_cfg.input_offset	= g_iav_buf.y_addr - g_iav_info.addr;

load_vin_exit:
	return ret;
}

int annotate_faces(int num)
{
	int			i, j, ret = 0;
	unsigned short		r, g, b;
	int			k, l;
	int			th[8];
	unsigned short		color;

	ret = ioctl(g_fb_fd, FBIOBLANK, FB_BLANK_NORMAL);
	if (ret < 0) {
		ret = -1;
		goto annotate_faces_exit;
	}

	if (!g_fdet_cfg.input_source) {
		for (i = 0; i < g_input_size.height; i++) {
			for (j = 0; j < g_input_size.width; j++) {
				k = i * g_input_size.width	   + j;
				l = i * g_fb_finfo.line_length / 2 + j;

				r		= g_orig_buf[k] >> 3;
				g		= g_orig_buf[k] >> 2;
				b		= g_orig_buf[k] >> 3;
				g_fb_mem[l]	= (r << 11) | (g << 5) | (b << 0);
			}
		}
	}


	for (i = 0; i < num; i++) {
		if (g_policy & FDET_POLICY_DEBUG) {
			printf("\tFace %2d, Type %d: (%3d, %3d), %3d\n", i,
				g_faces[i].type, g_faces[i].x, g_faces[i].y, g_faces[i].size);
		}

		if (g_faces[i].type == FDET_RESULT_TYPE_FS) {
			color = FACE_LINE_PIXEL_FS;
		} else {
			color = FACE_LINE_PIXEL_TS;
		}

		th[0] = g_faces[i].x - g_faces[i].size / 2 - FACE_LINE_WIDTH;
		th[1] = g_faces[i].x - g_faces[i].size / 2 + FACE_LINE_WIDTH;
		th[2] = g_faces[i].x + g_faces[i].size / 2 - FACE_LINE_WIDTH;
		th[3] = g_faces[i].x + g_faces[i].size / 2 + FACE_LINE_WIDTH;

		th[4] = g_faces[i].y - g_faces[i].size / 2 - FACE_LINE_WIDTH;
		th[5] = g_faces[i].y - g_faces[i].size / 2 + FACE_LINE_WIDTH;
		th[6] = g_faces[i].y + g_faces[i].size / 2 - FACE_LINE_WIDTH;
		th[7] = g_faces[i].y + g_faces[i].size / 2 + FACE_LINE_WIDTH;

		if (th[0] < 0) {
			th[0] = 0;
		}
		if (th[0] >= g_fb_vinfo.xres) {
			th[0] = g_fb_vinfo.xres - 1;
		}
		if (th[1] < 0) {
			th[1] = 0;
		}
		if (th[1] >= g_fb_vinfo.xres) {
			th[1] = g_fb_vinfo.xres - 1;
		}
		if (th[2] < 0) {
			th[2] = 0;
		}
		if (th[2] >= g_fb_vinfo.xres) {
			th[2] = g_fb_vinfo.xres - 1;
		}
		if (th[3] < 0) {
			th[3] = 0;
		}
		if (th[3] >= g_fb_vinfo.xres) {
			th[3] = g_fb_vinfo.xres - 1;
		}

		if (th[4] < 0) {
			th[4] = 0;
		}
		if (th[4] >= g_fb_vinfo.yres) {
			th[4] = g_fb_vinfo.yres - 1;
		}
		if (th[5] < 0) {
			th[5] = 0;
		}
		if (th[5] >= g_fb_vinfo.yres) {
			th[5] = g_fb_vinfo.yres - 1;
		}
		if (th[6] < 0) {
			th[6] = 0;
		}
		if (th[6] >= g_fb_vinfo.yres) {
			th[6] = g_fb_vinfo.yres - 1;
		}
		if (th[7] < 0) {
			th[7] = 0;
		}
		if (th[7] >= g_fb_vinfo.yres) {
			th[7] = g_fb_vinfo.yres - 1;
		}

		for (j = th[4]; j <= th[5]; j++) {
			for (k = th[0]; k <= th[3]; k++) {
				g_fb_mem[j * g_fb_finfo.line_length / 2 + k] = color;
			}
		}

		for (j = th[6]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[3]; k++) {
				g_fb_mem[j * g_fb_finfo.line_length / 2 + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[1]; k++) {
				g_fb_mem[j * g_fb_finfo.line_length / 2 + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[2]; k <= th[3]; k++) {
				g_fb_mem[j * g_fb_finfo.line_length / 2 + k] = color;
			}
		}
	}

	ret = ioctl(g_fb_fd, FBIOPAN_DISPLAY, &g_fb_vinfo);

annotate_faces_exit:
	return ret;
}

int fdet_still(void)
{
	int		i, ret = 0;

	ret = ioctl(g_fdet_fd, FDET_IOCTL_SET_CONFIGURATION, &g_fdet_cfg);
	if (ret < 0) {
		perror("Unable to configure face detection ");
		ret = -1;
		goto fdet_still_exit;
	}

	i = 0;
	while (i < g_frames) {
		switch (g_input_type) {
		case FDET_INPUT_TYPE_Y:
			ret = load_y();
			i++;
			break;

		case FDET_INPUT_TYPE_YV12:
			ret = load_yv12();
			i++;
			break;

		case FDET_INPUT_TYPE_VIN:
			ret = load_vin();
			break;

		default:
			ret = -1;
			break;
		}

		if (ret < 0) {
			break;
		}

		ret = ioctl(g_fdet_fd, FDET_IOCTL_START, NULL);
		if (ret < 0) {
			perror("Unable to start face detection ");
			ret = -1;
			break;
		}

		ret = ioctl(g_fdet_fd, FDET_IOCTL_GET_RESULT, g_faces);
		if (ret >= 0) {
			annotate_faces(ret);
		}

		ret = ioctl(g_fdet_fd, FDET_IOCTL_STOP, NULL);
		if (ret < 0) {
			perror("Unable to stop face detection ");
			ret = -1;
			break;
		}
	}

fdet_still_exit:
	return ret;
}

int fdet_video(void)
{
	int		i, ret = 0;

	switch (g_input_type) {
	case FDET_INPUT_TYPE_Y:
		ret = load_y();
		break;

	case FDET_INPUT_TYPE_YV12:
		ret = load_yv12();
		break;

	case FDET_INPUT_TYPE_VIN:
		ret = load_vin();
		break;

	default:
		ret = -1;
		break;
	}

	if (ret < 0) {
		goto fdet_video_exit;
	}

	g_fdet_cfg.input_mode = FDET_MODE_VIDEO;
	ret = ioctl(g_fdet_fd, FDET_IOCTL_SET_CONFIGURATION, &g_fdet_cfg);
	if (ret < 0) {
		perror("Unable to configure face detection ");
		ret = -1;
		goto fdet_video_exit;
	}

	ret = ioctl(g_fdet_fd, FDET_IOCTL_START, NULL);
	if (ret < 0) {
		perror("Unable to start face detection ");
		ret = -1;
		goto fdet_video_exit;
	}

	i = 0;
	while (i < g_frames) {
		ret = ioctl(g_fdet_fd, FDET_IOCTL_GET_RESULT, g_faces);
		if (ret < 0) {
			break;
		} else {
			annotate_faces(ret);
		}

		switch (g_input_type) {
		case FDET_INPUT_TYPE_Y:
			ret = load_y();
			i++;
			break;

		case FDET_INPUT_TYPE_YV12:
			ret = load_yv12();
			i++;
			break;

		case FDET_INPUT_TYPE_VIN:
			ret = load_vin();
			break;

		default:
			ret = -1;
			break;
		}

		if (ret < 0) {
			break;
		}

		ret = ioctl(g_fdet_fd, FDET_IOCTL_TRACK_FACE, g_fdet_cfg.input_offset);
		if (ret < 0) {
			break;
		}
	}

	ret = ioctl(g_fdet_fd, FDET_IOCTL_STOP, NULL);
	if (ret < 0) {
		perror("Unable to stop face detection ");
		ret = -1;
		goto fdet_video_exit;
	}

fdet_video_exit:
	return ret;
}

void quit(void)
{
	int		ret;

	ret = ioctl(g_fb_fd, FBIOBLANK, FB_BLANK_NORMAL);
	if (ret < 0) {
		perror("Error blank fb ");
		goto early_quit;
	}

	ret = ioctl(g_fb_fd, FBIOPAN_DISPLAY, &g_fb_vinfo);
	if (ret < 0) {
		perror("Error pan fb ");
		goto early_quit;
	}

	ret = ioctl(g_fdet_fd, FDET_IOCTL_STOP, NULL);
	if (ret < 0) {
		perror("Unable to stop face detection ");
		goto early_quit;
	}

	exit(0);

early_quit:
	exit(1);
}

int main(int argc, char **argv)
{
	int				ret;

	ret = init_param(argc, argv);
	if (ret < 0) {
		return -1;
	}

	ret = init_fdet();
	if (ret < 0) {
		return -1;
	}

	ret = init_fb();
	if (ret < 0) {
		return -1;
	}

	if (g_input_type == FDET_INPUT_TYPE_VIN) {
		ret = init_iav();
	} else {
		ret = init_sequence();
	}
	if (ret < 0) {
		return -1;
	}

	signal(SIGINT,  (__sighandler_t)quit);
	signal(SIGQUIT, (__sighandler_t)quit);
	signal(SIGTERM, (__sighandler_t)quit);

	if (!g_video_mode) {
		fdet_still();
	} else {
		fdet_video();
	}

	munmap((void *)g_fb_fd, g_fb_size);
	close(g_fdet_fd);
	close(g_fb_fd);

	return 0;
}

