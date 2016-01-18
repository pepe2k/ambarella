/**
 * @file system/include/flash/flash.h
 *
 * History:
 *	2005/03/31 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FLASH_FLASH_H__
#define __FLASH_FLASH_H__

#include <basedef.h>
#include <ambhw.h>

#ifndef __ASM__

__BEGIN_C_PROTO__

/**
 * NOR chip(s) configuration.
 */
typedef struct nor_dev_s
{
	u32	id;		/**< ID */
	char	name[64];	/**< Name */
	u32	status;		/**< Status */
	u8	present;	/**< Device is present */
	int	devices;	/**< Number of devices(chips) */
	int	total_banks;	/**< Totals banks in system */
	int	format;		/**< File system format ID */

	/** Device info */
	struct {
		int	type;		/**< Device type */
#define SNOR_TYPE_UNIFORM_BLOCK		0
#define SNOR_TYPE_TOP_BOOT_BLOCK	1
#define SNOR_TYPE_BOTTOM_BOOT_BLOCK	2

		int	chip_size;		/**< Chip size (Byte) */
		int	block1_size;		/**< Sector size */
		int	block1s_per_bank;	/**< block1s per bank */
		int	total_block1s_size;	/**< total block1s size */
		int	block2_size;		/**< Sector size */
		int	block2s_per_bank;	/**< block2s per bank */
		int	total_block2s_size;	/**< total block2s size */
		int	blocks_per_bank;	/**< blocks per bank */
		int	sector_size;		/**< Sector size */
		int	total_sectors;		/**< Total blocks */
		u32	bank_size;		/**< bank size (Byte) */
		int	banks;			/**< banks per device */
	} devinfo;

	/** Device partition info */
	struct {
		int	rsv_blocks;	/**< Blocks reserved for bld, pref... */
		int	start_trltbl;	/**< Starting block of nftl init tbl */
		int	trl_blocks;	/**< Blocks used for translation tbl */
		int	pages_per_tchunk; /**< Pages of tchunk */
		int	start_part;	/**< Starting block of partition */
		int	total_blocks;	/**< Total blocks in partition */
		int	total_zones;	/**< Total zones in partition */
		int	pblks_per_zone;	/**< Physical blocks per zone */
		int	lblks_per_zone;	/**< Logical blocks per zone */
		int	rblks_per_zone; /**< Reserved blocks per zone */
	} dev_part_info;

	/** Chip(s) timing parameters */
	struct {
		u32	t0_reg;
		u32	t1_reg;
		u32	t2_reg;
		u32	t3_reg;
		u32	t4_reg;
		u32	t5_reg;
#if defined(ONENAND_NOR_SUPPORT)
		u32	t6_reg;
		u32	t7_reg;
		u32	t8_reg;
		u32	t9_reg;
#endif
	} timing;

	void *ctr;	/**< pointer of dev_ctrl */
} nor_dev_t;

extern int flash_timing(int minmax, int val);
#define FLASH_TIMING_MIN(x) flash_timing(0, x)
#define FLASH_TIMING_MAX(x) flash_timing(1, x)

__END_C_PROTO__

#endif /* __ASM__ */

#include <flash/slcnand.h>
#include <flash/onenand.h>
#include <flash/inor.h>
#include <flash/snor.h>

#ifndef __ASM__

__BEGIN_C_PROTO__

extern nand_dev_t *nand_get_dev(void);

extern int nand_is_init(void);

extern nor_dev_t *nor_get_dev(void);

extern int nor_is_init(void);

extern void flash_set_host_type(void);

extern int flash_get_host_type(void);
#define HOST_NONENAND_TYPE	0
#define HOST_NONENOR_TYPE	0
#define HOST_SLCNAND_TYPE	1
#define HOST_ONENAND_TYPE	2
#define HOST_INOR_TYPE	3
#define HOST_SNOR_TYPE	4

__END_C_PROTO__

#endif /* __ASM__ */

#endif
