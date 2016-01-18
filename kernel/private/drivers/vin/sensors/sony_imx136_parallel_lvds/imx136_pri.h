/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_pri.h
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

#ifndef __IMX136_PRI_H__
#define __IMX136_PRI_H__


#define IMX136_STANDBY   			0x0200
#define IMX136_REGHOLD   			0x0201
#define IMX136_XMSTA    			0x0202
#define IMX136_SWRESET   			0x0203

#define IMX136_ADBIT      		0x0205
#define IMX136_MODE       		0x0206
#define IMX136_WINMODE    		0x0207

#define IMX136_FRSEL      		0x0209
#define IMX136_BLKLEVEL_LSB		0x020A
#define IMX136_BLKLEVEL_MSB   0x020B
#define IMX136_WDMODE			0x020C

#define IMX136_GAIN_LSB    		0x0214
#define IMX136_GAIN_MSB    		0x0215

#define IMX136_VMAX_LSB    		0x0218
#define IMX136_VMAX_MSB    		0x0219
#define IMX136_VMAX_HSB   		0x021A
#define IMX136_HMAX_LSB    		0x021B
#define IMX136_HMAX_MSB   		0x021C

#define IMX136_SHS1_LSB    		0x0220
#define IMX136_SHS1_MSB   		0x0221
#define IMX136_SHS1_HSB   		0x0222

#define IMX136_SHS2_LSB    		0x0223
#define IMX136_SHS2_MSB   		0x0224
#define IMX136_SHS2_HSB   		0x0225

#define IMX136_SHS3_LSB    		0x0226
#define IMX136_SHS3_MSB   		0x0227
#define IMX136_SHS3_HSB   		0x0228

#define IMX136_SHS4_LSB    		0x0229
#define IMX136_SHS4_MSB   		0x022A
#define IMX136_SHS4_HSB   		0x022B

#define IMX136_WINWV_OB  			0x0236

#define IMX136_WINPV_LSB 			0x0238
#define IMX136_WINPV_MSB 			0x0239
#define IMX136_WINWV_LSB 			0x023A
#define IMX136_WINWV_MSB 			0x023B
#define IMX136_WINPH_LSB 			0x023C
#define IMX136_WINPH_MSB 			0x023D
#define IMX136_WINWH_LSB 			0x023E
#define IMX136_WINWH_MSB 			0x023F

#define IMX136_OUTCTRL   			0x0244

#define IMX136_XVSLNG   			0x0246
#define IMX136_XHSLNG   			0x0247

#define IMX136_XVHSOUT_SEL		0x0249

#define IMX136_INCKSEL0   		0x025B
#define IMX136_INCKSEL1   		0x025C
#define IMX136_INCKSEL2   		0x025D
#define IMX136_INCKSEL3   		0x025E
#define IMX136_INCKSEL4   		0x025F

#define IMX136_REG65			0x0265
#define IMX136_VRSET			0x0284
#define IMX136_REG86			0x0286

#define IMX136_VIDEO_FPS_REG_NUM		(2)
#define IMX136_VIDEO_FORMAT_REG_NUM		(23)
#define IMX136_VIDEO_FORMAT_REG_TABLE_SIZE	(5)
#define IMX136_VIDEO_PLL_REG_TABLE_SIZE		(0)
#define IMX136_WDR_MODE_TABLE_SIZE		(6)
#define IMX136_WDR_MODE_REG_NUM			(16)

#define IMX136_NEW_WDR_2FRAME_16X_MODE (1)
#define IMX136_NEW_WDR_2FRAME_32X_MODE (2)
#define IMX136_NEW_WDR_4FRAME_MODE (3)
#define IMX136_NEW_WDR_2FRAME_CT_16X_MODE (4)
#define IMX136_NEW_WDR_2FRAME_CT_32X_MODE (5)

#define IMX136_V_FLIP	(1<<0)
#define IMX136_H_MIRROR	(1<<1)

/* ========================================================================== */
struct imx136_reg_table {
	u16 reg;
	u8 data;
};

struct imx136_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct imx136_reg_table regs[IMX136_VIDEO_PLL_REG_TABLE_SIZE];
};

struct imx136_video_fps_reg_table {
	u16 reg[IMX136_VIDEO_FPS_REG_NUM];
	struct {
	const struct imx136_pll_reg_table *pll_reg_table;
	u32 fps;
	u8 system;
	u16 data[IMX136_VIDEO_FPS_REG_NUM];
	} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct imx136_video_format_reg_table {
	u16 reg[IMX136_VIDEO_FORMAT_REG_NUM];
	struct {
	void (*ext_reg_fill)(struct __amba_vin_source *src);
	u8 data[IMX136_VIDEO_FORMAT_REG_NUM];
	const struct imx136_video_fps_reg_table *fps_table;
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
	} table[IMX136_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct imx136_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 bayer_pattern;
};

struct imx136_wdr_mode_table {
	u16 reg[IMX136_WDR_MODE_REG_NUM];
	struct {
	u8 data[IMX136_WDR_MODE_REG_NUM];
	} table[IMX136_WDR_MODE_TABLE_SIZE];
};

struct imx136_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};

#endif /* __IMX136_PRI_H__ */

