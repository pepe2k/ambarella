/*
 * iav_timer.c
 *
 * History:
 *	2012/02/03 - [Jian Tang] created to enhance polling read out mode
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "amba_dsp.h"
#include "dsp_api.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "utils.h"
#include "iav_mem.h"
#include "iav_api.h"
#include "iav_priv.h"


#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>


#define MS_TO_NS(x)	((u32)(((u32)x) * (u32)1000000L))


typedef enum {
	DEFAULT_HRTIMER_PERIOD_MS = 3,
} HRTIMER_PERIOD_IN_MS;


//make the hr timer to be periodical
static int hrtimer_prepare_for_next(iav_hrtimer_t * hrtimer)
{
	ktime_t ktime = ktime_set(0, MS_TO_NS(hrtimer->period_in_ms));
	return highres_timer_start(&hrtimer->hr_timer, ktime, HRTIMER_MODE_REL);
}

static enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	iav_hrtimer_t * hrtimer;
	//use container_of trick to get hrtimer pointer
	hrtimer = (iav_hrtimer_t *)container_of(timer, struct iav_hrtimer_s, hr_timer);
	//hrtimer cannot be zero
	if (!hrtimer) {
		BUG();
	}
	//up sem only if there is someone to wait for
	if (hrtimer->wait_count > 0) {
		up(&hrtimer->sem_ready);
		hrtimer->wait_count --;
	}

	//always trigger next timer
	hrtimer_prepare_for_next(hrtimer);
	return HRTIMER_NORESTART;
}

int iav_hrtimer_init(int ms, iav_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		iav_printk("hrtimer already init, error\n");
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_HRTIMER_PERIOD_MS;
	}
	//set hr timer
	memset(hrtimer, 0, sizeof(iav_hrtimer_t));

	sema_init(&hrtimer->sem_ready, 0);
	hrtimer->period_in_ms = ms;
	hrtimer->wait_count = 0;
	highres_timer_init(&hrtimer->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->hr_timer.function = &hrtimer_callback;
	hrtimer->method = 0;
	hrtimer->enable = 1;

	return 0;
}

int iav_hrtimer_deinit(iav_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		highres_timer_cancel(&hrtimer->hr_timer);
		hrtimer->enable = 0;
	}
	memset(hrtimer, 0, sizeof(iav_hrtimer_t));
	return 0;
}

int iav_hrtimer_start(iav_hrtimer_t *hrtimer)
{
	if (hrtimer->enable)
		return hrtimer_prepare_for_next(hrtimer);
	else
		return -1;
}

int iav_hrtimer_wait_next(iav_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		hrtimer->wait_count++;
		if (down_interruptible(&hrtimer->sem_ready) != 0) {
			iav_printk("polling_wait_for_next interrupted \n");
			return -1;
		} else {
			//DRV_PRINT(KERN_DEBUG "polling got\n");
			return 0;
		}
	} else {
		return -1;
	}
}



