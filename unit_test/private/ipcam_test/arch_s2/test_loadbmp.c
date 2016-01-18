/*
 * test_overlay.c
 *
 * History:
 * 2010/3/2 - [Louis Sun] created for A5s
 * 2010/4/15 - [Luo Fei] modified for multistream
 * 2010/7/28 - [Jian Tang] add 3 areas for each stream
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
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
#include <signal.h>


#define MAX_ENCODE_STREAM_NUM		(4)
#define MAX_OVERLAY_AREA_NUM		(3)
#define OVERLAY_CLUT_SIZE			(1024)
#define OVERLAY_CLUT_OFFSET			(0)
#define OVERLAY_YUV_OFFSET			(12*1024)


int fd_overlay;
iav_mmap_info_t overlay_info;
iav_mmap_info_t alpha_mask_info;
int overlay_size;
u8 * overlay_data;

static int current_stream = -1;		// -1 is a invalid stream, for initialize data only
static int current_area = -1;		// -1 is a invalid area, for initialize data only
static int disable_osd[MAX_ENCODE_STREAM_NUM];
static int flag[MAX_ENCODE_STREAM_NUM];
static int trans_limit = 0;

//bmp file header  14bytes
typedef struct tagBITMAPFILEHEADER
{
u16	bfType;			// file type
u32	bfSize;			//file size
u16	bfReserved1;
u16	bfReserved2;
u32	bfOffBits;
}BITMAPFILEHEADER;
//bmp inforamtion header 40bytes
typedef struct tagBITMAPINFOHEADER
{
u32	biSize;
u32	biWidth;				//bmp width
u32        biHeight;		//bmp height
u16        biPlanes;
u16        biBitCount;		// 1,4,8,16,24 ,32 color attribute
u32        biCompression;
u32        biSizeImage;		//Image size
u32        biXPelsPerMerer;
u32        biYPelsPerMerer;
u32        biClrUsed;
u32        biClrImportant;
}BITMAPINFOHEADER;

//clut, if 256 bit map it needs,
typedef struct tagRGBQUAD
{
u8        rgbBlue;
u8        rgbGreen;
u8        rgbRed;
u8        rgbReserved;
}RGBQUAD;

typedef struct osd_clut_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;
} osd_clut_t;


static void RGB_To_YUV(const RGBQUAD * rgb, osd_clut_t *yuv)
{
	yuv->y = (u8)(0.257f * rgb->rgbRed + 0.504f * rgb->rgbGreen + 0.098f * rgb->rgbBlue + 16);
	yuv->u = (u8)(0.439f * rgb->rgbBlue - 0.291f * rgb->rgbGreen - 0.148f * rgb->rgbRed + 128);
	yuv->v = (u8)(0.439f * rgb->rgbRed - 0.368f * rgb->rgbGreen - 0.071f * rgb->rgbBlue + 128);
	yuv->alpha = 255;

	if (trans_limit == 0) {
		if ((rgb->rgbRed < 90) && (rgb->rgbBlue < 90) && (rgb->rgbGreen < 90)) {
			yuv->alpha = 128;
		}
		if ((rgb->rgbRed < 20) && (rgb->rgbBlue < 20) && (rgb->rgbGreen < 20)) {
			yuv->alpha = 0;
		}
	} else if (trans_limit  > 0) {
			if ((rgb->rgbRed < trans_limit) && (rgb->rgbBlue < trans_limit) &&
				(rgb->rgbGreen < trans_limit)) {
				yuv->alpha = 128;
			}
		if ((rgb->rgbRed < (trans_limit >> 2)) && (rgb->rgbBlue < (trans_limit >> 2))
			&& (rgb->rgbGreen < (trans_limit >> 2))) {
			yuv->alpha = 0;
		}
	}
}


int map_overlay(void)
{
	if (ioctl(fd_overlay, IAV_IOC_MAP_OVERLAY, &overlay_info) < 0) {
		perror("IAV_IOC_MAP_OVERLAY");
		return -1;
	}
	printf("overlay: start = 0x%p, size = 0x%x\n", overlay_info.addr, overlay_info.length);
	//split into four part
	overlay_size = (overlay_info.length - OVERLAY_YUV_OFFSET) / MAX_ENCODE_STREAM_NUM;

	return 0;
}

int check_for_overlay(void)
{
	iav_state_info_t info;

	if (ioctl(fd_overlay, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	if ((info.state != IAV_STATE_PREVIEW) &&
		(info.state != IAV_STATE_ENCODING)) {
		printf("overlay need iav to be in preview or encoding, cur state is %d.\n", info.state);
		return -1;
	}

	return 0;
}

//static overlay_insert_ex_t overlay_insert;
static overlay_insert_ex_t overlay_insert_arr[MAX_ENCODE_STREAM_NUM];

static int loadbmp_flag = 0;
static char bmp_filename[MAX_ENCODE_STREAM_NUM][MAX_OVERLAY_AREA_NUM][256];

//const static char *default_bmp_filename="/usr/local/bin/Ambarella-256x128-8bit.bmp";

static int  bmp_convert(int stream,int area,u8 * data)
{
	FILE * fp;

	RGBQUAD  rgb;
	int i = 0;
	int clut_id = stream * MAX_OVERLAY_AREA_NUM + area;
	osd_clut_t * clut_data = (osd_clut_t *)(overlay_info.addr + OVERLAY_CLUT_SIZE * clut_id);

	if ((fp = fopen(bmp_filename[stream][area], "r")) == NULL) {
	 	printf ("%s open fail or not exists \n",bmp_filename[stream][area]);
		return -1;
	}
	BITMAPFILEHEADER filehead;
	BITMAPINFOHEADER infohead;
	memset(&filehead, 0, sizeof(BITMAPFILEHEADER));
	memset(&infohead, 0, sizeof(BITMAPINFOHEADER));
	fread(&filehead.bfType, sizeof(filehead.bfType), 1, fp);
	fread(&filehead.bfSize, sizeof(filehead.bfSize), 1, fp);
	fread(&filehead.bfReserved1, sizeof(filehead.bfReserved1), 1, fp);
	fread(&filehead.bfReserved2, sizeof(filehead.bfReserved2), 1, fp);
	fread(&filehead.bfOffBits, sizeof(filehead.bfOffBits), 1, fp);

	if (filehead.bfType != 0x4d42) {
		printf("this is not a bmp file \n");
		fclose(fp);
		return -1;
	}

	fread(&infohead, sizeof(BITMAPINFOHEADER), 1, fp);
	int biW = infohead.biWidth;
	int biH =infohead.biHeight;
	int sizeImg = (infohead.biSizeImage);

	if (infohead.biBitCount > 8) {
		printf("please use 8bit bmp file \n");
		fclose(fp);
		return -1;
	}
//	printf("bfType [%d]\t  bfSize[%d]\t filehead.bfOffBits[%d]\t\n",filehead.bfType,filehead.bfSize,filehead.bfOffBits);
	int color_count=(filehead.bfOffBits - 14 - 40) / sizeof(RGBQUAD);
//	printf("bmp_Width [%d]\t  bmp_height[%d]\t color_count[%d]\t\n",biW,biH,color_count);
	if ((biW & 0x1F) || (biH & 0x3)) {
		fclose(fp);
		printf("the image size %dx%d, width must be multiple of 32, height must be multiple of 4.\n",
			biW, biH);
		return -1;
	}
	overlay_insert_arr[stream].area[area].width = biW;
	overlay_insert_arr[stream].area[area].height = biH;
	overlay_insert_arr[stream].area[area].pitch = biW;
	overlay_insert_arr[stream].area[area].clut_id = clut_id;

	memset(clut_data, 0, OVERLAY_CLUT_SIZE);
	for(i = 0; i < color_count; i++) {
		fread(&rgb, sizeof(RGBQUAD), 1, fp);
		RGB_To_YUV(&rgb, &clut_data[i]);
	}

	memset(data, 0, sizeImg);
	for(i = 0; i < biH; i++) {
		fread(data + (biH - 1 - i)*biW, 1, biW, fp);
	}
	fclose(fp);

	return 0;
}


int set_overlay(int stream)
{
	int i;
	int enable;
	int total_size;
	u8 * data = NULL;

	/*select overlay buffer*/
	/*   |--OVERLAY_YUV_OFFSET--|--stream 0--|--stream 1--|--stream 2--|--stream 3--|  */
	overlay_data = overlay_info.addr + OVERLAY_YUV_OFFSET + overlay_size*stream;
	printf("overlay [stream %d]: start = 0x%p, size = 0x%x\n", stream, overlay_data, overlay_size);

	total_size = enable = 0;
	for (i = 0; i <MAX_OVERLAY_AREA_NUM; ++i) {
		//prepare for overlay insert
		if (disable_osd[stream]) {
			overlay_insert_arr[stream].area[i].enable = !disable_osd[stream];
		}
		if (overlay_insert_arr[stream].area[i].enable) {

			data = overlay_data + total_size;
			bmp_convert(stream,i,data);
			overlay_insert_arr[stream].area[i].total_size = overlay_insert_arr[stream].area[i].pitch *
				overlay_insert_arr[stream].area[i].height;
			total_size += overlay_insert_arr[stream].area[i].total_size;

			/*check buffer overflow*/
			if (total_size > overlay_size) {
				printf("overlay size: total size [%d] of all areas can not be larger than the buffer size [%d]\n",
					total_size, overlay_size);
				return -1;
			}
			printf("stream %d overlay size area [%d]:\n", stream, i);
			printf("\t      width = %d\n", overlay_insert_arr[stream].area[i].width);
			printf("\t     height = %d\n", overlay_insert_arr[stream].area[i].height);
			printf("\t      pitch = %d\n", overlay_insert_arr[stream].area[i].pitch);
			printf("\t 	area size = %d\n", overlay_insert_arr[stream].area[i].total_size);
			printf("\t total size = %d\n", total_size);
			//validate w, h, x, y, p
			if (!overlay_insert_arr[stream].area[i].width ||
				!overlay_insert_arr[stream].area[i].height ||
				!overlay_insert_arr[stream].area[i].pitch) {
				printf("stream %d overlay size error:\n", stream);
				printf("\t      width = %d\n", overlay_insert_arr[stream].area[i].width);
				printf("\t     height = %d\n", overlay_insert_arr[stream].area[i].height);
				printf("\t      pitch = %d\n", overlay_insert_arr[stream].area[i].pitch);
				printf("\t 	area size = %d\n", overlay_insert_arr[stream].area[i].total_size);
				printf("\t total size = %d\n", total_size);
				return -1;
			}
			overlay_insert_arr[stream].area[i].data = data;
		}
		enable |= overlay_insert_arr[stream].area[i].enable;
	}
	overlay_insert_arr[stream].id = (1 << stream);
	overlay_insert_arr[stream].enable = enable;

	if (ioctl(fd_overlay, IAV_IOC_OVERLAY_INSERT_EX, &overlay_insert_arr[stream]) < 0) {
		perror("IAV_IOC_OVERLAY_INSERT");
		return -1;
	}

	return 0;
}

