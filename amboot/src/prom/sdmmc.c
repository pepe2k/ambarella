/**
 * system/src/prom/sdmmc.c
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

#define __AMBPROM_IMPL__
#include "prom.h"
#include <sdmmc.h>
#include <fio/sdboot.h>

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)  || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2) || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
/* Only compiled when SM media is SD device. */

#ifndef __DEBUG_BUILD__
#undef SDMMC_DEBUG
#endif

#ifdef SDMMC_DEBUG
#define DEBUG_MSG uart_putstr
#else
#define DEBUG_MSG(...)
#endif

#ifdef SDMMC_DEBUG
#define DEBUG_HEX_MSG uart_puthex
#else
#define DEBUG_HEX_MSG(...)
#endif

#define ENABLE_TIMER
#define SD_INIT_MAX_RETRY	10
#define SD_INIT_TIMEOUT		1000
#define SD_TRAN_TIMEOUT		5000

/* SDMMC command */
#define cmd_1	0x0102	/* SEND_OP_COND		expect 48 bit response (R3) */
#define cmd_2	0x0209	/* ALL_SEND_CID		expect 136 bit response (R2) */
#define cmd_3	0x031a	/* SET_RELATIVE_ADDR	expect 48 bit response (R6) */
#define acmd_6	0x061a	/* SET_BUS_WIDTH	expect 48 bit response (R1) */
#define acmd_23	0x171a	/* SET_WR_BLK_ERASE_CNT	expect 48 bit response (R1) */
#define cmd_6	0x061b	/* SWITCH_BUSWIDTH	expect 48 bit response (R1B) */
#define cmd_7	0x071b	/* SELECT_CARD		expect 48 bit response (R1B) */
#define cmd_8	0x081a	/* SEND_EXT_CSD		expect 48 bit response (R7) */
#define cmd_9	0x0909	/* GET_THE_CSD		expect 136 bit response (R2) */
#define cmd_13	0x0d1a	/* SEND_STATUS		expect 48 bit response (R1) */
#define cmd_16	0x101a	/* SET_BLOCKLEN		expect 48 bit response (R1) */
#define cmd_17	0x113a	/* READ_SINGLE_BLOCK	expect 48 bit response (R1) */
#define cmd_18	0x123a	/* READ_MULTIPLE_BLOCK	expect 48 bit response (R1) */
#define cmd_24	0x183a	/* WRITE_BLOCK		expect 48 bit response (R1) */
#define cmd_25	0x193a	/* WRITE_MULTIPLE_BLOCK	expect 48 bit response (R1) */
#define cmd_32	0x201a	/* ERASE_WR_BLK_START	expect 48 bit response (R1) */
#define cmd_33	0x211a	/* ERASE_WR_BLK_END	expect 48 bit response (R1) */
#define cmd_35	0x231a	/* ERASE_GROUP_START	expect 48 bit response (R1) */
#define cmd_36	0x241a	/* ERASE_GROUP_END	expect 48 bit response (R1) */
#define cmd_38	0x261b	/* ERASE		expect 48 bit response (R1B) */
#define acmd_41	0x2902	/* SD_SEND_OP_COND	expect 48 bit response (R3) */
#define acmd_42	0x2a1b	/* LOCK_UNLOCK		expect 48 bit response (R1B) */
#define cmd_55	0x371a	/* APP_CMD		expect 48 bit response (R1) */

#define cmd_8_arg 0x01aa /* (SDMMC_HIGH_VOLTAGE << 8) | SDMMC_CHECK_PATTERN */

#define CCS	(0x1 << CCS_SHIFT_BIT)

#define SEC_SIZE	512	/* sector size */
#define SEC_CNT		1024	/* sector count */

typedef struct card_s {
	int hcs;
	int rca;
	int ocr;
	u32 clk_hz;
} card_t;

static int sector_mode = 0;

static int G_mmc_rca = 0x01;

