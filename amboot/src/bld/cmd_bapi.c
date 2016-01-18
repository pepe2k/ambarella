/**
 * src/bld/cmd_bapi.c
 *
 * History:
 *    2011/04/27 - [Anthony Ginger] created file
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <atag.h>
#include <fio/ftl_const.h>
#include <sm.h>
#include <sdmmc.h>

/* ==========================================================================*/
extern int aoss_init(u32, u32, u32, u32);
extern void aoss_outcoming(u32, u32, u32, u32);

/* ==========================================================================*/
static struct ambarella_bapi_s	*bapi_info = NULL;

/* ==========================================================================*/
static void cmd_bapi_info()
{
	if (bapi_info) {
		putstr("bapi_info: 0x");
		puthex((u32)bapi_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->magic);
		putstr("\r\nversion: 0x");
		puthex(bapi_info->version);
		putstr("\r\nsize: 0x");
		puthex(bapi_info->size);
		putstr("\r\ncrc: 0x");
		puthex(bapi_info->crc);
		putstr("\r\nmode: 0x");
		puthex(bapi_info->mode);
		putstr("\r\nblock_dev: 0x");
		puthex(bapi_info->block_dev);
		putstr("\r\nblock_start: 0x");
		puthex(bapi_info->block_start);
		putstr("\r\nblock_num: 0x");
		puthex(bapi_info->block_num);
		putstr("\r\n\r\n");

		putstr("reboot_info: 0x");
		puthex((u32)&bapi_info->reboot_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->reboot_info.magic);
		putstr("\r\nmode: 0x");
		puthex(bapi_info->reboot_info.mode);
		putstr("\r\nflag: 0x");
		puthex(bapi_info->reboot_info.flag);
		putstr("\r\n\r\n");

		putstr("fb_start: 0x");
		puthex(bapi_info->fb_start);
		putstr("\r\nfb_length: 0x");
		puthex(bapi_info->fb_length);
		putstr("\r\n\r\n");

		putstr("aoss_info: 0x");
		puthex((u32)&bapi_info->aoss_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->aoss_info.magic);
		putstr("\r\ntotal_pages: 0x");
		puthex(bapi_info->aoss_info.total_pages);
		putstr("\r\ncopy_pages: 0x");
		puthex(bapi_info->aoss_info.copy_pages);
		putstr("\r\npage_info: 0x");
		puthex(bapi_info->aoss_info.page_info);
		putstr("\r\n");
	}
}

static int cmd_bapi_aoss_check(int verbose)
{
	int					rval = 0;

	if (bapi_info == NULL) {
		if (verbose) {
			putstr("bapi_info NULL.\r\n");
		}
		rval = -1;
	}

	return rval;
}

static int cmd_bapi_aoss_check_hb(int verbose)
{
	int					rval = 0;

	rval = cmd_bapi_aoss_check(verbose);
	if (rval) {
		goto cmd_bapi_aoss_check_hb_exit;
	}

	if (bapi_info->aoss_info.magic != DEFAULT_BAPI_AOSS_MAGIC) {
		if (verbose) {
			putstr("Wrong aoss_maigc: ");
			puthex(bapi_info->aoss_info.magic);
			putstr("\r\n");
		}
		rval = -1;
	}

	if (verbose) {
		cmd_bapi_info();
	}

cmd_bapi_aoss_check_hb_exit:
	return rval;
}

/* ==========================================================================*/
int cmd_bapi_aoss_self_refresh_set(int verbose)
{
	int					rval = 0;
	u32					reboot_mode;

	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval) {
		rval = 0;
		goto cmd_bapi_aoss_self_refresh_set_exit;
	}

	if (cmd_bapi_reboot_get(0, &reboot_mode) == 0) {
		if (reboot_mode == AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH) {
			disable_interrupts();
			if ((amboot_bsp_self_refresh_enter) &&
				(pli_cache_region(
				amboot_bsp_self_refresh_enter, 4096) == 0)) {
				rval = amboot_bsp_self_refresh_enter();
			}
			enable_interrupts();
			cmd_bapi_reboot_set(0,
				AMBARELLA_BAPI_CMD_REBOOT_NORMAL);
			rval = 1;
		}
	}

cmd_bapi_aoss_self_refresh_set_exit:
	return rval;
}

int cmd_bapi_aoss_self_refresh_get(int verbose)
{
	int					rval = 0;
	u32					reboot_mode;

	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval) {
		rval = 0;
		goto cmd_bapi_aoss_self_refresh_get_exit;
	}

	if (cmd_bapi_reboot_get(0, &reboot_mode) == 0) {
		if (reboot_mode == AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH) {
			if (amboot_bsp_self_refresh_check_valid)
				rval = amboot_bsp_self_refresh_check_valid();
			cmd_bapi_reboot_set(0,
				AMBARELLA_BAPI_CMD_REBOOT_NORMAL);
		}
	}

cmd_bapi_aoss_self_refresh_get_exit:
	return rval;
}

/* ==========================================================================*/
static void cmd_bapi_aoss_boot_outcoming(boot_fn_t fn,
	int verbose, int argc, char *argv[])
{
	flpart_table_t				ptb;
	char					cmdline[FLDEV_CMD_LINE_SIZE];

	if (argc >= 1) {
		strfromargv(cmdline, sizeof(cmdline), argc, argv);
		fn(cmdline, verbose);
	} else {
		flprog_get_part_table(&ptb);
		fn(ptb.dev.cmdline, verbose);
	}
}

static void cmd_bapi_aoss_boot_fn(boot_fn_t fn,
	int verbose, int argc, char *argv[])
{
	int					aoss_status = 0;

	disable_interrupts();
	aoss_status = aoss_init((u32)bapi_info->aoss_info.fn_pri, 0, 0, 0);
	if (aoss_status != 0) {
		cmd_bapi_aoss_boot_incoming(verbose);
	} else {
		enable_interrupts();
		cmd_bapi_aoss_boot_outcoming(fn, verbose, argc, argv);
	}
}

/* ==========================================================================*/
static void cmd_bapi_aoss_info_memop(int verbose, u32 address, u32 size)
{
	if (verbose) {
		putstr("AOSS mem 0x");
		puthex(address);
		putstr(" size 0x");
		puthex(size);
		putstr("\r\n");
	}
}

