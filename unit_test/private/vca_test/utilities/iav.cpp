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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <basetypes.h>
#include <iav_drv.h>
#include "iav.h"

static int							fd;
iav_mmap_info_t						m_info;
struct iav_yuv_buffer_info_ex_s		second_info;
struct iav_me1_buffer_info_ex_s		me1_info;

int init_iav(void)
{
	fd = open("/dev/iav", O_RDWR);
	if (!fd) {
		return -1;
	}

	return ioctl(fd, IAV_IOC_MAP_DSP, &m_info);
}

int get_second_buf_size(int *w, int *h, int *p)
{
	int		ret;

	memset((void *)&second_info, 0, sizeof(second_info));
	second_info.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;

	ret = ioctl(fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &second_info);
	if (ret < 0) {
		return -1;
	}

	*w = second_info.width;
	*h = second_info.height;
	*p = second_info.pitch;

	return 0;
}

int get_second_y_buf(char *buf)
{
	int				ret;
	unsigned int	i;
	char			*y;

	memset((void *)&second_info, 0, sizeof(second_info));
	second_info.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;

	ret = ioctl(fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &second_info);
	if (ret < 0) {
		return -1;
	}

	y = (char *)second_info.y_addr;
	for (i = 0; i < second_info.height; i++) {
		memcpy(buf, y, second_info.width);
		buf	+= second_info.width;
		y	+= second_info.pitch;

	}

	return 0;
}

int get_second_rgb_buf(char *buf)
{
	int				ret;
	unsigned int	i, j;
	char			*y, *uv;
	int				_r, _g, _b, _y, _u, _v;

	memset((void *)&second_info, 0, sizeof(second_info));
	second_info.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;

	ret = ioctl(fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &second_info);
	if (ret < 0) {
		return -1;
	}

	y	= (char *)second_info.y_addr;
	uv	= (char *)second_info.uv_addr;
	for (i = 0; i < second_info.height; i++) {
		for (j = 0; j < second_info.width; j++) {
			_y	= y[j];
			_u	= uv[j / 2];
			_v	= uv[j / 2 + 1];

			_r	= (1000 * _y + 1402 * (_v - 128)) / 1000;
			_g	= (100000 * _y - 34413 * (_u - 128) - 71414 * (_v - 128)) / 100000;
			_b	= (1000 * _y + 1772 * (_u - 128)) / 1000;

			buf[3 * j + 0] = _b;
			buf[3 * j + 1] = _g;
			buf[3 * j + 2] = _r;
		}
		buf	+= 3 * second_info.width;
		y	+= second_info.pitch;
		uv	+= second_info.pitch / 2;
	}

	return 0;
}

int get_second_buf_offset(char *buf, unsigned int *offset)
{
	int				ret;
	unsigned int	i;
	char			*y;

	memset((void *)&second_info, 0, sizeof(second_info));
	second_info.source = IAV_ENCODE_SOURCE_SECOND_BUFFER;

	ret = ioctl(fd, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &second_info);
	if (ret < 0) {
		return -1;
	}

	y = (char *)second_info.y_addr;
	for (i = 0; i < second_info.height; i++) {
		memcpy(buf, y, second_info.width);
		buf	+= second_info.width;
		y	+= second_info.pitch;

	}
	*offset = second_info.y_addr - m_info.addr;
	return 0;
}

int get_me1_buf_size(int *w, int *h, int *p)
{
	int		ret;

	memset((void *)&me1_info, 0, sizeof(me1_info));

	ret = ioctl(fd, IAV_IOC_READ_ME1_BUFFER_INFO_EX, &me1_info);
	if (ret < 0) {
		return -1;
	}

	*w = me1_info.width;
	*h = me1_info.height;
	*p = me1_info.pitch;

	return 0;
}

int get_me1_buf(char *buf)
{
	int				ret;
	unsigned int	i;
	char			*y;

	memset((void *)&me1_info, 0, sizeof(me1_info));

	ret = ioctl(fd, IAV_IOC_READ_ME1_BUFFER_INFO_EX, &me1_info);
	if (ret < 0) {
		return -1;
	}

	y = (char *)me1_info.addr;
	for (i = 0; i < me1_info.height; i++) {
		memcpy(buf, y, me1_info.width);
		buf	+= me1_info.width;
		y	+= me1_info.pitch;
	}

	return 0;
}

int exit_iav(void)
{
	if (fd >= 0) {
		close(fd);
	}

	return 0;
}
