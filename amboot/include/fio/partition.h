/**
 * @file system/include/fio/partition.h
 *
 * Firmware partition information.
 *
 * History:
 *    2009/10/06 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2008-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FIRMWARE_PARTITION_H__
#define __FIRMWARE_PARTITION_H__

#include <basedef.h>

/**
 * Partitions on device.
 */

/* ------------------------------------------------------------------------- */
/* Below are firmware partitions. (with pre-built image) */
#define PART_BST	0
#define PART_PTB	1
#define PART_BLD	2
#define PART_HAL	3
#define PART_PBA	4
#define PART_PRI	5
#define PART_SEC	6
#define PART_BAK	7
#define PART_RMD	8
#define PART_ROM	9
#define PART_DSP	10
#define PART_LNX	11
/* ------------------------------------------------------------------------- */
/* Below are general purpose partitions. (with/without pre-built image) */
#define PART_SWP	12
#define PART_ADD	13
#define PART_ADC	14
/* ------------------------------------------------------------------------- */
/* Below are media(nftl) partitions. (without pre-built image) */
#define PART_RAW	15
#define PART_STG2	16
#define PART_STG	17
#define PART_PRF	18
#define PART_CAL	19
/* ------------------------------------------------------------------------- */
#define PART_MAX		20
#define PART_MAX_WITH_RSV	32


#define HAS_IMG_PARTS		15
#define HAS_NO_IMG_PARTS	0
#define TOTAL_FW_PARTS		(HAS_IMG_PARTS + HAS_NO_IMG_PARTS)

#define TOTAL_MP_PARTS		5

/**
 * General-purpose partitions.
 */
#define GP_SWP		0	/* Swap partition in linux */
#define GP_ADD		1	/* Android data partition */
#define GP_ADC		2	/* Android cache partition */
#define GP_MAX		3

/**
 * Media partitions. (Must have the same order as PART_RAW, PART_STG ...)
 * These should synchronize with NAND_FTL_PART_ID_XXX in nand_common.h.
 */
#define MP_RAW			0
#define MP_STG2			1
#define MP_STG			2
#define MP_PRF			3
#define MP_CAL			4
#define MP_MAX			5
#define MP_MAX_WITH_RSV		10

/* ------------------------------------------------------------------------- */
/**
 * Device to contain partitions.
 */
#define FW_DEV_NAND	0
#define FW_DEV_NOR	1
#define FW_DEV_SM	2
#define FW_DEV_MAX	3

/* ------------------------------------------------------------------------- */
/* Partitions size 							     */
/* ------------------------------------------------------------------------- */
#ifndef AMBOOT_HAL_SIZE
#define AMBOOT_HAL_SIZE 0
#endif

#ifndef AMBOOT_SEC_SIZE
#define AMBOOT_SEC_SIZE	0
#endif

#ifndef AMBOOT_LNX_SIZE
#define AMBOOT_LNX_SIZE	0
#endif

#ifndef AMBOOT_SWP_SIZE
#define AMBOOT_SWP_SIZE	0
#endif

#ifndef AMBOOT_ADD_SIZE
#define AMBOOT_ADD_SIZE	0
#endif

#ifndef AMBOOT_ADC_SIZE
#define AMBOOT_ADC_SIZE	0
#endif

/* ------------------------------------------------------------------------- */
/* Partitions location 							     */
/* ------------------------------------------------------------------------- */
#define BOOT_DEV_FROM_RCT	0

#ifndef PART_BST_DEV
#define PART_BST_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_PTB_DEV
#define PART_PTB_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_BLD_DEV
#define PART_BLD_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_HAL_DEV
#define PART_HAL_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_PBA_DEV
#define PART_PBA_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_PRI_DEV
#define PART_PRI_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_SEC_DEV
#define PART_SEC_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_BAK_DEV
#define PART_BAK_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_RMD_DEV
#define PART_RMD_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_ROM_DEV
#define PART_ROM_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_DSP_DEV
#define PART_DSP_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_LNX_DEV
#define PART_LNX_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_SWP_DEV
#define PART_SWP_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_ADD_DEV
#define PART_ADD_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_ADC_DEV
#define PART_ADC_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_RAW_DEV
#define PART_RAW_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_STG2_DEV
#define PART_STG2_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_STG_DEV
#define PART_STG_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_PRF_DEV
#define PART_PRF_DEV		BOOT_DEV_FROM_RCT
#endif
#ifndef	PART_CAL_DEV
#define PART_CAL_DEV		BOOT_DEV_FROM_RCT
#endif

/* BST size is fixed to 2k in HAL, hard coding to 2k here. */
#define AMBOOT_BST_FIXED_SIZE	2048

#ifndef __ASM__
/**
 * Partition string(for BLD). (bst, ptb, bld ...)
 */
extern const char *g_part_str[];

/**
 * Partition string(for PRI). (bst, ptb, bld ...)
 */
extern const char *part_name[];

/**
 * (for BLD).
 */
extern int g_part_dev[PART_MAX];

/**
 * (for PRI).
 */
extern int part_dev[PART_MAX];

/**
 * The names of nftl partition.
 */
extern char *g_nftl_str[];

/* Export APIs */
extern void get_part_size(int * part_size);
extern void set_part_dev(void);
extern u32 get_part_dev(u32 id);

#endif

/* Define Amboot support Linux MTD bad block table management. */
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
#define NAND_SUPPORT_BBT
#else
#undef NAND_SUPPORT_BBT
#endif

#endif
