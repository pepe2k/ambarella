/*
 * test_yuvcap.c
 *
 * History:
 *	2012/02/09 - [Jian Tang] created file
 *	2013/11/27 - [Zhaoyang Chen] modified file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
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
#include "datatx_lib.h"
#include <signal.h>

#ifndef VERIFY_BUFFERID
#define VERIFY_BUFFERID(x)	do {		\
			if (((x) < IAV_ENCODE_SOURCE_BUFFER_FIRST) ||		\
				((x) >= IAV_ENCODE_SOURCE_BUFFER_LAST)) {	\
				printf("Invalid buffer id %d.\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif


typedef enum {
	CAPTURE_NONE = 255,
	CAPTURE_PREVIEW_BUFFER = 0,
	CAPTURE_MOTION_BUFFER,
	CAPTURE_RAW_BUFFER,
	CAPTURE_TYPE_NUM,
} CAPTURE_TYPE;

#define MAX_YUV_BUFFER_SIZE		(5120*4096)		// 5120x4096
#define MAX_ME1_BUFFER_SIZE		(MAX_YUV_BUFFER_SIZE / 16)	// 1/16 of 5120x4096
#define MAX_FRAME_NUM				(120)

#define YUVCAP_PORT					(2024)
#define YUV_ME1_INFO_MAX_NUM				(5)
#define YUV_SKIP_FRAME_NUMBER		(100)                 /*for calculate frame life cycle*/

#ifndef RING_ADD
#define RING_ADD(x, y)	do {		\
			(x) ++;	\
			if (((x) < 0) || ((x) >= (y))) {	\
				(x) = 0;	\
			}	\
		} while (0)
#endif

typedef enum {
	YUV420_IYUV = 0,	// Pattern: YYYYYYYYUUVV
	YUV420_YV12 = 1,	// Pattern: YYYYYYYYVVUU
	YUV420_NV12 = 2,	// Pattern: YYYYYYYYUVUV
	YUV422_YU16 = 3,	// Pattern: YYYYYYYYUUUUVVVV
	YUV422_YV16 = 4,	// Pattern: YYYYYYYYVVVVUUUU
	YUV_FORMAT_TOTAL_NUM,
	YUV_FORMAT_FIRST = YUV420_IYUV,
	YUV_FORMAT_LAST = YUV_FORMAT_TOTAL_NUM,
} YUV_FORMAT;

int fd_iav;

static int transfer_method = TRANS_METHOD_NFS;
static int port = YUVCAP_PORT;

static int current_buffer = -1;
static int capture_select = 0;
static int capture_vca_flag = 0;
static int non_block_read = 0;

static int yuv_buffer_id = 0;
static int yuv_format = YUV420_IYUV;

static int me1_buffer_id = 0;
static int frame_count = 1;
static int quit_yuv_stream = 0;
static int print_time_stamp = 0;
static int verbose = 0;
static int dry_run = 0;
static int yuv_frame_life_cycle = 0;
static int delay_frame_cap_data = 0;
static int G_multi_vin_num = 1;

const char *default_filename_nfs = "/mnt/media/test.yuv";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";
const char *default_filename;
static char filename[256];
static int fd_yuv[IAV_ENCODE_SOURCE_TOTAL_NUM];
static int fd_me1[IAV_ENCODE_SOURCE_TOTAL_NUM];
static int fd_raw = -1;


static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}

