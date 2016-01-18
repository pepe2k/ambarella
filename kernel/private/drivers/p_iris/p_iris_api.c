/*
 * kernel/private/drivers/ambarella/p_iris/p_iris_api.c
 *
 * History:
 *	2012/06/29 - [Louis Sun]
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_p_iris.h>
#include "p_iris_priv.h"





static inline void p_iris_range_check(amba_p_iris_controller_t *p_controller)
{
        if (p_controller->pos > p_controller->cfg.max_mechanical)
                p_controller->pos = p_controller->cfg.max_mechanical;
        else if (p_controller->pos <   p_controller->cfg.min_mechanical)
                p_controller->pos = p_controller->cfg.min_mechanical;
}



/* callback runned by the high resolution timer */
static int p_iris_timer_callback(void * p_iris_controller)
{
        amba_p_iris_controller_t *p_controller = (amba_p_iris_controller_t*) p_iris_controller;
	if (!p_controller) {
		printk(KERN_DEBUG "Error: null arg in p_iris_timer_callback!\n");
		return -1;
	}
	//lock mutex and then move, avoid the case of being interrupted by another user IOCTL
	if( p_controller->steps_to_move > 0) {
            p_controller->state = P_IRIS_WORK_STATE_RUNNING;
            printk(KERN_DEBUG "move 1\n");
            amba_p_iris_impl_move_one_step(p_controller, 1);
            p_controller->steps_to_move--;
            p_controller->pos ++;
            //position range check.
            p_iris_range_check(p_controller);
 	} else if (p_controller->steps_to_move < 0) {
            p_controller->state = P_IRIS_WORK_STATE_RUNNING;
            printk(KERN_DEBUG "move -1\n");
            amba_p_iris_impl_move_one_step(p_controller, -1);
            p_controller->steps_to_move++;
            p_controller->pos --;
             //position range check.
            p_iris_range_check(p_controller);
  	} else {
            if (p_controller->state == P_IRIS_WORK_STATE_RUNNING) {
                p_controller->state = P_IRIS_WORK_STATE_READY;
                complete(&(p_controller->move_compl));
            }

	}
	return 0;
}


static int p_iris_reset(amba_p_iris_controller_t * p_controller)
{


       amba_p_iris_impl_reset(p_controller);
       printk(KERN_DEBUG "P-Iris reset done,  reset pos = 0 \n");
       p_controller->pos = P_IRIS_DEFAULT_POS;

       return 0;
}


/* the first function to init P-iris and set default confg and state to make it ready to run */
int amba_p_iris_init_config(amba_p_iris_context_t * context, amba_p_iris_cfg_t __user * arg)
{
	amba_p_iris_cfg_t cfg;
	amba_p_iris_controller_t * p_controller = context->controller;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	if (p_controller->state != P_IRIS_WORK_STATE_NOT_INIT) {
		printk(KERN_DEBUG "P-iris state = %d, cannot init again \n", p_controller->state);
		return -EINVAL;
	}

	//validate the config
	if (arg->timer_period < P_IRIS_CONTROL_DEFAULT_PERIOD) {
            arg->timer_period = P_IRIS_CONTROL_DEFAULT_PERIOD;
            printk(KERN_DEBUG "piris config use too small timer_period %d, set to default 10ms\n", arg->timer_period);
	}

	if ((arg->gpio_id [0] <= 0) || (arg->gpio_id [1] <= 0)) {
		printk(KERN_DEBUG "iris gpio not setup \n");
		return -EINVAL;
	}

        //init completion
        init_completion(&(p_controller->move_compl));

	//if OK, then run
	memcpy(&(p_controller->cfg), &cfg, sizeof(cfg));

	//iris device init
	amba_p_iris_impl_init(p_controller);

	//iris timer init
	if (amba_p_iris_init_timer(p_controller, p_iris_timer_callback) < 0) {
		printk(KERN_DEBUG "init p_iris timer failed\n");
		return -1;
	}

	if (amba_p_iris_start_timer(p_controller) < 0) {
		printk(KERN_DEBUG "start p_iris timer failed\n");
		return -1;
	}

	p_iris_reset(p_controller);
	return 0;
}

/* read out the iris config */
int amba_p_iris_get_config(amba_p_iris_context_t * context, amba_p_iris_cfg_t __user * arg)
{
	amba_p_iris_controller_t * p_controller = context->controller;
	//check state
	if (p_controller->state != P_IRIS_WORK_STATE_READY)
		return -EAGAIN;

	return copy_to_user(arg, &p_controller->cfg, sizeof(*arg)) ? -EFAULT : 0;
}


/* reset iris to default position and state, likes the 'init again', however, no need arg */
int amba_p_iris_reset(amba_p_iris_context_t * context, int arg)
{
	//check state
	amba_p_iris_controller_t * p_controller = context->controller;
	if (p_controller->state == P_IRIS_WORK_STATE_NOT_INIT) {
            printk(KERN_DEBUG "cannot reset non initialized p-iris \n");
            return -EINVAL;
	} else  if (p_controller->state != P_IRIS_WORK_STATE_READY) {
            return -EAGAIN;
	}else   {
		return p_iris_reset(p_controller);
	}
}

/* make moves */
int amba_p_iris_move_steps(amba_p_iris_context_t * context, int steps)
{
	amba_p_iris_controller_t * p_controller = context->controller;

	//check state
	if (p_controller->state != P_IRIS_WORK_STATE_READY)
		return -EAGAIN;
	//valid steps, if it has no move, then don't move
        if (steps == 0)
            return 0;
	//if OK, then move
	p_controller->steps_to_move += steps;
       wait_for_completion(&(p_controller->move_compl));

       return 0;
}


/* report position */
int amba_p_iris_get_position(amba_p_iris_context_t * context, int __user * pos)
{
	amba_p_iris_controller_t * p_controller = context->controller;
	if (p_controller->state != P_IRIS_WORK_STATE_READY)
		return -EAGAIN;
	return copy_to_user(pos, &p_controller->pos, sizeof(*pos)) ? -EFAULT : 0;
}

int amba_p_iris_deinit(amba_p_iris_controller_t * p_controller)
{
	/* should call deinit, here,
	  auto called when module removed
	 */
	if (p_controller->state != P_IRIS_WORK_STATE_NOT_INIT) {
		return amba_p_iris_deinit_timer(p_controller);
	} else {
		return 0;
	}
}


