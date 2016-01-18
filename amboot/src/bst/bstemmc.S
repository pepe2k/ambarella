/**
 * system/src/bst/bstemmc.S
 *
 * History:
 *    2010/04/30 - [Evan Chen] created file
 *
 * Copyright (C) 2010-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

@#define ENABLE_EMMC_BOOT
#ifdef ENABLE_EMMC_BOOT

#include <ambhw.h>
#include <fio/sdboot.h>
#include <fio/firmfl.h>
#include "partinfo.h"

@#define BSTEMMC_PIO_MODE

/*
 * r0  - register base address
 * r1  - temporary variable
 * r2  - temporary variable
 * r3  - card->hcs
 * r4  - card->rca
 * r5  - card->ocr
 * r6  - command argument
 * r7  - command
 * r8  - start address
 * r9  - blocks
 * r10 - target address
 * r11 - readl return address
 * r12 - boot mode[bit 11: SPI boot, bit 10: SD boot] and
	 card type[bit 7~0 : 0 SDHC, 9 SD]
	 target address
*/

.text

#if defined(ENABLE_SD)

#define NORMAL_SD_CARD		0x00000009
#define SDMMC_PTB_SBLK		0x4

sdmmc_init_done:
	b 	sdmmc_init_done

.globl sdmmc_readl
sdmmc_readl:
	ldr	r3, =(t_sdmmc_readl + 1)
	mov	lr, pc
	bx	r3
	b	readl_done

readl_done:
	mov	pc, r11

/* -------------------------------------------------------------------------- */

.globl emmc_load_bld
emmc_load_bld:
	mov	r1, #0x4			@ Set r12 to BOOT_USING_SD_CARD
	mov	r1, r1, lsl #0x8
	mov	r12, r1