static int save_yuv_luma_buffer(u8* output, iav_yuv_buffer_info_ex_t * info)
{
	int i;
	u8 *in;
	u8 *out;

	if (info->pitch < info->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	if (info->pitch == info->width) {
		memcpy(output, info->y_addr, info->width * info->height);
	} else {
		in = info->y_addr;
		out = output;
		for (i = 0; i < info->height; i++) {		//row
			memcpy(out, in, info->width);
			in += info->pitch;
			out += info->width;
		}
	}

	return 0;
}

typedef struct yuv_neon_arg {
	u8 *in;
	u8 *u;
	u8 *v;
	int row;
	int col;
	int pitch;
} yuv_neon_arg;

extern void chrome_convert(struct yuv_neon_arg *);

static int save_yuv_chroma_buffer(u8* output, iav_yuv_buffer_info_ex_t * info)
{
	int width, height, pitch;
	u8* input = info->uv_addr;
	int i;
	static u32 time_stamp = 0, prev_stamp = 0, frame = 0;
	int ret = 0;
	yuv_neon_arg yuv_arg;
	int size_u, size_v;

	width = info->width;
	height = info->height;
	pitch = info->pitch;
	yuv_arg.in = input;
	yuv_arg.col = width;
	yuv_arg.pitch = pitch;

	// input yuv is uv interleaved with padding (uvuvuvuv.....)
	if (info->format == IAV_YUV_420_FORMAT) {
		size_u = width * height / 4;
		size_v = size_u;
		yuv_arg.row = height / 2;
		if (yuv_format == YUV420_YV12) {
			// YV12 format (YYYYYYYYVVUU)
			yuv_arg.u = output + size_v;
			yuv_arg.v = output;
			chrome_convert(&yuv_arg);
		} else if (yuv_format == YUV420_IYUV) {
			// IYUV (I420) format (YYYYYYYYUUVV)
			yuv_arg.u = output;
			yuv_arg.v = output + size_u;
			chrome_convert(&yuv_arg);
		} else if (yuv_format == YUV420_NV12) {
			// NV12 format (YYYYYYYYUVUV)
			for (i = 0; i < height / 2; ++i) {
				memcpy(output + i * width, info->uv_addr + i * pitch, width);
			}
		} else {
			// YUV interleaved
			printf("Error: Unsupported YUV 420 output format!\n");
			ret = -1;
		}
	} else if (info->format == IAV_YUV_422_FORMAT) {
		yuv_arg.row = height;
		size_u = width * height / 2;
		size_v = size_u;
		if (yuv_format == YUV422_YU16) {
			yuv_arg.u = output;
			yuv_arg.v = output + size_u;
			chrome_convert(&yuv_arg);
		} else if (yuv_format == YUV422_YV16) {
			yuv_arg.u = output + size_v;
			yuv_arg.v = output;
			chrome_convert(&yuv_arg);
		} else {
			printf("Error: Unsupported YUV input format!\n");
			ret = -1;
		}
	} else {
		printf("Error: Unsupported YUV input format!\n");
		ret = -1;
	}

	if (ret == 0) {
		if (print_time_stamp) {
			input = info->uv_addr + pitch * height * 2;
			time_stamp = ((input[3] << 24) | (input[2] << 16) |
				(input[1] << 8) | (input[0]));
			printf("=[%2d]============== time stamp : [0x%08X], prev [0x%08X], diff [%u].\n",
				frame, time_stamp, prev_stamp, (time_stamp - prev_stamp));
			prev_stamp = time_stamp;
			++frame;
		}
	}

	return ret;
}

static int save_yuv_data(int fd, int buffer, iav_yuv_buffer_info_ex_t * info,
	u8 * luma, u8 * chroma)
{
	static u64 mono_pts_prev = 0, mono_pts = 0;
	static u64 dsp_pts_prev = 0, dsp_pts = 0;
	int luma_size, chroma_size;

	if (dry_run) {
		printf("BUF [%d] Y 0x%08x, UV 0x%08x, pitch %u, %ux%u = Seqnum[%u].\n",
			buffer, (u32)info->y_addr, (u32)info->uv_addr, info->pitch,
			info->width, info->height, info->seqnum);
		return 0;
	}

	luma_size = info->width * info->height;
	if (info->format == IAV_YUV_420_FORMAT) {
		chroma_size = luma_size / 2;
	} else if (info->format == IAV_YUV_422_FORMAT) {
		chroma_size = luma_size;
	} else {
		printf("Error: Unrecognized yuv data format from DSP!\n");
		return -1;
	}

	if (save_yuv_luma_buffer(luma, info) < 0) {
		perror("Failed to save luma data into buffer !\n");
		return -1;
	}

	if (save_yuv_chroma_buffer(chroma, info) < 0) {
		perror("Failed to save chroma data into buffer !\n");
		return -1;
	}

	if (amba_transfer_write(fd, luma, luma_size, transfer_method) < 0) {
		perror("Failed to save luma data into file !\n");
		return -1;
	}

	if (amba_transfer_write(fd, chroma, chroma_size, transfer_method) < 0) {
		perror("Failed to save chroma data into file !\n");
	 	return -1;
	}

	if (verbose) {
		mono_pts = info->mono_pts;
		dsp_pts = info->dsp_pts;
		printf("BUF [%d] Y 0x%08x, UV 0x%08x, pitch %u, %ux%u = Seqnum[%u], mono_pts [%llu-%llu],"
				" dsp_pts [%llu-%llu], save to file [%d].\n",
			buffer, (u32)info->y_addr, (u32)info->uv_addr, info->pitch,
			info->width, info->height, info->seqnum, mono_pts, (mono_pts - mono_pts_prev),
			dsp_pts, (dsp_pts - dsp_pts_prev), fd);
		mono_pts_prev = mono_pts;
		dsp_pts_prev = dsp_pts;
	}

	return 0;
}

static int capture_yuv(int buff_id, int count)
{
	int i, buf, save[IAV_ENCODE_SOURCE_TOTAL_NUM];
	u8 read = 0;
	u8 write = read;
	u8 vca_buf;
	char yuv_file[256];
	u8 * luma = NULL;
	u8 * chroma = NULL;
	char format[32];
	iav_yuv_buffer_info_ex_t info;
	iav_yuv_buffer_info_ex_t info_cache[YUV_ME1_INFO_MAX_NUM];
	iav_yuv_cap_t *yuv;
	iav_buf_cap_t cap;

	luma = malloc(MAX_YUV_BUFFER_SIZE);
	if (luma == NULL) {
		printf("Not enough memory for preview capture !\n");
		goto yuv_error_exit;
	}
	memset(luma, 1, MAX_YUV_BUFFER_SIZE);
	chroma = malloc(MAX_YUV_BUFFER_SIZE);
	if (chroma == NULL) {
		printf("Not enough memory for preivew capture !\n");
		goto yuv_error_exit;
	}
	memset(chroma, 1, MAX_YUV_BUFFER_SIZE);
	memset(save, 0, sizeof(save));

	for (i = 0; i < count*10; ++i) {
		for (buf = IAV_ENCODE_SOURCE_BUFFER_FIRST;
			buf < IAV_ENCODE_SOURCE_BUFFER_LAST; ++buf) {
			memset(&info, 0, sizeof(info));
			info.source = buf;
			if (!non_block_read) {
				if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &info) < 0) {
					if (errno == EINTR) {
						continue;		/* back to for() */
					} else {
						perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
						goto yuv_error_exit;
					}
				}
			} else {
				memset(&cap, 0, sizeof(cap));
				cap.flag |= non_block_read;
				if (ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap)) {
					if (errno == EINTR) {
						continue;		/* back to for() */
					} else {
						perror("IAV_IOC_READ_BUF_CAP_EX");
						goto yuv_error_exit;
					}
				}
				yuv = &cap.yuv[buf];
				info.y_addr = yuv->y_addr;
				info.uv_addr = yuv->uv_addr;
				info.width = yuv->width;
				info.height = yuv->height;
				info.pitch = yuv->pitch;
				info.seqnum = yuv->seqnum;
				info.mono_pts = yuv->mono_pts;
				info.dsp_pts = yuv->dsp_pts;
				info.format = yuv->format;
			}

			// for VCA buffer
			if (capture_vca_flag && (buf == IAV_ENCODE_SOURCE_SECOND_BUFFER)) {
				yuv = &cap.vca;
				info.y_addr = yuv->y_addr;
				info.uv_addr = yuv->uv_addr;
				info.width = yuv->width;
				info.height = yuv->height;
				info.pitch = yuv->pitch;
				info.seqnum = yuv->seqnum;
				info.mono_pts = yuv->mono_pts;
				info.dsp_pts = yuv->dsp_pts;
				info.format = yuv->format;
				sprintf(yuv_file, "%s_vca_%dx%d.yuv", filename, info.width, info.height);

				if (!info.y_addr || !info.uv_addr) {
					printf("Y and UV address are NULL. VCA buffer is NOT enabled yet!\n");
					return -1;
				}

				vca_buf = IAV_ENCODE_SOURCE_TOTAL_NUM - 1;
				if (fd_yuv[vca_buf] < 0) {
					fd_yuv[vca_buf] = amba_transfer_open(yuv_file, transfer_method, port++);
				}
				if (fd_yuv[vca_buf] < 0) {
					printf("Cannot open file [%s] for VCA buffer.\n", yuv_file);
					continue;
				}
				if (save_yuv_data(fd_yuv[vca_buf], vca_buf, &info, luma, chroma) < 0) {
					printf("Failed to save YUV data for VCA buffer.\n");
					goto yuv_error_exit;
				}
			}

			if (buff_id & (1 << buf)) {
				if ((info.y_addr == NULL) || (info.uv_addr == NULL)) {
					printf("YUV buffer [%d] address is NULL! Skip to next!\n", buf);
					continue;
				}

				/* For multi VIN mode, YUV is stitched vertically. */
				info.height *= G_multi_vin_num;
				if (fd_yuv[buf] < 0) {
					memset(yuv_file, 0, sizeof(yuv_file));
					sprintf(yuv_file, "%s_prev_%c_%dx%d.yuv", filename,
						(buf == IAV_ENCODE_SOURCE_MAIN_BUFFER) ? 'M' :
						('a' + IAV_ENCODE_SOURCE_FOURTH_BUFFER - buf),
						info.width, info.height);
					if (fd_yuv[buf] < 0) {
						fd_yuv[buf] = amba_transfer_open(yuv_file,
							transfer_method, port++);
					}
					if (fd_yuv[buf] < 0) {
						printf("Cannot open file [%s].\n", yuv_file);
						continue;
					}
				}
				info_cache[write] = info;
				if (((write -read) >= delay_frame_cap_data) ||
					( (read > write) && (YUV_ME1_INFO_MAX_NUM - read + write) >=
						delay_frame_cap_data )) {
					if (save_yuv_data(fd_yuv[buf], buf, &info_cache[read],
								luma, chroma) < 0) {
						printf("Failed to save YUV data of buf [%d].\n", buf);
						goto yuv_error_exit;
					}
					RING_ADD(read, YUV_ME1_INFO_MAX_NUM);
				}
				RING_ADD(write, YUV_ME1_INFO_MAX_NUM);
				if (save[buf] == 0) {
					save[buf] = 1;
					if (info.format == IAV_YUV_422_FORMAT) {
						if (yuv_format == YUV422_YU16) {
							sprintf(format,"YU16");
						} else if (yuv_format == YUV422_YV16) {
							sprintf(format, "YV16");
						} else {
							printf("Error: Unsupported YUV 422 format!\n");
							return -1;
						}
					} else if (info.format == IAV_YUV_420_FORMAT) {
						switch (yuv_format) {
						case YUV420_YV12:
							sprintf(format, "YV12");
							break;
						case YUV420_NV12:
							sprintf(format, "NV12");
							break;
						case YUV420_IYUV:
							sprintf(format, "IYUV");
							break;
						default:
							sprintf(format, "IYUV");
							break;
						}
					} else {
						sprintf(format, "Unknown [%d]", info.format);
					}
					printf("Delay %d frame capture yuv data.\n", delay_frame_cap_data);
					printf("Capture_yuv_buffer : resolution %dx%d in %s format\n",
						info.width, info.height, format);
				}
			}
		}
	}

	//capture remain frame YUV data
	while (read != write) {
		for (buf = IAV_ENCODE_SOURCE_BUFFER_FIRST;
			buf < IAV_ENCODE_SOURCE_BUFFER_LAST; ++buf) {
			if (buff_id & (1 << buf)) {
				if (save_yuv_data(fd_yuv[buf], buf, &info_cache[read], luma, chroma) < 0) {
					printf("Failed to save YUV data of buf [%d].\n", buf);
					goto yuv_error_exit;
				}
				RING_ADD(read, YUV_ME1_INFO_MAX_NUM);
			}
		}
	}

	free(luma);
	free(chroma);
	return 0;

