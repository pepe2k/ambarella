/*
 * eis_timer.c
 *
 * History:
 *	2012/08/03 - [Jian Tang] created to enhance polling read out mode
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

//#include "utils.h"

#include <amba_common.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>

#include "eis_timer.h"


//make the hr timer to be periodical
static int hrtimer_prepare_for_next(eis_hrtimer_t * hrtimer)
{
	ktime_t ktime = ktime_set(0, US_TO_NS(hrtimer->period_in_us));
	return highres_timer_start(&hrtimer->hr_timer, ktime, HRTIMER_MODE_REL);
}

static enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	eis_hrtimer_t * hrtimer;
	//use container_of trick to get hrtimer pointer
	hrtimer = (eis_hrtimer_t *)container_of(timer, struct eis_hrtimer_s, hr_timer);
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

int eis_hrtimer_init(int us, eis_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		eis_printk("hrtimer already init, error\n");
		return -1;
	}

	if (us <= 0) {
		us = DEFAULT_HRTIMER_PERIOD_US;
	}
	//set hr timer
	memset(hrtimer, 0, sizeof(eis_hrtimer_t));

	sema_init(&hrtimer->sem_ready, 0);
	hrtimer->period_in_us = us;
	hrtimer->wait_count = 0;
	highres_timer_init(&hrtimer->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->hr_timer.function = &hrtimer_callback;
	hrtimer->method = 0;
	hrtimer->enable = 1;

	return 0;
}

int eis_hrtimer_deinit(eis_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		highres_timer_cancel(&hrtimer->hr_timer);
		hrtimer->enable = 0;
	}
	memset(hrtimer, 0, sizeof(eis_hrtimer_t));
	return 0;
}

int eis_hrtimer_start(eis_hrtimer_t *hrtimer)
{
	if (hrtimer->enable)
		return hrtimer_prepare_for_next(hrtimer);
	else
		return -1;
}

int eis_hrtimer_wait_next(eis_hrtimer_t *hrtimer)
{
	if (hrtimer->enable) {
		hrtimer->wait_count++;
		if (down_interruptible(&hrtimer->sem_ready) != 0) {
			eis_printk("polling_wait_for_next interrupted \n");
			return -1;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}



