/**
 * system/src/bst/bstonenand.S
 *
 * History:
 *    2009/10/08 - [Evan Chen] created file
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
#include <flash/onenand.h>
#include "partinfo.h"
#include <fio/ftl_const.h>

.text

#if !defined(CONFIG_ONENAND_NONE)

#define BSTONENAND_STACK_START		0xc0043e20

#define MIN_MAIN_DMA_SIZE		2048
#define MIN_SPARE_DMA_SIZE		64

#define ONENAND_CMD_ADDR_SHIFT		4
#define ONENAND_ADDR_HI_SHIFT		16

#define ONENAND_DEV_SYS_CFG1_VAL	(ONENAND_SYS_CFG1_DEF_BRL | \
					 ONENAND_SYS_CFG1_RDY_POL | \
					 ONENAND_SYS_CFG1_INT_POL | \
					 ONENAND_SYS_CFG1_IOBE	  | \
					 ONENAND_SYS_CFG1_RDY_CONF)

#define ONENAND_R_ASYNC_ADDR0	(ONENAND_CMD_ADDR_HI(ONENAND_DEV_ADDR8_REG) | \
				 ONENAND_CMD_ADDR_LO(ONENAND_DEV_ADDR1_REG))
#define ONENAND_R_ASYNC_ADDR1	(ONENAND_CMD_ADDR_HI(ONENAND_DEV_CMD_REG) | \
				 ONENAND_CMD_ADDR_LO(ONENAND_DEV_INT_STA_REG))
#define ONENAND_R_ASYNC_ADDR2	(ONENAND_CMD_ADDR_LO(ONENAND_DEV_CTR_STA_REG))


onenand_ctr1_val:	.word	(ONENAND_CONTROL1 | \
				 ONENAND_CTR1_CE_SEL(0))

onenand_ctr2_val:	.word \
		(ONENAND_CONTROL2 | \
		 ONENAND_CTR2_POLL_POLAR | \
		 ONENAND_CTR2_POLL_TARGET_BIT(ONENAND_DATA_POLL_TARGET_BIT))

/**
 * Initialize the FIO and load the AMBOOT_BLD code into the memory, and then
 * jump to it.
 */
.globl onenand_load_bld
onenand_load_bld:
	/* Setup timing parameters */
