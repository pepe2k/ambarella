/**********************************************************************
 *
 * mw_image_priv.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *	2012/03/23 - [Jian Tang] Modified this file
 *
 * Copyright (C) 2007 - 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include <pthread.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "ambas_imgproc_arch.h"

#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_hdr_api_arch.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"

#include "mn34210pl_adj_param_hdr.c"
#include "mn34210pl_aeb_param_hdr.c"

#include "mn34220pl_adj_param_hdr.c"
#include "mn34220pl_aeb_param_hdr.c"

#include "ov4689_adj_param_hdr.c"
#include "ov4689_aeb_param_hdr.c"

#include "imx123_adj_param_hdr.c"
#include "imx123_aeb_param_hdr.c"

#include "imx224_adj_param_hdr.c"
#include "imx224_aeb_param_hdr.c"

typedef enum  {
	SENSOR_50HZ_LINES,
	SENSOR_60HZ_LINES,
	SENSOR_60P50HZ_LINES,
	SENSOR_60P60HZ_LINES,
	PIRIS_SENSOR_50HZ_LINES,
	PIRIS_SENSOR_60HZ_LINES,
	PIRIS_SENSOR_60P50HZ_LINES,
	PIRIS_SENSOR_60P60HZ_LINES,
	LINES_TOTAL_NUM,
} LINES_ID;

typedef enum  {
	SHARPEN_B_ENABLED_OFFSET = 0,
	SHARPEN_B_DISABLED_OFFSET,
} SHARPEN_B_OFFSET;

/***************************************
*
*  Sensor: ar0331
*
***************************************/

typedef enum  {
	AR0331_LINEAR_ADJ_PARAM,
	AR0331_ADJ_PARAM,
	AR0331_ADJ_PARAM_TOTAL_NUM,
} AR0331_ADJ_PARAM_ID;

typedef enum  {
	AR0331_LINEAR_AWB_PARAM,
	AR0331_AWB_PARAM,
	AR0331_AWB_PARAM_TOTAL_NUM,
} AR0331_AWB_PARAM_ID;

typedef enum  {
	AR0331_LINEAR_SENSOR_CONFIG,
	AR0331_SENSOR_CONFIG,
	AR0331_SENSOR_CONFIG_TOTAL_NUM,
} AR0331_SENSOR_CONFIG_ID;


#define CALI_FILES_PATH		"/ambarella/calibration"
#define	ADJ_PARAM_PATH	"/etc/idsp/adj_params"

#define SHT_TIME(SHT_Q9)		DIV_ROUND(512000000, (SHT_Q9))

#define AWB_TILE_NUM_COL		(24)
#define AWB_TILE_NUM_ROW		(16)

#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)

/************************** VIN_VSYNC *********************************/

static char	vin_int_array[8];
static int		G_vin_tick = 0;
static u32		G_vin_width = 1920;
static u32		G_vin_height = 1080;
static int G_aaa_task_active = 0;

static lens_cali_t lens_cali_info = {
	NORMAL_RUN,
	{92},
};

extern mw_sensor_param G_sensor_param;
extern mw_aperture_param G_lens_aperture;
extern u32 g_is_rgb_sensor_vin;

/*************************** AE lines **********************************/

static u8 G_gain_table[] = {
	ISO_100,
	ISO_150,
	ISO_200,	//6db
	ISO_300,
	ISO_400,	//12db
	ISO_600,
	ISO_800,	//18db
	ISO_1600,	//24db
	ISO_3200 ,	//30db
	ISO_6400,	//36db
	ISO_12800,	//42db
	ISO_25600,	//48db
	ISO_51200,	//54db
	ISO_102400,//60db
};
#define	GAIN_TABLE_NUM		(sizeof(G_gain_table) / sizeof(u8))

