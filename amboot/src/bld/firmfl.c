/**
 * @file system/src/bld/firmfl.c
 *
 * Embedded Flash and Firmware Programming Utilities
 *
 * History:
 *    2005/01/31 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition and consolidated
 *			different parition programmings into one
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
#include <sm.h>

#ifdef __BUILD_AMBOOT__
#include <bldfunc.h>
#else
#include <kutil.h>
#include <string.h>
#endif

#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>
#include <fio/fw_part.h>
#include <flash/flash.h>
#include <fio/ftl_const.h>
#include <peripheral/eeprom.h>

#if defined(ENABLE_SD)
#include <fio/sparse_format.h>
#endif

#define ERASE_UNUSED_BLOCK	1

#if defined(NAND_SUPPORT_BBT)
#define FLASH_BBT_BLOCKS	2
#endif

const char *FLPROG_ERR_STR[] =
{
	"program ok",
	"wrong magic number",
	"invalid length",
	"invalid CRC32 code",
	"invalid version number",
	"invalid version date",
	"program image failed",
	"get ptb failed",
	"set ptb failed"
};

#define FWPROG_BUF_SIZE		0x04000		/* 16K */

static u8 page_data[FWPROG_BUF_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));
static u8 check_buf[FWPROG_BUF_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));

#if defined(ENABLE_SD)
static const u32 zero_buf = DRAM_START_ADDR + (DRAM_SIZE - 0x1000000);
#define ZERO_BUF_SIZE 0x1000000 /* 16M */
#endif

int output_progress(int percentage, void *arg);
void output_bad_block(u32 block, int bb_tyep);

#if defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE) && \
    defined(NAND_BB_PRE_SCAN)
static bb_pre_scan_t G_bb_pre_scan
__attribute__ ((aligned(32), section(".bss.noinit")));
#endif

#if !defined(NAND_FLASH_NONE)
struct nand_oobfree oobfree_512[] =
			{{0, 5}, {11, 5}, {0, 0}};
struct nand_oobfree oobfree_2048[] =
			{{1, 7}, {13, 3}, {17, 7}, {29, 3},
			{33, 7}, {45, 3}, {49, 7}, {61, 3}, {0, 0}};

int nand_fill_oob(u8 *raw_image, u8 *oob_buf, u32 main_size, u32 spare_size)
{
	int i, start, len, filled;
	struct nand_oobfree *oobfree_info;
	if (main_size == 512)
		oobfree_info = oobfree_512;
	else
	if (main_size == 2048)
		oobfree_info = oobfree_2048;
	else
		return -1;
	memset(oob_buf, 0xff, spare_size);

	for (i = 0, filled = 0; oobfree_info[i].length > 0; i++) {
		start = oobfree_info[i].offset;
		len = oobfree_info[i].length;
		memcpy(oob_buf + start, raw_image + filled, len);
		filled += len;
	}

	return 0;
}

int memcmp_oob(const void *dst, const void *src, u32 main_size, u32 spare_size)
{
	int i, start, len;
	int rval, error = 0;

	struct nand_oobfree *oobfree_info;
	if (main_size == 512)
		oobfree_info = oobfree_512;
	else
	if (main_size == 2048)
		oobfree_info = oobfree_2048;
	else
		return -1;

	for (i = 0; oobfree_info[i].length > 0; i++) {
		start = oobfree_info[i].offset;
		len = oobfree_info[i].length;
		rval = memcmp((u8 *)dst + start, (u8 *)src + start, len);
		if (rval != 0)
			error++;
	}

	return error;
}
flnand_t flnand;
#endif

#if !defined(NOR_FLASH_NONE)
flnor_t flnor;
#endif

#if defined(ENABLE_FLASH)

#if !defined(CONFIG_NAND_NONE)
/**
 * Check for bad block.
 */
int nand_is_bad_block(u32 block)
{
	int rval = -1, i;
	u8 bi;

#if defined(NAND_SUPPORT_BBT)
	if(nand_has_bbt())
		return nand_isbad_bbt(block);
#endif

#if (CHIP_REV == A1)
	for (i = 0; i < BAD_BLOCK_PAGES; i++) {
		u8 *sbuf;

		if (flnand.main_size == 512)
			sbuf = check_buf + i * (1 << NAND_SPARE_16_SHF);
		else
			sbuf = check_buf + i * (1 << NAND_SPARE_64_SHF);

		rval = nand_read_spare(block, 0, 1, sbuf);
		if (rval < 0) {
			putstr("check bad block failed >> "
					"read spare data error.\r\n");
			/* Treat as factory bad block */
			return NAND_INITIAL_BAD_BLOCK;
		}
	}
#else
	rval = nand_read_spare(block, 0, BAD_BLOCK_PAGES, check_buf);
	if (rval < 0) {
		putstr("check bad block failed >> "
				"read spare data error.\r\n");
		/* Treat as factory bad block */
		return NAND_INITIAL_BAD_BLOCK;
	}
#endif

	for (i = 0; i < INIT_BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(check_buf + i * (1 << NAND_SPARE_16_SHF) + 5);
		else
			bi = *(check_buf + i * (1 << NAND_SPARE_64_SHF));

		if (bi != 0xff)
			break;
	}


	/* Good block */
	if (i == INIT_BAD_BLOCK_PAGES)
		return NAND_GOOD_BLOCK;

	for (i = INIT_BAD_BLOCK_PAGES; i < BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(check_buf + i * (1 << NAND_SPARE_16_SHF) + 5);
		else
			bi = *(check_buf + i * (1 << NAND_SPARE_64_SHF));

		if (bi != 0xff)
			break;
	}

	if (i < BAD_BLOCK_PAGES) {
		/* Late developed bad blocks. */
		return NAND_LATE_DEVEL_BAD_BLOCK;
	} else {
		/* Initial invalid blocks. */
		return NAND_INITIAL_BAD_BLOCK;
	}
}

/**
 * Mark a bad block.
 */
int nand_mark_bad_block(u32 block)
{
	int rval = -1, i, j;
	u8 bi;

#if defined(NAND_SUPPORT_BBT)
	nand_update_bbt(block, 0);
#endif

	for (i = AMB_BB_START_PAGE; i < BAD_BLOCK_PAGES; i++) {
		memset(check_buf, 0xff, flnand.spare_size);
		if (flnand.main_size == 512) {
			*(check_buf + 5) = AMB_BAD_BLOCK_MARKER;
		} else {
			for (j = 0; j < 4; j++) {
				*(check_buf + j * (1 << NAND_SPARE_64_SHF)) =
				AMB_BAD_BLOCK_MARKER;
			}
		}

		rval = nand_prog_spare(block, i, 1, check_buf);
		if (rval < 0) {
			putstr("mark bad block failed >> "
				"write spare data error.\r\n");
			return rval;
		}

		rval = nand_read_spare(block, i, 1, check_buf);
		if (rval < 0) {
			putstr("mark bad block failed >> "
				"read spare data error.\r\n");
			return rval;
		}

		if (flnand.main_size == 512)
			bi = *(check_buf + 5);
		else
			bi = *check_buf;

		if (bi == 0xff) {
			putstr("mark bad block failed >> "
				"verify failed at block ");
			putdec(block);
			putstr("\r\n");
			return -1;
		}
	}

	return 0;
}

/**
 * Sequence for programming a chunk of image from memory to NAND flash.
 */
