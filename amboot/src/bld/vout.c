/**
 * system/src/bld/vout.c
 *
 * History:
 *    2008/02/24 - [E-John Lien] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <ambhw.h>
#include <bldfunc.h>
#include <vout.h>

#if defined(SHOW_AMBOOT_SPLASH)

u8 bld_clut[256 * 3] = {
  5,   4,   3,	// black
191,   0,   0,	// red
  0, 191,   0,	// green
191, 191,   0,	// brown
  0,   0, 191,	// blue
191,   0, 191,	// magenta
  0, 191, 191,	// cyan
192, 192, 192,	// lightgray
128, 128, 128,	// darkgray
255,   0,   0,	// lightred
  0, 255,   0,	// lightgreen
255, 255,   0,	// yellow
  0,   0, 255,	// lightblue
255,   0, 255,	// lightmagenta
  0, 255, 255,	// lightcyan
255, 255, 255,	// white
  0,   0,   0,  // Start of default 216 color Windows Palette
 51,   0,   0,
102,   0,   0,
153,   0,   0,
204,   0,   0,  // 20
255,   0,   0,
  0,  51,   0,
 51,  51,   0,
102,  51,   0,
153,  51,   0,
204,  51,   0,
255,  51,   0,
  0, 102,   0,
 51, 102,   0,
102, 102,   0,  // 30
153, 102,   0,
204, 102,   0,
255, 102,   0,
  0, 153,   0,
 51, 153,   0,
102, 153,   0,
153, 153,   0,
204, 153,   0,
255, 153,   0,
  0, 204,   0,  //40
 51, 204,   0,
102, 204,   0,
153, 204,   0,
204, 204,   0,
255, 204,   0,
  0, 255,   0,
 51, 255,   0,
102, 255,   0,
153, 255,   0,
204, 255,   0,  // 50
255, 255,   0,
  0,   0,  51,
 51,   0,  51,
102,   0,  51,
153,   0,  51,
204,   0,  51,
255,   0,  51,
  0,  51,  51,
 51,  51,  51,
102,  51,  51,  // 60
153,  51,  51,
204,  51,  51,
255,  51,  51,
  0, 102,  51,
 51, 102,  51,
102, 102,  51,
153, 102,  51,
204, 102,  51,
255, 102,  51,
  0, 153,  51,  // 70
 51, 153,  51,
102, 153,  51,
153, 153,  51,
204, 153,  51,
255, 153,  51,
  0, 204,  51,
 51, 204,  51,
102, 204,  51,
153, 204,  51,
204, 204,  51,  // 80
255, 204,  51,
  0, 255,  51,
 51, 255,  51,
102, 255,  51,
153, 255,  51,
204, 255,  51,
255, 255,  51,
  0,   0, 102,
 51,   0, 102,
102,   0, 102,  // 90
153,   0, 102,
204,   0, 102,
255,   0, 102,
  0,  51, 102,
 51,  51, 102,
102,  51, 102,
153,  51, 102,
204,  51, 102,
255,  51, 102,
  0, 102, 102,  // 100
 51, 102, 102,
102, 102, 102,
153, 102, 102,
204, 102, 102,
255, 102, 102,
  0, 153, 102,
 51, 153, 102,
102, 153, 102,
153, 153, 102,
204, 153, 102,  // 110
255, 153, 102,
  0, 204, 102,
 51, 204, 102,
102, 204, 102,
153, 204, 102,
204, 204, 102,
255, 204, 102,
  0, 255, 102,
 51, 255, 102,
102, 255, 102,  // 120
153, 255, 102,
204, 255, 102,
255, 255, 102,
  0,   0, 153,
 51,   0, 153,
102,   0, 153,
153,   0, 153,
204,   0, 153,
255,   0, 153,
  0,  51, 153,  // 130
 51,  51, 153,
102,  51, 153,
153,  51, 153,
204,  51, 153,
255,  51, 153,
  0, 102, 153,
 51, 102, 153,
102, 102, 153,
153, 102, 153,
204, 102, 153,  // 140
255, 102, 153,
  0, 153, 153,
 51, 153, 153,
102, 153, 153,
153, 153, 153,
204, 153, 153,
255, 153, 153,
  0, 204, 153,
 51, 204, 153,
102, 204, 153,  // 150
153, 204, 153,
204, 204, 153,
255, 204, 153,
  0, 255, 153,
 51, 255, 153,
102, 255, 153,
153, 255, 153,
204, 255, 153,
255, 255, 153,
  0,   0, 204,  // 160
 51,   0, 204,
102,   0, 204,
153,   0, 204,
204,   0, 204,
255,   0, 204,
  0,  51, 204,
 51,  51, 204,
102,  51, 204,
153,  51, 204,
204,  51, 204,  // 170
255,  51, 204,
  0, 102, 204,
 51, 102, 204,
102, 102, 204,
153, 102, 204,
204, 102, 204,
255, 102, 204,
  0, 153, 204,
 51, 153, 204,
102, 153, 204,  // 180
153, 153, 204,
204, 153, 204,
255, 153, 204,
  0, 204, 204,
 51, 204, 204,
102, 204, 204,
153, 204, 204,
204, 204, 204,
255, 204, 204,
  0, 255, 204,  // 190
 51, 255, 204,
102, 255, 204,
153, 255, 204,
204, 255, 204,
255, 255, 204,
  0,   0, 255,
 51,   0, 255,
102,   0, 255,
153,   0, 255,
204,   0, 255,  // 200
255,   0, 255,
  0,  51, 255,
 51,  51, 255,
102,  51, 255,
153,  51, 255,
204,  51, 255,
255,  51, 255,
  0, 102, 255,
 51, 102, 255,
102, 102, 255,  // 210
153, 102, 255,
204, 102, 255,
255, 102, 255,
  0, 153, 255,
 51, 153, 255,
102, 153, 255,
153, 153, 255,
204, 153, 255,
255, 153, 255,
  0, 204, 255,  // 220
 51, 204, 255,
102, 204, 255,
153, 204, 255,
204, 204, 255,
255, 204, 255,
  0, 255, 255,
 51, 255, 255,
102, 255, 255,
153, 255, 255,
204, 255, 255,  // 230
255, 255, 255,	// 231

// 232 - 247 are entries for the anti-aliased font palette.
0, 0, 0,
17,17,17,
34,34,34,
51,51,51,       // 235
68,68,68,
85,85,85,
102,102,102,
119,119,119,
136,136,136,    // 240
153,153,153,
170,170,170,
187,187,187,
204,204,204,
221,221,221,
238,238,238,
255, 255, 255,   // 247
/* the following is empty in peg coor table */
/* suppose it to be all (255, 255, 255) */
  0,   0,   0,   // 248
  0,   0,   0,   // 249
  0,   0,   0,   // 250
  0,   0,   0,   // 251
  0,   0,   0,   // 252
  0,   0,   0,   // 253, menu background
