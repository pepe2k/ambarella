/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx172/imx172_pri.h
 *
 * History:
 *    2012/08/22 - [Cao Rongrong] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX172_PRI_H__
#define __IMX172_PRI_H__


#define IMX172_REG_00		0x0000
#define IMX172_REG_01		0x0001
#define IMX172_REG_02		0x0002
#define IMX172_REG_03		0x0003
#define IMX172_REG_MDSEL1	0x0004
#define IMX172_REG_MDSEL2	0x0005
#define IMX172_REG_MDSEL3	0x0006
#define IMX172_REG_MDSEL4	0x0007
#define IMX172_REG_08		0x0008
#define IMX172_REG_PGC_LSB	0x0009
#define IMX172_REG_PGC_MSB	0x000A
#define IMX172_REG_SHR_LSB	0x000B
#define IMX172_REG_SHR_MSB	0x000C
#define IMX172_REG_SVR_LSB	0x000D
#define IMX172_REG_SVR_MSB	0x000E
#define IMX172_REG_SPL_LSB	0x000F
#define IMX172_REG_SPL_MSB	0x0010
#define IMX172_REG_DGAIN	0x0011
#define IMX172_REG_MDVREV	0x001A
#define IMX172_REG_MDSEL13	0x0026
#define IMX172_REG_MDSEL14	0x0027
#define IMX172_REG_MDSEL15	0x0028
#define IMX172_REG_BLKLEVEL	0x0045
#define IMX172_REG_MDSEL5_LSB	0x007E
#define IMX172_REG_MDSEL5_MSB	0x007F
#define IMX172_REG_MDPLS01	0x0080
#define IMX172_REG_MDPLS02	0x0081
#define IMX172_REG_MDPLS03	0x0082
#define IMX172_REG_MDPLS04	0x0083
#define IMX172_REG_MDPLS05	0x0084
#define IMX172_REG_MDPLS06	0x0085
#define IMX172_REG_MDPLS07	0x0086
#define IMX172_REG_MDPLS08	0x0087
#define IMX172_REG_MDPLS09	0x0095
#define IMX172_REG_MDPLS10	0x0096
#define IMX172_REG_MDPLS11	0x0097
#define IMX172_REG_MDPLS12	0x0098
#define IMX172_REG_MDPLS13	0x0099
#define IMX172_REG_MDPLS14	0x009A
#define IMX172_REG_MDPLS15	0x009B
#define IMX172_REG_MDPLS16	0x009C
#define IMX172_REG_MDSEL6	0x00B6
#define IMX172_REG_MDSEL7	0x00B7
#define IMX172_REG_MDSEL8	0x00B8
#define IMX172_REG_MDSEL9	0x00B9
#define IMX172_REG_MDSEL10	0x00BA
#define IMX172_REG_MDSEL11	0x00BB
#define IMX172_REG_MDPLS17	0x00BC
#define IMX172_REG_MDPLS18	0x00BD
#define IMX172_REG_MDPLS19	0x00BE
#define IMX172_REG_MDPLS20	0x00BF
#define IMX172_REG_MDPLS21	0x00C0
#define IMX172_REG_MDPLS22	0x00C1
#define IMX172_REG_MDPLS23	0x00C2
#define IMX172_REG_MDPLS24	0x00C3
#define IMX172_REG_MDPLS25	0x00C4
#define IMX172_REG_MDPLS26	0x00C5
#define IMX172_REG_MDPLS27	0x00C6
#define IMX172_REG_MDPLS28	0x00C7
#define IMX172_REG_MDPLS29	0x00C8
#define IMX172_REG_MDPLS30	0x00C9
#define IMX172_REG_MDPLS31	0x00CA
#define IMX172_REG_MDPLS32	0x00CB
#define IMX172_REG_MDPLS33	0x00CC
#define IMX172_REG_MDSEL12	0x00CE
#define IMX172_REG_PLSTMG11_LSB	0x0222
#define IMX172_REG_PLSTMG11_MSB	0x0223
#define IMX172_REG_APGC01_LSB		0x0352
#define IMX172_REG_APGC01_MSB		0x0353
#define IMX172_REG_APGC02_LSB		0x0356
#define IMX172_REG_APGC02_MSB		0x0357
#define IMX172_REG_PLSTMG00		0x0358
#define IMX172_REG_PLSTMG01		0x0528
#define IMX172_REG_PLSTMG13		0x0529
#define IMX172_REG_PLSTMG02		0x052A
#define IMX172_REG_PLSTMG14		0x052B
#define IMX172_REG_PLSTMG15		0x0534
#define IMX172_REG_PLSTMG03		0x057E
#define IMX172_REG_PLSTMG04		0x057F
#define IMX172_REG_PLSTMG05		0x0580
#define IMX172_REG_PLSTMG06		0x0581
#define IMX172_REG_PLSTMG07_LSB	0x0585
#define IMX172_REG_PLSTMG07_MSB	0x0586
#define IMX172_REG_PLSTMG12		0x0617
#define IMX172_REG_PLSTMG08		0x065C
#define IMX172_REG_PLSTMG09_LSB	0x0700
#define IMX172_REG_PLSTMG09_MSB	0x0701

#define IMX172_REG_MNL_VADR08	0x0088
#define IMX172_REG_MNL_VADR09	0x0089
#define IMX172_REG_MNL_VADR14	0x008E
#define IMX172_REG_MNL_VADR15	0x008F
#define IMX172_REG_MNL_VADR20	0x0094
#define IMX172_REG_MNL_VADR29	0x009D
#define IMX172_REG_MNL_VADR30	0x009E
#define IMX172_REG_MNL_VADR35	0x00A3
#define IMX172_REG_MNL_VADR36	0x00A4
#define IMX172_REG_MNL_VADR41	0x00A9
#define IMX172_REG_MNL_VADR42	0x00AA
#define IMX172_REG_MNL_VADR43	0x00AB
#define IMX172_REG_MNL_VADR44	0x00AC
#define IMX172_REG_MNL_VADR45	0x00AD
#define IMX172_REG_MNL_VADR46	0x00AE
#define IMX172_REG_MNL_VADR47	0x00AF
#define IMX172_REG_MNL_VADR48	0x00B0
#define IMX172_REG_MNL_VADR49	0x00B1
#define IMX172_REG_MNL_VADR50	0x00B2
#define IMX172_REG_MNL_VADR51	0x00B3
#define IMX172_REG_MNL_VADR52	0x00B4
#define IMX172_REG_MNL_VADR53	0x00B5
#define IMX172_REG_MNL_VADR83	0x00D3

#define IMX172_REG_0D         	0x000D
#define IMX172_REG_ADBIT       	0x0019




#define IMX172_VIDEO_FORMAT_REG_NUM		(4)
#define IMX172_VIDEO_FORMAT_REG_TABLE_SIZE	(8)
#define IMX172_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define IMX172_SET_SHUTTER         	(1<<0)
#define IMX172_SET_AGC				(1<<1)
#define IMX172_UPDATE_SYNC			(0x10)

#define IMX172_V_FLIP					(1<<0)
/* ========================================================================== */
struct imx172_reg_table {
	u16 reg;
	u8 data;
};

struct imx172_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	u8 factor;
	struct imx172_reg_table regs[IMX172_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx172_video_format_reg_table {
	u16 reg[IMX172_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[IMX172_VIDEO_FORMAT_REG_NUM];
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
	} table[IMX172_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx172_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx172_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX172_PRI_H__ */

