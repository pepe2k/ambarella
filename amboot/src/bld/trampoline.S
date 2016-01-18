/**
 * system/src/bld/trampoline.S
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <hotboot.h>

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

.text

/* Start of the BLD code */
.globl _trampoline
_trampoline:
	ldr	pc, =reset
	ldr	pc, =undefined_instruction
	ldr	pc, =software_interrupt
	ldr	pc, =prefetch_abort
	ldr	pc, =data_abort
	ldr	pc, =not_used
	ldr	pc, =irq
	ldr	pc, =fiq
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

reset:

	/* First read in some misc registers */
	mrc	p15, 0, r0, c0, c0, 0	/* Read ID value */
	mrc	p15, 0, r1, c0, c0, 1	/* Read cache type */
	mrc	p15, 0, r2, c0, c0, 2	/* Read TCM status */

	/* Turn on instrucion cache and disable MMU */
	mrc	p15, 0, r0, c1, c0, 0	/* Read control register */
	orr	r0, r0, #0x1000		@ Turn on bit 12 - I-cache
	bic	r0, r0, #0x2000		@ Turn off bit 13 - HV
	bic	r0, r0, #0x1		@ Turn off bit 1 - MMU
#if defined(__ARM1136JS__)
#if defined(BROKEN_UNALIGNED_ACCESS_SUPPORT)
	@orr	r0, r0, #0x2		@ Turn on bit 1 - Alignment fault
	bic	r0, r0, #0x400000	@ Turn off bit 22 - Unainged support
#else
	bic	r0, r0, #0x2		@ Turn off bit 1 - Alignment fault
	orr	r0, r0, #0x400000	@ Turn on bit 22 - Unainged support
#endif
	bic	r0, r0, #0x800000	@ Turn off bit 23 - XP
#endif
	mcr	p15, 0, r0, c1, c0, 0	/* Write control register */

	/* Give client access to domain 0 */
	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0

#if defined(__ARM1136JS__)
#if	(PHY_BUS_MAP_TYPE == 1)
	/* -------------------------------------------------- */
	/* Redirect peripheral port 0xe0000000 - 0xefffffff   */
	/* -------------------------------------------------- */
	mov	r0, #AHB_PHYS_BASE
	orr	r0, r0, #0x00000013
	mcr	p15, 0, r0, c15, c2, 4
#else
	/* -------------------------------------------------- */
	/* Redirect peripheral port 0x60000000 - 0x7fffffff   */
	/* -------------------------------------------------- */
	mov	r0, #AHB_PHYS_BASE
	orr	r0, r0, #0x00000014
	mcr	p15, 0, r0, c15, c2, 4
