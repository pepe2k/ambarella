/**********************************************************
 * test_hdr_tuning.c
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
//#include <linux/unistd.h>
//#include <linux/inotify.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/socket.h>
#include<netinet/in.h>

#include "basetypes.h"
#include "ambas_common.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"
#include "test_hdr_tuning.h"
#include "ambas_vin.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "mn34041pl_adj_param_hdr.c"
#include "mn34041pl_aeb_param_hdr.c"

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

#define	IMGPROC_PARAM_PATH	"/etc/idsp"


int fd_iav;
static u8 hdr_expo_num = 2;
color_correction_reg_t color_corr_reg;
color_correction_t color_corr;

static wb_gain_t wb_gain;
static u32 dgain;
static dbp_correction_t dbp_correction_setting;
static SA_ASF_PKG sa_asf_pkg;
static fir_t fir = {-1, {0, -1, 0, 0, 0, 8, 0, 0, 0, 0}};
static luma_high_freq_nr_t luma_hfnr_strength;
static max_change_t max_change;
static u8 retain_level = 1;
static u16 linearization_strenght;
static sharpen_level_t sharpen_setting_min;
static sharpen_level_t sharpen_setting_overall;
static SHARPEN_PKG sharpen_pkg;
static rgb_to_yuv_t rgb2yuv_matrix;

static chroma_filter_t chroma_noise_filter;
static video_mctf_info_t mctf_info;
static cdnr_info_t cdnr_info;
//static HDR_MIXER_INFO hdr_mixer_info;
static HDR_CFA_INFO hdr_cfa_info;
static HDR_SHT_INFO hdr_sht_info;
static HDR_LE_INFO hdr_le_info;
static HDR_BLC_INFO hdr_blc_info;
//static HDR_TONE_INFO hdr_tone_info;
static HDR_HISTO_PKG hdr_histo_pkg;
static HDR_ALPHA_PKG hdr_alpha_pkg;
static HDR_CT_PKG hdr_ct_pkg;
static HDR_WB_PKG hdr_wb_pkg;
static HDR_AGC_INFO hdr_agc_info;
static HDR_CONTRAST_PKG hdr_contrast_pkg;
static color_correction_t hdr_color_corr[MAX_HDR_EXPOSURE_NUM];
static aaa_cntl_t aaa_cntl_hdr = {1, 1, 0, 1};

static int vin_tick = -1;
static int init_vin_tick()
{
	if((vin_tick = open(VIN0_VSYNC, O_RDONLY))<0) {
		perror(VIN0_VSYNC);
		return -1;
	}

	return 0;
}

static int get_vin_tick()
{
	char vin_int_array[8];
	read(vin_tick, vin_int_array, 8);

	return 0;
}

inline int get_vin_frame_rate_hdr(u32 *pFrame_time)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
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

int get_is_info(image_sensor_param_t* info,
					hdr_sensor_param_t* hdr_info,
					char* sensor_name, hdr_sensor_cfg_t *p_sensor_config)
{
	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}
	u8 pattern = 0;
	u8 step = 0;
	u32 vin_fps;
	get_vin_frame_rate_hdr(&vin_fps);

	memset(info, 0, sizeof(image_sensor_param_t));
	memset(hdr_info, 0, sizeof(hdr_sensor_param_t));
	memset(p_sensor_config, 0, sizeof(hdr_sensor_cfg_t));

	switch (vin_info.sensor_id) {
		case SENSOR_MN34041PL:
			memcpy(p_sensor_config, &mn34041pl_sensor_config, sizeof(hdr_sensor_cfg_t));
			info->p_adj_param = &mn34041pl_adj_param;
			info->p_rgb2yuv = mn34041pl_rgb2yuv;
			info->p_chroma_scale = &mn34041pl_chroma_scale;
			info->p_awb_param = &mn34041pl_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mn34041pl_60p50hz_lines;
				info->p_60hz_lines = mn34041pl_60p60hz_lines;
			}else{
				printf("Error: FPS should be 60fps.\n");
			}
			info->p_tile_config = &mn34041pl_tile_config;
			info->p_ae_agc_dgain = mn34041pl_ae_agc_dgain;
			info->p_ae_sht_dgain = mn34041pl_ae_sht_dgain;
			info->p_dlight_range = mn34041pl_dlight;
			info->p_manual_LE = mn34041pl_manual_LE;
			hdr_info->p_ae_target = NULL;
			hdr_info->p_sht_lines = NULL;
			sprintf(sensor_name, "mn34041pl");
			break;
		case SENSOR_MN34210PL:
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
			if(hdr_expo_num == 3){
				hdr_info->p_ae_target = mn34210pl_multifrm_ae_target_3x;
				hdr_info->p_sht_lines = (line_t**)&mn34210pl_multifrm_sht_lines_3x[0];
			}else if(hdr_expo_num == 2){
				hdr_info->p_ae_target = mn34210pl_multifrm_ae_target_2x;
				hdr_info->p_sht_lines = (line_t**)&mn34210pl_multifrm_sht_lines_2x[0];
			}
			sprintf(sensor_name, "mn34210pl");
			break;
		case SENSOR_MN34220PL:
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
			if(hdr_expo_num == 3){
				hdr_info->p_ae_target = mn34220pl_multifrm_ae_target_3x;
				hdr_info->p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_3x[0];
			}else if(hdr_expo_num == 2){
				hdr_info->p_ae_target = mn34220pl_multifrm_ae_target_2x;
				hdr_info->p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_2x[0];
			}
			sprintf(sensor_name, "mn34220pl");
			break;
		case SENSOR_OV4689:
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
			if(hdr_expo_num == 3){
				hdr_info->p_ae_target = ov4689_multifrm_ae_target_3x;
				hdr_info->p_sht_lines = (line_t**)&ov4689_multifrm_sht_lines_3x[0];
			}else if(hdr_expo_num == 2){
				hdr_info->p_ae_target = ov4689_multifrm_ae_target_2x;
				hdr_info->p_sht_lines = (line_t**)&ov4689_multifrm_sht_lines_2x[0];
			}
			sprintf(sensor_name, "ov4689");
			break;
		case SENSOR_IMX123:
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
			if(hdr_expo_num == 3){
				hdr_info->p_ae_target = imx123_multifrm_ae_target_3x;
				hdr_info->p_sht_lines = (line_t**)&imx123_multifrm_sht_lines_3x[0];
			}else if(hdr_expo_num == 2){
				hdr_info->p_ae_target = imx123_multifrm_ae_target_2x;
				hdr_info->p_sht_lines = (line_t**)&imx123_multifrm_sht_lines_2x[0];
			}
			sprintf(sensor_name, "imx123");
			break;
		case SENSOR_IMX224:
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
			if(hdr_expo_num == 3){
				hdr_info->p_ae_target = imx224_multifrm_ae_target_3x;
				hdr_info->p_sht_lines = (line_t**)&imx224_multifrm_sht_lines_3x[0];
			}else if(hdr_expo_num == 2){
				hdr_info->p_ae_target = imx224_multifrm_ae_target_2x;
				hdr_info->p_sht_lines = (line_t**)&imx224_multifrm_sht_lines_2x[0];
			}
			sprintf(sensor_name, "imx224");
			break;
		default:
			printf("Not supported sensor Id [%d].\n", vin_info.sensor_id);
			return -1;
	}

	get_sensor_bayer_pattern(&pattern);
	get_sensor_step(&step);

	p_sensor_config->pat = (bayer_pattern)pattern;
	p_sensor_config->step = step;

	return 0;
}

int load_mctf_bin_hdr()
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
		sprintf(filename, "%s/%s", IMGPROC_PARAM_PATH, bin_fn_hdr[i]);
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
		if(rval < 0){
			return -1;
		}
	}
	free(bin_buff);
	return 0;
}

int load_dsp_cc_table(int fd_iav)
{
	int rval = 0;
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8 *reg = NULL, *matrix = NULL, *sec_cc = NULL;
	char filename[128];
	int file, count;

	if(reg == NULL){
		reg = (u8*)malloc(CC_REG_SIZE);
		if(reg == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	if(matrix == NULL){
		matrix = (u8*)malloc(CC_3D_SIZE);
		if(matrix == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	if(sec_cc == NULL){
		sec_cc = (u8*)malloc(SEC_CC_SIZE);
		if(sec_cc == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("reg.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("3D.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;
	img_dsp_set_color_correction_reg(&color_corr_reg);
	img_dsp_set_color_correction(fd_iav, &color_corr);

load_dsp_cc_exit:
	if(reg != NULL){
		free(reg);
		reg = NULL;
	}
	if(matrix != NULL){
		free(matrix);
		matrix = NULL;
	}
	if(sec_cc != NULL){
		free(sec_cc);
		sec_cc = NULL;
	}
	return rval;
}

int load_adj_cc_table(char * sensor_name)
{
	int file = -1;
	int count = 0;
	char filename[128];
	u8 *matrix = NULL;
	u8 i, adj_mode = 4;
	int rval = 0;

	if(matrix == NULL){
		matrix = malloc(CC_3D_SIZE);
		if(matrix == NULL){
			rval = -1;
			printf("error: malloc no memory\n");
			goto load_adj_cc_exit;
		}
	}

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("3D.bin cannot be opened\n");
			rval = -1;
			goto load_adj_cc_exit;
		}
		if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			printf("read %s error\n",filename);
			rval = -1;
			goto load_adj_cc_exit;
		}
		img_adj_load_cc_table((u32)matrix, i);
	}

load_adj_cc_exit:
	if(file >= 0){
		close(file);
		file = -1;
	}
	if(matrix != NULL){
		free(matrix);
		matrix = NULL;
	}
	return rval;
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
	}
	return length;
}

static int send_msg(int sock_fd,u8* buff,int size)
{
	int length =0;
	int remain =size;
	while(length < size)
	{
		int retv=0;
		int send_size =2000;

		if(remain>2000)
			send_size =2000;
		else
			send_size =remain;
	 	retv=send(sock_fd,buff+length,send_size,0);
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
		remain =size -length;
		usleep(1000);
	}
	return length;
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

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}

static int get_hdr_raw(int sock_fd,int num)
{
	init_vin_tick();
	iav_raw_info_t		raw_info;
	int rval = 0;

	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}
	int raw_size =raw_info.height*raw_info.width*2;
	u8* raw_data =malloc(num*raw_size);
	int index =0;
	while(index!=num)
	{
		get_vin_tick();
		if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
			perror("IAV_IOC_READ_RAW_INFO");
			rval = -1;
			goto get_hdr_raw_exit;
	 	}
		memcpy(raw_data+index*raw_size,raw_info.raw_addr,raw_size);
		index++;
	}

/*	sprintf(pic_file_name, "%s.raw", pic_file_name);
	fd_raw = open(pic_file_name, O_WRONLY | O_CREAT, 0666);
	if (write(fd_raw, raw_info.raw_addr, raw_info.width * raw_info.height * 2) < 0) {
		perror("write(save_raw)");
		return -1;
	}

	printf("raw picture written to %s\n", pic_file_name);
	close(fd_raw);*/

	printf("raw_addr = %p\n", raw_info.raw_addr);
	printf("resolution: %dx%d\n", raw_info.width, raw_info.height);
	int length =  num*raw_size;

	send(sock_fd,(u8*)&length,sizeof(int),0);
	printf("length =%d\n",length);
	send_msg(sock_fd,raw_data,length);
