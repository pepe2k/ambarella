/*
 * test_piris.c
 *
 * History:
 *	2012/07/02 - [Louis Sun] create
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
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
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>

#include "ambas_common.h"
#include <signal.h>
#include "amba_p_iris.h"


// the device file handle
int fd_iris;
int iris_init_flag = 0;
int iris_reset_flag = 0;
int iris_move_flag = 0;
int iris_move_steps = 0;
int iris_show_cfg_flag = 0;
int iris_show_pos_flag = 0;

#define IRIS_DEVICE_NAME "/dev/amb_iris"

#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"init", NO_ARG, 0, 'i'},
	{"reset", NO_ARG, 0, 'r'},
	{"move", HAS_ARG, 0, 'm'},
	{"show-cfg", NO_ARG, 0, 's'},
	{"show-pos", NO_ARG, 0, 'p'},
	{0, 0, 0, 0}
};

static const char *short_options = "irm:sp";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
        {"", "\tinit device state"},
        {"", "\treset device to init mechanical position, and clear statistics"},
        {"-n~n", "\tmove iris to n steps"},
        {"", "\tshow current config"},
        {"", "\tshow current position"},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		case 'i':
			iris_init_flag = 1;
			break;
		case 'r':
			iris_reset_flag = 1;
			break;
		case 'm':
			iris_move_flag = 1;
			iris_move_steps = atoi(optarg);
			break;
		case 's':
			iris_show_cfg_flag = 1;
			break;
		case 'p':
			iris_show_pos_flag = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

void usage(void)
{
	int i;

	printf("test_piris usage:\n");
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


int p_iris_init(void)
{
    int ret;
    amba_p_iris_cfg_t cfg;

    cfg.gpio_id[0] = 52;	 // IN1 for LB1909M , on sensor daughter board
    cfg.gpio_id[1] = 50; // IN2 for LB1909M , on sensor daughter board
    cfg.gpio_id[2] = 53; // EN for LB1909M , on sensor daughter board
    cfg.max_mechanical = 100;
    cfg.max_optical = 100;
    cfg.min_mechanical = 0;
    cfg.min_optical = 0;
    cfg.timer_period = 10;  //every 10ms

    ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_INIT_CFG, &cfg);
    if (ret < 0 ) {
        perror("AMBA_IOC_P_IRIS_INIT_CFG");
        return -1;
    }
    return 0;
}

int p_iris_reset(void)
{
	int ret;
	ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_RESET, 0);
	if (ret < 0) {
		perror("AMBA_IOC_P_IRIS_RESET");
		return -1;
	}
	return 0;
}

int p_iris_move(int steps)
{
	int ret;
	ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_MOVE_STEPS, steps);
	if (ret < 0) {
		perror("AMBA_IOC_P_IRIS_MOVE_STEPS");
		return -1;
	}
	return 0;
}

int p_iris_show_cfg(void)
{
	int ret;
	amba_p_iris_cfg_t cfg;
	if ((ret= ioctl(fd_iris, AMBA_IOC_P_IRIS_GET_CFG, &cfg)) < 0) {
		perror("AMBA_IOC_P_IRIS_GET_CFG");
		return ret;
	}

	printf("P-IRIS CONFIG Dump \n");
	printf("GPIO_1 :%d, GPIO_2: %d \n", cfg.gpio_id[0], cfg.gpio_id[1]);
	printf("Mechanical step range %d ~ %d \n", cfg.min_mechanical, cfg.max_mechanical);
	printf("Optical step range %d ~ %d \n", cfg.min_optical, cfg.max_optical);
	printf("timer period %dms \n", cfg.timer_period);

	return 0;
}

int p_iris_show_pos(void)
{
	int pos, ret;
	if ((ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_GET_POS, &pos)) < 0) {
		perror("AMBA_IOC_P_IRIS_GET_POS");
		return ret;
	}
	printf("P-IRIS position: %d \n", pos);
	return 0;
}


int main(int argc, char **argv)
{
	// open the device
	if ((fd_iris = open(IRIS_DEVICE_NAME , O_RDWR, 0)) < 0) {
		perror(IRIS_DEVICE_NAME);
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;


        //check param
        if (iris_init_flag || iris_reset_flag ||iris_move_flag )
            iris_show_pos_flag =1;

	if (iris_init_flag) {
		if (p_iris_init() < 0)
			return -1;
	}

	if (iris_reset_flag) {
		if (p_iris_reset() < 0)
			return -1;
	}

	if (iris_move_flag) {
		if (p_iris_move(iris_move_steps) < 0)
			return -1;
	}

	if (iris_show_cfg_flag) {
		if (p_iris_show_cfg() < 0)
			return -1;
	}

	if (iris_show_pos_flag) {
		if(p_iris_show_pos() < 0)
			return -1;
	}

	return 0;
}

