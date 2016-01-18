/*
 * Filename : mt9p006_pri.h
 *
 * History:
 *    2011/05/23 - [Haowei Lo] Create
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
#ifndef __MT9P006_PRI_H__
#define __MT9P006_PRI_H__

#define MT9P006_VIDEO_FPS_REG_NUM			(0)
#define MT9P006_VIDEO_FORMAT_REG_NUM			(31)
#define MT9P006_VIDEO_FORMAT_REG_TABLE_SIZE		(5)
#define MT9P006_VIDEO_PLL_REG_TABLE_SIZE		(4)

/*      Name				Reg#	Description (16-bit content)*/
#define	MT9P006_CHIP_ID			0x00	/**< Chip ID, read only */
#define	MT9P006_ROW_START		0x01	/**< Row Start, even number */
#define	MT9P006_COL_START		0x02	/**< Column Start, even number */
#define	MT9P006_ROW_SIZE		0x03	/**< Row Size=(# of rows-1) */
#define	MT9P006_COL_SIZE		0x04	/**< Column Size=(# of cols-1)*/
#define	MT9P006_HORI_BLANKING		0x05	/**< Horizontal blank >=0x15 */
#define	MT9P006_VERT_BLANKING		0x06	/**< Vertical Blank >= 0x3 */
#define	MT9P006_OUTPUT_CTRL		0x07	/**< Output Control */
#define	MT9P006_SHR_WIDTH_UPPER		0x08	/**< Shutter width MSB byte)*/
#define	MT9P006_SHR_WIDTH		0x09	/**< Shutter Width >=0x1*/
#define	MT9P006_PCLK_CTRL		0x0a	/**< Pixel Clock Control */
#define	MT9P006_RESTART			0x0b	/**< Frame Restart */
#define	MT9P006_SHR_DELAY		0x0c	/**< Shutter Delay */
#define	MT9P006_RESET			0x0d	/**< Reset */
#define	MT9P006_REG_0F			0x0F	/**< Register 0x0F */
#define MT9P006_PLL_CTRL                0x10    /**< PLL control */
#define MT9P006_PLL_CONFIG1             0x11    /**< PLL config 1 */
#define MT9P006_PLL_CONFIG2             0x12    /**< PLL config 2 */
#define	MT9P006_REG_14			0x14	/**< Register 0x14 */
#define	MT9P006_REG_15			0x15	/**< Register 0x15 */
#define	MT9P006_READ_MODE_1		0x1e	/**< Read Mode 1 */
#define	MT9P006_READ_MODE_2		0x20	/**< Read Mode 2 */
#define	MT9P006_ROW_ADDR_MODE		0x22	/**< Raw Address Mode */
#define	MT9P006_COL_ADDR_MODE		0x23	/**< Column Address Mode */
#define MT9P006_REG_24			0x24	/**< Register 0x24 */
#define MT9P006_REG_27			0x27	/**< Register 0x27 */
#define MT9P006_REG_29			0x29	/**< Register 0x29 */
#define MT9P006_REG_2a			0x2a	/**< Register 0x2a */
#define	MT9P006_GREEN1_GAIN	 	0x2b	/**< Green1 Gain */
#define	MT9P006_BLUE_GAIN	 	0x2c	/**< Blue Gain */
#define	MT9P006_RED_GAIN	 	0x2d	/**< Red Gain */
#define	MT9P006_GREEN2_GAIN	 	0x2e	/**< Green2 Gain */
#define MT9P006_REG_30			0x30	/**< Must > 0 */
#define MT9P006_REG_32			0x30	/**< Register 0x32 */
#define	MT9P006_GLOBAL_GAIN	 	0x35	/**< Global Gain */
#define MT9P006_REG_3C			0x3C	/**< Register 0x3C */
#define MT9P006_REG_3D			0x3D	/**< Register 0x3D */
#define MT9P006_REG_3E			0x3E	/**< Register 0x3E */
#define MT9P006_REG_3F			0x3F	/**< Register 0x3F */
#define MT9P006_REG_40			0x40	/**< Register 0x40 */
#define MT9P006_REG_41			0x41	/**< Register 0x41 */
#define MT9P006_REG_42			0x42	/**< Register 0x42 */
#define MT9P006_REG_43			0x43	/**< Register 0x43 */
#define MT9P006_REG_44			0x44	/**< Register 0x44 */
#define MT9P006_REG_45			0x45	/**< Register 0x45 */
#define MT9P006_REG_46			0x46	/**< Register 0x46 */
#define MT9P006_REG_47			0x47	/**< Register 0x47 */
#define MT9P006_REG_48			0x48	/**< Register 0x48 */
#define	MT9P006_ROW_BLACK_TARGET	0x49	/**< Row black target */
#define MT9P006_REG_4A			0x4A	/**< Register 0x4A */
#define	MT9P006_ROW_BLACK_DEF_OFFSET	0x4b	/**< Row black default offset*/
#define MT9P006_REG_4C			0x4C	/**< Register 0x4C */
#define MT9P006_REG_4D			0x4D	/**< Register 0x4D */
#define MT9P006_REG_4E			0x4E	/**< Register 0x4E */
#define MT9P006_REG_4F			0x4F	/**< Register 0x4F */
#define MT9P006_REG_50			0x50	/**< Register 0x50 */
#define MT9P006_REG_51			0x51	/**< Register 0x51 */
#define MT9P006_REG_52			0x52	/**< Register 0x52 */
#define MT9P006_REG_53			0x53	/**< Register 0x53 */
#define MT9P006_REG_54			0x54	/**< Register 0x54 */
#define MT9P006_REG_56			0x56	/**< Register 0x56 */
#define MT9P006_REG_57			0x57	/**< Register 0x57 */
#define MT9P006_REG_58			0x58	/**< Register 0x58 */
#define MT9P006_REG_59			0x59	/**< Register 0x59 */
#define MT9P006_REG_5A			0x5A	/**< Register 0x5A */
#define MT9P006_BLC_SAMPLE_SIZE         0x5b    /**< BLC sample size */
#define MT9P006_BLC_TUNE1               0x5c    /**< BLC tune 1 */
#define MT9P006_BLC_DELTA_THR           0x5d    /**< BLC delta thresholds */
#define MT9P006_BLC_TUNE2               0x5e    /**< BLC tune 2 */
#define MT9P006_BLC_TARGET_THR          0x5f    /**< BLC target thresholds */
#define	MT9P006_GREEN1_OFFSET		0x60	/**< Green1 offset */
#define	MT9P006_GREEN2_OFFSET		0x61	/**< Green2 offset */
#define	MT9P006_BLACK_LEVEL_CAL		0x62	/**< Black level calibration */
#define	MT9P006_RED_OFFSET		0x63	/**< Red offset */
#define	MT9P006_BLUE_OFFSET		0x64	/**< Blue offset */
#define MT9P006_REG_65			0x65	/**< Register 0x65 */
#define MT9P006_REG_68			0x68	/**< Register 0x68 */
#define MT9P006_REG_69			0x69	/**< Register 0x69 */
#define MT9P006_REG_6A			0x6A	/**< Register 0x6A */
#define MT9P006_REG_6B			0x6B	/**< Register 0x6B */
#define MT9P006_REG_6C			0x6C	/**< Register 0x6C */
#define MT9P006_REG_6D			0x6D	/**< Register 0x6D */
#define MT9P006_REG_70			0x70	/**< Register 0x70 */
#define MT9P006_REG_71			0x71	/**< Register 0x71 */
#define MT9P006_REG_72			0x72	/**< Register 0x72 */
#define MT9P006_REG_73			0x73	/**< Register 0x73 */
#define MT9P006_REG_74			0x74	/**< Register 0x74 */
#define MT9P006_REG_75			0x75	/**< Register 0x75 */
#define MT9P006_REG_76			0x76	/**< Register 0x76 */
#define MT9P006_REG_77			0x77	/**< Register 0x77 */
#define MT9P006_REG_78			0x78	/**< Register 0x78 */
#define MT9P006_REG_79			0x79	/**< Register 0x79 */
#define MT9P006_REG_7A			0x7a	/**< Register 0x7a */
#define MT9P006_REG_7B			0x7b	/**< Register 0x7b */
#define MT9P006_REG_7C			0x7c	/**< Register 0x7c */
#define MT9P006_REG_7D			0x7d	/**< Register 0x7d */
#define MT9P006_REG_7E			0x7e	/**< Register 0x7e */
#define MT9P006_REG_7F			0x7f	/**< Register 0x7f */
#define MT9P006_REG_80			0x80	/**< Register 0x80 */
#define MT9P006_REG_F9			0xF9	/**< Register 0xF9 */
#define MT9P006_REG_FD			0xFD	/**< Register 0xFD */
#define	MT9P006_CHIP_VER_ALT		0xff	/**< Chip version alt */

#define MT9P006_READMODE2_REG_MIRROR_ROW		(0x01 << 15)
#define MT9P006_READMODE2_REG_MIRROR_COLUMN 		(0x01 << 14)
#define MT9P006_MIRROR_MASK    (MT9P006_READMODE2_REG_MIRROR_ROW + MT9P006_READMODE2_REG_MIRROR_COLUMN)

/* ========================================================================== */
struct mt9p006_reg_table {
	u16 reg;
	u16 data;
};

struct mt9p006_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct mt9p006_reg_table regs[MT9P006_VIDEO_PLL_REG_TABLE_SIZE];
};

struct mt9p006_video_fps_reg_table {
	u16 reg[MT9P006_VIDEO_FPS_REG_NUM];
	struct {
		const struct mt9p006_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[MT9P006_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct mt9p006_video_format_reg_table {
	u16 reg[MT9P006_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u16 data[MT9P006_VIDEO_FORMAT_REG_NUM];
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
		u8  downsample;
		} table[MT9P006_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct mt9p006_video_info {
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

struct mt9p006_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
#endif /* __MT9P006_PRI_H__ */