static line_t G_50HZ_LINES[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F11}}
	},

	{
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F5P6}}
	},

	{
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_F5P6}}
	},

	{
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_F2P8}}
	},

	{
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY100_SEC, ISO_200, APERTURE_F2P8}}
	},

	{
		{{SHUTTER_1BY50_SEC, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY50_SEC, ISO_100, APERTURE_F1P2}}
	},

	{
		{{SHUTTER_1BY50_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY50_SEC, ISO_6400,APERTURE_F1P2}}
	},

	{
		{{SHUTTER_1BY33_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY33_SEC, ISO_6400, APERTURE_F1P2}},
	},

	{
		{{SHUTTER_1BY30_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY30_SEC, ISO_6400,APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY15_SEC, ISO_6400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY7P5_SEC, ISO_6400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

static line_t G_60HZ_LINES[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024_SEC, ISO_100,APERTURE_F11}}
	},

	{
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F5P6}}
	},

	{
		{{SHUTTER_1BY1024_SEC, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_F5P6}}
	},

	{
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_F2P8}}
	},

	{
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY120_SEC, ISO_200, APERTURE_F2P8}}
	},

	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_F1P2}}
	},

	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY60_SEC, ISO_400,APERTURE_F1P2}}
	},

	{
		{{SHUTTER_1BY40_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY40_SEC, ISO_6400,APERTURE_F1P2}}
	},

	{
		{{SHUTTER_1BY30_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY30_SEC, ISO_6400,APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY15_SEC, ISO_6400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY7P5_SEC, ISO_6400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

//line_t G_50HZ_LINES[MAX_AE_LINE_NUM];
//line_t G_60HZ_LINES[MAX_AE_LINE_NUM];
static line_t G_mw_ae_lines[MAX_AE_LINE_NUM];
static int G_line_num;
static int G_line_belt_default;
static int G_line_belt_current;

/********************* Slow shutter task control *****************************/
static u32			slow_shutter_task_exit_flag = 0;
static pthread_t	slow_shutter_task_id = 0;

/*********************************************************************/
sensor_model_t G_mw_sensor_model = {
	.load_files = {
		.adj_file = "",
		.aeb_file = "",
		.lens_file = "",
	},
	.lens_id = LENS_CMOUNT_ID,
	.sensor_slow_shutter = 0,
};

/*****************************Convert vig to awb control********************************/
static u32 lookup_shift = -1;
static u32 G_vig_to_awb = 0;


/* ***************************************************************
	Calibration data file
******************************************************************/
mw_cali_file			G_mw_cali;


/**********************************************************************
 *      Internal functions
 *********************************************************************/
static int search_nearest(u32 key, u32 arr[], int size) 	//the arr is in reverse order
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	while(1) {
		if (l == r)
			return l;
		if (key > arr[m]) {
			r = m;
		} else if  (key < arr[m]) {
			l = m + 1;
		} else {
			return m;
		}
		m = (l+r) / 2;
	}
	return -1;
}

extern u32 TIME_DATA_TABLE_128[SHUTTER_TIME_TABLE_LENGTH];
int shutter_q9_to_index(u32 shutter_time)
{
	int tmp_idx;
	tmp_idx = search_nearest(shutter_time, TIME_DATA_TABLE_128, SHUTTER_TIME_TABLE_LENGTH);
	return SHUTTER_TIME_TABLE_LENGTH - tmp_idx;
}

u32 shutter_index_to_q9(int index)
{
	if (index < 0 || index >= SHUTTER_TIME_TABLE_LENGTH) {
		MW_ERROR("The index is not in the range\n");
		return -1;
	}

	return TIME_DATA_TABLE_128[index];
}

extern const u32 IRIS_DATA_TABLE_128[IRIS_FNO_TABLE_LENGTH];

static int search_nearest_order(u32 key, const u32 arr[], int size) //the arr is in order
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	while(1) {
		if (l == r)
			return l;
		if (key < arr[m]) {
			r = m;
		} else if  (key > arr[m]) {
			l = m + 1;
		} else {
			return m;
		}
		m = (l+r) / 2;
	}
	return -1;
}

int iris_to_index(u32 lens_fno)
{
	int tmp_idx;

	tmp_idx = search_nearest_order(lens_fno,
		IRIS_DATA_TABLE_128, IRIS_FNO_TABLE_LENGTH);
	return IRIS_FNO_TABLE_LENGTH - tmp_idx;
}

u32 iris_index_to_fno(int index)
{
	if (index < 0 || index >= IRIS_FNO_TABLE_LENGTH) {
		return 0;
	}

	return IRIS_DATA_TABLE_128[IRIS_FNO_TABLE_LENGTH_1 - index];
}

#define LENS_FNO(index)		(iris_index_to_fno(index))

static tone_curve_t tone_curve = {
	.tone_curve_red = {/* red */
               0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
              64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
             128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
             193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
             257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
             321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
             385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
             449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
             514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
             578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
             642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
             706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
             770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
             834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
             899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
             963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
	.tone_curve_green = {/* green */
               0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
              64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
             128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
             193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
             257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
             321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
             385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
             449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
             514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
             578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
             642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
             706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
             770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
             834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
             899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
             963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
	.tone_curve_blue = {/* blue */
               0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
              64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
             128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
             193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
             257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
             321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
             385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
             449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
             514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
             578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
             642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
             706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
             770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
             834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
             899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
             963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
};

int get_sensor_aaa_params(int fd, sensor_model_t * sensor)
{
	u32 vin_fps;
	struct amba_vin_source_info vin_info;
	struct amba_vin_aaa_info aaa_info;
	iav_sharpen_filter_cfg_t sharpen_filter;

	if (ioctl(fd, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX, &sharpen_filter) < 0) {
		perror("IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX");
		return -1;
	}

	get_vin_frame_rate(&vin_fps);
	sensor->default_fps = vin_fps;
	sensor->current_fps = vin_fps;

	sensor->sensor_slow_shutter = 0;

	switch (vin_info.sensor_id) {
		case SENSOR_IMX172:
		case SENSOR_IMX178:
		case SENSOR_IMX226:
		case SENSOR_OV4689:
		case SENSOR_MN34220PL:
			if (ioctl(fd, IAV_IOC_VIN_SRC_GET_AAAINFO, &aaa_info) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_AAAINFO error\n");
				return -1;
			}
			sensor->sensor_slow_shutter = aaa_info.slow_shutter_support;
			break;
		default:
			break;
	}

	switch (vin_info.sensor_id) {
		/* Aptina  Sensors */
		case SENSOR_MT9T002:
			sensor->sensor_id = MT_9T002;
			sprintf(sensor->name, "mt9t002");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_AR0331:
			sensor->sensor_id = AR_0331;
			sprintf(sensor->name, "ar0331");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			amba_vin_sensor_op_mode op_mode;
			if (ioctl(fd, IAV_IOC_VIN_GET_OPERATION_MODE, &op_mode) < 0) {
				perror("IAV_IOC_VIN_GET_OPERATION_MODE");
				return -1;
			}

			if(op_mode == AMBA_VIN_LINEAR_MODE) {
				sensor->offset_num[LIST_AWB_PARAM] = AR0331_LINEAR_AWB_PARAM;
				sensor->offset_num[LIST_ADJ_PARAM] = AR0331_LINEAR_ADJ_PARAM;
				sensor->offset_num[LIST_SENSOR_CONFIG] = AR0331_LINEAR_SENSOR_CONFIG;
			} else {
				sensor->offset_num[LIST_AWB_PARAM] = AR0331_AWB_PARAM;
				sensor->offset_num[LIST_ADJ_PARAM] = AR0331_ADJ_PARAM;
				sensor->offset_num[LIST_SENSOR_CONFIG] = AR0331_SENSOR_CONFIG;
			}
			sensor->sensor_op_mode = op_mode;

			break;

		/* Sony Sensors */
		case SENSOR_IMX121:
			sensor->sensor_id = IMX_121;
			sprintf(sensor->name, "imx121");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_IMX172:
			sensor->sensor_id = IMX_172;
			sprintf(sensor->name, "imx172");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			sensor->offset_num[LIST_ADJ_PARAM] = sharpen_filter.sharpen_b_enable ?
				SHARPEN_B_ENABLED_OFFSET : SHARPEN_B_DISABLED_OFFSET;
			break;
		case SENSOR_IMX104:
			sensor->sensor_id = IMX_104;
			sprintf(sensor->name, "imx104");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_IMX136:
			sensor->sensor_id = IMX_136;
			sprintf(sensor->name, "imx136");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_IMX178:
			sensor->sensor_id = IMX_178;
			sprintf(sensor->name, "imx178");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_IMX185:
			sensor->sensor_id = IMX_185;
			sprintf(sensor->name, "imx185");
			break;
		case SENSOR_IMX123:
			sensor->sensor_id = IMX_123;
			sprintf(sensor->name, "imx123");
			if (strcmp(sensor->load_files.lens_file, "") != 0) {
				if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
					sensor->offset_num[LIST_LINES] = PIRIS_SENSOR_60P50HZ_LINES;
				} else {
					sensor->offset_num[LIST_LINES] = PIRIS_SENSOR_50HZ_LINES;
				}
			} else if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_IMX226:
			sensor->sensor_id = IMX_226;
			sprintf(sensor->name, "imx226");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			sensor->offset_num[LIST_ADJ_PARAM] = sharpen_filter.sharpen_b_enable ?
				SHARPEN_B_ENABLED_OFFSET : SHARPEN_B_DISABLED_OFFSET;
			break;

		/* Panasonic Sensors */
		case SENSOR_MN34041PL:
			sensor->sensor_id = MN_34041PL;
			sprintf(sensor->name, "mn34041pl");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_MN34210PL:
			sensor->sensor_id = MN_34210PL;
			sprintf(sensor->name, "mn34210pl");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_MN34220PL:
			sensor->sensor_id = MN_34220PL;
			sprintf(sensor->name, "mn34220pl");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;

		/* OV Sensors */
		case SENSOR_OV2710:
			sensor->sensor_id = OV_2710;
			sprintf(sensor->name, "ov2710");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
		case SENSOR_OV5658:
			sensor->sensor_id = OV_5658;
			sprintf(sensor->name, "ov5658");
			break;
		case SENSOR_OV4689:
			sensor->sensor_id = OV_4689;
			sprintf(sensor->name, "ov4689");
			break;
		default:
			sensor->sensor_id = MN_34041PL;
			sprintf(sensor->name, "mn34041pl");
			if (sensor->current_fps == AMBA_VIDEO_FPS_60) {
				sensor->offset_num[LIST_LINES] = SENSOR_60P50HZ_LINES;
			}
			break;
	}

	if (strcmp(sensor->load_files.adj_file, "") == 0) {
		sprintf(sensor->load_files.adj_file, "%s/%s_adj_param.bin",
			ADJ_PARAM_PATH, sensor->name);
	}

	if (strcmp(sensor->load_files.aeb_file, "") == 0) {
		sprintf(sensor->load_files.aeb_file, "%s/%s_aeb_param.bin",
			ADJ_PARAM_PATH, sensor->name);
	}
	return 0;
}

int load_dsp_cc_table(void)
{
	int file = -1, count;
	char filename[128];
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;
	u8* reg = NULL, *matrix = NULL, *sec_cc = NULL;
	int ret = 0;

	if ((reg = malloc(CC_REG_SIZE)) == NULL) {
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("./reg.bin", O_RDONLY, 0)) < 0) {
			if ((file = open("reg.bin", O_RDONLY, 0)) < 0) {
				MW_ERROR("reg.bin cannot be opened!\n");
				ret = -1;
				goto load_dsp_cc_table_exit;
			}
		}
	}
	if ((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		MW_ERROR("Read reg.bin file error!\n");
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);
	file = -1;

	if ((matrix = malloc(CC_3D_SIZE)) == NULL) {
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if((file = open("./3D.bin", O_RDONLY, 0)) < 0) {
			if ((file = open("3D.bin", O_RDONLY, 0)) < 0) {
				MW_ERROR("3D.bin cannot be opened!\n");
				ret = -1;
				goto load_dsp_cc_table_exit;
			}
		}
	}
	if ((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		MW_ERROR("Read 3D.bin file error!\n");
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);
	file = -1;

	if ((sec_cc = malloc(SEC_CC_SIZE)) == NULL) {
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("/system/ambarella/3D_sec.bin", O_RDONLY, 0)) < 0) {
			if ((file = open("3D_sec.bin", O_RDONLY, 0)) < 0) {
				MW_ERROR("3D_sec.bin cannot be opened!\n");
				ret = -1;
				goto load_dsp_cc_table_exit;
			}
		}
	}
	if ((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		MW_ERROR("Read 3D_sec.bin file error!\n");
		ret = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);
	file = -1;

	memset(&color_corr_reg, 0, sizeof(color_corr_reg));
	color_corr_reg.reg_setting_addr = (u32)reg;
	memset(&color_corr, 0, sizeof(color_corr));
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;

	if ((ret = img_dsp_set_color_correction_reg(&color_corr_reg)) < 0) {
		MW_ERROR("img_dsp_set_color_correction_reg error!\n");
		goto load_dsp_cc_table_exit;
	}
	if ((ret = img_dsp_set_color_correction(fd_iav_aaa, &color_corr)) < 0) {
		MW_ERROR("img_dsp_set_color_correction error!\n");
		goto load_dsp_cc_table_exit;
	}
	if ((ret = img_dsp_set_tone_curve(fd_iav_aaa, &tone_curve)) < 0) {
		MW_ERROR("img_dsp_set_tone_curve error!\n");
		goto load_dsp_cc_table_exit;
	}
	if ((ret = img_dsp_set_sec_cc_en(fd_iav_aaa, 0)) < 0) {
		MW_ERROR("img_dsp_set_sec_cc_en error!\n");
		goto load_dsp_cc_table_exit;
	}
	if ((ret = img_dsp_enable_color_correction(fd_iav_aaa)) < 0) {
		MW_ERROR("img_dsp_enable_color_correction error!\n");
		goto load_dsp_cc_table_exit;
	}

load_dsp_cc_table_exit:
	if (reg != NULL) {
		free(reg);
		reg = NULL;
	}
	if (matrix != NULL) {
		free(matrix);
		matrix = NULL;
	}
	if (sec_cc != NULL) {
		free(sec_cc);
		sec_cc = NULL;
	}
	if (file > 0) {
		close(file);
		file = -1;
	}
	return ret;
}

int mw_get_wb_cal(mw_wb_gain * pGain)
{
	wb_gain_t wb_gain;
	if (pGain == NULL) {
		MW_ERROR("[mw_get_wb_cal] NULL pointer!\n");
		return -1;
	}

	img_awb_get_wb_cal(&wb_gain);

	pGain->r_gain = wb_gain.r_gain;
	pGain->g_gain = wb_gain.g_gain;
	pGain->b_gain = wb_gain.b_gain;
	return 0;
}

int mw_set_custom_gain(mw_wb_gain * pGain)
{
	wb_gain_t wb_gain;

	if (pGain == NULL) {
		MW_ERROR("[mw_set_custom_gain] NULL pointer!\n");
		return -1;
	}

	wb_gain.r_gain = pGain->r_gain;
	wb_gain.g_gain = pGain->g_gain;
	wb_gain.b_gain = pGain->b_gain;

	if (img_awb_set_custom_gain(&wb_gain) < 0) {
		MW_ERROR("img_awb_set_custom_gain!\n");
		return -1;
	}
	return 0;
}

inline int get_vin_mode(u32 *vin_mode)
{
	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, vin_mode) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
		return -1;
	}
	return 0;
}

