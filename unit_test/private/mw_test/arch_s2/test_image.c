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

#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"

#include "mw_struct.h"
#include "mw_api.h"
#include "img_dsp_interface_arch.h"

#define	DIV_ROUND(divident, divider)    (((divident)+((divider)>>1)) / (divider))

typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_HIST,
	STATIS_AF_HIST,
} STATISTICS_TYPE;

typedef enum {
	BACKGROUND_MODE = 0,
	INTERACTIVE_MODE,
	TEST_3A_LIB_MODE,
	WORK_MODE_NUM,
	WORK_MODE_FIRST = BACKGROUND_MODE,
	WORK_MODE_LAST = WORK_MODE_NUM,
} mw_work_mode;

int fd_iav;
int fd_vin = -1;
mw_local_exposure_curve local_exposure_curve;
int work_mode = INTERACTIVE_MODE;

int G_expo_num = 0;
mw_sensor_param G_hdr_param = {0};
int G_log_level = 0;

#define SENSOR_STEPS_PER_DB	6

/****************************************************/
/*	AE Related Settings									*/
/****************************************************/
mw_ae_param			ae_param = {
	.anti_flicker_mode		= MW_ANTI_FLICKER_60HZ,
	.shutter_time_min		= SHUTTER_1BY8000_SEC,
	.shutter_time_max		= SHUTTER_1BY30_SEC,
	.sensor_gain_max		= ISO_6400,
	.slow_shutter_enable	= 0,
};
mw_ae_metering_mode metering_mode	= MW_AE_CENTER_METERING;
u32	dc_iris_enable		= 0;
mw_dc_iris_pid_coef pid_coef= {1500, 1, 1000};
int	load_mctf_flag = 0;

int	load_file_flag[FILE_TYPE_TOTAL_NUM] = {0};
char	G_file_name[FILE_TYPE_TOTAL_NUM][FILE_NAME_LENGTH] = {""};

#define MCTF_INFO_STRUCT_NUM	40

char	G_mctf_file[FILE_NAME_LENGTH]="";

int G_lens_id = LENS_CMOUNT_ID;

static mw_ae_line ae_60hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_50hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY50_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY50_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY25_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY25_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_customer_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_150, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_150, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_150, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_150, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_102400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static line_t hdr_ae_line[] = {
	{{{800, 0, 0}}, {{800, 120,0}}},

	{{{800, 120, 0}}, {{800, 480,0}}}
};

/****************************************************/
/*	AWB Related Settings								*/
/****************************************************/
u32	white_balance_mode	= MW_WB_AUTO;

/****************************************************/
/*	Image Adjustment Settings							*/
/****************************************************/
u32	saturation			= 64;
u32	brightness			= 0;
u32	contrast				= 64;
u32	sharpness			= 6;

/****************************************************/
/*	Image Enhancement Settings							*/
/****************************************************/
u32	mctf_strength			= 1;
u32	auto_local_exposure_mode	= 0;
u32	backlight_comp_enable		= 0;
u32	day_night_mode_enable	= 0;


/***************************************************************************************
	function:	int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
	input:	input_str: input string line for analysis
			argarr: buffer array accommodating the parsed arguments
			argcnt: number of the arguments parsed
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
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


#define DEFAULT_KEY 					5678 //don't modify it
#define SHM_DATA_SZ 					sizeof(img_aaa_stat_t)  //don't modify it

static int shmid = 0;
static char *shm = NULL;

static int init_shm_get(void)
{
	/* Locate the segment.*/
	if ((shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666)) < 0) {
		perror("3A is off!\n");
		perror("shmget");
		exit(1);
	}

	return 0;
}

static int get_shm_data(void)
{
	if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*) - 1) {
		perror("shmat");
		exit(1);
	}
	return 0;
}


