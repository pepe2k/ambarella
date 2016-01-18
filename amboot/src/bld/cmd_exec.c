/**
 * system/src/bld/cmd_exec.c
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

static int cmd_exec(int argc, char *argv[])
{
	u32 addr;
	void (*jump_to_img)(void) = NULL;

	if (argc != 2 || strtou32(argv[1], &addr) < 0) {
		uart_putstr("address (eg. 0x");
		uart_puthex(DRAM_START_ADDR + 0x00100000);
		uart_putstr(") is required\r\n");
		return -1;
	}

	if (addr < (DRAM_START_ADDR + 0x00100000) ||
		addr > (DRAM_START_ADDR + DRAM_SIZE - 1)) {
		uart_putstr("address out of range!\r\n");
		return -1;
	}

	jump_to_img = (void *) addr;

	if (jump_to_img != 0x0) {
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

static char help_exec[] =
	"exec [address]\r\n"
	"Execute program in memory location specified by address\r\n";

__CMDLIST(cmd_exec, "exec", help_exec);