#if 0
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, =ONENAND_TIM0_VAL
	str	r1, [r0, #ONENAND_TIM0_OFFSET]
	ldr	r1, =ONENAND_TIM1_VAL
	str	r1, [r0, #ONENAND_TIM1_OFFSET]
	ldr	r1, =ONENAND_TIM2_VAL
	str	r1, [r0, #ONENAND_TIM2_OFFSET]
	ldr	r1, =ONENAND_TIM3_VAL
	str	r1, [r0, #ONENAND_TIM3_OFFSET]
	ldr	r1, =ONENAND_TIM4_VAL
	str	r1, [r0, #ONENAND_TIM4_OFFSET]
	ldr	r1, =ONENAND_TIM5_VAL
	str	r1, [r0, #ONENAND_TIM5_OFFSET]
	ldr	r1, =ONENAND_TIM6_VAL
	str	r1, [r0, #ONENAND_TIM6_OFFSET]
	ldr	r1, =ONENAND_TIM7_VAL
	str	r1, [r0, #ONENAND_TIM7_OFFSET]
	ldr	r1, =ONENAND_TIM8_VAL
	str	r1, [r0, #ONENAND_TIM8_OFFSET]
	ldr	r1, =ONENAND_TIM9_VAL
	str	r1, [r0, #ONENAND_TIM9_OFFSET]
#endif

	/* Setup stack pointer */
	ldr     sp, =BSTONENAND_STACK_START

	/* Start loading BLD */
	b	do_onenand_bld

	/* Jump to_BLD now that it has been loaded */
onenand_jump_to_bld:
	ldr	r0, =AMBOOT_BLD_RAM_START
	mov	pc, r0

/**
 * Perform BLD loading on the ONENAND flash
 *
 * r0 - r2 - Garbled by the page loader function, so they are used as tmp var.
 *
 * r3 - DRAM address
 * r4 - Start block #
 * r5 - page (variable)
 * r6 - main or spare
 * r7 - End block #
 *
 * r8 - Length of BLD
 * r9 - Length of code loaded
 */
do_onenand_bld:
	bl	clear_onenand_regs

	/* Load SYS_CONFIG_REG */

#if 1
	/* Reset onenand chip */
	ldr	r10, =ONENAND_DEV_CMD_REG
	ldr	r11, =ONENAND_RESET_ONENAND
	bl	onenand_write_reg
#endif

	/* Set ONENAND_DEV_SYS_CFG1 */
	ldr	r10, =ONENAND_DEV_SYS_CFG1_REG
	ldr	r11, =ONENAND_DEV_SYS_CFG1_VAL
	bl	onenand_write_reg

	/* Load PTB first */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #ONENAND_PTB_SBLK
	mov	r5, #0			/* Start form page 0 */

find_good_ptb:
	bl	onenand_check_block	/* Find good PTB block */
	cmp	r0, #0
	beq	found_ptb
	add	r4, r4, #1
	bne	find_good_ptb
found_ptb:
	mov	r6, #0			/* Main area */
	bl	onenand_load_page	/* Load PTB block */

	ldr	r0, =FLPART_MAGIC
check_bst_magic:
	/* Verify PTB's BST magic signature */
	ldr	r8, [r3, #0x18]		@ WARNING: make sure offset is right!
	cmp	r0, r8
	bne	check_bst_magic		/* If test fails, loop forever... */

check_bld_magic:
	/* Verify PTB's BLD magic signature */
	ldr	r8, [r3, #0x34]		@ WARNING: make sure offset is right!
	cmp	r0, r8
	bne	check_bld_magic		/* If test fails, loop forever... */

	/* Get the BLD length from page just loaded */
	ldr	r8, [r3, #0x44]		@ WARNING: make sure offset is right!
	mov	r9, #0

	/* Load the BLD code into memory */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #ONENAND_BLD_SBLK
	mov	r7, #ONENAND_BLD_NBLK
	add	r7, r4, r7
onenand_load_block_loop:
	cmp	r4, r7
	beq	onenand_jump_to_bld	/* No more blocks to load */
	mov	r5, #0			/* Set page number to 0 */
	bl	onenand_check_block
	cmp	r0, #0
	add	r4, r4, #1
	bne	onenand_load_block_loop	/* Skip block */
	sub	r4, r4, #1

onenand_load_page_loop:
	mov	r6, #0				/* Main area */
	bl	onenand_load_page		/* Load pages */
	add	r9, r9, #MIN_MAIN_DMA_SIZE	/* increment count */
	cmp	r9, r8
	bhi	onenand_jump_to_bld		/* Loaded more indicated length */
	add	r3, r3, #MIN_MAIN_DMA_SIZE	/* increment target address */
	ldr	r0, =ONENAND_MAIN_SIZE		/* Check main area size */
	cmp	r0, #MIN_MAIN_DMA_SIZE
	addne	r5, r5, #2
	addeq	r5, r5, #1			/* increment page */
	ldr	r0, =ONENAND_PAGES_PER_BLOCK
	cmp	r5, r0
	bne	onenand_load_page_loop
	add	r4, r4, #1		/* increment block */
	b	onenand_load_block_loop

/**
 * Get page addr on the ONENAND flash
 *
 * r2 - Page addr (output)
 */
get_page_addr:
	/* addr = (blk * page_per_blk + page) * main_size */
	ldr	r0, =ONENAND_PAGES_PER_BLOCK
	mul	r2, r4, r0
	add	r2, r2, r5
	ldr	r1, =ONENAND_MAIN_SIZE
	mul	r0, r2, r1
	mov	r2, r0

	mov	pc, lr

/**
 * Load a sector from flash to memory
 * (Garbles r0 - r1).
 */
clear_onenand_regs:
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
 * Write ONENAND flash register
 * (Garbles r0 - r2)
 *
 * r3 - DRAM address
 * r4 - Start block #
 * r5 - page (variable)
 * r6 - main or spare
 * r7 - End block #
 *
 * r8 - Length of BLD
 * r9 - Length of code loaded
 *
 * arg:
 * r10 - Register address
 * r11 - Program value
 */
onenand_write_reg:

	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup ONENAND Flash Control Register */
	ldr	r1, onenand_ctr1_val
	str	r1, [r0, #ONENAND_CTR1_OFFSET]
	ldr	r1, onenand_ctr2_val
	orr	r1, r1, #ONENAND_CTR2_DTA_CYC(ONENAND_WRITE_REG_DATA_CYC)
	orr	r1, r1, #ONENAND_CTR2_POLL_POLAR
	str	r1, [r0, #ONENAND_CTR2_OFFSET]

	/* Setup program data pattern Register */
	str	r11, [r0, #ONENAND_PROG_DTA_OFFSET]

	/* Setup command register */
	mov	r1, #ONENAND_CMD_WRITE_REG
	mov	r10, r10, lsl #ONENAND_CMD_ADDR_SHIFT
	orr	r1, r1, r10
	str	r1, [r0, #ONENAND_CMD_OFFSET]

_wait_onenand_cmd_w_reg_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NAND_INT_DI
	cmp	r1, #NAND_INT_DI
	bne	_wait_onenand_cmd_w_reg_done

	/* Clear ONENAND interrupt register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	mov	pc, lr

/**
 * Polling ONENAND flash INT_STA register
 * (Garbles r0 - r2)
 *
 * r3 - DRAM address
 * r4 - Start block #
 * r5 - page (variable)
 * r6 - main or spare
 * r7 - End block #
 *
 * r8 - Length of BLD
 * r9 - Length of code loaded
 *
 */
onenand_poll_int_reg:

	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup ONENAND Flash Control Register */
	ldr	r1, onenand_ctr1_val
	orr	r1, r1, #ONENAND_CTR1_AVD_EN
	mov	r2, #ONENAND_CTR1_POLL_MODE(ONENAND_POLL_MODE_REG_POLL)
	orr	r1, r1, r2
	str	r1, [r0, #ONENAND_CTR1_OFFSET]

	ldr	r1, onenand_ctr2_val
	orr	r1, r1, #ONENAND_CTR2_POLL_POLAR
	mov	r2, #ONENAND_CTR2_POLL_TARGET_BIT(ONENAND_DATA_POLL_TARGET_BIT)
	orr	r1, r1, r2
	str	r1, [r0, #ONENAND_CTR2_OFFSET]

	/* Setup command register */
	ldr 	r10, =ONENAND_DEV_INT_STA_REG
	mov	r10, r10, lsl #ONENAND_CMD_ADDR_SHIFT
	orr	r10, r10, #ONENAND_CMD_POLL_REG
	str	r10, [r0, #ONENAND_CMD_OFFSET]

_wait_onenand_cmd_poll_reg_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NAND_INT_DI
	cmp	r1, #NAND_INT_DI
	bne	_wait_onenand_cmd_poll_reg_done

	/* Clear ONENAND interrupt register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	mov	pc, lr

/**
 * Unlock Onenand flash a block
 * (Garbles r0 - r1).
 *
 * r3 - DRAM address
 * r5 - Page
 * r6 - Main or Spare area
 *
 * arg:
 * r4 - Block
 */
onenand_unlock_block:
	stmfd	sp!, {lr}

	/* Write Start_BLK addr  */
	ldr 	r10, =ONENAND_DEV_BLK_ADDR_REG
	mov	r11, r4
	bl	onenand_write_reg

	/* Write INT */
	ldr	r10, =ONENAND_DEV_INT_STA_REG
	mov	r11, #0
	bl onenand_write_reg

	/* Write CMD : set ONENAND_DEV_CMD_REG */
	ldr	r10, =ONENAND_DEV_CMD_REG
	mov	r11, #ONENAND_UNLOCK_BLK
	bl onenand_write_reg

	/* Poll INT */
	bl	onenand_poll_int_reg	/* block is unlocked */

	ldmfd	sp, {pc}

/**
 * Check if a block is bad. The result is returned in r0
 * (Garbles r0 - r2, r12 is used for lr)
 *
 * r3 - DRAM address (for temp storage of spare-area data)
 * r4 - Block
 * r0 - 0 means it is a good block, otherwise it is a bad block
 */
onenand_check_block:
	mov	r12, lr			/* Save lr */
#if 0
	/* There's some problem in block check */
	bl	onenand_unlock_block	/* Unlock this block */

	mov	r6, #1
_onenand_check_block_loop:
	bl	onenand_load_page
	ldrb	r0, [r3, #NAND_BB_MARKER_OFFSET]
	subs	r0, r0, #0xff		/* r1 = r1 - 0xff */
	movne	r0, #1			/* If bad block, r0 = 1 */
	movne	pc, r12			/* If bad block, return */
	ldr	r1, =ONENAND_SPARE_SIZE
	cmp	r1, #MIN_SPARE_DMA_SIZE
	addne	r5, r5, #2
	addeq	r5, r5, #1		/* increment page */
	cmp	r5, #BAD_BLOCK_PAGES 	/* Check bad block pages */
	bne	_onenand_check_block_loop
	mov	r5, #0			/* All pages be checked, set r5 to 0 */
	mov	pc, r12			/* return  */
#else
	mov	r0, #0
	mov	r5, #0			/* All pages be checked, set r5 to 0 */
	mov	pc, r12			/* return  */
#endif

/**
 * Setup ONENAND controller DMA engine
 * (Garbles r0 - r1).
 *
 * r3 - DRAM address
 * r4 - Block
 * r5 - Page
 * r6 - Main or Spare area
 */
onenand_load_page_dma:

	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	cmp	r6, #0
	orrne	r1, r1, #FIO_CTR_RS
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup ONENAND Flash Control Register */
	ldr	r1, onenand_ctr1_val
	orr	r1, r1, #ONENAND_CTR1_AVD_EN
	orr	r1, r1, #ONENAND_CTR1_POLL_MODE(ONENAND_POLL_MODE_REG_INT_POLL)
	cmp	r6, #0
	orrne	r1, r1, #ONENAND_CTR1_SPARE_EN
	str	r1, [r0, #ONENAND_CTR1_OFFSET]		/* set ctr1 */

	ldr	r1, onenand_ctr2_val
	bic	r1, r1, #ONENAND_CTR2_POLL_POLAR
	orr	r1, r1, #ONENAND_CTR2_ONENAND_DMA
	orr	r1, r1, #ONENAND_CTR2_DMA_CMD
	orr	r1, r1, #ONENAND_CTR2_CMD_CYC(ONENAND_READ_ASYNC_CMD_CYC)
	orr	r1, r1, #0x100
	str	r1, [r0, #ONENAND_CTR2_OFFSET]		/* set ctr2 */

	/* Setup command address and data register */
	ldr	r1, =ONENAND_R_ASYNC_ADDR0
	str	r1, [r0, #ONENAND_CMD_ADDR0_OFFSET]	/* set cmd addr0 */

	mov	r1, r5, lsl #ONENAND_ADDR_HI_SHIFT
	orr	r1, r1, r4
	str	r1, [r0, #ONENAND_CMD_WORD0_OFFSET]	/* set cmd word0 */

	ldr	r1, =ONENAND_R_ASYNC_ADDR1
	str	r1, [r0, #ONENAND_CMD_ADDR1_OFFSET]	/* set cmd addr1 */

	mov	r1, #ONENAND_CMD_WORD_HI(ONENAND_LOAD_SEC)
	cmp	r6, #0
	movne	r1, #ONENAND_CMD_WORD_HI(ONENAND_LOAD_SPARE)
	str	r1, [r0, #ONENAND_CMD_WORD1_OFFSET]	/* set cmd word1 */

	ldr	r1, =ONENAND_R_ASYNC_ADDR2
	str	r1, [r0, #ONENAND_CMD_ADDR2_OFFSET]	/* set cmd addr2 */

	mov	r1, #0x0
	str	r1, [r0, #ONENAND_CMD_WORD2_OFFSET]	/* set cmd word2 */

	/* Setup command register */
	mov	r1, #ONENAND_CMD_ADDR(ONENAND_DAT_RAM_START)
	cmp	r6, #0
	ldrne	r1, =ONENAND_CMD_ADDR(ONENAND_SPARE_RAM_START)
	str	r1, [r0, #ONENAND_CMD_OFFSET]

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
	cmp	r6, #0
	orreq	r1, r1, #MIN_MAIN_DMA_SIZE
	orrne	r1, r1, #MIN_SPARE_DMA_SIZE	/* length */
	str	r1, [r0, #DMA_CHAN0_CTR_OFFSET]

	/* Compute page offset */
	mov	r11, lr		/* backup lr */
	bl	get_page_addr
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
	cmp	r6, #0
	orreq	r1, r1, #MIN_MAIN_DMA_SIZE
	orrne	r1, r1, #MIN_SPARE_DMA_SIZE	/* length */
	str	r1, [r0, #FIO_DMACTR_OFFSET]

	/* Wait for interrupt for operation done */
_wait_onenand_cmd_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NAND_INT_DI
	cmp	r1, #NAND_INT_DI
	bne	_wait_onenand_cmd_done

	/* Wait for interrupt from FIO DMA */
_wait_onenand_fiodma_done:
	ldr	r1, [r0, #FIO_DMASTA_OFFSET]
	and	r1, r1, #FIO_DMASTA_DN
	cmp	r1, #FIO_DMASTA_DN
	bne	_wait_onenand_fiodma_done

	/* Wait for interrupt from DMA */
_wait_onenand_dma_done:
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	ldr	r1, [r0, #DMA_INT_OFFSET]
	and	r1, r1, #(0x1 << FIO_DMA_CHAN)
	cmp	r1, #0
	beq	_wait_onenand_dma_done

	/* Clear ONENAND interrupt register */
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

	mov	pc, lr		/* return */

/**
 * Load a page from flash to memory
 * (Garbles r0 - r1).
 *
 * r3 - DRAM address
 * r4 - Block
 * r5 - Page
 * r6 - Main or Spare area
 */
onenand_load_page:
	stmfd	sp!, {lr}

	/* Write Start buffer */
	ldr	r10, =ONENAND_DEV_BUF_START_REG
	mov	r11, #ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT)
	bl	onenand_write_reg

	/* set SYS_CFG1 */

	bl	onenand_load_page_dma

eot_onenand:
	ldmfd	sp, {pc}

#endif  /* !CONFIG_ONENAND_NONE */

