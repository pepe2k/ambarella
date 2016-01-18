/**
 * system/src/bst/start.S
 *
 * History:
 *    2005/01/26 - [Charles Chiou] created file
 *    2008/04/03 - [Charles Chiou] added bst_bsp_entry weak symbol callout
 *	support and removed undef/irq/fiq/svc/abort mode init to save space
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>

.text

.globl	_start
_start:
	b	reset
	b	bst_dead
	b	bst_dead
	b	bst_dead
	b	bst_dead
	b	bst_dead
	b	bst_dead
	b	bst_dead
	nop
	nop

POSTRELOC_START_ADDR:		.word __postreloc_start
POSTRELOC_END_ADDR:		.word __postreloc_end
AMBOOT_BST_RAM_START_ADDR:	.word AMBOOT_BST_RAM_START

	nop

.globl reset
reset:
normal_boot:

	/* Turn on instrucion cache and disable MMU */
	mrc	p15, 0, r0, c1, c0, 0	/* Read control register */
	orr	r0, r0, #0x1000		/* Turn on bit 12 */
	bic	r0, r0, #0x2000		@ Turn off bit 13 - HV
	bic	r0, r0, #0x1		/* Turn off bit 1 */
#if defined(__ARM1136JS__)
#if defined(BROKEN_UNALIGNED_ACCESS_SUPPORT)
	@orr	r0, r0, #0x2		@ Turn on bit 1 - Alignment fault
	bic	r0, r0, #0x400000	@ Turn off bit 22 - Unainged support
#else
	bic	r0, r0, #0x2		@ Turn off bit 1 - Alignment fault
	orr	r0, r0, #0x400000	@ Turn on bit 22 - Unainged support
#endif
#endif
	mcr	p15, 0, r0, c1, c0, 0	/* Write control register */

	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0	/* Write domain access control reg */

#if defined(__ARM1136JS__)
	/* -------------------------------------------------- */
	/* Redirect peripheral port 0x60000000 - 0x7fffffff   */
	/* -------------------------------------------------- */
	mov	r0, #0x60000000
	orr	r0, r0, #0x00000014
	mcr	p15, 0, r0, c15, c2, 4
#endif

	/* system mode */
	mrs	r1, cpsr	@ read-modify-write to get to SYS mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1f	@ set system mode
	msr	cpsr, r1	@ now we should be in system mode
	msr	spsr, r1	@ spsr_sys
	mov	sp,  #0x0	@ r13
	mov	lr,  #0x0	@ r14

	ldr	r0, custom_bst_sym
	cmp	r0, #0x0
	movne	lr, pc
	movne	pc, r0

.globl custom_bst_sym_done
custom_bst_sym_done:

#if	!defined(USE_RTPLL_CHANGE)
	bl	switch_core_freq	/* Change PLL for core if necessary */
#endif

#if	defined(__FPGA__)
	nop
#elif (CHIP_REV == A1) || (CHIP_REV == A2)  || 		\
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)
	bl	memsetup		/* Initialize Memory */
#elif (CHIP_REV == A3)
	bl	memsetup3		/* Initialize Memory */
#elif (CHIP_REV == A5)
	bl	memsetup5		/* Initialize Memory */
#elif (CHIP_REV == A6)
	bl	memsetup6		/* Initialize Memory */
#elif (CHIP_REV == A5L)
	ldr		r0, =memsetup5l
	blx		r0		/* Initialize Memory */
#endif

	/* Delay 2 MS in order to PLL/DLL to be locked. */
	ldr	r0, ARM_CYCLES_2MS
	bl	delay_loop

	nop

dram_cfg_done:
relocate:
	/* Relocate the 2nd stage loader to load BLD */
	ldr	r0, POSTRELOC_START_ADDR
	ldr	r1, AMBOOT_BST_RAM_START_ADDR
	ldr	r2, POSTRELOC_END_ADDR

/**
 * copy_loop:
 *
 * r0 - source address
 * r1 - target address
 * r2 - source end address
 */
copy_loop:
	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r0, r2
	ble	copy_loop

	/* The 2nd stage loader is copied to DRAM, so jump to it */
	ldr	r0, AMBOOT_BST_RAM_START_ADDR
	mov	pc, r0
	nop

/**
 * delay_loop:
 *
 * r0 - delay count: 1 sec delay is approch to (core_frequence_Hz / 5)
 *
 */
ARM_CYCLES_2MS:			.word 86400

delay_loop:
	nop
	subs	r0, r0, #1
	bne	delay_loop
	mov	pc, lr

/**
 * The bootstrap program is not going to be able to handle exceptions.
 * Therefore, this is an unrecoverable state...
 */

bst_dead:
	b	bst_dead

.align 2
custom_bst_sym:
	.word	bst_bsp_entry
	.weak	bst_bsp_entry
