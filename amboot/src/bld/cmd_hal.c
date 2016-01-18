/**
 * system/src/bld/cmd_hal.c
 *
 * History:
 *    2008/11/18 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <hal/hal.h>

static int cmd_hal(int argc, char *argv[])
{
	int i;
	int result;
	amb_hal_function_info_t *finfo;

	if (argc == 1) {
		/* Enumerate HAL */
		finfo = (amb_hal_function_info_t *)
			(g_haladdr + 128);
		for (i = 0; finfo[i].function != NULL; i++) {
			uart_putdec(i);
			uart_putchar(' ');
			uart_puthex((u32) finfo[i].function);
			uart_putchar(' ');
			uart_putstr((char *) (g_haladdr + (u32) finfo[i].name));
			uart_putstr("\r\n");
		}

		return 0;
	}

	/* Invoke HAL */
	result = hal_call_name(argc -1, &argv[1]);
	uart_putstr("result = ");
	uart_putdec(result);
	uart_putstr("\r\n");

	return 0;
}

static char help_hal[] =
	"hal fn [arguments]\r\n"
	"Invoke a HAL function\r\n";

__CMDLIST(cmd_hal, "hal", help_hal);
