/**
 * am_new.cpp
 *
 * History:
 *    2010/05/28 - [Louis Sun] created file
 *
 * Copyright (C) 2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define DEBUG_DELAY
#ifdef DEBUG_DELAY

#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include "am_types.h"
#include "osal.h"
#include "am_hwtimer.h"

static AM_U32 *G_TIMER2_COUNTER;
static int G_APB_FREQ;

int AM_open_hw_timer()
{
	int debug_fd;
	char *debug_mem;
	AM_U8* apb_mem;
	struct amba_debug_mem_info debug_mem_info;
	static int initialized_hw_timer = 0;
	char filebuf[1024];
	int proc_fd, read_count;
	char *apb_str, *hz_str;
	static CMutex* pMutex = CMutex::Create();

	#define TIMER2_APB_OFFSET 	0xB000
       #define MMAP_SIZE  0x40

	if(!pMutex || initialized_hw_timer)
		return 0;

	pMutex->Lock();
	
	if ((debug_fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		pMutex->Unlock();
		return -1;
	}

	if (ioctl(debug_fd, AMBA_DEBUG_IOC_GET_MEM_INFO, &debug_mem_info) < 0) {
		perror("AMBA_DEBUG_IOC_GET_MEM_INFO");
		pMutex->Unlock();
		return -1;
	}

	debug_mem = (char *)mmap(NULL, (TIMER2_APB_OFFSET + MMAP_SIZE),
		PROT_READ | PROT_WRITE, MAP_SHARED, debug_fd, debug_mem_info.modules[AMBA_DEBUG_MODULE_APB].start);

	if ((int)debug_mem < 0) {
		perror("mmap");
		pMutex->Unlock();
		return -1;
	}

	//do timer register read out 
	//
	//apb_mem = (AM_U8 *)(debug_mem + debug_mem_info.ahb_size);
	G_TIMER2_COUNTER = (AM_U32*)(debug_mem + TIMER2_APB_OFFSET);
	
	//get APB frequency
	memset(filebuf, 0, sizeof(filebuf));
	if ((proc_fd = open("/proc/ambarella/performance", O_RDONLY)) < 0) {
		perror("/dev/ambarella/performance");
		pMutex->Unlock();
		return -1;
	}
	if ((read_count = read(proc_fd, filebuf, sizeof(filebuf)-1)) < 0) {
		perror("read proc file");
		pMutex->Unlock();
		return -1;
	}
	close(proc_fd);
	apb_str = strstr(filebuf, "APB:");
	apb_str += strlen("APB:");
	hz_str = strstr(apb_str, "Hz");
	if (!hz_str) {
		printf("cannot find HZ \n");
	}else {
		hz_str[0] = '\0';
	}
	G_APB_FREQ = atoi(apb_str);
	AM_PRINTF("APB frequency is %d\n", G_APB_FREQ);
	initialized_hw_timer = 1;
	pMutex->Unlock();
	pMutex->Delete();
	pMutex = NULL;
	
	return 0;
}

//get APB frequency level of accuracy counter (66 MHz) 
AM_U32 AM_get_hw_timer_count()
{
	return 	*G_TIMER2_COUNTER;
}


//a version to print time diff in micro second level accuracy
void AM_print_timing()
{
	static AM_U32 old_time_count = 0;
	AM_U32 time_count, diff, us_diff;
	time_count = AM_get_hw_timer_count();
	diff = old_time_count - time_count;
	old_time_count = time_count;
	us_diff = diff / (G_APB_FREQ/1000000);		
	printf("diff is %d uS \n", us_diff);
}

// convert the ABP freq to us
AM_U32 AM_hwtimer2us(AM_U32 hwtimer)
{
    AM_U32 time_count, diff, us_diff;    
    time_count = hwtimer / (G_APB_FREQ/1000000);
//    printf("diff is %d uS [%d]us\n", hwtimer, time_count);
    return time_count;
}

#endif
 