#endif
#endif

	/* Disabale IRQ/FIQ interrupts */
	mrs	r0, cpsr
	orr	r0, r0, #0xc0
	msr	cpsr_c, r0

	/* Clear regs. */
	mov	r0, #0
	mov	r1, #0
	mov	r2, #0
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
	mov	r9, #0
	mov	r10, #0
	mov	r11, #0
	mov	r12, #0

	/* undef mode */
	mrs	r1, cpsr
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1b	@ set UNDEF mode
	msr	cpsr, r1	@ now we should be in UNDEF mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, und_stack_end	@ r13_und
	mov	lr, #0x0		@ r14_und

	/* abort mode */
	mrs	r1, cpsr
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x17	@ set ABORT mode
	msr	cpsr, r1	@ now we should be in ABORT mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, abt_stack_end	@ r13_abort
	mov	lr, #0x0		@ r14_abort

	/* irq mode */
	mrs	r1, cpsr	@ read-modify-write to get to IRQ mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x12	@ set IRQ mode
	msr	cpsr, r1	@ now we should be in IRQ mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, irq_stack_end	@ r13_irq
	mov	lr, #0x0		@ r14_irq

	/* fiq mode */
	mrs	r1, cpsr	@ read-modify-write to get to FIQ mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x11	@ set FIQ mode
	msr	cpsr, r1	@ now we should be in FIQ mode
	msr	spsr, r1	@ spsr_fiq
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, fiq_stack_end	@ r13_fiq
	mov	lr, #0x0		@ r14_fiq

	/* svc mode */
	mrs	r1, cpsr	@ read-modify-write to get to supervisor mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x13	@ set supervisor mode
	msr	cpsr, r1	@ now we should be in supervisor mode
	msr	spsr, r1	@ spsr_svc
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, svc_stack_end	@ r13_svc
	mov	lr, #0x0		@ r14_svc

	/* system mode */
	mrs	r1, cpsr	@ read-modify-write to get to system mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1f	@ set system mode
	msr	cpsr, r1	@ now we should be in system mode
	msr	spsr, r1	@ spsr_sys
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, sys_stack_end	@ r13
	mov	lr, #0x0		@ r14

	/* Some magics to make hw work fine. */
	#include <gumo.S>

	/* Get & save hotboot info */
	mov	r0, #DRAM_START_ADDR
	orr	r0, r0, #0x80000
	sub	r0, r0, #0x10
	ldr	r1, [r0, #0x0]
	ldr	r2, hotboot_magic0
	cmp	r1, r2
	bne	hotboot_done
	ldr	r1, [r0, #0xc]
	ldr	r2, hotboot_magic1
	bne	hotboot_done
	mov	r1, #0x1
	ldr	r2, hotboot_valid_addr
	str	r1, [r2]
	ldr	r1, [r0, #0x4]
	ldr	r2, hotboot_pattern_addr
	str	r1, [r2]
	mov	r1, #0x0
	str	r1, [r0, #0x0]
	str	r1, [r0, #0x4]
	str	r1, [r0, #0x8]
	str	r1, [r0, #0xc]
hotboot_done:

	/* Initialize GPIO */
	bl	gpio_init

	/* Initialize LED for debug usage */
	bl	led_init

	/* Set up the page tables and enable MMU */
	bl	mmu_init
	bl	_enable_dcache

	/* Zero out the BSS section */
	/* r1 = start address */
	/* r2 = end address */
	ldr	r1, bss_start
	ldr	r2, bss_end

	mov	r0, #0
1:
	stmia	r1!, {r0}
	cmp	r1, r2
	blo	1b

	/* Jump to C code */
	bl	main

	/* If main ever returns we just call it again */
	b	_trampoline

/* Busy Wait loop */
wait:
	mov	r5, #0x1000000
wait_:
	subs	r5, r5, #1
	bne	wait_
	mov	pc, lr

/* Blink LED for ever */
endless_blink:
	bl	wait
	mov	r0, r6
	bl	led_blink
	b	endless_blink

undefined_instruction:
	mov	r6, #3
	b	endless_blink

software_interrupt:
	mov	r6, #4
	b	endless_blink

prefetch_abort:
	mov	r6, #5
	b	endless_blink

data_abort:
	mov	r6, #6
	b	endless_blink

not_used:
	mov	r6, #7
	b	endless_blink

irq:
        sub	sp, sp, #S_FRAME_SIZE
        stmia	sp, {r0 - r12}		@ Calling r0-r12
					@ !!!! R8 NEEDS to be saved !!!!
					@ a reserved stack spot would be good.
        add	r8, sp, #S_PC
        stmdb	r8, {sp, lr}^		@ Calling SP, LR
        str	lr, [r8, #0]		@ Save calling PC
        mrs	r6, spsr
        str	r6, [r8, #4]		@ Save CPSR
        str	r0, [r8, #8]		@ Save OLD_R0
        mov	r0, sp

	mov	r0, #AHB_BASE
	orr	r0, r0, #VIC_OFFSET
	ldr	r5, [r0, #0x0]		@ read irq status register
#if (ETH_INSTANCES >= 2)
	mov	r0, #AHB_BASE
	orr	r0, r0, #VIC2_OFFSET
	ldr	r5, [r0, #0x0]		@ read irq status register
	tst	r5, #(1 << 20)		@ test for Ethernet2 interrupt
	bne	irq_eth			@ Ethernet irq handler
.globl	irq_eth_done
irq_eth_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #VIC_OFFSET
	ldr	r5, [r0, #0x0]		@ read irq status register
	tst	r5, #0x10		@ test for USB interrupt
	bne	irq_usb			@ usb irq handler
#elif (ETH_INSTANCES >= 1)
	tst	r5, #(1 << ETH_INT_VEC)	@ test for Ethernet interrupt
	bne	irq_eth			@ Ethernet irq handler
.globl	irq_eth_done
irq_eth_done:
	tst	r5, #0x10		@ test for USB interrupt
	bne	irq_usb			@ usb irq handler
#else
	tst	r5, #0x10		@ test for USB interrupt
	bne	irq_usb			@ usb irq handler
#endif
	tst	r5, #0x1000		@ test for timer 1 interrupt
	bne	irq_timer1		@ timer 1 irq handler
	tst	r5, #0x2000		@ test for timer 2 interrupt
	bne	irq_timer2		@ timer 2 irq handler
	tst	r5, #0x4000		@ test for timer 3 interrupt
	bne	irq_timer3		@ timer 3 irq handler
.globl	irq_done
irq_done:

        ldmia	sp, {r0 - lr}^		@ Calling r0 - lr
        mov	r0, r0
        ldr	lr, [sp, #S_PC]		@ Get PC
        add	sp, sp, #S_FRAME_SIZE
        subs	pc, lr, #4		@ return & move spsr_svc into cpsr

fiq:
	mov	r6, #9
	b	endless_blink

bss_start:	.word	__bss_start
bss_end:	.word	__bss_end
und_stack_end:	.word	__und_stack_end
abt_stack_end:	.word	__abt_stack_end
irq_stack_end:	.word	__irq_stack_end
fiq_stack_end:	.word	__fiq_stack_end
svc_stack_end:	.word	__svc_stack_end
sys_stack_end:	.word	__sys_stack_end

hotboot_magic0:		.word	HOTBOOT_MAGIC0
hotboot_magic1:		.word	HOTBOOT_MAGIC1
hotboot_valid_addr:	.word	hotboot_valid
hotboot_pattern_addr:	.word	hotboot_pattern
