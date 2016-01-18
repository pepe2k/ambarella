/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_pri.h
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX124_PRI_H__
#define __IMX124_PRI_H__

#define IMX124_STANDBY    		0x3000
#define IMX124_XMSTA			0x3002
#define IMX124_SWRESET    		0x3003

#define IMX124_WINMODE    		0x3007
#define IMX124_FRSEL			0x3009

#define IMX124_GAIN_LSB    		0x3014
#define IMX124_GAIN_MSB    		0x3015

#define IMX124_VMAX_LSB    		0x3018
#define IMX124_VMAX_MSB    		0x3019
#define IMX124_VMAX_HSB   		0x301A
#define IMX124_HMAX_LSB    		0x301B
#define IMX124_HMAX_MSB   		0x301C

#define IMX124_SHS1_LSB    		0x301E
#define IMX124_SHS1_MSB   		0x301F
#define IMX124_SHS1_HSB   		0x3020

#define IMX124_INCKSEL1			0x3061
#define IMX124_INCKSEL2			0x3062
#define IMX124_INCKSEL3			0x316C
#define IMX124_INCKSEL4			0x316D
#define IMX124_INCKSEL5			0x3170
#define IMX124_INCKSEL6			0x3171

#define IMX124_VIDEO_FORMAT_REG_NUM			(7)
#define IMX124_VIDEO_FORMAT_REG_TABLE_SIZE	(2)
#define IMX124_VIDEO_PLL_REG_TABLE_SIZE		(6)

#define IMX124_V_FLIP	(1<<0)
#define IMX124_H_MIRROR	(1<<1)
/* ========================================================================== */
struct imx124_reg_table {
	u16 reg;
	u16 data;
};

struct imx124_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx124_reg_table regs[IMX124_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx124_video_format_reg_table {
	u16 reg[IMX124_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[IMX124_VIDEO_FORMAT_REG_NUM];
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
	} table[IMX124_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx124_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 act_start_x;
	u16 act_start_y;
	u16 act_width;
	u16 act_height;
	u16 slvs_eav_col;
	u16 slvs_sav2sav_dist;
	u16 sync_start;
	u8 type_ext;
	u8 bayer_pattern;
};

struct imx124_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __IMX124_PRI_H__ */
