/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_pri.h
 *
 * History:
 *    2013/12/27 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX123_PRI_H__
#define __IMX123_PRI_H__

#define IMX123_STANDBY    		0x3000
#define IMX123_XMSTA			0x3002
#define IMX123_SWRESET    		0x3003

#define IMX123_WINMODE    		0x3007

#define IMX123_GAIN_LSB    		0x3014
#define IMX123_GAIN_MSB    		0x3015

#define IMX123_VMAX_LSB    		0x3018
#define IMX123_VMAX_MSB    		0x3019
#define IMX123_VMAX_HSB   		0x301A
#define IMX123_HMAX_LSB    		0x301B
#define IMX123_HMAX_MSB   		0x301C

#define IMX123_SHS1_LSB    		0x301E
#define IMX123_SHS1_MSB   		0x301F
#define IMX123_SHS1_HSB   		0x3020

#define IMX123_SHS2_LSB    		0x3021
#define IMX123_SHS2_MSB   		0x3022
#define IMX123_SHS2_HSB   		0x3023

#define IMX123_SHS3_LSB    		0x3024
#define IMX123_SHS3_MSB   		0x3025
#define IMX123_SHS3_HSB   		0x3026

#define IMX123_RHS1_LSB    		0x302E
#define IMX123_RHS1_MSB   		0x302F
#define IMX123_RHS1_HSB   		0x3030

#define IMX123_RHS2_LSB    		0x3031
#define IMX123_RHS2_MSB   		0x3032
#define IMX123_RHS2_HSB   		0x3033

#define IMX123_GAIN2_LSB    		0x30F2
#define IMX123_GAIN2_MSB    		0x30F3

#define IMX123_VIDEO_FORMAT_REG_NUM			(30)
#define IMX123_VIDEO_FORMAT_REG_TABLE_SIZE	(10)
#define IMX123_VIDEO_PLL_REG_TABLE_SIZE		(6)

#define IMX123_V_FLIP	(1<<0)
#define IMX123_H_MIRROR	(1<<1)

#define IMX123_LINEAR_MODE (0)
#define IMX123_2X_WDR_MODE (1)
#define IMX123_3X_WDR_MODE (2)

#define IMX123_QXGA_BRL		(1564)
#define IMX123_1080P_BRL		(1108)
#define IMX123_QXGA_H_PIXEL	(2064)
#define IMX123_1080P_H_PIXEL	(1936)
#define IMX123_H_PERIOD		(2250)
/* ========================================================================== */
struct imx123_reg_table {
	u16 reg;
	u16 data;
};

struct imx123_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx123_reg_table regs[IMX123_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx123_video_format_reg_table {
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		struct imx123_reg_table regs[IMX123_VIDEO_FORMAT_REG_NUM];
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
		struct amba_vin_wdr_win_offset hdr_win_offset;
	} table[IMX123_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx123_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 act_start_x;
	u16 act_start_y;
	u16 act_width;
	u16 act_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx123_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __IMX123_PRI_H__ */
