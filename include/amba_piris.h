
/*
 * amba_piris.h
 *
 * History:
 *	2013/07/25 - [Louis Sun] created file
 *	2014/06/11 - [Peter Jiao] Mod
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_PIRIS_H__
#define  __AMBA_PIRIS_H__


enum P_IRIS_WORK_STATE {
    P_IRIS_WORK_STATE_NOT_INIT = 0,	//uninitialized
    P_IRIS_WORK_STATE_PREPARING =  1, 	//doing init
    P_IRIS_WORK_STATE_READY	    = 2,	//init done , ready to adjust p-iris (normal working), or run stop (back to ready)
    P_IRIS_WORK_STATE_RUNNING     = 3,    //running.
};


#define P_IRIS_IOC_MAGIC			'p'


enum P_IRIS_IOC_ENUM {
        IOC_P_IRIS_SET_POS    =   0,
        IOC_P_IRIS_RESET   = 1,
        IOC_P_IRIS_MOVE_STEPS  = 2,
        IOC_P_IRIS_GET_POS = 3,
        IOC_P_IRIS_GET_STATE = 4,
};


#define AMBA_IOC_P_IRIS_SET_POS	 	_IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_SET_POS, int)
#define AMBA_IOC_P_IRIS_RESET		        _IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_RESET, int)
#define AMBA_IOC_P_IRIS_MOVE_STEPS	 _IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_MOVE_STEPS, int)
#define AMBA_IOC_P_IRIS_GET_POS		 _IOR(P_IRIS_IOC_MAGIC, IOC_P_IRIS_GET_POS, int *)
#define AMBA_IOC_P_IRIS_GET_STATE	 _IOR(P_IRIS_IOC_MAGIC, IOC_P_IRIS_GET_STATE, int *)


#define P_IRIS_CONTROLLER_MAX_GPIO_NUM  3
typedef struct amba_p_iris_cfg_s{
    int gpio_id[P_IRIS_CONTROLLER_MAX_GPIO_NUM];        //gpio_id[0]= IN1, gpio_id[1]=IN2, gpio_id[2]=EN
    int gpio_val;                   //bit0 is IN1, bit1 is IN2
    int timer_period;           //in ms
    int max_mechanical;
    int min_mechanical ;
    int max_optical;
    int min_optical;
}amba_p_iris_cfg_t;



#endif // __AMBA_PIRIS_H__


