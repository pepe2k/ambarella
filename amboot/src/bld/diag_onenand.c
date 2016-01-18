/**
 * system/src/bld/diag_onenand.c
 *
 * History:
 *    2009/09/18 - [Evan Chen] created file
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

#if !defined(CONFIG_ONENAND_NONE)

int diag_onenand_read_reg(int argc, char *argv[])
{
	int rval;
	u32 bank = 0, addr = 0, val = 0;

	strtou32(argv[0], &bank);
	strtou32(argv[1], &addr);

	putstr("onenand read register ...");

	rval = onenand_read_reg((u8)bank, (u16)addr, (u16 *)&val);
	if (rval < 0)
		putstr("fail\r\n");
	else {
		putstr("0x");
		puthex((u16)val);
		putstr("\r\n");
	}
	return rval;
}

int diag_onenand_write_reg(int argc, char *argv[])
{
	int rval;
	u32 bank = 0, addr = 0, val = 0;

	strtou32(argv[0], &bank);
	strtou32(argv[1], &addr);

	putstr("onenand write register 0x");
	puthex((u16)addr);

	rval = onenand_write_reg((u8)bank, (u16)addr, (u16)val);
	if (rval < 0)
		putstr("...fail\r\n");
	else
		putstr("...success\r\n");

	return rval;
}

int diag_onenand_erase(int argc, char *argv[])
{
	int rval = 0;
	u32 sblk = 0, nblk = 0, block;

	strtou32(argv[0], &sblk);
	strtou32(argv[1], &nblk);

	putstr("onenand erase block from ");
	putdec(sblk);
	putstr(" length ");
	putdec(nblk);

	for (block = 0; block < sblk + nblk; block++) {
		rval = onenand_erase_block(block);
		if (rval < 0) {
			putstr("fail at");
			putdec(sblk);
			putstr("\r\n");
			goto done;
		}
	}
	putstr("...success!\r\n");
done:
	return rval;
}

int diag_onenand_read_pages(int argc, char *argv[])
{
	int rval = 0;
	u32 block, page, pages;
	u8 *buf = (u8 *)DRAM_START_ADDR;

	strtou32(argv[1], &block);
	strtou32(argv[2], &page);
	strtou32(argv[3], &pages);

	putstr("onenand read block: ");
	putdec(block);
	putstr(" page: ");
	putdec(page);
	putstr(" pages: ");
	putdec(pages);
	putstr("\r\n");

	if (strcmp(argv[0], "m") == 0)
		rval = onenand_read_pages(block, page, pages, buf);
	else if (strcmp(argv[0], "s") == 0)
		rval = onenand_read_spare(block, page, pages, buf);

	return rval;
}

int diag_onenand_prog(int argc, char *argv[])
{
	int rval = 0;
	u32 block, page, pages;
	u8 *buf = (u8 *)DRAM_START_ADDR;

	strtou32(argv[1], &block);
	strtou32(argv[2], &page);
	strtou32(argv[3], &pages);

	putstr("onenand prog block: ");
	putdec(block);
	putstr(" page: ");
	putdec(page);
	putstr(" pages: ");
	putdec(pages);
	putstr("\r\n");

	if (strcmp(argv[0], "m") == 0)
		rval = onenand_prog_pages(block, page, pages, buf);
	else if (strcmp(argv[0], "s") == 0)
		rval = onenand_prog_spare(block, page, pages, buf);

	return rval;
}

#else /* !CONFIG_ONENAND_NONE */

int diag_onenand_read_reg(int argc, char *argv[])
{
	putstr("ONENAND disabled!\r\n");
	return 0;
}

int diag_onenand_write_reg(int argc, char *argv[])
{
	putstr("ONENAND disabled!\r\n");
	return 0;
}

int diag_onenand_erase(int argc, char *argv[])
{
	putstr("ONENAND disabled!\r\n");
	return 0;
}

#endif /* CONFIG_ONENAND_NONE */

