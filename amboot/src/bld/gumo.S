/**
 * system/src/bld/gumo.S
 *
 * History:
 *    2009/06/10 - [Jack Huang] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*======================================================================*/
/* //TS need to execute this sequence to 				*/
/* //escape deadlock after power on. 					*/
/*									*/
/*#define TS_ESC_TEST_ADDR	0xc0100000				*/
/*#define TS_ESC_SIGNATURE	0xabcd9876				*/
/*void ts_esc_dead_lock(void)						*/
/*{									*/
/*	int chan;							*/
/*	if (readl(TS_ESC_TEST_ADDR) == TS_ESC_SIGNATURE) {		*/
/*		return;							*/
/*	}								*/
/*									*/
/*	for (chan = 0; chan < TS_TX_MAX_CHANS; chan++) {		*/
/*		writel(TS_CHX_RX_TEST_CTR_REG(chan),			*/
/*				TS_RX_TEST_CTR_LOOPBACK_EN);		*/
/*		writel(TS_CHX_TX_PKT_INFO_REG(chan),			*/
/*				TS_TX_PKT_INFO_LEN(188 >> 2) |		*/
/*				TS_TX_PKT_INFO_VALID_LEN(188 >> 2));	*/
/*		writel(TS_CHX_TX_DMA_BASE_REG(chan),  0xc0100000);	*/
/*		writel(TS_CHX_TX_DMA_LIMIT_REG(chan), 0xc0110000);	*/
/*		writel(TS_CHX_TX_DMA_STOP_REG(chan), 0x0);		*/
/*		writel(TS_CHX_TX_CTR_REG(chan),				*/
/*				TS_TX_CTR_CLK_EN | TS_TX_CTR_SRC_EXT |	*/
/*				TS_TX_CTR_DIV(5) | TS_TX_CTR_START);	*/
/*	}								*/
/*									*/
/*	writel(TS_ESC_TEST_ADDR, TS_ESC_SIGNATURE);			*/
/*									*/
/*	//Chip soft reset, no DLL reset.				*/
/*	writel(SOFT_RESET_REG, 0x2);					*/
/*	writel(SOFT_RESET_REG, 0x3);					*/
/*}									*/
/*======================================================================*/

#if defined(ENABLE_TS) && (TS_ESC_DEAD_LOCK)

#define TS_ESC_TEST_ADDR	0xc0100000
#define TS_ESC_SIGNATURE	0xabcd9876

.text
	b	_ts_esc_dead_lock

ts_esc_test_addr:	.word	TS_ESC_TEST_ADDR
ts_esc_signature:	.word	TS_ESC_SIGNATURE
ts_esc_value_1:		.word	TS_RX_TEST_CTR_LOOPBACK_EN
ts_esc_value_2:		.word	(TS_TX_PKT_INFO_LEN(188 >> 2) | \
				TS_TX_PKT_INFO_VALID_LEN(188 >> 2))
ts_esc_value_3:		.word	0xc0100000
ts_esc_value_4:		.word	0xc0110000

ts_esc_value_5:		.word	0x0
ts_esc_value_6:		.word	(TS_TX_CTR_CLK_EN| TS_TX_CTR_SRC_EXT | \
				 TS_TX_CTR_DIV(5) | TS_TX_CTR_START)

_ts_esc_dead_lock:
 	ldr	r2, ts_esc_test_addr
	ldr	r2, [r2]
	ldr	r3, ts_esc_signature

 	cmp	r2, r3
	ble	_ts_done

	mov	r0, #AHB_BASE
	add	r0, r0, #TS_OFFSET

	mov	r3, #TS_TX_MAX_CHANS
	mov	r4, #0x0

_ts_loop:
	add	r1, r0, #TS_CH0_RX_TEST_CTR_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_1
	str	r2, [r1]

	add	r1, r0, #TS_CH0_TX_PKT_INFO_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_2
	str	r2, [r1]

	add	r1, r0, #TS_CH0_TX_DMA_BASE_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_3
	str	r2, [r1]

	add	r1, r0, #TS_CH0_TX_DMA_LIMIT_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_4
	str	r2, [r1]

	add	r1, r0, #TS_CH0_TX_DMA_STOP_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_5
	str	r2, [r1]

	add	r1, r0, #TS_CH0_TX_CTR_OFFSET
	add	r1, r1, r4
	ldr	r2, ts_esc_value_6
	str	r2, [r1]

	add	r4, r4, #0x80
	subs	r3, r3, #0x1
	bne	_ts_loop

	mov	r4, #0
	mov	r3, #0

	ldr	r2, ts_esc_test_addr @set TS boot flag
	ldr	r0, ts_esc_signature
 	str	r0, [r2]

 	mov 	r0, #APB_BASE @software reset
	add	r0, r0, #RCT_OFFSET
	add	r0, r0, #SOFT_RESET_OFFSET
 	mov	r2, #2
 	mov	r1, #3
 	str	r2, [r0]
 	str	r1, [r0]

_ts_done:
#endif

#if (NAND_DUMMY_XFER == 1)
/* Notice: This will overwrite pri and make self-refresh function fail */

nand_dummy_xfer:
	/* Enable fio dma. */
	ldr	r0, =I2S_24BITMUX_MODE_REG
	mov	r1, #0x0
	str	r1, [r0]

	/* Setup nand */
	ldr	r0, =FIO_BASE
	ldr	r1, =0x15
	str	r1, [r0, #FIO_CTR_OFFSET]
	ldr	r1, =0x4080110
	str	r1, [r0, #FLASH_CTR_OFFSET]

	/* Setup dma */
	ldr	r0, =DMA_FIOS_BASE
	mov	r1, #0x0
	str	r1, [r0, #DMA_CHAN0_STA_OFFSET]
	ldr	r1, =0x60000000
	str	r1, [r0, #DMA_CHAN0_SRC_OFFSET]
	ldr	r1, =(DRAM_START_ADDR + 0x100000)
	str	r1, [r0, #DMA_CHAN0_DST_OFFSET]
	ldr	r1, =0xae800020
	str	r1, [r0, #DMA_CHAN0_CTR_OFFSET]

	/* Setup fiodma */
	ldr	r0, =FIO_BASE
	mov	r1, #0
	str	r1, [r0, #FIO_DMAADR_OFFSET]
	ldr	r1, =0x86800020
	str	r1, [r0, #FIO_DMACTR_OFFSET]

	mov	r2, #0

	/* Wait for DMA done */
	ldr	r0, =DMA_FIOS_BASE
dma_done:
	ldr	r1, [r0, #DMA_INT_OFFSET]
	tst	r1, #0x1
	beq	dma_done
	str	r2, [r0, #DMA_INT_OFFSET]

	/* Wait for FIOCMD done */
	ldr	r0, =FIO_BASE
fiocmd_done:
	ldr	r1, [r0, #FLASH_INT_OFFSET]
	tst	r1, #0x1
	beq	fiocmd_done
	str	r2, [r0, #FLASH_INT_OFFSET]

	/* Wait for FIODMA done */
fiodma_done:
	ldr	r1, [r0, #FIO_DMASTA_OFFSET]
	tst	r1, #0x1000000
	beq	fiodma_done
	str	r2, [r0, #FIO_DMASTA_OFFSET]
#endif /* NAND_DUMMY_XFER */
