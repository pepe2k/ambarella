
/**********************************************************
 * test_tuning.c
 *
 * History:
 *	2012/12/24 - [Teng Huang] created
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************/
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
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "types.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#ifdef BUILD_AMBARELLA_EIS
#include "ambas_eis.h"
#endif
#include "ambas_vin.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"
#include <sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<pthread.h>
#include <semaphore.h>
#include <math.h>
#include"test_tuning.h"
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
//#include "imx185_1_adj_param.c"
#include "imx185_aeb_param.c"

#include "imx226_adj_param.c"
#include "imx226_aeb_param.c"

#include "ov5658_adj_param.c"
#include "ov5658_aeb_param.c"

#include "ov4689_adj_param.c"
#include "ov4689_aeb_param.c"

#include "m13vp288ir_piris_param.c"

static image_sensor_param_t app_param_image_sensor;
static sensor_config_t sensor_config_info;

static const char* short_options = "aI:D:";
static struct option long_options[] = {
	{"3a", NO_ARG, 0, 'a'},
	{"lens_mount_id", HAS_ARG, 0, 'I'},
	{"debug_mode", HAS_ARG, 0, 'D'},
	{0, 0, 0, 0},
};
static bayer_pattern pattern;
static u8  start_aaa_flag=0;
static u8 anti_aliasing_strength;

static BLC_INFO blc_info;
static EC_INFO ec_info;

static TILE_PKG tile_pkg;
static SHARPEN_PKG sharpen_pkg;
static HDR_CONTRAST_PKG hdr_contrast_pkg;
static SA_ASF_PKG sa_asf_pkg;
static wb_gain_t wb_gain;
static u32 dgain;
static chroma_median_filter_t chroma_median_setup;

static MCTF_INFO mctf_info;
static luma_high_freq_nr_t luma_hfnr_strength;
static rgb_to_yuv_t rgb2yuv_matrix;
static sharpen_level_t sharpen_setting_min;
static sharpen_level_t sharpen_setting_overall;
static digital_sat_level_t  d_gain_satuation_level;
static dbp_correction_t dbp_correction_setting;

static af_statistics_ex_t af_statistic_setup_ex;
static aaa_cntl_t aaa_cntl_station;
static rgb_aaa_stat_t p_rgb_stat;
static cfa_aaa_stat_t  p_cfa_stat;
static cfa_noise_filter_info_t cfa_noise_filter;
static chroma_filter_t chroma_noise_filter;
static  gmv_info_t gmv_info_setting;

extern u16 sensor_double_step;
static color_correction_reg_t color_corr_reg;
static color_correction_t color_corr;
static cdnr_info_t cdnr_info;

static cfa_leakage_filter_t cfa_leakage_filter;
static max_change_t max_change;
//static spatial_filter_t spatial_filter_info;
static fir_t fir = {-1, {0, -1, 0, 0, 0, 8, 0, 0, 0, 0}};
static u8 retain_level = 1;
static u16 linearization_strenght;
static u8 *bsb_mem;
static u32 bsb_size;
static chroma_scale_filter_t cs;
static img_aaa_stat_t aaa_stat_tuning;
static histogram_stat_t dsp_histo_info;
static AAA_INFO_PKG aaa_info_pkg;
static aaa_tile_report_t act_tile;
static int vin_op_mode = AMBA_VIN_LINEAR_MODE;	//VIN is linear or HDR
static lens_ID lens_mount_id = LENS_CMOUNT_ID;
static lens_cali_t lens_cali_info = {
	NORMAL_RUN,
	{92},
};
static lens_param_t lens_param_info = {
	{NULL, NULL, 0, NULL},
};
static FPN_PKG fpn_pkg;
static fpn_correction_t fpn;
static cali_badpix_setup_t badpixel_detect_algo;
static LENS_CALI_PKG lens_cali_pkg;
static vignette_info_t vignette_info = {0};
#if 0
static u16 le_gain_curve[256];
static u16 tone_curve_table[256];
static  u8 * uv_buffer_clipped;
static u8 * y_buffer_clipped;
static int uv_width, uv_height;
static statistics_config_t aaa_statistic_config;
#endif
static void sigstop()
{
	img_stop_aaa();
	printf("3A is off.\n");
	exit(1);
}


int fd_iav;
static int map_buffer(void)
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
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0)
	{
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}
	mem_mapped = 1;
	return 0;
}

static int receive_msg(int sock_fd,u8* buff,int size)
{
	int length =0;
	while(length < size)
	{
		int retv=0;
	 	retv=recv(sock_fd,buff+length,size - length,0);
		if(retv<=0)
		{
			if (retv == 0)
			{
				printf("Port  closed\n");
				return -2;
			}
			printf("recv() returns %d\n", retv);
			return -1;
		}
		length+=retv;
	//	printf("length %d , size %d\n",length,size);
	}
	return length;
}
static int send_msg(int sock_fd,u8* buff,int size)
{
	int length =0;
	while(length < size)
	{
		int tosend =2048;
		int remain=size -length;
		if(remain<2048)
			tosend =remain;
		int retv=0;
	 	retv=send(sock_fd,buff+length,tosend,MSG_NOSIGNAL);
		if(retv<=0)
		{
			if (retv == 0)
			{
				printf("Port  closed\n");
				return -2;
			}
			printf("send() returns %d\n", retv);
			return -1;
		}
		length+=retv;
	//	printf("length =%d\n",length);
	}
	return length;
}
#if 0
static inline int remove_padding_from_pitched_y
	(u8* output_y, const u8* input_y, int pitch, int width, int height)
{
	int row;
	for (row = 0; row < height; row++) { 		//row
		memcpy(output_y, input_y, width);
		input_y = input_y + pitch;
		output_y = output_y + width ;
	}
	return 0;
}

static inline int remove_padding_and_deinterlace_from_pitched_uv
	(u8* output_uv, const u8* input_uv, int pitch, int width, int height)
{
	int row, i;
	u8 * output_v = output_uv;
	u8 * output_u = output_uv + width * height;	//without padding

	for (row = 0; row < height; row++) { 		//row
		for (i = 0; i < width; i++) {			//interlaced UV row
			*output_u++ = *input_uv++;   	//U Buffer
			*output_v++ =  *input_uv++;	//V buffer
		}
		input_uv += (pitch - width) * 2;		//skip padding
	}
	return 0;
}
static int get_preview_buffer(int fd)
{
	iav_yuv_buffer_info_ex_t preview_buffer_info;
	int uv_pitch;
	const int yuv_format = 1;  // 0 is yuv422, 1 is yuv420
	int quit_yuv_stream=0;
	while (!quit_yuv_stream)
	{
		preview_buffer_info.source= 1;
		if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &preview_buffer_info) < 0)
		{
			if (errno == EINTR)
			{
				continue;		/* back to for() */
			} else
			{
				perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
				return -1;
			}
		}
		if (preview_buffer_info.pitch == preview_buffer_info.width)
		{
			memcpy(y_buffer_clipped, preview_buffer_info.y_addr,
				preview_buffer_info.width * preview_buffer_info.height);
		}
		else if (preview_buffer_info.pitch > preview_buffer_info.width)
		{
			remove_padding_from_pitched_y(y_buffer_clipped,
				preview_buffer_info.y_addr, preview_buffer_info.pitch,
				preview_buffer_info.width, preview_buffer_info.height);
		} else
		{
			printf("pitch size smaller than width!\n");
			return -1;
		}
		//convert uv data from interleaved into planar format
		if (yuv_format == 1)
		{
			uv_pitch = preview_buffer_info.pitch / 2;
			uv_width = preview_buffer_info.width / 2;
			uv_height = preview_buffer_info.height / 2;
		}
		else
		{
			uv_pitch = preview_buffer_info.pitch / 2;
			uv_width = preview_buffer_info.width / 2;
			uv_height = preview_buffer_info.height;
		}
		remove_padding_and_deinterlace_from_pitched_uv(uv_buffer_clipped,
			preview_buffer_info.uv_addr, uv_pitch, uv_width, uv_height);
		quit_yuv_stream=1;
	}
