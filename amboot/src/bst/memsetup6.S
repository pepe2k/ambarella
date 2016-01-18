/**
 * system/src/bst/memsetup6.S
 *
 * History:
 *    2007/07/16 - [Charles Chiou] adapted from AMBoot
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

.text
.globl	memsetup6

/*
 * Setup Power-On-Config specific DDR2/DDR3 constants.
 */
#if	defined(PWC_DRAM_216MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_216MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_216MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_216MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_216MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_216MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_216MHZ
#define PWC_DLL0_VAL		RCT_DLL0_216MHZ
#define PWC_DLL1_VAL		RCT_DLL1_216MHZ
#define PWC_DLL2_VAL		RCT_DLL2_216MHZ
#define PWC_DLL3_VAL		RCT_DLL3_216MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_216MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_216MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_216MHZ
#define PWC_DRAM_FREQ_VAL	0x0f011100
#elif	defined(PWC_DRAM_270MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_270MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_270MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_270MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_270MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_270MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_270MHZ
#define PWC_DLL0_VAL		RCT_DLL0_270MHZ
#define PWC_DLL1_VAL		RCT_DLL1_270MHZ
#define PWC_DLL2_VAL		RCT_DLL2_270MHZ
#define PWC_DLL3_VAL		RCT_DLL3_270MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_270MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_270MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_270MHZ
#define PWC_DRAM_FREQ_VAL	0x13011100
#elif	defined(PWC_DRAM_297MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_297MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_297MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_297MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_297MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_297MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_297MHZ
#define PWC_DLL0_VAL		RCT_DLL0_297MHZ
#define PWC_DLL1_VAL		RCT_DLL1_297MHZ
#define PWC_DLL2_VAL		RCT_DLL2_297MHZ
#define PWC_DLL3_VAL		RCT_DLL3_297MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_297MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_297MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_297MHZ
#define PWC_DRAM_FREQ_VAL	0x15011100
#elif	defined(PWC_DRAM_324MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_324MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_324MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_324MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_324MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_324MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_324MHZ
#define PWC_DLL0_VAL		RCT_DLL0_324MHZ
#define PWC_DLL1_VAL		RCT_DLL1_324MHZ
#define PWC_DLL2_VAL		RCT_DLL2_324MHZ
#define PWC_DLL3_VAL		RCT_DLL3_324MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_324MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_324MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_324MHZ
#define PWC_DRAM_FREQ_VAL	0x17011100
#elif	defined(PWC_DRAM_337MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_337MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_337MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_337MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_337MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_337MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_337MHZ
#define PWC_DLL0_VAL		RCT_DLL0_337MHZ
#define PWC_DLL1_VAL		RCT_DLL1_337MHZ
#define PWC_DLL2_VAL		RCT_DLL2_337MHZ
#define PWC_DLL3_VAL		RCT_DLL3_337MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_337MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_337MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_337MHZ
#define PWC_DRAM_FREQ_VAL	0x18011100
#elif	defined(PWC_DRAM_432MHZ)
#define PWC_DRAM_EXT_MODE0_VAL	DRAM_EXT_MODE0_432MHZ
#define PWC_DRAM_EXT_MODE1_VAL	DRAM_EXT_MODE1_432MHZ
#define PWC_DRAM_EXT_MODE2_VAL	DRAM_EXT_MODE2_432MHZ
#define PWC_DRAM_EXT_MODE3_VAL	DRAM_EXT_MODE3_432MHZ
#define PWC_DRAM_TIMING1_VAL	DRAM_TIMING1_432MHZ
#define PWC_DRAM_TIMING2_VAL	DRAM_TIMING2_432MHZ
#define PWC_DLL0_VAL		RCT_DLL0_432MHZ
#define PWC_DLL1_VAL		RCT_DLL1_432MHZ
#define PWC_DLL2_VAL		RCT_DLL2_432MHZ
#define PWC_DLL3_VAL		RCT_DLL3_432MHZ
#define PWC_DLL_CTRL_SEL	RCT_DLL_CTRL_SEL_432MHZ
#define PWC_DLL_OCD_BITS_VAL	RCT_DLL_OCD_BITS_432MHZ
#define PWC_DDRIO_CALIB_VAL	RCT_DDRIO_CALIB_432MHZ
#define PWC_DRAM_FREQ_VAL	0x1f011100
#else
#error "Unknown Power-On-Config DRAM Frequency!"
#endif
	
