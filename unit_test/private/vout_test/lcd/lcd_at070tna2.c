/*
 * lcd_at070tna2.c
 *
 * History:
 *	2012/01/13 - [Long Zhao] created file
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
#define TRUELY_AT070TNA2_PWM_PATH(x)	"/sys/class/backlight/pwm-backlight.0/"x

static void lcd_truely_at070tna2_config_wsvga()
{
#ifndef LCD_BRIGHEST
	char			buf[64];
	int			pwm_fd1, pwm_fd2;
	int			lcd_brightness;
	int			ret;
#endif

	/* Power On */
	lcd_power_on();

	/* Hardware Reset */
	lcd_reset();

#ifdef LCD_BRIGHEST
	/* Backlight on */
	lcd_backlight_on();
#endif

#ifndef LCD_BRIGHEST
	/* Backlight on */
	pwm_fd1 = open(TRUELY_AT070TNA2_PWM_PATH("max_brightness"), O_RDONLY);
	if (pwm_fd1 < 0) {
		perror("Can't open max_brightness to read");
		goto lcd_truely_at070tna2_config_wsvga_exit;
	}
	ret = read(pwm_fd1, buf, sizeof(buf));
	close(pwm_fd1);
	if (ret <= 0) {
		perror("Can't read max_brightness");
		goto lcd_truely_at070tna2_config_wsvga_exit;
	} else {
		lcd_brightness = atoi(buf) / 2;
	}

	pwm_fd2 = open(TRUELY_AT070TNA2_PWM_PATH("brightness"), O_WRONLY);
	if (pwm_fd2 < 0) {
		perror("Can't open brightness to write");
		goto lcd_truely_at070tna2_config_wsvga_exit;
	}
	sprintf(buf, "%d", lcd_brightness);
	ret = write(pwm_fd2, buf, sizeof(buf));
	close(pwm_fd2);
	if (ret <= 0) {
		perror("Can't write brightness");
		goto lcd_truely_at070tna2_config_wsvga_exit;
	}
#endif

	return;
}

/* ========================================================================== */
static int lcd_truely_at070tna2_wsvga(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode			= AMBA_VIDEO_MODE_WSVGA;
	pvout_cfg->ratio		= AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits			= AMBA_VIDEO_BITS_16;
	pvout_cfg->type			= AMBA_VIDEO_TYPE_RGB_RAW;
	pvout_cfg->format		= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type		= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y		= 0x10;
	pvout_cfg->bg_color.cb		= 0x80;
	pvout_cfg->bg_color.cr		= 0x80;

	pvout_cfg->lcd_cfg.mode		= AMBA_VOUT_LCD_MODE_RGB888;
	pvout_cfg->lcd_cfg.seqt		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge	= AMBA_VOUT_LCD_CLK_FALLING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 512000000;
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_AT070TNA2;

	lcd_truely_at070tna2_config_wsvga();

	return 0;
}

int lcd_at070tna2_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_WSVGA:
		errCode = lcd_truely_at070tna2_wsvga(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
