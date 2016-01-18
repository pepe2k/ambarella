/**
 * @file system/include/flash/inor/e28f320.h
 *
 * History:
 *    2006/03/31 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __28F320J3A_H__
#define __28F320J3A_H__

#define NOR_NAME	"intel StrataFlash Memory 28F320J3A"

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
#define NOR_TAS			55
#define NOR_TCS			0
#define NOR_TDS			50
#define NOR_TAH			0
#define NOR_TCH			10
#define NOR_TDH			0
#define NOR_TWP			70	/* write pulse width */
#define NOR_TWH			30
#define NOR_TRC			110	/* read cycle time */
#define NOR_TADELAY		110
#define NOR_TCDELAY		110
#define NOR_TRPDELAY		150
#define NOR_TRDELAY		50
#define NOR_TABDELAY		10	/* NOT IMPLEMENT */
#define NOR_TWHR		0	/* NOT IMPLEMENT */
#define NOR_TIR			25	/* NOT IMPLEMENT */
#define NOR_TRHZ		25	/* NOT IMPLEMENT */
#define NOR_TRPH		25	/* NOT IMPLEMENT */

#endif
