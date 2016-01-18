/**
 * system/src/bld/sdmmc.c
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#include <sdmmc.h>

//#define SDMMC_DEBUG
#define SDMMC_SELF_TEST
#undef PIO_MODE

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

#define SD_INIT_TIMEOUT	1000
#define SD_TRAN_TIMEOUT 5000

/*
 *	A6	SD_BOOT[11]	NAND_READ_CONFIRM[6]	2K[5]
 *	A2S/M	SD_BOOT[11]	RDY_PL[13]		HIF type[12]
 *----------------------------------------------------------------------
 *	SD		1		1		0
 *	SDHC		1		0		1
 *	MMC		1		0		0
 *	MoviNAND	1		1		1
 */

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

/* SDIO pin definition */
//#define SDMMC_PWR_PIN		69	/* SMIO 38 */
#define SDMMC_CD_PIN		75	/* SMIO 44 */
#define SDMMC_WP_PIN		76	/* SMIO 45 */

typedef struct card_s {
	u32 		hcs;		/**< Host capacity support */
	u32 		rca;		/**< the current active card's RCA */
	u32 		ocr;		/**< Operation condition Register */
	u32		ccs;
	u32		resp[4];	/**< Response from card */
	u64		capacity;	/**< Card capacity */
	u32		clk_hz;		/**< controller clock divider */
	u8		init_loop;	/**< Initialization loop count */
	u8		is_mmc;
	u32		slot;
	struct sdmmc_csd	csd;	/**< Card Specific Register */
	struct sdmmc_ext_csd	ext_csd;/**< Card Specific Register */
} card_t;

static u8 safe_buf[1024] __attribute__ ((aligned(1024)));
static int sector_mode = 0;
static int G_bld_mmc_rca = 0x01;
static int sd_base = SD_BASE;
static card_t card __attribute__ ((aligned(32)));

int sdmmc_read_sector(int, int, u32 *);

/**
 * Reset the CMD line.
 */
static void sdmmc_reset_cmd_line(u32 sd_base)
{
	/* Reset the CMD line */
	DEBUG_MSG("set SD_RESET_REG 0x");
	DEBUG_HEX_MSG(SD_RESET_CMD);
	DEBUG_MSG("\r\n");
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_CMD);

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET)) & SD_STA_CMD_INHIBIT_CMD);
}

/**
 * Reset the DATA line.
 */
static void sdmmc_reset_data_line(u32 sd_base)
{
	/* Reset the DATA line */
	DEBUG_MSG("set SD_RESET_REG 0x");
	DEBUG_HEX_MSG(SD_RESET_DAT);
	DEBUG_MSG("\r\n");
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_DAT);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET)) & SD_STA_CMD_INHIBIT_DAT);
}

/**
 * Reset the SD controller.
 */
static void sdmmc_reset_all(u32 sd_base)
{
	/* Reset the controller and wait for the register to clear */
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_ALL);

	/* Wait for reset to complete (busy wait!) */
	while (readb(sd_base + SD_RESET_OFFSET) != 0x0);

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

/*
 * Clean interrupts status.
 */
static void sdmmc_clean_interrupt(void)
{
	writew((sd_base + SD_NIS_OFFSET), readw(sd_base + SD_NIS_OFFSET));

	writew((sd_base + SD_EIS_OFFSET), readw(sd_base + SD_EIS_OFFSET));
}

/**
 * Get sd protocol response.
 */
static void sdmmc_get_resp(u32 sd_base, u32 *resp)
{
	/* The Arasan HC has unusual response register contents (compared) */
	/* to other host controllers, we need to read and convert them into */
	/* one supported by the protocol stack */
	resp[0] = ((readl(sd_base + SD_RSP3_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP2_OFFSET)) >> 24);
	resp[1] = ((readl(sd_base + SD_RSP2_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP1_OFFSET) >> 24));
	resp[2] = ((readl(sd_base + SD_RSP1_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP0_OFFSET) >> 24));
	resp[3] = (readl(sd_base + SD_RSP0_OFFSET) << 8);
}

/**
 * Utility function for unstuffing SD/MMC protocol response messages.
 */
static u32 UNSTUFF_BITS(u32 *resp, int start, int size)
{
	const u32 __mask = (1 << (size)) - 1;
	const int __off = 3 - ((start) / 32);
	const int __shft = (start) & 31;
	u32 __res;

	__res = resp[__off] >> __shft;
	if ((size) + __shft >= 32)
		__res |= resp[__off-1] << (32 - __shft);
	return __res & __mask;
}


/**
 * Check if card is in slot.
 *
 * @returns - 0 if card is absent; 1 if card is present
 */
static u32 sdmmc_card_in_slot(u32 sd_base, int slot)
{
	u32 rval = 0;

	switch (slot) {
	case SCARDMGR_SLOT_SD:
#if (SD_INSTANCES == 2)
	case SCARDMGR_SLOT_SD2:
#endif	/* SD_INSTANCES == 2 */
		if ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CARD_INSERTED) ==
		    SD_STA_CARD_INSERTED)
			rval = 1;
		else
			putstr("No card present.\r\n");
		break;
	case SCARDMGR_SLOT_SDIO:
