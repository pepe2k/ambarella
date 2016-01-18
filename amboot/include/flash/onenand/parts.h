/**
 * @file system/include/flash/onenand/parts.h
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

#ifndef __ONENAND_PARTS_H__
#define __ONENAND_PARTS_H__

#if   defined(CONFIG_ONENAND_KFG5616U1A)
#include <flash/onenand/kfg5616u1a.h>
#endif

#if defined(CONFIG_ONENAND_NONE)
/******************************************/
/* Physical NAND flash device information */
/******************************************/

#define ONENAND_CONTROL1		0
#define ONENAND_CONTROL2		0
#define ONENAND_MANID              	0
#define ONENAND_DEVID              	0
#define ONENAND_VERID                	0
#define ONENAND_ID4                	0
#define ONENAND_ID			0

#define ONENAND_MAIN_SIZE		0
#define ONENAND_SPARE_SIZE		0
#define ONENAND_PAGE_SIZE		0
#define ONENAND_PAGES_PER_BLOCK		0
#define ONENAND_BLOCKS_PER_PLANE	0
#define ONENAND_BLOCKS_PER_ZONE		0
#define ONENAND_BLOCKS_PER_BANK		0
#define ONENAND_PLANES_PER_BANK		0
#define ONENAND_BANKS_PER_DEVICE	0
#define ONENAND_TOTAL_BLOCKS		0
#define ONENAND_TOTAL_ZONES		0
#define ONENAND_TOTAL_PLANES		0
#define ONENAND_BLOCK_ADDR_BIT		0
#define ONENAND_PLANE_ADDR_BIT		0
#define ONENAND_PLANE_MASK		0
#define ONENAND_PLANE_ADDR_MASK		0
#define ONENAND_PLANE_MAP		0
#define ONENAND_COLUMN_CYCLES		0
#define ONENAND_PAGE_CYCLES		0
#define ONENAND_ID_CYCLES		0
#define ONENAND_CHIP_WIDTH		0
#define ONENAND_CHIP_SIZE_MB		0
#define ONENAND_BUS_WIDTH		0
#define ONENAND_DEVICES			0
#define ONENAND_NAME		"ONENAND_NONE"

#define ONENAND_TOTAL_BANKS		0
#define ONENAND_BB_MARKER_OFFSET	0
#define ONENAND_RSV_BLKS_PER_ZONE  	0

/* TIM0 */
#define ONENAND_TAA			0
#define ONENAND_TOE			0
#define ONENAND_TOEH			0
#define ONENAND_TCE			0
/* TIM1 */
#define ONENAND_TPA			0	/* don't care */
#define ONENAND_TRP			0
#define ONENAND_TRH			0	/* use min */
#define ONENAND_TOES			0
/* TIM2 */
#define ONENAND_TCS			0
#define ONENAND_TCH			0
#define ONENAND_TWP			0
#define ONENAND_TWH			0
/* TIM3 */
#define ONENAND_TRB			0	/* NOR flash */
#define ONENAND_TAHT			0	/* don't care */
#define ONENAND_TASO			0	/* don't care */
#define ONENAND_TOEPH			0	/* don't care */
/* TIM4 */
#define ONENAND_TAVDCS			0	/* NOR flash */
#define ONENAND_TAVDCH			0	/* NOR flash */
#define ONENAND_TAS			0
#define ONENAND_TAH			0
/* TIM5 */
#define ONENAND_TELF			0	/* don't care */
#define ONENAND_TDS			0
#define ONENAND_TDH			0
#define ONENAND_TBUSY			0	/* NOR flash */
/* TIM6 */
#define ONENAND_TDP			0
#define ONENAND_TCEPH			0
#define ONENAND_TOEZ			0
#define ONENAND_TBA			0
/* TIM7 */
#define ONENAND_TREADY			0	/* NOR flash */
#define ONENAND_TACH			0
#define ONENAND_TCEHP			0	/* NOR flash */
/* TIM8 */
#define ONENAND_TAVDS			0
#define ONENAND_TOH			0	/* NOR flash */
#define ONENAND_TAVDP			0
#define ONENAND_TAAVDH			0
/* TIM9 */
#define ONENAND_TIAA			0
#define ONENAND_TCLK_PERIOD		0
#define ONENAND_TWEA			0
#define ONENAND_TACS			0

