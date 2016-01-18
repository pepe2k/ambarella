#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>

#include "basetypes.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "ambas_vin.h"
#include "af_algo.h"
#include "dummy_af_drv.h"
#include "tamronDF003_drv.h"

#include "mt9t002_adj_param.c"
#include "mt9t002_aeb_param.c"

#include "mn34041pl_adj_param.c"
#include "mn34041pl_aeb_param.c"


#include "imx172_adj_param.c"
#include "imx172_aeb_param.c"

#include "imx178_adj_param.c"
#include "imx178_aeb_param.c"


#include "imx136_adj_param.c"
#include "imx136_aeb_param.c"

#include "ov2710_adj_param.c"
#include "ov2710_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

extern lens_dev_drv_t tamronDF003_drv;

extern int af_print_level;
extern lens_dev_drv_t dummy_af_drv;
int fd_iav;

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

tone_curve_t tone_curve = {
	{/* red */
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
	{/* green */
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
	{/* blue */
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

int load_mctf_bin()
{
	int i;
	int file = -1, count;
	u8* bin_buff;
	char filename[256];
	idsp_one_def_bin_t one_bin;
	idsp_def_bin_t bin_map;
#define BIN_SIZE		27600

	img_dsp_get_default_bin_map(&bin_map);
	bin_buff = malloc(BIN_SIZE);
	if (bin_buff == NULL)
		return -1;
	for (i = 0; i<bin_map.num;i++)
	{
		memset(filename,0,sizeof(filename));
		memset(bin_buff,0,BIN_SIZE);
		sprintf(filename, "%s/%s", IMGPROC_PARAM_PATH, bin_fn[i]);
		if((file = open(filename, O_RDONLY, 0))<0) {
			printf("%s cannot be opened\n",filename);
			goto free_and_exit;
		}
		if((count = read(file, bin_buff, bin_map.one_bin[i].size)) != bin_map.one_bin[i].size) {
			printf("read %s error\n",filename);
			goto free_and_exit;
		}
		one_bin.size = bin_map.one_bin[i].size;
		one_bin.type = bin_map.one_bin[i].type;
		if (img_dsp_load_default_bin((u32)bin_buff, one_bin) < 0) {
			printf("img_dsp_load_default_bin error\n");
			goto free_and_exit;
		}
		if (file > 0) {
			close(file);
			file = -1;
		}
	}

	free(bin_buff);
	return 0;

free_and_exit:
	if (bin_buff != NULL) {
		free(bin_buff);
	}
	if (file > 0) {
		close(file);
		file = -1;
	}

	return -1;
}

int load_dsp_cc_table(void)
{
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8* reg, *matrix, *sec_cc;
	char filename[128];
	int file = -1, count;

	reg = malloc(CC_REG_SIZE);
	matrix = malloc(CC_3D_SIZE);
	sec_cc = malloc(SEC_CC_SIZE);
	if (reg == NULL || matrix == NULL || sec_cc == NULL) {
		goto free_and_exit;
	}
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("reg.bin cannot be opened\n");
		goto free_and_exit;
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		goto free_and_exit;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("3D.bin cannot be opened\n");
		goto free_and_exit;
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		goto free_and_exit;
	}
	close(file);

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		goto free_and_exit;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		goto free_and_exit;
	}
	close(file);
	file = -1;

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;
	if (img_dsp_set_color_correction_reg(&color_corr_reg) < 0) {
		printf("img_dsp_set_color_correction_reg error\n");
		goto free_and_exit;
	}

	if (img_dsp_set_color_correction(fd_iav, &color_corr) < 0) {
		printf("img_dsp_set_color_correction error\n");
		goto free_and_exit;
	}

	if (img_dsp_set_tone_curve(fd_iav, &tone_curve) < 0) {
		printf("img_dsp_set_tone_curve error\n");
		goto free_and_exit;
	}
	if (img_dsp_set_sec_cc_en(fd_iav, 0) < 0) {
		printf("img_dsp_set_sec_cc_en error\n");
		goto free_and_exit;
	}
	if (img_dsp_enable_color_correction(fd_iav) < 0) {
		printf("img_dsp_enable_color_correction error\n");
		goto free_and_exit;
	}

/*	sprintf(filename, "%s/hiiso.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		if((file = open("./hiiso.bin", O_RDONLY, 0)) < 0) {
			printf("hiiso.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, matrix, 17536)) != 17536) {
		printf("read hiiso.bin error\n");
		return -1;
	}
	close(file);*/

	free(reg);
	free(matrix);
	free(sec_cc);
	return 0;

free_and_exit:
	if (reg != NULL) {
		free(reg);
	}
	if (matrix != NULL) {
		free(matrix);
	}
	if (sec_cc != NULL) {
		free(sec_cc);
	}
	if (file > 0) {
		close(file);
		file = -1;
	}
	return -1;
}

int load_adj_cc_table(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[17536];
	u8 i, adj_mode = 4;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("can not open 3D.bin\n");
			return -1;
		}
		if((count = read(file, matrix, 17536)) != 17536) {
			printf("read imx036_01_3D.bin error\n");
			close(file);
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}

	return 0;
}

static int print_menu(void)
{
	printf("\n================================================\n");
	printf("  m -- print settings (enable/disable AF MSG)\n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static int print_setting(void)
{
	int i, quit_flag, error_opt;
	char ch;

	print_menu();
	ch = getchar();

	while(ch) {
		quit_flag = 0;
		error_opt = 0;

		switch (ch) {
		case 'm':
			printf("\n 0: disable, 1: enable \n> ");
			scanf("%d", &i);
			if (i > 1 || i < 0) {
				printf("Error: the value must be 0|1\n> ");
				return -1;
			}
			af_print_level = i;
			printf("\n================================================\n\n");
			printf("> ");
			break;
		default:
			break;
		}

		if (error_opt == 0) {
			print_menu();
		}
		ch = getchar();
		if (ch == 'q') {
			quit_flag = 1;
		}
		if (quit_flag)
			break;
	}

	return 0;
}

static void sigstop(int signo)
{
	if (img_stop_aaa() < 0) {
		printf("img_stop_aaa error!\n");
		return;
	}
	usleep(1000);
	if (img_lib_deinit() < 0) {
		printf("img_lib_deinit error!\n");
		return;
	}

	close(fd_iav);
	fd_iav = -1;

	exit(1);
}

int prepare_sensor_params(char * pSensor_name, sensor_label *pSensor_lb,
	image_sensor_param_t *pParam_image)
{
	struct amba_vin_source_info vin_info;
	u32 vin_fps;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &vin_fps) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}

	switch (vin_info.sensor_id) {
		case SENSOR_OV2710:
			*pSensor_lb = OV_2710;
		      	sprintf(pSensor_name, "ov2710");
			pParam_image->p_adj_param = &ov2710_adj_param;
			pParam_image->p_rgb2yuv = ov2710_rgb2yuv;
			pParam_image->p_chroma_scale = &ov2710_chroma_scale;
			pParam_image->p_awb_param = &ov2710_awb_param;
			if (vin_fps == AMBA_VIDEO_FPS_60) {
				pParam_image->p_50hz_lines = ov2710_60p50hz_lines;
				pParam_image->p_60hz_lines =ov2710_60p60hz_lines;
			} else {
				pParam_image->p_50hz_lines = ov2710_50hz_lines;
				pParam_image->p_60hz_lines =ov2710_60hz_lines;
			}
			pParam_image->p_tile_config = &ov2710_tile_config;
			pParam_image->p_ae_agc_dgain = ov2710_ae_agc_dgain;
			pParam_image->p_ae_sht_dgain = ov2710_ae_sht_dgain;
			pParam_image->p_dlight_range =ov2710_dlight;
			pParam_image->p_manual_LE = ov2710_manual_LE;
			break;
		case SENSOR_MN34041PL:
			*pSensor_lb = MN_34041PL;
			sprintf(pSensor_name, "mn34041pl");
			pParam_image->p_adj_param = &mn34041pl_adj_param;
			pParam_image->p_rgb2yuv = mn34041pl_rgb2yuv;
			pParam_image->p_chroma_scale = &mn34041pl_chroma_scale;
			pParam_image->p_awb_param = &mn34041pl_awb_param;
			if (vin_fps == AMBA_VIDEO_FPS_60) {
				pParam_image->p_50hz_lines = mn34041pl_60p50hz_lines;
				pParam_image->p_60hz_lines = mn34041pl_60p60hz_lines;
			} else {
				pParam_image->p_50hz_lines = mn34041pl_50hz_lines;
				pParam_image->p_60hz_lines = mn34041pl_60hz_lines;
			}
			pParam_image->p_tile_config = &mn34041pl_tile_config;
			pParam_image->p_ae_agc_dgain = mn34041pl_ae_agc_dgain;
			pParam_image->p_ae_sht_dgain = mn34041pl_ae_sht_dgain;
			pParam_image->p_dlight_range = mn34041pl_dlight;
			pParam_image->p_manual_LE = mn34041pl_manual_LE;
			break;
		case SENSOR_IMX136:
			*pSensor_lb = IMX_136;
		      	sprintf(pSensor_name, "imx136");
			pParam_image->p_adj_param = &imx136_adj_param;
			pParam_image->p_rgb2yuv = imx136_rgb2yuv;
			pParam_image->p_chroma_scale = &imx136_chroma_scale;
			pParam_image->p_awb_param = &imx136_awb_param;
			if (vin_fps == AMBA_VIDEO_FPS_60) {
				pParam_image->p_50hz_lines = imx136_60p50hz_lines;
				pParam_image->p_60hz_lines =imx136_60p60hz_lines;
			} else {
				pParam_image->p_50hz_lines = imx136_50hz_lines;
				pParam_image->p_60hz_lines =imx136_60hz_lines;
			}
			pParam_image->p_tile_config = &imx136_tile_config;
			pParam_image->p_ae_agc_dgain = imx136_ae_agc_dgain;
			pParam_image->p_ae_sht_dgain = imx136_ae_sht_dgain;
			pParam_image->p_dlight_range = imx136_dlight;
			pParam_image->p_manual_LE = imx136_manual_LE;
			break;
		case SENSOR_IMX172:
			*pSensor_lb = IMX_172;
			sprintf(pSensor_name, "imx172");
			pParam_image->p_adj_param = &imx172_adj_param;
			pParam_image->p_rgb2yuv = imx172_rgb2yuv;
			pParam_image->p_chroma_scale = &imx172_chroma_scale;
			pParam_image->p_awb_param = &imx172_awb_param;
			if (vin_fps == AMBA_VIDEO_FPS_60) {
				pParam_image->p_50hz_lines = imx172_60p50hz_lines;
				pParam_image->p_60hz_lines =imx172_60p60hz_lines;
			} else {
				pParam_image->p_50hz_lines = imx172_50hz_lines;
				pParam_image->p_60hz_lines = imx172_60hz_lines;
			}
			pParam_image->p_tile_config = &imx172_tile_config;
			pParam_image->p_ae_agc_dgain = imx172_ae_agc_dgain;
			pParam_image->p_ae_sht_dgain = imx172_ae_sht_dgain;
			pParam_image->p_dlight_range = imx172_dlight;
			pParam_image->p_manual_LE = imx172_manual_LE;

			break;
		case SENSOR_IMX178:
			*pSensor_lb = IMX_178;
			sprintf(pSensor_name, "imx178");
			pParam_image->p_adj_param = &imx178_adj_param;
			pParam_image->p_rgb2yuv = imx178_rgb2yuv;
			pParam_image->p_chroma_scale = &imx178_chroma_scale;
			pParam_image->p_awb_param = &imx178_awb_param;
			if (vin_fps == AMBA_VIDEO_FPS_60) {
				pParam_image->p_50hz_lines = imx178_60p50hz_lines;
				pParam_image->p_60hz_lines =imx178_60p60hz_lines;
			} else {
				pParam_image->p_50hz_lines = imx178_50hz_lines;
				pParam_image->p_60hz_lines = imx178_60hz_lines;
			}
			pParam_image->p_tile_config = &imx178_tile_config;
			pParam_image->p_ae_agc_dgain = imx178_ae_agc_dgain;
			pParam_image->p_ae_sht_dgain = imx178_ae_sht_dgain;
			pParam_image->p_dlight_range = imx178_dlight;
			pParam_image->p_manual_LE = imx178_manual_LE;
			break;

		default:
			return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	char sensor_name[32];
	sensor_label sensor_lb;
	image_sensor_param_t app_param_image_sensor = {0};
	void * p_af_param = NULL;
	void * p_zoom_map = NULL;

	aaa_api_t custom_aaa_api = {0};
	iav_sharpen_filter_cfg_t sharpen_filter;

	custom_aaa_api.p_ae_control = NULL;
	custom_aaa_api.p_ae_control_init = NULL;
	custom_aaa_api.p_af_control = custom_af_control;
	custom_aaa_api.p_af_control_init = custom_af_control_init;
	custom_aaa_api.p_af_set_calib_param = custom_af_set_calib_param;
	custom_aaa_api.p_af_set_range = custom_af_set_range;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	memset(&sharpen_filter, 0, sizeof(sharpen_filter));
	if (ioctl(fd_iav, IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX, &sharpen_filter) < 0) {
		perror("IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX");
		return -1;
	}
	printf("=== sharpen B [%d].\n", sharpen_filter.sharpen_b_enable);
	 img_lib_init(1, sharpen_filter.sharpen_b_enable);

	load_mctf_bin();

	if (prepare_sensor_params(sensor_name,
		&sensor_lb, &app_param_image_sensor) < 0) {
		perror("prepare_sensor_params");
		return -1;
	}

	if (img_config_sensor_info(sensor_lb) < 0) {
		return -1;
	}

	if (load_dsp_cc_table() < 0)
		return -1;
	if (load_adj_cc_table(sensor_name) < 0)
		return -1;

	if(img_load_image_sensor_param(&app_param_image_sensor) < 0) {
		printf("img_load_image_sensor_param error\n");
		return -1;
	}

	if (img_config_lens_info(LENS_CUSTOM_ID) < 0) {
		printf("img_config_lens_info error\n");
		return -1;
	}

	if (img_register_aaa_algorithm(custom_aaa_api) < 0) {
		printf("img_register_aaa_algorithm error\n");
		return -1;
	}

	if (img_register_lens_drv(dummy_af_drv) < 0) {
		printf("img_register_lens_drv error\n");
		return -1;
	}

	img_af_load_param(p_af_param, p_zoom_map);

	if (img_lens_init() < 0) {
		printf("img_lens_init error\n");
		return -1;
	}

	if(img_start_aaa(fd_iav) < 0) {
		printf("img_start_aaa error!\n");
		return -1;
	}
	if (img_set_work_mode(0)) {
		printf("img_set_work_mode error!\n");
		return -1;
	}
	img_enable_af(1);

	print_setting();

	if (img_stop_aaa() < 0) {
		printf("img_stop_aaa error!\n");
		return -1;
	}
	usleep(1000);
	if (img_lib_deinit() < 0) {
		printf("img_lib_deinit error!\n");
		return -1;
	}

	close(fd_iav);
	fd_iav = -1;
	return 0;
}

