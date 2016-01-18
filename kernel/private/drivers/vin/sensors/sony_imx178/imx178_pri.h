/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_pri.h
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX178_PRI_H__
#define __IMX178_PRI_H__

#define IMX178_STANDBY    		0x3000
#define IMX178_XMSTA			0x3008
#define IMX178_SWRESET    		0x3009

#define IMX178_WINMODE    		0x300F

#define IMX178_LPMODE    		0x301B

#define IMX178_GAIN_LSB    		0x301F
#define IMX178_GAIN_MSB    		0x3020

#define IMX178_VMAX_LSB    		0x302C
#define IMX178_VMAX_MSB    		0x302D
#define IMX178_VMAX_HSB   		0x302E
#define IMX178_HMAX_LSB    		0x302F
#define IMX178_HMAX_MSB   		0x3030

#define IMX178_SHS1_LSB    		0x3034
#define IMX178_SHS1_MSB   		0x3035
#define IMX178_SHS1_HSB   		0x3036

#define IMX178_VIDEO_FORMAT_REG_NUM			(15)
#define IMX178_VIDEO_FORMAT_REG_TABLE_SIZE		(9)
#define IMX178_VIDEO_PLL_REG_TABLE_SIZE		(48)

#define IMX178_V_FLIP	(1<<0)
#define IMX178_H_MIRROR	(1<<1)

/* ========================================================================== */
struct imx178_reg_table {
	u16 reg;
	u16 data;
};

struct imx178_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx178_reg_table regs[IMX178_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx178_video_format_reg_table {
	u16 reg[IMX178_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[IMX178_VIDEO_FORMAT_REG_NUM];
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
		u32 h_inck;
		u8 lane_num;
	} table[IMX178_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx178_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 type_ext;
	u8 bayer_pattern;
};

struct imx178_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __IMX178_PRI_H__ */
