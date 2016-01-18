/*
 * kernel/private/drivers/ambarella/p_iris/p_iris_impl.c
 *
 * History:
 *	2012/07/2 - [Louis Sun]
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * This file is to implement iris control on selected GPIO based IRIS driver
 * so this implementation depends on different IRIS drive chip design
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
#include <asm/gpio.h>


//this is defined to move more so that no mechanical miss steps
#define MORE_STEPS_TO_MOVE      10

static void config_gpio(int gpio_id)
{
        printk(KERN_DEBUG "config GPIO %d to be SW output \n", gpio_id);
        ambarella_gpio_config(gpio_id, GPIO_FUNC_SW_OUTPUT);
}

static void write_gpio(int gpio_id, int value)
{
	printk(KERN_DEBUG "write GPIO %d, value =%d \n", gpio_id, value);
	ambarella_gpio_set(gpio_id, value);

}

static int read_gpio(int gpio_id)
{
        int value = 0;
        value = ambarella_gpio_get(gpio_id);
        printk(KERN_DEBUG "read GPIO %d, value =%d \n", gpio_id,  value);
        return value;
}


/* move one step,  direction 1:  move forward,  -1:backward   all other values: invalid */
int amba_p_iris_impl_move_one_step(amba_p_iris_controller_t * p_controller, int direction)
{
	//check the lens driver datasheet, to use the Stepper motor 2 input pins to do move on direction.
	int logic_bits;
	int target_bits = 0xFF;

	logic_bits = p_controller->cfg.gpio_val & 0x3;

	if (direction > 0) {

		switch (logic_bits)
		{
			case 0x2: //IN1 = 0 , IN2 = 1
				target_bits = 0x3; //IN1 = 1 , IN2 = 1
				break;
			case 0x3:
				target_bits = 0x1;
				break;
			case 0x1:
				target_bits = 0x0;
				break;
			case 0x0:
				target_bits = 0x2;
				break;
			default:
				printk("wrong bits 0x%x \n", logic_bits);
				break;
		}
	} else {
		switch (logic_bits)
		{
			case 0x2://IN1 = 0 , IN2 = 1
				target_bits = 0x0;//IN1 = 0 , IN2 = 0
				break;
			case 0x0:
				target_bits = 0x1;
				break;
			case 0x1:
				target_bits = 0x3;
				break;
			case 0x3:
				target_bits = 0x2;
				break;
			default:
				printk("wrong bits 0x%x \n", logic_bits);
				break;
		}

	}

	//log for debugging
	printk(KERN_DEBUG "DIR %d, CURRENT 0x%x, TARGET 0x%x\n", direction, logic_bits, target_bits);

	//In stepper motor control, it's not possible for both bits changed

	//update P_IRIS_GPIO_1
	if ((target_bits & 1) != (p_controller->cfg.gpio_val & 1)) {
		write_gpio(p_controller->cfg.gpio_id[0], target_bits & 1);
	}

	//update P_IRIS_GPIO_2
	if ((target_bits & 2) != (p_controller->cfg.gpio_val & 2)) {
		write_gpio(p_controller->cfg.gpio_id[1], (target_bits >> 1) & 1);
	}

	//update gpio value cache
	p_controller->cfg.gpio_val = target_bits;




	return 0;
}




/* low level driver to change iris */
int	amba_p_iris_impl_init(amba_p_iris_controller_t * p_controller)
{

        int i;
        //enable lens control chip

        for (i=0; i< P_IRIS_CONTROLLER_MAX_GPIO_NUM; i++)
     {
         config_gpio(p_controller->cfg.gpio_id[i]);
     }

        write_gpio(p_controller->cfg.gpio_id[2], 1);    //enable
        write_gpio(p_controller->cfg.gpio_id[0], 0);    //set IN1 = Low
        write_gpio(p_controller->cfg.gpio_id[1], 0);    //set IN2 = Low

	return 0;
}


/* reset to default position, which is min_mechanical position */
int amba_p_iris_impl_reset(amba_p_iris_controller_t * p_controller)
{
	int gpio_val1, gpio_val2;

        int max_steps_to_rewind =  (p_controller->pos - p_controller->cfg.min_mechanical) + MORE_STEPS_TO_MOVE;

	//reverse the iris to back position 0
	p_controller->steps_to_move -=  max_steps_to_rewind;

       //wait for some time,
       printk(KERN_DEBUG "P Iris reset, wait %d ms till it finishes \n",  max_steps_to_rewind * p_controller->cfg.timer_period );
       wait_for_completion(&(p_controller->move_compl));

	gpio_val1 = read_gpio(p_controller->cfg.gpio_id[0]);
	gpio_val2 = read_gpio(p_controller->cfg.gpio_id[1]);

	//update init gpio
	p_controller->cfg.gpio_val = gpio_val1 | (gpio_val2 << 1);

	printk(KERN_DEBUG "P-Iris reset done, gpio state 0x%x \n",  p_controller->cfg.gpio_val);


	return 0;
}