#ifndef PIO_MODE
static __ARMCC_ALIGN(1024) u8 safe_buf[1024] __GNU_ALIGN(1024);
#endif

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)
static int sd_base = SD_BASE;
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2)
static int sd_base = SD2_BASE;
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
static int sd_base = SD_BASE;
#else
static int sd_base = SD_BASE;	/* SCARDMGR_SLOT_SDIO */
#endif

extern u32 get_core_bus_freq_hz(void);
int sdmmc_read_sector(int sector, int sectors, unsigned int *target);

/**
 * Initialize the timer to start ticking.
 */
static inline void timer1_init(void)
{
#ifdef ENABLE_TIMER
	/* Reset all timers */
	writel(TIMER_CTR_REG, 0x0);

	writel(TIMER1_STATUS_REG, 0xffffffff);
        writel(TIMER1_RELOAD_REG, 0xffffffff);
        writel(TIMER1_MATCH1_REG, 0x0);
        writel(TIMER1_MATCH2_REG, 0x0);

	/* Start timer */
	writel(TIMER_CTR_REG, 0x5);
#endif
}

static void timer1_enable(void)
{
#ifdef ENABLE_TIMER
	u32 val;

	val = readl(TIMER_CTR_REG);
	writel(TIMER_CTR_REG, val | 0x5);
#endif
}

static void timer1_disable(void)
{
#ifdef ENABLE_TIMER
	u32 val;

	val = readl(TIMER_CTR_REG);
	writel(TIMER_CTR_REG, val & (~0xf));
	writel(TIMER1_STATUS_REG, 0xffffffff);
#endif
}

static u32 timer1_get_count(void)
{
#ifdef ENABLE_TIMER
	return readl(TIMER1_STATUS_REG);
#else
	return 0;
#endif
}

/**
 * Get duration in ms.
 *
 */
static u32 timer1_get_end_time(u32 s_tck, u32 e_tck)
{
#ifdef ENABLE_TIMER
	u32 apb_freq = get_core_bus_freq_hz() >> 1;
	return (s_tck - e_tck) / (apb_freq / 1000);
#else
	return 0;
#endif
}

static void dly_ms(u32 dly_tim)
{
#ifdef ENABLE_TIMER
	u32 cur_tim;
	u32 s_tck, e_tck;

	timer1_disable();
	timer1_enable();
	s_tck = timer1_get_count();
	while (1) {
		e_tck = timer1_get_count();
		cur_tim = timer1_get_end_time(s_tck, e_tck);
		if (cur_tim >= dly_tim)
			break;
	}
#else
	int cnt;
	u32 arm_freq = get_core_bus_freq_hz();

#if (CHIP_REV == A5S) || (CHIP_REV == A7)
	arm_freq *= 4;
#endif
	cnt = (arm_freq / 1000) * dly_tim / 10;
	for (; cnt > 0; cnt--);
#endif
}

/**
 * HW setup, initial power mode and GPIO.
 */
static inline void sdmmc_init_setup(void)
{
	u32 is_sdio = 0;

#if defined(SM_PWRCYC_GPIO)
	gpio_config_sw_out(GPIO(SM_PWRCYC_GPIO));
	gpio_set(GPIO(SM_PWRCYC_GPIO));

	/* wait power ready (about 3 ms) */
	dly_ms(3);
#endif

	/* TODO: to configurate RCT */

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)
	DEBUG_MSG("SD1\r\n");
	is_sdio = 0;
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2)
	DEBUG_MSG("SD2\r\n");
	is_sdio = 0;
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
	DEBUG_MSG("SDIO\r\n");
	is_sdio = 1;
#else
	DEBUG_MSG("SDIO\r\n");
	is_sdio = 1;
#endif /* FIRMWARE_CONTAINER */

	/* configure FIO */
	if (is_sdio)
		writel(FIO_CTR_REG, (readl(FIO_CTR_REG) | FIO_CTR_XD));
	else
		writel(FIO_CTR_REG, (readl(FIO_CTR_REG) & ~FIO_CTR_XD));

	/* wait bus ready (about 3 ms) */
	dly_ms(3);

	/* enable SD controller */
	writel(FIO_DMACTR_REG,
	       ((readl(FIO_DMACTR_REG) & 0xcfffffff) | FIO_DMACTR_SD));
}

