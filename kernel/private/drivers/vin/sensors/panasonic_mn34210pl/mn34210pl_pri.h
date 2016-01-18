/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34210pl/mn34210pl_pri.h
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

#ifndef __MN34210PL_PRI_H__
#define __MN34210PL_PRI_H__

#define MN34210PL_VIDEO_FORMAT_REG_NUM			(0)
#define MN34210PL_VIDEO_FORMAT_REG_TABLE_SIZE		(5)
#define MN34210PL_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define MN34210PL_V_FLIP (1<<1)

#define MN34210PL_LINEAR_MODE (0)
#define MN34210PL_2X_WDR_MODE (1)
#define MN34210PL_3X_WDR_MODE (2)
#define MN34210PL_4X_WDR_MODE (3)

/* ========================================================================== */
struct mn34210pl_reg_table {
	u16 reg;
	u16 data;
};

struct mn34210pl_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct mn34210pl_reg_table regs[MN34210PL_VIDEO_PLL_REG_TABLE_SIZE];
};

struct mn34210pl_video_format_reg_table {
	u16 reg[MN34210PL_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[MN34210PL_VIDEO_FORMAT_REG_NUM];
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
	} table[MN34210PL_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct mn34210pl_video_info {
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

struct mn34210pl_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __MN34210PL_PRI_H__ */
