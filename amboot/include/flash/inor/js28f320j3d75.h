/**
 * @file system/include/flash/inor/js28f320j3d75.h
 *
 * History:
 *    2008/03/24 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __JS28F320J3D75_H__
#define __JS28F320J3D75_H__

#define NOR_NAME	"Intel Embedded Flash Memory JS28F320J3D75"

/**
 * nand control register initial setting
 */
#define __NOR_CONTROL				\
	(NOR_CTR_BZ_8KB		|		\
	 NOR_CTR_KZ_128KB	|		\
	 NOR_CTR_IE		|		\
	 NOR_CTR_SZ_32M		|		\
	 NOR_CTR_WD_8BIT)

/**
 * define for device info
 */
#define NOR_SECTOR_SIZE		(128 * 1024)
#define NOR_CHIP_SIZE		((64 * 1024) >> 8)
#define NOR_TOTAL_SECTORS	(NOR_CHIP_SIZE / NOR_SECTOR_SIZE)
#define NOR_BANKS_PER_DEVICE	1

#if defined(CONFIG_NOR_1DEVICE)
#define NOR_DEVICES		1
#elif defined(CONFIG_NOR_2DEVICE)
#define NOR_DEVICES		2
#elif defined(CONFIG_NOR_4DEVICE)
#define NOR_DEVICES		4
#endif

#define NOR_TOTAL_BANKS		(NOR_DEVICES * NOR_BANKS_PER_DEVICE)

#if (NOR_TOTAL_BANKS == 1)
#define NOR_CONTROL		(__NOR_CONTROL | NOR_CTR_1BANK)
#elif (NOR_TOTAL_BANKS == 2)
#define NOR_CONTROL		(__NOR_CONTROL | NOR_CTR_2BANK)
#elif (NOR_TOTAL_BANKS == 4)
#define NOR_CONTROL		(__NOR_CONTROL | NOR_CTR_4BANK)
#elif (NOR_TOTAL_BANKS > 4)
#error Unsupport nor flash banks
#endif

/**
 * timing parameter in ns
 */

/* write operation */

/* address setup to write pulse rising edge time */
#define NOR_TAS			55	/* Tavwh */
/* chip enable setup to write pulse falling edge time */
#define NOR_TCS			0	/* Telwl */
/* data setup to write pulse rising edge time */
#define NOR_TDS			50	/* Tdvwh */

/* address hold from write pulse rising edge time */
#define NOR_TAH			0
/* chip enable hold from write pulse rising edge time */
#define NOR_TCH			0
/* data hold from write pulse rising edge time */
#define NOR_TDH			0

#define NOR_TWP			60	/* write pulse width */
#define NOR_TWH			30	/* write high hold time */

/* read operation */

#define NOR_TRC			75	/* read cycle time */
/* address to data valid time including all C to Q delays, transit time delays,
   buffer delays, etc */
#define NOR_TADELAY		75
/* chip enable to data valid time including all C to Q delays, transit time
   delays, buffer delays, etc  */
#define NOR_TCDELAY		75
/* reset to data valid time including all C to Q delays, transit time delays,
   buffer delays, etc  */
#define NOR_TRPDELAY		150

/* output enable falling edge to data valid time including all C to Q delays,
   transit time delays, buffer delays, etc */
#define NOR_TRDELAY		0
/* burst read address low to data valid time including all C to Q delays,
   transit time delays, buffer delays, etc */
#define NOR_TABDELAY		0
/* write pulse rising edge to output enable falling edge time */
#define NOR_TWHR		0
/* flash chip output high Z to output enable falling edge time */
#define NOR_TIR			50

/* output enable rising edge to flash chip output high Z time */
#define NOR_TRHZ		25
/* reset to write pulse falling edge time */
#define NOR_TRPH		25

#endif