#define ONENAND_RSV_BLKS_PER_ZONE	0

/*****************************************/
/* Logical NAND flash device information */
/*****************************************/

#undef ONENAND_NUM_INTLVE
#define ONENAND_NUM_INTLVE		0

#define ONENAND_LMAIN_SIZE		0
#define ONENAND_LSPARE_SIZE		0
#define ONENAND_LPAGE_SIZE		0
#define ONENAND_LPAGES_PER_BLOCK	0
#define ONENAND_LBLOCKS_PER_PLANE	0
#define ONENAND_LBLOCKS_PER_ZONE	0
#define ONENAND_LBLOCKS_PER_BANK	0
#define ONENAND_LPLANES_PER_BANK	0
#define ONENAND_LBANKS_PER_DEVICE	0
#define ONENAND_LTOTAL_BLOCKS		0
#define ONENAND_LTOTAL_ZONES		0
#define ONENAND_LTOTAL_PLANES		0
#define ONENAND_LTOTAL_BANKS		0
#define ONENAND_LDEVICES		0

#else /* CONFIG_ONENAND_NONE */
#define ONENAND_ID		((unsigned long)			   \
			 (ONENAND_MANID << 24) | (ONENAND_DEVID << 16) | \
			 (ONENAND_VERID << 8) | ONENAND_ID4)

/*****************************************/
/* Logical NAND flash device information */
/*****************************************/

#ifndef ONENAND_NUM_INTLVE
#undef ONENAND_NUM_INTLVE
#define ONENAND_NUM_INTLVE		1
#endif

#if (ONENAND_NUM_INTLVE == 1) || (ONENAND_NUM_INTLVE == 2) || \
    (ONENAND_NUM_INTLVE == 4)
/* Logical device info(logical bank info) */
/* Page characteristic is "ONENAND_NUM_INTLVE" times of original */
#define ONENAND_LMAIN_SIZE		(ONENAND_MAIN_SIZE * ONENAND_NUM_INTLVE)
#define ONENAND_LSPARE_SIZE		(ONENAND_SPARE_SIZE * ONENAND_NUM_INTLVE)
#define ONENAND_LPAGE_SIZE		(ONENAND_PAGE_SIZE * ONENAND_NUM_INTLVE)

#define ONENAND_LPAGES_PER_BLOCK	ONENAND_PAGES_PER_BLOCK
#define ONENAND_LBLOCKS_PER_PLANE	ONENAND_BLOCKS_PER_PLANE
#define ONENAND_LBLOCKS_PER_ZONE	ONENAND_BLOCKS_PER_ZONE
#define ONENAND_LBLOCKS_PER_BANK	ONENAND_BLOCKS_PER_BANK
#define ONENAND_LPLANES_PER_BANK	ONENAND_PLANES_PER_BANK

#if (ONENAND_NUM_INTLVE == 1)
#define ONENAND_LBANKS_PER_DEVICE	ONENAND_BANKS_PER_DEVICE
#else
#define ONENAND_LBANKS_PER_DEVICE	1
#endif

#define ONENAND_LTOTAL_BLOCKS		(ONENAND_LBLOCKS_PER_BANK * ONENAND_LBANKS_PER_DEVICE)
#define ONENAND_LTOTAL_ZONES		(ONENAND_LTOTAL_BLOCKS / ONENAND_LBLOCKS_PER_ZONE)
#define ONENAND_LTOTAL_PLANES		(ONENAND_LTOTAL_BLOCKS / ONENAND_LBLOCKS_PER_PLANE)

/* Information for all devices considerd*/
#define ONENAND_LTOTAL_BANKS		(ONENAND_TOTAL_BANKS / ONENAND_NUM_INTLVE)

#if (ONENAND_NUM_INTLVE == 1)
#define ONENAND_LDEVICES		ONENAND_DEVICES
#else
#define ONENAND_LDEVICES		ONENAND_LTOTAL_BANKS
#endif

#if defined(ENABLE_FLASH)
#if (ONENAND_LTOTAL_BANKS == 0) || (ONENAND_TOTAL_BANKS % ONENAND_NUM_INTLVE)
#error Unsupport nand logical device information
#endif
#endif

#else
#error Unsupport nand flash interleave banks
#endif

#endif /* CONFIG_ONENAND_NONE */

#endif