static int cmd_bapi_aoss_switch(int verbose)
{
	int					rval = 0;

	disable_interrupts();
#if defined(AMBOOT_DEV_BOOT_CORTEX)
	cmd_cortex_resume((u32)aoss_outcoming,
		(u32)bapi_info->aoss_info.fn_pri, verbose);
#else
	_drain_write_buffer();
	_clean_flush_all_cache();
	_disable_dcache();
	aoss_outcoming((u32)bapi_info->aoss_info.fn_pri, 0, 0, 0);
	_enable_dcache();
#endif
	enable_interrupts();

	return rval;
}

/* ==========================================================================*/
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))

static void cmd_bapi_aoss_info_nandrwerr(int verbose,
	const char *name, u32 block, u32 page)
{
	if (verbose) {
		putstr(name);
		putstr(" <block ");
		putdec(block);
		putstr(", page ");
		putdec(page);
		putstr("> fail.\r\n");
	}
}

static int cmd_bapi_aoss_nand_read(u32 sblk, u32 eblk,
	u8 *raw_buf, u32 raw_size, int verbose)
{
	int					rval = 0;
	u32					block = 0;
	u32					page;
	u32					raw_offset;
	u32					pre_offset;

	if (raw_size % flnand.main_size) {
		rval = -100;
		goto cmd_bapi_aoss_nand_read_check;
	}

	raw_offset = 0;
	for (block = sblk; block < eblk; block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_ALL_BAD_BLOCK_FLAGS) {
			rval = 0;
			continue;
		}
		pre_offset = raw_offset;
		for (page = 0; page < flnand.pages_per_block; page++) {
			rval = nand_read_pages(block, page, 1,
				(raw_buf + raw_offset), NULL, 1);
			if (rval < 0) {
				cmd_bapi_aoss_info_nandrwerr(verbose,
					"read", block, page);
				break;
			}
			raw_offset += flnand.main_size;
			if (raw_offset >= raw_size)
				goto cmd_bapi_aoss_nand_read_check;
		}
		if (rval < 0) {
			raw_offset = pre_offset;
			putstr("NAND damaged by read, try next...\r\n");
			//break;
		}
	}

cmd_bapi_aoss_nand_read_check:
	if (block >= eblk) {
		rval = -1;
		if (verbose)
			putstr("Out of space.\r\n");
	}

	return (rval < 0) ? rval : block;
}

static int cmd_bapi_aoss_nand_write(u32 sblk, u32 eblk,
	u8 *raw_buf, u32 raw_size, int verbose, int dryrun)
{
	int					rval = 0;
	u32					block = 0;
	u32					page = 0;
	u32					raw_offset;
	u32					pre_offset;

	if (raw_size % flnand.main_size) {
		rval = -100;
		goto cmd_bapi_aoss_nand_write_check;
	}

	raw_offset = 0;
	for (block = sblk; block < eblk; block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_ALL_BAD_BLOCK_FLAGS) {
			rval = 0;
			continue;
		}
		rval = nand_erase_block(block);
		if (rval < 0) {
			cmd_bapi_aoss_info_nandrwerr(verbose,
				"erase", block, page);
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				break;
			else
				continue;
		}
		pre_offset = raw_offset;
		for (page = 0; page < flnand.pages_per_block; page++) {
			if (!dryrun) {
				rval = nand_prog_pages(block, page, 1,
					(raw_buf + raw_offset), NULL);
				if (rval < 0) {
					cmd_bapi_aoss_info_nandrwerr(verbose,
						"program", block, page);
					break;
				}
			}
			raw_offset += flnand.main_size;
			if (raw_offset >= raw_size)
				goto cmd_bapi_aoss_nand_write_check;
		}
		if (rval < 0) {
			raw_offset = pre_offset;
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				break;
			else
				continue;
		}
	}

cmd_bapi_aoss_nand_write_check:
	if (block >= eblk) {
		rval = -1;
		if (verbose)
			putstr("Out of space.\r\n");
	}

	return (rval < 0) ? rval : block;
}

static int cmd_bapi_aoss_nand_read_info(u32 sblk, u32 eblk,
	struct ambarella_bapi_aoss_s *aoss_info, int verbose)
{
	int					rval = 0;
	u32					block = 0;
	u32					page = 0;
	u32					pages_per_op;
	u32					page_offset;
	u32					page_index;
	u32					pre_page_index;
	u32					pre_page_offset;
	struct ambarella_bapi_aoss_page_info_s	*aoss_page_info;
	u32					total_size;

	page_index = 0;
	page_offset = 0;
	total_size = 0;
	aoss_page_info = (struct ambarella_bapi_aoss_page_info_s *)
		CORTEX_TO_ARM11(aoss_info->page_info);
	if (aoss_info->copy_pages == 0)
		goto cmd_bapi_aoss_nand_read_info_check;

	for (block = sblk; block < eblk; block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_ALL_BAD_BLOCK_FLAGS) {
			rval = 0;
			continue;
		}
		pre_page_offset = page_offset;
		pre_page_index = page_index;
		pages_per_op = (aoss_page_info[page_index].size -
			page_offset) / flnand.main_size;
		if (pages_per_op > flnand.pages_per_block)
			pages_per_op = flnand.pages_per_block;
		for (page = 0; page < flnand.pages_per_block;) {
			rval = nand_read_pages(block, page, pages_per_op,
				(u8 *)(aoss_page_info[page_index].src +
				page_offset), NULL, 1);
			if (rval < 0) {
				cmd_bapi_aoss_info_nandrwerr(verbose,
					"read", block, page);
				break;
			}
			page_offset += pages_per_op * flnand.main_size;
			page += pages_per_op;
			if (page_offset >= aoss_page_info[page_index].size) {
				cmd_bapi_aoss_info_memop(verbose,
					aoss_page_info[page_index].src,
					aoss_page_info[page_index].size);
				total_size += aoss_page_info[page_index].size;
				page_index++;
				if (page_index > aoss_info->copy_pages)
					goto cmd_bapi_aoss_nand_read_info_check;
				page_offset = 0;
				pages_per_op = aoss_page_info[page_index].size / flnand.main_size;
				if ((page + pages_per_op) > flnand.pages_per_block)
					pages_per_op = flnand.pages_per_block - page;
			}
		}
		if (rval < 0) {
			page_offset = pre_page_offset;
			page_index = pre_page_index;
			putstr("NAND damaged by read, try next...\r\n");
			//break;
		}
	}

