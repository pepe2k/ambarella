/*
 * udecutil.c
 *
 * History:
 *	2013/03/31 - [Zhi He] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>

#include "udecutil.h"

static int verbose = 0;

 //utils functions
 #if 0
static long long get_tick(void)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	static struct timeval start_tv;
	static int started;
	struct timeval tv;
	long long rval;

	pthread_mutex_lock(&mutex);

	if (!started) {
		started = 1;
		gettimeofday(&start_tv, NULL);
		rval = 0;
	} else {
		gettimeofday(&tv, NULL);
		rval = (1000000LL * (tv.tv_sec - start_tv.tv_sec) + (tv.tv_usec - start_tv.tv_usec)) / 1000;
	}

	pthread_mutex_unlock(&mutex);

	return rval;
}
 #endif

int u_printf(const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	//long long ticks;

	//ticks = get_tick();

	pthread_mutex_lock(&mutex);

	//printf("%lld  ", ticks);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int u_printf_index(unsigned index, const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	//long long ticks;

	//ticks = get_tick();

	pthread_mutex_lock(&mutex);

	//printf("%lld [%d] ", ticks, index);
	printf("[%d]:", index);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int v_printf(const char *fmt, ...)
{
	va_list args;
	int rval;
	if (verbose == 0)
		return 0;
	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);
	return rval;
}

static void mem2leword(unsigned char *mem, unsigned int *w)
{
	unsigned char data[4];

	data[0] = *(mem++);
	data[1] = *(mem++);
	data[2] = *(mem++);
	data[3] = *(mem++);

	*w = (data[3] << 24) |
	     (data[2] << 16) |
	     (data[1] <<  8) |
	     (data[0]);
}

static void mem2lehfword(unsigned char *mem, unsigned short *hfw)
{
	unsigned char data[2];

	data[0] = *(mem++);
	data[1] = *(mem++);

	*hfw = (data[1] <<  8) |
	       (data[0]);
}

void release_bmp_file(bmp_t *bmp)
{
	u_assert(bmp);
	if (bmp) {
		if (bmp->buf) {
			free(bmp->buf);
			bmp->buf = NULL;
		}

		if (bmp->display_buf) {
			free(bmp->display_buf);
			bmp->display_buf = NULL;
		}

		if (bmp->fd) {
			fclose(bmp->fd);
			bmp->fd = NULL;
		}
	}
}

int open_bmp_file(char* filename, bmp_t *bmp)
{
	unsigned int w;
	unsigned short hfw;
	unsigned char* bmp_addr = NULL;

	u_assert(filename);
	u_assert(bmp);

	if (filename && bmp) {
		bmp->fd = fopen(filename, "rb");
		if (bmp->fd) {
			fseek(bmp->fd, 0L, SEEK_END);
			bmp->filesize = ftell(bmp->fd);
			fseek(bmp->fd, 0L, SEEK_SET);
			u_printf("bmp file size %d\n", bmp->filesize);

			u_assert(!bmp->buf);
			bmp->buf = (unsigned char*) malloc(bmp->filesize + 4);
			u_assert(bmp->buf);

			if (bmp->buf) {
				fread(bmp->buf, 1, bmp->filesize, bmp->fd);
				bmp_addr = bmp->buf;
				mem2lehfword(bmp_addr, &hfw);
				bmp_addr += 2;
				bmp->header.fheader.id = hfw;
				if (bmp->header.fheader.id != 0x4d42) {
					u_printf_error("The file is not bmp file!");
					return (-5);
				}

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.fheader.file_size = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.fheader.reserved = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.fheader.offset = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.bmp_header_size = w;
				if (bmp->header.iheader.bmp_header_size != 0x28) {
					if (bmp->header.iheader.bmp_header_size == 0x0c)
						u_printf_error("It's OS/2 1.x info header size!");
					else if (bmp->header.iheader.bmp_header_size == 0xf0)
						u_printf_error("It's OS/2 2.x info header size!");
					else
						u_printf_error("Unknown bmp info header size!");

					u_printf_error("It's not Microsoft bmp info header size!");
					return (-6);
				}

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.width = w;
				if (bmp->header.iheader.width < 0) {
					u_printf_error("BMP width < 0!");
					return (-7);
				}

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.height = w;
				if (bmp->header.iheader.height < 0) {
					u_printf_error("BMP height < 0!");
					return (-8);
				}

				mem2lehfword(bmp_addr, &hfw);
				bmp_addr += 2;
				bmp->header.iheader.planes = hfw;

				mem2lehfword(bmp_addr, &hfw);
				bmp_addr += 2;
				bmp->header.iheader.bits_per_pixel = hfw;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.compression = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.bmp_data_size = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.h_resolution = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.v_resolution = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.used_colors = w;

				mem2leword(bmp_addr, &w);
				bmp_addr += 4;
				bmp->header.iheader.important_colors = w;

				bmp->bmp_data = bmp->buf + bmp->header.fheader.offset;
				bmp->bmp_clut = bmp_addr;

				bmp->width = bmp->header.iheader.width;
				/* BMP pitch need be the mutilple of 4 */
				if ((bmp->width % 4) != 0)
					bmp->width = ((bmp->header.iheader.width / 4) + 1) * 4;

				bmp->height = bmp->header.iheader.height;
				bmp->pitch  = bmp->width;
				bmp->size   = bmp->header.iheader.bmp_data_size;
			} else {
				u_printf_error("no memory, request size %d\n", bmp->filesize);
				return (-3);
			}
		} else {
			u_printf_error("open bmp file(%s) fail\n", filename);
			return (-2);
		}
	} else {
		u_printf_error("NULL input file %p, or NULL bmp %p\n", filename, bmp);
		return (-1);
	}

	return 0;
}

