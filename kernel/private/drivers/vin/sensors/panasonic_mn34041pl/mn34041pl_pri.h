/*
 * Filename : mn34041pl_pri.h
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __MN34041PL_PRI_H__
#define __MN34041PL_PRI_H__

#define MN34041PL_VIDEO_FPS_REG_NUM			(8)
#define MN34041PL_VIDEO_FORMAT_REG_NUM			(0)
#define MN34041PL_VIDEO_FORMAT_REG_TABLE_SIZE		(2)
#define MN34041PL_VIDEO_PLL_REG_TABLE_SIZE		(6)

#define MN34041PL_MIRROR_FLIP (1<<15)

/* ========================================================================== */
struct mn34041pl_reg_table {
	u16 reg;
	u16 data;
};

struct mn34041pl_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct mn34041pl_reg_table regs[MN34041PL_VIDEO_PLL_REG_TABLE_SIZE];
};

struct mn34041pl_video_fps_reg_table {
	u16 reg[MN34041PL_VIDEO_FPS_REG_NUM];
	struct {
		const struct mn34041pl_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[MN34041PL_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct mn34041pl_video_format_reg_table {
	u16 reg[MN34041PL_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[MN34041PL_VIDEO_FORMAT_REG_NUM];
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
		} table[MN34041PL_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct mn34041pl_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 slvs_eav_col;
	u16 slvs_sav2sav_dist;
	u16 sync_start;
	u8 type_ext;
	u8 bayer_pattern;
};

struct mn34041pl_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __MN34041PL_PRI_H__ */