cmd_bapi_aoss_nand_read_info_check:
	if (verbose) {
		putstr("Total Size:");
		putdec(total_size);
		putstr("Byte, ");
		putdec(total_size / 1024);
		putstr("KB, ");
		putdec(total_size / 1024 / 1024);
		putstr("MB.\r\n");
	}
	if (block >= eblk) {
		rval = -1;
		if (verbose)
			putstr("Out of space.\r\n");
	}

	return (rval < 0) ? rval : block;
}

static int cmd_bapi_aoss_nand_write_info(u32 sblk, u32 eblk,
	struct ambarella_bapi_aoss_s *aoss_info, int verbose)
{
	int					rval = 0;
	u32					block = 0;
	u32					page = 0;
	u32					pages_per_op;
	u32					page_offset;
	u32					page_index;
	u32					pre_page_index;
	u32					pre_page_offset;
	struct ambarella_bapi_aoss_page_info_s	*aoss_page_info;
	u32					total_size;

	page_index = 0;
	page_offset = 0;
	total_size = 0;
	aoss_page_info = (struct ambarella_bapi_aoss_page_info_s *)
		CORTEX_TO_ARM11(aoss_info->page_info);
	if (aoss_info->copy_pages == 0)
		goto cmd_bapi_aoss_nand_write_info_check;

	for (block = sblk; block < eblk; block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_ALL_BAD_BLOCK_FLAGS) {
			rval = 0;
			continue;
		}
		rval = nand_erase_block(block);
		if (rval < 0) {
			cmd_bapi_aoss_info_nandrwerr(verbose,
				"erase", block, page);
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				break;
			else
				continue;
		}
		pre_page_offset = page_offset;
		pre_page_index = page_index;
		pages_per_op = (aoss_page_info[page_index].size -
			page_offset) / flnand.main_size;
		if (pages_per_op > flnand.pages_per_block)
			pages_per_op = flnand.pages_per_block;
		for (page = 0; page < flnand.pages_per_block;) {
			rval = nand_prog_pages(block, page, pages_per_op,
				(u8 *)(aoss_page_info[page_index].dst +
				page_offset), NULL);
			if (rval < 0) {
				cmd_bapi_aoss_info_nandrwerr(verbose,
					"program", block, page);
				break;
			}
			page_offset += pages_per_op * flnand.main_size;
			page += pages_per_op;
			if (page_offset >= aoss_page_info[page_index].size) {
				cmd_bapi_aoss_info_memop(verbose,
					aoss_page_info[page_index].dst,
					aoss_page_info[page_index].size);
				total_size += aoss_page_info[page_index].size;
				page_index++;
				if (page_index > aoss_info->copy_pages)
					goto cmd_bapi_aoss_nand_write_info_check;
				page_offset = 0;
				pages_per_op = aoss_page_info[page_index].size / flnand.main_size;
				if ((page + pages_per_op) > flnand.pages_per_block)
					pages_per_op = flnand.pages_per_block - page;
			}
		}
		if (rval < 0) {
			page_offset = pre_page_offset;
			page_index = pre_page_index;
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				break;
			else
				continue;
		}
	}

cmd_bapi_aoss_nand_write_info_check:
	if (verbose) {
		putstr("Total Size:");
		putdec(total_size);
		putstr("Byte, ");
		putdec(total_size / 1024);
		putstr("KB, ");
		putdec(total_size / 1024 / 1024);
		putstr("MB.\r\n");
	}
	if (block >= eblk) {
		rval = -1;
		if (verbose)
			putstr("Out of space.\r\n");
	}

	return (rval < 0) ? rval : block;
}

static int cmd_bapi_aoss_nand_resume_hb(int verbose)
{
	int					rval = 0;
	u32					sblk;
	u32					eblk;
	u32					sblk_data;

	if ((flnand.sblk[DEFAULT_BAPI_AOSS_PART] == 0) ||
		(flnand.nblk[DEFAULT_BAPI_AOSS_PART] == 0)) {
		rval = -9;
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	}
	sblk = flnand.sblk[DEFAULT_BAPI_AOSS_PART];
	eblk = sblk + flnand.nblk[DEFAULT_BAPI_AOSS_PART];

	rval = cmd_bapi_aoss_nand_read(sblk, eblk,
		(u8 *)bapi_info, bapi_info->size, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	if (rval < sblk) {
		rval = -10;
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	}

	sblk_data = rval + DEFAULT_BAPI_NAND_OFFSET;
	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval)
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	if (bapi_info->block_start != sblk_data) {
		if (verbose) {
			putstr("Start error: 0x");
			puthex(sblk_data);
			putstr(" vs 0x");
			puthex(bapi_info->block_start);
			putstr("\r\n");
		}
		rval = -11;
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	}
	if (bapi_info->crc != crc32(&bapi_info->mode, (bapi_info->size - 16))) {
		if (verbose) {
			putstr("CRC error: 0x");
			puthex(bapi_info->crc);
			putstr(" vs 0x");
			puthex(crc32(&bapi_info->mode, (bapi_info->size - 16)));
			putstr("\r\n");
		}
		rval = -12;
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	}

	rval = cmd_bapi_aoss_nand_read_info(sblk_data, eblk,
		&bapi_info->aoss_info, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	if (rval < sblk_data) {
		rval = -13;
		goto cmd_bapi_aoss_nand_resume_hb_exit;
	}

	rval = 0;
cmd_bapi_aoss_nand_resume_hb_exit:
	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_nand_resume_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}

	return rval;
}

