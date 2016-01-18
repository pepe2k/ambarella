/**********************************************************
 * test_mctf.c
 *
 * History:
 *	2012/12/24 - [Teng Huang] created
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_usb.h"
#include <signal.h>
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"


static u8 mctf_mask_level_16[16] =
{0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF};
static u8 strength_array[16] =
{0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8};

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define NO_ARG		0
#define HAS_ARG		1
#define OPTIONS_BASE		0

enum numeric_short_options {
	DISABLE_PRIVACY_MASK = OPTIONS_BASE,
};


#define MAX_MASK_BUF_NUM		(8)

static int num_block_x = 0;
static int num_block_y = 0;


static u8 * pm_start_addr[MAX_MASK_BUF_NUM];
static u8 pm_buffer_id = 0;
static u32 pm_buffer_size = 0;
static u16 pm_buffer_pitch = 0;
static int verbose = 0;
static int fd_iav =-1;

static int strength =8;
static int tile_id =0;


static int get_tmctf_mask_buffer_id(void)
{
	iav_privacy_mask_info_ex_t mask_info;
	if (ioctl(fd_iav, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
		return -1;
	}
	pm_buffer_pitch = mask_info.buffer_pitch;
	if (verbose)
	{
		printf("Next PM buffer id to use : %d.\n", pm_buffer_id);
	}
	return 0;
}

static int map_tmctf_mask(void)
{
	int i;
	iav_mmap_info_t mmap_info;

	if (ioctl(fd_iav, IAV_IOC_MAP_PRIVACY_MASK_EX, &mmap_info) < 0) {
		perror("IAV_IOC_MAP_PRIVACY_MASK_EX");
		return -1;
	}
	if (get_tmctf_mask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}

	pm_buffer_size = mmap_info.length / MAX_MASK_BUF_NUM;
	for (i = 0; i < MAX_MASK_BUF_NUM; ++i) {
		pm_start_addr[i] = mmap_info.addr + i * pm_buffer_size;
	}

	return 0;
}


//calculate privacy mask size for memory filling,  privacy mask take effects on macro block (16x16)
static int calc_tmctf_mask_size(void)
{
	iav_source_buffer_setup_ex_t setup;

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}
	num_block_x = ROUND_UP(setup.size[0].width, 16) / 16;
	num_block_y = ROUND_UP(setup.size[0].height, 16) / 16;

	if (verbose) {
		printf("num_block_x %d, num_block_y %d \n", num_block_x, num_block_y);
	}
	return 0;
}

static int fill_tmctf_mask_mem()
{
	int i, j;
	int m,n;
	iav_mctf_filter_strength_t *pm = NULL;
	iav_mctf_filter_strength_t *pm_backup;
	if ((num_block_x == 0) || (num_block_y == 0)) {
		printf("privacy mask block number error \n");
		return -1;
	}
	if (get_tmctf_mask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}
	pm = (iav_mctf_filter_strength_t *)pm_start_addr[pm_buffer_id];
	pm_backup =pm;
	int NUM_TILE =4;
	int tile_w =num_block_x/NUM_TILE;
	int tile_h =num_block_y/NUM_TILE;
	for(i =0;i<NUM_TILE;i++)
	{
		for(j =0;j<NUM_TILE;j++)
		{
			for(m =i*tile_h;m<=(i+1)*tile_h;m++)
			{
				for(n =j*tile_w;n<=(j+1)*tile_w;n++)
				{
					u8 tmp =i*NUM_TILE+j;
					pm->u =strength_array[tmp];
					pm->v= strength_array[tmp];
					pm->y= strength_array[tmp];
					pm=pm_backup+ m*num_block_x+n;
				//	printf("n %d,m %d,%d\n",n,m, m*num_block_x+n);
				}
			}
		}
	}
	return 0;
}

//call IOCTL to set privacy mask
static int set_tmctf_mask(int enable, int save_to_next)
{
	iav_digital_zoom_privacy_mask_ex_t dptz_pm;
	memset(&dptz_pm, 0, sizeof(dptz_pm));

	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &dptz_pm.zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM_EX");
		return -1;
	}

	dptz_pm.privacy_mask.enable = enable;
	dptz_pm.privacy_mask.buffer_addr = pm_start_addr[pm_buffer_id];
	dptz_pm.privacy_mask.buffer_pitch = pm_buffer_pitch;
	dptz_pm.privacy_mask.buffer_height = num_block_y;

	if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX,
	    &dptz_pm) < 0) {
		perror("IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX");
		return -1;
	}

	return 0;
}

static void sigstop()
{
	memset(strength_array,mctf_mask_level_16[8],16);
	fill_tmctf_mask_mem();
	set_tmctf_mask(1, 0);
	exit(1);
}
#if 0
static u8 mv_base_flag =0;
static int current_stream =0;
u32 avg_mv =0;
u32* motion_mb;
int motion_mb_count=0;
static video_mctf_info_t mctf_param_test=
{// alpha   t0   t1 max_chg 	radius  3d spat  adj
	{
		{32,	40,60,	255,		255, 255,60,  	192},
		{32,	40,60,	255,		255, 255,60,  	192},
		{32,	40,60,	255,		255, 255,60,  	192},
	},
	1,
};
static int display_mv_statistics(int fd_iav,iav_enc_statis_info_ex_t * stat_info)
{
	iav_motion_vector_ex_t * mv_dump = NULL;
	iav_encode_format_ex_t encode_format;
	u32 stream_pitch, stream_height, unit_sz = 0;
	int i, j;

	encode_format.id = (1 << current_stream);
	ioctrl(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format);
	stream_pitch = ROUND_UP(ROUND_UP(encode_format.encode_width, 16) / 16 *
		sizeof(iav_motion_vector_ex_t), 32);
	if (stream_pitch != stat_info->mvdump_pitch) {
		printf("Incorrect MV dump pitch [%d] for stream [%d] of pitch [%d].\n",
			stat_info->mvdump_pitch, current_stream, stream_pitch);
		return -1;
	}
	stream_height = ROUND_UP(encode_format.encode_height, 16) / 16;
	unit_sz = stream_pitch * stream_height;
	if (unit_sz != stat_info->mvdump_unit_size) {
		printf("Incorrect MV dump unit size [%d] for stream [%d] of size [%d].\n",
			stat_info->mvdump_unit_size, current_stream, unit_sz);
		return -1;
	}
	stream_pitch /= sizeof(iav_motion_vector_ex_t);
	mv_dump = (iav_motion_vector_ex_t *)stat_info->mv_start_addr;
	printf("Motion Vector X Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].x);
		}
		printf("\n");
	}
/*	printf("Motion Vector Y Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].y);
		}
		printf("\n");
	}*/
