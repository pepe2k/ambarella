#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_struct_arch.h"
#include "ambas_vin.h"
#include "img_api_arch.h"
#include "img_dsp_interface_arch.h"

#define BAD_PIXEL_CALI_FILENAME			"bpcmap.bin"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

static const char *default_filename = BAD_PIXEL_CALI_FILENAME;
static char filename[256];

static int fd_iav;

static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int mask_flag = 0;

static cali_badpix_setup_t badpixel_detect_algo;
static fpn_correction_t fpn;

static int detect_param[10];
static int correct_param[2];
static int restore_param[2];

#define NO_ARG			0
#define HAS_ARG			1

static struct option long_options[] = {
	{"detect", HAS_ARG, 0, 'd'},
	{"correct", HAS_ARG, 0, 'c'},
	{"restore", HAS_ARG, 0, 'r'},
	{"filename", HAS_ARG, 0, 'f'},
	{"privacy-mask", NO_ARG, 0, 'm'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "d:c:r:f:m";

static const struct hint_s hint[] = {
	{"", "\t\t10 params for bad pixel detect"},
	{"", "\t\t2 params for bad pixel correct"},
	{"", "\t\t2 params for bad pixel restore"},
	{"filename", "specify location for saving calibration result"},
	{"", "\t\ttry add a privacy mask"},
};

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
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			if (get_multi_arg(optarg, detect_param, ARRAY_SIZE(detect_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(detect_param), ch);
				return -1;
			}
			break;

		case 'c':
			correct_flag = 1;
			if (get_multi_arg(optarg, correct_param, ARRAY_SIZE(correct_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(correct_param), ch);
				return -1;
			}
			break;

		case 'r':
			restore_flag = 1;
			if (get_multi_arg(optarg, restore_param, ARRAY_SIZE(restore_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(restore_param), ch);
				return -1;
			}
			break;

		case 'f':
			strcpy(filename, optarg);
			break;

		case 'm':
			mask_flag = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

int get_vin_info(u32 *width, u32 *height, sensor_config_t *config)
{
	int vin_mode;
	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, &vin_mode)) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
		return -1;
	}

	switch (vin_mode) {
	case AMBA_VIDEO_MODE_720P:
		*width = 1280;
		*height = 720;
		break;
	case AMBA_VIDEO_MODE_WXGA:
		*width = 1280;
		*height = 800;
		break;
	case AMBA_VIDEO_MODE_1080P:
		*width = 1920;
		*height = 1080;
		break;
	case AMBA_VIDEO_MODE_XGA:
		*width = 1024;
		*height = 768;
		break;
	case AMBA_VIDEO_MODE_QSXGA:
		*width = 2592;
		*height = 1944;
		break;
	case AMBA_VIDEO_MODE_QXGA:
		*width = 2048;
		*height = 1536;
		break;
	case AMBA_VIDEO_MODE_3840x2160:
		*width = 3840;
		*height = 2160;
		break;
	case AMBA_VIDEO_MODE_4096x2160:
		*width = 4096;
		*height = 2160;
		break;
	case AMBA_VIDEO_MODE_4000x3000:
		*width = 4000;
		*height = 3000;
		break;
	case AMBA_VIDEO_MODE_3072_2048:
		*width = 3072;
		*height = 2048;
		break;
	case AMBA_VIDEO_MODE_2560_2048:
		*width = 2560;
		*height = 2048;
		break;
	case AMBA_VIDEO_MODE_1536_1024:
		*width = 1536;
		*height = 1024;
		break;
	case AMBA_VIDEO_MODE_2304x1536:
		*width = 2304;
		*height = 1536;
		break;
	case AMBA_VIDEO_MODE_2304x1296:
		*width = 2304;
		*height = 1296;
		break;
	default:
		printf("unrecogized vin mode %d\n", vin_mode);
		return -1;
	}

	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	switch (vin_info.sensor_id) {
	case SENSOR_AR0331:
		config->sensor_lb = AR_0331;
		break;
	case SENSOR_MT9T002:
		config->sensor_lb = MT_9T002;
		break;
	case SENSOR_MN34041PL:
		config->sensor_lb = MN_34041PL;
		break;
	case SENSOR_MN34210PL:
		config->sensor_lb = MN_34210PL;
		break;
	case SENSOR_MN34220PL:
		config->sensor_lb = MN_34220PL;
		break;
	case SENSOR_IMX172:
		config->sensor_lb = IMX_172;
		break;
	case SENSOR_IMX178:
		config->sensor_lb = IMX_178;
		break;
	case SENSOR_IMX121:
		config->sensor_lb = IMX_121;
		break;
	case SENSOR_IMX104:
		config->sensor_lb = IMX_104;
		break;
	case SENSOR_IMX136:
		config->sensor_lb = IMX_136;
		break;
	case SENSOR_OV2710:
		config->sensor_lb = OV_2710;
		break;
	default:
		printf("sensor Id [%d].\n", vin_info.sensor_id);
		return -1;
	}

	struct amba_video_info video_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	config->pattern = video_info.pattern;
	config->max_gain_db = 48;	// only suitible for all sensors. the max 48dB.
	config->gain_step = 60;	// only suitible for all sensors.
	config->shutter_delay = 2;	// only suitible for all sensors. delay 2 frames.
	config->agc_delay = 2;	// only suitible for all sensors. delay 2 frames.
	return 0;
}
static sensor_config_t sensor_config_info;

int main(int argc, char **argv)
{
	u32					cap_width;
	u32					cap_height;
	int 					fd_bpc = -1;
	u8* 					fpn_map_addr = NULL;
	u32					fpn_map_size = 0;
	u32 					bpc_num = 0;
	u32 					raw_pitch;
	u32 					width = 0;
	u32 					height = 0;
	int					count = -1;
	static cfa_noise_filter_info_t 	cfa_noise_filter;
	static dbp_correction_t 		bad_corr;

	bad_corr.dark_pixel_strength = 0;
	bad_corr.hot_pixel_strength = 0;
	bad_corr.enable = 0;
	memset(&cfa_noise_filter, 0, sizeof(cfa_noise_filter));

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if(img_lib_init(1,0)<0) {
		perror("img_lib init failed.\n");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	if (get_vin_info(&cap_width, &cap_height, &sensor_config_info) < 0) {
		printf("get_vin_info error\n");
		return -1;
	}

	if (img_config_sensor_info(&sensor_config_info) < 0) {
		return -1;
	}
	img_config_lens_info(LENS_CMOUNT_ID);
	img_lens_init();

	if (detect_flag) {
		width = detect_param[0];
		height = detect_param[1];
	} else if (correct_flag) {
		width = correct_param[0];
		height = correct_param[1];
	} else if (restore_flag) {
		width = restore_param[0];
		height = restore_param[1];
	} else {
		width = cap_width;
		height = cap_height;
	}

	if (width != cap_width || height != cap_height) {
		printf("badpixel map size %dx%d doesn't match sensor capture window %dx%d!",
			width, height, cap_width, cap_height);
		return -1;
	}

	raw_pitch = ROUND_UP(width/8, 32);
	fpn_map_size = raw_pitch*height;
	fpn_map_addr = malloc(fpn_map_size);
	if (fpn_map_addr < 0) {
		printf("can not malloc memory for bpc map\n");
		return -1;
	}
	memset(fpn_map_addr,0,(fpn_map_size));

	if (detect_flag) {
		// disable dynamic bad pixel correction
		img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
		img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
		img_dsp_set_anti_aliasing(fd_iav, 0);

		badpixel_detect_algo.cap_width = detect_param[0];
		badpixel_detect_algo.cap_height= detect_param[1];
		badpixel_detect_algo.block_w= detect_param[2];
		badpixel_detect_algo.block_h = detect_param[3];
		badpixel_detect_algo.badpix_type = detect_param[4];
		badpixel_detect_algo.upper_thres= detect_param[5];
		badpixel_detect_algo.lower_thres= detect_param[6];
		badpixel_detect_algo.detect_times = detect_param[7];
		badpixel_detect_algo.agc_idx = detect_param[8];
		badpixel_detect_algo.shutter_idx= detect_param[9];
		badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
		badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

		bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
		printf("Total number is %d\n", bpc_num);

		if((fd_bpc = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			printf("map file open error!\n");
		}
		write(fd_bpc, fpn_map_addr, fpn_map_size);
		free(fpn_map_addr);
	}

	if (correct_flag || restore_flag) {
		// disable dynamic bad pixel correction
		img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
		img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
		img_dsp_set_anti_aliasing(fd_iav, 0);

		if (correct_flag) {
			if((fd_bpc = open(filename, O_RDONLY, 0)) < 0) {
				printf("bpcmap.bin cannot be opened\n");
				return -1;
			}
			if((count = read(fd_bpc, fpn_map_addr, fpn_map_size)) != fpn_map_size) {
				printf("read bpcmap.bin error\n");
				return -1;
			}
			fpn.enable = 3;
		} else if (restore_flag) {
			fpn.enable = 0;
		}

		fpn.fpn_pitch = raw_pitch;
		fpn.pixel_map_height = height;
		fpn.pixel_map_width = width;
		fpn.pixel_map_size = fpn_map_size;
		fpn.pixel_map_addr = (u32)fpn_map_addr;
		img_dsp_set_static_bad_pixel_correction(fd_iav, &fpn);
		free(fpn_map_addr);
	}

	if (mask_flag) {	//try add a area of privacy mask
		int i;
		memset(fpn_map_addr,0,(fpn_map_size));
		for (i = raw_pitch*400; i < raw_pitch*600; i++) {
			if ((i % raw_pitch) > 100 && (i % raw_pitch) < 150)
				fpn_map_addr[i] = 0xff;
		}
		fpn.enable = 3;
		fpn.fpn_pitch = raw_pitch;
		fpn.pixel_map_height = height;
		fpn.pixel_map_width = width;
		fpn.pixel_map_size = fpn_map_size;
		fpn.pixel_map_addr = (u32)fpn_map_addr;
		img_dsp_set_static_bad_pixel_correction(fd_iav, &fpn);
		free(fpn_map_addr);
	}

	return 0;
}

