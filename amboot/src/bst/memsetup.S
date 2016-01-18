/**
 * system/src/bst/memsetup.S
 *
 * History:
 *    2005/01/26 - [Charles Chiou] created file
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
.globl	memsetup

/*
 * Setup Powe-On-Config specific DDR2 constants.
 */
#if	defined(PWC_CORE_108MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____108MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_108MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_108MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_108MHZ
#elif	defined(PWC_CORE_135MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____135MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_135MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_135MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_135MHZ
#elif	defined(PWC_CORE_162MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____162MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_162MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_162MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_162MHZ
#elif	defined(PWC_CORE_182MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____182MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_182MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_182MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_182MHZ
#elif	defined(PWC_CORE_189MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____189MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_189MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_189MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_189MHZ
#elif	defined(PWC_CORE_216MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____216MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_216MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_216MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_216MHZ
#elif	defined(PWC_CORE_229MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____229MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_229MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_229MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_229MHZ
#elif	defined(PWC_CORE_243MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____243MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_243MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_243MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_243MHZ
#elif	defined(PWC_CORE_256MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____256MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_256MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_256MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_256MHZ
#elif	defined(PWC_CORE_270MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____270MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_270MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_270MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_270MHZ
#elif	defined(PWC_CORE_283MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____283MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_283MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_283MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_283MHZ
#else
#error "Unknown Power-On-Config Core Frequency!"
#endif

/*
 * Setup RCT specific DDR2 constants
 */
#if	defined(FIX_CORE_108MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____108MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_108MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_108MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_108MHZ
#elif	defined(FIX_CORE_135MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____135MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_135MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_135MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_135MHZ
#elif	defined(FIX_CORE_162MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____162MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_162MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_162MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_162MHZ
#elif	defined(FIX_CORE_182MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____182MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_182MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_182MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_182MHZ
#elif	defined(FIX_CORE_189MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____189MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_189MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_189MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_189MHZ
#elif	defined(FIX_CORE_216MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____216MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_216MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_216MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_216MHZ
#elif	defined(FIX_CORE_229MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____229MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_229MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_229MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_229MHZ
#elif	defined(FIX_CORE_243MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____243MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_243MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_243MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_243MHZ
#elif	defined(FIX_CORE_250MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____250MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_250MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_250MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_250MHZ
#elif	defined(FIX_CORE_256MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____256MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_256MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_256MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_256MHZ
#elif	defined(FIX_CORE_263MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____263MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_263MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_263MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_263MHZ
#elif	defined(FIX_CORE_270MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____270MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_270MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_270MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_270MHZ
#elif	defined(FIX_CORE_283MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____283MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_283MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_283MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_283MHZ
#elif	defined(FIX_CORE_290MHZ)
#define FIX_DRAM_MODE_VAL	DRAM_MODE____290MHZ
#define FIX_DRAM_TIMING1_VAL	DRAM_TIMING1_290MHZ
#define FIX_DRAM_TIMING2_VAL	DRAM_TIMING2_290MHZ
#define FIX_DRAM_REF_CTL_VAL	DRAM_REF_CTL_290MHZ
#else
#define FIX_DRAM_MODE_VAL	PWC_DRAM_MODE_VAL
#define FIX_DRAM_TIMING1_VAL	PWC_DRAM_TIMING1_VAL
#define FIX_DRAM_TIMING2_VAL	PWC_DRAM_TIMING2_VAL
#define FIX_DRAM_REF_CTL_VAL	PWC_DRAM_REF_CTL_VAL
#endif

