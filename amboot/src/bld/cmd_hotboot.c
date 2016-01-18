/**
 * system/src/bld/cmd_hotboot.c
 *
 * History:
 *    2005/11/12 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

static int cmd_hotboot(int argc, char *argv[])
{
	u32 pattern;

	if (argc != 2) {
		uart_putstr("pattern argument is required\r\n");
		return -1;
	}

	if (strtou32(argv[1], &pattern) < 0) {
		uart_putstr("invalid parameter!\r\n");
		return -2;
	}

	*((u32 *) &hotboot_pattern) = pattern;
	*((u32 *) &hotboot_valid) = 1;

	{
#if (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE))
		flpart_table_t ptb;
		flprog_get_part_table(&ptb);
		boot(ptb.dev.cmdline, 1);
#else
		boot(NULL, 1);
#endif
	}

	return 0;
}

static char help_hotboot[] =
	"hotboot <pattern>\r\n"
	"This command is equavalent to boot() with hotboot pattern set\r\n";

__CMDLIST(cmd_hotboot, "hotboot", help_hotboot);