//	printf("get_preview_buffer done\n");
//	printf("Output YUV resolution %d x %d in YV12 format\n", preview_buffer_info.width, preview_buffer_info.height);
	return 0;
}
static int send_preview_buffer(int fd_sock)
{
	int rev;
	char char_w_len[10];
	char char_h_len[10];

	int y_buffer_w = 2*uv_width ;
	int w_len = Int32ToCharArray(y_buffer_w,char_w_len);
	send(fd_sock,char_w_len,w_len,0);
	send(fd_sock," ",1,0);

	int y_buffer_h=2*uv_height ;
	int h_len=Int32ToCharArray(y_buffer_h,char_h_len);
	send(fd_sock,char_h_len,h_len,0);
	send(fd_sock," ",1,0);
	send(fd_sock,"!",1,0);

	rev=send(fd_sock,y_buffer_clipped,y_buffer_w*y_buffer_h,0);
	if(rev<0)
		printf("send y fail\n");
	rev=send(fd_sock,uv_buffer_clipped, uv_width*uv_height*2, 0);
	if(rev<0)
		printf("send uv fail\n");
	return rev;
}
#endif
 static void idsp_dump(int sock_fd, u8 id)
{
	u8 *bin_buffer = NULL;
	iav_idsp_config_info_t	dump_idsp_info;
	int rval = -1;

	if(id >= 8 && id != 100){
		printf("err: unknow id section.\n");
		return;
	}

	if(bin_buffer == NULL){
		bin_buffer = (u8*)malloc(MAX_DUMP_BUFFER_SIZE);
	}
	if(bin_buffer != NULL){
		memset(bin_buffer, 0, MAX_DUMP_BUFFER_SIZE);
	}else{
		return;
	}
	dump_idsp_info.id_section = id;
	dump_idsp_info.addr = bin_buffer;

	rval = ioctl(fd_iav, IAV_IOC_IMG_DUMP_IDSP_SEC, &dump_idsp_info);
	if (rval < 0) {
		perror("IAV_IOC_IMG_DUMP_IDSP_SEC");
	}
	send(sock_fd,(u8*)&dump_idsp_info.addr_long,sizeof(u32),0);
	send_msg(sock_fd,bin_buffer,dump_idsp_info.addr_long);

	if(bin_buffer != NULL){
		free(bin_buffer);
		bin_buffer = NULL;
	}
}
 static int is_cali_mode(int sock_fd)
 {
	iav_encmode_cap_ex_t cap;
	u8 raw_mode =1;
	memset(&cap, 0, sizeof(cap));
	cap.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if(ioctl(fd_iav, IAV_IOC_QUERY_ENCMODE_CAP_EX, &cap)<0)
		return -1;
	raw_mode =cap.raw_cap_possible;
/* 	iav_system_resource_setup_ex_t  resource_limit_setup;

	memset(&resource_limit_setup, 0, sizeof(resource_limit_setup));
	resource_limit_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
 	if(ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup)<0)
		return -1;
	if(resource_limit_setup.encode_mode == IAV_ENCODE_CALIBRATION_MODE||
		resource_limit_setup.encode_mode == IAV_ENCODE_HIGH_MEGA_MODE ||
		resource_limit_setup.encode_mode == IAV_ENCODE_CALIBRATION_MODE ||
		resource_limit_setup.encode_mode == IAV_ENCODE_HDR_FRAME_MODE ||
		resource_limit_setup.encode_mode == IAV_ENCODE_HDR_LINE_MODE ||
		resource_limit_setup.encode_mode == IAV_ENCODE_HIGH_MP_FULL_PERF_MODE)
		raw_mode =1;
	else
		raw_mode =0;*/
	send(sock_fd,&raw_mode,1,0);
	return raw_mode;
 }
static int get_raw(int sock_fd)
{
	iav_raw_info_t		raw_info;
	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}
	printf("raw_addr = %p\n", raw_info.raw_addr);
	printf("resolution: %dx%d\n", raw_info.width, raw_info.height);
	int length =  raw_info.width * raw_info.height * 2;
	send(sock_fd,(u8*)&length,sizeof(int),0);
	send_msg(sock_fd,raw_info.raw_addr,length);