int nand_prog_block_loop(u8 *raw_image, unsigned int raw_size,
			 u32 sblk, u32 nblk,
			 int (*output_progress)(int, void *), void *arg)
{
	int rval = 0, firm_ok = 0;;
	u32 block, page;
	u32 percentage;
	unsigned int offset, pre_offset;

	/* Program image into the flash */
	offset = 0;
	for (block = sblk; block < (sblk + nblk); block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			rval = 0;
			continue;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try next block...\r\n");

			/* Marked and skipped bad block */
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				return FLPROG_ERR_PROG_IMG;
			else
				continue;
		}

#ifdef	ERASE_UNUSED_BLOCK
		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;
#endif

		pre_offset = offset;
		/* Program each page */
		for (page = 0; page < flnand.pages_per_block; page++) {
			/* Program a page */
			rval = nand_prog_pages(block, page, 1,
							(raw_image + offset), NULL);
			if (rval < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Read it back for verification */
			rval = nand_read_pages(block, page, 1, check_buf, NULL, 1);
			if (rval < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset, check_buf,
				      flnand.main_size);
			if (rval != 0) {
				putstr("nand_prog_block_loop check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				rval = -1;
				break;
			}

			/* The spare layout of 6/8b ECC is different from 1b ECC. */
			if ((flnand.image_flag & IMAGE_HAS_OOB_DATA) && flnand.ecc_bits <= 1) {
				/* Prepare oob data from raw image. */
				rval = nand_fill_oob(
					(raw_image + offset + flnand.main_size),
					page_data, flnand.main_size,
					flnand.spare_size);
				if (rval < 0) {
					putstr("fill oob failed. <block ");
					putdec(block);
					putstr(", page ");
					putdec(page);
					putstr(">\r\n");
					break;
				}

				rval = nand_prog_spare(block, page, 1, page_data);
				if (rval < 0) {
					putstr("program spare failed. <block ");
					putdec(block);
					putstr(", page ");
					putdec(page);
					putstr(">\r\n");
					break;
				}

				rval = nand_read_spare(block, page, 1,
					check_buf);
				if (rval < 0) {
					putstr("read spare failed. <block ");
					putdec(block);
					putstr(", page ");
					putdec(page);
					putstr(">\r\n");
					break;
				}

				/* Compare memory content after read back */
				rval = memcmp_oob(page_data, check_buf,
						flnand.main_size,
						flnand.spare_size);
				if (rval != 0) {
					putstr("oob check failed. <block ");
					putdec(block);
					putstr(", page ");
					putdec(page);
					putstr(">\r\n");
					rval = -1;
					break;
				}

				offset += flnand.main_size + flnand.spare_size;
			} else {
				offset += flnand.main_size;
			}


			if (offset >= raw_size) {
				firm_ok = 1;
#ifndef	ERASE_UNUSED_BLOCK
				block = (sblk + nblk + 1);  /* force out! */
#endif
				break;
			}
		}

		if (rval < 0) {
			offset = pre_offset;
			rval = nand_mark_bad_block(block);
			if (rval < 0)
				break;
			else {
				rval = nand_is_bad_block(block);
				output_bad_block(block, rval);
				rval = 0;
				continue;
			}
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if (rval < 0 || firm_ok == 0)
		rval = FLPROG_ERR_PROG_IMG;

	return rval;
}

/**
 * Sequence for programming a chunk of image from memory to NAND flash.
 * (non-skip bad block)
 */
int nand_prog_block_loop_ns(u8 *raw_image, unsigned int raw_size,
			 u32 sblk, u32 nblk,
			 int (*output_progress)(int, void *), void *arg)
{
	int rval = 0, firm_ok = 0;
	u32 block, page;
	u32 percentage;
	unsigned int offset = 0;

	/* Program image into the flash */
	for (block = sblk; block < (sblk + nblk); block++) {
		rval = nand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			continue;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try to program...\r\n");
		}

#ifdef	ERASE_UNUSED_BLOCK
		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;
#endif

		/* Program each page */
		for (page = 0; page < flnand.pages_per_block; page++) {
			/* Program a page */
			rval = nand_prog_pages(block, page, 1,
							(raw_image + offset), NULL);
			if (rval < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Read it back for verification */
			rval = nand_read_pages(block, page, 1, check_buf, NULL, 1);
			if (rval < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset, check_buf,
				      flnand.main_size);
			if (rval != 0) {
				putstr("check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				rval = -1;
				break;
			}

			offset += flnand.main_size;
			if (offset >= raw_size) {
				firm_ok = 1;
#ifndef	ERASE_UNUSED_BLOCK
				block = (sblk + nblk + 1);  /* force out! */
#endif
				break;
			}
		}

		if (rval < 0) {
			break;
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if (rval < 0 || firm_ok == 0)
		rval = FLPROG_ERR_PROG_IMG;

	return rval;
}

/* Functions below are used to caculate nftl-based partition information. */
static void nand_get_rbz(int nblk, int zonet, int rb_per_zt,
			 int min_rb, int *zones, int *rb_per_zone)
{
	int z, r, nblk_per_zone;

	for (z = 0; (z * zonet) < nblk; z++);

	nblk_per_zone = nblk /z;
	r = rb_per_zt * nblk_per_zone / zonet;

	if (rb_per_zt * nblk_per_zone % zonet)
		r++;

	if (r < min_rb)
		r = min_rb;

	*zones = z;
	*rb_per_zone = r;
}

#if defined(NAND_SUPPORT_BBT)
static u32 nand_get_bbt_rbz(u32 end_block)
{
	u32 n, bbt_blks = FLASH_BBT_BLOCKS;

	for (n = 0; n < bbt_blks; n++) {
		if (nand_is_bad_block(end_block)) {
			bbt_blks++;
		}
		end_block--;
	}
	return bbt_blks;
}
#endif

static void nand_get_media_part(nand_mp_t *mp)
{
	int sblk, nblk, block_size;
	int zones, r, err = 0;
	u32 start_block, end_block;
	int i;

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnand.nblk[i];
	}

	end_block = flnand.blocks_per_bank * flnand.banks;
#if defined(NAND_SUPPORT_BBT)
	end_block -= nand_get_bbt_rbz(end_block);
#endif
	memset(mp, 0x0, sizeof(*mp));

	if (start_block >= end_block) {
		err = -1;
		goto done;
	}

	block_size = flnand.main_size * flnand.pages_per_block;

	/***************************************************/
	/* Partitions grow from the end of the NAND flash. */
	/***************************************************/

	/* Preferences partition */
	nblk = MP_PRF_SIZE / block_size;
	if ((MP_PRF_SIZE % block_size) != 0x0)
		nblk++;

	if (nblk > 0) {
		nand_get_rbz(nblk,
			     PREF_ZONE_THRESHOLD,
			     PREF_RSV_BLOCKS_PER_ZONET,
			     PREF_MIN_RSV_BLOCKS_PER_ZONE,
			     &zones, &r);
		mp->rblk[MP_PRF] = r;
		mp->nzone[MP_PRF] = zones;
		mp->trlb[MP_PRF] = PREF_TRL_TABLES;
		nblk += mp->rblk[MP_PRF];
		mp->nblk[MP_PRF] = nblk;
	}

	/* Calibration partition */

	nblk = MP_CAL_SIZE / block_size;
	if ((MP_CAL_SIZE % block_size) != 0x0)
		nblk++;

	if (nblk > 0) {
		nand_get_rbz(nblk,
			     CALIB_ZONE_THRESHOLD,
			     CALIB_RSV_BLOCKS_PER_ZONET,
			     CALIB_MIN_RSV_BLOCKS_PER_ZONE,
			     &zones, &r);
		mp->rblk[MP_CAL] = r;
		mp->nzone[MP_CAL] = zones;
		mp->trlb[MP_CAL] = CALIB_TRL_TABLES;
		nblk += mp->rblk[MP_CAL];
		mp->nblk[MP_CAL] = nblk;
	}

	mp->sblk[MP_PRF] = end_block - mp->nblk[MP_PRF] - mp->nblk[MP_CAL];
	mp->sblk[MP_CAL] = end_block - mp->nblk[MP_CAL];

	/*****************************************************/
	/* Partitions grow from the start of the NAND flash. */
	/*****************************************************/
	end_block = mp->sblk[MP_PRF];

	/* Raw partition */
	sblk = start_block;
	if (sblk >= end_block) {
		err = -1;
		goto done;
	}

	nblk = MP_RAW_SIZE / block_size;
	if ((MP_RAW_SIZE % block_size) != 0x0)
		nblk++;

	if (nblk > 0) {
		nand_get_rbz(nblk,
			     RAW_ZONE_THRESHOLD,
			     RAW_RSV_BLOCKS_PER_ZONET,
			     RAW_MIN_RSV_BLOCKS_PER_ZONE,
			     &zones, &r);
		mp->rblk[MP_RAW] = r;
		mp->nzone[MP_RAW] = zones;
		mp->trlb[MP_RAW] = RAW_TRL_TABLES;
		nblk += mp->rblk[MP_RAW];
		mp->sblk[MP_RAW] = sblk;
		mp->nblk[MP_RAW] = nblk;
	}

	/* Stg2 partition */
	sblk += nblk;
	if (sblk >= end_block) {
		err = -1;
		goto done;
	}

	nblk = MP_STG2_SIZE / block_size;
	if ((MP_STG2_SIZE % block_size) != 0x0)
		nblk++;

	if (nblk > 0) {
		nand_get_rbz(nblk,
			     STG2_ZONE_THRESHOLD,
			     STG2_RSV_BLOCKS_PER_ZONET,
			     STG2_MIN_RSV_BLOCKS_PER_ZONE,
			     &zones, &r);
		mp->rblk[MP_STG2] = r;
		mp->nzone[MP_STG2] = zones;
		mp->trlb[MP_STG2] = STG2_TRL_TABLES;
		nblk += mp->rblk[MP_STG2];
		mp->sblk[MP_STG2] = sblk;
		mp->nblk[MP_STG2] = nblk;
	}

	/****************************************************/
	/* Partitions use the rest space of the NAND flash. */
	/****************************************************/

	/* Storage partition */
	sblk += nblk;
	if (sblk > end_block) {
		/* Not enough space for previous partition. */
		err = -1;
		goto done;
	} else if (sblk == end_block) {
		/* Only no space for storage partition. */
		goto done;
	}

	nblk = end_block - sblk;
	if (nblk > 0) {
		nand_get_rbz(nblk,
			     STG_ZONE_THRESHOLD,
			     STG_RSV_BLOCKS_PER_ZONET,
			     STG_MIN_RSV_BLOCKS_PER_ZONE,
			     &zones, &r);
	}

	/* end_block should align to zones. */
	if (end_block % zones) {
		end_block -= end_block % zones;
		nblk = end_block - sblk;
	}

	if (nblk > (r * zones)) {
		mp->rblk[MP_STG] = r;
		mp->nzone[MP_STG] = zones;
		mp->trlb[MP_STG] = STG_TRL_TABLES;
		mp->sblk[MP_STG] = sblk;
		mp->nblk[MP_STG] = nblk;
	}
done:
	if (err < 0) {
		uart_putstr("NAND is out of space for partitinos");
	}
}

static void nand_partition_bb_scan(u32 id, u32 fw_len)
{
#ifdef NAND_BB_PRE_SCAN
	u32 rval = 0;
	u32 block, part_eblk;
	u32 n_blk;

	bb_pre_scan_t* bb_pre_scan = (bb_pre_scan_t*) &G_bb_pre_scan;

	putstr("program nand bad block info ... ");

	if (id == PART_ROM) {
		block = flnand.sblk[PART_ROM];
		part_eblk = flnand.sblk[PART_ROM] + flnand.nblk[PART_ROM] - 1;
		n_blk = flnand.nblk[PART_ROM];
	} else if (id == PART_DSP) {
		block = flnand.sblk[PART_DSP];
		part_eblk = flnand.sblk[PART_DSP] + flnand.nblk[PART_DSP] - 1;
		n_blk = flnand.nblk[PART_DSP];
	} else {
		putstr("skip\r\n");
		return;
	}

	/* check if last block is bad */
	rval = nand_is_bad_block(part_eblk);
	if (rval != 0) {
		putstr("end blk is bad\r\n");
		return;
	}
	memset(bb_pre_scan, 0x0, sizeof(bb_pre_scan_t));
	bb_pre_scan->magic = BB_PRE_SCAN_MAGIC;

	/* scan part's bb */
	for (; block <= part_eblk; block++) {
		rval = nand_is_bad_block(block);

		/* skip all types of bad block */
		if (rval != 0) {
			if (bb_pre_scan->nbb < FW_PART_MAX_BB) {
				bb_pre_scan->bb_info[bb_pre_scan->nbb] = block;
				bb_pre_scan->nbb++;
			} else {
				putstr("Warning - partition"
				       "too many bad blocks ...\r\n");
				return;
			}
		}
	}
	/* check if last block contains firmware */
	if ((fw_len / flnand.block_size + bb_pre_scan->nbb) >= n_blk) {
		putstr("no enough blk\r\n");
		return;
	}

	putstr("blk ");
	putdec(part_eblk);

	/* In PRI only read a page to get partition bb info */
	if (sizeof(bb_pre_scan_t) > flnand.main_size) {
		putstr("\r\nbb_pre_scan struct size");
		putdec(sizeof(bb_pre_scan_t));
		putstr("lagger then a page\r\n");
		putstr("erase bad blk info...");
		rval = nand_erase_block(part_eblk);
		if (rval < 0)
			putstr("erase failed.\r\n");
		else
			putstr("success.\r\n");
		return;
	}

	rval = nand_prog_block_loop(((u8 *)bb_pre_scan), sizeof(bb_pre_scan_t),
				    part_eblk, 1, NULL, NULL);
	if (rval >= 0)
		putstr("... success\r\n");

#endif /* NAND_BB_PRE_SCAN */
	return;
}

#endif  /* !CONFIG_NAND_NONE */

#if !defined(CONFIG_ONENAND_NONE)
int onenand_prog_block_loop(u8 *raw_image, unsigned int raw_size,
				u32 sblk, u32 nblk,
				int (*output_progress)(int, void *), void *arg)
{
#define ONENAND_MIN_MAIN_DMA_SIZE	2048

	int rval = 0, firm_ok = 0;;
	u32 block, page;
	u32 percentage;
	unsigned int offset, pre_offset;
	u32 page_inc;

	/* Program image into the flash */
	page_inc = ONENAND_MIN_MAIN_DMA_SIZE / flnand.main_size;
	offset = 0;
	for (block = sblk; block < (sblk + nblk); block++) {
		rval = onenand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			continue;
		}

		rval = onenand_erase_block(block);
		if (rval < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try next block...\r\n");

			/* Marked and skipped bad block */
			/* nand_mark_bad_block(block); */
			continue;
		}

#ifdef	ERASE_UNUSED_BLOCK
		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;
#endif

		pre_offset = offset;
		/* Program each page */
		for (page = 0; page < flnand.pages_per_block; page += page_inc) {
			/* Program a page */
			rval = onenand_prog_pages(block, page, page_inc,
							(raw_image + offset));
			if (rval < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Read it back for verification */
			rval = onenand_read_pages(block, page, page_inc, check_buf);
			if (rval < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset, check_buf,
				      flnand.main_size * page_inc);
			if (rval != 0) {
				putstr("check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				rval = -1;
				break;
			}

			offset += flnand.main_size * page_inc;
			if (offset >= raw_size) {
				firm_ok = 1;
#ifndef	ERASE_UNUSED_BLOCK
				block = (sblk + nblk + 1);  /* force out! */
#endif
				break;
			}
		}

		if (rval < 0) {
			offset = pre_offset;
			/* nand_mark_bad_block(block); */
			continue;
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if (rval < 0 || firm_ok == 0)
		rval = FLPROG_ERR_PROG_IMG;

	return rval;


}

/**
 * Sequence for programming a chunk of image from memory to ONENAND flash.
 * (non-skip bad block)
 */
int onenand_prog_block_loop_ns(u8 *raw_image, unsigned int raw_size,
			       u32 sblk, u32 nblk,
			       int (*output_progress)(int, void *), void *arg)
{
	int rval = 0, firm_ok = 0;
	u32 block, page;
	u32 percentage;
	unsigned int offset = 0;
	u32 page_inc = ONENAND_MIN_MAIN_DMA_SIZE / flnand.main_size;

	/* Program image into the flash */
	for (block = sblk; block < (sblk + nblk); block++) {
		rval = onenand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			continue;
		}

		rval = onenand_erase_block(block);
		if (rval < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try to program...\r\n");
		}

#ifdef	ERASE_UNUSED_BLOCK
		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;
#endif

		/* Program each page */
		for (page = 0; page < flnand.pages_per_block; page += page_inc) {
			/* Program a page */
			rval = onenand_prog_pages(block, page, page_inc,
							(raw_image + offset));
			if (rval < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Read it back for verification */
			rval = onenand_read_pages(block, page, page_inc, check_buf);
			if (rval < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset, check_buf,
				      flnand.main_size * page_inc);
			if (rval != 0) {
				putstr("check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				rval = -1;
				break;
			}

			offset += flnand.main_size * page_inc;
			if (offset >= raw_size) {
				firm_ok = 1;
#ifndef	ERASE_UNUSED_BLOCK
				block = (sblk + nblk + 1);  /* force out! */
#endif
				break;
			}
		}

		if (rval < 0) {
			break;
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if (rval < 0 || firm_ok == 0)
		rval = FLPROG_ERR_PROG_IMG;

	return rval;
}

static void onenand_get_media_part(nand_mp_t *mp)
{
	/* Not implement */
	memset(mp, 0x0, sizeof(nand_mp_t));

	return;
}
#endif /* !CONFIG_ONENAND_NONE */

#if !defined(CONFIG_NOR_NONE)

int nor_prog_sector_loop(u8 *raw_image, unsigned int raw_size,
			 u32 ssec, u32 nsec,
			 int (*output_progress)(int, void *), void *arg)
{
	int rval = 0;
	u32 sector = 0;
	u32 offset = 0;
	u32 len = 0;
	u32 percentage = 0;
	u32 buf_check_addr = 0;
	u32 buf_check_len = 0;

	/* Program image into the flash */
	for (sector = ssec; sector < (ssec + nsec); sector++) {
		/* Unlock sector */
		rval = nor_unlock(sector);
		if (rval < 0) {
			putstr("unlock sector ");
			putdec(sector);
			putstr(" ... failed\r\n");
			goto done;
		}

		/* Erase the sector before programming */
		rval = nor_erase_sector(sector);
		if (rval < 0) {
			putstr("erasing sector ");
			putdec(sector);
			putstr(" ... failed\r\n");
			goto done;
		}

		if ((raw_size - offset) > flnor.sector_size) {
			len = flnor.sector_size;
		} else {
			len = raw_size - offset;
			if ((len % 8) != 0)
				len += (8 - (len % 8));
		}

		/* Program the sector */
		rval = nor_prog_sector(sector, raw_image + offset, len);
		if (rval < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(" len ");
			putdec(len);
			putstr(" ... failed in program\r\n");
		}



		while(buf_check_addr < len) {

			if ((buf_check_addr + sizeof(check_buf)) > len) {
				buf_check_len = len - buf_check_addr;
			} else {
				buf_check_len = sizeof(check_buf);
			}

			/* Read it back for verification */
			rval = nor_read(((sector * flnor.sector_size) + buf_check_addr)
					,check_buf, buf_check_len);
			if (rval < 0) {
				putstr("reading sector ");
				putdec(sector);
				putstr(" len ");
				putdec(len);
				putstr(" ... failed in read back\r\n");
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset + buf_check_addr,
					check_buf, buf_check_len);
			if (rval != 0) {
				putstr("reading sector ");
				putdec(sector);
				putstr(" len ");
				putdec(len);
				putstr(" ... failed in verify\r\n");
				rval = -1;
			}

			buf_check_addr += sizeof(check_buf);
		}

		/* Lock sector */
		rval = nor_lock(sector);
		if (rval < 0) {
			putstr("lock sector ");
			putdec(sector);
			putstr(" ... failed\r\n");
			goto done;
		}

		offset += len;

		/* display progress */
		if (output_progress) {
			if (offset >= raw_size)
				percentage = 100;
			else
				percentage = offset / (raw_size / 100);

			output_progress(percentage, NULL);
		}

		if (offset >= raw_size) {
			break;
		}
	}

done:
	return rval;
}

#endif  /* !CONFIG_NOR_NONE */

#if !defined(CONFIG_SNOR_NONE)

int snor_prog_sector_loop(u8 *raw_image, unsigned int raw_size,
			 u32 ssec, u32 nsec,
			 int (*output_progress)(int, void *), void *arg)
{
	int rval = 0;
	u32 sector = 0;
	u32 offset = 0;
	u32 len = 0;
	u32 percentage = 0;
	u32 block_size, addr;
	u32 buf_check_addr = 0;
	u32 buf_check_len = 0;

	/* Program image into the flash */
	for (sector = ssec; sector < (ssec + nsec); sector++) {
		/* Erase the sector before programming */
		rval = snor_erase_block(sector);
		if (rval < 0) {
			putstr("erasing sector ");
			putdec(sector);
			putstr(" ... failed\r\n");
			goto done;
		}

		block_size = snor_blocks_to_len(ssec, 1);
		if ((raw_size - offset) > block_size) {
			len = block_size;
		} else {
			len = raw_size - offset;
			if ((len % 8) != 0)
				len += (8 - (len % 8));
		}

		/* Program the sector */
		addr = snor_get_addr(sector, 0);
		rval = snor_prog(addr, len, raw_image + offset);
		if (rval < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(" len ");
			putdec(len);
			putstr(" ... failed in program\r\n");
		}

		while(buf_check_addr < len) {

			if ((buf_check_addr + sizeof(check_buf))> len) {
				buf_check_len = len - buf_check_addr;
			} else {
				buf_check_len = sizeof(check_buf);
			}

			/* Read it back for verification */
			rval = snor_read(addr + buf_check_addr , buf_check_len , check_buf);
			if (rval < 0) {
				putstr("reading sector ");
				putdec(sector);
				putstr(" len ");
				putdec(len);
				putstr(" ... failed in read back\r\n");
			}

			/* Compare memory content after read back */
			rval = memcmp(raw_image + offset + buf_check_addr , check_buf, buf_check_len);
			if (rval != 0) {
				putstr("reading sector ");
				putdec(sector);
				putstr(" len ");
				putdec(len);
				putstr(" ... failed in verify\r\n");
				rval = -1;
			}
			buf_check_addr += sizeof(check_buf);
		}

		offset += len;

		/* display progress */
		if (output_progress) {
			if (offset > raw_size)
				percentage = 100;
			else
				percentage = offset * 100 / raw_size;

			output_progress(percentage, NULL);
		}

		if (offset >= raw_size) {
			break;
		}
	}

done:
	return rval;
}

#endif /* !CONFIG_SNOR_NONE */

#endif  /* ENABLE_FLASH */

#if defined(ENABLE_SD)

static u8 *sd_check_buf = (u8 *)BSB_RAM_START;
#define SDPROG_BUF_SIZE		0x80000

int sm_prog_sector_loop(u8 *raw_image, unsigned int raw_size,
			u32 ssec, u32 nsec,
			int (*output_progress)(int, void *), void *arg)
{
	int rval = 0;
	u32 sector = 0;
	u32 sectors = (SDPROG_BUF_SIZE >> 9);
	u32 offset = 0;
	u32 len = 0;
	u32 percentage = 0;

	/* Program image into the flash */
	for (sector = ssec; sector < (ssec + nsec); sector += sectors) {
		if ((raw_size - offset) > SDPROG_BUF_SIZE) {
			len = SDPROG_BUF_SIZE;
		} else {
			len = raw_size - offset;
			if ((len % 8) != 0)
				len += (8 - (len % 8));
		}

		/* Program the sector */
		rval = sm_write_sector(sector, raw_image + offset, len);
		if (rval < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(" len ");
			putdec(len);
			putstr(" ... failed in program\r\n");
		}

		/* Read it back for verification */
		rval = sm_read_sector(sector, sd_check_buf, len);
		if (rval < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(" len ");
			putdec(len);
			putstr(" ... failed in read back\r\n");
		}

		/* Compare memory content after read back */
		rval = memcmp(raw_image + offset, sd_check_buf, len);
		if (rval != 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(" len ");
			putdec(len);
			putstr(" ... failed in verify\r\n");
			rval = -1;
		}

		offset += len;

		/* display progress */
		if (output_progress) {
			if (offset >= raw_size)
				percentage = 100;
			else
				percentage = offset / (raw_size / 100);

			output_progress(percentage, NULL);
		}

		if (offset >= raw_size) {
			break;
		}
	}

	return rval;
}

#endif  /* ENABLE_SD */

#if ((!defined(ENABLE_FLASH)) || \
     (defined(CONFIG_NAND_NONE) && defined(CONFIG_NOR_NONE) && \
      defined(CONFIG_ONENAND_NONE) && defined(CONFIG_SNOR_NONE) \
      && !defined(FIRMWARE_CONTAINER)))

/***********************************************/
/* Dummy functions that always fail the caller */
/***********************************************/
void flprog_get_dev_param(flpart_table_t *table) { return; }
int flprog_validate_image(u8 *image, unsigned int len) { return -1; }
int flprog_get_part_table(flpart_table_t *table) { return -1; }
int flprog_set_part_table(flpart_table_t *table) { return -1; }
int flprog_prog(int pid, u8 *image, unsigned int len) { return -1; }
int flprog_erase_partition(int pid) { return -1; };

#else

#if !defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE) || \
    !defined(CONFIG_ONENAND_NONE) || !defined(CONFIG_SNOR_NONE) || \
    defined(FIRMWARE_CONTAINER)

/**
 * Get the ptb.dev parameters which are original settings of firmware.
 */
void flprog_get_dev_param(flpart_table_t *table)
{
	/* Always update device entry of ptb. */
#if defined(AMBOOT_DEV_USBDL_MODE)
	table->dev.usbdl_mode = 1;
#endif
#if defined(AMBOOT_DEV_AUTO_BOOT)
	table->dev.auto_boot = 1;
#endif
#if defined(AMBOOT_DEV_CMDLINE)
	strncpy(table->dev.cmdline, AMBOOT_DEV_CMDLINE,
		sizeof(table->dev.cmdline) - 1);
#endif
#if defined(AMBOOT_DEFAULT_SN)
		strncpy(table->dev.sn, AMBOOT_DEFAULT_SN, sizeof(table->dev.sn) - 1);
#endif
}

/**
 * Validate the content of the image supplied by the caller.
 */
int flprog_validate_image(u8 *image, unsigned int len)
{
	partimg_header_t *header;
	u32 raw_crc32;

#ifndef __BUILD_AMBOOT__
	K_ASSERT(image != NULL);
	K_ASSERT(len > sizeof(partimg_header_t));
#endif

	header = (partimg_header_t *) image;

	if (header->magic != PARTHD_MAGIC) {
		putstr("wrong magic!\r\n");
		return FLPROG_ERR_MAGIC;
	}

	if (header->ver_num == 0x0) {
		putstr("invalid version!\r\n");
		return FLPROG_ERR_VER_NUM;
	}

	if (header->ver_date == 0x0) {
		putstr("invalid date!\r\n");
		return FLPROG_ERR_VER_DATE;
	}

	putstr("verifying image crc ... ");
	raw_crc32 = crc32((u8 *) (image + sizeof(partimg_header_t)),
			  header->img_len);

	if (raw_crc32 != header->crc32) {
		putstr("0x");
		puthex(raw_crc32);
		putstr(" != 0x");
		puthex(header->crc32);
		putstr(" failed!\r\n");
		return FLPROG_ERR_CRC;
	} else {
		putstr("done\r\n");
	}

	return FLPROG_OK;
}

int flprog_update_part_info_from_meta(void)
{
#ifndef NAND_FLASH_NONE
	flpart_meta_t table;
	int i, rval;
	u32 boot_from = rct_boot_from();

	if (((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) ||
	    ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND)) {
		memzero(&table, sizeof(flpart_meta_t));
		rval = flprog_get_meta(&table);
		if (rval < 0)
			return rval;

		for (i = 0; i < TOTAL_FW_PARTS; i++) {
			if (g_part_dev[i] != BOOT_DEV_NAND &&
			    g_part_dev[i] != BOOT_DEV_ONENAND)
				continue;
			flnand.sblk[i] = table.part_info[i].sblk;
			flnand.nblk[i] = table.part_info[i].nblk;
		}
	}
#endif
	return 0;
}

/**
 * Get the content of the partition table.
 */
int flprog_get_part_table(flpart_table_t *table)
{
	int rval = -1;

#ifdef CONFIG_KERNEL_DUAL_CPU
	unsigned int boot_from = BOOT_FROM_NAND;// fix it
#else
	unsigned int boot_from = rct_boot_from();
#endif

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(ENABLE_SD)
		/* NOTE: sizeof(flpart_table_t) */
		/* should align to sector boundry? */
		smpart_t *smpart = sm_get_part(PART_PTB);
		rval = sm_read_sector(smpart->ssec, page_data,
				      sizeof(flpart_table_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		int buf_size, pages;
		u32 block;
		int i;
		int flsize = flnand.main_size;

		pages = sizeof(*table) / flsize;
		if (sizeof(*table) % flsize)
			pages++;

		buf_size = pages * flsize;
		if (buf_size > FWPROG_BUF_SIZE) {
			putstr("out of buf\n\r");
		}

		for (block = flnand.sblk[PART_PTB];
		     block < flnand.sblk[PART_PTB] + flnand.nblk[PART_PTB];
		     block++) {
			if (nand_is_bad_block(block)) {
				continue;
			} else {
				for (i = 0; i < pages; i++) {
					rval = nand_read_pages(block, i, 1,
							page_data + i * flsize, NULL,
							1);
					if (rval < 0)
						break;
				}
				if (rval == 0)
					break;
			}
		}
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		u8 *src = (u8 *)(flnand.sblk[PART_PTB] * flnand.block_size);
		rval = onenand_read_data(page_data, src,
					 sizeof(flpart_table_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		/* NOTE: sizeof(flpart_table_t) */
		/* should align to sector boundry? */
		rval = nor_read_sector(flnor.sblk[PART_PTB], page_data,
				       sizeof(flpart_table_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {
		u32 addr = snor_get_addr(flnor.sblk[PART_PTB], 0);
		rval = snor_read(addr, sizeof(flpart_table_t), page_data);
		goto done;
#endif
	}

done:

	if (rval >=  0) {
		memcpy(table, page_data, sizeof(flpart_table_t));
	} else {
		putstr("Get ptb failed\r\n");
		memset(table, 0xff, sizeof(flpart_table_t));
		rval = FLPROG_ERR_PTB_GET;
	}

	return rval;
}

/**
 * Program the PTB entry.
 */
int flprog_set_part_table(flpart_table_t *table)
{
	int rval = -1;
	unsigned int boot_from = rct_boot_from();
	int i;

	if (table->dev.magic != FLPART_MAGIC) {
		memzero(&table->dev, sizeof(table->dev));
		flprog_get_dev_param(table);
		table->dev.magic = FLPART_MAGIC;
	}

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (table->part[i].magic != FLPART_MAGIC) {
			memzero(&table->part[i], sizeof(flpart_t));
			table->part[i].magic = FLPART_MAGIC;
		}
	}

	memzero(page_data, sizeof(page_data));
	memcpy(page_data, table, sizeof(flpart_table_t));

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(ENABLE_SD)
		smpart_t *smpart = sm_get_part(PART_PTB);
		rval = sm_prog_sector_loop(page_data, sizeof(flpart_table_t),
					   smpart->ssec,
					   smpart->nsec,
					   NULL, NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
#ifdef FIRMWARE_CONTAINER
		smpart_t *smpart = sm_get_part(PART_PTB);
		putstr("Pragram 2nd PTB on SM\r\n");
		rval = sm_prog_sector_loop(page_data, sizeof(flpart_table_t),
					   smpart->ssec, smpart->nsec,
					   NULL, NULL);
		if (rval < 0) {
			putstr("...fail!");
			goto done;
		}
#endif
		rval = nand_prog_block_loop(page_data, sizeof(flpart_table_t),
					    flnand.sblk[PART_PTB],
					    flnand.nblk[PART_PTB],
					    NULL, NULL);
		goto done;

#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		rval = onenand_prog_block_loop(page_data,
					       sizeof(flpart_table_t),
					       flnand.sblk[PART_PTB],
					       flnand.nblk[PART_PTB],
					       NULL, NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		rval = nor_prog_sector_loop(page_data, sizeof(flpart_table_t),
					    flnor.sblk[PART_PTB],
					    flnor.nblk[PART_PTB],
					    NULL, NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {
		rval = snor_prog_sector_loop(page_data, sizeof(flpart_table_t),
					    flnor.sblk[PART_PTB],
					    flnor.nblk[PART_PTB],
					    NULL, NULL);
		goto done;
#endif
	}

done:

	if (rval < 0) {
		putstr("Set ptb failed\r\n");
		rval = FLPROG_ERR_PTB_SET;
	} else {
		rval = flprog_set_meta();
	}

	return rval;
}

/**
 * Get the content of the partition table.
 */
int flprog_get_meta(flpart_meta_t *table)
{
	int rval = -1;

	unsigned int boot_from = rct_boot_from();

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(ENABLE_SD)
		/* NOTE: sizeof(flpart_table_t) */
		/* should align to sector boundry? */
		u32 offset;
		smdev_t *smdev = sm_get_dev();
		smpart_t *smpart = sm_get_part(PART_PTB);
		offset = sizeof(flpart_table_t) / smdev->sector_size;
		rval = sm_read_sector(smpart->ssec + offset, page_data,
				      sizeof(flpart_meta_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		int buf_size, pages, page;
		u32 block;
		int i;
		int flsize = flnand.main_size;

		page = sizeof(flpart_table_t) / flsize;
		pages = sizeof(flpart_meta_t) / flsize;
		if (sizeof(*table) % flsize)
			pages++;

		buf_size = pages * flsize;
		if (buf_size > FWPROG_BUF_SIZE) {
			putstr("out of buf\n\r");
		}

		for (block = flnand.sblk[PART_PTB];
		     block < flnand.sblk[PART_PTB] + flnand.nblk[PART_PTB];
		     block++) {
			if (nand_is_bad_block(block)) {
				continue;
			} else {
				for (i = 0; i < pages; i++) {
					rval = nand_read_pages(block, i + page,
						     1, page_data + i * flsize, NULL,
						     1);
					if (rval < 0)
						break;
				}
				if (rval == 0)
					break;
			}
		}
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		u32 page = sizeof(flpart_meta_t);
		u8 *src = (u8 *)(flnand.sblk[PART_PTB] * flnand.block_size +
				page);
		rval = onenand_read_data(page_data, src,
					 sizeof(flpart_table_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		/* NOTE: sizeof(flpart_table_t) */
		/* should align to sector boundry? */
		u32 sec = (sizeof(flpart_meta_t) / flnor.sector_size;
		rval = nor_read_sector(flnor.sblk[PART_PTB] + sec, page_data,
				       sizeof(flpart_table_t));
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {
		u32 sec = (sizeof(flpart_meta_t) / flnor.sector_size;
		u32 addr = snor_get_addr(flnor.sblk[PART_PTB] + sec, 0);
		rval = snor_read(addr, sizeof(flpart_table_t), page_data);
		goto done;
#endif
	}

done:

	if (rval >=  0) {
		memcpy(table, page_data, sizeof(flpart_meta_t));
	} else {
		putstr("Get meta failed\r\n");
		memset(table, 0xff, sizeof(flpart_meta_t));
		rval = FLPROG_ERR_META_GET;
	}

	return rval;
}


static void flprog_update_smstg2_info(flpart_meta_t *meta)
{
#if defined(SM_STG2_SLOT) || defined(FIRMWARE_CONTAINER)
#if defined(SM_STG2_SLOT)
	int slot = SM_STG2_SLOT;
#else
	int slot = FIRMWARE_CONTAINER;
#endif
	u32 nsec;
	smpart_t *smpart = sm_get_part(PART_STG);

	meta->sm_stg[0].sblk = smpart->ssec;
	meta->sm_stg[0].nblk = smpart->nsec;

#if defined(SM_STG2_SIZE)
	nsec = (SM_STG2_SIZE & (SM_SECTOR_SIZE - 1)) ?
	       (SM_STG2_SIZE / SM_SECTOR_SIZE + 1):
	       (SM_STG2_SIZE / SM_SECTOR_SIZE);
#endif
	if (smpart->nsec != 0 && g_part_dev[PART_STG2] == BOOT_DEV_SM)
		nsec = smpart->nsec;

	if (nsec == 0)
		return;

	meta->sm_stg[1].sblk = (smpart->ssec + smpart->nsec) - nsec;
	meta->sm_stg[1].nblk = nsec;

	if ((slot != SCARDMGR_SLOT_FL) && (slot != SCARDMGR_SLOT_FL2)) {
		/* Let start sector align to Boundary Unit */
	    	u32 misalign = smpart->ssec & (SM_SECTOR_ALIGN - 1);
		meta->sm_stg[1].sblk += (misalign) ?
					(SM_SECTOR_ALIGN - misalign) : (0);
		meta->sm_stg[1].nblk -= (misalign) ?
					(SM_SECTOR_ALIGN - misalign) : (0);
	}

	meta->sm_stg[0].nblk -= meta->sm_stg[1].nblk;
#else
	return;
#endif
}

static void flprog_update_meta(flpart_meta_t *meta)
{
	int i;

	for (i = 0; i < PART_MAX; i++) {
		memcpy(meta->part_info[i].name,
		       g_part_str[i],
		       strlen(g_part_str[i]));
		meta->part_dev[i] = g_part_dev[i];
	}

	meta->magic = PTB_META_MAGIC;

#ifdef FW_MODEL_NAME
	K_ASSERT(strlen(FW_MODEL_NAME) <= FW_MODEL_NAME_SIZE);
	strcpy(meta->model_name, FW_MODEL_NAME);
#endif
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		if ((g_part_dev[i] == BOOT_DEV_NAND) ||
		    (g_part_dev[i] == BOOT_DEV_ONENAND)) {
#if !defined(NAND_FLASH_NONE)
			meta->part_info[i].sblk = flnand.sblk[i];
			meta->part_info[i].nblk = flnand.nblk[i];
#endif
#if !defined(NOR_FLASH_NONE)
		} else if ((g_part_dev[i] == BOOT_DEV_NOR) ||
			   (g_part_dev[i] == BOOT_DEV_SNOR)) {
			meta->part_info[i].sblk = flnor.sblk[i];
			meta->part_info[i].nblk = flnor.nblk[i];
#endif
#if defined(ENABLE_SM)
		} else if ((g_part_dev[i] == BOOT_DEV_SM)) {
			smpart_t *smpart = sm_get_part(i);
			meta->part_info[i].sblk = smpart->ssec;
			meta->part_info[i].nblk = smpart->nsec;
#endif
		} else
			K_ASSERT(0);
	}

	flprog_update_smstg2_info(meta);

	meta->crc32 = crc32((void *)meta, sizeof(flpart_meta_t) - sizeof(u32));
}

static int prog_meta_to_sm(void)
{
#ifdef FIRMWARE_CONTAINER
	smdev_t *smdev = sm_get_dev();
	u32 secs = sizeof(flpart_meta_t) / smdev->sector_size;
	u32 offset = sizeof(flpart_table_t) / smdev->sector_size;

	return sm_prog_sector_loop(page_data, sizeof(flpart_meta_t),
				   smdev->part[PART_PTB].ssec + offset,
				   secs, NULL, NULL);
#else
	return 0;
#endif
}

/**
 * Program the PTB meta.
 */
int flprog_set_meta(void)
{
	int i, rval = -1;
	unsigned int boot_from = rct_boot_from();
	flpart_meta_t table;
	u32 offset;

	/* Meta data would be pleace behind PTB.	      */
	/* PTB block would be erased during firmware program. */
	/* So we don't erase PTB block here 		      */
	K_ASSERT(sizeof(flpart_meta_t) == PTB_META_SIZE);
	memzero(&table, sizeof(flpart_meta_t));
	flprog_update_meta(&table);

	memzero(page_data, sizeof(page_data));
	memcpy(page_data, &table, sizeof(flpart_meta_t));

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(ENABLE_SD)
		rval = prog_meta_to_sm();
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		u32 sblk = flnand.sblk[PART_PTB], nblk = flnand.nblk[PART_PTB];
		u32 pages = sizeof(flpart_meta_t) / flnand.main_size;
		offset = sizeof(flpart_table_t) / flnand.main_size;

		for (i = sblk; i < (sblk + nblk); i++){
			rval = nand_is_bad_block(i);
			if (rval == 0) {
				nand_prog_pages(i, offset, pages, page_data, NULL);
				memzero(page_data, sizeof(page_data));
				nand_read_pages(i, offset, pages, page_data, NULL, 1);
				rval = memcmp(page_data,
					      &table,
					      sizeof(flpart_meta_t));
				break;
			}
			rval = -1;
		}
		rval = prog_meta_to_sm();
		if (rval < 0)
			putstr("...fail!\n");

		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		u32 pages = sizeof(flpart_meta_t) / flnand.main_size;
		offset = sizeof(flpart_table_t) / flnand.main_size;

		for (i = flnand.sblk[PART_PTB]; i < flnand.nblk[PART_PTB]; i++){
			rval = onenand_is_bad_block(i);
			if (rval == 0) {
				onenand_prog_pages(i, offset, pages, page_data);
				memzero(page_data, sizeof(page_data));
				onenand_read_pages(i, offset, pages, page_data);
				rval = memcmp(page_data,
					      &table,
					      sizeof(flpart_meta_t));
				break;
			}
		}
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		u32 secs = sizeof(flpart_meta_t) / flnor.sector_size;
		offset = sizeof(flpart_table_t) / flnor.sector_size;
		rval = nor_prog_sector_loop(page_data, sizeof(flpart_meta_t),
					    flnor.sblk[PART_PTB] + offset,
					    secs,
					    NULL, NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {
		u32 secs = sizeof(flpart_meta_t) / flnor.sector_size;
		offset = sizeof(flpart_table_t) / flnor.sector_size;
		rval = snor_prog_sector_loop(page_data, sizeof(flpart_meta_t),
					    flnor.sblk[PART_PTB] + offset,
					    secs,
					    NULL, NULL);
		goto done;
#endif
	}

done:

	if (rval < 0) {
		putstr("set meta failed\r\n");
		rval = FLPROG_ERR_META_SET;
	}

	return rval;
}

#if defined(ENABLE_SD)
int restore_sparse_img(int pid, u8 *raw_image, smpart_t *smpart, int (*output_progress)(int, void *))
{
	u8 *cur_ptr;
	u8 *image;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	u32 chunk_cnt, left_sz, ssec, nsec, len = 0, percentage = 0;
	int part_size[TOTAL_FW_PARTS];

	cur_ptr = raw_image;
	sparse_header = (sparse_header_t *)cur_ptr;

	get_part_size(part_size);
	if ((sparse_header->total_blks * sparse_header->blk_sz) > part_size[pid]) {
		putstr(g_part_str[pid]);
		putstr(" length is bigger than partition\r\n");
		return FLPROG_ERR_LENGTH;
	}

	if(sparse_header->magic != SPARSE_HEADER_MAGIC){
		putstr("not a sparse partition!\n");
		return FLPROG_ERR_MAGIC;
	}

	cur_ptr += sparse_header->file_hdr_sz;// move to chunk header
	memset((u8 *)zero_buf, '\0', ZERO_BUF_SIZE);
	ssec = smpart->ssec;

	for(chunk_cnt = 1; chunk_cnt <= sparse_header->total_chunks; chunk_cnt++){
		chunk_header = (chunk_header_t *)cur_ptr;
		if(chunk_header->chunk_type == CHUNK_TYPE_RAW){ //RAW data
			nsec = (chunk_header->chunk_sz * sparse_header->blk_sz)/SM_SECTOR_SIZE;
			len = chunk_header->chunk_sz * sparse_header->blk_sz;
			image = cur_ptr + sparse_header->chunk_hdr_sz;
			sm_prog_sector_loop(image, len, ssec, nsec, NULL, NULL);
			ssec += nsec;
		} else if(chunk_header->chunk_type == CHUNK_TYPE_DONT_CARE){ //zero data
			left_sz = chunk_header->chunk_sz * sparse_header->blk_sz;
			while(left_sz > 0){
				if(left_sz >= ZERO_BUF_SIZE){// zero buffer > 16M
					nsec = ZERO_BUF_SIZE/SM_SECTOR_SIZE;
					len = ZERO_BUF_SIZE;
					image = (u8 *)zero_buf;
				} else {
					nsec = left_sz/SM_SECTOR_SIZE;
					len = left_sz;
					image = (u8 *)zero_buf;
				}
				sm_prog_sector_loop(image, len, ssec, nsec, NULL, NULL);
				left_sz -= len;
				ssec += nsec;
			}
		} else {
			putstr("chunk header err\r\n");
			return FLPROG_ERR_FIRM_FILE;
		}
		cur_ptr += chunk_header->total_sz;

		if (chunk_cnt > sparse_header->total_chunks)
			percentage = 100;
		else
			percentage = 100 * chunk_cnt / sparse_header->total_chunks;

		output_progress(percentage, NULL);
	}

	if((ssec - smpart->ssec) != smpart->nsec){
		putstr("sector number is not correct!\r\n");
		return FLPROG_ERR_LENGTH;
	}

	return FLPROG_OK;
}
#endif

/**
 * Program to a particular partition.
 */
int flprog_prog(int pid, u8 *image, unsigned int len)
{
	unsigned int boot_from = rct_boot_from();

	int rval = 0;
	partimg_header_t *header;
	u8 *raw_image;
#if !defined(NAND_FLASH_NONE)
	u32 sblk = 0, nblk = 0;
#endif
#if !defined(NOR_FLASH_NONE)
	u32 sblk = 0, nblk = 0;
#endif
#if defined(ENABLE_SD)
	smpart_t *smpart = NULL;
#endif

	header = (partimg_header_t *) image;
	raw_image = image + sizeof(partimg_header_t);
	if (pid >= TOTAL_FW_PARTS || pid == PART_PTB)
		return -1;

#if !defined(NAND_FLASH_NONE)
	flnand.image_flag = (header->flag & PART_INCLUDE_OOB)?
			IMAGE_HAS_OOB_DATA : IMAGE_HAS_NO_OOB_DATA;
#endif
	putstr("program ");
	putstr(g_part_str[pid]);

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0 ||
	    (g_part_dev[pid] == BOOT_DEV_SM)) {
		if ((boot_from & BOOT_FROM_SPI) && pid == PART_BST) {
			putstr(" to PROM ...\r\n");
			goto init_done;
		}

		putstr(" to SDMMC ...\r\n");
#if defined(ENABLE_SD)
		smpart = sm_get_part(pid);
		goto init_done;
#endif
	} else if ((boot_from & BOOT_FROM_SPI) && pid == PART_BST) {
		putstr(" to PROM ...\r\n");
		goto init_done;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		putstr(" to NAND flash ...\r\n");
		sblk = flnand.sblk[pid];
		nblk = flnand.nblk[pid];
		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		putstr(" to ONENAND flash ...\r\n");
		sblk = flnand.sblk[pid];
		nblk = flnand.nblk[pid];
		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && \
	(!defined(CONFIG_NOR_NONE) || !defined(CONFIG_SNOR_NONE)))
	} else if (boot_from & BOOT_FROM_NOR_FLASH) {
		putstr(" to NOR flash ...\r\n");
		sblk = flnor.sblk[pid];
		nblk = flnor.nblk[pid];
		goto init_done;
#endif
	} else {
		return -1;
	}

init_done:

	/* First, validate the image */
	rval = flprog_validate_image(image, len);
	if (rval < 0)
		return rval;

	if (pid == PART_BST) {
		if (boot_from & BOOT_FROM_SPI) {
#ifdef BUILD_AMBPROM
			rval = eeprom_prog(EEPROM_ID_DEV0, image,
						len, output_progress, NULL);
			goto done;
#else
			return -1;
#endif
		} else {
			/* Special check for BST which must fit in 2KB for FIO FIFO */
			if (header->img_len > AMBOOT_BST_FIXED_SIZE) {
				putstr("bst length is bigger than 2048\r\n");
				putstr("It may be caused by nand and nor drivers "
				       "compiled into bst at the same time\r\n");
				return FLPROG_ERR_LENGTH;
			}
		}
	} else {
		int part_size[TOTAL_FW_PARTS];
		get_part_size(part_size);
		if (header->img_len > part_size[pid]) {
			putstr(g_part_str[pid]);
			putstr(" length is bigger than partition\r\n");
			return FLPROG_ERR_LENGTH;
		}
	}

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0 ||
	    (g_part_dev[pid] == BOOT_DEV_SM)) {
#if defined(ENABLE_SD)
		if (pid == PART_BST)
			sdmmc_prog_bootp();
		if(header->flag&PART_SPARSE_EXT){
			rval = restore_sparse_img(pid, raw_image, smpart, output_progress);
		}else{
			rval = sm_prog_sector_loop(raw_image,
				   header->img_len,
				   smpart->ssec,
				   smpart->nsec,
				   output_progress,
				   NULL);
		}
		if (pid == PART_BST)
			sdmmc_prog_accp();
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		if (pid == PART_BST)
			rval = nand_prog_block_loop_ns(raw_image,
						       header->img_len,
						       sblk,
						       nblk,
						       output_progress,
						       NULL);
		else
			rval = nand_prog_block_loop(raw_image,
						    header->img_len,
						    sblk,
						    nblk,
						    output_progress,
						    NULL);
		if (pid == PART_ROM || pid == PART_DSP)
			nand_partition_bb_scan(pid, header->img_len);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		if (pid == PART_BST)
			rval = onenand_prog_block_loop_ns(raw_image,
							  header->img_len,
							  sblk,
							  nblk,
							  output_progress,
							  NULL);
		else
			rval = onenand_prog_block_loop(raw_image,
						       header->img_len,
						       sblk,
						       nblk,
						       output_progress,
						       NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		rval = nor_prog_sector_loop(raw_image,
					    header->img_len,
					    sblk,
					    nblk,
					    output_progress,
					    NULL);
		goto done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {
		rval = snor_prog_sector_loop(raw_image,
					    header->img_len,
					    sblk,
					    nblk,
					    output_progress,
					    NULL);
		goto done;
#endif
	} else {
		rval = -1;
	}

done:

	/* Update the PTB's entry */
	if ((rval == 0) && (pid != PART_BST)) {
		flpart_table_t ptb;
		flpart_t part;
		int i, sanitize = 0;

		part.crc32 =	header->crc32;
		part.ver_num =	header->ver_num;
		part.ver_date =	header->ver_date;
		part.img_len =	header->img_len;
		part.mem_addr =	header->mem_addr;
		part.flag =	header->flag;
		part.magic =	FLPART_MAGIC;

		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			putstr("Can't get ptb, erase it\r\n");
		}

		/*
		 * Check magic against all partitions, sanitize the whole
		 * PTB on any mismatch!
		 */
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			if (ptb.part[i].magic != FLPART_MAGIC) {
				sanitize = 1;
				break;
			}
		}

		if (sanitize) {
			putstr("PTB on flash is outdated, sanitizing ...\r\n");
			memset(&ptb, 0x0, sizeof(ptb));

			for (i = 0; i < HAS_IMG_PARTS; i++) {
				ptb.part[i].magic = FLPART_MAGIC;
			}
		}

		/* Write PTB */
		memcpy(&ptb.part[pid], &part, sizeof(flpart_t));

		flprog_get_dev_param(&ptb);
		rval = flprog_set_part_table(&ptb);
	}

	/* Erase Android partitions (ADD, ADC). */
	if (rval == 0 && (header->flag & PART_ERASE_APARTS)) {
		rval = flprog_erase_partition(PART_ADD);
		rval |= flprog_erase_partition(PART_ADC);
		if (rval != 0)
			putstr("Erase Android partitions failed\r\n");
	}

	return rval;
}

/******************************/
/* partition erase functions. */
/******************************/

/**
 * Erase a partition.
 */
int flprog_erase_partition(int pid)
{
	unsigned int boot_from = rct_boot_from();

	int rval = 0;
#if !defined(CONFIG_NAND_NONE) || !defined(CONFIG_ONENAND_NONE)
	u32 sblk = 0, nblk = 0;
	u32 block;
	nand_mp_t mp;
#endif
#if !defined(NOR_FLASH_NONE)
	u32 sblk = 0, nblk = 0;
	u32 sector;
#endif
#if defined(ENABLE_SD)
	int i;
	u32 smssec = 0, smnsec = 0;
	u32 smssec2 = 0, smnsec2 = 0;
	u32 p, wsecs;
#ifdef ENABLE_EMMC_BOOT
	u32 max_wsecs = 2 * 1024 * 128;		// Erase 128MB once
#else
	u32 max_wsecs = 256;
#endif
	smpart_t *smpart = NULL;
	smdev_t *smdev = NULL;
#endif

	putstr("erase ");
	putstr(g_part_str[pid]);
	putstr(" ... \r\n");

	if (pid < PART_MAX) {
		switch(get_part_dev(pid)) {
		case BOOT_DEV_NAND:
		case BOOT_DEV_ONENAND:
			boot_from = BOOT_FROM_NAND_FLASH;
			break;

		case BOOT_DEV_NOR:
		case BOOT_DEV_SNOR:
			boot_from = BOOT_FROM_NOR_FLASH;
			break;

		case BOOT_DEV_SM:
			boot_from = BOOT_FROM_SDMMC;
			break;

		default:
			break;
		}
	}

	if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
#if defined(BUILD_AMBPROM)
		if ((boot_from & BOOT_FROM_SPI) &&
		    (pid == PART_BST || pid == PART_MAX)) {
			/* Only erase first 2048 bytes of eeprom */
			/* due to low speed of eeprom. */
			putstr("stage 1: eeprom ...\r\n");
			rval = eeprom_erase(EEPROM_ID_DEV0, 0, 2048);
			output_progress(100, NULL);
			if (rval < 0) {
				putstr("\r\nfailed\r\n");
			} else {
				putstr("\r\ndone\r\n");
			}
		}
#endif

#if defined(ENABLE_SD)
		if (pid == PART_MAX) {
			smdev = sm_get_dev();
			/* 1. Partitions start from the top of devcie. */
			smssec = 0;
			for (i = 0; i < PART_STG; i++) {
				smnsec += smdev->part[i].nsec;
			}

			/* 2. Partitions start from the bottom of devcie. */
			smpart = sm_get_part(PART_PRF);
			smssec2 = smpart->ssec;
			smnsec2 = smpart->nsec;

			smpart = sm_get_part(PART_CAL);
			if (smnsec2 == 0)  {
				smssec2 = smpart->ssec;
				smnsec2 = smpart->nsec;
			} else {
				smnsec2 += smpart->nsec;
			}
		} else {
			smpart = sm_get_part(pid);
		}

		if (pid == PART_BST || pid == PART_MAX)
			putstr("stage 2: ");

		putstr("sector-media...\r\n");

		if (pid == PART_MAX) {
			u32 tsecs = smnsec + smnsec2;

			putstr("erase all partitions in sector-media "
				"except STG partition\r\n");
			for (i = 0; i < smnsec; i += max_wsecs) {
				if ((smnsec - i) > max_wsecs)
					wsecs = max_wsecs;
				else
					wsecs = (smnsec - i);
				rval = sm_erase_sector(smssec + i, wsecs);
				if (rval < 0) {
					putstr("failed!\r\n");
				}

				if ((i + wsecs) == tsecs)
					p = 100;
				else
					p = (i + wsecs) * 100 / tsecs;
				output_progress(p, NULL);
			}

			for (i = 0; i < smnsec2; i += max_wsecs) {
				if ((smnsec2 - i) > max_wsecs)
					wsecs = max_wsecs;
				else
					wsecs = (smnsec2 - i);
				rval = sm_erase_sector(smssec2 + i, wsecs);
				if (rval < 0) {
					putstr("failed!\r\n");
				}

				if ((i + wsecs + smnsec) == tsecs)
					p = 100;
				else
					p = (i + wsecs + smnsec) * 100 / tsecs;
				output_progress(p, NULL);
			}
		} else {
			smssec = smpart->ssec;
			smnsec = smpart->nsec;

			for (i = 0; i < smnsec; i += max_wsecs) {
				if ((smnsec - i) > max_wsecs)
					wsecs = max_wsecs;
				else
					wsecs = (smnsec - i);
				rval = sm_erase_sector(smssec + i, wsecs);
				if (rval < 0) {
					putstr("failed!\r\n");
				}

				if ((i + wsecs) == smnsec)
					p = 100;
				else
					p = (i + wsecs) * 100 / smnsec;
				output_progress(p, NULL);
			}
		}

		putstr("\r\n");
		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	} else if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		nand_get_media_part(&mp);

		if (pid == PART_MAX) {
			sblk = 0;
			nblk = flnand.blocks_per_bank * flnand.banks;
		} else if (pid >= TOTAL_FW_PARTS) {
			sblk = mp.sblk[pid - TOTAL_FW_PARTS];
			nblk = mp.nblk[pid - TOTAL_FW_PARTS];
		} else {
			sblk = flnand.sblk[pid];
			nblk = flnand.nblk[pid];
		}

		for (block = sblk; block < (sblk + nblk); block++) {
			rval = nand_is_bad_block(block);
			if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
				output_bad_block(block, rval);
				continue;
			}

			rval = nand_erase_block(block);
			if (rval < 0) {
				nand_mark_bad_block(block);
				putstr(" failed! <block ");
				putdec(block);
				putstr(">\r\n");
			}
		}

		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		onenand_get_media_part(&mp);

		if (pid == PART_MAX) {
			sblk = 0;
			nblk = flnand.blocks_per_bank * flnand.banks;
		} else if (pid >= TOTAL_FW_PARTS) {
			sblk = mp.sblk[pid - TOTAL_FW_PARTS];
			nblk = mp.nblk[pid - TOTAL_FW_PARTS];
		} else {
			sblk = flnand.sblk[pid];
			nblk = flnand.nblk[pid];
		}

		for (block = sblk; block < (sblk + nblk); block++) {
			rval = onenand_is_bad_block(block);
			if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
				output_bad_block(block, rval);
				continue;
			}

			rval = onenand_erase_block(block);
			if (rval < 0) {
				putstr(" failed! <block ");
				putdec(block);
				putstr(">\r\n");
			}
		}

		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if (boot_from & BOOT_FROM_NOR) {

		if (pid == PART_RAW || pid == PART_STG ||
		    pid == PART_PRF || pid == PART_CAL) {
			putstr("Not implement yet!\r\n");
			return -1;
		} else if(pid == PART_MAX) {
			ssec = 0;
			nsec = NOR_TOTAL_SECTORS;
		} else {
			sblk = flnor.sblk[pid];
			nblk = flnor.nblk[pid];
		}

		if (boot_from & BOOT_FROM_NOR) {
			for (sector = ssec; sector < (ssec + nsec);  sector++) {
				rval = nor_unlock(sector);
				rval += nor_erase_sector(sector);
				rval += nor_lock(sector);
				if (rval < 0) {
					putstr("failed! <sector ");
					putdec(sector);
					putstr(">\r\n");
				}
			}
		}
		goto init_done;
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if (boot_from & BOOT_FROM_SNOR) {

		if (pid == PART_RAW || pid == PART_STG ||
		    pid == PART_PRF || pid == PART_CAL) {
			putstr("Not implement yet!\r\n");
			return -1;
		} else if(pid == PART_MAX) {
			sblk = 0;
			nblk = NOR_TOTAL_SECTORS;
		} else {
			sblk = flnor.sblk[pid];
			nblk = flnor.nblk[pid];
		}

		if (boot_from & BOOT_FROM_SNOR) {
			for (sector = sblk; sector < (sblk + nblk);  sector++) {
				rval = snor_erase_block(sector);
				if (rval < 0) {
					putstr("failed! <sector ");
					putdec(sector);
					putstr(">\r\n");
				}
			}
		}

		goto init_done;
#endif
	} else {
		rval = -1;
	}

init_done:

	/* Update the PTB's entry */
	if (rval == 0 && pid != PART_MAX && pid != PART_PTB) {
		flpart_table_t ptb;
		flpart_t part;

		part.crc32 =	0xffffffff;
		part.ver_num =	0xffffffff;
		part.ver_date =	0xffffffff;
		part.img_len =	0xffffffff;
		part.mem_addr =	0xffffffff;
		part.flag =	0xffffffff;
		part.magic =	0xffffffff;

		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			putstr("Can't get ptb, erase it\r\n");
		}

		memcpy(&ptb.part[pid], &part, sizeof(flpart_t));

		rval = flprog_set_part_table(&ptb);
		if (rval < 0)
			putstr("unable to update PTB!\r\n");
	}
#if !defined(CONFIG_NAND_NONE)
	if (flnand.nblk[PART_BST] && rval >= 0)
		putstr("done\r\n");
#endif
#if !defined(CONFIG_NOR_NONE) || !defined(CONFIG_SNOR_NONE)
	if (flnor.nblk[PART_BST] && rval >= 0)
		putstr("done\r\n");
#endif
#if defined(ENABLE_SD)
	smpart = sm_get_part(PART_BST);
	if (smpart->nsec && rval >= 0)
		putstr("done\r\n");
#endif

	return rval;
}

#endif  /* !CONFIG_NAND_NONE || !CONFIG_NOR_NONE */

#endif  /* ENABLE_FLASH */

/*---------------------------------------------------------------------------*/
/**
 * Output header information of partition image .
 * @param header - The partition header.
 * @param len - The size of image.
 */
void output_header(partimg_header_t *header, u32 len)
{
	uart_putstr("\tcrc32:\t\t0x");
	uart_puthex(header->crc32);
	uart_putstr("\r\n");
	uart_putstr("\tver_num:\t");
	uart_putdec(header->ver_num >> 16);
	uart_putstr(".");
	uart_putdec(header->ver_num & 0xffff);
	uart_putstr("\r\n");
	uart_putstr("\tver_date:\t");
	uart_putdec(header->ver_date >> 16);
	uart_putstr("/");
	uart_putdec((header->ver_date >> 8) & 0xff);
	uart_putstr("/");
	uart_putdec(header->ver_date & 0xff);
	uart_putstr("\r\n");
	uart_putstr("\timg_len:\t");
	uart_putdec(header->img_len);
	uart_putstr("\r\n");
	uart_putstr("\tmem_addr:\t0x");
	uart_puthex(header->mem_addr);
	uart_putstr("\r\n");
	uart_putstr("\tflag:\t\t0x");
	uart_puthex(header->flag);
	uart_putstr("\r\n");
	uart_putstr("\tmagic:\t\t0x");
	uart_puthex(header->magic);
	uart_putstr("\r\n");
}

void output_failure(int errcode)
{
	if (errcode == 0) {
		uart_putstr("\r\n");
		uart_putstr(FLPROG_ERR_STR[errcode]);
		uart_putstr("\r\n");
	} else if (errcode < 0) {
		uart_putstr("\r\nfailed: (");
		uart_putstr(FLPROG_ERR_STR[-errcode]);
		uart_putstr(")!\r\n");
	} else {
		uart_putstr("\r\nfailed: (");
		uart_putstr(FLPROG_ERR_STR[errcode]);
		uart_putstr(")!\r\n");
	}
}

int output_progress(int percentage, void *arg)
{
	putstr("progress: ");
	putdec(percentage);
	putstr("%\r");

	return 0;
}

void output_report(const char *name, u32 flag)
{
	if ((flag & FWPROG_RESULT_FLAG_LEN_MASK) == 0)
		return;

	putstr(name);
	putstr(":\t");

	if ((flag & FWPROG_RESULT_FLAG_CODE_MASK) == 0)
		putstr("success\r\n");
	else
		putstr("*** FAILED! ***\r\n");
}

void output_bad_block(u32 block, int bb_type)
{
	if (bb_type & NAND_INITIAL_BAD_BLOCK)
		putstr("initial bad block. <block ");
	else if (bb_type & NAND_LATE_DEVEL_BAD_BLOCK)
		putstr("late developed bad block. <block ");
	else
		putstr("other bad block. <block ");

	putdec(block);
	putstr(">\r\n");
	putstr("Try next block...\r\n");
}

int prog_firmware(u32 addr, u32 len, u32 flag)
{
	int rval = 0;
	u8 code;
	u8 *image;
	devfw_header_t *header;
	int payload_len[HAS_IMG_PARTS];
	int part_size[TOTAL_FW_PARTS];
	int i;
	u8 buffer[512] __attribute__ ((aligned(8)));
	unsigned int boot_from = rct_boot_from();
	fwprog_result_t result;

	memzero(buffer, sizeof(buffer));
	memzero(&result, sizeof(result));
	result.magic = FWPROG_RESULT_MAGIC;

	get_part_size(part_size);

	/* Turn off VT100 insertion mode */
	putstr("\x1b[4l");	/* Set terminal to replacement mode */

	header = (devfw_header_t *) addr;

	/* Calculate the firmware payload offsets of images */
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (i == PART_PTB)
			continue;

		payload_len[i] = header->end_image[i] - header->begin_image[i];

		putstr(g_part_str[i]);
		putstr(" code found in firmware!\r\n");

		/* Check the code size is smaller than the header structure or
		 * end pointer over the code range. */
		if ((payload_len[i] < sizeof(partimg_header_t)) ||
		    (header->end_image[i] > len)) {
			putstr(g_part_str[i]);
			putstr(" code is not correct!\r\n");
			rval--;
		}
	}

	if (rval < 0) {
		return rval;
	}

	i = PART_BST;
	if (payload_len[i] > 0) {
		if ((boot_from & BOOT_FROM_FLASH) ||
		    (boot_from & BOOT_FROM_SPI)) {
			image = (u8 *) addr + header->begin_image[i];
			output_header((partimg_header_t *) image,
					payload_len[i]);
			rval = flprog_prog(PART_BST, image, payload_len[i]);
			if (rval == 0 && (boot_from & BOOT_FROM_SPI))
				goto cre_param;
			else
				goto done;
		} else if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
cre_param:
			/* save parameters to sector 1 */
			rval = create_parameter(PART_BLD,
					(struct sdmmc_header_s *)buffer);
			if (rval < 0)
				goto done;
			putstr("program sm boot parameters\r\n");
			rval = sm_write_sector(1, buffer, sizeof(buffer));
			if (rval == 0) {
				putstr("done\r\n");
			} else {
				putstr("failed\r\n");
			}
			goto done;
		} else {
			rval = -1;
		}
done:
		code = rval < 0 ? -rval : rval;
		result.flag[i] = FWPROG_RESULT_MAKE(code, payload_len[i]);
		output_failure(rval);
	}

	for (i = PART_BLD; i < HAS_IMG_PARTS; i++) {
		if (payload_len[i] > 0) {
			if (payload_len[i] > part_size[i]) {
				rval = FLPROG_ERR_LENGTH;
			} else {
				image = (u8 *) addr + header->begin_image[i];
				output_header((partimg_header_t *)
						image, payload_len[i]);
				rval = flprog_prog(i, image, payload_len[i]);
			}
			code = rval < 0 ? -rval : rval;
			result.flag[i] = FWPROG_RESULT_MAKE(code,
							    payload_len[i]);
			output_failure(rval);
		}
	}

	/* Output program message */
	putstr("\r\n------ Report ------\r\n");

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		output_report(g_part_str[i], result.flag[i]);
	}

	putstr("\r\n\t- Program Terminated -\r\n");

	return 0;
}
