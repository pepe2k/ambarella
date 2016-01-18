/**
 * system/src/bld/cmd_show.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>
#include <fio/ftl_const.h>

extern void output_part(const char *s, const flpart_t *part);
extern const char *AMBOOT_LOGO;

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))

static u32 init_bad_tbl[2048] __attribute__((section(".bss.noinit")));
static u32 late_bad_tbl[2048] __attribute__((section(".bss.noinit")));
static u32 other_bad_tbl[2048] __attribute__((section(".bss.noinit")));

static void show_nand_bb(void)
{
	u32 block, blocks;
	u32 init_bad = 0, late_bad = 0, other_bad = 0, total_bad = 0;
	int rval, i;

	blocks = flnand.blocks_per_bank * flnand.banks;

	for (block = 0; block < blocks; block++) {
		rval = nand_is_bad_block(block);
		if (rval != 0) {
			if (rval == NAND_INITIAL_BAD_BLOCK) {
				init_bad_tbl[init_bad++] = block;
			} else if (rval == NAND_LATE_DEVEL_BAD_BLOCK) {
				late_bad_tbl[late_bad++] = block;
			} else {
				other_bad_tbl[other_bad++] = block;
			}

			total_bad++;

			if (init_bad >= 2048 || late_bad >= 2048 ||
			    other_bad >= 2048)
				break;
		}
	}

	if (init_bad)
		uart_putstr("----- Initial bad blocks -----");
	for (i = 0; i < init_bad; i++) {
		if (i % 5)
			uart_putstr("\t");
		else
			uart_putstr("\r\n");

		uart_putdec(init_bad_tbl[i]);
	}

	if (late_bad)
		uart_putstr("\r\n----- Late developed bad blocks -----");
	for (i = 0; i < late_bad; i++) {
		if (i % 5)
			uart_putstr("\t");
		else
			uart_putstr("\r\n");

		uart_putdec(late_bad_tbl[i]);
	}

	if (other_bad)
		uart_putstr("\r\n----- Other bad blocks -----");
	for (i = 0; i < other_bad; i++) {
		if (i % 5)
			uart_putstr("\t");
		else
			uart_putstr("\r\n");

		uart_putdec(other_bad_tbl[i]);
	}

	if (total_bad) {
		uart_putstr("\r\nTotal bad blocks: ");
		uart_putdec(total_bad);
		uart_putstr("\r\n");
	} else {
		uart_putstr("\r\nNo bad blocks\r\n");
	}
}
#else
static void show_nand_bb(void)
{
}
#endif

static char __to_4bit_char(u8 v)
{
	if (v <= 9)
		return '0' + v;
	else if (v < 15)
		return 'a' + v - 10;
	return 'f';
}

static void show_mac(const char *msg, u8 *mac)
{
	uart_putstr(msg);
	uart_putchar(__to_4bit_char(mac[0] >>  4));
	uart_putchar(__to_4bit_char(mac[0] & 0xf));
	uart_putchar(':');
	uart_putchar(__to_4bit_char(mac[1] >>  4));
	uart_putchar(__to_4bit_char(mac[1] & 0xf));
	uart_putchar(':');
	uart_putchar(__to_4bit_char(mac[2] >>  4));
	uart_putchar(__to_4bit_char(mac[2] & 0xf));
	uart_putchar(':');
	uart_putchar(__to_4bit_char(mac[3] >>  4));
	uart_putchar(__to_4bit_char(mac[3] & 0xf));
	uart_putchar(':');
	uart_putchar(__to_4bit_char(mac[4] >>  4));
	uart_putchar(__to_4bit_char(mac[4] & 0xf));
	uart_putchar(':');
	uart_putchar(__to_4bit_char(mac[5] >>  4));
	uart_putchar(__to_4bit_char(mac[5] & 0xf));
	uart_putstr("\r\n");
}

static void show_ip(const char *msg, u32 ip)
{
	uart_putstr(msg);
	uart_putdec(ip & 0xff);
	uart_putchar('.');
	uart_putdec((ip >> 8) & 0xff);
	uart_putchar('.');
	uart_putdec((ip >> 16) & 0xff);
	uart_putchar('.');
	uart_putdec((ip >> 24) & 0xff);
	uart_putstr("\r\n");
}

static int cmd_show(int argc, char *argv[])
{
	int rval;

	unsigned int boot_from = rct_boot_from();

	if (argc != 2) {
		uart_putstr("requires an argument!\r\n");
		return -1;
	}

	if (strcmp(argv[1], "logo") == 0) {
		uart_putstr(AMBOOT_LOGO);

		return 0;
#if defined(ENABLE_FLASH)
	} else if (strcmp(argv[1], "bb") == 0) {
		show_nand_bb();
#if defined(NAND_SUPPORT_BBT)
	} else if (strcmp(argv[1], "bbt") == 0) {
		nand_show_bbt();
#endif
	} else if (strcmp(argv[1], "flash") == 0) {
		if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
#if !defined(CONFIG_NAND_NONE)
			int i;

			uart_putstr("RCT configured to NAND mode\r\n");
			uart_putstr("main size: ");
			uart_putdec(flnand.main_size);
			uart_putstr("\r\n");
			uart_putstr("pages per block: ");
			uart_putdec(flnand.pages_per_block);
			uart_putstr("\r\n");

			for (i = 0; i < TOTAL_FW_PARTS; i++) {
				uart_putstr(g_part_str[i]);
				uart_putstr(" partition blocks: ");
				uart_putdec(flnand.sblk[i]);
				uart_putstr(" - ");
				uart_putdec(flnand.sblk[i] + flnand.nblk[i]);
				uart_putstr("\r\n");
			}
#endif
#if !defined(CONFIG_NOR_NONE)
		} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
			int i;

			uart_putstr("RCT configured to NOR mode\r\n");

			for (i = 0; i < TOTAL_FW_PARTS; i++) {
				uart_putstr(g_part_str[i]);
				uart_putstr(" partition blocks: ");
				uart_putdec(flnor.sblk[i]);
				uart_putstr(" - ");
				uart_putdec(flnor.sblk[i] + flnor.nblk[i]);
				uart_putstr("\r\n");
			}
#endif
		}
#endif
#if defined(ENABLE_SD)
	} else if (strcmp(argv[1], "sm") == 0) {
		if (((boot_from & BOOT_FROM_SPI) != 0x0) ||
		    ((boot_from & BOOT_FROM_SDMMC) != 0x0)) {
		    	int i;
			smpart_t *smpart;
			uart_putstr("RCT configured to sector media mode\r\n");

			for (i = 0; i < TOTAL_FW_PARTS; i++) {
				smpart = sm_get_part(i);

				uart_putstr(g_part_str[i]);
				uart_putstr(" partition blocks: ");
				uart_putdec(smpart->ssec);
				uart_putstr(" - ");
				uart_putdec(smpart->ssec + smpart->nsec);
				uart_putstr("\r\n");
			}
		}
#endif
#if (defined(ENABLE_FLASH) || defined(ENABLE_SD))
	 } else if (strcmp(argv[1], "ptb") == 0) {
		flpart_table_t ptb;
		int i;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0)
			return rval;

		/* Display PTB content */
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			if (i == PART_PTB)
				continue;
			output_part(g_part_str[i], &ptb.part[i]);
		}

		uart_putstr("S/N: ");
		for (i = 0; i < sizeof(ptb.dev.sn); i++) {
			if (ptb.dev.sn[i] == '\0')
				break;
			uart_putchar(ptb.dev.sn[i]);
		}
		uart_putstr("\r\nusbdl_mode: ");
		uart_putdec(ptb.dev.usbdl_mode);
		uart_putstr("\r\nauto_boot: ");
		uart_putdec(ptb.dev.auto_boot);
		uart_putstr("\r\ncmdline: \"");
		if (strlen(ptb.dev.cmdline) < sizeof(ptb.dev.cmdline))
			uart_putstr(ptb.dev.cmdline);
		uart_putstr("\"\r\n");
	 } else if (strcmp(argv[1], "meta") == 0) {
	 	const char *fw_dev[] = FW_DEVICE;
		flpart_meta_t meta;
		int i;

		/* Read the meta data */
		rval = flprog_get_meta(&meta);
		if (rval < 0 || meta.magic != PTB_META_MAGIC) {
			putstr("meta appears damaged...\r\n");
			return rval;
		}

		/* Display META content */
		for (i = 0; i < PART_MAX; i++) {
			putstr(meta.part_info[i].name);
			putstr("\t sblk: ");
			putdec(meta.part_info[i].sblk);
			putstr("\t nblk: ");
			putdec(meta.part_info[i].nblk);
			putstr("\t on: ");
			putstr(fw_dev[meta.part_dev[i]]);
			putstr("\r\n");
		}
		putstr("\r\n");

		for (i = 0; i < 2; i++) {
			putstr("sm_stg[");
			putdec(i);
			putstr("]");
			putstr("\t sblk: ");
			putdec(meta.sm_stg[i].sblk);
			putstr("\t nblk: ");
			putdec(meta.sm_stg[i].nblk);
			putstr("\r\n");
		}
		putstr("\r\n");

		putstr("model name:	");
		putstr((char *)meta.model_name);
		putstr("\r\n");

		putstr("crc32:		0x");
		puthex(meta.crc32);
		putstr("\r\n");
#endif
	} else if (strcmp(argv[1], "netboot") == 0) {
		flpart_table_t ptb;
		int i;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0)
			return rval;

		show_mac("eth0_mac: ", ptb.dev.eth[0].mac);
		show_ip("eth0_ip: ", ptb.dev.eth[0].ip);
		show_ip("eth0_mask: ", ptb.dev.eth[0].mask);
		show_ip("eth0_gw: ", ptb.dev.eth[0].gw);
