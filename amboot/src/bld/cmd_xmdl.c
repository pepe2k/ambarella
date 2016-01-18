/**
 * system/src/bld/cmd_xmdl.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>

static int cmd_xmdl(int argc, char *argv[])
{
	int firmware = 0;
	u32 addr;
	int len;
	void (*jump_to_img)(void) = NULL;

	if (argc == 2 && strcmp(argv[1], "firmware") == 0) {
		firmware = 1;
		addr = DRAM_START_ADDR + 0x00100000;
	} else if (argc < 2 || argc > 3 || strtou32(argv[1], &addr) < 0) {
		uart_putstr("address (eg. 0x");
		uart_puthex(DRAM_START_ADDR + 0x00100000);
		uart_putstr(") is required\r\n");
		return -1;
	}

	if (argc == 3 && strcmp(argv[2], "exec") != 0) {
		uart_putstr("3rd argument must be exec...\r\n");
		return -2;
	}

	if (addr < (DRAM_START_ADDR + 0x00100000) ||
		addr > (DRAM_START_ADDR + DRAM_SIZE - 1)) {
		uart_putstr("address out of range!\r\n");
		return -3;
	}

	uart_putstr("download to 0x");
	uart_puthex(addr);
	uart_putstr("\r\n");
	uart_putstr("start sending using the XMODEM protocol...\r\n");
	uart_putstr("  press ^X to quit\r\n");

	len = xmodem_recv((char *) addr, 0x10000000);
	if (len < 0) {
		uart_putstr("\r\ndownload failed!\r\n");
		return len;
	}

	uart_putstr("\r\nsuccessfully received ");
	uart_putdec(len);
	uart_putstr(" bytes.\r\n");

	/* Program firmware received */
	if (firmware) {
		return prog_firmware(addr, len, 0x0);
	}

	/* Jump to image */
	jump_to_img = (void *) addr;
	if (argc == 3 && jump_to_img != 0x0) {
		uart_putstr("jumping to 0x");
		uart_puthex(addr);
		uart_putstr(" ...\r\n");

		_clean_flush_all_cache();
		_disable_icache();
		_disable_dcache();
		disable_interrupts();

		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");

		jump_to_img();
	}

	return 0;
}

static char help_xmdl[] =
	"xmdl [address] [exec]\r\n"
	"xmdl firmware\r\n"
	"Download binary image to target memory address over UART\r\n"
	"using the XMODEM protocol\r\n";

__CMDLIST(cmd_xmdl, "xmdl", help_xmdl);