#if defined(SDMMC_SDIO_CD_GPIO)
		if (gpio_get(GPIO(SDMMC_SDIO_CD_GPIO)) == 0x0)
			rval = 1;
		else
			putstr("No card present.\r\n");
#else
		rval = 1;
#endif
		break;
	}

	return rval;
}


/*
 * SD/MMC command
 */
int sdmmc_command(int command, int argument, int timeout)
{
	int retval = 0;
	u32 eis, nis;
	u32 cur_tim;
	u32 s_tck, e_tck;

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	/* argument */
	writel((sd_base + SD_ARG_OFFSET), argument);

	/* command */
	writew((sd_base + SD_CMD_OFFSET), command);

	/* enable timer */
	timer_disable(TIMER3_ID);
	timer_enable(TIMER3_ID);
	s_tck = timer_get_tick(TIMER3_ID);

	/* Wait for command to complete */
	do {
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

		e_tck = timer_get_tick(TIMER3_ID);
		cur_tim = timer_tick2ms(s_tck, e_tck);
		if (cur_tim >= timeout)
			break;
	} while (((nis & SD_NIS_CMD_DONE) != SD_NIS_CMD_DONE) && (eis == 0) &&
		 ((nis & SD_NIS_ERROR) != SD_NIS_ERROR));
	nis = readw(sd_base + SD_NIS_OFFSET);
	eis = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (cur_tim >= timeout) {
		putstr("sdmmc_command( 0x");
		puthex(command);
		putstr(" ) timeout\r\n");
		retval = -1;
	}

	if ((eis != 0x0) || ((nis & SD_NIS_ERROR) == SD_NIS_ERROR)) {
		sdmmc_reset_cmd_line(sd_base);
		putstr("sdmmc_command( 0x");
		puthex(command);
		putstr(" ) [ 0x");
		puthex(eis);
		putstr(" , 0x");
		puthex(nis);
		putstr(" ] failed\r\n");
		retval = -2;
	}

	return retval;
}

