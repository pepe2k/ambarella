/*
 * test_motdetect3a.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2010/04/22 - [Feiyu Lei] create this file base on test2.c
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

typedef struct roi_info_s {
	u16	x;
	u16	y;
	u16	x_width;
	u16	y_height;
	u16	threshold;
	u16	sensitivity;
	u16	valid;
} roi_info_t;

typedef struct aaa_ae_data_s {
         u32 lin_y;
         u32 non_lin_y; //maybe more useful
}aaa_ae_data_t;  //don't modify this structure

#define MAX_ROI_NUM					4
#define MAX_ROW   					8
#define MAX_COLUMN      				12
#define DEFAULT_KEY 					5678 //don't modify it
#define LUMA_DIFF_THRESHOLD 			300
#define SHM_DATA_SZ 					sizeof(aaa_ae_data_t)*96  //don't modify it

#ifndef ABS
#define ABS(x) ({				\
		int __x = (x);			\
		(__x < 0) ? -__x : __x;		\
            })
#endif

//default roi location values are as follows, you can change them as you like.
//ROI0 x = 0, y = 0, w = 6, h = 4 
//ROI1 x = 0, y = 4, w = 6, h = 4 
//ROI2 x = 6, y = 0, w = 6, h = 4 
//ROI3 x = 6, y = 4, w = 6, h = 4 
static roi_info_t  roi[MAX_ROI_NUM] = {
	{0, 0, 6, 4, 300, 100, 1},
	{0, 4, 6, 4, 300, 100, 1},
	{6, 0, 6, 4, 300, 100, 1},
	{6, 4, 6, 4, 300, 100, 1}	
};

static aaa_ae_data_t  cur_ae_data[96] ;
static int shmid = 0;
static char *shm = NULL;

static aaa_ae_data_t prev_ae_data[96];
static u32		tiles_md_flag[96];
static u32		tiles_threshold = 300;


static int init_shm_get( void )
{
	/*
	 * Locate the segment.
	 */
	if ((shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666)) < 0) {
		printf("3A have been off ! \n");
		perror("shmget");
		exit(1);
	}

	return 0;
}