static void paint_rgb565(bmp_t *bmp, unsigned short* p, unsigned int x, unsigned int y)
{
	unsigned int r, g, b, o;
	unsigned char* p0;
	unsigned char* p1;

	if (8 == bmp->header.iheader.bits_per_pixel) {
		u_assert(bmp->bmp_clut);
		p0 = bmp->bmp_data + (y * bmp->width + x);
		o = (*p0) * 4;
		p1 = bmp->bmp_clut + o;
		r = p1[2];
		g = p1[1];
		b = p1[0];

		*p = ((r >> 3) << 11) | ((g >> 2) << 5) | (b & 0x1f);
	} else if (16 == bmp->header.iheader.bits_per_pixel) {
		*p = *((unsigned short *)(bmp->bmp_data + (y * bmp->width + x) * 2));
	} else if (24 == bmp->header.iheader.bits_per_pixel) {
		p1 = bmp->bmp_data + (y * bmp->width + x) * 3;
		r = p1[2];
		g = p1[1];
		b = p1[0];

		*p = ((r >> 3) << 11) | ((g >> 2) << 5) | (b & 0x1f);
	} else if (32 == bmp->header.iheader.bits_per_pixel) {
		p1 = bmp->bmp_data + (y * bmp->width + x) * 4;
		r = p1[2];
		g = p1[1];
		b = p1[0];

		*p = ((r >> 3) << 11) | ((g >> 2) << 5) | (b & 0x1f);
	} else {
		u_printf_error("not support bits_per_pixel %d by now!\n", bmp->header.iheader.bits_per_pixel);
	}

}

int bmf_file_to_display_buffer(bmp_t *bmp, unsigned int target_width, unsigned int target_height, unsigned int target_format)
{
	unsigned int i, j;
	unsigned int map_c, map_r;

	u_assert(bmp);
	u_assert(target_width);
	u_assert(target_height);

	u_assert(bmp->width);
	u_assert(bmp->height);
	u_assert(bmp->bmp_data);
	u_assert(bmp->buf);

	if (bmp && target_width && target_height) {
		unsigned short* p_rgb565;
		//free previous
		if (bmp->display_buf) {
			free(bmp->display_buf);
			bmp->display_buf = NULL;
		}

		if (e_buffer_format_rgb565 == target_format) {
			bmp->display_buf_size = target_width * target_height * 2;
			bmp->display_buf = malloc(bmp->display_buf_size);
			if (bmp->display_buf) {
				bmp->target_width = target_width;
				bmp->target_height = target_height;

				p_rgb565 = (unsigned short *)bmp->display_buf;
				for (j = 0; j < target_height; j ++) {
					map_r = (bmp->height * j) / target_height;
					for (i = 0; i < target_width; i ++) {
						map_c = (bmp->width * i) / target_width;
						paint_rgb565(bmp, p_rgb565, map_c, map_r);
						p_rgb565 ++;
					}
				}
			} else {
				u_printf_error("no memory, request size %d\n", bmp->display_buf_size);
				return (-4);
			}
		} else {
			u_printf_error("not supported format %d\n", target_format);
			return (-5);
		}
	} else {
		u_printf_error("NULL bmp %p, or zero width %d, height %d\n", bmp, target_width, target_height);
		return (-3);
	}

	return 0;
}

void set_end_of_string(char* string, char end, unsigned int max_length)
{
	unsigned int i = 0;
	while (i < max_length) {
		if (string[i] == end) {
			string[i] = 0x0;
			return;
		}
		i ++;
	}
}

