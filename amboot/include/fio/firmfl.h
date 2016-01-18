/**
 * @file system/include/fio/firmfl.h
 *
 * Embedded Flash and Firmware Programming Utilities
 *
 * History:
 *    2005/01/25 - [Charles Chiou] created file
 *    2007/04/16 - [Charles Chiou] added additional boot parameter options to
 *			the partition table
 *    2007/10/09 - [Charles Chiou] added PBA partition
 *    2007/12/30 - [Charles Chiou] added memfwprog process result table
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FIRMFL_H__
#define __FIRMFL_H__

#include <bsp.h>
#include <basedef.h>
#include <fio/partition.h>

/**
 * @defgroup firmfl Embedded Flash and Firmware Programming Utilities
 *
 * This module exports a set of utilities for programming embedded flash
 * memory area. (This does not include managerment of a file system on
 * NAND flash memory beyond the device code + data area).
 *
 * Definitions:
 * <pre>
 *
 * Boot Strapper:
 *	Code that resides in the first 2K area that is loaded and executed
 *	by the hardware. This code should contain minimal hardware
 *	initialization routines and be responsible for loading a 2nd stage
 *	code bootloader that contain more utilities.
 * Boot Loader:
 *	Code that executes after it has been loaded and placed in DRAM by the
 *	first stage boot strapper. This code should contain:
 *		- Loading firmware code, ramdisk, etc. to DRAM and execute it
 *		- Partition management utilities
 *		- Firmware programming utilities
 *		- Terminal (RS-232) interactive shell
 *		- Serial or USB download server
 *		- Diagnostic tools
 * Partition Table:
 *	A fixed region in the the flash memory that contain meta data on the
 *	location, size, and other information of 'partitions' stored in other
 *	parts of the flash area.
 * Primary Firmware:
 *	The system software code that is loaded and executed by the boot
 *	loader at system startup.
 * Backup Firmware:
 *	A valid firmware code that serves as a backup copy to the primary
 *	firmware.
 * Ramdisk:
 *	A contiguous block of data to be copied from the flash memory to
 *	the RAM area, which may later be intepreted by the operating system
 *	to be a RAM-resident file-system.
 * ROMFS:
 *	A simple read-only 'file system'.
 * Embedded Media FS:
 *	This region is not managed and known by the boot loader. It should
 *	be managed by the file system and block device driver.
 *
 * Embedded Flash Memory Layout:
 *
 *           +----------------------------+     -----
 *           | Boot Strapper              |       ^
 *           +----------------------------+       |
 *           | Partition Table            |       |
 *           +----------------------------+       |
 *           | Boot Loader                |       |
 *           +----------------------------+       |
 *           | H.A.L.                     |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Persistent BIOS App        |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Primary Firmware           |       |
 *           |                            |       |
 *           +----------------------------+       | Read-Only
 *           |                            |       | Simple Raw Flash
 *           | Secondary Firmware         |       | Format
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Backup Firmware            |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Ramdisk                    |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | ROMFS                      |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | DSP uCode                  |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | LINUX                  	  |       |
 *           |                            |       v
 *           +----------------------------+     -----
 *           |                            |       ^
 *           | Swap partition in linux 	  |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       | General purpose partitions.
 *           | Android data partition     |       | (without pre-built image)
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Android cache partition    |       |
 *           |                            |       v
 *           +----------------------------+     -----
 *           |                            |       ^
 *           | Raw Data                   |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Embedded Media FS          |       |
 *           |  (2nd FS in NAND)          |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Embedded Media FS          |       |
 *           |   (NAND)                   |       | NFTL Instances
 *           |                            |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Preferences Data           |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Calibration Data           |       |
 *           |                            |       v
 *           +----------------------------+     -----

 *
 * </pre>
 * Note: The boot strapper and partition area are fixed in address and length
 * and cannot be changed.
 */

/** @addtogroup firmfl */
/*@{*/

/* Error codes */
#define FLPROG_OK		 0
#define FLPROG_ERR_MAGIC	-1
#define FLPROG_ERR_LENGTH	-2
#define FLPROG_ERR_CRC		-3
#define FLPROG_ERR_VER_NUM	-4
#define FLPROG_ERR_VER_DATE	-5
#define FLPROG_ERR_PROG_IMG	-6
#define FLPROG_ERR_PTB_GET	-7
#define FLPROG_ERR_PTB_SET	-8
#define FLPROG_ERR_FIRM_FILE	-9
#define FLPROG_ERR_FIRM_FLAG	-10
#define FLPROG_ERR_NO_MEM	-11
#define FLPROG_ERR_FIFO_OPEN	-12
#define FLPROG_ERR_FIFO_READ	-13
#define FLPROG_ERR_PAYLOAD	-14
#define FLPROG_ERR_ILLEGAL_HDR	-15
#define FLPROG_ERR_EXTRAS_MAGIC	-16
#define FLPROG_ERR_PREPROCESS	-17
#define FLPROG_ERR_POSTPROCESS	-18
#define FLPROG_ERR_META_SET	-19
#define FLPROG_ERR_META_GET	-20
#define FLPROG_ERR_NOT_READY	0x00001000

