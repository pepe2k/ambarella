/*
 * kernel/private/drivers/ambarella/gyro/mpu6000_polling/mpu6000.c
 *
 * History:
 *    2012/08/23 - [Bingliang Hu] Create
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
#include <linux/bitrev.h>
#include "mpu6000.h"

/************************************************ should be moved to bsp ************************************************/
#define GYRO_XOUT_CHANNEL		GYRO_Y_AXIS
#define GYRO_YOUT_CHANNEL		GYRO_Z_AXIS
#define GYRO_XOUT_POLARITY		GYRO_POSITIVE_POLARITY
#define GYRO_YOUT_POLARITY		GYRO_POSITIVE_POLARITY
#define GYRO_SPI_INST			0
#define GYRO_SPI_ID			1
/************************************************ should be moved to bsp ************************************************/

#define GYRO_SPI_WR_SCLK  		1000000
#define GYRO_SPI_RD_SCLK  		10000000
#define GYRO_MAX_READ_BYTES		16

static u8 gyro_tx[2];
static u8 gyro_rx[GYRO_MAX_READ_BYTES];

static gyro_info_t gyro_info_main;
static s32 level_shift = 0, open_success = 0;
void mpu6000_get_info(gyro_info_t *gyroinfo)
{
	static u8 flg_once = 0;

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

#ifndef GYRO_XOUT_POLARITY
	        gyro_info_main.gyro_x_polar = 1;
#else
	        gyro_info_main.gyro_x_polar = GYRO_XOUT_POLARITY;
	        if (!gyro_info_main.gyro_x_polar)
	                gyro_info_main.gyro_x_polar = -1;
#endif

#ifndef GYRO_YOUT_POLARITY
	        gyro_info_main.gyro_y_polar = 1;
#else
	        gyro_info_main.gyro_y_polar = GYRO_YOUT_POLARITY;
	        if (!gyro_info_main.gyro_y_polar)
	                gyro_info_main.gyro_y_polar = -1;
#endif

#ifndef GYRO_XOUT_CHANNEL
		gyro_info_main.gyro_x_chan = 0;
#else
	        gyro_info_main.gyro_x_chan = GYRO_XOUT_CHANNEL;
#endif

#ifndef GYRO_YOUT_CHANNEL
		gyro_info_main.gyro_y_chan = 1;
#else
  	        gyro_info_main.gyro_y_chan = GYRO_YOUT_CHANNEL;
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

        	gyro_info_main.sensor_axis		= GYRO_2_AXIS;
        	gyro_info_main.max_sense 		= 135;				// LSB/(deg/sec)
        	gyro_info_main.min_sense 		= 127;				// LSB/(deg/sec)
        	gyro_info_main.max_bias 		= 35388;//2620;			// LSB
        	gyro_info_main.min_bias 		= 30148;//-2620;		// LSB
        	gyro_info_main.max_rms_noise		= 7;				// LSB
        	gyro_info_main.start_up_time		= 30;				// ms
        	gyro_info_main.full_scale_range 	= 250;				// deg/sec
        	gyro_info_main.max_sampling_rate	= 8000;				// sample/sec
        	gyro_info_main.adc_resolution           = 16;                           // bits

		gyro_info_main.sampling_rate = gyro_info_main.max_sampling_rate / (SMPLRT_DIV + 1);
		level_shift = 1 << (gyro_info_main.adc_resolution - 1);
	}
	memcpy(gyroinfo, &gyro_info_main, sizeof(gyro_info_t));
} /* void get_gyro_info(gyro_info_t *gyroinfo) */
EXPORT_SYMBOL(mpu6000_get_info);

static int mpu6000_write_reg(u16 subaddr, u16 data)
{
	s32 errCode = 0;
	static u8 init_flg = 0;
	static amba_spi_cfg_t config;
	static amba_spi_write_t write;

	gyro_tx[0] = subaddr & 0x7F;
	gyro_tx[1] = data;

	if (!init_flg) {
		config.cfs_dfs = 8;//bits
		config.baud_rate = GYRO_SPI_WR_SCLK;
		config.cs_change = 1;
		config.spi_mode = SPI_MODE_3;

		write.buffer = gyro_tx;
		write.n_size = 2;
		write.cs_id = GYRO_SPI_ID;
		write.bus_id = GYRO_SPI_INST;

		init_flg = 1;
	}

	errCode = ambarella_spi_write(&config, &write);
	return errCode;
}

static int mpu6000_burst_read_reg(u16 subaddr, u8 bytes)
{
//#define TEST_GYRO_SPI
	s32 errCode = 0;
	static u8 init_flg = 0;
	static amba_spi_cfg_t config;
	static amba_spi_write_then_read_t write;
#ifdef TEST_GYRO_SPI
	struct timeval  time1, time2;

	do_gettimeofday(&time1);
#endif
	if (bytes > GYRO_MAX_READ_BYTES)
		printk("ERROR: gyro read bytes larger than %d\n", GYRO_MAX_READ_BYTES);

	gyro_tx[0] = subaddr | 0x80;
	if (!init_flg) {
		config.cfs_dfs = 8;//bits
		config.baud_rate = GYRO_SPI_RD_SCLK;
		config.cs_change = 1;
		config.spi_mode = SPI_MODE_3;

		write.w_buffer = gyro_tx;
		write.w_size = 1;
		write.r_buffer = gyro_rx;
		write.cs_id = GYRO_SPI_ID;
		write.bus_id = GYRO_SPI_INST;

		init_flg = 1;
	}
	write.r_size = bytes;
	errCode = ambarella_spi_write_then_read(&config, &write);
#ifdef TEST_GYRO_SPI
	{
		long long 	timediff;

		do_gettimeofday(&time2);
		timediff = (time2.tv_sec - time1.tv_sec)* 1000000LL + (time2.tv_usec - time1.tv_usec);
		printk("33>SPI latency=%lld micro seconds \n", timediff);
	}
#endif

	return errCode;
}

