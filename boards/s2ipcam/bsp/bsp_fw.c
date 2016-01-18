/**
 * boards/s2ipcam/bsp/bsp_fw.c
 *
 * Author: Anthony Ginger <mapfly@gmail.com>
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include <fio/firmfl_api.h>

/* ==========================================================================*/
void hook_pre_memfwprog(fwprog_cmd_t *fwprog_cmd)
{
	if (fwprog_cmd) {
		switch(fwprog_cmd->cmd[0]) {
			 case 0xb007: {
				u32 i = 0;
				for (i = PART_BST; i < TOTAL_FW_PARTS; ++ i) {
					flprog_erase_partition(i);
				}
			 }break;
			 case 0x0005: {
				u32 i = 0;
				for (i = PART_PBA; i < TOTAL_FW_PARTS; ++ i) {
					flprog_erase_partition(i);
				}
			 }break;
			 case 0x0A11: {
				flprog_erase_partition(PART_MAX);
			 }break;
#if defined(NAND_SUPPORT_BBT)
			 case 0x0BB7: {
				nand_erase_bbt();
			 }break;
#endif
			 default:break;
		}
	}
}

void hook_post_memfwprog(fwprog_cmd_t *fwprog_cmd)
{
	if (fwprog_cmd) {
		flpart_table_t ptb;
		int rval = flprog_get_part_table(&ptb);
		if (rval < 0) {
			uart_putstr("PTB load error!\r\n");
		} else {
			u8* verStr = fwprog_cmd->data + sizeof(flpart_table_t);
			u32 magic = verStr[0]<<24 | verStr[1]<<16 | verStr[2]<<8 | verStr[3];
			u32 ver = verStr[4]<<24 | verStr[5]<<16 | verStr[6]<<8 | verStr[7];
			if ((magic == (('A'<<24) | ('m'<<16) | ('b'<<8) | 'a')) && (ver > 0x03030100)) {
				flpart_table_t* ptbData = (flpart_table_t*)fwprog_cmd->data;
				memset(ptb.dev.sn, 0, sizeof(ptb.dev.sn));
				strncpy(ptb.dev.sn, ptbData->dev.sn, strlen(ptbData->dev.sn));
				memcpy(ptb.dev.eth, ptbData->dev.eth, sizeof(ptbData->dev.eth));
				memcpy(ptb.dev.wifi, ptbData->dev.wifi, sizeof(ptbData->dev.wifi));
				memcpy(ptb.dev.usb_eth, ptbData->dev.usb_eth, sizeof(ptbData->dev.usb_eth));
			} else {
				memset(ptb.dev.sn, 0, sizeof(ptb.dev.sn));
				if (strlen((const char*)fwprog_cmd->data) > 0) {
					strncpy(ptb.dev.sn, (const char*)fwprog_cmd->data, sizeof(ptb.dev.sn));
				}
			}
			rval = flprog_set_part_table(&ptb);
			if (rval < 0) {
				uart_putstr("PTB save error!\r\n");
			}
		}
	}
#ifdef ENABLE_EMMC_BOOT
	sdmmc_command(0, 0xf0f0f0f0, 1000);
#endif

	writel(SOFT_RESET_REG, 0x2);
	writel(SOFT_RESET_REG, 0x3);
}

