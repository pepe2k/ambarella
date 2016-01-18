/*
 * lcd_1p3831.c
 *
 * History:
 *	2011/04/20 - [Zhenwu Xue] created file
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
#define TRUELY_1P3831_PWM_PATH(x)	"/sys/class/backlight/pwm-backlight.0/"x

typedef struct {
	u16		addr;
	u8		val;
} reg_t;

static const reg_t TRUELY_1P3831_REGS[] =
{
	/* Power Control */
	{0xc000, 0x86},
	{0xc001, 0x00},
	{0xc002, 0x86},
	{0xc003, 0x00},
	{0xc100, 0x40},
	{0xc200, 0x12},
	{0xc202, 0x42},

	/* Gamma Control */
	{0xe000, 0x0e},
	{0xe001, 0x2a},
	{0xe002, 0x33},
	{0xe003, 0x3b},
	{0xe004, 0x1e},
	{0xe005, 0x30},
	{0xe006, 0x64},
	{0xe007, 0x3f},
	{0xe008, 0x21},
	{0xe009, 0x27},
	{0xe00a, 0x88},
	{0xe00b, 0x14},
	{0xe00c, 0x35},
	{0xe00d, 0x56},
	{0xe00e, 0x79},
	{0xe00f, 0xb8},
	{0xe010, 0x55},
	{0xe011, 0x57},
	{0xe100, 0x0e},
	{0xe101, 0x2a},
	{0xe102, 0x33},
	{0xe103, 0x3b},
	{0xe104, 0x1e},
	{0xe105, 0x30},
	{0xe106, 0x64},
	{0xe107, 0x3f},
	{0xe108, 0x21},
	{0xe109, 0x27},
	{0xe10a, 0x88},
	{0xe10b, 0x14},
	{0xe10c, 0x35},
	{0xe10d, 0x56},
	{0xe10e, 0x79},
	{0xe10f, 0xb8},
	{0xe110, 0x55},
	{0xe111, 0x57},
	{0xe200, 0x0e},
	{0xe201, 0x2a},
	{0xe202, 0x33},
	{0xe203, 0x3b},
	{0xe204, 0x1e},
	{0xe205, 0x30},
	{0xe206, 0x64},
	{0xe207, 0x3f},
	{0xe208, 0x21},
	{0xe209, 0x27},
	{0xe20a, 0x88},
	{0xe20b, 0x14},
	{0xe20c, 0x35},
	{0xe20d, 0x56},
	{0xe20e, 0x79},
	{0xe20f, 0xb8},
	{0xe210, 0x55},
	{0xe211, 0x57},
	{0xe300, 0x0e},
	{0xe301, 0x2a},
	{0xe302, 0x33},
	{0xe303, 0x3b},
	{0xe304, 0x1e},
	{0xe305, 0x30},
	{0xe306, 0x64},
	{0xe307, 0x3f},
	{0xe308, 0x21},
	{0xe309, 0x27},
	{0xe30a, 0x88},
	{0xe30b, 0x14},
	{0xe30c, 0x35},
	{0xe30d, 0x56},
	{0xe30e, 0x79},
	{0xe30f, 0xb8},
	{0xe310, 0x55},
	{0xe311, 0x57},
	{0xe400, 0x0e},
	{0xe401, 0x2a},
	{0xe402, 0x33},
	{0xe403, 0x3b},
	{0xe404, 0x1e},
	{0xe405, 0x30},
	{0xe406, 0x64},
	{0xe407, 0x3f},
	{0xe408, 0x21},
	{0xe409, 0x27},
	{0xe40a, 0x88},
	{0xe40b, 0x14},
	{0xe40c, 0x35},
	{0xe40d, 0x56},
	{0xe40e, 0x79},
	{0xe40f, 0xb8},
	{0xe410, 0x55},
	{0xe411, 0x57},
	{0xe500, 0x0e},
	{0xe501, 0x2a},
	{0xe502, 0x33},
	{0xe503, 0x3b},
	{0xe504, 0x1e},
	{0xe505, 0x30},
	{0xe506, 0x64},
	{0xe507, 0x3f},
	{0xe508, 0x21},
	{0xe509, 0x27},
	{0xe50a, 0x88},
	{0xe50b, 0x14},
	{0xe50c, 0x35},
	{0xe50d, 0x56},
	{0xe50e, 0x79},
	{0xe50f, 0xb8},
	{0xe510, 0x55},
	{0xe511, 0x57},

	/* RGB Interface Format */
	{0x3a00, 0x07},
	{0x3b00, 0x03},
};

