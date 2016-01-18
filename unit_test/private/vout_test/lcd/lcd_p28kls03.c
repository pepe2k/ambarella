/*
 * lcd_p28kls03.c
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

/* ========================================================================== */
#include <linux/spi/spidev.h>

static void lcd_p28kls03_config_480x640()
{
	int				spi_fd;
	u16				data[32];
	u8				mode, bits;
	u32				speed;
	char				buf[64];
	int				ret;

	/* Power On */
	lcd_power_on();

	/* Hardware Reset */
	lcd_reset();

	/* Backlight on */
	lcd_backlight_on();

	if (lcd_spi_dev_node(buf)) {
		perror("Unable to get lcd spi bus_id or cs_id!\n");
		goto lcd_p28kls03_config_480x640_exit;
	}

	spi_fd = open(buf, O_RDWR);
	if (spi_fd < 0) {
		perror("Can't open P28K_SPI_DEV_NODE to write");
		goto lcd_p28kls03_config_480x640_exit;
	}

	mode = 3;
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("Can't set SPI mode");
		close(spi_fd);
		goto lcd_p28kls03_config_480x640_exit;
	}

	bits = 9;
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("Can't set SPI bits");
		close(spi_fd);
		goto lcd_p28kls03_config_480x640_exit;
	}

	speed = 500000;
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("Can't set SPI speed");
		close(spi_fd);
		goto lcd_p28kls03_config_480x640_exit;
	}

	/* Send command */
	data[0] = 0x01;			/* Software reset */
	write(spi_fd, data, 2);

	data[0] = 0x11;			/* Sleep out */
	write(spi_fd, data, 2);

	data[0] = 0xc2;			/* Power control */
	data[1] = 0x107;
	write(spi_fd, data, 4);

	data[0] = 0xc1;			/* Power control */
	data[1] = 0x122;
	write(spi_fd, data, 4);

	data[0] = 0xc5;			/* Vcom control */
	data[1] = 0x1cd;
	write(spi_fd, data, 4);

	data[0] = 0xc6;			/* Vcom control */
	data[1] = 0x1a4;
	write(spi_fd, data, 4);

	data[0] = 0xf3;			/* Unknown register */
	data[1] = 0x128;
	write(spi_fd, data, 4);

	data[0] = 0x36;			/* Memory access order */
	data[1] = 0x180;
	write(spi_fd, data, 4);

	data[0] = 0x3a;			/* Interface pixel format */
	data[1] = 0x150;
	write(spi_fd, data, 4);

	data[0] = 0xf5;			/* Unknown register */
	data[1] = 0x110;
	write(spi_fd, data, 4);

	data[0] = 0xb0;			/* Signal polarity control */
	data[1] = 0x100;
	write(spi_fd, data, 4);

	data[0] = 0xb4;			/* Inversion */
	data[1] = 0x100;
	write(spi_fd, data, 4);

	data[0] = 0xf0;			/* Input resolution */
	data[1] = 0x180;
	write(spi_fd, data, 4);

	data[0] = 0xbb;			/* Display Timing */
	data[1] = 0x102;
	data[2] = 0x128;
	data[3] = 0x100;
	data[4] = 0x114;
	data[5] = 0x10a;
	data[6] = 0x111;
	data[7] = 0x11c;
	data[8] = 0x111;
	data[9] = 0x12e;
	data[10] = 0x10f;
	write(spi_fd, data, 22);

	data[0] = 0xe0;			/* Gamma correction */
	data[1] = 0x103;
	data[2] = 0x10d;
	data[3] = 0x112;
	data[4] = 0x138;
	data[5] = 0x12e;
	data[6] = 0x127;
	data[7] = 0x114;
	data[8] = 0x109;
	data[9] = 0x10f;
	data[10] = 0x110;
	data[11] = 0x10d;
	data[12] = 0x10d;
	data[13] = 0x100;
	data[14] = 0x104;
	data[15] = 0x115;
	data[16] = 0x117;
	write(spi_fd, data, 34);

	data[0] = 0xe1;			/* Gamma correction */
	data[1] = 0x127;
	data[2] = 0x12e;
	data[3] = 0x138;
	data[4] = 0x112;
	data[5] = 0x10d;
	data[6] = 0x103;
	data[7] = 0x109;
	data[8] = 0x114;
	data[9] = 0x117;
	data[10] = 0x115;
	data[11] = 0x104;
	data[12] = 0x100;
	data[13] = 0x10d;
	data[14] = 0x10d;
	data[15] = 0x110;
	data[16] = 0x10f;
	write(spi_fd, data, 34);

	data[0] = 0x29;			/* Display on */
	write(spi_fd, data, 2);

	close(spi_fd);

lcd_p28kls03_config_480x640_exit:
	return;
}

/* ========================================================================== */
static int lcd_p28kls03_480_640(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode = AMBA_VIDEO_MODE_480_640;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_4_3;
	pvout_cfg->bits = AMBA_VIDEO_BITS_16;
	pvout_cfg->type = AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_RGB565;
	pvout_cfg->lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz = 27000000;

	lcd_p28kls03_config_480x640();

	return 0;
}

int lcd_p28kls03_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_480_640:
		errCode = lcd_p28kls03_480_640(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

