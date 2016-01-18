#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "types.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "ambas_vin.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"

#include "ar0331_adj_param.c"
#include "ar0331_aeb_param.c"

#define TUNING_printf(...)
#define NO_ARG	0
#define HAS_ARG	1
#define UNIT_chroma_scale (64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#define CHECK_RVAL do{if(rval<0)	{\
	printf("error: %s,%s:%d\n",__FILE__,__func__,__LINE__);\
	return -1;}}while(0)

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

static const char* short_options = "m:s:";
static struct option long_options[] = {
	{"multi-vin-ena", HAS_ARG, 0, 'm'},
	{"statictics-display-type", HAS_ARG, 0, 's'},
	{0, 0, 0, 0},
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1", "\tmulti-CFA vin option"},
	{"0~3", "\tdisplay statistics type option, 0-AWB, 1-AE, 2-AF, 3-embeded-hist"},
	{0, 0},
};

void usage(void){
	int i;

	printf("test_idsp usage:\n");
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
}

typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_SENSOR_HIST,
} STATISTICS_TYPE;

static STATISTICS_TYPE G_display_type = STATIS_AE;

u8 *bsb_mem;
u32 bsb_size;
int fd_iav;
u8 multi_vin_flag = 0, multi_vin_ena = 0;
static int G_multi_vin_num = 1;

int map_bsb(int fd_iav)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB2");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
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

static int display_AF_data(af_stat_t * pAf_data,
	u16 tile_num_col, u16 tile_num_row)
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

static int display_sensor_hist_data(embed_hist_stat_t  *pembeded_hist)
{
	int i, j;

	#define HIST_BIN_TOTAL_NUM		244

	printf("== embed_hist stat: valid[%d], frame_cnt[%d], frame_id[%d] ==\n",
		pembeded_hist->valid, pembeded_hist->frame_cnt, pembeded_hist->frame_id);

	for (i = 0; i < HIST_BIN_TOTAL_NUM / 8; ++i) {
		for (j = 0; j < 8; ++j) {
			printf("  %5d  ", pembeded_hist->hist_bin_data[i * 8 + j]);
	}
		printf("\n");
	}

	return 0;
}

int get_multi_vin_num(void)
{
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;

	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		return -1;
	}
	G_multi_vin_num = resource_setup.vin_num;
	printf("vin num =%d\n", G_multi_vin_num);
	return 0;
}

int display_statistics_data_multi_vin(img_aaa_stat_t *pStatistics_data)
{
	int retv = 0;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;
	awb_data_t * pawb_info = NULL;
	ae_data_t * pae_info = NULL;
	af_stat_t * paf_info = NULL;
	cfa_histogram_stat_t	*pCfa_hist = NULL;
	rgb_histogram_stat_t	*pRgb_hist = NULL;
	embed_hist_stat_t 	*pSensor_hist = NULL;

	printf("\t total channel num:%d, channel_idx:%d\n",
		pStatistics_data->tile_info.total_channel_num,
		pStatistics_data->tile_info.channel_idx);

	awb_tile_row = pStatistics_data->tile_info.awb_tile_num_row;
	awb_tile_col = pStatistics_data->tile_info.awb_tile_num_col;
	ae_tile_row = pStatistics_data->tile_info.ae_tile_num_row;
	ae_tile_col = pStatistics_data->tile_info.ae_tile_num_col;
	af_tile_row = pStatistics_data->tile_info.af_tile_num_row;
	af_tile_col = pStatistics_data->tile_info.af_tile_num_col;

	pawb_info = (awb_data_t *)pStatistics_data->awb_info;
	pae_info = (ae_data_t *)((u8 *)pawb_info + awb_tile_row *
		awb_tile_col * sizeof(awb_data_t));
	paf_info = (af_stat_t *)((u8 *)pae_info + ae_tile_row *
		ae_tile_col * sizeof(ae_data_t));

	pCfa_hist = (cfa_histogram_stat_t *)((u8 *)paf_info + af_tile_row *
		af_tile_col * sizeof(af_stat_t));
	pRgb_hist = (rgb_histogram_stat_t *)((u8 *)pCfa_hist + sizeof(cfa_histogram_stat_t));
	pSensor_hist = (embed_hist_stat_t *)((u8 *)pRgb_hist + sizeof(rgb_histogram_stat_t));

	switch (G_display_type) {
	case STATIS_AWB:
		retv = display_AWB_data(pawb_info, awb_tile_col, awb_tile_row);
		break;
	case STATIS_AE:
		retv = display_AE_data(pae_info, ae_tile_col, ae_tile_row);
		break;
	case STATIS_AF:
		retv = display_AF_data(paf_info, af_tile_col, af_tile_row);
		break;
	case STATIS_SENSOR_HIST:
		retv = display_sensor_hist_data(pSensor_hist);
		break;
	default:
		printf("Invalid statistics type !\n");
		retv = -1;
		break;
	}

	return retv;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	int i;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'm':
			multi_vin_flag = 1;
			multi_vin_ena = atoi(optarg);
			break;
		case 's':
			i = atoi(optarg);
			if ( i < 0  || i > 3) {
				printf("The value must be 0~3\n");
				break;
			}
			G_display_type = i;
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static void sigstop()
{
//	img_stop_aaa();
	printf("3A is off.\n");
	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	img_config_info_t img_config_info;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0){
		printf("error: init_param\n");
		return -1;
	}
	if (map_bsb(fd_iav) < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	if(multi_vin_flag){
		if (get_multi_vin_num() < 0) {
			return -1;
		}
		img_aaa_stat_t multi_vin_stat[G_multi_vin_num];
		aaa_tile_report_t multi_vin_tile[G_multi_vin_num];
		int i = 0;
		if(img_config_working_status(fd_iav, &img_config_info) < 0){
			printf("error: img_config_working_status\n");
			return -1;
		}
		if(img_lib_init(img_config_info.defblc_enable, img_config_info.sharpen_b_enable) < 0){
			printf("error: img_lib_init\n");
			return -1;
		}

		img_dsp_config_statistics_info(fd_iav, &ar0331_tile_config);

		while(1){
			img_dsp_get_statistics_multi_vin(fd_iav, multi_vin_stat, multi_vin_tile);
			// deal with the 3A process
			for (i = 0; i < G_multi_vin_num; i++) {
				display_statistics_data_multi_vin(&(multi_vin_stat[i]));
			}
			sleep(1);
		}
	}

	return 0;
}
