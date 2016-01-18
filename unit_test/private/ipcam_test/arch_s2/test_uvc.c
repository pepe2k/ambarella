
/*
 * test_uvc.c
 * the program can read map buffer from UVC and feed buffer to DSP EFM
 *
 * History:
 *	2014/8/11 - [Tao Wu] Create file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
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

#include <signal.h>
#include <basetypes.h>
#include <linux/videodev2.h>

#include "iav_drv.h"
#include "iav_drv_ex.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#define UVC_BUF_COUNT		(3)
#define MAX_UVC_ID			(32)
#define MAX_YUV_FRAME_NUM	(120)

static int uvc_id = 0;
static int uvc_width = 640;
static int uvc_height = 480;
static int frame_rate = 30;
static u32 frame_size = 0;
static u32 yuv_pts_distance = 3000; // for 30 fps

// the iav device file handle
int fd_iav = -1;
// the uvc device file handle
int fd_uvc = -1;

//used for uvc
typedef struct {
	void *start;
	int length;
} BUFTYPE;

BUFTYPE *uvc_buf;
static int uvc_buf_count = 0;

//NV12 format
static u8 * yuv_addr = NULL;
static u8 * yuv_end = NULL;
static u8 * frame_start = NULL;

static u32 efm_uvc_task_exit_flag = 0;
int capture_data = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"dev",	HAS_ARG,	0,	'd'},
	{"fsize",	HAS_ARG,	0,	's'},
	{"fps",	HAS_ARG,	0,	'r'},
	{"cap-data",	NO_ARG,	0,	'c'},
	{0, 0, 0, 0}
};

static const char *short_options = "d:s:r:c";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0~6", "\tChoose UVC ID"},
	{"WxH", "Setting UVC resolution"},
	{"1~120", "Setting frame rate of stream"},
	{"", "\tWhether caputre camera original YUV, NV12, ME1"},
};

static void usage(void)
{
	int i;
	printf("test_uvc usage:\n");
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
	printf("E.g.: Setting UVC resolution to 640x480, FPS is 30, Capture data\n");
	printf("\t> test_uvc -d 0 -s 640x480 -r 30 -c\n");
}

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

static int init_param(int argc, char **argv)
{
	int i, ch;
	int width, height;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv,
				short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			i = atoi(optarg);
			if ((i < 0) ||(i >= MAX_UVC_ID)) {
				printf("Invalid uvc id : %d. must  0 < id < %d\n", i, MAX_UVC_ID);
				return -1;
			}
			uvc_id = i;
			break;
		case 's':
			if (get_arbitrary_resolution(optarg, &width, &height) < 0) {
				printf("Failed to get resolution from [%s].\n", optarg);
			}
			uvc_width = width;
			uvc_height = height;
			break;
		case 'r':
			i = atoi(optarg);
			if (i < 1 || i > MAX_YUV_FRAME_NUM) {
				printf("Total frame number [%d] must be in the range of [1~120].\n", i);
				return -1;
			}
			frame_rate = i;
			break;
		case 'c':
			capture_data = 1;
			break;

		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	return 0;
}

static int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i = 0;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stderr, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else {
		fprintf(stderr, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "\r");
		dot_count = 0;
	}

	fflush(stderr);
	return 0;
}

static int open_uvc(int uvc_id)
{
	int fd = -1;
	char fs[32];

	snprintf( fs, sizeof(fs), "/dev/video%d", uvc_id);
	if ( (fd = open(fs, O_RDWR /*| O_NONBLOCK*/)) < 0) {
		printf("Try open %s ...\n", fs);
		perror("Failed to open uvc");
	} else {
		printf("open %s success %d\n", fs, fd);
	}

	return fd;
}

static int init_uvc_buf(int fd)
{
	int i = 0;
	struct v4l2_requestbuffers reqbuf;

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.count = 3;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
		perror("Fail to ioctl VIDIOC_REQBUFS");
		return -1;
	}

	uvc_buf_count = reqbuf.count;
	uvc_buf = calloc(reqbuf.count, sizeof(*uvc_buf));

	printf("uvc_buf_count = %d\n", uvc_buf_count);

	if (uvc_buf == NULL) {
		fprintf(stderr,"Out of memory\n");
		return -1;
	}

	for (i = 0; i < reqbuf.count; i++) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("Fail to ioctl VIDIOC_QUERYBUF");
			return -1;
		}
		uvc_buf[i].length = buf.length;
		uvc_buf[i].start = mmap( NULL, /*start anywhere*/
			buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset );

		if (MAP_FAILED == uvc_buf[i].start) {
			printf("i = %d\n",i);
			perror("Fail to mmap\n");
			return -1;
		}