static int cmd_bapi_aoss_nand_save_hb(int verbose)
{
	int					rval = 0;
	u32					sblk;
	u32					eblk;
	u32					sblk_data;

	if ((flnand.sblk[DEFAULT_BAPI_AOSS_PART] == 0) ||
		(flnand.nblk[DEFAULT_BAPI_AOSS_PART] == 0)) {
		rval = -9;
		goto cmd_bapi_aoss_nand_save_hb_exit;
	}
	sblk = flnand.sblk[DEFAULT_BAPI_AOSS_PART];
	eblk = sblk + flnand.nblk[DEFAULT_BAPI_AOSS_PART];

	rval = cmd_bapi_aoss_nand_write(sblk, eblk, (u8 *)bapi_info,
		bapi_info->size, verbose, 1);
	if (rval < 0)
		goto cmd_bapi_aoss_nand_save_hb_exit;
	if (rval < sblk) {
		rval = -10;
		goto cmd_bapi_aoss_nand_save_hb_exit;
	}

	sblk_data = rval + DEFAULT_BAPI_NAND_OFFSET;
	rval = cmd_bapi_aoss_nand_write_info(sblk_data, eblk,
		&bapi_info->aoss_info, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_nand_save_hb_exit;
	if (rval < sblk_data) {
		rval = -11;
		goto cmd_bapi_aoss_nand_save_hb_exit;
	}

	bapi_info->block_start = sblk_data;
	bapi_info->block_num = (rval - sblk_data);
	bapi_info->crc = crc32(&bapi_info->mode, (bapi_info->size - 16));
	rval = cmd_bapi_aoss_nand_write(sblk, eblk, (u8 *)bapi_info,
		bapi_info->size, verbose, 0);
	if (rval < 0)
		goto cmd_bapi_aoss_nand_save_hb_exit;
	if ((rval + DEFAULT_BAPI_NAND_OFFSET) != sblk_data) {
		cmd_bapi_aoss_nand_write(sblk, eblk, (u8 *)bapi_info,
			bapi_info->size, verbose, 1);
		rval = -12;
		goto cmd_bapi_aoss_nand_save_hb_exit;
	}

	rval = 0;
cmd_bapi_aoss_nand_save_hb_exit:
	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_nand_save_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}

	return rval;
}
#endif

/* ==========================================================================*/
#if (defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER))

static void cmd_bapi_aoss_info_smrwerr(int verbose,
	const char *name, u32 sec, u32 count)
{
	if (verbose) {
		putstr(name);
		putstr(" <sec ");
		putdec(sec);
		putstr(", count ");
		putdec(count);
		putstr("> fail.\r\n");
	}
}

static int cmd_bapi_aoss_sm_read(u32 ssec, u32 esec,
	u8 *raw_buf, u32 raw_size, int verbose)
{
	int					rval = 0;
	u32					sec_count;

	if (raw_size % SM_SECTOR_SIZE) {
		if (verbose) {
			putstr("Wrong size: ");
			putdec(raw_size);
			putstr("\r\n");
		}
		rval = -100;
		goto cmd_bapi_aoss_sm_read_check;
	}

	sec_count = raw_size / SM_SECTOR_SIZE;
	if ((ssec + sec_count) >= esec) {
		if (verbose) {
			putstr("Out of space: 0x");
			puthex(ssec);
			putstr(" + 0x");
			puthex(sec_count);
			putstr(" >= 0x");
			puthex(esec);
			putstr("\r\n");
		}
		rval = -9;
		goto cmd_bapi_aoss_sm_read_check;
	}

	rval = sdmmc_read_sector(ssec, sec_count, (unsigned int *)raw_buf);
	if (rval < 0) {
		cmd_bapi_aoss_info_smrwerr(verbose,
			__func__, ssec, sec_count);
	}

cmd_bapi_aoss_sm_read_check:
	return (rval < 0) ? rval : (ssec + sec_count);
}

static int cmd_bapi_aoss_sm_write(u32 ssec, u32 esec,
	u8 *raw_buf, u32 raw_size, int verbose, int dryrun)
{
	int					rval = 0;
	u32					sec_count;

	if (raw_size % SM_SECTOR_SIZE) {
		if (verbose) {
			putstr("Wrong size: ");
			putdec(raw_size);
			putstr("\r\n");
		}
		rval = -100;
		goto cmd_bapi_aoss_sm_write_check;
	}

	sec_count = raw_size / SM_SECTOR_SIZE;
	if ((ssec + sec_count) >= esec) {
		if (verbose) {
			putstr("Out of space: 0x");
			puthex(ssec);
			putstr(" + 0x");
			puthex(sec_count);
			putstr(" >= 0x");
			puthex(esec);
			putstr("\r\n");
		}
		rval = -9;
		goto cmd_bapi_aoss_sm_write_check;
	}

	if (dryrun) {
		rval = sdmmc_erase_sector(ssec, sec_count);
		if (rval < 0) {
			cmd_bapi_aoss_info_smrwerr(verbose,
				"sm_erase", ssec, sec_count);
		}
	} else {
		rval = sdmmc_write_sector(ssec, sec_count,
			(unsigned int *)raw_buf);
		if (rval < 0) {
			cmd_bapi_aoss_info_smrwerr(verbose,
				"sm_write", ssec, sec_count);
		}
	}

cmd_bapi_aoss_sm_write_check:
	return (rval < 0) ? rval : (ssec + sec_count);
}

static int cmd_bapi_aoss_sm_read_info(u32 ssec, u32 esec,
	struct ambarella_bapi_aoss_s *aoss_info, int verbose)
{
	int					rval = 0;
	u32					sec;
	u32					page_index;
	u32					total_size;
	u32					sec_count;
	struct ambarella_bapi_aoss_page_info_s	*aoss_page_info;

	sec = ssec;
	total_size = 0;
	aoss_page_info = (struct ambarella_bapi_aoss_page_info_s *)
		CORTEX_TO_ARM11(aoss_info->page_info);
	if (aoss_info->copy_pages == 0)
		goto cmd_bapi_aoss_sm_read_info_check;

