/**
 * system/src/bld/atag.c
 *
 * History:
 *    2006/12/31 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <atag.h>
#include <bldfunc.h>

u8 *atag_data;
struct tag *params; /* used to point at the current tag */
static char extra_cmdline[128];

static void setup_core_tag(void *address, long pagesize)
{
	params = (struct tag *) address;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 1;
	params->u.core.pagesize = pagesize;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}

static void setup_mem_tag(u32 hdrtag, u32 start, u32 len)
{
	params->hdr.tag = hdrtag;
	params->hdr.size = tag_size(tag_mem32);

	params->u.mem.start = start;
	params->u.mem.size = len;

	params = tag_next(params);
}

static void setup_initrd_tag(u32 start, u32 size)
{
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size(tag_initrd);

	params->u.initrd.start = start;
	params->u.initrd.size = size;

	params = tag_next(params);
}

#if defined(AMBARELLA_LINUX_LAYOUT)
static void setup_serialnr_tag(u32 hdrtag, u32 low, u32 high)
{
	params->hdr.tag = hdrtag;
	params->hdr.size = tag_size(tag_serialnr);

	params->u.serialnr.low = low;
	params->u.serialnr.high = high;

	params = tag_next(params);
}

static void setup_tag_revision(u32 rev)
{
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size(tag_revision);

	params->u.revision.rev = rev;

	params = tag_next(params);
}

#if (USE_HAL == 1)
/* Reuse tag_ramdisk to pass HAL info
 * start = Phy Address
 * size = size
 * flags = Virtual Address
 */
static void setup_hal_tag(u32 pstart, u32 size, u32 vstart)
{
	params->hdr.tag = ATAG_AMBARELLA_HAL;
	params->hdr.size = tag_size(tag_ramdisk);

	params->u.ramdisk.start = pstart;
	params->u.ramdisk.size = size;
	params->u.ramdisk.flags = vstart;

	params = tag_next(params);
}
#endif

#endif

static void setup_cmdline_tag(const char *cmdline)
{
	int					slen;
	int					cmdlen;
	char					*cmdend;

	if ((cmdline == NULL) || (*cmdline == '\0'))
		return;

	cmdlen = strlen(extra_cmdline);
	slen = strlen(cmdline);
	if (slen > MAX_CMDLINE_LEN)
		slen = MAX_CMDLINE_LEN;
	if ((slen + cmdlen) > MAX_CMDLINE_LEN)
		cmdlen = MAX_CMDLINE_LEN - slen;

	cmdend = strncpy(params->u.cmdline.cmdline, cmdline, slen);
	if (cmdlen)
		cmdend = strncpy(cmdend, extra_cmdline, cmdlen);
	slen = (cmdend - params->u.cmdline.cmdline) + 3;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = ((sizeof(struct tag_header) + slen) >> 2);
	params = tag_next(params);
}

