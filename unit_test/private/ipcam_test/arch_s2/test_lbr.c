/*
 * test_lbr.c
 *
 * History:
 *	2014/02/17 - [Louis Sun] created file
 *	2014/08/18 - [Bin Wang] modified this file and added LBR auto control demo
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
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
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "lbr_api.h"  //LBR is implementation for smartAVC
#include "md_motbuf.h"
#define NO_ARG	0
#define HAS_ARG	1
#define LBR_MOTION_NONE_DELAY 120
#define LBR_MOTION_LOW_DELAY 60
#define LBR_MOTION_HIGH_DELAY 0
#define LBR_NOISE_NONE_DELAY 120
#define LBR_NOISE_LOW_DELAY 60
#define LBR_NOISE_HIGH_DELAY 0

struct hint_s
{
	const char *arg;
	const char *str;
};

static int fd_iav = -1;
static const char *short_options = "b:m:n:t:l:a:ABCDT";
//support initial config setting for all four streams.
static int lbr_bitrate_ceiling[LBR_MAX_STREAM_NUM] = {0};
static int lbr_bitrate_ceiling_flag[LBR_MAX_STREAM_NUM] = {0};
static int lbr_style[LBR_MAX_STREAM_NUM] = {0};
static int lbr_style_flag[LBR_MAX_STREAM_NUM] = {0};
static int lbr_motion_level[LBR_MAX_STREAM_NUM] = {0};
static int lbr_motion_level_flag[LBR_MAX_STREAM_NUM] = {0};
static int lbr_noise_level[LBR_MAX_STREAM_NUM] = {0};
static int lbr_noise_level_flag[LBR_MAX_STREAM_NUM] = {0};
static int lbr_auto_run_flag = 0;
static int lbr_stress_test_flag = 0;
static int stream_selection = -1;
static int current_stream_id = 0;
static int log_level = 1;
static int auto_run = 0;
static u32 motion_value = 0;
static int noise_value = 0;
static u32 motion_low_threshold = 2000;
static u32 motion_high_threshold = 5000;
static int noise_low_threshold = 1;
static int noise_high_threshold = 6;
static pthread_t lbr_control_thread_id = 0;
static int lbr_control_exit_flag = 0;
static md_motbuf_init_t init_mdbuf = {
	.me_buffer = 1,
	.frame_interval =1,
	.pixel_line_interval = 2,
	.motion_end_num = 30,
};

static struct option long_options[] =
{
	{"bitrate", HAS_ARG, 0, 'b'}, /* set bitrate ceiling */
	{"motion", HAS_ARG, 0, 'm'}, /* inject motion,  with a value to indicate the motion level */
	{"noise", HAS_ARG, 0, 'n'}, /* noise level , 0: no noise,  2 :high noise */
	{"style", HAS_ARG, 0, 't'}, /* style  */
	{"log", HAS_ARG, 0, 'l'}, /* log level  */
	{"auto", HAS_ARG, 0, 'a'}, /* auto motion&&noise detection*/
	{"test", NO_ARG, 0, 'T'}, /* stress test lbr */
	{"stream0", NO_ARG, 0, 'A'}, /* switch current config context to stream A */
	{"stream1", NO_ARG, 0, 'B'}, /* switch current config context to stream B */
	{"stream2", NO_ARG, 0, 'C'}, /* switch current config context to stream C */
	{"stream3", NO_ARG, 0, 'D'}, /* switch current config context to stream D */
	{0, 0, 0, 0}
};

static const struct hint_s hint[] =
{
	{"64000~n", "\tset bitrate ceiling (per GOP)"},
	{"0~2", "\t0: no motion,\t1: small motion,\t2: big motion"},
	{"0~2", "\t0: no noise,\t1: low noise,\t\t2: high noise"},
	{"0~2", "\t0: full fps auto bitrate,  1: enable fps drop  2: security IPCAM style CBR"},
	{"0~3", "\t\t0: error,  1: msg  2: info  3: debug"},
	{"0~1", "\t\tLBR auto control demo, 1: enable, 0: disable"},
	{"", "\t\tstress test LBR"},
	{"", "\t\tswitch current config context to stream A"},
	{"", "\t\tswitch current config context to stream B"},
	{"", "\t\tswitch current config context to stream C"},
	{"", "\t\tswitch current config context to stream D"},
};