static void truely_1p3831_write_cmd(int spi_fd, u16 cmd)
{
	u16	data;

	/* Address High Byte */
	data = 0x2000 | (cmd >> 8);
	write(spi_fd, &data, sizeof(data));

	/* Address Low Byte */
	data = 0x0000 | (cmd & 0xff);
	write(spi_fd, &data, sizeof(data));
}

static void truely_1p3831_write_cmd_data(int spi_fd, reg_t reg)
{
	u16	data;

	/* Address High Byte */
	data = 0x2000 | (reg.addr >> 8);
	write(spi_fd, &data, sizeof(data));

	/* Address Low Byte */
	data = 0x0000 | (reg.addr & 0xff);
	write(spi_fd, &data, sizeof(data));

	/* Data */
	data = 0x4000 | reg.val;
	write(spi_fd, &data, sizeof(data));
}

/* ========================================================================== */
static void truely_1p3831_set_spi_bus(int fd)
{
	u8	mode	= 0;		/* CPHA = 0, CPOL = 0 */
	u8	bits	= 16;		/* bits per word is 16 */
	u32	speed	= 500000;	/* SSI_CLK = 500KHz, can speed up if necessary */

	ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

static void lcd_truely_1p3831_config_480_800()
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

	/* Program TRUELY_1P3831 to output 480x800 */
	if (lcd_spi_dev_node(buf)) {
		perror("Unable to get lcd spi bus_id or cs_id!\n");
		goto lcd_truely_1p3831_config_480_800_exit;
	}

	spi_fd = open(buf, O_WRONLY);
	if (spi_fd < 0) {
		perror("Can't open TRUELY_1P3831_SPI_DEV_NODE to write");
		goto lcd_truely_1p3831_config_480_800_exit;
	} else {
		truely_1p3831_set_spi_bus(spi_fd);
	}

	truely_1p3831_write_cmd(spi_fd, 0x1100);	//Sleep Out
	MSLEEP(200);

	for (i = 0; i < sizeof(TRUELY_1P3831_REGS) / sizeof(reg_t); i++) {
		truely_1p3831_write_cmd_data(spi_fd, TRUELY_1P3831_REGS[i]);
	}

	truely_1p3831_write_cmd(spi_fd, 0x2900);	//Display On
	MSLEEP(200);

	close(spi_fd);

#ifndef LCD_BRIGHEST
	/* Backlight on */
	pwm_fd1 = open(TRUELY_1P3831_PWM_PATH("max_brightness"), O_RDONLY);
	if (pwm_fd1 < 0) {
		perror("Can't open max_brightness to read");
		goto lcd_truely_1p3831_config_480_800_exit;
	}
	ret = read(pwm_fd1, buf, sizeof(buf));
	close(pwm_fd1);
	if (ret <= 0) {
		perror("Can't read max_brightness");
		goto lcd_truely_1p3831_config_480_800_exit;
	} else {
		lcd_brightness = atoi(buf) / 2;
	}

	pwm_fd2 = open(TRUELY_1P3831_PWM_PATH("brightness"), O_WRONLY);
	if (pwm_fd2 < 0) {
		perror("Can't open brightness to write");
		goto lcd_truely_1p3831_config_480_800_exit;
	}
	sprintf(buf, "%d", lcd_brightness);
	ret = write(pwm_fd2, buf, sizeof(buf));
	close(pwm_fd2);
	if (ret <= 0) {
		perror("Can't write brightness");
		goto lcd_truely_1p3831_config_480_800_exit;
	}
#endif

lcd_truely_1p3831_config_480_800_exit:
	return;
}

/* ========================================================================== */
static int lcd_truely_1p3831_480_800(struct amba_video_sink_mode *pvout_cfg)
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
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_1P3831;

	lcd_truely_1p3831_config_480_800();

	return 0;
}

int lcd_1p3831_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_480_800:
		errCode = lcd_truely_1p3831_480_800(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
