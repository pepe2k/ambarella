/*
 * test_raw_yuvcap.c
 *
 * History:
 *	2015/04/23 - [Lei Hong] created file
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

#define MAX_RAW_BUFFER_SIZE		(1024*1024*6)
#define MAX_VIN_SIZE				(1920*1080)
#define YUVCAP_PORT					(2024)

int fd_iav;
static int transfer_method = TRANS_METHOD_NFS;
static int port = YUVCAP_PORT;
static int non_block_read = 0;
static int verbose = 0;

const char *default_filename_nfs = "/mnt/media/test.yuv";
const char *default_filename_tcp = "media/test";
const char *default_filename;
static char filename[256];
static int fd_yuv = -1;
static int fd_raw = -1;

static u8 *user_mem_start_addr = NULL;
static int user_mem_size = 0;

static int gdma_cpoy(iav_gdma_copy_ex_t *pGdma)
{
	//copy to new position
	if (ioctl(fd_iav, IAV_IOC_GDMA_COPY_EX, pGdma) < 0) {
		printf("iav gdma copy failed \n");
		return -1;
	}
	return 0;
}

static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;
	iav_mmap_info_t user_info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}


	if (ioctl(fd_iav, IAV_IOC_MAP_USER, &user_info) < 0) {
		printf("map user mem failed \n");
		return -1;
	}
	user_mem_start_addr = user_info.addr;
	user_mem_size = user_info.length;

	mem_mapped = 1;
	return 0;
}

static int save_raw_gdma(u8* output, iav_raw_info_t * info)
{
	iav_gdma_copy_ex_t gdma_param;
	iav_system_setup_info_ex_t sys_info;
	u8 *user_mem_ptr = NULL;
	struct timeval time1, time2;
	unsigned long time_diff = 0;

	// DSP memory is used for "non-cahced" GDMA copy
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &sys_info) < 0) {
		perror("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
		return -1;
	}
	if (verbose) {
		gettimeofday(&time1, NULL);
	}
	user_mem_ptr = output;
	gdma_param.src_non_cached = sys_info.dsp_noncached;
	gdma_param.dst_non_cached = sys_info.dsp_noncached;

	gdma_param.usr_src_addr = (u32)(info->raw_addr);
	gdma_param.usr_dst_addr = (u32)user_mem_ptr;
	gdma_param.src_mmap_type = IAV_MMAP_DSP;
	gdma_param.dst_mmap_type = IAV_MMAP_USER;
	gdma_param.src_pitch = info->pitch;
	gdma_param.dst_pitch = info->pitch;
	gdma_param.width = info->pitch;
	gdma_param.height = info->height;

	if (gdma_cpoy(&gdma_param) < 0) {
		printf("gdma_cpoy error\n");
		return -1;
	}
	if (verbose) {
		gettimeofday(&time2, NULL);
		time_diff = (time2.tv_usec - time1.tv_usec) / 1000L +
			(time2.tv_sec - time1.tv_sec) * 1000L;
		printf("raw data gdma copy time diff is %ld ms. \n", time_diff);
	}
	return 0;
}

static int save_yuv_luma_chroma_gdma(u8* output, iav_yuv_buffer_info_ex_t * info)
{
	iav_gdma_copy_ex_t gdma_param;
	iav_system_setup_info_ex_t sys_info;
	u8 *user_mem_ptr = NULL;
	struct timeval time1, time2;
	unsigned long time_diff = 0;

	// DSP memory is used for "non-cahced" GDMA copy
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &sys_info) < 0) {
		perror("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
		return -1;
	}
	if (verbose) {
		gettimeofday(&time1, NULL);
	}
	user_mem_ptr = output;
	gdma_param.src_non_cached = sys_info.dsp_noncached;
	gdma_param.dst_non_cached = sys_info.dsp_noncached;

	gdma_param.usr_src_addr = (u32)(info->y_addr);
	gdma_param.usr_dst_addr = (u32)user_mem_ptr;
	gdma_param.src_mmap_type = IAV_MMAP_DSP;
	gdma_param.dst_mmap_type = IAV_MMAP_USER;

	gdma_param.src_pitch = info->pitch;
	gdma_param.dst_pitch = info->width;
	gdma_param.width = info->width;
	gdma_param.height = info->height;

	if (gdma_cpoy(&gdma_param) < 0) {
		printf("gdma_cpoy error\n");
		return -1;
	}

	user_mem_ptr += info->width * info->height;

	gdma_param.usr_src_addr = (u32)(info->uv_addr);
	gdma_param.usr_dst_addr = (u32)user_mem_ptr;
	gdma_param.src_mmap_type = IAV_MMAP_DSP;
	gdma_param.dst_mmap_type = IAV_MMAP_USER;

	gdma_param.src_pitch = info->pitch;
	gdma_param.dst_pitch = info->width;
	gdma_param.width = info->width;
	gdma_param.height = info->height >> 1;

	if (gdma_cpoy(&gdma_param) < 0) {
		printf("gdma_cpoy error\n");
		return -1;
	}
	if (verbose) {
		gettimeofday(&time2, NULL);
		time_diff = (time2.tv_usec - time1.tv_usec) / 1000L +
			(time2.tv_sec - time1.tv_sec) * 1000L;
		printf("yuv data gdma copy time diff is %ld ms. \n", time_diff);
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

int capture_yuv_raw_task(void)
{
	struct amba_video_info vin_info;
	u32 vin_width, vin_height, buffer_size;
	char raw_file[256];
	iav_raw_info_t raw_info;
	iav_system_resource_setup_ex_t resource;
	iav_yuv_buffer_info_ex_t info;
	iav_buf_cap_t cap;
	iav_yuv_cap_t *yuv;
	char yuv_file[256];
	int luma_size, chroma_size;

	memset(&vin_info, 0, sizeof(vin_info));
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	vin_width = vin_info.width;
	vin_height = vin_info.height;
	memset(&raw_info, 0, sizeof(raw_info));
	raw_info.flag |= non_block_read;
	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}
	memset(&resource, 0, sizeof(resource));
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		return -1;
	}
	if (resource.raw_capture_enable &&
		((raw_info.width != vin_width) || (raw_info.height != vin_height))) {
		printf("VIN resolution %dx%d, raw data info %dx%d is incorrect!\n",
			vin_width, vin_height, raw_info.width, raw_info.height);
		return -1;
	}
	save_raw_gdma(user_mem_start_addr, &raw_info);
	while (1) {
		memset(&info, 0, sizeof(info));
		info.source = IAV_ENCODE_SOURCE_MAIN_BUFFER;
		if (!non_block_read) {
			if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &info) < 0) {
				perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
				return -1;
			}
		} else {
			memset(&cap, 0, sizeof(cap));
			cap.flag |= non_block_read;
			if (ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap)) {
				perror("IAV_IOC_READ_BUF_CAP_EX");
				return -1;
			}
			yuv = &cap.yuv[0];
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
		if ((info.y_addr == NULL) || (info.uv_addr == NULL)) {
			printf("YUV buffer address is NULL!\n");
			return -1;
		}
		if (raw_info.raw_dsp_pts == info.dsp_pts) {
			luma_size = info.width * info.height;
			if (info.format == IAV_YUV_420_FORMAT) {
				chroma_size = luma_size / 2;
			} else if (info.format == IAV_YUV_422_FORMAT) {
				chroma_size = luma_size;
			} else {
				printf("Error: Unrecognized yuv data format from DSP!\n");
				return -1;
			}
			if (save_yuv_luma_chroma_gdma(user_mem_start_addr +
				MAX_RAW_BUFFER_SIZE , &info) < 0) {
				printf("Failed to save YUV data of buf.\n");
				return -1;
			}
			break;
		}
	}

	if (fd_raw < 0) {
		memset(raw_file, 0, sizeof(raw_file));
		sprintf(raw_file, "%s_raw.raw", filename);
		fd_raw = amba_transfer_open(raw_file, transfer_method, port++);
		if (fd_raw < 0) {
			printf("Cannot open file [%s].\n", raw_file);
			goto yuv_raw_error_exit;
		}
	}
	buffer_size = raw_info.pitch * raw_info.height;
	if (amba_transfer_write(fd_raw, user_mem_start_addr, buffer_size,
		transfer_method) < 0) {
		perror("Failed to save RAW data into file !\n");
		goto yuv_raw_error_exit;
	}
	if (verbose) {
		printf("raw_addr 0x%08x, pitch %u, %ux%u, dsp_pts %llu, save to file [%d].\n",
			(u32)raw_info.raw_addr, raw_info.pitch, raw_info.width,
			raw_info.height, raw_info.raw_dsp_pts, fd_raw);
	}
	if (fd_yuv < 0) {
		memset(yuv_file, 0, sizeof(yuv_file));
		sprintf(yuv_file, "%s_prev_%c_%dx%d.yuv", filename, 'M', info.width,
			info.height);
		fd_yuv = amba_transfer_open(yuv_file, transfer_method, port++);
		if (fd_yuv < 0) {
			printf("Cannot open file [%s].\n", yuv_file);
			goto yuv_raw_error_exit;
		}
	}
	if (amba_transfer_write(fd_yuv, user_mem_start_addr + MAX_RAW_BUFFER_SIZE,
		luma_size, transfer_method) < 0) {
		perror("Failed to save luma data into file !\n");
		goto yuv_raw_error_exit;
	}
	if (amba_transfer_write(fd_yuv, user_mem_start_addr + MAX_RAW_BUFFER_SIZE
		+ luma_size, chroma_size, transfer_method) < 0) {
		perror("Failed to save chroma data into file !\n");
		goto yuv_raw_error_exit;
	}
	if (verbose) {
		printf("Y 0x%08x, UV 0x%08x, pitch %u, %ux%u = Seqnum[%u],"
				" dsp_pts %llu, save to file [%d].\n",
				(u32)info.y_addr, (u32)info.uv_addr, info.pitch,
				info.width, info.height, info.seqnum, info.dsp_pts, fd_yuv);
	}

yuv_raw_error_exit:
	if (fd_yuv >= 0) {
		amba_transfer_close(fd_raw, transfer_method);
		fd_yuv = -1;
	}
	if (fd_raw >= 0) {
		amba_transfer_close(fd_raw, transfer_method);
		fd_raw = -1;
	}
	return 0;
}

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "B:f:tuv";

static struct option long_options[] = {
	{"non-block",	HAS_ARG, 0, 'B'},
	{"filename",	HAS_ARG, 0, 'f'},		/* specify file name*/
	{"tcp", 		NO_ARG, 0, 't'},
	{"usb",		NO_ARG, 0,'u'},
	{"verbose",	NO_ARG, 0, 'v'},
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0|1", "select the read method, 0 is block call, 1 is non-block call. Default is block call."},
	{"?",	"filename to store output yuv"},
	{"",	"\tuse tcp to send data to PC"},
	{"",	"\tuse usb to send data to PC"},
	{"",	"\tprint more messages"},
};

