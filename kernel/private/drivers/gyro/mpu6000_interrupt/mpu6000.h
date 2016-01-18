/*
 * kernel/private/drivers/ambarella/gyro/mpu6000_interrupt/mpu6000.h
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

#ifndef __MPU6000_H__
#define __MPU6000_H__

#include <eis_drv.h>

#define MPU6000_SPI_BUS_ID		0
#define MPU6000_SPI_CS_ID		1
#define MPU6000_SPI_WRITE_CLK  		(1000 * 1000)
#define MPU6000_SPI_READ_CLK  		(10 * 1000 * 1000)

typedef enum {
	MPU6000_REG_SMPLRT_DIV		= 25,
	MPU6000_REG_CONFIG		= 26,

	MPU6000_REG_INTERRUPT_PIN	= 55,
	MPU6000_REG_INTERRUPT_ENABLE	= 56,
	MPU6000_REG_INTERRUPT_STATUS	= 58,

	MPU6000_REG_YOUT_H		= 69,

	MPU6000_REG_USER_CTRL		= 106,
	MPU6000_REG_PWR_MGMT_1		= 107,

	MPU6000_REG_WHO_AM_I		= 117,
} mpu6000_reg_t;

#define MPU6000_ID			0x68

#define MPU6000_INT_RD_CLEAR		(1 << 4)
#define MPU6000_LATCH_INT_EN		(1 << 5)
#define	MPU6000_INT_OPEN_DRAIN		(1 << 6)
#define MPU6000_INT_LEVEL_LOW		(1 << 7)

#define	MPU6000_DATA_READY		(1 << 0)

typedef struct {
	struct ambarella_gpio_irq_info	irq;

	struct task_struct		*daemon;
	u32				killing_daemon;
	wait_queue_head_t		waitqueue;
	u32				data_ready;

	gyro_data_t			data;
	spinlock_t			data_lock;

	gyro_eis_callback_t		eis_callback;
	void				*eis_arg;
} mpu6000_info_t;

#endif

