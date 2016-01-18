/**
 * system/src/bld/diag_mem.c
 *
 * History:
 *    2005/09/09 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

extern int testram3(u32 addr, u32 size);

/**
 * Run diagnostic on DRAM: test all regions pass the R/W test. The 1st 1MB
 * of DRAM is skipped since the bootloader can't run without it being usable!
 */
void diag_mem(void)
{
	int rval;
	u32 addr;

	uart_putstr("running memory test ...\r\n");
#if (CHIP_REV == I1)
	uart_putstr("region 0x00000000 - 0x000fffff skipped\r\n");
#else
	uart_putstr("region 0xc0000000 - 0xc00fffff skipped\r\n");
#endif

	for (addr = (DRAM_START_ADDR + 0x00100000);
		addr <= DRAM_END_ADDR; addr += 0x00100000) {
		uart_putstr("testing 0x");
		uart_puthex(addr);
		uart_putstr(" - 0x");
		uart_puthex(addr | 0x000fffff);
		uart_putstr(" ... ");

		rval = testram3(addr, 0x00100000);
		if (rval != 0) {
			uart_putstr("\r\nmemory failure at 0x");
			uart_puthex(addr);
			uart_putstr("!\r\n");
			break;
		}

		uart_putstr("done\r\n");
	}
}
