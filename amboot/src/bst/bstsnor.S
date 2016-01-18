/**
 * system/src/bst/bstsnor.S
 *
 * History:
 *    2009/09/21 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2008-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <flash/flpart.h>
#include <fio/firmfl.h>
#include "partinfo.h"

.text

#if !defined(CONFIG_SNOR_NONE)

#define SNOR_MIN_DMA_SIZE	2048

snor_ctr1_val:		.word	(SNOR_CONTROL1 | \
				 SNOR_CTR1_CE_SEL(0) | \
				 SNOR_CTR1_WP)

snor_ctr2_val:		.word	(SNOR_CONTROL2 | \
				 SNOR_CTR2_CMD_CYC(0) | \
				 SNOR_CTR2_DTA_CYC(1) | \
				 SNOR_CTR2_DMA_CMD)

/**
 * Initialize the FIO and load the AMBOOT_BLD code into the memory, and then
 * jump to it.
 */
.globl snor_load_bld
snor_load_bld:
	/* Setup timing parameters */
#if 0
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, =SNOR_TIM0_VAL
	str	r1, [r0, #SNOR_TIM0_OFFSET]
	ldr	r1, =SNOR_TIM1_VAL
	str	r1, [r0, #SNOR_TIM1_OFFSET]
	ldr	r1, =SNOR_TIM2_VAL
	str	r1, [r0, #SNOR_TIM2_OFFSET]
	ldr	r1, =SNOR_TIM3_VAL
	str	r1, [r0, #SNOR_TIM3_OFFSET]
	ldr	r1, =SNOR_TIM4_VAL
	str	r1, [r0, #SNOR_TIM4_OFFSET]
	ldr	r1, =SNOR_TIM5_VAL
	str	r1, [r0, #SNOR_TIM5_OFFSET]
	ldr	r1, =SNOR_TIM6_VAL
	str	r1, [r0, #SNOR_TIM6_OFFSET]
	ldr	r1, =SNOR_TIM7_VAL
	str	r1, [r0, #SNOR_TIM7_OFFSET]
	ldr	r1, =SNOR_TIM8_VAL
	str	r1, [r0, #SNOR_TIM8_OFFSET]
	ldr	r1, =SNOR_TIM9_VAL
	str	r1, [r0, #SNOR_TIM9_OFFSET]
#endif

	/* Start loading BLD */
	b	do_snor_bld

	/* Jump to_BLD now that it has been loaded */
snor_jump_to_bld:
	ldr	r0, =AMBOOT_BLD_RAM_START
	mov	pc, r0

/**
 * Perform BLD loading on the SNOR flash
 *
 * r0 - r2 - Garbled by the page loader function, so they are used as tmp var.
 * r3 - DRAM address
 * r4 - Start sector #
 * r5 - Length (variable)
 * r6 - End sector #
 *
 * r7 - Length of BLD code need to load
 */
do_snor_bld:
	bl	clear_snor_regs

	/* Load PTB first */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #SNOR_PTB_SSEC
	mov	r5, #SNOR_MIN_DMA_SIZE
	bl	snor_load_sector

	ldr	r0, =FLPART_MAGIC
check_bst_magic:
	/* Verify PTB's BST magic signature */
	ldr	r7, [r3, #0x18]		@ WARNING: make sure offset is right!
	cmp	r0, r7
	bne	check_bst_magic		/* If test fails, loop forever... */

check_bld_magic:
	/* Verify PTB's BLD magic signature */
	ldr	r7, [r3, #0x34]		@ WARNING: make sure offset is right!
	cmp	r0, r7
	bne	check_bld_magic		/* If test fails, loop forever... */

	/* Get the BLD length from page just loaded */
	ldr	r7, [r3, #0x28]		@ WARNING: make sure offset is right!

	/* Load the BLD code into memory */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #SNOR_BLD_SSEC
	mov	r6, #SNOR_BLD_NSEC
	add	r6, r4, r6
snor_load_sector_loop:
	cmp	r4, r6
	beq	snor_jump_to_bld		/* No more blocks to load */
	mov	r5, r7
	bl	get_sec_size
	cmp	r5, r9			/* r9 contains sector size */
	movhi	r5, r9
	/* Round the length argument to multiple of 2048 (NOR controller restriction) */
	ldr	r1, =(SNOR_MIN_DMA_SIZE - 1)
	and	r0, r5, r1
	cmp	r0, #0
	beq	snor_len_fixed
	add	r5, r5, #SNOR_MIN_DMA_SIZE
	sub	r5, r5, r0
snor_len_fixed:
	bl	snor_load_sector		/* Load one sector */
	add	r3, r3, r5
	add	r4, r4, #0x1
	sub	r7, r7, r5
	b	snor_load_sector_loop

/**
 * Get sector size on the SNOR flash
 * r4 - Sector # (input)
 * r8 - block1s (internal var)
 * r9 - Sector size (output)
 */
get_sec_size:
	ldr	r8, =SNOR_BLOCK1S_PER_BANK
	cmp	r4, r8
	ldrlt	r9, =SNOR_BLOCK1_SIZE
	ldrge	r9, =SNOR_BLOCK2_SIZE
	mov	pc, lr

/**
 * Get sector addr on the SNOR flash
 * r4 - Sector # (input)
 * r8 - block1s (internal var)
 * r9 - block_size (internal var)
 * r10 - block2s (internal var)
 * r2 - Sector addr (output)
 */
get_sec_addr:
	ldr	r8, =SNOR_BLOCK1S_PER_BANK
	subs	r10, r4, r8
	movlt	r8, r4		/* If block# in block1s, move block1s to r8 */
	ldr	r9, =SNOR_BLOCK1_SIZE
	mul	r2, r8, r9	/* Caculate block1s addr */
	ldrgt	r9, =SNOR_BLOCK2_SIZE
	mulgt	r8, r10, r9
	addgt	r2, r2, r8
	mov	pc, lr

/**
 * Load a sector from flash to memory
 * (Garbles r0 - r1).
 */
clear_snor_regs:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET

	/* Clear Flash Interrupt Register */
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	/* Clear Flash Command Register */
	mov	r1, #0x0
	str	r1, [r0, #FLASH_CMD_OFFSET]

	/* Clear Flash IO DMA Control Register */
	mov	r1, #0x0
	str	r1, [r0, #FIO_DMACTR_OFFSET]

	/* Clear Flash IO DMA Status Register */
	mov	r1, #0x0
	str	r1, [r0, #FIO_DMASTA_OFFSET]

	/* Clear DMA Chan Control Register */
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	mov	r1, #0x0
	str	r1, [r0, #DMA_CHAN0_CTR_OFFSET]
	mov	pc, lr

/**
 * Load a sector from flash to memory
 * (Garbles r0 - r1).
 *
 * r3 - DRAM address
 * r4 - Sector
 * r5 - Length
 */
snor_load_sector:
	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup Flash Control Register */
	ldr	r1, snor_ctr1_val
	str	r1, [r0, #SNOR_CTR1_OFFSET]
	ldr	r1, snor_ctr2_val
	str	r1, [r0, #SNOR_CTR2_OFFSET]

	/* Setup external DMA engine transfer */
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	mov	r1, #0
	str	r1, [r0, #DMA_CHAN0_STA_OFFSET]

	mov	r1, #FIO_FIFO_BASE
	str	r1, [r0, #DMA_CHAN0_SRC_OFFSET]

	str	r3, [r0, #DMA_CHAN0_DST_OFFSET]

	mov	r1, #DMA_CHANX_CTR_EN
	orr	r1, r1, #DMA_CHANX_CTR_WM
	orr	r1, r1, #DMA_CHANX_CTR_NI
#if	defined(__FPGA__)
	orr	r1, r1, #DMA_CHANX_CTR_BLK_32B
	orr	r1, r1, #DMA_CHANX_CTR_TS_4B
#else
	orr	r1, r1, #DMA_CHANX_CTR_BLK_512B
	orr	r1, r1, #DMA_CHANX_CTR_TS_4B
#endif
	orr	r1, r1, r5	/* length */
	str	r1, [r0, #DMA_CHAN0_CTR_OFFSET]

	/* Compute sector offset */
	mov	r11, lr		/* backup lr */
	bl	get_sec_addr
	mov	lr, r11		/* restore lr */

	/* Finally, write the address of target memory */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	str	r2, [r0, #FIO_DMAADR_OFFSET]

	/* Setup Flash IO DMA Ccontrol Register */
	mov	r1, #FIO_DMACTR_EN
	orr	r1, r1, #FIO_DMACTR_FL
#if	defined(__FPGA__)
	orr	r1, r1, #FIO_DMACTR_BLK_32B
	orr	r1, r1, #FIO_DMACTR_TS4B
#else
	orr	r1, r1, #FIO_DMACTR_BLK_512B
	orr	r1, r1, #FIO_DMACTR_TS4B
#endif
	orr	r1, r1, r5	/* length */
	str	r1, [r0, #FIO_DMACTR_OFFSET]

	/* Wait for interrupt for operation done */
_wait_snor_cmd_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NOR_INT_DI
	cmp	r1, #NOR_INT_DI
	bne	_wait_snor_cmd_done

	/* Wait for interrupt from FIO DMA */
_wait_snor_fiodma_done:
	ldr	r1, [r0, #FIO_DMASTA_OFFSET]
	and	r1, r1, #FIO_DMASTA_DN
	cmp	r1, #FIO_DMASTA_DN
	bne	_wait_snor_fiodma_done

	/* Wait for interrupt from DMA */
_wait_snor_dma_done:
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	ldr	r1, [r0, #DMA_INT_OFFSET]
	and	r1, r1, #(0x1 << FIO_DMA_CHAN)
	cmp	r1, #0
	beq	_wait_snor_dma_done

	/* Clear SNOR interrupt register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	/* Clear FIODMA status register */
	str	r1, [r0, #FIO_DMASTA_OFFSET]

	/* Clear DMA status register */
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	mov	r1, #0x0
	str	r1, [r0, #DMA_INT_OFFSET]

eot_snor:
	mov	pc, lr

#endif  /* !CONFIG_SNOR_NONE */