#define NO_ARG	  0
#define HAS_ARG	 1
static struct option long_options[] = {
	{"streamA", NO_ARG, 0, 'A' },   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB", NO_ARG, 0, 'B' },
	{"streamC", NO_ARG, 0, 'C' },
	{"streamD", NO_ARG, 0, 'D' },
	{"area", HAS_ARG, 0, 'a'},
	{"bmp", HAS_ARG, 0, 'L'},
	{"xstart", HAS_ARG, 0, 'x'},
	{"ystart", HAS_ARG, 0, 'y'},
	{"trans", HAS_ARG, 0, 't'},

	//turn it off
	{"disable", NO_ARG, 0, 10},

	{0, 0, 0, 0}
};

static const char *short_options = "ABCDL:a:x:y:t:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\tset stream A"},
	{"", "\tset stream B"},
	{"", "\tset stream C"},
	{"", "\tset stream D"},

	{"0~2", "\tset overlay area num"},
	{"", "\tbmp file to load: 8bit BMP, width must be multiple of 32; height must be multiple of 4"},
	{"", "\tset offset x"},
	{"", "\tset offset y"},
	{"", "\tset trans limit "},

	{"", "\tturn off osd insert"},
};

void usage(void)
{
	int i;

	printf("test_overlay usage:\n");
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

#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while(0)

#define CHECK_STREAMID_EXIST(x) do  { 		\
			if ((x) >=0)  {	\
				printf("stream already set to %d \n", (x));	\
				return -1;		\
			}	\
		}while(0)