yuv_error_exit:
	if (luma)
		free(luma);
	if (chroma)
		free(chroma);
	return -1;
}

static int save_me1_luma_buffer(u8* output, iav_me1_buffer_info_ex_t * info)
{
	int i;
	u8 *in;
	u8 *out;

	if (info->pitch < info->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	if (info->pitch == info->width) {
		memcpy(output, info->addr, info->width * info->height);
	} else {
		in = info->addr;
		out = output;
		for (i = 0; i < info->height; i++) {		//row
			memcpy(out, in, info->width);
			in += info->pitch;
			out += info->width;
		}
	}

	return 0;
}


static int save_me1_data(int fd, int buffer, iav_me1_buffer_info_ex_t * info,
	u8 * y_buf, u8 * uv_buf)
{
	static u64 mono_pts_prev = 0, mono_pts = 0;
	static u64 dsp_pts_prev = 0, dsp_pts = 0;

	if (dry_run) {
		printf("BUF [%d] me1 0x%08x, pitch %d, %dx%d, seqnum [%d].\n",
			buffer, (u32)info->addr, info->pitch, info->width,
			info->height, info->seqnum);
		return 0;
	}
	save_me1_luma_buffer(y_buf,info);

	if (amba_transfer_write(fd, y_buf,
		info->width * info->height, transfer_method) < 0) {
		perror("Failed to save ME1 data into file !\n");
		return -1;
	}

	if (amba_transfer_write(fd, uv_buf,
		info->width * info->height / 2, transfer_method) < 0) {
		perror("Failed to save ME1 data into file !\n");
		return -1;
	}

	if (verbose) {
		mono_pts = info->mono_pts;
		dsp_pts = info->dsp_pts;
		printf("BUF [%d] 0x%08x, pitch %d, %dx%d, seqnum [%d], mono_pts [%llu-%llu], dsp_pts [%llu-%llu].\n",
			buffer, (u32)info->addr, info->pitch, info->width,
			info->height, info->seqnum, mono_pts, (mono_pts - mono_pts_prev),
			dsp_pts, (dsp_pts - dsp_pts_prev));
		mono_pts_prev = mono_pts;
		dsp_pts_prev = dsp_pts;

	}

	return 0;
}

static int capture_me1(int buff_id, int count)
{
	int i, buf, save[IAV_ENCODE_SOURCE_TOTAL_NUM];
	int read = 0;
	int write = read;
	char me1_file[256];
	u8 * luma = NULL;
	u8 * chroma = NULL;
	iav_me1_buffer_info_ex_t info;
	iav_me1_buffer_info_ex_t info_cache[YUV_ME1_INFO_MAX_NUM];
	iav_me1_cap_t *me1;
	iav_buf_cap_t cap;

	luma = malloc(MAX_ME1_BUFFER_SIZE);
	if (luma == NULL) {
		printf("Not enough memory for ME1 buffer capture !\n");
		goto me1_error_exit;
	}

	//clear UV to be B/W mode, UV data is not useful for motion detection,
	//just fill UV data to make YUV to be YV12 format, so that it can play in YUV player
	chroma = malloc(MAX_ME1_BUFFER_SIZE);
	if (chroma == NULL) {
		printf("Not enough memory for ME1 buffer capture !\n");
		goto me1_error_exit;
	}
	memset(chroma, 0x80, MAX_ME1_BUFFER_SIZE);
	memset(save, 0, sizeof(save));

	for (i = 0; i < count; ++i) {
		for (buf = IAV_ENCODE_SOURCE_BUFFER_FIRST;
			buf < IAV_ENCODE_SOURCE_BUFFER_LAST; ++buf) {
			if (buff_id & (1 << buf)) {
				memset(&info, 0, sizeof(info));
				info.source = buf;
				if (!non_block_read) {
					if (ioctl(fd_iav, IAV_IOC_READ_ME1_BUFFER_INFO_EX, &info) < 0) {
						if (errno == EINTR) {
							continue;		/* back to for() */
						} else {
							perror("IAV_IOC_READ_ME1_BUFFER_INFO_EX");
							goto me1_error_exit;
						}
					}
				} else {
					memset(&cap, 0, sizeof(cap));
					cap.flag |= non_block_read;
					if (ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap)) {
						if (errno == EINTR) {
							continue;		/* back to for() */
						} else {
							perror("IAV_IOC_READ_BUF_CAP_EX");
							goto me1_error_exit;
						}
					}
					me1 = &cap.me1[buf];
					info.addr = me1->addr;
					info.width = me1->width;
					info.height = me1->height;
					info.pitch = me1->pitch;
					info.seqnum = me1->seqnum;
					info.mono_pts = me1->mono_pts;
					info.dsp_pts = me1->dsp_pts;
				}
				if (info.addr == NULL) {
					printf("ME1 buffer [%d] address is NULL! Skip to next!\n", buf);
					continue;
				}
				/* For multi VIN mode, ME1 is stitched vertically. */
				info.height *= G_multi_vin_num;
				if (fd_me1[buf] < 0) {
					memset(me1_file, 0, sizeof(me1_file));
					sprintf(me1_file, "%s_me1_%c_%dx%d.yuv", filename,
						(buf == IAV_ENCODE_SOURCE_MAIN_BUFFER) ? 'm' :
						('a' + IAV_ENCODE_SOURCE_FOURTH_BUFFER - buf),
						info.width, info.height);
					if (fd_me1[buf] < 0) {
						fd_me1[buf] = amba_transfer_open(me1_file,
							transfer_method, port++);
					}
					if (fd_me1[buf] < 0) {
						printf("Cannot open file [%s].\n", me1_file);
						continue;
					}
				}
				info_cache[write] = info;
				if (( (write -read) >= delay_frame_cap_data) ||
					( (read > write) && (YUV_ME1_INFO_MAX_NUM - read + write) >=
						delay_frame_cap_data )) {
					if (save_me1_data(fd_me1[buf], buf, &info_cache[read], luma, chroma) < 0) {
						printf("Failed to save ME1 data of buf [%d].\n", buf);
						goto me1_error_exit;
					}
					RING_ADD(read, YUV_ME1_INFO_MAX_NUM);
				}
				RING_ADD(write, YUV_ME1_INFO_MAX_NUM);

				if (save[buf] == 0) {
					save[buf] = 1;
					printf("Delay %d frame capture me1 data.\n", delay_frame_cap_data);
					printf("Save_me1_buffer : resolution %dx%d in YV12 format\n",
						info.width, info.height);
				}
			}
		}
	}

	//capture remain frame ME1 data
	while (read != write) {
		for (buf = IAV_ENCODE_SOURCE_BUFFER_FIRST;
			buf < IAV_ENCODE_SOURCE_BUFFER_LAST; ++buf) {
			if (buff_id & (1 << buf)) {
				if (save_me1_data(fd_me1[buf], buf, &info_cache[read], luma, chroma) < 0) {
					printf("Failed to save ME1 data of buf [%d].\n", buf);
					goto me1_error_exit;
				}
				RING_ADD(read, YUV_ME1_INFO_MAX_NUM);
			}
		}
	}

	free(luma);
	free(chroma);
	return 0;

me1_error_exit:
	if (luma)
		free(luma);
	if (chroma)
		free(chroma);
	return -1;
}