static inline void sdmmc_reset_all(u32 sd_base)
{
	/* Reset the controller and wait for the register to clear */
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_ALL);

	/* Wait for reset to complete (busy wait!) */
	while (readl(sd_base + SD_RESET_OFFSET) != 0x0);

	/* Set SD Power (3.3V) */
	writeb((sd_base + SD_PWR_OFFSET), (SD_PWR_3_3V | SD_PWR_ON));

	/* Set data timeout */
	writeb((sd_base + SD_TMO_OFFSET), 0xe);

	/* Setup clock */
	writew((sd_base + SD_CLK_OFFSET),
	       (SD_CLK_DIV_256 | SD_CLK_ICLK_EN | SD_CLK_EN));

	/* Wait until clock becomes stable */
	while ((readw(sd_base + SD_CLK_OFFSET) & SD_CLK_ICLK_STABLE) !=
	       SD_CLK_ICLK_STABLE);

	/* Enable interrupts on events we want to know */
#if defined(PIO_MODE)
	writew((sd_base + SD_NISEN_OFFSET), 0x1f7);
	writew((sd_base + SD_NIXEN_OFFSET), 0x1f7);
#else	/* DMA_MODE */
	writew((sd_base + SD_NISEN_OFFSET), 0x1cf);
	writew((sd_base + SD_NIXEN_OFFSET), 0x1cf);
#endif	/* defined(PIO_MODE) */

	/* Enable error interrupt status */
	writew((sd_base + SD_EISEN_OFFSET), 0x1ff);

	/* Enable error interrupt signal */
	writew((sd_base + SD_EIXEN_OFFSET), 0x1ff);
}

