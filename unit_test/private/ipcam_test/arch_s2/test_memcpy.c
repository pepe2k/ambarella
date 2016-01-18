/*
 * test_memcpy.c
 *
 * History:
 *    2010/9/6 - [Louis Sun] create it to test memcpy by GDMA
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
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
#include "amba_usb.h"
#include <signal.h>


int fd_iav;
u8 *bsb_mem;
u32 bsb_size;
int test_option = 0;


#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "t";

static struct option long_options[] = {
	{"test", 		NO_ARG, 0, 't'},
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\t test only"},
};

static void usage(void)
{
	u32 i;
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

	printf("    >test_memcpy  -t  \n" );

}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 't':
			test_option = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}

static int do_memcpy_test(void)
{
	int i;
	u32 input_size = 0x200000;
	iav_gdma_copy_ex_t gdma_param;
//prepare data
	for (i = 0 ; i< 0x10000; i++) {
		bsb_mem[i] = i & 0xFF;
	}

	gdma_param.usr_src_addr = (u32)bsb_mem;
	gdma_param.usr_dst_addr = (u32)bsb_mem + input_size;
	gdma_param.src_mmap_type = IAV_MMAP_BSB2;
	gdma_param.dst_mmap_type = IAV_MMAP_BSB2;

	gdma_param.src_pitch = 2048;
	gdma_param.dst_pitch = 2048;
	gdma_param.width = 2;
	gdma_param.height = 128;

	//copy to new position
	if (ioctl(fd_iav, IAV_IOC_GDMA_COPY_EX, &gdma_param) < 0) {
		printf("iav gdma copy failed \n");
		return -1;
	} else {
		printf("iav gdma copy ok \n");
	}

	return 0;

}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0)
		return -1;

	if (test_option) {
		if (do_memcpy_test() < 0)
			return -1;
	}
	return 0;
}

