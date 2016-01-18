/*
 * kernel/private/drivers/ambarella/vin/sbrg/sbrg_pri.h
 *
 * History:
 *    2011/06/17 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __SBRG_PRI_H
#define __SBRG_PRI_H

#include <sbrg/amba_sbrg.h>
#include <amba_debug.h>

/* ========================================================================== */
#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#define sbrg_printk(level, format, arg...)	\
	DRV_PRINT(level format, ## arg)

#define sbrg_err(format, arg...)		\
	sbrg_printk(KERN_ERR , "Sbrg error: " format , ## arg)
#define sbrg_info(format, arg...)		\
	sbrg_printk(KERN_INFO , "Sbrg info: " format , ## arg)
#define sbrg_warn(format, arg...)		\
	sbrg_printk(KERN_WARNING , "Sbrg warn: " format , ## arg)
#define sbrg_notice(format, arg...)		\
	sbrg_printk(KERN_NOTICE , "Sbrg notice: " format , ## arg)

#ifndef CONFIG_AMBARELLA_SBRG_DEBUG
#define sbrg_dbg(format, arg...)
#else
#define sbrg_dbg(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_SBRG)	\
		sbrg_printk(KERN_DEBUG , "Sbrg debug: " format , ## arg)
#endif

#if 1
#define sbrg_dbgv(format, arg...)
#else
#define sbrg_dbgv(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_SBRG)	\
		sbrg_printk(KERN_DEBUG , "Sbrg debug: " format , ## arg)
#endif

/* ========================================================================== */

#define SBRG_S3D_ADAPTER_ID		0x0

#define AMBA_SBRG_BASIC_PARAM_CALL(name, address, adapter, perm) \
	static int name##_addr = address; \
	module_param(name##_addr, int, perm); \
	static int adapter_id = adapter; \
	module_param(adapter_id, int, perm)

/*
#define AMBA_SBRG_HW_RESET()	\
	do { \
	ambarella_set_gpio_reset(&ambarella_board_generic.sbrg_reset); \
	} while(0)
*/

/* ========================================================================== */

#define AMBA_SBRG_NAME_LENGTH		(32)
struct __amba_sbrg_adapter {
	int 			id;
	u32 			dev_type;
	char 			name[AMBA_SBRG_NAME_LENGTH];

	void 			*pinfo;

	struct list_head 	list;

	struct mutex 		cmd_lock;

	int (*docmd)(struct __amba_sbrg_adapter *adap,
		int cmd, void *args);
};

/* ========================================================================== */
int amba_sbrg_add_adapter(struct __amba_sbrg_adapter *adap);
int amba_sbrg_del_adapter(struct __amba_sbrg_adapter *adap);
struct __amba_sbrg_adapter *amba_sbrg_get_adapter(int id);

#endif //__SBRG_PRI_H