//	send(sock_fd,raw_info.raw_addr,length,0);

get_hdr_raw_exit:
	if(raw_data != NULL){
		free(raw_data);
		raw_data = NULL;
	}
	return rval;
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
					char test[100] ="test_hdr_tuning";
					send(sock_fd,&test,strlen(test),0);
					printf("Available IP addess!\n");
					break;
				}
				case GET_RAW:
				{
					printf("get raw\n");
					int num =3;
					get_hdr_raw(sock_fd,num);
					printf("get raw data done!\n");
					break;
				}
			}
		}
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
				case HDR_BLC:
				hdr_blc_info.auto_mode=img_adj_hdr_get_auto_blc();
				img_adj_hdr_get_blc(hdr_blc_info.blc_offset, hdr_expo_num);
				send(sock_fd,(char*)&hdr_blc_info,sizeof(HDR_BLC_INFO),0);
				printf("HDR_BLC!\n");
				break;

			case ColorCorrection:
				break;
			case RGBtoYUVMatrix:
				img_dsp_get_rgb2yuv_matrix(&rgb2yuv_matrix);
				send(sock_fd,(char*)&rgb2yuv_matrix,sizeof(rgb_to_yuv_t),0);
				printf("RGB to YUV Matrix!\n");
				break;
			case HDR_ToneCurve:
				break;

			case WhiteBalanceGains:
				img_dsp_get_rgb_gain(&wb_gain, &dgain);
				send(sock_fd,(char*)&wb_gain,sizeof(wb_gain_t),0);
				printf("White Balance Gains!\n");
				break;

			case BadPixelCorrection:
				img_dsp_get_dynamic_bad_pixel_correction(&dbp_correction_setting);
				send(sock_fd,(char*)&dbp_correction_setting,sizeof(dbp_correction_t),0);
				printf("Bad Pixel Correction!\n");
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
			case MCTFControl:
				img_dsp_get_video_mctf(&mctf_info);
				send(sock_fd,(char*)&mctf_info,sizeof(video_mctf_info_t),0);
				printf("MCTF Control!\n");
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

			case HDR_LOCALEXPOSURE:
				hdr_le_info.auto_mode= img_hdr_get_auto_le();
				img_hdr_get_le_param(hdr_le_info.le_config,hdr_le_info.le_info,hdr_expo_num);
				send(sock_fd,(char*)&hdr_le_info,sizeof(hdr_le_info),0);
				printf("hdr le!\n");
				break;

			case HDR_SHUTTER:
			{
				int i=0;
				img_hdr_get_aaa_status(&aaa_cntl_hdr);
				hdr_sht_info.auto_mode =aaa_cntl_hdr.ae_enable;
				hdr_sht_info.index_or_row = 1;
				for(i = 0; i < MAX_HDR_EXPOSURE_NUM; ++i){
					hdr_sht_info.shutter_index[i] = 0;
				}
				img_ae_hdr_get_shutter_row(hdr_sht_info.shutter_row, hdr_expo_num);
				for(i=0;i<hdr_expo_num;i++){
					hdr_sht_info.ae_luma_target[i]=img_ae_hdr_get_target(i);
				}
				send(sock_fd,(char*)&hdr_sht_info,sizeof(hdr_sht_info),0);
				printf("hdr shutter!\n");
				break;
			}
			case HDR_MIXER:
				break;

			case HDR_CFA:
				img_adj_hdr_get_cfa(hdr_cfa_info.cfa_noise_filter_setup, hdr_expo_num);
				hdr_cfa_info.auto_mode =img_adj_hdr_get_auto_cfa();
				send(sock_fd,(char*)&hdr_cfa_info,sizeof(hdr_cfa_info),0);
				printf("hdr cfa!\n");
				break;

			case ConfigAAAControl:
				img_hdr_get_aaa_status(&aaa_cntl_hdr);
				send(sock_fd,(char*)&aaa_cntl_hdr,sizeof(aaa_cntl_hdr),0);
				printf("hdr aaa!\n");
				break;

			case HDR_Alpha:
				{
				alpha_map_config_t alpha_config[MAX_HDR_EXPOSURE_NUM];
				int i = 0;
				memset(alpha_config, 0, MAX_HDR_EXPOSURE_NUM * sizeof(alpha_map_config_t));
				hdr_alpha_pkg.auto_mode=img_hdr_get_auto_alpha_map();
				img_hdr_get_alpha_map_blend(alpha_config, hdr_expo_num);
				for(i = 0; i <hdr_expo_num; ++i){
					memcpy(&hdr_alpha_pkg.outlut_level[i], &alpha_config[i].out_lut_level, sizeof(out_lut_level_t));
					memcpy(&hdr_alpha_pkg.outlut_table[i], &alpha_config[i].out_lut_table, sizeof(out_lut_table_t));
				}
				send(sock_fd,(char*)&hdr_alpha_pkg,sizeof(hdr_alpha_pkg),0);
				printf("hdr alpha!\n");
				}
				break;
			case HDR_CT:
				img_hdr_get_cc_tone(hdr_ct_pkg.tone_info,hdr_expo_num);
				send(sock_fd,(char*)&hdr_ct_pkg,sizeof(hdr_ct_pkg),0);
				printf("hdr cc&tone!\n");
				break;
			case HDR_WB:
				img_hdr_get_wb_gain(hdr_wb_pkg.wb,hdr_expo_num);
				send(sock_fd,(char*)&hdr_wb_pkg,sizeof(hdr_wb_pkg),0);
				printf("hdr wb!\n");
				break;

			case HDR_AGC:
				img_ae_hdr_get_agc_index(hdr_agc_info.agc,hdr_expo_num);
				img_ae_hdr_get_dgain(hdr_agc_info.dgain,hdr_expo_num);
				send(sock_fd,(char*)&hdr_agc_info,sizeof(hdr_agc_info),0);
				printf("hdr agc&dgain!\n");
				break;
			case HDR_CONTRAST:
				hdr_contrast_pkg.radius=img_hdr_get_yuv_contrast_low_pass_radius();
				hdr_contrast_pkg.enable =img_hdr_get_yuv_contrast_enable();
				send(sock_fd,(char*)&hdr_contrast_pkg,sizeof(hdr_contrast_pkg),0);
				printf("hdr contrast!\n");
				break;
			}
		}
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
		case HDR_BLC:
			rval=recv(sock_fd,(char*)&hdr_blc_info,sizeof(HDR_BLC_INFO),0);
			img_adj_hdr_set_auto_blc(hdr_blc_info.auto_mode);
			img_adj_hdr_set_blc(hdr_blc_info.blc_offset,hdr_expo_num);
			printf("HDR_BLC!\n");
			break;

		case ColorCorrection:
		{
		//	printf("\n");
		//	u8* cc_reg =malloc(cc_reg_size);
		//	u8* cc_matrix=malloc(cc_matrix_size);
		//	u8* cc_sec=malloc(cc_sec_size);;
		//	receive_msg(sock_fd,cc_reg,cc_reg_size);
		//	receive_msg(sock_fd,cc_matrix,cc_matrix_size);
		//	receive_msg(sock_fd,cc_sec,cc_reg_size);
		//	color_corr_reg.reg_setting_addr = (u32)cc_reg;
		//	color_corr.matrix_3d_table_addr = (u32)cc_matrix;
		//	color_corr.sec_cc_addr = (u32)cc_sec;
		//	rval = img_dsp_set_color_correction_reg(&color_corr_reg);
		//	CHECK_RVAL
		//	rval = img_dsp_set_color_correction(fd_iav, &color_corr);
		//	CHECK_RVAL
		//	rval = img_dsp_set_tone_curve(fd_iav, &tone_curve_hdr);
		//	CHECK_RVAL
		//	rval = img_dsp_enable_color_correction(fd_iav);
		//	free(cc_reg);
		//	free(cc_matrix);
		//	free(cc_sec);
		//	printf("Color Correction!\n");
			break;
		}
		case RGBtoYUVMatrix:
			rval=recv(sock_fd,(char*)&rgb2yuv_matrix,sizeof(rgb_to_yuv_t),0);
			rval = img_dsp_set_rgb2yuv_matrix(fd_iav, &rgb2yuv_matrix);
			CHECK_RVAL
			printf("RGB to YUV Matrix!\n");
			break;
		case HDR_ToneCurve:
		{
		//	rval=recv(sock_fd,(char*)&hdr_tone_info,sizeof(tone_curve_t),0);
		//	img_hdr_auto_tone_enable(hdr_tone_info.auto_mode);
		//	rval =img_hdr_auto_tone_set_alpha(hdr_tone_info.alpha);
		//	img_hdr_auto_tone_set_speed(hdr_tone_info.speed);
		//	rval = img_dsp_set_tone_curve(fd_iav,&hdr_tone_info.tone);
		//	CHECK_RVAL
		//	printf("Tone Curve!\n ");
			break;
		}

		case WhiteBalanceGains:
			rval=recv(sock_fd,(char*)&wb_gain,sizeof(wb_gain_t),0);
			rval = img_dsp_set_wb_gain(fd_iav, &wb_gain);
			CHECK_RVAL
			printf("White Balance !\n");
			break;

		case BadPixelCorrection:
			rval=recv(sock_fd,(char*)&dbp_correction_setting,sizeof(dbp_correction_t),0);
			img_dsp_set_dynamic_bad_pixel_correction(fd_iav,&dbp_correction_setting);
			printf("Bad Pixel Correction!\n");
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
		case SharpeningBControl:
		{
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
		}

		case MCTFControl:
			rval=recv(sock_fd,(char*)&mctf_info,sizeof(mctf_info_t),0);
			img_dsp_set_video_mctf_enable(2);
			img_dsp_set_video_mctf_compression_enable(1);
			img_dsp_set_video_mctf(fd_iav, &mctf_info);
			printf("MCTF Control!\n");
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

		case HDR_LOCALEXPOSURE:
			rval=recv(sock_fd,(char*)&hdr_le_info,sizeof(hdr_le_info),0);
			img_hdr_set_auto_le(hdr_le_info.auto_mode);
			img_hdr_set_le_param(hdr_le_info.le_config,hdr_le_info.le_info, hdr_expo_num);
			printf("hdr le!\n");
			break;
			break;

		case HDR_SHUTTER:
		{
			int i=0;
			rval=recv(sock_fd,(char*)&hdr_sht_info,sizeof(hdr_sht_info),0);
			img_hdr_enable_ae(hdr_sht_info.auto_mode);
			if(hdr_sht_info.index_or_row){
				img_ae_hdr_set_shutter_row(hdr_sht_info.shutter_row, hdr_expo_num);
			}
			for(i=0;i<hdr_expo_num;i++){
				img_ae_hdr_set_target(hdr_sht_info.ae_luma_target[i], i);
			}
			printf("hdr shutter!\n");
			break;
		}
		case HDR_MIXER:
			break;

		case HDR_CFA:
			rval=recv(sock_fd,(char*)&hdr_cfa_info,sizeof(hdr_cfa_info),0);
			img_adj_hdr_set_auto_cfa(hdr_cfa_info.auto_mode);
			img_adj_hdr_set_cfa(hdr_cfa_info.cfa_noise_filter_setup, hdr_expo_num);
			printf("hdr cfa!\n");
			break;
		case ConfigAAAControl:
			rval=recv(sock_fd,(char*)&aaa_cntl_hdr,sizeof(aaa_cntl_hdr),0);
			img_hdr_enable_adj(aaa_cntl_hdr.adj_enable);
			img_hdr_enable_awb(aaa_cntl_hdr.awb_enable);
			img_hdr_enable_ae(aaa_cntl_hdr.ae_enable);
			printf("hdr aaa!\n");
			break;
		case HDR_Alpha:
			{
			int i = 0;
			alpha_map_config_t alpha_config[MAX_HDR_EXPOSURE_NUM];
			memset(alpha_config, 0, MAX_HDR_EXPOSURE_NUM * sizeof(alpha_map_config_t));
			rval=recv(sock_fd,(char*)&hdr_alpha_pkg,sizeof(hdr_alpha_pkg),0);
			for(i = 0; i < hdr_expo_num; ++i){
				memcpy(&alpha_config[i].out_lut_level, &hdr_alpha_pkg.outlut_level[i], sizeof(out_lut_level_t));
				memcpy(&alpha_config[i].out_lut_table, &hdr_alpha_pkg.outlut_table[i], sizeof(out_lut_table_t));
			}
			img_hdr_set_auto_alpha_map(hdr_alpha_pkg.auto_mode);
			img_hdr_set_alpha_map_blend(alpha_config, hdr_expo_num);
			printf("hdr alpha!\n");
			}
			break;
		case HDR_CT:
		{
			int i=0;
			rval =receive_msg(sock_fd,(u8*)&hdr_ct_pkg,sizeof(hdr_ct_pkg));
			for(i=0;i<hdr_expo_num;i++){
				hdr_color_corr[i].matrix_3d_table_addr =(u32)hdr_ct_pkg.cc[i].cc_matrix;
			}
			if(hdr_ct_pkg.cc_load_flag){
				img_hdr_set_cc_matrix(hdr_color_corr,hdr_expo_num);
			}
			if(hdr_ct_pkg.tone_load_flag){
				img_hdr_set_cc_tone(hdr_ct_pkg.tone_info,hdr_expo_num);
			}
			printf("hdr cc&tone!\n");
			break;
		}
		case HDR_WB:
			rval=recv(sock_fd,(char*)&hdr_wb_pkg,sizeof(hdr_wb_pkg),0);
			img_hdr_set_wb_gain(hdr_wb_pkg.wb,hdr_expo_num);
			printf("hdr wb!\n");
			break;

		case HDR_AGC:
			rval=recv(sock_fd,(char*)&hdr_agc_info,sizeof(hdr_agc_info),0);
			img_ae_hdr_set_agc_index(hdr_agc_info.agc,hdr_expo_num);
			img_ae_hdr_set_dgain(hdr_agc_info.dgain,hdr_expo_num);
			printf("hdr agc&dgain!\n");
			break;
		case HDR_CONTRAST:
			rval=recv(sock_fd,(char*)&hdr_contrast_pkg,sizeof(hdr_contrast_pkg),0);
			img_hdr_set_yuv_contrast_enable(hdr_contrast_pkg.enable);
			img_hdr_set_yuv_contrast_low_pass_radius(hdr_contrast_pkg.radius);
			printf("hdr contrast!\n");
			break;
			}
		}
	}
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

