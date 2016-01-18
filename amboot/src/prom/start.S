/**
 * system/src/prom/start.s
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

.text
.align 4

@====================================================================
@ Hard Vectors
@====================================================================
.globl reset
reset:

#if 0
	/* These DDR re-programming sequence only need on A5S  */
	@	@ Change DRAM frequency	@
	mov	r0, #0x70000000
	orr	r0, r0, #0x170000

	ldr	r1, =0x3f710000
	str	r1, [r0, #0x110]
	ldr	r1, =0x00068300
	str	r1, [r0, #0x114]

	ldr	r1, =0x0f111100
	str	r1, [r0, #0x0dc]

	ldr	r1, =0x0f111101
	str	r1, [r0, #0x0dc]

	ldr	r1, =0x0f111100
	str	r1, [r0, #0x0dc]

	mov     r0, #0x00180000
	@ 50 ms at corefreq at 121.5MHz
_dram_access_delay:
	subs    r0, r0, #0x1
	bne     _dram_access_delay
#endif	/* CHIP_REV == A5S */

	@ --------------------------------------------------
	@ Flush all cache
	@ --------------------------------------------------
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0

	@ --------------------------------------------------
	@ Turn on instrucion cache and disable MMU
	@ --------------------------------------------------
	mrc	p15, 0, r0, c1, c0, 0	@ Read control register
	orr	r0, r0, #0x1000		@ Turn on bit 12
	bic	r0, r0, #0x1		@ Turn off bit 1
	mcr	p15, 0, r0, c1, c0, 0	@ Write control register

	/* Disabale IRQ/FIQ interrupts */
	mrs	r0, cpsr
	orr	r0, r0, #0xc0
	msr	cpsr_c, r0

	/* system mode */
	mrs	r1, cpsr	@ read-modify-write to get to system mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1f	@ set system mode
	msr	cpsr, r1	@ now we should be in system mode
	msr	spsr, r1	@ spsr_sys
	ldr	sp, sys_stack_end	@ r13
	mov	lr, #0x0		@ r14

	@ Clear the BSS section
	ldr	r0, bss_start		@ r0 = start address
	ldr	r1, bss_end		@ r1 = end address
	sub	r0, r1, r0		@ r0 = number of bytes
	mov	r2, #0x0		@ set value

clear_bss:				@ clear loop
	stmia	r1!, {r2}
	subs	r0, r0, #4
	bne	clear_bss

	@---------------------------------------
	@ Jump to C written entry
	@---------------------------------------

	ldr	r0, =main
	mov	lr, pc
	bx	r0

.align 4

bss_start:		.word	__bss_start
bss_end:		.word	__bss_end
sys_stack_end:		.word	__sys_stack_end
