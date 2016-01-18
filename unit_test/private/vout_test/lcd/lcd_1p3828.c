/*
 * lcd_1p3828.c
 *
 * History:
 *	2011/09/19 - [Zhenwu Xue] created file
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
#include <linux/spi/spidev.h>

#define LCD_BRIGHEST

#define MSLEEP(x)			usleep(1000 * (x))
#define TRUELY_1P3828_PWM_PATH(x)	"/sys/class/backlight/pwm-backlight.0/"x

static void truely_1p3828_write_cmd(int spi_fd, u8 cmd)
{
	u16	data;

	data = cmd;
	write(spi_fd, &data, sizeof(data));
}

static void truely_1p3828_write_data(int spi_fd, u8 dat)
{
	u16	data;

	data = 0x0100 | dat;
	write(spi_fd, &data, sizeof(data));
}

/* ========================================================================== */
static void truely_1p3828_set_spi_bus(int fd)
{
	u8	mode	= 0;		/* CPHA = 0, CPOL = 0 */
	u8	bits	= 9;		/* bits per word is 9 */
	u32	speed	= 500000;	/* SSI_CLK = 500KHz, can speed up if necessary */

	ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

static void lcd_truely_1p3828_config_480_800()
{
	int			spi_fd, i;
	char			buf[64];

#ifndef LCD_BRIGHEST
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

	/* Program TRUELY_1P3828 to output 480x800 */
	if (lcd_spi_dev_node(buf)) {
		perror("Unable to get lcd spi bus_id or cs_id!\n");
		goto lcd_truely_1p3828_config_480_800_exit;
	}

	spi_fd = open(buf, O_WRONLY);
	if (spi_fd < 0) {
		perror("Can't open TRUELY_1P3828_SPI_DEV_NODE to write");
		goto lcd_truely_1p3828_config_480_800_exit;
	} else {
		truely_1p3828_set_spi_bus(spi_fd);
	}

	truely_1p3828_write_cmd(spi_fd, 0xB9); //Set_EXTC
	truely_1p3828_write_data(spi_fd, 0xFF);
	truely_1p3828_write_data(spi_fd, 0x83);
	truely_1p3828_write_data(spi_fd, 0x69);

	truely_1p3828_write_cmd(spi_fd, 0xB1);  //Set Power
	truely_1p3828_write_data(spi_fd, 0x01);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x34);
	truely_1p3828_write_data(spi_fd, 0x06);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x0F);  //
	truely_1p3828_write_data(spi_fd, 0x0F);
	truely_1p3828_write_data(spi_fd, 0x2A);
	truely_1p3828_write_data(spi_fd, 0x32);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x07);
	truely_1p3828_write_data(spi_fd, 0x23);
	truely_1p3828_write_data(spi_fd, 0x01);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xE6);

	truely_1p3828_write_cmd(spi_fd, 0xB2);  // SET Display  480x800
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x2b);
	truely_1p3828_write_data(spi_fd, 0x0A);
	truely_1p3828_write_data(spi_fd, 0x0A);
	truely_1p3828_write_data(spi_fd, 0x70);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0xFF);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x03);
	truely_1p3828_write_data(spi_fd, 0x03);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x01);

	truely_1p3828_write_cmd(spi_fd, 0xB4);  // SET Display  480x800
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x18);
	truely_1p3828_write_data(spi_fd, 0x80);
	truely_1p3828_write_data(spi_fd, 0x10);
	truely_1p3828_write_data(spi_fd, 0x01);

	truely_1p3828_write_cmd(spi_fd, 0xB6);  // SET VCOM
	truely_1p3828_write_data(spi_fd, 0x2C);
	truely_1p3828_write_data(spi_fd, 0x2C);

	truely_1p3828_write_cmd(spi_fd, 0xD5);  //SET GIP
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x05);
	truely_1p3828_write_data(spi_fd, 0x03);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x01);
	truely_1p3828_write_data(spi_fd, 0x09);
	truely_1p3828_write_data(spi_fd, 0x10);
	truely_1p3828_write_data(spi_fd, 0x80);
	truely_1p3828_write_data(spi_fd, 0x37);
	truely_1p3828_write_data(spi_fd, 0x37);
	truely_1p3828_write_data(spi_fd, 0x20);
	truely_1p3828_write_data(spi_fd, 0x31);
	truely_1p3828_write_data(spi_fd, 0x46);
	truely_1p3828_write_data(spi_fd, 0x8A);
	truely_1p3828_write_data(spi_fd, 0x57);
	truely_1p3828_write_data(spi_fd, 0x9B);
	truely_1p3828_write_data(spi_fd, 0x20);
	truely_1p3828_write_data(spi_fd, 0x31);
	truely_1p3828_write_data(spi_fd, 0x46);
	truely_1p3828_write_data(spi_fd, 0x8A);
	truely_1p3828_write_data(spi_fd, 0x57);
	truely_1p3828_write_data(spi_fd, 0x9B);
	truely_1p3828_write_data(spi_fd, 0x07);
	truely_1p3828_write_data(spi_fd, 0x0F);
	truely_1p3828_write_data(spi_fd, 0x02);
	truely_1p3828_write_data(spi_fd, 0x00);

	truely_1p3828_write_cmd(spi_fd, 0xE0);  //SET GAMMA
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x08);
	truely_1p3828_write_data(spi_fd, 0x0D);
	truely_1p3828_write_data(spi_fd, 0x2D);
	truely_1p3828_write_data(spi_fd, 0x34);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0x38);
	truely_1p3828_write_data(spi_fd, 0x09);
	truely_1p3828_write_data(spi_fd, 0x0E);
	truely_1p3828_write_data(spi_fd, 0x0E);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x14);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x14);
	truely_1p3828_write_data(spi_fd, 0x13);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0x00);
	truely_1p3828_write_data(spi_fd, 0x08);

	truely_1p3828_write_data(spi_fd, 0x0D);
	truely_1p3828_write_data(spi_fd, 0x2D);
	truely_1p3828_write_data(spi_fd, 0x34);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0x38);
	truely_1p3828_write_data(spi_fd, 0x09);
	truely_1p3828_write_data(spi_fd, 0x0E);
	truely_1p3828_write_data(spi_fd, 0x0E);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x14);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x14);
	truely_1p3828_write_data(spi_fd, 0x13);
	truely_1p3828_write_data(spi_fd, 0x19);

	truely_1p3828_write_cmd(spi_fd, 0xC1); //set DGC
	truely_1p3828_write_data(spi_fd, 0x01); //enable DGC function
	truely_1p3828_write_data(spi_fd, 0x02); //SET R-GAMMA
	truely_1p3828_write_data(spi_fd, 0x08);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x1A);
	truely_1p3828_write_data(spi_fd, 0x22);
	truely_1p3828_write_data(spi_fd, 0x2A);
	truely_1p3828_write_data(spi_fd, 0x31);
	truely_1p3828_write_data(spi_fd, 0x36);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x48);
	truely_1p3828_write_data(spi_fd, 0x51);
	truely_1p3828_write_data(spi_fd, 0x58);
	truely_1p3828_write_data(spi_fd, 0x60);
	truely_1p3828_write_data(spi_fd, 0x68);
	truely_1p3828_write_data(spi_fd, 0x70);
	truely_1p3828_write_data(spi_fd, 0x78);
	truely_1p3828_write_data(spi_fd, 0x80);
	truely_1p3828_write_data(spi_fd, 0x88);
	truely_1p3828_write_data(spi_fd, 0x90);
	truely_1p3828_write_data(spi_fd, 0x98);
	truely_1p3828_write_data(spi_fd, 0xA0);
	truely_1p3828_write_data(spi_fd, 0xA7);
	truely_1p3828_write_data(spi_fd, 0xAF);
	truely_1p3828_write_data(spi_fd, 0xB6);
	truely_1p3828_write_data(spi_fd, 0xBE);
	truely_1p3828_write_data(spi_fd, 0xC7);
	truely_1p3828_write_data(spi_fd, 0xCE);
	truely_1p3828_write_data(spi_fd, 0xD6);
	truely_1p3828_write_data(spi_fd, 0xDE);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xEF);
	truely_1p3828_write_data(spi_fd, 0xF5);
	truely_1p3828_write_data(spi_fd, 0xFB);
	truely_1p3828_write_data(spi_fd, 0xFC);
	truely_1p3828_write_data(spi_fd, 0xFE);
	truely_1p3828_write_data(spi_fd, 0x8C);
	truely_1p3828_write_data(spi_fd, 0xA4);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0xEC);
	truely_1p3828_write_data(spi_fd, 0x1B);
	truely_1p3828_write_data(spi_fd, 0x4C);

	truely_1p3828_write_data(spi_fd, 0x40);
	truely_1p3828_write_data(spi_fd, 0x02); //SET G-Gamma
	truely_1p3828_write_data(spi_fd, 0x08);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x1A);
	truely_1p3828_write_data(spi_fd, 0x22);
	truely_1p3828_write_data(spi_fd, 0x2A);
	truely_1p3828_write_data(spi_fd, 0x31);
	truely_1p3828_write_data(spi_fd, 0x36);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x48);
	truely_1p3828_write_data(spi_fd, 0x51);
	truely_1p3828_write_data(spi_fd, 0x58);
	truely_1p3828_write_data(spi_fd, 0x60);
	truely_1p3828_write_data(spi_fd, 0x68);
	truely_1p3828_write_data(spi_fd, 0x70);
	truely_1p3828_write_data(spi_fd, 0x78);
	truely_1p3828_write_data(spi_fd, 0x80);
	truely_1p3828_write_data(spi_fd, 0x88);
	truely_1p3828_write_data(spi_fd, 0x90);
	truely_1p3828_write_data(spi_fd, 0x98);
	truely_1p3828_write_data(spi_fd, 0xA0);
	truely_1p3828_write_data(spi_fd, 0xA7);
	truely_1p3828_write_data(spi_fd, 0xAF);
	truely_1p3828_write_data(spi_fd, 0xB6);
	truely_1p3828_write_data(spi_fd, 0xBE);
	truely_1p3828_write_data(spi_fd, 0xC7);
	truely_1p3828_write_data(spi_fd, 0xCE);
	truely_1p3828_write_data(spi_fd, 0xD6);
	truely_1p3828_write_data(spi_fd, 0xDE);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xEF);
	truely_1p3828_write_data(spi_fd, 0xF5);
	truely_1p3828_write_data(spi_fd, 0xFB);
	truely_1p3828_write_data(spi_fd, 0xFC);
	truely_1p3828_write_data(spi_fd, 0xFE);
	truely_1p3828_write_data(spi_fd, 0x8C);
	truely_1p3828_write_data(spi_fd, 0xA4);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0xEC);
	truely_1p3828_write_data(spi_fd, 0x1B);
	truely_1p3828_write_data(spi_fd, 0x4C);
	truely_1p3828_write_data(spi_fd, 0x40);
	truely_1p3828_write_data(spi_fd, 0x02); //SET B-Gamma
	truely_1p3828_write_data(spi_fd, 0x08);
	truely_1p3828_write_data(spi_fd, 0x12);
	truely_1p3828_write_data(spi_fd, 0x1A);
	truely_1p3828_write_data(spi_fd, 0x22);
	truely_1p3828_write_data(spi_fd, 0x2A);
	truely_1p3828_write_data(spi_fd, 0x31);
	truely_1p3828_write_data(spi_fd, 0x36);
	truely_1p3828_write_data(spi_fd, 0x3F);
	truely_1p3828_write_data(spi_fd, 0x48);
	truely_1p3828_write_data(spi_fd, 0x51);
	truely_1p3828_write_data(spi_fd, 0x58);
	truely_1p3828_write_data(spi_fd, 0x60);
	truely_1p3828_write_data(spi_fd, 0x68);
	truely_1p3828_write_data(spi_fd, 0x70);
	truely_1p3828_write_data(spi_fd, 0x78);

	truely_1p3828_write_data(spi_fd, 0x80);
	truely_1p3828_write_data(spi_fd, 0x88);
	truely_1p3828_write_data(spi_fd, 0x90);
	truely_1p3828_write_data(spi_fd, 0x98);
	truely_1p3828_write_data(spi_fd, 0xA0);
	truely_1p3828_write_data(spi_fd, 0xA7);
	truely_1p3828_write_data(spi_fd, 0xAF);
	truely_1p3828_write_data(spi_fd, 0xB6);
	truely_1p3828_write_data(spi_fd, 0xBE);
	truely_1p3828_write_data(spi_fd, 0xC7);
	truely_1p3828_write_data(spi_fd, 0xCE);
	truely_1p3828_write_data(spi_fd, 0xD6);
	truely_1p3828_write_data(spi_fd, 0xDE);
	truely_1p3828_write_data(spi_fd, 0xE6);
	truely_1p3828_write_data(spi_fd, 0xEF);
	truely_1p3828_write_data(spi_fd, 0xF5);
	truely_1p3828_write_data(spi_fd, 0xFB);
	truely_1p3828_write_data(spi_fd, 0xFC);
	truely_1p3828_write_data(spi_fd, 0xFE);
	truely_1p3828_write_data(spi_fd, 0x8C);
	truely_1p3828_write_data(spi_fd, 0xA4);
	truely_1p3828_write_data(spi_fd, 0x19);
	truely_1p3828_write_data(spi_fd, 0xEC);
	truely_1p3828_write_data(spi_fd, 0x1B);
	truely_1p3828_write_data(spi_fd, 0x4C);
	truely_1p3828_write_data(spi_fd, 0x40);

	truely_1p3828_write_cmd(spi_fd, 0x2D);//Look up table
	for(i = 0; i < 64; i++) {
		truely_1p3828_write_data(spi_fd, 8 * i);
	}
	for(i = 0; i < 64; i++) {
		truely_1p3828_write_data(spi_fd, 4 * i);
	}
	for(i = 0; i < 64; i++) {
		truely_1p3828_write_data(spi_fd, 8 * i);
	}

	MSLEEP(10);
	truely_1p3828_write_cmd(spi_fd, 0x3A);  //Set COLMOD
	truely_1p3828_write_data(spi_fd, 0x77);

	truely_1p3828_write_cmd(spi_fd, 0x11);  //Sleep Out
	MSLEEP(120);

	truely_1p3828_write_cmd(spi_fd, 0x29);  //Display On
	//truely_1p3828_write_cmd(spi_fd, 0x2C);  //Display On


	close(spi_fd);

