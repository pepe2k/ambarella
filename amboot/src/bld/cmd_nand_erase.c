/**
 * system/src/bld/cmd_nand_erase.c
 *
 * History:
 *    2008/11/19 - [Chien-Yang Chen] created file
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
#include <fio/ftl_const.h>

#if defined(CONFIG_NAND_NONE)
static int cmd_nand_erase(int argc, char *argv[])
{
	return 0;
}
#else
static int cmd_nand_erase(int argc, char *argv[])
{
	u32 start_block, block, blocks, total_blocks;
	int i, rval;

	total_blocks = flnand.blocks_per_bank * flnand.banks;

	if (argc != 3) {
		uart_putstr("nand_erase [block] [blocks]!\r\n");
		uart_putstr("Total blocks: ");
		uart_putdec(total_blocks);
		uart_putstr("\r\n");
		return -1;
	}

	putstr("erase nand blocks including any bad blocks...\r\n");
	putstr("press enter to start!\r\n");
	putstr("press any key to terminate!\r\n");
	rval = uart_wait_escape(0xffffffff);
	if (rval == 0)
		return -1;

	strtou32(argv[1], &start_block);
	strtou32(argv[2], &blocks);

	for (i = 0, block = start_block; i < blocks; i++, block++) {
		if (uart_poll())
			break;
		if (block >= total_blocks)
			break;

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

	return 0;
}
#endif

static char help_nand_erase[] =
	"nand_erase [block] [blocks]\r\n"
	"Force to erase nand blocks including initial bad blocks\r\n";

__CMDLIST(cmd_nand_erase, "nand_erase", help_nand_erase);