static u8 exit_flag =0;
void handle_pipe()
{
	printf("histo thread exit\n");
	exit_flag =1;
}
 void histo_thread_proc()
 {
	int histo_sock_server = -1;
	int histo_sock_client = -1;

	int idx = 0;
	static img_aaa_stat_t hdr_statis_gp[MAX_HDR_EXPOSURE_NUM];
	memset(hdr_statis_gp, 0, MAX_HDR_EXPOSURE_NUM*sizeof(img_aaa_stat_t));

	struct sockaddr_in  addr_client;
	histo_sock_server=SockServer_Setup(histo_sock_server, HISTO_SOCKET_PORT);
	while(1)
	{
		socklen_t s_size=sizeof(struct sockaddr_in);
		if((histo_sock_client=accept(histo_sock_server,(struct sockaddr*)&addr_client,&s_size))==-1)
		{
			perror("accapt");
			continue;
		}
		while(1)
		{
			if(img_hdr_get_statis(hdr_statis_gp, hdr_expo_num) < 0) continue;
			for(idx = 0; idx < hdr_expo_num; ++idx){
				memcpy(&hdr_histo_pkg.dsp_histo_data[idx].cfa_histogram, &hdr_statis_gp[idx].cfa_hist, sizeof(cfa_histogram_stat_t));
				memcpy(&hdr_histo_pkg.dsp_histo_data[idx].rgb_histogram, &hdr_statis_gp[idx].rgb_hist, sizeof(rgb_histogram_stat_t));
			}
			send_msg(histo_sock_client,(u8*)&hdr_histo_pkg,sizeof(hdr_histo_pkg));
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
					if (histo_sock_client != -1)
						close(histo_sock_client);
					break;
				}
				recv(histo_sock_client, ack, 1, MSG_WAITALL);
			//	printf("ack =%c\n", ack[0]);
				if(ack[0] =='!')
					break;
			}
			if(exit_flag==1)
				break;
			usleep(1000*1200);
		}
	}
 }

 static void sigstop()
 {
 	img_stop_hdr();
	printf("3A is off.\n");
	exit(1);
 }