	for (page_index = 0; page_index <= aoss_info->copy_pages;
		page_index++) {
		sec_count = aoss_page_info[page_index].size / SM_SECTOR_SIZE;
		if ((sec + sec_count) >= esec) {
			if (verbose) {
				putstr("Out of space: 0x");
				puthex(ssec);
				putstr(" + 0x");
				puthex(sec_count);
				putstr(" >= 0x");
				puthex(esec);
				putstr("\r\n");
			}
			rval = -9;
			goto cmd_bapi_aoss_sm_read_info_check;
		}
		rval = sdmmc_read_sector(sec, sec_count,
			(unsigned int *)aoss_page_info[page_index].src);
		if (rval < 0) {
			cmd_bapi_aoss_info_smrwerr(verbose,
				__func__, sec, sec_count);
			goto cmd_bapi_aoss_sm_read_info_check;
		}
		sec += sec_count;
		total_size += aoss_page_info[page_index].size;
		cmd_bapi_aoss_info_memop(verbose,
			aoss_page_info[page_index].src,
			aoss_page_info[page_index].size);
	}

cmd_bapi_aoss_sm_read_info_check:
	if (verbose) {
		putstr("Total Size:");
		putdec(total_size);
		putstr("Byte, ");
		putdec(total_size / 1024);
		putstr("KB, ");
		putdec(total_size / 1024 / 1024);
		putstr("MB.\r\n");
	}

	return (rval < 0) ? rval : sec;
}

static int cmd_bapi_aoss_sm_write_info(u32 ssec, u32 esec,
	struct ambarella_bapi_aoss_s *aoss_info, int verbose)
{
	int					rval = 0;
	u32					sec;
	u32					page_index;
	u32					total_size;
	u32					sec_count;
	struct ambarella_bapi_aoss_page_info_s	*aoss_page_info;

	sec = ssec;
	total_size = 0;
	aoss_page_info = (struct ambarella_bapi_aoss_page_info_s *)
		CORTEX_TO_ARM11(aoss_info->page_info);
	if (aoss_info->copy_pages == 0)
		goto cmd_bapi_aoss_sm_write_info_check;

	for (page_index = 0; page_index <= aoss_info->copy_pages;
		page_index++) {
		sec_count = aoss_page_info[page_index].size / SM_SECTOR_SIZE;
		if ((sec + sec_count) >= esec) {
			if (verbose) {
				putstr("Out of space: 0x");
				puthex(ssec);
				putstr(" + 0x");
				puthex(sec_count);
				putstr(" >= 0x");
				puthex(esec);
				putstr("\r\n");
			}
			rval = -9;
			goto cmd_bapi_aoss_sm_write_info_check;
		}
		rval = sdmmc_write_sector(sec, sec_count,
			(unsigned int *)aoss_page_info[page_index].dst);
		if (rval < 0) {
			cmd_bapi_aoss_info_smrwerr(verbose,
				__func__, sec, sec_count);
			goto cmd_bapi_aoss_sm_write_info_check;
		}
		sec += sec_count;
		total_size += aoss_page_info[page_index].size;
		cmd_bapi_aoss_info_memop(verbose,
			aoss_page_info[page_index].dst,
			aoss_page_info[page_index].size);
	}

cmd_bapi_aoss_sm_write_info_check:
	if (verbose) {
		putstr("Total Size:");
		putdec(total_size);
		putstr("Byte, ");
		putdec(total_size / 1024);
		putstr("KB, ");
		putdec(total_size / 1024 / 1024);
		putstr("MB.\r\n");
	}

	return (rval < 0) ? rval : sec;
}

static int cmd_bapi_aoss_sm_resume_hb(int verbose)
{
	int					rval = 0;
	u32					ssec;
	u32					esec;
	u32					ssec_data;
	smpart_t				*aoss_part;

	aoss_part = sm_get_part(DEFAULT_BAPI_AOSS_PART);
	if ((aoss_part == NULL) || (aoss_part->ssec == 0) ||
		(aoss_part->nsec == 0)) {
		rval = -9;
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	}
	ssec = aoss_part->ssec;
	esec = ssec + aoss_part->nsec;

	rval = cmd_bapi_aoss_sm_read(ssec, esec,
		(u8 *)bapi_info, bapi_info->size, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	if (rval < ssec) {
		rval = -10;
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	}

	ssec_data = rval + DEFAULT_BAPI_SM_OFFSET;
	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval)
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	if (bapi_info->block_start != ssec_data) {
		if (verbose) {
			putstr("Start error: 0x");
			puthex(ssec_data);
			putstr(" vs 0x");
			puthex(bapi_info->block_start);
			putstr("\r\n");
		}
		rval = -11;
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	}
	if (bapi_info->crc != crc32(&bapi_info->mode, (bapi_info->size - 16))) {
		if (verbose) {
			putstr("CRC error: 0x");
			puthex(bapi_info->crc);
			putstr(" vs 0x");
			puthex(crc32(&bapi_info->mode, (bapi_info->size - 16)));
			putstr("\r\n");
		}
		rval = -12;
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	}

	rval = cmd_bapi_aoss_sm_read_info(ssec_data, esec,
		&bapi_info->aoss_info, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	if (rval < ssec_data) {
		rval = -13;
		goto cmd_bapi_aoss_sm_resume_hb_exit;
	}

	rval = 0;
cmd_bapi_aoss_sm_resume_hb_exit:
	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_sm_resume_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}

	return rval;
}

static int cmd_bapi_aoss_sm_save_hb(int verbose)
{
	int					rval = 0;
	u32					ssec;
	u32					esec;
	u32					ssec_data;
	smpart_t				*aoss_part;

	aoss_part = sm_get_part(DEFAULT_BAPI_AOSS_PART);
	if ((aoss_part == NULL) || (aoss_part->ssec == 0) ||
		(aoss_part->nsec == 0)) {
		rval = -9;
		goto cmd_bapi_aoss_sm_save_hb_exit;
	}
	ssec = aoss_part->ssec;
	esec = ssec + aoss_part->nsec;

	rval = cmd_bapi_aoss_sm_write(ssec, esec, (u8 *)bapi_info,
		bapi_info->size, verbose, 1);
	if (rval < 0)
		goto cmd_bapi_aoss_sm_save_hb_exit;
	if (rval < ssec) {
		rval = -10;
		goto cmd_bapi_aoss_sm_save_hb_exit;
	}

	ssec_data = rval + DEFAULT_BAPI_SM_OFFSET;
	rval = cmd_bapi_aoss_sm_write_info(ssec_data, esec,
		&bapi_info->aoss_info, verbose);
	if (rval < 0)
		goto cmd_bapi_aoss_sm_save_hb_exit;
	if (rval < ssec_data) {
		rval = -11;
		goto cmd_bapi_aoss_sm_save_hb_exit;
	}

	bapi_info->block_start = ssec_data;
	bapi_info->block_num = (rval - ssec_data);
	bapi_info->crc = crc32(&bapi_info->mode, (bapi_info->size - 16));
	rval = cmd_bapi_aoss_sm_write(ssec, esec, (u8 *)bapi_info,
		bapi_info->size, verbose, 0);
	if (rval < 0)
		goto cmd_bapi_aoss_sm_save_hb_exit;
	if ((rval + DEFAULT_BAPI_SM_OFFSET) != ssec_data) {
		cmd_bapi_aoss_sm_write(ssec, esec, (u8 *)bapi_info,
			bapi_info->size, verbose, 1);
		rval = -12;
		goto cmd_bapi_aoss_sm_save_hb_exit;
	}

	rval = 0;
cmd_bapi_aoss_sm_save_hb_exit:
	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_sm_save_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}

	return rval;
}
#endif

