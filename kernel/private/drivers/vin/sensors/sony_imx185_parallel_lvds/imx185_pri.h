/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185_pri.h
 *
 * History:
 *    2013/04/02 - [Ken He] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX185_PRI_H__
#define __IMX185_PRI_H__

#define IMX185_STANDBY    		0x3000
#define IMX185_REGHOLD			0x3001
#define IMX185_XMSTA			0x3002
#define IMX185_SWRESET    		0x3003

#define IMX185_ADBIT			0x3005
#define IMX185_MODE				0x3006
#define IMX185_WINMODE    		0x3007

#define IMX185_FRSEL			0x3009
#define IMX185_BLKLEVEL_LSB		0X300A
#define IMX185_BLKLEVEL_MSB		0x300B

#define IMX185_GAIN_LSB    		0x3014

#define IMX185_VMAX_LSB    		0x3018
#define IMX185_VMAX_MSB    		0x3019
#define IMX185_VMAX_HSB   		0x301A
#define IMX185_HMAX_LSB    		0x301B
#define IMX185_HMAX_MSB   		0x301C

#define IMX185_SHS1_LSB    		0x3020
#define IMX185_SHS1_MSB   		0x3021
#define IMX185_SHS1_HSB   		0x3022

#define IMX185_ODBIT			0x3044

#define IMX185_INCKSEL1			0x305C
#define IMX185_INCKSEL2			0x305D
#define IMX185_INCKSEL3			0x305E
#define IMX185_INCKSEL4			0x305F
#define IMX185_INCKSEL5			0x3063

#define IMX185_VIDEO_FPS_REG_NUM		(2)
#define IMX185_VIDEO_FORMAT_REG_NUM			(10)
#define IMX185_VIDEO_FORMAT_REG_TABLE_SIZE		(6)
#define IMX185_VIDEO_PLL_REG_TABLE_SIZE		(5)

#define IMX185_V_FLIP	(1<<0)
#define IMX185_H_MIRROR	(1<<1)

/* ========================================================================== */
struct imx185_reg_table {
	u16 reg;
	u16 data;
};

struct imx185_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx185_reg_table regs[IMX185_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx185_video_fps_reg_table {
	u16 reg[IMX185_VIDEO_FPS_REG_NUM];
	struct {
	const struct imx185_pll_reg_table *pll_reg_table;
	u32 fps;
	u8 system;
	u16 data[IMX185_VIDEO_FPS_REG_NUM];
	} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct imx185_video_format_reg_table {
	u16 reg[IMX185_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[IMX185_VIDEO_FORMAT_REG_NUM];
		const struct imx185_video_fps_reg_table *fps_table;
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
		u32 h_pixel;
		u32 data_rate;
	} table[IMX185_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx185_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx185_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __IMX185_PRI_H__ */
