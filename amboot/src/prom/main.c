/**
 * system/src/prom/main.c
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define __AMBPROM_IMPL__
#include "prom.h"

#ifdef __DEBUG_BUILD__
#define DEBUG_MSG uart_putstr
#else
#define DEBUG_MSG(...)
#endif

const char *AMBPROM_LOGO = "\r\n";

int main(void)
{
	int rval = 0;
#ifdef	FIRMWARE_CONTAINER
	unsigned int boot_from = rct_boot_from();
#endif

	gpio_init();
	rct_fix_spiboot();
#if (CHIP_REV != A5L)
	rct_switch_core_freq();
#endif
#ifdef __DEBUG_BUILD__
	uart_init();
#endif

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)  || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2) || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
	rval = sdmmc_init(FIRMWARE_CONTAINER, boot_from);
	if (rval != 0x0) {
		rval = -10;
		goto done;
	}

	rval = sdmmc_boot();
	if (rval != 0x0) {
		rval = -11;
		goto done;
	}

#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_CF)
	/* To be implemented */
	rval = -10;
	goto done;
#else
	rval = -10;
	goto done;
#endif

done:
	if (rval == -10) {
		DEBUG_MSG("Device initial failed.\r\n");
	} else if (rval == -11) {
		DEBUG_MSG("System boot failed.\r\n");
	}

#ifdef __DEBUG_BUILD__
	{
		char cmd[256];

		uart_putstr(AMBPROM_LOGO);
		while (1) {
			uart_putstr("ambprom> ");
			uart_getcmd(cmd, sizeof(cmd));
		}
	}
#else
	while (1);
#endif

	return 0;
}
