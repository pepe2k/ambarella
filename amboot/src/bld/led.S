/**
 * system/src/bld/led.S
 *
 * History:
 *    2005/01/26 - [Charles Chiou] created file
 *    2012/02/01 - [Jack Huang] rewrote led_blink function.
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
.text

.globl	led_init

/**
 * Initialize LED.
 */
led_init:
#if defined(DEBUG_LED_GPIO)
	mov	ip, sp
	stmfd	sp!, {r0 - r3, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r0, #DEBUG_LED_GPIO
	bl	gpio_config_sw_out
	ldmfd	sp, {r0 - r3, fp, sp, pc}
#else
	mov	pc, lr
#endif

/**
 * Turn on LED.
 */
led_on:
#if defined(DEBUG_LED_GPIO)
	mov	ip, sp
	stmfd	sp!, {r0 - r3, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r0, #DEBUG_LED_GPIO
	bl	gpio_set
	ldmfd	sp, {r0 - r3, fp, sp, pc}
	mov	pc, lr
#else
	mov	pc, lr
#endif

/**
 * Turn off LED.
 */
led_off:
#if defined(DEBUG_LED_GPIO)
	mov	ip, sp
	stmfd	sp!, {r0 - r3, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r0, #DEBUG_LED_GPIO
	bl	gpio_clr
	ldmfd	sp, {r0 - r3, fp, sp, pc}
	mov	pc, lr
#else
	mov	pc, lr
#endif

/* delay one second */
blink_one_second:
	stmfd	sp!, {r0 - r2, lr}
	mov	r0, #APB_BASE
	orr	r0, r0, #RTC_OFFSET
	ldr	r1, [r0, #RTC_CURT_OFFSET]
	bl	led_off
polling_rct_reg:
	ldr	r2, [r0, #RTC_CURT_OFFSET]
	cmp	r2, r1
	beq	polling_rct_reg
	bl	led_on
polling_rct_reg_:
	ldr	r1, [r0, #RTC_CURT_OFFSET]
	cmp	r1, r2
	beq	polling_rct_reg_
	bl	led_off
	ldmfd	sp!, {r0 - r2, pc}

/**
 * R0 ~ R2 are used for this procedure
 */
.globl	led_blink
led_blink:
	mov	r2, lr
	mov	r1, r0
blinking:
	bl	blink_one_second
	subs	r1, r1, #1
	bne	blinking
	mov	pc, r2