204,   0,   0,   // 254, warning message
242, 102,  34    // 255, Specified transparency color!!
};

#if (VOUT_8_BITS_BLENDING == 0)

u8 bld_blending[256] = {
	0,  15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15,  8, 15,  0
};
#else
u8 bld_blending[256] = {
	0,   255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 128, 255,  0
};
#endif

#ifdef CONFIG_HDMI_MODE_480P
int hdmi_set_vout_ctrl_480p(lcd_dev_vout_cfg_t *vout_cfg)
{
	memset(vout_cfg, 0, sizeof(*vout_cfg));

  	/* Fill the Sync Mode and Color space */
	vout_cfg->lcd_sync		= INPUT_FORMAT_601;
	vout_cfg->color_space		= CS_RGB;

	/* Fill the HVSync polarity */
	vout_cfg->hs_polarity		= ACTIVE_LO;
	vout_cfg->vs_polarity		= ACTIVE_LO;

	/* Fill the number of VD_CLK per H-Sync, number of H-Sync's per field */
	vout_cfg->ftime_hs		= 858;
	vout_cfg->ftime_vs_top		= 525;
	vout_cfg->ftime_vs_bot		= 525;

	/* HSync start and end */
	vout_cfg->hs_start		= 0;
	vout_cfg->hs_end		= 62;

	/* VSync start and end, use two dimension(x,y) to calculate */
	vout_cfg->vs_start_row_top	= 0;
	vout_cfg->vs_start_row_bot	= 0;
	vout_cfg->vs_end_row_top	= 6;
	vout_cfg->vs_end_row_bot	= 6;

	vout_cfg->vs_start_col_top	= 0;
	vout_cfg->vs_start_col_bot	= 858 / 2;
	vout_cfg->vs_end_col_top	= 0;
	vout_cfg->vs_end_col_bot	= 858 / 2;

	/* Active start and end, use two dimension(x,y) to calculate */
	vout_cfg->act_start_row_top	= 36;
	vout_cfg->act_start_row_bot	= 36;
	vout_cfg->act_end_row_top	= 36 + 480 - 1;
	vout_cfg->act_end_row_bot	= 36 + 480 - 1;

	vout_cfg->act_start_col_top	= 122;
	vout_cfg->act_start_col_bot	= 122;
	vout_cfg->act_end_col_top	= 122 + 720 - 1;
	vout_cfg->act_end_col_bot	= 122 + 720 - 1;

	/* Set default color */
	vout_cfg->bg_color		= BG_BLACK;

	/* LCD paremeters */
	vout_cfg->lcd_display		= LCD_NTSC_DISPLAY;

	vout_cfg->osd_width		= 720;
	vout_cfg->osd_height		= 480;
	vout_cfg->osd_resolution	= VO_RGB_480P;

	vout_cfg->clock_hz		= 27 * 1000 * 1000;

	return 0;
}