inline int get_vin_frame_rate(u32 *pFrame_time)
{
	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}
	return 0;
}

static inline int set_vsync_vin_framerate(u32 frame_time)
{
	int rval;

	assert(G_vin_tick > 0);
	read(G_vin_tick, vin_int_array, 8);

	if ((rval = ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_SET_FRAME_RATE, frame_time)) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
		return rval;
	}

	return 0;
}

static iav_change_framerate_factor_ex_t pre_frame_rate[MAX_ENCODE_STREAM_NUM];
static u32 pre_frame_100[MAX_ENCODE_STREAM_NUM];

static int update_vin_frame_rate(u32 curr_frame_time, u32 target_frame_time)
{
	u8 new_numerator;
	int i;
	u32 curr_fps_100, target_fps_100;
	iav_change_framerate_factor_ex_t frame_rate;

	curr_fps_100 = (u64)51200000000 / curr_frame_time;
	target_fps_100 = (u64)51200000000 / target_frame_time;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		memset(&frame_rate, 0, sizeof(frame_rate));
		frame_rate.id = (1 << i);
		if (ioctl(fd_iav_aaa, IAV_IOC_GET_FRAMERATE_FACTOR_EX, &frame_rate) < 0) {
			perror("IAV_IOC_GET_FRAMERATE_FACTOR_EX");
			return -1;
		}

		if (frame_rate.keep_fps_in_ss == 1) {
			if (pre_frame_rate[i].keep_fps_in_ss == 0) {
				pre_frame_rate[i] = frame_rate;
				pre_frame_100[i] = curr_fps_100 *
					frame_rate.ratio_numerator / frame_rate.ratio_denominator;
			}
			if (target_fps_100 <= pre_frame_100[i]) {
				frame_rate.ratio_denominator = 1;
				frame_rate.ratio_numerator = 1;
			} else {
				new_numerator = pre_frame_100[i] * 100 / target_fps_100;
				frame_rate.ratio_denominator = 100;
				frame_rate.ratio_numerator = new_numerator;
			}

			if (ioctl(fd_iav_aaa, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX, &frame_rate) < 0) {
				perror("IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX");
				return -1;
			}
		}
	}
	if (ioctl(fd_iav_aaa, IAV_IOC_UPDATE_VIN_FRAMERATE_EX, 0) < 0) {
		perror("IAV_IOC_UPDATE_VIN_FRAMERATE_EX");
		return -1;
	}
	MW_INFO("[Done] update_vin_frame_rate !\n");
	return 0;
}

