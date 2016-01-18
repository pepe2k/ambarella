/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds_pri.h
 *
 * History:
 *    2014/12/30 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBDS_PRI_H__
#define __AMBDS_PRI_H__

#define AMBDS_STANDBY    		0x3000
#define AMBDS_XMSTA			0x3002
#define AMBDS_SWRESET    		0x3003

#define AMBDS_WINMODE    		0x3007
#define AMBDS_FRSEL			0x3009

#define AMBDS_GAIN_LSB    		0x3014
#define AMBDS_GAIN_MSB    		0x3015

#define AMBDS_VMAX_LSB    		0x3018
#define AMBDS_VMAX_MSB    		0x3019
#define AMBDS_VMAX_HSB   		0x301A
#define AMBDS_HMAX_LSB    		0x301B
#define AMBDS_HMAX_MSB   		0x301C

#define AMBDS_SHS1_LSB    		0x301E
#define AMBDS_SHS1_MSB   		0x301F
#define AMBDS_SHS1_HSB   		0x3020

#define AMBDS_INCKSEL1			0x3061
#define AMBDS_INCKSEL2			0x3062
#define AMBDS_INCKSEL3			0x316C
#define AMBDS_INCKSEL4			0x316D
#define AMBDS_INCKSEL5			0x3170
#define AMBDS_INCKSEL6			0x3171

#define AMBDS_VIDEO_FORMAT_REG_NUM		(0)
#define AMBDS_VIDEO_FORMAT_REG_TABLE_SIZE	(2)
#define AMBDS_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define AMBDS_V_FLIP	(1<<0)
#define AMBDS_H_MIRROR	(1<<1)
/* ========================================================================== */
struct ambds_reg_table {
	u16 reg;
	u16 data;
};

struct ambds_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct ambds_reg_table regs[AMBDS_VIDEO_PLL_REG_TABLE_SIZE];
};

struct ambds_video_format_reg_table {
	u16 reg[AMBDS_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[AMBDS_VIDEO_FORMAT_REG_NUM];
		u16 width;
		u16 height;
		u8 format;
		u8 type;
		u8 bits;
		u8 ratio;
		u32 srm;
		u32 sw_limit;
		u32 max_fps;
		u32 auto_fps;
		u32 pll_index;
		u16 slvs_eav_col;
		u16 slvs_sav2sav_dist;
	} table[AMBDS_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct ambds_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 type_ext;
};

struct ambds_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __AMBDS_PRI_H__ */