static void setup_end_tag(void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

/**
 * Set up tags for booting Linux kernel.
 */
void setup_tags(void *jump_addr, const char *cmdline,
	u32 mem_base, u32 mem_size,
	u32 initrd2_start, u32 initrd2_size,
	int verbose)
{
	u32 atag_data_size = 4096;
#if defined(AMBARELLA_LINUX_LAYOUT)
	int block_size;
	flpart_table_t ptb;
	u32 low, high;
	u32 kernelp, kernels;
	u32 bsbp, bsbs;
	u32 dspp, dsps;
#endif

	if (((u32)jump_addr) > (DRAM_START_ADDR + 0x00200000)) {
		/* Special handling for OS allocated in higer memory region */
		mem_base = ((u32)jump_addr) & 0xfff00000;
		mem_size -= (mem_base - DRAM_START_ADDR);
		atag_data = (void *)mem_base;
	} else {
		atag_data = (void *)(DRAM_START_ADDR + 0x000c0000);
	}

	memzero(atag_data, atag_data_size);
	setup_core_tag(atag_data, atag_data_size);
	if (verbose) {
		putstr("atag_data: 0x");
		puthex((u32)atag_data);
		putstr("\r\n");
	}

#if defined(AMBARELLA_LINUX_LAYOUT)
	ambarella_linux_mem_layout(&kernelp, &kernels,
		&bsbp, &bsbs, &dspp, &dsps);
	bsbp = ARM11_TO_CORTEX(bsbp);
	dspp = ARM11_TO_CORTEX(dspp);
	kernelp = ARM11_TO_CORTEX(kernelp);
	if (verbose) {
		putstr("kernelp: 0x");
		puthex(kernelp);
		putstr(" kernels: 0x");
		puthex(kernels);
		putstr("\r\n");

		putstr("bsbp: 0x");
		puthex(bsbp);
		putstr(" bsbs: 0x");
		puthex(bsbs);
		putstr("\r\n");

		putstr("dspp: 0x");
		puthex(dspp);
		putstr(" dsps: 0x");
		puthex(dsps);
		putstr("\r\n");
	}
	setup_mem_tag(ATAG_MEM, kernelp, kernels);
	setup_mem_tag(ATAG_AMBARELLA_BSB, bsbp, bsbs);
	setup_mem_tag(ATAG_AMBARELLA_DSP, dspp, dsps);

	setup_tag_revision(AMBOOT_BOARD_ID);
	if (verbose) {
		putstr("AMBOOT_BOARD_ID: 0x");
		puthex(AMBOOT_BOARD_ID);
		putstr("\r\n");
	}

	if (flprog_get_part_table(&ptb) >= 0) {
		high = ptb.dev.eth[0].mac[5];
		high <<= 8;
		high |= ptb.dev.eth[0].mac[4];
		low = ptb.dev.eth[0].mac[3];
		low <<= 8;
		low |= ptb.dev.eth[0].mac[2];
		low <<= 8;
		low |= ptb.dev.eth[0].mac[1];
		low <<= 8;
		low |= ptb.dev.eth[0].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_ETH0, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_ETH0: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}

		high = ptb.dev.eth[1].mac[5];
		high <<= 8;
		high |= ptb.dev.eth[1].mac[4];
		low = ptb.dev.eth[1].mac[3];
		low <<= 8;
		low |= ptb.dev.eth[1].mac[2];
		low <<= 8;
		low |= ptb.dev.eth[1].mac[1];
		low <<= 8;
		low |= ptb.dev.eth[1].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_ETH1, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_ETH1: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}

		high = ptb.dev.wifi[0].mac[5];
		high <<= 8;
		high |= ptb.dev.wifi[0].mac[4];
		low = ptb.dev.wifi[0].mac[3];
		low <<= 8;
		low |= ptb.dev.wifi[0].mac[2];
		low <<= 8;
		low |= ptb.dev.wifi[0].mac[1];
		low <<= 8;
		low |= ptb.dev.wifi[0].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_WIFI0, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_WIFI0: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}

		/*Updating with the fldev_t struct*/
		high = ptb.dev.wifi[1].mac[5];
		high <<= 8;
		high |= ptb.dev.wifi[1].mac[4];
		low = ptb.dev.wifi[1].mac[3];
		low <<= 8;
		low |= ptb.dev.wifi[1].mac[2];
		low <<= 8;
		low |= ptb.dev.wifi[1].mac[1];
		low <<= 8;
		low |= ptb.dev.wifi[1].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_WIFI1, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_WIFI1: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}

		high = ptb.dev.usb_eth[0].mac[5];
		high <<= 8;
		high |= ptb.dev.usb_eth[0].mac[4];
		low = ptb.dev.usb_eth[0].mac[3];
		low <<= 8;
		low |= ptb.dev.usb_eth[0].mac[2];
		low <<= 8;
		low |= ptb.dev.usb_eth[0].mac[1];
		low <<= 8;
		low |= ptb.dev.usb_eth[0].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_USB_ETH0, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_USB_ETH0: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}

		high = ptb.dev.usb_eth[1].mac[5];
		high <<= 8;
		high |= ptb.dev.usb_eth[1].mac[4];
		low = ptb.dev.usb_eth[1].mac[3];
		low <<= 8;
		low |= ptb.dev.usb_eth[1].mac[2];
		low <<= 8;
		low |= ptb.dev.usb_eth[1].mac[1];
		low <<= 8;
		low |= ptb.dev.usb_eth[1].mac[0];
		setup_serialnr_tag(ATAG_AMBARELLA_USB_ETH1, low, high);
		if (verbose) {
			putstr("ATAG_AMBARELLA_USB_ETH1: 0x");
			puthex(high);
			putstr(" 0x");
			puthex(low);
			putstr("\r\n");
		}
		/* updating with the fldev_t struct end*/