int load_bad_pixel_cali_data(char * file)
{
	int fd_bpc, count, rval;
	u8 * bpc_map_addr;
	u32 raw_pitch, bpc_map_size;
	fpn_correction_t fpn;
	dbp_correction_t bad_corr;
	cfa_noise_filter_info_t cfa_noise_filter;

	memset(&fpn, 0, sizeof(fpn));
	memset(&bad_corr, 0, sizeof(bad_corr));
	memset(&cfa_noise_filter, 0, sizeof(cfa_noise_filter));
	img_dsp_set_cfa_noise_filter(fd_iav_aaa, &cfa_noise_filter);
	img_dsp_set_dynamic_bad_pixel_correction(fd_iav_aaa, &bad_corr);
	img_dsp_set_anti_aliasing(fd_iav_aaa, 0);

	fd_bpc = count = -1;
	if ((fd_bpc = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}
	bpc_map_addr = NULL;
	raw_pitch = ROUND_UP(G_vin_width / 8, 32);
	bpc_map_size = raw_pitch * G_vin_height;
	if ((bpc_map_addr = malloc(bpc_map_size)) == NULL) {
		MW_ERROR("CANNOT malloc memory for BPC map!\n");
		return -1;
	}
	memset(bpc_map_addr, 0, bpc_map_size);
	if ((count = read(fd_bpc, bpc_map_addr, bpc_map_size)) != bpc_map_size) {
		MW_ERROR("Read [%s] error!\n", file);
		rval = -1;
		goto free_bpc_map;
	}
	fpn.enable = 3;
	fpn.fpn_pitch = raw_pitch;
	fpn.pixel_map_width = G_vin_width;
	fpn.pixel_map_height = G_vin_height;
	fpn.pixel_map_size = bpc_map_size;
	fpn.pixel_map_addr = (u32)bpc_map_addr;

	rval = img_dsp_set_static_bad_pixel_correction(fd_iav_aaa, &fpn);
	MW_INFO("[DONE] load bad pixel calibration data [%dx%d] from [%s]!\n",
		G_vin_width, G_vin_height, file);

free_bpc_map:
	close(fd_bpc);
	free(bpc_map_addr);
	return rval;
}

static int get_multi_int_arg(char * optarg, int * arr, int count)
{
	int i;
	char * delim = ":\n";
	char * ptr = NULL;

	ptr = strtok(optarg, delim);
	arr[0] = atoi(ptr);

	for (i = 1; i < count; ++i) {
		if ((ptr = strtok(NULL, delim)) == NULL)
			break;
		arr[i] = atoi(ptr);
	}
	return (i < count) ? -1 : 0;
}

static int correct_awb_cali_data(int *correct_param)
{
	wb_gain_t orig[2], target[2];
	int th_r, th_b;
	int low_r, low_b, high_r, high_b;
	int i;

	for (i = 0; i < 2; ++i) {
		orig[i].r_gain = correct_param[i*3];
		orig[i].g_gain = correct_param[i*3+1];
		orig[i].b_gain = correct_param[i*3+2];
		target[i].r_gain = correct_param[i*3+6];
		target[i].g_gain = correct_param[i*3+7];
		target[i].b_gain = correct_param[i*3+8];
	}

	low_r = ABS(target[0].r_gain - orig[0].r_gain);
	high_r = ABS(target[1].r_gain - orig[1].r_gain);
	low_b = ABS(target[0].b_gain - orig[0].b_gain);
	high_b = ABS(target[1].b_gain - orig[1].b_gain);

	th_r = MAX(low_r, high_r);
	th_b = MAX(low_b, high_b);

	if (img_awb_set_cali_diff_thr(th_r, th_b) < 0) {
		MW_ERROR("img_awb_set_cali_diff_thr error!\n");
		return -1;
	}
	if (img_awb_set_wb_shift(orig, target) < 0) {
		MW_ERROR("img_awb_set_wb_shift error!\n");
		return -1;
	}

	return 0;
}

char bin_fn[64][64]= {
	"default_MCTF.bin",
	"S2_cmpr_config.bin",
	"lowiso_MCTF.bin",
	"high_iso/01_mctf_level_based.bin",
	"high_iso/02_mctf_vlf_adapt.bin",
	"high_iso/03_mctf_l4c_edge.bin",
	"high_iso/04_mctf_mix_124x.bin",
	"high_iso/05_mctf_mix_main.bin",
	"high_iso/06_mctf_c4.bin",
	"high_iso/07_mctf_c8.bin",
	"high_iso/08_mctf_cA.bin",
	"high_iso/09_mctf_vvlf_adapt.bin",
	"high_iso/10_sec_cc_l4c_edge.bin",
	"high_iso/11_cc_reg_normal.bin",
	"high_iso/12_cc_3d_normal.bin",
	"high_iso/13_cc_out_normal.bin",
	"high_iso/14_thresh_dark_normal.bin",
	"high_iso/15_thresh_hot_normal.bin",
	"high_iso/16_fir1_normal.bin",
	"high_iso/17_fir2_normal.bin",
	"high_iso/18_coring_normal.bin",
	"high_iso/19_lnl_tone_normal.bin",
	"high_iso/20_chroma_scale_gain_normal.bin",
	"high_iso/21_local_exposure_gain_normal.bin",
	"high_iso/22_cmf_table_normal.bin",
	"high_iso/23_alpha_luma.bin",
	"high_iso/24_alpha_chroma.bin",
	"high_iso/25_cc_reg.bin",
	"high_iso/26_cc_3d.bin",
	"high_iso/27_fir1_spat_dep.bin",
	"high_iso/28_fir2_spat_dep.bin",
	"high_iso/29_coring_spat_dep.bin",
	"high_iso/30_fir1_vlf.bin",
	"high_iso/31_fir2_vlf.bin",
	"high_iso/32_coring_vlf.bin",
	"high_iso/33_fir1_nrl.bin",
	"high_iso/34_fir2_nrl.bin",
	"high_iso/35_coring_nrl.bin",
	"high_iso/36_fir1_124x.bin",
	"high_iso/37_coring_coring4x.bin",
	"high_iso/38_coring_coring2x.bin",
	"high_iso/39_coring_coring1x.bin",
	"high_iso/40_fir1_high_noise.bin",
	"high_iso/41_coring_high_noise.bin",
	"high_iso/42_alpha_unity.bin",
	"high_iso/43_zero.bin",
	"high_iso/46_mctf_1.bin",
	"high_iso/47_mctf_2.bin",
	"high_iso/48_mctf_3.bin",
	"high_iso/49_mctf_4.bin",
	"high_iso/50_mctf_5.bin",
	"high_iso/51_mctf_6.bin",
	"high_iso/52_mctf_7.bin",
	"high_iso/53_mctf_8.bin"
};

int load_mctf_bin(void)
{
	#define SIZE_OF_MCTF_BIN		27600
	int i;
	int file, count;
	u8* bin_buff;
	char filename[256];
	idsp_one_def_bin_t one_bin;
	idsp_def_bin_t bin_map;

	img_dsp_get_default_bin_map(&bin_map);
	bin_buff =(u8 *) malloc(SIZE_OF_MCTF_BIN);
	if (bin_buff == NULL) {
		MW_ERROR(" error:bin_buff is NULL!\n");
		return -1;
	}

	for (i = 0; i < bin_map.num; i++)
	{
		memset(filename, 0, sizeof(filename));
		memset(bin_buff, 0, SIZE_OF_MCTF_BIN);
		sprintf(filename, "%s/%s", IMGPROC_PARAM_PATH, bin_fn[i]);
		if((file = open(filename, O_RDONLY, 0)) < 0) {
			MW_ERROR("%s cannot be opened\n", filename);
			free(bin_buff);
			return -1;
		}
		if((count = read(file, bin_buff, bin_map.one_bin[i].size)) != bin_map.one_bin[i].size) {
			MW_ERROR("read %s error\n", filename);
			free(bin_buff);
			close(file);
			return -1;
		}
		one_bin.size = bin_map.one_bin[i].size;
		one_bin.type = bin_map.one_bin[i].type;
		if (img_dsp_load_default_bin((u32)bin_buff, one_bin) < 0) {
			MW_ERROR("img_dsp_load_default_bin error!\n");
			free(bin_buff);
			close(file);
			return -1;
		}
		close(file);
	}
	free(bin_buff);
	return 0;
}

int load_awb_cali_data(char * file)
{
	#define	TOTAL_PARAMS		(12)
	char content[128];
	int correct_param[TOTAL_PARAMS];
	int fd_wb;

	fd_wb = -1;
	if ((fd_wb = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}
	if (read(fd_wb, content, sizeof(content)) < 0) {
		MW_ERROR("Read [%s] error!\n", file);
		return -1;
	}
	if (get_multi_int_arg(content, correct_param, TOTAL_PARAMS) < 0) {
		MW_ERROR("Invalid white balance calibration data!\n");
		return -1;
	}

	if (correct_awb_cali_data(correct_param) < 0) {
		return -1;
	}
	MW_INFO("[DONE] load white balance calibration data!\n");
	close(fd_wb);
	return 0;
}

int load_lens_shading_cali_data(char *file)
{
	int fd_shading, count, total_count, rval;
	u16 * shading_table;
	vignette_info_t vignette_info;
	static u32 raw_width, raw_height;

	shading_table = NULL;
	total_count = 4 * VIGNETTE_MAX_SIZE * sizeof(u16);
	memset(&vignette_info, 0, sizeof(vignette_info));

	fd_shading = count = -1;
	G_vig_to_awb = 0;

	if ((fd_shading = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}

	if ((shading_table = malloc(total_count)) == NULL) {
		MW_ERROR("CANNOT malloc memory for lens shading table!\n");
		return -1;
	}
	memset(shading_table, 0, total_count);
	if ((count = read(fd_shading, shading_table, total_count)) != total_count) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}
	if ((count = read(fd_shading, &lookup_shift, sizeof(u32))) != sizeof(u32)) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}

	if((count = read(fd_shading,&raw_width,sizeof(raw_width))) != sizeof(raw_width)){
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}
	if((count = read(fd_shading,&raw_height,sizeof(raw_height))) != sizeof(raw_height)){
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}

	vignette_info.enable = 1;
	vignette_info.gain_shift = (u8)lookup_shift;
	vignette_info.vignette_red_gain_addr = (u32)
		(shading_table + 0 * VIGNETTE_MAX_SIZE);
	vignette_info.vignette_green_even_gain_addr = (u32)
		(shading_table + 1 * VIGNETTE_MAX_SIZE);
	vignette_info.vignette_green_odd_gain_addr = (u32)
		(shading_table + 2 * VIGNETTE_MAX_SIZE);
	vignette_info.vignette_blue_gain_addr = (u32)
		(shading_table + 3 * VIGNETTE_MAX_SIZE);

	rval = img_dsp_set_vignette_compensation(fd_iav_aaa, &vignette_info);
	MW_INFO("[DONE] load lens shading calibration data!\n");

	G_vig_to_awb = 1;

free_shading_table:
	close(fd_shading);
	free(shading_table);
	return rval;
}


int load_calibration_data(mw_cali_file * file, MW_CALIBRATION_TYPE type)
{
	int rval = 0;
	char file_name[256];
	switch (type) {
	case MW_CALIB_BPC:
		if (strlen(file->bad_pixel) > 0) {
			if (load_bad_pixel_cali_data(file->bad_pixel) < 0) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_bad_pixel.bin", CALI_FILES_PATH,
				G_vin_width, G_vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_bad_pixel_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	case MW_CALIB_WB:
		if (strlen(file->wb) > 0) {
			if (load_awb_cali_data(file->wb) < 0) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_awb.bin", CALI_FILES_PATH,
				G_vin_width, G_vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_awb_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	case MW_CALIB_LENS_SHADING:
		if (strlen(file->shading) > 0) {
			if (load_lens_shading_cali_data(file->shading)) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_lens_shading.bin", CALI_FILES_PATH,
				G_vin_width, G_vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_lens_shading_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	case MW_CONVERT_VIG_TO_AWB:
		break;
	default:
		rval = -1;
		break;
	}

	return rval;
}

static int load_default_image_param(void)
{
	image_property_t image_prop;

	// Get image parameters from imgproc lib
	img_get_img_property(&image_prop);
	G_mw_image.saturation = image_prop.saturation;
	G_mw_image.brightness = image_prop.brightness;
	G_mw_image.hue = image_prop.hue;
	G_mw_image.contrast = image_prop.contrast;

	// Get basic parameters from imgproc lib
	G_mw_ae_param.current_vin_fps = G_mw_sensor_model.current_fps;
	G_mw_ae_param.shutter_time_max = G_mw_sensor_model.default_fps;

	G_mctf_strength = image_prop.mctf;
	G_auto_local_exposure_mode = image_prop.local_exp ? MW_LE_AUTO : MW_LE_STOP;
	G_exposure_level[0] = (img_ae_get_target_ratio() * 100) >> 10;

	G_mw_awb.wb_mode = (u32)img_awb_get_mode();

	MW_INFO("   saturation = %d.\n", G_mw_image.saturation);
	MW_INFO("   brightness = %d.\n", G_mw_image.brightness);
	MW_INFO("          hue = %d.\n", G_mw_image.hue);
	MW_INFO("     contrast = %d.\n", G_mw_image.contrast);
	MW_INFO("    sharpness = %d.\n", G_mw_image.sharpness);

	return 0;
}

int reload_previous_params(void)
{
	//restores AE line
	if (G_mw_ae_param.slow_shutter_enable) {
		create_slowshutter_task();
		G_mw_sensor_model.current_fps =G_mw_ae_param.current_vin_fps;
	}
	load_ae_exp_lines(&G_mw_ae_param);

	//restore mctf strength
	if (img_set_mctf_strength(G_mctf_strength) < 0) {
		MW_ERROR("img_set_mctf_strength error!\n");
		return -1;
	}
	//restore AE exposure level
	if (img_ae_set_target_ratio((G_exposure_level[0] << 10) / 100) < 0) {
		MW_ERROR("img_ae_set_target_ratio error\n");
		return -1;
	}

	//restores Awb
	if (G_mw_awb.wb_mode == MW_WB_MODE_HOLD) {
		if (img_enable_awb(0) < 0) {
			MW_ERROR("img_enable_awb error\n");
			return -1;
		}
	} else {
		if (img_enable_awb(1) < 0) {
			MW_ERROR("img_enable_awb error\n");
			return -1;
		}
		if (img_awb_set_mode(G_mw_awb.wb_mode) < 0) {
			MW_ERROR("img_awb_set_mode error\n");
			return -1;
		}
	}

	// restore image parameters
	if (img_set_color_saturation(G_mw_image.saturation) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	if (img_set_color_brightness(G_mw_image.brightness) < 0) {
		MW_ERROR("img_set_color_brightness error!\n");
		return -1;
	}

	if (img_set_color_hue(G_mw_image.hue) < 0) {
		MW_ERROR("img_set_color_hue error!\n");
		return -1;
	}

	if (img_set_color_contrast(G_mw_image.contrast) < 0) {
		MW_ERROR("img_set_color_contrast error!\n");
		return -1;
		}

	if (img_set_sharpness(G_mw_image.sharpness) < 0) {
		MW_ERROR("img_set_sharpness error!\n");
		return -1;
	}

	return 0;
}

static int load_default_af_param(void)
{
	memset(&G_mw_af, 0, sizeof(G_mw_af));

	G_mw_af.lens_type = LENS_CMOUNT_ID;
	G_mw_af.zm_idx = 1;
	G_mw_af.fs_idx = 1;
	G_mw_af.fs_near = 1;
	G_mw_af.fs_far = 1;

	return 0;
}

static inline int print_ae_lines(line_t *lines,
	int line_num, int line_belt, int enable_convert)
{
	int i;
	MW_INFO("===== [MW] ==== Automatic Generates AE lines =====\n");
	for (i = 0; i < line_num; ++i) {
		if (i == line_belt)
			MW_INFO("===== [MW] ====== This is the line belt. =========\n");
		if (enable_convert) {
			if ((lines[i].start.factor[MW_IRIS] != APERTURE_AUTO) ||
				(lines[i].end.factor[MW_IRIS] != APERTURE_AUTO)) {
				MW_INFO(" [%d] start (1/%d, %d, %3.2f) == end (1/%d, %d, %3.2f)\n",
					(i + 1), SHT_TIME(lines[i].start.factor[MW_SHUTTER]),
					lines[i].start.factor[MW_DGAIN],
					(float)LENS_FNO(lines[i].start.factor[MW_IRIS]) /
					LENS_FNO_UNIT, SHT_TIME(lines[i].end.factor[MW_SHUTTER]),
					lines[i].end.factor[MW_DGAIN],
					(float)LENS_FNO(lines[i].end.factor[MW_IRIS]) / LENS_FNO_UNIT);
			} else {
				MW_INFO(" [%d] start (1/%d, %d, %d) == end (1/%d, %d, %d)\n",
					(i + 1), SHT_TIME(lines[i].start.factor[MW_SHUTTER]),
					lines[i].start.factor[MW_DGAIN],
					lines[i].start.factor[MW_IRIS],
					SHT_TIME(lines[i].end.factor[MW_SHUTTER]),
					lines[i].end.factor[MW_DGAIN],
					lines[i].end.factor[MW_IRIS]);
			}
		} else {
			MW_INFO(" [%d] start (%d, %d, %d) == end (%d, %d, %d)\n", (i + 1),
				lines[i].start.factor[MW_SHUTTER],
				lines[i].start.factor[MW_DGAIN], lines[i].start.factor[MW_IRIS],
				lines[i].end.factor[MW_SHUTTER],
				lines[i].end.factor[MW_DGAIN], lines[i].end.factor[MW_IRIS]);
		}
	}
	MW_INFO("======= [MW] ==== NUM [%d] ==== BELT [%d] ==========\n\n",
		line_num, line_belt);
	return 0;
}

static int generate_normal_ae_lines(mw_ae_param *p, line_t *lines, int *line_num, int *line_belt)
{
	int dst, src, i;
	int s_max, s_min, g_max, g_min, p_index_max, p_index_min;
	line_t *p_src = NULL;
	int flicker_off_shutter_time = 0;
	int longest_possible_shutter = 0, curr_belt = 0;

	amba_vin_agc_info_t vin_agc_info;

	s_max = p->shutter_time_max;
	s_min = p->shutter_time_min;
	g_max = p->sensor_gain_max;
	if ((p->lens_aperture.aperture_min == APERTURE_AUTO) ||
		(p->lens_aperture.aperture_max == APERTURE_AUTO)) {
		p_index_max = 0;
		p_index_min  = 0;
	} else {
		p_index_max = iris_to_index(p->lens_aperture.aperture_min);
		p_index_min =iris_to_index(p->lens_aperture.aperture_max);
	}
	p_src = (p->anti_flicker_mode == 1) ? G_60HZ_LINES : G_50HZ_LINES;
	flicker_off_shutter_time = (p->anti_flicker_mode == 1) ?
		SHUTTER_1BY120_SEC : SHUTTER_1BY100_SEC;
	dst = src = curr_belt = 0;

	// create line 1 - shutter line / digital gain line
	if (s_min < flicker_off_shutter_time) {
		// create shutter line
		lines[dst] = p_src[src];
		lines[dst].start.factor[MW_SHUTTER] = s_min;
		if (s_max < flicker_off_shutter_time) {
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			++dst;
			lines[dst].start.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_DGAIN] = g_max;
			++dst;
			curr_belt = dst;
			goto GENERATE_LINES_EXIT;
		}
		++dst;
		++src;
	} else {
		// create digital gain line
		while (s_min > p_src[src].start.factor[MW_SHUTTER])
			++src;
		lines[dst] = p_src[src];
		++dst;
		++src;
	}

	// create other lines - digital gain line
	while (s_max >= p_src[src].start.factor[MW_SHUTTER]) {
		if (p_src[src].start.factor[MW_SHUTTER] == SHUTTER_INVALID)
			break;
		lines[dst] = p_src[src];
		++dst;
		++src;
		if (src >= MAX_AE_LINE_NUM) {
			MW_ERROR("Fatal error: AE line number exceeds MAX_AE_LINE_NUM %d",
					MAX_AE_LINE_NUM);
			return -1;
		}
	}
	lines[dst - 1].end.factor[MW_DGAIN] = g_max;

	// change min gain from sensor driver
	memset(&vin_agc_info, 0, sizeof(amba_vin_agc_info_t));
	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_AGC_INFO, &vin_agc_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO");
		return 0;
	}
	g_min = (vin_agc_info.db_min >> 24);

	i = 0;
	while (G_gain_table[i] < g_min) {
		if (++i == GAIN_TABLE_NUM) {
			--i;
			break;
		}
	}
	g_min = G_gain_table[i];

	MW_INFO("---Min Gain: %d, Max Gain;%d\n", g_min, g_max);
	i = 0;
	while (i < dst) {
		if (lines[i].start.factor[MW_SHUTTER] != lines[i].end.factor[MW_SHUTTER]) {
			/* Shutter line: change min gain both for start and end points */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
			if (lines[i].end.factor[MW_DGAIN] < g_min) {
				lines[i].end.factor[MW_DGAIN] = g_min;
			}
		} else {
			/* Gain line: change min gain for start point */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
			if (lines[i].end.factor[MW_DGAIN] < g_min) {
				lines[i].end.factor[MW_DGAIN] = g_min;
			}
		}
		++i;
	}

	//change the min/max piris lens size
	if (p_index_min < lines[0].start.factor[MW_IRIS]) {
		 lines[0].start.factor[MW_IRIS] = p_index_min;
	}
	if (p_index_max > lines[dst].end.factor[MW_IRIS]) {
		 lines[dst].end.factor[MW_IRIS] = p_index_max;
	}
	i = 0;
	while (i < dst) {
		if (p_index_min > lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_min > lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_max < lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_max;
		}
		if (p_index_max < lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_max;
		}
		++i;
	}

	// calculate line belt of current and default
	curr_belt = dst;
	while (1) {
		longest_possible_shutter = lines[curr_belt - 1].end.factor[MW_SHUTTER];
		if (longest_possible_shutter <= G_mw_sensor_model.default_fps) {
			G_line_belt_default = curr_belt;
		}
		if (p->slow_shutter_enable) {
			if (longest_possible_shutter <= G_mw_sensor_model.current_fps) {
				MW_INFO("\tSlow shutter is enabled, set curr_belt [%d] to"
					" current frame rate [%d].\n",
					curr_belt, G_mw_sensor_model.current_fps);
				break;
			}
		} else {
			if (longest_possible_shutter <= G_mw_sensor_model.default_fps) {
				lines[curr_belt - 1].end.factor[MW_DGAIN] = g_max;
				MW_INFO("\tSlow shutter is disabled, restore curr_belt [%d] to"
					" default frame rate [%d].\n",
					curr_belt, G_mw_sensor_model.default_fps);
				break;
			}
		}
		--curr_belt;
		MW_INFO("\t\t\t=== curr_belt [%d] def_belt [%d] == VIN [%d fps] == \n",
			curr_belt, G_line_belt_default,
			SHT_TIME(G_mw_sensor_model.current_fps));
	}

GENERATE_LINES_EXIT:
	*line_num = dst;
	*line_belt = curr_belt;

	print_ae_lines(lines, dst, curr_belt, 1);
	return 0;
}

static inline int generate_manual_shutter_lines(mw_ae_param *p,
	line_t *lines, int *line_num, int *line_belt)
{
	int total_lines = 0;
	lines[total_lines].start.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].start.factor[MW_DGAIN] = 0;
	lines[total_lines].start.factor[MW_IRIS] = 0;
	lines[total_lines].end.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].end.factor[MW_DGAIN] = p->sensor_gain_max;
	lines[total_lines].end.factor[MW_IRIS] = 0;
	++total_lines;
	*line_num = total_lines;
	*line_belt = total_lines;

	print_ae_lines(lines, total_lines, total_lines, 1);
	return 0;
}

#if 0	// for manual gain lines
static inline int generate_manual_gain_lines(mw_ae_param *p,
	line_t *lines, int *line_num, int *line_belt)
{
	int total_lines = 0;
	lines[total_lines].start.factor[MW_SHUTTER] = p->shutter_time_min;
	lines[total_lines].start.factor[MW_DGAIN] = ISO_200;
	lines[total_lines].start.factor[MW_IRIS] = 0;
	lines[total_lines].end.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].end.factor[MW_DGAIN] = ISO_200;
	lines[total_lines].end.factor[MW_IRIS] = 0;
	++total_lines;
	*line_num = total_lines;
	*line_belt = total_lines;

	print_ae_lines(lines, total_lines, total_lines, 1);
	return 0;
}
#endif

