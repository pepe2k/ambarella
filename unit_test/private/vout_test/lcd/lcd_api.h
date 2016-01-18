/*
 * lcd_api.h
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

#ifndef __LCD_API_H__
#define __LCD_API_H__

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "basetypes.h"

#include "iav_drv.h"
#include "ambas_vout.h"

#define GPIO(x)				(x)

/******************************************/
typedef int (*LCD_SETMODE_FUNC)(int mode, struct amba_video_sink_mode *pcfg);
typedef int (*LCD_POST_SETMODE_FUNC)(int mode);


/* ========================================================================== */
static void gpio_set(u8 gpio_id)
{
	int _export, direction, unexport;
	char buf[128];

	_export = open("/sys/class/gpio/export", O_WRONLY);
	if (_export < 0) {
		printf("%s: Can't open export sys file!\n", __func__);
		goto gpio_set_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(_export, buf, sizeof(buf));
	close(_export);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_set_exit;
	}
	sprintf(buf, "high");
	write(direction, buf, sizeof(buf));
	close(direction);

	unexport = open("/sys/class/gpio/unexport", O_WRONLY);
	if (unexport < 0) {
		printf("%s: Can't open unexport sys file!\n", __func__);
		goto gpio_set_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(unexport, buf, sizeof(buf));
	close(unexport);

gpio_set_exit:
	return;
}

static void gpio_clr(u8 gpio_id)
{
	int _export, direction, unexport;
	char buf[128];

	_export = open("/sys/class/gpio/export", O_WRONLY);
	if (_export < 0) {
		printf("%s: Can't open export sys file!\n", __func__);
		goto gpio_clr_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(_export, buf, sizeof(buf));
	close(_export);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_clr_exit;
	}
	sprintf(buf, "low");
	write(direction, buf, sizeof(buf));
	close(direction);

	unexport = open("/sys/class/gpio/unexport", O_WRONLY);
	if (unexport < 0) {
		printf("%s: Can't open unexport sys file!\n", __func__);
		goto gpio_clr_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(unexport, buf, sizeof(buf));
	close(unexport);

gpio_clr_exit:
	return;
}

#define	PARAMETERS_PATH1(param)	"/sys/module/ambarella_config/parameters/"param
#define	PARAMETERS_PATH2(param)	"/sys/module/board/parameters/"param
#define MAX_STR_LEN		64

typedef struct {
	int	fd;
	int	value;
} lcd_parameter_t;

static void lcd_reset(void)
{
	lcd_parameter_t		gpio, level, delay;
	char			buf[MAX_STR_LEN];
	int			size;

	gpio.fd = open(PARAMETERS_PATH1("board_lcd_reset_gpio_id"), O_RDONLY);
	if (gpio.fd < 0) {
		gpio.fd = open(PARAMETERS_PATH2("board_lcd_reset_gpio_id"), O_RDONLY);
	}
	if (gpio.fd < 0) {
		perror("Unable to read lcd reset gpio id");
		goto lcd_reset_exit;
	}
	size = read(gpio.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd reset gpio id");
		goto lcd_reset_exit;
	}
	buf[size] = '\0';
	gpio.value = atoi(buf);

	level.fd = open(PARAMETERS_PATH1("board_lcd_reset_active_level"), O_RDONLY);
	if (level.fd < 0) {
		level.fd = open(PARAMETERS_PATH2("board_lcd_reset_active_level"), O_RDONLY);
	}
	if (level.fd < 0) {
		perror("Unable to read lcd reset active level");
		goto lcd_reset_exit;
	}
	size = read(level.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd reset active level");
		goto lcd_reset_exit;
	}
	buf[size] = '\0';
	level.value = atoi(buf);

	delay.fd = open(PARAMETERS_PATH1("board_lcd_reset_active_delay"), O_RDONLY);
	if (delay.fd < 0) {
		delay.fd = open(PARAMETERS_PATH2("board_lcd_reset_active_delay"), O_RDONLY);
	}
	if (delay.fd < 0) {
		perror("Unable to read lcd reset active delay");
		goto lcd_reset_exit;
	}
	size = read(delay.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd reset active delay");
		goto lcd_reset_exit;
	}
	buf[size] = '\0';
	delay.value = atoi(buf);

	if (gpio.value < 0 || level.value < 0 || delay.value < 0) {
		goto lcd_reset_exit;
	}

	if (level.value) {
		gpio_set(gpio.value);
		usleep(delay.value << 10);
		gpio_clr(gpio.value);
		usleep(delay.value << 10);
	} else {
		gpio_clr(gpio.value);
		usleep(delay.value << 10);
		gpio_set(gpio.value);
		usleep(delay.value << 10);
	}

lcd_reset_exit:
	return;
}

static void lcd_power_on(void)
{
	lcd_parameter_t		gpio, level, delay;
	char			buf[MAX_STR_LEN];
	int			size;

	gpio.fd = open(PARAMETERS_PATH1("board_lcd_power_gpio_id"), O_RDONLY);
	if (gpio.fd < 0) {
		gpio.fd = open(PARAMETERS_PATH2("board_lcd_power_gpio_id"), O_RDONLY);
	}
	if (gpio.fd < 0) {
		perror("Unable to read lcd power gpio id");
		goto lcd_power_on_exit;
	}
	size = read(gpio.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd power gpio id");
		goto lcd_power_on_exit;
	}
	buf[size] = '\0';
	gpio.value = atoi(buf);

	level.fd = open(PARAMETERS_PATH1("board_lcd_power_active_level"), O_RDONLY);
	if (level.fd < 0) {
		level.fd = open(PARAMETERS_PATH2("board_lcd_power_active_level"), O_RDONLY);
	}
	if (level.fd < 0) {
		perror("Unable to read lcd power active level");
		goto lcd_power_on_exit;
	}
	size = read(level.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd power active level");
		goto lcd_power_on_exit;
	}
	buf[size] = '\0';
	level.value = atoi(buf);

	delay.fd = open(PARAMETERS_PATH1("board_lcd_power_active_delay"), O_RDONLY);
	if (delay.fd < 0) {
		delay.fd = open(PARAMETERS_PATH2("board_lcd_power_active_delay"), O_RDONLY);
	}
	if (delay.fd < 0) {
		perror("Unable to read lcd power active delay");
		goto lcd_power_on_exit;
	}
	size = read(delay.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd power active delay");
		goto lcd_power_on_exit;
	}
	buf[size] = '\0';
	delay.value = atoi(buf);

	if (gpio.value < 0 || level.value < 0 || delay.value < 0) {
		goto lcd_power_on_exit;
	}

	if (level.value) {
		gpio_set(gpio.value);
		usleep(delay.value << 10);
	} else {
		gpio_clr(gpio.value);
		usleep(delay.value << 10);
	}

lcd_power_on_exit:
	return;
}

static void lcd_backlight_on(void)
{
	lcd_parameter_t		gpio, level, delay;
	char			buf[MAX_STR_LEN];
	int			size;

	gpio.fd = open(PARAMETERS_PATH1("board_lcd_backlight_gpio_id"), O_RDONLY);
	if (gpio.fd < 0) {
		gpio.fd = open(PARAMETERS_PATH2("board_lcd_backlight_gpio_id"), O_RDONLY);
	}
	if (gpio.fd < 0) {
		perror("Unable to read lcd backlight gpio id");
		goto lcd_backlight_on_exit;
	}
	size = read(gpio.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd backlight gpio id");
		goto lcd_backlight_on_exit;
	}
	buf[size] = '\0';
	gpio.value = atoi(buf);

	level.fd = open(PARAMETERS_PATH1("board_lcd_backlight_active_level"), O_RDONLY);
	if (level.fd < 0) {
		level.fd = open(PARAMETERS_PATH2("board_lcd_backlight_active_level"), O_RDONLY);
	}
	if (level.fd < 0) {
		perror("Unable to read lcd backlight active level");
		goto lcd_backlight_on_exit;
	}
	size = read(level.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd backlight active level");
		goto lcd_backlight_on_exit;
	}
	buf[size] = '\0';
	level.value = atoi(buf);

	delay.fd = open(PARAMETERS_PATH1("board_lcd_backlight_active_delay"), O_RDONLY);
	if (delay.fd < 0) {
		delay.fd = open(PARAMETERS_PATH2("board_lcd_backlight_active_delay"), O_RDONLY);
	}
	if (delay.fd < 0) {
		perror("Unable to read lcd backlight active delay");
		goto lcd_backlight_on_exit;
	}
	size = read(delay.fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read lcd backlight active delay");
		goto lcd_backlight_on_exit;
	}
	buf[size] = '\0';
	delay.value = atoi(buf);

	if (gpio.value < 0 || level.value < 0 || delay.value < 0) {
		goto lcd_backlight_on_exit;
	}

	if (level.value) {
		gpio_set(gpio.value);
		usleep(delay.value << 10);
	} else {
		gpio_clr(gpio.value);
		usleep(delay.value << 10);
	}

lcd_backlight_on_exit:
	return;
}

static int lcd_spi_dev_node(char *node)
{
	int			fd, errorCode = 0;
	int			bus_id, cs_id;
	char			buf[MAX_STR_LEN];
	int			size;

	fd = open(PARAMETERS_PATH1("board_lcd_spi_bus_id"), O_RDONLY);
	if (fd < 0) {
		fd = open(PARAMETERS_PATH2("board_lcd_spi_bus_id"), O_RDONLY);
	}
	if (fd < 0) {
		perror("Unable to open board_lcd_spi_bus_id");
		errorCode = -1;
		goto lcd_spi_dev_node_exit;
	}
	size = read(fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read board_lcd_spi_bus_id");
		errorCode = -1;
		goto lcd_spi_dev_node_exit;
	}
	buf[size] = '\0';
	bus_id = atoi(buf);
	close(fd);

	fd = open(PARAMETERS_PATH1("board_lcd_spi_cs_id"), O_RDONLY);
	if (fd < 0) {
		fd = open(PARAMETERS_PATH2("board_lcd_spi_cs_id"), O_RDONLY);
	}
	if (fd < 0) {
		perror("Unable to open board_lcd_spi_cs_id");
		errorCode = -1;
		goto lcd_spi_dev_node_exit;
	}
	size = read(fd, buf, MAX_STR_LEN);
	if (size <= 0) {
		perror("Unable to read board_lcd_spi_cs_id");
		errorCode = -1;
		goto lcd_spi_dev_node_exit;
	}
	buf[size] = '\0';
	cs_id = atoi(buf);

	size = sprintf(node, "/dev/spidev%d.%d", bus_id, cs_id);
	if (size <= 13) {
		errorCode = -1;
	}

lcd_spi_dev_node_exit:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	return errorCode;
}

#endif //__LCD_API_H__
