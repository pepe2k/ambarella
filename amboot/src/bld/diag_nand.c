/**
 * system/src/bld/diag_nand.c
 *
 * History:
 *    2005/09/21 - [Chien-Yang Chen] created file
 *    2007/10/11 - [Charles Chiou] Added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/ftl_const.h>

#if !defined(CONFIG_NAND_NONE)

extern void output_part(const char *s, const flpart_t *part);
extern int rand(void);
extern void output_bad_block(u32 block, int bb_type);

static int nand_verify_data(u8* origin, u8* data, u32 len)
{
	int err_cnt = 0;
	u32 i;

	for (i = 0; i < len; i++) {
		if (origin[i] != data[i]){
			err_cnt++;
		}
	}

	return err_cnt;
}

void diag_nand_read(void)
{
	u32 start_block, block, blocks;
	u32 block_size;
	u8 *rbuf;
	int i, rval,c=0;

	putstr("running nand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnand.nblk[i];
	}

	blocks = flnand.blocks_per_bank * flnand.banks - start_block;
	block_size = flnand.pages_per_block * flnand.main_size;

	rbuf = (u8*) (g_hugebuf_addr +
			flnand.pages_per_block * flnand.main_size);

	for (i = 0, block = start_block; i < blocks; i++, block++) {

		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}

		rval = nand_read_pages(block, 0, flnand.pages_per_block, rbuf, NULL, 1);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}

		if (rval < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}
	}

	putstr("\r\ndone!\r\n");
}

void diag_nand_read_speed(void)
{
	u32 start_block, block, blocks;
	u32 block_size, read_size = 0, time, speed;
	u8 *rbuf;
	int rval;

	putstr("running nand speed test ...\r\n");
	putstr("press any key to terminate, or read the entire nand!\r\n");

	start_block = 0;

	blocks = flnand.blocks_per_bank * flnand.banks - start_block;
	block_size = flnand.pages_per_block * flnand.main_size;

	rbuf = (u8*) (g_hugebuf_addr +
			flnand.pages_per_block * flnand.main_size);

	timer_reset_count(TIMER2_ID);
	timer_enable(TIMER2_ID);

	for (block = start_block; block < start_block + blocks; block++) {

		if (uart_poll())
			break;

		if (nand_is_bad_block(block))
			continue;

		rval = nand_read_pages(block, 0, flnand.pages_per_block, rbuf, NULL, 1);

		if (rval < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		} else {
			read_size += block_size;
		}
	}

	time = timer_get_count(TIMER2_ID);	// millisecond
	timer_disable(TIMER2_ID);

	speed = read_size / (time / 1000);	// Avoid overflow

	putstr("\r\nTotally read ");
	putdec(read_size);
	putstr(" Bytes, and speed is ");
	putdec(speed);
	putstr(" B/s!\r\n\r\n");
}

void diag_nand_prog(void)
{
	u32 start_block, block, blocks;
	u32 block_size;
	u8 *wbuf;
	int i, rval,c=0;

	putstr("running nand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnand.nblk[i];
	}

	blocks = flnand.blocks_per_bank * flnand.banks - start_block;
	block_size = flnand.pages_per_block * flnand.main_size;

	wbuf = (u8 *) g_hugebuf_addr;

	for(i = 0; i < block_size; i++)
		wbuf[i] = i;

	putstr("\r\nstart to program blocks\r\n");
	for (i = 0, block = start_block; i < blocks; i++, block++) {

		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}
		rval = nand_prog_pages(block, 0, flnand.pages_per_block, wbuf, NULL);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}

		if (rval < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}

	}
	putstr("\r\ndone!\r\n");
}

/**
 * Run diagnostic on NAND: test all regions pass the R/W test.
 * @returns - total bad block number
 */
