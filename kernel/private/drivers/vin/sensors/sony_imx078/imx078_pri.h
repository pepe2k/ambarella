/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/imx078_pri.h
 *
 * History:
 *    2013/03/11 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __imx078_PRI_H__
#define __imx078_PRI_H__


#define imx078_REG_00         	0x0000
#define imx078_REG_01      	    0x0001
#define imx078_REG_SVR_LSB    	0x0002
#define imx078_REG_SVR_MSB    	0x0003
#define imx078_REG_SPL_LSB    	0x0004
#define imx078_REG_SPL_MSB    	0x0005
#define imx078_REG_SHR_LSB    	0x0006
#define imx078_REG_SHR_MSB    	0x0007
#define imx078_REG_PGC_LSB    	0x0008
#define imx078_REG_PGC_MSB    	0x0009
#define imx078_REG_0A         	0x000A
#define imx078_REG_0D         	0x000D
#define imx078_REG_0E         	0x000E
#define imx078_REG_BLKLEVEL    	0x000F
#define imx078_REG_DGAIN      	0x0010
#define imx078_REG_17      	0x0017
#define imx078_REG_FREQ        	0x0018
#define imx078_REG_ADBIT       	0x0019
#define imx078_REG_4C        	  0x004C
#define imx078_REG_4D         	0x004D
#define imx078_REG_DCKRST     	0x005C
#define imx078_REG_69     	0x0069
#define imx078_REG_330        	0x0330



#define imx078_VIDEO_FPS_REG_NUM		(2)
#define imx078_VIDEO_FORMAT_REG_NUM		(8)
#define imx078_VIDEO_FORMAT_REG_TABLE_SIZE	(1)
#define imx078_VIDEO_PLL_REG_TABLE_SIZE		(0)

/* ========================================================================== */
struct imx078_reg_table {
	u16 reg;
	u8 data;
};

struct imx078_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx078_reg_table regs[imx078_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx078_video_fps_reg_table {
	u16 reg[imx078_VIDEO_FPS_REG_NUM];
	struct {
		const struct imx078_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[imx078_VIDEO_FPS_REG_NUM];
	} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct imx078_video_format_reg_table {
	u16 reg[imx078_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[imx078_VIDEO_FORMAT_REG_NUM];
		const struct imx078_video_fps_reg_table *fps_table;
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
	} table[imx078_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx078_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
	u16 slvs_eav_col;
	u16 slvs_sav2sav_dist;
};

struct imx078_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __imx078_PRI_H__ */