static int generate_ae_lines(mw_ae_param *p,
	line_t *lines, int *line_num, int *line_belt)
{
	int retv = 0;

	if (p->shutter_time_max != p->shutter_time_min)
		retv = generate_normal_ae_lines(p, lines, line_num, line_belt);
	else
		retv = generate_manual_shutter_lines(p, lines, line_num, line_belt);

	return retv;
}

static int load_ae_lines(line_t* line, u16 line_num, u16 line_belt)
{
	int i;
	static line_t img_ae_lines[MAX_AE_LINE_NUM];

	memcpy(&img_ae_lines[0], line, sizeof(line_t) * line_num);
	//transfer q9 format to shutter index format
	for (i = 0; i < line_num; i++) {
		img_ae_lines[i].start.factor[MW_SHUTTER]
			= shutter_q9_to_index(line[i].start.factor[MW_SHUTTER]);
		img_ae_lines[i].end.factor[MW_SHUTTER]
			= shutter_q9_to_index(line[i].end.factor[MW_SHUTTER]);
	}

	MW_INFO("=== Convert shutter time to shutter index ===\n");
	print_ae_lines(img_ae_lines, line_num, line_belt, 0);
	if (img_ae_load_exp_line(img_ae_lines, line_num, line_belt) < 0) {
		MW_MSG("[img_ae_load_exp_line error] : line_num [%d] line_belt [%d].\n",
			line_num, line_belt);
		return -1;
	}

	return 0;
}