static void usage(void)
{
	u32 i;
	printf("This program is used to test SmartAVC (LBR/low bitrate), options are:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i ++) {
		if (isalpha(long_options[i].val)) {
			printf("-%c ", long_options[i].val);
		} else {
			printf("   ");
		}
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0) {
			printf(" [%s]", hint[i].arg);
		}
		printf("\t%s\n", hint[i].str);
	}
}

static int validate_bitrate_setting(int bitrate)
{
	if ((bitrate < 6400) || (bitrate > 20 * 1024 * 1024)) {
		printf("lbr bitrate ceiling setting is wrong \n");
		return -1;
	} else {
		return 0;
	}
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
	                         &option_index)) != -1) {
		switch (ch) {
			case 'b':
				VERIFY_STREAM_ID(stream_selection);
				lbr_bitrate_ceiling[stream_selection] = atoi(optarg);
				lbr_bitrate_ceiling_flag[stream_selection] = 1;
				//data validation
				if (validate_bitrate_setting(
				            lbr_bitrate_ceiling[stream_selection]) < 0) {
					return -1;
				}
				break;
			case 'm':
				VERIFY_STREAM_ID(stream_selection);
				lbr_motion_level[stream_selection] = atoi(optarg);
				lbr_motion_level_flag[stream_selection] = 1;
				if ((lbr_motion_level[stream_selection] < 0)
				            || (lbr_motion_level[stream_selection] > 2)) {
					printf("invalid motion level \n");
					return -1;
				}
				break;
			case 'n':
				VERIFY_STREAM_ID(stream_selection);
				lbr_noise_level[stream_selection] = atoi(optarg);
				lbr_noise_level_flag[stream_selection] = 1;
				if ((lbr_noise_level[stream_selection] < 0)
				            || (lbr_noise_level[stream_selection] > 2)) {
					printf("invalid noise level \n");
					return -1;
				}
				break;
			case 't':
				VERIFY_STREAM_ID(stream_selection);
				lbr_style[stream_selection] = atoi(optarg);
				lbr_style_flag[stream_selection] = 1;
				if ((lbr_style[stream_selection] < 0)
				            || (lbr_style[stream_selection] > 2)) {
					printf("invalid lbr style \n");
					return -1;
				}
				break;
			case 'l':
				log_level = atoi(optarg);
				break;
			case 'a':
				lbr_auto_run_flag = 1;
				auto_run = atoi(optarg);
				break;
			case 'A':
				stream_selection = 0;
				current_stream_id = 0;
				break;
			case 'B':
				stream_selection = 1;
				current_stream_id = 1;
				break;
			case 'C':
				stream_selection = 2;
				current_stream_id = 2;
				break;
			case 'D':
				stream_selection = 3;
				current_stream_id = 3;
				break;
			case 'T':
				lbr_stress_test_flag = 1;
				break;
			default:
				printf("unknown option found: %c\n", ch);
				return -1;
		}
	}

	return 0;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		return -1;
	}

	//if it is encoding, then stop encoding
	if ((state != IAV_STATE_ENCODING) && (state != IAV_STATE_PREVIEW)) {
		printf("IAV is not in encoding or preview state, cannot control LBR!\n");
		return -1;
	}

	return 0;
}

static int get_lbr_style(int stream_id)
{
	LBR_STYLE style;
	lbr_get_style(&style, stream_id);
	return style;
}

static int get_lbr_motion_level(int stream_id)
{
	LBR_MOTION_LEVEL motion_level;
	lbr_get_motion_level(&motion_level, stream_id);
	return motion_level;
}

static int get_lbr_noise_level(int stream_id)
{
	LBR_NOISE_LEVEL noise_level;
	lbr_get_noise_level(&noise_level, stream_id);
	return noise_level;
}

