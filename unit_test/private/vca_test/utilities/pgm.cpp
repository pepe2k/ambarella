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
#include "pgm.h"

int load_pgm(const char *file, pgm_header_t *header, char **data)
{
	int		ret = 0;
	FILE		*f;
	unsigned int	fmt, w, h, v, offset, sz;
	char		*d;

	f = fopen(file, "r");
	if (!f) {
		ret = -1;
		goto load_pgm_exit;
	}
	ret = fscanf(f, "P%d %d %d %d", &fmt, &w, &h, &v);
	if (ret != 4) {
		ret = -1;
		goto load_pgm_exit;
	}

	if (fmt != 5) {
		ret = -1;
		goto load_pgm_exit;
	}
	if (v != 255) {
		ret = -1;
		goto load_pgm_exit;
	}

	header->format		= PGM_FORMAT_P5;
	header->width		= w;
	header->height		= h;
	header->max_value	= v;

	sz = w * h;
	d = (char *)malloc(sz);
	if (!d) {
		ret = -1;
		goto load_pgm_exit;
	}

	offset = ftell(f);
	fclose(f);
	f = fopen(file, "rb");
	if (!f) {
		ret = -1;
		goto load_pgm_exit;
	}

	ret = fseek(f, offset + 1, SEEK_SET);
	if (ret) {
		ret = -1;
		goto load_pgm_exit;
	}

	ret = fread(d, 1, sz, f);
	if (ret != (int)sz) {
		ret = -1;
		goto load_pgm_exit;
	}

	*data	= d;
	ret	= 0;

load_pgm_exit:
	if (f) {
		fclose(f);
	}
	return ret;
}

int save_pgm(const char *file, const pgm_header_t *header, char *data)
{
	int		ret = 0;
	unsigned int	sz;
	FILE		*f;

	f = fopen(file, "w");
	if (!f) {
		ret = -1;
		goto save_pgm_exit;
	}

	fprintf(f, "P5 %u %u 255 ", header->width, header->height);
	fclose(f);

	f = fopen(file, "ab+");
	if (!f) {
		ret = -1;
		goto save_pgm_exit;
	}

	sz = header->width * header->height;
	ret = fwrite(data, 1, sz, f);
	if (ret != (int)sz) {
		ret = -1;
		goto save_pgm_exit;
	}

save_pgm_exit:
	if (f) {
		fclose(f);
	}
	return ret;
}
