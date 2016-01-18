/**
 * system/src/bld/cmd_erase.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] Added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
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

static int cmd_erase(int argc, char *argv[])
{
	u32 i;

	if (argc != 2) {
		uart_putstr("partition needs to be specified!\r\n");
		return -1;
	}

	if (strcmp(argv[1], "boot") == 0) {
		for (i = PART_BST; i < TOTAL_FW_PARTS; i++) {
			flprog_erase_partition(i);
		}
		return 0;
	} else if (strcmp(argv[1], "os") == 0) {
		for (i = PART_PBA; i < TOTAL_FW_PARTS; i++) {
			flprog_erase_partition(i);
		}
		return 0;
	} else if (strcmp(argv[1], "all") == 0) {
		return flprog_erase_partition(PART_MAX);
#if defined(NAND_SUPPORT_BBT)
	} else if (strcmp(argv[1], "bbt") == 0) {
		return nand_erase_bbt();
#endif
	}

	for (i = 0; i < PART_MAX; i++) {
		if (strcmp(argv[1], g_part_str[i]) == 0)
			return flprog_erase_partition(i);
	}
	uart_putstr("parition name not recognized!\r\n");

	return -2;
}

static char help_erase[] =
	"erase [bst|ptb|bld|hal|pba|pri|sec|bak|rmd|rom|dsp|lnx|\r\n"
	"               raw|stg|prf|cal|all|os|boot|bbt]\r\n"
	"Where [boot] means bst, ptb, bld, hal, pba, pri, sec, bak, \r\n"
	"                   rmd, rom, dsp, lnx, swp, add, adc partitions.\r\n"
	"      [os]   means pba, pri, sec, bak, rmd, rom, dsp, lnx, \r\n"
	"                   swp, add, adc partitions.\r\n"
	"      [all]  means full chip.\r\n"
#if defined(NAND_SUPPORT_BBT)
	"      [bbt]  means bad block table.\r\n"
#endif
	"Erase a parition as specified\r\n";

__CMDLIST(cmd_erase, "erase", help_erase);