glo_dram_cfg:		.word	DRAM_CFG_VAL
glo_dram_ext_mode:	.word	DRAM_EXT_MODE_VAL
pwc_dram_mode:		.word	PWC_DRAM_MODE_VAL
pwc_dram_timing1:	.word	PWC_DRAM_TIMING1_VAL
pwc_dram_timing2:	.word	PWC_DRAM_TIMING2_VAL
pwc_dram_ref_ctl:	.word	PWC_DRAM_REF_CTL_VAL
fix_dram_mode:		.word	FIX_DRAM_MODE_VAL
fix_dram_timing1:	.word	FIX_DRAM_TIMING1_VAL
fix_dram_timing2:	.word	FIX_DRAM_TIMING2_VAL
fix_dram_ref_ctl:	.word	FIX_DRAM_REF_CTL_VAL
expect_core_val:	.word	EXPECT_PLL_CORE_VAL

/*
 * Setup DLL value for FIX_CORE
 */
#if (CHIP_REV != A1)
#if	defined(FIX_CORE_108MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_108MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_108MHZ
#elif	defined(FIX_CORE_135MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_135MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_135MHZ
#elif	defined(FIX_CORE_162MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_162MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_162MHZ
#elif	defined(FIX_CORE_182MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_182MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_182MHZ
#elif	defined(FIX_CORE_189MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_189MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_189MHZ
#elif	defined(FIX_CORE_216MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_216MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_216MHZ
#elif	defined(FIX_CORE_229MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_230MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_230MHZ
#elif	defined(FIX_CORE_243MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_243MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_243MHZ
#elif	defined(FIX_CORE_250MHZ)
#error Unsupport DLL vaule in this fix core frequence
#elif	defined(FIX_CORE_256MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_256MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_256MHZ
#elif	defined(FIX_CORE_263MHZ)
#error Unsupport DLL vaule in this fix core frequence
#elif	defined(FIX_CORE_270MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_270MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_270MHZ
#elif	defined(FIX_CORE_283MHZ)
#define FIX_DRAM_DLL0_VAL	RCT_DLL0_283MHZ
#define FIX_DRAM_DLL1_VAL	RCT_DLL1_283MHZ
#elif	defined(FIX_CORE_290MHZ)
#error Unsupport DLL vaule in this fix core frequence
#else
#undef FIX_DRAM_DLL0_VAL
#undef FIX_DRAM_DLL1_VAL
#endif

#if defined(FIX_DRAM_DLL0_VAL) && defined(FIX_DRAM_DLL1_VAL)
fix_dram_dll0_val:	.word	FIX_DRAM_DLL0_VAL
fix_dram_dll1_val:	.word	FIX_DRAM_DLL1_VAL
#endif
#endif /* CHIP_REV != A1 */

/**
 * Setup the DDR2 Memory Controller.
 *
 * We use r5 to store whether we should use the PWC or RCT values to
 * program the DDR2 controller.
 */
memsetup:
	@
	@ Delay access to DRAM controller - on cold-reset, there may be
	@ POR-induced di/dt - some delay here will avoid that state
	@
	mov	r0, #0x00600000
