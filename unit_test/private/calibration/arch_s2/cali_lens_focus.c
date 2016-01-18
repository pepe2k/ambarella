/***********************************************************
 * cali_lens_focus.c
 *
 * History:
 *	2013/08/13 - [Jingyang qiu] created file
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
#include "stdio.h"
#include "fb_image.h"

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_struct_arch.h"
#include "img_api_arch.h"

#include "img_dsp_interface_arch.h"

static int log_level = MSG_LEVEL;

typedef enum {
	RGB_STATIS_AF_HIST = 0,
	CFA_STATIS_AF_HIST,
} STATISTICS_AF_TYPE;

typedef enum {
	AF_FY = 0,
	AF_FV1,
	AF_FV2,
} STATISTICS_AF_HISTOGRAM_TYPE;


int fd_iav = -1;

static pthread_t show_histogram_task_id;
static int show_histogram_task_exit_flag = 0;
fb_show_histogram_t hist_data;
static int get_data_method_flag = 0;
static int enable_fb_show_histogram = 0;

static int get_fb_config_size(int fd, fb_vin_vout_size_t *pFb_size)
{
	iav_source_buffer_setup_ex_t buffer_setup;
	iav_system_resource_setup_ex_t resource_setup;

	if (ioctl(fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		PRINT_ERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX error \n");
		return -1;
	}

	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(fd, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup) < 0) {
		PRINT_ERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX error \n");
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
		PRINT_ERROR("no preview for frame buffer\n");
		return -1;
	}

	return 0;
}

static int config_fb_vout(int fd)
{
	iav_vout_fb_sel_t fb_sel;
	fb_sel.vout_id = 1;
	fb_sel.fb_id = 0;
	if (ioctl(fd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
		PRINT_ERROR("Change osd of vout1");
		return -1;
	}
	return 0;
}

static int show_af_hist_data(struct af_stat * af_data, int af_col, int af_row)
{
	int bin_num;
	int i, j, max_value = 0;

	hist_data.row_num = af_row;
	hist_data.col_num = af_col;
	hist_data.total_num = af_col * af_row;
	for (bin_num = 0; bin_num < hist_data.total_num; ++bin_num) {
		switch(hist_data.hist_type) {
		case AF_FY:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fy;
			break;
		case AF_FV1:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv1;
			break;
		case AF_FV2:
		default:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv2;
			break;
		}
	}

	for (i = 0; i < af_row; ++i) {
		for (j = 0; j< af_col; ++j) {
			PRINT_INFO("%6d ", hist_data.histogram_value[i * af_col + j]);
			if (max_value < hist_data.histogram_value[i * af_col + j]) {
				max_value = hist_data.histogram_value[i * af_col + j];
			}
		}
		PRINT_INFO("\n");
	}
	PRINT_INFO("\n--------------\n");

	hist_data.the_max = max_value;

	if (enable_fb_show_histogram) {
		draw_histogram_plane_figure(&hist_data);
	}

	return 0;
}

void show_histogram_task(void *arg)
{
	cfa_aaa_stat_t cfa_aaa;
	rgb_aaa_stat_t rgb_aaa;
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;
	struct cfa_awb_stat * pawb_stat = NULL;
	struct cfa_ae_stat * pae_stat = NULL;
	struct af_stat * pcfa_af_stat = NULL, * prgb_af_stat = NULL;

	if (img_lib_init(1, 0) < 0) {
		PRINT_ERROR("/dev/iav device!\n");
		return;
	}

	memset(&cfa_aaa, 0, sizeof(cfa_aaa));
	memset(&rgb_aaa, 0, sizeof(rgb_aaa));

	if (img_dsp_get_statistics_raw(fd_iav, &rgb_aaa, &cfa_aaa) < 0) {
		PRINT_ERROR("img_dsp_get_statistics_raw error\n");
		return;
	}

	awb_tile_row = cfa_aaa.tile_info.awb_tile_num_row;
	awb_tile_col = cfa_aaa.tile_info.awb_tile_num_col;
	ae_tile_row = cfa_aaa.tile_info.ae_tile_num_row;
	ae_tile_col = cfa_aaa.tile_info.ae_tile_num_col;
	af_tile_row = cfa_aaa.tile_info.af_tile_num_row;
	af_tile_col = cfa_aaa.tile_info.af_tile_num_col;
	pawb_stat = (struct cfa_awb_stat *)cfa_aaa.awb_stat;
	pae_stat = (struct cfa_ae_stat *)((u8 *)pawb_stat + awb_tile_row *
		awb_tile_col * sizeof(struct cfa_awb_stat));
	pcfa_af_stat = (struct af_stat *)((u8 *)pae_stat + ae_tile_row *
		ae_tile_col * sizeof(struct cfa_ae_stat));
	prgb_af_stat = (struct af_stat *)rgb_aaa.af_stat;

	PRINT_INFO("\nAF tils [row =%d, colume = %d ] \n",
		af_tile_col, af_tile_row);

	do {

		memset(&cfa_aaa, 0, sizeof(cfa_aaa));
		memset(&rgb_aaa, 0, sizeof(rgb_aaa));

		if (img_dsp_get_statistics_raw(fd_iav, &rgb_aaa, &cfa_aaa) < 0) {
			PRINT_ERROR("img_dsp_get_statistics_raw error\n");
			return;
		}

		switch (hist_data.statist_type) {
		case RGB_STATIS_AF_HIST:
			retv = show_af_hist_data(prgb_af_stat, af_tile_col, af_tile_row);
			break;
		case CFA_STATIS_AF_HIST:
			retv = show_af_hist_data(pcfa_af_stat, af_tile_col, af_tile_row);
			break;
		default:
			PRINT_ERROR("Invalid statistics type !\n");
			retv = -1;
			break;
		}

		if (retv) {
			PRINT_ERROR("display error\n");
			return;
		}
		usleep (500000);
	} while (!show_histogram_task_exit_flag);

	if (img_lib_deinit() < 0) {
		PRINT_ERROR("img_lib_deinit error!\n");
		return;
	}

	return;
}


void show_histogram_once(void)
{
	cfa_aaa_stat_t cfa_aaa;
	rgb_aaa_stat_t rgb_aaa;
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;
	struct cfa_awb_stat * pawb_stat = NULL;
	struct cfa_ae_stat * pae_stat = NULL;
	struct af_stat * pcfa_af_stat = NULL, * prgb_af_stat = NULL;
	char ch = 1;

	if (img_lib_init(1, 0) < 0) {
		PRINT_ERROR("/dev/iav device!\n");
		return;
	}

	memset(&cfa_aaa, 0, sizeof(cfa_aaa));
	memset(&rgb_aaa, 0, sizeof(rgb_aaa));

	if (img_dsp_get_statistics_raw(fd_iav, &rgb_aaa, &cfa_aaa) < 0) {
		PRINT_ERROR("img_dsp_get_statistics_raw error\n");
		return;
	}

	awb_tile_row = cfa_aaa.tile_info.awb_tile_num_row;
	awb_tile_col = cfa_aaa.tile_info.awb_tile_num_col;
	ae_tile_row = cfa_aaa.tile_info.ae_tile_num_row;
	ae_tile_col = cfa_aaa.tile_info.ae_tile_num_col;
	af_tile_row = cfa_aaa.tile_info.af_tile_num_row;
	af_tile_col = cfa_aaa.tile_info.af_tile_num_col;
	pawb_stat = (struct cfa_awb_stat *)cfa_aaa.awb_stat;
	pae_stat = (struct cfa_ae_stat *)((u8 *)pawb_stat + awb_tile_row *
		awb_tile_col * sizeof(struct cfa_awb_stat));
	pcfa_af_stat = (struct af_stat *)((u8 *)pae_stat + ae_tile_row *
		ae_tile_col * sizeof(struct cfa_ae_stat));
	prgb_af_stat = (struct af_stat *)rgb_aaa.af_stat;

	PRINT_INFO("\nAF tils [row =%d, colume = %d ] \n",
		af_tile_col, af_tile_row);

	while (ch) {

		memset(&cfa_aaa, 0, sizeof(cfa_aaa));
		memset(&rgb_aaa, 0, sizeof(rgb_aaa));

		if (img_dsp_get_statistics_raw(fd_iav, &rgb_aaa, &cfa_aaa) < 0) {
			PRINT_ERROR("img_dsp_get_statistics_raw error\n");
			return;
		}

		switch (hist_data.statist_type) {
		case RGB_STATIS_AF_HIST:
			retv = show_af_hist_data(prgb_af_stat, af_tile_col, af_tile_row);
			break;
		case CFA_STATIS_AF_HIST:
			retv = show_af_hist_data(pcfa_af_stat, af_tile_col, af_tile_row);
			break;
		default:
			PRINT_ERROR("Invalid statistics type !\n");
			retv = -1;
			break;
		}

		if (retv) {
			PRINT_ERROR("display error\n");
			return;
		}
		printf("next time('q' = exit & return to upper level)>> \n ");
		ch = getchar();
		if (ch == 'q') {
			break;
		}
	}

	if (img_lib_deinit() < 0) {
		PRINT_ERROR("img_lib_deinit error!\n");
		return;
	}

	return;
}


static int setting_param(fb_show_histogram_t *pHistogram)
{
	if (enable_fb_show_histogram) {
		fb_vin_vout_size_t fbsize;

		if (get_fb_config_size(fd_iav, &fbsize) < 0) {
			PRINT_MSG("Error:get_fb_config_size\n");
			return -1;
		}
		if (init_fb(&fbsize) < 0) {
			PRINT_MSG("frame buffer doesn't work successfully, please check the steps\n");
			PRINT_MSG("Example:\n");
			PRINT_MSG("\tcheck whether frame buffer is malloced\n");
			PRINT_MSG("\t\tnandwrite --show_info | grep cmdline\n");
			PRINT_MSG("\tconfirm ambarella_fb module has installed\n");
			PRINT_MSG("\t\tmodprobe ambarella_fb\n");
			return -1;
		}
		if(config_fb_vout(fd_iav) < 0) {
			printf("Error:config_fb_vout\n");
			return -1;
		}
	}
	hist_data.statist_type = pHistogram->statist_type;
	hist_data.hist_type = pHistogram ->hist_type;

	return 0;
}

static int create_show_histogram_task(fb_show_histogram_t *pHistogram)
{
	show_histogram_task_exit_flag = 0;

	if (setting_param(pHistogram) < 0) {
		PRINT_ERROR("Setting_param error!\n");
		return -1;
	}

	if (show_histogram_task_id == 0) {
		if (pthread_create(&show_histogram_task_id, NULL, (void *)show_histogram_task, NULL)
			!= 0) {
			PRINT_ERROR("Failed. Cant create thread <show_histogram_task>!\n");
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
			PRINT_ERROR("Failed. Cant destroy thread <show_histogram_task>!\n");
		}
		PRINT_MSG("Destroy thread <show_histogram_task> successful !\n");
	}
	show_histogram_task_exit_flag = 0;
	show_histogram_task_id = 0;
	deinit_fb();

	return 0;
}

static int active_histogram(fb_show_histogram_t *pHistogram)
{

	 if (show_histogram_task_id) {
		if (destroy_show_histogram_task() < 0)  {
			printf("Error: destroy_show_histogram_task\n");
			return -1;
		}
	}

	if (get_data_method_flag) {
		if (setting_param(pHistogram) < 0) {
			PRINT_ERROR("Setting_param error!\n");
			return -1;
		}
		show_histogram_once();
	} else if (create_show_histogram_task(pHistogram) < 0) {
		printf("Error:create_show_histogram_task\n> ");
		show_histogram_task_id = 0;
		return -1;
	}

	return 0;

}

static int show_histogram_menu()
{
	printf("\n================ Histogram Settings ================\n");
	printf("  m -- Get statistics data method\n");
	printf("  f -- AF histogram setting\n");
	printf("  e -- Show histogram with fb\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("d > ");
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
		case 'e':
			printf("Show Histogram with fb: 0 - disable  1 - enable\n> ");
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
			PRINT_MSG("get statistics dat method:(0-auto; 1-press key)\n> ");
			scanf("%d", &i);
			if ( i < 0 || i> 1) {
				PRINT_ERROR("The value must be 0|1\n ");
			}
			get_data_method_flag = i;
			break;
		case 'f':
			printf("Select af histogram type: 0 - CFA:fy, 1 - CFA:fv1, 2 - CFA:fv2,\n");
			printf("\t\t\t  3 - RGB:fy, 4 - RGB:fv1, 5 - RGB:fv2\n> ");
			scanf("%d", &i);
			if (i < 0 || i > 5) {
				printf("Error:The value must be 0~5\n> ");
				break;
			}
			hist.statist_type = i / 3;
			hist.hist_type = i % 3;
			active_histogram(&hist);
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


int set_log_level(int log)
{
	if (log < ERROR_LEVEL || log >= (LOG_LEVEL_NUM - 1)) {
		PRINT_ERROR("Invalid log level, please set it in the range of (0~%d).\n",
			(LOG_LEVEL_NUM - 1));
		return -1;
	}
	log_level = log + 1;
	return 0;
}

static void sigstop(int signo)
{
	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
	}
	close(fd_iav);
	exit(1);
}

static int show_menu(void)
{
	PRINT_MSG("\n================================================\n");
	PRINT_MSG("  h -- Histogram menu \n");
	PRINT_MSG("  v -- Set log level\n");
	PRINT_MSG("  q -- Quit");
	PRINT_MSG("\n================================================\n\n");
	PRINT_MSG("> ");
	return 0;
}

int main(int argc, char ** argv)
{
	char ch, error_opt, quit_flag;
	int i;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		PRINT_ERROR("open /dev/iav");
		return -1;
	}

	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'h':
			histogram_setting();
			break;
		case 'v':
			PRINT_MSG("Input log level: (0~%d)\n> ", LOG_LEVEL_NUM - 2);
			scanf("%d", &i);
			set_log_level(i);
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

	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
	}
	close(fd_iav);

	return 0;
}

