/**
 * @file system/include/flash/nand_common.h
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

#ifndef __FLASH_NAND_COMMON_H__
#define __FLASH_NAND_COMMON_H__

#include <basedef.h>
#include <ambhw.h>
#include <fio/partition.h>

/**
 * Define for xdhost and nand_host
 */
#define MAIN_ONLY		0
#define SPARE_ONLY		1
#define MAIN_ECC		2
#define SPARE_ECC		3
#define SPARE_ONLY_BURST	4
#define SPARE_ECC_BURST		5

/* ------------------------------------------------------------------------- */
/* NAND HOST                                                                 */
/* ------------------------------------------------------------------------- */

/* Define for plane mapping */
/* Device does not support copyback command */
#define NAND_PLANE_MAP_0	0
/* plane address is according the lowest block address */
#define NAND_PLANE_MAP_1	1
/* plane address is according the highest block address */
#define NAND_PLANE_MAP_2	2
/* plane address is according the lowest and highest block address */
#define NAND_PLANE_MAP_3	3

/* Minimun number of translation tables in trl_blocks */
#define NAND_TRL_TABLES		16
#define NAND_SS_BLK_PAGES	1
#define NAND_IMARK_PAGES	1
#define NAND_BBINFO_SIZE	(4 * 1024)

/* NAND FTL partition ID and max partition number. */
#define NAND_FTL_PART_ID_RAW		MP_RAW
#define NAND_FTL_PART_ID_STORAGE2	MP_STG2
#define NAND_FTL_PART_ID_STORAGE	MP_STG
#define NAND_FTL_PART_ID_PREF		MP_PRF
#define NAND_FTL_PART_ID_CALIB		MP_CAL
#define NAND_MAX_FTL_PARTITION		MP_MAX

#define PREF_ZONE_THRESHOLD		1000
#define PREF_RSV_BLOCKS_PER_ZONET	24
#define PREF_MIN_RSV_BLOCKS_PER_ZONE	5
#define PREF_TRL_TABLES			0

#define CALIB_ZONE_THRESHOLD		1000
#define CALIB_RSV_BLOCKS_PER_ZONET	24
#define CALIB_MIN_RSV_BLOCKS_PER_ZONE	5
#define CALIB_TRL_TABLES		0

#define RAW_ZONE_THRESHOLD		1000
#define RAW_RSV_BLOCKS_PER_ZONET	24
#define RAW_MIN_RSV_BLOCKS_PER_ZONE	24
#define RAW_TRL_TABLES			NAND_TRL_TABLES

#define STG_ZONE_THRESHOLD		1000
#define STG_RSV_BLOCKS_PER_ZONET	24
#define STG_MIN_RSV_BLOCKS_PER_ZONE	24
#define STG_TRL_TABLES			NAND_TRL_TABLES

#define STG2_ZONE_THRESHOLD		1000
#define STG2_RSV_BLOCKS_PER_ZONET	24
#define STG2_MIN_RSV_BLOCKS_PER_ZONE	24
#define STG2_TRL_TABLES			NAND_TRL_TABLES

/* Definitions for filesystem storage1/storage2 in NAND flash. */
#define FLFS_ID_STG	0
#define FLFS_ID_STG2	1

#if defined(ENABLE_FLASH)
#if defined(MP_STG2_SIZE) && (MP_STG2_SIZE > 0) && !defined(CONFIG_NAND_NONE)
#define FLFS_ID_MAX	2
#else
#define FLFS_ID_MAX	1
#endif
#else	/* ENABLE_FLASH */
#define FLFS_ID_MAX	0
#endif

#define FLFS_CHECK_ID(id)	K_ASSERT(id < FLFS_ID_MAX)


#ifndef __ASM__

__BEGIN_C_PROTO__

/**
 * Data structure describing media partition in NAND flash.
 */
typedef struct nand_mp_s {
	u32 	sblk[MP_MAX];	/* Start block */
	u32	nblk[MP_MAX];	/* Number of blocks */
	u32	rblk[MP_MAX];	/* Number of reserved blocks */
	u32	nzone[MP_MAX];	/* Number of zones */
	u32	trlb[MP_MAX];	/* Number of translation tables in trl_blocks */
} nand_mp_t;

/**
 * Data structure describing a NAND flash device(s) configuration.
 */