#if 0
	@ Configure GPIO (SMIO 2 ~ 6)
	ldr	r0, gpio2_offset

	mov	r1, #0x0000001f
	str	r1, [r0, #0x18]	@ set hardware mode
	str	r1, [r0, #0x2c]	@ enable
#endif
	@ Program flash io controller
	ldr	r0, fio_offset
	ldr	r1, [r0, #0x0]
	ldr	r2, =FIO_CTR_XD
#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
	orr	r1, r2, r1
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)
	bic	r1, r1, r2
#endif
	str	r1, [r0, #0x0]

	@ Program base address of flash io controller
	ldr	r0, fio_dma_ctl_reg
	mov	r2, #0x30			@ r2 = 0x30000000
	mov	r2, r2, lsl #0x18
	ldr	r1, [r0, #0x0]
	orr	r1, r2, r1			@ enable SD controller
	str	r1, [r0, #0x0]

	@ Program base address of sd
	ldr	r0, sd_offset			@ load sd base address register

	@ Reset all
	mov	r1, #0x1
	strb	r1, [r0, #SD_RESET_OFFSET]
_wait_reset_done:
	ldrb	r1, [r0, #SD_RESET_OFFSET]
	cmp	r1, #0
	bne	_wait_reset_done

	@ Set SD Power (3.3V)
	ldr	r2, sd_pwr_reg
	mov	r1, #(SD_PWR_3_3V | SD_PWR_ON)
	strb	r1, [r2, #0x0]

	@ Set data timeout
	ldr	r2, sd_tmo_reg
	mov	r1, #0xe
	strb	r1, [r2, #0x0]

	@ Setup clock
	mov	r1, #0x80			@ XXMhz input clock
	mov	r1, r1, lsl #0x8
	mov	r2, #(SD_CLK_ICLK_EN | SD_CLK_EN)
	orr	r1, r1, r2
	strh	r1, [r0, #SD_CLK_OFFSET]

	@ Wait until clock becomes stable
	mov	r2, #SD_CLK_ICLK_STABLE
_wait_clock_stable:
	ldrh	r1, [r0, #SD_CLK_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_clock_stable

	@ Enable interrupts on events we want to know
#ifdef BSTEMMC_PIO_MODE
	mov	r1, #0x1
	mov	r1, r1, lsl #0x8
	mov	r2, #0xf7
	orr	r1, r1, r2			@ PIO
#else
	mov	r1, #0xcf
#endif
	strh	r1, [r0, #SD_NISEN_OFFSET]
#ifndef BSTEMMC_PIO_MODE
	ldr	r1, =0x1cf
#endif
	strh	r1, [r0, #SD_NIXEN_OFFSET]

	@ Enable error interrupt status
#ifdef BSTEMMC_PIO_MODE
	mov	r2, #0xff
	orr	r1, r1, r2
#else
	ldr	r1, =0x1ff
#endif
	strh	r1, [r0, #SD_EISEN_OFFSET]

	@ Enable error interrupt signal
	strh	r1, [r0, #SD_EIXEN_OFFSET]

	@ make sure card present
	mov	r2, #0x4			@ SD_STA_CDP_L
	mov	r2, r2, lsl #0x10
	ldr	r1, [r0, #SD_STA_OFFSET]
	and	r1, r1, r2
	beq	sdboot_failed_0

	@ Configure SD/MMC card

	@ Clear registers
	mov	r3, #0x0			@ card->hcs << HCS_SHIFT_BIT
	mov	r3, r3, lsl #0x1e
	mov	r4, #0x0
	mov	r5, #0xff			@ 0x00ff0000
	mov	r5, r5, lsl #0x10

	@
	@ CMD0
	@

	mov	r6, #0x0
	mov	r7, #0x0000			@ send CMD0 - GO_IDLE_STATE
	bl	sdmmc_command

_setup_mmc_card:
	@
	@ CMD1 - Send CMD1 with working voltage until the memory becomes ready
	@

	@mov	r6, r5				@ card->ocr
	ldr	r6, =0x40ff0000
	ldr	r7, cmd_1
	bl	sdmmc_command

	cmp	r1, #0x0			@ success
	bne	_setup_mmc_card

	mov	r2, #0x1			@ r2 = 0x80000000
	mov	r2, r2, lsl #0x1f
	@ ocr = UNSTUFF_BITS(cmd.resp, 8, 32);
	ldr	r5, [r0, #SD_RSP0_OFFSET]
	mov	r1, r5
	and	r1, r1, r2				@ SDMMC_CARD_BUSY
	cmp	r1, r2
	bne	_setup_mmc_card

	@
	@ CMD2 - All Send CID
	@

	mov	r6, #0x0
	ldr	r7, cmd_2
	bl	sdmmc_command

	@
	@ CMD3 - Ask RCA
	@
	mov	r4, #0x1			@ card->rca = 1
	mov	r4, r4, lsl #0x10		@ r6 = (card->rca << 16)
	mov	r6, r4
	ldr	r7, cmd_3
	bl	sdmmc_command

	@
	@ CMD7 - Select card
	@

	mov	r6, r4				@ (card->rca << 16)
	ldr	r7, cmd_7
	bl	sdmmc_command

	@ Set bus width to 1 bit
	ldr	r2, sd_host_reg
	mov	r3, #0x0			@ host
	strb	r3, [r2, #0x0]

	mov	r1, r12
	mov	r2, #NORMAL_SD_CARD		@ set current card is SD card
	orr	r1, r1, r2
	mov	r12, r1

	b	_setup_card_done

_setup_card_done:

	@
	@ CMD16: Set block length
	@

	mov	r6, #0x1			@ r6 = 512
	mov	r6, r6, lsl #0x9
	ldr	r7, cmd_16
	bl	sdmmc_command

	@ Disable clock
	ldrh	r1, [r0, #SD_CLK_OFFSET]
	mov	r2, #(SD_CLK_ICLK_EN | SD_CLK_EN)
	bic	r1, r1, r2
	strh	r1, [r0, #SD_CLK_OFFSET]

	@ Setup clock
	@mov	r1, #0x2			@ 12 MHz
	mov	r1, #0x1			@ 24 MHz
	mov	r1, r1, lsl #0x8
	orr	r1, r1, r2
	strh	r1, [r0, #SD_CLK_OFFSET]

	@ Wait until clock becomes stable
	mov	r2, #SD_CLK_ICLK_STABLE
_wait_sd_clock_stable:
	ldrh	r1, [r0, #SD_CLK_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_sd_clock_stable

	@ Set block size
	mov	r3, #0x72			@ block size : 512 (r3 = 7200)
	mov	r3, r3, lsl #0x8
	strh	r3, [r0, #SD_BLK_SZ_OFFSET]

	bl	t_sdmmc_boot

sdboot_failed:
	b	sdboot_failed

/*
 * Boot from sd/mmc
 *
 * r8  - start address
 * r9  - blocks
 * r12 - target address
 */
t_sdmmc_boot:
	ldr	r0, sd_offset			@ load sd base address register
#ifndef BSTEMMC_PIO_MODE
	ldr	r1, =SM_PTB_SSEC
	ldr	r5, =SM_PTB_NSEC
	ldr	r6, =AMBOOT_BLD_RAM_START
	bl	t_sdmmc_read_sector

sdmmc_check_bst_magic:
	/* Verify PTB's BST magic signature */
	ldr	r8, [r6, #PTB_BST_MAGIC_OFFSET]	@ WARNING: make sure offset is right!
	ldr	r1, =FLPART_MAGIC
	cmp	r8, r1
	bne	sdmmc_check_bst_magic		@ If test fails, loop forever...

	ldr	r1, =SM_BLD_SSEC
	ldr	r5, =SM_BLD_NSEC
	ldr	r6, =AMBOOT_BLD_RAM_START
	bl	t_sdmmc_read_sector

	ldr	r0, =AMBOOT_BLD_RAM_START
	bx	r0

@-------------------------------------------------------------------------------
#else /* BSTEMMC_PIO_MODE */

	mov	r2, #SD_STA_CMD_INHIBIT_DAT
_wait_dat_line_ready_m:
	ldr	r1, [r0, #SD_STA_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	bne	_wait_dat_line_ready_m

	ldr	r10, =AMBOOT_BLD_RAM_START	@ target address

	ldr	r1, =SM_BLD_NSEC
	strh	r1, [r0, #SD_BLK_CNT_OFFSET]
	mov	r9, r1				@ save block count to r9

	ldr	r1, =SM_BLD_SSEC
	str	r1, [r0, #SD_ARG_OFFSET]

	mov	r1, #0x36
	strh	r1, [r0, #SD_XFR_OFFSET]

	ldr	r1, cmd_18
	strh	r1, [r0, #SD_CMD_OFFSET]	@ command

	@ wait for command complete
	mov	r2, #SD_NIS_CMD_DONE
_wait_for_command_complete_m:
	ldrh	r1, [r0, #SD_NIS_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_for_command_complete_m

	@ wait read buffer ready
	mov	r4, #0x0			@ block count (r4)
	mov	r5, r10				@ target address (r5)

_read_block_loop_m:
	@ wait read buffer ready
	mov	r2, #SD_NIS_READ_READY		@ buffer read ready bit
_wait_buffer_ready_m:
	ldrh	r1, [r0, #SD_NIS_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_buffer_ready_m

	@ save error status to r1 and clear interrupts
	strh	r1, [r0, #SD_NIS_OFFSET]

	@ To see if errors happen
	ldrh	r1, [r0, #SD_EIS_OFFSET]
	strh	r1, [r0, #SD_EIS_OFFSET]

	cmp	r1, #0x0			@ if (error status != 0x0)
	bne	sdboot_failed

	@ Copy data from data port to target address (r10)
	mov	r2, #0				@ loop count(i) 0 ~ 512
	mov	r6, #0x1			@ r6 = 512
	mov	r6, r6, lsl #0x9
_read_loop_m:
	ldr	r3, [r0, #SD_DATA_OFFSET]	@ read data to r3
	str	r3, [r5, r2]			@ save data to DRAM
	add	r2, r2, #0x4
	cmp	r2, r6
	blt	_read_loop_m

	add	r4, r4, #0x1			@ update block count
	add	r5, r5, r6			@ update target address
	cmp	r4, r9
	blt	_read_block_loop_m

	@ Wait for transfer complete
	mov	r2, #SD_NIS_XFR_DONE
_wait_transfer_complete_m:
	ldrh	r1, [r0, #SD_NIS_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_transfer_complete_m

	@ save error status to r1 and clear interrupts
	strh	r1, [r0, #SD_NIS_OFFSET]

	@ To see if errors happen
	ldrh	r1, [r0, #SD_EIS_OFFSET]
	strh	r1, [r0, #SD_EIS_OFFSET]

	cmp	r1, #0x0			@ if (error status != 0x0)
	bne	sdboot_failed

	mov	r12, r10
#endif	/* BSTEMMC_PIO_MODE */
	ldr	r0, =AMBOOT_BLD_RAM_START
	bx	r0

/*
 * r6 - argument
 * r7 - command
 *
 * The result is returned in r1.
 */
sdmmc_command:

	str	r6, [r0, #SD_ARG_OFFSET]	@ argument

	strh	r7, [r0, #SD_CMD_OFFSET]	@ command

	@ Wait for command to complete
	mov	r2, #SD_NIS_CMD_DONE
_wait_command_complete:
	ldrh	r1, [r0, #SD_NIS_OFFSET]
	and	r1, r1, r2
	cmp	r1, #0
	beq	_wait_command_complete

	@ save error status to r1 and clear interrupts
	strh	r1, [r0, #SD_NIS_OFFSET]

	@ Check for errors
	ldrh	r1, [r0, #SD_EIS_OFFSET]
	strh	r1, [r0, #SD_EIS_OFFSET]

	mov	pc, lr

/*
 * Read a 4-byte word from SD/MMC.
 *
 * r1 - address to read
 * r5 - count
 * r3 - temp
 * r6, r7
 *
 * The result is returned in r1.
 */
t_sdmmc_readl:
	mov	r7, r0
	mov	r6, r1

	ldr	r0, sd_offset			@ load sd base address register

	mov	r3, #0x0
	strh	r3, [r0, #SD_BLK_CNT_OFFSET]

	mov	r3, #0x1			@ block (sector 1)
	mov	r1, r12
	mov	r5, #0xff
	and	r5, r5, r1
	mov	r3, r3, lsl r5
	str	r3, [r0, #SD_ARG_OFFSET]	@ argument

	mov	r3, #0x10			@ single block access (not used)
	strh	r3, [r0, #SD_XFR_OFFSET]

	ldr	r3, cmd_17
	strh	r3, [r0, #SD_CMD_OFFSET]	@ command

	@ Wait for command to complete
	mov	r5, #SD_NIS_CMD_DONE
_wait_command_complete_s:
	ldrh	r3, [r0, #SD_NIS_OFFSET]
	and	r3, r3, r5
	cmp	r3, #0
	beq	_wait_command_complete_s

	@ Wait read buffer ready
	mov	r5, #SD_NIS_READ_READY
_wait_buffer_ready_s:
	ldrh	r3, [r0, #SD_NIS_OFFSET]
	and	r3, r3, r5				@ buffer read ready bit
	cmp	r3, #0
	beq	_wait_buffer_ready_s

	@ Save error status to r1 and clear interrupts
	strh	r3, [r0, #SD_NIS_OFFSET]

	@ Check for errors
	ldrh	r3, [r0, #SD_EIS_OFFSET]
	strh	r3, [r0, #SD_EIS_OFFSET]

	cmp	r3, #0x0			@ if (error status != 0x0)
	bne	sdmmc_readl_failed

	@ Read data to r1
	mov	r5, #0x0			@ loop count(i) 0 ~ 512
	mov	r4, #0x1			@ r4 = 512
	mov	r4, r4, lsl #0x9
_read_loop_s:
	cmp	r5, r6				@ if (i == target address)
	ldr	r3, [r0, #SD_DATA_OFFSET]
	bne	_readl_jump
	mov	r1, r3				@ save data to r1
_readl_jump:
	add	r5, r5, #4
	cmp	r5, r4
	blt	_read_loop_s

	@ Wait for transfer complete
	mov	r5, #SD_NIS_XFR_DONE
_wait_transfer_complete_s:
	ldrh	r3, [r0, #SD_NIS_OFFSET]
	and	r3, r3, r5
	cmp	r3, #0
	beq	_wait_transfer_complete_s

	@ save error status to r1 and clear interrupts
	strh	r3, [r0, #SD_NIS_OFFSET]

	@ To see if errors happen
	ldrh	r3, [r0, #SD_EIS_OFFSET]
	strh	r3, [r0, #SD_EIS_OFFSET]

	cmp	r3, #0x0			@ if (error status != 0x0)
	bne	sdmmc_readl_failed

	mov	r0, r7
	bx	lr

sdmmc_readl_failed:
	b	sdmmc_readl_failed

/*
 * Reading a sector from SD/MMC in DMA mode.
 *
 * r1 - sector to read
 * r5 - count
 * r3 - temp
 * r6, r7
 *
 * The result is returned in r1.
 */
t_sdmmc_read_sector:

	ldr	r0, sd_offset			@ load sd base address register

	mov	r3, r6
	str	r3, [r0, #SD_DMA_ADDR_OFFSET]

	mov	r3, #0xe
	strb	r3, [r0, #SD_TMO_OFFSET]

	mov	r3, r5
	strh	r3, [r0, #SD_BLK_CNT_OFFSET]

	mov	r3, r1
	str	r3, [r0, #SD_ARG_OFFSET]	@ argument

	mov	r3, #0x37
	strh	r3, [r0, #SD_XFR_OFFSET]

	ldr	r3, cmd_18			@ CMD18 Read multiple sectors
	strh	r3, [r0, #SD_CMD_OFFSET]	@ command

	@ Wait for command to complete
	mov	r5, #SD_NIS_CMD_DONE
_wait_cmd_complete_s:
	ldrh	r3, [r0, #SD_NIS_OFFSET]
	and	r3, r3, r5
	cmp	r3, #0
	beq	_wait_cmd_complete_s

	@ Save error status to r1 and clear interrupts
	strh	r3, [r0, #SD_NIS_OFFSET]

	@ Check for errors
	ldrh	r3, [r0, #SD_EIS_OFFSET]
	strh	r3, [r0, #SD_EIS_OFFSET]

	cmp	r3, #0x0			@ if (error status != 0x0)
	bne	sdmmc_read_sector_failed

	@ Wait for transfer complete
	mov	r5, #SD_NIS_XFR_DONE
_wait_trans_complete_s:
	ldrh	r3, [r0, #SD_NIS_OFFSET]
	and	r3, r3, r5
	cmp	r3, #0
	beq	_wait_trans_complete_s

	@ save error status to r1 and clear interrupts
	strh	r3, [r0, #SD_NIS_OFFSET]

	@ To see if errors happen
	ldrh	r3, [r0, #SD_EIS_OFFSET]
	strh	r3, [r0, #SD_EIS_OFFSET]

	cmp	r3, #0x0			@ if (error status != 0x0)
	bne	sdmmc_read_sector_failed

	bx	lr

sdmmc_read_sector_failed:
	b	sdmmc_read_sector_failed


sdboot_failed_0:
	b	sdboot_failed_0

/* -------------------------------------------------------------------------- */

.align 2

magic:		.word	SDBOOT_HEADER_MAGIC
gpio2_offset:	.word	(APB_BASE | GPIO2_OFFSET)
fio_offset:	.word	(AHB_BASE | FIO_OFFSET)
sd_offset:	.word	(AHB_BASE | SD_OFFSET)

sys_config_reg:	.word	(APB_BASE | RCT_OFFSET | SYS_CONFIG_OFFSET)
fio_dma_ctl_reg:.word	(AHB_BASE | FIO_OFFSET | FIO_DMACTR_OFFSET)
sd_host_reg:	.word	(AHB_BASE | SD_OFFSET | SD_HOST_OFFSET)
sd_pwr_reg:	.word	(AHB_BASE | SD_OFFSET | SD_PWR_OFFSET)
sd_tmo_reg:	.word	(AHB_BASE | SD_OFFSET | SD_TMO_OFFSET)
sd_reset_reg:	.word	(AHB_BASE | SD_OFFSET | SD_RESET_OFFSET)


cmd_1:		.word	0x0102		@ send CMD1 - SEND_OP_COND
					@ expect 48 bit response (R3)
cmd_2:		.word	0x0209		@ send CMD2 - ALL_SEND_CID
					@ expect 136 bit response (R2)
cmd_3:		.word	0x031a		@ send CMD3 - SET_RELATIVE_ADDR
					@ expect 48 bit response (R6)
acmd_6:		.word	0x061a		@ send CMD6 - SET_BUS_WIDTH
					@ expect 48 bit response (R1)
cmd_7:		.word	0x071b		@ send CMD7 - SELECT_CARD
					@ expect 48 bit response (R1B)
cmd_8_arg:	.word	0x01aa		@ (SDMMC_HIGH_VOLTAGE << 8) |
					@ SDMMC_CHECK_PATTERN
cmd_8:		.word	0x081a		@ send CMD8 - SEND_EXT_CSD
					@ expect 48 bit response (R7)
cmd_13:		.word	0x0d1a		@ send CMD13 - SEND_STATUS
					@ expect 48 bit response (R1)
cmd_16:		.word	0x101a		@ send CMD16 - SET_BLOCKLEN
					@ expect 48 bit response (R1)
cmd_17:		.word	0x113a		@ send CMD17 - READ_SINGLE_BLOCK
					@ expect 48 bit response (R1)
cmd_18:		.word	0x123a		@ CMD18: READ_MULTIPLE_BLOCK
					@ expect 48 bit response (R1)
acmd_41:	.word	0x2902		@ send ACMD41 - SD_SEND_OP_COND
					@ expect 48 bit response (R3)
acmd_42:	.word	0x2a1a		@ send CMD42 - LOCK_UNLOCK
					@ expect 48 bit response (R1B)
cmd_55:		.word	0x371a		@ CMD55 - APP_CMD
					@ expect 48 bit response (R1)
#endif  /* ENABLE_SD */
#endif	/* ENABLE_EMMC_BOOT */

