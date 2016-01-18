/*
 * lcd_tpo648.c
 *
 * History:
 *	2009/11/25 - [Zhenwu Xue] created file
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

#define TPO648_HIGH_ADDR(addr)		(0x2000 | (addr))
#define TPO648_LOW_ADDR(addr)		(0x0000 | (addr))
#define TPO648_DATA(data)		(0x4000 | (data))

#define TPO648_WRITE_REGISTER(addr, data)	\
	cmd = TPO648_HIGH_ADDR(addr >> 8);	\
	write(spi_fd, &cmd, 2);	\
	cmd = TPO648_LOW_ADDR(addr & 0xff);	\
	write(spi_fd, &cmd, 2);	\
	cmd = TPO648_DATA(data);	\
	write(spi_fd, &cmd, 2);

/* ========================================================================== */
static void lcd_tpo648_config_480x800()
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
		goto lcd_tpo648_config_480x800_exit;
	}

	spi_fd = open(buf, O_RDWR);
	if (spi_fd < 0) {
		perror("Can't open TPO648_SPI_DEV_NODE to write");
		goto lcd_tpo648_config_480x800_exit;
	}

	mode = 0;
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("Can't set SPI mode");
		close(spi_fd);
		goto lcd_tpo648_config_480x800_exit;
	}

	bits = 16;
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("Can't set SPI bits");
		close(spi_fd);
		goto lcd_tpo648_config_480x800_exit;
	}

	speed = 500000;
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("Can't set SPI speed");
		close(spi_fd);
		goto lcd_tpo648_config_480x800_exit;
	}

	/* Don't reload MTP */
	TPO648_WRITE_REGISTER(0x2e80, 0x01);

	/* LTPS */
	TPO648_WRITE_REGISTER(0x0680, 0x2d)
	TPO648_WRITE_REGISTER(0xd380, 0x04)
	TPO648_WRITE_REGISTER(0xd580, 0x07)
	TPO648_WRITE_REGISTER(0xd680, 0x5a)
	TPO648_WRITE_REGISTER(0xd080, 0x0f)
	TPO648_WRITE_REGISTER(0xd180, 0x16)
	TPO648_WRITE_REGISTER(0xd280, 0x04)
	TPO648_WRITE_REGISTER(0xdc80, 0x04)
	TPO648_WRITE_REGISTER(0xd780, 0x01)
	TPO648_WRITE_REGISTER(0x2280, 0x0f)
	TPO648_WRITE_REGISTER(0x2480, 0x68)
	TPO648_WRITE_REGISTER(0x2580, 0x00)
	TPO648_WRITE_REGISTER(0x2780, 0xaf)

	/* RGB Interface */
	TPO648_WRITE_REGISTER(0x3a00, 0x66)
	TPO648_WRITE_REGISTER(0x3b00, 0x03)

	/* Gamma */
	TPO648_WRITE_REGISTER(0x0180, 0x00)
	TPO648_WRITE_REGISTER(0x4080, 0x51)
	TPO648_WRITE_REGISTER(0x4180, 0x55)
	TPO648_WRITE_REGISTER(0x4280, 0x58)
	TPO648_WRITE_REGISTER(0x4380, 0x64)
	TPO648_WRITE_REGISTER(0x4480, 0x1a)
	TPO648_WRITE_REGISTER(0x4580, 0x2e)
	TPO648_WRITE_REGISTER(0x4680, 0x5f)
	TPO648_WRITE_REGISTER(0x4780, 0x21)
	TPO648_WRITE_REGISTER(0x4880, 0x1c)
	TPO648_WRITE_REGISTER(0x4980, 0x22)
	TPO648_WRITE_REGISTER(0x4a80, 0x5d)
	TPO648_WRITE_REGISTER(0x4b80, 0x19)
	TPO648_WRITE_REGISTER(0x4c80, 0x46)
	TPO648_WRITE_REGISTER(0x4d80, 0x62)
	TPO648_WRITE_REGISTER(0x4e80, 0x48)
	TPO648_WRITE_REGISTER(0x4f80, 0x5b)
	TPO648_WRITE_REGISTER(0x5080, 0x2f)
	TPO648_WRITE_REGISTER(0x5180, 0x5e)
	TPO648_WRITE_REGISTER(0x5880, 0x2e)
	TPO648_WRITE_REGISTER(0x5980, 0x3b)
	TPO648_WRITE_REGISTER(0x5a80, 0x8d)
	TPO648_WRITE_REGISTER(0x5b80, 0xa7)
	TPO648_WRITE_REGISTER(0x5c80, 0x27)
	TPO648_WRITE_REGISTER(0x5d80, 0x39)
	TPO648_WRITE_REGISTER(0x5e80, 0x65)
	TPO648_WRITE_REGISTER(0x5f80, 0x55)
	TPO648_WRITE_REGISTER(0x6080, 0x1a)
	TPO648_WRITE_REGISTER(0x6180, 0x21)
	TPO648_WRITE_REGISTER(0x6280, 0x8f)
	TPO648_WRITE_REGISTER(0x6380, 0x22)
	TPO648_WRITE_REGISTER(0x6480, 0x53)
	TPO648_WRITE_REGISTER(0x6580, 0x66)
	TPO648_WRITE_REGISTER(0x6680, 0x8a)
	TPO648_WRITE_REGISTER(0x6780, 0x97)
	TPO648_WRITE_REGISTER(0x6880, 0x1f)
	TPO648_WRITE_REGISTER(0x6980, 0x26)

	/* Display on */
	TPO648_WRITE_REGISTER(0x1100, 0x00)
	usleep(200000);
	TPO648_WRITE_REGISTER(0x2900, 0x00)

	close(spi_fd);

lcd_tpo648_config_480x800_exit:
	return;
}

/* ========================================================================== */
static int lcd_tpo648_480_800(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode = AMBA_VIDEO_MODE_480_800;
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

	lcd_tpo648_config_480x800();

	return 0;
}

int lcd_tpo648_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_480_800:
		errCode = lcd_tpo648_480_800(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

