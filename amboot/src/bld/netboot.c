/**
 * system/src/bld/netboot.c
 *
 * History:
 *    2006/10/24 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#include <fio/firmfl.h>

#if (ETH_INSTANCES >= 1)

int netboot(const char *cmdline, int verbose)
{
	int rval = 0;
	flpart_table_t ptb;
	u32 rmd_start = 0;
	u32 rmd_size = 0;
	void *jump_addr = NULL;
	int eth0_config = 1;
#if (ETH_INSTANCES >= 2)
	int eth1_config = 1;
#endif

	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto cmd_cortex_load_net_exit;

	if ((ptb.dev.eth[0].mac[0] == 0x0 &&
		ptb.dev.eth[0].mac[1] == 0x0  &&
		ptb.dev.eth[0].mac[2] == 0x0  &&
		ptb.dev.eth[0].mac[3] == 0x0  &&
		ptb.dev.eth[0].mac[4] == 0x0  &&
		ptb.dev.eth[0].mac[5] == 0x0) ||
		ptb.dev.eth[0].ip == 0x0 ||
		ptb.dev.eth[0].mask == 0x0 ) {
		eth0_config = 0;
		uart_putstr("netboot parameter 'eth0' is invalid!\r\n");
	 }

#if (ETH_INSTANCES >= 2)
	if ((ptb.dev.eth[1].mac[0] == 0x0 &&
		ptb.dev.eth[1].mac[1] == 0x0  &&
		ptb.dev.eth[1].mac[2] == 0x0  &&
		ptb.dev.eth[1].mac[3] == 0x0  &&
		ptb.dev.eth[1].mac[4] == 0x0  &&
		ptb.dev.eth[1].mac[5] == 0x0) ||
		ptb.dev.eth[1].ip == 0x0 ||
		ptb.dev.eth[1].mask == 0x0 ) {
		eth1_config = 0;
		uart_putstr("netboot parameter 'eth1' is invalid!\r\n");
	}
#endif

		 if ((eth0_config
#if (ETH_INSTANCES >= 2)
			| eth1_config
#endif
			) == 0) {
		uart_putstr("No network device is valid!\r\n");
		return -2;
	}

	if (ptb.dev.tftpd == 0x0) {
		if (verbose)
			putstr("Invalid: tftpd!\r\n");
		rval = -5;
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_net_wait_ready(DEFAULT_NET_WAIT_TMO);
	if (rval < 0) {
		if (verbose)
			putstr("link down ...\r\n");
		bld_net_down();
		bld_net_init(verbose);
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_udp_bind(ptb.dev.tftpd, 0);
	if (rval < 0) {
		if (verbose)
			putstr("Bind UDP fail ...\r\n");
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_tftp_load(ptb.dev.dsp_addr, ptb.dev.dsp_file, verbose);

	rval = bld_tftp_load(ptb.dev.rmd_addr, ptb.dev.rmd_file, verbose);
	if (rval >= 0) {
		rmd_start = ptb.dev.rmd_addr;
		rmd_size  = rval;
	}

	rval = bld_tftp_load(ptb.dev.pri_addr, ptb.dev.pri_file, verbose);
	if (rval >= 0) {
		jump_addr = (void *)ptb.dev.pri_addr;
	}

	bld_udp_close();

	if (jump_addr != NULL) {
		if (verbose) {
			putstr("Jumping to 0x");
			puthex((u32)jump_addr);
			putstr("\r\n");
		}
		bld_net_down();
		setup_tags(jump_addr, cmdline, DRAM_START_ADDR, DRAM_SIZE,
			rmd_start, rmd_size, verbose);
		jump_to_kernel(jump_addr);
	}

cmd_cortex_load_net_exit:
	return rval;
}

#endif