static int mmc_send_ext_csd(unsigned int *target)
{
	u32 s_tck, e_tck;
	u32 cur_tim = 0;
	u16 nis, eis;

	u32 start_512kb = (u32)target & 0xfff80000;
	u32 end_512kb = ((u32)target + (512) - 1) & 0xfff80000;

	if (start_512kb != end_512kb) {
		putstr("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	/* wait DAT line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT));

	writel((sd_base + SD_DMA_ADDR_OFFSET), (u32)target);
	writel((sd_base + SD_ARG_OFFSET), 0);
	writew((sd_base + SD_BLK_SZ_OFFSET), (SD_BLK_SZ_512KB | 0x200));
	writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);
	writew((sd_base + SD_XFR_OFFSET), (SD_XFR_CTH_SEL | SD_XFR_DMA_EN));
	writew((sd_base + SD_CMD_OFFSET), 0x083a);

	/* Wait for command to complete */
	do {
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);
	} while (((nis & SD_NIS_CMD_DONE) != SD_NIS_CMD_DONE) && eis == 0x0);

	/* enable timer */
	timer_disable(TIMER3_ID);
	timer_enable(TIMER3_ID);
	s_tck = timer_get_tick(TIMER3_ID);

	do {
		e_tck = timer_get_tick(TIMER3_ID);
		cur_tim = timer_tick2ms(s_tck, e_tck);
		if (cur_tim >= SD_TRAN_TIMEOUT)
			goto done;

		/* get status */
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

	} while ((nis & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE && (eis == 0x0));
done:
	/* Check Error*/
	eis = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();
	flush_d_cache((void *)target, 512);

	if (eis != 0x0) {
		putstr("mmc_send_ext_csd() failed (0x");
		puthex(eis);
		putstr(")\r\n");
		return -1;
	}

	return eis;
}

static u32 sdmmc_send_status_cmd(u32 wait_busy, u32 wait_data_ready)
{
	u32					rsp0 = 0x0;
	int					rval;

	do {
		rval = sdmmc_command(cmd_13, (card.rca << 16), SD_INIT_TIMEOUT);
		if (rval != SDMMC_ERR_NONE) {
			DEBUG_MSG("failed cmd_13");
			goto sdmmc_send_status_exit;
		}
		rsp0 = readl(sd_base + SD_RSP0_OFFSET);
	} while ((wait_data_ready && !(rsp0 & (1 << 8))) ||
		(wait_busy && (((rsp0 & 0x00001E00) >> 9) == 7)));

sdmmc_send_status_exit:
	return rsp0;
}

#ifdef SDMMC_SELF_TEST
/**
* Do bus test to make sure current clock is ok
*/
static int sdmmc_self_test(void)
{
	int rval;
	unsigned int *target = (unsigned int *)safe_buf;

	rval = sdmmc_read_sector(0, 1, target);
	if (rval < 0)
		goto done;

	rval = sdmmc_read_sector(0, 2, target);
	if (rval < 0)
		goto done;
done:
	return rval;
}
#endif

static void sdmmc_fio_select(int slot)
{
	switch (slot) {
	case SCARDMGR_SLOT_SD:
		writel(FIO_CTR_REG, (readl(FIO_CTR_REG) & ~FIO_CTR_XD));
		break;
	case SCARDMGR_SLOT_SDIO:
		writel(FIO_CTR_REG, (readl(FIO_CTR_REG) | FIO_CTR_XD));
		break;
#if (SD_INSTANCES == 2)
	case SCARDMGR_SLOT_SD2:
		writel(FIO_CTR_REG, (readl(FIO_CTR_REG) & ~FIO_CTR_XD));
		break;
#endif	/* SD_INSTANCES */
	}

	/* enable SD controller */
	writel(FIO_DMACTR_REG,
	       ((readl(FIO_DMACTR_REG) & 0xcfffffff) | FIO_DMACTR_SD));
}

/**
 * Initialize card in SDIO aware host.
 *
 * This function follows the standard card initialization flow in SD mode (of
 * an SDIO aware host) as specified in the SDIO Simple Specification Version
 * 1.0 [Figure 2, page 5].
 */
int sdmmc_init(int slot, int device_type)
{
	int rval = -1;
	int poll_count;
	u32 mrdy;
	u32 clock = SDMMC_DEFAULT_CLK;

	DEBUG_MSG("Initialize AMB SD host controller ");

	/* TODO: to configurate RCT */

	switch (slot) {
	case SCARDMGR_SLOT_SD:
		DEBUG_MSG("SD1");
#if defined(AMBSD_SD_PWR_PIN)
		gpio_config_sw_out(GPIO(AMBSD_SD_PWR_PIN));
		gpio_set(GPIO(AMBSD_SD_PWR_PIN));
#endif
		sd_base = SD_BASE;
#if defined(AMBSD_SD_CLOCK)
		clock = AMBSD_SD_CLOCK;
#endif
		rct_set_sd_pll(SDMMC_DEFAULT_INIT_CLK);
		break;

	case SCARDMGR_SLOT_SDIO:
		DEBUG_MSG("SDIO");
#if defined(AMBSD_SDIO_PWR_PIN)
		gpio_config_sw_out(GPIO(AMBSD_SDIO_PWR_PIN));
		gpio_set(GPIO(AMBSD_SDIO_PWR_PIN));
#endif
		sd_base = SD_BASE;
#if defined(AMBSD_SDIO_CLOCK)
		clock = AMBSD_SDIO_CLOCK;
#endif
		rct_set_sd_pll(SDMMC_DEFAULT_INIT_CLK);
		break;

#if (SD_INSTANCES == 2)
	case SCARDMGR_SLOT_SD2:
		DEBUG_MSG("SD2");
#if defined(AMBSD_SD2_PWR_PIN)
		gpio_config_sw_out(GPIO(AMBSD_SD2_PWR_PIN));
		gpio_set(GPIO(AMBSD_SD2_PWR_PIN));
#endif
		sd_base = SD2_BASE;
#if defined(AMBSD_SD2_CLOCK)
		clock = AMBSD_SD2_CLOCK;
#endif
#if (SD_HAS_SDXC_CLOCK == 1)
		rct_set_sdxc_pll(SDMMC_DEFAULT_INIT_CLK);
#else
		rct_set_sd_pll(SDMMC_DEFAULT_INIT_CLK);
#endif
		break;
#endif	/* SD_INSTANCES */

	default:
		DEBUG_MSG("Not supported!");
		return -1;
	}

	DEBUG_MSG("\r\n");

	/* configure FIO */
	sdmmc_fio_select(slot);

	/* wait bus ready (about 3 ms) */
	timer_dly_ms(TIMER3_ID, 3);

	sdmmc_reset_all(sd_base);

	/* make sure card present */
	rval = sdmmc_card_in_slot(sd_base, slot);
	if (rval == 0)
		return SDMMC_ERR_NO_CARD;

	memset(&card, 0, sizeof(card));

	/* div to 24Mhz */
	card.clk_hz = clock;
	card.slot = slot;

	goto _sdmmc_init;

_sdmmc_init:

	/* Configure SD/MMC card */
	card.hcs = 0;
	card.rca = 0;
	card.ocr = 0x00ff0000;

	/* CMD0 - GO_IDLE_STATE */
	rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);

	/* Make sure the data line ready for Emmc boot */
	sdmmc_reset_data_line(sd_base);

	if (rval < 0)
		goto done_all;

	if (device_type != 0x0) {
		switch (device_type) {
		case SDMMC_TYPE_SD:
			DEBUG_MSG("SD card boot ...\r\n");
			sector_mode = 0;
			goto _setup_sd_card;
		case SDMMC_TYPE_SDHC:
			DEBUG_MSG("SDHC card boot ...\r\n");
			sector_mode = 1;
			card.hcs = 0x1;
			goto _setup_sdhc_card;
		case SDMMC_TYPE_MMC:
			DEBUG_MSG("MMC card boot ...\r\n");
			sector_mode = 0;
			goto _setup_mmc_card;
		case SDMMC_TYPE_MOVINAND:
			DEBUG_MSG("MoviNAND boot ...\r\n");
			sector_mode = 1;
			card.ocr = 0x40ff0000;
			goto _setup_mmc_card;
		}
	}

_setup_sdhc_card:

	/* CMD8 (SD Ver2.0 support only) */
	rval = sdmmc_command(cmd_8, cmd_8_arg, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		/* Ver2.00 or later SD memory card(voltage mismatch) */
		/* or Ver1.X SD memory card */
		/* or not SD memory card */
		DEBUG_MSG("failed cmd8");

		/* Reset again if card does not support SD ver2.00 */
		/* !!!This is not mentioned by SD 2.00 spec!!! */
		/* CMD0 - GO_IDLE_STATE */
		rval = sdmmc_command(0x0, 0x0000, SD_INIT_TIMEOUT);
		if (rval != SDMMC_ERR_NONE) {
			DEBUG_MSG("failed cmd0");
			goto done_all;
		}
	} else {
		sector_mode = 1;
		card.hcs = 0x1;
	}

	/* ACMD41: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		DEBUG_MSG("failed cmd55");
		goto _setup_mmc_card;
	}
	/* ACMD41: CMD41 */
	rval = sdmmc_command(acmd_41, (card.ocr | (card.hcs << HCS_SHIFT_BIT)),
		SD_INIT_TIMEOUT);
	sdmmc_get_resp(sd_base, card.resp);
	card.ocr = UNSTUFF_BITS(card.resp, 8, 32);

	if (rval == SDMMC_ERR_NONE) {
		/* If the response is valid, then it is definitely not an MMC
		 * card, we can skip it and go to SDMem initialization. */
		DEBUG_MSG("OCR of card is 0x");
		DEBUG_HEX_MSG(card.ocr);
		DEBUG_MSG("\r\n");
		goto _setup_sd_card;
	}

_setup_mmc_card:

	card.is_mmc = 1;

	/* CMD0 - GO_IDLE_STATE */
	rval = sdmmc_command(0x0, 0x0000, 0x3000);
	if (rval != SDMMC_ERR_NONE) {
		DEBUG_MSG("failed cmd0");
		goto done_all;
	}

	card.ocr = 0x40ff0000;
	/* Send CMD1 with working voltage until the memory becomes ready */
	poll_count = 0;
	do {
		/* CMD1 */
		mrdy = 0x0;
		rval = sdmmc_command(cmd_1, card.ocr, 0x3000);

		/* ocr = UNSTUFF_BITS(cmd.resp, 8, 32) */
		mrdy = readl(sd_base + SD_RSP0_OFFSET);

		if (rval != SDMMC_ERR_NONE) {
			DEBUG_MSG("failed cmd1");
			goto done_all;
		}

		poll_count++;
	} while ((mrdy & SDMMC_CARD_BUSY) == 0x0 && poll_count < 1000);

	if ((mrdy & SDMMC_CARD_BUSY) == 0) {
		DEBUG_MSG("give up polling cmd1");
		rval = -1;
		goto done_all;
	}

	if (mrdy & 0x40000000) {
		/* sector address mode */
		sector_mode = 1;
	}

	goto send_cmd2;

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
			goto done_all;
		}
	}

	if ((mrdy & SDMMC_CARD_BUSY) == 0x0) {
		DEBUG_MSG("give up polling acmd41");
		rval = -1;
		goto done_all;
	}

	/* ccs = UNSTUFF_BITS(cmd.resp, 38, 1) */
	if ((readl(sd_base + SD_RSP0_OFFSET) & CCS) != CCS) {
		/* CCS = 0 Standard capacity */
		card.ccs = 0;
		sector_mode = 0;
	} else {
		/* CCS = 1 High capacity */
		card.ccs = 1;
		sector_mode = 1;
	}