#if defined(SM_CD_GPIO)
static inline u32 sdmmc_card_in_slot(void)
{
	int val;

	gpio_config_sw_in(GPIO(sm_cd_gpio);
	val = gpio_get(GPIO(sm_cd_gpio);
	(val != 0) ? (return 0) : (return 1);
}
#endif

/*
 * Clean interrupts status.
 */
static void sdmmc_clean_interrupt(void)
{
	writew((sd_base + SD_NIS_OFFSET), readw(sd_base + SD_NIS_OFFSET));

	writew((sd_base + SD_EIS_OFFSET), readw(sd_base + SD_EIS_OFFSET));
}

/*
 * SD/MMC command
 */
static int sdmmc_command(int cmd, int arg, int timeout)
{
	u16 nis, eis;
	u32 cur_tim;
	u32 s_tck, e_tck;

	/* argument */
	writel((sd_base + SD_ARG_OFFSET), arg);

	/* command */
	writew((sd_base + SD_CMD_OFFSET), cmd);

	timer1_disable();
	timer1_enable();
	s_tck = timer1_get_count();

	/* Wait for command to complete */
	do {
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);
		e_tck = timer1_get_count();
		cur_tim = timer1_get_end_time(s_tck, e_tck);
		if (cur_tim >= timeout)
			break;
	} while (((nis & SD_NIS_CMD_DONE) != SD_NIS_CMD_DONE) && (eis == 0));

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (cur_tim >= timeout || eis != 0x0) {
		DEBUG_MSG("sdmmc_command( 0x");
		DEBUG_HEX_MSG(cmd);
		DEBUG_MSG(" ) timeout\r\n");
		if (eis != 0x0) {
			DEBUG_MSG("eis = 0x");
			DEBUG_HEX_MSG(eis);
			DEBUG_MSG("\r\n");
			writeb(sd_base + SD_RESET_OFFSET, SD_RESET_CMD);
			/* wait command line ready */
			while ((readl(sd_base + SD_STA_OFFSET)) &
			       SD_STA_CMD_INHIBIT_CMD);
			return -1;
		}
	}

	return 0;
}

static int sdmmc_self_test(void)
{
	int rval;
	u32 buf[1024];

	/* signal block read */
	rval = sdmmc_read_sector(0, 1, buf);
	if (rval != SDMMC_ERR_NONE)
		return rval;

	/* multiple block read */
	rval = sdmmc_read_sector(0, 2, buf);

	return rval;
}

int sdmmc_init(int slot, u32 type)
{
	int rval = -1;
	card_t card;
	int poll_count;
	u32 mrdy;
	u32 is_mmc = 0;
	u32 retry_cnt = 0;

	card.clk_hz = 24000000;
	rct_set_sd_pll(card.clk_hz);

	timer1_init();

	DEBUG_MSG("Initialize AMB SD host controller ");

_sdmmc_init:

	/* initial power, GPIO and FIO controller */
	sdmmc_init_setup();

	sdmmc_reset_all(sd_base);

#if defined(SM_CD_GPIO)
	rval = sdmmc_card_in_slot();
	if (rval != 1) {
		DEBUG_MSG("No card present.\r\n");
		return SDMMC_ERR_NO_CARD;
	}
#endif

	/* Configure SD/MMC card */
	card.hcs = 0;
	card.rca = 0;
	card.ocr = 0x00ff0000;

	/* CMD0 - GO_IDLE_STATE */
	rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);

	if ((type & BOOT_FROM_SD) == BOOT_FROM_SD) {
		DEBUG_MSG("SD card boot ...\r\n");
		sector_mode = 0;
		goto _setup_sd_card;
	} else if ((type & BOOT_FROM_SDHC) == BOOT_FROM_SDHC) {
		DEBUG_MSG("SDHC card boot ...\r\n");
		sector_mode = 1;
		card.hcs = 0x1;
		goto _setup_sdhc_card;
	} else if ((type & BOOT_FROM_MMC) == BOOT_FROM_MMC) {
		DEBUG_MSG("MMC card boot ...\r\n");
		is_mmc = 1;
		sector_mode = 0;
		goto _setup_mmc_card;
	} else if ((type & BOOT_FROM_MOVINAND) == BOOT_FROM_MOVINAND) {
		DEBUG_MSG("MoviNAND boot ...\r\n");
		sector_mode = 1;
		card.ocr = 0x40ff0000;
		goto _setup_mmc_card;
	}

#if (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_AUTO)
	/* CMD8 (SD Ver2.0 support only) */
	rval = sdmmc_command(cmd_8, cmd_8_arg, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		/* Ver2.00 or later SD memory card(voltage mismatch) */
		/* or Ver1.X SD memory card */
		/* or not SD memory card */

		/* CMD0 - GO_IDLE_STATE */
		rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);
		if (rval != SDMMC_ERR_NONE) {
			return -1;
		}
	} else {
		sector_mode = 1;
		card.hcs = 0x1;
	}

	/* ACMD41: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		/* CMD0 - GO_IDLE_STATE */
		rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);
		if (rval == SDMMC_ERR_NONE) {
			sector_mode = 0;
			goto _setup_mmc_card;
		} else {
			return -1;
		}
	}

	/* ACMD41: CMD41 */
	rval = sdmmc_command(acmd_41,
			     (card.ocr | (card.hcs << HCS_SHIFT_BIT)),
			     SD_INIT_TIMEOUT);

	if (rval == SDMMC_ERR_NONE) {
		sector_mode = 0;
		goto _setup_sd_card;
	} else
		goto _setup_card_done;;
#endif /* SDMMC_TYPE_AUTO */

_setup_mmc_card:

	card.ocr = 0x40ff0000;
	/* Send CMD1 with working voltage until the memory becomes ready */
	poll_count = 0;
	do {
		/* CMD1 */
		mrdy = 0x0;
		rval = sdmmc_command(cmd_1, card.ocr, SD_INIT_TIMEOUT);
		if (rval != 0) {
			DEBUG_MSG("cmd 1 failed.\r\n");
			goto _setup_card_done;
		}
		/* ocr = UNSTUFF_BITS(cmd.resp, 8, 32) */
		mrdy = readl(sd_base + SD_RSP0_OFFSET);
		poll_count++;
	} while ((mrdy & SDMMC_CARD_BUSY) == 0x0 && poll_count < 1000);

	if ((mrdy & SDMMC_CARD_BUSY) == 0) {
		DEBUG_MSG("give up polling cmd1");
		rval = -1;
		goto _setup_card_done;
	}

	if (mrdy & 0x40000000) {
		/* sector address mode */
		sector_mode = 1;
	}

	card.ocr = mrdy;
	is_mmc = 1;

	goto send_cmd2;  /* Jump to CMD2 */

