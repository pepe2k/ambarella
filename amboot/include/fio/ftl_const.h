/**
 * @file system/include/fio/ftl_const.h
 *
 * History:
 *    2005/11/03 - [Chien Yang Chen] created file
 *    2006/05/08 - [Charles Chiou] moved to system/include/fio
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FTL_CONST_H__
#define __FTL_CONST_H__

#include <basedef.h>

/* ------------------------------------------------------------------------- */
/* nand device information                                                   */
/* ------------------------------------------------------------------------- */
#define NAND_MAIN_512		512	/* main size 512 byte */
#define NAND_MAIN_2K		2048	/* main size 2k byte */
#define NAND_SPARE_16		16	/* spare size 16 byte */
#define NAND_SPARE_64		64	/* spare size 64 byte */
#define NAND_PPB_32		32	/* 32 pages per block */
#define NAND_PPB_64		64	/* 64 pages per block */
#define NAND_SPP_4		4	/* 4 sectors per page */
#define NAND_SPP_1		1	/* 1 sector per page */
#define NAND_512_SPB_32		32	/* 32 sectors per block */
#define NAND_2K_SPB_256		256	/* 256 sectors per block */

#define NAND_MAIN_512_SHF	9	/* main size 512 byte */
#define NAND_MAIN_2K_SHF	11	/* main size 2k byte */
#define NAND_SPARE_16_SHF	4	/* spare size 16 byte */
#define NAND_SPARE_32_SHF	5	/* spare size 32 byte */
#define NAND_SPARE_64_SHF	6	/* spare size 64 byte */
#define NAND_PPB_32_SHF		5	/* 32 pages per block */
#define NAND_PPB_64_SHF		6	/* 64 pages per block */
#define NAND_SPP_4_SHF		2	/* 4 sectors per page */
#define NAND_512_SPB_32_SHF	5	/* 32 sectors per block */
#define NAND_2K_SPB_256_SHF	8	/* 256 sectors per block */

#define NSPARE_SIZE		16
#define NSPARE_SHIFT		4

/**
 * Use to indicate the nand is 512 byte page or 2k byte page nand.
 */
#define NAND_TYPE_NONE		0
#define NAND_TYPE_512		1
#define NAND_TYPE_2K		2

/*************************/
/* Bad block information */
/*************************/

/* Pages to check if this block is a bad block. */
/* Initial bad block : page 2&3 are good while page 0|1 are bad. */
/* Late developed bad block : Page 2|3 and page 0|1 are bad. */
#define INIT_BAD_BLOCK_PAGES	2
#define LATE_BAD_BLOCK_PAGES	2
#define BAD_BLOCK_PAGES		(INIT_BAD_BLOCK_PAGES + LATE_BAD_BLOCK_PAGES)
#define VENDOR_BB_START_PAGE	0
#define AMB_BB_START_PAGE	0

/* Ambarella bad block marker. This is used to mark the late developed bad block
 * by Ambarella nand software. */
#define AMB_BAD_BLOCK_MARKER		0x0

#define NAND_GOOD_BLOCK			0x0
#define NAND_INITIAL_BAD_BLOCK		0x1
#define NAND_LATE_DEVEL_BAD_BLOCK	0x2
#define NAND_OTHER_BAD_BLOCK		0x4

#ifndef NAND_ALL_BAD_BLOCK_FLAGS
#define NAND_ALL_BAD_BLOCK_FLAGS	(NAND_INITIAL_BAD_BLOCK | \
					 NAND_LATE_DEVEL_BAD_BLOCK | \
					 NAND_OTHER_BAD_BLOCK)
#endif

/* The flag is used for firmfl. */
#ifndef NAND_FW_BAD_BLOCK_FLAGS
#define NAND_FW_BAD_BLOCK_FLAGS		(NAND_INITIAL_BAD_BLOCK | \
					 NAND_OTHER_BAD_BLOCK)
#endif

/* The flag is used for nftl. */
#ifndef NAND_FTL_BAD_BLOCK_FLAGS
#define NAND_FTL_BAD_BLOCK_FLAGS	(NAND_INITIAL_BAD_BLOCK | \
					 NAND_OTHER_BAD_BLOCK)
#endif

/* ------------------------------------------------------------------------- */
/* xd device information                                                     */
/* ------------------------------------------------------------------------- */
#define XD_MAIN_512		512	/* main size 512 byte */
#define XD_MAIN_2K		2048	/* main size 2k byte */
#define XD_SPARE_16		16	/* spare size 16 byte */
#define XD_SPARE_64		64	/* spare size 64 byte */
#define XD_PPB_32		32	/* 32 pages per block */
#define XD_PPB_64		64	/* 64 pages per block */
#define XD_SPP_4		4	/* 4 sectors per page */
#define XD_SPP_1		1	/* 1 sector per page */
#define XD_512_SPB_32		32	/* 32 sectors per block */
#define XD_2K_SPB_256		256	/* 256 sectors per block */

#define XD_MAIN_512_SHF		9	/* main size 512 byte */
#define XD_MAIN_2K_SHF		11	/* main size 2k byte */
#define XD_SPARE_16_SHF		4	/* spare size 16 byte */
#define XD_SPARE_64_SHF		6	/* spare size 64 byte */
#define XD_PPB_32_SHF		5	/* 32 pages per block */
#define XD_PPB_64_SHF		6	/* 64 pages per block */
#define XD_SPP_4_SHF		2	/* 4 sectors per page */
#define XD_512_SPB_32_SHF	5	/* 32 sectors per block */
#define XD_2K_SPB_256_SHF	8	/* 256 sectors per block */

#define XDSPARE_SIZE		16
#define XDPARE_SHIFT		4
/**
 * Use to indicate the nand is 512 byte page or 2k byte page nand.
 */
#define XD_TYPE_NONE		0
#define XD_TYPE_512		1
#define XD_TYPE_2K		2

/* ------------------------------------------------------------------------- */
/* ftl information                                                           */
/* ------------------------------------------------------------------------- */
#define FTL_UNUSED_BLK		0xffff

#define FTL_FROM_DEV		0
#define FTL_FROM_BUF		1
#define FTL_FROM_BBINFO		2
#define FTL_FROM_BBINFO_ICHUNK	3

#endif