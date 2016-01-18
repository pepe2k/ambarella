/**
 * system/src/bld/cmd_bios.c
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

static int cmd_bios(int argc, char *argv[])
{
	char cmdline[MAX_CMDLINE_LEN];

	if (argc == 1) {
#if (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE))
		flpart_table_t ptb;
		flprog_get_part_table(&ptb);
#if defined(AMBOOT_DEV_BOOT_CORTEX)
		cmd_cortex_bios(ptb.dev.cmdline, 1);
#else
		bios(ptb.dev.cmdline, 1);
#endif
#else
#if defined(AMBOOT_DEV_BOOT_CORTEX)
		cmd_cortex_bios(NULL, 1);
#else
		bios(NULL, 1);
#endif
#endif
	} else {
		strfromargv(cmdline, sizeof(cmdline), argc - 1, &argv[1]);
#if defined(AMBOOT_DEV_BOOT_CORTEX)
		cmd_cortex_bios(cmdline, 1);
#else
		bios(cmdline, 1);
#endif
	}

	return 0;
}

static char help_bios[] =
	"bios\r\n"
	"Load the PBA image from flash and boot to BIOS\r\n";

__CMDLIST(cmd_bios, "bios", help_bios);