_setup_sdhc_card:

	/* CMD8 (SD Ver2.0 support only) */
	rval = sdmmc_command(cmd_8, cmd_8_arg, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		/* CMD0 - GO_IDLE_STATE */
		rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);
		if (rval != SDMMC_ERR_NONE) {
			goto _setup_card_done;
		}
	} else {
		sector_mode = 1;
		card.hcs = 0x1;
	}

_setup_sd_card:
	/* ACMD41 - Send working voltage until the memory becomes ready */
	for (mrdy = poll_count = 0;
	     (mrdy & SDMMC_CARD_BUSY) == 0x0 && poll_count < 3000;
	     poll_count++) {

		/* ACMD41: CMD55 */
		rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

		/* ACMD41: CMD41 */
		mrdy = 0x0;
		rval = sdmmc_command(acmd_41,
				     (card.ocr | (card.hcs << HCS_SHIFT_BIT)),
				     SD_INIT_TIMEOUT);

		/* ocr = UNSTUFF_BITS(cmd.resp, 8, 32) */
		mrdy = readl(sd_base + SD_RSP0_OFFSET);

		if (rval != SDMMC_ERR_NONE) {
			DEBUG_MSG("failed in acmd41");
			goto _setup_card_done;
		}
	}

	if ((mrdy & SDMMC_CARD_BUSY) == 0x0) {
		DEBUG_MSG("give up polling acmd41");
		rval = -1;
		goto _setup_card_done;
	}

	card.ocr = mrdy;

	/* ccs = UNSTUFF_BITS(cmd.resp, 38, 1) */
	if ((readl(sd_base + SD_RSP0_OFFSET) & CCS) != CCS) {
		/* CCS = 0 Standard capacity */
		sector_mode = 0;
	} else {
		/* CCS = 1 High capacity */
		sector_mode = 1;
	}

send_cmd2:

	/* CMD2: All Send CID */
	rval = sdmmc_command(cmd_2, 0x0, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}

	/* CMD3: Ask RCA */
	if (is_mmc) {
		card.rca = (G_mmc_rca++);
	}

	rval = sdmmc_command(cmd_3, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}

	if (card.rca == 0x0)
		card.rca = readl(sd_base + SD_RSP0_OFFSET) >> 16;

	/* CMD13: Get status */
	rval = sdmmc_command(cmd_13, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}

	/* CMD7: Select card */
	rval = sdmmc_command(cmd_7, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}

	if (is_mmc)
		goto _setup_card_done;

	/* ACMD42: Clear card detect */
	/* ACMD42: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

	/* ACMD42: CMD42 */
	/* (set_cd ? 1 : 0) */
	rval = sdmmc_command(acmd_42, 0x0, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}


	/* ACMD6: Try to set the card to 4 bit data mode */
	/* ACMD6: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

	/* ACMD6: ACMD6 */
	rval = sdmmc_command(acmd_6, 0x2, SD_INIT_TIMEOUT);	/* width */
	if (rval != SDMMC_ERR_NONE) {
		goto _setup_card_done;
	}

	/* Set bus width to 4 bits */
	if (is_mmc)
		writeb((sd_base + SD_HOST_OFFSET), 0x0);
	else
		writeb((sd_base + SD_HOST_OFFSET), 0x2);

	/* CMD16: Set block length */
	rval = sdmmc_command(cmd_16, SEC_SIZE, SD_INIT_TIMEOUT);

_setup_card_done:

	if ((rval != SDMMC_ERR_NONE) && (retry_cnt < SD_INIT_MAX_RETRY)) {
		retry_cnt++;
		goto _sdmmc_init;
	}

	/* Disable clock */
	writew((sd_base + SD_CLK_OFFSET),
	    (readw(sd_base + SD_CLK_OFFSET) & ~(SD_CLK_ICLK_EN | SD_CLK_EN)));