static int show_menu(void)
{
	printf("\n================================================\n");
	printf("  s -- choose current stream \n");
	printf("  t -- LBR style \n");
	printf("  b -- LBR bitrate ceiling \n");
	printf("  m -- LBR motion level \n");
	printf("  n -- LBR noise level \n");
	printf("  l -- log level \n");
	printf("  a -- auto run \n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static int show_stream_select(void)
{
	int stream_id;
	printf("Current stream_id is %d\n", current_stream_id);
	printf("Choose the stream id you want to work on (0~3):\n");
	scanf("%d", &stream_id);
	if ((stream_id < 0) || (stream_id > 4)) {
		printf("stream id out of range \n");
		return -1;
	}
	current_stream_id = stream_id;
	printf("choose stream id %d \n", stream_id);
	return 0;
}

static int show_style_input(void)
{
	int style;
	printf("Current stream %d's LBR style is %d\n", current_stream_id,
	       get_lbr_style(current_stream_id));
	printf("0: Keep fps and auto control bitrate (Default)\n");
	printf("1: Keep quality and enable fps auto drop (Drop fps) \n");
	printf("2: Keep fps and keep CBR bitrate (CBR)\n");
	printf("Choose the LBR style for current stream (0~3):\n");
	scanf("%d", &style);
	switch (style) {
		case 0:
		case 1:
		case 2:
			lbr_set_style(style, current_stream_id);
			break;
		default:
			printf("invalid lbr style value %d \n", style);
	}

	return 0;
}

static int show_bitrate_menu()
{
	printf("\n================================================\n");
	printf("  a -- auto bitrate target\n");
	printf("  m -- manually set bitrate target \n");
	printf("  r -- return to upper menu");
	printf("\n================================================\n\n");
	printf("> ");

	return 0;
}

static int show_bitrate_input(void)
{
	lbr_bitrate_target_t bitrate_target;
	int bitrate_input;
	int quit_flag;
	int error_opt;
	char ch;

	lbr_get_bitrate_ceiling(&bitrate_target, current_stream_id);
	if (bitrate_target.auto_target) {
		printf("Current stream %d's bitrate control uses Auto target\n",
		       current_stream_id);
	} else {
		printf("Current stream %d's bitrate control uses Manual target for ceiling\n",
		       current_stream_id);
		printf("Current stream %d's bitrate ceiling is %d\n", current_stream_id,
		       bitrate_target.bitrate_ceiling);
	}

	printf("choose bitrate mode:\n");
	//submenu to select bitrate target auto
	show_bitrate_menu();

	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
			case 'a':
				printf("Now set Bitrate target Auto\n");
				bitrate_target.auto_target = 1;
				lbr_set_bitrate_ceiling(&bitrate_target, current_stream_id);
				break;
			case 'm':
				printf("Now set Bitrate target Manual, please input your target:\n");
				scanf("%d", &bitrate_input);
				if (validate_bitrate_setting(bitrate_input) < 0) {
					printf("invalid lbr bitrate value %d \n", bitrate_input);
					return -1;
				} else {
					bitrate_target.auto_target = 0;
					bitrate_target.bitrate_ceiling = bitrate_input;
					lbr_set_bitrate_ceiling(&bitrate_target, current_stream_id);
				}
				break;
			case 'r':
				quit_flag = 1;
				break;
			default:
				error_opt = 1;
				break;
		}

		if (quit_flag) {
			break;
		}

		if (error_opt == 0) {
			show_bitrate_menu();
		}
		ch = getchar();
	}

	return 0;
}

static int show_motion_input(void)
{
	int motion;
	printf("Current stream %d's LBR motion level is %d\n", current_stream_id,
	       get_lbr_motion_level(current_stream_id));
	printf("0: No Motion \n");
	printf("1: Small Motion\n");
	printf("2: Big Motion\n");
	printf("Choose the motion level for current stream (0~2):\n");
	scanf("%d", &motion);
	switch (motion) {
		case 0:
		case 1:
		case 2:
			lbr_set_motion_level(motion, current_stream_id);
			break;
		default:
			printf("invalid lbr motion value %d \n", motion);
			break;
	}

	return 0;
}

static int show_noise_input(void)
{
	int noise;
	printf("Current stream %d's LBR noise level is %d\n", current_stream_id,
	       get_lbr_noise_level(current_stream_id));
	printf("0: No noise \n");
	printf("1: Low noise\n");
	printf("2: High noise\n");
	printf("Choose the noise level for current stream (0~2):\n");
	scanf("%d", &noise);
	switch (noise) {
		case 0:
		case 1:
		case 2:
			lbr_set_noise_level(noise, current_stream_id);
			break;
		default:
			printf("invalid lbr noise value %d \n", noise);
			break;
	}

	return 0;
}

static int show_log_level(void)
{
	int log_level;
	lbr_get_log_level(&log_level);
	printf("Current log level is %d, please input new log level(0~3):\n",
	       log_level);
	scanf("%d", &log_level);
	if (lbr_set_log_level(log_level) < 0) {
		printf("set log level failed!\n");
		return -1;
	} else {
		lbr_get_log_level(&log_level);
		printf("Set log level to be %d\n", log_level);
	}

	return 0;
}

static int get_noise_value(void)
{
	int gain_db;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_AGC_DB, &gain_db) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_AGC_DB");
		return -1;
	}
	return (gain_db >> 24);
}

