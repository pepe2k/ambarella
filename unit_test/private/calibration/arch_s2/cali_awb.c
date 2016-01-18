/*
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_common.h"
#include "mw_struct.h"
#include "mw_api.h"
#define CHECK_RVAL if(rval<0)	printf("error: %s:%d\n",__func__,__LINE__);

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "img_struct_arch.h"
#include "ambas_vin.h"
#include "img_api_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_hdr_api_arch.h"

#include "mt9t002_adj_param.c"
#include "mt9t002_aeb_param.c"
#include "ar0331_adj_param.c"
#include "ar0331_aeb_param.c"

#include "mn34041pl_adj_param.c"
#include "mn34041pl_aeb_param.c"

#include "mn34210pl_adj_param.c"
#include "mn34210pl_aeb_param.c"

#include "mn34220pl_adj_param.c"
#include "mn34220pl_aeb_param.c"

#include "imx121_adj_param.c"
#include "imx121_aeb_param.c"

#include "imx123_adj_param.c"
#include "imx123_aeb_param.c"

#include "imx172_adj_param.c"
#include "imx172_aeb_param.c"

#include "imx178_adj_param.c"
#include "imx178_aeb_param.c"

#include "imx104_adj_param.c"
#include "imx104_aeb_param.c"

#include "imx136_adj_param.c"
#include "imx136_aeb_param.c"

#include "ov2710_adj_param.c"
#include "ov2710_aeb_param.c"

#include "imx185_adj_param.c"
#include "imx185_aeb_param.c"

#include "imx226_adj_param.c"
#include "imx226_aeb_param.c"

#include "ov5658_adj_param.c"
#include "ov5658_aeb_param.c"

#include "ov4689_adj_param.c"
#include "ov4689_aeb_param.c"
//#include "m13vp288ir_piris_param.c"

#define	IMGPROC_PARAM_PATH		"/etc/idsp"

#ifndef	ARRAY_SIZE
#define	ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

#ifndef ABS
#define ABS(a)	(((a) < 0) ? -(a) : (a))
#endif

#ifndef	MAX
#define MAX(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#define REG_SIZE			18752
#define MATRIX_SIZE			17536
#define CALI_AWB_FILENAME	"cali_awb.txt"

static const char *default_filename = CALI_AWB_FILENAME;
static char cali_awb_filename[256];
static char load_awb_filename[256];
static int fd_iav;
static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int save_file_flag = 0;
static int vin_op_mode = AMBA_VIN_LINEAR_MODE;

mw_sensor_param G_hdr_param = {0};
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
u32	saturation			= 64;
u32	brightness			= 0;
u32	contrast				= 64;
u32	sharpness			= 6;
u32	mctf_strength			= 1;
u32	auto_local_exposure_mode	= 0;
u32	backlight_comp_enable		= 0;
u32	day_night_mode_enable	= 0;

static wb_gain_t wb_gain;
static int correct_param[12];

#define NO_ARG	0
#define HAS_ARG	1

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

static struct option long_options[] = {
	{"detect",   NO_ARG,  0, 'd'},
	{"correct",  HAS_ARG, 0, 'c'},
	{"restore",  NO_ARG, 0, 'r'},
	{"savefile", HAS_ARG, 0, 'f'},
	{"loadfile", HAS_ARG, 0, 'l'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "dc:rf:l:";

static struct hint_s hint[] = {
	{"", "\t\tGet current AWB gain"},
	{"[v1,v2,...,v12]", "\t\torignal AWB gain in R/G/Bin low/high light, target WB gain in R/G/B in low/high light "},
	{"", "\t\tRestore AWB as default"},
	{"", "\t\tSpecify the file to save current AWB gain"},
	{"", "\t\tload the file for correction"},
};

static void sigstop()
{
	img_stop_aaa();
	printf("Quit cali_wb.\n");
	exit(1);
}

static void usage(void)
{
	int i;
	printf("\ncali_app usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("\n\n");
	printf("The white balance calibration is defined as a gain correction for\n");
	printf("red, green and blue components by gain factors.\n");
	printf("You can use -d option with the perfect sensor and light to get the\n");
	printf("target AWB gain factors (1024 as a unit) for your reference.\n");
	printf("When applying calibration for imperfect sensor, specify the wrong\n");
	printf("gain and the target gain to do calibration by -c option.\n\n");
}

static int get_multi_arg(char *optarg, int *argarr, int argcnt)
{
	int i;
	char *delim = ",:-";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;
	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, i = 0, data[3];
	int option_index = 0;
	FILE *fp;
	char s[256];
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			break;
		case 'c':
			correct_flag = 1;
			if (get_multi_arg(optarg, correct_param, ARRAY_SIZE(correct_param)) < 0) {
				printf("Need %d args for opt %c !\n", ARRAY_SIZE(correct_param), ch);
				return -1;
			}
			while (i < ARRAY_SIZE(correct_param)) {
				if (correct_param[i] <= 0) {
					printf("args should be greater than 0!\n");
					return -1;
				}
				i++;
			}
			break;
		case 'r':
			restore_flag = 1;
			break;
		case 'f':
			save_file_flag = 1;
			strcpy(cali_awb_filename, optarg);
			break;
		case 'l':
			correct_flag = 1;
			strcpy(load_awb_filename, optarg);
			if (strlen(load_awb_filename) == 0) {
				printf("Please input the file path used for correction. It is generated by awb_calibration.sh.\n");
				return -1;
			}
			if ((fp = fopen(load_awb_filename, "r+")) == NULL) {
				printf("Open file %s error.\n", load_awb_filename);
				return -1;
			}
			i = 0;
			while (NULL != fgets(s, sizeof(s), fp) && (i < 4)) {
				if (get_multi_arg(s, data, 3) < 0) {
					printf("wrong data format in %s.\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				if (data[0] <= 0 || data[1] <= 0 || data[2] <= 0) {
					printf("Data in %s should be greater than 0!\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				correct_param[i*3] = data[0];
				correct_param[i*3+1] = data[1];
				correct_param[i*3+2] = data[2];
				i++;
			}
			fclose(fp);
			if (i < 3) {
				printf("Incomplete data in %s.\n", load_awb_filename);
				return -1;
			}
			break;
		default:
			printf("Unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}
inline int get_vin_frame_rate(u32 *pFrame_time)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}
	return 0;
}
inline int get_system_sharpen_filter_config(iav_sharpen_filter_cfg_t *sharpen_filter)
{
	if (ioctl(fd_iav, IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX, sharpen_filter) < 0) {
		perror("IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX");
		return -1;
	}
	return 0;
}
inline int get_sensor_bayer_pattern(u8 *pattern)
{
	struct amba_video_info video_info;

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0){
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO error");
		return -1;
	}
	*pattern = video_info.pattern;
	return 0;
}

inline int get_sensor_step(u8 *step)
{
	amba_vin_agc_info_t sensor_db_info;

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_AGC_INFO, &sensor_db_info) < 0){
		perror("IAV_IOC_VIN_SRC_GET_AGC_INFO error");
		return -1;
	}
	*step = (u64)0x6000000/sensor_db_info.db_step;
	return 0;
}

int get_is_info(image_sensor_param_t* info, char* sensor_name, sensor_config_t* config)
{
	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}
	u8 pattern = 0;
	u8 step = 0;
	u32 vin_fps;
	iav_sharpen_filter_cfg_t sharpen_filter;
	get_vin_frame_rate(&vin_fps);
	get_system_sharpen_filter_config(&sharpen_filter);
	switch (vin_info.sensor_id) {
		case SENSOR_AR0331:
		{
			amba_vin_sensor_op_mode op_mode;
			if (ioctl(fd_iav, IAV_IOC_VIN_GET_OPERATION_MODE, &op_mode) < 0) {
				perror("IAV_IOC_VIN_GET_OPERATION_MODE");
				return -1;
			}
			if(op_mode == AMBA_VIN_LINEAR_MODE)
			{
				memcpy(config ,&ar0331_linear_sensor_config,sizeof(sensor_config_t));
				info->p_adj_param = &ar0331_linear_adj_param;
				vin_op_mode = 0;
				info->p_awb_param = &ar0331_linear_awb_param;
				sprintf(sensor_name, "ar0331_linear");
			}
			else
			{
				printf("HDR MODE\n");
				memcpy(config ,&ar0331_sensor_config,sizeof(sensor_config_t));
				info->p_adj_param = &ar0331_adj_param;
				vin_op_mode = 1;	//HDR mode
				info->p_awb_param = &ar0331_awb_param;
				sprintf(sensor_name, "ar0331");
			}
			config->sensor_lb =AR_0331;
			info->p_rgb2yuv = ar0331_rgb2yuv;
			info->p_chroma_scale = &ar0331_chroma_scale;
			info->p_50hz_lines = ar0331_50hz_lines;
			info->p_60hz_lines = ar0331_60hz_lines;
			info->p_tile_config = &ar0331_tile_config;
			info->p_ae_agc_dgain = ar0331_ae_agc_dgain;
			info->p_ae_sht_dgain = ar0331_ae_sht_dgain;
			info->p_dlight_range = ar0331_dlight;
			info->p_manual_LE = ar0331_manual_LE;
			printf("AR0331\n");
			break;
		}
		case SENSOR_MT9T002:
			info->p_adj_param = &mt9t002_adj_param;
			info->p_rgb2yuv = mt9t002_rgb2yuv;
			info->p_chroma_scale = &mt9t002_chroma_scale;
			info->p_awb_param = &mt9t002_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mt9t002_60p50hz_lines;
				info->p_60hz_lines = mt9t002_60p60hz_lines;
			}else{
				info->p_50hz_lines = mt9t002_50hz_lines;
				info->p_60hz_lines = mt9t002_60hz_lines;
			}
			info->p_tile_config = &mt9t002_tile_config;
			info->p_ae_agc_dgain = mt9t002_ae_agc_dgain;
			info->p_ae_sht_dgain = mt9t002_ae_sht_dgain;
			info->p_dlight_range = mt9t002_dlight;
			info->p_manual_LE = mt9t002_manual_LE;
			memcpy(config ,&mt9t002_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =MT_9T002;
			sprintf(sensor_name, "mt9t002");
			break;
		case SENSOR_MN34041PL:
			info->p_adj_param = &mn34041pl_adj_param;
			info->p_rgb2yuv = mn34041pl_rgb2yuv;
			info->p_chroma_scale = &mn34041pl_chroma_scale;
			info->p_awb_param = &mn34041pl_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mn34041pl_60p50hz_lines;
				info->p_60hz_lines = mn34041pl_60p60hz_lines;
			}else{
				info->p_50hz_lines = mn34041pl_50hz_lines;
				info->p_60hz_lines = mn34041pl_60hz_lines;
			}
			info->p_tile_config = &mn34041pl_tile_config;
			info->p_ae_agc_dgain = mn34041pl_ae_agc_dgain;
			info->p_ae_sht_dgain = mn34041pl_ae_sht_dgain;
			info->p_dlight_range = mn34041pl_dlight;
			info->p_manual_LE = mn34041pl_manual_LE;
			memcpy(config ,&mn34041pl_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =MN_34041PL;
			sprintf(sensor_name, "mn34041pl");
			break;
		case SENSOR_MN34210PL:
			info->p_adj_param = &mn34210pl_adj_param;
			info->p_rgb2yuv = mn34210pl_rgb2yuv;
			info->p_chroma_scale = &mn34210pl_chroma_scale;
			info->p_awb_param = &mn34210pl_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mn34210pl_60p50hz_lines;
				info->p_60hz_lines = mn34210pl_60p60hz_lines;
			}else{
				info->p_50hz_lines = mn34210pl_50hz_lines;
				info->p_60hz_lines = mn34210pl_60hz_lines;
			}
			info->p_tile_config = &mn34210pl_tile_config;
			info->p_ae_agc_dgain = mn34210pl_ae_agc_dgain;
			info->p_ae_sht_dgain = mn34210pl_ae_sht_dgain;
			info->p_dlight_range = mn34210pl_dlight;
			info->p_manual_LE = mn34210pl_manual_LE;
			memcpy(config ,&mn34210pl_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =MN_34210PL;
			sprintf(sensor_name, "mn34210pl");
			break;
		case SENSOR_MN34220PL:
			info->p_adj_param = &mn34220pl_adj_param;
			info->p_rgb2yuv = mn34220pl_rgb2yuv;
			info->p_chroma_scale = &mn34220pl_chroma_scale;
			info->p_awb_param = &mn34220pl_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mn34220pl_60p50hz_lines;
				info->p_60hz_lines = mn34220pl_60p60hz_lines;
			}else{
				info->p_50hz_lines = mn34220pl_50hz_lines;
				info->p_60hz_lines = mn34220pl_60hz_lines;
			}
			info->p_tile_config = &mn34220pl_tile_config;
			info->p_ae_agc_dgain = mn34220pl_ae_agc_dgain;
			info->p_ae_sht_dgain = mn34220pl_ae_sht_dgain;
			info->p_dlight_range = mn34220pl_dlight;
			info->p_manual_LE = mn34220pl_manual_LE;
			memcpy(config ,&mn34220pl_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =MN_34220PL;
			sprintf(sensor_name, "mn34220pl");
			break;
		case SENSOR_IMX172:
			if(sharpen_filter.sharpen_b_enable)
				info->p_adj_param = &imx172_adj_param;
			else
				info->p_adj_param = &imx172_adj_param_shpA;
			info->p_rgb2yuv = imx172_rgb2yuv;
			info->p_chroma_scale = &imx172_chroma_scale;
			info->p_awb_param = &imx172_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = imx172_60p50hz_lines;
				info->p_60hz_lines =imx172_60p60hz_lines;
			}else{
				info->p_50hz_lines = imx172_50hz_lines;
				info->p_60hz_lines = imx172_60hz_lines;
			}
			info->p_tile_config = &imx172_tile_config;
			info->p_ae_agc_dgain = imx172_ae_agc_dgain;
			info->p_ae_sht_dgain = imx172_ae_sht_dgain;
			info->p_dlight_range = imx172_dlight;
			info->p_manual_LE = imx172_manual_LE;
			memcpy(config ,&imx172_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_172;
			sprintf(sensor_name, "imx172");
			break;
		case SENSOR_IMX178:
			info->p_adj_param = &imx178_adj_param;
			info->p_rgb2yuv = imx178_rgb2yuv;
			info->p_chroma_scale = &imx178_chroma_scale;
			info->p_awb_param = &imx178_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = imx178_60p50hz_lines;
				info->p_60hz_lines =imx178_60p60hz_lines;
			}else{
				info->p_50hz_lines = imx178_50hz_lines;
				info->p_60hz_lines = imx178_60hz_lines;
			}
			info->p_tile_config = &imx178_tile_config;
			info->p_ae_agc_dgain = imx178_ae_agc_dgain;
			info->p_ae_sht_dgain = imx178_ae_sht_dgain;
			info->p_dlight_range = imx178_dlight;
			info->p_manual_LE = imx178_manual_LE;
			memcpy(config ,&imx178_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_178;
			sprintf(sensor_name, "imx178");
			break;
		case SENSOR_IMX121:
			info->p_adj_param = &imx121_adj_param;
			info->p_rgb2yuv = imx121_rgb2yuv;
			info->p_chroma_scale = &imx121_chroma_scale;
			info->p_awb_param = &imx121_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = imx121_60p50hz_lines;
				info->p_60hz_lines =imx121_60p60hz_lines;
			}else{
				info->p_50hz_lines = imx121_50hz_lines;
				info->p_60hz_lines = imx121_60hz_lines;
			}
			info->p_tile_config = &imx121_tile_config;
			info->p_ae_agc_dgain = imx121_ae_agc_dgain;
			info->p_ae_sht_dgain = imx121_ae_sht_dgain;
			info->p_dlight_range = imx121_dlight;
			info->p_manual_LE = imx121_manual_LE;
			memcpy(config ,&imx121_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_121;
			sprintf(sensor_name, "imx121");
			break;
		case SENSOR_IMX123:
			info->p_adj_param = &imx123_adj_param;
			info->p_rgb2yuv = imx123_rgb2yuv;
			info->p_chroma_scale = &imx123_chroma_scale;
			info->p_awb_param = &imx123_awb_param;
				if(vin_fps ==AMBA_VIDEO_FPS_60){
					info->p_50hz_lines = imx123_60p50hz_lines;
					info->p_60hz_lines =imx123_60p60hz_lines;
				}else{
					info->p_50hz_lines = imx123_50hz_lines;
					info->p_60hz_lines =imx123_60hz_lines;
				}

			info->p_tile_config = &imx123_tile_config;
			info->p_ae_agc_dgain = imx123_ae_agc_dgain;
			info->p_ae_sht_dgain = imx123_ae_sht_dgain;
			info->p_dlight_range = imx123_dlight;
			info->p_manual_LE = imx123_manual_LE;
			memcpy(config ,&imx123_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_136;
			sprintf(sensor_name, "imx123");
			break;
		case SENSOR_IMX104:
			info->p_adj_param = &imx104_adj_param;
			info->p_rgb2yuv = imx104_rgb2yuv;
			info->p_chroma_scale = &imx104_chroma_scale;
			info->p_awb_param = &imx104_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = imx104_60p50hz_lines;
				info->p_60hz_lines =imx104_60p60hz_lines;
			}else{
				info->p_50hz_lines = imx104_50hz_lines;
				info->p_60hz_lines =imx104_60hz_lines;
			}
			info->p_tile_config = &imx104_tile_config;
			info->p_ae_agc_dgain =imx104_ae_agc_dgain;
			info->p_ae_sht_dgain = imx104_ae_sht_dgain;
			info->p_dlight_range =imx104_dlight;
			info->p_manual_LE = imx104_manual_LE;
			memcpy(config ,&imx104_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_104;
			sprintf(sensor_name, "imx104");
			break;
		case SENSOR_IMX136:
			info->p_adj_param = &imx136_adj_param;
			info->p_rgb2yuv = imx136_rgb2yuv;
			info->p_chroma_scale = &imx136_chroma_scale;
			info->p_awb_param = &imx136_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = imx136_60p50hz_lines;
				info->p_60hz_lines =imx136_60p60hz_lines;
			}else{
				info->p_50hz_lines = imx136_50hz_lines;
				info->p_60hz_lines =imx136_60hz_lines;
			}
			info->p_tile_config = &imx136_tile_config;
			info->p_ae_agc_dgain = imx136_ae_agc_dgain;
			info->p_ae_sht_dgain = imx136_ae_sht_dgain;
			info->p_dlight_range = imx136_dlight;
			info->p_manual_LE = imx136_manual_LE;
			memcpy(config ,&imx136_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_136;
			sprintf(sensor_name, "imx136");
			break;
		case SENSOR_OV2710:
			info->p_adj_param = &ov2710_adj_param;
			info->p_rgb2yuv = ov2710_rgb2yuv;
			info->p_chroma_scale = &ov2710_chroma_scale;
			info->p_awb_param = &ov2710_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = ov2710_60p50hz_lines;
				info->p_60hz_lines =ov2710_60p60hz_lines;
			}else{
				info->p_50hz_lines = ov2710_50hz_lines;
				info->p_60hz_lines =ov2710_60hz_lines;
			}
			info->p_tile_config = &ov2710_tile_config;
			info->p_ae_agc_dgain = ov2710_ae_agc_dgain;
			info->p_ae_sht_dgain = ov2710_ae_sht_dgain;
			info->p_dlight_range =ov2710_dlight;
			info->p_manual_LE = ov2710_manual_LE;
			memcpy(config ,&ov2710_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =OV_2710;
			sprintf(sensor_name, "ov2710");
			break;
		case SENSOR_IMX185:
			info->p_adj_param = &imx185_adj_param;
			info->p_rgb2yuv = imx185_rgb2yuv;
			info->p_chroma_scale = &imx185_chroma_scale;
			info->p_awb_param = &imx185_awb_param;
			info->p_50hz_lines = imx185_50hz_lines;
			info->p_60hz_lines = imx185_60hz_lines;
			info->p_tile_config = &imx185_tile_config;
			info->p_ae_agc_dgain = imx185_ae_agc_dgain;
			info->p_ae_sht_dgain = imx185_ae_sht_dgain;
			info->p_dlight_range = imx185_dlight;
			info->p_manual_LE = imx185_manual_LE;
			memcpy(config ,&imx185_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_185;
			sprintf(sensor_name, "imx185");
			break;
		case SENSOR_IMX226:
			if(sharpen_filter.sharpen_b_enable)
				info->p_adj_param = &imx226_adj_param;
			else
				info->p_adj_param = &imx226_adj_param_shpA;
			info->p_rgb2yuv = imx226_rgb2yuv;
			info->p_chroma_scale = &imx226_chroma_scale;
			info->p_awb_param = &imx226_awb_param;
			info->p_50hz_lines = imx226_50hz_lines;
			info->p_60hz_lines = imx226_60hz_lines;
			info->p_tile_config = &imx226_tile_config;
			info->p_ae_agc_dgain = imx226_ae_agc_dgain;
			info->p_ae_sht_dgain = imx226_ae_sht_dgain;
			info->p_dlight_range = imx226_dlight;
			info->p_manual_LE = imx226_manual_LE;
			memcpy(config ,&imx226_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_226;
			sprintf(sensor_name, "imx226");
			break;
		case SENSOR_OV5658:
			info->p_adj_param = &ov5658_adj_param;
			info->p_rgb2yuv = ov5658_rgb2yuv;
			info->p_chroma_scale = &ov5658_chroma_scale;
			info->p_awb_param = &ov5658_awb_param;
			info->p_50hz_lines = ov5658_50hz_lines;
			info->p_60hz_lines = ov5658_60hz_lines;
			info->p_tile_config = &ov5658_tile_config;
			info->p_ae_agc_dgain = ov5658_ae_agc_dgain;
			info->p_ae_sht_dgain = ov5658_ae_sht_dgain;
			info->p_dlight_range = ov5658_dlight;
			info->p_manual_LE = ov5658_manual_LE;
			memcpy(config ,&ov5658_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =OV_5658;
			sprintf(sensor_name, "ov5658");
			break;
		case SENSOR_OV4689:
			info->p_adj_param = &ov4689_adj_param;
			info->p_rgb2yuv = ov4689_rgb2yuv;
			info->p_chroma_scale = &ov4689_chroma_scale;
			info->p_awb_param = &ov4689_awb_param;
			info->p_50hz_lines = ov4689_50hz_lines;
			info->p_60hz_lines = ov4689_60hz_lines;
			info->p_tile_config = &ov4689_tile_config;
			info->p_ae_agc_dgain = ov4689_ae_agc_dgain;
			info->p_ae_sht_dgain = ov4689_ae_sht_dgain;
			info->p_dlight_range = ov4689_dlight;
			info->p_manual_LE = ov4689_manual_LE;
			memcpy(config ,&ov4689_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =OV_4689;
			sprintf(sensor_name, "ov4689");
			break;

		default:
			printf("sensor Id [%d].\n", vin_info.sensor_id);
			return -1;
	}

	get_sensor_bayer_pattern(&pattern);
	get_sensor_step(&step);

	config->pattern = (bayer_pattern)pattern;
	config->gain_step = step;
	return 0;
}

static int load_dsp_cc_table(int fd_iav)
{
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8* reg, *matrix, *sec_cc;
	char filename[128];
	int file, count;
	int rval = 0;

	reg = malloc(CC_REG_SIZE);
	matrix = malloc(CC_3D_SIZE);
	sec_cc = malloc(SEC_CC_SIZE);
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("reg.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("3D.bin cannot be opened\n");
		return -1;
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;

	rval = img_dsp_set_color_correction_reg(&color_corr_reg);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_color_correction(fd_iav, &color_corr);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_tone_curve(fd_iav, &tone_curve);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_sec_cc_en(fd_iav, 0);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_enable_color_correction(fd_iav);
	if (rval < 0) {
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

	if (file >= 0) {
		close(file);
		file = -1;
	}
	return rval;
}

static int load_adj_cc_table(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[MATRIX_SIZE];
	u8 i, adj_mode = 4;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename, "%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("Open %s error!\n", filename);
			return -1;
		}
		if ((count = read(file, matrix, MATRIX_SIZE)) != MATRIX_SIZE) {
			printf("Read %s error!\n", filename);
			close(file);
			return -1;
		}
		close(file);

		if (img_adj_load_cc_table((u32)matrix, i) < 0) {
			printf("img_ad_load_cc_table error!\n");
			return -1;
		}
	}
	return 0;
}

int load_mctf_bin()
{
	int rval = -1;
	int i;
	int file, count;
	u8* bin_buff;
	char filename[256];
	idsp_one_def_bin_t one_bin;
	idsp_def_bin_t bin_map;
#define MCTF_BIN_SIZE	27600

	img_dsp_get_default_bin_map(&bin_map);
	bin_buff = malloc(MCTF_BIN_SIZE);
	for (i = 0; i<bin_map.num;i++)
	{
		memset(filename,0,sizeof(filename));
		memset(bin_buff, 0, MCTF_BIN_SIZE);
		sprintf(filename, "%s/%s", IMGPROC_PARAM_PATH, bin_fn[i]);
		if((file = open(filename, O_RDONLY, 0))<0) {
			printf("%s cannot be opened\n",filename);
			return -1;
		}
		if((count = read(file, bin_buff, bin_map.one_bin[i].size)) != bin_map.one_bin[i].size) {
			printf("read %s error\n",filename);
			return -1;
		}
		one_bin.size = bin_map.one_bin[i].size;
		one_bin.type = bin_map.one_bin[i].type;
		rval = img_dsp_load_default_bin((u32)bin_buff, one_bin);
		CHECK_RVAL
	}
	free(bin_buff);
	return 0;
}

int start_aaa_foo(int fd_iav, char* sensor_name, image_sensor_param_t app_param_is)
{
	aaa_api_t custom_aaa_api = {0};
	int rval = -1;

	rval = load_dsp_cc_table(fd_iav);
	CHECK_RVAL
	rval = load_adj_cc_table(sensor_name);
	CHECK_RVAL
	rval = img_load_image_sensor_param(&app_param_is);
	CHECK_RVAL
	rval = img_register_aaa_algorithm(custom_aaa_api);
	CHECK_RVAL
	rval = img_start_aaa(fd_iav);
	CHECK_RVAL
	rval = img_set_work_mode(0);
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);
	img_set_chroma_noise_filter_max_radius(32<<resource_setup.max_chroma_noise_shift);
	CHECK_RVAL
	return 0;
}

void show_lib_version()
{
	img_lib_version_t img_algo_info;
	img_lib_version_t img_dsp_info;

	img_algo_get_lib_version(&img_algo_info);
	printf("\n[Algo lib info]:\n");
	printf("   Algo lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		img_algo_info.description, img_algo_info.major,
		img_algo_info.minor, img_algo_info.patch,
		img_algo_info.mod_time);
	img_dsp_get_lib_version(&img_dsp_info);
	printf("\n[DSP lib info]:\n");
	printf("   DSP lib Version : %s-%d.%d.%d (Last updated: %x)\n\n",
		img_dsp_info.description, img_dsp_info.major,
		img_dsp_info.minor, img_dsp_info.patch,
		img_dsp_info.mod_time);
}

static int start_aaa(void)
{
	static image_sensor_param_t app_param_image_sensor;
	static sensor_config_t sensor_config_info;
	static int vin_op_mode = AMBA_VIN_LINEAR_MODE;	//VIN is linear or HDR
	static lens_ID lens_mount_id = LENS_CMOUNT_ID;
	static lens_param_t lens_param_info = {
		{NULL, NULL, 0, NULL},
	};
	static lens_cali_t lens_cali_info = {
		NORMAL_RUN,
		{92},
	};

	int rval=-1;
	char sensor_name[32];
	img_config_info_t img_config_info;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if(img_config_working_status(fd_iav, &img_config_info) < 0){
		printf("error: img_config_working_status\n");
		return -1;
	}
	rval = img_lib_init(img_config_info.defblc_enable, img_config_info.sharpen_b_enable);
	CHECK_RVAL;
	show_lib_version();
	rval = load_mctf_bin();
	CHECK_RVAL;

	rval = get_is_info(&app_param_image_sensor, sensor_name, &sensor_config_info);
	CHECK_RVAL;

	if (img_config_sensor_info(&sensor_config_info) < 0) {
		return -1;
	}
	if (img_config_sensor_hdr_mode(vin_op_mode) < 0) {
		return -1;
	}
	if(img_config_lens_info(lens_mount_id) < 0) {
		return -1;
	}
	if(img_load_lens_param(&lens_param_info) < 0) {
		return -1;
	}
	if(img_lens_init() < 0) {
		return -1;
	}

	rval = start_aaa_foo(fd_iav, sensor_name, app_param_image_sensor);
	CHECK_RVAL;
	if(img_config_lens_cali(&lens_cali_info) < 0) {
		return -1;
	}
	sleep(1);
	return 0;
}

int awb_cali_get_current_gain(void)
{
	if (img_awb_set_method(AWB_MANUAL) < 0) {
		printf("img_awb_set_method error!\n");
		return -1;
	}
	printf("\nWait to get current White Balance Gain...\n");
	sleep(2);

	img_awb_get_wb_cal(&wb_gain);
	printf("Current Red Gain %d, Green Gain %d, Blue Gain %d.\n",
		wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);

	if (img_awb_set_method(AWB_NORMAL) < 0) {
		printf("img_awb_set_method error!\n");
		return -1;
	}
	return 0;
}

int awb_cali_correct_gain(void)
{
	wb_gain_t orig[2], target[2];
	int thre_r, thre_b;
	int low_r, low_b, high_r, high_b;
	int i;

	for (i = 0; i < 2; i++) {
		orig[i].r_gain = correct_param[i*3];
		orig[i].g_gain = correct_param[i*3+1];
		orig[i].b_gain = correct_param[i*3+2];
	}

	for (i = 0; i < 2; i++) {
		target[i].r_gain = correct_param[i*3+6];
		target[i].g_gain = correct_param[i*3+7];
		target[i].b_gain = correct_param[i*3+8];
	}

	low_r = target[0].r_gain - orig[0].r_gain;
	low_r = ABS(low_r);
	high_r = target[1].r_gain - orig[1].r_gain;
	high_r = ABS(high_r);
	low_b = target[0].b_gain - orig[0].b_gain;
	low_b = ABS(low_b);
	high_b = target[1].b_gain-orig[1].b_gain;
	high_b = ABS(high_b);

	thre_r = MAX(low_r, high_r);
	thre_b = MAX(low_b, high_b);

	if (img_awb_set_cali_diff_thr(thre_r, thre_b) < 0) {
		printf("img_awb_set_cali_diff_thr error!\n");
		return -1;
	}
	if (img_awb_set_wb_shift(orig, target) < 0) {
		printf("img_awb_set_wb_shift error!\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	FILE *fp;

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (start_aaa() < 0)
		return -1;
	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	sleep(1);
	if (detect_flag) {
		if (awb_cali_get_current_gain() < 0)
			return -1;
		if (save_file_flag) {
			if ((fp = fopen(cali_awb_filename, "a+")) == NULL) {
				printf("Open file %s error. Save the result to default file %s.\n", cali_awb_filename, default_filename);

				if ((fp = fopen(default_filename, "a+")) == NULL) {
					printf("Save failed.\n");
				}
			}
			if (fp != NULL) {
				fprintf(fp, "%d:%d:%d\n", wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);
				fclose(fp);
			}
		}
	}

	if (correct_flag) {
		if (awb_cali_correct_gain() < 0)
			return -1;
	}

	if (correct_flag || restore_flag)
		while (1)
			sleep(10);

	return 0;
}
