/**
 * system/src/bld/gpio.S
 *
 * History:
 *    2005/09/03 - [Charles Chiou] created file
 *    2008/05/09 - [Charles Chiou] added macros specifying default DIR, MASK
 *    2009/10/15 - [Allen Wang] added macros for GPIO4.
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <ambhw/gpio.h>

#ifndef DEFAULT_GPIO0_AFSEL
#define DEFAULT_GPIO0_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO0_DIR
#define DEFAULT_GPIO0_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO0_MASK
#define DEFAULT_GPIO0_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO0_DATA
#define DEFAULT_GPIO0_DATA	0x00000000
#endif

#ifndef DEFAULT_GPIO1_AFSEL
#define DEFAULT_GPIO1_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO1_DIR
#define DEFAULT_GPIO1_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO1_MASK
#define DEFAULT_GPIO1_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO1_DATA
#define DEFAULT_GPIO1_DATA	0x00000000
#endif

#if (GPIO_INSTANCES >= 3)
#ifndef DEFAULT_GPIO2_AFSEL
#define DEFAULT_GPIO2_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO2_DIR
#define DEFAULT_GPIO2_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO2_MASK
#define DEFAULT_GPIO2_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO2_DATA
#define DEFAULT_GPIO2_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 4)
#ifndef DEFAULT_GPIO3_AFSEL
#define DEFAULT_GPIO3_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO3_DIR
#define DEFAULT_GPIO3_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO3_MASK
#define DEFAULT_GPIO3_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO3_DATA
#define DEFAULT_GPIO3_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 5)
#ifndef DEFAULT_GPIO4_AFSEL
#define DEFAULT_GPIO4_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO4_DIR
#define DEFAULT_GPIO4_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO4_MASK
#define DEFAULT_GPIO4_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO4_DATA
#define DEFAULT_GPIO4_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 6)
#ifndef DEFAULT_GPIO5_AFSEL
#define DEFAULT_GPIO5_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO5_DIR
#define DEFAULT_GPIO5_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO5_MASK
#define DEFAULT_GPIO5_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO5_DATA
#define DEFAULT_GPIO5_DATA	0x00000000
#endif
#endif

#if (RTC_SUPPORT_GPIO_PAD_PULL_CTRL == 1)
#ifndef DEFAULT_GPIO0_CTRL_ENA
#define DEFAULT_GPIO0_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO0_CTRL_DIR
#define DEFAULT_GPIO0_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_ENA
#define DEFAULT_GPIO1_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_DIR
#define DEFAULT_GPIO1_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_ENA
#define DEFAULT_GPIO2_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_DIR
#define DEFAULT_GPIO2_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_ENA
#define DEFAULT_GPIO3_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_DIR
#define DEFAULT_GPIO3_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_ENA
#define DEFAULT_GPIO4_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_DIR
#define DEFAULT_GPIO4_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_ENA
#define DEFAULT_GPIO5_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_DIR
#define DEFAULT_GPIO5_CTRL_DIR	0x00000000
#endif
#endif

#if (RCT_SUPPORT_IOCTRL_GPIO == 1)
#ifndef DEFAULT_IOCTRL_GPIO
#define DEFAULT_IOCTRL_GPIO	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_MISC1
#define DEFAULT_IOCTRL_MISC1	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_MISC2
#define DEFAULT_IOCTRL_MISC2	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SMIOA
#define DEFAULT_IOCTRL_SMIOA	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SMIOB
#define DEFAULT_IOCTRL_SMIOB	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SMIOC
#define DEFAULT_IOCTRL_SMIOC	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SMIOD
#define DEFAULT_IOCTRL_SMIOD	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_VD1
#define DEFAULT_IOCTRL_VD1	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SENSOR
#define DEFAULT_IOCTRL_SENSOR	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_STRIG
#define DEFAULT_IOCTRL_STRIG	0x00000001
#endif
#ifndef DEFAULT_IOCTRL_SDXC
#define DEFAULT_IOCTRL_SDXC	0x00000001
#endif
#ifndef DEFAULT_SDXC_PULL_CTRL
#define DEFAULT_SDXC_PULL_CTRL	0x00000000
#endif
#endif

.text

/**
 * Initiale the GPIO controller.
 */