#if (SD_SUPPORT_PLL_SCALER == 1)
	/* Setup clock 24 MHz */
	if (slot == SCARDMGR_SLOT_SD2) {
		card.clk_hz = 20000000;
		rct_set_sdxc_pll(card.clk_hz);
 	} else
		rct_set_sd_pll(card.clk_hz);

	writew((sd_base + SD_CLK_OFFSET), (SD_CLK_ICLK_EN | SD_CLK_EN));
#else
	/* Setup clock */
	writew((sd_base + SD_CLK_OFFSET),
	       (0x100 | SD_CLK_ICLK_EN | SD_CLK_EN)); /* 24 MHz */
#endif

	/* Wait until clock becomes stable */
	while ((readw(sd_base + SD_CLK_OFFSET) & SD_CLK_ICLK_STABLE) !=
	       SD_CLK_ICLK_STABLE);

	/* Set block size (block size : 512) */
	writew((sd_base + SD_BLK_SZ_OFFSET), (SD_BLK_SZ_512KB | 0x200));

	rval = sdmmc_self_test();
	if ((rval != SDMMC_ERR_NONE) && (retry_cnt < SD_INIT_MAX_RETRY)) {
		if (slot == SCARDMGR_SLOT_SD2)
			rct_set_sdxc_pll(SDMMC_FREQ_48MHZ);
		else
			rct_set_sd_pll(SDMMC_FREQ_48MHZ);
		card.clk_hz -= SDMMC_CLK_DECREASE;
		retry_cnt++;
		goto _sdmmc_init;
	}

	return rval;
}

/*****************************************************************************/

#if defined(PIO_MODE)

/*
 * Read single/multi sector from SD/MMC.
 */
int sdmmc_read_sector(int sector, int sectors, unsigned int *target)
{
	int rval = -1;

	int i, j;

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD) ==
	       SD_STA_CMD_INHIBIT_CMD);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT) ==
	       SD_STA_CMD_INHIBIT_DAT);

	/* argument */
	if (sector_mode)
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_CTH_SEL));

		writew((sd_base + SD_CMD_OFFSET), cmd_17);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						  SD_XFR_CTH_SEL	|
						  SD_XFR_AC12_EN	|
						  SD_XFR_BLKCNT_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_18);
	}

	/* Wait for command to complete */
	while ((readw(sd_base + SD_NIS_OFFSET) & SD_NIS_CMD_DONE) !=
	       SD_NIS_CMD_DONE);

	/* Read data */
	for (i = 0; i < sectors; i++) {

		/* Wait read buffer ready */
		while ((readw(sd_base + SD_NIS_OFFSET) & SD_NIS_READ_READY) !=
		       SD_NIS_READ_READY);

		/* clear read buffer ready */
		writew((sd_base + SD_NIS_OFFSET), SD_NIS_READ_READY);

		/* To see if errors happen */
		rval = readw(sd_base + SD_EIS_OFFSET);

		if (rval != 0x0) {
			DEBUG_MSG("sdmmc_read_sector() failed\r\n");
			return rval;
		}

		for (j = 0; j < (SEC_SIZE >> 2); j++) {
			*target = readl(sd_base + SD_DATA_OFFSET);
			target++;
		}
	}

	/* Wait for transfer complete */
	do {
		rval = readw(sd_base + SD_NIS_OFFSET);
	} while ((rval & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE);

#if defined(SDMMC_DEBUG)
	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(rval);
	DEBUG_MSG("\r\n");
#endif

	/* To see if errors happen */
	rval = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (rval != 0x0) {
		DEBUG_MSG("sdmmc_read_sector() failed (0x");
		DEBUG_HEX_MSG(rval);
		DEBUG_MSG(")\r\n");
		return -1;
	}

	return rval;
}

#else	/* DMA_MODE */

/*
 * Read single/multi sector from SD/MMC.
 */