#define PWC_DDR_CTR2		0x3f710000
#define PWC_DDR_CTR3		0x00068300

glo_dram_cfg:		.word	DRAM_CFG_VAL
pwc_dram_ext_mode0:	.word	PWC_DRAM_EXT_MODE0_VAL
pwc_dram_ext_mode1:	.word	PWC_DRAM_EXT_MODE1_VAL
pwc_dram_ext_mode2:	.word	PWC_DRAM_EXT_MODE2_VAL
pwc_dram_ext_mode3:	.word	PWC_DRAM_EXT_MODE3_VAL
pwc_dram_timing1:	.word	PWC_DRAM_TIMING1_VAL
pwc_dram_timing2:	.word	PWC_DRAM_TIMING2_VAL
pwc_dram_freq:		.word	PWC_DRAM_FREQ_VAL
pwc_dll0:		.word	PWC_DLL0_VAL
pwc_dll1:		.word	PWC_DLL1_VAL
pwc_dll2:		.word	PWC_DLL2_VAL
pwc_dll3:		.word	PWC_DLL3_VAL
pwc_dll_ctrl_sel:	.word 	PWC_DLL_CTRL_SEL
pwc_dll_ocd_bits:	.word	PWC_DLL_OCD_BITS_VAL
pwc_ddrio_calib:	.word	PWC_DDRIO_CALIB_VAL
pwc_ddr_ctr2:		.word   PWC_DDR_CTR2
pwc_ddr_ctr3:		.word   PWC_DDR_CTR3
    
/**
 * Setup the DDR2/DDR3 Memory Controller.
 */
memsetup6:
	
	@
	@ Delay access to DRAM controller - on cold-reset, there may be
	@ POR-induced di/dt - some delay here will avoid that state
	@
	mov     r0, #0x00600000
_dram_access_delay:
	subs    r0, r0, #0x1
	bne     _dram_access_delay
	
	@ Setup the base register
	mov	r0, #AHB_BASE
	orr	r0, r0, #DRAM_OFFSET	
		
	@ wait 250us
	mov	r1, #0x8000