static int get_shm_aedata( void )
{
	/*
	* Now we attach the segment to our data space.
	*/
	if ((shm = (char*)shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		exit(1);
	}

	return 0;
}


//default roi location values are as follows, you can change them .
//ROI0 x = 0, y = 0, w = 6, h = 4 
//ROI1 x = 0, y = 4, w = 6, h = 4 
//ROI2 x = 6, y = 0, w = 6, h = 4 
//ROI3 x = 6, y = 4, w = 6, h = 4 
//we divide screen to 4 ROIs according to above value
// ______________
//|		   |		|
//|	ROI0	   |	ROI2	|
//|______|______|
//|		   |		|
//|	ROI1	   |	ROI3	|
//|______|______|

static int md3a_set_roi_location( void ) //the unit is tile
{
	int i, chose_roi;		
	printf("Firstly choose a ROI. Index = ? (0~3)");
	scanf("%d", &i);
	chose_roi = i;
	if (chose_roi > MAX_ROI_NUM - 1) {
		printf("ROI index %d out of range(0~3)\n", chose_roi);
		return -1;
	}

	printf(" roi location unit is tile,  which is in 12x8 table \n");

	printf("set_roi_location input ROI x = ? (0~%d)\n", MAX_COLUMN-1);
	scanf("%d", &i);
	roi[chose_roi].x = i;

	printf("set_roi_location input ROI y = ? (0~%d)\n", MAX_ROW-1);
	scanf("%d", &i);
	roi[chose_roi].y = i;

	printf("set_roi_location input ROI width = ? (1~%d)\n", MAX_COLUMN - roi[chose_roi].x);
	scanf("%d", &i);
	roi[chose_roi].x_width = i;

	printf("set_roi_location input ROI height = ? (1~%d)\n", MAX_ROW - roi[chose_roi].y);
	scanf("%d", &i);
	roi[chose_roi].y_height= i;

	if (roi[chose_roi].x  > MAX_COLUMN -1) {
		printf("md_motbuf_set_roi_info invalid x %u  valid range 0 ~ %u\n",
			roi[chose_roi].x, MAX_COLUMN - 1);
		return -1;
	}
	if (roi[chose_roi].y > MAX_ROW-1) {
		printf("md_motbuf_set_roi_info invalid y %u  valid range 0 ~ %u \n",
			roi[chose_roi].y, MAX_ROW - 1);
		return -1;
	}
	if ((roi[chose_roi].x_width+roi[chose_roi].x) > MAX_COLUMN) {
		printf("md_motbuf_set_roi_info invalid width %u  valid range 1 ~ %u \n",
			roi[chose_roi].x_width, MAX_COLUMN  - roi[chose_roi].x);
		return -1;
	}
	if ((roi[chose_roi].y_height+roi[chose_roi].y) > MAX_ROW) {
		printf("md_motbuf_set_roi_info invalid height %u  valid range 1 ~ %u \n",
			roi[chose_roi].y_height, MAX_ROW - roi[chose_roi].y);
		return -1;
	}

	return 0;
}

static int md3a_show_roi_location(void)
{
	int i;
	printf(" roi location unit is tile,  which is in 12x8 table \n");
	for (i = 0; i < MAX_ROI_NUM; i++) {
		printf("roi %d x = %d y = %d width = %d height = %d \n",
			i, roi[i].x, roi[i].y, roi[i].x_width, roi[i].y_height);										
	}

	return 0;
}


static int md3a_set_tiles_threshold(void)
{
	int i, chose_roi;		
	printf("Firstly choose a ROI. Index = ? (0~3)");
	scanf("%d", &i);
	chose_roi = i;
	if (chose_roi > MAX_ROI_NUM - 1) {
		printf("ROI index %d out of range(0~3)\n", chose_roi);
		return -1;
	}

	printf("set tiles luma difference threshold = ? (0~1000)\n");
	scanf("%d", &i);
	tiles_threshold = i;

	return 0;
}


static int md3a_show_all_tiles_status(void)
{
	int j, k, m;
	while (1) {
		printf("\n");
		usleep(500*1000); // 0.5s
		memcpy(cur_ae_data, shm, SHM_DATA_SZ);
		for (j = 0; j < MAX_ROW; j++) {
			for (k = 0; k < MAX_COLUMN; k++) {
				if ((m+1) % MAX_COLUMN == 0)
					printf("\n");
				m = j * MAX_COLUMN + k;
				if (ABS(prev_ae_data[m].lin_y - cur_ae_data[m].lin_y) > tiles_threshold)
					tiles_md_flag[m] = 1;	
				else
					tiles_md_flag[m] = 0;
				printf("%2d, ",  tiles_md_flag[m]);
			}
		}
		memcpy(prev_ae_data, cur_ae_data, SHM_DATA_SZ);
	}

	return 0;
}


static int md3a_show_each_roi_status(void)
{
	int i, j, k, m;
	int roi_mark[4] ;
 
	printf("1 means motion happens, 0 means no motion happens \n");

	while (1) {
		printf("\n");
		usleep(500*1000); // 0.5s
		roi_mark[0] = roi_mark [1] = roi_mark[2]= roi_mark[3] = 0;
		memcpy(cur_ae_data, shm, SHM_DATA_SZ);
		for (j = 0; j < MAX_ROW; j++) {
			for (k = 0; k < MAX_COLUMN; k++) {
				m = j * MAX_COLUMN + k;
				if (ABS(prev_ae_data[m].lin_y - cur_ae_data[m].lin_y) > tiles_threshold)
					tiles_md_flag[m] = 1;	
				else
					tiles_md_flag[m] = 0;
			}
		}

		//check if the tile which motion has happened in some ROI
		for (j = 0; j < MAX_ROW; j++) {
			for (k = 0; k < MAX_COLUMN; k++) {
				m = j * MAX_COLUMN + k;
				if (tiles_md_flag[m] == 1) {
					for (i = 0; i < MAX_ROI_NUM; i++) {
						if (roi_mark[i] == 0) {
							if (k >= roi[i].x && j>= roi[i].y &&
							      k <= roi[i].x+roi[i].x_width-1 &&
							      j<= roi[i].y+roi[i].y_height-1) {
								roi_mark[i] = 1;
							}
						}
					}
				}
			}
		}

		printf(" roi [0] %d  roi [0] %d roi [0] %d  roi [0] %d \n",
			roi_mark[0] ,roi_mark[1] , roi_mark[2] , roi_mark[3]);

		memcpy(prev_ae_data, cur_ae_data, SHM_DATA_SZ);
	}

	return -1;
}


static void sigstop( )
{	
	printf("test_motdetect3a has been killed \n");	
	exit(1);
}

void md3a_show_general_prompt()
{
	printf("\n");
	printf("Please select\n");
	printf("0 - md3a_set_roi_location\n");
	printf("1 - md3a_show_roi_location\n");
	printf("2 - md3a_set_tiles_threshold\n");
	printf("3 - md3a_show_all_tiles_status\n");
	printf("4 - md3a_show_each_roi_status\n");
}

int main(int argc, char **argv)
{
	int ch;

	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	init_shm_get();
	get_shm_aedata(); //get ae data pointer from AAA share memory

	while (1) {
		md3a_show_general_prompt();
		scanf("%d", &ch);
		printf("your choice is :  %d \n", ch);
		switch (ch) {
			case 0:
				if (md3a_set_roi_location() <0) {
					printf("set roi location failed \n");
				}
				break;
			case 1:
				if (md3a_show_roi_location() <0) {
					printf("show roi location failed \n");
				}
				break;
			case 2:
				if (md3a_set_tiles_threshold() <0) {
					printf("set tiles threshold failed \n");
				}
				break;
			case 3:
				if (md3a_show_all_tiles_status() <0) {
					printf("show all tiles status failed \n");
				}
				break;
			case 4:
				if (md3a_show_each_roi_status() <0) {
					printf("show each roi status failed \n");
				}
				break;
			default:
				printf("unknown option found: %c\n", ch);
				break;
		}
	}

	return 0;
}


