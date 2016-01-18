/**
 * system/src/bld/cmd_w8.c
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

static int cmd_w8(int argc, char *argv[])
{
	u32	_taddr;
	u32	val;
	u32	addr;

	if (argc == 3) {
		/* Check address format */
		if (strtou32(argv[1], &addr) < 0) {
			uart_putstr("incorrect address!\r\n");
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

		/* Check value format */
		if (strtou32(argv[2], &val) < 0) {
			uart_putstr("incorrect value!\r\n");
			return -3;
		}
		if ((val & 0xffffff00) != 0x0) {
			uart_putstr("value out of range!\r\n");
			return -4;
		}
		writeb(addr, val);
	} else {
		uart_putstr("Type 'help w8' for help\r\n");
		return -5;
	}

	return 0;
}

static char help_w8[] =
	"w8 [address] [value]\r\n"
	"Write an 8-bit data into an address\r\n";

__CMDLIST(cmd_w8, "w8", help_w8);
