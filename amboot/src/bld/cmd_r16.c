/**
 * system/src/bld/cmd_r16.c
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

static int cmd_r16(int argc, char *argv[])
{
	u32	_taddr;
	u32	val;
	u32	addr;

	if (argc == 2) {
		/* Check address format */
		if (strtou32(argv[1], &addr) == -1) {
			uart_putstr("invalid address!\r\n");
			return -1;
		}

		/* Check memory range */
		_taddr = addr & 0xf0000000;
		if (_taddr != AHB_BASE &&
		    _taddr != APB_BASE &&
		    _taddr != DRAM_START_ADDR) {
			uart_putstr("address out of range!\r\n");
			return -2;
		}

		val = readw(addr);

		uart_putstr("0x");
		uart_puthex(addr);
		uart_putstr("  :  ");
		uart_putstr("0x");
		uart_putbyte(val >> 8);
		uart_putbyte(val & 0xff);
		uart_putstr("\r\n");
	} else {
		uart_putstr("Type 'help r16' for help\r\n");
		return -3;
	}

	return 0;
}

static char help_r16[] =
	"r16 [address]\r\n"
	"Read a 16-bit data from an address\r\n";

__CMDLIST(cmd_r16, "r16", help_r16);
