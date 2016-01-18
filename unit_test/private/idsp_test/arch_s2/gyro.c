/********************************************************************
 * gyro.c
 *
 * History:
 *	2012/10/22 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <basetypes.h>
#include <sys/time.h>

#include "gyro.h"

/************************** should be moved to bsp **************************/
#define GYRO_XOUT_CHANNEL		GYRO_Y_AXIS
#define GYRO_YOUT_CHANNEL		GYRO_Z_AXIS
#define GYRO_XOUT_POLARITY		GYRO_POSITIVE_POLARITY
#define GYRO_YOUT_POLARITY		GYRO_POSITIVE_POLARITY
#define GYRO_SPI_INST			0
#define GYRO_SPI_ID				1
/************************** should be moved to bsp **************************/

#define GYRO_SPI_WR_SCLK  		1000000
#define GYRO_SPI_RD_SCLK  		10000000
#define GYRO_MAX_WRITE_BYTES		(2)
#define GYRO_MAX_READ_BYTES		(16)

static u8 gyro_tx[GYRO_MAX_WRITE_BYTES];
static u8 gyro_rx[GYRO_MAX_READ_BYTES];

static gyro_info_t gyro_info_main;
static s32 level_shift = 0;
static s32 open_success = 0;
static s32 spi_handle = -1;

void mpu6000_get_info(gyro_info_t *gyroinfo)
{
	static u8 flg_once = 0;

	if (gyroinfo == NULL) {
		printf("NULL pointer for gyro get info!!\n");
		return ;
	}

	if (!flg_once) {
		flg_once = 1;
		memset(&gyro_info_main, 0, sizeof(gyro_info_t));

#ifdef BUILD_AMBARELLA_EIS
		gyro_info_main.gyro_id = GYRO_INVENSENSE_MPU6000_ID;
#else
		gyro_info_main.gyro_id = NULL_IMAGE_STABILIZER;
#endif

#ifdef GYRO_PWR_GPIO
		gyro_info_main.gyro_pwr_gpio = GYRO_PWR_GPIO;
#endif

#ifdef GYRO_INT_GPIO
		gyro_info_main.gyro_int_gpio = 0;//GYRO_INT_GPIO;
#endif

#ifdef GYRO_XOUT_POLARITY
		gyro_info_main.gyro_x_polar = GYRO_XOUT_POLARITY;
		if (!gyro_info_main.gyro_x_polar) {
			gyro_info_main.gyro_x_polar = -1;
		}
#else
		gyro_info_main.gyro_x_polar = 1;
#endif

#ifdef GYRO_YOUT_POLARITY
		gyro_info_main.gyro_y_polar = GYRO_YOUT_POLARITY;
		if (!gyro_info_main.gyro_y_polar) {
			gyro_info_main.gyro_y_polar = -1;
	        }
#else
		gyro_info_main.gyro_y_polar = 1;
#endif

#ifdef GYRO_XOUT_CHANNEL
		gyro_info_main.gyro_x_chan = GYRO_XOUT_CHANNEL;
#else
		gyro_info_main.gyro_x_chan = 0;
#endif

#ifdef GYRO_YOUT_CHANNEL
		gyro_info_main.gyro_y_chan = GYRO_YOUT_CHANNEL;
#else
		gyro_info_main.gyro_y_chan = 1;
#endif

#ifndef GYRO_VOL_DIVIDER_NUM
		gyro_info_main.vol_div_num = 1;
#else
		gyro_info_main.vol_div_num = GYRO_VOL_DIVIDER_NUM;
#endif

#ifndef GYRO_VOL_DIVIDER_DEN
		gyro_info_main.vol_div_den = 1;
#else
		gyro_info_main.vol_div_den = GYRO_VOL_DIVIDER_DEN;
#endif
		gyro_info_main.sensor_interface = GYRO_DIGITAL_INTERFACE_SPI;

		gyro_info_main.sensor_axis = GYRO_2_AXIS;
		gyro_info_main.max_sense = 135;				// LSB/(deg/sec)
		gyro_info_main.min_sense = 127;				// LSB/(deg/sec)
		gyro_info_main.max_bias = 35388;//2620;			// LSB
		gyro_info_main.min_bias = 30148;//-2620;		// LSB
		gyro_info_main.max_rms_noise = 7;				// LSB
		gyro_info_main.start_up_time = 30;				// ms
		gyro_info_main.full_scale_range = 250;				// deg/sec
		gyro_info_main.max_sampling_rate = 8000;				// sample/sec
		gyro_info_main.adc_resolution = 16;                           // bits
		gyro_info_main.phs_dly_in_ms = 4;				// ms

		gyro_info_main.sampling_rate = gyro_info_main.max_sampling_rate / (SMPLRT_DIV + 1);
		level_shift = 1 << (gyro_info_main.adc_resolution - 1);
	}

	memcpy(gyroinfo, &gyro_info_main, sizeof(gyro_info_t));
} /* void get_gyro_info(gyro_info_t *gyroinfo) */

