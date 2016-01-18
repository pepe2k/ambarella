/*
 * test_vin.c
 *
 * History:
 *	2009/08/05 - [Qiao Wang] create
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
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
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

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
#include "ambas_vin.h"
#include <signal.h>
#include "vin_init.c"
#include "decoder/rn6243.c"

#define NO_ARG	0
#define HAS_ARG	1

// the device file handle
int fd_iav;
int test_rn6243 = 0;
int test_rn6243_mode = 0;
int test_vin_fps_flag = 0;
int test_vin_flip_flag = 0;
int stop_stream_flag = 0;

static struct option long_options[] = {
	{"vin", HAS_ARG, 0, 'i'},
	{"src", HAS_ARG, 0, 'S'},
	{"ch", HAS_ARG, 0, 'C'},
	{"frame-rate", HAS_ARG, 0, 'F'},
	{"mirror_pattern", HAS_ARG, 0, 'm'},
	{"bayer_pattern", HAS_ARG, 0, 'b'},
	{"anti_flicker", HAS_ARG, 0, 'a'},
	{"test-rn6243",   HAS_ARG,  0, 'r'},
	{"test-fps", NO_ARG, 0, 't'},
	{"stop-stream", NO_ARG, 0, 's'},
	{0, 0, 0, 0}
};

static const char *short_options = "i:S:C:F:m:b:a:r:ts";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"vin mode", "\tchange vin mode"},
	{"source", "\tselect source"},
	{"channel", "\tselect channel"},
	{"frate", "\tset encoding frame rate"},
	{"0~3", "set vin mirror pattern"},
	{"0~3", "set vin bayer pattern "},
	{"0~2", "\tset anti-flicker only for sensor that output YUV data, 0:no-anti; 1:50Hz; 2:60Hz"},
	{"0~1",  "test rn6243 decoder, 0:default  1:WEAVE"},
	{"", "test vin fps "},
	{"", "stop sensor output"},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		case 'i':
			vin_flag = 1;
			vin_mode = get_vin_mode(optarg);
			if (vin_mode < 0)
				return -1;
			break;
		case 'C':
			channel = atoi(optarg);
			break;

		case 'S':
			source = atoi(optarg);
			break;

		case 'F':
			framerate_flag = 1;
			framerate = atoi(optarg);
			break;

		case 'm':
			mirror_pattern = atoi(optarg);
			if(mirror_pattern<0 || (mirror_pattern>3 && mirror_pattern != 255))
				return -1;
			mirror_mode.pattern = mirror_pattern;
			test_vin_flip_flag = 1;
			break;

		case 'b':
			bayer_pattern = atoi(optarg);
			if(bayer_pattern<0)
				return -1;
			mirror_mode.bayer_pattern = bayer_pattern;
			test_vin_flip_flag = 1;
			break;
		case 'a':
			anti_flicker= atoi(optarg);
			if(anti_flicker < 0 || anti_flicker > 2)
				return -1;
			break;
		case 'r':	//RN6243
			test_rn6243_mode = atoi(optarg);
			test_rn6243 = 1;

			printf("test_rn6243mode is %d , test is %d \n", test_rn6243_mode, test_rn6243);
			break;

		case 't':
			test_vin_fps_flag = 1;
			break;

		case 's':
			stop_stream_flag = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int test_vin_fps()
{
	struct amba_fps_report_s fps_report;

	printf("Start VIN FPS testing, please wait, it should be OK within 1 minute...\n");


	if (ioctl(fd_iav, IAV_IOC_START_VIN_FPS_STAT, 0) < 0) {
		perror("IAV_IOC_START_VIN_FPS_STAT\n");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_GET_VIN_FPS_STAT, &fps_report) < 0) {
		perror("IAV_IOC_GET_VIN_FPS_STAT\n");
		return -1;
	}

	if ((fps_report.avg_frame_diff == 0) || (fps_report.min_frame_diff == 0)||
		(fps_report.max_frame_diff == 0)) {
		printf("unexpected, frame diff is zero \n");
		return -1;
	} else {
		printf("AVG fps is %4.6f\n",  (double)fps_report.counter_freq/(double)fps_report.avg_frame_diff);
		printf("MIN fps is %4.6f\n",  (double)fps_report.counter_freq/(double)fps_report.max_frame_diff);
		printf("MAX fps is %4.6f\n",  (double)fps_report.counter_freq/(double)fps_report.min_frame_diff);
	}

	return 0;
}

int test_vin_flip(void)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_MIRROR_MODE, &mirror_mode) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_MIRROR_MODE(Add by BinWang!)");
		return -1;
	}
	return 0;
}

int stop_sensor_stream(void)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_STOP_SENSOR_STREAM, 0) < 0) {
		perror("IAV_IOC_VIN_SRC_STOP_SENSOR_STREAM\n");
		return -1;
	}
	return 0;
}

void usage(void)
{
	int i;

	printf("test_vin usage:\n");
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

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("\n");
			printf("           ");
		} else
			printf("%s  ", __vin_modes[i].name);
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	//just fill rn6243 register
	if (test_rn6243) {
		if (init_rn6243(test_rn6243_mode) < 0)
			return -1;
		else
			return 0;
	}

	// select channel: for A3 DVR
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	// switch vin operate mode
	if (sensor_operate_mode_changed) {
		if (set_vin_linear_wdr_mode(sensor_operate_mode) < 0) {
			return -1;
		}
	}

	// set vin
	if (vin_flag) {
		if (init_vin(vin_mode) < 0)
			return -1;
	}

	//test vin fps
	if (test_vin_fps_flag) {
		if(test_vin_fps() < 0)
			return -1;
	}

	//test vin flip
	if (test_vin_flip_flag) {
		if (test_vin_flip() < 0) {
			return -1;
		}
	}

	//stop sensor output
	if (stop_stream_flag) {
		if (stop_sensor_stream() < 0) {
			return -1;
		}
	}
	close(fd_iav);
	return 0;
}