//	send(sock_fd,raw_info.raw_addr,length,0);
	return 0;
}
static int SockServer_Setup(int sockfd,int port)
{
	int on =2048;
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		return -1;
	}
	setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	struct sockaddr_in  my_addr;
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(port);
	my_addr.sin_addr.s_addr=INADDR_ANY;
	bzero(&(my_addr.sin_zero),0);

	if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
	{
		perror("bind");
		return -1;
	}
	if(listen(sockfd,10)==-1)
	{
		perror("listen");
		return -1;
	}
	return sockfd;

}
static int SockServer_free(int sock_fd,int client_fd)
{
	if(sock_fd!=-1)
	{
		close(sock_fd);
		sock_fd =-1;
	}
	if(client_fd!=-1)
	{
		close(client_fd);
		client_fd =-1;
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
			if(lens_mount_id==LENS_M13VP288IR_ID) {
				lens_param_info.piris_std.dgain	= &M13VP288IR_PIRIS_DGAIN;
				lens_param_info.piris_std.scope = M13VP288IR_PIRIS_SCOPE.table;
				lens_param_info.piris_std.table = M13VP288IR_PIRIS_STEP.table;
				lens_param_info.piris_std.tbl_size = M13VP288IR_PIRIS_STEP.header.array_size;
				if(vin_fps ==AMBA_VIDEO_FPS_60){
					info->p_50hz_lines = imx123_piris_60p50hz_lines;
					info->p_60hz_lines =imx123_piris_60p60hz_lines;
				}else{
					info->p_50hz_lines = imx123_piris_50hz_lines;
					info->p_60hz_lines =imx123_piris_60hz_lines;
				}
			} else {
				if(vin_fps ==AMBA_VIDEO_FPS_60){
					info->p_50hz_lines = imx123_60p50hz_lines;
					info->p_60hz_lines =imx123_60p60hz_lines;
				}else{
					info->p_50hz_lines = imx123_50hz_lines;
					info->p_60hz_lines =imx123_60hz_lines;
				}
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
int load_dsp_cc_table(int fd_iav)
{
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8* reg, *matrix, *sec_cc;
	char filename[128];
	int file, count;
	int rval = -1;

	reg = malloc(CC_REG_SIZE);
	matrix = malloc(CC_3D_SIZE);
	sec_cc = malloc(SEC_CC_SIZE);
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("./reg.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		return -1;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		if((file = open("./3D.bin", O_RDONLY, 0)) < 0) {
			printf("3D.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		return -1;
	}
	close(file);

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		return -1;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		return -1;
	}
	close(file);

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;
	rval = img_dsp_set_color_correction_reg(&color_corr_reg);
	CHECK_RVAL
	rval = img_dsp_set_color_correction(fd_iav, &color_corr);
	CHECK_RVAL
	rval = img_dsp_set_tone_curve(fd_iav, &tone_curve);
	CHECK_RVAL
	rval = img_dsp_set_sec_cc_en(fd_iav, 0);
	CHECK_RVAL
	rval = img_dsp_enable_color_correction(fd_iav);
	CHECK_RVAL
	free(reg);
	free(matrix);
	free(sec_cc);
	return 0;
}

int load_adj_cc_table(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 *matrix;
	u8 i, adj_mode = 4;

	matrix = malloc(CC_3D_SIZE);

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			if((file = open("./3D.bin", O_RDONLY, 0))<0) {
				printf("3D.bin cannot be opened\n");
				return -1;
			}
		}
		if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			printf("read %s error\n",filename);
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}
	free(matrix);
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
//	sleep(1);
	rval = img_set_work_mode(0);
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);
	img_set_chroma_noise_filter_max_radius(32<<resource_setup.max_chroma_noise_shift);
	CHECK_RVAL
	return 0;
}
int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (ch) {

		case	'a':
			start_aaa_flag = 1;
			break;

		case	'I':
			lens_mount_id = atoi(optarg);
			break;

		case	'D':
			lens_cali_info.act_mode = atoi(optarg);
			break;

		default:
			start_aaa_flag =0;
		}
	}
	return 0;
}
static void load_fixed_pattern_noise(int sock_fd)//mode =0:globel, mode =1:local
{
	int width=fpn_pkg.fpn_info.fpn_correct_info.width;
	int height =fpn_pkg.fpn_info.fpn_correct_info.height;
	u32 raw_pitch = ROUND_UP((width/8), 32);
	u32 fpn_map_size = raw_pitch*height;

	u8* fpn_map_addr = (u8 *)malloc(fpn_map_size);
	receive_msg(sock_fd,fpn_map_addr,fpn_map_size);
	fpn.enable = 3;
	fpn.fpn_pitch = raw_pitch;
	fpn.pixel_map_height = height;
	fpn.pixel_map_width = width;
	fpn.pixel_map_size = fpn_map_size;
	fpn.pixel_map_addr = (u32)fpn_map_addr;

//	int i=0;
//	for(i=0;i<100;i++)
//		printf("%d,\n",*(fpn_map_addr+i));
	img_dsp_set_static_bad_pixel_correction(fd_iav,&fpn);

	if(fpn_map_addr!=NULL)
	{
		free(fpn_map_addr);
		fpn_map_addr =NULL;
	}
}
static void fixed_pattern_noise_cal(int sock_fd)
{
	cfa_noise_filter_info_t cfa_nf;
	dbp_correction_t dbp_nf;
	memset(&cfa_nf, 0, sizeof(cfa_noise_filter_info_t));
	memset(&dbp_nf, 0, sizeof(dbp_correction_t));
	cfa_nf.enable = 0;
	dbp_nf.enable = 0;
	dbp_nf.hot_pixel_strength = 0;
	dbp_nf.dark_pixel_strength = 0;
	img_dsp_set_cfa_noise_filter(fd_iav, &cfa_nf);
	img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &dbp_nf);
	img_dsp_set_anti_aliasing(fd_iav,0);

	badpixel_detect_algo.cap_width=fpn_pkg.fpn_info.fpn_detect_info.cap_width;
	badpixel_detect_algo.cap_height=fpn_pkg.fpn_info.fpn_detect_info.cap_height;
	badpixel_detect_algo.agc_idx=fpn_pkg.fpn_info.fpn_detect_info.agc_idx;
	badpixel_detect_algo.shutter_idx=fpn_pkg.fpn_info.fpn_detect_info.shutter_idx;
	badpixel_detect_algo.block_w=fpn_pkg.fpn_info.fpn_detect_info.block_w;
	badpixel_detect_algo.block_h=fpn_pkg.fpn_info.fpn_detect_info.block_h;
	badpixel_detect_algo.upper_thres=fpn_pkg.fpn_info.fpn_detect_info.upper_thres;
	badpixel_detect_algo.lower_thres=fpn_pkg.fpn_info.fpn_detect_info.lower_thres;
	badpixel_detect_algo.detect_times=fpn_pkg.fpn_info.fpn_detect_info.detect_times;
	badpixel_detect_algo.badpix_type=fpn_pkg.fpn_info.fpn_detect_info.badpix_type;

	int width =badpixel_detect_algo.cap_width;
	int height=badpixel_detect_algo.cap_height;

	u32 raw_pitch = ROUND_UP(width/8, 32);
	printf("raw_pitch %d\n",raw_pitch);
	u8* fpn_map_addr = (u8 *)malloc(raw_pitch*height);
	if (fpn_map_addr == NULL){
		printf("can not malloc memory for bpc map\n");
	}
	memset(fpn_map_addr,0,(raw_pitch*height));

	badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
	badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still
	int bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
	printf("Totol number is %d\n",bpc_num);
	send_msg(sock_fd,fpn_map_addr,raw_pitch*height);

	if(fpn_map_addr != NULL){
		free(fpn_map_addr);
		fpn_map_addr = NULL;
	}
}
static void lens_shading_cal(int sock_fd)
{
	int rval = -1;
	u16* raw_buff = NULL;
	iav_raw_info_t raw_info;
	blc_level_t blc;
	u32 raw_vin_width = 0, raw_vin_height = 0;

	//Capture raw here
	printf("Raw capture started...\n");
	rval = ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);
	if (rval < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
	}
	raw_vin_width = raw_info.width;
	raw_vin_height = raw_info.height;
	raw_buff = (u16*)malloc(raw_vin_width*raw_vin_height*sizeof(u16));
	memcpy(raw_buff,raw_info.raw_addr,(raw_vin_width * raw_vin_height * 2));
	/*input*/
	vignette_cal_t vig_detect_setup;
	vig_detect_setup.raw_addr = raw_buff;
	vig_detect_setup.raw_w = raw_vin_width;
	vig_detect_setup.raw_h = raw_vin_height;
	vig_detect_setup.bp = raw_info.bayer_pattern;
	vig_detect_setup.threshold = 4095;
	vig_detect_setup.compensate_ratio = 896;
	vig_detect_setup.lookup_shift = 255;
	/*output*/
	vig_detect_setup.r_tab = lens_cali_pkg.vignette_red_gain_addr;
	vig_detect_setup.ge_tab =lens_cali_pkg.vignette_green_even_gain_addr;
	vig_detect_setup.go_tab = lens_cali_pkg.vignette_green_odd_gain_addr;
	vig_detect_setup.b_tab = lens_cali_pkg.vignette_blue_gain_addr;
	img_dsp_get_global_blc(&blc);
	vig_detect_setup.blc.r_offset = blc.r_offset;
	vig_detect_setup.blc.gr_offset = blc.gr_offset;
	vig_detect_setup.blc.gb_offset = blc.gb_offset;
	vig_detect_setup.blc.b_offset = blc.b_offset;
	rval = img_cal_vignette(&vig_detect_setup);

	lens_cali_pkg.gain_shift =vig_detect_setup.lookup_shift;
	send_msg(sock_fd,(u8*)&lens_cali_pkg,sizeof(LENS_CALI_PKG));

	if(raw_buff != NULL){
		free(raw_buff);
		raw_buff = NULL;
	}
}
static void vignette_compensation(int sock_fd)
{
	receive_msg(sock_fd,(u8*)&lens_cali_pkg,sizeof(LENS_CALI_PKG));
	printf("gain_shift is %d\n",lens_cali_pkg.gain_shift);
	vignette_info.enable = 1;
	vignette_info.gain_shift = lens_cali_pkg.gain_shift;
	vignette_info.vignette_red_gain_addr = (u32)(lens_cali_pkg.vignette_red_gain_addr);
	vignette_info.vignette_green_even_gain_addr = (u32)(lens_cali_pkg.vignette_green_even_gain_addr);
	vignette_info.vignette_green_odd_gain_addr = (u32)(lens_cali_pkg.vignette_green_odd_gain_addr);
	vignette_info.vignette_blue_gain_addr = (u32)(lens_cali_pkg.vignette_blue_gain_addr);
	img_dsp_set_vignette_compensation(fd_iav, &vignette_info);

}
static void process_other(int sock_fd,TUNING_ID* p_tuning_id)
{
	switch(p_tuning_id->tab_id)
	{
		case OnlineTuning:
		{
			switch(p_tuning_id->item_id)
			{
				case CHECK_IP:
				{
					char test[100] ="test_tuning";
					send(sock_fd,&test,strlen(test),0);
					printf("Available IP addess!\n");
					break;
				}
				case GET_RAW:
				{
					if(is_cali_mode(sock_fd))
					{
						get_raw(sock_fd);
						printf("get raw data done!\n");
					}
					break;
				}
				case DSP_DUMP:
				{
					u8 sec_id =0;
					recv(sock_fd,&sec_id,sizeof(u8),MSG_WAITALL);
					idsp_dump(sock_fd,sec_id);
					printf("dump done!\n");
					break;
				}
				case BW_MODE:
				{
					u8 bw_mode =0;
					recv(sock_fd,&bw_mode,sizeof(u8),MSG_WAITALL);
					img_set_bw_mode(bw_mode);
					printf("bw mode =%d\n",bw_mode);
					break;
				}
				case BP_CALI:
				{
					recv(sock_fd,&fpn_pkg.fpn_info,sizeof(FPN_INFO),MSG_WAITALL);
					if(fpn_pkg.fpn_info.mode ==0)//correct
					{
						load_fixed_pattern_noise(sock_fd);
						printf("BP cali:correct done!\n");
					}
					else if(fpn_pkg.fpn_info.mode ==1)//detect
					{
						if(is_cali_mode(sock_fd))
						{
							fixed_pattern_noise_cal(sock_fd);
							printf("BP cali:detect done!\n");
						}
					}
					break;
				}
				case LENS_CALI:
				{
					u8 mode=0;
					recv(sock_fd,&mode,sizeof(u8),MSG_WAITALL);
					if(mode ==0)//calc
					{
						if(is_cali_mode(sock_fd))
						{
							lens_shading_cal(sock_fd);
							printf("Lens cali: calc bin done!\n");
						}
					}
					else
					{
						vignette_compensation(sock_fd);
						printf("Lens cali: load bin done!\n");
					}
					break;
				}
			}
		}
		break;
	}

}
static void process_load(int sock_fd,TUNING_ID* p_tuning_id)
{

	printf("Load!\n");
	switch(p_tuning_id->tab_id)
	{
	case OnlineTuning:
	{
		switch(p_tuning_id->item_id)
		{
			case BlackLevelCorrection:
				img_dsp_get_global_blc(&blc_info.blc);
				img_dsp_get_def_blc(&blc_info.defblc_enable,blc_info.def_blc);
				send(sock_fd,(char*)&blc_info,sizeof(BLC_INFO),0);
				printf("Black Level Correction!\n");
				break;

			case ColorCorrection:
				break;

			case ToneCurve:
				img_dsp_get_tone_curve(&tone_curve);
				send(sock_fd,(char*)&tone_curve,sizeof(tone_curve_t),0);
				printf("Tone Curve!\n ");
				break;

			case RGBtoYUVMatrix:
				img_dsp_get_rgb2yuv_matrix(&rgb2yuv_matrix);
				send(sock_fd,(char*)&rgb2yuv_matrix,sizeof(rgb_to_yuv_t),0);
				printf("RGB to YUV Matrix!\n");
				break;

			case WhiteBalanceGains:
				img_dsp_get_rgb_gain(&wb_gain, &dgain);
				send(sock_fd,(char*)&wb_gain,sizeof(wb_gain_t),0);
				printf("White Balance Gains!\n");
				break;

			case DGainSaturaionLevel:
				img_dsp_get_dgain_saturation_level(&d_gain_satuation_level);
				send(sock_fd,(char*)&d_gain_satuation_level,sizeof(digital_sat_level_t),0);
				printf("DGain Saturation Level!\n");
				break;

			case LocalExposure:
				img_dsp_get_local_exposure(&local_exposure);
				send(sock_fd,(char*)&local_exposure,sizeof(local_exposure_t),0);
				printf("Local Exposure!\n");
				break;

			case ChromaScale:
				 img_dsp_get_chroma_scale(&cs);
				send(sock_fd,(char*)&cs,sizeof(chroma_scale_filter_t),0);
				printf("Chroma Scale!\n");
				break;

			case FPNCorrection://not finished
				break;

			case BadPixelCorrection:
				img_dsp_get_dynamic_bad_pixel_correction(&dbp_correction_setting);
				send(sock_fd,(char*)&dbp_correction_setting,sizeof(dbp_correction_t),0);
				printf("Bad Pixel Correction!\n");
				break;

			case CFALeakageFilter:
				img_dsp_get_cfa_leakage_filter( &cfa_leakage_filter);
				send(sock_fd,(char*)&cfa_leakage_filter,sizeof(cfa_leakage_filter_t),0);
				printf("CFA Leakage Filter!\n");
				break;

			case AntiAliasingFilter:
				anti_aliasing_strength=img_dsp_get_anti_aliasing();
				send(sock_fd,(char*)&anti_aliasing_strength,sizeof(u8),0);
				printf("Anti-Aliasing Filter!\n");
				break;

			case CFANoiseFilter:
				img_dsp_get_cfa_noise_filter(& cfa_noise_filter);
				send(sock_fd,(char*)&cfa_noise_filter,sizeof(cfa_noise_filter_info_t),0);
				printf("CFA Noise Filter!\n");
				break;

			case ChromaMedianFiler:
				img_dsp_get_chroma_median_filter(&chroma_median_setup);
				send(sock_fd,(char*)&chroma_median_setup,sizeof(chroma_median_filter_t),0);
				printf("Chroma Median Filer!\n");
				break;

			case SharpeningA_ASF:
				sa_asf_pkg.select_mode = img_dsp_get_video_sharpen_a_or_spatial_filter();
				img_dsp_get_advance_spatial_filter(0,&sa_asf_pkg.asf_info);
				img_dsp_get_sharpen_a_fir(0, &sa_asf_pkg.sa_info.fir);
				img_dsp_get_sharpen_a_coring( 0, &sa_asf_pkg.sa_info.coring);
				sa_asf_pkg.sa_info.retain_level= img_dsp_get_sharpen_a_signal_retain_level(0);
				img_dsp_get_sharpen_a_max_change(0,&sa_asf_pkg.sa_info.max_change);
				img_dsp_get_sharpen_a_level_minimum(0, &sa_asf_pkg.sa_info.sharpen_min);
				img_dsp_get_sharpen_a_level_overall(0,&sa_asf_pkg.sa_info.sharpen_overall);
				sa_asf_pkg.sa_info.linearization_strength =0;
				sa_asf_pkg.sa_info.luma_hfnr_strength.strength_coarse =0;
				sa_asf_pkg.sa_info.luma_hfnr_strength.strength_fine=0;
				send(sock_fd,(char*)&sa_asf_pkg,sizeof(SA_ASF_PKG),0);
				printf("SharpeningA_ASF!\n");
				break;

			case MCTFControl:
				img_dsp_get_video_mctf(&mctf_info.mctf);
				mctf_info.zmv= img_dsp_get_video_mctf_zmv_enable();
				send(sock_fd,(char*)&mctf_info,sizeof(MCTF_INFO),0);
				printf("MCTF Control!\n");
				break;

			case SharpeningBControl:
				img_dsp_get_luma_high_freq_noise_reduction(&sharpen_pkg.luma_hfnr_strength);
				img_dsp_get_sharpen_b_fir(0, &sharpen_pkg.fir);
				img_dsp_get_sharpen_b_coring( 0, &sharpen_pkg.coring);
				sharpen_pkg.retain_level= img_dsp_get_sharpen_b_signal_retain_level(0);
				img_dsp_get_sharpen_b_max_change(0,&sharpen_pkg.max_change);
				img_dsp_get_sharpen_b_level_minimum(0, &sharpen_pkg.sharpen_min);
				img_dsp_get_sharpen_b_level_overall(0,&sharpen_pkg.sharpen_overall);
				sharpen_pkg.linearization_strength= img_dsp_get_sharpen_b_linearization(0);
				send(sock_fd,(char*)&sharpen_pkg,sizeof(SHARPEN_PKG),0);
				printf("SharpeningBControl!\n");
				break;

			case ColorDependentNoiseReduction:
				img_dsp_get_color_dependent_noise_reduction(&cdnr_info);
				send(sock_fd,(char*)&cdnr_info,sizeof(cdnr_info_t),0);
				printf("Color Dependent Noise Reduction\n");
				break;

			case ChromaNoiseFilter:
				img_dsp_get_chroma_noise_filter(&chroma_noise_filter);
				send(sock_fd,(char*)&chroma_noise_filter,sizeof(chroma_filter_t),0);
				printf("Chroma Noise Filter!\n");
				break;

			case GMVSETTING:
				 img_dsp_get_global_motion_vector( &gmv_info_setting);
				send(sock_fd,(char*)&gmv_info_setting,sizeof(gmv_info_t),0);
				printf("MCTF GMV Setting!\n");
				break;

			case ConfigAAAControl:
				img_get_3a_cntl_status(&aaa_cntl_station);
				send(sock_fd,(char*)&aaa_cntl_station,sizeof(aaa_cntl_t),0);
				printf(" Config AAA Control!\n");
				break;

			case TileConfiguration:
				img_dsp_get_config_statistics_info(  &tile_pkg.stat_config_info);
				img_dsp_get_statistics_raw( fd_iav, &p_rgb_stat,& p_cfa_stat);//only for tuning,should not be called when 3A on
				tile_pkg.ae_tile_info.cfa_tile_num_col=p_cfa_stat.tile_info.ae_tile_num_col;
				tile_pkg.ae_tile_info.cfa_tile_num_row=p_cfa_stat.tile_info.ae_tile_num_row;
				tile_pkg.ae_tile_info.cfa_tile_col_start=p_cfa_stat.tile_info.ae_tile_col_start;
				tile_pkg.ae_tile_info.cfa_tile_row_start=p_cfa_stat.tile_info.ae_tile_row_start;
				tile_pkg.ae_tile_info.cfa_tile_width=p_cfa_stat.tile_info.ae_tile_width;
				tile_pkg.ae_tile_info.cfa_tile_height=p_cfa_stat.tile_info.ae_tile_height;
				tile_pkg.ae_tile_info.cfa_tile_active_width =0;
				tile_pkg.ae_tile_info.cfa_tile_active_height =0;
				tile_pkg.ae_tile_info.cfa_tile_cfa_y_shift=p_cfa_stat.tile_info.ae_linear_y_shift;
				tile_pkg.ae_tile_info.cfa_tile_rgb_y_shift=p_cfa_stat.tile_info.ae_y_shift;
				tile_pkg.ae_tile_info.cfa_tile_min_max_shift =0;

				tile_pkg.awb_tile_info.cfa_tile_num_col=p_cfa_stat.tile_info.awb_tile_num_col;
				tile_pkg.awb_tile_info.cfa_tile_num_row=p_cfa_stat.tile_info.awb_tile_num_row;
				tile_pkg.awb_tile_info.cfa_tile_col_start=p_cfa_stat.tile_info.awb_tile_col_start;
				tile_pkg.awb_tile_info.cfa_tile_row_start=p_cfa_stat.tile_info.awb_tile_row_start;
				tile_pkg.awb_tile_info.cfa_tile_width=p_cfa_stat.tile_info.awb_tile_width;
				tile_pkg.awb_tile_info.cfa_tile_height=p_cfa_stat.tile_info.awb_tile_height;
				tile_pkg.awb_tile_info.cfa_tile_active_width =p_cfa_stat.tile_info.awb_tile_active_width;
				tile_pkg.awb_tile_info.cfa_tile_active_height =p_cfa_stat.tile_info.awb_tile_active_height;
				tile_pkg.awb_tile_info.cfa_tile_cfa_y_shift=p_cfa_stat.tile_info.awb_y_shift;
				tile_pkg.awb_tile_info.cfa_tile_rgb_y_shift=p_cfa_stat.tile_info.awb_rgb_shift;
				tile_pkg.awb_tile_info.cfa_tile_min_max_shift =p_cfa_stat.tile_info.awb_min_max_shift;

				tile_pkg.af_tile_info.cfa_tile_num_col=p_cfa_stat.tile_info.af_tile_num_col;
				tile_pkg.af_tile_info.cfa_tile_num_row=p_cfa_stat.tile_info.af_tile_num_row;
				tile_pkg.af_tile_info.cfa_tile_col_start=p_cfa_stat.tile_info.af_tile_col_start;
				tile_pkg.af_tile_info.cfa_tile_row_start=p_cfa_stat.tile_info.af_tile_row_start;
				tile_pkg.af_tile_info.cfa_tile_width=p_cfa_stat.tile_info.af_tile_width;
				tile_pkg.af_tile_info.cfa_tile_height=p_cfa_stat.tile_info.af_tile_height;
				tile_pkg.af_tile_info.cfa_tile_active_width =p_cfa_stat.tile_info.af_tile_active_width;
				tile_pkg.af_tile_info.cfa_tile_active_height =p_cfa_stat.tile_info.af_tile_active_height;
				tile_pkg.af_tile_info.cfa_tile_cfa_y_shift=p_cfa_stat.tile_info.af_cfa_y_shift;
				tile_pkg.af_tile_info.cfa_tile_rgb_y_shift=p_cfa_stat.tile_info.af_y_shift;
				tile_pkg.af_tile_info.cfa_tile_min_max_shift =0;

				send(sock_fd,(char*)&tile_pkg,sizeof(TILE_PKG),0);
				printf("AF Tile Configuration!\n");
				break;

			case AFStatisticSetupEx:
				img_dsp_get_af_statistics_ex(&af_statistic_setup_ex);
				send(sock_fd,(char*)&af_statistic_setup_ex,sizeof(af_statistics_ex_t),0);
				printf("AF Statistic Setup Ex!\n");
				break;

			case ExposureControl:
				ec_info.agc_index=img_get_sensor_agc_index();
				ec_info.shutter_index=img_get_sensor_shutter_index();
				img_dsp_get_rgb_gain( &wb_gain,  &ec_info.dgain);
				send(sock_fd,(char*)&ec_info,sizeof(EC_INFO),0);
				printf("Exposure Control!\n");
				break;
			case HDR_CONTRAST:
				hdr_contrast_pkg.radius=img_hdr_get_yuv_contrast_low_pass_radius();
				hdr_contrast_pkg.enable =img_hdr_get_yuv_contrast_enable();
				send(sock_fd,(char*)&hdr_contrast_pkg,sizeof(hdr_contrast_pkg),0);
				printf("hdr contrast!\n");
				break;
					break;
			}
			break;
		}


		case HDRTuning:
			break;

		case OTHER:
			printf("to be supported\n");
			break;

		default:
			printf("Unknow tab id [%c]\n",p_tuning_id->tab_id);
			break;
	}
}
static void process_apply(int sock_fd,TUNING_ID* p_tuning_id)
{
	printf("Apply!\n");
	int rval =0;
	switch(p_tuning_id->tab_id)
	{
	case OnlineTuning:
	{
		switch(p_tuning_id->item_id)
		{
		case BlackLevelCorrection:
			rval=recv(sock_fd,(char*)&blc_info,sizeof(BLC_INFO),0);
			img_dsp_set_global_blc(fd_iav, &blc_info.blc,pattern);
			img_dsp_set_def_blc( fd_iav,blc_info.defblc_enable,blc_info.def_blc);
			printf("Black Level Correction!\n");
			break;

		case ColorCorrection:
		{
			u8* cc_reg =malloc(cc_reg_size);
			u8* cc_matrix=malloc(cc_matrix_size);
			u8* cc_sec=malloc(cc_sec_size);;
			receive_msg(sock_fd,cc_reg,cc_reg_size);
			receive_msg(sock_fd,cc_matrix,cc_matrix_size);
			receive_msg(sock_fd,cc_sec,cc_reg_size);
			color_corr_reg.reg_setting_addr = (u32)cc_reg;
			color_corr.matrix_3d_table_addr = (u32)cc_matrix;
			color_corr.sec_cc_addr = (u32)cc_sec;
			rval = img_dsp_set_color_correction_reg(&color_corr_reg);
			CHECK_RVAL
			rval = img_dsp_set_color_correction(fd_iav, &color_corr);
			CHECK_RVAL
			rval = img_dsp_set_tone_curve(fd_iav, &tone_curve);
			CHECK_RVAL
			rval = img_dsp_enable_color_correction(fd_iav);
			free(cc_reg);
			free(cc_matrix);
			free(cc_sec);
			printf("Color Correction!\n");
			break;
		}
		case ToneCurve:
			{
				rval=recv(sock_fd,(char*)&tone_curve,sizeof(tone_curve_t),0);
				rval = img_dsp_set_tone_curve(fd_iav,&tone_curve);
				CHECK_RVAL
				printf("Tone Curve!\n ");
			}
			break;

		case RGBtoYUVMatrix:
			rval=recv(sock_fd,(char*)&rgb2yuv_matrix,sizeof(rgb_to_yuv_t),0);
			rval = img_dsp_set_rgb2yuv_matrix(fd_iav, &rgb2yuv_matrix);
			CHECK_RVAL
			printf("RGB to YUV Matrix!\n");
			break;

		case WhiteBalanceGains:
			rval=recv(sock_fd,(char*)&wb_gain,sizeof(wb_gain_t),0);
			rval = img_dsp_set_wb_gain(fd_iav, &wb_gain);
			CHECK_RVAL
			printf("White Balance !\n");
			break;

		case DGainSaturaionLevel:
			rval=recv(sock_fd,(char*)&d_gain_satuation_level,sizeof(digital_sat_level_t),0);
			rval = img_dsp_set_dgain_saturation_level( fd_iav, &d_gain_satuation_level);
			CHECK_RVAL
			printf("DGain Saturation Level!\n");
			break;

		case LocalExposure:
			rval=recv(sock_fd,(char*)&local_exposure,sizeof(local_exposure_t),0);
			rval = img_dsp_set_local_exposure(fd_iav, &local_exposure);
			CHECK_RVAL
			printf("Local Exposure!\n");
			break;

		case ChromaScale:
			rval=recv(sock_fd,(char*)&cs,sizeof(chroma_scale_filter_t),0);
			rval = img_dsp_set_chroma_scale(fd_iav,&cs);
			CHECK_RVAL
			printf("Chroma Scale!\n");
			break;

		case FPNCorrection://not finished



			break;
		case BadPixelCorrection:
			rval=recv(sock_fd,(char*)&dbp_correction_setting,sizeof(dbp_correction_t),0);
			img_dsp_set_dynamic_bad_pixel_correction(fd_iav,&dbp_correction_setting);
			printf("Bad Pixel Correction!\n");
			break;

		case CFALeakageFilter:
			rval=recv(sock_fd,(char*)&cfa_leakage_filter,sizeof(cfa_leakage_filter_t),0);
			img_dsp_set_cfa_leakage_filter(fd_iav, &cfa_leakage_filter);
			printf("CFA Leakage Filter!\n");
			break;

		case AntiAliasingFilter:
			rval=recv(sock_fd,(char*)&anti_aliasing_strength,sizeof(u8),0);
			img_dsp_set_anti_aliasing(fd_iav, anti_aliasing_strength);
			printf("Anti-Aliasing Filter!\n");
			break;

		case CFANoiseFilter:
			rval=recv(sock_fd,(char*)&cfa_noise_filter,sizeof(cfa_noise_filter_info_t),0);
			img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
			printf("CFA Noise Filter!\n");
			break;

		case ChromaMedianFiler:
			rval=recv(sock_fd,(char*)&chroma_median_setup,sizeof(chroma_median_filter_t),0);
			img_dsp_set_chroma_median_filter(fd_iav, &chroma_median_setup);
			printf("Chroma Median Filer!\n");
			break;

		case SharpeningA_ASF:
		{
			rval=recv(sock_fd,(char*)&sa_asf_pkg,sizeof(SA_ASF_PKG),0);
			if(sa_asf_pkg.select_mode==0)
			{
				img_dsp_set_video_sharpen_a_or_spatial_filter(fd_iav,0);
				printf("disable SharpeningA_ASF\n");
			}
			else if(sa_asf_pkg.select_mode ==1)
			{
				printf("SA\n");
				img_dsp_set_video_sharpen_a_or_spatial_filter(fd_iav,1);
				img_dsp_get_sharpen_a_fir(0, &fir);
				img_dsp_get_sharpen_a_coring( 0, &coring);
				retain_level= img_dsp_get_sharpen_a_signal_retain_level(0);
				img_dsp_get_sharpen_a_max_change(0,&max_change);
				img_dsp_get_sharpen_a_level_minimum(0, &sharpen_setting_min);
				img_dsp_get_sharpen_a_level_overall(0,&sharpen_setting_overall);

				int i=0;
				for(i = 0; i<FIR_COEFF_SIZE; i++)
				{
					if(fir.fir_coeff[i] !=sa_asf_pkg.sa_info.fir.fir_coeff[i]||
					fir.fir_strength !=sa_asf_pkg.sa_info.fir.fir_strength)
					{
						memcpy(&fir,&sa_asf_pkg.sa_info.fir,sizeof(fir_t));
						img_dsp_set_sharpen_a_fir(fd_iav,0,&fir);
						break;
					}
				}
				for(i = 0; i<CORING_TABLE_SIZE; i++)
				{
					if(coring.coring[i] != sa_asf_pkg.sa_info.coring.coring[i])
					{
						memcpy(&coring,&sa_asf_pkg.sa_info.coring,sizeof(coring_table_t));
						img_dsp_set_sharpen_a_coring(fd_iav, 0,&coring);
						break;
					}
				}
				if(retain_level != sa_asf_pkg.sa_info.retain_level)
				{
					retain_level = sa_asf_pkg.sa_info.retain_level;
					img_dsp_set_sharpen_a_signal_retain_level(fd_iav,0,retain_level);
				}

				if(max_change.max_change_down !=sa_asf_pkg.sa_info.max_change.max_change_down||
				max_change.max_change_up != sa_asf_pkg.sa_info.max_change.max_change_up)
				{
					max_change.max_change_down =sa_asf_pkg.sa_info.max_change.max_change_down;
					max_change.max_change_up =  sa_asf_pkg.sa_info.max_change.max_change_up;
					img_dsp_set_sharpen_a_max_change(fd_iav,0, &max_change);
				}
				if(sharpen_setting_min.low != sa_asf_pkg.sa_info.sharpen_min.low||
				sharpen_setting_min.low_delta !=sa_asf_pkg.sa_info.sharpen_min.low_delta||
				sharpen_setting_min.low_strength !=sa_asf_pkg.sa_info.sharpen_min.low_strength||
				sharpen_setting_min.mid_strength != sa_asf_pkg.sa_info.sharpen_min.mid_strength||
				sharpen_setting_min.high != sa_asf_pkg.sa_info.sharpen_min.high||
				sharpen_setting_min.high_delta != sa_asf_pkg.sa_info.sharpen_min.high_delta||
				sharpen_setting_min.high_strength != sa_asf_pkg.sa_info.sharpen_min.high_strength)
				{
					memcpy(&sharpen_setting_min,&sa_asf_pkg.sa_info.sharpen_min,sizeof(sharpen_level_t));
					img_dsp_set_sharpen_a_level_minimum(fd_iav,0,&sharpen_setting_min);
				}
				if(sharpen_setting_overall.low !=sa_asf_pkg.sa_info.sharpen_overall.low||
				sharpen_setting_overall.low_delta != sa_asf_pkg.sa_info.sharpen_overall.low_delta||
				sharpen_setting_overall.low_strength !=sa_asf_pkg.sa_info.sharpen_overall.low_strength||
				sharpen_setting_overall.mid_strength != sa_asf_pkg.sa_info.sharpen_overall.mid_strength||
				sharpen_setting_overall.high != sa_asf_pkg.sa_info.sharpen_overall.high||
				sharpen_setting_overall.high_delta != sa_asf_pkg.sa_info.sharpen_overall.high_delta||
				sharpen_setting_overall.high_strength != sa_asf_pkg.sa_info.sharpen_overall.high_strength)
				{
					memcpy(&sharpen_setting_overall,&sa_asf_pkg.sa_info.sharpen_overall,sizeof(sharpen_level_t));
					img_dsp_set_sharpen_a_level_overall(fd_iav,0,&sharpen_setting_overall);
				}
				if(luma_hfnr_strength.strength_coarse!= sa_asf_pkg.sa_info.luma_hfnr_strength.strength_coarse||
					luma_hfnr_strength.strength_fine!= sa_asf_pkg.sa_info.luma_hfnr_strength.strength_fine)
				{
					luma_hfnr_strength.strength_coarse =sa_asf_pkg.sa_info.luma_hfnr_strength.strength_coarse;
					luma_hfnr_strength.strength_fine =  sa_asf_pkg.sa_info.luma_hfnr_strength.strength_fine;
					img_dsp_set_luma_high_freq_noise_reduction(fd_iav,&luma_hfnr_strength);
				}
			}
			else if(sa_asf_pkg.select_mode== 2)
			{
				printf("ASF\n");
				rval = img_dsp_set_video_sharpen_a_or_spatial_filter(fd_iav, 2);
				img_dsp_set_advance_spatial_filter( fd_iav, 0, &sa_asf_pkg.asf_info);
			}
		}
		break;

		case MCTFControl:
			rval=recv(sock_fd,(char*)&mctf_info,sizeof(MCTF_INFO),0);
			img_dsp_set_video_mctf_zmv_enable(mctf_info.zmv);
			img_dsp_set_video_mctf_enable(2);
			img_dsp_set_video_mctf_compression_enable(1);
			img_dsp_set_video_mctf(fd_iav, &mctf_info.mctf);

			printf("MCTF Control!\n");
			break;
		case SharpeningBControl:
			rval=recv(sock_fd,(char*)&sharpen_pkg,sizeof(SHARPEN_PKG),0);
			img_dsp_get_sharpen_b_fir(0, &fir);
			img_dsp_get_sharpen_b_coring( 0, &coring);
			retain_level= img_dsp_get_sharpen_b_signal_retain_level(0);
			img_dsp_get_sharpen_b_max_change(0,&max_change);
			img_dsp_get_sharpen_b_level_minimum(0, &sharpen_setting_min);
			img_dsp_get_sharpen_b_level_overall(0,&sharpen_setting_overall);
			linearization_strenght = img_dsp_get_sharpen_b_linearization(0);
			img_dsp_get_luma_high_freq_noise_reduction(&luma_hfnr_strength);

			int i=0;
			if(linearization_strenght !=sharpen_pkg.linearization_strength)
			{
				linearization_strenght = sharpen_pkg.linearization_strength;
				img_dsp_set_sharpen_b_linearization(fd_iav,0,linearization_strenght);
			}
			for(i = 0; i<FIR_COEFF_SIZE; i++)
			{
				if(fir.fir_coeff[i] !=sharpen_pkg.fir.fir_coeff[i]||
				fir.fir_strength !=sharpen_pkg.fir.fir_strength)
				{
					memcpy(&fir,&sharpen_pkg.fir,sizeof(fir_t));
					img_dsp_set_sharpen_b_fir(fd_iav,0,&fir);
					break;
				}
			}
			for(i = 0; i<CORING_TABLE_SIZE; i++)
			{
				if(coring.coring[i] != sharpen_pkg.coring.coring[i])
				{
					memcpy(&coring,&sharpen_pkg.coring,sizeof(coring_table_t));
					img_dsp_set_sharpen_b_coring(fd_iav, 0,&coring);
					break;
				}
			}
			if(retain_level != sharpen_pkg.retain_level)
			{
				retain_level = sharpen_pkg.retain_level;
				img_dsp_set_sharpen_b_signal_retain_level(fd_iav,0,retain_level);
			}

			if(max_change.max_change_down !=sharpen_pkg.max_change.max_change_down||
			max_change.max_change_up != sharpen_pkg.max_change.max_change_up)
			{
				max_change.max_change_down =sharpen_pkg.max_change.max_change_down;
				max_change.max_change_up =  sharpen_pkg.max_change.max_change_up;
				img_dsp_set_sharpen_b_max_change(fd_iav,0, &max_change);
			}
			if(sharpen_setting_min.low != sharpen_pkg.sharpen_min.low||
			sharpen_setting_min.low_delta !=sharpen_pkg.sharpen_min.low_delta||
			sharpen_setting_min.low_strength !=sharpen_pkg.sharpen_min.low_strength||
			sharpen_setting_min.mid_strength != sharpen_pkg.sharpen_min.mid_strength||
			sharpen_setting_min.high != sharpen_pkg.sharpen_min.high||
			sharpen_setting_min.high_delta != sharpen_pkg.sharpen_min.high_delta||
			sharpen_setting_min.high_strength != sharpen_pkg.sharpen_min.high_strength)
			{
				memcpy(&sharpen_setting_min,&sharpen_pkg.sharpen_min,sizeof(sharpen_level_t));
				img_dsp_set_sharpen_b_level_minimum(fd_iav,0,&sharpen_setting_min);
			}
			if(sharpen_setting_overall.low !=sharpen_pkg.sharpen_overall.low||
			sharpen_setting_overall.low_delta != sharpen_pkg.sharpen_overall.low_delta||
			sharpen_setting_overall.low_strength !=sharpen_pkg.sharpen_overall.low_strength||
			sharpen_setting_overall.mid_strength != sharpen_pkg.sharpen_overall.mid_strength||
			sharpen_setting_overall.high != sharpen_pkg.sharpen_overall.high||
			sharpen_setting_overall.high_delta != sharpen_pkg.sharpen_overall.high_delta||
			sharpen_setting_overall.high_strength != sharpen_pkg.sharpen_overall.high_strength)
			{
				memcpy(&sharpen_setting_overall,&sharpen_pkg.sharpen_overall,sizeof(sharpen_level_t));
				img_dsp_set_sharpen_b_level_overall(fd_iav,0,&sharpen_setting_overall);
			}
			if(luma_hfnr_strength.strength_coarse!= sharpen_pkg.luma_hfnr_strength.strength_coarse||
				luma_hfnr_strength.strength_fine!= sharpen_pkg.luma_hfnr_strength.strength_fine)
			{
				luma_hfnr_strength.strength_coarse =sharpen_pkg.luma_hfnr_strength.strength_coarse;
				luma_hfnr_strength.strength_fine =  sharpen_pkg.luma_hfnr_strength.strength_fine;
				img_dsp_set_luma_high_freq_noise_reduction(fd_iav,&luma_hfnr_strength);
			}
			printf("SharpeningBControl\n");
			break;

		case ColorDependentNoiseReduction:
			rval=recv(sock_fd,(char*)&cdnr_info,sizeof(cdnr_info_t),0);
			img_dsp_set_color_dependent_noise_reduction( fd_iav, 0, &cdnr_info);
			printf("Color Dependent Noise Reduction\n");
			break;

		case ChromaNoiseFilter:
			rval=recv(sock_fd,(char*)&chroma_noise_filter,sizeof(chroma_filter_t),0);
			img_dsp_set_chroma_noise_filter(fd_iav,IMG_VIDEO, &chroma_noise_filter);
			printf("Chroma Noise Filter!\n");
			break;

		case GMVSETTING:
			rval=recv(sock_fd,(char*)&gmv_info_setting,sizeof(gmv_info_t),0);
			img_dsp_set_global_motion_vector(fd_iav, &gmv_info_setting);
			printf("MCTF GMV Setting!\n");
			break;

		case ConfigAAAControl:
			rval=recv(sock_fd,(char*)&aaa_cntl_station,sizeof(aaa_cntl_t),0);
			img_enable_ae(aaa_cntl_station.ae_enable);
			img_enable_awb(aaa_cntl_station.awb_enable);
			img_enable_af(aaa_cntl_station.af_enable);
			img_enable_adj(aaa_cntl_station.adj_enable);
			printf(" Config AAA Control!\n");
			break;

		case TileConfiguration:
			rval=recv(sock_fd,(char*)&tile_pkg.stat_config_info,sizeof(statistics_config_t),0);
			img_dsp_config_statistics_info(fd_iav,&tile_pkg.stat_config_info);
			printf("Tile Configuration!\n");
			break;

		case AFStatisticSetupEx:
			rval=recv(sock_fd,(char*)&af_statistic_setup_ex,sizeof(af_statistics_ex_t),0);
			img_dsp_set_af_statistics_ex(fd_iav,&af_statistic_setup_ex,1);
			printf("AF Statistic Setup Ex!\n");
			break;

		case ExposureControl:
			rval=recv(sock_fd,(char*)&ec_info,sizeof(EC_INFO),0);
		   	img_set_sensor_shutter_index(fd_iav,ec_info.shutter_index);
		  	img_set_sensor_agc_index(fd_iav,ec_info.agc_index,sensor_double_step);
			img_dsp_set_dgain(fd_iav,ec_info.dgain);
			printf("Exposure Control!\n");
			break;
		case HDR_CONTRAST:
			rval=recv(sock_fd,(char*)&hdr_contrast_pkg,sizeof(hdr_contrast_pkg),0);
			img_hdr_set_yuv_contrast_enable(hdr_contrast_pkg.enable);
			img_hdr_set_yuv_contrast_low_pass_radius(hdr_contrast_pkg.radius);
			printf("hdr contrast!\n");
			break;
	}
	}
	break;
	default:
		printf("undefined\n");
	}
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
static int process_tuning_id(int sock_fd,TUNING_ID* p_tuning_id)
{
	switch(p_tuning_id->req_id)
	{
	case APPLY:
		process_apply(sock_fd,p_tuning_id);
		break;
	case LOAD:
		process_load(sock_fd,p_tuning_id);
		break;
	case OTHER:
		process_other(sock_fd,p_tuning_id);
		break;
	default:
		printf("Unknown req_id [%c]\n",p_tuning_id->req_id);
		break;
	}
	return 0;
}
static int info_mode =0;
static int exit_flag=0;
int recv_mode_info(int sock,char* buffer)
{
	recv(sock,buffer,2,MSG_WAITALL);
//	printf("%c,%c\n",buffer[0],buffer[1]);
	if(buffer[0] =='2')
	{
		if(buffer[1]=='1')
		{
			info_mode =1;
			return 2;
		}
		else if(buffer[1]=='2')
		{
			info_mode =2;
			return 2;
		}
		else
		{
			info_mode = 2;
			return 2;
		}
	}
	else if(buffer[0]=='1')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void handle_pipe(int sig)
{
        exit_flag=1;
}
void AAAinfo_proc_thread()
{
	int sock_aaa_fd = -1,sock_aaa_client_fd = -1;
	int retval,maxfd=-1;
	fd_set rfds;
	socklen_t s_size;
	struct timeval tv;
	struct sockaddr_in  their_addr_thread;
	sock_aaa_fd=SockServer_Setup(sock_aaa_fd, INFO_SOCKET_PORT);

	while(1)
	{

WaitForConnection:
		printf("Waiting for connection!\n");
		s_size=sizeof(struct sockaddr_in);
		if((sock_aaa_client_fd=accept(sock_aaa_fd,(struct sockaddr*)&their_addr_thread,&s_size))==-1)
		{
			perror("accapt");
			continue;
		}
		exit_flag=0;
		char pre_rev_thread[4];
		int r=0;
		while(1)
		{
			struct sigaction action;
			action.sa_handler = handle_pipe;
			sigemptyset(&action.sa_mask);
			action.sa_flags = 0;
			sigaction(SIGPIPE, &action, NULL);

			if(exit_flag==1)
			{
				if (sock_aaa_client_fd != -1)
					close(sock_aaa_client_fd);
				break;
			}
			FD_ZERO(&rfds);maxfd=0;
			FD_SET(sock_aaa_client_fd,&rfds);
			if(sock_aaa_client_fd>maxfd)
				maxfd=sock_aaa_client_fd;
			tv.tv_sec=1;tv.tv_usec=0;
			retval=select(maxfd+1,&rfds,NULL,NULL,&tv);
			if(retval==-1)
			{
				printf("select error!%s!\n",strerror(errno));
				break;
			}
			else if(retval==0)
			{
				continue;
			}
			else
			{
				if (FD_ISSET(sock_aaa_client_fd, &rfds))
				{
					r=recv_mode_info(sock_aaa_client_fd,pre_rev_thread);
					if(r==0)
					{
						if(1)//////////////////to be improved
						{
						//	printf("Get Recv, and stop!\n");
							break;
						}
					}
					else if(r==1)
					{
						printf("3A info disvisible!!\n");
						goto WaitForConnection;
					}
					else if(r==2)
					{
 						img_dsp_get_statistics(fd_iav, &aaa_stat_tuning, &act_tile);
						if(info_mode ==1)
						{
							int index=0;
							for(index =0;index<96;index++)
								aaa_info_pkg.ae_lin_y[index] =aaa_stat_tuning.ae_info[index].lin_y;
							for(index =0;index<1024;index++)
							{
								aaa_info_pkg.awb_r[index] =aaa_stat_tuning.awb_info[index].r_avg;
								aaa_info_pkg.awb_g[index] =aaa_stat_tuning.awb_info[index].g_avg;
								aaa_info_pkg.awb_b[index] =aaa_stat_tuning.awb_info[index].b_avg;
							}
							for(index =0;index<256;index++)
								aaa_info_pkg.af_fv2[index] =aaa_stat_tuning.af_info[index].sum_fv2;
							for(index =0;index<256;index++)
								aaa_info_pkg.af_fv1[index] =aaa_stat_tuning.af_info[index].sum_fv1;
							int r =0;
							r =send_msg(sock_aaa_client_fd,(u8*)&aaa_info_pkg,sizeof(aaa_info_pkg));
					//		printf("r =%d\n",r);
							if(r<0)
								break;

							char ack[10];
							while(1)
							{
								struct sigaction action;
								action.sa_handler = handle_pipe;
								sigemptyset(&action.sa_mask);
								action.sa_flags = 0;
								sigaction(SIGPIPE, &action, NULL);
								if(exit_flag==1)
								{
									if (sock_aaa_client_fd != -1)
										close(sock_aaa_client_fd);
									break;
								}
								recv(sock_aaa_client_fd, ack, 1, MSG_WAITALL);
							//	printf("ack =%c\n", ack[0]);
								if(ack[0] =='!')
									break;
							}
							if(exit_flag==1)
								break;
							usleep(1000*1000);
						}
						else if(info_mode ==2)
						{
							memcpy(&dsp_histo_info.cfa_histogram, &aaa_stat_tuning.cfa_hist, sizeof(cfa_histogram_stat_t));
							memcpy(&dsp_histo_info.rgb_histogram, &aaa_stat_tuning.rgb_hist, sizeof(rgb_histogram_stat_t));
							send_msg(sock_aaa_client_fd,(u8*)&dsp_histo_info,sizeof(dsp_histo_info));
							char ack[10];
							while(1)
							{
								struct sigaction action;
								action.sa_handler = handle_pipe;
								sigemptyset(&action.sa_mask);
								action.sa_flags = 0;
								sigaction(SIGPIPE, &action, NULL);
								if(exit_flag==1)
								{
									if (sock_aaa_client_fd != -1)
										close(sock_aaa_client_fd);
									break;
								}
								recv(sock_aaa_client_fd, ack, 1, MSG_WAITALL);
							//	printf("ack =%c\n", ack[0]);
								if(ack[0] =='!')
									break;
							}
							exit_flag =1;
							if(exit_flag==1)
								break;
						}
					}
					else
					{
						printf("some thing superised!\n");
					}
				}
			}
		}
	}
	if (sock_aaa_fd != -1)
		close(sock_aaa_fd);
}

int main(int argc, char **argv)
{

	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	int rval = -1;
	char sensor_name[32];
	img_config_info_t img_config_info;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
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

	if (init_param(argc, argv) < 0)
		return -1;

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
	if(map_buffer()<0)
	{
		printf("map buffer fail!\n");
	}

	if(start_aaa_flag)
	{
		rval = start_aaa_foo(fd_iav, sensor_name, app_param_image_sensor);
		CHECK_RVAL;
		//Create a new thread for AAAinfo
		pthread_t thread_3A_info;
		if((pthread_create(&thread_3A_info,NULL,(void*)AAAinfo_proc_thread,NULL)!=0))
			printf("create a new thread fail!\n");

		//Create a new thread for preview
	//	pthread_t thread_preview;
	//	if((pthread_create(&thread_preview,NULL,(void*)preview_proc_thread,NULL)!=0))
	//		printf("create a new thread fail!\n");
	}

	if(img_config_lens_cali(&lens_cali_info) < 0) {
			return -1;
	}

	int sockfd =-1;
	int new_fd =-1;

	struct sockaddr_in  their_addr;
	sockfd=SockServer_Setup(sockfd, ALL_ITEM_SOCKET_PORT);
	////////server setup
	while(1)
	{
		if(new_fd!=-1)
			close(new_fd);
		socklen_t sin_size=sizeof(struct sockaddr_in);
		signal(SIGFPE, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		if((new_fd=accept(sockfd,(struct sockaddr*)&their_addr,&sin_size))==-1)
		{
			perror("accapt");
			continue;
		}
		TUNING_ID tuning_id;
		int rev =-1;
		rev=recv(new_fd,(char*)&tuning_id,sizeof(TUNING_ID),0);
		if(rev <0)
		{
			printf("recv error[%d]\n",rev);
			break;
		}
	//	printf("size %d,require %c,item %c,tab %c\n",sizeof(TUNING_ID),
	//		tuning_id.req_id,tuning_id.item_id,tuning_id.tab_id);
		process_tuning_id(new_fd,&tuning_id);
	}
	SockServer_free(sockfd,new_fd);
	return 0;
}