/* ==========================================================================*/
static int cmd_bapi_aoss_resume_hb(int verbose)
{
	int					rval = -101;
	u32					block_dev;

	block_dev = get_part_dev(DEFAULT_BAPI_AOSS_PART);

	if (block_dev == BOOT_DEV_NAND) {
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
		rval = cmd_bapi_aoss_nand_resume_hb(verbose);
#endif
	} else if (block_dev == BOOT_DEV_SM) {
#if (defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER))
		rval = cmd_bapi_aoss_sm_resume_hb(verbose);
#endif
	}

	if (!rval) {
		rval = cmd_bapi_aoss_switch(verbose);
	}

	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_resume_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}

	return rval;
}

static int cmd_bapi_aoss_save_hb(int verbose)
{
	int					rval;

	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval)
		goto cmd_bapi_aoss_save_hb_exit;

	bapi_info->block_dev = get_part_dev(DEFAULT_BAPI_AOSS_PART);

	rval = -101;
	if (bapi_info->block_dev == BOOT_DEV_NAND) {
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
		rval = cmd_bapi_aoss_nand_save_hb(verbose);
#endif
	} else if (bapi_info->block_dev == BOOT_DEV_SM) {
#if (defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER))
		sm_dev_init(FIRMWARE_CONTAINER);
		rval = cmd_bapi_aoss_sm_save_hb(verbose);
#endif
	}

cmd_bapi_aoss_save_hb_exit:
	return rval;
}

static int cmd_bapi_aoss_return_hb(int verbose)
{
	int					rval = 0;
	int					i;
	struct ambarella_bapi_aoss_page_info_s	*tmp_page_info;

	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval)
		goto cmd_bapi_aoss_return_hb_exit;

	tmp_page_info = (struct ambarella_bapi_aoss_page_info_s *)
		CORTEX_TO_ARM11(bapi_info->aoss_info.page_info);
	if (verbose) {
		putstr("page_info: ");
		puthex((u32)tmp_page_info);
		putstr("\r\n");
		putstr("aoss_outcoming: ");
		puthex((u32)aoss_outcoming);
		putstr("\r\n");
		putstr("copy_pages: ");
		puthex(bapi_info->aoss_info.copy_pages);
		putstr("\r\n");
	}
	if (bapi_info->aoss_info.copy_pages == 0)
		goto cmd_bapi_aoss_return_hb_switch;

	for (i = 0; i <= bapi_info->aoss_info.copy_pages; i++) {
		flush_d_cache((void *)tmp_page_info[i].dst,
			tmp_page_info[i].size);
		memcpy((void *)tmp_page_info[i].src,
			(void *)tmp_page_info[i].dst, tmp_page_info[i].size);
		clean_d_cache((void *)tmp_page_info[i].src,
			tmp_page_info[i].size);
		if (verbose) {
			putstr("copy from ");
			puthex(tmp_page_info[i].dst);
			putstr(" to ");
			puthex(tmp_page_info[i].src);
			putstr(" size ");
			puthex(tmp_page_info[i].size);
			putstr("\r\n");
		}
	}

cmd_bapi_aoss_return_hb_switch:
	rval = cmd_bapi_aoss_switch(verbose);

cmd_bapi_aoss_return_hb_exit:
	if ((rval < 0) && verbose) {
		putstr("cmd_bapi_aoss_return_hb: error ");
		puthex(rval);
		putstr("\r\n");
	}
	return rval;
}

/* ==========================================================================*/
int cmd_bapi_aoss_hibernate_check(int verbose)
{
	int					rval = 0;
	u32					reboot_mode;

	rval = cmd_bapi_aoss_check_hb(verbose);
	if (rval) {
		rval = 0;
		goto cmd_bapi_aoss_hibernate_check_exit;
	}

	if (cmd_bapi_reboot_get(0, &reboot_mode) == 0) {
		if (reboot_mode == AMBARELLA_BAPI_CMD_REBOOT_HIBERNATE) {
#if defined(CONFIG_AMBOOT_BAPI_AUTO_SAVE)
			rval = cmd_bapi_aoss_save_hb(verbose);
			if (!rval) {
				if (amboot_bsp_power_off) {
					amboot_bsp_power_off();
				} else {
					rct_reset_chip();
				}
			}
#endif
			cmd_bapi_reboot_set(0,
				AMBARELLA_BAPI_CMD_REBOOT_NORMAL);
			rval = 1;
		}
	}

cmd_bapi_aoss_hibernate_check_exit:
	return rval;
}

/* ==========================================================================*/
void ambarella_bapi_atag_entry(int verbose)
{
	if (AMBARELLA_PPM_SIZE == 0) {
		params->hdr.tag = ATAG_AMBARELLA_REVMEM;
		params->hdr.size = tag_size(tag_mem32);
		params->u.mem.start = ARM11_TO_CORTEX(DRAM_START_ADDR);
		params->u.mem.size = SIZE_1MB;
		params = tag_next(params);
		if (verbose) {
			putstr("rev mem: 0x");
			puthex(ARM11_TO_CORTEX(DRAM_START_ADDR));
			putstr(" size: 0x");
			puthex(SIZE_1MB);
			putstr("\r\n");
		}
	}

	params->hdr.tag = ATAG_AMBARELLA_BAPI;
	params->hdr.size = tag_size(tag_mem32);
	params->u.mem.start = ARM11_TO_CORTEX((u32)bapi_info);
	params->u.mem.size = DEFAULT_BAPI_TAG_MAGIC;
	params = tag_next(params);
}

