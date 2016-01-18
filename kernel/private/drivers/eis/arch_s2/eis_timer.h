/*
 * eis_timer.h
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

#ifndef __EIS_TIMER_H__
#define __EIS_TIMER_H__

#define eis_printk(S...)		printk(KERN_DEBUG "eis: "S)
#define eis_error(S...)			printk("eis error: "S)

#define MS_TO_NS(x)		((u32)(((u32)(x)) * (u32)1000000L))
#define US_TO_NS(x)		((u32)(((u32)(x)) * (u32)1000L))


typedef enum {
	DEFAULT_HRTIMER_PERIOD_MS = 1,
	DEFAULT_HRTIMER_PERIOD_US = 990,
} HRTIMER_PERIOD_IN_MS;


typedef struct eis_hrtimer_s {
	struct hrtimer	hr_timer;
	struct semaphore	sem_ready;
	int	enable;
	int	period_in_us;	//polling period in us UNIT
	int	method;		//not used now
	u32	wait_count;
} eis_hrtimer_t;

int eis_hrtimer_init(int us, eis_hrtimer_t *hrtimer);
int eis_hrtimer_deinit(eis_hrtimer_t *hrtimer);
int eis_hrtimer_start(eis_hrtimer_t *hrtimer);
int eis_hrtimer_wait_next(eis_hrtimer_t *hrtimer);


#endif	// __EIS_TIMER_H__

