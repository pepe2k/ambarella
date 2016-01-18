/**
 * system/src/bld/diag_gpio.c
 *
 * History:
 *    2005/09/09 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

void diag_gpio_set(int gpio, int on)
{
	gpio_config_sw_out(gpio);
	if (on) {
		gpio_set(gpio);
	} else {
		gpio_clr(gpio);
	}
}

void diag_gpio_get(int gpio)
{
	u32					gpio_val;

	gpio_config_sw_in(gpio);
	gpio_val = gpio_get(gpio);

	uart_putstr("gpio[");
	uart_putdec(gpio);
	uart_putstr("] = ");
	uart_putdec(gpio_val);
	uart_putstr("\r\n");
}

void diag_gpio_hw(int gpio)
{
	gpio_config_hw(gpio);
}

void diag_gpio_pull(int gpio, int val)
{
	u32					gpio_pull_en_base = 0;
	u32					gpio_pull_dir_base = 0;
	u32					gpio_offset;
	u32					en_set = 0;
	u32					en_clr = 0;
	u32					dir_set = 0;
	u32					dir_clr = 0;
	u32					gpio_pull_en;
	u32					gpio_pull_dir;

#if (RTC_SUPPORT_GPIO_PAD_PULL_CTRL == 1)
	if ((gpio >= 0) && (gpio < 32)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_0_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_0_REG;
	} else if ((gpio >= 32) && (gpio < 64)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_1_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_1_REG;
	} else if ((gpio >= 64) && (gpio < 96)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_2_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_2_REG;
	} else if ((gpio >= 96) && (gpio < 128)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_3_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_3_REG;
	} else if ((gpio >= 128) && (gpio < 160)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_4_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_4_REG;
	} else if ((gpio >= 160) && (gpio < 192)) {
		gpio_pull_en_base = RTC_GPIO_PULL_EN_5_REG;
		gpio_pull_dir_base = RTC_GPIO_PULL_DIR_5_REG;
	}
#endif
	gpio_offset = gpio % 32;
	switch(val & 0x02) {
	case 0x02:
		en_set = (0x01 << gpio_offset);
		break;
	default:
		en_clr = (0x01 << gpio_offset);
		break;
	}
	switch(val & 0x01) {
	case 0x01:
		dir_set = (0x01 << gpio_offset);
		break;
	default:
		dir_clr = (0x01 << gpio_offset);
		break;
	}

	if (gpio_pull_en_base && gpio_pull_dir_base) {
		gpio_pull_en = readl(gpio_pull_en_base);
		gpio_pull_en |= en_set;
		gpio_pull_en &= ~en_clr;

		gpio_pull_dir = readl(gpio_pull_dir_base);
		gpio_pull_dir |= dir_set;
		gpio_pull_dir &= ~dir_clr;

		writel(gpio_pull_dir_base, gpio_pull_dir);
		writel(gpio_pull_en_base, gpio_pull_en);
	}
}

