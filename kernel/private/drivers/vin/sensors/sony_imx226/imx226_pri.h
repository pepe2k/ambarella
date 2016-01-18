/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx226/imx226_pri.h
 *
 * History:
 *    2013/12/20 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX226_PRI_H__
#define __IMX226_PRI_H__


#define IMX226_REG_00			0x0000
#define IMX226_REG_01			0x0001
#define IMX226_REG_02			0x0002
#define IMX226_REG_03			0x0003
#define IMX226_REG_MDSEL1		0x0004
#define IMX226_REG_MDSEL2		0x0005
#define IMX226_REG_MDSEL3		0x0006
#define IMX226_REG_MDSEL4		0x0007
#define IMX226_REG_PGC_LSB		0x0009
#define IMX226_REG_PGC_MSB	0x000A
#define IMX226_REG_SHR_LSB		0x000B
#define IMX226_REG_SHR_MSB	0x000C
#define IMX226_REG_SVR_LSB		0x000D
#define IMX226_REG_SVR_MSB	0x000E
#define IMX226_REG_SPL_LSB		0x000F
#define IMX226_REG_SPL_MSB		0x0010
#define IMX226_REG_DGAIN		0x0011
#define IMX226_REG_FREQ		0x0012
#define IMX226_REG_MDVREV		0x001A
#define IMX226_REG_MDSEL13		0x0026
#define IMX226_REG_MDSEL14		0x0027
#define IMX226_REG_MDSEL15		0x0028
#define IMX226_REG_BLKLEVEL	0x0045
#define IMX226_REG_PLSTMG01	0x004D
#define IMX226_REG_PLSTMG02	0x0054
#define IMX226_REG_PLSTMG03	0x0057
#define IMX226_REG_MDSEL5_LSB	0x007E
#define IMX226_REG_MDSEL5_MSB	0x007F
#define IMX226_REG_MDPLS01	0x0080
#define IMX226_REG_MDPLS02	0x0081
#define IMX226_REG_MDPLS03	0x0082
#define IMX226_REG_MDPLS04	0x0083
#define IMX226_REG_MDPLS05	0x0084
#define IMX226_REG_MDPLS06	0x0085
#define IMX226_REG_MDPLS07	0x0086
#define IMX226_REG_MDPLS08	0x0087
#define IMX226_REG_MDPLS09	0x0095
#define IMX226_REG_MDPLS10	0x0096
#define IMX226_REG_MDPLS11	0x0097
#define IMX226_REG_MDPLS12	0x0098
#define IMX226_REG_MDPLS13	0x0099
#define IMX226_REG_MDPLS14	0x009A
#define IMX226_REG_MDPLS15	0x009B
#define IMX226_REG_MDPLS16	0x009C
#define IMX226_REG_MDSEL6		0x00B6
#define IMX226_REG_MDSEL7		0x00B7
#define IMX226_REG_MDSEL8		0x00B8
#define IMX226_REG_MDSEL9		0x00B9
#define IMX226_REG_MDSEL10		0x00BA
#define IMX226_REG_MDSEL11		0x00BB
#define IMX226_REG_MDPLS17	0x00BC
#define IMX226_REG_MDPLS18	0x00BD
#define IMX226_REG_MDPLS19	0x00BE
#define IMX226_REG_MDPLS20	0x00BF
#define IMX226_REG_MDPLS21	0x00C0
#define IMX226_REG_MDPLS22	0x00C1
#define IMX226_REG_MDPLS23	0x00C2
#define IMX226_REG_MDPLS24	0x00C3
#define IMX226_REG_MDPLS25	0x00C4
#define IMX226_REG_MDPLS26	0x00C5
#define IMX226_REG_MDPLS27	0x00C6
#define IMX226_REG_MDPLS28	0x00C7
#define IMX226_REG_MDPLS29	0x00C8
#define IMX226_REG_MDPLS30	0x00C9
#define IMX226_REG_MDPLS31	0x00CA
#define IMX226_REG_MDPLS32	0x00CB
#define IMX226_REG_MDPLS33	0x00CC
#define IMX226_REG_MDSEL12		0x00CE

#define IMX226_REG_PLSTMG04_LSB	0x0210
#define IMX226_REG_PLSTMG04_MSB	0x0211
#define IMX226_REG_PLSTMG05_LSB	0x0212
#define IMX226_REG_PLSTMG05_MSB	0x0213
#define IMX226_REG_PLSTMG22_LSB	0x021C
#define IMX226_REG_PLSTMG22_MSB	0x021D
#define IMX226_REG_PLSTMG06_LSB	0x021E
#define IMX226_REG_PLSTMG06_MSB	0x021F
#define IMX226_REG_PLSTMG23_LSB	0x0222
#define IMX226_REG_PLSTMG23_MSB	0x0223
#define IMX226_REG_PLSTMG07		0x0313
#define IMX226_REG_APGC01_LSB		0x0352
#define IMX226_REG_APGC01_MSB		0x0353
#define IMX226_REG_APGC02_LSB		0x0356
#define IMX226_REG_APGC02_MSB		0x0357
#define IMX226_REG_PLSTMG08		0x0366
#define IMX226_REG_PLSTMG09		0x0371
#define IMX226_REG_PLSTMG10		0x0528
#define IMX226_REG_PLSTMG11		0x0529
#define IMX226_REG_PLSTMG12		0x052C
#define IMX226_REG_PLSTMG13_LSB	0x052D
#define IMX226_REG_PLSTMG13_MSB	0x052E
#define IMX226_REG_PLSTMG24		0x0534
#define IMX226_REG_PLSTMG14_LSB	0x057A
#define IMX226_REG_PLSTMG14_MSB	0x057B
#define IMX226_REG_PLSTMG15		0x057D
#define IMX226_REG_PLSTMG16		0x057E
#define IMX226_REG_PLSTMG17		0x0582
#define IMX226_REG_PLSTMG18		0x0617
#define IMX226_REG_PLSTMG19		0x0650
#define IMX226_REG_PLSTMG20		0x065C
#define IMX226_REG_PLSTMG21_LSB	0x0700
#define IMX226_REG_PLSTMG21_MSB	0x0701

#define IMX226_VIDEO_FORMAT_REG_NUM		(4)
#define IMX226_VIDEO_FORMAT_REG_TABLE_SIZE	(7)
#define IMX226_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define IMX226_SET_SHUTTER         	(1<<0)
#define IMX226_SET_AGC				(1<<1)
#define IMX226_UPDATE_SYNC			(0x10)

#define IMX226_V_FLIP					(1<<0)
/* ========================================================================== */
struct imx226_reg_table {
	u16 reg;
	u8 data;
};

struct imx226_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	u8 factor;
	struct imx226_reg_table regs[IMX226_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx226_video_format_reg_table {
	u16 reg[IMX226_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[IMX226_VIDEO_FORMAT_REG_NUM];
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
		u16 slvs_eav_col;
		u8 h_xhs;
		u8 lane_num;
	} table[IMX226_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx226_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx226_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX226_PRI_H__ */

