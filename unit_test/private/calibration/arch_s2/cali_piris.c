/*******************************************************************************
 * cali_fisheye_center.c
 *
 * Histroy:
 *  April 15, 2014 - [Lei Hong] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/shm.h>

#include "img_struct_arch.h"
#include "mw_struct.h"
#include "mw_api.h"
#include "amba_p_iris.h"

#include "utils.h"



/* the device file handle */
int fd_iris;
int fd_iav;

#define NO_ARG    0
#define HAS_ARG   1

#define PIRIS_LUMA_HIGH_OFFSET		300
#define PIRIS_LUMA_MIDDLE_OFFSET	200
#define PIRIS_LUMA_LOW_OFFSET		100
#define	PIRIS_LUMA_SMALL_OFFSET     50
#define PIRIS_LUMA_HIGH_THROTTLE	8000
#define PIRIS_LUMA_MIDDLE_THROTTLE  5000
#define PIRIS_LUMA_LOW_THROTTLE     2000
#define AE_NUMBER			96

#define IRIS_DEVICE_NAME "/dev/amb_iris"
#define DEFAULT_KEY 		5678 //don't modify it
#define SHM_DATA_SZ 		sizeof(img_aaa_stat_t)  //don't modify it

static int loglevel = LOG_INFO;
static int show_luma_flag = 0;

static int default_exposure_level[MAX_HDR_EXPOSURE_NUM] = {200};
mw_sensor_param G_hdr_param = {0};

static int shmid = 0;
static char *shm = NULL;
u32	dc_iris_enable = 0;
mw_ae_metering_mode metering_mode	= MW_AE_CENTER_METERING;
mw_dc_iris_pid_coef pid_coef= {1500, 1, 1000};


/****************************************************/
/*	Image Adjustment Settings							*/
/****************************************************/
u32	saturation = 64;
u32	brightness = 0;
u32	contrast = 64;
u32	sharpness = 6;

/****************************************************/
/*	Image Enhancement Settings							*/
/****************************************************/
u32	mctf_strength = 1;
u32	auto_local_exposure_mode = 0;
u32	backlight_comp_enable = 0;
u32	day_night_mode_enable = 0;

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

typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_HIST,
	STATIS_AF_HIST,
} STATISTICS_TYPE;


static struct option long_options[] = {
  {"show-luma", HAS_ARG, 0, 'p'},
  {"exposure-level", HAS_ARG, 0, 'e'},
  {"level", HAS_ARG, 0, 'l'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const char *short_options = "p:e:l:";

static const struct hint_s hint[] = {
  {"", "\t\t show luma value and piris step"},
  {"", "\t\t set exposure level default:200 range: 25~400"},
  {"", "\t\t set log level 0: error, 1: info (default), 2: debug"}
};

static void usage(int argc, char **argv)
{
	int i;

	printf("\n%s usage:\n", argv[0]);
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
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;
	int i = 0;

	if (argc < 2) {
		usage(argc, argv);
		return -1;
	}

	while ((ch = getopt_long(argc, argv,short_options,long_options,&option_index)) != -1) {
		switch (ch) {
		  case 'p':
		    show_luma_flag = atoi(optarg);
		    break;
		  case 'e':
			for(i = 0;i < MAX_HDR_EXPOSURE_NUM;i++) {
		    	default_exposure_level[i] = atoi(optarg);
			}

			if(default_exposure_level[0] < 25 || default_exposure_level[0] > 400) {
				printf("exposure level value out of range, error value is %d, (rang: 25~400) \n",default_exposure_level[0]);
				return -1;
			}
		    break;
		  case 'l':
		  	loglevel = atoi(optarg);
			break;
		  default:
		    printf("Unknown option %d.\n", ch);
		    return -1;
		    break;
		}
	}

  return 0;
 }

static int piris_reset(void)
{
	int ret;
	ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_RESET, 0);
	if (ret < 0) {
		printf("AMBA_IOC_P_IRIS_RESET ERROR \n");
		return -1;
	}
	return 0;
}


static int piris_move(int steps)
{
	int ret;
	ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_MOVE_STEPS, steps);
	if (ret < 0) {
		printf("AMBA_IOC_P_IRIS_MOVE_STEPS ERROR \n");
		return -1;
	}
	return 0;
}

static int init_shm_get(void)
{
	/* Locate the segment.*/
	if ((shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666)) < 0) {
		printf("3A is off!\n");
		printf("shmget error\n");
		exit(1);
	}

	return 0;
}

static int get_shm_data(void)
{
	if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*) - 1) {
		printf("shmat error\n");
		exit(1);
	}
	return 0;
}

static int get_AE_data(ae_data_t *pAe_data,
	u16 tile_num_col, u16 tile_num_row)
{
	int i, j, ae_sum;

	ae_sum = 0;

	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			ae_sum += pAe_data[i * tile_num_col + j].lin_y;
		}
	}

	return (ae_sum/AE_NUMBER);
}




