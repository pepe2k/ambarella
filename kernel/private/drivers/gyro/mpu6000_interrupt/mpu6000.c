/*
 * kernel/private/drivers/ambarella/gyro/mpu6000_interrupt/mpu6000.c
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
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
#include "mpu6000.h"

mpu6000_info_t	*pinfo = NULL;

void gyro_register_eis_callback(gyro_eis_callback_t cb, void *arg)
{
	pinfo->eis_callback	= cb;
	pinfo->eis_arg		= arg;
}
EXPORT_SYMBOL(gyro_register_eis_callback);

void gyro_unregister_eis_callback(void)
{
	pinfo->eis_callback	= NULL;
	pinfo->eis_arg		= NULL;
}
EXPORT_SYMBOL(gyro_unregister_eis_callback);

static int mpu6000_write_reg(u8 subaddr, u8 data)
{
	u8			tx[2];
	amba_spi_cfg_t		config;
	amba_spi_write_t	write;

	config.cfs_dfs		= 8;
	config.baud_rate	= MPU6000_SPI_WRITE_CLK;
	config.cs_change	= 1;
	config.spi_mode		= SPI_MODE_3;

	tx[0]			= subaddr & 0x7F;
	tx[1]			= data;

	write.buffer		= tx;
	write.n_size		= sizeof(tx);
	write.bus_id		= MPU6000_SPI_BUS_ID;
	write.cs_id		= MPU6000_SPI_CS_ID;

	return ambarella_spi_write(&config, &write);
}

static int mpu6000_read_reg(u8 subaddr, u8 *data, int len)
{
	u8				tx[1];
	amba_spi_cfg_t			config;
	amba_spi_write_then_read_t	wr;

	config.cfs_dfs		= 8;
	config.baud_rate	= MPU6000_SPI_READ_CLK;
	config.cs_change	= 1;
	config.spi_mode		= SPI_MODE_3;

	tx[0]			= subaddr | 0x80;

	wr.w_buffer		= tx;
	wr.w_size		= sizeof(tx);
	wr.r_buffer		= data;
	wr.r_size		= len;
	wr.bus_id		= MPU6000_SPI_BUS_ID;
	wr.cs_id		= MPU6000_SPI_CS_ID;

	return ambarella_spi_write_then_read(&config, &wr);
}

static void mpu6000_read_data(gyro_data_t *pdata)
{
	u8		data[4];
	s16		y, z;

	mpu6000_read_reg(MPU6000_REG_YOUT_H, data, 4);

	y = (s16)((data[0] << 8) | data[1]) + (1 << 15);
	z = (s16)((data[2] << 8) | data[3]) + (1 << 15);

	pdata->x	= 0;
	pdata->y	= y;
	pdata->z	= z;
}

  struct timeval      cur_time;

static irqreturn_t mpu6000_isr(int irq, void *dev_data)
{

	pinfo->data_ready = 1;
    	do_gettimeofday(&cur_time);
//       printk(KERN_DEBUG "mpu6000_isr:  time %ld \n", cur_time.tv_sec * 1000000L + cur_time.tv_usec);
	wake_up(&pinfo->waitqueue);

	return IRQ_HANDLED;
}

static int mpu6000_daemon(void *arg)
{
	gyro_data_t		data;
        struct timeval      cur_time2;
	while (1) {
		wait_event_interruptible(pinfo->waitqueue,
			pinfo->data_ready || pinfo->killing_daemon);

        	do_gettimeofday(&cur_time2);
              printk(KERN_DEBUG "mpu6000_daemon:  time diff %ld \n", (cur_time2.tv_sec  - cur_time.tv_sec)* 1000000L + (cur_time2.tv_usec - cur_time.tv_usec));

		if (pinfo->killing_daemon) {
			break;
		}

		pinfo->data_ready = 0;
		mpu6000_read_data(&data);
		spin_lock(&pinfo->data_lock);
		pinfo->data = data;
		spin_unlock(&pinfo->data_lock);

		if (pinfo->eis_callback) {
			pinfo->eis_callback(&data, pinfo->eis_arg);
		}
	}

	return 0;
}

static int __init mpu6000_init(void)
{
	int				errCode = 0;
	u8				data;
	struct ambarella_gpio_irq_info	*pirq;

	mpu6000_read_reg(MPU6000_REG_WHO_AM_I, &data, 1);
	if(data != MPU6000_ID){
		printk("%s: SPI Bus Error!\n", __func__);
		errCode = -EIO;
		goto mpu6000_init_exit;
	}

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		printk("%s: Out of Memory!\n", __func__);
		errCode = -ENOMEM;
		goto mpu6000_init_exit;
	}

	pinfo->irq = ambarella_board_generic.gyro_irq;
	init_waitqueue_head(&pinfo->waitqueue);
	spin_lock_init(&pinfo->data_lock);
	pinfo->daemon = kthread_run(mpu6000_daemon, pinfo, "mpu6000d");

	/* Disable I2C Interface */
	mpu6000_write_reg(MPU6000_REG_USER_CTRL, 0x10);

	/* Disable Temperature Sensor */
	mpu6000_write_reg(MPU6000_REG_PWR_MGMT_1, 0x9);

	/* 1KHz Output Data Rate */
	mpu6000_write_reg(MPU6000_REG_CONFIG, 0);
	mpu6000_write_reg(MPU6000_REG_SMPLRT_DIV, 7);

	/* Configure Interrupt */
	mpu6000_write_reg(MPU6000_REG_INTERRUPT_PIN,
		MPU6000_INT_RD_CLEAR | MPU6000_LATCH_INT_EN |
		MPU6000_INT_OPEN_DRAIN | MPU6000_INT_LEVEL_LOW);
	mpu6000_write_reg(MPU6000_REG_INTERRUPT_ENABLE, MPU6000_DATA_READY);

	pirq = &pinfo->irq;
	ambarella_gpio_config(pirq->irq_gpio, pirq->irq_gpio_mode);
	errCode = request_irq(pirq->irq_line, mpu6000_isr,
			pirq->irq_type, "MPU6000", pinfo);
	if (errCode < 0) {
		printk("%s: Fail to reqeust irq!\n", __func__);
		goto mpu6000_init_exit;
	}

	mpu6000_read_data(&pinfo->data);

