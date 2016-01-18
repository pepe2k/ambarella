/**
 * system/src/bst/bstnand.S
 *
 * History:
 *    2005/01/26 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
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
#include <fio/ftl_const.h>

/* For the 2K-type NAND flash, we always use this setting to read it. */
#define NAND_2K_CONFIG	(NAND_CTR_C2 | NAND_CTR_RC | NAND_CTR_SZ_8G)

.text

#if !defined(CONFIG_NAND_NONE)

/**
 * Initialize the FIO and load the AMBOOT_BLD code into the memory, and then
 * jump to it.
 */
.globl nand_load_bld
nand_load_bld:
	/* Setup timing parameters */
#if 0
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, =NAND_TIM0_VAL
	str	r1, [r0, #FLASH_TIM0_OFFSET]
	ldr	r1, =NAND_TIM1_VAL
	str	r1, [r0, #FLASH_TIM1_OFFSET]
	ldr	r1, =NAND_TIM2_VAL
	str	r1, [r0, #FLASH_TIM2_OFFSET]
	ldr	r1, =NAND_TIM3_VAL
	str	r1, [r0, #FLASH_TIM3_OFFSET]
	ldr	r1, =NAND_TIM4_VAL
	str	r1, [r0, #FLASH_TIM4_OFFSET]
	ldr	r1, =NAND_TIM5_VAL
	str	r1, [r0, #FLASH_TIM5_OFFSET]
#endif

	/* Start loading BLD */
	b	do_nand_bld

	/* Jump to_BLD now that it has been loaded */
nand_jump_to_bld:
	ldr	r0, =AMBOOT_BLD_RAM_START
	mov	pc, r0

/**
 * Perform BLD loading on the NAND flash
 *
 * r0 - r2 - Garbled by the page loader function, so they are used as tmp var.
 * r3 - DRAM address
 * r4 - Start block #
 * r5 - page variable
 * r6 - main or spare
 * r7 - End block #
 *
 * r8 - Length of BLD
 * r9 - Length of code loaded
 * r10 - current main_size
 * r11 - current spare_size
 */
do_nand_bld:
	/* Load SYS_CONFIG_REG */
	ldr	r3, =SYS_CONFIG_REG
	ldr	r4, [r3]
	ands	r4, r4, #SYS_CONFIG_NAND_FLASH_PAGE
	mov	r5, #512
	movne	r5, r5, LSL #2	/* MAIN_SIZE is 2K */
	mov	r10, r5
	mov	r5, #16
	movne	r5, r5, LSL #2	/* SPARE_SIZE is 64 */
	mov	r11, r5
	movne	r5, #64
	strne	r5, __PAGES_PER_BLOCK

	/* Setup run-time nand_control */
	ldr	r5, RT_NAND_CONTROL
	orr	r5, r5, #NAND_CTR_P3	/* Always use P3 to support all NAND */
	ldrne	r3, =NAND_2K_CONFIG
	orrne	r5, r5, r3		/* Set NAND_CTR_C2 and NAND_CTR_RC */
	str	r5, RT_NAND_CONTROL

	/* Setup run-time bld_sblk and bld_nblk */
	ldrne	r3, =NAND_2K_BLD_SBLK
	strne	r3, RT_NAND_BLD_SBLK
	ldrne	r3, =NAND_2K_BLD_NBLK
	strne	r3, RT_NAND_BLD_NBLK

	/* Load PTB first */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #NAND_PTB_SBLK
	mov	r5, #0
find_good_ptb:
	bl	nand_check_block	/* Find good PTB block */
	cmp	r0, #0
	beq	found_ptb
	add	r4, r4, #1
	bne	find_good_ptb
found_ptb:
	mov	r6, #0			/* Main area */
	bl	nand_load_page		/* Load PTB block */

	ldr	r0, =FLPART_MAGIC
check_bst_magic:
	/* Verify PTB's BST magic signature */
	ldr	r8, [r3, #PTB_BST_MAGIC_OFFSET]	@ WARNING: make sure offset is right!
	cmp	r0, r8
	bne	check_bst_magic		/* If test fails, loop forever... */

check_bld_magic:
	/* Verify PTB's BLD magic signature */
	ldr	r8, [r3, #PTB_BLD_MAGIC_OFFSET]	@ WARNING: make sure offset is right!
	cmp	r0, r8
	bne	check_bld_magic		/* If test fails, loop forever... */

	/* Get the BLD length from page just loaded */
	ldr	r8, [r3, #PTB_BLD_LEN_OFFSET]	@ WARNING: make sure offset is right!
	mov	r9, #0

	/* Get PTB meta to find SBLK & NBLK of BLD */
	mov	r6, #0				/* Main area */
	cmp	r10, #512
	moveq	r5, #8				/* META start page = PTB length / page size */
	movne	r5, #2
	bl	nand_load_page			/* Load PTB META */
	ldr	r4, [r3, #META_BLD_SBLK_OFFSET]	@ WARNING: make sure offset is right!
	ldr	r7, [r3, #META_BLD_NBLK_OFFSET]	@ WARNING: make sure offset is right!

	/* Load the BLD code into memory */
	ldr	r3, =AMBOOT_BLD_RAM_START
	add	r7, r4, r7
nand_load_block_loop:
	cmp	r4, r7
	beq	nand_jump_to_bld	/* No more blocks to load */
	mov	r5, #0			/* Set page number to 0 */
	bl	nand_check_block
	cmp	r0, #0
	add	r4, r4, #1
	bne	nand_load_block_loop	/* Skip block */
	sub	r4, r4, #1

nand_load_page_loop:
	mov	r6, #0			/* Main area */
	bl	nand_load_page		/* Load one page */
	add	r9, r9, r10		/* increment count */
	cmp	r9, r8
	bhi	nand_jump_to_bld	/* Loaded more indicated length */
	add	r3, r3, r10		/* increment target address */
	add	r5, r5, #1		/* increment page */
	ldr	r0, __PAGES_PER_BLOCK
	cmp	r5, r0
	bne	nand_load_page_loop
	add	r4, r4, #1		/* increment block */
	b	nand_load_block_loop


/**
 * Check if a block is bad. The result is returned in r0
 * (Garbles r0 - r2, r12 is used for lr)
 *
 * r3 - DRAM address (for temp storage of spare-area data)
 * r4 - Block
 * r0 - 0 means it is a good block, otherwise it is a bad block
 */
nand_check_block:
	mov	r12, lr
	mov	r6, #1
nand_check_block_loop:
	bl	nand_load_page
	ldrb	r0, [r3, #NAND_BB_MARKER_OFFSET]
	subs	r0, r0, #0xff		/* r1 = r1 - 0xff */
	movne	r0, #1			/* If bad block, r0 = 1 */
	movne	pc, r12			/* If bad block, return */
	add	r5, r5, #1		/* increment page */
	cmp	r5, #BAD_BLOCK_PAGES 	/* Check bad block pages */
	bne	nand_check_block_loop
	mov	r5, #0			/* All pages be checked, set r5 to 0 */
	mov	pc, r12			/* return  */

/**
 * Load a page from flash to memory
 * (Garbles r0 - r2).
 *
 * r3 - DRAM address
 * r4 - Block
 * r5 - Page
 * r6 - Main or Spare area
 */
nand_load_page:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	orr	r1, r1, #(FIO_CTR_XD | FIO_CTR_RS)
	cmp	r6, #0
	bne	_skip_main
	orr	r1, r1, #(FIO_CTR_CO)
_skip_main:
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup Flash Control Register */
	ldr	r1, RT_NAND_CONTROL
	orr	r1, r1, #NAND_CTR_SE
	orr	r1, r1, #NAND_CTR_EC_MAIN
	cmp	r6, #0
	beq	_skip_spare
	orr	r1, r1, #NAND_CTR_SA		/* Enable spare bit */
	bic	r1, r1, #NAND_CTR_EC_MAIN	/* Mask out main access */
_skip_spare:
	str	r1, [r0, #FLASH_CTR_OFFSET]

	/* Setup external DMA engine transaction's descriptor */
	ldr	r0, __FIODMA_DESC_ADDR
	mov	r1, #FIO_FIFO_BASE
	str	r1, [r0, #0x00]		@ src address
	str	r3, [r0, #0x04]		@ dst address
	mov	r1, #0x0
	str	r1, [r0, #0x08]		@ next pointer (== NULL)
	ldr	r1, __FIODMA_REPT_ADDR
	mov	r2, #0x0
	str	r2, [r1]
	str	r1, [r0, #0x0c]		@ report address
	mov	r1, #0
	cmp	r6, #0
	beq	setup_dma_main_size
	orr	r1, r1, r11		/* Spare area size */
	b	setup_dma_reg
setup_dma_main_size:
	orr	r1, r1, r10		/* Main area size */
setup_dma_reg:
	str	r1, [r0, #0x10]		@ xfrcnt

	mov	r1, #DMA_DESC_WM
	orr	r1, r1, #DMA_DESC_EOC
	orr	r1, r1, #DMA_DESC_NI
	orr	r1, r1, #DMA_DESC_IE
	orr	r1, r1, #DMA_DESC_ST
	cmp	r6, #0x0
	bne	set_spare_dma_burst
#if	defined(__FPGA__)
	orr	r1, r1, #DMA_DESC_BLK_32B
	orr	r1, r1, #DMA_DESC_TS_4B
#else
	orr	r1, r1, #DMA_DESC_BLK_512B
	orr	r1, r1, #DMA_DESC_TS_4B
#endif
	b	done_set_dma_burst
set_spare_dma_burst:
#if	defined(__FPGA__)
	orr	r1, r1, #DMA_DESC_BLK_8B
	orr	r1, r1, #DMA_DESC_TS_4B
#else
	orr	r1, r1, #DMA_DESC_BLK_8B
	orr	r1, r1, #DMA_DESC_TS_4B
#endif
done_set_dma_burst:
	str	r1, [r0, #0x14]		@ ctrl

	/* Start DMA descripotor-mode transfer */
	ldr	r1, __FIODMA_DESC_ADDR
#if (DMA_SUPPORT_DMA_FIOS == 1)
	mov	r0, #AHB_BASE
	orr	r0, r0, #DMA_FIOS_OFFSET

	str	r1, [r0, #DMA_CHAN0_NDA_OFFSET]
	mov	r1, #0x0
	orr	r1, r1, #DMA_CHANX_CTR_D
	orr	r1, r1, #DMA_CHANX_CTR_EN
	str	r1, [r0, #DMA_CHAN0_CTR_OFFSET]
#else
	ldr	r0, =DMA_CHAN_DA_REG(FIO_DMA_CHAN)
	str	r1, [r0]
	mov	r1, #0x0
	orr	r1, r1, #DMA_CHANX_CTR_D
	orr	r1, r1, #DMA_CHANX_CTR_EN
	ldr	r0, =DMA_CHAN_CTR_REG(FIO_DMA_CHAN)
	str	r1, [r0]
#endif

	/* Caluculate target flash memory address */
	/* Memory target address */
	/* compute: addr = ((block * ppb) + page) * ps */
	ldr	r0, __PAGES_PER_BLOCK
	mul	r1, r4, r0
	add	r1, r5, r1
	mov	r0, r10
	mul	r2, r1, r0

	/* Write the address of target memory */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	str	r2, [r0, #FIO_DMAADR_OFFSET]

	/* Setup Flash IO DMA Ccontrol Register */
	mov	r1, #FIO_DMACTR_EN
	orr	r1, r1, #FIO_DMACTR_FL
	cmp	r6, #0
	bne	set_spare_fiodma_burst
#if	defined(__FPGA__)
	orr	r1, r1, #FIO_DMACTR_BLK_32B
	orr	r1, r1, #FIO_DMACTR_TS4B
#else
	orr	r1, r1, #FIO_DMACTR_BLK_512B
	orr	r1, r1, #FIO_DMACTR_TS4B
#endif
	b done_set_fiodma_burst
set_spare_fiodma_burst:
#if	defined(__FPGA__)
	orr	r1, r1, #FIO_DMACTR_BLK_8B
	orr	r1, r1, #FIO_DMACTR_TS4B
#else
	orr	r1, r1, #FIO_DMACTR_BLK_8B
	orr	r1, r1, #FIO_DMACTR_TS4B
#endif
done_set_fiodma_burst:
	cmp	r6, #0
	beq	setup_fiodma_main_size
	orr	r1, r1, r11		/* Spare area size */
	b	setup_fiodma_reg
setup_fiodma_main_size:
	orr	r1, r1, r10		/* Main area size */
setup_fiodma_reg:
	str	r1, [r0, #FIO_DMACTR_OFFSET]

	/* Wait for interrupt for operation done */
_wait_nand_cmd_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NAND_INT_DI
	cmp	r1, #NAND_INT_DI
	bne	_wait_nand_cmd_done

	/* Wait for interrupt from FIO DMA */
_wait_nand_fiodma_done:
	ldr	r1, [r0, #FIO_DMASTA_OFFSET]
	and	r1, r1, #FIO_DMASTA_DN
	cmp	r1, #FIO_DMASTA_DN
	bne	_wait_nand_fiodma_done

	/* Wait for interrupt from DMA */
_wait_nand_dma_done:
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	ldr	r1, [r0, #DMA_INT_OFFSET]
	and	r1, r1, #(0x1 << FIO_DMA_CHAN)
	cmp	r1, #0
	beq	_wait_nand_dma_done

	/* Poll for the descriptor report status */
	ldr	r0, __FIODMA_REPT_ADDR
	ldr	r1, [r0]
	and	r1, r1, #DMA_CHANX_STA_DN
	cmp	r1, #0
	beq	_wait_nand_dma_done

	/* Clear NAND interrupt register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	/* Clear FIODMA status register */
	str	r1, [r0, #FIO_DMASTA_OFFSET]

	/* Clear DMA status register */
	mov	r1, #0x0
#if (DMA_SUPPORT_DMA_FIOS == 1)
	mov	r0, #AHB_BASE
	orr	r0, r0, #DMA_FIOS_OFFSET
	str	r1, [r0, #DMA_INT_OFFSET]
	str	r1, [r0, #DMA_CHAN0_STA_OFFSET]
#else
	ldr	r0, =DMA_INT_REG
	str	r1, [r0]
	ldr	r0, =DMA_CHAN_STA_REG(FIO_DMA_CHAN)
	str	r1, [r0]
#endif

eot_nand:
	mov	pc, lr

__FIODMA_DESC_ADDR:	.word	__fiodma_desc_addr
__FIODMA_REPT_ADDR:	.word	__fiodma_rept_addr
__PAGES_PER_BLOCK:	.word	32
RT_NAND_CONTROL:	.word	NAND_CONTROL
RT_NAND_BLD_SBLK:	.word	NAND_512_BLD_SBLK
RT_NAND_BLD_NBLK:	.word	NAND_512_BLD_NBLK

#endif  /* !CONFIG_NAND_NONE */

