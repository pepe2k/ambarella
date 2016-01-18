/*
 * Filename : mt9t002_pri.h
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
#ifndef __MT9T002_PRI_H__
#define __MT9T002_PRI_H__

#define MT9T002_VIDEO_FPS_REG_NUM			(2)
#define MT9T002_VIDEO_FORMAT_REG_NUM			(13)
#define MT9T002_VIDEO_FORMAT_REG_TABLE_SIZE		(3)
#define MT9T002_VIDEO_PLL_REG_TABLE_SIZE		(7)

/** MT9T002 mirror mode*/
#define MT9T002_MIRROR_ROW			(0x01 << 14)
#define MT9T002_MIRROR_COLUMN 			(0x01 << 15)
#define MT9T002_MIRROR_MASK			(MT9T002_MIRROR_ROW + MT9T002_MIRROR_COLUMN)
/* ========================================================================== */
struct mt9t002_reg_table {
	u16 reg;
	u16 data;
};

struct mt9t002_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct mt9t002_reg_table regs[MT9T002_VIDEO_PLL_REG_TABLE_SIZE];
};

struct mt9t002_video_fps_reg_table {
	u16 reg[MT9T002_VIDEO_FPS_REG_NUM];
	struct {
		const struct mt9t002_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[MT9T002_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct mt9t002_video_format_reg_table {
	u16 reg[MT9T002_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[MT9T002_VIDEO_FORMAT_REG_NUM];
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
		} table[MT9T002_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct mt9t002_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 type_ext;
	u8 bayer_pattern;
};

struct mt9t002_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __MT9T002_PRI_H__ */