/* These are flags set on a firmware partition table entry */
#define PART_LOAD		0x0		/**< Load partition data */
#define PART_NO_LOAD		0x1		/**< Don't load part data */
#define PART_COMPRESSED		0x2		/**< Data is not compressed */
#define PART_READONLY		0x4		/**< Data is RO */
#define PART_INCLUDE_OOB	0x8		/**< Data include oob info*/
#define PART_ERASE_APARTS	0x10		/**< Erase Android partitions */
#define PART_SPARSE_EXT	0x20		/**< ext4 sparse partitions */

#define FLPART_MAGIC	0x8732dfe6
#define PARTHD_MAGIC	0xa324eb90

/* Limit parameter */
#define FIRMFL_FORCE		0x00	/**< No limit */
#define FIRMFL_VER_EQ		0x01	/**< Version is eqral */
#define FIRMFL_VER_GT		0x02	/**< Version is greater */
#define FIRMFL_DATE_EQ		0x04	/**< Date is equal */
#define FIRMFL_DATE_GT		0x08	/**< Date is greater */

#if (CHIP_REV == A1)  || (CHIP_REV == A2) || (CHIP_REV == A2S) || \
    (CHIP_REV == A2M) || (CHIP_REV == A2Q)
#define DSPFW_HEADER_BYTES	0x50
#else
#define DSPFW_HEADER_BYTES	0x50
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
    (CHIP_REV == A7L) || (CHIP_REV == S2) || (CHIP_REV == A8)
#define PART_BB_SKIP_START	PART_HAL
#else
#define PART_BB_SKIP_START	PART_BST
#endif

#define BOOT_DEV_NAND		0x1
#define BOOT_DEV_NOR		0x2
#define BOOT_DEV_SM		0x3
#define BOOT_DEV_ONENAND	0x4
#define BOOT_DEV_SNOR		0x5

#define FW_DEVICE {"DEFAULT", "NAND", "NOR", "SM", "ONENAND", "SNOR"}

#define FW_MODEL_NAME_SIZE	128


#if defined(ENABLE_SD) && ((FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO) || \
			   (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)   || \
			   (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2)  || \
			   (SM_STG2_SLOT == SCARDMGR_SLOT_SDIO)       || \
			   (SM_STG2_SLOT == SCARDMGR_SLOT_SD)         || \
			   (SM_STG2_SLOT == SCARDMGR_SLOT_SD2))
#define ENABLE_SM
#endif

#ifndef __ASM__

__BEGIN_C_PROTO__

extern const char *FLPROG_ERR_STR[];


#define DSP_VER_OFFSET		0x20
#define DSP_DATE_OFFSET		0x24

#if (CHIP_REV == A1)  || (CHIP_REV == A2)  || (CHIP_REV == A2S) || \
    (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
    (CHIP_REV == A5S) || (CHIP_REV == A5L)

/**
 * The header of DSP firmware stored in flash for ARCH_VERs 1 & 2.
 */
typedef __ARMCC_PACK__ struct dspfw_header_s {
	/** CODE segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} code;
	/** MEMD segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} memd;
	/** Data segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} data;
	u32	rsv[8];	/**< Padding */
} __ATTRIB_PACK__ dspfw_header_t;

#else

/**
 * The header of DSP firmware stored in flash for ARCH_VERs 3 and above.
 */
typedef __ARMCC_PACK__ struct dspfw_header_s {
	/** MAIN segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} main;
	/** SUB0 segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} sub0;
	/** SUB1 segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} sub1;
	/** DATA segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} data;
	/** AORC segment */
	__ARMCC_PACK__ struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} aorc;
} __ATTRIB_PACK__ dspfw_header_t;

