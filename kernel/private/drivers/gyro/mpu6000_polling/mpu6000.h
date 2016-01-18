/*
 * gyro.h
 *
 * History:
 *	2012/10/23 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#ifndef  GYRO_H
#define  GYRO_H


#define MPU6000_CHIP_ADDR			0x00

//atic int mpu6000_spi = -1;


//register addr
#define AUX_VDDIO_REG				1
#define SMPLRT_DIV_REG				25
#define CONFIG_REG					26
#define GYRO_CONFIG_REG			27
#define ACCEL_CONFIG_REG			28
#define FF_THR_REG					29
#define FF_DUR_REG					30
#define MOT_THR_REG				31
#define MOT_DUR_REG				32
#define ZRMOT_THR_REG				33
#define ZRMOT_DUR_REG				34
#define FIFO_EN_REG					35
#define INT_PIN_CFG_REG			55
#define INT_ENABLE_REG				56
#define INT_STATUS_REG				58
#define ACCEL_XOUT_H_REG			59
#define ACCEL_XOUT_L_REG			60
#define ACCEL_YOUT_H_REG			61
#define ACCEL_YOUT_L_REG			62
#define ACCEL_ZOUT_H_REG			63
#define ACCEL_ZOUT_L_REG			64
#define TEMP_OUT_H_REG				65
#define TEMP_OUT_L_REG				66
#define GYRO_XOUT_H_REG			67
#define GYRO_XOUT_L_REG			68
#define GYRO_YOUT_H_REG			69
#define GYRO_YOUT_L_REG			70
#define GYRO_ZOUT_H_REG			71
#define GYRO_ZOUT_L_REG			72

#define MOT_DETECT_STATUS_REG		97
#define SIGNAL_PATH_RESET_REG		104
#define MOT_DETECT_CTRL_REG		105
#define USER_CTRL_REG				106
#define PWR_MGMT_1_REG			107
#define PWR_MGMT_2_REG			108
#define FIFO_COUNTH_REG			114
#define FIFO_COUNTL_REG			115
#define FIFO_R_W_REG				116
#define WHO_AM_I_REG				117

#define M_PI					(3.14159265358979323846f)
#define LSG						(131.0f)
#define RANGE_GYRO				(250.0 * M_PI / 180.0)
#define CONVERT_GYRO			((M_PI / 180.0) / LSG)
#define CONVERT_GYRO_X			CONVERT_GYRO
#define CONVERT_GYRO_Y			CONVERT_GYRO
#define CONVERT_GYRO_Z			CONVERT_GYRO

#define SENSOR_MPU6000		1
#define SENSOR_IDG2000		2

#define TEST_GYRO		1
#define TEST_ACCEL		2
#define TEST_TEMP		3
#define TEST_DUMP		4

#define NO_ARG				0
#define HAS_ARG				1

#define LST						(340.0f)
#define HOMETEMPERATURE			(35.0F)
#define HOMEOFFSET				(-521)
#define RAGNE_TEMP				(85.0f)
#define CONVERT_TEMP			(1 / LST)

#define LSA				(16384.0f)
#define CONVERT_ACCEL	(1 / LSA)

#define SMPLRT_DIV			0
#define WHO_AM_I_ID		0x68

enum {
	IOC_GYRO_READ = 0,
};

#define	GYRO_MAGIC		'q'
#define GYRO_READ_GYRO	 _IOW(GYRO_MAGIC, IOC_GYRO_READ, u16)

/************************************************ general gyro definition ************************************************/
#define NULL_GYRO_SENSOR		0
#define GYRO_INVENSENSE_MPU6000_ID	0x6
#define NULL_IMAGE_STABILIZER           0xFF

/* Sensor interface definition */
enum gyro_interface
{
	GYRO_ANALOG_INTERFACE		=  0,
	GYRO_DIGITAL_INTERFACE_I2C,
	GYRO_DIGITAL_INTERFACE_SPI
};

/* Sensor axis definition */
enum gyro_axis
{
	GYRO_2_AXIS	=  2,
	GYRO_3_AXIS
};

/* Sensor direction definition */
enum gyro_dir
{
	GYRO_X_AXIS	=  0,
	GYRO_Y_AXIS,
	GYRO_Z_AXIS
};

/* Sensor polarity definition */
enum gyro_polarity
{
	GYRO_NEGATIVE_POLARITY	=  0,
	GYRO_POSITIVE_POLARITY
};

/************************************************ general gyro definition ************************************************/

void mpu6000_read(s32 *x, s32 *y, s32 *z);
void mpu6000_get_info(gyro_info_t *gyroinfo);

#endif // TEST_GYRO_H