/*	u32* mv_out =(u32*)malloc(stream_height*stream_pitch*sizeof(u32));
	if(motion_mb ==NULL)
		motion_mb =(u32*)malloc(stream_height*stream_pitch*sizeof(u32));
	int count=0;
	for (i = 0; i < stream_height; ++i)
	{
		for (j = 0; j < stream_pitch; ++j)
		{
			int pos =i*stream_pitch+j;
			mv_out[pos]=SQRT(mv_dump[pos].x,mv_dump[pos].y);
		//	mv_out[pos] =mv_dump[pos].y>0?mv_dump[pos].y:(-mv_dump[pos].y);
			printf("%d ",mv_out[pos]);
			if(mv_out[pos]!=0)
			{
				avg_mv+=mv_out[pos];
				count++;
			}
		}
	//	printf("\n");
	}
	printf("\n");
	printf("avg_mv.1 =%d\n",avg_mv);
	avg_mv /=(count+1);
	printf("avg_mv .2=%d\n",avg_mv);
	for (i = 0; i < stream_height; ++i)
	{
		for (j = 0; j < stream_pitch; ++j)
		{
			int pos =i*stream_pitch+j;
			if(mv_out[pos] >avg_mv)
			{
				motion_mb[pos] =1;
			//	motion_mb_count++;
			}
			else
				motion_mb[pos] =0;
		}
	}*/

	return 0;
}
static int get_mv_statistics(int fd_iav)
{

	iav_enc_statis_info_ex_t stat_info;
	AM_IOCTL(fd_iav, IAV_IOC_FETCH_STATIS_INFO_EX, &stat_info);
	if (display_mv_statistics(fd_iav,&stat_info) < 0) {
		printf("Failed to display motion vector!\n");
		return -1;
	}
	AM_IOCTL(fd_iav, IAV_IOC_RELEASE_STATIS_INFO_EX, &stat_info);
	return 0;
}
static int load_mctf_bin()
{
	int i;
	int file, count;
	u8* bin_buff;
	char filename[256];
	idsp_one_def_bin_t one_bin;
	idsp_def_bin_t bin_map;

	img_dsp_get_default_bin_map(&bin_map);
	bin_buff = malloc(27600);
	for (i = 0; i<bin_map.num;i++)
	{
		memset(filename,0,sizeof(filename));
		memset(bin_buff,0,sizeof(bin_buff));
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
		img_dsp_load_default_bin((u32)bin_buff, one_bin);;
	}
	free(bin_buff);
	return 0;
}
#endif