void hdmi_config_480p(void)
{
	writel(HDMI_HDMISE_SOFT_RESET_REG, HDMI_HDMISE_SOFT_RESET);
	writel(HDMI_HDMISE_SOFT_RESET_REG, ~HDMI_HDMISE_SOFT_RESET);
	writel(HDMI_CLOCK_GATED_REG, HDMI_CLOCK_GATED_HDMISE_CLOCK_EN);

	writel(HDMI_OP_MODE_REG, ~HDMI_OP_MODE_OP_EN);
	writel(HDMI_EESS_CTL_REG, HDMI_HDCPCE_CTL_USE_EESS(1) |
						HDMI_HDCPCE_CTL_HDCPCE_EN);
	writel(HDMI_PHY_CTRL_REG, 0);
	writel(HDMI_PHY_CTRL_REG, HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
					HDMI_PHY_CTRL_NON_RESET_HDMI_PHY);

	writel(HDMI_P2P_AFIFO_LEVEL_REG,
				HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(12) |
				HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(4) |
				HDMI_P2P_AFIFO_LEVEL_MAX_USAGE_LEVEL(7)	|
				HDMI_P2P_AFIFO_LEVEL_MIN_USAGE_LEVEL(7) |
				HDMI_P2P_AFIFO_LEVEL_CURRENT_USAGE_LEVEL(7)
		);
	writel(HDMI_P2P_AFIFO_CTRL_REG, HDMI_P2P_AFIFO_CTRL_EN);

#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
	writel(HDMI_VUNIT_VBLANK_VFRONT_REG, 9);
	writel(HDMI_VUNIT_VBLANK_PULSE_WIDTH_REG, 6);
	writel(HDMI_VUNIT_VBLANK_VBACK_REG, 30);

	writel(HDMI_VUNIT_HBLANK_HFRONT_REG, 16);
	writel(HDMI_VUNIT_HBLANK_PULSE_WIDTH_REG, 62);
	writel(HDMI_VUNIT_HBLANK_HBACK_REG, 60);

#else
	writel(HDMI_VUNIT_VBLANK_REG,
			HDMI_VUNIT_VBLANK_LEFT_OFFSET(9) |
			HDMI_VUNIT_VBLANK_PULSE_WIDTH(6) |
			HDMI_VUNIT_VBLANK_RIGHT_OFFSET(30));

	writel(HDMI_VUNIT_HBLANK_REG,
			HDMI_VUNIT_HBLANK_LEFT_OFFSET(16) |
			HDMI_VUNIT_HBLANK_PULSE_WIDTH(62) |
			HDMI_VUNIT_HBLANK_RIGHT_OFFSET(60));
#endif


	writel(HDMI_VUNIT_VACTIVE_REG, 480);
	writel(HDMI_VUNIT_HACTIVE_REG, 720);

	writel(HDMI_VUNIT_CTRL_REG,
			HDMI_VUNIT_CTRL_VIDEO_MODE(0) |
			HDMI_VUNIT_CTRL_HSYNC_POL(~0) |
			HDMI_VUNIT_CTRL_VSYNC_POL(~0));
	writel(HDMI_VUNIT_VSYNC_DETECT_REG, HDMI_VUNIT_VSYNC_DETECT_EN);

	writel(HDMI_PACKET_GENERAL_CTRL_REG, HDMI_PACKET_GENERAL_CTRL_CLR_AVMUTE_EN);
	writel(HDMI_PACKET_MISC_REG, 0);

	writel(HDMI_HDMISE_TM_REG, 0);
	writel(HDMI_OP_MODE_REG, HDMI_OP_MODE_OP_MODE_HDMI | HDMI_OP_MODE_OP_EN);
}
#endif