u32 diag_nand_verify(int argc, char *argv[])
{
	u32 start_block, block, blocks, page;
	u32 block_size, spa_size;
	u32 enable_ecc;
	u32 bb_num = 0;
	u8 *wbuf, *rbuf, *swbuf, *srbuf;
	int i, rval;

	putstr("running nand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnand.nblk[i];
	}

	blocks = flnand.blocks_per_bank * flnand.banks - start_block;
	block_size = flnand.pages_per_block * flnand.main_size;
	spa_size = flnand.pages_per_block * flnand.spare_size;

	wbuf = (u8*) g_hugebuf_addr;
	rbuf = (u8*) (g_hugebuf_addr + flnand.pages_per_block * flnand.main_size);
	swbuf = (u8*) (g_hugebuf_addr +
		       flnand.pages_per_block * flnand.main_size * 2);
	srbuf = (u8*) (g_hugebuf_addr +
		       flnand.pages_per_block * flnand.main_size * 2 +
		       flnand.pages_per_block * flnand.spare_size);

	for (i = 0; i < block_size; i++)
		wbuf[i] = rand() / 256;

	for (i = 0; i < spa_size; i++)
		swbuf[i] = rand() / 256;

	if (strcmp(argv[0], "boot") == 0) {
		blocks = start_block;
		start_block = 0;
	} else if (strcmp(argv[0], "storage") == 0) {
		/* do nothing */
	} else if (strcmp(argv[0], "all") == 0) {
		blocks += start_block;
		start_block = 0;
	} else if (strcmp(argv[0], "verify") == 0) {
		/* do nothing */
	} else
		return 0;

	if (strcmp(argv[1], "no_ecc") == 0)
		enable_ecc = 0;
	else
		enable_ecc = 1;

	for (i = 0, block = start_block; i < blocks; i++, block++) {

		if (uart_poll())
			break;

		rval = nand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			bb_num ++;
			continue;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = nand_prog_pages(block, 0, flnand.pages_per_block, wbuf, NULL);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nand_prog_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = nand_read_pages(block, 0, flnand.pages_per_block, rbuf, NULL,
				       enable_ecc);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nand_read_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		for (page = 0; page < flnand.pages_per_block; page++) {
			rval = nand_prog_spare(block, page, 1, swbuf);
			if (rval < 0) {
				nand_mark_bad_block(block);
				putstr("\r\nand_prog_spare failed\r\n");
				bb_num ++;
				goto done;
			}

			rval = nand_read_spare(block, page, 1, srbuf);
			if (rval < 0) {
				nand_mark_bad_block(block);
				putstr("\r\nand_read_spare failed\r\n");
				bb_num ++;
				goto done;
			}

			swbuf += flnand.spare_size;
			srbuf += flnand.spare_size;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = nand_verify_data(wbuf, rbuf, block_size);
		if (rval != 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand verify data error count ");
			putdec(rval);
			rval = -1;
			bb_num ++;
			goto done;
		}

		swbuf -= spa_size;
		srbuf -= spa_size;

		rval = nand_verify_data(swbuf, srbuf, spa_size);
		if (rval != 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand verify data error count ");
			putdec(rval);
			rval = -1;
			bb_num ++;
			goto done;
		}

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}

done:
		if (rval < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}
	}

	putstr("\r\ntotal bad blocks: ");
	putdec(bb_num);
	putstr("\r\n");

	putstr("\r\ndone!\r\n");

	return bb_num;
}

