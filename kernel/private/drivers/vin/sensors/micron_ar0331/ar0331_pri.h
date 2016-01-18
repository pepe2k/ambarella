/*
 * Filename : ar0331_pri.h
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
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
#ifndef __AR0331_PRI_H__
#define __AR0331_PRI_H__

#define AR0331_VIDEO_FPS_REG_NUM			(1)
#define AR0331_VIDEO_FORMAT_REG_NUM			(12)
#define AR0331_VIDEO_FORMAT_REG_TABLE_SIZE		(3)
#define AR0331_VIDEO_PLL_REG_TABLE_SIZE			(6)

#define AR0331_16BIT_HDR_MODE			(2)

/** AR0331 mirror mode*/
#define AR0331_MIRROR_ROW			(0x01 << 14)
#define AR0331_MIRROR_COLUMN 			(0x01 << 15)
#define AR0331_MIRROR_MASK			(AR0331_MIRROR_ROW + AR0331_MIRROR_COLUMN)

/* ========================================================================== */
struct ar0331_reg_table {
	u16 reg;
	u16 data;
};

struct ar0331_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct ar0331_reg_table regs[AR0331_VIDEO_PLL_REG_TABLE_SIZE];
};

struct ar0331_video_fps_reg_table {
	u16 reg[AR0331_VIDEO_FPS_REG_NUM];
	struct {
		const struct ar0331_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[AR0331_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct ar0331_video_format_reg_table {
	u16 reg[AR0331_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[AR0331_VIDEO_FORMAT_REG_NUM];
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
		} table[AR0331_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct ar0331_video_info {
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

struct ar0331_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __AR0331_PRI_H__ */