static int init_isp()
{
	image_sensor_param_t sensor_app_param;
	hdr_sensor_param_t hdr_app_param;
	char sensor_name[32] = "img";
	img_config_info_t img_config_info;
	hdr_sensor_cfg_t sensor_cfg;
	hdr_cntl_fp_t cus_cntl_fp = {0};
	int i = 0;

	if(img_config_working_status(fd_iav, &img_config_info) < 0){
		printf("error: img_config_working_status\n");
		return -1;
	}
	if(img_lib_init(img_config_info.defblc_enable, img_config_info.sharpen_b_enable) < 0){
		printf("error: img_lib_init\n");
		return -1;
	}
	hdr_expo_num = img_config_info.expo_num;
	load_mctf_bin_hdr();
	if (map_bsb(fd_iav) < 0) {
		printf("map bsb failed\n");
		return -1;
	}
	get_is_info(&sensor_app_param, &hdr_app_param, sensor_name, &sensor_cfg);
	load_dsp_cc_table(fd_iav);
	load_adj_cc_table(sensor_name);

	if (img_hdr_config_sensor_info(&sensor_cfg) < 0) {
		printf("error: img_hdr_config_sensor_info\n");
		return -1;
	}
	if(img_hdr_load_sensor_param(&sensor_app_param, &hdr_app_param) < 0){
		printf("error: img_hdr_load_sensor_param\n");
		return -1;
	}

	if(img_hdr_config_lens_info(LENS_CMOUNT_ID)< 0){
		printf("error: img_hdr_config_lens_info\n");
		return -1;
	}
	if(img_hdr_lens_init() < 0){
		printf("error: img_hdr_lens_init\n");
		return -1;
	}

	memset(&cus_cntl_fp, 0, sizeof(hdr_cntl_fp_t));
	for(i = 0; i < hdr_expo_num; ++i){
		if(img_hdr_register_aaa_algorithm(&cus_cntl_fp, i) < 0){
			printf("error : img_hdr_register_aaa_algorithm\n");
			return -1;
		}
	}

	return 0;
}