static int capture_raw(void)
{
	iav_raw_info_t info;
	u8 * raw_buffer = NULL;
	struct amba_video_info vin_info;
	iav_system_resource_setup_ex_t resource;
	u32 vin_width, vin_height, buffer_size;
	char raw_file[256];

	memset(&vin_info, 0, sizeof(vin_info));
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	vin_width = vin_info.width;
	vin_height = vin_info.height;

	memset(raw_file, 0, sizeof(raw_file));
	sprintf(raw_file, "%s_raw.raw", filename);
	if (fd_raw < 0) {
		fd_raw = amba_transfer_open(raw_file,
			transfer_method, port++);
	}
	if (fd_raw < 0) {
		printf("Cannot open file [%s].\n", raw_file);
		goto raw_error_exit;
	}
	memset(&info, 0, sizeof(info));
	info.flag |= non_block_read;
	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &info) < 0) {
		if (errno == EINTR) {
			// skip to do nothing
		} else {
			perror("IAV_IOC_READ_RAW_INFO");
			goto raw_error_exit;
		}
	}

	if (dry_run) {
		printf("Raw data 0x%08x: pitch in pixel [%d], width [%d], "
			"height [%d], Raw pitch [%d].\n", (u32)info.raw_addr, info.pitch >> 1,
			info.width, info.height, info.pitch);
		amba_transfer_close(fd_raw, transfer_method);
		return 0;
	}

	memset(&resource, 0, sizeof(resource));
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		goto raw_error_exit;
	}
	if (resource.raw_capture_enable &&
		((info.width != vin_width) || (info.height != vin_height))) {
		printf("VIN resolution %dx%d, raw data info %dx%d is incorrect!\n",
			vin_width, vin_height, info.width, info.height);
		goto raw_error_exit;
	}

	if ((info.width != 0) && (info.height != 0)) {
		buffer_size = info.pitch * info.height;
	} else {
		buffer_size = vin_width * vin_height * 2;
	}

	raw_buffer = (u8 *)malloc(buffer_size);
	if (raw_buffer == NULL) {
		printf("Not enough memory for read out raw buffer!\n");
		goto raw_error_exit;
	}
	memset(raw_buffer, 0, buffer_size);
	memcpy(raw_buffer, info.raw_addr, buffer_size);

	if (amba_transfer_write(fd_raw, raw_buffer, buffer_size,
		transfer_method) < 0) {
		perror("Failed to save RAW data into file !\n");
		goto raw_error_exit;
	}

	amba_transfer_close(fd_raw, transfer_method);
	free(raw_buffer);
	printf("save raw buffer done!\n");
	printf("VIN resolution [%d x %d], Raw data: pitch in pixel [%d], width [%d], "
		"height [%d], Raw pitch [%d].\n", vin_width, vin_height, info.pitch >> 1,
		info.width, info.height, info.pitch);

	return 0;