#endif  /* (CHIP_REV == A1) || (CHIP_REV == A2)  || (CHIP_REV == A2S) || \
	   (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	   (CHIP_REV == A5S) || (CHIP_REV == A5L) */

/**
 * Flash partition entry. This structure describes a partition allocated
 * on the embedded flash device for either:
 * <pre>
 *	- bootloader
 *	- primary firmware image
 *	- backup firmware image
 *	- ramdisk image
 *	- romfs image
 *	- dsp image
 * </pre>
 * The bootloader, as well as the online flash programming utilities
 * rely on these information to:
 * <pre>
 *	- Validate address boundary, range
 *	- Guard against version conflicts
 *	- Load image to RAM area
 *	- Compute checksum
 * </pre>
 */
typedef __ARMCC_PACK__ struct flpart_s
{
	u32	crc32;		/**< CRC32 checksum of image */
	u32	ver_num;	/**< Version number */
	u32	ver_date;	/**< Version date */
	u32	img_len;	/**< Lengh of image in the partition */
	u32	mem_addr;	/**< Starting address to copy to RAM */
	u32	flag;		/**< Special properties of this partition */
	u32	magic;		/**< Magic number */
} __ATTRIB_PACK__ flpart_t;

#define FLDEV_CMD_LINE_SIZE	1024

/**
 * Properties of the network device
 */
typedef __ARMCC_PACK__ struct netdev_s
{
	/* This section contains networking related settings */
	u8	mac[6];		/**< MAC address*/
	u32	ip;		/**< Boot loader's LAN IP */
	u32	mask;		/**< Boot loader's LAN mask */
	u32	gw;		/**< Boot loader's LAN gateway */
} __ATTRIB_PACK__ netdev_t;


/**
 * Properties of the target device that is stored in the flash.
 */
typedef __ARMCC_PACK__ struct fldev_s
{
	char	sn[32];		/**< Serial number */
	u8	usbdl_mode;	/**< USB download mode */
	u8	auto_boot;	/**< Automatic boot */
	char	cmdline[FLDEV_CMD_LINE_SIZE];	/**< Boot command line options */
	u8	rsv[2];
	u32	splash_id;

	/* This section contains networking related settings */
	netdev_t eth[2];
	netdev_t wifi[2];
	netdev_t usb_eth[2];	/* add wifi and usb etho member */

	/* This section contains update by network  related settings */
	u8	auto_dl;	/**< Automatic download? */
	u32	tftpd;		/**< Boot loader's TFTP server */
	u32	pri_addr;	/**< RTOS download address */
	char	pri_file[32];	/**< RTOS file name */
	u8	pri_comp;	/**< RTOS compressed? */
	u32	rmd_addr;	/**< Ramdisk download address */
	char	rmd_file[32];	/**< Ramdisk file name */
	u8	rmd_comp;	/**< Ramdisk compressed? */
	u32	dsp_addr;	/**< DSP download address */
	char	dsp_file[32];	/**< DSP file name */
	u8	dsp_comp;	/**< DSP compressed? */
	u8	rsv2[2];

	u32	magic;		/**< Magic number */
} __ATTRIB_PACK__ fldev_t;

/**
 * The partition table is a region in flash where meta data about
 * different partitions are stored.
 */
#define PTB_SIZE		4096
#define PTB_PAD_SIZE		\
	(PTB_SIZE - PART_MAX_WITH_RSV * sizeof(flpart_t) - sizeof(fldev_t))
typedef __ARMCC_PACK__ struct flpart_table_s
{
	flpart_t	part[PART_MAX_WITH_RSV];/** Partitions */
	/* ------------------------------------------ */
	fldev_t		dev;			/**< Device properties */
	u8		rsv[PTB_PAD_SIZE];	/**< Padding to 2048 bytes */
} __ATTRIB_PACK__ flpart_table_t;

/**
 * The meta data table is a region in flash after partition table.
 * The data need by dual boot are stored.
 */
#define FL_MAIN_SZ_TYPE		4
#define PART_NAME_LEN		8
#define PTB_META_ACTURAL_LEN	((sizeof(u32) * 2 + PART_NAME_LEN + sizeof(u32)) * \
				 PART_MAX + sizeof(u32) + sizeof(u32) + \
				 FW_MODEL_NAME_SIZE + (sizeof(u32) * 4))
#define PTB_META_SIZE		2048
#define PTB_META_PAD_SIZE	(PTB_META_SIZE - PTB_META_ACTURAL_LEN)