/***************************************************************************************
	function:	int load_local_exposure_curve(char *lect_filename, u16 *local_exposure_curve)
	input:	lect_filename: filename of local exposure curve table to be loaded
			local_exposure_curve: buffer structure accommodating the loaded local exposure curve
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int load_local_exposure_curve(char *lect_filename, mw_local_exposure_curve *local_exposure_curve)
{
	char key[64] = "local_exposure_curve";
	char line[1024];
	FILE *fp;
	int find_key = 0;
	u16 *param = &local_exposure_curve->gain_curve_table[0];
	int argcnt;

	if ((fp = fopen(lect_filename, "r")) == NULL) {
		printf("Open local exposure curve file [%s] failed!\n", lect_filename);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strstr(line, key) != NULL) {
			find_key = 1;
			break;
		}
	}

	if (find_key) {
		while ( fgets(line, sizeof(line), fp) != NULL) {
			get_multi_arg(line, param, &argcnt);
//			printf("argcnt %d\n", argcnt);
			param += argcnt;
			if (argcnt == 0)
				break;
		}
	}

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if (!find_key)
		return -1;

	return 0;
}

static int set_sensor_fps(u32 fps)
{
	static int init_flag = 0;
	static char vin_arr[8];
	u32 frame_time = AMBA_VIDEO_FPS_29_97;
	switch (fps) {
	case 0:
		frame_time = AMBA_VIDEO_FPS_29_97;
		break;
	case 1:
		frame_time = AMBA_VIDEO_FPS_30;
		break;
	case 2:
		frame_time = AMBA_VIDEO_FPS_15;
		break;
	case 3:
		frame_time = AMBA_VIDEO_FPS_7_5;
		break;
	default:
		frame_time = AMBA_VIDEO_FPS_29_97;
		break;
	}
	if (init_flag == 0) {
		fd_vin =open("/proc/ambarella/vin0_vsync", O_RDWR);
		if (fd_vin < 0) {
			printf("CANNOT OPEN VIN FILE!\n");
			return -1;
		}
		init_flag = 1;
	}
	read(fd_vin, vin_arr, 8);
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_FRAME_RATE, frame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
		return -1;
	}
	printf("[Done] set sensor fps [%d] [%d].\n", fps, frame_time);
	return 0;
}

static int update_sensor_fps(void)
{
	if (ioctl(fd_iav, IAV_IOC_UPDATE_VIN_FRAMERATE_EX, 0) < 0) {
		perror("IAV_IOC_UPDATE_VIN_FRAMERATE_EX");
		return -1;
	}
	printf("[Done] update_vin_frame_rate !\n");
	return 0;
}

static int show_global_setting_menu(void)
{
	printf("\n================ Global Settings ================\n");
	printf("  s -- 3A library start and stop\n");
//	printf("  f -- Change Sensor fps\n");
	printf("  z -- Freeze Video\n");
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
					mw_start_aaa(fd_iav);
					imgproc_running = 1;
				}
			} else {
				printf("Invalid input for this option!\n");
			}
			break;
		case 'f':
			printf("Set sensor frame rate : 0 - 29.97, 1 - 30, 2 - 15, 3 - 7.5\n");
			scanf("%d", &i);
			set_sensor_fps(i);
			update_sensor_fps();
			break;
		case 'z':
			printf("Set video freeze : 0 - resume, 1 - freeze\n");
			scanf("%d", &i);
			mw_set_video_freeze(i);
			printf("Video freeze is %s.\n", i ? "enabled" : "disabled");
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
	printf("  r -- Get current shutter time and sensor gain\n");
	printf("  G -- Manually set sensor gain\n");
	printf("  s -- Manually set shutter time\n");
	printf("  d -- DC iris PID coefficients\n");
	printf("  f -- Lens type option(Manul, DC or P iris) \n");
	printf("  D -- DC or P iris enable and disable\n");
	printf("  S -- P iris set exit step\n");
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  a -- Anti-flicker mode\n");
		printf("  T -- Shutter time max\n");
		printf("  t -- Shutter time min\n");
		printf("  R -- Slow shutter enable and disable\n");
		printf("  g -- Sensor gain max\n");
		printf("  m -- Set AE metering mode\n");
		printf("  l -- Get AE current lines\n");
		printf("  L -- Set AE customer lines\n");
		printf("  p -- Set AE switch point\n");
		printf("  C -- Setting ae area following pre-main input buffer enable and disable\n");
		printf("  y -- Get Luma value\n");
	}
		printf("  P -- Piris lens aperture range\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("E > ");
	return 0;
}

static int exposure_setting(void)
{

	int i, exit_flag, error_opt,iris_flag;
	char ch;
	mw_dc_iris_pid_coef pid;
	mw_ae_metering_table	custom_ae_metering_table[2] = {
		{	//Left half window as ROI
			{1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,}
		},
		{	//Right half window as ROI
			{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,}
		},
	};
	mw_ae_line lines[10];
	mw_ae_point switch_point;
	int value[MAX_HDR_EXPOSURE_NUM];
	float lens_aperture[2];

	show_exposure_setting_menu();
	ch = getchar();
	while (ch) {
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
			for (i = 0; i < G_hdr_param.hdr_expo_num; i++) {
				printf(" Exposure frame Index[%d] = %d \n", i, value[i]);
			}
			if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Input new exposure level: (range 25 ~ 400)\n> ");
				scanf("%d", value);
			} else {
				printf("Select the exposure frame index[0~%d]:\n> ",
					G_hdr_param.hdr_expo_num -1);
				scanf("%d", &i);
				if (i >= G_hdr_param.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_hdr_param.hdr_expo_num -1);
					break;
				}
				printf("Input new exposure level for exposure frame index %d (25~400)\n> ", i);
				scanf("%d", &value[i]);
			}
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
			if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				mw_get_shutter_time(fd_iav, value);
				printf("Current shutter time is 1/%d sec.\n",
					DIV_ROUND(512000000, value[0]));
				printf("Input new shutter time in 1/n sec format: (range 1 ~ 8000)\n> ");
				scanf("%d", &value[0]);
				value[0] = DIV_ROUND(512000000, value[0]);
				mw_set_shutter_time(fd_iav, value);
			} else {
				printf("Current %s hdr shutter mode:\n",
					G_hdr_param.hdr_shutter_mode ? "Row" : "Index");
				mw_get_shutter_time(fd_iav, value);
				for (i = 0; i < G_hdr_param.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i, value[i]);
				}
				printf("\nInput the new Shutter Frame index[0~%d]:\n> ",
					G_hdr_param.hdr_expo_num -1);
				scanf("%d", &i);
				if (i >= G_hdr_param.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_hdr_param.hdr_expo_num -1);
					break;
				}
				printf("Input new Shutter for Frame index %d\n> ", i);
				scanf("%d", &value[i]);
				if  (mw_set_shutter_time(fd_iav, value) < 0) {
					printf("mw_set_shutter_time error\n");
					break;
				}
			}
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
			if (mw_get_sensor_gain(fd_iav, value) < 0) {
				printf("mw_get_sensor_gain error\n");
				break;
			}
			if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Current sensor gain is %d dB\n", i >> 24);
				printf("Input new sensor gain in dB: (range 0 ~ 36)\n> ");
				scanf("%d", &value[0]);
				value[0] = value[0] << 24;
				if  (mw_set_sensor_gain(fd_iav, value) < 0) {
					printf("mw_set_sensor_gain error\n");
				}
			} else {
				printf("\nAgc dB:\n");
				for (i = 0; i < G_hdr_param.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i,
						 SENSOR_STEPS_PER_DB * value[i] / G_hdr_param.step);
				}
				printf("\nInput the new AGC Frame index [0~%d]:\n> ",
					G_hdr_param.hdr_expo_num -1);
				scanf("%d", &i);
				if (i >= G_hdr_param.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_hdr_param.hdr_expo_num -1);
					break;
				}
				printf("Input new AGC for Frame index %d\n> ", i);
				scanf("%d", &value[i]);
				value[i] = value[i] * G_hdr_param.step / SENSOR_STEPS_PER_DB;
				if  (mw_set_sensor_gain(fd_iav, value) < 0) {
					printf("mw_set_sensor_gain error\n");
				}
			}
			break;
		case 'm':
			printf("0 - Spot Metering, 1 - Center Metering, 2 - Average Metering, 3 - Custom Metering\n");
			mw_get_ae_metering_mode(&metering_mode);
			printf("Current ae metering mode is %d\n", metering_mode);
			printf("Input new ae metering mode:\n> ");
			scanf("%d", (int *)&metering_mode);
			mw_set_ae_metering_mode(metering_mode);
			if (metering_mode != MW_AE_CUSTOM_METERING)
				break;
			printf("Please choose the AE window:\n");
			printf("0 - left half window, 1 - right half window\n> ");
			scanf("%d", &i);
			mw_set_ae_metering_table(&custom_ae_metering_table[i]);
			break;
		case 'l':
			printf("Get current AE lines:\n");
			mw_get_ae_lines(lines, 10);
			break;
		case 'L':
			if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Set customer AE lines: (0: 60Hz, 1: 50Hz, 2: customize)\n> ");
				scanf("%d", &i);
				if (i == 0) {
					memcpy(lines, ae_60hz_lines, sizeof(ae_60hz_lines));
					mw_set_ae_lines(lines, sizeof(ae_60hz_lines) / sizeof(mw_ae_line));
				} else if (i == 1) {
					memcpy(lines, ae_50hz_lines, sizeof(ae_50hz_lines));
					mw_set_ae_lines(lines, sizeof(ae_50hz_lines) / sizeof(mw_ae_line));
				} else if (i == 2) {
					memcpy(lines, ae_customer_lines, sizeof(ae_customer_lines));
					mw_set_ae_lines(lines, sizeof(ae_customer_lines) / sizeof(mw_ae_line));
				} else {
					printf("Invalid input : %d.\n", i);
				}
			} else {
				printf("Just sample: load ae lines in hdr mode\n> ");
				img_ae_hdr_load_exp_line(hdr_ae_line,
					sizeof(hdr_ae_line) / sizeof(line_t), 1, 0);
			}
			break;
		case 'p':
			printf("Please set switch point in AE lines :\n");
			printf("Input shutter time in 1/n sec format: (range 30 ~ 120)\n> ");
			scanf("%d", &i);
			switch_point.factor[MW_SHUTTER] = DIV_ROUND(512000000, i);
			printf("Input switch point of AE line: (0 - start point; 1 - end point)\n> ");
			scanf("%d", &i);
			switch_point.pos = i;
			printf("Input sensor gain in dB: (range 0 ~ 36)\n> ");
			scanf("%d", &i);
			switch_point.factor[MW_DGAIN] = i;
			mw_set_ae_points(&switch_point, 1);
			break;
		case 'd':
			mw_get_dc_iris_pid_coef(&pid);
			printf("Current PID coefficients are: p_coef=%d,i_coef=%d,d_coef=%d\n",
					pid.p_coef, pid.i_coef, pid.d_coef);
			printf("Input new p_coef\n> ");
			scanf("%d", &pid.p_coef);
			printf("Input new i_coef\n> ");
			scanf("%d", &pid.i_coef);
			printf("Input new d_coef\n> ");
			scanf("%d", &pid.d_coef);
			mw_set_dc_iris_pid_coef(&pid);
			break;
		case 'f':
			printf("Lens type? 0 - Manul iris  1 - P iris  2 - DC iris \n> ");
			scanf("%d", &i);
			mw_set_iris_type(i);
			break;
		case 'D':
			iris_flag = mw_get_iris_type();
			printf("iris control? 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			if (MW_P_IRIS == iris_flag) {
				mw_enable_p_iris_control(i);
			} else if (MW_DC_IRIS == iris_flag) {
				mw_enable_dc_iris_control(i);
			} else {
				printf("please select lens type first! \n");
			}
			break;
		case 'S':
			printf("P iris exit step? (range 0~40)\n> ");
			scanf("%d", &i);
			if (i < 0 || i > 40) {
				printf("please enter 0-40 number \n");
			} else {
				mw_set_piris_exit_step(i);
			}
			break;
		case 'C':
			printf("Setting ae area following pre-main input buffer? (0 - disable, 1 - enable)\n> ");
			scanf("%d", &i);
			if (mw_set_ae_area(i) < 0) {
				printf("mw_set_ae_area error\n");
			}
			break;
		case 'y':
			mw_get_ae_luma_value(&i);
			printf("Current luma value is %d.\n", i);
			break;
		case 'r':
			if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				mw_get_shutter_time(fd_iav, value);
				printf("Current shutter time is 1/%d sec.\n",
					DIV_ROUND(512000000, value[0]));
				mw_get_sensor_gain(fd_iav, value);
				printf("Current sensor gain is %d dB.\n", (value[0] >> 24));
				i = img_ae_get_system_gain();
				i = i * 1024 * 6 / 128;
				printf("Current system gain is %d dB.\n", (i >> 10));
			} else {
				mw_get_shutter_time(fd_iav, value);
				printf("%s hdr shutter mode:\n",
					G_hdr_param.hdr_shutter_mode ? "Row" : "Index");
				for (i = 0; i < G_hdr_param.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i, value[i]);
				}
				mw_get_sensor_gain(fd_iav, value);
				printf("\nAgc dB:\n");
				for (i = 0; i < G_hdr_param.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i,
						 SENSOR_STEPS_PER_DB * value[i] / G_hdr_param.step);
				}
				printf("\n");
			}

			break;
		case 'P':
			if (mw_get_ae_param(&ae_param) < 0) {
				printf("mw_get_piris_lens_param error\n");
				return -1;
			}
			printf("The current range is [F%3.2f ~ F%3.2f]\n",
				(float)ae_param.lens_aperture.aperture_min / LENS_FNO_UNIT,
				(float)ae_param.lens_aperture.aperture_max / LENS_FNO_UNIT);
			printf("The spec range must be in [F%3.2f ~ F%3.2f]\n",
				(float)(ae_param.lens_aperture.FNO_min) / LENS_FNO_UNIT,
				(float)(ae_param.lens_aperture.FNO_max) / LENS_FNO_UNIT);
			printf("Input the new range:(E.x, 1.2, 2.0)\n> ");

			scanf("%f,%f", &lens_aperture[0], &lens_aperture[1]);

			ae_param.lens_aperture.aperture_min = (u32)(lens_aperture[0] * LENS_FNO_UNIT);
			ae_param.lens_aperture.aperture_max = (u32)(lens_aperture[1] * LENS_FNO_UNIT);

			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_piris_lens_param error\n");
				return -1;
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
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  m -- Select AWB mode(Need on AWB method: Normal method)\n");
		printf("  M -- Select AWB method\n");
		printf("  g -- Manually set RGB gain (Need to turn off AWB first)\n");
		printf("  c -- Set RGB gain for custom mode (Don't need to turn off AWB)\n");
	} else {
		printf("  s -- Manually set WB gain (Need to turn off AWB first)\n");
	}
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("W > ");
	return 0;
}

static int white_balance_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	mw_wb_gain wb_gain[MAX_HDR_EXPOSURE_NUM];
	mw_white_balance_method wb_method;
	mw_white_balance_mode wb_mode;

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
			if (mw_get_white_balance_mode(&wb_mode) < 0) {
				printf("mw_get_white_balance_mode error\n");
				break;
			}
			printf("Current AWB mode is:%d \n", wb_mode);
			printf("Choose AWB mode (%d~%d):\n",
				MW_WB_AUTO, MW_WB_MODE_NUMBER -1);
			printf("0 - auto 1 - 2800K 2 - 4000K 3 - 5000K 4 - 6500K 5 - 7500K");
			printf(" ... 10 - custom\n> ");
			scanf("%d", &i);
			mw_set_white_balance_mode(i);
			break;
		case 'M':
			mw_get_awb_method(&wb_method);
			printf("Current AWB method is [%d].\n", wb_method);
			printf("Choose AWB method (0 - Normal, 1 - Custom, 2 - Grey world)\n> ");
			scanf("%d", &i);
			mw_set_awb_method(i);
			break;
		case 'g':
			mw_get_rgb_gain(wb_gain);
			printf("Current rgb gain is %d,%d,%d\n", wb_gain[0].r_gain,
				wb_gain[0].g_gain, wb_gain[0].b_gain);
			printf("Input new rgb gain (Ex, 1500,1024,1400)\n> ");
			scanf("%d,%d,%d", &wb_gain[0].r_gain, &wb_gain[0].g_gain, &wb_gain[0].b_gain);
			wb_gain[0].d_gain = 1024;
			mw_set_rgb_gain(fd_iav, wb_gain);
			break;
		case 'c':
			getchar();
			printf("Enter custom method\n");
			mw_set_awb_method(MW_WB_CUSTOM_METHOD);
			printf("Must put a gray card in the front of the lens at first\n");
			printf("Then press any button\n");
			getchar();
			mw_get_wb_cal(wb_gain);
			printf("Current WB gain is %d, %d, %d\n", wb_gain[0].r_gain,
				wb_gain[0].g_gain, wb_gain[0].b_gain);
			printf("Input new RGB gain for normal method - custom mode \n");
			printf("(Ex, 1500,1024,1400) \n > ");
			scanf("%d,%d,%d", &wb_gain[0].r_gain, &wb_gain[0].g_gain, &wb_gain[0].b_gain);

			printf("Enter normal method:custom mode \n");
			mw_set_awb_method(MW_WB_NORMAL_METHOD);
			mw_set_white_balance_mode(MW_WB_CUSTOM);
			mw_set_custom_gain(wb_gain);
			printf("The new RGB gain you set for custom mode is : %d, %d, %d\n",
				wb_gain[0].r_gain, wb_gain[0].g_gain, wb_gain[0].b_gain);
			break;
		case 'q':
			exit_flag = 1;
			break;
		case 's':
			if (mw_get_wb_gain(wb_gain) < 0) {
				printf("mw_get_wb_gain error\n");
				break;
			}
			for (i  = 0; i < G_hdr_param.hdr_expo_num; i++) {
				printf("WB_GAIN[%d]: R[%d], G[%d], B[%d]\n", i,
					wb_gain[i].r_gain, wb_gain[i].g_gain, wb_gain[i].b_gain);
			}
			printf("Input the Frame index[0~%d]:\n", G_hdr_param.hdr_expo_num -1);
			scanf("%d", &i);
			if (i >= G_hdr_param.hdr_expo_num) {
				printf("The value must be [0~%d]!\n", G_hdr_param.hdr_expo_num -1);
				break;
			}
			printf("Input new WB gain for Frame index %d\n", i);
			printf("(Ex, 1500,1024,1400) \n > ");
			scanf("%d,%d,%d", &wb_gain[i].r_gain, &wb_gain[i].g_gain, &wb_gain[i].b_gain);
			mw_set_wb_gain(wb_gain);
			if (mw_set_wb_gain(wb_gain) < 0) {
				printf("mw_set_wb_gain error\n");
				break;
			}
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
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  b -- Brightness adjustment\n");
	}
	printf("  c -- Contrast adjustment\n");
	printf("  s -- Sharpness adjustment\n");
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
			printf("Input new saturation: (range 0 ~ 255)\n> ");
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
			printf("Input new contrast: (range 0 ~ 128)\n> ");
			scanf("%d", &i);
			mw_set_contrast(i);
			break;
		case 's':
			mw_get_sharpness(&i);
			printf("Current sharpness is %d\n", i);
			printf("Input new sharpness: (range 0 ~ 10)\n> ");
			scanf("%d", &i);
			mw_set_sharpness(i);
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
	printf("  m -- Set MCTF 3D noise filter strength\n");
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  L -- Set auto local exposure mode\n");
		printf("  l -- Load custom local exposure curve from file\n");
		printf("  b -- Enable backlight compensation\n");
		printf("  d -- Enable day and night mode\n");
		printf("  c -- Enable auto contrast mode\n");
		printf("  t -- Set auto contrast strength\n");
		printf("  n -- Set chroma noise filter strength\n");
		printf("  w -- Set auto wdr strength\n");
	}
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
	u32 value;
	char str[64];
	mw_local_exposure_curve local_exposure_curve;

	show_enhancement_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'm':
			mw_get_mctf_strength(&value);
			printf("Current mctf strength is %d\n", value);
			printf("Input new mctf strength: (range 0 ~ 11)\n> ");
			scanf("%d", &value);
			mw_set_mctf_strength(value);
			break;
		case 'L':
			printf("Auto local exposure (0~255): 0 - Stop, 1 - Auto, 64~255 - weakest~strongest\n> ");
			scanf("%d", &i);
			mw_set_auto_local_exposure_mode(i);
			break;
		case 'l':
			printf("Input the filename of local exposure curve: (Ex, le_2x.txt)\n> ");
			scanf("%s", str);
			if (load_local_exposure_curve(str, &local_exposure_curve) < 0) {
				printf("The curve file %s is either found or corrupted!\n", str);
				return -1;
			}
			mw_set_local_exposure_curve(fd_iav, &local_exposure_curve);
			break;
		case 'b':
			printf("Backlight compensation: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_backlight_compensation(i);
			break;
		case 'd':
			printf("Day and night mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_day_night_mode(i);
			break;
		case 'c':
			mw_get_auto_color_contrast(&value);
			printf("Current auto contrast control mode is %s\n", value ? "enabled" : "disabled");
			printf("Auto contrast control mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_set_auto_color_contrast(i);
			break;
		case 't':
			printf("Set auto contrast control strength: 0~128, 0: no effect, 128: full effect\n> ");
			scanf("%d", &i);
			mw_set_auto_color_contrast_strength(i);
			break;
		case 'j':
			mw_get_adj_status(&i);
			printf("Current ADJ is %s\n", i ? "enabled" : "disabled");
			printf("Change ADJ mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_adj(i);
			break;
		case 'n':
			if (mw_get_chroma_noise_strength(&i) < 0) {
				printf("mw_get_chroma_noise_strength error");
				break;
			}
			printf("Current chroma_noise_filter strength is %d\n", i);
			printf("Change chroma_noise_filter strength: 0~256)\n> ");
			scanf("%d", &i);
			if (mw_set_chroma_noise_strength(i) < 0) {
				printf("mw_set_chroma_noise_strength error");
				break;
			}
			break;
		case 'w':
			if (mw_get_auto_wdr_strength(&i) < 0) {
				printf("mw_get_auto_wdr_strength error");
				break;
			}
			printf("Current auto wdr strength is %d\n", i);
			printf("Change auto wdr strength: 0~128)\n> ");
			scanf("%d", &i);
			if (mw_set_auto_wdr_strength(i) < 0) {
				printf("mw_set_auto_wdr_strength error");
				break;
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

	if (mw_display_adj_bin_version() < 0) {
		perror("mw_get_version_info error");
		return -1;
	}
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

static int show_statistics_menu(void)
{
	printf("\n================ Statistics Data ================\n");
	printf("  b -- Get AWB statistics data once\n");
	printf("  e -- Get AE statistics data once\n");
	printf("  f -- Get AF statistics data once\n");
	printf("  h -- Get Histogram data once\n");
	printf("  d -- Disaplay Histogram data\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("S > ");
	return 0;
}

static int display_AWB_data(awb_data_t *pAwb_data,
	u16 tile_num_col, u16 tile_num_row)
{
	u32 i, j, sum_r, sum_g, sum_b;

	sum_r = sum_g = sum_b = 0;

	printf("== AWB CFA = [%dx%d] = [R : G : B]\n", tile_num_col, tile_num_row);

	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			sum_r += pAwb_data[i * tile_num_col + j].r_avg;
			sum_g += pAwb_data[i * tile_num_col + j].g_avg;
			sum_b += pAwb_data[i * tile_num_col + j].b_avg;
		}
	}
	printf("== AWB  CFA Total value == [R : G : B] -> [%d : %d : %d].\n",
		sum_r, sum_g, sum_b);
	sum_r = (sum_r << 10) / sum_g;
	sum_b = (sum_b << 10) / sum_g;
	printf("== AWB = Normalized to 1024 [%d : 1024 : %d].\n",
		sum_r, sum_b);

	return 0;
}

static int display_AE_data(ae_data_t *pAe_data,
	u16 tile_num_col, u16 tile_num_row)
{
	int i, j, ae_sum;

	ae_sum = 0;


	printf("== AE = [%dx%d] ==\n", tile_num_col, tile_num_row);
	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			printf("  %5d  ",  pAe_data[i * tile_num_col + j].lin_y);
			ae_sum += pAe_data[i * tile_num_col + j].lin_y;
	}
		printf("\n");
	}
	printf("== AE = [%dx%d] == Total lum value : [%d].\n",
		tile_num_col, tile_num_row, ae_sum);

	return 0;
}

static int display_AF_data(af_stat_t * pAf_data, u16 tile_num_col, u16 tile_num_row)
{
	int i, j;
	u32 sum_fv2, sum_lum;
	u16 tmp_fv2, tmp_lum;
	sum_fv2 = 0;
	sum_lum = 0;

	printf("== AF FV2 = [%dx%d] ==\n", tile_num_col, tile_num_row);

	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			tmp_fv2 = pAf_data[i * tile_num_col + j].sum_fv2;
			printf("%6d ", tmp_fv2);
			sum_fv2 += tmp_fv2;

			tmp_lum = pAf_data[i * tile_num_col + j].sum_fy;
			sum_lum += tmp_lum;
		}
		printf("\n");
	}

	printf("== AF: FV2, Lum = [%dx%d] == Total value :%d, %d\n",
		tile_num_col, tile_num_row, sum_fv2, sum_lum);

	return 0;
}

static int display_hist_data(cfa_histogram_stat_t *cfa,
	rgb_histogram_stat_t *rgb)
{
	const int total_bin_num = HIST_BIN_NUM;
	int bin_num;
	int total_rgb_y = 0;
	int total_cfg_y = 0;

	while (1) {
		printf("  Please choose the bin number (range 0~63) : ");
		scanf("%d", &bin_num);
		if ((bin_num >= 0) && (bin_num < total_bin_num))
			break;
		printf("  Invalid bin number [%d], choose again!\n", bin_num);
	}
	printf("== HIST CFA Bin [%d] [Y : R : G : B] = [%d : %d : %d : %d]\n",
		bin_num, cfa->his_bin_y[bin_num], cfa->his_bin_r[bin_num],
		cfa->his_bin_g[bin_num], cfa->his_bin_b[bin_num]);
	printf("== HIST RGB Bin [%d] [Y : R : G : B] = [%d : %d : %d : %d]\n",
		bin_num, rgb->his_bin_y[bin_num], rgb->his_bin_r[bin_num],
		rgb->his_bin_g[bin_num], rgb->his_bin_b[bin_num]);
	bin_num = 0;
	while (bin_num < total_bin_num) {
		 total_rgb_y += rgb->his_bin_y[bin_num];
		 total_cfg_y += cfa->his_bin_y[bin_num];
		 bin_num++;
	}

	printf("== HIST total Y:   rgb[%d], cfa[%d] ==\n", total_rgb_y, total_cfg_y);

	return 0;
}

static pthread_t show_histogram_task_id;
static int show_histogram_task_exit_flag = 0;
fb_show_histogram_t hist_data;
static int histogram_total_pixl = 1;
static int enable_fb_show_histogram = 0;
static int enable_terminal_show_histogram = 0;

static int get_fb_config_size(int fd, fb_vin_vout_size_t *pFb_size)
{
	iav_source_buffer_setup_ex_t buffer_setup;
	iav_system_resource_setup_ex_t resource_setup;

	if (ioctl(fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		printf("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX error \n");
		return -1;
	}

	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(fd, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup) < 0) {
		printf("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX error \n");
		return -1;
	}

	if (buffer_setup.type[IAV_ENCODE_SOURCE_BUFFER_FIRST] ==
		IAV_SOURCE_BUFFER_TYPE_ENCODE) {
		pFb_size->vin_width =   buffer_setup.size[IAV_ENCODE_SOURCE_BUFFER_FIRST].width;
		pFb_size->vin_height =  buffer_setup.size[IAV_ENCODE_SOURCE_BUFFER_FIRST].height;
	}

	if (resource_setup.encode_mode == IAV_ENCODE_WARP_MODE) {
		pFb_size->vin_width =  buffer_setup.pre_main.width;
		pFb_size->vin_height =  buffer_setup.pre_main.height;
	}

	if (buffer_setup.type[IAV_ENCODE_SOURCE_THIRD_BUFFER] ==
		IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
		pFb_size->vout_width = resource_setup.voutB.width;
		pFb_size->vout_height = resource_setup.voutB.height;
	} else if (buffer_setup.type[IAV_ENCODE_SOURCE_FOURTH_BUFFER] ==
		IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
		pFb_size->vout_width = resource_setup.voutA.width;
		pFb_size->vout_height = resource_setup.voutA.height;
		} else {
		printf("no preview for frame buffer\n");
		return -1;
	}

	histogram_total_pixl = pFb_size->vin_width * pFb_size->vin_height;

	return 0;
}

static int config_fb_vout(int fd)
{
	iav_vout_fb_sel_t fb_sel;
	fb_sel.vout_id = 1;
	fb_sel.fb_id = 0;
	if (ioctl(fd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
		perror("Change osd of vout1");
		return -1;
	}
	return 0;
}

static int hist_data_classify(cfa_histogram_stat_t *cfa,
	rgb_histogram_stat_t *rgb)
{
	const int total_bin_num = HIST_BIN_NUM;
	int bin_num;
	u32 *data;

	switch(hist_data.hist_type) {
	case 0:
		data = cfa->his_bin_y;
		break;
	case 1:
		data = cfa->his_bin_r;
		break;
	case 2:
		data = cfa->his_bin_g;
		break;
	case 3:
		data = cfa->his_bin_b;
		break;
	case 4:
		data = rgb->his_bin_y;
		break;
	case 5:
		data = rgb->his_bin_r;
		break;
	case 6:
		data = rgb->his_bin_g;
		break;
	case 7:
		data = rgb->his_bin_b;
		break;
	default:
		data = cfa->his_bin_y;
		break;
	}

	if (show_histogram_task_id) {
		hist_data.total_num = total_bin_num;
		hist_data.the_max = 0;
		for (bin_num = 0; bin_num < total_bin_num; ++bin_num) {
			hist_data.histogram_value[bin_num] = data[bin_num];
			if (hist_data.the_max < hist_data.histogram_value[bin_num])
				hist_data.the_max = hist_data.histogram_value[bin_num];
		}
		if (enable_fb_show_histogram) {
			draw_histogram_figure(&hist_data);
		}
	}
	return 0;
}

static int output_areas_af_data(fb_show_histogram_t *pHdata)
{
	int i,j;
#define CENTRE_AREA_WIDTH	4
#define CENTRE_AREA_HEIGHT	4
	int row_num = pHdata->row_num;
	int col_num = pHdata->col_num;
	int row_start = (row_num - CENTRE_AREA_WIDTH) / 2;
	int col_start  = (col_num - CENTRE_AREA_HEIGHT) / 2;

	for (i = row_start; i < row_start + CENTRE_AREA_WIDTH; ++i) {
		for (j = col_start; j < col_start + CENTRE_AREA_HEIGHT; ++j) {
			printf("%6d ", hist_data.histogram_value[i * col_num + j]);
		}
		printf("\n");
	}
	printf("\n--------------\n");

	return 0;

}

static int show_af_hist_data(af_stat_t * af_data, int af_col, int af_row)
{
	int bin_num;
	int i, j, max_value = 0;
	if (show_histogram_task_id) {
		hist_data.total_num = af_col * af_row;
		hist_data.row_num = af_row;
		hist_data.col_num = af_col;
		for (bin_num = 0; bin_num < hist_data.total_num; ++bin_num) {
			switch(hist_data.hist_type) {
			case 0:
				hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fy;
				break;
			case 1:
				hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv1;
				break;
			case 2:
			default:
				hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv2;
				break;
			}
		}

		for (i = 0; i < af_row; ++i) {
			for (j = 0; j< af_col; ++j) {
				if (max_value < hist_data.histogram_value[i * af_col + j]) {
					max_value = hist_data.histogram_value[i * af_col + j];
				}
			}
		}
		if (enable_terminal_show_histogram) {
			output_areas_af_data(&hist_data);
		}

		hist_data.the_max = max_value;
		if (enable_fb_show_histogram) {
			draw_histogram_figure(&hist_data);
		}
	}

	return 0;
}

void show_histogram_task(void *arg)
{
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;
	awb_data_t * pawb_info = NULL;
	ae_data_t * pae_info = NULL;
	af_stat_t * paf_info = NULL;

	img_aaa_stat_t aaa_shm_data;
	cfa_histogram_stat_t	*pCfa_hist = NULL;
	rgb_histogram_stat_t	*pRgb_hist = NULL;

	init_shm_get();
	memset(&aaa_shm_data, 0, sizeof(img_aaa_stat_t));
	get_shm_data();
	memcpy(&aaa_shm_data, shm, sizeof(img_aaa_stat_t));

	awb_tile_row = aaa_shm_data.tile_info.awb_tile_num_row;
	awb_tile_col = aaa_shm_data.tile_info.awb_tile_num_col;
	ae_tile_row = aaa_shm_data.tile_info.ae_tile_num_row;
	ae_tile_col = aaa_shm_data.tile_info.ae_tile_num_col;
	af_tile_row = aaa_shm_data.tile_info.af_tile_num_row;
	af_tile_col = aaa_shm_data.tile_info.af_tile_num_col;

	pawb_info = (awb_data_t *)aaa_shm_data.awb_info;
	pae_info = (ae_data_t *)((u8 *)pawb_info + awb_tile_row *
		awb_tile_col * sizeof(awb_data_t));
	paf_info = (af_stat_t *)((u8 *)pae_info + ae_tile_row *
		ae_tile_col * sizeof(ae_data_t));
	pCfa_hist = (cfa_histogram_stat_t *)((u8 *)paf_info + af_tile_row *
		af_tile_col * sizeof(af_stat_t));
	pRgb_hist = (rgb_histogram_stat_t *)((u8 *)pCfa_hist + sizeof(cfa_histogram_stat_t));

	do {
		memset(&aaa_shm_data, 0, sizeof(img_aaa_stat_t));
		get_shm_data();
		memcpy(&aaa_shm_data, shm, sizeof(img_aaa_stat_t));

		switch (hist_data.statist_type) {
		case STATIS_HIST:
			retv = hist_data_classify(pCfa_hist, pRgb_hist);
			break;
		case STATIS_AF_HIST:
	 		retv = show_af_hist_data(paf_info, af_tile_col, af_tile_row);
			break;
		default:
			printf("Invalid statistics type !\n");
			retv = -1;
			break;
		}
		if (retv) {
			printf("display error\n");
			return;
		}
		usleep (1000000);
	}while (!show_histogram_task_exit_flag);

	return;
}

static int create_show_histogram_task(fb_show_histogram_t *pHistogram)
{
	fb_vin_vout_size_t fbsize;

	if (pHistogram == NULL) {
		return -1;
	}

	show_histogram_task_exit_flag = 0;

	if (enable_fb_show_histogram) {
		if (get_fb_config_size(fd_iav, &fbsize) < 0) {
			printf("Error:get_fb_config_size\n");
			return -1;
		}
		if (init_fb(&fbsize) < 0) {
			printf("frame buffer doesn't work successfully, please check the steps\n");
			printf("Example:\n");
			printf("\tcheck whether frame buffer is malloced\n");
			printf("\t\tnandwrite --show_info | grep cmdline\n");
			printf("\tconfirm ambarella_fb module has installed\n");
			printf("\t\tmodprobe ambarella_fb\n");
			return -1;
		}
		if(config_fb_vout(fd_iav) < 0) {
			printf("Error:config_fb_vout\n");
			return -1;
		}
	}

	hist_data.statist_type = pHistogram->statist_type;
 	hist_data.hist_type = pHistogram ->hist_type;

	if (show_histogram_task_id == 0) {
		if (pthread_create(&show_histogram_task_id, NULL, (void *)show_histogram_task, NULL)
			!= 0) {
			perror("Failed. Cant create thread <show_histogram_task>!\n");
			return -1;
		}
	}
	return 0;
}

static int destroy_show_histogram_task(void)
{
	show_histogram_task_exit_flag = 1;
	if (show_histogram_task_id != 0) {
		if (pthread_join(show_histogram_task_id, NULL) != 0) {
			perror("Failed. Cant destroy thread <show_histogram_task>!\n");
		}
		printf("Destroy thread <show_histogram_task> successful !\n");
	}
	show_histogram_task_exit_flag = 0;
	show_histogram_task_id = 0;
	deinit_fb();

	return 0;
}

static int get_statistics_data(STATISTICS_TYPE type)
{
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;
	awb_data_t * pawb_info = NULL;
	ae_data_t * pae_info = NULL;
	af_stat_t * paf_info = NULL;
	img_aaa_stat_t aaa_data[MAX_HDR_EXPOSURE_NUM];
	cfa_histogram_stat_t	*pCfa_hist = NULL;
	rgb_histogram_stat_t	*pRgb_hist = NULL;
	int frame_id = 0;

	if (G_hdr_param.hdr_expo_num < MIN_HDR_EXPOSURE_NUM) {
		printf("Error: Expo num smaller than 1. \n");
		return -1;
	}

	if (G_hdr_param.hdr_expo_num == MIN_HDR_EXPOSURE_NUM) {
		init_shm_get();
		memset(&aaa_data[0], 0, sizeof(img_aaa_stat_t));
		get_shm_data();
		memcpy(&aaa_data[0], shm, sizeof(img_aaa_stat_t));
	} else {
		memset(&aaa_data[0], 0, G_hdr_param.hdr_expo_num * sizeof(img_aaa_stat_t));
		if (img_hdr_get_statis(aaa_data, G_hdr_param.hdr_expo_num) < 0) {
			printf("Error: img_hdr_get_statis \n");
			return -1;
		}
	}

	do {
		printf("\n====== Frame_id: %d ======\n", frame_id);
		awb_tile_row = aaa_data[frame_id].tile_info.awb_tile_num_row;
		awb_tile_col = aaa_data[frame_id].tile_info.awb_tile_num_col;
		ae_tile_row = aaa_data[frame_id].tile_info.ae_tile_num_row;
		ae_tile_col = aaa_data[frame_id].tile_info.ae_tile_num_col;
		af_tile_row = aaa_data[frame_id].tile_info.af_tile_num_row;
		af_tile_col = aaa_data[frame_id].tile_info.af_tile_num_col;

		pawb_info = (awb_data_t *)aaa_data[frame_id].awb_info;
		pae_info = (ae_data_t *)((u8 *)pawb_info + awb_tile_row *
			awb_tile_col * sizeof(awb_data_t));
		paf_info = (af_stat_t *)((u8 *)pae_info + ae_tile_row *
			ae_tile_col * sizeof(ae_data_t));
		pCfa_hist = (cfa_histogram_stat_t *)((u8 *)paf_info + af_tile_row *
			af_tile_col * sizeof(af_stat_t));
		pRgb_hist = (rgb_histogram_stat_t *)((u8 *)pCfa_hist + sizeof(cfa_histogram_stat_t));

		switch (type) {
			case STATIS_AWB:
				retv = display_AWB_data(pawb_info, awb_tile_col, awb_tile_row);
				break;
			case STATIS_AE:
				retv = display_AE_data(pae_info, ae_tile_col, ae_tile_row);
				break;
			case STATIS_AF:
				retv = display_AF_data(paf_info, af_tile_col, af_tile_row);
				break;
			case STATIS_HIST:
				retv = display_hist_data(pCfa_hist, pRgb_hist);
				break;
			default:
				printf("Invalid statistics type !\n");
				retv = -1;
				break;
		}
		if (retv < 0) {
			break;
		}
	} while(++frame_id < G_hdr_param.hdr_expo_num);

	return retv;
}


static int show_histogram_menu()
{
	printf("\n================ Histogram Settings ================\n");
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  e -- enable/disable fb to show histogram data(must set before 'a'&'f')\n");
		printf("  m -- enable/disable terminal print histogram data(center)\n");
	}
	printf("  a -- AE tiles Histogram \n");
	printf("  f -- AF data Histogram \n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("d > ");
	return 0;
}

static int active_histogram(fb_show_histogram_t *pHistogram)
{
	if (pHistogram == NULL) {
		return -1;
	}
	 if (show_histogram_task_id) {
		if (destroy_show_histogram_task() < 0)  {
			printf("Error: destroy_show_histogram_task\n");
			return -1;
		}
	}

	if (create_show_histogram_task(pHistogram) < 0) {
		printf("Error:create_show_histogram_task\n> ");
		show_histogram_task_id = 0;
		return -1;
	}

	return 0;
}

int print_data(int *pData, int row, int col)
{
	int i, j;
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) {
			printf("%6d", pData[i*col + j]);
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}
static int hdr_active_histogram(fb_show_histogram_t *pHistogram)
{
	int idx = 0, i;
	static img_aaa_stat_t hdr_statis_gp[MAX_HDR_EXPOSURE_NUM];
	memset(hdr_statis_gp, 0, MAX_HDR_EXPOSURE_NUM*sizeof(img_aaa_stat_t));
	int *buf = NULL, row, col;
	if (pHistogram == NULL) {
		return -1;
	}

	if (img_hdr_get_statis(hdr_statis_gp, G_hdr_param.hdr_expo_num) < 0) {
		printf("Error: img_hdr_get_statis \n");
		return -1;
	}

	switch (pHistogram->statist_type) {
	case STATIS_HIST:
		printf("Select the slice [0~%d]\n > ", HIST_BIN_NUM -1);
		scanf("%d", &i);
		if (i >= HIST_BIN_NUM) {
			printf("The value must be [0~%d]\n", HIST_BIN_NUM -1);
			return -1;
		}
		for (idx = 0; idx < G_hdr_param.hdr_expo_num; idx++) {
			switch (pHistogram->hist_type) {
			case 0:
				printf("[%d]-%8d\t", idx, hdr_statis_gp[idx].cfa_hist.his_bin_y[i]);
				break;
			case 1:
				printf("[%d]-%8d\t", idx, hdr_statis_gp[idx].cfa_hist.his_bin_r[i]);
				break;
			case 2:
				printf("[%d]-%8d\t", idx, hdr_statis_gp[idx].cfa_hist.his_bin_g[i]);
				break;
			case 3:
				printf("[%d]-%8d\t",idx, hdr_statis_gp[idx].cfa_hist.his_bin_b[i]);
				break;
			case 4:
				printf("[%d]-%8d\t",idx, hdr_statis_gp[idx].rgb_hist.his_bin_y[i]);
				break;
			case 5:
				printf("[%d]-%8d\t", idx, hdr_statis_gp[idx].rgb_hist.his_bin_r[i]);
				break;
			case 6:
				printf("[%d]-%8d\t", idx, hdr_statis_gp[idx].rgb_hist.his_bin_g[i]);
				break;
			case 7:
				printf("[%d]-%8d\t",idx, hdr_statis_gp[idx].rgb_hist.his_bin_b[i]);
				break;
			default:
				printf("No the histogram type\n");
				return -1;
			}
		}
		printf("\n");
		break;
	case STATIS_AF_HIST:
		row =  hdr_statis_gp[idx].tile_info.af_tile_num_row;
		col =  hdr_statis_gp[idx].tile_info.af_tile_num_col;
		buf = malloc(sizeof(int) * row * col);

		for (idx = 0; idx < G_hdr_param.hdr_expo_num; idx++) {
			printf("Frame index[%d]:\n", idx);
			switch (pHistogram->hist_type) {
			case 0:
				for (i = 0; i < row * col; i++) {
					buf[i] = hdr_statis_gp[idx].af_info[i].sum_fy;
				}
				break;
			case 1:
				for (i = 0; i < row * col; i++) {
					buf[i] = hdr_statis_gp[idx].af_info[i].sum_fv1;
				}
				break;
			case 2:
				for (i = 0; i < row * col; i++) {
					buf[i] = hdr_statis_gp[idx].af_info[i].sum_fv2;
				}
				break;
			default:
				printf("No the histogram type\n");
				if (buf != NULL) {
					free(buf);
					buf = NULL;
				}
				return -1;
			}
			print_data(buf,row, col);
		}

		if (buf != NULL) {
			free(buf);
			buf = NULL;
		}
		break;
	default:
		printf("Error: No the type\n");
		return -1;
	}

	return 0;
}


static int histogram_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	fb_show_histogram_t hist;

	show_histogram_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'a':
			printf("Select ae histogram type: 0 - CFA:Y, 1 - CFA:R, 2 - CFA:G, 3 - CFA:B\n");
			printf("\t\t\t  4 - RGB:Y, 5 - RGB:R, 6 - RGB:G, 7 - RGB:B\n> ");
			scanf("%d", &i);
			if (i < 0 || i > 7) {
				printf("Error:The value must be 0~7\n> ");
				break;
			}
			hist.statist_type = STATIS_HIST;
			hist.hist_type = i;
			if (G_hdr_param.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
				hdr_active_histogram(&hist);
			} else {
				active_histogram(&hist);
			}
			break;
		case 'f':
			printf("Select af histogram type: 0 - FY, 1 - FV1, 2 - FV2\n");
			scanf("%d", &i);
			if (i < 0 || i > 2) {
				printf("Error:The value must be 0~2\n> ");
				break;
			}
			hist.statist_type = STATIS_AF_HIST;
			hist.hist_type = i;
			if (G_hdr_param.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
				hdr_active_histogram(&hist);
			} else {
				active_histogram(&hist);
			}
			break;
		case 'e':
			printf("0 - fb disable  1 - fb enable\n> ");
			scanf("%d", &i);
			if ( i > 1 || i < 0) {
				printf("the value must be 0|1\n> ");
				break;
			}
			if (show_histogram_task_id > 0) {
				destroy_show_histogram_task();
			}
			enable_fb_show_histogram = i;
			break;
		case 'm':
			printf("0 - disable output  1 - enable output\n> ");
			scanf("%d", &i);
			if ( i > 1 || i < 0) {
				printf("the value must be 0|1\n> ");
				break;
			}
			enable_terminal_show_histogram = i;
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
			show_histogram_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int statistics_data_setting(void)
{
	int exit_flag, error_opt;
	char ch;

	show_statistics_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'b':
			get_statistics_data(STATIS_AWB);
			break;
		case 'e':
			get_statistics_data(STATIS_AE);
			break;
		case 'f':
			get_statistics_data(STATIS_AF);
			break;
		case 'h':
			get_statistics_data(STATIS_HIST);
			break;
		case 'd':
			histogram_setting();
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
			show_statistics_menu();
		}
		ch = getchar();
	}

	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
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
	printf("  s -- Get statistics data (AE, AWB, AF, Histogram)\n");
	printf("  h -- Enhancement settings (3D de-noise, local exposure, backlight)\n");
	printf("  m -- Misc settings (Log level)\n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static void sigstop(int signo)
{

	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
	}
	if (!load_mctf_flag) {
		mw_stop_aaa();
	}
	close(fd_iav);
	exit(1);
}

#define NO_ARG	0
#define HAS_ARG	1

static const char* short_options = "d:e:i:l:p:L:m:";
static struct option long_options[] = {
	{"mode", HAS_ARG, 0, 'i'},
	{"load", HAS_ARG, 0, 'l'},
	{"aeb", HAS_ARG, 0, 'e'},
	{"adj", HAS_ARG, 0, 'd'},
	{"piris", HAS_ARG, 0, 'p'},
	{"lens", HAS_ARG, 0, 'L'},
	{"log-level", HAS_ARG, 0, 'm'},
	{0, 0, 0, 0},
};
struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1|2", "0:background mode, 1:interactive mode, 2: auto test start/stop aaa mode"},
	{"", "load filter binary"},
	{"", "load aeb binary file for 3A params"},
	{"", "load adj binary file for 3A params"},
	{"", "load piris params binary file for 3A params"},
	{"", "config lens id (0: fixed lens; 2: p-iris M13VP288IR)"},
	{"0~2", "set log level"},

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
		case 'l':
			load_mctf_flag = 1;
			strncpy(G_mctf_file, optarg, FILE_NAME_LENGTH);
			G_mctf_file[sizeof(G_mctf_file) - 1] = '\0';
			break;
		case 'e':
			strncpy(G_file_name[FILE_TYPE_AEB], optarg,
				sizeof(G_file_name[FILE_TYPE_AEB]));
			G_file_name[FILE_TYPE_AEB][sizeof(G_file_name[FILE_TYPE_AEB]) - 1] = '\0';
			load_file_flag[FILE_TYPE_AEB] = 1;
			break;
		case 'd':
			strncpy(G_file_name[FILE_TYPE_ADJ], optarg,
				sizeof(G_file_name[FILE_TYPE_ADJ]));
			G_file_name[FILE_TYPE_ADJ][sizeof(G_file_name[FILE_TYPE_ADJ]) - 1] = '\0';
			load_file_flag[FILE_TYPE_ADJ] = 1;
		case 'p':
			strncpy(G_file_name[FILE_TYPE_PIRIS], optarg,
				sizeof(G_file_name[FILE_TYPE_PIRIS]));
			G_file_name[FILE_TYPE_PIRIS][sizeof(G_file_name[FILE_TYPE_PIRIS]) - 1] = '\0';
			load_file_flag[FILE_TYPE_PIRIS] = 1;
			break;
		case 'L':
			i = atoi(optarg);
			if ((i == LENS_M13VP288IR_ID) || (i == LENS_CMOUNT_ID)) {
				G_lens_id = i;
			} else {
				printf("No support\n");
				return -1;
			}
			break;
		case 'm':
			i = atoi(optarg);
			if (i < MW_ERROR_LEVEL || i >= (MW_LOG_LEVEL_NUM - 1)) {
				printf("Invalid log level, please set it in the range of (0~%d).\n",
					(MW_LOG_LEVEL_NUM - 1));
				return -1;
			}
			G_log_level = i;
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
	printf("\ttest_image -i 0\n");
	printf("1. run with interactive menu\n");
	printf("\ttest_image -i 1\n");
	printf("2. run with test mode: auto test start/stop 3A\n");
	printf("\ttest_image -i 2\n");
	printf("3. only load mctf params file(the file must contain %d params)...\n",
		MCTF_INFO_STRUCT_NUM);
	printf("\ttest_image -l mctf_off.txt\n");
	printf("4. open 3A with M13VP288IR p-iris lens(imx123)\n");
	printf("\ttest_image -i 1 -p /etc/idsp/adj_params/m13vp288ir_piris_param.bin -L 2\n");

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
		case 's':
			statistics_data_setting();
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
	return 0;
}

int start_aaa_main()
{
	if (mw_start_aaa(fd_iav) < 0) {
		perror("mw_start_aaa");
		return -1;
	}
	/*
	 * Exposure Control Settings
	 */
	if (mw_get_sensor_param_for_3A(&G_hdr_param) < 0) {
		printf("mw_get_sensor_hdr_expo error\n");
		return -1;
	}
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("===Sensor is Normal mode\n");
	} else {
		printf("===Sensor is %dX HDR mode", G_hdr_param.hdr_expo_num);
	}

	mw_enable_ae(1);
	mw_enable_dc_iris_control(dc_iris_enable);
	mw_set_dc_iris_pid_coef(&pid_coef);

	/*
	 * White Balance Settings
	 */
	mw_enable_awb(1);
	/*
	 * Image Adjustment Settings
	 */
	mw_set_saturation(saturation);
	mw_set_contrast(contrast);
	mw_set_sharpness(sharpness);

	/*
	 * Image Enhancement Settings
	 */
	mw_set_mctf_strength(mctf_strength);

	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		mw_set_ae_param(&ae_param);
		mw_set_ae_metering_mode(metering_mode);

	/*
	 * White Balance Settings
	 */
		mw_set_white_balance_mode(MW_WB_AUTO);

	/*
	 * Image Adjustment Settings
	 */
		mw_set_brightness(brightness);

	/*
	 * Image Enhancement Settings
	 */
		mw_set_auto_local_exposure_mode(auto_local_exposure_mode);
		mw_enable_backlight_compensation(backlight_comp_enable);
		mw_enable_day_night_mode(day_night_mode_enable);
	}

	return 0;
}