#if (USE_HAL == 1)
		setup_hal_tag(ARM11_TO_CORTEX(ptb.part[PART_HAL].mem_addr),
			ptb.part[PART_HAL].img_len, HAL_BASE_VIRT);
		if (verbose) {
			putstr("HAL: 0x");
			puthex(HAL_BASE_VIRT);
			putstr(" [0x");
			puthex(ARM11_TO_CORTEX(ptb.part[PART_HAL].mem_addr));
			putstr("] size: 0x");
			puthex(ptb.part[PART_HAL].img_len);
			putstr("\r\n");
		}
#endif
	}

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	block_size = flnand.main_size * flnand.pages_per_block;

	setup_serialnr_tag(ATAG_AMBARELLA_NAND_CS,
		flnand.control, block_size);

	setup_serialnr_tag(ATAG_AMBARELLA_NAND_T0,
		flnand.nandtiming0, flnand.nandtiming1);

	setup_serialnr_tag(ATAG_AMBARELLA_NAND_T1,
		flnand.nandtiming2, flnand.nandtiming3);

	setup_serialnr_tag(ATAG_AMBARELLA_NAND_T2,
		flnand.nandtiming4, flnand.nandtiming5);

	setup_serialnr_tag(ATAG_AMBARELLA_NAND_ECC,
		flnand.ecc_bits, 0);

	if (verbose) {
		putstr("NAND control: 0x");
		puthex(flnand.control);
		putstr(" block_size: 0x");
		puthex(block_size);
		putstr("\r\n");

		putstr("timing0: 0x");
		puthex(flnand.timing0);
		putstr(" timing1: 0x");
		puthex(flnand.timing1);
		putstr("\r\n");

		putstr("timing2: 0x");
		puthex(flnand.timing2);
		putstr(" timing3: 0x");
		puthex(flnand.timing3);
		putstr("\r\n");

		putstr("timing4: 0x");
		puthex(flnand.timing4);
		putstr(" timing5: 0x");
		puthex(flnand.timing5);
		putstr(" ecc bit: 0x");
		puthex(flnand.ecc_bits);
		putstr("\r\n");
	}
#endif

#else
	setup_mem_tag(ATAG_MEM, mem_base, mem_size);
#endif

	if (initrd2_start != 0x0 && initrd2_size != 0x0) {
		setup_initrd_tag(ARM11_TO_CORTEX(initrd2_start), initrd2_size);
		if (verbose) {
			putstr("initrd: 0x");
			puthex(ARM11_TO_CORTEX(initrd2_start));
			putstr(" size: 0x");
			puthex(initrd2_size);
			putstr("\r\n");
		}
	}

	if (ambarella_bapi_atag_entry)
		ambarella_bapi_atag_entry(verbose);

	if (ambarella_cortex_atag_entry)
		ambarella_cortex_atag_entry(verbose);

	if (cmdline != NULL&& cmdline[0] != '\0') {
		setup_cmdline_tag(cmdline);
		if (verbose) {
			putstr(cmdline);
			putstr("\r\n");
		}
	}

	setup_end_tag();
	clean_d_cache(atag_data, atag_data_size);
}

