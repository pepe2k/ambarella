/*
 * kernel/private/drivers/ambarella/vin/sensors/omnivision_ov4689/ov4689_pri.h
 *
 * History:
 *    2013/05/29 - [Long Zhao] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __OV4689_PRI_H__
#define __OV4689_PRI_H__

#define OV4689_STANDBY			0x0100
#define OV4689_IMG_ORI			0x0101
#define OV4689_SWRESET			0x0103

#define OV4689_GRP_ACCESS		0x3208

#define OV4689_HTS_MSB			0x380C
#define OV4689_HTS_LSB			0x380D
#define OV4689_VTS_MSB			0x380E
#define OV4689_VTS_LSB			0x380F

#define OV4689_X_START			0x0345
#define OV4689_Y_START			0x0347

#define OV4689_L_EXPO_HSB	0x3500
#define OV4689_L_EXPO_MSB	0x3501
#define OV4689_L_EXPO_LSB	0x3502

#define OV4689_L_GAIN_HSB	0x3507
#define OV4689_L_GAIN_MSB	0x3508
#define OV4689_L_GAIN_LSB	0x3509

#define OV4689_M_EXPO_HSB		0x350A
#define OV4689_M_EXPO_MSB		0x350B
#define OV4689_M_EXPO_LSB		0x350C

#define OV4689_M_GAIN_HSB		0x350D
#define OV4689_M_GAIN_MSB		0x350E
#define OV4689_M_GAIN_LSB		0x350F

#define OV4689_S_EXPO_HSB		0x3510
#define OV4689_S_EXPO_MSB		0x3511
#define OV4689_S_EXPO_LSB		0x3512

#define OV4689_S_GAIN_HSB		0x3513
#define OV4689_S_GAIN_MSB		0x3514
#define OV4689_S_GAIN_LSB		0x3515

#define OV4689_M_MAX_EXPO_MSB	0x3760
#define OV4689_M_MAX_EXPO_LSB		0x3761

#define OV4689_S_MAX_EXPO_MSB	0x3762
#define OV4689_S_MAX_EXPO_LSB		0x3763

#define OV4689_L_WB_R_GAIN_MSB	0x500C
#define OV4689_L_WB_R_GAIN_LSB	0x500D

#define OV4689_L_WB_G_GAIN_MSB	0x500E
#define OV4689_L_WB_G_GAIN_LSB	0x500F

#define OV4689_L_WB_B_GAIN_MSB	0x5010
#define OV4689_L_WB_B_GAIN_LSB	0x5011

#define OV4689_M_WB_R_GAIN_MSB	0x5012
#define OV4689_M_WB_R_GAIN_LSB	0x5013

#define OV4689_M_WB_G_GAIN_MSB	0x5014
#define OV4689_M_WB_G_GAIN_LSB	0x5015

#define OV4689_M_WB_B_GAIN_MSB	0x5016
#define OV4689_M_WB_B_GAIN_LSB	0x5017

#define OV4689_S_WB_R_GAIN_MSB	0x5018
#define OV4689_S_WB_R_GAIN_LSB	0x5019

#define OV4689_S_WB_G_GAIN_MSB	0x501A
#define OV4689_S_WB_G_GAIN_LSB	0x501B

#define OV4689_S_WB_B_GAIN_MSB	0x501C
#define OV4689_S_WB_B_GAIN_LSB	0x501D

#define OV4689_V_FORMAT		0x3820
#define OV4689_H_FORMAT		0x3821

#define OV4689_VIDEO_FORMAT_REG_NUM			(0)
#define OV4689_VIDEO_FORMAT_REG_TABLE_SIZE	(6)
#define OV4689_VIDEO_PLL_REG_TABLE_SIZE		(0)


#define OV4689_V_FLIP	(0x6)
#define OV4689_H_MIRROR	(0x6)

#define OV4689_LINEAR_MODE (0)
#define OV4689_2X_WDR_MODE (1)
#define OV4689_3X_WDR_MODE (2)
#define OV4689_4X_WDR_MODE (3)
/* ========================================================================== */
struct ov4689_reg_table {
	u16 reg;
	u16 data;
};

struct ov4689_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct ov4689_reg_table regs[OV4689_VIDEO_PLL_REG_TABLE_SIZE];
};

struct ov4689_video_format_reg_table {
	u16 reg[OV4689_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[OV4689_VIDEO_FORMAT_REG_NUM];
		const struct ov4689_video_fps_reg_table *fps_table;
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
		struct amba_vin_wdr_win_offset hdr_win_offset;
	} table[OV4689_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct ov4689_video_info {
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

struct ov4689_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __OV4689_PRI_H__ */
