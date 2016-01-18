/**
 * system/src/bld/cmd_help.c
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

extern cmdlist_t *commands;

static int cmd_help(int argc, char *argv[])
{
	int i;
	cmdlist_t *cmd;

	/* Help on a command? */
	if (argc >= 2) {
		for (cmd = commands; cmd != NULL; cmd = cmd->next) {
			if (strcmp(cmd->name, argv[1]) == 0) {
				uart_putstr("Help for '");
				uart_putstr(argv[1]);
				uart_putstr("':\r\nUsage: ");
				uart_putstr(cmd->help);
				return 0;
			}
		}

		return -1;
	}

	uart_putstr("The following commands are supported:\r\n");

	for (cmd = commands, i = 0; cmd != NULL; cmd = cmd->next, i++) {
		uart_putstr(cmd->name);
		uart_putchar('\t');
		if ((i % 4) == 3)
			uart_putstr("\r\n");
	}
	uart_putstr("\r\n");

	uart_putstr("Use 'help' to get help on a specific command\r\n");

	return 0;
}

static char help_help[] =
	"help [command]\r\n"
	"Get help on [command], "
	"or a list of supported commands if a command is omitted.\r\n";

__CMDLIST(cmd_help, "help", help_help);