void diag_nand_rclm_bb(int argc, char *argv[])
{
	u32 block, blocks, page;
	u32 block_size, spa_size;
	u8 *wbuf, *rbuf, *swbuf, *srbuf;
	int i, rval, bb_type,c=0;

	blocks = flnand.blocks_per_bank * flnand.banks;
	block_size = flnand.pages_per_block * flnand.main_size;
	spa_size = flnand.pages_per_block * flnand.spare_size;

	wbuf = (u8*) g_hugebuf_addr;
	rbuf = (u8*) (g_hugebuf_addr + flnand.pages_per_block * flnand.main_size);
	swbuf = (u8*) (g_hugebuf_addr +
		       flnand.pages_per_block * flnand.main_size * 2);
	srbuf = (u8*) (g_hugebuf_addr +
		       flnand.pages_per_block * flnand.main_size * 2 +
		       flnand.pages_per_block * flnand.spare_size);

	for (i = 0; i < block_size; i++)
		wbuf[i] = rand() / 256;

	for (i = 0; i < spa_size; i++)
		swbuf[i] = rand() / 256;

	if (strcmp(argv[0], "init") == 0) {
		bb_type = NAND_INITIAL_BAD_BLOCK;
	} else if (strcmp(argv[0], "late") == 0) {
		bb_type = NAND_LATE_DEVEL_BAD_BLOCK;
	} else if (strcmp(argv[0], "other") == 0) {
		bb_type = NAND_OTHER_BAD_BLOCK;
	} else if (strcmp(argv[0], "all") == 0) {
		bb_type = NAND_INITIAL_BAD_BLOCK |
			  NAND_LATE_DEVEL_BAD_BLOCK |
			  NAND_OTHER_BAD_BLOCK;
	} else {
		return;
	}

	putstr("\r\n Start to reclaim bad blocks...\r\n");

	for (block = 0; block < blocks; block++) {

		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}

		rval = nand_is_bad_block(block);
		if ((rval & bb_type)== 0) {
			continue;
		}

		/* Start to reclaim bad block. */
		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		rval = nand_prog_pages(block, 0, flnand.pages_per_block, wbuf, NULL);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_prog_pages failed\r\n");
			goto done;
		}

		rval = nand_read_pages(block, 0, flnand.pages_per_block, rbuf, NULL, 1);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_pages failed\r\n");
			goto done;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		for (page = 0; page < flnand.pages_per_block; page++) {
			rval = nand_prog_spare(block, page, 1, swbuf);
			if (rval < 0) {
				nand_mark_bad_block(block);
				putstr("nand_prog_spare failed\r\n");
				goto done;
			}

			rval = nand_read_spare(block, page, 1, srbuf);
			if (rval < 0) {
				nand_mark_bad_block(block);
				putstr("nand_read_spare failed\r\n");
				goto done;
			}

			swbuf += flnand.spare_size;
			srbuf += flnand.spare_size;
		}

		rval = nand_erase_block(block);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		rval = nand_verify_data(wbuf, rbuf, block_size);
		if (rval != 0) {
			nand_mark_bad_block(block);
			putstr("nand verify data error count\r\n");
			putdec(rval);
			rval = -1;
			goto done;
		}

		swbuf -= spa_size;
		srbuf -= spa_size;

		rval = nand_verify_data(swbuf, srbuf, spa_size);
		if (rval != 0) {
			nand_mark_bad_block(block);
			putstr("nand verify data error count\r\n");
			putdec(rval);
			rval = -1;
			goto done;
		}

done:
		if (rval == 0) {
			putstr("block ");
			putdec(block);
			putstr(" is a good block\r\n");
#if defined(NAND_SUPPORT_BBT)
			nand_update_bbt(0, block);
#endif
		} else {
			putstr("\r\nblock ");
			putdec(block);
			putstr(" is a bad block\r\n");
		}
	}

	putstr("done!\r\n");
}

void diag_nand_dump(int argc, char *argv[])
{
	u32 page_addr, pages, total_pages;
	u32 block, page;
	u32 i, j, enable_ecc;
	u8 *rbuf, *srbuf;
	int rval = 0;

	total_pages = flnand.pages_per_block * flnand.blocks_per_bank * flnand.banks;

	if (strtou32(argv[0], &page_addr) < 0) {
		putstr("invalid page address!\r\n");
		rval = -1;
		goto done;
	}

	if (argc == 1 || strtou32(argv[1], &pages) < 0)
		pages = 1;

	if (page_addr + pages > total_pages) {
		putstr("page_addr = 0x");
		puthex(page_addr);
		putstr(", pages = 0x");
		puthex(pages);
		putstr(", Overflow!\r\n");
		rval = -1;
		goto done;
	}

	if (strcmp(argv[1], "no_ecc") == 0 || strcmp(argv[2], "no_ecc") == 0)
		enable_ecc = 0;
	else
		enable_ecc = 1;

	rbuf = (u8*)g_hugebuf_addr;
	srbuf = (u8*)(g_hugebuf_addr + flnand.main_size);

	for (i = 0; i < pages; i++) {
		block = (page_addr + i) / flnand.pages_per_block;
		page = (page_addr + i) % flnand.pages_per_block;

		if (nand_is_bad_block(block)) {
			putstr("block ");
			putdec(block);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = nand_read_pages(block, page, 1, rbuf, NULL, enable_ecc);
		if (rval == 1) {
			putstr("BCH code corrected (0x");
			puthex(block);
			putstr(")!\n\r");
		} else if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_pages failed\r\n");
			goto done;
		}

		putstr("PAGE[");
		putdec(page_addr + i);
		putstr("] main data:\r\n");
		for (j = 0; j < flnand.main_size; j++) {
			putstr(" ");
			putbyte(rbuf[j]);
			if ((j+1) % 32 == 0)
				putstr("\r\n");
		}
		putstr("\r\n");

		rval = nand_read_spare(block, page, 1, srbuf);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_spare failed\r\n");
			goto done;
		}

		putstr("PAGE[");
		putdec(page_addr + i);
		putstr("] spare data:\r\n");
		for (j = 0; j < flnand.spare_size; j++) {
			putstr(" ");
			putbyte(srbuf[j]);
			if ((j+1) % 32 == 0)
				putstr("\r\n");
		}
		putstr("\r\n");
	}