//		printf("uvc_buf start:08%lx\n", uvc_buf[i].start);
	}

	return 0;
}

static int init_uvc_device(int fd)
{
	struct v4l2_fmtdesc fmt;
	struct v4l2_capability cap;
	struct v4l2_format stream_fmt;
	struct v4l2_streamparm stream;

	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	yuv_pts_distance = 90000/frame_rate;

	while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
		fmt.index ++ ;
		printf("pixelformat = %c%c%c%c, description = '%s'\n",
			fmt.pixelformat & 0xff, (fmt.pixelformat >> 8)&0xff,
			(fmt.pixelformat >> 16) & 0xff, (fmt.pixelformat >> 24)&0xff,
			fmt.description);
	}

	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		perror("FAIL to ioctl VIDIOC_QUERYCAP");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("The Current device is not a video capture device\n");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		printf("The Current device does not support streaming i/o\n");
		return -1;
	}

	memset(&stream_fmt, 0 , sizeof(stream_fmt));

	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream_fmt.fmt.pix.width = uvc_width;
	stream_fmt.fmt.pix.height = uvc_height;
	stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //V4L2_PIX_FMT_YUYV;
	stream_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(fd, VIDIOC_S_FMT, &stream_fmt) < 0) {
		perror("Fail to ioctl VIDIOC_S_FMT\n");
		return -1;
	}

	if (ioctl(fd, VIDIOC_G_FMT, &stream_fmt) < 0) {
		perror("Fail to ioctl VIDIOC_G_FMT\n");
		return -1;
	} else {
		printf("fmt.type:\t\t%d\n", stream_fmt.type);
		printf("pix.pixelformat:\t%c%c%c%c\n",
			stream_fmt.fmt.pix.pixelformat & 0xFF,
			(stream_fmt.fmt.pix.pixelformat >> 8) & 0xFF,
			(stream_fmt.fmt.pix.pixelformat >> 16) & 0xFF,
			(stream_fmt.fmt.pix.pixelformat >> 24) & 0xFF);
		printf("pix.height:\t\t%d\n", stream_fmt.fmt.pix.height);
		printf("pix.width:\t\t%d\n", stream_fmt.fmt.pix.width);
		printf("pix.field:\t\t%d\n", stream_fmt.fmt.pix.field);
		printf("fps:\t\t\t%d\n", frame_rate);
		printf("pts_distance:\t\t%u\n", yuv_pts_distance);
	}
	if ( (uvc_width != stream_fmt.fmt.pix.width) ||
			(uvc_height != stream_fmt.fmt.pix.height) ) {
		uvc_width = stream_fmt.fmt.pix.width;
		uvc_height = stream_fmt.fmt.pix.height;
		printf("Resetting width and height. width=%d, height=%d.\n",
			uvc_width, uvc_height);
	}

	init_uvc_buf(fd);
	memset(&stream, 0 , sizeof(stream));
	stream.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream.parm.capture.capturemode = 0;
	stream.parm.capture.timeperframe.numerator = 1;
	stream.parm.capture.timeperframe.denominator = frame_rate;

	if (ioctl(fd, VIDIOC_S_PARM, &stream) < 0) {
		perror("Fail to ioctl VIDIOC_S_PARM");
		return -1;
	}

	return 0;
}

static int prepare_efm_uvc(int w, int h)
{
	u32  total_size;

	frame_size = w*h;
	total_size = frame_size*3/2 ;

	// read YV12 planar format
	yuv_addr = (u8*)malloc(total_size);
	if (!yuv_addr) {
		printf("Failed to allocate buffers.");
		goto ERROR_EXIT;
	}

	frame_start = yuv_addr;
	yuv_end = yuv_addr + total_size;
//	printf ("frame_start=08%lx, yuv_end=08%lx, size=%u.\n",
//			frame_start, yuv_end, frame_size);
	return 0;

ERROR_EXIT:
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	return -1;
}

