/**
 * system/src/bst/bstnor.S
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

.text

#if !defined(CONFIG_NOR_NONE)

/**
 * Initialize the FIO and load the AMBOOT_BLD code into the memory, and then
 * jump to it.
 */
.globl nor_load_bld
nor_load_bld:
	/* Setup timing parameters */
#if 0
	/* FIXME: Temporarily disabled due to problems on McGrawl's A1BUB */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, =NOR_TIM0_VAL
	str	r1, [r0, #FLASH_TIM0_OFFSET]
	ldr	r1, =NOR_TIM1_VAL
	str	r1, [r0, #FLASH_TIM1_OFFSET]
	ldr	r1, =NOR_TIM2_VAL
	str	r1, [r0, #FLASH_TIM2_OFFSET]
	ldr	r1, =NOR_TIM3_VAL
	str	r1, [r0, #FLASH_TIM3_OFFSET]
	ldr	r1, =NOR_TIM4_VAL
	str	r1, [r0, #FLASH_TIM4_OFFSET]
	ldr	r1, =NOR_TIM5_VAL
	str	r1, [r0, #FLASH_TIM5_OFFSET]
#endif

	/* Start loading BLD */
	b	do_nor_bld

	/* Jump to_BLD now that it has been loaded */
nor_jump_to_bld:
	ldr	r0, =AMBOOT_BLD_RAM_START
	mov	pc, r0

/**
 * Perform BLD loading on the NOR flash
 *
 * r0 - r2 - Garbled by the page loader function, so they are used as tmp var.
 * r3 - DRAM address
 * r4 - Start sector #
 * r5 - Length (variable)
 * r6 - End sector #
 *
 * r7 - Length of BLD code need to load
 */
do_nor_bld:
	/* Load PTB first */
	ldr	r3, =AMBOOT_BLD_RAM_START
	mov	r4, #NOR_PTB_SSEC
	mov	r5, #NOR_SECTOR_SIZE
	bl	nor_load_sector

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
	mov	r4, #NOR_BLD_SSEC
	mov	r6, #NOR_BLD_NSEC
	add	r6, r4, r6
nor_load_sector_loop:
	cmp	r4, r6
	beq	nor_jump_to_bld		/* No more blocks to load */
	mov	r5, r7
	cmp	r5, #NOR_SECTOR_SIZE
	movhi	r5, #NOR_SECTOR_SIZE
	/* Round the length argument to multiple of 8 (FIOS/DMA restriction) */
	and	r0, r5, #0x7
	cmp	r0, #0
	beq	nor_len_fixed
	add	r5, r5, #8
	sub	r5, r5, r0
nor_len_fixed:
	bl	nor_load_sector		/* Load one sector */
	add	r3, r3, r5
	add	r4, r4, #0x1
	sub	r7, r7, r5
	b	nor_load_sector_loop

/**
 * Load a sector from flash to memory
 * (Garbles r0 - r1).
 *
 * r3 - DRAM address
 * r4 - Sector
 * r5 - Length
 */
nor_load_sector:
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

	/* Setup FIO Control Register */
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #0x0
	orr	r1, r1, #FIO_CTR_XD
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Setup Flash Control Register */
	ldr	r1, =NOR_CONTROL
	str	r1, [r0, #FLASH_CTR_OFFSET]

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
	mov	r0, #NOR_SECTOR_SIZE
	mul	r2, r4, r0

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
_wait_nor_cmd_done:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	and	r1, r1, #NOR_INT_DI
	cmp	r1, #NOR_INT_DI
	bne	_wait_nor_cmd_done

	/* Wait for interrupt from FIO DMA */
_wait_nor_fiodma_done:
	ldr	r1, [r0, #FIO_DMASTA_OFFSET]
	and	r1, r1, #FIO_DMASTA_DN
	cmp	r1, #FIO_DMASTA_DN
	bne	_wait_nor_fiodma_done

	/* Wait for interrupt from DMA */
_wait_nor_dma_done:
	mov	r0, #AHB_BASE
#if (DMA_SUPPORT_DMA_FIOS == 1)
	orr	r0, r0, #DMA_FIOS_OFFSET
#else
	orr	r0, r0, #DMA_OFFSET
#endif
	ldr	r1, [r0, #DMA_INT_OFFSET]
	and	r1, r1, #(0x1 << FIO_DMA_CHAN)
	cmp	r1, #0
	beq	_wait_nor_dma_done

	/* Clear NOR interrupt register */
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

	/*******************************
	 * Now poll the status register.
	 *******************************/
poll_nor_status:
	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET
	mov	r1, #NOR_CMD_READSTATUS
	str	r1, [r0, #FLASH_CMD_OFFSET]
_wait_nor_readstatus_done:
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	cmp	r1, #0x0
	beq	_wait_nor_readstatus_done
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]
	ldr	r1, [r0, #FLASH_STA_OFFSET]
	and	r1, r1, #0x80
	cmp	r1, #0x80
	bne	poll_nor_status

eot_nor:
	mov	pc, lr

#endif  /* !CONFIG_NOR_NONE */