#ifdef CONFIG_KERNEL_DUAL_CPU
/**
 * Set up tags for arm kernel in booting dual Linux kernels.
 */
void setup_arm_tags(void *jump_addr, const char *cmdline,
	u32 mem_base, u32 mem_size,
	u32 initrd2_start, u32 initrd2_size,
	int verbose)
{
	u32 atag_data_size = 4096;
#if defined(AMBARELLA_LINUX_LAYOUT)
	u32 kernelp, kernels;
	u32 bsbp, bsbs;
	u32 dspp, dsps;
#endif

	if (((u32)jump_addr) > (DRAM_START_ADDR + 0x00200000)) {
		/* Special handling for OS allocated in higer memory region */
		mem_base = ((u32)jump_addr) & 0xfff00000;
		mem_size -= (mem_base - DRAM_START_ADDR);
		atag_data = (void *)mem_base;
	} else {
		atag_data = (void *)(DRAM_START_ADDR + 0x000c0000);
	}

	memzero(atag_data, atag_data_size);
	setup_core_tag(atag_data, atag_data_size);
	if (verbose) {
		putstr("atag_data: 0x");
		puthex((u32)atag_data);
		putstr("\r\n");
	}

#if defined(AMBARELLA_LINUX_LAYOUT)
	ambarella_linux_arm_mem_layout(&kernelp, &kernels,
		&bsbp, &bsbs, &dspp, &dsps);
	if (verbose) {
		putstr("kernelp: 0x");
		puthex(kernelp);
		putstr(" kernels: 0x");
		puthex(kernels);
		putstr("\r\n");

		putstr("bsbp: 0x");
		puthex(bsbp);
		putstr(" bsbs: 0x");
		puthex(bsbs);
		putstr("\r\n");

		putstr("dspp: 0x");
		puthex(dspp);
		putstr(" dsps: 0x");
		puthex(dsps);
		putstr("\r\n");
	}
	setup_mem_tag(ATAG_MEM, kernelp, kernels);
	setup_mem_tag(ATAG_AMBARELLA_BSB, bsbp, bsbs);
	setup_mem_tag(ATAG_AMBARELLA_DSP, dspp, dsps);

	setup_tag_revision(AMBOOT_BOARD_ID);
	if (verbose) {
		putstr("AMBOOT_BOARD_ID: 0x");
		puthex(AMBOOT_BOARD_ID);
		putstr("\r\n");
	}
#else
	setup_mem_tag(ATAG_MEM, mem_base, mem_size);
#endif

	if (initrd2_start != 0x0 && initrd2_size != 0x0) {
		setup_initrd_tag(initrd2_start, initrd2_size);
		if (verbose) {
			putstr("initrd: 0x");
			puthex(initrd2_start);
			putstr(" size: 0x");
			puthex(initrd2_size);
			putstr("\r\n");
		}
	}

	if (ambarella_bapi_arm_atag_entry)
		ambarella_bapi_arm_atag_entry(verbose);

	if (cmdline != NULL&& cmdline[0] != '\0') {
		setup_cmdline_tag(cmdline);
		if (verbose) {
			putstr(cmdline);
			putstr("\r\n");
		}
	}

	setup_end_tag();
	clean_d_cache(atag_data, atag_data_size);
}
#endif

int setup_extra_cmdline(const char *cmdline)
{
	int					slen;
	int					cmdlen;
	int					retval = 0;
	char					*pcmd;

	if ((cmdline == NULL) || (*cmdline == '\0')) {
		retval = -1;
		goto setup_extra_cmdline_exit;
	}

	cmdlen = strlen(extra_cmdline);
	slen = strlen(cmdline);
	if ((slen + cmdlen + 1) >= sizeof(extra_cmdline)) {
		retval = -1;
		goto setup_extra_cmdline_exit;
	}

	pcmd = &extra_cmdline[cmdlen];
	*pcmd++ = ' ';
	strncpy(pcmd, cmdline, slen);

setup_extra_cmdline_exit:
	return retval;
}

