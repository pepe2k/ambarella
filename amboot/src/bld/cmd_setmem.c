/**
 * system/src/bld/cmd_setmem.c
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

static int cmd_setmem(int argc, char *argv[])
{
	u32	val;
	u32	_saddr;
	u32	_eaddr;
	u32	_caddr;
	u32	_taddr;
	int	num;

	if (argc == 7) {
		if (strcmp("8", argv[1]) == 0) {
			num = 1;
		} else if (strcmp("16", argv[1]) == 0) {
			num = 2;
		} else if (strcmp("32", argv[1]) == 0) {
			num = 4;
		} else {
			uart_putstr("2nd argument must be [8|16|32]!\r\n");
			return -1;
		}

		/* Get starting address */
		if (strtou32(argv[2], &_saddr) < 0) {
			uart_putstr("invalid start address!\r\n");
			return -2;
		}

		/* Get start address range */
		_taddr = _saddr & 0xf0000000;
		if (_taddr != AHB_BASE &&
		    _taddr != APB_BASE &&
		    _taddr != DRAM_START_ADDR) {
			uart_putstr("start address out of range!\r\n");
			return -3;
		}

		/* Check for '-' */
		if (strcmp("-", argv[3]) != 0) {
			uart_putstr("syntax error!\r\n");
			return -4;
		}

		/* Get end address */
		if (strtou32(argv[4], &_eaddr) < 0) {
			uart_putstr("incorrect ending address\r\n");
			return -5;
		}

		/* Check end address range */
		_taddr = _eaddr & 0xf0000000;
		if (_taddr != AHB_BASE &&
		    _taddr != APB_BASE &&
		    _taddr != DRAM_START_ADDR) {
			uart_putstr("end address out of range!\r\n");
			return -6;
		}

		/* Check address range */
		if (_eaddr < _saddr) {
			uart_putstr("end address must be larger than ");
			uart_putstr("start address\r\n");
			return -7;
		}

		/* Check for ':' */
		if (strcmp(":", argv[5]) != 0) {
			uart_putstr("syntax error!\r\n");
			return -8;
		}

		/* Get pattern */
		if (strtou32(argv[6], &val) < 0) {
			uart_putstr("invalid pattern!\r\n");
			return -9;
		}

		/* Perform the set operation */
		for (_caddr = _saddr; _caddr <= _eaddr; ) {
			if (num == 1 && (val & 0xffffff00) == 0) {
				writeb(_caddr, val);
				_caddr += 1;
			} else if (num == 2 && (val & 0xffff0000) == 0) {
				writeb(_caddr, val);
				writeb(_caddr + 1, val >> 8);
				_caddr += 2;
			} else if (num == 4) {
				writeb(_caddr, val);
				writeb(_caddr + 1, val >> 8);
				writeb(_caddr + 2, val >> 16);
				writeb(_caddr + 3, val >> 24);
				_caddr += 4;
			}
		}
	} else {
		uart_putstr("Type 'help setmem' for help\r\n");
		return -10;
	}

	return 0;
}

static char help_setmem[] =
	"setmem [8|16|32] [start address] - [end address] : [pattern]\r\n"
	"Set memory content with 8/16/32-bit a pattern\r\n"
	"from specified starting and ending addresses\r\n";

__CMDLIST(cmd_setmem, "setmem", help_setmem);