send_cmd2:

	/* CMD2: All Send CID */
	rval = sdmmc_command(cmd_2, 0x0, SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		DEBUG_MSG("failed cmd2");
		goto done_all;
	}

	/* CMD3: Ask RCA */
	if (card.is_mmc) {
		card.rca = (G_bld_mmc_rca++);
	}
	rval = sdmmc_command(cmd_3, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		DEBUG_MSG("failed cmd3");
		goto done_all;
	}

	if (card.rca == 0x0)
		card.rca = readl(sd_base + SD_RSP0_OFFSET) >> 16;

	/* CMD9: Get the CSD */
	rval = sdmmc_command(cmd_9, (card.rca << 16), SD_INIT_TIMEOUT);
	if (rval != SDMMC_ERR_NONE) {
		DEBUG_MSG("failed cmd9");
		goto done_all;
	}

	sdmmc_get_resp(sd_base, card.resp);

	card.csd.spec_vers =	UNSTUFF_BITS(card.resp, 122,  4);
	card.csd.read_bl_len = 	UNSTUFF_BITS(card.resp,  80,  4);

	if (card.ccs == 0) {
		/* CSD version 1.0 */
		card.csd.c_size = 	UNSTUFF_BITS(card.resp,  62, 12);
		card.csd.c_size_mult =	UNSTUFF_BITS(card.resp,  47,  3);
	} else {
		/* CSD version 2.0 */
		card.csd.c_size = 	UNSTUFF_BITS(card.resp,  48, 22);
		card.csd.c_size &= 0x00ffff;/* Upper 6 bits shall be set to 0 */
	}

	/* Perform some calculation on the CSD just obtained */
	if (card.ccs == 0) {
		/* Standard capacity */
		card.capacity = (u64) (1 + card.csd.c_size) <<
				 (card.csd.c_size_mult + 2);
		card.capacity *= (u64) (1 << card.csd.read_bl_len);
	} else {
		/* High capacity */
		card.capacity = (u64) (card.csd.c_size + 1) * 512 * 1024;
	}

	/* Set bus width to 1 bit */
	writeb((sd_base + SD_HOST_OFFSET), 0x0);

	/* CMD13: Get status */
	sdmmc_send_status_cmd(1, 0);

	/* CMD7: Select card */
	rval = sdmmc_command(cmd_7, (card.rca << 16), SD_INIT_TIMEOUT);

	if (card.is_mmc)
		goto _setup_ext_mmc;

	/* ACMD42: Clear card detect */
	/* ACMD42: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

	/* ACMD42: CMD42 */
	rval = sdmmc_command(acmd_42, 0x0, 0x3000);	/* (set_cd ? 1 : 0) */

	/* ACMD6: Try to set the card to 4 bit data mode */
	/* ACMD6: CMD55 */
	rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

	/* ACMD6: ACMD6 */
	rval = sdmmc_command(acmd_6, 0x2, 0x3000);	/* width */

	/* Set bus width to 4 bits */
	writeb((sd_base + SD_HOST_OFFSET), SD_HOST_4BIT);

	goto _setup_card_done;