#ifdef CONFIG_HDMI_MODE_576P
int hdmi_set_vout_ctrl_576p(lcd_dev_vout_cfg_t *vout_cfg)
{
	memset(vout_cfg, 0, sizeof(*vout_cfg));

  	/* Fill the Sync Mode and Color space */
	vout_cfg->lcd_sync		= INPUT_FORMAT_601;
	vout_cfg->color_space		= CS_RGB;

	/* Fill the HVSync polarity */
	vout_cfg->hs_polarity		= ACTIVE_LO;
	vout_cfg->vs_polarity		= ACTIVE_LO;

	/* Fill the number of VD_CLK per H-Sync, number of H-Sync's per field */
	vout_cfg->ftime_hs		= 864;
	vout_cfg->ftime_vs_top		= 625;
	vout_cfg->ftime_vs_bot		= 625;

	/* HSync start and end */
	vout_cfg->hs_start		= 0;
	vout_cfg->hs_end		= 64;

	/* VSync start and end, use two dimension(x,y) to calculate */
	vout_cfg->vs_start_row_top	= 0;
	vout_cfg->vs_start_row_bot	= 0;
	vout_cfg->vs_end_row_top	= 5;
	vout_cfg->vs_end_row_bot	= 5;

	vout_cfg->vs_start_col_top	= 0;
	vout_cfg->vs_start_col_bot	= 864 / 2;
	vout_cfg->vs_end_col_top	= 0;
	vout_cfg->vs_end_col_bot	= 864 / 2;

	/* Active start and end, use two dimension(x,y) to calculate */
	vout_cfg->act_start_row_top	= 44;
	vout_cfg->act_start_row_bot	= 44;
	vout_cfg->act_end_row_top	= 44 + 576 - 1;
	vout_cfg->act_end_row_bot	= 44 + 576 - 1;

	vout_cfg->act_start_col_top	= 132;
	vout_cfg->act_start_col_bot	= 132;
	vout_cfg->act_end_col_top	= 132 + 720 - 1;
	vout_cfg->act_end_col_bot	= 132 + 720 - 1;

	/* Set default color */
	vout_cfg->bg_color		= BG_BLACK;

	/* LCD paremeters */
	vout_cfg->lcd_display		= LCD_PAL_DISPLAY;

	vout_cfg->osd_width		= 720;
	vout_cfg->osd_height		= 576;
	vout_cfg->osd_resolution	= VO_RGB_576P;

	vout_cfg->clock_hz		= 27 * 1000 * 1000;

	return 0;
}

