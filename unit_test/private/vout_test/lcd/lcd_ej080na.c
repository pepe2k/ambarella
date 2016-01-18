/*
 * lcd_ej080na.c
 *
 * History:
 *	2011/09/27 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#define EJ080NA_POWER_PIN		GPIO(41)
#define EJ080NA_RESET_PIN		GPIO(105)
#define EJ080NA_STANDBY_PIN		GPIO(154)

#define EJ080NA_BACKLIGHT_ENABLE_PIN	GPIO(140)
#define EJ080NA_BACKLIGHT_STRENGTH_PIN	GPIO(16)

#define EJ080NA_CONVERTER_POWER_PIN	GPIO(153)

static void lcd_ej080na_config_xga()
{
	/* Disable Backlight */
	gpio_clr(EJ080NA_BACKLIGHT_ENABLE_PIN);
	gpio_set(EJ080NA_BACKLIGHT_STRENGTH_PIN);

	/* Power Off LCD */
	gpio_set(EJ080NA_POWER_PIN);
	gpio_clr(EJ080NA_RESET_PIN);
	gpio_clr(EJ080NA_STANDBY_PIN);

	/* Power Off Converter */
	gpio_clr(EJ080NA_CONVERTER_POWER_PIN);

	/* Power On LCD */
	gpio_set(EJ080NA_RESET_PIN);
	gpio_set(EJ080NA_STANDBY_PIN);
	gpio_clr(EJ080NA_POWER_PIN);

	/* Power On Converter */
	gpio_set(EJ080NA_CONVERTER_POWER_PIN);

	/* Enable Backlight */
	gpio_clr(EJ080NA_BACKLIGHT_STRENGTH_PIN);
	gpio_set(EJ080NA_BACKLIGHT_ENABLE_PIN);
}

/* ========================================================================== */
static int lcd_ej080na_xga(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode			= AMBA_VIDEO_MODE_XGA;
	pvout_cfg->ratio		= AMBA_VIDEO_RATIO_4_3;
	pvout_cfg->bits			= AMBA_VIDEO_BITS_16;
	pvout_cfg->type			= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format		= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type		= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y		= 0x10;
	pvout_cfg->bg_color.cb		= 0x80;
	pvout_cfg->bg_color.cr		= 0x80;

	pvout_cfg->lcd_cfg.mode		= AMBA_VOUT_LCD_MODE_RGB888;
	pvout_cfg->lcd_cfg.seqt		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge	= AMBA_VOUT_LCD_CLK_FALLING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 65000000;
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_EJ080NA;

	return 0;
}

int lcd_ej080na_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_XGA:
		errCode = lcd_ej080na_xga(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

int lcd_ej080na_post_setmode(int mode)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_XGA:
		lcd_ej080na_config_xga();
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

