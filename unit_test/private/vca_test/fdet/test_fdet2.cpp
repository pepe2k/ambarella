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
#include <getopt.h>
#include "pgm.h"
#include "fb16.h"
#include "iav.h"
#include "fdet.h"

struct option long_options[] = {
	{"video",	0,	0,	'v'},
	{"policy",	1,	0,	'p'},
	{"classifier",	1,	0,	'c'},
	{"input",	1,	0,	'i'},
	{"output",	1,	0,	'o'},
	{0,		0,	0,	 0 },
};

const char *short_options = "vp:c:i:o:";

int		video_mode	= 0;
int		policy		= 0;
int		sv_pgm		= 0;
char		cls_binary[128]	= "/usr/local/bin/vca_data/fdet/classifiers/default.cls";
char		input_pgm[128]	= "/usr/local/bin/vca_data/fdet/samples/1.pgm";
char		output_pgm[128]	= "/tmp/mmcblk0p1/face.pgm";
char		*buf, *_buf;

struct fdet_configuration	fdet_cfg;
struct fdet_face		faces[32];
unsigned int			face_num;

int parse_parameters(int argc, char **argv)
{
	int	c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'v':
			video_mode = 1;
			break;

		case 'p':
			sscanf(optarg, "%x", &policy);
			break;

		case 'c':
			strcpy(cls_binary, optarg);
			break;

		case 'i':
			strcpy(input_pgm, optarg);
			break;

		case 'o':
			sv_pgm = 1;
			strcpy(output_pgm, optarg);
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			break;

		}
	}

	return 0;
}

int fdet_init_still(struct fdet_configuration *cfg)
{
	int		ret;
	pgm_header_t	header;
	char		*data;

	ret = load_pgm(input_pgm, &header, &data);
	if (ret < 0) {
		return ret;
	}

	ret = load_still(data, header.width * header.height);
	if (ret < 0) {
		return ret;
	}

	render_frame_gray(data, header.width, header.height);
	memcpy(buf, data, header.width * header.height);
	free(data);

	cfg->input_width	= header.width;
	cfg->input_height	= header.height;
	cfg->input_pitch	= header.width;
	cfg->input_source	= 0;
	cfg->policy			= (enum fdet_filtering_policy)policy;
	cfg->input_mode		= FDET_MODE_STILL;

	return 0;
}

int fdet_init_video(struct fdet_configuration *cfg)
{
	int		ret;

	ret = init_iav();
	if (ret < 0) {
		return -1;
	}

	ret = get_second_buf_size(&cfg->input_width, &cfg->input_height, &cfg->input_pitch);
	if (ret < 0) {
		return -1;
	}
	ret = get_second_buf_offset(buf, &cfg->input_offset);
	if (ret < 0) {
		return -1;
	}
	cfg->input_source	= 1;
	cfg->policy			= (enum fdet_filtering_policy)policy;
	cfg->input_mode		= FDET_MODE_VIDEO;

	return 0;
}

int save_faces(struct fdet_face *faces, unsigned num)
{
	static unsigned int		index = 0;
	unsigned int			i, j, k;
	unsigned int			xl, xr, yl, yh, w, h;
	pgm_header_t			header;
	char					pgm_file[128];

	if (!sv_pgm) {
		return 0;
	}

	for (i = 0; i < num; i++) {
		xl	= faces[i].x - faces[i].size / 2;
		xr	= faces[i].x + faces[i].size / 2;
		yl	= faces[i].y - faces[i].size / 2;
		yh	= faces[i].y + faces[i].size / 2;
		w	= xr - xl + 1;
		h	= yh - yl + 1;
		for (j = yl; j <= yh; j++) {
			k = j - yl;
			memcpy(&_buf[k * w], &buf[j * fdet_cfg.input_width + xl], w);
		}
		header.format	= PGM_FORMAT_P5;
		header.width	= w;
		header.height	= h;
		header.max_value= 255;
		sprintf(pgm_file, "%s/face_%03d.pgm", output_pgm, index + i);
		save_pgm(pgm_file, &header, _buf);
	}
	index += num;

	return 0;
}

int main(int argc, char **argv)
{
	int				ret = 0;

	buf = (char *)malloc(720 * 480 * 2);
	if (!buf) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}
	_buf = buf + 720 * 480;

	ret = parse_parameters(argc, argv);
	if (ret < 0) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return ret;
	}

	ret = load_classifier(cls_binary);
	if (ret < 0) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return ret;
	}

	ret = open_fb();
	if (ret < 0) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return ret;
	}

	if (!video_mode) {
		ret = fdet_init_still(&fdet_cfg);
	} else {
		ret = fdet_init_video(&fdet_cfg);
	}
	if (ret < 0) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return ret;
	}

	fdet_start(&fdet_cfg);
	if (ret < 0) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return ret;
	}

	while (1) {
		ret = fdet_get_faces(faces, &face_num);
		if (ret < 0) {
			printf("%s %d: Error!\n", __func__, __LINE__);
			return ret;
		}

		if (video_mode) {
			blank_fb();
		}

		ret = annotate_faces(faces, face_num);
		if (ret < 0) {
			printf("%s %d: Error!\n", __func__, __LINE__);
			return ret;
		}

		ret = save_faces(faces, face_num);
		if (ret < 0) {
			printf("%s %d: Error!\n", __func__, __LINE__);
			return ret;
		}

		if (!video_mode) {
			break;
		}

		ret = fdet_track_faces(buf);
		if (ret < 0) {
			printf("%s %d: Error!\n", __func__, __LINE__);
			return ret;
		}
	}

	close_fb();
	free(buf);

	return 0;
}