_setup_ext_mmc:
	if (card.csd.spec_vers < 4)
		goto _setup_card_done;

	if (card.clk_hz >= 24000000) {
		sdmmc_command(cmd_6, 0x03b90101, 0x3000);
		if (rval != SDMMC_ERR_NONE) {
			DEBUG_MSG("MMC into high-speed mode failed\r\n");
			goto _setup_card_done;
		}
	}

#if defined (ENABLE_MMC_8BIT)
	rval = sdmmc_command(cmd_6, 0x03b70200, 0x5000);
	if (rval == SDMMC_ERR_NONE) {
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_8BIT);
	} else {
		DEBUG_MSG("Set 8-bits bus width failed\r\n");
		writeb((sd_base + SD_HOST_OFFSET), 0x0);
	}
#else
	rval = sdmmc_command(cmd_6, 0x03b70100, 0x5000);
	if (rval == SDMMC_ERR_NONE) {
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_4BIT);
	} else  {
		DEBUG_MSG("Set 4-bits bus width failed\r\n");
		writeb((sd_base + SD_HOST_OFFSET), 0x0);
	}
#endif

_setup_card_done:

	/* CMD16: Set block length */
	rval = sdmmc_command(cmd_16, SEC_SIZE, SD_INIT_TIMEOUT);

	/* Disable clock */
	writew((sd_base + SD_CLK_OFFSET),
	    (readw(sd_base + SD_CLK_OFFSET) & ~(SD_CLK_ICLK_EN | SD_CLK_EN)));

#if (SD_SUPPORT_PLL_SCALER == 1)
	/* Setup clock 24 MHz */
	if (slot == SCARDMGR_SLOT_SD2) {
#if (SD_HAS_SDXC_CLOCK == 1)
		rct_set_sdxc_pll(card.clk_hz);
#else
		rct_set_sd_pll(card.clk_hz);
#endif
	} else
		rct_set_sd_pll(card.clk_hz);

	writew((sd_base + SD_CLK_OFFSET), (SD_CLK_ICLK_EN | SD_CLK_EN));
#else
	/* Setup clock */
	writew((sd_base + SD_CLK_OFFSET),
		(SD_CLK_DIV_2 | SD_CLK_ICLK_EN | SD_CLK_EN));
#endif

	/* Wait until clock becomes stable */
	while ((readw(sd_base + SD_CLK_OFFSET) & SD_CLK_ICLK_STABLE) !=
	       SD_CLK_ICLK_STABLE);

	/* Set block size (block size : 512) */
	writew((sd_base + SD_BLK_SZ_OFFSET), (SD_BLK_SZ_512KB | 0x200));

#ifdef SDMMC_SELF_TEST
	rval = sdmmc_self_test();
	if (rval < 0) {
		card.clk_hz -= SDMMC_CLK_DECREASE;
	}
#endif
	if ((card.is_mmc) && (card.csd.spec_vers >= 4)) {
		rval = mmc_send_ext_csd((unsigned int *)&card.ext_csd);
		if (rval != SDMMC_ERR_NONE)
			goto done_all;

		card.capacity = card.ext_csd.properties.sec_cnt;
		card.capacity <<= 9;
	}

