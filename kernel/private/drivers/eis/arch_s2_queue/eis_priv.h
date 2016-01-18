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

#if 0
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

#endif


#define	EIS_MAJOR		248
#define	EIS_MINOR		10
//#define EIS_VIN_IRQ		IDSP_VIN_SOFT_IRQ  //use IDSP VIN Soft IRQ instead (vic 2.3)
#define EIS_VIN_IRQ		IDSP_LAST_PIXEL_IRQ  //use IDSP last pixel instead (vic 2.5)

#define MAX_ENTRY_IN_RING_BUFFER     1024


typedef struct amba_eis_controller_s {
        int vin_irq;
        //ring buffer
       gyro_data_t     * gyro_ring_buffer;
        int         ring_buffer_read_index;
        int         ring_buffer_write_index;
        int         ring_buffer_entry_num;
        int         ring_buffer_frame_sync_index;       //this index is a record of write_index at the time of FRAME START , to sync with frame start.
	struct completion frame_compl;
        int         ring_buffer_full;
     	spinlock_t data_lock;
        int         wait_count;
}amba_eis_controller_t;

typedef struct amba_eis_context_s {
	void 	* file;
	struct mutex	* mutex;
        amba_eis_controller_t  * controller;
} amba_eis_context_t;

int amba_eis_start_stat(amba_eis_context_t * context);
int amba_eis_stop_stat(amba_eis_context_t * context);
int amba_eis_get_stat(amba_eis_context_t * context,  amba_eis_stat_t * eis_stat);
int amba_eis_get_info(amba_eis_context_t * context,  amba_eis_info_t * eis_info);


int eis_init(amba_eis_controller_t * context);
void eis_exit(amba_eis_controller_t * context);
#endif