typedef __ARMCC_PACK__ struct flpart_meta_s
{
	__ARMCC_PACK__ struct {
		u32	sblk;
		u32	nblk;
		char	name[PART_NAME_LEN];
	} part_info[PART_MAX];
	u32	magic;				/**< Magic number */
#ifdef ENABLE_EMMC_BOOT
#define PTB_META_MAGIC		0x33219fbd
#else
#define PTB_META_MAGIC		0x4432a0ce
#endif
	u32	part_dev[PART_MAX];
	u8	model_name[FW_MODEL_NAME_SIZE];
	__ARMCC_PACK__ struct {
		u32	sblk;
		u32	nblk;
	} sm_stg[2];
	u8 	rsv[PTB_META_PAD_SIZE];
	/* This meta crc32 doesn't include itself. */
	/* It's only calc data before this field.  */
	u32 	crc32;
} __ATTRIB_PACK__ flpart_meta_t;

/**
 * This is the header for a flash image partition.
 */
typedef __ARMCC_PACK__ struct partimg_header_s
{
	u32	crc32;		/**< CRC32 Checksum */
	u32	ver_num;	/**< Version number */
	u32	ver_date;	/**< Version date */
	u32	img_len;	/**< Image length */
	u32	mem_addr;	/**< Location to be loaded into memory */
	u32	flag;		/**< Flag of partition. */
	u32	magic;		/**< The magic number */
	u32	reserved[57];
} __ATTRIB_PACK__ partimg_header_t;

/**
 * The 'extras' structure is created to support run-time
 * change/upgrade/redefinition of the partitions in flash.
 */
typedef __ARMCC_PACK__ struct extras_s {
	u32	magic;		/**< Magic for the extras struct */
#define DEVFW_HEADER_EXTRA_MAGIC	0x33219fbd

	u32 part_size[PART_MAX_WITH_RSV];

	/**
	 * Media partition.
	 */
	__ARMCC_PACK__ struct {
		u32	zonet[MP_MAX_WITH_RSV];
		u32	rb_pzt[MP_MAX_WITH_RSV];
		u32	mrb_pz[MP_MAX_WITH_RSV];
		u32	trlb[MP_MAX_WITH_RSV];
	} mp_info;
} __ATTRIB_PACK__ extras_t;

/**
 * Header used in the device firmware payload.
 */
#define DEVFW_HEADER_SIZE		2048
typedef __ARMCC_PACK__ struct devfw_header_s {
	u32	begin_image[PART_MAX_WITH_RSV];	/**< Image start offset */
	u32	end_image[PART_MAX_WITH_RSV];	/**< Image end offset */

	extras_t extras;
	u8	model_name[FW_MODEL_NAME_SIZE];

#define DEVFW_HEADER_ACT_LEN	((sizeof(u32) * PART_MAX_WITH_RSV * 2) + \
				 sizeof(extras_t) + FW_MODEL_NAME_SIZE)
#define DEVFW_HEADER_PAD_SIZE	(DEVFW_HEADER_SIZE - DEVFW_HEADER_ACT_LEN)
	u8	rsv[DEVFW_HEADER_PAD_SIZE];
} __ATTRIB_PACK__ devfw_header_t;

typedef struct payload_len_s
{
	u32 len[HAS_IMG_PARTS];
} payload_len_t;

#if defined(NAND_SUPPORT_BBT)
/**
 * struct nand_bbt_descr - bad block table descriptor
 *
 * Descriptor for the bad block table marker and the descriptor for the
 * pattern which identifies good and bad blocks. The assumption is made
 * that the pattern and the version count are always located in the oob area
 * of the first block.
 */
typedef struct nand_bbt_descr_s {
	u32 options;
	int block;
	u8 offs;
	u8 veroffs;
	u8 version;
	u8 len;
	u8 maxblocks;
	u8 *pattern;
} nand_bbt_descr_t;

#define NAND_BBT_LASTBLOCK		0x00000010
#define NAND_BBT_VERSION		0x00000100

#define NAND_BBT_NO_OOB		0x00040000

#endif

#ifdef __BUILD_AMBOOT__

/*
 * The following data structure is used by the memfwprog program to output
 * the flash programming results to a memory area.
 */

#define FWPROG_RESULT_FLAG_LEN_MASK	0x00ffffff
#define FWPROG_RESULT_FLAG_CODE_MASK	0xff000000

#define FWPROG_RESULT_MAKE(code, len) \
	((code) << 24) | ((len) & FWPROG_RESULT_FLAG_LEN_MASK)

