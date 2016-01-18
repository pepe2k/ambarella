/**
 * system/src/bld/cmd_reboot.c
 *
 * History:
 *    2005/10/08 - [Charles Chiou] created file
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

static int cmd_reboot(int argc, char *argv[])
{
	if (argc > 1) {
		uart_putstr("reboot does not accept arguments\r\n");
		return -1;
	}

	rct_reset_chip();

	return 0;
}

static char help_reboot[] =
	"reboot\r\n"
	"Reboot the system by reseting hardware\r\n";

__CMDLIST(cmd_reboot, "reboot", help_reboot);
