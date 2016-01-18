/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "fb8.h"
#include <basetypes.h>
#include <text_insert.h>
#include <wchar.h>

typedef enum {
	FB8_COLOR_TRANSPARENT		= 0,
	FB8_COLOR_RED				= 1,
	FB8_COLOR_GREEN				= 2,
	FB8_COLOR_BLUE				= 3,
	FB8_COLOR_WHITE				= 4,
	FB8_COLOR_YELLOW			= 5,
	FB8_COLOR_CYAN				= 6,
} fb8_color_t;

#define FACE_LINE_WIDTH			0

#define FB8_LINE_SPACING(x)		((x) * 3 / 2)

static int						fd;
static struct fb_fix_screeninfo	finfo;
static struct fb_var_screeninfo	vinfo;
struct fb_cmap					cmap;
static unsigned char			*mem = NULL;

pixel_type_t					pixel_type;
font_attribute_t				font;
bitmap_info_t					bmp;

int open_fb(void)
{
	int				ret;
	unsigned short	*buf, *y, *u, *v, *a;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		ret = -1;
		goto open_fb_exit;
	}

	buf = (unsigned short *)malloc(256 * 4 * sizeof(unsigned short));
	if (!buf) {
		ret = -1;
		goto open_fb_exit;
	}
	memset(buf, 0, 256 * 4 * sizeof(unsigned short));
	y					= buf;
	u					= buf + 256;
	v					= buf + 512;
	a					= buf + 768;

	y[FB8_COLOR_RED]	= 82;
	u[FB8_COLOR_RED]	= 90;
	v[FB8_COLOR_RED]	= 240;
	a[FB8_COLOR_RED]	= 255;

	y[FB8_COLOR_GREEN]	= 145;
	u[FB8_COLOR_GREEN]	= 54;
	v[FB8_COLOR_GREEN]	= 34;
	a[FB8_COLOR_GREEN]	= 255;

	y[FB8_COLOR_BLUE]	= 41;
	u[FB8_COLOR_BLUE]	= 240;
	v[FB8_COLOR_BLUE]	= 110;
	a[FB8_COLOR_BLUE]	= 255;

	y[FB8_COLOR_WHITE]	= 235;
	u[FB8_COLOR_WHITE]	= 128;
	v[FB8_COLOR_WHITE]	= 128;
	a[FB8_COLOR_WHITE]	= 255;

	y[FB8_COLOR_YELLOW]	= 210;
	u[FB8_COLOR_YELLOW]	= 16;
	v[FB8_COLOR_YELLOW]	= 146;
	a[FB8_COLOR_YELLOW]	= 255;

	y[FB8_COLOR_CYAN]	= 170;
	u[FB8_COLOR_CYAN]	= 166;
	v[FB8_COLOR_CYAN]	= 16;
	a[FB8_COLOR_CYAN]	= 255;

	cmap.start	= 0;
	cmap.len	= 256;
	cmap.red	= y;
	cmap.green	= u;
	cmap.blue	= v;
	cmap.transp	= a;

	ret = ioctl(fd, FBIOPUTCMAP, &cmap);
	if (ret < 0) {
		ret = -1;
		goto open_fb_exit;
	}
	free(buf);

	pixel_type.pixel_background		= FB8_COLOR_TRANSPARENT;
	pixel_type.pixel_outline		= FB8_COLOR_CYAN;
	pixel_type.pixel_font			= FB8_COLOR_BLUE;
	ret = text2bitmap_lib_init(&pixel_type);
	if (ret < 0) {
		ret = -1;
		goto open_fb_exit;
    }

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
    if (ret < 0) {
		ret = -1;
		goto open_fb_exit;
    }

    ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    if (ret < 0) {
		ret = -1;
		goto open_fb_exit;
    }

    mem = (unsigned char *)mmap(0, finfo.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
    if (!mem) {
		ret = -1;
		goto open_fb_exit;
    }

	ret = ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
	if (ret < 0) {
		ret = -1;
		munmap(mem, finfo.smem_len);
		goto open_fb_exit;
	}

open_fb_exit:
	if (ret && fd > 0) {
		close(fd);
	}
	return ret;
}