static int mpu6000_write_reg(int fd, u8 addr, u8 data)
{
	u32 speed = GYRO_SPI_WR_SCLK;
	u32 speed2 = GYRO_SPI_RD_SCLK;

	gyro_tx[0] = addr & 0x7F;
	gyro_tx[1] = data;

	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	write(fd, gyro_tx, sizeof(gyro_tx));
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed2);

	return 0;
}

static int mpu6000_burst_read_reg(int fd, u8 addr, u8 bytes)
{
//#define TEST_GYRO_SPI
	static u8 init_flg = 0;
	static struct spi_ioc_transfer tr;
	s32 rval;

#ifdef TEST_GYRO_SPI
	struct timeval  time1, time2;
	gettimeofday(&time1, NULL);
#endif

	if (bytes >= GYRO_MAX_READ_BYTES) {
		printf("ERROR: gyro read bytes larger than %d\n",
			(GYRO_MAX_READ_BYTES - 1));
		return -1;
	}

	gyro_tx[0] = addr | 0x80;
	if (!init_flg) {
		tr.tx_buf = (unsigned long)gyro_tx;
		tr.rx_buf = (unsigned long)gyro_rx;
		init_flg = 1;
	}
	tr.len = bytes + 1;
	rval = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

#ifdef TEST_GYRO_SPI
	{
		long long timediff;

		gettimeofday(&time2, NULL);
		timediff = (time2.tv_sec - time1.tv_sec)* 1000000LL +
			(time2.tv_usec - time1.tv_usec);
		printf(">>>SPI latency=%lld micro seconds\n", timediff);
	}
#endif

	return rval;
}

int mpu6000_read(s32 *x, s32 *y, s32 *z)
{
	u8 *rx = &gyro_rx[1];
	s16 yy, zz;
	s32 rval;

	rval = mpu6000_burst_read_reg(spi_handle, GYRO_YOUT_H_REG, 4);
	yy = (s16)((*(rx) << 8) | *(rx + 1));
	zz = (s16)((*(rx + 2) << 8) | *(rx + 3));

	*x = 0;
	*y = yy + level_shift;
	*z = zz + level_shift;

//	printf("%s: x : %5d, y : %5d,  z : %5d \r", __func__, *x, *y, *z);

	return rval;
}

static int mpu6000_check_spi(void)
{
	char buffer[32];
	u8 mode	= SPI_MODE_3;
	u8 bits = 8;
	u32 speed = GYRO_SPI_WR_SCLK;

	sprintf (buffer, "/dev/spidev%d.%d", GYRO_SPI_INST, GYRO_SPI_ID);
	spi_handle = open(buffer, O_RDWR, 0);
	if (spi_handle < 0)
		return -1;
	ioctl(spi_handle, SPI_IOC_WR_MODE, &mode);
	ioctl(spi_handle, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(spi_handle, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

	return 0;
}

int mpu6000_init(void)
{
	u8 *rx = &gyro_rx[1];
	u32 speed = GYRO_SPI_RD_SCLK;

	if (open_success == -1) {
		printf("ERROR: Gyro open fail!!!\n");
		return (-1);
	}

	if (mpu6000_check_spi() < 0) {
		open_success = -1;
		printf("ERROR: SPI open fail!!!\n");
		return -1;
	}

	mpu6000_write_reg(spi_handle, USER_CTRL_REG, 0x10);
	mpu6000_burst_read_reg(spi_handle, WHO_AM_I_REG, 1);
	if (*rx != WHO_AM_I_ID) {
		printf("ERROR: gyro_id=0x%x is not right!\n", *rx);
		open_success = -1;
		return -1;
	}
	mpu6000_write_reg(spi_handle, PWR_MGMT_1_REG, 0x9);

	//set sample rate SMPLRT_DIV_REG
	mpu6000_write_reg(spi_handle, SMPLRT_DIV_REG, SMPLRT_DIV);

	//set gyroscope output rate
	mpu6000_write_reg(spi_handle, CONFIG_REG, 0x0);

	if (ioctl(spi_handle, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
		printf("%s: Can't change SPI speed to [%d]!", __func__, speed);
	}

	return 0;
}