static int start_uvc_capturing(int fd)
{
	unsigned int i = 0;
	enum v4l2_buf_type type;

	if (fd < 0 ) {
		printf("Not found uvc");
		return -1;
	}

	for (i = 0; i < uvc_buf_count; i++) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
			perror("Fail to ioctl VIDIOC_QBUF\n");
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		printf("i = %d.\n", i);
		perror("Fail to ioctl VIDIOC_STREAMON\n");
		return -1;
	}

	return 0;
}

static void stop_uvc_capturing(int fd)
{
	enum v4l2_buf_type type;
	int i =0;

	if (fd < 0 ) {
		printf("Not find uvc fd");
		return ;
	}
	efm_uvc_task_exit_flag = 1;
	for (i = 0; i < uvc_buf_count; i++ ) {
		if (munmap (uvc_buf[i].start, uvc_buf[i].length) < 0) {
			perror("munmap uvc_buf\n");
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
		perror("Fail to ioctl VIDIOC_STREAMOFF\n");
	}
	if (close(fd) < 0) {
		perror("Fail to close fd uvc");
	}
}

static int debug_capture_yuv(void *addr, u32 length, int yuv_format)
{
	FILE * file_fd = NULL;
	char name[64];

	snprintf(name, sizeof(name), "/tmp/%s_%dx%d.yuv",
		(yuv_format ? "NV12" : "YUYV"),  uvc_width, uvc_height);

	if ((file_fd = fopen(name, "w")) < 0) {
		perror(name);
		return -1;
	}

	fwrite(addr, length, 1, file_fd);
	fclose(file_fd);
	printf("Save YUV data to file [%s] OK.\n", name);

	return 0;
}

static int debug_capture_me1(u8 * data, u16 w, u16 h)
{
	int me1_test_fd = -1;
	char name[32];
	u8 * src = NULL;
	u8 * uv_80 = NULL;
	u32 size = w * h;
	int i = 0;
	static int init_flag = 0;

	if (init_flag == 2) {
		return 0;
	}

	snprintf(name, sizeof(name), "/tmp/me1_%dx%d.yuv", w, h);

	if ((me1_test_fd = open(name, O_CREAT | O_RDWR)) < 0) {
		printf("Failed to open file [%s].\n", name);
		return -1;
	}

	src = data;
	for (i = 0; i < h; i++, src += w*4) {
		write(me1_test_fd, src, w);
	}

	size >>= 1;
	if ((uv_80 = (u8*)malloc(size)) == NULL) {
		printf("Failed to malloc UV data for ME1 buffer!\n");
		close(me1_test_fd);
		me1_test_fd = -1;
		return -1;
	}
	memset(uv_80, 0x80, size);
	write(me1_test_fd, uv_80, size);

	free(uv_80);
	close(me1_test_fd);
	me1_test_fd = -1;

	init_flag++;
	printf("Save ME1 data to file [%s] OK.\n", name);

	return 0;
}

static void yuyv_422_to_nv12_420 (u8* output, u8* input, int w, u32 yuv_size)
{
	u32 i, k;
	u8* src = NULL;
	u8* dst = NULL;

	// Read Y data
	src = input;
	dst = output;
	//yuv_size = width x height
	for (i = 0; i < yuv_size; i++, dst++, src += 2) {
		*dst = *src;
	}

	// Read UV data
	src = input + 1;	// +1 is used for start at U
	dst = output + yuv_size;
	// i is src point, k is used for skip src two row.
	for (i = 0, k = 0; i <= yuv_size /2; i++, k++, dst++, src += 2) {
		if (k == w) {
			k = 0;
			src += w*2;	//skip src two row
		} else {
			*dst = *src;
		}
	}
}

static int feed_buf_from_uvc(iav_enc_dram_buf_frame_ex_t *buf)
{
	#define ROW_MAX	(4)
	#define COL_MAX		(4)
	struct v4l2_buffer vbuf;
	u8* src = NULL;
	u8* dst = NULL;
	u16 yuv_pitch;
	u32 i, j, row, col, me1_data;
	static int need_skip = 0;

	memset(&vbuf, 0, sizeof(vbuf));
	vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd_uvc, VIDIOC_DQBUF, &vbuf) < 0) {
		perror("Fail to ioctl VIDIOC_DQBUF");
		return -1;
	}
	assert(vbuf.index < uvc_buf_count);
	yuv_pitch = buf->max_size.width;

	yuyv_422_to_nv12_420(yuv_addr,
		(u8*)uvc_buf[vbuf.index].start, uvc_width, frame_size);

	for (i = 0; i < (uvc_height / ROW_MAX); ++i) {
		// Read Y data
		src = frame_start + i * ROW_MAX * uvc_width;
		dst = buf->y_addr + i * ROW_MAX * yuv_pitch;
		for (row = 0; row < ROW_MAX; ++row, src += uvc_width, dst += yuv_pitch) {
			memcpy(dst, src, uvc_width);
		}

		// Read UV data
		src = frame_start + (uvc_height + i * ROW_MAX / 2) * uvc_width;
		dst = buf->uv_addr + i * ROW_MAX / 2 * yuv_pitch;
		for (row = 0; row < ROW_MAX / 2; ++row, src += uvc_width, dst += yuv_pitch) {
			memcpy(dst, src, uvc_width);
		}

		// Read ME1 data
		src = frame_start + i * ROW_MAX * uvc_width;
		dst = buf->me1_addr + i * yuv_pitch;
		for (col = 0; col < (uvc_width / COL_MAX); ++col) {
			for (row = 0, j = col * COL_MAX, me1_data = 0;
					row < uvc_width * ROW_MAX; row += uvc_width) {
				me1_data += (src[row + j] + src[row + j + 1] +
					src[row + j + 2] + src[row + j + 3]);
			}
			dst[col] = me1_data >> 4;
		}
	}

	//Capture YUYV, NV12, ME1 data
	if (capture_data && (need_skip == 0)) {
		debug_capture_yuv((char *)uvc_buf[vbuf.index].start,
					uvc_buf[vbuf.index].length, 0);
		debug_capture_yuv((char *)yuv_addr, frame_size*3/2, 1);
		debug_capture_me1(buf->me1_addr, yuv_pitch/COL_MAX,
					uvc_height / ROW_MAX);
		need_skip = 1;
	}

	if (ioctl(fd_uvc, VIDIOC_QBUF, &vbuf) < 0) {
		perror("Fail to ioctl VIDIOC_QBUF");
		return -1;
	}

	return 0;
}

