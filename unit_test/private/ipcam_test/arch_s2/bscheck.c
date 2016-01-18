/*
 * bscheck.c
 * The program can read  the mode of bitstream from file.
 *
 * History:
 *    2014/3/24 - [Lei Hong] add read the mode of bitstream.
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
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
#include <pthread.h>



/* the device file handle*/
int fd_iav;
static char default_filename_svc[256] = {0};


#define NO_ARG		0
#define HAS_ARG		1

#define STREAM_FILE_SIZE  	(1*1024*1024)
#define NAL_HEAD_MAGIC_SIZE 3
#define NON_IDR_FLAG   		1
#define IDR_FLAG			0
#define NON_IDR_MAX_NUM	   	31
#define AMBA_SVC_LEVEL4		4
#define AMBA_SVC_LEVEL3		3
#define AMBA_SVC_LEVEL2    	2
#define S_AVC_MODE    		1
#define NON_AMBA_SVC		0
#define S_ERROR_MODE  		-1
#define NAL_REF_IDC_BIT		0x60
#define NAL_UNIT_TYPE_BIT	0x1f
#define NAL_REF_IDC_OFFSET  5
#define FILENAME_SIZE		256

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{0, 0, 0, 0}
};

static const char *short_options = "f:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\tset stream mode filename"},
};

void usage(void)
{
	int i;
	printf("bscheck usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
			case 'f':
				strcpy(default_filename_svc, optarg);
				break;
			default:
				printf("unknown command %s \n", optarg);
				return -1;
				break;
		}
	}

	return 0;
}