#ifdef CONFIG_KERNEL_DUAL_CPU
//for arm kernel in booting dual linux kernel
void ambarella_bapi_arm_atag_entry(int verbose)
{
	if (AMBARELLA_PPM_SIZE == 0) {
		params->hdr.tag = ATAG_AMBARELLA_REVMEM;
		params->hdr.size = tag_size(tag_mem32);
		params->u.mem.start = DRAM_START_ADDR;
		params->u.mem.size = SIZE_1MB;
		params = tag_next(params);
		if (verbose) {
			putstr("rev mem: 0x");
			puthex(DRAM_START_ADDR);
			putstr(" size: 0x");
			puthex(SIZE_1MB);
			putstr("\r\n");
		}
	}

	params->hdr.tag = ATAG_AMBARELLA_BAPI;
	params->hdr.size = tag_size(tag_mem32);
	params->u.mem.start = ((u32)bapi_info);
	params->u.mem.size = DEFAULT_BAPI_TAG_MAGIC;
	params = tag_next(params);
}
#endif

int cmd_bapi_init(int verbose)
{
	int					rval = 0;
	int					ppm_size;
	int					fb_size;
	struct ambarella_bapi_aoss_s		*aoss_info = NULL;
	u32					aoss_size;

	bapi_info = (struct ambarella_bapi_s *)
		(DRAM_START_ADDR + DEFAULT_BAPI_OFFSET);
	if (cmd_bapi_aoss_self_refresh_get(verbose)) {
		if (amboot_bsp_self_refresh_exit)
			rval = amboot_bsp_self_refresh_exit();
		if (!rval)
			rval = cmd_bapi_aoss_switch(verbose);
	}
	rval = 0;

	if (AMBARELLA_PPM_SIZE <= SIZE_1MB) {
		ppm_size = SIZE_1MB;
		fb_size = 0;
	} else {
		ppm_size = SIZE_1MB;
		fb_size = (AMBARELLA_PPM_SIZE - SIZE_1MB);
	}
	bapi_info->magic = DEFAULT_BAPI_MAGIC;
	bapi_info->version = DEFAULT_BAPI_VERSION;
	bapi_info->size = (ppm_size - DEFAULT_BAPI_OFFSET);
	if(bapi_info->size <= DEFAULT_BAPI_SIZE) {
		memzero(bapi_info, sizeof(struct ambarella_bapi_s));
		rval = -1;
		goto cmd_bapi_init_exit;
	}

	bapi_info->reboot_info.flag = AMBARELLA_BAPI_REBOOT_HIBERNATE;
	if (amboot_bsp_self_refresh_enter) {
		bapi_info->reboot_info.flag |=
			AMBARELLA_BAPI_REBOOT_SELFREFERESH;
	}

	bapi_info->fb_start = DRAM_START_ADDR + ppm_size;
	bapi_info->fb_length = fb_size;
	memzero(&bapi_info->fb0_info, sizeof(struct ambarella_bapi_fb_info_s));
	memzero(&bapi_info->fb1_info, sizeof(struct ambarella_bapi_fb_info_s));
#if defined(CONFIG_AMBOOT_BAPI_ZERO_FB)
	memzero((void *)bapi_info->fb_start, bapi_info->fb_length);
#endif

	aoss_info = &bapi_info->aoss_info;
	aoss_size = bapi_info->size - DEFAULT_BAPI_SIZE;
	memzero(aoss_info, (aoss_size + DEFAULT_BAPI_AOSS_SIZE));
	aoss_info->page_info = ARM11_TO_CORTEX((u32)aoss_info +
		DEFAULT_BAPI_AOSS_SIZE);
	aoss_info->total_pages = aoss_size /
		sizeof(struct ambarella_bapi_aoss_page_info_s);
	if (verbose) {
		cmd_bapi_info();
	}

cmd_bapi_init_exit:
	return rval;
}

void cmd_bapi_aoss_resume_boot(boot_fn_t fn,
	int verbose, int argc, char *argv[])
{
	int					rval = 0;

	memcpy((void *)(DRAM_START_ADDR + DEFAULT_BAPI_BACKUP_OFFSET),
		(void *)bapi_info, DEFAULT_BAPI_SIZE);
	rval = cmd_bapi_aoss_resume_hb(verbose);
	if (rval) {
		memcpy((void *)bapi_info,
			(void *)(DRAM_START_ADDR + DEFAULT_BAPI_BACKUP_OFFSET),
			DEFAULT_BAPI_SIZE);
		cmd_bapi_aoss_boot_fn(fn, verbose, argc, argv);
	}
}

void cmd_bapi_aoss_boot_incoming(int verbose)
{
	int					rval = 0;
	rval = cmd_bapi_aoss_self_refresh_set(verbose);
	if (rval == 1) {
		cmd_bapi_aoss_switch(verbose);
	}
	amboot_basic_reinit(verbose);
	enable_interrupts();
	rval = cmd_bapi_aoss_hibernate_check(verbose);
}

/* ==========================================================================*/
int cmd_bapi_reboot_set(int verbose, u32 mode)
{
	int					rval = 0;

	rval = cmd_bapi_aoss_check(verbose);
	if (rval) {
		goto cmd_bapi_reboot_set_exit;
	}

	bapi_info->reboot_info.magic = DEFAULT_BAPI_REBOOT_MAGIC;
	bapi_info->reboot_info.mode = mode;

	if (verbose) {
		putstr("mode: ");
		puthex(mode);
		putstr("\r\n");
	}

cmd_bapi_reboot_set_exit:
	return rval;
}