int blank_fb(void)
{
	if (fd < 0) {
		return -1;
	}

	ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
	return ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
}

int annotate_faces(struct fdet_face *faces, int num)
{
	int			ret;
	int			i, j, k;
	int			th[8];
	int			ox, oy;

	for (i = 0; i < num; i++) {
		unsigned short	color;

		if (faces[i].type == FDET_RESULT_TYPE_FS) {
			color = FB8_COLOR_RED;
		} else {
			color = FB8_COLOR_BLUE;
		}

		th[0] = faces[i].x - faces[i].size / 2 - FACE_LINE_WIDTH;
		th[1] = faces[i].x - faces[i].size / 2 + FACE_LINE_WIDTH;
		th[2] = faces[i].x + faces[i].size / 2 - FACE_LINE_WIDTH;
		th[3] = faces[i].x + faces[i].size / 2 + FACE_LINE_WIDTH;

		th[4] = faces[i].y - faces[i].size / 2 - FACE_LINE_WIDTH;
		th[5] = faces[i].y - faces[i].size / 2 + FACE_LINE_WIDTH;
		th[6] = faces[i].y + faces[i].size / 2 - FACE_LINE_WIDTH;
		th[7] = faces[i].y + faces[i].size / 2 + FACE_LINE_WIDTH;

		if (th[0] < 0) {
			th[0] = 0;
		}
		if (th[0] >= (int)vinfo.xres) {
			th[0] = vinfo.xres - 1;
		}
		if (th[1] < 0) {
			th[1] = 0;
		}
		if (th[1] >= (int)vinfo.xres) {
			th[1] = vinfo.xres - 1;
		}
		if (th[2] < 0) {
			th[2] = 0;
		}
		if (th[2] >= (int)vinfo.xres) {
			th[2] = vinfo.xres - 1;
		}
		if (th[3] < 0) {
			th[3] = 0;
		}
		if (th[3] >= (int)vinfo.xres) {
			th[3] = vinfo.xres - 1;
		}

		if (th[4] < 0) {
			th[4] = 0;
		}
		if (th[4] >= (int)vinfo.yres) {
			th[4] = vinfo.yres - 1;
		}
		if (th[5] < 0) {
			th[5] = 0;
		}
		if (th[5] >= (int)vinfo.yres) {
			th[5] = vinfo.yres - 1;
		}
		if (th[6] < 0) {
			th[6] = 0;
		}
		if (th[6] >= (int)vinfo.yres) {
			th[6] = vinfo.yres - 1;
		}
		if (th[7] < 0) {
			th[7] = 0;
		}
		if (th[7] >= (int)vinfo.yres) {
			th[7] = vinfo.yres - 1;
		}

		for (j = th[4]; j <= th[5]; j++) {
			for (k = th[0]; k <= th[3]; k++) {
				mem[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[6]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[3]; k++) {
				mem[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[1]; k++) {
				mem[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[2]; k <= th[3]; k++) {
				mem[j * vinfo.xres + k] = color;
			}
		}

		if (!strlen(faces[i].name)) {
			continue;
		}

		memset(&font, 0, sizeof(font));
		strcpy(font.type, "/usr/share/fonts/Lucida.ttf");
		font.size				= faces[i].size / strlen(faces[i].name);
		font.disable_anti_alias = 1;
		ret = text2bitmap_set_font_attribute(&font);
		if (ret < 0) {
			continue;
	    }

		ox = th[0] + faces[i].size / 4;
		oy = th[4] + (faces[i].size - font.size) / 2;
		for (j = 0; j < (int)strlen(faces[i].name) - 1; j++) {
			ret = text2bitmap_convert_character(faces[i].name[j], mem + oy * finfo.line_length, font.size,
					finfo.line_length, ox, &bmp);
			if (ret < 0) {
				continue;
			}
			ox += bmp.width;
		}
	}

	return ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
}

int close_fb(void)
{
	if (fd > 0 && mem) {
		munmap(mem, finfo.smem_len);
		close(fd);
		return 0;
	} else {
		return -1;
	}
}
