/**
 * @file system/include/flash/onenand/kfg5616u1a.h
 *
 * History:
 *    2009/08/17 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __KFG5616U1A_H__
#define __KFG5616U1A_H__

/**
 * onenand control register initial setting
 */
#define ONENAND_CONTROL1					\
	(ONENAND_CTR1_IE		|			\
	 ONENAND_CTR1_LATEN_OFFSET(6)   |			\
	 ONENAND_CTR1_TYPE_DEMUXED	|			\
	 ONENAND_CTR1_BLK_64_PAGE	|			\
	 ONENAND_CTR1_TYPE_SDP		|			\
	 ONENAND_CTR1_PAGE_SZ_1K	|			\
	 ONENAND_CTR1_AVD_TYPE1		|			\
	 ONENAND_CTR1_FLAG_ONENAND	|			\
	 ONENAND_CTR1_LEN_VALID_ADDR(25))

#define ONENAND_CONTROL2		0x0

#define ONENAND_MANID			0xec
#define ONENAND_DEVID			0x15
#define ONENAND_VERID			0x00
#define ONENAND_ID4			0x00

#define ONENAND_ID_LEN			2
#define	ONENAND_DEV_ID_LEN		1

/**
 * define for device info
 */
#define ONENAND_MAIN_SIZE		1024
#define ONENAND_SPARE_SIZE		32
#define ONENAND_PAGE_SIZE		1056
#define ONENAND_PAGES_PER_BLOCK		64
#define ONENAND_BLOCKS_PER_PLANE	512
#define ONENAND_BLOCKS_PER_ZONE		512
#define ONENAND_BLOCKS_PER_BANK		512
#define ONENAND_PLANES_PER_BANK	(ONENAND_BLOCKS_PER_BANK / ONENAND_BLOCKS_PER_PLANE)
#define ONENAND_BANKS_PER_DEVICE	1
#define ONENAND_TOTAL_BLOCKS	(ONENAND_BLOCKS_PER_BANK * ONENAND_BANKS_PER_DEVICE)
#define ONENAND_TOTAL_ZONES	(ONENAND_TOTAL_BLOCKS / ONENAND_BLOCKS_PER_ZONE)
#define ONENAND_TOTAL_PLANES	(ONENAND_TOTAL_BLOCKS / ONENAND_BLOCKS_PER_PLANE)

/* Copyback must be in the same plane, so we have to know the plane address */
#define ONENAND_BLOCK_ADDR_BIT		18
#define ONENAND_PLANE_ADDR_BIT		28
#define ONENAND_PLANE_MASK		0x1

/* Used to mask the plane address according to block address in the same bank */
#define ONENAND_PLANE_ADDR_MASK	(ONENAND_PLANE_MASK << 		\
				 (ONENAND_PLANE_ADDR_BIT - 	\
				 ONENAND_BLOCK_ADDR_BIT))

#define ONENAND_PLANE_MAP	NAND_PLANE_MAP_0
#define ONENAND_BUS_WIDTH	8
#define ONENAND_CHIP_SIZE_MB	32

#define ONENAND_NAME		"SAMSUNG OneNAND256(KFG5616u1A) 32MB"

#if defined(CONFIG_ONENAND_1DEVICE)
#define ONENAND_DEVICES		1
#elif defined(CONFIG_ONENAND_2DEVICE)
#define ONENAND_DEVICES		2
#elif defined(CONFIG_ONENAND_4DEVICE)
#define ONENAND_DEVICES		4
#else
#define ONENAND_DEVICES		1
#endif

#define ONENAND_TOTAL_BANKS	(ONENAND_DEVICES * ONENAND_BANKS_PER_DEVICE)

/**
 * Support Prog/Read mode
 */
#define ONENAND_READ_MODE	ONENAND_SUPPORT_SYNC_MODE
#define ONENAND_PROG_MODE	ONENAND_SUPPORT_ASYNC_MODE

/**
 * timing parameter in ns
 */

/* TIM0 */
#define ONENAND_TAA		76
#define ONENAND_TOE		20
#define ONENAND_TOEH		0
#define ONENAND_TCE		76
/* TIM1 */
#define ONENAND_TPA		0	/* don't care */
#define ONENAND_TRP		200
#define ONENAND_TRH		10	/* use min */
#define ONENAND_TOES		0
/* TIM2 */
#define ONENAND_TCS		0
#define ONENAND_TCH		10
#define ONENAND_TWP		40
#define ONENAND_TWH		30
/* TIM3 */
#define ONENAND_TRB		0	/* NOR flash */
#define ONENAND_TAHT		0	/* don't care */
#define ONENAND_TASO		0	/* don't care */
#define ONENAND_TOEPH		0	/* don't care */
/* TIM4 */
#define ONENAND_TAVDCS		0	/* NOR flash */
#define ONENAND_TAVDCH		0	/* NOR flash */
#define ONENAND_TAS		0
#define ONENAND_TAH		30
/* TIM5 */
#define ONENAND_TELF		0	/* don't care */
#define ONENAND_TDS		12
#define ONENAND_TDH		0
#define ONENAND_TBUSY		0	/* NOR flash */
/* TIM6 */
#define ONENAND_TDP		0
#define ONENAND_TCEPH		0
#define ONENAND_TOEZ		15
#define ONENAND_TBA		12
/* TIM7 */
#define ONENAND_TREADY		0	/* NOR flash */
#define ONENAND_TACH		6
#define ONENAND_TCEHP		0	/* NOR flash */
/* TIM8 */
#define ONENAND_TAVDS		5
#define ONENAND_TOH		0	/* NOR flash */
#define ONENAND_TAVDP		12
#define ONENAND_TAAVDH		7
/* TIM9 */
#define ONENAND_TIAA		70
#define ONENAND_TCLK_PERIOD	15
#define ONENAND_TWEA		15
#define ONENAND_TACS		5


#endif