raw_error_exit:
	if (raw_buffer)
		free(raw_buffer);
	if (fd_raw >= 0) {
		amba_transfer_close(fd_raw, transfer_method);
		fd_raw = -1;
	}
	return -1;
}

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "b:B:cCd:f:F:mnpr:RtuvY";

static struct option long_options[] = {
	{"buffer",		HAS_ARG, 0, 'b'},
	{"non-block",	HAS_ARG, 0, 'B'},
	{"vca",		NO_ARG, 0, 'c'},
	{"frame-cycle", NO_ARG, 0, 'C'},
	{"yuv",		NO_ARG, 0, 'Y'},
	{"me1",		NO_ARG, 0, 'm'},		/*capture me1 buffer */
	{"dry-run",  NO_ARG, 0, 'n'},
	{"raw",		NO_ARG, 0, 'R'},
	{"filename",	HAS_ARG, 0, 'f'},		/* specify file name*/
	{"format",	HAS_ARG, 0, 'F'},
	{"tcp", 		NO_ARG, 0, 't'},
	{"usb",		NO_ARG, 0,'u'},
	{"frames",	HAS_ARG,0, 'r'},
	{"print-time-stamp",	NO_ARG, 0, 'p'},
	{"delay-frame-cap-data",	HAS_ARG, 0, 'd'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0~3", "select source buffer id, 0 for main, 1 for 2nd buffer, 2 for 3rd buffer, 3 for 4th buffer"},
	{"0|1", "select the read method, 0 is block call, 1 is non-block call. Default is block call."},
	{"", "\tcapture YUV data from VCA buffer"},
	{"",  "calculate yuv frame life cycle"},
	{"",	"\tcapture YUV data from source buffer"},
	{"",	"\tcapture me1 data from source buffer"},
	{"",  "\tdon't save data, only dry run"},
	{"",	"\tcapture raw data"},
	{"?.yuv",	"filename to store output yuv"},
	{"0|1|2|3|4", "YUV420 data format for encode buffer, 0: IYUV(I420), 1: YV12, 2: NV12, 3:YU16, 4:YV16. Default is IYUV format"},
	{"",	"\tuse tcp to send data to PC"},
	{"",	"\tuse usb to send data to PC"},
	{"1~120",	"frame counts to capture. Default is 1."},
	{"",	"print time stamp for preview A YUV buffer data"},
	{"0~4",	"delay how many frame to capture yuv or me1 data. Default value is 0."},
	{"",	"\tprint more messages"},
};

static void usage(void)
{
	u32 i;
	char *itself = "test_yuvcap";

	printf("\n");
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
	printf("\n\nThis program captures YUV or ME1 buffer in YUV420 format for encode buffer, and save as IYUV, YV12 or NV12.\n");
	printf("  IYUV format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVV\n");
	printf("  YV12 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUU\n");
	printf("  NV12 format (U and V are interleaved buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUVUVUVUVUVUVUV\n"
 		 "\t\tUVUVUVUVUVUVUV\n"
	 	 "\t\tUVUVUVUVUVUVUV\n"
 	 	 "\t\tUVUVUVUVUVUVUV\n");

	printf("\n\nThis program captures YUV buffer in YUV422 format for preview buffer, and save as YU16 or YV16.\n");
	printf("  YU16 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n");
	printf("  YV16 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n");

	printf("\nE.g.: To get one single preview frame of IYUV format\n");
	printf("    > %s -b 1 -Y -f 1.yuv -F 0 --tcp\n\n", itself);
	printf("    To get one single preview frame of IYUV format and one frame from VCA buffer with non-blocking method\n");
	printf("    > %s -b 1 -Y -c -f 1.yuv -F 0 --tcp -B 1\n\n", itself);
	printf("    To get continous preview as .yuv file of YV12 format and with delay 1 frame capture yuv\n");
	printf("    > %s -b 1 -Y -f 1.yuv -F 1 -r 30 --tcp -d 1 \n\n", itself);
	printf("    To get one single preview frame of NV12 format with non-blocking call\n");
	printf("    > %s -b 1 -B 1 -Y -f /tmp/1 -F 2 --tcp\n\n", itself);
	printf("    To get continous me1 as .yuv file\n");
	printf("    > %s -b 0 -m -b 1 -m -f 2.me1 -r 20 --tcp\n\n", itself);
	printf("    To get raw data from RGB sensor input, please enter \n");
	printf("    > %s -R -f cap_raw --tcp\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'b':
			value = atoi(optarg);
			VERIFY_BUFFERID(value);
			current_buffer = value;
			break;
		case 'B':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Read flag [%d] must be [0|1].\n", value);
				return -1;
			}
			non_block_read = value;
			break;
		case 'c':
			capture_select = CAPTURE_PREVIEW_BUFFER;
			capture_vca_flag = 1;
			break;
		case 'Y':
			VERIFY_BUFFERID(current_buffer);
			capture_select = CAPTURE_PREVIEW_BUFFER;
			yuv_buffer_id |= (1 << current_buffer);
			break;
		case 'm':
			VERIFY_BUFFERID(current_buffer);
			capture_select = CAPTURE_MOTION_BUFFER;
			me1_buffer_id |= (1 << current_buffer);
			break;
		case 'R':
			capture_select = CAPTURE_RAW_BUFFER;
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'F':
			value = atoi(optarg);
			if (value < YUV_FORMAT_FIRST || value >= YUV_FORMAT_LAST) {
				printf("Invalid output yuv format : %d.\n", value);
				return -1;
			}
			yuv_format = value;
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'r':
			value = atoi(optarg);
			if (value < 0 || value > MAX_FRAME_NUM) {
				printf("Cannot capture YUV or ME1 data over %d frames [%d].\n",
					MAX_FRAME_NUM, value);
				return -1;
			}
			frame_count = value;
			break;
		case 'p':
			print_time_stamp = 1;
			break;
		case 'd':
			value = atoi(optarg);
			if (value < 0 || value >= YUV_ME1_INFO_MAX_NUM) {
				printf("Cannot delay capture YUV or ME1 data over %d frames [%d].\n",
					YUV_ME1_INFO_MAX_NUM, value);
				return -1;
			}
			delay_frame_cap_data = value;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'n':
			dry_run= 1;
			break;
		case 'C':
			yuv_frame_life_cycle= 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	if (transfer_method == TRANS_METHOD_TCP ||
		transfer_method == TRANS_METHOD_USB) {
		default_filename = default_filename_tcp;
	} else {
		default_filename = default_filename_nfs;
	}

	if (capture_vca_flag && !non_block_read) {
		printf("  VCA buffer capture is ONLY available in 'non-blocking' mode. "
			"Please add '-B 1' option to run command again.\n");
		return -1;
	}

	return 0;
}

static void sigstop(int a)
{
	quit_yuv_stream = 1;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		exit(2);
	}

