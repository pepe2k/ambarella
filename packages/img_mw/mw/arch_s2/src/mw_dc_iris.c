/**********************************************************************
 *
 * mw_dc_iris.c
 *
 * History:
 * 2010/02/28 - [Jian Tang] Created this file
 * 2012/10/29 - [Bin  Wang] Modified this file
 * Copyright (C) 2007 - 2012, Ambarella, Inc.
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

#include "mw_struct.h"
#include "mw_api.h"
#include "mw_defines.h"
#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"

//#define USE_FUZZY_PID_ALGO
#define PWM_CHANNEL_FOR_DC_IRIS 0
#define IGNORE_FULL_OPEN_CHECK -10
#define FULL_OPEN_CHECK 20
#define OPEN_DUTY 200
#define MAX_DUTY 900
#define MIN_DUTY 100
#define LUMA_DIFF_RANGE 1
#define Kp0 1500
#define Ki0 1
#define Kd0 1000
#define Kac 1000 //K accuracy class
#define Kp0_hdr 12000
#define Ki0_hdr 8
#define Kd0_hdr 0
#define DATA0 0
#define DATA0_hdr 500

typedef enum {
	MW_IRIS_IDLE = 0,
	MW_IRIS_FLUCTUATE,
	MW_IRIS_STATE_NUM,
} MW_IRIS_STATE;

typedef enum {
	NB,
	NS,
	Z0,
	PS,
	PB,
} Param_Domain;

typedef struct {
	int enable;
	int p_data;
	int i_data;
	int d_data;
} PID_Data;

typedef struct {
	int NB_PID;
	int NS_PID;
	int Z0_PID;
	int PS_PID;
	int PB_PID;
} Param_Domain_PID;

typedef struct {
	int T1;
	int T2;
	int T3;
	int T4;
} Param_Domain_Threshold;

PID_Data G_mw_pid_data = {
	.enable = 0,
	.p_data = 0,
	.i_data = 0,
	.d_data = 0,
};

mw_dc_iris_pid_coef G_mw_dc_iris_pid_coef = {
	.p_coef = Kp0,
	.i_coef = Ki0,
	.d_coef = Kd0,
}; //PID coefficients, you can tune it if needed.

Param_Domain_PID Kp_Domain = {
	.NB_PID = -2,
	.NS_PID = -1,
	.Z0_PID = 0,
	.PS_PID = 1,
	.PB_PID = 2,
};

Param_Domain_PID Ki_Domain = {
	.NB_PID = -10,
	.NS_PID = -5,
	.Z0_PID = 0,
	.PS_PID = 5,
	.PB_PID = 10,
};

Param_Domain_PID Kd_Domain = {
	.NB_PID = -2,
	.NS_PID = -1,
	.Z0_PID = 0,
	.PS_PID = 1,
	.PB_PID = 2,
};

Param_Domain_Threshold measure_Domain = {
	.T1 = -100,
	.T2 = -10,
	.T3 = 10,
	.T4 = 800,
};

Param_Domain_Threshold d_measure_Domain = {
	.T1 = -50,
	.T2 = -5,
	.T3 = 50,
	.T4 = 200,
};

static int pwm_proc = -1;
int duty = 200;
int wait_time = 0;
int luma_data[2] = {0, 0};
mw_sensor_param G_hdr_param = {0};
/*************************************************
 *
 *		Static Functions, for file internal used
 *
 *************************************************/
#ifdef USE_FUZZY_PID_ALGO
//function choose_domain and Update_K can be applied to fuzzy PID control algo.
static Param_Domain choose_domain(int data, Param_Domain_Threshold data_Domain)
{
	if (data > data_Domain.T4) {
		return PB;
	} else if (data > data_Domain.T3) {
		return PS;
	} else if (data > data_Domain.T2) {
		return Z0;
	} else if (data > data_Domain.T1) {
		return NS;
	} else {
		return NB;
	}
}

static void Update_K(int data, int d_data)
{
	Param_Domain measure = choose_domain(data, measure_Domain);
	Param_Domain d_measure = choose_domain(d_data, d_measure_Domain);
	switch(measure) {
	case NB:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case NS:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PS_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case Z0:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case PS:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case PB:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	}
}
#endif
static int my_itoa(int num,  char *str,  int base)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	u32	unum;
	int i = 0, j, k;

	if (base == 10 && num < 0) {
		unum = (u32)-num;
		str[i++] = '-';
	} else {
		unum = (u32)num;
	}

	do {
		str[i++] = index[unum % (u32)base];
		unum /= base;
	} while (unum);
	str[i] = '\0';

	if (str[0] == '-') {
		k = 1;
	} else {
		k = 0;
	}
	for (j = k; j <= (i - 1) / 2 + k; ++j) {
		num = str[j];
		str[j] = str[i-j-1+k];
		str[i-j-1+k] = num;
	}

	return 0;
}