int cmd_bapi_reboot_get(int verbose, u32 *mode)
{
	int					rval = 0;

	rval = cmd_bapi_aoss_check(verbose);
	if (rval) {
		goto cmd_bapi_reboot_get_exit;
	}

	if (bapi_info->reboot_info.magic == DEFAULT_BAPI_REBOOT_MAGIC) {
		*mode = bapi_info->reboot_info.mode;
	} else {
		*mode = AMBARELLA_BAPI_CMD_REBOOT_NORMAL;
		rval = -1;
	}

	if (verbose) {
		putstr("mode: 0x");
		puthex(*mode);
		putstr("\r\n");
		putstr("mode add: 0x");
		puthex((u32)&bapi_info->reboot_info.mode);
		putstr(" val: 0x");
		puthex(bapi_info->reboot_info.mode);
		putstr("\r\n");
	}

cmd_bapi_reboot_get_exit:
	return rval;
}

/* ==========================================================================*/
int cmd_bapi_set_fb_info(int verbose, u32 id, int xres, int yres,
	int xvirtual, int yvirtual, int format, u32 bits_per_pixel)
{
	int					rval = -1;
	u32					length;
	u32					max_length;

	rval = cmd_bapi_aoss_check(verbose);
	if (rval) {
		goto cmd_bapi_set_fb_info_exit;
	}

	length = xvirtual * yvirtual * bits_per_pixel / 8;
	if (id == 0) {
		max_length =
			(length + MEM_SIZE_1MB_MASK) & (~MEM_SIZE_1MB_MASK);
		if (bapi_info->fb_length >= max_length) {
			bapi_info->fb0_info.xres = xres;
			bapi_info->fb0_info.yres = yres;
			bapi_info->fb0_info.xvirtual = xvirtual;
			bapi_info->fb0_info.yvirtual = yvirtual;
			bapi_info->fb0_info.format = format;
			bapi_info->fb0_info.bits_per_pixel = bits_per_pixel;
			bapi_info->fb0_info.fb_start =
				ARM11_TO_CORTEX(bapi_info->fb_start);
			bapi_info->fb0_info.fb_length = max_length;
			rval = 0;
		}
	} else if (id == 1) {
		max_length = length;
		max_length += bapi_info->fb0_info.fb_length;
		if (bapi_info->fb_length >= max_length) {
			bapi_info->fb1_info.fb_start =
				bapi_info->fb0_info.fb_start +
				bapi_info->fb0_info.fb_length;
			max_length = bapi_info->fb_length -
				bapi_info->fb0_info.fb_length;
			bapi_info->fb1_info.xres = xres;
			bapi_info->fb1_info.yres = yres;
			bapi_info->fb1_info.xvirtual = xvirtual;
			bapi_info->fb1_info.yvirtual = yvirtual;
			bapi_info->fb1_info.format = format;
			bapi_info->fb1_info.bits_per_pixel = bits_per_pixel;
			bapi_info->fb1_info.fb_length = max_length;
			rval = 0;
		}
	}

cmd_bapi_set_fb_info_exit:
	return rval;
}

u32 cmd_bapi_get_splash_fb_start(void)
{
	if (cmd_bapi_aoss_check(0))
		return 0;

	return bapi_info->fb_start;
}

u32 cmd_bapi_get_splash_fb_length(void)
{
	if (cmd_bapi_aoss_check(0))
		return 0;

	return bapi_info->fb_length;
}

/* ==========================================================================*/
static int cmd_bapi(int argc, char *argv[])
{
	int					rval = 0;
	int					valid_cmd = 0;
	u32					tmp;

	if (argc > 1) {
		if (strcmp(argv[1], "init") == 0) {
			cmd_bapi_init(1);
			valid_cmd = 1;
		} else
		if (strcmp(argv[1], "info") == 0) {
			cmd_bapi_info();
			valid_cmd = 1;
		} else
		if (strcmp(argv[1], "fboot") == 0) {
			cmd_bapi_aoss_boot_fn(boot, 1, (argc - 2), &argv[2]);
			valid_cmd = 1;
#if (ETH_INSTANCES >= 1)
		} else
		if (strcmp(argv[1], "nboot") == 0) {
			cmd_bapi_aoss_boot_fn(netboot, 1, (argc - 2), &argv[2]);
			valid_cmd = 1;
#endif
#if defined(AMBOOT_DEV_BOOT_CORTEX)
		} else
		if (strcmp(argv[1], "fbootc") == 0) {
			cmd_bapi_aoss_boot_fn(cmd_cortex_boot,
				1, (argc - 2), &argv[2]);
			valid_cmd = 1;
		} else
		if (strcmp(argv[1], "nbootc") == 0) {
			cmd_bapi_aoss_boot_fn(cmd_cortex_boot_net,
				1, (argc - 2), &argv[2]);
			valid_cmd = 1;
#endif
		} else
		if (strcmp(argv[1], "hb") == 0) {
			if (argc > 2) {
				if (strcmp(argv[2], "save") == 0) {
					cmd_bapi_aoss_save_hb(1);
					valid_cmd = 1;
				} else
				if (strcmp(argv[2], "resume") == 0) {
					cmd_bapi_aoss_resume_hb(1);
					valid_cmd = 1;
				} else
				if (strcmp(argv[2], "return") == 0) {
					cmd_bapi_aoss_return_hb(1);
					valid_cmd = 1;
				}
			}
		} else
		if (strcmp(argv[1], "rb") == 0) {
			if ((argc > 3) && (strcmp(argv[2], "set") == 0)) {
				strtou32(argv[3], &tmp);
				cmd_bapi_reboot_set(1, tmp);
				valid_cmd = 1;
			} else
			if ((argc > 2) && (strcmp(argv[2], "get") == 0)) {
				cmd_bapi_reboot_get(1, &tmp);
				valid_cmd = 1;
			}
		}
	}

	if (valid_cmd == 0) {
		putstr("Type 'help bapi' for help\r\n");
		rval = -1;
	}

	return rval;
}

/* ==========================================================================*/
static char help_bapi[] =
	"bapi init                  - Setup bapi\r\n"
	"bapi [n/f]boot[c] <param>  - Boot Linux with bapi support\r\n"
	"bapi hb [save/resume]      - BAPI Hibernation\r\n"
	"bapi rb [set/get] <param>  - BAPI Reboot\r\n"
	"\r\n";

__CMDLIST(cmd_bapi, "bapi", help_bapi);