mpu6000_init_exit:
	if (errCode && pinfo) {
		kfree(pinfo);
		pinfo = NULL;
	}
	return errCode;
}

static void __exit mpu6000_exit(void)
{
	if (pinfo) {
		free_irq(pinfo->irq.irq_line, pinfo);

		pinfo->killing_daemon = 1;
		wake_up(&pinfo->waitqueue);
		kthread_stop(pinfo->daemon);

		kfree(pinfo);
		pinfo = NULL;
	}
}

void gyro_get_info(gyro_info_t *gyroinfo)
{
	memset(gyroinfo, 0, sizeof(gyro_info_t));

	gyroinfo->gyro_x_polar		= 1;
	gyroinfo->gyro_y_polar		= 1;
	gyroinfo->gyro_x_chan		= 1;
	gyroinfo->gyro_y_chan		= 2;
	gyroinfo->vol_div_num		= 1;
	gyroinfo->vol_div_den		= 1;
	gyroinfo->sensor_axis		= 2;
	gyroinfo->max_sense		= 135;				// LSB/(deg/sec)
	gyroinfo->min_sense		= 127;				// LSB/(deg/sec)
	gyroinfo->max_bias		= 35388;//2620;			// LSB
	gyroinfo->min_bias		= 30148;//-2620;		// LSB
	gyroinfo->max_rms_noise		= 7;				// LSB
	gyroinfo->start_up_time		= 30;				// ms
	gyroinfo->full_scale_range	= 250;				// deg/sec
	gyroinfo->max_sampling_rate	= 8000;				// sample/sec
	gyroinfo->adc_resolution	= 16;                           // bits
	gyroinfo->phs_dly_in_ms		= 4;				// ms
	gyroinfo->sampling_rate		= gyroinfo->max_sampling_rate ;
}
EXPORT_SYMBOL(gyro_get_info);

module_init(mpu6000_init);
module_exit(mpu6000_exit);
MODULE_DESCRIPTION("Ambarella Gyro Sensor MPU6000 Driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");


