/**
 * system/src/bst/dramfreq5l.h
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

#include <ambhw.h>

@setup ddr pll

@ base address of RCT registers
	ldr	r0, =(APB_BASE | RCT_OFFSET)

dramfreq:

	/* If the constant is 0xffffffff, then we use the */
	/* POWER_ON_CONFIG default and return to the caller */
		
	load_from_table	pll_ctrl_val, r1
	ldr	r2, =0xffffffff
	cmp	r2, r1
	beq	setup_dram_dll /* Ingore the pll change */

#if (USE_PLL_CORE_FOR_DDR == 0)
	load_from_table	pll_ctrl_val, r1
	ldr	r2, =(PLL_DDR_CTRL_OFFSET)
	str	r1, [r0, r2]

	load_from_table	pll_ddr_ctrl2_val, r1
	ldr	r2, =(PLL_DDR_CTRL2_OFFSET)
	str	r1, [r0, r2]

	load_from_table	pll_ddr_ctrl3_val, r1
	ldr	r2, =(PLL_DDR_CTRL3_OFFSET)
	str	r1, [r0, r2]

	load_from_table	pll_ctrl_val, r1
	mov	r3, #0x1
	orr	r1, r1, r3
	ldr	r2, =(PLL_DDR_CTRL_OFFSET)
	str	r1, [r0, r2]

	mvn	r3, r3
	and	r1, r3
	str	r1, [r0, r2]
#endif

#if (USE_PLL_CORE_FOR_DDR == 0)
@ use pll_out_ddr
	mov	r1, #0x1
#else
@ use pll_vco_out_core
	mov	r1, #0x2
#endif

	ldr	r2, =(USE_PLL_CORE_FOR_DDR_OFFSET)
	str	r1, [r0, r2]

@ setup ddio post scaler
#if (USE_PLL_CORE_FOR_DDR == 0)
	mov	r1, #1
#else
	load_from_table	pwc_ddrio_post_scale, r1
#endif

	ldr	r2, =(SCALER_DDRIO_POST_OFFSET)
	str	r1, [r0, r2]

@ setup dram dll
setup_dram_dll:
	load_from_table	pwc_dll0, r1
	ldr	r2, =(DLL0_OFFSET)
	str	r1, [r0, r2]

	load_from_table	pwc_dll1, r1
	ldr	r2, =(DLL1_OFFSET)
	str	r1, [r0, r2]

	load_from_table	pwc_dll_ctrl_sel, r1
	ldr	r2, =(DLL_CTRL_SEL_OFFSET)
	str	r1, [r0, r2]


#if (USE_PLL_CORE_FOR_DDR == 0)
	mov	r3, #0x20
#else
@ use pll_vco_out_core
	mov	r3, #0x40
#endif

	ldr	r2, =(PLL_LOCK_OFFSET)
dram_pll_lock:
	ldr	r1, [r0, r2]
	and	r1, r3
	beq	dram_pll_lock

