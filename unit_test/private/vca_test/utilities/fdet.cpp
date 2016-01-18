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
#include "ambas_fdet.h"
#include "iav.h"

static int fd;

int load_classifier(const char *cls)
{
	FILE		*f;
	char		*binary;
	int		ret, sz;

	fd = open("/dev/fdet", O_RDWR);
	if (fd < 0) {
		return -1;
	}

	f = fopen(cls, "rb");
	if (!f) {
		return -1;
	}

	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	ret = ioctl(fd, FDET_IOCTL_SET_MMAP_TYPE, FDET_MMAP_TYPE_CLASSIFIER_BINARY);
	if (ret < 0) {
		return -1;
	}

	binary = (char *)mmap(NULL, sz, PROT_WRITE, MAP_SHARED, fd, 0);
	if (!binary) {
		return -1;
	}

	ret = fread(binary, 1, sz, f);
	if (ret != sz) {

		return -1;
	}

	fclose(f);
	munmap(binary, sz);

	return 0;
}

int load_still(const char *d, unsigned int sz)
{
	int		ret;
	char		*map;

	ret = ioctl(fd, FDET_IOCTL_SET_MMAP_TYPE, FDET_MMAP_TYPE_ORIG_BUFFER);
	if (ret < 0) {
		return -1;
	}

	map = (char *)mmap(NULL, sz, PROT_WRITE, MAP_SHARED, fd, 0);
	if (!map) {
		return -1;
	}

	memcpy(map, d, sz);

	return 0;
}

int fdet_start(struct fdet_configuration *cfg)
{
	int		ret;

	ret = ioctl(fd, FDET_IOCTL_SET_CONFIGURATION, cfg);
	if (ret < 0) {
		return -1;
	}

	ret = ioctl(fd, FDET_IOCTL_START, NULL);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int fdet_get_faces(struct fdet_face *faces, unsigned int *num)
{
	int		ret;

	ret = ioctl(fd, FDET_IOCTL_GET_RESULT, faces);
	if (ret < 0) {
		return -1;
	}

	*num = ret;
	return 0;
}

int fdet_track_faces(char *buf)
{
	int		ret;
	unsigned int	offset;

	ret = get_second_buf_offset(buf, &offset);
	if (ret) {
		return ret;
	}

	ret = ioctl(fd, FDET_IOCTL_TRACK_FACE, offset);
	if (ret) {
		return -1;
	} else {
		return 0;
	}
}