/**********************************************************************
 *      External functions
 *********************************************************************/

inline int is_rgb_sensor_vin(void)
{
	struct amba_vin_source_info src_info;
	struct amba_video_info video_info;

	memset(&src_info, 0, sizeof(src_info));
	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO");
		return 0;
	}

	memset(&video_info, 0, sizeof(video_info));
	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return 0;
	}
	G_vin_width = video_info.width;
	G_vin_height = video_info.height;

	return ((src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) &&
		((video_info.type == AMBA_VIDEO_TYPE_RGB_601) ||
		(video_info.type == AMBA_VIDEO_TYPE_RGB_RAW)));
}

inline int check_state(void)
{
	iav_state_info_t info;

	memset(&info, 0, sizeof(info));
	if (ioctl(fd_iav_aaa, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	if (info.state == IAV_STATE_IDLE || info.state == IAV_STATE_INIT)
		return -1;

	return 0;
}

int config_sensor_lens_info(sensor_model_t * sensor)
{
	if (get_sensor_aaa_params(fd_iav_aaa, sensor) < 0) {
		MW_ERROR("get_sensor_aaa_params error\n");
		return -1;
	}
	if (img_config_lens_info(G_mw_sensor_model.lens_id) < 0) {
		MW_ERROR("img_config_lens_info error!\n");
		return -1;
	}

	return 0;
}

int get_hdr_sensor_adj_aeb_param(image_sensor_param_t* info,
	hdr_sensor_param_t* hdr_info, sensor_model_t * sensor,
	hdr_sensor_cfg_t *p_sensor_config)
{
	u32 vin_fps, vin_mode;
	struct amba_vin_source_info vin_info;
	struct amba_vin_aaa_info aaa_info;

	if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	get_vin_mode(&vin_mode);
	get_vin_frame_rate(&vin_fps);
	sensor->default_fps = vin_fps;
	sensor->current_fps = vin_fps;

	sensor->sensor_slow_shutter = 0;

	switch (vin_info.sensor_id) {
		case SENSOR_OV4689:
		case SENSOR_MN34220PL:
			if (ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_AAAINFO, &aaa_info) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_AAAINFO error\n");
				return -1;
			}
			sensor->sensor_slow_shutter = aaa_info.slow_shutter_support;
			break;
		default:
			break;
	}

	memset(info, 0, sizeof(image_sensor_param_t));
	memset(hdr_info, 0, sizeof(hdr_sensor_param_t));
	memset(p_sensor_config, 0, sizeof(hdr_sensor_cfg_t));

	switch (vin_info.sensor_id) {
	case SENSOR_MN34210PL:
		sensor->sensor_id = MN_34210PL;
		sprintf(sensor->name, "mn34210pl");
		memcpy(p_sensor_config, &mn34210pl_sensor_config, sizeof(hdr_sensor_cfg_t));

		info->p_adj_param = &mn34210pl_adj_param;
		info->p_rgb2yuv = mn34210pl_rgb2yuv;
		info->p_chroma_scale = &mn34210pl_chroma_scale;
		info->p_awb_param = &mn34210pl_awb_param;
		info->p_50hz_lines = NULL;
		info->p_60hz_lines = NULL;
		info->p_tile_config = &mn34210pl_tile_config;
		info->p_ae_agc_dgain = mn34210pl_ae_agc_dgain;
		info->p_ae_sht_dgain = mn34210pl_ae_sht_dgain;
		info->p_dlight_range = mn34210pl_dlight;
		info->p_manual_LE = mn34210pl_manual_LE;
		if(G_img_config_info.expo_num == 3){
			hdr_info->p_ae_target = mn34210pl_multifrm_ae_target_3x;
			hdr_info->p_sht_lines = (line_t**)&mn34210pl_multifrm_sht_lines_3x[0];
		}else if(G_img_config_info.expo_num == 2){
			hdr_info->p_ae_target = mn34210pl_multifrm_ae_target_2x;
			hdr_info->p_sht_lines = (line_t**)&mn34210pl_multifrm_sht_lines_2x[0];
		}
		break;
	case SENSOR_MN34220PL:
		sensor->sensor_id = MN_34220PL;
		sprintf(sensor->name, "mn34220pl");
		memcpy(p_sensor_config, &mn34220pl_sensor_config, sizeof(hdr_sensor_cfg_t));
		info->p_adj_param = &mn34220pl_adj_param;
		info->p_rgb2yuv = mn34220pl_rgb2yuv;
		info->p_chroma_scale = &mn34220pl_chroma_scale;
		info->p_awb_param = &mn34220pl_awb_param;
		info->p_50hz_lines = NULL;
		info->p_60hz_lines = NULL;
		info->p_tile_config = &mn34220pl_tile_config;
		info->p_ae_agc_dgain = mn34220pl_ae_agc_dgain;
		info->p_ae_sht_dgain = mn34220pl_ae_sht_dgain;
		info->p_dlight_range = mn34220pl_dlight;
		info->p_manual_LE = mn34220pl_manual_LE;
		if(G_img_config_info.expo_num == 3){
			hdr_info->p_ae_target = mn34220pl_multifrm_ae_target_3x;
			hdr_info->p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_3x[0];
		}else if(G_img_config_info.expo_num == 2){
			hdr_info->p_ae_target = mn34220pl_multifrm_ae_target_2x;
			hdr_info->p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_2x[0];
		}
		break;
	case SENSOR_OV4689:
		sensor->sensor_id = OV_4689;
		sprintf(sensor->name, "ov4689");
		memcpy(p_sensor_config, &ov4689_sensor_config, sizeof(hdr_sensor_cfg_t));
		info->p_adj_param = &ov4689_adj_param;
		info->p_rgb2yuv = ov4689_rgb2yuv;
		info->p_chroma_scale = &ov4689_chroma_scale;
		info->p_awb_param = &ov4689_awb_param;
		info->p_50hz_lines = NULL;
		info->p_60hz_lines = NULL;
		info->p_tile_config = &ov4689_tile_config;
		info->p_ae_agc_dgain = ov4689_ae_agc_dgain;
		info->p_ae_sht_dgain = ov4689_ae_sht_dgain;
		info->p_dlight_range = ov4689_dlight;
		info->p_manual_LE = ov4689_manual_LE;
		if(G_img_config_info.expo_num == 3){
			hdr_info->p_ae_target = ov4689_multifrm_ae_target_3x;
			hdr_info->p_sht_lines = (line_t**)&ov4689_multifrm_sht_lines_3x[0];
		}else if(G_img_config_info.expo_num == 2){
			hdr_info->p_ae_target = ov4689_multifrm_ae_target_2x;
			hdr_info->p_sht_lines = (line_t**)&ov4689_multifrm_sht_lines_2x[0];
		}
		break;
	case SENSOR_IMX123:
		sensor->sensor_id = IMX_123;
		sprintf(sensor->name, "imx123");
		memcpy(p_sensor_config, &imx123_sensor_config, sizeof(hdr_sensor_cfg_t));
		info->p_adj_param = &imx123_adj_param;
		info->p_rgb2yuv = imx123_rgb2yuv;
		info->p_chroma_scale = &imx123_chroma_scale;
		info->p_awb_param = &imx123_awb_param;
		info->p_50hz_lines = NULL;
		info->p_60hz_lines = NULL;
		info->p_tile_config = &imx123_tile_config;
		info->p_ae_agc_dgain = imx123_ae_agc_dgain;
		info->p_ae_sht_dgain = imx123_ae_sht_dgain;
		info->p_dlight_range = imx123_dlight;
		info->p_manual_LE = imx123_manual_LE;
		if(G_img_config_info.expo_num == 3){
			hdr_info->p_ae_target = imx123_multifrm_ae_target_3x;
			hdr_info->p_sht_lines = (line_t**)&imx123_multifrm_sht_lines_3x[0];
		}else if(G_img_config_info.expo_num == 2){
			hdr_info->p_ae_target = imx123_multifrm_ae_target_2x;
			hdr_info->p_sht_lines = (line_t**)&imx123_multifrm_sht_lines_2x[0];
		}
		break;
	case SENSOR_IMX224:
		sensor->sensor_id = IMX_224;
		sprintf(sensor->name, "imx224");
		memcpy(p_sensor_config, &imx224_sensor_config, sizeof(hdr_sensor_cfg_t));
		info->p_adj_param = &imx224_adj_param;
		info->p_rgb2yuv = imx224_rgb2yuv;
		info->p_chroma_scale = &imx224_chroma_scale;
		info->p_awb_param = &imx224_awb_param;
		info->p_50hz_lines = NULL;
		info->p_60hz_lines = NULL;
		info->p_tile_config = &imx224_tile_config;
		info->p_ae_agc_dgain = imx224_ae_agc_dgain;
		info->p_ae_sht_dgain = imx224_ae_sht_dgain;
		info->p_dlight_range = imx224_dlight;
		info->p_manual_LE = imx224_manual_LE;
		if(G_img_config_info.expo_num == 3) {
			hdr_info->p_ae_target = imx224_multifrm_ae_target_3x;
			hdr_info->p_sht_lines = (line_t**)&imx224_multifrm_sht_lines_3x[0];
		}else if (G_img_config_info.expo_num == 2) {
			hdr_info->p_ae_target = imx224_multifrm_ae_target_2x;
			hdr_info->p_sht_lines = (line_t**)&imx224_multifrm_sht_lines_2x[0];
		}
		break;
	default:
		printf("Not supported sensor Id [%d].\n", vin_info.sensor_id);
		return -1;
	}
	return 0;
}

int load_hdr_param_load_adj_aeb(void)
{
	image_sensor_param_t sensor_app_param;
	hdr_sensor_param_t hdr_app_param;
	hdr_sensor_cfg_t hdr_sensor_cfg;

	get_hdr_sensor_adj_aeb_param(&sensor_app_param, &hdr_app_param,
		&G_mw_sensor_model, &hdr_sensor_cfg);

	if (img_hdr_config_sensor_info(&hdr_sensor_cfg) < 0) {
		MW_ERROR("img_hdr_config_sensor_info error!\n");
		return -1;
	}

	G_sensor_param.step = hdr_sensor_cfg.step;
	G_sensor_param.max_g_db = hdr_sensor_cfg.max_g_db;
	G_sensor_param.max_ag_db = hdr_sensor_cfg.max_ag_db;
	G_sensor_param.max_dg_db = hdr_sensor_cfg.max_dg_db;

	if (img_hdr_load_sensor_param(&sensor_app_param, &hdr_app_param) < 0) {
		MW_ERROR("img_hdr_param_load_adj_aeb error!\n");
		return -1;
	}

	return 0;
}

int load_adj_cc_table(sensor_model_t *sensor)
{
	int file = -1, count;
	char filename[128];
	u8 *matrix = NULL;
	u8 i, adj_mode = 4;
	int ret = 0;
	hdr_cntl_fp_t cus_cntl_fp = {0};

	if ((matrix = malloc(CC_3D_SIZE)) == NULL) {
		ret = -1;
		goto load_adj_cc_table_exit;
	}
	for (i = 0; i < adj_mode; ++i) {
		if ((sensor->sensor_id == AR_0331) &&
			(sensor->sensor_op_mode == AMBA_VIN_LINEAR_MODE)) {
			sprintf(filename, "%s/sensors/%s_linear_0%d_3D.bin",
				IMGPROC_PARAM_PATH, sensor->name, (i+1));

		} else {
			sprintf(filename, "%s/sensors/%s_0%d_3D.bin",
				IMGPROC_PARAM_PATH, sensor->name, (i+1));
		}
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			MW_ERROR("%s cannot be opened!\n", filename);
			ret = -1;
			goto load_adj_cc_table_exit;
		}
		if ((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			MW_MSG("read %s error \n", filename);
			ret = -1;
			goto load_adj_cc_table_exit;
		}
		close(file);
		file = -1;

		if ((ret = img_adj_load_cc_table((u32)matrix, i)) < 0) {
			MW_ERROR("img_adj_load_cc_table error!\n");
			goto load_adj_cc_table_exit;
		}
	}

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if ((ret = load_hdr_param_load_adj_aeb()) < 0) {
			MW_ERROR("load_hdr_param_load_adj_aeb error!\n");
			goto load_adj_cc_table_exit;
		}
		memset(&cus_cntl_fp, 0, sizeof(hdr_cntl_fp_t));
		for(i = 0; i < G_img_config_info.expo_num; ++i){
			if((ret = img_hdr_register_aaa_algorithm(&cus_cntl_fp, i)) < 0){
				printf("error : img_hdr_register_aaa_algorithm\n");
				goto load_adj_cc_table_exit;
			}
		}
	} else {
		if ((ret = get_sensor_aaa_params_from_bin(sensor)) < 0) {
			MW_ERROR("get_sensor_aaa_params_from_bin error!\n");
			goto load_adj_cc_table_exit;
		}
	}


load_adj_cc_table_exit:
	if (matrix != NULL) {
		free(matrix);
		matrix = NULL;
	}
	if (file > 0) {
		close(file);
		file = -1;
	}
	return ret;
}