static int get_ae_statistics_data(STATISTICS_TYPE type)
{
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	awb_data_t * pawb_info = NULL;
	ae_data_t * pae_info = NULL;
	img_aaa_stat_t aaa_shm_data;

	init_shm_get();
	memset(&aaa_shm_data, 0, sizeof(img_aaa_stat_t));
	get_shm_data();
	memcpy(&aaa_shm_data, shm, sizeof(img_aaa_stat_t));

	awb_tile_row = aaa_shm_data.tile_info.awb_tile_num_row;
	awb_tile_col = aaa_shm_data.tile_info.awb_tile_num_col;
	ae_tile_row = aaa_shm_data.tile_info.ae_tile_num_row;
	ae_tile_col = aaa_shm_data.tile_info.ae_tile_num_col;

	pawb_info = (awb_data_t *)aaa_shm_data.awb_info;
	pae_info = (ae_data_t *)((u8 *)pawb_info + awb_tile_row *
		awb_tile_col * sizeof(awb_data_t));

	switch (type) {
		case STATIS_AE:
			retv = get_AE_data(pae_info, ae_tile_col, ae_tile_row);
			break;
		default:
			printf("Invalid statistics type !\n");
			retv = -1;
			break;
	}

	return retv;
}


static int piris_show_luma()
{
	int i = 0;
	int count = 100;
	int luma_value_max = 0;
	int luma_value = 0;
	int piris_luma_offset = 0;
	mw_enable_ae(0);
	mw_set_exposure_level(default_exposure_level);

	if (piris_reset() < 0) {
		printf("piris reset error.\n");
		return -1;
	}

	sleep(5);
	luma_value_max = get_ae_statistics_data(STATIS_AE);

	if (luma_value_max > PIRIS_LUMA_HIGH_THROTTLE) {
		piris_luma_offset = PIRIS_LUMA_HIGH_OFFSET;
	} else if ((luma_value_max < PIRIS_LUMA_HIGH_THROTTLE) && (luma_value_max > PIRIS_LUMA_MIDDLE_THROTTLE)) {
		piris_luma_offset = PIRIS_LUMA_MIDDLE_OFFSET;
	} else if ((luma_value_max < PIRIS_LUMA_MIDDLE_THROTTLE) && (luma_value_max > PIRIS_LUMA_LOW_THROTTLE)) {
		piris_luma_offset = PIRIS_LUMA_LOW_OFFSET;
	} else {
		piris_luma_offset = PIRIS_LUMA_SMALL_OFFSET;
	}

	printf("piris position is 0 \n");
	printf("luma_value is %d \n",luma_value_max);
	printf("F aperture is 1.2 \n");

	for(i = 1; i < count; i++) {
		if (0 == piris_move(1)) {
			luma_value = get_ae_statistics_data(STATIS_AE);
			DEBUG("piris position is %d.\n",i);
			DEBUG("step luma value is %d .\n",luma_value);

			if (luma_value < piris_luma_offset) {
				printf("luma value is %d. \n",luma_value);
				return 0;
			} else if (((luma_value_max/2 - piris_luma_offset) < luma_value) && (luma_value < (luma_value_max/2 + piris_luma_offset))) {
				printf("piris position is %d.\n",i);
				printf("luma value is %d.\n",luma_value);
				printf("F aperture is 1.2x1.414 \n");
			} else if (((luma_value_max/4 - piris_luma_offset) < luma_value) && (luma_value < (luma_value_max/4 + piris_luma_offset))) {
				printf("piris position is %d.\n",i);
				printf("luma value is %d.\n",luma_value);
				printf("F aperture is 2.4 \n");
			} else if (((luma_value_max/8 - piris_luma_offset) < luma_value) && (luma_value < (luma_value_max/8 + piris_luma_offset))) {
				printf("piris position is %d.\n",i);
				printf("luma value is %d.\n",luma_value);
				printf("F aperture is 2.4*1.414 \n");
			} else if (((luma_value_max/16 - piris_luma_offset) < luma_value) && (luma_value < (luma_value_max/16 + piris_luma_offset))) {
				printf("piris position is %d.\n",i);
				printf("luma value is %d.\n",luma_value);
				printf("F aperture is 4.8 \n");
			} else if (((luma_value_max/32 - piris_luma_offset) < luma_value) && (luma_value < (luma_value_max/32 + piris_luma_offset))) {
				printf("piris position is %d.\n",i);
				printf("luma value is %d.\n",luma_value);
				printf("F aperture is 4.8*1.414 \n");
			}
		} else {
			printf("piris move error.\n");
			return -1;
		}
	}

	mw_enable_ae(1);

	if (piris_reset() < 0) {
		printf("piris reset error.\n");
		return -1;
	}

	return 0;
}

int start_3a_main()
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
	/*mw_enable_dc_iris_control(dc_iris_enable);
	mw_set_dc_iris_pid_coef(&pid_coef);*/

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


int main(int argc, char **argv)
{
	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		printf("open /dev/iav error \n");
		return -1;
	}

	if (start_3a_main() < 0 ) {
		printf("start_3a_main error \n");
		return -1;
	}

	sleep(5);

	set_log(loglevel, NULL);

	/* open the device */
	if ((fd_iris = open(IRIS_DEVICE_NAME , O_RDWR, 0)) < 0) {
		printf("open" IRIS_DEVICE_NAME "error \n");
		return -1;
	}

	if (show_luma_flag) {
		if (piris_show_luma() < 0) {
			printf("piris_show_luma error .\n");
			return -1;
		}
	}

	return 0;
}