int parse_mctf_params(u16 *buf, mw_mctf_info_t *params, int num)
{
#define MCTF_ONE_CHAN_MEMBERS	13
#define MCTF_CHAN_NUM	3
	int i = 0;

	if ((buf == NULL) || (params == NULL) || num < MCTF_INFO_STRUCT_NUM) {
		printf ("The params is not correct\n");
		return -1;
	}

	for (i = 0; i < MCTF_CHAN_NUM; i++) {
		params->chan_info[i].temporal_alpha = (u8) buf[MCTF_CHAN_NUM * i];
		params->chan_info[i].temporal_alpha1 = (u8) buf[MCTF_CHAN_NUM * i + 1];
		params->chan_info[i].temporal_alpha2 = (u8) buf[MCTF_CHAN_NUM * i + 2];
		params->chan_info[i].temporal_alpha3 = (u8) buf[MCTF_CHAN_NUM * i + 3];
		params->chan_info[i].temporal_t0 = (u8) buf[MCTF_CHAN_NUM * i + 4];
		params->chan_info[i].temporal_t1 = (u8) buf[MCTF_CHAN_NUM * i + 5];
		params->chan_info[i].temporal_t2 = (u8) buf[MCTF_CHAN_NUM * i + 6];
		params->chan_info[i].temporal_t3 = (u8) buf[MCTF_CHAN_NUM * i + 7];
		params->chan_info[i].temporal_maxchange = (u8) buf[MCTF_CHAN_NUM * i + 8];
		params->chan_info[i].radius = buf[MCTF_CHAN_NUM * i + 9];
		params->chan_info[i].str_3d = buf[MCTF_CHAN_NUM * i + 10];
		params->chan_info[i].str_spatial = buf[MCTF_CHAN_NUM * i + 11];
		params->chan_info[i].level_adjust = buf[MCTF_CHAN_NUM * i + 12];
	}

	params->combined_str_y = buf[MCTF_ONE_CHAN_MEMBERS * MCTF_CHAN_NUM];

	return num;
}