static void get_my_motion_info(const u8 *md_event)
{
	md_motbuf_evt_t *event = (md_motbuf_evt_t *)md_event;
	/*int i;
	for (i = 0; i < MD_MAX_ROI_N; i ++) {
		if (event[i].motion_flag == MOT_START)
			printf("\tROI#%d Motion Start!\n", i);
		else if (event[i].motion_flag == MOT_END)
			printf("\tROI#%d Motion End!\n", i);
	}*/
	motion_value = event[0].diff_value;
}

static int get_motion_value_loop_stop(void)
{
	int ret = 0;
	ret = md_motbuf_thread_stop();
	md_motbuf_deinit();
	return ret;
}

static int get_motion_value_loop_start(void)
{
	int ret = 0;
	int i;
	int buffer_p, buffer_w, buffer_h;
	md_motbuf_roi_location_t roi_location[MD_MAX_ROI_N];
	do {
		//frame interval = 1. For each frame, pixels in 2x lines are taken into account.
		ret = md_motbuf_init(&init_mdbuf);
		if (ret < 0) {
			printf("Init motion detect based on motion buffer failed.\n");
			md_motbuf_deinit();
			printf("\nDeinit motion detect based on motion buffer......Yes!\n");
			break;
		}

		ret = md_motbuf_get_current_buffer_P_W_H(&buffer_p, &buffer_w, &buffer_h);
		if (ret < 0) {
			printf("Get Motion Buffer pitch, width & height failed.\n");
			md_motbuf_deinit();
			printf("\nDeinit motion detect based on motion buffer......Yes!\n");
			break;
		}

		// Config ROIs
		roi_location[0].x = 0; //ROI0 cover the whole buffer
		roi_location[0].y = 0;
		roi_location[0].width = buffer_w - 1;
		roi_location[0].height = buffer_h - 1;

		roi_location[1].x = 0;
		roi_location[1].y = 0;
		roi_location[1].width = 0;
		roi_location[1].height = 0;

		roi_location[2].x = 0;
		roi_location[2].y = 0;
		roi_location[2].width = 0;
		roi_location[2].height = 0;

		roi_location[3].x = 0;
		roi_location[3].y = 0;
		roi_location[3].width = 0;
		roi_location[3].height = 0;

		// Set ROIs
		for (i = 0; i < MD_MAX_ROI_N; i ++) {
			ret = md_motbuf_set_roi_location(i, &roi_location[i]);
			if (ret < 0) {
				printf("roi%d: md_motbuf_set_roi_location failed!\n", i);
				break;
			}
		}

		if (ret < 0) {
			break;
		}

		// Set threshold
		ret = md_motbuf_set_roi_threshold(0, 10);
		if (ret < 0) {
			printf("roi0: md_motbuf_set_roi_threshold failed!\n");
			break;
		}

		// Set sensitivity
		ret = md_motbuf_set_roi_sensitivity(0, 90);
		if (ret < 0) {
			printf("roi0: md_motbuf_set_roi_sensitivity failed!\n");
			break;
		}

		// Set valid, only use ROI0
		ret = md_motbuf_set_roi_validity(0, 1);
		if (ret < 0) {
			printf("roi0: md_motbuf_set_roi_validity failed!\n");
			break;
		}

		ret = md_motbuf_thread_start();
		if (ret < 0) {
			printf("md_motbuf_thread_start failed!\n");
			break;
		}

		ret = md_motbuf_algo_setup(algo_framediff_mse);
		if (ret < 0) {
			printf("md_motbuf_algo_setup failed!\n");
			break;
		}

		ret = md_motbuf_callback_setup(get_my_motion_info);
		if (ret < 0) {
			printf("md_motbuf_callback_setup failed!\n");
			break;
		}

	} while (0);

	return ret;
}

