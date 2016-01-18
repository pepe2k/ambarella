/*
 * lcd_e330qhd.c
 *
 * History:
 *	2012/05/11 - [Long Zhao] created file
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
#define ISL97901_ADDR 0x50
#define E330QHD_ADDR  0x7C

#ifndef I2C_SLAVE
#define I2C_SLAVE   0x0703
#endif
#ifndef I2C_TENBIT
#define I2C_TENBIT  0x0704
#endif

#define LED_DRIVENR_EN  GPIO(23)
#define D5V_PWR_EN      GPIO(104)
#define D5VP_PWR_EN     GPIO(105)
#define D3V3P_PWR_EN    GPIO(106)
#define D1V8P_PWR_EN    GPIO(107)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

struct reg_table {
	u8 reg;
	u8 data;
};

static const struct reg_table lcos_isl97901_regs[] = {
	{0x02, 0x04},
	{0x06, 0x06},

	{0x13, 0x2f},
	{0x14, 0x2f},
	{0x15, 0x2f},
	{0x16, 0x2f},

	{0x18, 0xff},
	{0x1e, 0x80},
};
#define ISL97901_REG_TABLE_SIZE		ARRAY_SIZE(lcos_isl97901_regs)

static const struct reg_table lcos_e330qhd_regs[] = {
	{0x01, 0x00},//24-bit RGB
	{0xd0, 0x01},//H Flip
	{0x0c, 0x00},//V Delay
	{0x0d, 0x00},//V Delay Offset
	{0x0e, 0x00},//H Delay
	{0x55, 0x11},//To display
};
#define LCOS_E330QHD_REG_TABLE_SIZE		ARRAY_SIZE(lcos_e330qhd_regs)

static int i2c_write(u8 slaveaddr, u8 subaddr, u8 data)
{
	int				i2c_fd;
	int				fd_opened = 0;
	u8				w_data[2];
	int				ret = 0;

	/* device */
	i2c_fd = open("/dev/i2c-0", O_RDWR, 0);
	if (i2c_fd < 0) {
		perror("Incorrect i2c_dev!\n");
		ret = -1;
		goto i2c_write_exit;
	}
	fd_opened = 1;

	/* 7-bit mode */
	ret = ioctl(i2c_fd, I2C_TENBIT, 0);
	if (ret < 0) {
		perror("Error: Unable to set"
				" 7-bit address mode!\n");
		goto i2c_write_exit;
	}

	/* set slave address */
	ret = ioctl(i2c_fd, I2C_SLAVE, slaveaddr >> 1);
	if (ret < 0) {
		perror("Error: Unable to set slave address!\n");
		goto i2c_write_exit;
	}

	/* write data */
	w_data[0] = subaddr;
	w_data[1] = data;

	ret = write(i2c_fd, &w_data, 2);
	if (ret != 2) {
		perror("Error: Unable to write i2c!\n");
		ret = -1;
		goto i2c_write_exit;
	}

i2c_write_exit:
	if (fd_opened)
		close(i2c_fd);
	return ret;
}

static void lcos_e330qhd_set_power()
{
	/* POWER ON: LED_DRIVER_EN */
	gpio_set(LED_DRIVENR_EN);
	/* POWER ON: 5V */
	gpio_set(D5V_PWR_EN);
	usleep(10000);
	/* POWER ON: P5V */
	gpio_set(D5VP_PWR_EN);
	usleep(10000);
	/* POWER ON: P3.3V */
	gpio_set(D3V3P_PWR_EN);
	usleep(10000);
	/* POWER ON: P1.8V */
	gpio_set(D1V8P_PWR_EN);
	usleep(10000);
}

static void lcos_e330qhd_set_hw()
{
	int i;
	const struct reg_table *reg_tbl;

	lcos_e330qhd_set_power();

	/* init ISL97901 */
	reg_tbl = lcos_isl97901_regs;
	for (i = 0; i < ISL97901_REG_TABLE_SIZE; i++) {
		i2c_write(ISL97901_ADDR, reg_tbl[i].reg, reg_tbl[i].data);
	}

	/* init E330qHD pannel */
	reg_tbl = lcos_e330qhd_regs;
	for (i = 0; i < LCOS_E330QHD_REG_TABLE_SIZE; i++) {
		i2c_write(E330QHD_ADDR, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

/* ========================================================================== */
static int lcos_e330qhd_config(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode			= AMBA_VIDEO_MODE_960_540;
	pvout_cfg->ratio		= AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits			= AMBA_VIDEO_BITS_8;
	pvout_cfg->type			= AMBA_VIDEO_TYPE_RGB_RAW;
	pvout_cfg->format		= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type		= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y		= 0x10;
	pvout_cfg->bg_color.cb		= 0x80;
	pvout_cfg->bg_color.cr		= 0x80;

	pvout_cfg->lcd_cfg.mode		= AMBA_VOUT_LCD_MODE_RGB888;
	pvout_cfg->lcd_cfg.seqt		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb		= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge	= AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 25000000;
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_E330QHD;

	lcos_e330qhd_set_hw();

	return 0;
}

int lcos_e330qhd_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_960_540:
		errCode = lcos_e330qhd_config(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}
