/**
 * system/src/bst/memsetup5l_lpddr.h
 *
 * History:
 *    2009/11/16 - [Allen Wang] created file, used A5S memsetup.
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

memsetup5l:
@ base address of dram table is loaded into sp
	ldr		r0, =dram_params_table
	mov		sp, r0

#include "dramfreq5l.h"

write_dram_main_regs:
@
@ Delay access to DRAM controller - on cold-reset, there may be
@ POR-induced di/dt - some delay here will avoid that state
@
	ldr     r1, =0x8000	
_dram_access_delay:
	sub	r1, r1, #0x1
	bne	_dram_access_delay

@ Setup the base register
	ldr		r6, =(AHB_BASE | DRAM_OFFSET)

@ write to DRAM_CFG_REG step 2 (disable sync and zq_clbr)
	load_from_table	glo_dram_cfg, r1
	str		r1, [r6, #DRAM_CFG_OFFSET]

@ write to DRAM_TIMING1 reg step 3
	load_from_table	pwc_dram_timing1, r1
	str		r1, [r6, #DRAM_TIMING1_OFFSET]

@ write to DRAM_TIMING2 reg step 3
	load_from_table	pwc_dram_timing2, r1
	str		r1, [r6, #DRAM_TIMING2_OFFSET]

@ write to DRAM_TIMING3 reg step 3
	load_from_table	pwc_dram_timing3, r1
	str		r1, [r6, #DRAM_TIMING3_OFFSET]

@ disable DQS
	mov		r1, #DRAM_CONFIG_DQS_SYNC_DIS
	str		r1, [r6, #DRAM_DQS_SYNC_OFFSET]

@ disable ZQ calibration
	mov		r1, #DRAM_ZQ_CALIB_DISABLE_ZQ_CLBR
	str		r1, [r6, #DRAM_ZQ_CALIB_OFFSET]

@ write to PAD TERM 
	load_from_table	pwc_dram_pad_term, r1
	ldr		r2, =DRAM_PADS_RESET
	orr		r1, r2
	str		r1, [r6, #DRAM_PAD_ZCTL_OFFSET]

@ set DRAM_CKE bit step 6
	mov	r1, #DRAM_CONTROL_CKE
	str	r1, [r6, #DRAM_CTL_OFFSET]	
	
@ wait 200us for cke high step 5
	ldr     r1, =0x100
_dram_delay200us:	
	sub     r1, #0x1
	bne     _dram_delay200us

@ PRE_ALL_EN step 8
	mov		r2, #DRAM_INIT_CTL_PRE_ALL_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for PRE_ALL_BUSY to clear step 8
LOOP1:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP1

@ DLL_RST_EN  to reset on-chip DLL
	mov		r1, #DRAM_INIT_CTL_DLL_RST_EN
	str		r1, [r6, #DRAM_INIT_CTL_OFFSET]

@ issue refresh step 15
	mov		r2, #DRAM_INIT_CTL_IMM_REF_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for refresh to finish step 15
LOOP9:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP9


@ wait 150ns before next refresh
	ldr     r1, =0x1
_dram_delay150ns:	
	sub     r1, #0x1
	bne     _dram_delay150ns

@ issue refresh step 16
	mov		r2, #DRAM_INIT_CTL_IMM_REF_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for refresh to finish step 16
LOOP11:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP11


@ wait for DLL_RST_EN to clear step 8
	mov		r2, #DRAM_INIT_CTL_DLL_RST_EN
LOOP7:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP7

@ write to DRAM_EXT_MODE_REG register step 11
	load_from_table	pwc_dram_ext_mode1, r1
	str		r1, [r6, #DRAM_MODE_OFFSET]

@ poll for busy bit step 11
	ldr		r2, =DRAM_MODE_BUSY	
LOOP4:
	ldr		r1, [r6, #DRAM_MODE_OFFSET]
	and		r1, r2
	bne		LOOP4
	
@ set DRAM_MODE_REG step 17
	load_from_table	pwc_dram_ext_mode0, r1
	str		r1, [r6, #DRAM_MODE_OFFSET]

@ wait for MODE_REG not busy step 17
LOOP12:
	ldr		r1, [r6, #DRAM_MODE_OFFSET]
	and		r1, r2
	bne		LOOP12

@ set DRAM_AUTO_REF_EN step 18
	mov		r1, #DRAM_AUTO_REF_EN
	str		r1, [r6, #DRAM_CTL_OFFSET]


@ set PAD_CLB_EN bit step 20.5
	mov		r2, #DRAM_INIT_CTL_PAD_CLB_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for PAD Calibration to finish step 20.5
LOOP15:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP15


@ set GET_RTT_EN bit step 21
	mov		r2, #DRAM_INIT_CTL_GET_RTT_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for GET_RTT to finish step 21
LOOP16:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne		LOOP16

@ set DQS ON
	load_from_table	pwc_dram_dqs_sync, r1
	str		r1, [r6, #DRAM_DQS_SYNC_OFFSET]

@set periodic pad calibration on
	load_from_table	pwc_dram_qz_calib, r1
	str		r1, [r6, #DRAM_ZQ_CALIB_OFFSET] 

@disable pad to save power
	load_from_table	pwc_dram_pad_term, r1
	ldr		r2, =(PAD_RESET_MASK)
	and		r1, r2
	str		r1, [r6, #DRAM_PAD_ZCTL_OFFSET]


@ set DRAM_ENABLE bit
	mov		r1, #(DRAM_CONTROL_ENABLE|DRAM_AUTO_REF_EN)
	str		r1, [r6, #DRAM_CTL_OFFSET]

	mov     r1, #0x6
_dram_delay6:
	sub     r1, #0x1
	bne     _dram_delay6

@ DRAM INITIALIZATION done
	bx		lr		@Return to caller



