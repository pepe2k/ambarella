/*
 * amba_debug.h
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_DEBUG_H
#define __AMBA_DEBUG_H

#define AMBA_DEBUG_IOC_MAGIC		'd'

#define AMBA_DEBUG_IOC_GET_DEBUG_FLAG		_IOR(AMBA_DEBUG_IOC_MAGIC, 1, int *)
#define AMBA_DEBUG_IOC_SET_DEBUG_FLAG		_IOW(AMBA_DEBUG_IOC_MAGIC, 1, int *)

enum amba_debug_module {
	AMBA_DEBUG_MODULE_AXI,
	AMBA_DEBUG_MODULE_AHB,
	AMBA_DEBUG_MODULE_APB,
	AMBA_DEBUG_MODULE_RAM,

	AMBA_DEBUG_MODULE_VIN,
	AMBA_DEBUG_MODULE_VOUT,
	AMBA_DEBUG_MODULE_VOUT2,
	AMBA_DEBUG_MODULE_HDMI,

	AMBA_DEBUG_MODULE_GPIO0,
	AMBA_DEBUG_MODULE_GPIO1,
	AMBA_DEBUG_MODULE_GPIO2,
	AMBA_DEBUG_MODULE_GPIO3,
	AMBA_DEBUG_MODULE_GPIO4,

	AMBA_DEBUG_MODULE_SPI,
	AMBA_DEBUG_MODULE_SPI2,
	AMBA_DEBUG_MODULE_SPI3,
	AMBA_DEBUG_MODULE_SPI_SLAVE,

	AMBA_DEBUG_MODULE_IDC,
	AMBA_DEBUG_MODULE_IDC2,

	AMBA_DEBUG_MODULE_ETH,
	AMBA_DEBUG_MODULE_ETH2,

	AMBA_DEBUG_MODULE_SD,
	AMBA_DEBUG_MODULE_SD2,

	AMBA_DEBUG_MODULE_RCT,
	AMBA_DEBUG_MODULE_RTC,
	AMBA_DEBUG_MODULE_ADC,
	AMBA_DEBUG_MODULE_WDOG,
	AMBA_DEBUG_MODULE_TIMER,
	AMBA_DEBUG_MODULE_PWM,
	AMBA_DEBUG_MODULE_STEPPER,
	AMBA_DEBUG_MODULE_IR,
	AMBA_DEBUG_MODULE_VIC,
	AMBA_DEBUG_MODULE_VIC2,

	AMBA_DEBUG_MODULE_SENSOR,
	AMBA_DEBUG_MODULE_SBRG,

	AMBA_DEBUG_MODULE_FDET,

	AMBA_DEBUG_MODULE_NUM,
};

struct amba_debug_module_info
{
	u32			exist;
	u32			start;
	u32			size;
};

struct amba_debug_mem_info
{
	struct amba_debug_module_info		modules[AMBA_DEBUG_MODULE_NUM];
};

#define AMBA_DEBUG_MODULE_SIZE_UNKNOWN		0xffffffff

#define AMBA_DEBUG_IOC_GET_MEM_INFO		_IOR(AMBA_DEBUG_IOC_MAGIC, 2, struct amba_debug_mem_info *)

#define AMBA_DEBUG_IOC_VIN_SET_SRC_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 200, int)

#define AMBA_DEBUG_IOC_VIN_GET_DEV_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 201, u32 *)

struct amba_vin_test_reg_data {
	u32 reg;
	u32 data;
	u32 regmap;
};
#define AMBA_DEBUG_IOC_VIN_GET_REG_DATA		_IOR(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)
#define AMBA_DEBUG_IOC_VIN_SET_REG_DATA		_IOW(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)

struct amba_vin_test_gpio {
	u32 id;
	u32 data;
};
#define AMBA_DEBUG_IOC_GET_GPIO			_IOR(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)
#define AMBA_DEBUG_IOC_SET_GPIO			_IOW(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)

#define AMBA_DEBUG_IOC_VIN_GET_SBRG_REG_DATA		_IOR(AMBA_DEBUG_IOC_MAGIC, 204, struct amba_vin_test_reg_data *)
#define AMBA_DEBUG_IOC_VIN_SET_SBRG_REG_DATA		_IOW(AMBA_DEBUG_IOC_MAGIC, 204, struct amba_vin_test_reg_data *)

#endif	//AMBA_DEBUG_IOC_MAGIC

