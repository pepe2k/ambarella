/***********************************************************
 * test_image.c
 *
 * History:
 *	2010/03/25 - [Jian Tang] created file
 *
 * Copyright (C) 2008-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ***********************************************************/


#include <sys/signal.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <sys/shm.h>

#include "fb_image.h"

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"

#include "img_abs_filter.h"

#include "img_api_adv_arch.h"

#include "mw_struct_hiso.h"
#include "mw_api_hiso.h"

#define	DIV_ROUND(divident, divider)    (((divident)+((divider)>>1)) / (divider))

#define MCTF_INFO_STRUCT_NUM	40

int fd_iav;
int fd_vin = -1;

typedef enum {
	BACKGROUND_MODE = 0,
	INTERACTIVE_MODE,
	TEST_3A_LIB_MODE,
	WORK_MODE_NUM,
	WORK_MODE_FIRST = BACKGROUND_MODE,
	WORK_MODE_LAST = WORK_MODE_NUM,
} mw_work_mode;

int work_mode = INTERACTIVE_MODE;

int G_aaa_mode = MW_AAA_MODE_HISO;

/***************************************************************************************
	function:	int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
	input:	input_str: input string line for analysis
			argarr: buffer array accommodating the parsed arguments
			argcnt: number of the arguments parsed
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
/*
static int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
{
	int i = 0;
	char *delim = " ,:-\r\n\t";
	char *ptr;
	*argcnt = 0;

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = atoi(ptr);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = atoi(ptr);
	}

	*argcnt = i;
	return 0;
}

*/

/* ***************************************************************
	Exposure params
******************************************************************/
mw_ae_param ae_param = {
	.ae_enable = 1,
	.ae_exp_level = 100,
	.anti_flicker_mode = MW_ANTI_FLICKER_60HZ,
	.shutter_time_min = SHUTTER_1BY32000_SEC,
	.shutter_time_max = SHUTTER_1BY60_SEC,
	.sensor_gain_max = ISO_6400,
	.slow_shutter_enable = 0,
	.current_vin_fps = AMBA_VIDEO_FPS_60,
};

//
static int show_global_setting_menu(void)
{
	printf("\n================ Global Settings ================\n");
	printf("  s -- 3A library start and stop\n");
//	printf("  f -- Change Sensor fps\n");
//	printf("  z -- Freeze Video\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("G > ");
	return 0;
}