done_all:
	if (rval < 0) {
		if (card.init_loop < 3) {
			card.init_loop ++;
			sdmmc_reset_all(sd_base);
			goto _sdmmc_init;
		} else
			putstr("sdmmc init fail\r\n");
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

	sdmmc_fio_select(card.slot);

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
		while ((readw(sd_base + SD_STA_OFFSET) &
		       SD_STA_BUFFER_READ_EN) != SD_STA_BUFFER_READ_EN);

		/* clear read buffer ready */
		writew((sd_base + SD_NIS_OFFSET), SD_NIS_READ_READY);

		/* To see if errors happen */
		rval = readw(sd_base + SD_EIS_OFFSET);

		if (rval != 0x0) {
			putstr("sdmmc_read_sector() failed\r\n");
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

	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(rval);
	DEBUG_MSG("\r\n");

	/* To see if errors happen */
	rval = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (rval != 0x0) {
		putstr("sdmmc_read_sector() failed (0x");
		puthex(rval);
		putstr(")\r\n");
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
	u32 s_tck, e_tck;
	u32 cur_tim = 0;
	u16 nis, eis;
	unsigned int dma_addr = 0;

	u32 start_512kb = (u32)target & 0xfff80000;
	u32 end_512kb = ((u32)target + (sectors << 9) - 1) & 0xfff80000;

	if (start_512kb != end_512kb) {
		putstr("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

	DEBUG_MSG("target address: 0x");
	DEBUG_HEX_MSG((u32)target);
	DEBUG_MSG("\r\n");

	clean_flush_d_cache((void *) target, (sectors << 9));

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	/* wait DAT line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT));

	writel((sd_base + SD_DMA_ADDR_OFFSET), (u32)target);

	DEBUG_MSG("set SD_DMA_ADDR_REG : 0x");
	DEBUG_HEX_MSG((u32)target);
	DEBUG_MSG("\r\n");

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

	/* enable timer */
	timer_disable(TIMER3_ID);
	timer_enable(TIMER3_ID);
	s_tck = timer_get_tick(TIMER3_ID);

	do {
		e_tck = timer_get_tick(TIMER3_ID);
		cur_tim = timer_tick2ms(s_tck, e_tck);
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
			/* set DMA address register */
			writel((sd_base + SD_DMA_ADDR_OFFSET), dma_addr);
		}
		timer_dly_ms(TIMER3_ID, 2);

	} while ((nis & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE && (eis == 0x0));

	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(nis);
	DEBUG_MSG("\r\n");

done:
	eis = readw(sd_base + SD_EIS_OFFSET);
	sdmmc_clean_interrupt();
	if (eis != 0x0) {
		putstr("sdmmc_read_sector() failed (0x");
		puthex(eis);
		putstr(")\r\n");
		sdmmc_reset_cmd_line(sd_base);
		sdmmc_reset_data_line(sd_base);
		return -1;
	}
	sdmmc_send_status_cmd(1, 1);

	return eis;
}

/**
 * Handle read DMA mode 512Kb alignment.
 */
static int sdmmc_read_sector_DMA(int sector, int sectors, unsigned int *target)
{
	int rval = 0;
	u32 s_bound;	/* Start address boundary */
	u32 e_bound;	/* End address boundary */
	u32 s_block, e_block, blocks;
	u32 *buf_ptr = target;

	int addr;

	if (sectors == 1)
		goto single_block_read;
	else
		goto multi_block_read;

single_block_read:
	/* Single-block read */

	s_bound = ((u32) buf_ptr) & 0xfff80000;
	e_bound = ((u32) buf_ptr) + (sectors << 9) - 1;
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
		s_bound = ((u32) target) + (s_block << 9);
		s_bound &= 0xfff80000;

		e_bound = ((u32) target) + ((s_block + 1) << 9) - 1;
		e_bound &= 0xfff80000;

		if (s_bound != e_bound) {
			u32 *tmp = (u32 *)safe_buf;

			addr = (sector + s_block);

			/* Read single block */
			rval = _sdmmc_read_sector_DMA(addr, 1, tmp);
			if (rval < 0)
				return rval;
			memcpy((u32 *)((u32)target + (s_block << 9)), tmp, 512);
			s_block++;

			if (s_block >= sectors)
				break;
		}

		/* Try with maximum data within same boundary */
		s_bound = ((u32) target) + (s_block << 9);
		s_bound &= 0xfff80000;
		e_block = s_block;
		do {
			e_block++;
			e_bound = ((u32) target) + ((e_block + 1) << 9) - 1;
			e_bound &= 0xfff80000;
		} while (e_block < blocks && s_bound == e_bound);
		e_block--;

		addr = (sector + s_block);

		/* Read multiple blocks */
		rval = _sdmmc_read_sector_DMA(addr, e_block - s_block + 1,
					(u32 *)((u32)target + (s_block << 9)));
		if (rval < 0)
			return rval;

		s_block = e_block + 1;
	}

	return rval;
}

int sdmmc_read_sector(int sector, int sectors, unsigned int *target)
{
	int rval = -1;

	sdmmc_fio_select(card.slot);

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

#if defined(PIO_MODE)

/*
 * Write single/multi sector to SD/MMC.
 */
int sdmmc_write_sector(int sector, int sectors, unsigned int *image)
{
	int rval = -1;

	int i, j;

	sdmmc_fio_select(card.slot);

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD) ==
	       SD_STA_CMD_INHIBIT_CMD);

	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_DAT);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT) ==
	       SD_STA_CMD_INHIBIT_DAT);

	if (sector_mode)			/* argument */
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET), 0x0);

		writew((sd_base + SD_CMD_OFFSET), cmd_24);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						  SD_XFR_AC12_EN	|
						  SD_XFR_BLKCNT_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_25);
	}

	/* Wait for command to complete */
	while ((readw(sd_base + SD_NIS_OFFSET) & SD_NIS_CMD_DONE) !=
	       SD_NIS_CMD_DONE);

	/* clear cmd compele state */
	writew(sd_base + SD_NIS_OFFSET, SD_NIS_CMD_DONE);

	/* write data */
	for (i = 0; i < sectors; i++) {

		/* Wait write buffer ready */
		while ((readw(sd_base + SD_STA_OFFSET) &
		       SD_STA_BUFFER_WRITE_EN) != SD_STA_BUFFER_WRITE_EN);

		/* clear write buffer ready */
		writew((sd_base + SD_NIS_OFFSET), SD_NIS_WRITE_READY);

		/* To see if errors happen */
		rval = readw(sd_base + SD_EIS_OFFSET);

		if (rval != 0x0) {
			putstr("sdmmc_write_sector() failed\r\n");
			return rval;
		}

		for (j = 0; j < (SEC_SIZE >> 2); j++) {
			writel((sd_base + SD_DATA_OFFSET), *image);
			image++;
		}
	}

	/* Wait for transfer complete */
	do {
		rval = readw(sd_base + SD_NIS_OFFSET);
	} while ((rval & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE);

	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(rval);
	DEBUG_MSG("\r\n");

	/* To see if errors happen */
	rval = readw(sd_base + SD_EIS_OFFSET);

	/* clear interrupts */
	sdmmc_clean_interrupt();

	if (rval != 0x0) {
		putstr("sdmmc_read_sector() failed (0x");
		puthex(rval);
		putstr(")\r\n");
		return -1;
	}

	return rval;
}

