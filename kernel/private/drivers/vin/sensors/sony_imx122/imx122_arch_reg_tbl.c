/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/arch_a2/imx122_arch_reg_tbl.c
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

/* ========================================================================== */

/*
 * As imx122 need to feed 0x3XX area with different mysterious binary for 3M and 1080p class,
 * the share regs is only useful for the same 3M class or 1080p class mode, for example:
 * 1920x1080p and 1024x768 share the same imx122_1080p_class_regs,
 */
static const struct imx122_reg_table imx122_3M_class_regs[] = {
	{0x0220, 0x00},
	{0x0225, 0x47}, //TODO: 1080p should be 43 for LVDS but it is 47
	{0x022A, 0x22}, //TODO: 1080p should 0 or by pass, but write 0x22? , 0x0, 0x20, 0x02 all three stall, 0x26 stall for no preview
	{0x022D, 0x08}, //TODO: use 0x08...? slave mode
	{0x025E, 0xFF},
	{0x025F, 0x02},
	{0x0273, 0xB1},
	{0x0274, 0x01},
	{0x0276, 0x60},
	{0x029D, 0x00},
	{0x02D4, 0x00},
	{0x02D6, 0x40},
	{0x02D8, 0x66},
	{0x02D9, 0x0E},
	{0x02DB, 0x06},
	{0x02DC, 0x06},
	{0x02DD, 0x0C},
	{0x02E0, 0x7C},
/*------Section 3, can't be read----------*/
	{0x0301, 0x50},
	{0x0302, 0x1c},
	{0x0304, 0xF0}, // 1080p 0x20 vs 3M 0xf0
	{0x0305, 0x01}, // 1080p 0x00 vs 3M 0x01
	{0x0306, 0x93},
	{0x0307, 0xd0},
	{0x0308, 0x09},
	{0x0309, 0xe3},
	{0x030A, 0xb1},
	{0x030B, 0x1f},
	{0x030C, 0xc5},
	{0x030D, 0x31},
	{0x030E, 0x21},
	{0x030F, 0x2b},
	{0x0310, 0x02}, // 1080p 0x04 vs 3M 0x02
	{0x0311, 0x59},
	{0x0312, 0x15},
	{0x0313, 0x59},
	{0x0314, 0x15},
	{0x0315, 0x71},
	{0x0316, 0x1d},
	{0x0317, 0x59},
	{0x0318, 0x15},
	{0x0319, 0x00},
	{0x031B, 0x00},
	{0x031F, 0x9c},
	{0x0320, 0xf0},
	{0x0321, 0x09},
	{0x0323, 0xb0}, // 1080p 0x60 vs 3M b0
	{0x0324, 0x22}, // 1080P 0x44 vs 3M 0x22
	{0x0325, 0xc5},
	{0x0326, 0x31},
	{0x0327, 0x21},
	{0x0329, 0x30},
	{0x032A, 0x21},
	{0x032B, 0xc5},
	{0x032C, 0x31},
	{0x032D, 0x21},
	{0x032E, 0x2c},
	{0x032F, 0x62},
	{0x0330, 0x1c},
	{0x0332, 0xf0}, // 1080p 0xd0 vs 3M 0xf0
	{0x0335, 0x40}, // 1080p 0x10 vs 3M 0x40
	{0x0337, 0xc5},
	{0x0338, 0x11}, // 1080p 0x13 vs 3M 0x11
	{0x033A, 0xc5},
	{0x033B, 0x11}, // 1080p 0x13 vs 3M 0x11
	{0x033E, 0x90},
	{0x0343, 0x91},
	{0x0344, 0x20},
	{0x0345, 0x09},
	{0x0346, 0x2f},
	{0x034B, 0x35},
	{0x034D, 0x3f},
	{0x034F, 0x3f},
	{0x0351, 0xad},
	{0x0352, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x0353, 0xad},
	{0x0354, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x0357, 0x46},
	{0x0359, 0x12}, // 1080p 0x0f vs 3M 0x12
	{0x035B, 0xc5},
	{0x035C, 0x01}, // 1080p 0x03 vs 3M 0x01
	{0x035D, 0x11}, // 1080p 0x0e vs 3M 0x11
	{0x035F, 0x97},
	{0x1223, 0xc4},
	{0x1224, 0x01}, // 1080p 0x03 vs 3M 0x01
	{0x1229, 0x44},
	{0x122B, 0x3a},
	{0x122D, 0xa0},
	{0x122F, 0x9d},
	{0x0373, 0x3a},
	{0x0374, 0x00},//TODO: co, but the initial value is 0 already
	{0x0379, 0x3c},
	{0x037B, 0x3a},
	{0x037D, 0x9f},
	{0x037F, 0x9d},
	{0x0383, 0x9d},
	{0x0398, 0x9e},
	{0x039A, 0xa7},
	{0x039B, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x039C, 0x07}, // 1080p 0x06 vs 3M 0x07
	{0x039E, 0x11}, // 1080p 0x0e vs 3M 0x11
	{0x03AC, 0x1b}, // 1080p 0x16 vs 3M 0x1b
	{0x03AE, 0x08}, // 1080p 0x07 vs 3M 0x08
	{0x03B0, 0x1b}, // 1080p 0x16 vs 3M 0x1b
	{0x03B2, 0x08}, // 1080p 0x07 vs 3M 0x08
	{0x03B4, 0x11}, // 1080p 0x0e vs 3M 0x11
	{0x03B6, 0xa7},
	{0x03B7, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03B8, 0xa7},
	{0x03B9, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03BA, 0x11}, // 1080p 0x0e vs 3M 0x11
	{0x03BC, 0x11}, // 1080p 0x0e vs 3M 0x11
	{0x03BE, 0xa7},
	{0x03BF, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03C0, 0xa9},
	{0x03C1, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03C2, 0xab},
	{0x03C3, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03C4, 0x16}, // 1080p 0x12 vs 3M 0x16
	{0x03C6, 0x1b}, // 1080p 0x16 vs 3M 0x1b
	{0x03C8, 0x9e},
	{0x03C9, 0xb0},
	{0x03CA, 0x0a}, // 1080p 0x11 vs 3M 0x0a (12 bit mode)
	{0x03CB, 0x07}, // 1080p 0x06 vs 3M 0x07
	{0x03CC, 0xb0}, // 1080p 0x60 vs 3M 0xb0
	{0x03CE, 0x46},
	{0x03D0, 0x92},
	{0x03D1, 0x80},
	{0x03D2, 0x0b}, // 1080p 0x13 vs 3M 0x0b
	{0x03D3, 0x81},
	{0x03D4, 0x01}, // 1080p 0x03 vs 3M 0x01
	{0x03D5, 0x17}, // 1080p 0x24 vs 3M 0x17 (LVDS)
	{0x03D6, 0x02}, // 1080p 0x04 vs 3M 0x02 (LVDS)
	{0x03D7, 0x1b}, // 1080p 0x28 vs 3M 0x1b (LVDS)
	{0x03D8, 0x02}, // 1080p 0x04 vs 3M 0x02 (LVDS)
	{0x03DD, 0x53},
	{0x03DF, 0x91},
	{0x03E1, 0xbc},
	{0x03E2, 0x00}, // 1080p 0x01 vs 3M 0x00
	{0x03E3, 0x80},
	{0x03E4, 0x01}, // 1080p 0x03 vs 3M 0x01
};

#define IMX122_SHARE_3M_CLASS_REG_SIZE		ARRAY_SIZE(imx122_3M_class_regs)

static const struct imx122_reg_table imx122_1080p_class_regs[] = {
	{0x0227, 0x20},
	{0x023b, 0xe0},
/*------Section 3, can't be read----------*/
	{0x0300, 0x01},
	{0x0317, 0x0d},
};

#define IMX122_SHARE_1080P_CLASS_REG_SIZE		ARRAY_SIZE(imx122_1080p_class_regs)

static const struct imx122_reg_table imx122_share_regs[] = {
	{0x022D, 0x48},// 10-bit output 2-bit shift(right justified)
};
#define IMX122_SHARE_REG_SIZE		ARRAY_SIZE(imx122_share_regs)

static const struct imx122_pll_reg_table imx122_pll_tbl[] = {
	[0] = {//for 30/60fps
		.pixclk = PLL_CLK_74_25MHZ,
		.extclk = PLL_CLK_37_125MHZ,
		.regs = {
		}
	},
	[1] = {//for 29.97/59.94fps
		.pixclk = 74175750,
		.extclk = 37087875,
		.regs = {
		}
	},
};