static int load_yuv_mctf_param(char *mctf_file, mw_mctf_info_t *mctf_param)
{
#define FILE_SIZE	1024
	char buf[FILE_SIZE];
	FILE *fp;
	u16 param[MCTF_INFO_STRUCT_NUM];
	int argcnt = 0;
	int num;

	if ((fp = fopen(mctf_file, "r")) == NULL) {
		printf("Open mctf file  [%s] failed!\n", mctf_file);
		return -1;
	}

	while (fgets(buf, FILE_SIZE, fp) != NULL) {
		get_multi_arg(buf, &(param[argcnt]), &num);
		argcnt += num;
	}

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	if ((MCTF_INFO_STRUCT_NUM != argcnt) || (argcnt == 0)) {
		printf("%s total params[%d], not correct\n", mctf_file, argcnt);
		return -1;
	}

	if (parse_mctf_params(param, mctf_param, argcnt) != argcnt) {
		perror("transfer_params");
		return -1;
	}

	return 0;
}

int main(int argc, char ** argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);
	int i = 0;
	mw_mctf_info_t	mctf_info;

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
	if (mw_set_log_level(G_log_level) < 0) {
		perror("mw_set_log_level");
		return -1;
	}

	if (load_mctf_flag) {
		memset(&mctf_info, 0, sizeof(mctf_info));
		if (load_yuv_mctf_param(G_mctf_file, &mctf_info) < 0) {
			perror("load_yuv_mctf_param");
			return -1;
		}
		if (mw_init_mctf(fd_iav, &mctf_info) < 0) {
			perror("mw_load_mctf");
			return -1;
		}
	} else {
		for (i = FILE_TYPE_FIRST; i < FILE_TYPE_LAST; i++) {
			if (load_file_flag[i]) {
				if (mw_load_aaa_param_file(G_file_name[i], i) < 0)
					return -1;
			}
		}
		if (load_file_flag[FILE_TYPE_PIRIS] && (G_lens_id != LENS_CMOUNT_ID)) {
			if (mw_set_lens_id(G_lens_id) < 0)
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
					mw_start_aaa(fd_iav);
				}
				break;
			default:
				printf("Unknown option!\n");
				break;
		}
	}

	return 0;
}