done:
	putstr("done!\r\n");
}

void diag_nand_write(int argc, char *argv[])
{
	u32 page_addr, pages, total_pages;
	u32 block, page, i;
	u32 enable_ecc, bitflip;
	u8 *wbuf;
	int rval = 0;

	total_pages = flnand.pages_per_block * flnand.blocks_per_bank * flnand.banks;

	if (strtou32(argv[0], &page_addr) < 0) {
		putstr("invalid page address!\r\n");
		rval = -1;
		goto done;
	}

	if (argc == 1 || strtou32(argv[1], &pages) < 0)
		pages = 1;

	if (page_addr + pages > total_pages) {
		putstr("page_addr = 0x");
		puthex(page_addr);
		putstr(", pages = 0x");
		puthex(pages);
		putstr(", Overflow!\r\n");
		rval = -1;
		goto done;
	}

	if (strcmp(argv[1], "no_ecc") == 0) {
		enable_ecc = 0;
		if (argc > 2 && strtou32(argv[2], &bitflip) < 0) {
			rval = -1;
			goto done;
		}
	} else if (strcmp(argv[2], "no_ecc") == 0) {
		enable_ecc = 0;
		if (argc > 3 && strtou32(argv[3], &bitflip) < 0) {
			rval = -1;
			goto done;
		}
	} else {
		enable_ecc = 1;
		bitflip = 0;
	}

	wbuf = (u8*)g_hugebuf_addr;
	memset(wbuf, 0xff, flnand.main_size * pages);
	if (enable_ecc == 0) {
		/* create bitflip manually on the first bytes data */
		for (i = 0; i < bitflip; i++)
			wbuf[0] &= ~(1 << i);
	}

	for (i = 0; i < pages; i++) {
		block = (page_addr + i) / flnand.pages_per_block;
		page = (page_addr + i) % flnand.pages_per_block;

		if (nand_is_bad_block(block)) {
			putstr("block ");
			putdec(block);
			putstr(" is a bad block\r\n");
			continue;
		}

		if (enable_ecc) {
			rval = nand_prog_pages(block, page, 1,
					wbuf + flnand.main_size * i, NULL);
			if (rval < 0) {
				putstr("block ");
				putdec(block);
				putstr(": nand_erase_block failed\r\n");
				goto done;
			}
		} else {
			rval = nand_prog_pages_noecc(block, page, 1,
					wbuf + flnand.main_size * i);
			if (rval < 0) {
				putstr("block ");
				putdec(block);
				putstr(": nand_erase_block failed\r\n");
				goto done;
			}
		}
	}

done:
	putstr("done!\r\n");
}

extern const u32 crc32_tab[];

static int check_part_crc(u32 pid, const flpart_t *part)
{
	int rval;
	u8 *buf, *p;
	u32 size, crc, block, page;
	int i;

	buf = (u8*) g_hugebuf_addr;

	if (part->img_len == 0 || part->img_len == 0xffffffff) {
		rval = -1;
		goto done;
	}

	if (part->magic != FLPART_MAGIC) {
		rval = -1;
		goto done;
	}

	if (pid >= HAS_IMG_PARTS) {
		rval = -1;
		goto done;
	}

	/* Get partition block start address */
	block = flnand.sblk[pid];

	while (nand_is_bad_block(block)) {
		block++;
	}
	page = 0;

	/* Start CRC32 caculation */
	crc = ~0U;
	for (i = 0; i < part->img_len; i += flnand.main_size) {
		if (part->img_len - i > flnand.main_size)
			size = flnand.main_size;
		else
			size = part->img_len - i;

		rval = nand_read_pages(block, page, 1, buf, NULL, 1);
		if (rval < 0) {
			rval = -1;
			goto done;
		}

		page++;
		if (page / flnand.pages_per_block > 0) {
			block++;
			page = 0;
			while (nand_is_bad_block(block)) {
				block++;
			}
		}

		p = buf;
		 while (size > 0) {
        	        crc = crc32_tab[(crc ^ *p++) & 0xff] ^ (crc >> 8);
			size--;
		}
	}

	crc ^= ~0U;

	/* Check if CRC is correct */
	if (part->crc32 == crc) {
		rval = 0;
	} else {
		rval = -1;
	}

done:

	return rval;
}

