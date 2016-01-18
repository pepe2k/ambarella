/**
 * system/src/bld/loader.c
 *
 * History:
 *    2005/03/08 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>
#include <hotboot.h>
#include <self_refresh_const.h>

#ifdef ENABLE_DEBUG_MESSAGE
#define DEBUG_MSG	putstr
#else
#define DEBUG_MSG(...)
#endif

extern u32 __heap_start;
extern u32 __heap_end;

/**
 * The bootloader may need to use code decompression algorithm
 * on images stored in flash memory. The memory pointed to by
 * by this address is used as a temporary storage area for the
 * compressed code when it is loaded from the flash memory.
 *
 * For now, we just use the last 16MB of DRAM.
 */
static const u32 G_scratch_addr = DRAM_START_ADDR + (DRAM_SIZE - 0x1000000);

/*
 *
 */
#define output_part_tab(a,b,c,d,e)	\
{					\
	putstr(a);			\
	putstr(":");			\
	putdec(b);			\
	putstr(c);			\
	putdec(d);			\
	putstr(e);			\
}

void output_part(const char *s, const flpart_t *part)
{
	if ((part->img_len == 0) || (part->img_len == 0xffffffff))
		return;

	if (part->magic != FLPART_MAGIC) {
		putstr(s);
		putstr(": partition appears damaged...\r\n");
		return;
	}

	putstr(s);
	putstr(": 0x");
	puthex(part->crc32);
	putstr(" ");
	putdec(part->ver_num >> 16);
	putstr(".");
	putdec(part->ver_num & 0xffff);
	putstr(" \t(");
	if ((part->ver_date >> 16) == 0)
		putstr("0000");
	else
		putdec(part->ver_date >> 16);
	putstr("/");
	putdec((part->ver_date >> 8) & 0xff);
	putstr("/");
	putdec(part->ver_date & 0xff);
	putstr(")\t0x");
	puthex(part->mem_addr);
	putstr(" 0x");
	puthex(part->flag);
	putstr(" (");
	putdec(part->img_len);
	putstr(")\r\n");
}

void display_ptb_content(const flpart_table_t *ptb)
{
	int i;

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		output_part(g_part_str[i], &ptb->part[i]);
	}
}