void hdmi_config_576p(void)
{
	writel(HDMI_HDMISE_SOFT_RESET_REG, HDMI_HDMISE_SOFT_RESET);
	writel(HDMI_HDMISE_SOFT_RESET_REG, ~HDMI_HDMISE_SOFT_RESET);
	writel(HDMI_CLOCK_GATED_REG, HDMI_CLOCK_GATED_HDMISE_CLOCK_EN);

	writel(HDMI_OP_MODE_REG, ~HDMI_OP_MODE_OP_EN);
	writel(HDMI_EESS_CTL_REG, HDMI_HDCPCE_CTL_USE_EESS(1) |
						HDMI_HDCPCE_CTL_HDCPCE_EN);
	writel(HDMI_PHY_CTRL_REG, 0);
	writel(HDMI_PHY_CTRL_REG, HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
					HDMI_PHY_CTRL_NON_RESET_HDMI_PHY);

	writel(HDMI_P2P_AFIFO_LEVEL_REG,
				HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(12) |
				HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(4) |
				HDMI_P2P_AFIFO_LEVEL_MAX_USAGE_LEVEL(7)	|
				HDMI_P2P_AFIFO_LEVEL_MIN_USAGE_LEVEL(7) |
				HDMI_P2P_AFIFO_LEVEL_CURRENT_USAGE_LEVEL(7)
		);
	writel(HDMI_P2P_AFIFO_CTRL_REG, HDMI_P2P_AFIFO_CTRL_EN);

#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
	writel(HDMI_VUNIT_VBLANK_VFRONT_REG, 5);
	writel(HDMI_VUNIT_VBLANK_PULSE_WIDTH_REG, 5);
	writel(HDMI_VUNIT_VBLANK_VBACK_REG, 39);

	writel(HDMI_VUNIT_HBLANK_HFRONT_REG, 12);
	writel(HDMI_VUNIT_HBLANK_PULSE_WIDTH_REG, 64);
	writel(HDMI_VUNIT_HBLANK_HBACK_REG, 68);

#else
	writel(HDMI_VUNIT_VBLANK_REG,
			HDMI_VUNIT_VBLANK_LEFT_OFFSET(5) |
			HDMI_VUNIT_VBLANK_PULSE_WIDTH(5) |
			HDMI_VUNIT_VBLANK_RIGHT_OFFSET(39));

	writel(HDMI_VUNIT_HBLANK_REG,
			HDMI_VUNIT_HBLANK_LEFT_OFFSET(12) |
			HDMI_VUNIT_HBLANK_PULSE_WIDTH(64) |
			HDMI_VUNIT_HBLANK_RIGHT_OFFSET(68));
#endif


	writel(HDMI_VUNIT_VACTIVE_REG, 576);
	writel(HDMI_VUNIT_HACTIVE_REG, 720);

	writel(HDMI_VUNIT_CTRL_REG,
			HDMI_VUNIT_CTRL_VIDEO_MODE(0) |
			HDMI_VUNIT_CTRL_HSYNC_POL(~0) |
			HDMI_VUNIT_CTRL_VSYNC_POL(~0));
	writel(HDMI_VUNIT_VSYNC_DETECT_REG, HDMI_VUNIT_VSYNC_DETECT_EN);

	writel(HDMI_PACKET_GENERAL_CTRL_REG, HDMI_PACKET_GENERAL_CTRL_CLR_AVMUTE_EN);
	writel(HDMI_PACKET_MISC_REG, 0);

	writel(HDMI_HDMISE_TM_REG, 0);
	writel(HDMI_OP_MODE_REG, HDMI_OP_MODE_OP_MODE_HDMI | HDMI_OP_MODE_OP_EN);
}
#endif

