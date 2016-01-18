/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/imx122_pri.h
 *
 * History:
 *    2011/09/23 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX122_PRI_H__
#define __IMX122_PRI_H__

/*      NAME            Address     Bit     Symbol      Description                Default value   Reflection timing   */
#define IMX122_REG00    	0x0200    /*  	bit[0] 	STANDBY       	Standy 0: Operation 1:Standby       1       immediately*/
					/*  	bit[3:1]		b010
						bit[5:4] ZPLTD	0:Other than 1080p HD mode 1:1080p HD mode	0  immediately
						bit[7:6]		b01 */
#define IMX122_REG01    	0x0201    /*  	bit[0] 	Vertical readout inversion control 0:Normal	1:Inversed	0  V
						bit[3:1]	0
						bit[6:4]   0		V*/
#define IMX122_REG02    	0x0202    /*	bit[5:0] MODE[5:0]	Addition readout drive mode
						 00h:All pixel scan
						 0Fh:HD1080p mode
						 other: invalid	V*/
#define IMX122_HMAX_LSB    	0x0203    /*  	bit[7:0] HMAX[7:0]  line length setting   */
#define IMX122_HMAX_MSB    	0x0204   /*  	bit[5:0] HMAX[13:8]  line length setting  */
#define IMX122_VMAX_LSB    	0x0205    /* 	VMAX[7:0]	*/
#define IMX122_VMAX_MSB  	0x0206    /* 	VMAX[15:8]	*/
#define IMX122_REG07 	0x0207    /*  	bit[7:0] 0 */
#define IMX122_SHS1_LSB 	0x0208    /* 	SHS1[7:0]	*/
#define IMX122_SHS1_MSB    	0x0209    /* 	SHS1[15:8]	*/
#define IMX122_REG12    	0x0212    /* 	bit[1] AD gradation setting bits 0:10bit 1:12bit 0	
									bit[0] SSBRK */
#define IMX122_REG0A    	0x020A    /*  	bit[7:0] 0 */
#define IMX122_REG0B    	0x020B    /*  	bit[7:0] 0 */
#define IMX122_REG0C    	0x020C    /*  	bit[7:0] 0 */
#define IMX122_REG0D    	0x020D    /* 	bit[7:0] SPL[7:0]	low speed shutter storage time adjust*/
#define IMX122_REG0E    	0x020E    /* 	bit[1:0] SPL[9:8]	low speed shutter storage time adjust 
										bit[7:2] Fix to "00h"  */
#define IMX122_REG11     	0x0211    /* 	bit[2:0] FRSEL[2:0] output data rate 
									bit[4:3] output system
									bit[7:5] 0 */
#define IMX122_BLC_LSB     	0x0220    /* 	bit[7:0] BLKLEVEL[7:0] black level setting */
#define IMX122_BLC_MSB     	0x0221    /* 	bit[0] BLKLEVEL[8] black level setting 
										bit[5:4] XHSLNG[1:0] 1:6clk 1:12clk 2:22clk 3:128clk*/
#define IMX122_REG_GAIN    	0x021E    /*  	bit[7:0] Gain setting */
#define IMX122_REG16    	0x0216
#define IMX122_REG9A    	0x029A
#define IMX122_REG9B    	0x029B

#define IMX122_WIN_PIX_ST_LSB	0x02A6
#define IMX122_WIN_PIX_ST_MSB	0x02A7
#define IMX122_WIN_PIX_END_LSB	0x02A8
#define IMX122_WIN_PIX_END_MSB	0x02A9
#define IMX122_WIN_OB_END	0x02A5
#define IMX122_WIN_OB_ST	0x02A4

#define IMX122_VIDEO_FORMAT_REG_NUM		(29)
#define IMX122_VIDEO_FORMAT_REG_TABLE_SIZE	(2)
#define IMX122_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define IMX122_V_FLIP	(1<<0)
#define IMX122_H_MIRROR	(1<<1)

/* ========================================================================== */
struct imx122_reg_table {
	u16 reg;
	u8 data;
};

struct imx122_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx122_reg_table regs[IMX122_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx122_video_format_reg_table {
	u16 reg[IMX122_VIDEO_FORMAT_REG_NUM];
	struct {
	void (*ext_reg_fill)(struct __amba_vin_source *src);
	u8 data[IMX122_VIDEO_FORMAT_REG_NUM];
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
	} table[IMX122_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx122_video_info {
	u32 format_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx122_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX122_PRI_H__ */