static int open_pwm_proc(int channel)
{
	char pwm_device[64];

	if (pwm_proc < 0) {
		sprintf(pwm_device, "/sys/class/backlight/pwm-backlight.%d/brightness",
			channel);
		if ((pwm_proc = open(pwm_device, O_RDWR | O_TRUNC)) < 0) {
			printf("open %s failed!\n",  pwm_device);
			return -1;
		}
	}

	return 0;
}

static int close_pwm_proc(void)
{
	if (pwm_proc >= 0) {
		close(pwm_proc);
		pwm_proc = -1;
	}
	return 0;
}

static int pwm_update(int channel, int duty_cycle)
{
	u32 n = 0;
	char value[16];

	my_itoa(duty_cycle, value, 10);
	//MW_DEBUG("pwm duty : %s\n", value);
	n = write(pwm_proc, value, strlen(value));
	if (n != strlen(value)) {
		printf("pwm duty [%s] set error [%d].\n", value, n);
		return -1;
	}

	return 0;
}

static int iris_duty_adjust(int luma_diff)
{
	luma_data[1] = luma_data[0];
	luma_data[0] = luma_diff;
	//data_p
	G_mw_pid_data.p_data = luma_data[0];
	//data_i
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		if (luma_diff < 0) {  //rectify luma_diff to increase its linearity
			if(luma_data[0] > -10) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 5;
			} else if(luma_data[0] > -20) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 6;
			} else if(luma_data[0] > -40) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 7;
			} else if(luma_data[0] > -60) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 8;
			} else if(luma_data[0] > -80) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 10;
			} else if(luma_data[0] > -100) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 12;
			} else {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 16;
			}
		} else {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 4;
		}
	} else {  //in HDR mode luma_diff range is -25~23.
		if (luma_diff > 0) {
			if (luma_data[0] > 20) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 5;
			} else if (luma_data[0] > 15){
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 4;
			} else if (luma_data[0] > 10) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 3;
			} else if (luma_data[0] > 5) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 2;
			} else {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 1;
			}
		} else {
			if (luma_data[0] < -20) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 5;
			} else if (luma_data[0] < -15){
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 4;
			} else if (luma_data[0] < -10) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 3;
			} else if (luma_data[0] < -5) {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 2;
			} else {
				G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 1;
			}
		}
	}

	//data_d
	G_mw_pid_data.d_data = luma_data[0] - luma_data[1];
	#ifdef USE_FUZZY_PID_ALGO
	Update_K(luma_diff, G_mw_pid_data.d_data);
	//update K when using fuzzy PID control algorithm.
	duty = (Kp0 + G_mw_dc_iris_pid_coef.p_coef) * G_mw_pid_data.p_data / Kac +
			(Ki0 + G_mw_dc_iris_pid_coef.i_coef) * G_mw_pid_data.i_data / Kac +
			(Kd0 + G_mw_dc_iris_pid_coef.d_coef) * G_mw_pid_data.d_data / Kac;
	//fuzzy PID control
	#else
	if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		duty = DATA0 +
			G_mw_dc_iris_pid_coef.p_coef * G_mw_pid_data.p_data / Kac +
			G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac +
			G_mw_dc_iris_pid_coef.d_coef * G_mw_pid_data.d_data / Kac;
	} else {
		duty = DATA0_hdr +
			G_mw_dc_iris_pid_coef.p_coef * G_mw_pid_data.p_data / Kac +
			G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac +
			G_mw_dc_iris_pid_coef.d_coef * G_mw_pid_data.d_data / Kac;
	}

	#endif
	if (duty > MAX_DUTY) {
		duty = MAX_DUTY;
	} else if (duty < MIN_DUTY) {
		duty = MIN_DUTY;
	}
	pwm_update(PWM_CHANNEL_FOR_DC_IRIS, duty);
	return 0;
}

static int iris_check_full_open(int luma_diff)
{
	int diff_value = 0;
	static int last_low_luma_diff = -1;
	static int count = 0;
	if (duty < OPEN_DUTY) {
		if ( luma_diff >= IGNORE_FULL_OPEN_CHECK) {
			goto check_full_open_exit;
		}

		while (diff_value * (diff_value + 1) < ABS(luma_diff))
			diff_value ++;

		if (ABS(luma_diff - last_low_luma_diff) < diff_value) {
			if (count ++ > FULL_OPEN_CHECK) {
				count = 0;
				G_mw_pid_data.p_data = 0;
				G_mw_pid_data.i_data = 0;
				G_mw_pid_data.d_data = 0;
				return 1;
			}
			return 0;
		}
	}
check_full_open_exit:
	count = 0;
	last_low_luma_diff = luma_diff;
	return 0;
}

static void iris_check_no_action(int luma_diff)
{
	if (luma_diff > 0 && duty == MAX_DUTY &&
			G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac >=
			MAX_DUTY) {
		G_mw_pid_data.p_data = 0;
		G_mw_pid_data.i_data = 0;
		G_mw_pid_data.d_data = 0;
		MW_ERROR("No action from DC-Iris, "
				"Please check the connection of DC-Iris!\n");
	}
}

