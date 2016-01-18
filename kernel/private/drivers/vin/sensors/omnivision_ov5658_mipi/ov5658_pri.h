/*
 * Filename : ov5658_pri.h
 *
 * History:
 *    2013/08/28 - [Johnson Diao] Create
 *
 * Copyright (C) 2013-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __OV5658_PRI_H__
#define __OV5658_PRI_H__
// TODO cddiao need to check all of the sensor
/* 		register name 				address default value	R/W	description  */

#define ov5658_mode_select		0x0100

#define ov5658_soft_reset 		0x0103


#define OV5658_CHIP_ID_H			0x300A	/*- R Chip ID High Byte*/
#define OV5658_CHIP_ID_L			0x300B	/*- R Chip ID Low Byte*/

#define OV5658_SRM_GRUP_ACCESS			0x3208 /* 0x00 RW */
							/*Bit[7]:group_hold*/
							/*Bit[6]:group_access_tm*/
							/*Bit[5]:group_launch*/
							/*Bit[4]:group_hold_end*/
							/*Bit[3:0]: group_id, 0~3 (groups for register access)*/

#define OV5658_LONG_EXPO_H			0x3500	/*0x00 RW Bit[7:4]: Not used Bit[3:0]: long_exposure[19:16]*/
#define OV5658_LONG_EXPO_M			0x3501	/*0x00 RW Bit[7:0]: long_exposure[15:8]*/
#define OV5658_LONG_EXPO_L			0x3502	/*0x02 RW Bit[7:0]: long_exposure[7:0]*/

#define OV5658_AGC_ADJ_H			0x350A	/*0x00 RW Bit[7:1]: Debug mode. Bit[0]:	Gain high bit Gain = (0x350B[6]+1) ¡Á (0x350B[5]+1) ¡Á (0x350B[4]+1) ¡Á (0x350B[3:0]/16+1)*/
#define OV5658_AGC_ADJ_L			0x350B	/*0x00 RW Bit[7:0]: Gain low bits Gain = (0x350B[6]+1) ¡Á (0x350B[5]+1) ¡Á (0x350B[4]+1) ¡Á (0x350B[3:0]/16+1)*/

#define OV5658_TIMING_HTS_H			0x380C	/*0x0C RW Bit[7:5]: Debug mode Bit[4:0]: timing_hts[12:8]*/
#define OV5658_TIMING_HTS_L			0x380D	/*0x2C RW Total Horizontal Size Bit[7:0]: timing_hts[7:0]*/
#define OV5658_TIMING_VTS_H			0x380E	/*0x07 RW	Total Vertical Size Bit[3:0]: timing_vts[11:8]*/
#define OV5658_TIMING_VTS_L			0x380F	/*0xB0 RW	Total Vertical Size Bit[7:0]: timing_vts[7:0]*/


#define OV5658_VIDEO_FPS_REG_NUM		(0)
#define OV5658_VIDEO_FORMAT_REG_NUM		(28)
#define OV5658_VIDEO_FORMAT_REG_TABLE_SIZE	(2)
#define OV5658_VIDEO_PLL_REG_TABLE_SIZE		(4)
/** OV5658 mirror mode*/
#define OV5658_H_MIRROR_REG 0x3821
#define OV5658_V_FLIP_REG	0x3820
#define MIRROR_FLIP_ENABLE 0x6

/* ========================================================================== */
struct ov5658_reg_table {
	u16 reg;
	u8 data;
};

struct ov5658_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct ov5658_reg_table regs[OV5658_VIDEO_PLL_REG_TABLE_SIZE];
};

struct ov5658_video_fps_reg_table {
	u16 reg[OV5658_VIDEO_FPS_REG_NUM];
	struct {
		const struct ov5658_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 vfr;
		u8 system;
		u16 data[OV5658_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct ov5658_video_format_reg_table {
	u16 reg[OV5658_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[OV5658_VIDEO_FORMAT_REG_NUM];
		u8 downsample;
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
		} table[OV5658_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct ov5658_video_info {
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

struct ov5658_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __OV5658_PRI_H__ */

