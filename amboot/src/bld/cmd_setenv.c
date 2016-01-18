/**
 * system/src/bld/cmd_setenv.c
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
#define __FLDRV_IMPL__
#include <fio/firmfl.h>

/**
 * set_mac - set the mac address of specified deviced.
 * @param arg		argument
 * @param dev		specified device, 0 - eth, 1 - wifi, 2 - usb
 * @param ins		specified instances
 * @return		error or OK
 */
static int set_mac(char *arg, u16 dev, u16 ins)
{
	u8 hwaddr[6];
	flpart_table_t ptb;
	int rval;

	if (str_to_hwaddr(arg, hwaddr) < 0) {
		uart_putstr("MAC addr format error!\r\n");
		return -2;
	}

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB load error!\r\n");
		return rval;
	}

	switch (dev) {
		case 0:
			memcpy(ptb.dev.eth[ins].mac, hwaddr, 6);
			break;
		case 1:
			memcpy(ptb.dev.wifi[ins].mac, hwaddr, 6);
			break;
		case 2:
			memcpy(ptb.dev.usb_eth[ins].mac, hwaddr, 6);
			break;
		default:
			uart_putstr("Device error!\r\n");
			return -1;
	}

	/* Save the partition table */
	rval = flprog_set_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB save error!\r\n");
		return rval;
	}
	return rval;
}

/**
 * set_ip - set the IP address of specified deviced.
 * @param arg		argument
 * @param dev		specified device, 0 - eth, 1 - wifi, 2 - usb
 * @param ins		specified instances
 * @return		error or OK
 */
static int set_ip(char *arg, u16 dev, u16 ins)
{
	u32 addr;
	flpart_table_t ptb;
	int rval;

	if (str_to_ipaddr(arg, &addr) < 0) {
		uart_putstr("IP addr format error!\r\n");
		return -2;
	}

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB load error!\r\n");
		return rval;
	}

	switch (dev) {
		case 0:
			memcpy(&ptb.dev.eth[ins].ip, &addr, sizeof(addr));
			break;
		case 1:
			memcpy(&ptb.dev.wifi[ins].ip, &addr, sizeof(addr));
			break;
		case 2:
			memcpy(&ptb.dev.usb_eth[ins].ip, &addr, sizeof(addr));
			break;
		default:
			uart_putstr("Device error!\r\n");
			return -1;
	}

	/* Save the partition table */
	rval = flprog_set_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB save error!\r\n");
		return rval;
	}
	return rval;
}

/**
 * set_mask - set the network mask of specified deviced.
 * @param arg		argument
 * @param dev		specified device, 0 - eth, 1 - wifi, 2 - usb
 * @param ins		specified instances
 * @return		error or OK
 */
static int set_mask(char *arg, u16 dev, u16 ins)
{
	u32 addr;
	flpart_table_t ptb;
	int rval;

	if (str_to_ipaddr(arg, &addr) < 0) {
		uart_putstr("Mask addr format error!\r\n");
		return -2;
	}

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB load error!\r\n");
		return rval;
	}

	switch (dev) {
		case 0:
			memcpy(&ptb.dev.eth[ins].mask, &addr, sizeof(addr));
			break;
		case 1:
			memcpy(&ptb.dev.wifi[ins].mask, &addr, sizeof(addr));
			break;
		case 2:
			memcpy(&ptb.dev.usb_eth[ins].mask, &addr, sizeof(addr));
			break;
		default:
			uart_putstr("Device error!\r\n");
			return -1;
	}

	/* Save the partition table */
	rval = flprog_set_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB save error!\r\n");
		return rval;
	}
	return rval;
}

/**
 * set_gw - set the gateway of specified deviced.
 * @param arg		argument
 * @param dev		specified device, 0 - eth, 1 - wifi, 2 - usb
 * @param ins		specified instances
 * @return		error or OK
 */
