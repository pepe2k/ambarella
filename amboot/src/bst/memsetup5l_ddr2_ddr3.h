/**
 * system/src/bst/memsetup5l_ddr2_ddr3.h
 *
 * History:
 *    2009/11/16 - [Allen Wang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "params_common.h"

.text
.globl	memsetup5l
.thumb_func

/**
 * Setup the DDR2 Memory Controller.
 *
 * We use r5 to store whether we should use the PWC or RCT values to
 * program the DDR2 controller.
 */
memsetup5l:
	mov		r10,	lr  /* save link register in r10 */

@ base address of dram table is loaded into sp
#ifndef _SPIBOOT_
	ldr		r0, =dram_params_table
	mov		sp, r0
#else
	ldr		r4, =spi_readlx
#endif

#include "dramfreq5l.h"

write_dram_main_regs:
	@
	@ Delay access to DRAM controller - on cold-reset, there may be
	@ POR-induced di/dt - some delay here will avoid that state
	@

	ldr     r1, =0x8000
_dram_access_delay:
	sub     r1, #0x1
	bne     _dram_access_delay

	@ Setup the base register
	ldr	r0, =(AHB_BASE | DRAM_OFFSET)

@ write to DRAM_CFG_REG step 2 (disable sync and zq_clbr)
	load_from_table	glo_dram_cfg, r1
	str	r1, [r0, #DRAM_CFG_OFFSET]

@ Save DDR type to r3
	mov	r3, #0x3
	and	r3, r1

@ write to DRAM_TIMING1 reg step 3
	load_from_table	pwc_dram_timing1, r1
	str	r1, [r0, #DRAM_TIMING1_OFFSET]

@ write to DRAM_TIMING2 reg step 3
	load_from_table	pwc_dram_timing2, r1
	str	r1, [r0, #DRAM_TIMING2_OFFSET]

@ write to DRAM_TIMING3 reg step 3
	load_from_table	pwc_dram_timing3, r1
	str	r1, [r0, #DRAM_TIMING3_OFFSET]

@ Set SWDQS or auto
	load_from_table	pwc_dram_dqs_sync, r1
	str	r1, [r0, #DRAM_DQS_SYNC_OFFSET]

@ disable ZQ calibration
	mov	r1, #DRAM_ZQ_CALIB_DISABLE_ZQ_CLBR
	str	r1, [r0, #DRAM_ZQ_CALIB_OFFSET]

@ write to PAD TERM
	load_from_table	pwc_dram_pad_term, r1
	ldr	r2, =DRAM_PADS_RESET
	orr	r1, r2
	str	r1, [r0, #DRAM_PAD_ZCTL_OFFSET]

	mov	r2, #0 @clear r2 for use later
	cmp	r3, #DRAM_CONFIG_TYPE_DDR2
	beq	skip_ddr3_reset_high

	@-----START DDR3 only ----------@
@ deassert DRAM_RESET step 4
	mov	r2, #DRAM_CONTROL_RESET
	str	r2, [r0, #DRAM_CTL_OFFSET]

	@ wait 500us for reset step 5
	ldr     r1, =0x1800
_dram_delay500us:
	sub     r1, #0x1
	bne     _dram_delay500us
	@----- END DDR3 only ----------@

skip_ddr3_reset_high:
	@ set DRAM_CKE bit step 6
	mov	r1, #DRAM_CONTROL_CKE
	orr	r1, r2
	str	r1, [r0, #DRAM_CTL_OFFSET]

	@ wait 400ns for cke high step 5
	mov     r1, #0x18
_dram_delay400ns:
	sub     r1, #0x1
	bne     _dram_delay400ns

	cmp	r3, #DRAM_CONFIG_TYPE_DDR2
	bne	skip_ddr2_precharge_all

	@-----START DDR2 only ----------@
	@ PRE_ALL_EN step 8
	mov	r1, #DRAM_INIT_CTL_PRE_ALL_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for PRE_ALL_BUSY to clear step 8
	mov	r2, #DRAM_INIT_CTL_PRE_ALL_EN
LOOP1:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP1

	@-----END DDR2 only ----------@
skip_ddr2_precharge_all:

@ DRAM_EXT_MODE_REG2 step 9
	load_from_table	pwc_dram_ext_mode2, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]

@ wait EXT_MODE_REG2 busy to clear step9
	ldr	r2, =DRAM_MODE_BUSY
LOOP2:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP2

@ DRAM_EXT_MODE_REG3 step 10
	load_from_table	pwc_dram_ext_mode3, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]

@ wait EXT_MODE_REG3 busy to clear stp10
	ldr	r2, =DRAM_MODE_BUSY
LOOP3:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP3

@ write to DRAM_EXT_MODE_REG register step 11
	load_from_table	pwc_dram_ext_mode1, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]

	@ poll for busy bit step 11
	ldr 	r2, =DRAM_MODE_BUSY
LOOP4:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP4

@ write DRAM_MODE_REG to reset DLL on memory
	load_from_table	pwc_dram_ext_mode0, r1
	ldr	r2, =DRAM_MODE_REG0_DLL_RESET
	orr	r1, r2
	str	r1, [r0, #DRAM_MODE_OFFSET]

@ poll for busy bit step 12
	ldr	r2, =DRAM_MODE_BUSY
LOOP5:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP5

@ wait 5 clocks to allow for tMOD
	mov     r1, #0x10
_dram_delay5clks:
	sub     r1, #0x1
	bne     _dram_delay5clks

	cmp	r3,#DRAM_CONFIG_TYPE_DDR2
	bne	start_ddr3_path

		@ ---------- DDR2 PATH BEGIN ----------
start_ddr2_path:

@ PRE_ALL_EN step 8
	mov	r2, #DRAM_INIT_CTL_PRE_ALL_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for PRE_ALL_BUSY to clear step 8
LOOP6:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP6


@ DLL_RST_EN  to reset on-chip DLL
	mov	r1, #DRAM_INIT_CTL_DLL_RST_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for DLL_RST_EN to clear step 8
	mov	r2, #DRAM_INIT_CTL_DLL_RST_EN
LOOP7:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP7


@ issue refresh step 15
	mov	r2, #DRAM_INIT_CTL_IMM_REF_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for refresh to finish step 15
LOOP9:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP9


@ wait 75ns before next refresh
	mov     r1, #0x5
_dram_delay75ns:
	sub     r1, #0x1
	bne     _dram_delay75ns

@ issue refresh step 16
	mov	r1, #DRAM_INIT_CTL_IMM_REF_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for refresh to finish step 16
	mov	r2, #DRAM_INIT_CTL_IMM_REF_EN
LOOP11:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP11

@ set DRAM_MODE_REG step 17
	load_from_table	pwc_dram_ext_mode0, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]

@ wait for MODE_REG not busy step 17
	ldr	r2, =DRAM_MODE_BUSY
LOOP12:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP12

@ set DRAM_AUTO_REF_EN step 18
	mov	r1, #DRAM_AUTO_REF_EN
	str	r1, [r0, #DRAM_CTL_OFFSET]

@ write DRAM_EXT_MODE_REG for OCD default step19
	load_from_table	pwc_dram_ext_mode1, r1
	ldr	r2, =DRAM_MODE_REG1_OCD_DEFAULT
	orr	r1, r2
	str	r1, [r0, #DRAM_MODE_OFFSET]


@ poll busy bit step19
	ldr	r2, =DRAM_MODE_BUSY
LOOP13:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP13

@ write DRAM_EXT_MODE_REG for OCD exit step20
	load_from_table	pwc_dram_ext_mode1, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]

@ poll busy bit step20
	ldr	r2, =DRAM_MODE_BUSY
LOOP14:
	ldr	r1, [r0, #DRAM_MODE_OFFSET]
	and	r1, r2
	bne	LOOP14

	@ ---------- DDR2 PATH END ----------

	b		end_ddr3_path

	@ ---------- DDR3 PATH BEGIN ----------
start_ddr3_path:

	@ issue ZQCL command
	mov	r2, #DRAM_INIT_CTL_ZQ_CLB_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for ZQ Calibration to complete
LOOP35:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP35

@ DLL_RST_EN
	mov	r2, #DRAM_INIT_CTL_DLL_RST_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for DLL_RST_EN to clear step 8
LOOP37:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP37

@ set DRAM_AUTO_REF_EN step 18
	mov	r1, #DRAM_DDR3_AUTO_REF_EN
	str	r1, [r0, #DRAM_CTL_OFFSET]

	@ ---------- DDR3 PATH END ----------

end_ddr3_path:
@ set PAD_CLB_EN bit step 20.5
	mov	r2, #DRAM_INIT_CTL_PAD_CLB_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for PAD Calibration to finish step 20.5
LOOP15:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP15

@ set GET_RTT_EN bit step 21
	mov	r2, #DRAM_INIT_CTL_GET_RTT_EN
	str	r2, [r0, #DRAM_INIT_CTL_OFFSET]

@ wait for GET_RTT to finish step 21
LOOP16:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	and	r1, r2
	bne	LOOP16

@set periodic pad calibration and ZQ calibration on
	load_from_table	pwc_dram_qz_calib, r1
	str	r1, [r0, #DRAM_ZQ_CALIB_OFFSET]

@disable pad to save power
	load_from_table	pwc_dram_pad_term, r1
	ldr	r2, =(PAD_RESET_MASK)
	and	r1, r2
	str	r1, [r0, #DRAM_PAD_ZCTL_OFFSET]


@ set DRAM_ENABLE bit
	mov	r2, #DRAM_CONTROL_ENABLE
	ldr	r1, [r0, #DRAM_CTL_OFFSET]
	orr	r1, r2
	str	r1, [r0, #DRAM_CTL_OFFSET]

	mov     r1, #0xa
_dram_delay3clks:
	sub     r1, #0x1
	bne     _dram_delay3clks

@ set DRAM_DDRC_REQ_CREDIT_REG

	ldr	r2, =(DRAM_DDRC_REQ_CREDIT_VAL)
	str	r2, [r0, #DRAM_DDRC_REQ_CREDIT_OFFSET]

@ DRAM INITIALIZATION done

#if	defined(DRAM_TYPE_DDR3)
@ Keep dram_mode reg as pwc_dram_ext_mode1 value to facilitate future use in App
	load_from_table	pwc_dram_ext_mode1, r1
	str	r1, [r0, #DRAM_MODE_OFFSET]
#endif

	bx	r10		@Return to caller

