/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_priv.h
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

#ifndef __EIS_PRIV_H__
#define __EIS_PRIV_H__

#include <eis_drv.h>

typedef struct {
	eis_coeff_info_t	coeff_info[2];
	eis_update_info_t	update_info;
} eis_enhance_turbo_buf_t;

typedef enum {
	EIS_BUF_PING = 0,
	EIS_BUF_PONG = 1,

	EIS_BUF_NA   = 255,
} eis_buf_pingpong_t;

typedef struct {
	u8	data[32];
} eis_sensor_data_t;

typedef struct {
	eis_buf_pingpong_t	cortex_pingpong;
	eis_buf_pingpong_t	arm11_pingpong;
	eis_sensor_data_t	sensor_data[2];
} eis_sensor_ipc_t;

typedef struct {
	eis_enhance_turbo_buf_t	enhance_turbo_buf;
	eis_sensor_ipc_t	sensor_ipc;
} eis_share_data_t;

typedef struct {
	struct cdev		char_dev;
	dev_t			dev_id;
	int			vin_irq;

	eis_share_data_t	*share_data;
} eis_ipc_info_t;

#define EIS_IPC_DATA_PHYS_ADDR	get_ambarella_ppm_phys()
#define EIS_IPC_DATA_VIRT_ADDR	get_ambarella_ppm_virt()

#define	EIS_MAJOR		248
#define	EIS_MINOR		10

#define EIS_VIN_IRQ		IDSP_SENSOR_VSYNC_IRQ

#endif