inline int get_shutter_time(u32 *pShutter_time)
{
	int rval;
	if ((rval = ioctl(fd_iav_aaa, IAV_IOC_VIN_SRC_GET_SHUTTER_TIME, pShutter_time)) < 0) 	{
		perror("IAV_IOC_VIN_SRC_GET_SHUTTER_TIME");
		return rval;
	}
	return 0;
}

int load_ae_exp_lines(mw_ae_param *ae)
{
	memset(G_mw_ae_lines, 0, sizeof(G_mw_ae_lines));

	if (ae->shutter_time_max < ae->shutter_time_min) {
		MW_INFO("shutter limit max [%d] is less than shutter min [%d]. Tie them to shutter min\n",
			ae->shutter_time_max, ae->shutter_time_min);
		ae->shutter_time_max = ae->shutter_time_min;
	}

	if (generate_ae_lines(ae, G_mw_ae_lines,
		&G_line_num, &G_line_belt_current) < 0) {
		MW_ERROR("generate_ae_lines error\n");
		return -1;
	}

	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}

	return 0;
}

int get_ae_exposure_lines(mw_ae_line * lines, u32 num)
{
	int i;

	for (i = 0; i < num && i < G_line_num; ++i) {
		lines[i].start.factor[MW_SHUTTER] = G_mw_ae_lines[i].start.factor[MW_SHUTTER];
		lines[i].start.factor[MW_DGAIN] = G_mw_ae_lines[i].start.factor[MW_DGAIN];
		lines[i].start.factor[MW_IRIS] = G_mw_ae_lines[i].start.factor[MW_IRIS];
		lines[i].end.factor[MW_SHUTTER] = G_mw_ae_lines[i].end.factor[MW_SHUTTER];
		lines[i].end.factor[MW_DGAIN] = G_mw_ae_lines[i].end.factor[MW_DGAIN];
		lines[i].end.factor[MW_IRIS] = G_mw_ae_lines[i].end.factor[MW_IRIS];
	}
	print_ae_lines(G_mw_ae_lines, i, G_line_belt_current, 1);

	return 0;
}

int set_ae_exposure_lines(mw_ae_line * lines, u32 num)
{
	int i, curr_belt;

	for (i = 0; i < num - 1; ++i) {
		if (lines[i+1].start.factor[MW_SHUTTER] == SHUTTER_INVALID)
			break;
		if (lines[i].start.factor[MW_SHUTTER] > lines[i+1].start.factor[MW_SHUTTER]) {
			MW_MSG("AE line [%d] shutter time [%d] must be larger than line [%d] shutter time [%d].\n",
				i+1, lines[i+1].start.factor[MW_SHUTTER], i, lines[i].start.factor[MW_SHUTTER]);
			return -1;
		}
	}
	memset(G_mw_ae_lines, 0, sizeof(G_mw_ae_lines));
	for (i = 0, curr_belt = 0; i < num; i++) {
		if (lines[i].start.factor[MW_SHUTTER] == SHUTTER_INVALID)
			break;
		G_mw_ae_lines[i].start.factor[MW_SHUTTER] = lines[i].start.factor[MW_SHUTTER];
		G_mw_ae_lines[i].start.factor[MW_DGAIN] = lines[i].start.factor[MW_DGAIN];
		G_mw_ae_lines[i].start.factor[MW_IRIS] = lines[i].start.factor[MW_IRIS];
		G_mw_ae_lines[i].end.factor[MW_SHUTTER] = lines[i].end.factor[MW_SHUTTER];
		G_mw_ae_lines[i].end.factor[MW_DGAIN] = lines[i].end.factor[MW_DGAIN];
		G_mw_ae_lines[i].end.factor[MW_IRIS] = lines[i].end.factor[MW_IRIS];
		MW_DEBUG("AE line [%d], shutter [%d], vin [%d]\n",
			i, lines[i].end.factor[MW_SHUTTER], G_mw_sensor_model.current_fps);
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_sensor_model.current_fps) {
			curr_belt = i + 1;
		}
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_sensor_model.default_fps) {
			G_line_belt_default = i + 1;
		}
	}
	G_line_num = i;
	G_line_belt_current = curr_belt;

	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}

	return 0;
}

int set_ae_switch_points(mw_ae_point * points, u32 num)
{
	int i, j;
	joint_t * switch_point = NULL;

	for (i = 0; i < num; ++i) {
		for (j = 0; j < G_line_num; j++) {
			if (points[i].factor[MW_SHUTTER] == SHUTTER_INVALID)
				continue;
			if (G_mw_ae_lines[j].start.factor[MW_SHUTTER] == points[i].factor[MW_SHUTTER])
				break;
		}
		if (j == G_line_num) {
			MW_MSG("Invalid switch point [%d, %d].\n",
				points[i].factor[MW_SHUTTER], points[i].factor[MW_DGAIN]);
			continue;
		}
		switch_point = (points[i].pos == MW_AE_END_POINT) ? &G_mw_ae_lines[j].end
			: &G_mw_ae_lines[j].start;
		switch_point->factor[MW_DGAIN] = points[i].factor[MW_DGAIN];
	}

	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}

	return 0;
}

int set_day_night_mode(u32 dn_mode)
{
	if (img_set_bw_mode(dn_mode) < 0) {
		MW_ERROR("img_set_bw_mode error.\n");
		return -1;
	}
	return 0;
}

