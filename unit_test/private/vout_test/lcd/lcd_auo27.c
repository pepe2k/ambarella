/*
 * lcd_auo27.c
 *
 * History:
 *	2009/05/20 - [Anthony Ginger] created file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "basetypes.h"

#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vout.h"

#include "lcd_api.h"

/* ========================================================================== */
#define SPI_DEV_NODE			"/dev/spidev0.3"

//#define A3DVR
#ifdef A3DVR
#define GPIO_HDMI_LCD			GPIO(46)
#define GPIO_656			GPIO(50)
#else
#define GPIO_HDMI_LCD			GPIO(57)
#define GPIO_656			GPIO(58)
#endif

/* ========================================================================== */
static void set_spi_bus(int fd)
{
	u8 mode = 0;		/*CPHA = 0, CPOL = 0*/
	u8 bits = 16;		/*bits per word is 16*/
	u32 speed = 500000;	/*SSI_CLK = 500KHz, can speed up if necessary*/

	ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

static int lcd_auo27_config(u16 rst_reg, u16 *data, u16 size, int b656mode)
{
	int					spi_fd;
	int					i;

	gpio_set(GPIO_HDMI_LCD);
	if (b656mode)
		gpio_clr(GPIO_656);
	else
		gpio_set(GPIO_656);

	spi_fd = open(SPI_DEV_NODE, O_WRONLY);
	if (spi_fd < 0) {
		perror("Can't open SPI_DEV_NODE to write");
		goto lcd_auo27_config_exit;
	} else {
		set_spi_bus(spi_fd);
		write(spi_fd, &rst_reg, 2);
	}
	sleep(1);
	for (i = 0; i < size; i++) {
		write(spi_fd, &data[i], 2);
	}
	close(spi_fd);

lcd_auo27_config_exit:
	return 0;
}

/* ========================================================================== */
static int lcd_auo27_960_240(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x040b,		/* set ups051 960x240 */
			0x07f1,
			0x0615,
			0x0003,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capavility */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_960_240;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 0);
}

/* ========================================================================== */
static int lcd_auo27_320_240(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0417,		/* set ups052 320x240 */
			0x07f1,
			0x0615,
			0x0003,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capavility */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_320_240;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 0);
}

/* ========================================================================== */
static int lcd_auo27_320_288(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0413,		/* set ups052 320x288 */
			0x07f1,
			0x0613,
			0x0003,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capavility */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_320_288;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 0);
}

/* ========================================================================== */
static int lcd_auo27_360_240(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0427,		/* set ups052 360x240 */
			0x07f1,
			0x0615,
			0x0003,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capavility */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_360_240;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 0);
}

/* ========================================================================== */
static int lcd_auo27_360_288(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0423,		/* set ups052 360x288 */
			0x07f1,
			0x0613,
			0x0003,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capavility */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_360_288;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 0);
}

/* ========================================================================== */
static int lcd_auo27_480i(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0477,		/* set ccir656 input mode (YUV720)*/
			0x0616,		/* VBLK settings = 22 rows */
			0x0003,
			0x0c26,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capability */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_480I;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_YUV_656;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 1);
}

/* ========================================================================== */
static int lcd_auo27_576i(struct amba_video_sink_mode *pvout_cfg)
{
	u16 rst_reg = 0x0526;		/* reset */
	u16 data[] = {	0x0473,		/* set ccir656 input mode (YUV720)*/
			0x0615,		/* VBLK settings = 24 rows */
			0x0003,
			0x0c26,
			0x032e,
			0x0d4b,
			0x0000,
			0x0830,		/* set backlight driving capability */
			0x0567		/* release standby */
		      };

	pvout_cfg->mode = AMBA_VIDEO_MODE_576I;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits = AMBA_VIDEO_BITS_8;
	pvout_cfg->type = AMBA_VIDEO_TYPE_YUV_656;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_INTERLACE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

	return lcd_auo27_config(rst_reg, data, sizeof(data)/sizeof(data[0]), 1);
}

int lcd_auo27_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int					errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_480I:
		errCode = lcd_auo27_480i(pcfg);
		break;

	case AMBA_VIDEO_MODE_576I:
		errCode = lcd_auo27_576i(pcfg);
		break;

	case AMBA_VIDEO_MODE_960_240:
		errCode = lcd_auo27_960_240(pcfg);
		break;

	case AMBA_VIDEO_MODE_320_240:
		errCode = lcd_auo27_320_240(pcfg);
		break;

	case AMBA_VIDEO_MODE_320_288:
		errCode = lcd_auo27_320_288(pcfg);
		break;

	case AMBA_VIDEO_MODE_360_240:
		errCode = lcd_auo27_360_240(pcfg);
		break;

	case AMBA_VIDEO_MODE_360_288:
		errCode = lcd_auo27_360_288(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