static int luma_control(int run,  int luma_diff,  int *state)
{
	static int iris_state = MW_IRIS_IDLE;
	if (G_mw_pid_data.enable == 0) {
		pwm_update(PWM_CHANNEL_FOR_DC_IRIS, MIN_DUTY);
		iris_state = MW_IRIS_IDLE;
		goto luma_control_exit;
	}

	if (!run) {
		pwm_update(PWM_CHANNEL_FOR_DC_IRIS, MIN_DUTY);
		iris_state = MW_IRIS_IDLE;
		goto luma_control_exit;
	}
	switch (iris_state) {
	case MW_IRIS_IDLE:
			pwm_update(PWM_CHANNEL_FOR_DC_IRIS, MAX_DUTY);
			wait_time = -1;
			iris_state = MW_IRIS_FLUCTUATE;
		break;

	case MW_IRIS_FLUCTUATE:
		if (iris_check_full_open(luma_diff)) {
			iris_state = MW_IRIS_IDLE;
		} else {
			iris_duty_adjust(luma_diff);
			iris_state = MW_IRIS_FLUCTUATE;
		}
		iris_check_no_action(luma_diff);
		break;

	default:
		assert(0);
		break;
	}

	luma_control_exit:
	*state = (iris_state != MW_IRIS_IDLE);
	MW_DEBUG("[%d] luma_diff %d, duty %d"
			 " Kp %d, Ki %d, Kd %d,"
			 "data_p %d, data_i %d, data_d %d,"
			 "state %d, \n\n",
			 run, luma_diff, duty,
			 G_mw_dc_iris_pid_coef.p_coef, G_mw_dc_iris_pid_coef.i_coef,
			 G_mw_dc_iris_pid_coef.d_coef,
			 G_mw_dc_iris_pid_coef.p_coef * G_mw_pid_data.p_data / Kac,
			 G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac,
			 G_mw_dc_iris_pid_coef.d_coef * G_mw_pid_data.d_data / Kac,
			 *state);
	//MW_DEBUG("[%d] luma_diff %d, duty %d\n", run, luma_diff, duty);
	return 0;
}

int dc_iris_get_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	pPid_coef->p_coef = G_mw_dc_iris_pid_coef.p_coef;
	pPid_coef->i_coef = G_mw_dc_iris_pid_coef.i_coef;
	pPid_coef->d_coef = G_mw_dc_iris_pid_coef.d_coef;
	return 0;
}

int dc_iris_set_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	if (G_mw_pid_data.enable == 1) {
		MW_ERROR("Cannot set PID coefficients when DC-Iris is running!\n");
		return -1;
	}
	G_mw_dc_iris_pid_coef.p_coef = pPid_coef->p_coef;
	G_mw_dc_iris_pid_coef.i_coef = pPid_coef->i_coef;
	G_mw_dc_iris_pid_coef.d_coef = pPid_coef->d_coef;
	return 0;
}

int enable_dc_iris(u8 enable)
{
	dc_iris_cntl iris_cntl = NULL;

	if (mw_get_sensor_param_for_3A(&G_hdr_param) < 0) {
		printf("mw_get_sensor_hdr_expo error\n");
		return -1;
	}
	if (enable) {
		if (open_pwm_proc(PWM_CHANNEL_FOR_DC_IRIS) < 0) {
			perror("open_pwm_proc error\n");
			return -1;
		}
		iris_cntl = luma_control;
		if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
			G_mw_dc_iris_pid_coef.p_coef = Kp0;
			G_mw_dc_iris_pid_coef.i_coef = Ki0;
			G_mw_dc_iris_pid_coef.d_coef = Kd0;
			img_register_iris_cntl(iris_cntl);
		} else {
			G_mw_dc_iris_pid_coef.p_coef = Kp0_hdr;
			G_mw_dc_iris_pid_coef.i_coef = Ki0_hdr;
			G_mw_dc_iris_pid_coef.d_coef = Kd0_hdr;
			img_hdr_register_iris_cntl(iris_cntl, 0);
		}
		MW_DEBUG("Function luma_control is registered into AAA algo.\n");
	} else if (pwm_proc >= 0) {
		G_mw_pid_data.enable = 0;
		iris_cntl = NULL;
		if (G_hdr_param.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
			img_register_iris_cntl(iris_cntl);
		} else {
			img_hdr_register_iris_cntl(iris_cntl, 0);
		}
		pwm_update(PWM_CHANNEL_FOR_DC_IRIS, 1);
		if (close_pwm_proc() < 0) {
			perror("close pwm_proc error\n");
			return -1;
		}
		MW_DEBUG("Function luma_control is unregistered into AAA algo.\n");
	}
	G_mw_pid_data.enable = enable;
	return 0;
}
