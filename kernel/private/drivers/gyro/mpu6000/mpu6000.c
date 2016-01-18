/*
 * kernel/private/drivers/ambarella/gyro/mpu6000/mpu6000.c
 * This version use MPU6000 to generate interrupt, but use SPI polling mode to read within tasklet
 * The purpose is to avoid using workqueue and avoid the wait for SPI interrupt mode, in order to
 * get best real time performance
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *    2013/09/25 - [Louis Sun] Modify it to use tasklet to read SPI
 *
 * Copyright (C) 2004-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <linux/interrupt.h>
#include <amba_common.h>
#include <amba_eis.h>
#include "mpu6000.h"


static struct tasklet_struct gyro_tasklet;
//static struct timeval      cur_time;
static int  sample_id = 0;
mpu6000_info_t	*pinfo = NULL;

void gyro_register_eis_callback(GYRO_EIS_CALLBACK cb, void *arg)
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
       #define ACCEL_GYRO_TOTAL_SIZE 14
       #define ACCEL_OFFSET 0
       #define GYRO_OFFSET 8
	u8		data[ACCEL_GYRO_TOTAL_SIZE];
	s16		x, y, z;

	mpu6000_read_reg(MPU6000_REG_ACCEL_XOUT_H, data, ACCEL_GYRO_TOTAL_SIZE);
        x = (s16)((data[ACCEL_OFFSET+0] << 8) | data[ACCEL_OFFSET+1]);
        y  = (s16)((data[ACCEL_OFFSET+2] << 8) | data[ACCEL_OFFSET+3]);
	 z = (s16)((data[ACCEL_OFFSET+4] << 8) | data[ACCEL_OFFSET+5]);
	pdata->xa	= x;
	pdata->ya	= y;
	pdata->za	= z;
        x = (s16)((data[GYRO_OFFSET+0] << 8) | data[GYRO_OFFSET+1]);
        y  = (s16)((data[GYRO_OFFSET+2] << 8) | data[GYRO_OFFSET+3]);
	 z = (s16)((data[GYRO_OFFSET+4] << 8) | data[GYRO_OFFSET+5]);
	pdata->xg	= x;
	pdata->yg	= y;
	pdata->zg	= z;

       pdata->sample_id = sample_id++;
      //printk(KERN_DEBUG "xa=0x%x, ya=0x%x, za=0x%x,   xg=0x%x,yg=0x%x,zg=0x%x\n",  pdata->xa, pdata->ya, pdata->za, pdata->xg, pdata->yg, pdata->zg);

}


static void  gyro_read_tasklet(unsigned long arg)
{

//        unsigned long flags;

        if (unlikely(pinfo->busy))        //return when it's already busy
       {
            printk(KERN_DEBUG "already busy\n");
            return;
       }

        pinfo->busy = 1;
   //     spin_lock_irqsave(&pinfo->data_lock, flags);
   // struct timeval      cur_time2;
        mpu6000_read_data(&(pinfo->data));
    // do_gettimeofday(&cur_time2);
    //   printk(KERN_DEBUG "mpu6000 gyro tasklet:  time diff %ld \n", (cur_time2.tv_sec  - cur_time.tv_sec)* 1000000L + (cur_time2.tv_usec - cur_time.tv_usec));

       if (pinfo->eis_callback) {
            pinfo->eis_callback(&(pinfo->data), pinfo->eis_arg);
        }

   //    spin_unlock_irqrestore(&pinfo->data_lock, flags);
       pinfo->busy = 0;
}



static irqreturn_t mpu6000_isr(int irq, void *dev_data)
{
         // do_gettimeofday(&cur_time);
        tasklet_schedule(&gyro_tasklet);
        return IRQ_HANDLED;
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
       pinfo->busy = 0;
        //init tasklet
//  	spin_lock_init(&pinfo->data_lock);
        memset(&gyro_tasklet, 0, sizeof(struct tasklet_struct));
        tasklet_init(&gyro_tasklet, gyro_read_tasklet, (unsigned long)pinfo);

	/* Disable I2C Interface */
	mpu6000_write_reg(MPU6000_REG_USER_CTRL, 0x10);

	/* Disable Temperature Sensor */
	mpu6000_write_reg(MPU6000_REG_PWR_MGMT_1, 0x9);

	/* 2KHz Output Data Rate */
	mpu6000_write_reg(MPU6000_REG_CONFIG, 0);
	mpu6000_write_reg(MPU6000_REG_SMPLRT_DIV, 3);

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
              tasklet_disable(&gyro_tasklet);       //disable tasklet with sync
		free_irq(pinfo->irq.irq_line, pinfo);
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