#ifdef CONFIG_CVBS_MODE_480I
int cvbs_set_vout_ctrl_480i(lcd_dev_vout_cfg_t *vout_cfg)
{
	memset(vout_cfg, 0, sizeof(*vout_cfg));

  	/* Fill the Sync Mode and Color space */
	vout_cfg->lcd_sync		= INPUT_FORMAT_601;
	vout_cfg->color_space		= CS_YUV;

	/* Fill the HVSync polarity */
	vout_cfg->hs_polarity		= ACTIVE_HI;
	vout_cfg->vs_polarity		= ACTIVE_HI;

	/* Fill the number of VD_CLK per H-Sync, number of H-Sync's per field */
	vout_cfg->ftime_hs		= 858 * 2;
	vout_cfg->ftime_vs_top		= 263;
	vout_cfg->ftime_vs_bot		= 262;

	/* HSync start and end */
	vout_cfg->hs_start		= 0;
	vout_cfg->hs_end		= 1;

	/* VSync start and end, use two dimension(x,y) to calculate */
	vout_cfg->vs_start_row_top	= 0;
	vout_cfg->vs_start_row_bot	= 0x3fff;
	vout_cfg->vs_end_row_top	= 0;
	vout_cfg->vs_end_row_bot	= 0x3fff;

	vout_cfg->vs_start_col_top	= 0;
	vout_cfg->vs_start_col_bot	= 0;
	vout_cfg->vs_end_col_top	= 8;
	vout_cfg->vs_end_col_bot	= 1;

	/* Active start and end, use two dimension(x,y) to calculate */
	vout_cfg->act_start_row_top	= 22;
	vout_cfg->act_start_row_bot	= 21;
	vout_cfg->act_end_row_top	= 22 + 480 / 2 - 1;
	vout_cfg->act_end_row_bot	= 21 + 480 / 2 - 1;

	vout_cfg->act_start_col_top	= 273;
	vout_cfg->act_start_col_bot	= 273;
	vout_cfg->act_end_col_top	= 273 + 720 * 2 - 1;
	vout_cfg->act_end_col_bot	= 273 + 720 * 2 - 1;

	/* Set default color */
	vout_cfg->bg_color		= BG_BLACK;

	/* LCD paremeters */
	vout_cfg->lcd_display		= LCD_NTSC_DISPLAY;

	vout_cfg->osd_width		= 720;
	vout_cfg->osd_height		= 480;
	vout_cfg->osd_resolution	= VO_RGB_480I;

	vout_cfg->clock_hz		= 27 * 1000 * 1000;
	vout_cfg->use_dve		= 1;

	return 0;
}

void cvbs_config_480i(dram_dve_t *dve)
{
	memset(dve, 0, sizeof(*dve));

	dve->phi_24_31			= 0x00000021;
	dve->phi_16_23			= 0x000000f0;
	dve->phi_15_8			= 0x0000007c;
	dve->phi_7_0			= 0x0000001f;

	dve->black_lvl			= 0x0000007d;
	dve->blank_lvl			= 0x0000007a;
	dve->clamp_lvl			= 0x00000004;
	dve->sync_lvl			= 0x00000008;

	dve->dve_46.s.t_ydel_adj	= 4;
	dve->dve_46.s.t_sel_ylpf	= 0;
	dve->dve_46.s.t_ygain_val	= 1;

	dve->nba			= 0x000000c4;
	dve->pba			= 0;

	dve->dve_56.s.y_tsyn_mode	= 1;
	dve->dve_56.s.y_tencd_mode	= 0;

	dve->hcl_9_8			= 0x00000003;
	dve->hcl_7_0			= 0x00000059;

	dve->dve_88.s.bst_zone_sw1	= 8;

	dve->dve_89.s.bz3_end		= 5;
	dve->dve_89.s.adv_bs_en		= 1;
	dve->dve_89.s.bz_invert_en	= 4;

	dve->dve_97.s.sel_y_lpf		= 1;

	dve->dve_99.s.byp_c_lpf		= 1;

	dve->vsmr_4_0			= 0x00010000;
}
#endif

