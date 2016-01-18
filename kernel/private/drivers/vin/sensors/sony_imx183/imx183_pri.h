/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx183/imx183_pri.h
 *
 * History:
 *    2014/08/13 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX183_PRI_H__
#define __IMX183_PRI_H__


#define IMX183_REG_00			0x0000
#define IMX183_REG_01			0x0001
#define IMX183_REG_02			0x0002
#define IMX183_REG_03			0x0003

#define IMX183_REG_PGC_LSB		0x0009
#define IMX183_REG_PGC_MSB	0x000A
#define IMX183_REG_SHR_LSB		0x000B
#define IMX183_REG_SHR_MSB	0x000C
#define IMX183_REG_SVR_LSB		0x000D
#define IMX183_REG_SVR_MSB	0x000E
#define IMX183_REG_SPL_LSB		0x000F
#define IMX183_REG_SPL_MSB		0x0010
#define IMX183_REG_DGAIN		0x0011

#define IMX183_REG_MDVREV		0x001A

#define IMX183_REG_BLKLEVEL	0x0045

#define IMX183_VIDEO_FORMAT_REG_NUM		(5)
#define IMX183_VIDEO_FORMAT_REG_TABLE_SIZE	(5)
#define IMX183_VIDEO_PLL_REG_TABLE_SIZE		(0)
/* ========================================================================== */
struct imx183_reg_table {
	u16 reg;
	u8 data;
};

struct imx183_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx183_reg_table regs[IMX183_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx183_video_format_reg_table {
	u16 reg[IMX183_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[IMX183_VIDEO_FORMAT_REG_NUM];
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
		u16 xhs_clk;
	} table[IMX183_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx183_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx183_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX183_PRI_H__ */