static int quit_flag =0;
static int show_menu(void)
{
	printf("\n================================================\n");
	printf("  a --set all tile strength\n");
	printf("  l --set local tile strength\n");
	printf("  h --set half screen strength\n");
	printf("  t --set 2*2 screen strength\n");
//	printf("  f  --set 4*4 screen strength\n");
	printf("  g --get current strength\n");
	printf("  q --quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}
int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);
//	int rval =-1;
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0)
	{
		perror("/dev/iav");
		return -1;
	}

	if (map_tmctf_mask() < 0)
	{
		printf("map privacy mask failed \n");
		return -1;
	}

	if (calc_tmctf_mask_size() < 0)
	{
		printf("calc privacy mask size failed \n");
		return -1;
	}
#if 0
	if(mv_base_flag)
	{
		iav_enc_statis_config_ex_t stat_config;
		memset(&stat_config, 0, sizeof(stat_config));
		stat_config.id = (1 << current_stream);
		stat_config.enable_mv_dump = 1;
		stat_config.mvdump_division_factor = 1;
		ioctrl(fd_iav, IAV_IOC_SET_STATIS_CONFIG_EX, &stat_config);
		get_mv_statistics(fd_iav);
	}
#endif

	show_menu();
	char error_opt;
	char key = getchar();
	while (key) {
		quit_flag = 0;
		error_opt = 0;
		switch (key) {
		case 'a':
		{
			printf("input all strength:[0~15]\n");
			scanf("%d",&strength);
			memset(strength_array,mctf_mask_level_16[strength],16);
			break;
		}
		case 'h':
		{
			printf("input left strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[0] =mctf_mask_level_16[strength];
			strength_array[1] =mctf_mask_level_16[strength];
			strength_array[4] =mctf_mask_level_16[strength];
			strength_array[5] =mctf_mask_level_16[strength];
			strength_array[8] =mctf_mask_level_16[strength];
			strength_array[9] =mctf_mask_level_16[strength];
			strength_array[12] =mctf_mask_level_16[strength];
			strength_array[13] =mctf_mask_level_16[strength];
			printf("input right strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[2] =mctf_mask_level_16[strength];
			strength_array[3] =mctf_mask_level_16[strength];
			strength_array[6] =mctf_mask_level_16[strength];
			strength_array[7] =mctf_mask_level_16[strength];
			strength_array[10] =mctf_mask_level_16[strength];
			strength_array[11] =mctf_mask_level_16[strength];
			strength_array[14] =mctf_mask_level_16[strength];
			strength_array[15] =mctf_mask_level_16[strength];
			break;
		}
		case 'l':
		{
			printf("0\t1\t2\t3\n");
			printf("4\t5\t6\t7\n");
			printf("8\t9\t10\t11\n");
			printf("12\t13\t14\t15\n\n");
			printf("input the tile id:[0~15]\n");
			scanf("%d",&tile_id);
			printf("input the strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[tile_id] =mctf_mask_level_16[strength];
			break;
		}
		case 't':
		{
			printf("input upleft strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[0] =mctf_mask_level_16[strength];
			strength_array[1] =mctf_mask_level_16[strength];
			strength_array[4] =mctf_mask_level_16[strength];
			strength_array[5] =mctf_mask_level_16[strength];
			printf("input upright strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[2] =mctf_mask_level_16[strength];
			strength_array[3] =mctf_mask_level_16[strength];
			strength_array[6] =mctf_mask_level_16[strength];
			strength_array[7] =mctf_mask_level_16[strength];
			printf("input downleft strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[8] =mctf_mask_level_16[strength];
			strength_array[9] =mctf_mask_level_16[strength];
			strength_array[12] =mctf_mask_level_16[strength];
			strength_array[13] =mctf_mask_level_16[strength];
			printf("input downright strength:[0~15]\n");
			scanf("%d",&strength);
			strength_array[10] =mctf_mask_level_16[strength];
			strength_array[11] =mctf_mask_level_16[strength];
			strength_array[14] =mctf_mask_level_16[strength];
			strength_array[15] =mctf_mask_level_16[strength];
			break;
		}
		case 'g':
		{
			int i=0,j=0;
			for(i=0;i<4;i++)
			{
				for(j=0;j<4;j++)
					printf("%d,",strength_array[i*4+j]);
				printf("\n");
			}
			break;
		}

		case 'q':
			quit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (quit_flag)
		{
			sigstop();
			break;
		}
		if (error_opt == 0) {
			show_menu();
		}
		fill_tmctf_mask_mem();
		set_tmctf_mask(1,1);
		key = getchar();
	}

	return 0;
}