LOOP250:
	subs	r1, r1, #0x1
	bne	LOOP250

	@ write to DRAM_CFG_REG step 2 (disable sync and zq_clbr)
	ldr	r1, glo_dram_cfg
	and	r1, r1, #0xffc0ffff
	orr	r1, r1, #DRAM_CONFIG_DQS_SYNC_DIS
	orr	r1, r1, #DRAM_CONFIG_ZQ_CLBR_DIS
	str	r1, [r0, #DRAM_CFG_OFFSET]

	@ write to DRAM_TIMING reg step 3
	ldr	r1, pwc_dram_timing1
	str	r1, [r0, #DRAM_TIMING1_OFFSET]

	@ write to DRAM_TIMING2 reg step 3
	ldr	r1, pwc_dram_timing2
	str	r1, [r0, #DRAM_TIMING2_OFFSET]

	@ DDR3: deassert DRAM_RESET
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	tst	r1, #DRAM_CONFIG_TYPE_DDR3
	beq	ddr3_dram_reset_done
	mov	r1, #DRAM_CONTROL_RESET
	str	r1, [r0, #DRAM_CTL_OFFSET]

	@ wait 500us for reset
	mov	r1, #0x10000
LOOP500:
	subs	r1, r1, #0x1
	bne	LOOP500
ddr3_dram_reset_done:

	@ set DRAM_CKE bit step 6
	ldr	r1, [r0, #DRAM_CTL_OFFSET]
	orr	r1, r1, #DRAM_CONTROL_CKE
	str	r1, [r0, #DRAM_CTL_OFFSET]

	@ wait 400 ns step 7
	mov	r2, #0x40
LOOP0:
	subs	r2, r2, #0x1
	bne	LOOP0

	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	tst	r1, #DRAM_CONFIG_TYPE_DDR3
	bne	ddr3_skip_pre_all

	@ PRE_ALL_EN step 8
	mov	r1, #DRAM_INIT_CTL_PRE_ALL_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait for PRE_ALL_BUSY to clear step 8
LOOP1:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_PRE_ALL_EN
	bne	LOOP1
ddr3_skip_pre_all:

	@ DRAM_EXT_MODE2_REG step 9
	ldr	r1, pwc_dram_ext_mode2
	str	r1, [r0, #DRAM_EXT_MODE2_OFFSET]

	@ wait DRAM_EXT_MODE2_REG busy to clear step 9
LOOP2:
	ldr	r1, [r0, #DRAM_EXT_MODE2_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE2_BUSY
	bne LOOP2

	@ DRAM_EXT_MODE3_REG step 10
	ldr	r1, pwc_dram_ext_mode3
	str	r1, [r0, #DRAM_EXT_MODE3_OFFSET]

	@ wait DRAM_EXT_MODE3_REG busy to clear stp10
LOOP3:
	ldr	r1, [r0, #DRAM_EXT_MODE3_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE3_BUSY
	bne	LOOP3

	@ write to DRAM_EXT_MODE1_REG register step 11
	ldr	r1, pwc_dram_ext_mode1
	str	r1, [r0, #DRAM_EXT_MODE1_OFFSET]

	@ poll for busy bit step 11
LOOP4:
	ldr	r1, [r0, #DRAM_EXT_MODE1_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE1_BUSY
	bne	LOOP4

	@
	@ Program the DLL values
	@
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET

	@ change DRAM frequency
	ldr	r1, pwc_ddr_ctr2
	str	r1, [r0, #PLL_DDR_CTRL2_OFFSET]
	ldr	r1, pwc_ddr_ctr3
	str	r1, [r0, #PLL_DDR_CTRL3_OFFSET]
	ldr	r1, pwc_dram_freq
	str	r1, [r0, #PLL_DDR_CTRL_OFFSET]
	orr	r1, r1, #0x1
	str	r1, [r0, #PLL_DDR_CTRL_OFFSET]

	mov     r1, #0x00600000
_dram_change_freq_delay:
	subs    r1, r1, #0x1
	bne     _dram_change_freq_delay

	ldr	r1, pwc_dll0
	str	r1, [r0, #DLL0_OFFSET]
	ldr	r1, pwc_dll1
	str	r1, [r0, #DLL1_OFFSET]
	ldr	r1, pwc_dll2
	str	r1, [r0, #DLL2_OFFSET]
	ldr	r1, pwc_dll3
	str	r1, [r0, #DLL3_OFFSET]
	ldr	r1, pwc_dll_ctrl_sel
	str	r1, [r0, #DLL_CTRL_SEL_OFFSET]
	ldr	r1, pwc_dll_ocd_bits
	str	r1, [r0, #DLL_OCD_BITS_OFFSET]
	ldr	r1, pwc_ddrio_calib
	str	r1, [r0, #DDRIO_CALIB_OFFSET]

	mov	r0, #AHB_BASE
	orr	r0, r0, #DRAM_OFFSET

	@ write DRAM_EXT_MODE0_REG to reset DLL
	ldr	r1, pwc_dram_ext_mode0
	orr	r1, r1, #DRAM_EXT_MODE0_DLL
	str	r1, [r0, #DRAM_EXT_MODE0_OFFSET]

	@ poll for busy bit step 12
LOOP5:
	ldr	r1, [r0, #DRAM_EXT_MODE0_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE0_BUSY
	bne	LOOP5

	@ DDR3 ZQCL
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	tst	r1, #DRAM_CONFIG_TYPE_DDR3
	beq	ddr3_zqcl_done

	@ issue ZQCL command
	mov	r1, #0x10
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	@ wait for ZQ calibration to complete
LOOP_ZQCL:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #0x10
	bne	LOOP_ZQCL
ddr3_zqcl_done:

	@ DLL_RST_EN
	mov	r1, #DRAM_INIT_CTL_DLL_RST_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait 200 clocks step 13
	mov	r2, #0x50
LOOP6:
	subs	r2, r2, #0x1
	bne	LOOP6

	@ wait for DLL_RST_EN to clear step 8
LOOP7:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_DLL_RST_EN
	bne	LOOP7

	@ Skip PRE_ALL_EN step for DDR3
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	tst	r1, #DRAM_CONFIG_TYPE_DDR3
	bne	pre_all_en_done

	@ PRE_ALL_EN step 14
	mov	r1, #DRAM_INIT_CTL_PRE_ALL_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait for PRE_ALL_BUSY to clear step 14
LOOP8:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_PRE_ALL_EN
	bne	LOOP8

	@ issue auto refresh step 15
	mov	r1, #DRAM_INIT_CTL_IMM_REF_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait for auto refresh to finish step 15
LOOP9:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_IMM_REF_EN
	bne	LOOP9

	@ wait 75ns before next refresh
	mov	r2, #0x20
LOOP10:
	subs	r2, r2, #0x1
	bne	LOOP10

	@ issue auto refresh step 16
	mov	r1, #DRAM_INIT_CTL_IMM_REF_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait for auto refresh to finish step 16
LOOP11:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_IMM_REF_EN
	bne	LOOP11

	@ set DRAM_EXT_MODE0_REG step 17
	ldr	r1, pwc_dram_ext_mode0
	bic	r1, r1, #DRAM_EXT_MODE0_DLL
	str	r1, [r0, #DRAM_EXT_MODE0_OFFSET]

	@ wait for EXT_MODE_REG not busy step 17
LOOP12:
	ldr	r1, [r0, #DRAM_EXT_MODE0_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE0_BUSY
	bne	LOOP12
pre_all_en_done:

	@ set DRAM_AUTO_REF_EN step 18
	ldr	r1, [r0, #DRAM_CTL_OFFSET]
	orr	r1, r1, #DRAM_CONTROL_AUTO_REG_EN
	str	r1, [r0, #DRAM_CTL_OFFSET]

	@ move ahead to get_rtt_en step for DDR3
	ldr	r1, [r0, #DRAM_CFG_OFFSET]
	tst	r1, #DRAM_CONFIG_TYPE_DDR3
	bne	get_rtt_en

	@ write DRAM_EXT_MODE1_REG for OCD default step 19
	ldr	r1, [r0, #DRAM_EXT_MODE1_OFFSET]
	orr	r1, r1, #0x00000380
	str	r1, [r0, #DRAM_EXT_MODE1_OFFSET]

	@ poll busy bit step 19
LOOP13:
	ldr	r1, [r0, #DRAM_EXT_MODE1_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE1_BUSY
	bne	LOOP13

	@ write DRAM_EXT_MODE_REG for OCD exit step 20
	ldr	r2, [r0, #DRAM_EXT_MODE1_OFFSET]
	and	r2, r2, #0xfffffc7f
	ldr	r1, pwc_dram_ext_mode1
	ands	r1, r1, #0x44
	orr     r1, r1, r2
	str	r1, [r0, #DRAM_EXT_MODE1_OFFSET]

	@ poll busy bit step 20
LOOP14:
	ldr	r1, [r0, #DRAM_EXT_MODE1_OFFSET]
	ands	r1, r1, #DRAM_EXT_MODE1_BUSY
	bne	LOOP14
get_rtt_en:

	@ set GET_RTT_EN bit step 21
	mov	r1, #DRAM_INIT_CTL_GET_RTT_EN
	str	r1, [r0, #DRAM_INIT_CTL_OFFSET]

	@ wait for GET_RTT to finish step 21
LOOP15:
	ldr	r1, [r0, #DRAM_INIT_CTL_OFFSET]
	ands	r1, r1, #DRAM_INIT_CTL_GET_RTT_EN
	bne	LOOP15

	@ write to DRAM_CFG setting dqs SYNC ON
	ldr	r1, glo_dram_cfg
	str	r1, [r0, #DRAM_CFG_OFFSET]

	@ set DRAM_ENABLE bit
	ldr	r1, [r0, #DRAM_CTL_OFFSET]
	orr	r1, r1, #DRAM_CONTROL_ENABLE
	str	r1, [r0, #DRAM_CTL_OFFSET]

	ldr	r2, =0x4
LOOP16:
	subs	r2, r2, #0x01
	bne	LOOP16

	@ DRAM INITIALIZATION done
	mov	pc, lr		@ Retuen to caller
