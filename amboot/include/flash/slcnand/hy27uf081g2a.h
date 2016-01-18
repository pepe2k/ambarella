/**
 * @file system/include/flash/slcnand/hy27uf081g2a.h
 *
 * History:
 *    2007/12/26 - [Louvre Tseng] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __HY27UF081G2A_H__
#define __HY27UF081G2A_H__

#define __NAND_CONTROL						  \
	(NAND_CTR_C2		|				  \
	 NAND_CTR_RC		|				  \
	 NAND_CTR_I4		|				  \
	 NAND_CTR_CC		|				  \
	 NAND_CTR_IE		|				  \
	 NAND_CTR_SZ_1G		|				  \
	 NAND_CTR_WD_8BIT)

#define NAND_MANID		0xad
#define NAND_DEVID		0xf1
#define NAND_ID3		0x80
#define NAND_ID4		0x1d

/**
 * define for device info
 */
#define NAND_MAIN_SIZE		2048
#define NAND_SPARE_SIZE		64
#define NAND_PAGE_SIZE		2112
#define NAND_PAGES_PER_BLOCK	64
#define NAND_BLOCKS_PER_PLANE	1024
#define NAND_BLOCKS_PER_ZONE	1024
#define NAND_BLOCKS_PER_BANK	1024
#define NAND_PLANES_PER_BANK	(NAND_BLOCKS_PER_BANK / NAND_BLOCKS_PER_PLANE)
#define NAND_BANKS_PER_DEVICE	1
#define NAND_TOTAL_BLOCKS	(NAND_BLOCKS_PER_BANK * NAND_BANKS_PER_DEVICE)
#define NAND_TOTAL_ZONES	(NAND_TOTAL_BLOCKS / NAND_BLOCKS_PER_ZONE)
#define NAND_TOTAL_PLANES	(NAND_TOTAL_BLOCKS / NAND_BLOCKS_PER_PLANE)

/* Copyback must be in the same plane, so we have to know the plane address */
#define NAND_BLOCK_ADDR_BIT	18
/* Devcie has only one plane and the plane address should be set */
/* outside of device. */
#define NAND_PLANE_ADDR_BIT	28
#define NAND_PLANE_MASK		0x1

/* Used to mask the plane address according to block address in the same bank */
#define NAND_PLANE_ADDR_MASK	(NAND_PLANE_MASK << (NAND_PLANE_ADDR_BIT - \
						     NAND_BLOCK_ADDR_BIT))

#define NAND_PLANE_MAP		NAND_PLANE_MAP_2
#define NAND_COLUMN_CYCLES	2
#define NAND_PAGE_CYCLES	2
#define NAND_ID_CYCLES		4
#define NAND_CHIP_WIDTH		8
#define NAND_CHIP_SIZE_MB	128
#define NAND_BUS_WIDTH		8

#define NAND_NAME	"HYNIX HY27UF081G2A_128MB_PG2K"

#if defined(CONFIG_NAND_1DEVICE)
#define NAND_DEVICES		1
#elif defined(CONFIG_NAND_2DEVICE)
#define NAND_DEVICES		2
#elif defined(CONFIG_NAND_4DEVICE)
#define NAND_DEVICES		4
#endif

#define NAND_TOTAL_BANKS	(NAND_DEVICES * NAND_BANKS_PER_DEVICE)

#if (NAND_TOTAL_BANKS == 1)
#define NAND_CONTROL		(__NAND_CONTROL | NAND_CTR_1BANK)
#elif (NAND_TOTAL_BANKS == 2)
#define NAND_CONTROL		(__NAND_CONTROL | NAND_CTR_2BANK)
#elif (NAND_TOTAL_BANKS == 4)
#define NAND_CONTROL		(__NAND_CONTROL | NAND_CTR_4BANK)
#elif (NAND_TOTAL_BANKS > 4)
#error Unsupport nand flash banks
#endif

#define NAND_BB_MARKER_OFFSET	0	/* bad block information */

/**
 * define for partition info
 */
#define NAND_RSV_BLKS_PER_ZONE	24	

/**
 * timing parameter in ns
 */
#define NAND_TCLS		15
#define NAND_TALS		15
#define NAND_TCS		20
#define NAND_TDS		5
#define NAND_TCLH		5
#define NAND_TALH		5
#define NAND_TCH		5
#define NAND_TDH		5
#define NAND_TWP		15
#define NAND_TWH		10
#define NAND_TWB		100
#define NAND_TRR		20
#define NAND_TRP		15
#define NAND_TREH		10
#define NAND_TRB		100	/* ? */
#define NAND_TCEH		30	/* ? */
#define NAND_TRDELAY		20	/* trea ? */
#define NAND_TCLR		15
#define NAND_TWHR		60
#define NAND_TIR		0
#define NAND_TWW		100	/* not define */
#define NAND_TRHZ		50	/* use max value */
#define NAND_TAR		15

#endif

