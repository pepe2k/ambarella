/*
 * kernel/private/drivers/ambarella/vin/vin_pri.h
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __VIN_PRI_H
#define __VIN_PRI_H

#include <amba_vin.h>
#include <amba_debug.h>
#include <msg_print.h>

extern u32 vsync_irq_flag;
extern wait_queue_head_t vsync_irq_wait;

/* ========================================================================== */
#define AMBA_VIN_IRQ_NA			(0x00)
#define AMBA_VIN_IRQ_WAIT		(0x01)
#define AMBA_VIN_IRQ_READY		(0x02)

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif
#define vin_printk(level, format, arg...)	\
	DRV_PRINT(level format, ## arg)

#define vin_err(format, arg...)		\
	vin_printk(KERN_ERR , "Vin error: " format , ## arg)
#define vin_info(format, arg...)		\
	vin_printk(KERN_INFO , "Vin info: " format , ## arg)
#define vin_warn(format, arg...)		\
	vin_printk(KERN_WARNING , "Vin warn: " format , ## arg)
#define vin_notice(format, arg...)		\
	vin_printk(KERN_NOTICE , "Vin notice: " format , ## arg)

#ifndef CONFIG_AMBARELLA_VIN_DEBUG
#define vin_dbg(format, arg...)
#else
#define vin_dbg(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_VIN)	\
		vin_printk(KERN_DEBUG , "Vin debug: " format , ## arg)
#endif

#if 1
#define vin_dbgv(format, arg...)
#else
#define vin_dbgv(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_VIN)	\
		vin_printk(KERN_DEBUG , "Vin debug: " format , ## arg)
#endif

/* ========================================================================== */
#define AMBA_VIN_BASIC_PARAM_CALL(name, address, adapter, perm) \
	static int name##_addr = address; \
	module_param(name##_addr, int, perm); \
	static int adapter_id = adapter; \
	module_param(adapter_id, int, perm)

#define AMBA_VIN_HW_RESET()	\
	do { \
		if (!adapter_id) {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_power, 0); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_reset, 1); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_power, 1); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_reset, 0); \
		} else {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_power, 0); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_reset, 1); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_power, 1); \
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_reset, 0); \
		}	\
	} while(0)

#define AMBA_VIN_HW_POWERON()	\
	do { \
		if (!adapter_id) {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_power, 1); \
		} else {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_power, 1); \
		}	\
	} while(0)

#define AMBA_VIN_HW_POWEROFF()	\
	do { \
		if (!adapter_id) {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_power, 0); \
		} else {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_power, 0); \
		}	\
	} while(0)

#define AMBA_VIN_HW_AFON()	\
	do { \
		if (!adapter_id) {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_af_power, 1); \
		} else {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_af_power, 1); \
		}	\
	} while(0)

#define AMBA_VIN_HW_AFOFF()	\
	do { \
		if (!adapter_id) {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin0_af_power, 0); \
		} else {	\
			ambarella_set_gpio_output(&ambarella_board_generic.vin1_af_power, 0); \
		}	\
	} while(0)

/* ========================================================================== */
enum amba_vin_vsync_irq_info {
	AMBA_VIN_VSYNC_IRQ_VSYNC = 0x0000,
	AMBA_VIN_VSYNC_IRQ_VIN,
	AMBA_VIN_VSYNC_IRQ_IDSP_LAST_PIXEL,
	AMBA_VIN_VSYNC_IRQ_IDSP,
	AMBA_VIN_VSYNC_IRQ_GPIO,
	AMBA_VIN_VSYNC_IRQ_IDSP_SOF ,
	AMBA_VIN_VSYNC_IRQ_NUM
};

struct __amba_vin_irq_info {
	unsigned int irq;
	unsigned long flags;
	char name[AMBA_VIN_NAME_LENGTH];

	u32 counter;
	struct proc_dir_entry *proc_file;
	struct ambsync_proc_hinfo proc_hinfo;

	void *callback_data;
	void (*callback)(void *callback_data, u32 counter);
};

struct __amba_vin_adapter {
	int id;
	u32 dev_type;
	char name[AMBA_VIN_NAME_LENGTH];

	void *pinfo;
	struct list_head list;

	struct mutex cmd_lock;
	int (*docmd)(struct __amba_vin_adapter *adap,
		enum amba_vin_adap_cmd cmd, void *args);

	int total_source_num;
	int active_source_id;

	struct __amba_vin_irq_info *pvsync_irq;
	struct __amba_vin_irq_info *pvin_irq;
	struct __amba_vin_irq_info *pidsp_last_pixel_irq;
	struct __amba_vin_irq_info *pidsp_irq;
	struct __amba_vin_irq_info *pgpio_irq;
       struct __amba_vin_irq_info *pidsp_sof_irq;
	struct hrtimer vsync_timer;
	u32 vsync_mode;
	u32 vsync_delay; //usec

	spinlock_t lock;
};


struct __amba_vin_source {
	int id;
	int adapid;
	u32 dev_type;
	char name[AMBA_VIN_NAME_LENGTH];

	struct module *owner;
	void *pinfo;
	void *psrcinfo;
	struct list_head list;

	struct mutex cmd_lock;
	u32 total_channel_num;
	u32 active_channel_id;
	int (*docmd)(struct __amba_vin_source *src,
		enum amba_vin_src_cmd cmd, void *args);
};

/* ========================================================================== */
int amba_vin_add_adapter(struct __amba_vin_adapter *adap);
int amba_vin_del_adapter(struct __amba_vin_adapter *adap);

int amba_vin_add_source(struct __amba_vin_source *src);
int amba_vin_del_source(struct __amba_vin_source *src);

int amba_vin_irq_add(struct __amba_vin_adapter *adap, unsigned int irq,
	unsigned long flags, u32 mode, struct __amba_vin_irq_info *pirqinfo, u8 use_proc);
void amba_vin_irq_remove(struct __amba_vin_irq_info *pirqinfo);
int amba_vin_vsync_bind(struct __amba_vin_adapter *adap,
	struct amba_vin_irq_fix *pirq_fix);
void amba_vin_vsync_unbind(struct __amba_vin_adapter *adap);

void amba_vin_vsync_update_fps_stat(void);

/* ========================================================================== */
void rct_set_so_clk_src(u32 mode);
void rct_set_so_freq_hz(u32 freq_hz);
u32 rct_get_so_freq_hz(void);

#define VIN_LVDS_PAD_MODE_LVCMOS		(0)
#define VIN_LVDS_PAD_MODE_LVDS			(1)
#define VIN_LVDS_PAD_MODE_SLVS 			(2)
void rct_set_vin_lvds_pad(int mode);

/* ========================================================================== */

#endif //__VIN_PRI_H

