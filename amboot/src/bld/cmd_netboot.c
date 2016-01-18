/**
 * system/src/bld/cmd_netboot.c
 *
 * History:
 *    2006/10/16 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <fio/firmfl.h>

#if (ETH_INSTANCES >= 1)

static int cmd_netboot(int argc, char *argv[])
{
	int rval = 0;
	char cmdline[MAX_CMDLINE_LEN];

	if (argc == 1) {
#if (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE))
		flpart_table_t ptb;
		flprog_get_part_table(&ptb);
		rval = netboot(ptb.dev.cmdline, 1);
#else
		rval = netboot(NULL, 1);
#endif
	} else {
		strfromargv(cmdline, sizeof(cmdline), argc - 1, &argv[1]);
		netboot(cmdline, 1);
	}

	if (rval < 0) {
		uart_putstr("failed! ...\r\n");
	}

	return 0;
}

static char help_netboot[] =
	"netboot <cmdline>\r\n"
	"Load images from TFTP server to memory and boot\r\n";

__CMDLIST(cmd_netboot, "netboot", help_netboot);

#endif
