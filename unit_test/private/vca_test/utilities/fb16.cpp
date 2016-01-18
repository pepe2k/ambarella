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
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "fb16.h"

#define COLOR_RED		0xf800
#define COLOR_BLUE		0x001f
#define FACE_LINE_WIDTH		0

static int			fd;
static struct fb_fix_screeninfo	finfo;
static struct fb_var_screeninfo	vinfo;
static unsigned short		*mem = NULL;

int open_fb(void)
{
	int		ret;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
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

	if (vinfo.yres > vinfo.yres_virtual / 2) {
		ret = -1;
		goto open_fb_exit;
	}

    mem = (unsigned short *)mmap(0, finfo.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
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

int get_fb_size(int *w, int *h)
{
	*w = vinfo.xres;
	*h = vinfo.yres;

	return 0;
}

int blank_fb(void)
{
	if (fd < 0) {
		return -1;
	}

	ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
	return ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
}

int render_frame_gray(char *d, int w, int h)
{
	int			ret;
	int			i, j, k, l;
	unsigned short		r, g, b;
	unsigned char		y;
	unsigned short		*p;

	if (fd < 0 || !mem) {
		return -1;
	}

	if (w > vinfo.xres || h > vinfo.yres) {
		return -1;
	}

	if (vinfo.yoffset) {
		vinfo.yoffset	= 0;
		p				= mem;
	} else {
		vinfo.yoffset	= vinfo.yres;
		p				= mem + vinfo.yoffset * finfo.line_length / 2;
	}

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			k	= i * w + j;
			y	= d[k];

			r	= y >> 3;
			g	= y >> 2;
			b	= y >> 3;

			l	= i * finfo.line_length / 2 + j;
			p[l]	= (r << 11) | (g << 5) | (b << 0);
			if (!p[l]) {
				p[l] = 1;
			}
		}
	}

	ret = ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int render_frame_bgr888(char *d, int w, int h)
{
	int			ret;
	int			i, j, k, l;
	unsigned short		r, g, b;
	unsigned char		y;
	unsigned short		*p;

	if (fd < 0 || !mem) {
		return -1;
	}

	if (w > vinfo.xres || h > vinfo.yres) {
		return -1;
	}

	if (vinfo.yoffset) {
		vinfo.yoffset	= 0;
		p				= mem;
	} else {
		vinfo.yoffset	= vinfo.yres;
		p				= mem + vinfo.yoffset * finfo.line_length / 2;
	}

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			k	= (i * w + j) * 3;

			r	= d[k + 2] >> 3;
			g	= d[k + 1] >> 2;
			b	= d[k + 0] >> 3;

			l	= (i + (vinfo.yres - h) / 2) * finfo.line_length / 2 + j + (vinfo.xres - w) / 2;
			p[l]	= (r << 11) | (g << 5) | (b << 0);
		}
	}

	ret = ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int annotate_faces(struct fdet_face *faces, int num)
{
	int				i, j, k;
	int				th[8];
	unsigned short	*mem2;

	mem2 = mem + vinfo.yoffset * finfo.line_length / 2;

	for (i = 0; i < num; i++) {
		unsigned short	color;

		if (faces[i].type == FDET_RESULT_TYPE_FS) {
			color = COLOR_RED;
		} else {
			color = COLOR_BLUE;
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
				mem2[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[6]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[3]; k++) {
				mem2[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[0]; k <= th[1]; k++) {
				mem2[j * vinfo.xres + k] = color;
			}
		}

		for (j = th[4]; j <= th[7]; j++) {
			for (k = th[2]; k <= th[3]; k++) {
				mem2[j * vinfo.xres + k] = color;
			}
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