#ifndef LCD_BRIGHEST
	/* Backlight on */
	pwm_fd1 = open(TRUELY_1P3828_PWM_PATH("max_brightness"), O_RDONLY);
	if (pwm_fd1 < 0) {
		perror("Can't open max_brightness to read");
		goto lcd_truely_1p3828_config_480_800_exit;
	}
	ret = read(pwm_fd1, buf, sizeof(buf));
	close(pwm_fd1);
	if (ret <= 0) {
		perror("Can't read max_brightness");
		goto lcd_truely_1p3828_config_480_800_exit;
	} else {
		lcd_brightness = atoi(buf) / 2;
	}

	pwm_fd2 = open(TRUELY_1P3828_PWM_PATH("brightness"), O_WRONLY);
	if (pwm_fd2 < 0) {
		perror("Can't open brightness to write");
		goto lcd_truely_1p3828_config_480_800_exit;
	}
	sprintf(buf, "%d", lcd_brightness);
	ret = write(pwm_fd2, buf, sizeof(buf));
	close(pwm_fd2);
	if (ret <= 0) {
		perror("Can't write brightness");
		goto lcd_truely_1p3828_config_480_800_exit;
	}
#endif

lcd_truely_1p3828_config_480_800_exit:
	return;
}

/* ========================================================================== */
static int lcd_truely_1p3828_480_800(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode			= AMBA_VIDEO_MODE_480_800;
	pvout_cfg->ratio		= AMBA_VIDEO_RATIO_16_9;
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
	pvout_cfg->lcd_cfg.dclk_edge	= AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 27000000;
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_1P3828;

	lcd_truely_1p3828_config_480_800();

	return 0;
}

int lcd_1p3828_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_480_800:
		errCode = lcd_truely_1p3828_480_800(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