static int _sdmmc_read_sector_DMA(int sector, int sectors, unsigned int *target)
{
	u16 nis, eis;
	unsigned int dma_addr = 0;
	u32 s_tck, e_tck;
	u32 cur_tim = 0;

	unsigned int start_512kb = (unsigned int)target & 0xfff80000;
	unsigned int end_512kb = ((unsigned int)target + (sectors << 9) - 1) &
					0xfff80000;
	if (start_512kb != end_512kb) {
		DEBUG_MSG("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

#if defined(SDMMC_DEBUG)
	DEBUG_MSG("target address: 0x");
	DEBUG_HEX_MSG((unsigned int)target);
	DEBUG_MSG("\r\n");
#endif

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD) ==
	       SD_STA_CMD_INHIBIT_CMD);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT) ==
	       SD_STA_CMD_INHIBIT_DAT);

	writel((sd_base + SD_DMA_ADDR_OFFSET), (unsigned int)target);

#if defined(SDMMC_DEBUG)
	DEBUG_MSG("set SD_DMA_ADDR_REG : 0x");
	DEBUG_HEX_MSG((unsigned int)target);
	DEBUG_MSG("\r\n");
#endif

	if (sector_mode)			/* argument */
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET),
		       (SD_XFR_CTH_SEL | SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_17);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						   SD_XFR_CTH_SEL	|
						   SD_XFR_AC12_EN	|
						   SD_XFR_BLKCNT_EN	|
						   SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_18);
	}

	/* Wait for command to complete */
	do {
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);
	} while (((nis & SD_NIS_CMD_DONE) != SD_NIS_CMD_DONE) && eis == 0x0);

	if (eis != 0x0)
		goto done;

	timer1_disable();
	timer1_enable();
	s_tck = timer1_get_count();

	do {
		e_tck = timer1_get_count();
		cur_tim = timer1_get_end_time(s_tck, e_tck);
		if (cur_tim >= SD_TRAN_TIMEOUT)
			goto done;

		/* get status */
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

		/* clear interrupts */
		writel(sd_base + SD_NIS_OFFSET, nis);

		if ((nis & SD_NIS_DMA) == SD_NIS_DMA) {
			dma_addr = readl(sd_base + SD_DMA_ADDR_OFFSET);

#if defined(SDMMC_DEBUG)
			DEBUG_MSG("reset SD_DMA_ADDR_REG : 0x");
			DEBUG_HEX_MSG(dma_addr);
			DEBUG_MSG("\r\n");
#endif

#if 0
			/* Wait read buffer ready */
			while ((readw(sd_base + SD_NIS_OFFSET) &
				SD_NIS_READ_READY) != SD_NIS_READ_READY);
#endif
			/* set DMA address register */
			writel((sd_base + SD_DMA_ADDR_OFFSET), dma_addr);
		}
		dly_ms(1);

	} while (((nis & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE) && (eis == 0x0));

#if defined(SDMMC_DEBUG)
	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(nis);
	DEBUG_MSG("\r\n");
#endif

done:

	/* To see if errors happen */
	eis = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (eis != 0x0) {
		DEBUG_MSG("sdmmc_read_sector() failed (0x");
		DEBUG_HEX_MSG(eis);
		DEBUG_MSG(")\r\n");
		return -1;
	} else if (cur_tim >= SD_TRAN_TIMEOUT) {
		DEBUG_MSG("sdmmc_read_sector() timeout");
		return -1;
	}

	return eis;
}

/**
 * Handle read DMA mode 512Kb alignment.
 */