static int set_gw(char *arg, u16 dev, u16 ins)
{
	u32 addr;
	flpart_table_t ptb;
	int rval;

	if (str_to_ipaddr(arg, &addr) < 0) {
		uart_putstr("Gateway addr format error!\r\n");
		return -2;
	}

	/* Read the partition table */
	rval = flprog_get_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB load error!\r\n");
		return rval;
	}

	switch (dev) {
		case 0:
			memcpy(&ptb.dev.eth[ins].gw, &addr, sizeof(addr));
			break;
		case 1:
			memcpy(&ptb.dev.wifi[ins].gw, &addr, sizeof(addr));
			break;
		case 2:
			memcpy(&ptb.dev.usb_eth[ins].gw, &addr, sizeof(addr));
			break;
		default:
			uart_putstr("Device error!\r\n");
			return -1;
	}

	/* Save the partition table */
	rval = flprog_set_part_table(&ptb);
	if (rval < 0) {
		uart_putstr("PTB save error!\r\n");
		return rval;
	}
	return rval;
}
static int cmd_setenv(int argc, char *argv[])
{
	flpart_table_t ptb;
	int rval;
	u16 dev = 0, ins = 0;

	if (argc < 2) {
		uart_putstr("Type 'help setenv' for help\r\n");
		return -1;
	}

	if (strcmp(argv[1], "sn") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		strncpy(ptb.dev.sn, argv[2], sizeof(ptb.dev.sn));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "usbdl_mode") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		else if (strcmp(argv[2], "2") == 0)
			yes = 2;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.usbdl_mode = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "auto_boot") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.auto_boot = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "cmdline") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		strfromargv(ptb.dev.cmdline, sizeof(ptb.dev.cmdline),
			    argc - 2, &argv[2]);

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if ((strcmp(argv[1], "eth") == 0) ||
			(strcmp(argv[1], "wifi") == 0) ||
			(strcmp(argv[1], "usb") == 0)) {

		if (strcmp(argv[1], "eth") == 0) {
			dev = 0;
		} else if (strcmp(argv[1], "wifi") == 0) {
			dev = 1;
		} else {
			dev = 2;
		}

		if (strcmp(argv[2], "1") == 0) {
			ins = 1;
		} else {
			ins = 0;
		}

		if (strcmp(argv[3], "mac") == 0) {
			set_mac(argv[4], dev, ins);
		} else if (strcmp(argv[3], "ip") == 0) {
			set_ip(argv[4], dev, ins);
		} else if (strcmp(argv[3], "mask") == 0) {
			set_mask(argv[4], dev, ins);
		} else if (strcmp(argv[3], "gw") == 0) {
			set_gw(argv[4], dev, ins);
		} else {
			uart_putstr("Type 'help setenv' for help\r\n");
			return -1;
		}
	} else if (strcmp(argv[1], "auto_dl") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.auto_dl = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "tftpd") == 0) {
		u32 addr;

		if (str_to_ipaddr(argv[2], &addr) < 0) {
			uart_putstr("IP addr error!\r\n");
			return -2;
		}

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		memcpy(&ptb.dev.tftpd, &addr, sizeof(addr));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "pri_addr") == 0) {
		u32 addr;

		if (strtou32(argv[2], &addr) < 0) {
			uart_putstr("incorrect value!\r\n");
			return -2;
		}

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		memcpy(&ptb.dev.pri_addr, &addr, sizeof(addr));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "pri_file") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		strncpy(ptb.dev.pri_file, argv[2], sizeof(ptb.dev.pri_file));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "pri_comp") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.pri_comp = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "rmd_addr") == 0) {
		u32 addr;

		if (strtou32(argv[2], &addr) < 0) {
			uart_putstr("incorrect value!\r\n");
			return -2;
		}

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		memcpy(&ptb.dev.rmd_addr, &addr, sizeof(addr));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "rmd_file") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		strncpy(ptb.dev.rmd_file, argv[2], sizeof(ptb.dev.rmd_file));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "rmd_comp") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.rmd_comp = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "dsp_addr") == 0) {
		u32 addr;

		if (strtou32(argv[2], &addr) < 0) {
			uart_putstr("incorrect value!\r\n");
			return -2;
		}

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		memcpy(&ptb.dev.dsp_addr, &addr, sizeof(addr));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "dsp_file") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		strncpy(ptb.dev.dsp_file, argv[2], sizeof(ptb.dev.dsp_file));

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
	} else if (strcmp(argv[1], "dsp_comp") == 0) {
		int yes = 0;

		if (strcmp(argv[2], "1") == 0)
			yes = 1;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}

		ptb.dev.dsp_comp = yes;

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
#if defined(SHOW_AMBOOT_SPLASH)
	} else if (strcmp(argv[1], "splash_id") == 0) {
		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
			return rval;
		}
		if ((strtou32(argv[2], &ptb.dev.splash_id) < 0) ||
		    (ptb.dev.splash_id < 0) || (ptb.dev.splash_id > 6)) {
			uart_putstr("incorrect splash id!\r\n");
			return -2;
		}

		/* Save the partition table */
		rval = flprog_set_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB save error!\r\n");
			return rval;
		}
#endif
	} else {
		uart_putstr("Type 'help setenv' for help\r\n");
		return -1;
	}

	return 0;
}

static char help_setenv[] =
	"setenv [param] [val]\r\n"
	"sn        - Serial number\r\n"
	"auto_boot - Automatic boot\r\n"
	"cmdline   - Boot parameters\r\n"
#if defined(SHOW_AMBOOT_SPLASH)
	"splash_id - splash logo id\r\n"
#endif
	"[eth|wifi|usb] [0|1] [mac|ip|mask|gw]\r\n"
	"\t - [device] [instances] [mac addr|IP addr|network mask|gateway]\r\n"
	"auto_dl   - Automatically try to boot over network\r\n"
	"tftpd     - TFTP server address\r\n"
	"pri_addr  - RTOS download address\r\n"
	"pri_file  - RTOS file name\r\n"
	"pri_comp  - RTOS compressed?\r\n"
	"rmd_addr  - Ramdisk download address\r\n"
	"rmd_file  - Ramdisk file name\r\n"
	"rmd_comp  - Ramdisk compressed?\r\n"
	"dsp_addr  - DSP download address\r\n"
	"dsp_file  - DSP file name\r\n"
	"dsp_comp  - DSP compressed?\r\n"
	;

__CMDLIST(cmd_setenv, "setenv", help_setenv);
