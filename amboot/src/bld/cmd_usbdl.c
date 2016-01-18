/**
 * system/src/bld/cmd_usbdl.c
 *
 * History:
 *    2005/09/27 - [Arthur Yang] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>

#define DEFAULT_ADDR	MEMFWPROG_RAM_START

extern int changed_to_ext_pll;
extern void init_usb_pll(void);

static int cmd_usbdl(int argc, char *argv[])
{
	u32 addr = DEFAULT_ADDR;
	int len;
	int flag = 0;
	int exec = 0;

	/* Parameters parsing */
	if ((argc == 2) || (argc == 3)) {
		/* Check argv[1] */
		if (strcmp(argv[1], "fwprog") == 0) {
			exec = 1;
			flag |= USB_FLAG_FW_PROG;
		} else if (strcmp(argv[1], "ext") == 0) {
			uart_putstr("Using external osc for USB\r\n");
			changed_to_ext_pll = 1;
			return 0;
		} else if (strcmp(argv[1], "kernel") == 0) {
			exec = 1;
			flag |= USB_FLAG_KERNEL;
		} else if (strcmp(argv[1], "upload") == 0) {
			flag |= USB_FLAG_UPLOAD;
		} else if (strcmp(argv[1], "test") == 0) {
			/* Check argv[2] */
			if (argc == 2)
				flag |= USB_FLAG_TEST_DOWNLOAD;
			else {
				if (strcmp(argv[2], "download") == 0)
					flag |= USB_FLAG_TEST_DOWNLOAD;
				else if (strcmp(argv[2], "pll") == 0)
					flag |= USB_FLAG_TEST_PLL;
				else {
					uart_putstr ("Invalid test\r\n");
					return -1;
				}
			}
		} else {
			if ((strtou32(argv[1], &addr) < 0) ||
			    ((addr < DRAM_START_ADDR) ||
			    (addr > DRAM_START_ADDR + DRAM_SIZE - 1))) {
				uart_putstr ("Invalid hex address\r\n");
				return -2;
			}
			if ((argc == 3) && ((strcmp(argv[2], "exec") == 0) ||
					     (strcmp(argv[2], "1") == 0)))
				exec = 1;
			flag |= USB_FLAG_MEMORY;
		}
	} else if (argc != 1) {
		uart_putstr("Invalid parameter\r\n");
		return -3;
	}

	uart_putstr("Start transferring using USB...\r\n");

	/* TIMER1 ISR seems to corrupt usb xmit/recv
	 * disable TIMER1 during running USB task */

	vic_disable(TIMER1_INT_VEC);

	init_usb_pll();
	len = usb_download((void *)addr, exec, flag);

	vic_enable(TIMER1_INT_VEC);

	if (len < 0) {
		uart_putstr("\r\ntransfer failed!\r\n");
		return len;
	}

	uart_putstr("\r\nSuccessfully transferred ");
	uart_putdec(len);
	uart_putstr(" bytes.\r\n");

#if 0
	/* Program firmware received */
	if (flag & USB_FLAG_FW_PROG)
		return prog_firmware(addr, len, 0x0);
#endif

	return 0;
}

static char help_usbdl[] = "The command execute actions over USB\r\n"
	"usbdl "
		"(Perform actions controlled by Host)\r\n"
	"usbdl ADDRESS exec "
		"(Download image to target address and execute it)\r\n"
	"usbdl fwprog "
		"(Download firware and program it into flash)\r\n"
	"usbdl ext "
		"(Turn on USB external clock)\r\n"
	"usbdl kernel "
		"(Download kernel files and execute it)\r\n"
	"usbdl upload "
		"(Upload data to Host by host's request)\r\n"
	"usbdl test [download | pll] "
		"(Test download or dll power-on/off)\r\n";

__CMDLIST(cmd_usbdl, "usbdl", help_usbdl);
