/*
 * kernel/private/drivers/ambarella/vout/vout_pri.h
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

#ifndef __VOUT_PRI_H
#define __VOUT_PRI_H

#include <amba_vout.h>
#include <amba_debug.h>
#include <msg_print.h>

/* ========================================================================== */
#define AMBA_VOUT_TIMEOUT		(HZ / 6 + 1)

#define AMBA_VOUT_IRQ_NA		(0x00)
#define AMBA_VOUT_IRQ_WAIT		(1 << 0)
#define AMBA_VOUT_IRQ_READY		(1 << 1)

#define AMBA_VOUT_SETUP_NA		(0x00)
#define AMBA_VOUT_SETUP_VALID		(1 << 0)
#define AMBA_VOUT_SETUP_CHANGED		(1 << 1)
#define AMBA_VOUT_SETUP_NEW		(AMBA_VOUT_SETUP_VALID | AMBA_VOUT_SETUP_CHANGED)

#define VO_CLK_ONCHIP_PLL_27MHZ   	0x0    	/* Default setting */

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#define vout_printk(level, format, arg...)	\
	DRV_PRINT(level format, ## arg)

#define vout_err(format, arg...)		\
	vout_printk(KERN_ERR , "Vout error: " format , ## arg)
#define vout_info(format, arg...)		\
	vout_printk(KERN_INFO , "Vout info: " format , ## arg)
#define vout_warn(format, arg...)		\
	vout_printk(KERN_WARNING , "Vout warn: " format , ## arg)
#define vout_notice(format, arg...)		\
	vout_printk(KERN_NOTICE , "Vout notice: " format , ## arg)

#ifndef CONFIG_AMBARELLA_VOUT_DEBUG
#define vout_dbg(format, arg...)
#else
#define vout_dbg(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_VOUT)	\
		vout_printk(KERN_DEBUG , "Vout debug: " format , ## arg)
#endif

#if 1
#define vout_dbgv(format, arg...)
#else
#define vout_dbgv(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_VOUT)	\
		vout_printk(KERN_DEBUG , "Vout debug: " format , ## arg)
#endif

#define vout_errorcode()			\
	vout_err("%s:%d = %d!\n", __func__, __LINE__, errorCode)

#ifndef CONFIG_AMBARELLA_VOUT_DEBUG
#define vout_dsp(cmd, arg)
#else
#define vout_dsp(cmd, arg)		\
	DRV_PRINT("%s: "#arg" = 0x%X\n", __func__, (u32)cmd.arg);
#endif

static inline int amba_vout_hdmi_check_i2c(struct i2c_adapter *adap)
{
	int				errorCode = 0;

	if (strcmp(adap->name, "ambarella-i2c") != 0) {
		errorCode = -ENODEV;
		goto amba_vin_check_i2c_exit;
	}

	if ((adap->class & I2C_CLASS_DDC) == 0) {
		errorCode = -EACCES;
		goto amba_vin_check_i2c_exit;
	}

amba_vin_check_i2c_exit:
	return errorCode;
}

/* ========================================================================== */
struct __amba_vout_video_source {
	int id;
	u32 source_type;
	char name[AMBA_VOUT_NAME_LENGTH];

	struct module *owner;
	void *pinfo;
	struct list_head list;

	struct mutex cmd_lock;
	int (*docmd)(struct __amba_vout_video_source *adap,
		enum amba_video_source_cmd cmd, void *args);

	int total_sink_num;
	int active_sink_id;
};

struct __amba_vout_video_sink {
	int id;
	int source_id;
	enum amba_vout_sink_type sink_type;
	char name[AMBA_VOUT_NAME_LENGTH];
	enum amba_vout_sink_state pstate;	//state before suspension
	enum amba_vout_sink_state state;
	enum amba_vout_sink_plug hdmi_plug;
	enum amba_video_mode hdmi_modes[32];
	enum amba_video_mode hdmi_native_mode;
	u16 hdmi_native_width;
	u16 hdmi_native_height;
	enum amba_vout_hdmi_overscan hdmi_overscan;
	u32 hdmi_force_native_timing;

	struct module *owner;
	void *pinfo;
	struct list_head list;

	struct mutex cmd_lock;
	int (*docmd)(struct __amba_vout_video_sink *src,
		enum amba_video_sink_cmd cmd, void *args);
};

/* ========================================================================== */
int amba_vout_add_video_source(struct __amba_vout_video_source *psrc);
int amba_vout_del_video_source(struct __amba_vout_video_source *psrc);

int amba_vout_add_video_sink(struct __amba_vout_video_sink *psink);
int amba_vout_del_video_sink(struct __amba_vout_video_sink *psink);

/* ========================================================================== */
void rct_set_vout_clk_src(u32 mode);
void rct_set_vout_freq_hz(u32 freq_hz);
u32 rct_get_vout_freq_hz(void);

void rct_set_vout2_clk_src(u32 mode);
void rct_set_vout2_freq_hz(u32 freq_hz);
u32 rct_get_vout2_freq_hz(void);

/* ========================================================================== */

#endif //__VOUT_PRI_H