#else	/* DMA_MODE */

/**
 * ACMD23 Set pre-erased write block count.
 */
static int sdmmc_set_wr_blk_erase_cnt(u32 blocks)
{
	int rval = 0;
#if (SD_SUPPORT_ACMD23)
	if (card.is_mmc == 0) {
		/* ACMD23 : CMD55 */
		rval = sdmmc_command(cmd_55, (card.rca << 16), SD_INIT_TIMEOUT);

		/* ACMD23 */
		rval = sdmmc_command(acmd_23, blocks, SD_TRAN_TIMEOUT);
	}
#endif
	return rval;
}

/**
 * Write single/multi sector to SD/MMC.
 */
static int _sdmmc_write_sector_DMA(int sector, int sectors, unsigned int *image)
{
	u16 nis, eis;
	u32 s_tck, e_tck;
	u32 cur_tim = 0;

	u32 start_512kb = (u32)image & 0xfff80000;
	u32 end_512kb = ((u32)image + (sectors << 9) - 1) & 0xfff80000;

	if (start_512kb != end_512kb) {
		putstr("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

	clean_d_cache((void *)image, (sectors << 9));

	DEBUG_MSG("image address: 0x");
	DEBUG_HEX_MSG((unsigned int)image);
	DEBUG_MSG("\r\n");

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD) ==
	       SD_STA_CMD_INHIBIT_CMD);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT) ==
	       SD_STA_CMD_INHIBIT_DAT);

	writel((sd_base + SD_DMA_ADDR_OFFSET), (unsigned int)image);

	DEBUG_MSG("set SD_DMA_ADDR_REG : 0x");
	DEBUG_HEX_MSG((unsigned int)image);
	DEBUG_MSG("\r\n");

	if (sector_mode)			/* argument */
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_24);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						   SD_XFR_AC12_EN	|
						   SD_XFR_BLKCNT_EN	|
						   SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_25);
	}

	/* Wait for command to complete */
	do {
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);
	} while (((nis & SD_NIS_CMD_DONE) != SD_NIS_CMD_DONE) && eis == 0x0);

	if (eis != 0x0)
		goto done;

	/* enable timer */
	timer_disable(TIMER3_ID);
	timer_enable(TIMER3_ID);
	s_tck = timer_get_tick(TIMER3_ID);

	do {
		e_tck = timer_get_tick(TIMER3_ID);
		cur_tim = timer_tick2ms(s_tck, e_tck);
		if (cur_tim >= SD_TRAN_TIMEOUT)
			goto done;

		/* get status */
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

	} while (((nis & SD_NIS_XFR_DONE) != SD_NIS_XFR_DONE) && (eis == 0x0));

	DEBUG_MSG("get SD_NIS_REG : 0x");
	DEBUG_HEX_MSG(nis);
	DEBUG_MSG("\r\n");

done:
	eis = readw(sd_base + SD_EIS_OFFSET);
	sdmmc_clean_interrupt();
	if (eis != 0x0) {
		putstr("sdmmc_write_sector() failed (0x");
		puthex(eis);
		putstr(")\r\n");
		sdmmc_reset_cmd_line(sd_base);
		sdmmc_reset_data_line(sd_base);
		return -1;
	}

	sdmmc_send_status_cmd(1, 1);
	return eis;
}

/**
 * Handle write DMA mode 512Kb alignment.
 */
