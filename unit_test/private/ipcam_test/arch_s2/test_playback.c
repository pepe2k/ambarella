/**********************************************************************
 * test_playback.c
 *
 * History:
 *	2008/04/21 - [Oliver Li] created file
 *	2011/02/21 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 **********************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"

#include <signal.h>
#include <pthread.h>

int fd_iav;
#include "../../vout_test/vout_init.c"


// the bitstream buffer
static u8 *bsb_mem;
static u32 bsb_size;

enum {
	DEC_NOTHING,
	DEC_H264,
	DEC_JPEG,
};

enum {
	DEC_SPEED_1X = 0,
	DEC_SPEED_2X,
	DEC_SPEED_4X,
	DEC_SPEED_1BY2,
	DEC_SPEED_1BY4,
};

enum {
	DEC_MODE_START,
	DEC_MODE_PAUSE,
	DEC_MODE_RESUME,
};

int decode_flag = DEC_NOTHING;
int decode_task_exit_flag = 0;
pthread_t decode_task_id = 0;

static int repeat_times = 1;

char filename[256];
const char *default_jpeg_filename = "/mnt/media/test.jpg";
const char *default_h264_filename = "/mnt/media/test";

static iav_h264_config_ex_t h264_config;
static u32 config_size;

static int decoded_frame_count = 0;

#define NO_ARG				0
#define HAS_ARG				1
#define VOUT_OPTIONS_BASE			20
#define DECODING_OPTIONS_BASE		130

enum numeric_short_options {
	//Vout
	VOUT_NUMERIC_SHORT_OPTIONS,

	//Decoding
};

static struct option long_options[] = {
	//Decoding
	{"filename", HAS_ARG, 0, 'f'},
	{"repeat", HAS_ARG, 0, 'r'},

	{"h264", NO_ARG, 0, 'h'},
	{"jpeg", NO_ARG, 0, 'j'},

	//Vout
	VOUT_LONG_OPTIONS()

	{0, 0, 0, 0}
};

static const char *short_options = "f:hjr:v:V:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	//Decoding
	{"file",	"\t"	"specify filename."},
	{"times",	"\t"	"repeat playback times, 0 means forever."},

	{"",		"\t\t"	"decode h.264"},
	{"",		"\t\t"	"decode jpeg\n"},

	//Vout
	VOUT_PARAMETER_HINTS()
};

int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DECODE_BSB, &info) < 0) {
		perror("IAV_IOC_MAP_DECODE_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;
	mem_mapped = 1;

	memset(bsb_mem, 0, bsb_size);

	printf("[map_bsb] bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	return 0;
}

int map_dsp(void)
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

	printf("dsp_mem = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);
	return 0;
}

int do_goto_idle(void)
{
	if (ioctl(fd_iav, IAV_IOC_ENTER_IDLE, 0) < 0) {
		perror("IAV_IOC_ENTER_IDLE");
		return -1;
	}

	return 0;
}

int do_vout_setup(void)
{
	//check vout
	if (check_vout() < 0)
		return -1;

	//Dynamically change vout
	if (dynamically_change_vout())
		return 0;

	//Init vout
	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;
	return 0;
}

int select_channel(void)
{
	int channel = 0;

	if (ioctl(fd_iav, IAV_IOC_SELECT_CHANNEL, IAV_DEC_CHANNEL(channel)) < 0) {
		perror("IAV_IOC_SELECT_CHANNEL");
		return -1;
	}

	return 0;
}

int enter_decoding(void)
{
	ioctl(fd_iav, IAV_IOC_STOP_DECODE, 0);

	if (ioctl(fd_iav, IAV_IOC_START_DECODE, 0) < 0) {
		perror("IAV_IOC_START_DECODE");
		return -1;
	}

	return 0;
}

int get_file_size(int fd)
{
	struct stat stat;

	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		return -1;
	}

	return stat.st_size;
}

int open_file(const char *name, u32 *size)
{
	int fd;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		perror(name);
		return -1;
	}

	if (size != NULL)
		*size = get_file_size(fd);

	return fd;
}

int read_file(int fd, void *buffer, u32 size)
{
	if (read(fd, buffer, size) != size) {
		perror("read");
		return -1;
	}

	return 0;
}

int decode_jpeg(void)
{
	int fd;
	u32 size;
	u32 round_size;
	iav_jpeg_info_t info;

	if (filename[0] == '\0')
		strcpy(filename, default_jpeg_filename);

	if ((fd = open_file(filename, &size)) < 0)
		return -1;

	if (read_file(fd, bsb_mem, size) < 0) {
		close(fd);
		return -1;
	}

	close(fd);

	round_size = (size + 31) & ~31;
	memset(bsb_mem + size, 0, round_size - size);
	size = round_size;

	info.start_addr = bsb_mem;
	info.size = size;
	if (ioctl(fd_iav, IAV_IOC_DECODE_JPEG, &info) < 0) {
		perror("IAV_IOC_DECODE_JPEG");
		return -1;
	}

	printf("decode %s done\n", filename);
	return 0;
}

int info_file_size(void)
{
	return 2*sizeof(int) + config_size;
}

u8 *copy_to_bsb(u8 *ptr, u8 *buffer, u32 size)
{
	if (ptr + size <= bsb_mem + bsb_size) {
		memcpy(ptr, buffer, size);
		return ptr + size;
	} else {
		int room = (bsb_mem + bsb_size) - ptr;
		u8 *ptr2;
		memcpy(ptr, buffer, room);
		ptr2 = buffer + room;
		size -= room;
		memcpy(bsb_mem, ptr2, size);
		return bsb_mem + size;
	}
}

#define GOP_HEADER_SIZE		22
u8 *fill_gop_header(u8 *ptr, u32 pts)
{
	u8 buf[GOP_HEADER_SIZE];
	u32 tick_high = (h264_config.pic_info.rate >> 16) & 0x0000ffff;
	u32 tick_low = h264_config.pic_info.rate & 0x0000ffff;
	u32 scale_high = (h264_config.pic_info.scale >> 16) & 0x0000ffff;
	u32 scale_low = h264_config.pic_info.scale & 0x0000ffff;
	u32 pts_high = (pts >> 16) & 0x0000ffff;
	u32 pts_low = pts & 0x0000ffff;

	buf[0] = 0;			// start code
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 1;

	buf[4] = 0x7a;			// NAL header
	buf[5] = 0x01;			// version main
	buf[6] = 0x01;			// version sub

	buf[7] = tick_high >> 10;
	buf[8] = tick_high >> 2;
	buf[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
	buf[10] = tick_low >> 3;

	buf[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
	buf[12] = scale_high >> 4;
	buf[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
	buf[14] = scale_low >> 5;

	buf[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
	buf[16] = pts_high >> 6;

	buf[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
	buf[18] = pts_low >> 7;
	buf[19] = (pts_low << 1) | 1;

	buf[20] = h264_config.N;
	buf[21] = (h264_config.M << 4) & 0xf0;

	return copy_to_bsb(ptr, buf, sizeof(buf));
}

u8 *fill_eos(u8 *ptr)
{
	static u8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
	return copy_to_bsb(ptr, eos, sizeof(eos));
}

typedef struct video_frame_s {
	u32	flags;
	u32	pts;
	u32	size;
	u32	seq;
} video_frame_t;

typedef struct decode_var_s {
	int	fd;
	int	fd_size;
	u32	info_size;
	int	frame_count;
} decode_var_t;

decode_var_t Gvar;

int open_h264_files(void)
{
	int filename_len, filetype_len;
	char full_filename[256];
	char filetype[] = ".h264";

	if (filename[0] == '\0')
		strcpy(filename, default_h264_filename);

	filename_len = strlen(filename);
	filetype_len = strlen(filetype);
	if (filename_len < filetype_len || strcmp(filename + filename_len - filetype_len, filetype) != 0) {
		sprintf(full_filename, "%s%s", filename, filetype);
		if ((Gvar.fd = open_file(full_filename, NULL))  < 0)
			return -1;

		sprintf(full_filename, "%s%s.info", filename, filetype);
		if ((Gvar.fd_size = open_file(full_filename, &Gvar.info_size)) < 0)
			return -1;
	} else {
		if ((Gvar.fd = open_file(filename, NULL)) < 0)
			return -1;

		sprintf(full_filename, "%s.info", filename);
		if ((Gvar.fd_size = open_file(full_filename, &Gvar.info_size)) < 0)
			return -1;
	}

	return 0;
}

int close_h264_files(void)
{
	close(Gvar.fd_size);
	close(Gvar.fd);
	return 0;
}

#define VERSION	0x00000005
int read_h264_header_info(void)
{
	int version;

	if ((read(Gvar.fd_size, &version, sizeof(version)) < 0) ||
		(read(Gvar.fd_size, &config_size, sizeof(config_size)) < 0)) {
		perror("read");
		return -1;
	}

	if (version != VERSION) {
		printf("version [%x] is not correct, decode version is [%x].\n", version, VERSION);
		return -1;
	}

	if (read(Gvar.fd_size, &h264_config, config_size) < 0) {
		perror("read");
		return -1;
	}

	Gvar.frame_count = (Gvar.info_size - info_file_size()) / sizeof(video_frame_t);
	if (Gvar.frame_count <= 0) {
		printf("file is empty\n");
		return -1;
	}

	printf("\n============ Decode H.264 file ============\n");
	printf("                    file = [%s]\n", filename);
	printf("            total frames = %d\n", Gvar.frame_count);
	printf("                       M = %d\n", h264_config.M);
	printf("                       N = %d\n", h264_config.N);
	printf("               gop_model = 0x%x\n", h264_config.gop_model);
	printf("            idr_interval = %d\n", h264_config.idr_interval);
	printf("                 bitrate = %d\n", h264_config.average_bitrate);
	printf("              frame_mode = %d\n", h264_config.pic_info.frame_mode);
	printf("                   scale = %d\n", h264_config.pic_info.scale);
	printf("                    rate = %d\n", h264_config.pic_info.rate);
	printf("                   width = %d\n", h264_config.pic_info.width);
	printf("                  height = %d\n", h264_config.pic_info.height);
	printf("===========================================\n");

	return 0;
}

int create_output_file(const char *filename)
{
	int fd;

	if ((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(filename);
		return -1;
	}

	return fd;
}

int decode_frame(u8 *start_addr, u8 *end_addr, u32 num_pics)
{
	iav_h264_decode_t decode_info;

	decode_info.start_addr = start_addr;
	decode_info.end_addr = end_addr;
	decode_info.first_display_pts = 0;
	decode_info.num_pics = num_pics;
	decode_info.next_size = 0;
	decode_info.pic_width = 0;
	decode_info.pic_height = 0;

	if (ioctl(fd_iav, IAV_IOC_DECODE_H264, &decode_info) < 0) {
		perror("IAV_IOC_DECODE_H264");
		return -1;
	}

	++decoded_frame_count;

	return 0;
}

int config_decoder_by_header(void)
{
	iav_config_decoder_t config;
	memset(&config, 0, sizeof(config));

	config.flags = 0;
	if (h264_config.pic_info.width == 0 || h264_config.pic_info.height == 0) {
		config.pic_width = 1280;
		config.pic_height = 720;
	} else {
		config.pic_width = h264_config.pic_info.width;
		config.pic_height = h264_config.pic_info.height;
	}

	if (ioctl(fd_iav, IAV_IOC_CONFIG_DECODER, &config) < 0) {
		perror("IAV_IOC_CONFIG_DECODER");
		return -1;
	}

	return 0;
}

void decode_h264(void * arg)
{
	video_frame_t frame;
	u8 *ptr;
	u8 *frame_start_ptr;
	int frame_index;
	iav_wait_decoder_t wait;
	iav_decode_info_t info;
	u32 total_bytes, repeat = 1;

	u32 first_pts = 0;
	u32 begin_pts = 0;

	ptr = bsb_mem;
	repeat = repeat_times;
	memset(bsb_mem, 0, bsb_size);
	while (1) {
		frame_index = 0;
		total_bytes = 0;
		lseek(Gvar.fd, 0, SEEK_SET);
		lseek(Gvar.fd_size, info_file_size(), SEEK_SET);

		while (1) {
			frame_start_ptr = ptr;

			// read frame info
			if (read_file(Gvar.fd_size, &frame, sizeof(frame)) < 0)
				goto error_exit;

			// wait bsb, or decoded frame
			while (1) {
				wait.flags = IAV_WAIT_BSB;
				wait.emptiness.room = frame.size + GOP_HEADER_SIZE;
				wait.emptiness.start_addr = ptr;

				if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, &wait) < 0) {
					if (errno != EAGAIN) {
						perror("IAV_IOC_WAIT_DECODER");
						goto error_exit;
					}
					break;
				}
				if (wait.flags == IAV_WAIT_BSB) {
					// bsb has room, continue
					break;
				}
			}
			if (frame_index == 0) {
				first_pts = frame.pts;
			}
			// fill GOP header before IDR
			if (frame.flags == IDR_FRAME)
				ptr = fill_gop_header(ptr, frame.pts+begin_pts);

			// read data into bsb
			if (ptr + frame.size <= bsb_mem + bsb_size) {
				if (read_file(Gvar.fd, ptr, frame.size) < 0)
					goto error_exit;
				ptr += frame.size;
			} else {
				printf(" wrap around at frame [%d].\n", frame_index + 1);
				u32 size = (bsb_mem + bsb_size) - ptr;
				if (read_file(Gvar.fd, ptr, size) < 0)
					goto error_exit;
				size = frame.size - size;
				if (read_file(Gvar.fd, bsb_mem, size) < 0)
					goto error_exit;
				ptr = bsb_mem + size;
			}

			// decode the frame
			if (ioctl(fd_iav, IAV_IOC_GET_DECODE_INFO, &info) < 0) {
				perror("IAV_IOC_GET_DECODE_INFO");
			}
			//printf("== [%4d] frame NO. : %d, PTS : %08d.\n", frame_index,
			//	info.decoded_frames, info.curr_pts);
			if (decode_task_exit_flag || (decode_frame(frame_start_ptr, ptr, 1) < 0))
				goto error_exit;

			total_bytes += frame.size;
			frame_index++;

			if (frame_index == Gvar.frame_count) {
				break;
			}
		}

		// send EOS in the end of file
		{
			frame_start_ptr = ptr;
			ptr = fill_eos(ptr);
			if (decode_frame(frame_start_ptr, ptr, 0) < 0) {
				printf("Failed to send EOS frame in the end of file.\n");
				break;
			}
			--decoded_frame_count;
		}

		printf("  decoded_frame_count = %d / %d.\n", decoded_frame_count,
			Gvar.frame_count);

		if (repeat) {
			begin_pts += frame.pts - first_pts + h264_config.pic_info.rate / 2;
			if (--repeat <= 0) {
				break;
			}
			decoded_frame_count = 0;
		}

		printf("repeat for %d times.\n", repeat);
	}

error_exit:
	printf("  decoded_frame_count = %d / %d.\n", decoded_frame_count,
		Gvar.frame_count);
	while (!decode_task_exit_flag) {
		sleep(1);
	}
}

int display_decode_h264_menu(void)
{
	printf("\n================ Decode H.264 control ================\n");
	printf("  s -- Start to decode H.264 file\n");
	printf("  R -- Restart to decode H.264 file\n");
	printf("  p -- Pause decoding\n");
	printf("  r -- Resume to decode\n");
	printf("  f -- Fast play forward\n");
	printf("  q -- Quit decode control\n");
	printf("======================================================\n");
	printf("> ");
	return 0;
}

int create_decode_task(void)
{
	decode_task_exit_flag = 0;
	if (decode_task_id == 0) {
		if (pthread_create(&decode_task_id, NULL, (void *)decode_h264, NULL) != 0) {
			printf("Failed, cannot create thread <decode_h264_task> !\n");
		}
	}
	return 0;
}

int destroy_decode_task(void)
{
	decode_task_exit_flag = 1;
	if (decode_task_id != 0) {
		if (pthread_join(decode_task_id, NULL) != 0) {
			printf("Failed, cannot destroy thread <decode_h264_task> !\n");
		}
	}
	ioctl(fd_iav, IAV_IOC_STOP_DECODE, 0);
	decoded_frame_count = 0;
	decode_task_exit_flag = 0;
	decode_task_id = 0;
	return 0;
}

int set_decode_speed(int speed)
{
	int play_speed = 0x100;
	iav_trick_play_t trick_play;

	switch (speed) {
	case DEC_SPEED_1X:
		break;
	case DEC_SPEED_2X:
		play_speed <<= 1;
		break;
	case DEC_SPEED_4X:
		play_speed <<= 2;
		break;
	case DEC_SPEED_1BY2:
		play_speed >>= 1;
		break;
	case DEC_SPEED_1BY4:
		play_speed >>= 2;
		break;
	default:
		printf(" Invalid speed option : %d.\n", speed);
		return -1;
		break;
	}
	trick_play.speed = play_speed;
	trick_play.scan_mode = 1;
	trick_play.direction = 0;
	if (ioctl(fd_iav, IAV_IOC_TRICK_PLAY, &trick_play) < 0) {
		perror("IAV_IOC_TRICK_PLAY");
		return -1;
	}
	printf(" [Playback speed] 0x%x / 0x100 = %d / 16.\n", play_speed,
		(play_speed * 16 / 0x100));

	return 0;
}

int do_decode_h264(void)
{
	char ch;
	int i, error_opt, quit_flag = 0;

	if (open_h264_files() < 0)
		return -1;

	if (read_h264_header_info() < 0)
		return -1;

	if (config_decoder_by_header() < 0)
		return -1;

	display_decode_h264_menu();
	while ((ch = getchar())) {
		error_opt = 0;
		switch (ch) {
		case 's':
			printf("\n  Start to decode H.264 file [%s].\n", filename);
			create_decode_task();
			break;
		case 'R':
			destroy_decode_task();
			create_decode_task();
			printf("\n  Re-start to decode H.264 file [%s].\n", filename);
			break;
		case 'p':
			printf("\n  Pause to decode H.264 file.\n");
			if (ioctl(fd_iav, IAV_IOC_DECODE_PAUSE, 0) < 0) {
				perror("IAV_IOC_DECODE_PAUSE");
				return -1;
			}
			break;
		case 'r':
			printf("\n  Resume to decode H.264 file.\n");
			if (ioctl(fd_iav, IAV_IOC_DECODE_RESUME, 0) < 0) {
				perror("IAV_IOC_DECODE_RESUME");
				return -1;
			}
			break;
		case 'f':
			printf("\n  Fast play forward (0 - 1X, 1 - 2X, 2 - 4X, 3 - 0.5X, 4 - 0.25X)\n");
			scanf("%d", &i);
			if (set_decode_speed(i) < 0)
				return -1;
			break;
		case 'q':
			quit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (quit_flag)
			break;
		if (error_opt == 0) {
			display_decode_h264_menu();
		}
	}

	destroy_decode_task();
	close_h264_files();
	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		//Vout
		VOUT_INIT_PARAMETERS()

		//Decoding
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'r':
			repeat_times = atoi(optarg);
			break;
		case 'h':
			decode_flag = DEC_H264;
			break;
		case 'j':
			decode_flag = DEC_JPEG;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

void usage(void)
{
	int i;

	printf("test_playback usage:\n");
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

int open_iav(void)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	return fd_iav;
}

void sigstop()
{
	if (destroy_decode_task() < 0) {
		printf("Stop decode h264 failed!\n");
	}
	exit(1);
}

int main(int argc, char **argv)
{
	// register signal handler for Ctrl+C, Ctrl+'\', and 'kill' sys command
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	// open the device
	if (open_iav() < 0)
		return -1;

	if (init_param(argc, argv) < 0)
		return -1;

	if (decode_flag != DEC_NOTHING)
		if (map_bsb() < 0)
			return -1;

	if (do_goto_idle() < 0)
		return -1;

	if (do_vout_setup() < 0)
		return -1;

	if (select_channel() < 0)
		return -1;

	if (decode_flag != DEC_NOTHING)
		if (enter_decoding() < 0)
			return -1;

	if (decode_flag == DEC_JPEG) {
		if (decode_jpeg() < 0)
			return -1;
	}

	if (decode_flag == DEC_H264) {
		if (do_decode_h264() < 0)
			return -1;
	}

	return 0;
}


