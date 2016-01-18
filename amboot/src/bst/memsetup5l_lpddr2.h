/**
 * system/src/bst/memsetup5l_lpddr2.h
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
	mov		r10,	lr  /* save link register in r10 */
@ base address of dram table is loaded into sp
	ldr		r0, =dram_params_table
	mov		sp, r0

#include "dramfreq5l.h"

slow_clock_mode:
@ set ddrio in lpddr2 init mode
	load_from_table	glo_dram_cg_ddr_init, r1
	ldr		r2, =CG_DDR_INIT_REG
	str		r1, [r0, r2]
		
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

@ wait 100ns for cke high step 5
	ldr     r1, =0x1
_dram_delay100ns:	
	sub     r1, #0x1
	bne     _dram_delay100ns

@ set DRAM_CKE bit step 6
	mov		r1, #DRAM_CONTROL_CKE
	str		r1, [r6, #DRAM_CTL_OFFSET]

@ 200us delay after cke high
	ldr     r1, =0x100
_dram_delay200us:	
	sub     r1, #0x1
	bne     _dram_delay200us

@ reset LPDDR2
	load_from_table	glo_dram_mode_rstlpdd2, r1	
	str		r1, [r6, #DRAM_MODE_OFFSET]
	
	mov     r1, #0x6
_dram_delay2:
	sub     r1, #0x1
	bne     _dram_delay2

@ write to DRAM_MODE_REG1 register
	load_from_table	pwc_dram_ext_mode1, r1
	str		r1, [r6, #DRAM_MODE_OFFSET]

@ poll for busy bit
	ldr		r2, =DRAM_MODE_BUSY
LOOP4:
	ldr		r1, [r6, #DRAM_MODE_OFFSET]
	and		r1, r2
	bne LOOP4

@ write to DRAM_MODE_REG 2 register
	load_from_table	pwc_dram_ext_mode2, r1
	str		r1, [r6, #DRAM_MODE_OFFSET]

@ poll for busy bit
LOOP5:
	ldr		r1, [r6, #DRAM_MODE_OFFSET]
	and		r1, r2
	bne LOOP5

@ set DRAM_AUTO_REF_EN step 18
	mov		r1, #DRAM_AUTO_REF_EN
	str		r1, [r6, #DRAM_CTL_OFFSET]

@ putting in power down for clock change
	mov		r1, #DRAM_POWER_DOWN
	str		r1, [r6, #DRAM_CTL_OFFSET]
	
@wait few clocks
	mov     r1, #0x6
_dram_delay3:
	sub     r1, #0x1
	bne     _dram_delay3

@ base address of RCT registers is in r0

@ remove ddrio out of lpddr2 init mode
	load_from_table	glo_dram_cg_ddr_normal, r1
	ldr		r2, =CG_DDR_INIT_REG
	str		r1, [r0, r2]

@wait a few clocks
	mov     r1, #0x6
_dram_delay4:
	sub     r1, #0x1
	bne     _dram_delay4

@ DLL_RST_EN
	mov		r2, #DRAM_INIT_CTL_DLL_RST_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

	mov     r1, #0x6
_dram_delay5:
	sub     r1, #0x1
	bne     _dram_delay5

@ wait for DLL_RST_EN to clear
LOOP10:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne LOOP10

@ waking up from power down after clock change
	mov		r1, #DRAM_POWER_DOWN_CKE_HIGH
	str		r1, [r6, #DRAM_CTL_OFFSET]

	mov     r1, #0x3
_dram_delay6:
	sub     r1, #0x1
	bne     _dram_delay6

@ power down exit pulling CS low
	mov		r1, #DRAM_AUTO_REF_EN
	str		r1, [r6, #DRAM_CTL_OFFSET]

@ set PAD_CLB_EN bit step 20.5
	mov		r2, #DRAM_INIT_CTL_PAD_CLB_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

LOOP11:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne LOOP11

@ set GET_RTT_EN bit step 21
	mov		r2, #DRAM_INIT_CTL_GET_RTT_EN
	str		r2, [r6, #DRAM_INIT_CTL_OFFSET]

@ wait for GET_RTT to finish step 21
LOOP14:
	ldr		r1, [r6, #DRAM_INIT_CTL_OFFSET]
	and		r1, r2
	bne LOOP14

@ set DQS ON
	load_from_table	pwc_dram_dqs_sync, r1
	str		r1, [r6, #DRAM_DQS_SYNC_OFFSET]

@ set pad ZQ calibration ON
	load_from_table	pwc_dram_qz_calib, r1
	str		r1, [r6, #DRAM_ZQ_CALIB_OFFSET]

@disable pad to save power
	load_from_table	pwc_dram_pad_term, r1
	ldr		r2, =(PAD_RESET_MASK)
	and		r1, r2
	str		r1, [r6, #DRAM_PAD_ZCTL_OFFSET]


@ initiate read of MR0
	mov		r1, #0x0
	str		r1, [r6, #DRAM_MODE_OFFSET]

@ wait for DAI bit to clear
	ldr		r2, =0x80000001
LOOP15:
	ldr		r1, [r6, #DRAM_MODE_OFFSET]
	and		r1, r2
	bne LOOP15

@ set DRAM_ENABLE bit
	mov		r1, #(DRAM_CONTROL_ENABLE | DRAM_AUTO_REF_EN)
	str		r1, [r6, #DRAM_CTL_OFFSET]

	mov     r1, #0x6
_dram_delay7:
	sub     r1, #0x1
	bne     _dram_delay7

@ DRAM INITIALIZATION done
	bx		r10		@Return to caller