#if 0
static void mpu6000_dump_reg(void)
{
	int value=0;
	value = mpu6000_read_reg(AUX_VDDIO_REG);
	printk("AUX_VDDIO_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(SMPLRT_DIV_REG);
	printk("SMPLRT_DIV_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(CONFIG_REG);
	printk("CONFIG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_CONFIG_REG);
	printk("GYRO_CONFIG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_CONFIG_REG);
	printk("ACCEL_CONFIG_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(FF_THR_REG);
	printk("FF_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FF_DUR_REG);
	printk("FF_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_THR_REG);
	printk("MOT_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_DUR_REG);
	printk("MOT_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ZRMOT_THR_REG);
	printk("ZRMOT_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ZRMOT_DUR_REG);
	printk("ZRMOT_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_EN_REG);
	printk("FIFO_EN_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(INT_PIN_CFG_REG);
	printk("INT_PIN_CFG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(INT_ENABLE_REG);
	printk("INT_ENABLE_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(INT_STATUS_REG);
	printk("INT_STATUS_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(ACCEL_XOUT_H_REG);
	printk("ACCEL_XOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_XOUT_L_REG);
	printk("ACCEL_XOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_YOUT_H_REG);
	printk("ACCEL_YOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_YOUT_L_REG);
	printk("ACCEL_YOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_ZOUT_H_REG);
	printk("ACCEL_ZOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_ZOUT_L_REG);
	printk("ACCEL_ZOUT_L_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(TEMP_OUT_H_REG);
	printk("TEMP_OUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(TEMP_OUT_L_REG);
	printk("TEMP_OUT_L_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(GYRO_XOUT_H_REG);
	printk("GYRO_XOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_XOUT_L_REG);
	printk("GYRO_XOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_YOUT_H_REG);
	printk("GYRO_YOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_YOUT_L_REG);
	printk("GYRO_YOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_ZOUT_H_REG);
	printk("GYRO_ZOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_ZOUT_L_REG);
	printk("GYRO_ZOUT_L_REG=0x%x\r\n",value);


	value = mpu6000_read_reg(MOT_DETECT_STATUS_REG);
	printk("MOT_DETECT_STATUS_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(SIGNAL_PATH_RESET_REG);
	printk("SIGNAL_PATH_RESET_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_DETECT_CTRL_REG);
	printk("MOT_DETECT_CTRL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(USER_CTRL_REG);
	printk("USER_CTRL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(PWR_MGMT_1_REG);
	printk("PWR_MGMT_1_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(PWR_MGMT_2_REG);
	printk("PWR_MGMT_2_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_COUNTH_REG);
	printk("FIFO_COUNTH_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_COUNTL_REG);
	printk("FIFO_COUNTL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_R_W_REG);
	printk("FIFO_R_W_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(WHO_AM_I_REG);
	printk("WHO_AM_I_REG=0x%x\r\n",value);

}
#endif

void mpu6000_read(s32 *x, s32 *y, s32 *z)
{
	s16 yy, zz;

	mpu6000_burst_read_reg(GYRO_YOUT_H_REG, 4);
	yy = (s16)((*(gyro_rx) << 8) | *(gyro_rx + 1));
	zz = (s16)((*(gyro_rx + 2) << 8) | *(gyro_rx + 3));

	*x = 0;
	*y = yy + level_shift;
	*z = zz + level_shift;
}
EXPORT_SYMBOL(mpu6000_read);


static int mpu6000_init(void)
{
	if (open_success == -1) {
                printk("ERROR: Gyro open fail!!!\n");
	        return (-1);
	}

	mpu6000_write_reg(USER_CTRL_REG, 0x10);
	mpu6000_burst_read_reg(WHO_AM_I_REG, 1);
	if(gyro_rx[0] != WHO_AM_I_ID){
		printk("ERROR: mpu6000_init fails\n");
		open_success = -1;
		return -1;
	}
	mpu6000_write_reg(PWR_MGMT_1_REG, 0x9);

	//set sample rate SMPLRT_DIV_REG
	mpu6000_write_reg(SMPLRT_DIV_REG,SMPLRT_DIV);

	//set gyroscope output rate
	mpu6000_write_reg(CONFIG_REG, 0);

	return 0;
}


static int __init gyro_init(void)
{
	return mpu6000_init();
}

static void __exit gyro_exit(void)
{

}

module_init(gyro_init);
module_exit(gyro_exit);
MODULE_DESCRIPTION("Gyro mpu6000");
MODULE_LICENSE("Proprietary");