static int global_setting(int imgproc_running)
{
	int i, exit_flag, error_opt;
	char ch;

	show_global_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 's':
			printf("Imgproc library is %srunning.\n",
				imgproc_running ? "" : "not ");
			printf("0 - Stop 3A library, 1 - Start 3A library\n> ");
			scanf("%d", &i);
			if (i == 0) {
				if (imgproc_running == 1) {
					mw_stop_aaa();
					imgproc_running = 0;
				}
			} else if (i == 1) {
				if (imgproc_running == 0) {
					mw_start_aaa();
					imgproc_running = 1;
				}
			} else {
				printf("Invalid input for this option!\n");
			}
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_global_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_exposure_setting_menu(void)
{
	printf("\n================ Exposure Control Settings ================\n");
	printf("  E -- AE enable and disable\n");
	printf("  e -- Set exposure level\n");
	printf("  G -- Manually set sensor gain\n");
	printf("  s -- Manually set shutter time\n");
	printf("  a -- Anti-flicker mode\n");
	printf("  T -- Shutter time max\n");
	printf("  t -- Shutter time min\n");
	printf("  R -- Slow shutter enable and disable\n");
	printf("  g -- Sensor gain max\n");
	printf("  l -- Get AE current lines\n");
	printf("  r -- Get current shutter time and sensor gain\n");
	printf("  y -- Get Luma value\n");

/*	printf("  d -- DC iris PID coefficients\n");
	printf("  f -- Lens type option(Manul, DC or P iris) \n");
	printf("  D -- DC or P iris enable and disable\n");
	printf("  m -- Set AE metering mode\n");
	printf("  C -- Setting ae area following pre-main input buffer enable and disable\n");
*/
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("E > ");
	return 0;
}

static int exposure_setting(void)
{

	int i, exit_flag, error_opt;
	char ch;
	int value[1];
	mw_ae_line lines[10];

	show_exposure_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'E':
			printf("0 - AE disable  1 - AE enable\n> ");
			scanf("%d", &i);
			mw_enable_ae(i);
			break;
		case 'e':
			mw_get_exposure_level(value);
			printf("Current exposure level: \n");
			printf(" Exposure frame Index[%d] = %d \n", i, value[0]);
			printf("Input new exposure level: (range 25 ~ 400)\n> ");
			scanf("%d", value);

			mw_set_exposure_level(value);
			break;
		case 'a':
			printf("Anti-flicker mode? 0 - 50Hz  1 - 60Hz\n> ");
			scanf("%d", &i);
			mw_get_ae_param(&ae_param);
			ae_param.anti_flicker_mode = i;
			mw_set_ae_param(&ae_param);
			break;
		case 's':
			mw_get_shutter_time(value);
			printf("Current shutter time is 1/%d sec.\n",
				DIV_ROUND(512000000, value[0]));
			printf("Input new shutter time in 1/n sec format: (range 1 ~ 8000)\n> ");
			scanf("%d", &value[0]);
			value[0] = DIV_ROUND(512000000, value[0]);
			mw_set_shutter_time(value);
			break;
		case 'T':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time max is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_max));
			printf("Input new shutter time max in 1/n sec fomat? (Ex, 6, 10, 15...)(slow shutter)\n> ");
			scanf("%d", &i);
			ae_param.shutter_time_max = DIV_ROUND(512000000, i);
			mw_set_ae_param(&ae_param);
			break;
		case 't':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time min is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_min));
			printf("Input new shutter time min in 1/n sec fomat? (Ex, 120, 200, 400 ...)\n> ");
			scanf("%d", &i);
			ae_param.shutter_time_min = DIV_ROUND(512000000, i);
			mw_set_ae_param(&ae_param);
			break;
		case 'R':
			mw_get_ae_param(&ae_param);
			printf("Current slow shutter mode is %s.\n",
				ae_param.slow_shutter_enable ? "enabled" : "disabled");
			printf("Input slow shutter mode? (0 - disable, 1 - enable)\n> ");
			scanf("%d", &i);
			ae_param.slow_shutter_enable = !!i;
			mw_set_ae_param(&ae_param);
			break;
		case 'g':
			mw_get_ae_param(&ae_param);
			printf("Current sensor gain max is %d dB\n",
				ae_param.sensor_gain_max);
			printf("Input new sensor gain max in dB? (Ex, 24, 36, 48 ...)\n> ");
			scanf("%d", &i);
			ae_param.sensor_gain_max = i;
			mw_set_ae_param(&ae_param);
			break;
		case 'G':
			if (mw_get_sensor_gain(value) < 0) {
				printf("mw_get_sensor_gain error\n");
				break;
			}
			printf("Current sensor gain is %d dB\n",value[0]);
			printf("Input new sensor gain in dB: (range 0 ~ 60)\n> ");
			scanf("%d", &value[0]);
			if  (mw_set_sensor_gain(value) < 0) {
				printf("mw_set_sensor_gain error\n");
			}

			break;
		case 'l':
			printf("Get current AE lines:\n");
			mw_get_ae_lines(lines, 10);
			break;
		case 'y':
			mw_get_ae_luma_value(&i);
			printf("Current luma value is %d.\n", i);
			break;
		case 'r':
			mw_get_shutter_time(value);
			printf("Current shutter time is 1/%d sec.\n",
				DIV_ROUND(512000000, value[0]));
			mw_get_sensor_gain(value);
			printf("Current sensor gain is %d dB.\n", value[0]);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_exposure_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_white_balance_menu(void)
{
	printf("\n================ White Balance Settings ================\n");
	printf("  W -- AWB enable and disable\n");
	printf("  m -- Select AWB mode(Need on AWB method: Normal method)\n");
	printf("  M -- Select AWB method\n");
	printf("  c -- Set RGB gain for custom mode (Don't need to turn off AWB)\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("W > ");
	return 0;
}

static int white_balance_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	wb_gain_t wb_gain;
	awb_work_method_t wb_method;
	awb_control_mode_t wb_mode;

	show_white_balance_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'W':
			printf("0 - AWB disable  1 - AWB enable\n> ");
			scanf("%d", &i);
			mw_enable_awb(i);
			break;
		case 'm':
			if (mw_get_awb_mode(&wb_mode) < 0) {
				printf("mw_get_white_balance_mode error\n");
				break;
			}
			printf("Current AWB mode is:%d \n", wb_mode);
			printf("Choose AWB mode (%d~%d):\n",
				WB_AUTOMATIC, WB_MODE_NUMBER -1);
			printf("0 - auto 1 - 2800K 2 - 4000K 3 - 5000K 4 - 6500K 5 - 7500K");
			printf(" ... 10 - custom\n> ");
			scanf("%d", &i);
			mw_set_awb_mode(i);
			break;
		case 'M':
			mw_get_awb_method(&wb_method);
			printf("Current AWB method is [%d].\n", wb_method);
			printf("Choose AWB method (0 - Normal, 1 - Custom, 2 - Grey world)\n> ");
			scanf("%d", &i);
			mw_set_awb_method(i);
			break;
		case 'c':
			getchar();
			printf("Enter custom method\n");
			mw_set_awb_method(AWB_MANUAL);
			printf("Must put a gray card in the front of the lens at first\n");
			printf("Then press any button\n");
			getchar();
			mw_get_wb_cal(&wb_gain);
			printf("Current WB gain is %d, %d, %d\n", wb_gain.r_gain,
				wb_gain.g_gain, wb_gain.b_gain);
			printf("Input new RGB gain for normal method - custom mode \n");
			printf("(Ex, 1500,1024,1400) \n > ");
			scanf("%d,%d,%d", &wb_gain.r_gain, &wb_gain.g_gain, &wb_gain.b_gain);

			printf("Enter normal method:custom mode \n");
			mw_set_awb_method(AWB_NORMAL);
			mw_set_awb_mode(WB_CUSTOM);
			mw_set_custom_gain(&wb_gain);
			printf("The new RGB gain you set for custom mode is : %d, %d, %d\n",
				wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_white_balance_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_adjustment_menu(void)
{
	printf("\n================ Image Adjustment Settings ================\n");
	printf("  a -- Saturation adjustment\n");
	printf("  b -- Brightness adjustment\n");
	printf("  c -- Contrast adjustment\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("A > ");
	return 0;
}

static int adjustment_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;

	show_adjustment_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'a':
			mw_get_saturation(&i);
			printf("Current saturation is %d\n", i);
			printf("Input new saturation: (range 0 ~ 255, unit 64)\n> ");
			scanf("%d", &i);
			mw_set_saturation(i);
			break;
		case 'b':
			mw_get_brightness(&i);
			printf("Current brightness is %d\n", i);
			printf("Input new brightness: (range -255 ~ 255)\n> ");
			scanf("%d", &i);
			mw_set_brightness(i);
			break;
		case 'c':
			mw_get_contrast(&i);
			printf("Current contrast is %d\n", i);
			printf("Input new contrast: (range 0 ~ 128, unit 64)\n> ");
			scanf("%d", &i);
			mw_set_contrast(i);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_adjustment_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_enhancement_menu(void)
{
	printf("\n================ Image Enhancement Settings ================\n");
/*  Need update the feature.
	printf("  m -- Set MCTF 3D noise filter strength\n");
	printf("  s -- Set sharpening strength\n");
*/
	printf("  d -- Enable day and night mode\n");
	printf("  j -- ADJ enable and disable\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("H > ");
	return 0;
}

static int enhancement_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	int value;

	show_enhancement_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'm':
			mw_get_mctf_strength(&value);
			printf("Current mctf strength is %d\n", value);
			printf("Input new mctf strength: (range 0 ~ 256, unit 64)\n> ");
			scanf("%d", &value);
			mw_set_mctf_strength(value);
			break;
		case 'd':
			mw_get_night_mode(&value);
			printf("Current Day and night mode is %s\n", value ? "disabled" : "enabled");
			printf("Day and night mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_set_night_mode(i);
			break;
		case 'j':
			printf("Change ADJ mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_adj(i);
			break;
		case 's':
			mw_get_sharpening_strength(&value);
			printf("Current mctf strength is %d\n", value);
			printf("Input new mctf strength: (range 0 ~ 128, unit 64)\n> ");
			scanf("%d", &value);
			mw_set_sharpening_strength(value);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_enhancement_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_misc_menu(void)
{
	printf("\n================ Misc Settings ================\n");
	printf("  v -- Set log level\n");
	printf("  m -- get mw lib version\n");
	printf("  V -- get aaa lib version\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("M > ");
	return 0;
}

static int display_aaa_lib_version(void)
{
	mw_aaa_lib_version algo_lib_version;
	mw_aaa_lib_version dsp_lib_version;
	if (mw_get_lib_version(&algo_lib_version, &dsp_lib_version) < 0) {
		perror("display_aaa_lib_version");
		return -1;
	}
	printf("\n[Algo lib info]:\n");
	printf("   Algo lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		algo_lib_version.description, algo_lib_version.major,
		algo_lib_version.minor, algo_lib_version.patch,
		algo_lib_version.mod_time);
	printf("\n[DSP lib info]:\n");
	printf("   DSP lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		dsp_lib_version.description, dsp_lib_version.major,
		dsp_lib_version.minor, dsp_lib_version.patch,
		dsp_lib_version.mod_time);
	return 0;
}

static int display_mw_lib_version(void)
{
	mw_version_info mw_version;
	if (mw_get_version_info(&mw_version) < 0) {
		perror("mw_get_version_info error");
		return -1;
	}
	printf("\n[MW lib info]:\n");
	printf("   MW lib major : 0x%8x, minor: 0x%8x, patch: 0x%8x (Last updated: %x)\n",
		mw_version.major, mw_version.minor, mw_version.patch, mw_version.update_time);

	return 0;
}

static int misc_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;

	show_misc_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'v':
			printf("Input log level: (0~2)\n> ");
			scanf("%d", &i);
			mw_set_log_level(i);
			break;
		case 'V':
			if (display_aaa_lib_version() < 0) {
				perror("display_aaa_lib_version");
			}
			break;
		case 'm':
			if (display_mw_lib_version() < 0) {
				perror("display_mw_lib_version");
			}
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_misc_menu();
		}
		ch = getchar();
	}
	return 0;
}


static int show_menu(void)
{
	printf("\n================================================\n");
	printf("  g -- Global settings (3A library control, sensor fps, read 3A info)\n");
	printf("  e -- Exposure control settings (AE lines, shutter, gain, DC-iris,P-iris, metering)\n");
	printf("  w -- White balance settings (AWB control, WB mode, RGB gain)\n");
	printf("  a -- Adjustment settings (Saturation, brightness, contrast, ...)\n");
//	printf("  s -- Get statistics data (AE, AWB, AF, Histogram)\n");
	printf("  h -- Enhancement settings (3D de-noise, day night mode)\n");
	printf("  m -- Misc settings (Log level)\n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static void sigstop(int signo)
{
	mw_stop_aaa();
	mw_deinit_aaa();
	close(fd_iav);
	exit(1);
}

#define NO_ARG	0
#define HAS_ARG	1


static const char* short_options = "i:m:";
static struct option long_options[] = {
	{"work_mode", HAS_ARG, 0, 'i'},
	{"mode", HAS_ARG, 0, 'm'},

	{0, 0, 0, 0},
};
struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1|2", "0:background mode, 1:interactive mode, 2: auto test start/stop aaa mode"},
	{"0|1", "config aaa mode (0: LISO; 1: HISO)"},

	{0, 0},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int i = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'i':
			work_mode = atoi(optarg);
			if ((work_mode < WORK_MODE_FIRST)
				|| (work_mode >= WORK_MODE_LAST)) {
				printf("work mode must be in [%d~%d] \n",
					WORK_MODE_FIRST, WORK_MODE_LAST);
				return -1;
			}
			break;
		case 'm':
			i = atoi(optarg);
			if ((i < MW_AAA_MODE_FIRST) && (i > MW_AAA_MODE_LAST)) {
				printf("No support\n");
				return -1;
			}
			G_aaa_mode = i;
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

void usage(void){
	int i;

	printf("test_image usage:\n");
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

	printf("Example:\n");
	printf("0. run without interactive menu\n");
	printf("\ttest_image_hiso -i 0\n");
	printf("1. run with interactive menu\n");
	printf("\ttest_image_hiso -i 1\n");
	printf("2. run with test mode: auto test start/stop 3A\n");
	printf("\ttest_image -i 2\n");
	printf("3. run with interactive menu with hiso mode\n");
	printf("\ttest_image_hiso -i 1 -m 1 or \n");
	printf("\ttest_image_hiso -i 1 (default m = 1)\n");
	printf("\n");
	printf("Notice:\n");
	printf("\ttest_image_hiso just support enc-mode 9,\n"
		 "\t Must run before enter enc-mode 9 preview when DSP is IDLE state \n");
	printf("Steps:\n");
	printf("\t 1. # test_encode --idle --nopreview\n");
	printf("\t 2. # test_image_hiso -i 1\n");
	printf("\t 3. # test_encode -i0 -V480p --hdmi --enc-mode 9\n");

}

int run_interactive_mode()
{
	char ch, error_opt;
	int quit_flag, imgproc_running_flag = 1;
	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'g':
			global_setting(imgproc_running_flag);
			break;
		case 'e':
			exposure_setting();
			break;
		case 'w':
			white_balance_setting();
			break;
		case 'a':
			adjustment_setting();
			break;
		case 'h':
			enhancement_setting();
			break;
		case 'm':
			misc_setting();
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
			show_menu();
		}
		ch = getchar();
	}
	if (mw_stop_aaa() < 0) {
		perror("mw_stop_aaa");
		return -1;
	}
	if (mw_deinit_aaa() < 0) {
		perror("exit test_image_hiso");
		return -1;
	}
	return 0;
}

int start_aaa_main()
{
	if (mw_init_aaa(fd_iav, G_aaa_mode) < 0) {
		perror("mw_init_aaa");
		return -1;
	}
	if (mw_start_aaa() < 0) {
		perror("mw_start_aaa");
		return -1;
	}

	return 0;
}

int main(int argc, char ** argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (start_aaa_main() < 0 ) {
		perror("start_aaa_main");
		return -1;
	}

	switch (work_mode) {
		case BACKGROUND_MODE:
			while (1) {
				sleep(2);
			}
			break;
		case INTERACTIVE_MODE:
			if (run_interactive_mode() < 0) {
				printf("run_interactive_mode error\n");
				return -1;
			}
			close(fd_iav);
			break;
		case TEST_3A_LIB_MODE:
			while (1) {
				sleep(3);
				mw_stop_aaa();
				sleep(3);
				mw_start_aaa();
			}
			break;
		default:
			printf("Unknown option!\n");
			break;
	}
	return 0;
}