	if ((state != IAV_STATE_PREVIEW) && state != IAV_STATE_ENCODING) {
		printf("IAV is not in preview / encoding state, cannot get yuv buf!\n");
		return -1;
	}

	return 0;
}

int get_multi_vin_num(void)
{
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;

	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		return -1;
	}
	G_multi_vin_num = resource_setup.vin_num;
	if (verbose) {
		printf("vin num =%d\n", G_multi_vin_num);
	}
	return 0;
}

static int calc_yuv_frame_life_cycle(int buf_id)
{
	int ret = 0;
	iav_yuv_buffer_info_ex_t info;
	iav_yuv_buffer_info_ex_t tmp_info;
	int count = 0;
	int start_flag = 0;
	struct timeval time1, time2;
	unsigned long time_diff = 0;
	int magic_number[4] = {0x55AA,0xAA55,0x55AA,0xAA55};

	while (1) {
		count ++;
		memset(&info, 0, sizeof(info));
		info.source = buf_id;
		if (start_flag && (memcmp(tmp_info.y_addr, magic_number, 4) != 0)) {
			gettimeofday(&time2, NULL);
			time_diff = (time2.tv_usec - time1.tv_usec) / 1000L +
				(time2.tv_sec - time1.tv_sec) * 1000L;
			printf("frame count is %d, time diff %ld ms. \n",
				count -YUV_SKIP_FRAME_NUMBER, time_diff);
			start_flag = 0;
			break;
		}

		if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &info) < 0) {
			perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
			ret = -1;
		}

		/*dsp needs to skip front frames*/
		if ((count >= YUV_SKIP_FRAME_NUMBER) && (!start_flag)) {
			tmp_info = info;
			memcpy(tmp_info.y_addr, magic_number, 4);
			start_flag = 1;
			gettimeofday(&time1, NULL);
		}
	}

	return ret;
}



