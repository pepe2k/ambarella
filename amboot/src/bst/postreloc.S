/**
 * system/src/bst/postreloc.S
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
#include "partinfo.h"

.text

/*
 * NOTE: The entire code contained in this file resides in the boot code
 *	 but is relocated and executed from DRAM.
 */
.globl	start_post_relocate
start_post_relocate:

#if (HOST_MAX_AHB_CLK_EN_BITS == 10)
	/* Disable boot-select */
	mov	r0, #AHB_BASE
	orr	r0, r0, #HOST_OFFSET
	ldr	r1, [r0, #HOST_AHB_CLK_ENABLE_OFFSET]
	bic	r1, r1, #HOST_AHB_BOOT_SEL
	bic	r1, r1, #HOST_AHB_FDMA_BURST_DIS
	str	r1, [r0, #HOST_AHB_CLK_ENABLE_OFFSET]
#endif

#if (I2S_24BITMUX_MODE_REG_BITS == 4)
	/* Disable boot-select */
	ldr	r0, =I2S_24BITMUX_MODE_REG
	mov	r1, #0x0
	str	r1, [r0]
#endif

	mov	r0, #AHB_BASE
	orr	r0, r0, #FIO_OFFSET

	/* Exit random read mode by writing to the FIO Control Register */
	mov	r1, #0x0
	orr	r1, r1, #FIO_CTR_XD
	str	r1, [r0, #FIO_CTR_OFFSET]

	/* Clear the FIO DMA Status Register */
	mov	r1, #0x0
	str	r1, [r0, $FIO_DMASTA_OFFSET]

	/* Clear the FLASH Interrupt Register */
	mov	r1, #0x0
	str	r1, [r0, #FLASH_INT_OFFSET]

	/* Read the SYS_CONFIG register to see if we boot from NOR or NAND */
	mov	r0, #APB_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, [r0, #SYS_CONFIG_OFFSET]
	and	r1, r1, #SYS_CONFIG_BOOTMEDIA
	cmp	r1, #SYS_CONFIG_BOOTMEDIA

#if defined(ENABLE_FLASH)
#if !defined(CONFIG_NOR_NONE)
	beq	nor_load_bld
#endif
#if !defined(CONFIG_SNOR_NONE)
	/* FIXME: check sys_config */
	b	snor_load_bld
#endif
#if !defined(CONFIG_NAND_NONE)
	bne	nand_load_bld
#endif
#if !defined(CONFIG_ONENAND_NONE)
	/* FIXME: check sys_config */
	b	onenand_load_bld
#endif
#endif
	b	start_post_relocate
