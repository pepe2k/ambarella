/**
 * system/src/bld/testreboot.c
 *
 * History:
 *    2006/07/12 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>

#ifdef ENABLE_AMBOOT_TEST_REBOOT

#ifndef TEST_REBOOT_GPIO
#define TEST_REBOOT_GPIO	GPIO(44)
#endif

/**
 * This function is used *ONLY* for stress testing the robustness of
 * AMBoot. It works by computing the CRC32 of all the partition data stored
 * in flash memory. If the test passes, a GPIO signal is set so that an
 * external controller can possibly:
 *	(1) Apply the HW reset signal to the board to reboot
 *	(2) Power-cycle the board to reboot.
 */
void test_reboot(void)
{
#if !defined(CONFIG_NAND_NONE) && (0)
	{
		extern int diag_nand_partition(char *, u32);
		if (diag_nand_partition("all", 1) != 0) {
			putstr("diag nand partition failed!\r\n");
			while (1);
		}
	}
#endif

	putstr("setting GPIO ");
	putdec(TEST_REBOOT_GPIO);
	putstr(" for reboot...\r\n");
	/* Set the GPIO output pin to HI */
	gpio_config_sw_out(TEST_REBOOT_GPIO);
	gpio_set(TEST_REBOOT_GPIO);
}

#endif  /* ENABLE_AMBOOT_TEST_REBOOT */