void slow_shutter_task(void *arg)
{
	#define	HISTORY_LENGTH		(1)
	static int transition_counter = 0;
	u32 curr_frame_time = G_mw_sensor_model.default_fps;
	u32 default_frame_time = 0;
	u32 target_frame_time = 0;
	int ae_cursor;

	if (G_vin_tick <= 0) {
		G_vin_tick = open(VIN0_VSYNC, O_RDONLY);
		if (G_vin_tick < 0) {
			MW_MSG("CANNOT OPEN [%s].\n", VIN0_VSYNC);
			return;
		}
	}

	while (!slow_shutter_task_exit_flag) {
		read(G_vin_tick, vin_int_array, 8);
		ae_cursor = img_ae_get_cursor();
		default_frame_time = G_mw_sensor_model.default_fps;
		if (G_mw_sensor_model.sensor_slow_shutter == 0) {
			get_vin_frame_rate(&curr_frame_time);
		}
		G_line_belt_current = MAX(ae_cursor, G_line_belt_default);
		G_line_belt_current = MIN(G_line_belt_current, G_line_num);

		target_frame_time = G_mw_ae_lines[G_line_belt_current-1].start.factor[0];
		if (target_frame_time < default_frame_time)
			target_frame_time = default_frame_time;

		if (target_frame_time != curr_frame_time) {
			++transition_counter;
			if (transition_counter == HISTORY_LENGTH) {
				if (G_mw_sensor_model.sensor_slow_shutter == 0) {
					set_vsync_vin_framerate(target_frame_time);
					update_vin_frame_rate(curr_frame_time, target_frame_time);
				} else {
					curr_frame_time = target_frame_time;
				}
				img_ae_set_belt(G_line_belt_current);
				G_mw_sensor_model.current_fps = target_frame_time;
				MW_INFO(" [CHANGE] = def [%d], curr [%d], target [%d], belt [%d].\n",
					default_frame_time, curr_frame_time,
					target_frame_time, G_line_belt_current);
				transition_counter = 0;
			}
		}
	}

	/* Todo:
	 * Exit slow shutter task, restore the sensor framerate to default.
	 */
	if (G_mw_sensor_model.current_fps > G_mw_sensor_model.default_fps) {
		set_vsync_vin_framerate(G_mw_sensor_model.default_fps);
		update_vin_frame_rate(curr_frame_time, target_frame_time);
		img_ae_set_belt(G_line_belt_default);
	}
	close(G_vin_tick);
	G_vin_tick = 0;
}

int create_slowshutter_task(void)
{
	// create slow shutter thread
	slow_shutter_task_exit_flag = 0;
	if (slow_shutter_task_id == 0) {
		if (pthread_create(&slow_shutter_task_id, NULL,
			(void *)slow_shutter_task, NULL) != 0) {
			MW_MSG("Failed. Can't create thread <slow_shutter_task> !\n");
		}
		MW_INFO("Create thread <slow_shutter_task> successful !\n");
	}
	return 0;
}

int destroy_slowshutter_task(void)
{
	slow_shutter_task_exit_flag = 1;
	int i;
	if (slow_shutter_task_id != 0) {
		if (pthread_join(slow_shutter_task_id, NULL) != 0) {
			MW_MSG("Failed. Can't destroy thread <slow_shutter_task> !\n");
		}
		MW_INFO("Destroy thread <slow_shutter_task> successful !\n");
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
			if (pre_frame_rate[i].keep_fps_in_ss == 1) {
				if (ioctl(fd_iav_aaa, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX,
					&pre_frame_rate[i]) < 0) {
					perror("IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX");
					return -1;
				}
			}
		}
		memset(pre_frame_rate, 0, sizeof(pre_frame_rate));
		memset(pre_frame_100, 0, sizeof(pre_frame_100));
	}
	slow_shutter_task_exit_flag = 0;
	slow_shutter_task_id = 0;

	return 0;
}

int load_default_params(void)
{
	if (load_default_image_param() < 0) {
		MW_ERROR("load_default_image_param error!\n");
		return -1;
	}

	if (load_default_af_param() < 0) {
		MW_ERROR("load_default_af_param");
		return -1;
	}

	return 0;
}

int check_af_params(mw_af_param *pAF)
{
	if ((pAF->fs_dist < FOCUS_MACRO || pAF->fs_dist > FOCUS_INFINITY) ||
		(pAF->fs_near < FOCUS_MACRO || pAF->fs_near > FOCUS_INFINITY) ||
		(pAF->fs_far < FOCUS_MACRO || pAF->fs_far > FOCUS_INFINITY)) {
		MW_MSG("[focus distance / focus near / focus far] :"
			" one of them is out of focus range [%d, %d] (cm)\n",
			FOCUS_MACRO, FOCUS_INFINITY);
		return -1;
	}

	if (pAF->fs_near > pAF->fs_far) {
		MW_MSG("focus near [%d] is bigger than focus far [%d]\n",
			pAF->fs_near, pAF->fs_far);
		return -1;
	}

	return 0;
}

int check_aaa_state(int fd_iav)
{
	if (fd_iav < 0) {
		MW_ERROR("invalid iav fd!\n");
		return -1;
	}
	fd_iav_aaa = fd_iav;

	if (check_state() < 0) {
		MW_MSG("====can't start aaa on idle state or unkown VIN state====\n");
		return -1;
	}

	return 0;
}

int set_chroma_noise_filter_max(int fd_iav)
{
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;

	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup) < 0) {
		MW_ERROR("mw_load_mctf error!\n");
		return -1;
	}

	img_set_chroma_noise_filter_max_radius(32<<resource_setup.max_chroma_noise_shift);
	MW_MSG("resource_setup.max_chroma_noise_shift = %d\n",resource_setup.max_chroma_noise_shift);

	return 0;
}

int load_binary_file(void)
{
	if (load_mctf_bin() < 0) {
		MW_ERROR("load_mctf_bin error!\n");
		return -1;
	}
	if (config_sensor_lens_info(&G_mw_sensor_model) < 0) {
		MW_ERROR("config_sensor_lens_info error!\n");
		return -1;
	}

	if (load_calibration_data(&G_mw_cali, MW_CALIB_BPC) < 0) {
		MW_ERROR("Failed to load bad pixel calibration data!\n");
		return -1;
	}
	if (load_calibration_data(&G_mw_cali, MW_CALIB_LENS_SHADING) < 0) {
		MW_ERROR("Failed to load lens shading calibration data!\n");
		return -1;
	}

	if (load_dsp_cc_table() < 0) {
		MW_ERROR("dsp_load_cc_table error!\n");
		return -1;
	}
	if (load_adj_cc_table(&G_mw_sensor_model) < 0) {
		MW_ERROR("adj_load_cc_table error!\n");
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if(img_hdr_config_lens_info(LENS_CMOUNT_ID)< 0){
			printf("error: img_hdr_config_lens_info\n");
			return -1;
		}
		if(img_hdr_lens_init() < 0){
			printf("error: img_hdr_lens_init\n");
			return -1;
		}
	} else {
		if (img_lens_init() < 0) {
			MW_ERROR("img_lens_init error!\n");
			return -1;
		}
	}
	if (img_dsp_set_video_mctf_compression_enable(0) < 0) {
		MW_ERROR("img_dsp_set_video_mctf_compression_enable error!\n");
		return -1;
	}
	if (load_calibration_data(&G_mw_cali, MW_CONVERT_VIG_TO_AWB) < 0) {
		MW_ERROR("Failed to set viginette to awb table!\n");
		return -1;
	}
	if (load_calibration_data(&G_mw_cali, MW_CALIB_WB) < 0) {
		MW_ERROR("Failed to load white balance calibration data!\n");
		return -1;
	}

	return 0;
}

int start_aaa_task(void)
{
	static int aaa_first_enable = 1;

	g_is_rgb_sensor_vin = is_rgb_sensor_vin();
	if (g_is_rgb_sensor_vin) {
		if(img_config_working_status(fd_iav_aaa, &G_img_config_info) < 0){
			MW_ERROR("img_config_working_status\n");
			return -1;
		}
		if (img_lib_init(G_img_config_info.defblc_enable,
			G_img_config_info.sharpen_b_enable) < 0) {
			MW_ERROR("/dev/iav device!\n");
			return -1;
		}

		if (load_binary_file() < 0) {
			MW_ERROR("mw_load_mctf error!\n");
			return -1;
		}
		if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
			if (img_start_hdr(fd_iav_aaa) < 0) {
				MW_ERROR("start_aaa error!\n");
				return -1;
			}
		} else {
			if (img_start_aaa(fd_iav_aaa) < 0) {
				MW_ERROR("start_aaa error!\n");
				return -1;
			}
			if (img_set_work_mode(0)) {
				MW_ERROR("img_set_work_mode error!\n");
				return -1;
			}
			if (set_chroma_noise_filter_max(fd_iav_aaa) < 0) {
				MW_ERROR("set_chroma_mois_filter_max error!\n");
				return -1;
			}
			if(img_config_lens_cali(&lens_cali_info) < 0) {
				return -1;
			}
			if (aaa_first_enable) {
				if (load_default_params() < 0) {
					MW_ERROR("load_default_params error!\n");
					return -1;
				}
				aaa_first_enable = 0;
			} else {
				if (reload_previous_params() < 0) {
					MW_ERROR("reload_previous_params error!\n");
					return -1;
				}
			}
			if (G_mw_ae_param.slow_shutter_enable) {
				create_slowshutter_task();
			}
		}
	}
	G_aaa_task_active = 1;
	MW_MSG("======= [DONE] start_aaa ======= \n");

	return 0;

}

int stop_aaa_task(void)
{
	if (G_aaa_task_active) {
		if (g_is_rgb_sensor_vin) {

			if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
				if (img_stop_hdr() < 0) {
					MW_ERROR("stop aaa error!\n");
					return -1;
				}
			} else {
				if (G_mw_ae_param.slow_shutter_enable) {
					destroy_slowshutter_task();
				}
				if (img_stop_aaa() < 0) {
					MW_ERROR("stop aaa error!\n");
					return -1;
				}
			}

			usleep(1000);
			if (img_lib_deinit() < 0) {
				MW_ERROR("img_lib_deinit error!\n");
				return -1;
			}
		}
		MW_MSG("======= [DONE] stop_aaa ======= \n");
		G_aaa_task_active = 0;
	}

	return 0;
}

#define __END_OF_FILE__