#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_OVERLAY_AREA_NUM)) {	\
				printf("area id wrong %d, not in range [0~2].\n", (x));	\
				return -1;	\
			}	\
		} while (0)

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
			case 'A':
					current_stream = 0;
					flag[current_stream] = 1;
					break;
			case 'B':
					current_stream = 1;
					flag[current_stream] = 1;
					break;
			case 'C':
					current_stream = 2;
					flag[current_stream] = 1;
					break;
			case 'D':
					current_stream = 3;
					flag[current_stream] = 1;
					break;
			case 'a':
					current_area = atoi(optarg);
					VERIFY_STREAMID(current_stream);
					VERIFY_AREAID(current_area);
					overlay_insert_arr[current_stream].area[current_area].enable = 1;
					break;
			case 'L':
					VERIFY_STREAMID(current_stream);
					VERIFY_AREAID(current_area);
					strcpy(bmp_filename[current_stream][current_area], optarg);
					loadbmp_flag=1;
					break;
			case 'x':
					VERIFY_STREAMID(current_stream);
					VERIFY_AREAID(current_area);
					overlay_insert_arr[current_stream].area[current_area].start_x = atoi(optarg);
					break;
			case 'y':
					VERIFY_STREAMID(current_stream);
					VERIFY_AREAID(current_area);
					overlay_insert_arr[current_stream].area[current_area].start_y = atoi(optarg);
					break;
			case 't':
					VERIFY_STREAMID(current_stream);
					VERIFY_AREAID(current_area);
					trans_limit = atoi(optarg);
					break;
			case 10:
					VERIFY_STREAMID(current_stream);
					disable_osd[current_stream] = 1;
					break;
			default:
					printf("unknown option found: %c\n", ch);
					return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i;

	if ((fd_overlay = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (map_overlay() < 0)
		return -1;

	if (check_for_overlay() < 0)
		return -1;
	if (loadbmp_flag==1)
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (flag[i] == 1) {
			if (set_overlay(i) < 0)
				return -1;
		}
	}

	return 0;
}

