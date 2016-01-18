/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104_pri.h
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX104_PRI_H__
#define __IMX104_PRI_H__


#define IMX104_STANDBY   			0x0200
#define IMX104_REGHOLD   			0x0201
#define IMX104_XMSTA    			0x0202
#define IMX104_SWRESET   			0x0203

#define IMX104_ADBIT      		0x0205
#define IMX104_MODE       		0x0206
#define IMX104_WINMODE    		0x0207

#define IMX104_FRSEL      		0x0209
#define IMX104_BLKLEVEL_LSB		0x020A
#define IMX104_BLKLEVEL_MSB   0x020B

#define IMX104_LPMODE    		0x0211

#define IMX104_GAIN       		0x0214

#define IMX104_VMAX_LSB    		0x0218
#define IMX104_VMAX_MSB    		0x0219
#define IMX104_VMAX_HSB   		0x021A
#define IMX104_HMAX_LSB    		0x021B
#define IMX104_HMAX_MSB   		0x021C

#define IMX104_SHS1_LSB    		0x0220
#define IMX104_SHS1_MSB   		0x0221
#define IMX104_SHS1_HSB   		0x0222

#define IMX104_SHS2_LSB    		0x0223
#define IMX104_SHS2_MSB   		0x0224
#define IMX104_SHS2_HSB   		0x0225

#define IMX104_WINWV_OB  			0x0236

#define IMX104_WINPV_LSB 			0x0238
#define IMX104_WINPV_MSB 			0x0239
#define IMX104_WINWV_LSB 			0x023A
#define IMX104_WINWV_MSB 			0x023B
#define IMX104_WINPH_LSB 			0x023C
#define IMX104_WINPH_MSB 			0x023D
#define IMX104_WINWH_LSB 			0x023E
#define IMX104_WINWH_MSB 			0x023F

#define IMX104_OUTCTRL   			0x0244

#define IMX104_XVSLNG   			0x0246
#define IMX104_XHSLNG   			0x0247

#define IMX104_XVHSOUT_SEL		0x0249

#define IMX104_INCKSEL1   		0x025B
#define IMX104_INCKSEL2   		0x025D
#define IMX104_INCKSEL3   		0x025F

#define IMX104_VIDEO_FPS_REG_NUM		(2)
#define IMX104_VIDEO_FORMAT_REG_NUM		(15)
#define IMX104_VIDEO_FORMAT_REG_TABLE_SIZE	(3)
#define IMX104_VIDEO_PLL_REG_TABLE_SIZE		(0)

#define IMX104_V_FLIP	(1<<0)
#define IMX104_H_MIRROR	(1<<1)

/* ========================================================================== */
struct imx104_reg_table {
	u16 reg;
	u8 data;
};

struct imx104_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx104_reg_table regs[IMX104_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx104_video_fps_reg_table {
	u16 reg[IMX104_VIDEO_FPS_REG_NUM];
	struct {
	const struct imx104_pll_reg_table *pll_reg_table;
	u32 fps;
	u8 system;
	u16 data[IMX104_VIDEO_FPS_REG_NUM];
	} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct imx104_video_format_reg_table {
	u16 reg[IMX104_VIDEO_FORMAT_REG_NUM];
	struct {
	void (*ext_reg_fill)(struct __amba_vin_source *src);
	u8 data[IMX104_VIDEO_FORMAT_REG_NUM];
	const struct imx104_video_fps_reg_table *fps_table;
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
	} table[IMX104_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx104_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 slvs_eav_col;
	u16 slvs_sav2sav_dist;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx104_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX104_PRI_H__ */