static void *lbr_control_mainloop(void *arg)
{
	u32 i0_motion = 0, i1_motion = 0, i2_motion = 0;
	u32 i0_light = 0, i1_light = 0, i2_light = 0;
	LBR_MOTION_LEVEL motion_level = LBR_MOTION_LOW;
	LBR_NOISE_LEVEL noise_level = LBR_NOISE_LOW;
	LBR_MOTION_LEVEL real_motion_level[LBR_MAX_STREAM_NUM];
	LBR_NOISE_LEVEL real_noise_level[LBR_MAX_STREAM_NUM];
	while (lbr_control_exit_flag == 0) {
		usleep(20000);
		noise_value = get_noise_value();
		/*printf("motion_value = %d, noise_value = %d\n", motion_value,
		       noise_value);*/
		if (motion_value < motion_low_threshold) {
			i0_motion ++;
			i1_motion = 0;
			i2_motion = 0;
			if (i0_motion >= LBR_MOTION_NONE_DELAY) {
				if (motion_level == LBR_MOTION_HIGH) {
					motion_level = LBR_MOTION_LOW;
					i0_motion = 0;
				} else {
					motion_level = LBR_MOTION_NONE;
					i0_motion = 0;
				}
			}
		} else if (motion_value < motion_high_threshold) {
			i0_motion = 0;
			i1_motion ++;
			i2_motion = 0;
			if (i1_motion >= LBR_MOTION_LOW_DELAY) {
				motion_level = LBR_MOTION_LOW;
				i1_motion = 0;
			}
		} else {
			i0_motion = 0;
			i1_motion = 0;
			i2_motion ++;
			if (i2_motion >= LBR_MOTION_HIGH_DELAY) {
				motion_level = LBR_MOTION_HIGH;
				i2_motion = 0;
			}
		}

		if (noise_value > noise_high_threshold) {
			i0_light ++;
			i1_light = 0;
			i2_light = 0;
			if (i0_light >= LBR_NOISE_HIGH_DELAY) {
				noise_level = LBR_NOISE_HIGH;
				i0_light = 0;
			}
		} else if (noise_value > noise_low_threshold) {
			i0_light = 0;
			i1_light ++;
			i2_light = 0;
			if (i1_light >= LBR_NOISE_LOW_DELAY) {
				noise_level = LBR_NOISE_LOW;
				i1_light = 0;
			}
		} else {
			i0_light = 0;
			i1_light = 0;
			i2_light ++;
			if (i2_light >= LBR_NOISE_NONE_DELAY) {
				noise_level = LBR_NOISE_NONE;
				i2_light = 0;
			}
		}
		/*printf("current motion level = %d, noise level = %d\n", motion_level,
		       noise_level);*/
		lbr_get_motion_level(&real_motion_level[current_stream_id],
		                     current_stream_id);
		lbr_get_noise_level(&real_noise_level[current_stream_id],
		                    current_stream_id);
		if (motion_level != real_motion_level[current_stream_id]) {
			lbr_set_motion_level(motion_level, current_stream_id);
		}
		if (noise_level != real_noise_level[current_stream_id]) {
			lbr_set_noise_level(noise_level, current_stream_id);
		}
	}
	return NULL ;
}

int lbr_control_thread_start(void)
{
	int ret = 0;
	lbr_control_exit_flag = 0;
	if (lbr_control_thread_id == 0) {
		ret = pthread_create(&lbr_control_thread_id, NULL, lbr_control_mainloop,
		                     NULL );
		if (ret != 0) {
			perror("lbr_control pthread create failed.");
		}
	}
	return ret;
}

int lbr_control_thread_stop(void)
{
	int ret;
	if (lbr_control_thread_id == 0) {
		ret = 0;
	} else {
		lbr_control_exit_flag = 1;
		ret = pthread_join(lbr_control_thread_id, NULL );
		lbr_control_thread_id = 0;
	}
	return ret;
}

static int show_auto_run_menu(void)
{
	printf("\n================================================\n");
	printf("  a -- start auto run(have already set bitrate ceiling and style correctly) \n");
	printf("  s -- stop auto run \n");
	printf("  c -- config motion/noise threshold \n");
	printf("  r -- return to upper menu");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static void show_auto_run_input(void)
{
	char ch;
	int quit_flag;
	int error_opt;
	show_auto_run_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
			case 'a':
				printf("start auto run!\n");
				if (!auto_run) {
					get_motion_value_loop_start();
					lbr_control_thread_start();
					auto_run = 1;
				} else {
					printf("Already started auto run!\n");
				}
				break;
			case 's':
				if (auto_run) {
					printf("stop auto run!\n");
					lbr_control_thread_stop();
					get_motion_value_loop_stop();
					auto_run = 0;
				} else {
					printf("Already stopped auto run!\n");
				}
				break;
			case 'c':
				printf("current motion_low_threshold = %d, input new\n>",
				       motion_low_threshold);
				scanf("%d", &motion_low_threshold);
				printf("current motion_high_threshold = %d, input new\n>",
				       motion_high_threshold);
				scanf("%d", &motion_high_threshold);
				printf("current noise_low_threshold = %d, input new\n>",
				       noise_low_threshold);
				scanf("%d", &noise_low_threshold);
				printf("current noise_high_threshold = %d, input new\n>",
				       noise_high_threshold);
				scanf("%d", &noise_high_threshold);
				break;
			case 'r':
				quit_flag = 1;
				break;
			default:
				error_opt = 1;
				break;
		}
		if (quit_flag) {
			break;
		}
		if (error_opt == 0) {
			show_auto_run_menu();
		}
		ch = getchar();
	}
}

