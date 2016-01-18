/**
 * system/src/bst/corefreq.S
 *
 * History:
 *    2006/07/27 - [Charles Chiou] created file
 *
 * The implementation of the core frequency adjustment used to reside in
 * the 2nd-stage boot-loader. It worked fine. However, when the system needs
 * to switch from a POWER_ON_CONFIG core frequency setting to another fixed
 * frequency, it is best that we do this as early as possible to speed up
 * the process. The added benefit is that we can run it out of the FIO 2KB
 * FIFO and by-pass the DRAM initialization stage.
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

.text

.globl switch_core_freq

#if	defined(__FPGA__)

switch_core_freq:
	mov	pc, lr

#elif	(CHIP_REV == A1)

/**
 * Check the PLL settings and repgram the RCT registers to adjust
 * core frequency if necessary.
 *
 * Clobbered registers:
 *	r0 - r2
 */
switch_core_freq:
	/* If the constant is 0xffff, then we use the */
	/* POWER_ON_CONFIG default and return to the caller */
	ldr	r2, expect_pll_core_val
	mov	r1, #0xff00
	orr	r1, r1, #0x00ff
	cmp	r2, r1
	moveq	pc, lr		@ return to caller

	/* If CG_SETTING_CHANGED_REG is non-zero, then we skip the */
	/* RCT programming */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #CG_SETTING_CHANGED_OFFSET]
	cmp	r1, #0x0
	bne	_done

	/* Check if the PLL_CORE register value equals to the constant */
	/* that we expect. If not, then we program it with the desired */
	/* value and trigger a chip reset */
	ldrh	r1, [r0, #PLL_CORE_OFFSET]
	cmp	r1, r2
	moveq	pc, lr		@ return to caller
	strh	r2, [r0, #PLL_CORE_OFFSET]

	/* Delay loop */
	mov	r1, #100
_loop1:
	subs	r1, r1, #0x1
	bne	_loop1

	/* Reset... */
	mov	r1, #CG_SETTING_CHANGED_CORE_CLK
	orr	r1, r1, #CG_SETTING_CHANGED_CORE_PLL
	strh	r1, [r0, #CG_SETTING_CHANGED_OFFSET]

	/* The chip should start reset at this point */
	mov	r1, #0x0
	mcr	p15, 0, r1, c7, c0, 4

_done:
	/* Clear the CG_SETTING_CHANGED register */
	mov	r1, #0x0
	strh	r1, [r0, #CG_SETTING_CHANGED_OFFSET]

	mov	pc, lr

expect_pll_core_val:	.word	EXPECT_PLL_CORE_VAL

#elif	((CHIP_REV == A2) || (CHIP_REV == A3))

#if	defined(CORE_PLL_FINE_ADJUST)
custom_pll_mask:	.word	0xfff00000
custom_pll_val: 	.word	CORE_PLL_FINE_ADJUST
pll_183mhz_val:		.word	PLL_CORE_182MHZ_VAL
pll_189mhz_val: 	.word	PLL_CORE_189MHZ_VAL
pll_216mhz_val:		.word	PLL_CORE_216MHZ_VAL
pll_243mhz_val:		.word	PLL_CORE_243MHZ_VAL
#endif

/**
 * Check the PLL settings and repgram the RCT registers to adjust
 * core frequency if necessary.
 *
 * Clobbered registers:
 *	r0 - r2
 */
switch_core_freq:

#if	defined(CORE_PLL_FINE_ADJUST)
	/* load core PLL to r1. */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #PLL_CORE_CTRL_OFFSET]

	/* If core pll is not 183mhz, 216mhz, 243mhz, skip change pll. */
	ldr	r2, pll_183mhz_val
	cmp	r1, r2
	beq	change_pll
	ldr	r2, pll_189mhz_val
	cmp	r1, r2
	beq	change_pll
	ldr	r2, pll_216mhz_val
	cmp	r1, r2
	beq	change_pll
	ldr	r2, pll_243mhz_val
	cmp	r1, r2
	beq	change_pll
	bne	skip_change_pll

	/* Change core pll. */
change_pll:
	ldr	r2, custom_pll_mask
	and	r1, r1, r2
	ldr	r2, custom_pll_val
	orr	r1, r1, r2
	str	r1, [r0, #PLL_CORE_CTRL_OFFSET]

skip_change_pll:
#endif

#if	defined(CONFIG_USE_ALTERNATIVE_CORE_PLL)
	/* Use alternative core PLL */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #PLL_CORE_CTRL_OFFSET]
#if	defined(CONFIG_USE_CUSTOM_216MHZ_PLL)
	ldr	r2, default_216mhz_pll_val
	cmp	r1, r2
	bne	__done_alternative_pll
	ldr	r2, custom_216mhz_pll_val
	str	r2, [r0, #PLL_CORE_CTRL_OFFSET]
#endif
__done_alternative_pll:
#endif

	/* If the constant is 0xffff, then we use the */
	/* POWER_ON_CONFIG default and return to the caller */
	ldr	r2, expect_pll_core_val
	mov	r1, #0xff00
	orr	r1, r1, #0x00ff
	cmp	r2, r1
	moveq	pc, lr		@ return to caller

	/* Check if the PLL_CORE_CTR register value equals to the constant */
	/* that we expect. If not, then we program it with the desired */
	/* value and trigger a chip reset */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #PLL_CORE_CTRL_OFFSET]
	cmp	r1, r2
	moveq	pc, lr		@ return to caller
	str	r2, [r0, #PLL_CORE_CTRL_OFFSET]

	mov	pc, lr

expect_pll_core_val:	.word	EXPECT_PLL_CORE_VAL

#if	defined(CONFIG_USE_ALTERNATIVE_CORE_PLL)
#if	defined(CONFIG_USE_CUSTOM_216MHZ_PLL)
default_216mhz_pll_val:	.word	PLL_CORE_216MHZ_VAL
custom_216mhz_pll_val:	.word	CUSTOM_216MHZ_PLL
#endif
#endif

#elif	((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	 (CHIP_REV == A5) || (CHIP_REV == A5L))
/* Select the source of gclk_core
	0: Use pll_out_core
	1: Use pll_vco_out_core
*/

#define USE_PLL_CORE_VCO_FOR_CLK_CORE	0

/**
 * Check the PLL settings and repgram the RCT registers to adjust
 * core frequency if necessary.
 *
 * Clobbered registers:
 *	r0 - r2
 */
switch_core_freq:

	/* If the constant is 0xffffffff, then we use the */
	/* POWER_ON_CONFIG default and return to the caller */
	ldr	r2, expect_pll_core_val
	ldr	r1, =0xffffffff
	cmp	r2, r1
	moveq	pc, lr		@ return to caller

	/* Check if the PLL_CORE_CTR register value equals to the constant */
	/* that we expect. If not, then we program it with the desired */
	/* value and trigger a chip reset */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #PLL_CORE_CTRL_OFFSET]

	cmp	r1, r2
	moveq	pc, lr		@ return to caller

	ldr	r2, pll_core_ctrl2_val
	str	r2, [r0, #PLL_CORE_CTRL2_OFFSET]
	ldr	r2, pll_core_ctrl3_val
	str	r2, [r0, #PLL_CORE_CTRL3_OFFSET]
#if 	(CHIP_REV == A5L)
	ldr	r2, pll_core_scaler_val
	str	r2, [r0, #SCALER_CORE_POST_OFFSET]
	
	ldr	r2, pll_arm_scaler_val
	str	r2, [r0, #SCALER_ARM_ASYNC_OFFSET]
#endif

	ldr	r2, expect_pll_core_val
	bic	r2, r2, #0x1
	str	r2, [r0, #PLL_CORE_CTRL_OFFSET]

#if 	(CHIP_REV == A5L)
	@ldr	r2, gclk_core_clk_src_sel
	@str	r2, [r0, #USE_PLL_CORE_VCO_FOR_CLK_CORE_OFFSET]
#endif
	orr	r2, r2, #0x1
	str	r2, [r0, #PLL_CORE_CTRL_OFFSET]

	mov	pc, lr		@ return to caller

expect_pll_core_val:	.word	EXPECT_PLL_CORE_VAL

pll_core_ctrl2_val:	.word	0x3f710000
pll_core_ctrl3_val:	.word	0x00068300

#if 	(CHIP_REV == A5L)
pll_arm_scaler_val:	.word	PLL_ARM_SCALER_VAL
pll_core_scaler_val:	.word 	PLL_CORE_SCALER_VAL

/* Select the source of gclk_core
	0: Use pll_out_core
	1: Use pll_vco_out_core
*/
gclk_core_clk_src_sel:	.word 	0
#endif

#elif	(CHIP_REV == A6)
/*
 * The jump from low core frequency to high core frequency is not stable.
 * The stepping method to increase core frequency is necessary.
 * Due to the 2K size is too small to implement swtich_core_frequency with
 * stepping method, we do switch_core_frequency in bld.
 */
switch_core_freq:
	mov	pc, lr

#elif	(CHIP_REV == A5S)

/*
 * The HAL API should be used to switch core frequencies, so this
 * function essentially becomes a NOP.
 */
switch_core_freq:
	mov	pc, lr

#elif	(CHIP_REV == S2)

/*
 * The HAL API should be used to switch core frequencies, so this
 * function essentially becomes a NOP.
 */
switch_core_freq:
	mov	pc, lr

#else
#error "Unsupported architecure!"
#endif
