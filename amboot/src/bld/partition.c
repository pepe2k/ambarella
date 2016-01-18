/**
 * @file system/src/bld/partition.c
 *
 * Firmware partition information.
 *
 * History:
 *    2009/10/06 - [Evan Chen] created file
 *
 * Copyright (C) 2008-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <fio/partition.h>

const char *g_part_str[] = {"bst", "ptb", "bld", "hal", "pba",
			    "pri", "sec", "bak", "rmd", "rom",
			    "dsp", "lnx", "swp", "add", "adc",
			    "raw", "stg2", "stg", "prf", "cal",
			    "all"};

int g_part_dev[PART_MAX] =
{
	PART_BST_DEV,	/**< BST */
	PART_PTB_DEV,	/**< PTB */
	PART_BLD_DEV,	/**< BLD */
	PART_HAL_DEV,	/**< HAL */
	PART_PBA_DEV,	/**< PBA */
	PART_PRI_DEV,	/**< PRI */
	PART_SEC_DEV,	/**< SEC */
	PART_BAK_DEV,	/**< BAK */
	PART_RMD_DEV,	/**< RMD */
	PART_ROM_DEV,	/**< ROM */
	PART_DSP_DEV,	/**< DSP */
	PART_LNX_DEV,	/**< LNX */
	PART_SWP_DEV,	/**< SWP */
	PART_ADD_DEV,	/**< ADD */
	PART_ADC_DEV,	/**< ADC */
	PART_RAW_DEV,	/**< RAW  */
	PART_STG2_DEV, 	/**< STG2 */
	PART_STG_DEV, 	/**< STG  */
	PART_PRF_DEV, 	/**< PRF  */
	PART_CAL_DEV  	/**< CAL  */
};

void get_part_size(int *part_size)
{
#ifdef AMBOOT_BST_FIXED_SIZE
	part_size[PART_BST] = AMBOOT_BST_FIXED_SIZE;
#else
	part_size[PART_BST] = AMBOOT_BST_SIZE;
#endif
	part_size[PART_PTB] = AMBOOT_PTB_SIZE;
	part_size[PART_BLD] = AMBOOT_BLD_SIZE;
	part_size[PART_HAL] = AMBOOT_HAL_SIZE;
	part_size[PART_PBA] = AMBOOT_PBA_SIZE;
	part_size[PART_PRI] = AMBOOT_PRI_SIZE;
	part_size[PART_SEC] = AMBOOT_SEC_SIZE;
	part_size[PART_BAK] = AMBOOT_BAK_SIZE;
	part_size[PART_RMD] = AMBOOT_RMD_SIZE;
	part_size[PART_ROM] = AMBOOT_ROM_SIZE;
	part_size[PART_DSP] = AMBOOT_DSP_SIZE;
	part_size[PART_LNX] = AMBOOT_LNX_SIZE;
	part_size[PART_SWP] = AMBOOT_SWP_SIZE;
	part_size[PART_ADD] = AMBOOT_ADD_SIZE;
	part_size[PART_ADC] = AMBOOT_ADC_SIZE;
}

void set_part_dev(void)
{
	u32 n, dev = 0;
	u32 boot_from = rct_boot_from();

	dev = (boot_from & BOOT_FROM_NAND)    ? BOOT_DEV_NAND :
	      (boot_from & BOOT_FROM_SDMMC)   ? BOOT_DEV_SM :
	      (boot_from & BOOT_FROM_ONENAND) ? BOOT_DEV_ONENAND :
	      (boot_from & BOOT_FROM_SNOR)    ? BOOT_DEV_SNOR :
	      (boot_from & BOOT_FROM_NOR)     ? FW_DEV_NOR : 0;

	K_ASSERT(dev != 0);

	for (n = 0; n < PART_MAX; n++) {
		if (g_part_dev[n] == BOOT_DEV_FROM_RCT)
			g_part_dev[n] = dev;
	}
}

u32 get_part_dev(u32 id)
{
	K_ASSERT(id < PART_MAX);
	return g_part_dev[id];
}