#ifdef CONFIG_CVBS_MODE_576I
int cvbs_set_vout_ctrl_576i(lcd_dev_vout_cfg_t *vout_cfg)
{
	memset(vout_cfg, 0, sizeof(*vout_cfg));

  	/* Fill the Sync Mode and Color space */
	vout_cfg->lcd_sync		= INPUT_FORMAT_601;
	vout_cfg->color_space		= CS_YUV;

	/* Fill the HVSync polarity */
	vout_cfg->hs_polarity		= ACTIVE_HI;
	vout_cfg->vs_polarity		= ACTIVE_HI;

	/* Fill the number of VD_CLK per H-Sync, number of H-Sync's per field */
	vout_cfg->ftime_hs		= 864 * 2;
	vout_cfg->ftime_vs_top		= 313;
	vout_cfg->ftime_vs_bot		= 312;

	/* HSync start and end */
	vout_cfg->hs_start		= 0;
	vout_cfg->hs_end		= 1;

	/* VSync start and end, use two dimension(x,y) to calculate */
	vout_cfg->vs_start_row_top	= 0;
	vout_cfg->vs_start_row_bot	= 0x3fff;
	vout_cfg->vs_end_row_top	= 0;
	vout_cfg->vs_end_row_bot	= 0x3fff;

	vout_cfg->vs_start_col_top	= 0;
	vout_cfg->vs_start_col_bot	= 0;
	vout_cfg->vs_end_col_top	= 8;
	vout_cfg->vs_end_col_bot	= 1;

	/* Active start and end, use two dimension(x,y) to calculate */
	vout_cfg->act_start_row_top	= 23;
	vout_cfg->act_start_row_bot	= 22;
	vout_cfg->act_end_row_top	= 23 + 576 / 2 - 1;
	vout_cfg->act_end_row_bot	= 22 + 576 / 2 - 1;

	vout_cfg->act_start_col_top	= 286;
	vout_cfg->act_start_col_bot	= 286;
	vout_cfg->act_end_col_top	= 286 + 720 * 2 - 1;
	vout_cfg->act_end_col_bot	= 286 + 720 * 2 - 1;

	/* Set default color */
	vout_cfg->bg_color		= BG_BLACK;

	/* LCD paremeters */
	vout_cfg->lcd_display		= LCD_PAL_DISPLAY;

	vout_cfg->osd_width		= 720;
	vout_cfg->osd_height		= 576;
	vout_cfg->osd_resolution	= VO_RGB_576I;

	vout_cfg->clock_hz		= 27 * 1000 * 1000;
	vout_cfg->use_dve		= 1;

	return 0;
}

void cvbs_config_576i(dram_dve_t *dve)
{
	memset(dve, 0, sizeof(*dve));

	dve->phi_24_31			= 0x0000002a;
	dve->phi_16_23			= 0x00000009;
	dve->phi_15_8			= 0x0000008a;
	dve->phi_7_0			= 0x000000cb;

	dve->black_lvl			= 0x0000007e;
	dve->blank_lvl			= 0x0000007e;
	dve->clamp_lvl			= 0x00000013;
	dve->sync_lvl			= 0x00000010;

	dve->dve_46.s.t_sel_ylpf	= 1;
	dve->dve_46.s.t_ygain_val	= 1;

	dve->nba			= 0x000000d3;
	dve->pba			= 0x0000002d;

	dve->dve_52.s.pal_c_lpf		= 1;
	dve->dve_52.s.sel_c_gain	= 1;


	dve->dve_56.s.y_tsyn_mode	= 1;
	dve->dve_56.s.y_tencd_mode	= 1;

	dve->hcl_9_8			= 0x00000003;
	dve->hcl_7_0			= 0x0000005f;

	dve->dve_88.s.bst_zone_sw1	= 8;

	dve->dve_89.s.bz3_end		= 5;
	dve->dve_89.s.adv_bs_en		= 1;
	dve->dve_89.s.bz_invert_en	= 4;

	dve->dve_97.s.sel_y_lpf		= 1;

	dve->vsmr_4_0			= 0x00010000;}
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == S2)
#include "vout_dsp.c"
#else
#include "vout_ahb.c"
#endif

#endif

