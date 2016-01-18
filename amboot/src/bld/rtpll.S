
/**
 * system/src/bld/rtpll.S
 *
 * History:
 *    2009/09/25 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>

#if	(RTPLL_LOCK_ICACHE == 1) && ((CHIP_REV == A2) || \
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q))
.text

.globl rtpll_text_begin
rtpll_text_begin:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

.globl rtpll_set_core_pll
rtpll_set_core_pll:
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, rtpll_core_pll_val
	str	r1, [r0, #PLL_CORE_CTRL_OFFSET]
	orr	r1, r1, #0x1
	str	r1, [r0, #PLL_CORE_CTRL_OFFSET]
	
	/* Delay a while for lock */
	ldr	r1, =0x50000
loop0:
	subs	r1, r1, #0x1
	bne	loop0
	bx	lr

.globl rtpll_core_pll_val
rtpll_core_pll_val:	.word	0x00000000

.globl rtpll_text_end
rtpll_text_end:
	nop

#endif