int main(int argc, char **argv)
{
	int retv = 0, i;

	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0)
		return -1;

	//check iav state
	if (check_state() < 0)
		return -1;

	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	for (i = IAV_ENCODE_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SOURCE_BUFFER_LAST; ++i) {
		fd_yuv[i] = -1;
		fd_me1[i] = -1;
	}

	if (get_multi_vin_num() < 0) {
		perror("get_multi_vin_num");
		return -1;
	}

	if (yuv_frame_life_cycle) {
		calc_yuv_frame_life_cycle(current_buffer);
		return retv;
	}
	switch (capture_select) {
	case CAPTURE_PREVIEW_BUFFER:
		capture_yuv(yuv_buffer_id, frame_count);
		break;
	case CAPTURE_MOTION_BUFFER:
		capture_me1(me1_buffer_id, frame_count);
		break;
	case CAPTURE_RAW_BUFFER:
		capture_raw();
		break;
	default:
		printf("Invalid capture mode [%d] !\n", capture_select);
		return -1;
		break;
	}

	for (i = IAV_ENCODE_SOURCE_BUFFER_FIRST;
		i < IAV_ENCODE_SOURCE_BUFFER_LAST; ++i) {
		if (fd_yuv[i] >= 0) {
			amba_transfer_close(fd_yuv[i], transfer_method);
			fd_yuv[i] = -1;
		}
		if (fd_me1[i] >= 0) {
			amba_transfer_close(fd_me1[i], transfer_method);
			fd_me1[i] = -1;
		}
	}

	if (amba_transfer_deinit(transfer_method) < 0) {
		retv = -1;
	}

	return retv;
}

