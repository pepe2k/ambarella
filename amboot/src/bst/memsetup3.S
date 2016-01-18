/**
 * system/src/bst/memsetup3.S
 *
 * History:
 *    2007/07/03 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

.text
.globl	memsetup3

/*
 * Setup Power-On-Config specific DDR2 constants.
 */
#if	defined(PWC_DRAM_148MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____148MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_148MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_148MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_148MHZ
#define PWC_DLL0_VAL		RCT_DLL0_148MHZ
#define PWC_DLL1_VAL		RCT_DLL1_148MHZ
#define PWC_DLL2_VAL		RCT_DLL2_148MHZ
#define PWC_DLL3_VAL		RCT_DLL3_148MHZ
#elif	defined(PWC_DRAM_216MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____216MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_216MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_216MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_216MHZ
#define PWC_DLL0_VAL		RCT_DLL0_216MHZ
#define PWC_DLL1_VAL		RCT_DLL1_216MHZ
#define PWC_DLL2_VAL		RCT_DLL2_216MHZ
#define PWC_DLL3_VAL		RCT_DLL3_216MHZ
#elif	defined(PWC_DRAM_337MHZ)
#define PWC_DRAM_MODE_VAL	DRAM_MODE____337MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_337MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_337MHZ
#define PWC_DRAM_REF_CTL_VAL	DRAM_REF_CTL_337MHZ
#define PWC_DLL0_VAL		RCT_DLL0_337MHZ
#define PWC_DLL1_VAL		RCT_DLL1_337MHZ
#define PWC_DLL2_VAL		RCT_DLL2_337MHZ
#define PWC_DLL3_VAL		RCT_DLL3_337MHZ
#else
#error "Unknown Power-On-Config DRAM Frequency!"
#endif

glo_dram_cfg:		.word	DRAM_CFG_VAL
glo_dram_ext_mode:	.word	DRAM_EXT_MODE_VAL
pwc_dram_mode:		.word	PWC_DRAM_MODE_VAL
pwc_dram_timing1:	.word	PWC_DRAM_TIMING1_VAL
pwc_dram_timing2:	.word	PWC_DRAM_TIMING2_VAL
pwc_dram_ref_ctl:	.word	PWC_DRAM_REF_CTL_VAL
pwc_dll0:		.word	PWC_DLL0_VAL
pwc_dll1:		.word	PWC_DLL1_VAL
pwc_dll2:		.word	PWC_DLL2_VAL
pwc_dll3:		.word	PWC_DLL3_VAL

/**
 * Setup the DDR2 Memory Controller.
 */
memsetup3:
	@
	@ Delay access to DRAM controller - on cold-reset, there may be
	@ POR-induced di/dt - some delay here will avoid that state
	@
	mov	r0, #0x00600000
_dram_access_delay:
	subs	r0, r0, #0x1
	bne	_dram_access_delay

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
	str	r1, [r0, #DRAM_TIMING_OFFSET]

	@
	@ Setup DRAM_TIMING2_REG
	@
	ldr	r1, pwc_dram_timing2
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

	/* Program the values into the DLL0/DLL1/DLL2/DLL3 */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, pwc_dll0
	str	r1, [r0, #DLL0_OFFSET]
	ldr	r1, pwc_dll1
	str	r1, [r0, #DLL1_OFFSET]
	ldr	r1, pwc_dll2
	str	r1, [r0, #DLL2_OFFSET]
	ldr	r1, pwc_dll3
	str	r1, [r0, #DLL3_OFFSET]

	mov	r0, #AHB_BASE
	orr	r0, r0, #DRAM_OFFSET

	@
	@ DRAM_DLL_CTL_REG:	 Reset the DLL
	@
	mov     r1, #DRAM_DLL_CTL_DLL_RST_EN
	str     r1, [r0, #DRAM_DLL_CTL_OFFSET]

	@
	@ DRAM_MODE_REG
	@
	ldr	r1, pwc_dram_mode
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
	str	r1, [r0, #DRAM_MODE_OFFSET]

LOOP8:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	ands	r1, r1, #0x80000000
	bne	LOOP8

	@
	@ DRAM_REF_CTL_REG -
	@
	ldr	r1, pwc_dram_ref_ctl
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