static void usage(void)
{
	u32 i;
	char *itself = "test_raw_yuvcap";

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
	printf("\n\nPlease enable dsp noncache function and set vin size can't be larger than the limit 1080p.\n");
	printf("\n\nThis program captures YUV and RAW buffer in YUV420 format for encode buffer, and save as NV12.\n");
	printf("  NV12 format (U and V are interleaved buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUVUVUVUVUVUVUV\n"
		 "\t\tUVUVUVUVUVUVUV\n"
	 	 "\t\tUVUVUVUVUVUVUV\n"
	 	 "\t\tUVUVUVUVUVUVUV\n");
	printf("\nE.g.: To get one single preview frame of IYUV format\n");
	printf("    > %s -f example --tcp\n\n", itself);
	printf("    To get one single preview frame of IYUV format with non-blocking call\n");
	printf("    > %s -B 1 -f /tmp/1 --tcp\n\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'B':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Read flag [%d] must be [0|1].\n", value);
				return -1;
			}
			non_block_read = value;
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'v':
			verbose = 1;
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

	return 0;
}

static void sigstop(int a)
{
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

static int check_vin(void)
{
	struct amba_video_info vin_info;
	u32 vin_width, vin_height;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	vin_width = vin_info.width;
	vin_height = vin_info.height;

	if (vin_width * vin_height > MAX_VIN_SIZE) {
		printf("vin size: %d can't be larger than the limit 1080p. \n",
			vin_width * vin_height);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int retv = 0;

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
	if (check_vin() < 0) {
		return -1;
	}
	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}
	if (filename[0] == '\0')
		strcpy(filename, default_filename);
	if (capture_yuv_raw_task() < 0) {
		return -1;
	}
	if (amba_transfer_deinit(transfer_method) < 0) {
		retv = -1;
	}

	return retv;
}

