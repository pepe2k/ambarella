/*
 * lcd_wdf2440.c
 *
 * History:
 *	2011/01/28 - [Zhenwu Xue] created file
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
#define WDF2440_SPI_DEV_NODE		"/dev/spidev0.1"
#define WDF2440_PWM_PATH(x)		"/sys/class/backlight/pwm-backlight.0/"x

#define WDF2440_WRITE_REGISTERS(data)	\
	write(spi_fd, data, sizeof(data))

static const u16 data1[] = {
	0x00F6, /* 17.Interface control */
	0x0100,
	0x0101,
	0x0111,
	0x0100,
	0x00F0,	/* 1.MTP Control Test Key */
	0x015A,
	0x00F3,	/* 2.Power Conrol Register */
	0x0100,
	0x00F1,	/* 3.TEST KEY Control */
	0x015A,
	0x00FF,	/* 4.Logic Test Register2 */
	0x0100,
	0x0100,
	0x0100,
	0x0140,
	0x0011, /* 5.Sleep Out */
};

static const u16 data2[] = {
	0x00F3, /* 6.Power Control Register */
	0x0101,
	0x0100,
	0x0100,
	0x0101, //VC=0001=1.75V
	0x0122,
	0x00F4, /* 7.VCOM Control Register */
	0x0171,
	0x0171,
	0x016A,
	0x016A,
	0x0144,
	0x00F5,	/* 18.Source Output Control Register */
	0x0112,	//0x0102: one gamma, 0x0112:three gamma
	0x0100,
	0x0105,
	0x0100,
	0x0100,
	0x011F,
};

static const u16 data3[] = {
	0x00F3, /* 9.Power Control Register */
	0x0103,
};

static const u16 data4[] = {
	0x00FF, /* 10.EDS Test Register */
	0x0100,
	0x0100,
	0x0100,
	0x0160,
};

static const u16 data5[] = {
	0x00F3, /* 11.Power Control Register */
	0x0107,
};

static const u16 data6[] = {
	0x00FF, /* 12.EDS Test Register */
	0x0100,
	0x0100,
	0x0100,
	0x0170,
};

static const u16 data7[] = {
	0x00F3, /* 13.Power Control Register */
	0x010F,
	0x00FF, /* 14.EDS Test Register */
	0x0100,
	0x0100,
	0x0100,
	0x0178,
};

static const u16 data8[] = {
	0x00F3, /* 15.Power Control Register */
	0x011F,
};

static const u16 data9[] = {
	0x00F3, /* 16.Power Control Register */
	0x013F,
};

static const u16 data11[] = {
	0x002A, /* 19.Column Address Set */
	0x0100,
	0x0100,
	0x0100,
	0x01EF,
	0x002B, /* 20.Page Address Set */
	0x0100,
	0x0100,
	0x0101,
	0x018F,
	//wintek rgb gamma v0.6D09
	0x00F7, /*R */
	0x0180,
	0x0115,
	0x0100,
	0x0100,
	0x0103,
	0x010E,
	0x0111,
	0x0122,
	0x010A,
	0x0105,
	0x0106,
	0x0106,
	0x0101,
	0x0122,
	0x0123,

	0x00F8,
	0x01A3,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0111,
	0x011A,
	0x012C,
	0x010D,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0122,
	0x0120,

	0x00F9, /* G */
	0x0180,
	0x0113,
	0x0100,
	0x0100,
	0x0100,
	0x010A,
	0x010D,
	0x011E,
	0x010B,
	0x0105,
	0x0105,
	0x0105,
	0x0100,
	0x0122,
	0x0123,

	0x00FA,
	0x01A3,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0111,
	0x011A,
	0x012C,
	0x010D,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0122,
	0x0120,

	0x00FB, /* B */
	0x0180,
	0x0112,  //B,C,D,f, 0x112
	0x0100,
	0x0100,
	0x0101,
	0x010A,
	0x010C,
	0x0117,
	0x010A,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0122,
	0x0123,

	0x00FC,
	0x01A3,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0111,
	0x011A,
	0x012C,
	0x010D,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0122,
	0x0120,
};

static const u16 data12[] = {
	0x00F1, /* 23.TEST KEY Control Register */
	0x0100,
	0x00D9, /* 24.Power Control Register */
	0x0106,
	0x00F3, /* 24.Power Control Register */
	0x017F,
};