//***************************************************************************************
// HDR auto-run pipeline stop and restart sample
#if 0
void restart_sample(int argc, char ** argv)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	// first start
	if(init_isp()<0){
		printf("error: init_isp\n");
		return -1;
	}
	if(img_start_hdr(fd_iav) !=0) {
		printf("error: img_start_hdr\n");
		exit(-1);
	}

	sleep(5);
	// first stop
	if(img_stop_hdr() != 0){
		printf("error: img_start_hdr\n");
		exit(-2);
	}
	if(img_lib_deinit() < 0){
		printf("error: img_start_hdr\n");
		return -1;
	}

	sleep(5);
	// second start
	if(init_isp()<0){
		printf("error: init_isp\n");
		return -1;
	}
	if(img_start_hdr(fd_iav) !=0) {
		printf("error: img_start_hdr\n");
		exit(-1);
	}

	while(1){
		sleep(10);
	}
}
#endif
//***************************************************************************************

int main(int argc, char ** argv)
{
	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if(init_isp()<0){
		printf("error: init_isp\n");
		return -1;
	}
	if(img_start_hdr(fd_iav) !=0) {
		printf("error: img_start_hdr\n");
		exit(-1);
	}

	pthread_t histo_thread;
	if((pthread_create(&histo_thread,NULL,(void*)histo_thread_proc,NULL)!=0))
		printf("create histo_thread fail!\n");
	int sockfd =-1;
	int new_fd =-1;

	struct sockaddr_in  their_addr;
	sockfd=SockServer_Setup(sockfd, ALL_ITEM_SOCKET_PORT);
	while(1)
	{
		if(new_fd!=-1)
			close(new_fd);
		socklen_t sin_size=sizeof(struct sockaddr_in);

//		signal(SIGFPE, SIG_IGN);
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


