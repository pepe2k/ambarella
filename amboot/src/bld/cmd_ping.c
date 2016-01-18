/**
 * system/src/bld/cmd_ping.c
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

#if (ETH_INSTANCES >= 1)

static int cmd_ping(int argc, char *argv[])
{
	int rval = 0;
	bld_ip_t ip;

	if (argc != 2) {
		uart_putstr("Type 'help ping' for help\r\n");
		return -1;
	}

	if (str_to_ipaddr(argv[1], &ip) < 0) {
		uart_putstr("IP addr error!\r\n");
		return -2;
	}

	rval = bld_net_wait_ready(DEFAULT_NET_WAIT_TMO);
	if (rval < 0) {
		putstr("link down ...\r\n");
		return -3;
	}

	rval = bld_ping(ip, DEFAULT_NET_WAIT_TMO);
	if (rval >= 0) {
		uart_putstr("alive!\r\n");
	} else {
		uart_putstr("no echo reply! ...\r\n");
	}

	return rval;
}

static char help_ping[] =
	"ping [addr]\r\n"
	"PING a host\r\n";

__CMDLIST(cmd_ping, "ping", help_ping);

#endif
