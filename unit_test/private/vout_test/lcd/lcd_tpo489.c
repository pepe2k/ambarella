/*
 * lcd_tpo489.c
 *
 * History:
 *	2009/11/24 - [Zhenwu Xue] created file
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

#define TPO489_WRITE_REGISTER(addr, val)	\
	cmd = ((addr) << 8) | (val);	\
	write(spi_fd, &cmd, 2);

/* ========================================================================== */
static void lcd_tpo489_config_320x480()
{
	int				spi_fd;
	u16				cmd;
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
		goto lcd_tpo489_config_320x480_exit;
	}

	spi_fd = open(buf, O_RDWR);
	if (spi_fd < 0) {
		perror("Can't open TPO489_SPI_DEV_NODE to write");
		goto lcd_tpo489_config_320x480_exit;
	}

	mode = 0;
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("Can't set SPI mode");
		close(spi_fd);
		goto lcd_tpo489_config_320x480_exit;
	}

	bits = 16;
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("Can't set SPI bits");
		close(spi_fd);
		goto lcd_tpo489_config_320x480_exit;
	}

	speed = 500000;
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("Can't set SPI speed");
		close(spi_fd);
		goto lcd_tpo489_config_320x480_exit;
	}

	/* Software Reset */
	TPO489_WRITE_REGISTER(0x08, 0x01)
	usleep(10000);
	TPO489_WRITE_REGISTER(0x08, 0x00)

	/* Sync Polarity */
	TPO489_WRITE_REGISTER(0x02, 0x00)

	/* Vactive Start */
	TPO489_WRITE_REGISTER(0x03, 0x04)

	/* Hactive Start */
	TPO489_WRITE_REGISTER(0x04, 0x0e)

	/* Interface to CPU */
	TPO489_WRITE_REGISTER(0x09, 0x03)

	/* R32-R59 Access */
	TPO489_WRITE_REGISTER(0x0b, 0x08)

	/* Gama */
	TPO489_WRITE_REGISTER(0x0c, 0x53)

	/* Vactive Size [9:8] */
	TPO489_WRITE_REGISTER(0x0d, 0x01)

	/* Vactive Size [7:0] */
	TPO489_WRITE_REGISTER(0x0e, 0xe0)

	/* Hactive Size [10:8] */
	TPO489_WRITE_REGISTER(0x0f, 0x01)

	/* Hactive Size [7:0] */
	TPO489_WRITE_REGISTER(0x10, 0xf4)

	/* R32-R59 */
	TPO489_WRITE_REGISTER(0x20, 0x1e)
	TPO489_WRITE_REGISTER(0x21, 0x0a)
	TPO489_WRITE_REGISTER(0x22, 0x0a)
	TPO489_WRITE_REGISTER(0x23, 0x1e)
	TPO489_WRITE_REGISTER(0x25, 0x32)
	TPO489_WRITE_REGISTER(0x26, 0x00)
	TPO489_WRITE_REGISTER(0x27, 0xac)
	TPO489_WRITE_REGISTER(0x29, 0x04)
	TPO489_WRITE_REGISTER(0x2a, 0xe4)
	TPO489_WRITE_REGISTER(0x2b, 0x45)
	TPO489_WRITE_REGISTER(0x2c, 0x45)
	TPO489_WRITE_REGISTER(0x2d, 0x15)
	TPO489_WRITE_REGISTER(0x2e, 0x5a)
	TPO489_WRITE_REGISTER(0x2f, 0xff)
	TPO489_WRITE_REGISTER(0x30, 0x6b)
	TPO489_WRITE_REGISTER(0x31, 0x2e)
	TPO489_WRITE_REGISTER(0x32, 0x5e)
	TPO489_WRITE_REGISTER(0x33, 0x8e)
	TPO489_WRITE_REGISTER(0x34, 0xc8)
	TPO489_WRITE_REGISTER(0x35, 0xf5)
	TPO489_WRITE_REGISTER(0x36, 0x1e)
	TPO489_WRITE_REGISTER(0x37, 0x97)
	TPO489_WRITE_REGISTER(0x38, 0x04)
	TPO489_WRITE_REGISTER(0x39, 0x5c)
	TPO489_WRITE_REGISTER(0x3a, 0xa8)
	TPO489_WRITE_REGISTER(0x3b, 0xff)

	/* Auto Power On */
	TPO489_WRITE_REGISTER(0x07, 0xc9)

	close(spi_fd);

lcd_tpo489_config_320x480_exit:
	return;
}

/* ========================================================================== */
static int lcd_tpo489_320_480(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode = AMBA_VIDEO_MODE_HVGA;
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

	lcd_tpo489_config_320x480();

	return 0;
}

int lcd_tpo489_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_HVGA:
		errCode = lcd_tpo489_320_480(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