#if (ETH_INSTANCES >= 2)
		show_mac("eth1_mac: ", ptb.dev.eth[1].mac);
		show_ip("eth1_ip: ", ptb.dev.eth[1].ip);
		show_ip("eth1_mask: ", ptb.dev.eth[1].mask);
		show_ip("eth1_gw: ", ptb.dev.eth[1].gw);
#endif
		uart_putstr("auto_dl: ");
		uart_putdec(ptb.dev.auto_dl);
		uart_putstr("\r\n");

		/* tftpd */
		show_ip("tftpd: ", ptb.dev.tftpd);

		/* pri_addr */
		uart_putstr("pri_addr: 0x");
		uart_puthex(ptb.dev.pri_addr);
		uart_putstr("\r\n");

		/* pri_file */
		uart_putstr("pri_file: ");
		for (i = 0; i < sizeof(ptb.dev.pri_file); i++) {
			if (ptb.dev.pri_file[i] == '\0')
				break;
			uart_putchar(ptb.dev.pri_file[i]);
		}
		uart_putstr("\r\n");

		/* pri_comp */
		uart_putstr("pri_comp: ");
		uart_putdec(ptb.dev.pri_comp);
		uart_putstr("\r\n");

		/* rmd_addr */
		uart_putstr("rmd_addr: 0x");
		uart_puthex(ptb.dev.rmd_addr);
		uart_putstr("\r\n");

		/* rmd_file */
		uart_putstr("rmd_file: ");
		for (i = 0; i < sizeof(ptb.dev.rmd_file); i++) {
			if (ptb.dev.rmd_file[i] == '\0')
				break;
			uart_putchar(ptb.dev.rmd_file[i]);
		}
		uart_putstr("\r\n");

		/* rmd_comp */
		uart_putstr("rmd_comp: ");
		uart_putdec(ptb.dev.rmd_comp);
		uart_putstr("\r\n");

		/* dsp_addr */
		uart_putstr("dsp_addr: 0x");
		uart_puthex(ptb.dev.dsp_addr);
		uart_putstr("\r\n");

		/* dsp_file */
		uart_putstr("dsp_file: ");
		for (i = 0; i < sizeof(ptb.dev.dsp_file); i++) {
			if (ptb.dev.dsp_file[i] == '\0')
				break;
			uart_putchar(ptb.dev.dsp_file[i]);
		}
		uart_putstr("\r\n");

		/* dsp_comp */
		uart_putstr("dsp_comp: ");
		uart_putdec(ptb.dev.dsp_comp);
		uart_putstr("\r\n");

	} else if (strcmp(argv[1], "wifi") == 0) {
		flpart_table_t ptb;

		/* Read the partition table */
		rval = flprog_get_part_table(&ptb);
		if (rval < 0)
			return rval;

		show_mac("wifi0_mac: ", ptb.dev.wifi[0].mac);
		show_ip("wifi0_ip: ", ptb.dev.wifi[0].ip);
		show_ip("wifi0_mask: ", ptb.dev.wifi[0].mask);
		show_ip("wifi0_gw: ", ptb.dev.wifi[0].gw);
		show_mac("wifi1_mac: ", ptb.dev.wifi[1].mac);
		show_ip("wifi1_ip: ", ptb.dev.wifi[1].ip);
		show_ip("wifi1_mask: ", ptb.dev.wifi[1].mask);
		show_ip("wifi1_gw: ", ptb.dev.wifi[1].gw);

	} else if (strcmp(argv[1], "usb_eth") == 0) {
		flpart_table_t ptb;

		rval = flprog_get_part_table(&ptb);
		if (rval < 0)
			return rval;

		show_mac("usb_eth0_mac: ", ptb.dev.usb_eth[0].mac);
		show_ip("usb_eth0_ip: ", ptb.dev.usb_eth[0].ip);
		show_ip("usb_eth0_mask: ", ptb.dev.usb_eth[0].mask);
		show_ip("usb_eth0_gw: ", ptb.dev.usb_eth[0].gw);
		show_mac("usb_eth1_mac: ", ptb.dev.usb_eth[1].mac);
		show_ip("usb_eth1_ip: ", ptb.dev.usb_eth[1].ip);
		show_ip("usb_eth1_mask: ", ptb.dev.usb_eth[1].mask);
		show_ip("usb_eth1_gw: ", ptb.dev.usb_eth[1].gw);

	} else {
		uart_putstr("show '");
		uart_putstr(argv[1]);
		uart_putstr("' is not recognized...\r\n");
	}

	return -2;
}

static char help_show[] =
	"\r\n"
	"show logo      - boot loader logo\r\n"
#if defined(ENABLE_FLASH)
	"show flash     - flash info and allocation scheme\r\n"
	"show ptb       - flash partition table\r\n"
	"show bb        - show NAND bad blocks\r\n"
#endif
#if defined(NAND_SUPPORT_BBT)
	"show bbt       - show NAND bad block table\r\n"
#endif
	"show wifi      - show wifi infomations\r\n"
	"show usb_eth   - show usb ethernet infomations\r\n"
#if defined(ENABLE_FLASH) || defined(ENABLE_SD)
	"show meta      - show meta\r\n"
#endif
	"show netboot   - netboot parameters\r\n"
	"Display various system properties\r\n";

__CMDLIST(cmd_show, "show", help_show);