typedef struct nand_dev_s
{
	u32	id;		/**< ID */
	u8	id5;		/**< ID5 */
	char	name[64];	/**< Name */
	int	db_id;		/**< Nand DB id, negative value for none */
	u32	control;	/**< Control register setting */
	u8	present;	/**< Device is present */
	int	devices;	/**< Number of devices(chips) */
	int	ldevices;	/**< Number of logic devices
				  (usable while NAND_NUM_INTLVE > 1) */
	int	total_banks;	/**< Totals banks in system */
	int	format[2];	/**< File system format ID for STG */
	u32	status;		/**< Status of nand_dev */
#define NAND_OUT_OF_SPACE	0x1

	/** Device info */
	struct {
		int	main_size;		/**< Main area size */
		int	spare_size;		/**< Spare area size */
		int	page_size;		/**< Page size */
		int	sector_size;		/**< Sector size */
		int	pages_per_block;	/**< Pages per block */
		int	sectors_per_page;	/**< Sectors per page */
		int	sectors_per_block;	/**< Sectors per block */
		int	blocks_per_plane;	/**< Blocks per plane */
		int	blocks_per_zone;	/**< Blocks per zone */
		int	blocks_per_bank;	/**< Blocks per bank */
		int	planes_per_bank;	/**< Planes per bank */
		int	total_blocks;		/**< Total blocks */
		int	total_planes;		/**< Total planes */
		int	total_zones;		/**< Total zones */
		int	plane_mask;		/**< Plane address mask */
		int	plane_map;		/**< Plane map */
		int	column_cycles;		/**< Column access cycles */
		int	page_cycles;		/**< Page access cycles */
		int	id_cycles;		/**< ID access cycles */
		int	chip_width;		/**< Chip data bus width */
		int	chip_size;		/**< Chip size (MB) */
		int	bus_width;		/**< Data bus width of ctrl. */
		int	banks;			/**< Banks in device */
		int	cap;			/**< Capability of device */
#define NAND_CAP_CACHE_CB	0x1
#define NAND_CAP_CACHE_PROG	0x2
		int ecc_bits;			/**< ECC bits per 512B of device */
#define NAND_ECC_BIT_1			0x1
#define NAND_ECC_BIT_6			0x6
#define NAND_ECC_BIT_8			0x8
	} devinfo;

	/** Device logic info */
	struct {
		int	intlve;			/**< Number of interleave */
		int	main_size;		/**< Main area size */
		int	spare_size;		/**< Spare area size */
		int	page_size;		/**< Page size */
		int	sector_size;		/**< Sector size */
		int	pages_per_block;	/**< Pages per block */
		int	sectors_per_page;	/**< Sectors per page */
		int	sectors_per_block;	/**< Sectors per block */
		int	blocks_per_plane;	/**< Blocks per plane */
		int	blocks_per_bank;	/**< Blocks per bank */
		int	planes_per_bank;	/**< Planes per bank */
		int	total_blocks;		/**< Total blocks */
		int	total_planes;		/**< Total planes */
		int	total_zones;		/**< Total zones */
	} devlinfo;

	/* Media partition info. Some of fileds have the same meaning with */
	/* fields in dev_part_info. To have the duplicate data is for */
	/* backward compatiability purpose. */
	nand_mp_t	mp;

	/** Device partition info */
	struct {
		int	start_block;	/**< Start block of device partition */
		int	start_trltbl;	/**< Starting block of nftl init tbl */
		int	trl_blocks;	/**< Blocks used for translation tbl */
		int	pages_per_tchunk; /**< Pages of tchunk */
		int	start_part;	/**< Starting block of ftl partition */
		int	ftl_blocks;	/**< Total blocks in ftl partition */
		int	total_zones;	/**< Total zones in ftl partition */
		int	total_blocks;	/**< Total blocks in partition */
		int	pblks_per_zone;	/**< Physical blocks per zone */
		int	lblks_per_zone;	/**< Logical blocks per zone */
		int	rblks_per_zone; /**< Reserved blocks per zone */
	} dev_part_info[NAND_MAX_FTL_PARTITION];

	/** Chip(s) timing parameters */
	struct {
		u32	t0_reg;
		u32	t1_reg;
		u32	t2_reg;
		u32	t3_reg;
		u32	t4_reg;
		u32	t5_reg;
#if (ONENAND_NOR_SUPPORT == 1)
		u32	t6_reg;
		u32	t7_reg;
		u32	t8_reg;
		u32	t9_reg;
#endif
	} timing;
} nand_dev_t;

/**
 * NAND with page 512 byte spare area layout. It follows the Samsung's
 * NAND spare area definition and with some private definition field.
 */
typedef __ARMCC_PACK__ struct nand_small_spare_s
{
	u8 lsn[3];	/* logical sector number */
	u8 wc[2];	/* status flag against suddden power failure during write*/
	u8 bi;		/* bad block information */
	u8 main_ecc[3];	/* ecc code for main area data */
	u16 lsn_ecc;	/* ecc code for lsn data */
	u32 usp;	/* used pages bit pattern in block */
	u8 rsv;		/* reserved area */
} __ATTRIB_PACK__ nand_small_spare_t;

/**
 * NAND with page 2K byte spare area layout. It follows the Samsung's
 * NAND spare area definition and with some private definition field.
 */
typedef __ARMCC_PACK__ struct nand_big_spare_s
{
	u8 bi;		/* bad block information */
	u8 rsv;		/* reserved area */
	u8 lsn[3];	/* logical sector number */
	u8 usp1;	/* used sectors bit pattern */
	u8 wc[2];	/* status flag against suddden power failure during write*/
	u8 main_ecc[3];	/* ecc code for main area data */
	u16 lsn_ecc;	/* ecc code for lsn data */
	u8 usp2[3];	/* used sectors bit pattern */
} __ATTRIB_PACK__ nand_big_spare_t;

#define NAND_BCH6_BYTES	10
#define NAND_BCH8_BYTES	13

struct nand_oobfree {
	u32 offset;
	u32 length;
};

#if	!defined(__BUILD_AMBOOT__)
extern void nand_get_rbz(int nblk, int zonet, int rb_per_zt,
		  int min_rb, int *zones, int *rb_per_zone);
extern void nand_get_media_part(nand_dev_t *dev, nand_mp_t *mp,
		u32 start_block);
extern void nand_init_media_part(nand_dev_t *dev, nand_mp_t *mp);

extern void nand_set_scm1(void);
extern void nand_set_scm2(void);
#endif

__END_C_PROTO__

#endif /* __ASM__ */

#endif