.globl	gpio_init
gpio_init:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	ldr	r1, __gpio0_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio0_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio0_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio0_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
        ldr	r1, __gpio1_afsel_dval
        str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio1_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio1_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio1_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#if (GPIO_INSTANCES >= 3)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	ldr	r1, __gpio2_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio2_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio2_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio2_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#if (GPIO_MAX_LINES == 81)
	@ BWO-A2 ----------------------------
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, #0x1
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, #0x1
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ BWO-A2 ----------------------------
#endif	/* GPIO_MAX_LINES == 81 */
#endif	/* GPIO_INSTANCES >= 3 */
#if (GPIO_INSTANCES >= 4)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	ldr	r1, __gpio3_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio3_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio3_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio3_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 4 */
#if (GPIO_INSTANCES >= 5)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	ldr	r1, __gpio4_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio4_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio4_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio4_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 5 */
#if (GPIO_INSTANCES >= 6)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	ldr	r1, __gpio5_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio5_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio5_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio5_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 6 */

#if (RTC_SUPPORT_GPIO_PAD_PULL_CTRL == 1)
	mov	r0, #APB_BASE
	orr	r0, r0, #RTC_OFFSET
	ldr	r1, __gpio0_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_0_OFFSET]
	ldr	r1, __gpio0_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_0_OFFSET]
	ldr	r1, __gpio1_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_1_OFFSET]
	ldr	r1, __gpio1_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_1_OFFSET]
	ldr	r1, __gpio2_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_2_OFFSET]
	ldr	r1, __gpio2_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_2_OFFSET]
	ldr	r1, __gpio3_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_3_OFFSET]
	ldr	r1, __gpio3_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_3_OFFSET]
	ldr	r1, __gpio4_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_4_OFFSET]
	ldr	r1, __gpio4_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_4_OFFSET]
	ldr	r1, __gpio5_ctrl_ena_dval
	str	r1, [r0, #RTC_GPIO_PULL_EN_5_OFFSET]
	ldr	r1, __gpio5_ctrl_dir_dval
	str	r1, [r0, #RTC_GPIO_PULL_DIR_5_OFFSET]
#endif /* IO PAD CONTROL */

#if (RCT_SUPPORT_IOCTRL_GPIO == 1)
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, __ioctrl_gpio_dval
	str	r1, [r0, #IOCTRL_GPIO_OFFSET]
	ldr	r1, __ioctrl_misc1_dval
	str	r1, [r0, #IOCTRL_MISC1_OFFSET]
	ldr	r1, __ioctrl_misc2_dval
	str	r1, [r0, #IOCTRL_MISC2_OFFSET]
	ldr	r1, __ioctrl_smioa_dval
	str	r1, [r0, #IOCTRL_SMIOA_OFFSET]
	ldr	r1, __ioctrl_smiob_dval
	str	r1, [r0, #IOCTRL_SMIOB_OFFSET]
	ldr	r1, __ioctrl_smioc_dval
	str	r1, [r0, #IOCTRL_SMIOC_OFFSET]
	ldr	r1, __ioctrl_smiod_dval
	str	r1, [r0, #IOCTRL_SMIOD_OFFSET]
	ldr	r1, __ioctrl_vd1_dval
	str	r1, [r0, #IOCTRL_VD1_OFFSET]
	ldr	r1, __ioctrl_sensor_dval
	str	r1, [r0, #IOCTRL_SENSOR_OFFSET]
	ldr	r1, __ioctrl_strig_dval
	str	r1, [r0, #IOCTRL_STRIG_OFFSET]
	ldr	r1, __ioctrl_sdxc_dval
	str	r1, [r0, #IOCTRL_SDXC_OFFSET]
	ldr	r1, __sdxc_pull_ctrl_dval
	str	r1, [r0, #SDXC_PULL_CTRL_OFFSET]
#endif /* RCT_SUPPORT_IOCTRL_GPIO */
	mov	pc, lr

/**
 * Configure a GPIO pin to be a HW function pin.
 *
 * @param r0 - GPIO pin number
 * @returns - None
 */
.globl gpio_config_hw
gpio_config_hw:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio05
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio04
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio03
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio02
#endif
	cmp	r3, #31
	bhi	_gpio01
_gpio00:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO0_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO0_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
_gpio01:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO1_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO1_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio02:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO2_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO2_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio03:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO3_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO3_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio04:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO4_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO4_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio05:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #1
	@ read-modify-write GPIO5_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO5_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif

/**
 * Configure a GPIO pin to be a SW input pin.
 *
 * @param r0 - GPIO pin number
 * @returns - None
 */
.globl gpio_config_sw_in
gpio_config_sw_in:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio15
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio14
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio13
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio12
#endif
	cmp	r3, #31
	bhi	_gpio11
_gpio10:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO0_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO0_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO0_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
_gpio11:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO1_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO1_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO1_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio12:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO2_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO2_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO2_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio13:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO3_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO3_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO3_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio14:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO4_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO4_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO4_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio15:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO5_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO5_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO5_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif

/**
 * Configure a GPIO pin to be a SW output pin
 *
 * @param r0 - GPIO pin number
 * @returns - None
 */
.globl gpio_config_sw_out
gpio_config_sw_out:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio25
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio24
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio23
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio22
#endif
	cmp	r3, #31
	bhi	_gpio21
_gpio20:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO0_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO0_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO0_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
_gpio21:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO1_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO1_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO1_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio22:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO2_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO2_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO2_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio23:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO3_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO3_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO3_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio24:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO4_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO4_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO4_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio25:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO5_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO5_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO5_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	pc, lr
#endif

/**
 * Set GPIO.
 */
.globl gpio_set
gpio_set:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio35
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio34
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio33
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio32
#endif
	cmp	r3, #31
	bhi	_gpio31
_gpio30:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
_gpio31:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO1_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio32:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO2_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio33:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO3_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio34:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO4_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio35:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO5_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif

/**
 * Clear GPIO.
 */
.globl gpio_clr
gpio_clr:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio45
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio44
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio43
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio42
#endif
	cmp	r3, #31
	bhi	_gpio41
_gpio40:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
_gpio41:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO1_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio42:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO2_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio43:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO3_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio44:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO4_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio45:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #0x1
	@ read-modify-write GPIO5_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	pc, lr
#endif

/**
 * Get GPIO value.
 */
.globl gpio_get
gpio_get:
	mov	r3, r0
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	bhi	_gpio55
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	bhi	_gpio54
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	bhi	_gpio53
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	bhi	_gpio52
#endif
	cmp	r3, #31
	bhi	_gpio51
_gpio50:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret50
	mov	r0, #0x1
	mov	pc, lr
_ret50:
	mov	r0, #0x0
	mov	pc, lr
_gpio51:
	sub	r3, r3, #32
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO1_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret51
	mov	r0, #0x1
	mov	pc, lr
_ret51:
	mov	r0, #0x0
	mov	pc, lr
#if (GPIO_INSTANCES >= 3)
_gpio52:
	sub	r3, r3, #64
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO2_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret52
	mov	r0, #0x1
	mov	pc, lr
_ret52:
	mov	r0, #0x0
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
_gpio53:
	sub	r3, r3, #96
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO3_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret53
	mov	r0, #0x1
	mov	pc, lr
_ret53:
	mov	r0, #0x0
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
_gpio54:
	sub	r3, r3, #128
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO4_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret54
	mov	r0, #0x1
	mov	pc, lr
_ret54:
	mov	r0, #0x0
	mov	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
_gpio55:
	sub	r3, r3, #160
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO5_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	_ret55
	mov	r0, #0x1
	mov	pc, lr
_ret55:
	mov	r0, #0x0
	mov	pc, lr
#endif

__gpio0_afsel_dval:	.word	DEFAULT_GPIO0_AFSEL
__gpio0_dir_dval:	.word	DEFAULT_GPIO0_DIR
__gpio0_mask_dval:	.word	DEFAULT_GPIO0_MASK
__gpio0_data_dval:	.word	DEFAULT_GPIO0_DATA
__gpio1_afsel_dval:	.word	DEFAULT_GPIO1_AFSEL
__gpio1_dir_dval:	.word	DEFAULT_GPIO1_DIR
__gpio1_mask_dval:	.word	DEFAULT_GPIO1_MASK
__gpio1_data_dval:	.word	DEFAULT_GPIO1_DATA
#if (GPIO_INSTANCES >= 3)
__gpio2_afsel_dval:	.word	DEFAULT_GPIO2_AFSEL
__gpio2_dir_dval:	.word	DEFAULT_GPIO2_DIR
__gpio2_mask_dval:	.word	DEFAULT_GPIO2_MASK
__gpio2_data_dval:	.word	DEFAULT_GPIO2_DATA
#endif
#if (GPIO_INSTANCES >= 4)
__gpio3_afsel_dval:	.word	DEFAULT_GPIO3_AFSEL
__gpio3_dir_dval:	.word	DEFAULT_GPIO3_DIR
__gpio3_mask_dval:	.word	DEFAULT_GPIO3_MASK
__gpio3_data_dval:	.word	DEFAULT_GPIO3_DATA
#endif
#if (GPIO_INSTANCES >= 5)
__gpio4_afsel_dval:	.word	DEFAULT_GPIO4_AFSEL
__gpio4_dir_dval:	.word	DEFAULT_GPIO4_DIR
__gpio4_mask_dval:	.word	DEFAULT_GPIO4_MASK
__gpio4_data_dval:	.word	DEFAULT_GPIO4_DATA
#endif
#if (GPIO_INSTANCES >= 6)
__gpio5_afsel_dval:	.word	DEFAULT_GPIO5_AFSEL
__gpio5_dir_dval:	.word	DEFAULT_GPIO5_DIR
__gpio5_mask_dval:	.word	DEFAULT_GPIO5_MASK
__gpio5_data_dval:	.word	DEFAULT_GPIO5_DATA
#endif

#if (RTC_SUPPORT_GPIO_PAD_PULL_CTRL == 1)
__gpio0_ctrl_ena_dval:	.word	DEFAULT_GPIO0_CTRL_ENA
__gpio0_ctrl_dir_dval:	.word	DEFAULT_GPIO0_CTRL_DIR
__gpio1_ctrl_ena_dval:	.word	DEFAULT_GPIO1_CTRL_ENA
__gpio1_ctrl_dir_dval:	.word	DEFAULT_GPIO1_CTRL_DIR
__gpio2_ctrl_ena_dval:	.word	DEFAULT_GPIO2_CTRL_ENA
__gpio2_ctrl_dir_dval:	.word	DEFAULT_GPIO2_CTRL_DIR
__gpio3_ctrl_ena_dval:	.word	DEFAULT_GPIO3_CTRL_ENA
__gpio3_ctrl_dir_dval:	.word	DEFAULT_GPIO3_CTRL_DIR
__gpio4_ctrl_ena_dval:	.word	DEFAULT_GPIO4_CTRL_ENA
__gpio4_ctrl_dir_dval:	.word	DEFAULT_GPIO4_CTRL_DIR
__gpio5_ctrl_ena_dval:	.word	DEFAULT_GPIO5_CTRL_ENA
__gpio5_ctrl_dir_dval:	.word	DEFAULT_GPIO5_CTRL_DIR
#endif

#if (RCT_SUPPORT_IOCTRL_GPIO == 1)
__ioctrl_gpio_dval:	.word	DEFAULT_IOCTRL_GPIO
__ioctrl_misc1_dval:	.word	DEFAULT_IOCTRL_MISC1
__ioctrl_misc2_dval:	.word	DEFAULT_IOCTRL_MISC2
__ioctrl_smioa_dval:	.word	DEFAULT_IOCTRL_SMIOA
__ioctrl_smiob_dval:	.word	DEFAULT_IOCTRL_SMIOB
__ioctrl_smioc_dval:	.word	DEFAULT_IOCTRL_SMIOC
__ioctrl_smiod_dval:	.word	DEFAULT_IOCTRL_SMIOD
__ioctrl_vd1_dval:	.word	DEFAULT_IOCTRL_VD1
__ioctrl_sensor_dval:	.word	DEFAULT_IOCTRL_SENSOR
__ioctrl_strig_dval:	.word	DEFAULT_IOCTRL_STRIG
__ioctrl_sdxc_dval:	.word	DEFAULT_IOCTRL_SDXC
__sdxc_pull_ctrl_dval:	.word	DEFAULT_SDXC_PULL_CTRL
#endif

