/**
 * system/src/diag/diag_sd.c
 *
 * History:
 *    2008/07/08 - [Dragon Chiang] created file
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

#define SECTOR_SIZE		512
#define DIAG_SD_BUF_START	g_hugebuf_addr
#define SECTORS_PER_OP		256

extern int rand(void);

static int diag_sd_init(char *argv[], const char *test_name, int verbose)
{
	int rval = 0;
	int slot, type;

	putstr("running SD ");
	putstr(test_name);
	putstr(" test ...\r\n");
	putstr("press any key to terminate!\r\n");

	if (strcmp(argv[0], "sd") == 0) {
		slot = SCARDMGR_SLOT_SD;
	} else if (strcmp(argv[0], "sdio") == 0) {
		slot = SCARDMGR_SLOT_SDIO;
	} else if (strcmp(argv[0], "sd2") == 0) {
		slot = SCARDMGR_SLOT_SD2;
	} else {
		slot = SCARDMGR_SLOT_SD;
	}

	if (strcmp(argv[1], "sd") == 0) {
		type = SDMMC_TYPE_SD;
	} else if (strcmp(argv[1], "sdhc") == 0) {
		type = SDMMC_TYPE_SDHC;
	} else if (strcmp(argv[1], "mmc") == 0) {
		type = SDMMC_TYPE_MMC;
	} else if (strcmp(argv[1], "MoviNAND") == 0) {
		type = SDMMC_TYPE_MOVINAND;
	} else {
		type = SDMMC_TYPE_AUTO;
	}

	timer_reset_count(TIMER2_ID);
	timer_enable(TIMER2_ID);
	rval = sdmmc_init(slot, type);
	timer_disable(TIMER2_ID);
	if (verbose) {
		putstr("\r\nInit takes: ");
		putdec(timer_get_count(TIMER2_ID));
		putstr("mS\r\n");
		putstr("SD clock: ");
		putdec(get_sd_freq_hz());
		putstr("Hz\r\n");
#if (SD_HAS_SDXC_CLOCK == 1)
		putstr("SDXC clock: ");
#if (CHIP_REV == A7S)
		putdec((u32)amb_get_sdio_clock_frequency(HAL_BASE_VP));
#else
		putdec((u32)amb_get_sdxc_clock_frequency(HAL_BASE_VP));
#endif
		putstr("Hz\r\n");
#endif
		putstr("total_secs: ");
		putdec(sdmmc_get_total_sectors());
		putstr("\r\n");
	}

	return rval;
}

int diag_sd_read(char *argv[])
{
	int rval = 0;
	int i = 0;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	rval = diag_sd_init(argv, "Read", 1);
	if (rval < 0)
		return rval;

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		rval = sdmmc_read_sector(sector, sectors, (unsigned int *)buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (rval < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}

		if ((sector + SECTORS_PER_OP) >= total_secs) {
			putstr("\r\n");
			sector = -SECTORS_PER_OP;
		}
	}

	putstr("\r\ndone!\r\n");

	return rval;
}

int diag_sd_read_speed(char *argv[])
{
	int rval = 0;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;
	u32 op_size;

	rval = diag_sd_init(argv, "Read Speed", 1);
	if (rval < 0)
		return rval;

	total_secs = sdmmc_get_total_sectors();
	op_size = 0;
	timer_reset_count(TIMER2_ID);
	timer_enable(TIMER2_ID);
	for (sector = 0; sector < total_secs; sector += SECTORS_PER_OP) {
		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		rval = sdmmc_read_sector(sector, sectors, (unsigned int *)buf);
		if (rval < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}
		op_size += sectors;
	}
	timer_disable(TIMER2_ID);

	putstr("\r\nTotally read 0x");
	puthex(op_size * SECTOR_SIZE);
	putstr(" Bytes in ");
	putdec(timer_get_count(TIMER2_ID));
	putstr(" mS, about ");
	putdec(op_size * 500 / timer_get_count(TIMER2_ID));
	putstr(" KB/s!\r\n\r\n");

	return rval;
}

int diag_sd_write(char *argv[])
{
	int rval = 0;
	int i, j;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	rval = diag_sd_init(argv, "Write", 1);
	if (rval < 0)
		return rval;

	for (i = 0; i < SECTORS_PER_OP * SECTOR_SIZE / 16; i++) {
		for (j = 0; j < 16; j++) {
			buf[(i * 16) + j] = i;
		}
	}

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		rval = sdmmc_write_sector(sector, sectors, (unsigned int *)buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (rval < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
		}

		if ((sector + SECTORS_PER_OP) >= total_secs) {
			putstr("\r\n");
			sector = -SECTORS_PER_OP;
		}
	}

	putstr("\r\ndone!\r\n");

	return rval;
}

int diag_sd_write_speed(char *argv[])
{
	int rval = 0;
	int i, j;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;
	u32 op_size;

	rval = diag_sd_init(argv, "Write Speed", 1);
	if (rval < 0)
		return rval;

	for (i = 0; i < SECTORS_PER_OP * SECTOR_SIZE / 16; i++) {
		for (j = 0; j < 16; j++) {
			buf[(i * 16) + j] = i;
		}
	}

	total_secs = sdmmc_get_total_sectors();
	op_size = 0;
	timer_reset_count(TIMER2_ID);
	timer_enable(TIMER2_ID);
	for (sector = 0; sector < total_secs; sector += SECTORS_PER_OP) {
		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		rval = sdmmc_write_sector(sector, sectors, (unsigned int *)buf);
		if (rval < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}
		op_size += sectors;
	}
	timer_disable(TIMER2_ID);

	putstr("\r\nTotally write 0x");
	puthex(op_size * SECTOR_SIZE);
	putstr(" Bytes in ");
	putdec(timer_get_count(TIMER2_ID));
	putstr(" mS, about ");
	putdec(op_size * 500 / timer_get_count(TIMER2_ID));
	putstr(" KB/s!\r\n\r\n");

	return rval;
}

static int diag_sd_verify_data(u8* origin, u8* data, u32 len, int sector)
{
	int err_cnt = 0;
	u32 i;

	for (i = 0; i < len; i++) {
		if (origin[i] != data[i]) {
			err_cnt++;
			putstr("\r\nSector ");
			putdec(i / SECTOR_SIZE + sector);
			putstr(" byte ");
			putdec(i % SECTOR_SIZE);
			putstr(" failed data origin: 0x");
			puthex(origin[i]);
			putstr(" data: 0x");
			puthex(data[i]);
			putstr("\r\n");
		}
	}

	return err_cnt;
}

int diag_sd_verify(char *argv[])
{
	int rval = 0;
	int i;
	int total_secs;
	int sector, sectors;
	u8 *wbuf = (u8 *)DIAG_SD_BUF_START;
	u8 *rbuf = (u8 *)DIAG_SD_BUF_START + 0x100000;

	rval = diag_sd_init(argv, "Verify", 1);
	if (rval < 0)
		return rval;

	for (i = 0; i < SECTORS_PER_OP * SECTOR_SIZE; i++) {
		wbuf[i] = rand() / SECTORS_PER_OP;
	}

	total_secs = sdmmc_get_total_sectors();
	for (i = 0, sector = 0, sectors = 0; sector < total_secs;
		i++, sector += SECTORS_PER_OP) {

		if (uart_poll())
			break;

		sectors++;
		if (sectors > SECTORS_PER_OP)
			sectors = 1;

		rval = sdmmc_write_sector(sector, sectors,
			(unsigned int *)wbuf);
		if (rval != 0) {
			putstr("Write sector fail ");
			putdec(rval);
			putstr(" @ ");
			putdec(sector);
			putstr(" : ");
			putdec(sectors);
			putstr("\r\n");
		}

		rval = sdmmc_read_sector(sector, sectors, (unsigned int *)rbuf);
		if (rval != 0) {
			putstr("Read sector fail ");
			putdec(rval);
			putstr(" @ ");
			putdec(sector);
			putstr(" : ");
			putdec(sectors);
			putstr("\r\n");
		}

		rval = diag_sd_verify_data(wbuf, rbuf,
			(sectors * SECTOR_SIZE), sector);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if ((sector + SECTORS_PER_OP) >= total_secs) {
			putstr("\r\n");
			sector = -SECTORS_PER_OP;
		}
	}

	putstr("\r\ndone!\r\n");

	return rval;
}

