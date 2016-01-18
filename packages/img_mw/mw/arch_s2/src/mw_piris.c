/**********************************************************************
 *
 * mw_piris.c
 *
 * History:
 * 2014/4/17 - [Lei Hong] Create this file
 * Copyright (C) 2007 - 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include "img_struct_arch.h"
#include "mw_struct.h"
#include "mw_defines.h"
#include "mw_piris.h"
#include "img_api_arch.h"
#include "amba_p_iris.h"


#define PIRIS_IGNORE_CHECK		10
#define PIRIS_ALLOW_RANGE		70
#define IRIS_DEVICE_NAME "/dev/amb_iris"
#define PIRIS_HIGHT_STEP 		15
#define PIRIS_LOW_STEP			3
#define PIRIS_EXIT_STEP			5
#define LUMA_DIFF_THROTTLE		90
#define LUMA_STEP_CHANGE_RANGE	100
#define PIRIS_MAX_STEP          100
#define PIRIS_MIN_STEP			0


int piris_step = PIRIS_LOW_STEP;
int piris_exit_step = PIRIS_EXIT_STEP;
static int fd_iris = 0;
static int piris_enable = 0;
static int piris_run_flag = 0;

typedef enum {
	MW_PIRIS_IDLE = 0,
	MW_PIRIS_FLUCTUATE,
	MW_PIRIS_STATE_NUM,
} MW_PIRIS_STATE;

typedef enum {
	PIRIS_NEED_ADJUST = 0,
	PIRIS_EXIT,
	PIRIS_IDLE_LOOP,
} PIRIS_CHECK_STATE;


static int piris_move(int steps)
{
	int ret;
	ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_MOVE_STEPS, steps);
	if (ret < 0) {
		printf("AMBA_IOC_P_IRIS_MOVE_STEPS ERROR \n");
		return -1;
	}
	return 0;
}

static int piris_get_pos(void)
{
	int pos, ret;
	if ((ret = ioctl(fd_iris, AMBA_IOC_P_IRIS_GET_POS, &pos)) < 0) {
		printf("AMBA_IOC_P_IRIS_GET_POS ERROR \n");
		return ret;
	}
	return pos;
}


static int piris_adjust(int luma_diff)
{
	int pos = 0;

	if (luma_diff < 0) {
		MW_DEBUG("piris_adjust over dark luma_diff is %d, piris_step is %d \n",
			luma_diff,piris_step);
		if (ABS(luma_diff) > LUMA_DIFF_THROTTLE) {
			pos = piris_get_pos();
			if (pos > get_piris_exit_step()) {
				pos = pos - get_piris_exit_step();
				piris_move(-pos);
			}
		} else {
			piris_move(-piris_step);
		}
	} else if (luma_diff > 0) {
		MW_DEBUG("piris_adjust over light luma_diff is %d,piris_step is %d \n",
			luma_diff,piris_step);
		if (luma_diff > LUMA_DIFF_THROTTLE) {
			piris_move(PIRIS_HIGHT_STEP);
		} else {
			piris_move(piris_step);
		}
	}

	return 0;
}

int set_piris_exit_step(int step)
{
	if (step > PIRIS_MAX_STEP || step < PIRIS_MIN_STEP) {
		printf("piris set exit step error, step is %d. \n",step);
		return -1;
	}
	piris_exit_step = step;
	return 0;
}

int get_piris_exit_step()
{
	return piris_exit_step;
}


static int piris_check(int luma_diff)
{
	static int last_low_luma_diff = 0;
	static int count = 0;
	int pos;

	pos = piris_get_pos();

	MW_DEBUG("piris_check luma_diff is %d, last_luma_diff is %d, pos is %d \n",
		luma_diff,last_low_luma_diff,pos);

	if ((pos <= get_piris_exit_step())&&(1 == piris_run_flag)) {
		last_low_luma_diff = luma_diff;
		piris_step = PIRIS_LOW_STEP;
		count = 0;
		return PIRIS_EXIT;
	}

	if (ABS(luma_diff) <= PIRIS_IGNORE_CHECK) {
		last_low_luma_diff = luma_diff;
		piris_step = PIRIS_LOW_STEP;
		count = 0;
		return PIRIS_IDLE_LOOP;
	}

	if (((luma_diff < 0) && (last_low_luma_diff > 0)) || ((luma_diff > 0) && (last_low_luma_diff < 0))) {
		if (piris_step > 1) {
			piris_step--;
		} else {
			if (count >= 1) {
				last_low_luma_diff = luma_diff;
				piris_step = PIRIS_LOW_STEP;
				count = 0;
				return PIRIS_IDLE_LOOP;
			}
			count++;
		}
	}

	last_low_luma_diff = luma_diff;

	return PIRIS_NEED_ADJUST;
}



static int piris_luma_control(int run,  int luma_diff,  int *state)
{
	static int iris_state = MW_PIRIS_IDLE;
	int check_value = 0;

	if (0 == piris_enable || !run) {
		iris_state = MW_PIRIS_IDLE;
		goto luma_control_exit;
	}

	switch (iris_state) {
	case MW_PIRIS_IDLE:
		iris_state = MW_PIRIS_FLUCTUATE;
		break;

	case MW_PIRIS_FLUCTUATE:

		check_value = piris_check(luma_diff);

		if (PIRIS_EXIT == check_value) {
			iris_state = MW_PIRIS_IDLE;
			piris_run_flag = 0;
		} else if (PIRIS_IDLE_LOOP == check_value) {
			iris_state = MW_PIRIS_FLUCTUATE;
			piris_run_flag = 1;
		} else {
			piris_adjust(luma_diff);
			iris_state = MW_PIRIS_FLUCTUATE;
		}
		break;

	default:
		assert(0);
		break;
	}

luma_control_exit:
	*state = (iris_state != MW_PIRIS_IDLE);
	MW_INFO("[%d] luma_diff %d,state %d\n",run, luma_diff, *state);
	return 0;
}

int enable_piris(u8 enable)
{
	dc_iris_cntl iris_cntl = NULL;

	if (enable) {
		if (piris_enable) {
			printf("piris reinit \n");
			return 0;
		}
		/* open the device */
		if ((fd_iris = open(IRIS_DEVICE_NAME , O_RDWR, 0)) < 0) {
			printf("open piris dev error");
			return -1;
		}

		piris_enable = 1;
		iris_cntl = piris_luma_control;
		img_register_iris_cntl(iris_cntl);
		MW_INFO("Function luma_control is registered into AAA algo.\n");
	} else {
		if (fd_iris) {
			if (close(fd_iris) < 0) {
				printf("piris close failed \n");
				return -1;
			}
			fd_iris = 0;
		}
		piris_enable = 0;
		iris_cntl = NULL;
		img_register_iris_cntl(iris_cntl);
		MW_INFO("Function luma_control is unregistered into AAA algo.\n");
	}

	return 0;
}