static int sdmmc_write_sector_DMA(int sector, int sectors, unsigned int *image)
{
	int rval = 0;
	u32 s_bound;	/* Start address boundary */
	u32 e_bound;	/* End address boundary */
	u32 s_block, e_block, blocks;
	u32 *buf_ptr = image;
	int addr;

	if (sectors == 1)
		goto single_block_write;
	else
		goto multi_block_write;

single_block_write:
	/* Single-block write */

	s_bound = ((u32) image) & 0xfff80000;
	e_bound = ((u32) image) + (sectors << 9) - 1;
	e_bound &= 0xfff80000;

	rval = sdmmc_set_wr_blk_erase_cnt(1);
	if (rval < 0)
		return rval;

	if (s_bound != e_bound) {
		u32 *tmp = (u32 *)safe_buf;
		memcpy(tmp, buf_ptr, 512);
		rval = _sdmmc_write_sector_DMA(sector, 1, tmp);
	} else {
		rval = _sdmmc_write_sector_DMA(sector, 1, buf_ptr);
	}
	return rval;

multi_block_write:
	/* Multi-block write */
	blocks = sectors;
	s_block = 0;
	e_block = 0;
	while (s_block < blocks) {
		/* Check if the first block crosses DMA buffer */
		/* boundary */
		s_bound = ((u32) image) + (s_block << 9);
		s_bound &= 0xfff80000;

		e_bound = ((u32) image) + ((s_block + 1) << 9) - 1;
		e_bound &= 0xfff80000;

		if (s_bound != e_bound) {
			u32 *tmp = (u32 *)safe_buf;
			memcpy(tmp, (u32 *)((u32)image + (s_block << 9)), 512);

			addr = (sector + s_block);

			/* Set pre-erased write block count */
			rval = sdmmc_set_wr_blk_erase_cnt(1);
			if (rval < 0)
				return rval;

			/* Write single block */
			rval = _sdmmc_write_sector_DMA(addr, 1, tmp);
			if (rval < 0)
				return rval;
			s_block++;

			if (s_block >= sectors)
				break;
		}

		/* Try with maximum data within same boundary */
		s_bound = ((u32) image) + (s_block << 9);
		s_bound &= 0xfff80000;
		e_block = s_block;
		do {
			e_block++;
			e_bound = ((u32) image) + ((e_block + 1) << 9) - 1;
			e_bound &= 0xfff80000;
		} while (e_block < blocks && s_bound == e_bound);
		e_block--;

		addr = (sector + s_block);

		/* Set pre-erased write block count */
		rval = sdmmc_set_wr_blk_erase_cnt(e_block - s_block + 1);
		if (rval < 0)
			return rval;

		/* Write multiple blocks */
		rval = _sdmmc_write_sector_DMA(addr, e_block - s_block + 1,
					(u32 *)((u32)image + (s_block << 9)));
		if (rval < 0)
			return rval;

		s_block = e_block + 1;
	}

	return rval;
}

int sdmmc_write_sector(int sector, int sectors, unsigned int *image)
{
	int rval = -1;

	sdmmc_fio_select(card.slot);

	while (sectors > SEC_CNT) {

		rval = sdmmc_write_sector_DMA(sector, SEC_CNT, image);
		if (rval < 0)
			return rval;

		sector += SEC_CNT;
		sectors -= SEC_CNT;
		image += ((SEC_SIZE * SEC_CNT) >> 2);
	}

	rval = sdmmc_write_sector_DMA(sector, sectors, image);

	return rval;
}

#endif	/* defined(PIO_MODE) */

int __sdmmc_erase_sector(int sector, int sectors)
{
	int cmd1, cmd2, rval;

	if (sector + sectors > card.capacity) {
		putstr("sdmmc_erase_sector() wrong argument");
		return -1;
	}

	sectors += sector;
	if (sector_mode == 0) {
		sector <<= 9;
		sectors <<= 9;
	}

	(card.is_mmc) ? (cmd1 = cmd_35, cmd2 = cmd_36):
			(cmd1 = cmd_32, cmd2 = cmd_33);

	sdmmc_fio_select(card.slot);

	/* CMD32/35:ERASE_WR_BLK_START */
	rval = sdmmc_command(cmd1, sector, SD_TRAN_TIMEOUT);
	if (rval < 0)
		goto done;

	/* CMD33/36:ERASE_WR_BLK_END */
	rval = sdmmc_command(cmd2, sectors, SD_TRAN_TIMEOUT);
	if (rval < 0)
		goto done;

	/* CMD38:ERASE */
	rval = sdmmc_command(cmd_38, 0x0, SD_TRAN_TIMEOUT);
	if (rval < 0)
		goto done;

	sdmmc_send_status_cmd(1, 1);
done:
	return rval;

}

int sdmmc_erase_sector(int sector, int sectors)
{
	int rval = 0;

#ifndef USE_SDMMC_ERASE_WRITE_SECTOR
	rval = __sdmmc_erase_sector(sector, sectors);
#else
	int i = sector;
#ifndef USE_SDMMC_HUGE_BUFFER
	u32 max_wsec = sizeof(safe_buf) / SEC_SIZE;
	u32 *tmp_buf = (u32 *)safe_buf;
#else
	u32 max_wsec = 256;
	u32 *tmp_buf = (u32 *)g_hugebuf_addr;
#endif
	u32 n = sectors / max_wsec;
	u32 remind = sectors % max_wsec;

	memzero((void *)tmp_buf, max_wsec << 9);

	for (; n > 0; n--) {
		rval = sdmmc_write_sector(i, max_wsec, (u32 *)tmp_buf);
		if (rval < 0)
			return rval;
		i += max_wsec;
	}
	if (remind != 0)
		rval = sdmmc_write_sector(i, remind, (u32 *)tmp_buf);
#endif

	return rval;
}

u32 sdmmc_get_total_sectors(void)
{
	return card.capacity >> 9;
}