typedef struct fwprog_result_s
{
	u32	magic;
#define FWPROG_RESULT_MAGIC	0xb0329ac3

	u32	bad_blk_info;
#define BST_BAD_BLK_OVER	0x00000001
#define BLD_BAD_BLK_OVER	0x00000002
#define HAL_BAD_BLK_OVER	0x00000004
#define PBA_BAD_BLK_OVER	0x00000008
#define PRI_BAD_BLK_OVER	0x00000010
#define SEC_BAD_BLK_OVER	0x00000020
#define BAK_BAD_BLK_OVER	0x00000040
#define RMD_BAD_BLK_OVER	0x00000080
#define ROM_BAD_BLK_OVER	0x00000100
#define DSP_BAD_BLK_OVER	0x00000200

	u32	flag[HAS_IMG_PARTS];
} fwprog_result_t;

typedef struct fwprog_cmd_s
{
#define FWPROG_MAX_CMD		8
	u32	cmd[FWPROG_MAX_CMD];
	u32	error;
	u32	result_data;
	u32	result_str_len;
	u32	result_str_addr;
	u8	data[MEMFWPROG_HOOKCMD_SIZE - 12 * sizeof(u32)];
} fwprog_cmd_t;

#endif /* __BUILD_AMBOOT__ */

#ifdef __FLDRV_IMPL__

/* Private functions for implementors of NAND/NOR host controller */

/**
 * Compile-time info. of NAND.
 */
typedef struct flnand_s
{
	u32	control;		/**< Control parameter */
	u32	timing0;		/**< Timing param. */
	u32	timing1;		/**< Timing param. */
	u32	timing2;		/**< Timing param. */
	u32	timing3;		/**< Timing param. */
	u32	timing4;		/**< Timing param. */
	u32	timing5;		/**< Timing param. */
#if defined(ONENAND_NOR_SUPPORT)
	u32	timing6;		/**< Timing param. */
	u32	timing7;		/**< Timing param. */
	u32	timing8;		/**< Timing param. */
	u32	timing9;		/**< Timing param. */
#endif
	u32	nandtiming0;	/**<nand Timing param. */
	u32	nandtiming1;	/**<nand Timing param. */
	u32	nandtiming2;	/**<nand Timing param. */
	u32	nandtiming3;	/**<nand Timing param. */
	u32	nandtiming4;	/**<nand Timing param. */
	u32	nandtiming5;	/**<nand Timing param. */
	u32	banks;			/**< Total banks */
	u32	blocks_per_bank;	/**< blocks_per_bank */
	u32	main_size;		/**< Main size */
	u32	spare_size;		/**< Spare size */
	u32	pages_per_block;	/**< Pages per block */
	u32	block_size;		/**< Block size */
	u32	sblk[TOTAL_FW_PARTS]; 	/**< Starting block */
	u32	nblk[TOTAL_FW_PARTS]; 	/**< Number of blocks */

/* An flag to indicate if image contains data in spare area (oob).*/
/* The image contains yaffs file format should set this flag.  */
	u32	image_flag;		/**< Flag to indicate image type. */
#define IMAGE_HAS_NO_OOB_DATA	0x0
#define IMAGE_HAS_OOB_DATA	0x1
	u32	ecc_bits;		/**< ECC bits for new controller. */
} flnand_t;

extern flnand_t flnand;

/**
 * Compile-time info. of NOR.
 */
typedef struct flnor_s
{
	u32	control;		/**< Control parameter */
	u32	timing0;		/**< Timing param. */
	u32	timing1;		/**< Timing param. */
	u32	timing2;		/**< Timing param. */
	u32	timing3;		/**< Timing param. */
	u32	timing4;		/**< Timing param. */
	u32	timing5;		/**< Timing param. */
#if defined(ONENAND_NOR_SUPPORT)
	u32	timing6;		/**< Timing param. */
	u32	timing7;		/**< Timing param. */
	u32	timing8;		/**< Timing param. */
	u32	timing9;		/**< Timing param. */
	u32	chip_size;		/**< Chip size (Byte) */
	u32	block1_size;		/**< Sector size */
	u32	block1s_per_bank;	/**< block1s per bank */
	u32	total_block1s_size;	/**< total block1s size */
	u32	block2_size;		/**< Sector size */
	u32	block2s_per_bank;	/**< block2s per bank */
	u32	total_block2s_size;	/**< total block2s size */
	u32	blocks_per_bank;	/**< blocks per bank */
	u32	total_sectors;		/**< Total blocks */
	u32	bank_size;		/**< bank size (Byte) */
	u32	banks;			/**< banks per device */
#endif
	u32	sector_size;		/**< Sector size */
	u32	sblk[TOTAL_FW_PARTS]; 	/**< Starting block */
	u32	nblk[TOTAL_FW_PARTS]; 	/**< Number of blocks */
} flnor_t;

extern flnor_t flnor;

#endif

__END_C_PROTO__

#endif  /* !__ASM__ */

/*@}*/

#endif
