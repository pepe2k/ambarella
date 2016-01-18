/**
 * system/src/bld/cmd_boot.c
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

static int cmd_boot(int argc, char *argv[])
{
	char cmdline[MAX_CMDLINE_LEN];

	if (argc == 1) {
#if (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE))
		flpart_table_t ptb;
		flprog_get_part_table(&ptb);
		boot(ptb.dev.cmdline, 1);
#else
		boot(NULL, 1);
#endif
	} else {
		strfromargv(cmdline, sizeof(cmdline), argc - 1, &argv[1]);
		boot(cmdline, 1);
	}

	return 0;
}

static char help_boot[] =
	"boot <param>\r\n"
	"Load images from flash to memory and boot\r\n";

__CMDLIST(cmd_boot, "boot", help_boot);
