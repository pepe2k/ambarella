/**
 * system/src/bld/diag_snor.c
 *
 * History:
 *    2009/09/18 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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

#if !defined(CONFIG_SNOR_NONE)
extern int rand(void);

static int cmp_data(u8* origin, u8* data, u32 len)
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

int diag_snor_verify(int argc, char *argv[])
{
	u32 start_block, block, blocks;
	u32 block_size;
	u32 bb_num = 0;
	u8 *wbuf, *rbuf;
	int i, rval, loop = 0;

	putstr("running nand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	start_block = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		start_block += flnor.nblk[i];
	}

	blocks = flnor.total_sectors - start_block;

	wbuf = (u8*) g_hugebuf_addr;
	rbuf = (u8*) (g_hugebuf_addr + 0x100000);


	if (flnor.block1_size > flnor.block2_size)
		block_size = flnor.block1_size;
	else
		block_size = flnor.block2_size;

repeat:
	for (i = 0; i < block_size; i++)
		wbuf[i] = rand() / 256;

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

	if (argc == 2 && strcmp(argv[1], "loop") == 0)
		loop = 1;

	for (i = 0, block = start_block; i < blocks; i++, block++) {

		if (uart_poll())
			break;

		rval = snor_erase_block(block);
		if (rval < 0) {
			putstr("\r\nsnor_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = snor_prog_block(block, 1, wbuf);
		if (rval < 0) {
			putstr("\r\nsnor_prog_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = snor_read_block(block, 1, rbuf);
		if (rval < 0) {
			putstr("\r\nsnor_read_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = snor_erase_block(block);
		if (rval < 0) {
			putstr("\r\nsnor_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		rval = cmp_data(wbuf, rbuf, block_size);
		if (rval != 0) {
			putstr("\r\nsnor verify data error count ");
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

	if (loop)
		goto repeat;

	return bb_num;
}

#else  /* !CONFIG_SNOR_NONE */

int diag_snor_verify(int argc, char *argv[])
{
	putstr("SNOR disabled!\r\n");
	return 0;
}

#endif  /* CONFIG_SNOR_NONE */