int diag_nand_partition(char *s, u32 times)
{
	flpart_table_t ptb;
	int rval, i, j,c=0;
	int ok_flag[HAS_IMG_PARTS];
	int ng_flag[HAS_IMG_PARTS];
	int do_flag[HAS_IMG_PARTS];
	int pid = -1;

	memset(ok_flag, 0x0, sizeof(ok_flag));
	memset(ng_flag, 0x0, sizeof(ng_flag));
	memset(do_flag, 0x0, sizeof(do_flag));

	putstr("running nand stress test ...\r\n");
	putstr("press space to terminate!\r\n");

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		return rval;

	/* Display PTB content */
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (strcmp(s, g_part_str[i]) == 0) {
			pid = i;
			break;
		}
	}
	if (strcmp(s, "all") == 0)
		pid = HAS_IMG_PARTS;
	else if (pid == -1)
		return -1;

	if (pid < HAS_IMG_PARTS) {
		output_part(g_part_str[pid], &ptb.part[pid]);
		if (ptb.part[pid].img_len != 0 &&
		    ptb.part[pid].img_len != 0xffffffff)
			do_flag[pid] = 1;

	} else {
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			output_part(g_part_str[i], &ptb.part[i]);
			if (ptb.part[i].img_len != 0 &&
			    ptb.part[i].img_len != 0xffffffff)
				do_flag[i] = 1;
		}
	}

	for (i = 0; i < times; i++) {
		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}
		for (j = 0; j < HAS_IMG_PARTS; j++) {
			if (do_flag[j]) {
				rval = check_part_crc(j, &ptb.part[j]);
				if (rval == 0)
					ok_flag[j]++;
				else
					ng_flag[j]++;
			}
		}

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(times);
			putstr(" (");
			putdec(i * 100 / times);
			putstr("%)\t\t\r");
		}
	}

	putstr("\r\nTotal:\t");
	putdec(i);

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (do_flag[i]) {
			putstr("\r\n");
			putstr(g_part_str[i]);
			putstr("\tSuccess: ");
			putdec(ok_flag[i]);
			putstr("\tFailed: ");
			putdec(ng_flag[i]);
		}
	}

	putstr("\r\ndone\r\n");

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (ng_flag[i] != 0)
			return -1;
	}

	return 0;
}

void diag_nand_erase(int argc, char *argv[])
{
	u32 start_block, block, blocks, end_block;
	int i, rval,c=0;

	putstr("running nand stress test ...\r\n");
	putstr("press space key to terminate!\r\n");

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnand.nblk[i];
	}

	if (argc == 0)
		end_block = 0;
	else if (argc == 1)
		strtou32(argv[0], &end_block);
	else
		return;

	if (end_block == 0)
		blocks = flnand.blocks_per_bank * flnand.banks - start_block;
	else
		blocks = end_block - start_block;

	for (i = 0, block = start_block; i < blocks; i++, block++) {

		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}

		rval = nand_is_bad_block(block);
		if (rval & NAND_FW_BAD_BLOCK_FLAGS) {
			output_bad_block(block, rval);
			continue;
		}

		rval = nand_erase_block(block);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}

		if (rval < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}
	}

	putstr("\r\ndone!\r\n");
}

#else  /* !CONFIG_NAND_NONE */

void diag_nand_read(void)
{
	putstr("NAND disabled!\r\n");
}

void diag_nand_read_speed(void)
{
	putstr("NAND disabled!\r\n");
}

void diag_nand_prog(void)
{
	putstr("NAND disabled!\r\n");
}

u32 diag_nand_verify(int argc, char *argv[])
{
	putstr("NAND disabled!\r\n");
	return 0;
}

void diag_nand_rclm_bb(int argc, char *argv[])
{
	putstr("NAND disabled!\r\n");
}

void diag_nand_dump(int argc, char *argv[])
{
	putstr("NAND disabled!\r\n");
}

void diag_nand_write(int argc, char *argv[])
{
	putstr("NAND disabled!\r\n");
}
void diag_nand_partition(u32 times)
{
	putstr("NAND disabled!\r\n");
}

void diag_nand_erase(int argc, char *argv[])
{
	putstr("NAND disabled!\r\n");
}

#endif  /* CONFIG_NAND_NONE */
