/*******************************************************************************
 * test_eis_warp.c
 *
 * History:
 *  Oct 25, 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <basetypes.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "utils.h"
#include "ambas_vin.h"
#include "iav_drv.h"
#include "utils.h"
#include "lib_eis.h"

#define FILENAME_LENGTH			(256)
#define DIV_ROUND(divident, divider)   (((divident)+((divider)>>1)) / (divider))

static int fd_iav = -1;
static int fd_eis = -1;
static int fd_hwtimer = -1;
static int cali_num = 3000;
static int debug_level = LOG_INFO;
static char save_file[FILENAME_LENGTH] = {0};
static int save_file_flag = 0;
static FILE *fd_save_file = NULL;
static char input[16] = {0};
static int calibrate_done = 0;

eis_setup_t EIS_SETUP = { // MPU6000 on A5s board, AR0330, 2304x1296
        .accel_full_scale_range = 2,
        .accel_lsb = 16384,
        .gyro_full_scale_range = 250.0,
        .gyro_lsb = 131.0,
        .gyro_sample_rate_in_hz = 2000,
        .gyro_shift = {
                .xg = -594,
                .yg = -266,
                .zg = -23,
                .xa = 464,
                .ya = 40,
                .za = 14175,
        },
        .gravity_axis = AXIS_Z,
        .lens_focal_length_in_um = 8000,
        .threshhold = 0.00001,
        .frame_buffer_num = 60,
};

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"gravity", HAS_ARG, 0, 'g'},
	{"cali", HAS_ARG, 0, 'c'},
	{"focal", HAS_ARG, 0, 'F'},
	{"debug", HAS_ARG, 0, 'd'},
	{"save", HAS_ARG, 0, 's'},
	{"threshhold", HAS_ARG, 0, 't'},
	{"frame-buf-num", HAS_ARG, 0, 'f'},
	{"help", NO_ARG, 0, 'h'},

	{0, 0, 0, 0}
};

static const char *short_options = "g:c:F:d:s:t:f:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1|2", "\tThe axis of gravity. 0: Z, 1: Y, 2: X"},
	{">0", "\tData number to calibrate gyro shift. The default is 3000."},
	{">0", "\tLens focal length in um."},
	{"0~2", "\tlog level. 0: error, 1: info (default), 2: debug"},
	{"", "\tset file name to save gyro data"},
	{"", "\tset threshhold"},
	{"", "\tset Frame buffer num"},
	{"", "\thelp"},
};

static void usage(void)
{
	int i;

	printf("test_eis_warp usage:\n");
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
	printf("\nExamples:");
	printf("\n  test_encode --cmd-read 4 -i 2048x1536 -V720p --hdmi --mixer 0 -J --btype off -X --bsize 1920x1088 --bmaxsize 1920x1088 -Y --btype off -K --btype enc --bsize 1080p --bmaxsize 1080p --bunwarp 1 --binoffset 64x228 --binsize 1080p --vout-swap 1 -w 2048 -W1920 -P --bsize 2048x1536 --bins 2048x1536 --bino 0x0 --sharp 1 --enc-mode 1 --eis-delay-count 1 -B --smaxsize 1080p\n");
	printf("\n");
	printf("\n  AR0330 on main board:\n    test_eis_warp\n");
	printf("\n  AR0330 on sensor board:\n    test_eis_warp -g 1 -c\n");
	printf("\n  Stream A is corrected, Stream B is uncorrected:");
	printf("\n    test_encode -A -h1080p --offset 0x4 -e -B -h1080p -b3 -e\n");
	printf("\n");
}

static int init_param(int argc, char** argv)
{
	int ch;
	int option_index = 0;
	int file_name_len = 0;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'g':
			EIS_SETUP.gravity_axis = atoi(optarg);
			break;
		case 'c':
			cali_num = atoi(optarg);
			break;
		case 'F':
			EIS_SETUP.lens_focal_length_in_um = atoi(optarg);
			break;
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 's':
			file_name_len = strlen(optarg);
			strncpy(save_file, optarg, file_name_len > FILENAME_LENGTH ? FILENAME_LENGTH :file_name_len);
			save_file_flag = 1;
			break;
		case 't':
			EIS_SETUP.threshhold = atof(optarg);
			break;
		case 'f':
			EIS_SETUP.frame_buffer_num = atoi(optarg);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

inline int set_eis_warp(const iav_warp_control_ex_t* warp_control)
{
	DEBUG("warp:  input %dx%d+%dx%d, output %dx%d, hor enable %d (%dx%d),"
			" ver enable %d (%dx%d)\n",
			warp_control->area[0].input.width,
			warp_control->area[0].input.height,
			warp_control->area[0].input.x, warp_control->area[0].input.y,
			warp_control->area[0].output.width,
			warp_control->area[0].output.height,
			warp_control->area[0].hor_map.enable,
			warp_control->area[0].hor_map.output_grid_row,
			warp_control->area[0].hor_map.output_grid_col,
			warp_control->area[0].ver_map.enable,
			warp_control->area[0].ver_map.output_grid_row,
			warp_control->area[0].ver_map.output_grid_col);

	if (ioctl(fd_iav, IAV_IOC_SET_WARP_CONTROL_EX, warp_control) < 0) {
		perror("IAV_IOC_SET_WARP_CONTROL_EX");
		return -1;
	}
	DEBUG("IAV_IOC_SET_WARP_CONTROL_EX\n");
	return 0;
}

inline int get_eis_stat(amba_eis_stat_t* eis_stat)
{
	int i = 0;
	u64 hw_pts = 0;
	char pts_buf[32];
	if (ioctl(fd_eis, AMBA_IOC_EIS_GET_STAT, eis_stat) < 0) {
		perror("AMBA_IOC_EIS_GET_STAT\n");
		return -1;
	}
	if (unlikely(eis_stat->discard_flag || eis_stat->gyro_data_count == 0)) {
		DEBUG("stat discard\n");
		return -1;
	}

	if (save_file_flag && calibrate_done) {
		read(fd_hwtimer, pts_buf, sizeof(pts_buf));
		hw_pts = strtoull(pts_buf,(char **)NULL, 10);
		fprintf(fd_save_file, "===pts:%llu\n", hw_pts);
		lseek(fd_hwtimer, 0L, SEEK_SET);

		for (i = 0; i < eis_stat->gyro_data_count; i++) {
			fprintf(fd_save_file, "%d\t%d\t%d\t%d\t%d\t%d\n", eis_stat->gyro_data[i].xg,
				eis_stat->gyro_data[i].yg, eis_stat->gyro_data[i].zg, eis_stat->gyro_data[i].xa,
				eis_stat->gyro_data[i].ya, eis_stat->gyro_data[i].za);
		}
	}
	DEBUG("AMBA_IOC_EIS_GET_STAT: count %d\n", eis_stat->gyro_data_count);
	return 0;
}

static float change_fps_to_hz(u32 fps_q9)
{
	switch(fps_q9) {
	case AMBA_VIDEO_FPS_29_97:
		return 29.97f;
	case AMBA_VIDEO_FPS_59_94:
		return 59.94f;
	default:
		return DIV_ROUND(512000000, fps_q9);
	}

	return 0;
}

static int prepare(void)
{
	iav_source_buffer_setup_ex_t buffer_setup;
	struct amba_video_info vin_info;
	struct amba_vin_eis_info vin_eis_info;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if ((fd_eis = open("/dev/eis", O_RDWR, 0)) < 0) {
		perror("open /dev/eis");
		return -1;
	}

	if ((fd_hwtimer = open("/proc/ambarella/ambarella_hwtimer", O_RDWR, 0)) < 0) {
		perror("open /proc/ambarella/ambarella_hwtimer");
		return -1;
	}

	if (save_file_flag){
		if ((fd_save_file = fopen(save_file, "w+")) == NULL) {
			perror(save_file);
			return -1;
		}
	}
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_EIS_INFO, &vin_eis_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_EIS_INFO");
		return -1;
	}

	EIS_SETUP.vin_width = vin_info.width;
	EIS_SETUP.vin_height = vin_info.height;
	EIS_SETUP.vin_col_bin = vin_eis_info.column_bin;
	EIS_SETUP.vin_row_bin = vin_eis_info.row_bin;
	EIS_SETUP.vin_cell_width_in_um = (float) vin_eis_info.sensor_cell_width / 100.0;
	EIS_SETUP.vin_cell_height_in_um = (float) vin_eis_info.sensor_cell_height / 100.0;
	EIS_SETUP.vin_frame_rate_in_hz = change_fps_to_hz(vin_info.fps);
	EIS_SETUP.vin_vblank_in_ms = vin_eis_info.row_time * vin_eis_info.vb_lines / 1000000;

	EIS_SETUP.premain_input_width = buffer_setup.pre_main_input.width;
	EIS_SETUP.premain_input_height = buffer_setup.pre_main_input.height;
	EIS_SETUP.premain_input_offset_x = buffer_setup.pre_main_input.x;
	EIS_SETUP.premain_input_offset_y = buffer_setup.pre_main_input.y;

	EIS_SETUP.premain_width = buffer_setup.pre_main.width;
	EIS_SETUP.premain_height = buffer_setup.pre_main.height;
	EIS_SETUP.output_width = buffer_setup.size[0].width;
	EIS_SETUP.output_height = buffer_setup.size[0].height;

	return 0;
}


void calibrate(void)
{
	int i, n = 0;
	s32 sum_xg = 0, sum_yg = 0, sum_zg = 0, sum_xa = 0, sum_ya = 0, sum_za = 0;
	amba_eis_stat_t eis_stat;

	while (n < cali_num) {
		if (get_eis_stat(&eis_stat) == 0) {
			for (i = 0; i < eis_stat.gyro_data_count; i++) {
				sum_xg += eis_stat.gyro_data[i].xg;
				sum_yg += eis_stat.gyro_data[i].yg;
				sum_zg += eis_stat.gyro_data[i].zg;
				sum_xa += eis_stat.gyro_data[i].xa;
				sum_ya += eis_stat.gyro_data[i].ya;
				sum_za += eis_stat.gyro_data[i].za;
			}
			n += eis_stat.gyro_data_count;
		}
	}
	EIS_SETUP.gyro_shift.xg = sum_xg / n;
	EIS_SETUP.gyro_shift.yg = sum_yg / n;
	EIS_SETUP.gyro_shift.zg = sum_zg / n;
	EIS_SETUP.gyro_shift.xa = sum_xa / n;
	EIS_SETUP.gyro_shift.ya = sum_ya / n;
	EIS_SETUP.gyro_shift.za = sum_za / n;

	INFO("Calibration: accel ( %d, %d, %d ), gyro ( %d, %d, %d )\n",
	    EIS_SETUP.gyro_shift.xa, EIS_SETUP.gyro_shift.ya,
	    EIS_SETUP.gyro_shift.za, EIS_SETUP.gyro_shift.xg,
	    EIS_SETUP.gyro_shift.yg, EIS_SETUP.gyro_shift.zg);
}

static void sigstop()
{
	if (fd_save_file) {
		fclose(fd_save_file);
	}
	eis_enable(EIS_DISABLE);
	if (atoi(input)) {
		eis_close();
	}
	if (fd_hwtimer > 0) {
		close(fd_hwtimer);
	}
	exit(0);
}

int main(int argc,  char* argv[])
{
	version_t version;

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	set_log(debug_level, NULL);
	eis_version(&version);
	printf("\nLibrary Version: %s-%d.%d.%d (Last updated: %x)\n\n",
	    version.description, version.major, version.minor,
	    version.patch, version.mod_time);

	if (prepare() < 0)
		return -1;

	if (cali_num > 0) {
		calibrate();
		calibrate_done = 1;
	}

	if (eis_setup(&EIS_SETUP, set_eis_warp, get_eis_stat) < 0)
		return -1;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	while (1) {
		printf(" enable (1: pitch, disable (0):");
		scanf("%s", input);
		if (atoi(input)) {
			if (eis_open() < 0) {
				return -1;
			}
			eis_enable(atoi(input));
		} else {
			eis_close();
		}
	}

	return 0;
}