_dram_access_delay:
	subs	r0, r0, #0x1
	bne	_dram_access_delay

	@
	@ Read the PLL_CORE_REG value and subtract it from
	@ expect_core_val - then test r5, if r5 is zero, then we
	@ use RCT_xxx set of parameters, otherwise use the PWC_xxx set
	@ of parameters
	@
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
#if	(CHIP_REV == A1)
	ldr	r5, [r0, #PLL_CORE_OFFSET]
	mov	r0, #0xff00
	orr	r0, r0, #0xff
	and	r5, r5, r0
#else
	ldr	r5, [r0, #PLL_CORE_CTRL_OFFSET]
#endif
	ldr	r0, expect_core_val
	sub	r5, r5, r0

	@ Setup the base register
	mov	r0, #AHB_BASE
	orr	r0, r0, #DRAM_OFFSET

	@
	@ Setup DRAM_CFG_REG with the size, bus mode, and ODT fields
	@
	ldr	r1, glo_dram_cfg
	str	r1, [r0, #DRAM_CFG_OFFSET]

	@
	@ Setup DRAM_TIMING_REG
	@
	ldr	r1, pwc_dram_timing1
	cmp	r5, #0x0
	bne	deviate0
	ldr	r1, fix_dram_timing1
deviate0:
	str	r1, [r0, #DRAM_TIMING_OFFSET]

	@
	@ Setup DRAM_TIMING2_REG
	@
	ldr	r1, pwc_dram_timing2
	cmp	r5, #0x0
	bne	deviate1
	ldr	r1, fix_dram_timing2
deviate1:
	str	r1, [r0, #DRAM_TIMING2_OFFSET]

	@
	@ DRAM_CFG - set DRAM_CKE
	@
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	orr     r1, r1, #DRAM_CFG_CKE
	str     r1, [r0, #DRAM_CFG_OFFSET]

	@
	@ Wait for at least 400 ns
	@
	mov	r2, #0x40

LOOP:
	subs	r2, r2, #0x1
	bne     LOOP

	@
	@ DRAM_PRE_CTL
	@
	mov	r1, #0x1
	str	r1, [r0, #DRAM_PRE_CTL_OFFSET]

LOOP0:
	ldr	r1, [r0, #DRAM_PRE_CTL_OFFSET]
	ands	r1, r1, #DRAM_PRE_CTL_PRE_BUSY
	bne	LOOP0

	@
	@ DRAM_EXT_MODE_REG2
	@
	mov     r1, #0x0
	str     r1, [r0, #DRAM_EXT_MODE_2_OFFSET]

LOOP1:
	ldr	r1, [r0, #DRAM_EXT_MODE_2_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE_2_BUSY
	bne	LOOP1

	@
	@ DRAM_EXT_MODE_REG3
	@
	mov     r1, #0x0
	str     r1, [r0, #DRAM_EXT_MODE_3_OFFSET]

LOOP2:
	ldr	r1, [r0, #DRAM_EXT_MODE_3_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE_3_BUSY
	bne	LOOP2

	@
	@ DRAM_EXT_MODE_REG
	@
	mov     r1, #0x0
	str     r1, [r0, #DRAM_EXT_MODE_OFFSET]

LOOP3:
	ldr	r1, [r0, #DRAM_EXT_MODE_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE_BUSY
	bne	LOOP3

#if	(CHIP_REV != A1)
	/* In A2 we have the option to program DLL0/DLL1 into the */
	/* RCT to adjust the DLL values for a particular setting */
a2_pll_dll:
	/* First, we look up the values based on POWER_ON_CONFIG setting */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r4, [r0, #SYS_CONFIG_OFFSET]

#if defined(FIX_DRAM_DLL0_VAL) && defined(FIX_DRAM_DLL1_VAL)
	/* Load fix_dram_dll_val and jump to program it. */
	ldr	r1, fix_dram_dll0_val
	ldr	r2, fix_dram_dll1_val
	b	prog_dll
#endif

#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)

	and	r4, r4, #0x6
	cmp	r4, #0x6
#if	defined(RCT_DLL0_135MHZ) && defined(RCT_DLL1_135MHZ)
	ldreq	r1, =RCT_DLL0_135MHZ
	ldreq	r2, =RCT_DLL1_135MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x4
#if	defined(RCT_DLL0_243MHZ) && defined(RCT_DLL1_243MHZ)
	ldreq	r1, =RCT_DLL0_243MHZ
	ldreq	r2, =RCT_DLL1_243MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x2
#if	defined(RCT_DLL0_108MHZ) && defined(RCT_DLL1_108MHZ)
	ldreq	r1, =RCT_DLL0_108MHZ
	ldreq	r2, =RCT_DLL1_108MHZ
#else
	beq	a2_pll_dll_done
#endif

#else	/* (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) */

	and	r4, r4, #0xe
	cmp	r4, #0xe
#if	defined(RCT_DLL0_283MHZ) && defined(RCT_DLL1_283MHZ)
	ldreq	r1, =RCT_DLL0_283MHZ
	ldreq	r2, =RCT_DLL1_283MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0xc
#if	defined(RCT_DLL0_270MHZ) && defined(RCT_DLL1_270MHZ)
	ldreq	r1, =RCT_DLL0_270MHZ
	ldreq	r2, =RCT_DLL1_270MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0xa
#if	defined(RCT_DLL0_256MHZ) && defined(RCT_DLL1_256MHZ)
	ldreq	r1, =RCT_DLL0_256MHZ
	ldreq	r2, =RCT_DLL1_256MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x8
#if	defined(RCT_DLL0_243MHZ) && defined(RCT_DLL1_243MHZ)
	ldreq	r1, =RCT_DLL0_243MHZ
	ldreq	r2, =RCT_DLL1_243MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x6
#if	defined(RCT_DLL0_230MHZ) && defined(RCT_DLL1_230MHZ)
	ldreq	r1, =RCT_DLL0_230MHZ
	ldreq	r2, =RCT_DLL1_230MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x4
#if	defined(RCT_DLL0_182MHZ) && defined(RCT_DLL1_182MHZ)
	ldreq	r1, =RCT_DLL0_182MHZ
	ldreq	r2, =RCT_DLL1_182MHZ
#else
	beq	a2_pll_dll_done
#endif
	cmp	r4, #0x2
#if	defined(RCT_DLL0_135MHZ) && defined(RCT_DLL1_135MHZ)
	ldreq	r1, =RCT_DLL0_135MHZ
	ldreq	r2, =RCT_DLL1_135MHZ
#else
	beq	a2_pll_dll_done
#endif

#endif	/* (CHIP_REV == A2S) || (CHIP_REV == A2M) */

	cmp	r4, #0x0
#if	defined(RCT_DLL0_216MHZ) && defined(RCT_DLL1_216MHZ)
	ldreq	r1, =RCT_DLL0_216MHZ
	ldreq	r2, =RCT_DLL1_216MHZ
#else
	beq	a2_pll_dll_done
#endif

prog_dll:
	/* Program the values */
	str	r1, [r0, #DLL0_OFFSET]
	str	r2, [r0, #DLL1_OFFSET]
#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)
#if defined(RCT_DLL_CTRL_SEL_VAL)
	ldr	r1, =RCT_DLL_CTRL_SEL_VAL
	str	r1, [r0, #DLL_CTRL_SEL_OFFSET]
#endif
#if defined(RCT_DLL_OCD_BITS_VAL)
	ldr	r1, =RCT_DLL_OCD_BITS_VAL
	str	r1, [r0, #DLL_OCD_BITS_OFFSET]
#endif
#if defined(RCT_DDRIO_CALIB_VAL)
	ldr	r1, =RCT_DDRIO_CALIB_VAL
	str	r1, [r0, #DDRIO_CALIB_OFFSET]
#endif
#endif	/* (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) */

a2_pll_dll_done:

	mov	r0, #AHB_BASE
	orr	r0, r0, #DRAM_OFFSET

#endif  /* CHIP_REV != A1 */

	@
	@ DRAM_DLL_CTL_REG:	 Reset the DLL
	@
	mov     r1, #DRAM_DLL_CTL_DLL_RST_EN
	str     r1, [r0, #DRAM_DLL_CTL_OFFSET]

	@
	@ DRAM_MODE_REG
	@
	ldr	r1, pwc_dram_mode
	cmp	r5, #0x0
	bne	deviate2
	ldr	r1, fix_dram_mode
deviate2:
	orr	r1, r1, #DRAM_MODE_DLL
	str     r1, [r0, #DRAM_MODE_OFFSET]


LOOP4:
	ldr	r1, [r0, #DRAM_DLL_CTL_OFFSET]
	ands	r1, r1, #DRAM_DLL_CTL_DLL_RST_EN
	bne	LOOP4

LOOP4a:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	ands	r1, r1, #DRAM_MODE_BUSY
	bne	LOOP4a

	@
	@ DRAM_PRE_CTL_REG
	@
	mov     r1, #0x1
	str     r1, [r0, #DRAM_PRE_CTL_OFFSET]

LOOP5:
 	ldr	r1, [r0, #DRAM_PRE_CTL_OFFSET]
 	ands	r1, r1, #DRAM_PRE_CTL_PRE_BUSY
 	bne	LOOP5

	@
	@ DRAM_REF_CTL
	@
 	mov	r1, #DRAM_REF_CTL_IMM_REF_EN
 	str	r1, [r0, #DRAM_REF_CTL_OFFSET]

LOOP6:
	ldr	r1, [r0, #DRAM_REF_CTL_OFFSET]
	ands	r1, r1, #0xff000000
	bne	LOOP6

	@
	@  DRAM_REF_CTL_REG
	@
	mov	r1, #DRAM_REF_CTL_IMM_REF_EN
	str	r1, [r0, #DRAM_REF_CTL_OFFSET]

LOOP7:
	ldr	r1, [r0, #DRAM_REF_CTL_OFFSET]
	ands	r1, r1, #0xff000000
	bne	LOOP7

	@
	@ DRAM_REF_CTL_REG
	@
	mov	r1, #DRAM_REF_CTL_IMM_REF_EN
	str	r1, [r0, #DRAM_REF_CTL_OFFSET]

	@
	@ Wait for about 100ns
	@
	mov	r2, #0x00000010
LOOPb:
	subs	r2, r2, #0x01
	bne	LOOPb

LOOP7c:
	ldr	r1, [r0, #DRAM_REF_CTL_OFFSET]
	ands	r1, r1, #0xff000000
	bne	LOOP7c

	@
	@ DRAM_MODE_REG
	@
	ldr	r1, pwc_dram_mode
	cmp	r5, #0x0
	bne	deviate3
	ldr	r1, fix_dram_mode
deviate3:
	str	r1, [r0, #DRAM_MODE_OFFSET]

LOOP8:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	ands	r1, r1, #0x80000000
	bne	LOOP8

	@
	@ DRAM_REF_CTL_REG -
	@
	ldr	r1, pwc_dram_ref_ctl
	cmp	r5, #0x0
	bne	deviate4
	ldr	r1, fix_dram_ref_ctl
deviate4:
	str	r1, [r0, #DRAM_REF_CTL_OFFSET]

	@
	@ DRAM_EXT_MODE_REG
	@
	ldr	r1, [r0, #DRAM_EXT_MODE_OFFSET]
	orr	r1, r1, #0x00000380
	str	r1, [r0, #DRAM_EXT_MODE_OFFSET]

LOOP9:
	ldr	r1, [r0, #DRAM_EXT_MODE_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE_BUSY
	bne	LOOP9

	@
	@ DRAM_EXT_MODE_REG - add odt(rtt) value
	@
	ldr	r1, [r0, #DRAM_EXT_MODE_OFFSET]
	and	r1, r1, #0xfffffc7f
	ldr	r2, glo_dram_ext_mode
	ands	r2, r2, #0x44
        orr     r1, r1, r2
	str	r1, [r0, #DRAM_EXT_MODE_OFFSET]

LOOP10:
	ldr	r1, [r0, #DRAM_EXT_MODE_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE_BUSY
	bne	LOOP10

	@
	@ GET_RTT_EN
	@
	mov     r1, #DRAM_DQS_CTL_GET_RTT_EN
	str    r1, [r0, #DRAM_DQS_CTL_OFFSET]

LOOP11:
	ldr	r1, [r0, #DRAM_DQS_CTL_OFFSET]
	ands	r1, r1, #DRAM_DQS_CTL_GET_RTT_EN
	bne	LOOP11

	@
	@ DRAM_ENABLE
	@
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	orr	r1, r1, #DRAM_CFG_EN
	str	r1, [r0, #DRAM_CFG_OFFSET]

	mov	pc, lr		@ Retuen to caller