void display_dsp_information(const flpart_table_t *ptb)
{
	u32 dsp_ver, dsp_date;
	u32 addr_date, addr_ver;
	dspfw_header_t *dsp_header =
		(dspfw_header_t *) ptb->part[PART_DSP].mem_addr;

#if ((CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A2S) || 	\
     (CHIP_REV == A2M) || (CHIP_REV == A2Q) || (CHIP_REV == A5S) || \
     (CHIP_REV == A5L))
	/* Print CODE version and date info */
	putstr("dsp code version: ");
	addr_ver = ((u32) dsp_header) +
				dsp_header->code.offset + DSP_VER_OFFSET;
	if (check_mem_access(addr_ver, addr_ver) == 0) {
		dsp_ver  = readl(addr_ver);
		putdec(dsp_ver >> 16);
		putstr(".");
		putdec(dsp_ver & 0xffff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	putstr("dsp code date: ");
	addr_date = ((u32) dsp_header) + dsp_header->code.offset + DSP_DATE_OFFSET;
	if (check_mem_access(addr_date, addr_date) == 0) {
		dsp_date = readl(addr_date);
		putdec(dsp_date >> 16);
		putstr("/");
		putdec((dsp_date & 0xff00) >> 8);
		putstr("/");
		putdec(dsp_date & 0xff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	/* Print MEMD version and date info */
	putstr("dsp memd version: ");
	addr_ver = ((u32) dsp_header) + dsp_header->memd.offset + DSP_VER_OFFSET;
	if (check_mem_access(addr_ver, addr_ver) == 0) {
		dsp_ver  = readl(addr_ver);
		putdec(dsp_ver >> 16);
		putstr(".");
		putdec(dsp_ver & 0xffff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	putstr("dsp memd date: ");
	addr_date = ((u32) dsp_header) + dsp_header->memd.offset + DSP_DATE_OFFSET;
	if (check_mem_access(addr_date, addr_date) == 0) {
		dsp_date = readl(addr_date);
		putdec(dsp_date >> 16);
		putstr("/");
		putdec((dsp_date & 0xff00) >> 8);
		putstr("/");
		putdec(dsp_date & 0xff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");
#else
	/* Print MAIN version and date info */
	putstr("dsp main version: ");
	addr_ver = ((u32) dsp_header) + dsp_header->main.offset + DSP_VER_OFFSET;
	if (check_mem_access(addr_ver, addr_ver) == 0) {
		dsp_ver  = readl(addr_ver);
		putdec(dsp_ver >> 16);
		putstr(".");
		putdec(dsp_ver & 0xffff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	putstr("dsp main date: ");
	addr_date = ((u32) dsp_header) + dsp_header->main.offset + DSP_DATE_OFFSET;
	if (check_mem_access(addr_date, addr_date) == 0) {
		dsp_date = readl(addr_date);
		putdec(dsp_date >> 16);
		putstr("/");
		putdec((dsp_date & 0xff00) >> 8);
		putstr("/");
		putdec(dsp_date & 0xff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	/* Print SUB0 version and date info */
	putstr("dsp sub0 version: ");
	addr_ver = ((u32) dsp_header) + dsp_header->sub0.offset + DSP_VER_OFFSET;
	if (check_mem_access(addr_ver, addr_ver) == 0) {
		dsp_ver  = readl(addr_ver);
		putdec(dsp_ver >> 16);
		putstr(".");
		putdec(dsp_ver & 0xffff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	putstr("dsp sub0 date: ");
	addr_date = ((u32) dsp_header) + dsp_header->sub0.offset + DSP_DATE_OFFSET;
	if (check_mem_access(addr_date, addr_date) == 0) {
		dsp_date = readl(addr_date);
		putdec(dsp_date >> 16);
		putstr("/");
		putdec((dsp_date & 0xff00) >> 8);
		putstr("/");
		putdec(dsp_date & 0xff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	/* Print SUB1 version and date info */
	putstr("dsp sub1 version: ");
	addr_ver = ((u32) dsp_header) + dsp_header->sub1.offset + DSP_VER_OFFSET;
	if (check_mem_access(addr_ver, addr_ver) == 0) {
		dsp_ver  = readl(addr_ver);
		putdec(dsp_ver >> 16);
		putstr(".");
		putdec(dsp_ver & 0xffff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");

	putstr("dsp sub1 date: ");
	addr_date = ((u32) dsp_header) + dsp_header->sub1.offset + DSP_DATE_OFFSET;
	if (check_mem_access(addr_date, addr_date) == 0) {
		dsp_date = readl(addr_date);
		putdec(dsp_date >> 16);
		putstr("/");
		putdec((dsp_date & 0xff00) >> 8);
		putstr("/");
		putdec(dsp_date & 0xff);
	} else {
		putstr("Can't read");
	}
	putstr("\r\n");
#endif
}

/*****************************************************************************/
/**
 * NAND code loading routine.
 */
int nand_load(int part_id, u32 mem_addr, u32 img_len, u32 flag)
{
	int rval = -1;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB)
		return rval;

	/* Load image from flash into memory */
	rval = nand_read_data((u8 *) mem_addr,
			      (u8 *) (flnand.sblk[part_id] * flnand.block_size),
			      img_len);
	if (rval == img_len) {
		rval = 0;
	} else {
		putstr("failed!\r\n");
		rval = -1;
	}
#endif
	return rval;
}

/**
 * ONENAND code loading routine.
 */
int onenand_load(int part_id, u32 mem_addr, u32 img_len, u32 flag)
{
	int rval = -1;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	u32 sblk = 0x0;
	u32 nblk = 0x0;
	int i;

	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB)
		return rval;

	for (i = PART_BST; i < TOTAL_FW_PARTS; i++) {
		if (i == part_id) {
			sblk = flnand.sblk[i];
			nblk = flnand.nblk[i];
			break;
		}
	}

	/* Load image from flash into memory */
	rval = onenand_read_data((u8 *)mem_addr,
				 (u8 *)(sblk * flnand.block_size),
				 img_len);
	if (rval == img_len) {
		rval = 0;
	} else {
		putstr("failed!\r\n");
		rval = -1;
	}
#endif
	return rval;
}


/**
 * NOR loading routine.
 */
int nor_load(int part_id, u32 mem_addr, u32 img_len, u32 flag)
{
	int rval = -1;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	u32 sector;
	u32 len;
	unsigned int offset;
	u32 ssec = 0x0, nsec = 0x0;

	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB)
		return -1;

	ssec = flnor.sblk[part_id];
	nsec = flnor.nblk[part_id];

	/* Load image from flash into memory */
	for (sector = ssec, offset = 0; sector < (ssec + nsec); sector++) {
		if ((img_len - offset) > flnor.sector_size) {
			len = flnor.sector_size;
		} else {
			len = img_len - offset;
			if ((len % 8) != 0)
				len += (8 - (len % 8));
		}

		rval = nor_read_sector(sector, (u8 *) mem_addr + offset, len);
		if (rval < 0) {
			putstr("failed!\r\n");
			goto done;
		}

		offset += len;
		if (offset >= img_len) {
			break;
		}
	}
done:

#endif

	return rval;
}

/**
 * SNOR loading routine.
 */
int snor_load(int part_id, u32 mem_addr, u32 img_len, u32 flag)
{
	int rval = -1;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	u32 sector;
	u32 len, addr;
	unsigned int offset;
	u32 ssec = 0x0, nsec = 0x0;

	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB)
		return -1;

	ssec = flnor.sblk[part_id];
	nsec = flnor.nblk[part_id];

	/* Load image from flash into memory */
	for (sector = ssec, offset = 0; sector < (ssec + nsec); sector++) {
		if ((img_len - offset) > flnor.sector_size) {
			len = flnor.sector_size;
		} else {
			len = img_len - offset;
			if ((len % 8) != 0)
				len += (8 - (len % 8));
		}

		addr = snor_get_addr(sector, 0);
		rval = snor_read(addr, len, (u8 *) mem_addr + offset);
		if (rval < 0) {
			putstr("failed!\r\n");
			goto done;
		}

		offset += len;
		if (offset >= img_len) {
			break;
		}
	}
done:

#endif

	return rval;
}

/**
 * sector media loading routine.
 */
int sm_load(int part_id, u32 mem_addr, u32 img_len, u32 flag)
{
	int rval = 0;
	smpart_t *smpart = NULL;

	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB)
		return -1;

	smpart = sm_get_part(part_id);

	/* Load image from sector media into memory */
	rval = sm_read_sector(smpart->ssec, (u8 *) mem_addr, img_len);
	if (rval < 0) {
		putstr("failed!\r\n");
	}

	return rval;
}

/*****************************************************************************/

/**
 * return value < 0:	error
 * 		= 0:	success
 * 		> 0:	decompress result size
 */
int fwprog_load_partition(int dev, int part_id, const flpart_t *part,
			  int verbose, int override)
{
	const char *s = NULL;

	int rval = 0x0;
	u32 mem_addr = 0x0;
	u32 img_len = 0x0;

	if (part_id >= TOTAL_FW_PARTS || part_id == PART_PTB) {
		putstr("illegal partition ID.\r\n");
		return -1;
	} else
		s = g_part_str[part_id];

	if (part->magic != FLPART_MAGIC) {
		putstr(s);
		putstr(" partition appears damaged... skipping\r\n");
		return -1;
	}

	if ((part->mem_addr < DRAM_START_ADDR) ||
		(part->mem_addr > (DRAM_START_ADDR + DRAM_SIZE - 1))){
		DEBUG_MSG(" wrong load address... skipping\r\n");
		return -4;
	}

	if ((part->img_len == 0) || (part->img_len == 0xffffffff)) {
		DEBUG_MSG(s);
		DEBUG_MSG(" image absent... skipping\r\n");
		return -2;
	}

	if ((part->flag & PART_NO_LOAD) & !override) {
		DEBUG_MSG(s);
		DEBUG_MSG(" has no-load flag set... skipping\r\n");
		return -3;
	}

	img_len = part->img_len;

	if (verbose) {
		putstr("loading ");
		putstr(s);
		putstr(" to 0x");
	}

	mem_addr = (part->flag & PART_COMPRESSED) ? (G_scratch_addr) :
						    (part->mem_addr);
	if (verbose) {
		puthex(mem_addr);
		putstr("\r\n");
	}

	switch (dev) {
	case BOOT_DEV_NAND:
		rval = nand_load(part_id, mem_addr, img_len, part->flag);
		break;
	case BOOT_DEV_ONENAND:
		rval = onenand_load(part_id, mem_addr, img_len, part->flag);
		break;
	case BOOT_DEV_NOR:
		rval = nor_load(part_id, mem_addr, img_len, part->flag);
		break;
	case BOOT_DEV_SNOR:
		rval = snor_load(part_id, mem_addr, img_len, part->flag);
		break;
	case BOOT_DEV_SM:
		rval = sm_load(part_id, mem_addr, img_len, part->flag);
		break;
	}

	if ((rval >= 0x0) &&
	    (part->flag & PART_COMPRESSED)) {
		rval = decompress(s, mem_addr, (mem_addr + img_len),
				  part->mem_addr,
				  (u32) &__heap_start, (u32) &__heap_end);
	}

	if (rval < 0) {
		putstr(s);
		putstr(" - load_partition() failed!\r\n");
	}

	return rval;
}

/*****************************************************************************/
#if defined(ENABLE_FLASH)

#if !defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE) || \
    !defined(CONFIG_SNOR_NONE) || !defined(CONFIG_ONENAND_NONE) || \
    defined(FIRMWARE_CONTAINER)
static int boot_from_dev(u32 dev_id, flpart_table_t *ptb,
			 const char *cmdline, int verbose)
{
	int rval = 0;
	u32 rmd_start = 0;
	u32 rmd_size = 0;
	void *jump_addr = NULL;

#if (!defined(LOAD_DSP_UCODE_FROM_PRI) &&	\
     !defined(LOAD_AUD_UCODE_FROM_PRI))
	if (!hotboot_valid || (hotboot_pattern & HOTBOOT_DSP)) {
		rval = fwprog_load_partition(g_part_dev[PART_DSP], PART_DSP,
					     &ptb->part[PART_DSP],
					     verbose, hotboot_valid);
		if (rval >= 0x0 && verbose)
			display_dsp_information(ptb);
	}
#endif

	if (!hotboot_valid || (hotboot_pattern & HOTBOOT_RMD)) {
		rval = fwprog_load_partition(g_part_dev[PART_RMD], PART_RMD,
					     &ptb->part[PART_RMD],
					     verbose, hotboot_valid);
		if (rval >= 0x0) {
			rmd_start = ptb->part[PART_RMD].mem_addr;
			if (rval == 0x0)
				rmd_size  = ptb->part[PART_RMD].img_len;
			else
				rmd_size  = rval;
		}
	}

	if (!hotboot_valid || (hotboot_pattern & HOTBOOT_ROM)) {
		rval = fwprog_load_partition(g_part_dev[PART_ROM], PART_ROM,
					     &ptb->part[PART_ROM],
					     verbose, hotboot_valid);
	}

	if (hotboot_valid && (hotboot_pattern & HOTBOOT_BAK)) {
		rval = fwprog_load_partition(g_part_dev[PART_BAK], PART_BAK,
					     &ptb->part[PART_BAK],
					     verbose, hotboot_valid);
		if (rval >= 0x0) {
			jump_addr = (void *) ptb->part[PART_BAK].mem_addr;
		}
	}

	if (hotboot_valid && (hotboot_pattern & HOTBOOT_SEC)) {
		rval = fwprog_load_partition(g_part_dev[PART_SEC], PART_SEC,
					     &ptb->part[PART_SEC],
					     verbose, hotboot_valid);
		if (rval >= 0x0) {
			jump_addr = (void *) ptb->part[PART_SEC].mem_addr;
		}
	}

	if (!hotboot_valid || (hotboot_pattern & HOTBOOT_PRI)) {
#if 0
		extern u32 *sr_get_password_ptr(void);
		extern void sr_suspend_load(int dev_id, flpart_table_t *ptb,
					int verbose);
		extern int is_rtc_self_refresh_mode(void);

		u32 password = 0;
		u32 *p_password = sr_get_password_ptr();

		DEBUG_MSG_SR_HEX("SR: address of password = 0x", (u32) p_password);
		DEBUG_MSG_SR_HEX("SR: data of password = 0x", (u32) *p_password);
		DEBUG_MSG_SR_HEX("SR: RTC_CURT_REG = 0x", (u32) (readl(RTC_CURT_REG)));

		if (p_password != NULL)
			password = *p_password;

		switch (password) {
		case SR_PASSWORD_SUSPEND:
			DEBUG_MSG_SR_STR("SR: suspend loading...\r\n");
			sr_suspend_load(g_part_dev[PART_PRI], ptb, verbose);
			break;
		case SR_PASSWORD_RESUME:
			/*
			 * To check self-refresh mode,
			 * 1. check RTC_CURT_REG bit 30
			 * 2. check dram magic pattern
			 */
			if (is_rtc_self_refresh_mode()) {
				DEBUG_MSG_SR_STR("SR: resume no loading...\r\n");
				rval = 0;
				break;
			} else {
				DEBUG_MSG_SR_STR("SR: resume but rtc not in self-refresh\r\n");
			}
		default:
			DEBUG_MSG_SR_STR("SR: normal loading...\r\n");
			rval = fwprog_load_partition(g_part_dev[PART_PRI], PART_PRI,
						&ptb->part[PART_PRI],
						verbose, hotboot_valid);
			break;
		}
#else
		rval = fwprog_load_partition(g_part_dev[PART_PRI], PART_PRI,
			&ptb->part[PART_PRI], verbose, hotboot_valid);
#endif

		if (rval >= 0x0) {
			jump_addr = (void *) (ptb->part[PART_PRI].mem_addr);
		}
	}
	if (jump_addr != 0x0) {
		if (verbose) {
			putstr("jumping to 0x");
			puthex((u32) jump_addr);
			putstr(" ...\r\n");
		}

#if (ETH_INSTANCES >= 1)
		bld_net_down();
#endif

		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			rmd_start, rmd_size, verbose);
		jump_to_kernel(jump_addr);
	}

	/* Special hack for USB download */
	if (hotboot_valid && (hotboot_pattern == HOTBOOT_USB)) {
		/* Must reset hotboot for normal boot after firmware programming */
		*((u32 *) &hotboot_pattern) = 0;
		*((u32 *) &hotboot_valid) = 0;

		usb_boot(USB_DL_DIRECT_USB);
		/* It won't return on success */
		rval = 0;
	}

	return rval;
}
#endif

#if !defined(CONFIG_NAND_NONE)

/**
 * Try to boot from NAND.
 */
static int nand_boot(const char *cmdline, int verbose)
{
	int i, rval = 0;
	flpart_table_t ptb;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Display PTB content */
	if (verbose) {
		display_ptb_content(&ptb);

		for (i = 0; i < TOTAL_FW_PARTS; i++) {
			output_part_tab(g_part_str[i],
					flnand.sblk[i],
					"/",
					flnand.nblk[i],
					"\r\n");
		}
		putstr("\r\n");
	}

	rval = boot_from_dev(BOOT_DEV_NAND, &ptb, cmdline, verbose);
done:

	return rval;
}

#endif  /* !CONFIG_NAND_NONE */

#if !defined(CONFIG_ONENAND_NONE)

/**
 * Try to boot from ONENAND.
 */
static int onenand_boot(const char *cmdline, int verbose)
{
	int i, rval = 0;
	flpart_table_t ptb;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Display PTB content */
	if (verbose) {
		display_ptb_content(&ptb);

		for (i = 0; i < TOTAL_FW_PARTS; i++) {
			output_part_tab(g_part_str[i],
					flnand.sblk[i],
					"/",
					flnand.nblk[i],
					"\r\n");
		}
		putstr("\r\n");
	}

	rval = boot_from_dev(BOOT_DEV_ONENAND, &ptb, cmdline, verbose);
done:

	return rval;
}

#endif  /* !CONFIG_ONENAND_NONE */

#if !defined(CONFIG_NOR_NONE)

/**
 * Try to boot from NOR.
 */
static int nor_boot(const char *cmdline, int verbose)
{
	int i, rval = 0;
	flpart_table_t ptb;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Display PTB content */
	if (verbose) {
		display_ptb_content(&ptb);
		for (i = 0; i < TOTAL_FW_PARTS; i++) {
			output_part_tab(g_part_str[i], flnor.sblk[i], "/",
				flnor.nblk[i], "\r\n");
		}
		putstr("\r\n");
	}

	rval = boot_from_dev(BOOT_DEV_NOR, &ptb, cmdline, verbose);
done:

	return rval;
}
#endif  /* !CONFIG_NOR_NONE */

#if !defined(CONFIG_SNOR_NONE)

/**
 * Try to boot from SNOR.
 */
static int snor_boot(const char *cmdline, int verbose)
{
	int i, rval = 0;
	flpart_table_t ptb;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Display PTB content */
	if (verbose) {
		display_ptb_content(&ptb);
		for (i = 0; i < TOTAL_FW_PARTS; i++) {
			output_part_tab(g_part_str[i], flnor.sblk[i], "/",
				flnor.nblk[i], "\r\n");
		}
		putstr("\r\n");
	}

	rval = boot_from_dev(BOOT_DEV_SNOR, &ptb, cmdline, verbose);
done:

	return rval;
}
#endif  /* !CONFIG_SNOR_NONE */

#endif	/* ENABLE_FLASH */

#if defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER)
/**
 * Try to boot from sector media.
 */
static int sm_boot(const char *cmdline, int verbose)
{
	int i, rval = 0;
	flpart_table_t ptb;
	smpart_t *smpart = NULL;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Display PTB content */
	if (verbose) {
		display_ptb_content(&ptb);

		for (i = PART_BST; i < TOTAL_FW_PARTS; i++) {
			smpart = sm_get_part(i);
			output_part_tab(g_part_str[i], smpart->ssec,
					"/", smpart->nsec, "\r\n");
		}

		putstr("\r\n");
	}

	rval = boot_from_dev(BOOT_DEV_SM, &ptb, cmdline, verbose);
done:

	return rval;
}
#endif

/*****************************************************************************/
#if defined(ENABLE_FLASH)

#if !defined(CONFIG_NAND_NONE)
/**
 * Boot the BIOS code in NAND.
 */
int nand_bios(const char *cmdline, int verbose)
{
	int rval = 0;
	flpart_table_t ptb;
	void *jump_addr = NULL;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	rval = fwprog_load_partition(BOOT_DEV_NAND, PART_PBA,
				     &ptb.part[PART_PBA], verbose, 0);

	if (rval < 0)
		goto done;

	jump_addr = (void *) ptb.part[PART_PBA].mem_addr;
	if (jump_addr != 0x0) {
		if (verbose) {
			putstr("jumping to 0x");
			puthex(ptb.part[PART_PBA].mem_addr);
			putstr(" ...\r\n");
		}

#if (ETH_INSTANCES >= 1)
		bld_net_down();
#endif

		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			0, 0, verbose);
		jump_to_kernel(jump_addr);
	}

done:
	return rval;
}
#endif  /* !CONFIG_NAND_NONE */

#if !defined(CONFIG_NOR_NONE)
/**
 * Boot to BIOS code in NOR.
 */
int nor_bios(const char *cmdline, int verbose)
{
	int rval = 0;
	flpart_table_t ptb;
	void *jump_addr = NULL;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	rval = fwprog_load_partition(BOOT_DEV_NOR, PART_PBA,
				     &ptb.pba, verbose, 0);

	if (rval < 0)
		goto done;

	jump_addr = (void *) ptb.pba.mem_addr;
	if (jump_addr != 0x0) {
		if (verbose) {
			putstr("jumping to 0x");
			puthex(ptb.pba.mem_addr);
			putstr(" ...\r\n");
		}

#if (ETH_INSTANCES >= 1)
		bld_net_down();
#endif

		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			0, 0, verbose);
		jump_to_kernel(jump_addr);
	}

done:
	return rval;
}
#endif  /* !CONFIG_NOR_NONE */

#if !defined(CONFIG_SNOR_NONE)
/**
 * Boot to BIOS code in SNOR.
 */
int snor_bios(const char *cmdline, int verbose)
{
	int rval = 0;
	flpart_table_t ptb;
	void *jump_addr = NULL;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	rval = fwprog_load_partition(BOOT_DEV_SNOR, PART_PBA,
				     &ptb.part[PART_PBA], verbose, 0);

	if (rval < 0)
		goto done;

	jump_addr = (void *) ptb.part[PART_PBA].mem_addr;
	if (jump_addr != 0x0) {
		if (verbose) {
			putstr("jumping to 0x");
			puthex(ptb.part[PART_PBA].mem_addr);
			putstr(" ...\r\n");
		}

#if (ETH_INSTANCES >= 1)
		bld_net_down();
#endif

		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			0, 0, verbose);
		jump_to_kernel(jump_addr);
	}

done:
	return rval;
}
#endif  /* !CONFIG_SNOR_NONE */

#endif	/* ENABLE_FLASH */

/**
 * Boot to BIOS code in sector media.
 */
int sm_bios(const char *cmdline, int verbose)
{
	int rval = -1;
	flpart_table_t ptb;
	void *jump_addr = NULL;

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	rval = fwprog_load_partition(BOOT_DEV_SM, PART_PBA,
				     &ptb.part[PART_PBA], verbose, 0);

	if (rval < 0)
		goto done;

	jump_addr = (void *) ptb.part[PART_PBA].mem_addr;
	if (jump_addr != 0x0) {
		if (verbose) {
			putstr("jumping to 0x");
			puthex(ptb.part[PART_PBA].mem_addr);
			putstr(" ...\r\n");
		}

#if (ETH_INSTANCES >= 1)
		bld_net_down();
#endif

		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			0, 0, verbose);
		jump_to_kernel(jump_addr);
	}

done:
	return rval;
}

/*****************************************************************************/
/**
 * Boot the primary firmware with ramdisk and DSP loading if necessary.
 */
int boot(const char *cmdline, int verbose)
{
	if (g_part_dev[PART_PRI] == BOOT_DEV_SM) {
#if defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER)
		return sm_boot(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if (g_part_dev[PART_PRI] == BOOT_DEV_NAND) {
		return nand_boot(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if (g_part_dev[PART_PRI] == BOOT_DEV_ONENAND) {
		return onenand_boot(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if (g_part_dev[PART_PRI] == BOOT_DEV_NOR) {
		return nor_boot(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (g_part_dev[PART_PRI] == BOOT_DEV_SNOR) {
		return snor_boot(cmdline, verbose);
#endif

	}

	return -1;
}

/**
 * Boot the BIOS code.
 */
int bios(const char *cmdline, int verbose)
{
	unsigned int boot_from = rct_boot_from();

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(ENABLE_SD)
		return sm_bios(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		return nand_bios(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		return nor_bios(cmdline, verbose);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if ((boot_from & BOOT_FROM_SNOR) == BOOT_FROM_SNOR) {
		return snor_bios(cmdline, verbose);
#endif

	}

	return -1;
}