static int sdmmc_read_sector_DMA(int sector, int sectors, unsigned int *buf)
{
	int rval = 0;
	int addr;
	int blocks;
	u32 s_bound;	/* Start address boundary */
	u32 e_bound;	/* End address boundary */
	int s_block;
	int e_block;

	u32 *buf_ptr = buf;

	if (sectors == 1)
		goto single_block_read;
	else
		goto multi_block_read;

single_block_read:
	/* Single-block read */

	s_bound = ((u32) buf_ptr) & 0xfff80000;
	e_bound = ((u32) buf_ptr) + (512 * sectors) - 1;
	e_bound &= 0xfff80000;

	if (s_bound != e_bound) {
		u32 *tmp = (u32 *)safe_buf;
		rval = _sdmmc_read_sector_DMA(sector, 1, tmp);
		memcpy(buf_ptr, tmp, 512);
	} else {
		rval = _sdmmc_read_sector_DMA(sector, 1, buf_ptr);
	}

	return rval;

multi_block_read:
	/* Multi-block read */
	blocks = sectors;
	s_block = 0;
	e_block = 0;
	while (s_block < blocks) {
		/* Check if the first block crosses DMA buffer */
		/* boundary */
		s_bound = ((u32) buf) + (512 * s_block);
		s_bound &= 0xfff80000;

		e_bound = ((u32) buf) + (512 * (s_block + 1)) - 1;
		e_bound &= 0xfff80000;

		if (s_bound != e_bound) {
			u32 *tmp = (u32 *)safe_buf;

			addr = (sector + s_block);

			/* Read single block */
			rval = _sdmmc_read_sector_DMA(addr, 1, tmp);
			if (rval < 0)
				return rval;
			memcpy((u32 *)((u32)buf + (512 * s_block)), tmp,512);
			s_block++;

			if (s_block >= sectors)
				break;
		}

		/* Try with maximum data within same boundary */
		s_bound = ((u32) buf) + (512 * s_block);
		s_bound &= 0xfff80000;
		e_block = s_block;
		do {
			e_block++;
			e_bound = ((u32) buf) + (512 * (e_block + 1)) - 1;
			e_bound &= 0xfff80000;
		} while (e_block < blocks && s_bound == e_bound);
		e_block--;

		addr = (sector + s_block);

		/* Read multiple blocks */
		rval = _sdmmc_read_sector_DMA(addr, e_block - s_block + 1,
					   (u32 *)((u32)buf + (512 * s_block)));
		if (rval < 0)
			return rval;

		s_block = e_block + 1;
	}

	return rval;
}

int sdmmc_read_sector(int sector, int sectors, unsigned int *target)
{
	int rval = -1;

	while (sectors > SEC_CNT) {

		rval = sdmmc_read_sector_DMA(sector, SEC_CNT, target);
		if (rval < 0)
			return rval;

		sector += SEC_CNT;
		sectors -= SEC_CNT;
		target += ((SEC_SIZE * SEC_CNT) >> 2);
	}

	rval = sdmmc_read_sector_DMA(sector, sectors, target);

	return rval;
}

#endif	/* defined(PIO_MODE) */

/*****************************************************************************/

typedef void (*amb_kernel_t)(void);

int sdmmc_boot(void)
{
	int rval = -1;
	int sector, sectors, target;
	unsigned int buffer[SEC_SIZE];

	amb_kernel_t kernel = NULL;

	/* Read sector 1 from SD card */
	rval = sdmmc_read_sector(1, 1, buffer);
	if (rval != 0) {
		DEBUG_MSG("can't read parameter data!\r\n");
		return -1;
	}

	/* Check sector is parameters sector */
	if (buffer[(SDBOOT_MAGIC_OFFSET >> 2)] != SDBOOT_HEADER_MAGIC) {
		DEBUG_MSG("magic number error!\r\n");
		return -1;
	}

	/* start sector */
	sector = buffer[(SDBOOT_START_BLOCK_OFFSET >> 2)];

	/* sectors */
	sectors = buffer[(SDBOOT_BLOCKS_OFFSET >> 2)];

	/* target address */
	target = buffer[(SDBOOT_MEM_TARGET_OFFSET >> 2)];

	kernel = (amb_kernel_t) target;

	/* load code to memory */
	rval = sdmmc_read_sector(sector, sectors, (unsigned int *) target);
	if (rval != 0) {
		DEBUG_MSG("can't read code data!\r\n");
		return -1;
	}

	DEBUG_MSG("kernel start ...\r\n");

	/* jump to target */
	(*kernel)();

	return 0;
}

#endif