static const u16 data13[] = {
	0x00D9, /* 24.Power Control Register */
	0x0107,
	0x00FD, /* Gate Control Register, the number of lines = 400 */
	0x0111,
	0x0101,
	0x00F2, /* Display Control Register, to set the back and front porch of VS to 8 and 8 */
	0x011F,
	0x011F,
	0x0103,
	0x0108,
	0x0108,
	0x0108,
	0x0108,
	0x0100,
	0x0100,
	0x011F,
	0x011F,
	0x003A, /* 25.Interface Pixel Format */
	0x0177,
	0x0036, /* 26.Memory Data Access Control */
	0x0148,
	0x0029, /* 27.Display ON */
	0x00F3, /* 6.Power Control Register */
	0x017F,
	0x0100,
	0x0100,
	0x010A, //VCI=
	0x0133,
	0x017F,
	0x0166,
	0x012C,
};

/* ========================================================================== */
static void wdf2440_set_spi_bus(int fd)
{
	u8	mode	= 0;		/* CPHA = 0, CPOL = 0 */
	u8	bits	= 9;		/* bits per word is 9 */
	u32	speed	= 500000;	/* SSI_CLK = 500KHz, can speed up if necessary */

	ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

static void lcd_wdf2440_config_240_400()
{
	int			spi_fd;

#ifndef LCD_BRIGHEST
	int			pwm_fd1, pwm_fd2;
	int			lcd_brightness;
	int			ret;
	char			buf[64];
#endif

	/* Power On */
	lcd_power_on();

	/* Hardware Reset */
	lcd_reset();

#ifdef LCD_BRIGHEST
	/* Backlight on */
	lcd_backlight_on();
#endif

	/* Program WDF2440 to output 240x400 */
	spi_fd = open(WDF2440_SPI_DEV_NODE, O_WRONLY);
	if (spi_fd < 0) {
		perror("Can't open WDF2440_SPI_DEV_NODE to write");
		goto lcd_wdf2440_config_240_400_exit;
	} else {
		wdf2440_set_spi_bus(spi_fd);
	}
	WDF2440_WRITE_REGISTERS(data1);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data2);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data3);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data4);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data5);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data6);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data7);
	MSLEEP(10);
	WDF2440_WRITE_REGISTERS(data8);
	MSLEEP(20);
	WDF2440_WRITE_REGISTERS(data9);
	MSLEEP(30);
	WDF2440_WRITE_REGISTERS(data11);
	MSLEEP(30);
	WDF2440_WRITE_REGISTERS(data12);
	MSLEEP(30);
	WDF2440_WRITE_REGISTERS(data13);
	MSLEEP(20);
	close(spi_fd);

#ifndef LCD_BRIGHEST
	/* Backlight on */
	pwm_fd1 = open(WDF2440_PWM_PATH("max_brightness"), O_RDONLY);
	if (pwm_fd1 < 0) {
		perror("Can't open max_brightness to read");
		goto lcd_wdf2440_config_240_400_exit;
	}
	ret = read(pwm_fd1, buf, sizeof(buf));
	close(pwm_fd1);
	if (ret <= 0) {
		perror("Can't read max_brightness");
		goto lcd_wdf2440_config_240_400_exit;
	} else {
		lcd_brightness = atoi(buf) / 2;
	}

	pwm_fd2 = open(WDF2440_PWM_PATH("brightness"), O_WRONLY);
	if (pwm_fd2 < 0) {
		perror("Can't open brightness to write");
		goto lcd_wdf2440_config_240_400_exit;
	}
	sprintf(buf, "%d", lcd_brightness);
	ret = write(pwm_fd2, buf, sizeof(buf));
	close(pwm_fd2);
	if (ret <= 0) {
		perror("Can't write brightness");
		goto lcd_wdf2440_config_240_400_exit;
	}
#endif

lcd_wdf2440_config_240_400_exit:
	return;
}

/* ========================================================================== */
static int lcd_wdf2440_240_400(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode		= AMBA_VIDEO_MODE_240_400;
	pvout_cfg->ratio	= AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits		= AMBA_VIDEO_BITS_16;
	pvout_cfg->type		= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format	= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y	= 0x10;
	pvout_cfg->bg_color.cb	= 0x80;
	pvout_cfg->bg_color.cr	= 0x80;

	pvout_cfg->lcd_cfg.mode	= AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT;
	pvout_cfg->lcd_cfg.seqt	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 27000000;

	return 0;
}

int lcd_wdf2440_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_240_400:
		errCode = lcd_wdf2440_240_400(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

int lcd_wdf2440_post_setmode(int mode)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_240_400:
		lcd_wdf2440_config_240_400();
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