static int feed_efm_data(void)
{
	static u32 pts = 0;
	iav_enc_dram_buf_frame_ex_t buf_frame;
	iav_enc_dram_buf_update_ex_t buf_update;

	memset(&buf_frame, 0, sizeof(buf_frame));
	buf_frame.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX, &buf_frame);

	if (feed_buf_from_uvc(&buf_frame) < 0) {
		printf("feed_buf_from_uvc error\n");
		return -1;
	}

	// Update ME1 & YUV data
	memset(&buf_update, 0, sizeof(buf_update));
	buf_update.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
	buf_update.frame_id = buf_frame.frame_id;
	buf_update.frame_pts = pts;
	buf_update.size.width = uvc_width;
	buf_update.size.height = uvc_height;

	pts += yuv_pts_distance;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX, &buf_update);
	return 0;
}

static void efm_uvc_loop(int fd)
{
	fd_set fds;

	/*Timeout 1 s*/
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	while (!efm_uvc_task_exit_flag) {
		if (select(fd + 1, &fds, NULL, NULL, &tv) < 0) {
			perror("Fail to select\n");
			continue;
		}
		feed_efm_data();
		show_waiting();
	}

	return ;
}

static int map_dsp(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	/* Map for EFM feature */
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("dsp_mem = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);
	mem_mapped = 1;

	return 0;
}

static void sigstop()
{
	stop_uvc_capturing(fd_uvc);

	/* Free up the YUV data reading from file */
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	if (uvc_buf) {
		free(uvc_buf);
		uvc_buf = NULL;
	}

	exit(1);
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if ((fd_uvc = open_uvc(uvc_id)) < 0) {
		return -1;
	}

	if (map_dsp() < 0) {
		printf("map_dsp failed \n");
		return -1;
	}

	if (init_uvc_device(fd_uvc) < 0) {
		printf("init_uvc_device failed \n");
		return -1;
	}

	if (prepare_efm_uvc(uvc_width, uvc_height) < 0) {
		printf("prepare_efm_uvc failed \n");
		return -1;
	}

	if (start_uvc_capturing(fd_uvc) < 0) {
		printf("start_uvc_capturing failed \n");
		return -1;
	}

	efm_uvc_loop(fd_uvc);

	return 0;
}
