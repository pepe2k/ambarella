/*******************************************************************************
 * test_mainpp.c
 *
 * History:
 *  Feb 17, 2014 - [qianshen] created file
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"
#include "img_struct_arch.h"
#include "utils.h"
#include "lib_vproc.h"

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#define MAX_MASK_ID       (65536)

enum {
	MENU_RUN = 0,
	AUTO_RUN,
	AUTO_RUN2
};

static int exit_flag = 0;
static int loglevel = LOG_INFO;
static int run_mode = 0;
static int autorun_interval = 3;
static int autorun_size = 64;
static int verbose = 0;

static char* overlay_clut_bin = "/usr/local/bin/data/overlay_clut.bin";
static char* overlay_data_bin = "/usr/local/bin/data/overlay_data.bin";
static char* overlay_ayuv_16bit = "/usr/local/bin/data/overlay_ayuv_16bit.bin";
static char* overlay_argb_16bit = "/usr/local/bin/data/overlay_argb_16bit.bin";

static overlay_param_t default_overlay = {
	.pitch = 192,
	.rect = {
		.width = 192,
		.height = 128,
	},
	.clut_pm_entry = 1,
	.clut_bg_entry = 0,
};

static cawarp_param_t cawarp;

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"menu", NO_ARG, 0, 'm'},
	{"auto", HAS_ARG, 0, 'r'},
	{"auto-size", HAS_ARG, 0, 's' },
	{"random", NO_ARG, 0, 'R'},
	{"level", HAS_ARG, 0, 'l'},
	{"verbose", NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const char *short_options = "l:mr:Rs:v";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tInteractive menu"},
	{"3~30", "\tAuto run privacy mask every N frames"},
	{"8|16|32|...", "Auto run privacy mask size which must be multiple of 8."},
	{"","\t\tAuto add and remove one privacy mask randomly."},
	{"0~2", "\tConfigu log level. 0: error, 1: info (default), 2: debug"},
	{"", "\t\tPrint more message"},
};

static void usage(void)
{
	int i;

	printf("test_pm usage:\n");
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
	printf("\n  Menu:\n    test_mainpp -m\n");
	printf("\n  Auto run privacy masks every 10 frames:\n    test_mainpp -r 4\n");
	printf("\n  Auto add/remove one privacy mask randomly:\n    test_mainpp -R\n");
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'm':
			run_mode = MENU_RUN;
			break;
		case 'r':
			run_mode = AUTO_RUN;
			autorun_interval = atoi(optarg);
			break;
		case 'R':
			run_mode = AUTO_RUN2;
			break;
		case 's':
			autorun_size = atoi(optarg);
			break;
		case 'l':
			loglevel = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

static int load_cawarp_data_from_file(const char* filename)
{
#define LINE_BUF_LEN 128
	FILE* fp;
	int i = 0;
	char line[LINE_BUF_LEN];
	char* ptr;

	if (cawarp.data) {
		free(cawarp.data);
		cawarp.data = NULL;
	}
	if ((cawarp.data = malloc(cawarp.data_num * sizeof(float))) == NULL ) {
		perror("malloc");
		return -1;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("### Cannot open file [%s].\n", filename);
		return -1;
	}

	while ((ptr = fgets(line, LINE_BUF_LEN, fp)) != NULL) {
		cawarp.data[i++] = atof(ptr);
		if (i == cawarp.data_num)
			break;
	}

	fclose(fp);
	if (i < cawarp.data_num) {
		printf("Only %d are found in file. Expected is %d\n", i, cawarp.data_num);
	}

	return i < cawarp.data_num ? -1 : 0;
}

static int load_overlay_from_file(overlay_param_t* overlay, OSD_TYPE type)
{
	int fd_clut = -1, fd_data = -1, fd_ayuv = -1,fd_argb = -1;
	int bytes, size;

	switch (type) {
		case OSD_CLUT_8BIT:
			fd_clut = open(overlay_clut_bin, O_RDONLY, 0);
			if (fd_clut < 0) {
				printf("cannot open %s\n", overlay_clut_bin);
				perror("open");
				return -1;
			}

			fd_data = open(overlay_data_bin, O_RDONLY, 0);
			if (fd_data < 0) {
				printf("cannot open %s\n", overlay_data_bin);
				perror("open");
				return -1;
			}

			if ((overlay->clut_addr = malloc(OVERLAY_CLUT_BYTES)) == NULL) {
				perror("malloc");
				return -1;
			}

			overlay->pitch = 256;
			size = overlay->pitch * overlay->rect.height;
			if ((overlay->data_addr = malloc(size)) == NULL) {
				perror("malloc");
				return -1;
			}


			if((bytes = read(fd_clut, overlay->clut_addr, OVERLAY_CLUT_BYTES)) != OVERLAY_CLUT_BYTES) {
				printf("wrong clut size %d of %s\n", bytes, overlay_clut_bin);
				return -1;
			}

			if((bytes = read(fd_data, overlay->data_addr, size)) != size) {
				printf("wrong data size %d of %s. Expected is %d.\n", bytes, overlay_data_bin, size);
				return -1;
			}
			break;

		case OSD_AYUV_16BIT:
			fd_ayuv = open(overlay_ayuv_16bit, O_RDONLY, 0);
			if (fd_ayuv < 0) {
				printf("cannot open %s\n", overlay_ayuv_16bit);
				perror("open");
				return -1;
			}

			overlay->pitch = 256;
			overlay->rect.width = 128;
			size = overlay->pitch * overlay->rect.height;
			overlay->clut_addr = NULL;
			if ((overlay->data_addr = malloc(size)) == NULL) {
				perror("malloc");
				return -1;
			}

			if((bytes = read(fd_ayuv, overlay->data_addr, size)) != size) {
				printf("wrong data size %d of %s. Expected is %d.\n", bytes, overlay_ayuv_16bit, size);
				return -1;
			}
			break;
		case OSD_ARGB_16BIT:
			fd_argb = open(overlay_argb_16bit, O_RDONLY, 0);
			if (fd_argb < 0) {
				printf("cannot open %s\n", overlay_argb_16bit);
				perror("open");
				return -1;
			}

			overlay->pitch = 256;
			overlay->rect.width = 128;
			size = overlay->pitch * overlay->rect.height;
			overlay->clut_addr = NULL;
			if ((overlay->data_addr = malloc(size)) == NULL) {
				perror("malloc");
				return -1;
			}

			if((bytes = read(fd_argb, overlay->data_addr, size)) != size) {
				printf("wrong data size %d of %s. Expected is %d.\n", bytes, overlay_argb_16bit, size);
				return -1;
			}
			break;

		default:
			printf("No such OSD type!\n");
			return -1;
	}
	if (fd_clut > 0) {
		close(fd_clut);
	}
	if (fd_data > 0) {
		close(fd_data);
	}
	if (fd_ayuv > 0) {
		close(fd_ayuv);
	}
	if (fd_argb > 0) {
		close(fd_argb);
	}

	return 0;
}

static int show_menu(void)
{
	printf("\n");
	printf("|-------------------------------------|\n");
	printf("| 0  - Prepare before encode start    |\n");
	printf("| 1  - Privacy Mask                   |\n");
	printf("| 2* - Privacy Mask Color             |\n");
	printf("| 3* - MCTF                           |\n");
	printf("| 4  - DPTZ                           |\n");
	printf("| 5  - Chromatic Aberration Correction|\n");
	printf("| 6  - Overlay                        |\n");
	printf("| 7  - Encode Offset                  |\n");
	printf("| q  - Quit                           |\n");
	printf("|-------------------------------------|\n");
	printf("Note: Option with * is supported in specfic mode.\n");
	printf("\n");
	return 0;
}

static int input_value(int min, int max)
{
	int retry = 1, input;
	char tmp[16];
	do {
		scanf("%s", tmp);
		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			continue;
		}
		retry = 0;
	} while (retry);
	return input;
}

static void quit()
{
	exit_flag = 1;
	vproc_exit();
	if (cawarp.data) {
		free(cawarp.data);
		cawarp.data = NULL;
	}

	exit(0);
}

static int menu_run(void)
{
	char opt;
	char input[16], ca_file[128];
	int id;
	yuv_color_t color;
	pm_param_t pm;
	mctf_param_t mctf;
	iav_digital_zoom_ex_t dptz;
	offset_param_t offset;
	overlay_param_t overlay;

	memset(&cawarp, 0, sizeof(cawarp));

	while (!exit_flag) {
		show_menu();
		printf("Your choice: ");
		fflush(NULL);
		scanf("%s", input);
		opt = input[0];
		switch (opt) {
			case '0':
				printf("\nInput Stream Id : ");
				id = input_value(0, IAV_STREAM_MAX_NUM_IMPL -1);
				vproc_stream_prepare(id);
				break;
			case '1':
				printf("\nInput Enable (1: enable, 0: disable): ");
				pm.enable = input_value(0, 1);
				printf("\nInput mask Id : ");
				pm.id = input_value(0, MAX_MASK_ID);
				memset(&pm.rect, 0, sizeof(pm.rect));
				if (pm.enable) {
				printf("Input mask x : ");
				pm.rect.x = input_value(0, 10000);
				printf("Input mask y : ");
				pm.rect.y = input_value(0, 10000);
				printf("Input mask width : ");
				pm.rect.width = input_value(0, 10000);
				printf("Input mask height : ");
				pm.rect.height = input_value(0, 10000);
				}
				vproc_pm(&pm);
				break;

			case '2':
				printf("Input Y : ");
				color.y = input_value(0, 255);
				printf("Input U : ");
				color.u = input_value(0, 255);
				printf("Input V : ");
				color.v = input_value(0, 255);
				vproc_pm_color(&color);
				break;
			case '3':
				memset(&mctf.rect, 0, sizeof(mctf.rect));
				printf("Input MCTF area x : ");
				mctf.rect.x = input_value(0, 10000);
				printf("Input MCTF area y : ");
				mctf.rect.y = input_value(0, 10000);
				printf("Input MCTF area width : ");
				mctf.rect.width = input_value(0, 10000);
				printf("Input MCTF area height: ");
				mctf.rect.height = input_value(0, 10000);
				printf("Input MCTF strength: ");
				mctf.strength = input_value(0, 10000);
				vproc_mctf(&mctf);
				break;
			case '4':
				memset(&dptz, 0, sizeof(dptz));
				printf("Input Source Buffer ID: ");
				dptz.source = input_value(0, IAV_ENCODE_SOURCE_FOURTH_BUFFER);
				printf("Input DPTZ input offset x : ");
				dptz.input.x = input_value(0, 10000);
				printf("Input DPTZ input offset y : ");
				dptz.input.y = input_value(0, 10000);
				printf("Input DPTZ input width : ");
				dptz.input.width = input_value(0, 10000);
				printf("Input DPTZ input height : ");
				dptz.input.height = input_value(0, 10000);
				vproc_dptz(&dptz);
				break;
			case '5':
				memset(ca_file, 0, sizeof(ca_file));
				printf("Input CA data num (>= 2: enable CA, 0: disable CA) : ");
				cawarp.data_num = input_value(0, 10000);
				if (cawarp.data_num >= 2) {
					printf("Input CA data file: ");
					scanf("%s", ca_file);
					if (load_cawarp_data_from_file(ca_file) < 0) {
						break;
					}
					printf("Input Red scale factor: ");
					scanf("%f", &cawarp.red_factor);
					printf("Input Blue scale factor: ");
					scanf("%f", &cawarp.blue_factor);
					printf("Input Center Offset X: ");
					cawarp.center_offset_x = input_value(-10000, 10000);
					printf("Input Center Offset Y: ");
					cawarp.center_offset_y = input_value(-10000, 10000);
				}
				vproc_cawarp(&cawarp);

				break;
			case '6':
				overlay = default_overlay;
				printf("Input Stream ID: ");
				overlay.stream_id = input_value(0, IAV_STREAM_MAX_NUM_IMPL -1);
				printf("OSD Type (0: CLUT, 1: AYUV, 2: ARGB): ");
				overlay.type = (OSD_TYPE)input_value(0, OSD_TYPE_NUM - 1);
				printf("Input Area ID: ");
				overlay.area_id = input_value(0, MAX_NUM_OVERLAY_AREA - 1);
				printf("Enable (0: disable, 1: enable): ");
				overlay.enable = input_value(0, 1);
				if (overlay.enable) {
					printf("Input offset x: ");
					overlay.rect.x = input_value(0, 10000);
					printf("Input offset y: ");
					overlay.rect.y = input_value(0, 10000);

					if (load_overlay_from_file(&overlay, overlay.type) == 0) {
						vproc_stream_overlay(&overlay);
					}
					if (overlay.clut_addr) {
						free(overlay.clut_addr);
						overlay.clut_addr = NULL;
					}
					if (overlay.data_addr) {
						free(overlay.data_addr);
						overlay.data_addr = NULL;
					}
				} else {
					vproc_stream_overlay(&overlay);
				}
				break;
			case '7':
				printf("Input Stream ID: ");
				offset.stream_id = input_value(0, IAV_STREAM_MAX_NUM_IMPL - 1);
				printf("Input offset x: ");
				offset.x = input_value(0, 10000);
				printf("Input offset y: ");
				offset.y = input_value(0, 10000);
				vproc_stream_offset(&offset);
				break;
			case 'q':
				exit_flag = 1;
				break;
			default:
				printf("unknown option %c.\n", opt);
				break;
		}
	}
	return 0;
}

static int auto_run(void)
{
	int iav_fd = -1, vin_fd = -1;
	iav_privacy_mask_info_ex_t mask_info;
	iav_source_buffer_format_ex_t main_buffer_info;
	iav_source_buffer_setup_ex_t  setup;
	struct amba_video_info vin;
	int pic_width = 0, pic_height = 0;
	pm_param_t mask;
	int frame_count;
	char vin_int_arr[8];
	int remove = 0;
	int ret = 0;
	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if ((vin_fd = open(VIN0_VSYNC, O_RDONLY)) < 0) {
		printf("CANNOT open [%s].\n", VIN0_VSYNC);
		ret = -1;
		goto auto_run_exit;
	}

	if (ioctl(iav_fd, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
		ret = -1;
		goto auto_run_exit;
	}

	switch (mask_info.domain) {
		case IAV_PRIVACY_MASK_DOMAIN_MAIN:
			main_buffer_info.source = 0;
			if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX,
					&main_buffer_info) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
				ret = -1;
				goto auto_run_exit;
			}
			pic_width = ROUND_DOWN(main_buffer_info.size.width, 16);
			pic_height = ROUND_DOWN(main_buffer_info.size.height, 16);
			break;
		case IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT:
			if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
				ret = -1;
				goto auto_run_exit;
			}
			pic_width = setup.pre_main_input.width;
			pic_height = setup.pre_main_input.height;
			break;
		case IAV_PRIVACY_MASK_DOMAIN_VIN:
			if (ioctl(iav_fd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
				ret = -1;
				goto auto_run_exit;
			}
			pic_width = vin.width;
			pic_height = vin.height;
			break;
		default:
			printf("unknown privacy mask domain %d.\n", mask_info.domain);
			ret = -1;
			goto auto_run_exit;
	}

	mask.rect.x = 0;
	mask.rect.y = 0;
	mask.rect.width = autorun_size;
	mask.rect.height = autorun_size;
	mask.id = 0;
	while (1) {
		if (!remove) {
			if (mask.rect.x + mask.rect.width > pic_width) {
				mask.rect.x = 0;
				mask.rect.y += mask.rect.height;
			}

			if (mask.rect.y + mask.rect.height > pic_height) {
				remove = 1;
				--mask.id;
			}

		} else {
			if (mask.id < 0) {
				if (mask.rect.width == pic_width && mask.rect.height == pic_height) {
					mask.rect.width = autorun_size;
					mask.rect.height = autorun_size;
				} else {
					mask.rect.width *= 2;
					mask.rect.height *= 2;
					if (mask.rect.height > pic_height) {
						mask.rect.height = pic_height;
					}

					if (mask.rect.width > pic_width) {
						mask.rect.width = pic_width;
					}
				}
				mask.id = 0;
				mask.rect.x = 0;
				mask.rect.y = 0;
				remove = 0;
			}


		}

		if (verbose)
			start_timing();
		while (frame_count < autorun_interval) {
			read(vin_fd, vin_int_arr, 8);
			frame_count ++;
		}
		if (frame_count >= autorun_interval)
			frame_count = 0;

		if (remove) {
			mask.enable = 0;
			vproc_pm(&mask);
			INFO("remove mask [%d]\n", mask.id);
			--mask.id;
		} else {
			mask.enable = 1;
			vproc_pm(&mask);
			INFO("add mask [%d] x %d, y %d, w %d, h %d\n", mask.id,
				mask.rect.x, mask.rect.y, mask.rect.width, mask.rect.height);
			++mask.id;
			mask.rect.x += mask.rect.width;
		}

		if (verbose) {
			stop_timing();
			perf_report();
		}

	}

auto_run_exit:
	if (vin_fd >= 0) {
		close(vin_fd);
		vin_fd = -1;
	}
	if (iav_fd >= 0) {
		close(iav_fd);
		iav_fd = -1;
	}
	return ret;
}

int auto_run2(void)
{
	int iav_fd = -1;
	iav_privacy_mask_info_ex_t mask_info;
	iav_source_buffer_format_ex_t main_buffer_info;
	iav_source_buffer_setup_ex_t  setup;
	struct amba_video_info vin;
	int pic_width = 0, pic_height = 0, align;
	pm_param_t mask = {0};
	int ret = 0;

	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (ioctl(iav_fd, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
		ret = -1;
		goto auto_run2_exit;
	}

	switch (mask_info.domain) {
		case IAV_PRIVACY_MASK_DOMAIN_MAIN:
			main_buffer_info.source = 0;
			if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX,
			    &main_buffer_info) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
				ret = -1;
				goto auto_run2_exit;
			}
			pic_width = ROUND_DOWN(main_buffer_info.size.width, 16);
			pic_height = ROUND_DOWN(main_buffer_info.size.height, 16);
			align = 16;
			break;
		case IAV_PRIVACY_MASK_DOMAIN_PREMAIN_INPUT:
			if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
				ret = -1;
				goto auto_run2_exit;
			}
			pic_width = setup.pre_main_input.width;
			pic_height = setup.pre_main_input.height;
			align = 8;
			break;
		case IAV_PRIVACY_MASK_DOMAIN_VIN:
			if (ioctl(iav_fd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
				ret = -1;
				goto auto_run2_exit;
			}
			pic_width = vin.width;
			pic_height = vin.height;
			align = 8;
			break;
		default:
			printf("unknown privacy mask domain %d.\n", mask_info.domain);
			ret = -1;
			goto auto_run2_exit;
	}

	while (1) {
		mask.rect.x = ROUND_DOWN(rand() % pic_width, align);
		mask.rect.y = ROUND_DOWN(rand() % pic_height, align);
		mask.rect.width =  ROUND_UP(8 * (10 + rand() % 50), align);
		mask.rect.height = ROUND_UP(8 * (10 + rand() % 50), align);
		if (mask.rect.x + mask.rect.width > pic_width)
			mask.rect.x -= (mask.rect.x + mask.rect.width) - pic_width;

		if (mask.rect.y + mask.rect.height > pic_height)
			mask.rect.y -= (mask.rect.y + mask.rect.height) - pic_height;

		INFO("adding mask x %d, y %d, w %d, h %d\n",
		    mask.rect.x, mask.rect.y, mask.rect.width, mask.rect.height);
		mask.enable = 1;
		vproc_pm(&mask);
		usleep(1000 * 1000);
		INFO("removing ...\n");
		mask.enable = 0;
		vproc_pm(&mask);
		usleep(100 * 1000);
	}

auto_run2_exit:
	if (iav_fd >= 0) {
		close(iav_fd);
		iav_fd = -1;
	}
	return ret;
}

int main(int argc, char** argv)
{
	version_t version;
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGQUIT, quit);

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	vproc_version(&version);
	printf("Privacy Mask Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
	    version.description, version.major, version.minor,
	    version.patch, version.mod_time);

	set_log(loglevel, NULL);

	if (run_mode == MENU_RUN) {
		menu_run();
	} else if (run_mode == AUTO_RUN){
		auto_run();
	} else if (run_mode == AUTO_RUN2) {
		auto_run2();
	}

	quit();
	return 0;
}
