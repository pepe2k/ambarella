/**
 * system/src/bld/cmd_reset.c
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

static int cmd_reset(int argc, char *argv[])
{
	int i;

	if (argc > 1) {
		uart_putstr("reset does not accept arguments\r\n");
		return -1;
	}


	uart_putstr("          c");
	for (i = 0; i < 100; i++)
		uart_putstr("\r\n");
	uart_putstr("c");

	return 0;
}

static char help_reset[] =
	"reset\r\n"
	"Reset terminal\r\n";

__CMDLIST(cmd_reset, "reset", help_reset);