/*
*description: to distinguish between SVC and AVC mode of stream file
*mode: -1: ERROR; 0:NON AMBA SVC; 1:AVC; 2: AMBA SVC level 2;
		3: AMBA SVC level 3;  4:AMBA SVC level 4.
*author: Lei Hong
*date: 2014/03/24
*/
int read_stream_mode()
{
	int i = 0;
	int count = 0;
	int remain = 0;
	unsigned int length = 0;
	int non_idr_idc_index = 0;
	int nal_ref_idc = 0;
	int nal_unit_type = 0;
	int idr_begin = 0;
	int ret = S_ERROR_MODE;
	FILE* pfile = NULL;
	unsigned char *ps = NULL;
	unsigned char *ps_start = NULL;
	unsigned char *ps_end = NULL;
	int non_idr_idc[NON_IDR_MAX_NUM] = {0};
	char compare_filename[FILENAME_SIZE] = {0};
	unsigned char nal_head_magic[NAL_HEAD_MAGIC_SIZE] = {0x0,0x0,0x1};
	int amba_svc_level2[NON_IDR_MAX_NUM] = {
		0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,
		3,0,3,0,3,0,3,0,3,0,3,0,3,0};
	int amba_svc_level3[NON_IDR_MAX_NUM] = {
		0,2,0,3,0,2,0,3,0,2,0,3,0,2,0,3,0,
		2,0,3,0,2,0,3,0,2,0,3,0,2,0};
	int amba_svc_level4[NON_IDR_MAX_NUM] = {
		0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,3,0,
		1,0,2,0,1,0,3,0,1,0,2,0,1,0};

	if (0 == memcmp(default_filename_svc,compare_filename,FILENAME_SIZE)) {
		return S_ERROR_MODE;
	}

	pfile = fopen(default_filename_svc, "rb");

	if (!pfile) {
		printf("stream_mode cannot open input file(%s).\n", default_filename_svc);
		return S_ERROR_MODE;
	}

	fseek(pfile, 0L, SEEK_END);
	length = ftell(pfile);

	if (length <= (4*(NAL_HEAD_MAGIC_SIZE + 1))) {
		ret = S_ERROR_MODE;
		fclose(pfile);
		printf("stream file length is too small, length is %d. \n", length);
		return ret;
	}

	remain = length%STREAM_FILE_SIZE;
	count = length/STREAM_FILE_SIZE;
	ps_start = (unsigned char *)malloc(STREAM_FILE_SIZE);

	if (NULL == ps_start) {
		ret = S_ERROR_MODE;
		fclose(pfile);
		printf("stream_mode cannot alloc buffer. \n");
		return ret;
	}

	ps_end = ps_start + STREAM_FILE_SIZE - NAL_HEAD_MAGIC_SIZE;

	if (count > 0) {
		for (i = 0; i < count; i++) {
			ps = ps_start;
			memset(ps_start,0x0,STREAM_FILE_SIZE);
			fseek(pfile, i*(STREAM_FILE_SIZE - NAL_HEAD_MAGIC_SIZE), SEEK_SET);
			fread(ps, 1, STREAM_FILE_SIZE, pfile);
			while (ps < ps_end) {
				if (non_idr_idc_index >= NON_IDR_MAX_NUM) {
					goto IDC_COMPARE;
				}

				if (0 == memcmp(ps,nal_head_magic,NAL_HEAD_MAGIC_SIZE)) {
					nal_ref_idc = (int)((*(ps + NAL_HEAD_MAGIC_SIZE) &
						NAL_REF_IDC_BIT) >> NAL_REF_IDC_OFFSET);
					nal_unit_type = (int)(*(ps + NAL_HEAD_MAGIC_SIZE) &
						NAL_UNIT_TYPE_BIT);
					ps = ps + NAL_HEAD_MAGIC_SIZE + 1;

					if (IDR_FLAG == (nal_unit_type % 5)) {
						if (idr_begin) {
							idr_begin = 0;
							goto IDC_COMPARE;
						} else {
							idr_begin = 1;
							continue;
						}
					}

					if (idr_begin) {
						if (NON_IDR_FLAG == nal_unit_type) {
							non_idr_idc[non_idr_idc_index] = nal_ref_idc;
							non_idr_idc_index++;
							printf("stream file nal_ref_idc is %d,"
								"nal_unit_type is %d.\n",
								nal_ref_idc,nal_unit_type);
						}
					}
				} else {
					ps++;
				}
			}
		}

		remain = remain + count*NAL_HEAD_MAGIC_SIZE;

		/*if remain size larger than STREAM_FILE_SIZE, I will ingore the data
		which size exceeds STREAM_FILE_SIZE*/

		if (remain >= 1) {
			memset(ps_start,0x0,remain);
			ps = ps_start;
			fseek(pfile, count*(STREAM_FILE_SIZE - NAL_HEAD_MAGIC_SIZE), SEEK_SET);
			if (remain > STREAM_FILE_SIZE) {
				fread(ps, 1, STREAM_FILE_SIZE - NAL_HEAD_MAGIC_SIZE, pfile);
				ps_end = ps + STREAM_FILE_SIZE - NAL_HEAD_MAGIC_SIZE;
			} else {
				fread(ps, 1, remain, pfile);
				ps_end = ps + remain - NAL_HEAD_MAGIC_SIZE;
			}

			while (ps < ps_end) {
				if (non_idr_idc_index >= NON_IDR_MAX_NUM) {
					goto IDC_COMPARE;
				}

				if (0 == memcmp(ps, nal_head_magic, NAL_HEAD_MAGIC_SIZE)) {
					nal_ref_idc = (int)((*(ps + NAL_HEAD_MAGIC_SIZE) &
						NAL_REF_IDC_BIT) >> NAL_REF_IDC_OFFSET);
					nal_unit_type = (int)(*(ps + NAL_HEAD_MAGIC_SIZE) &
						NAL_UNIT_TYPE_BIT);
					ps = ps + NAL_HEAD_MAGIC_SIZE + 1;
					if (IDR_FLAG == (nal_unit_type % 5)) {
						if (idr_begin) {
							idr_begin = 0;
							goto IDC_COMPARE;
						} else {
							idr_begin = 1;
							continue;
						}
					}
					if (idr_begin) {
						if (NON_IDR_FLAG == nal_unit_type) {
							non_idr_idc[non_idr_idc_index] = nal_ref_idc;
							non_idr_idc_index++;
							printf("stream file nal_ref_idc is %d,"
								"nal_unit_type is %d.\n",
								nal_ref_idc,nal_unit_type);
						}
					}
				} else {
					ps++;
				}
			}
		}
	} else {
		memset(ps_start,0x0,remain);
		fseek(pfile, 0, SEEK_SET);
		fread(ps_start, 1, remain, pfile);
		ps = ps_start;
		ps_end = ps_start + remain - NAL_HEAD_MAGIC_SIZE;

		while (ps < ps_end) {
			if (non_idr_idc_index >= NON_IDR_MAX_NUM) {
				goto IDC_COMPARE;
			}

			if (0 == memcmp(ps,nal_head_magic,NAL_HEAD_MAGIC_SIZE)) {
				nal_ref_idc = (int)((*(ps + NAL_HEAD_MAGIC_SIZE) &
					NAL_REF_IDC_BIT) >> NAL_REF_IDC_OFFSET);
				nal_unit_type = (int)(*(ps + NAL_HEAD_MAGIC_SIZE) &
					NAL_UNIT_TYPE_BIT);
				ps = ps + NAL_HEAD_MAGIC_SIZE + 1;

				if (IDR_FLAG == (nal_unit_type % 5)) {
					if (idr_begin) {
						idr_begin = 0;
						goto IDC_COMPARE;
					} else {
						idr_begin = 1;
						continue;
					}
				}

				if (idr_begin) {
					if (NON_IDR_FLAG == nal_unit_type) {
						non_idr_idc[non_idr_idc_index] = nal_ref_idc;
						non_idr_idc_index++;
						printf("stream file nal_ref_idc is %d,"
							"nal_unit_type is %d.\n",
							nal_ref_idc,nal_unit_type);
					}
				}
			} else {
				ps++;
			}
		}
	}

IDC_COMPARE:

	if (non_idr_idc_index <= 1) {
		ret = S_ERROR_MODE;
		printf("stream_mode is error. \n");
		goto MODE_RET;
	}

	for (i = 0; i < non_idr_idc_index - 1; i++) {
		if (non_idr_idc[i] != non_idr_idc[i + 1]) {
			break;
		}
	}

	if (i == (non_idr_idc_index - 1)) {
		ret = S_AVC_MODE;
		printf("stream_mode is avc, non-idr num is %d.\n", non_idr_idc_index);
		goto MODE_RET;
	}

	if (0 == memcmp(non_idr_idc, amba_svc_level2, non_idr_idc_index)) {
		ret = AMBA_SVC_LEVEL2;
		printf("stream_mode is amba svc_lev2, non-idr num is %d.\n", non_idr_idc_index);
	} else if (0 == memcmp(non_idr_idc, amba_svc_level3, non_idr_idc_index)) {
		ret = AMBA_SVC_LEVEL3;
		printf("stream_mode is amba svc_lev3, non-idr num is %d.\n", non_idr_idc_index);
	} else if(0 == memcmp(non_idr_idc, amba_svc_level4, non_idr_idc_index)) {
		ret = AMBA_SVC_LEVEL4;
		printf("stream_mode is amba svc_lev4, non-idr num is %d.\n", non_idr_idc_index);
	} else {
		ret = NON_AMBA_SVC;
		printf("stream_mode is non amba svc, non-idr num is %d.\n", non_idr_idc_index);
	}

MODE_RET:
	fclose(pfile);
	free(ps_start);
	return ret;
}

int main(int argc, char **argv)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	read_stream_mode();

	return 0;
}