static void sigstop()
{
	lbr_control_thread_stop();
	get_motion_value_loop_stop();
	lbr_set_motion_level(LBR_MOTION_HIGH, current_stream_id);
	lbr_set_motion_level(LBR_NOISE_NONE, current_stream_id);
	lbr_close();
	close(fd_iav);
	exit(EXIT_SUCCESS);
}

/* test_lbr supports arg to do initial config, and it uses menu for dynamic config */
int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGTERM, sigstop);
	signal(SIGQUIT, sigstop);
	char ch, error_opt;
	int quit_flag;
	lbr_init_t lbr;
	int i;
	int j;
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (lbr_set_log_level(log_level) < 0) {
		printf("Set log level failed!\n");
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	//check iav state
	if (check_state() < 0) {
		return -1;
	}

	//init lbr
	if (lbr_open() < 0) {
		printf("lbr open failed \n");
		return -1;
	}
	memset(&lbr, 0, sizeof(lbr));
	lbr.fd_iav = fd_iav;
	if (lbr_init(&lbr) < 0) {
		printf("lbr init failed \n");
		return -1;
	}

	//apply initial cmdline settings here

	for (i = 0; i < LBR_MAX_STREAM_NUM; i ++) {
		if (lbr_bitrate_ceiling_flag[i]) {
			lbr_bitrate_target_t target;
			target.auto_target = 0;
			target.bitrate_ceiling = lbr_bitrate_ceiling[i];
			if (lbr_set_bitrate_ceiling(&target, i) < 0) {
				printf("lbr set bitrate ceiling %d for stream %d failed\n",
				       lbr_bitrate_ceiling[i], i);
			}
		}

		if (lbr_style_flag[i]) {
			if (lbr_set_style(lbr_style[i], i) < 0) {
				printf("lbr set style %d for stream %d failed\n", lbr_style[i],
				       i);
			}
		}

		if (lbr_motion_level_flag[i]) {
			if (lbr_set_motion_level(lbr_motion_level[i], i) < 0) {
				printf("lbr set motion level %d for stream %d failed\n",
				       lbr_motion_level[i], i);
			}
		}

		if (lbr_noise_level_flag[i]) {
			if (lbr_set_noise_level(lbr_noise_level[i], i) < 0) {
				printf("lbr set noise level %d for stream %d failed\n",
				       lbr_noise_level[i], i);
			}
		}
	}

	//stress test, it just runs and does not exit
	if (lbr_stress_test_flag) {
		printf("Now doing lbr stress test \n");
		while (1) {
			printf("set motion from 0 to 2 for all streams\n");
			for (i = 0; i < LBR_MAX_STREAM_NUM; i ++) {
				for (j = 0; j < 3; j ++) {
					lbr_set_motion_level(j, i);
				}
			}
		}
		return 0;
	}

	if (lbr_auto_run_flag) {
		if (auto_run) {
			printf("start LBR auto control, do not manually set noise/motion level!\n");
			if (get_motion_value_loop_start() < 0) {
				printf("get_motion_value_loop_start() failed!\n");
			}
			if (lbr_control_thread_start() < 0) {
				printf("lbr_control_thread_start() failed!\n");
			}
		}
	}

	//interactive menu selection

	show_menu();

	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
			case 's':
				show_stream_select();
				break;
			case 't':
				show_style_input();
				break;
			case 'b':
				show_bitrate_input();
				break;
			case 'm':
				show_motion_input();
				break;
			case 'n':
				show_noise_input();
				break;
			case 'l':
				show_log_level();
				break;
			case 'a':
				show_auto_run_input();
				break;
			case 'q':
				quit_flag = 1;
				break;
			default:
				error_opt = 1;
				break;
		}
		if (quit_flag) {
			break;
		}
		if (error_opt == 0) {
			show_menu();
		}
		ch = getchar();
	}
	lbr_control_thread_stop();
	get_motion_value_loop_stop();
	lbr_close();
	close(fd_iav);

	return 0;
}
